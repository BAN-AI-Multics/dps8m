/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 4d1552ca-f62b-11ec-9d14-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2014-2016 Charles Anthony
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE
#endif

#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>           /* For O_* constants */
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#if defined(__APPLE__)
# include <xlocale.h>
#endif
#include <locale.h>
#include <gtk/gtk.h>

#define API
#include "dps8_state.h"

static struct system_state_s * system_state;

typedef uint8_t byte;
typedef uint8_t * bytep;
static byte * ss_p, * cpu0_p, * cpun, * PPR_p, * cu_p, * PAR_p, * rX0_p, * TPR_p, * DSBR_p;
static uint8_t * PRR_p, * P_p, * rE_p, * rRALR_p, * RNR_p[8], * PR_BITNO_p[8], * TRR_p, * TBR_p, * U_p;
static uint16_t * PSR_p, * SNR_p[8], * TSR_p, * BND_p, * STACK_p;
static uint32_t * IC_p, * IR_p, * rTR_p, * rX_p[8], * WORDNO_p[8], * CA_p, * ADDR_p, * faultNumber_p;
static uint64_t * IWB_p, * rA_p, * rQ_p;
static size_t sizeof_cpu, sizeof_rX, sizeof_PAR;

static uint32_t FAULT_SDF, FAULT_STR, FAULT_MME, FAULT_F1, FAULT_TRO, FAULT_CMD, FAULT_DRL, FAULT_LUF, FAULT_CON, FAULT_PAR,
  FAULT_IPR, FAULT_ONC, FAULT_SUF, FAULT_OFL, FAULT_DIV, FAULT_EXF, FAULT_DF0, FAULT_DF1, FAULT_DF2, FAULT_DF3, FAULT_ACV,
  FAULT_MME2, FAULT_MME3, FAULT_MME4, FAULT_F2, FAULT_F3, FAULT_TRB;

static struct {
  uint8_t PRR, P, rE, rRALR, RNR[8], PR_BITNO[8], TRR, TBR, U;
  uint16_t PSR, SNR[8], BASE, BOUND, TSR, BND, STACK;
  uint32_t IC, rX[8], IR, rTR, WORDNO[8], CA, ADDR, faultNumber;
  uint64_t IWB, rA, rQ;
  } previous;

static uint32_t lookup (struct system_state_s * p, uint32_t stype, char * name,
                        uint32_t * symbolType, uint32_t * valueType, uint32_t * value) {
  struct symbol_s * s;
  for (int i = 0; s = p->symbolTable.symbols + i, s->symbolType != SYM_EMPTY; i ++) {
    if (strcmp (name, s->name) == 0 &&  (stype ==SYM_UNDEF || stype == s->symbolType)) {
      * symbolType = s->symbolType;
      * valueType = s->valueType;
      * value = s->value;
      return * value;
    }
  }
  (void)fprintf(stderr, "\rFATAL: Lookup of '%s' failed, aborting %s[%s:%d]\r\n",
                name, __func__, __FILE__, __LINE__);
  exit (1);
  /*NOTREACHED*/ /* unreachable */
  return 1;
}

static GdkRGBA lightOn, lightOff;

gboolean window_delete (GtkWidget * widget, cairo_t * cr, gpointer data) {
  (void)widget;
  (void)cr;
  (void)data;
  //return true;
  exit (0);
  /*NOTREACHED*/ /* unreachable */
  return 1;
}

gboolean draw_callback (GtkWidget * widget, cairo_t * cr, gpointer data) {
  guint width, height;

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);
  cairo_arc (cr,
             width / 2.0, height / 2.0,
             MIN (width, height) / 2.0 * 0.8,
             0, 2 * G_PI);
  //gtk_style_context_get_color (gtk_widget_get_style_context (widget),
                                 //0,
                                 //& color);
  gdk_cairo_set_source_rgba (cr, & lightOff);
  cairo_fill (cr);

  if ( * (bool *) data) {
    cairo_arc (cr,
               width / 2.0, height / 2.0,
               MIN (width, height) / 2.0 * 0.6,
               0, 2 * G_PI);
    gdk_cairo_set_source_rgba (cr, & lightOn);
    cairo_fill (cr);
  }

  return FALSE;
}

static GtkWidget * createLight (bool * state) {
  GtkWidget * drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (drawing_area, 10, 10);
  g_signal_connect (G_OBJECT (drawing_area), "draw",
                    G_CALLBACK (draw_callback), state);
  return drawing_area;
}

static GtkWidget * createLightArray (int n, bool state []) {
  GtkWidget * grid = gtk_grid_new (); // (1, n, TRUE);
  for (int i = 0; i < n; i ++) {
    GtkWidget * l = createLight (& state [i]);
    gtk_grid_attach (GTK_GRID (grid), l, i, 0, 1, 1);
  }
  return grid;
}

static bool PRR_state [3];
static bool PSR_state [15];
static bool P_state [1];
static bool IC_state [18];
static bool A_state [36];
static bool Q_state [36];
static bool inst_state [36];

static bool E_state [8];
static bool X_state [8][18];
static bool IR_state [18];
static bool TR_state [27];
static bool RALR_state [3];

static bool SNR_state [8][15];
static bool RNR_state [8][3];
static bool BITNO_state [8][6];
static bool WORDNO_state [8][18];

static bool TRR_state [3];
static bool TSR_state [15];
static bool TBR_state [6];
static bool CA_state [18];

//static bool BASE_state [9];
//static bool BOUND_state [9];

static bool ADDR_state [24];
static bool BND_state [14];
static bool U_state [1];
static bool STACK_state [12];

static bool FAULT_SDF_state;
static bool FAULT_STR_state;
static bool FAULT_MME_state;
static bool FAULT_F1_state;
static bool FAULT_TRO_state;
static bool FAULT_CMD_state;
static bool FAULT_DRL_state;
static bool FAULT_LUF_state;
static bool FAULT_CON_state;
static bool FAULT_PAR_state;
static bool FAULT_IPR_state;
static bool FAULT_ONC_state;
static bool FAULT_SUF_state;
static bool FAULT_OFL_state;
static bool FAULT_DIV_state;
static bool FAULT_EXF_state;
static bool FAULT_DF0_state;
static bool FAULT_DF1_state;
static bool FAULT_DF2_state;
static bool FAULT_DF3_state;
static bool FAULT_ACV_state;
static bool FAULT_MME2_state;
static bool FAULT_MME3_state;
static bool FAULT_MME4_state;
static bool FAULT_F2_state;
static bool FAULT_F3_state;
static bool FAULT_TRB_state;
//static bool FAULT_oob_state;

//static bool IPR_ill_op_state;
//static bool IPR_ill_mod_state;
//static bool IPR_ill_slv_state;
//static bool IPR_ill_proc_state;
//static bool ONC_nem_state;
//static bool STR_oob_state;
//static bool IPR_ill_dig_state;
//static bool PAR_proc_paru_state;
//static bool PAR_proc_parl_state;
//static bool CON_con_a_state;
//static bool CON_con_b_state;
//static bool CON_con_c_state;
//static bool CON_con_d_state;
//static bool ONC_da_err_state;
//static bool ONC_da_err2_state;
//static bool PAR_cpar_dir_state;
//static bool PAR_cpar_str_state;
//static bool PAR_cpar_ia_state;
//static bool PAR_cpar_blk_state;
//static bool FAULT_subtype_port_a_state;
//static bool FAULT_subtype_port_b_state;
//static bool FAULT_subtype_port_c_state;
//static bool FAULT_subtype_port_d_state;
//static bool FAULT_subtype_cpd_state;
//static bool FAULT_subtype_level_0_state;
//static bool FAULT_subtype_level_1_state;
//static bool FAULT_subtype_level_2_state;
//static bool FAULT_subtype_level_3_state;
//static bool FAULT_subtype_cdd_state;
//static bool FAULT_subtype_par_sdwam_state;
//static bool FAULT_subtype_par_ptwam_state;
//static bool ACV_ACDF0_state;
//static bool ACV_ACDF1_state;
//static bool ACV_ACDF2_state;
//static bool ACV_ACDF3_state;
//static bool ACV0_IRO_state;
//static bool ACV1_OEB_state;
//static bool ACV2_E_OFF_state;
//static bool ACV3_ORB_state;
//static bool ACV4_R_OFF_state;
//static bool ACV5_OWB_state;
//static bool ACV6_W_OFF_state;
//static bool ACV7_NO_GA_state;
//static bool ACV8_OCB_state;
//static bool ACV9_OCALL_state;
//static bool ACV10_BOC_state;
//static bool ACV11_INRET_state;
//static bool ACV12_CRT_state;
//static bool ACV13_RALR_state;
//static bool ACV_AME_state;
//static bool ACV15_OOSB_state;

//static bool intr_pair_addr_state [6];

static GtkWidget * PPR_display;
static GtkWidget * inst_display;
static GtkWidget * A_display;
static GtkWidget * Q_display;
static GtkWidget * E_display;
static GtkWidget * X_display[8];
static GtkWidget * IR_display;
static GtkWidget * TR_display;
static GtkWidget * RALR_display;
static GtkWidget * PAR_display[8];
//static GtkWidget * BAR_STRfault_display;
//static GtkWidget * TPR_display;
//static GtkWidget * DSBR_display;
static GtkWidget * fault_display[2];
//static GtkWidget * IPRfault_display;
//static GtkWidget * ONCfault_display;
//static GtkWidget * PARfault_display;
//static GtkWidget * CONfault_display;
//static GtkWidget * cachefault_display;
//static GtkWidget * ACDF_display;
//static GtkWidget * ACVfault_display;
//static GtkWidget * intrpair_display;

static gboolean time_handler (GtkWidget * widget) {
  bool update = false;

#define PROBE(v, s) \
  if (memcmp (v ## _p, & previous.v, sizeof (previous.v))) { \
    update = true; \
    previous.v = * v ## _p; \
    s \
  }

//#define PROBEn(v, n, s) \
//  if (memcmp (v ## _p + (n) * sizeof_ ## v, & previous.v[n], sizeof (previous.v[n]))) { \
//    update = true; \
//    previous.v[n] = * ((typeof (previous.v[n]) *)(v ## _p + (n) + sizeof_ ## v)); \
//    s \
//  }

#define PROBEns(v, n, s) \
  if (memcmp (v ## _p[n], & previous.v[n], sizeof (previous.v[n]))) { \
    update = true; \
    previous.v[n] = * ((typeof (previous.v[n]) *)(v ## _p[n])); \
    s \
  }

#define BIT(v) ((1llu << i) & previous.v) ? 1 : 0

  PROBE (PRR, { for (int i = 0; i <  3; i ++) PRR_state  [ 2 - i] = BIT (PRR); });
  PROBE (PSR, { for (int i = 0; i < 15; i ++) PSR_state  [14 - i] = BIT (PSR); });
  PROBE (P,   { for (int i = 0; i <  1; i ++) P_state    [ 0 - i] = BIT (P);   });
  PROBE (IC,  { for (int i = 0; i < 18; i ++) IC_state   [17 - i] = BIT (IC);  });
  PROBE (IWB, { for (int i = 0; i < 36; i ++) inst_state [35 - i] = BIT (IWB); });
  PROBE (rA,  { for (int i = 0; i < 36; i ++) A_state    [35 - i] = BIT (rA);  });
  PROBE (rQ,  { for (int i = 0; i < 36; i ++) Q_state    [35 - i] = BIT (rQ);  });
  PROBE (rE,  { for (int i = 0; i <  8; i ++) E_state    [ 7 - i] = BIT (rE);  });
  for(int nreg = 0; nreg < 8; nreg ++) {
    /* cppcheck-suppress internalAstError */
    PROBEns (rX, nreg, { for (int i = 0; i < 18; i ++) X_state [nreg][17 - i] = BIT (rX[nreg]);  });
  }
  PROBE (IR, { for (int i = 0; i < 18; i ++) IR_state [17 - i] = BIT (IR); });
  PROBE (rTR, { for (int i = 0; i < 26; i ++) TR_state [26 - i] = BIT (rTR); });
  PROBE (rRALR, { for (int i = 0; i < 3; i ++) RALR_state [2 - i] = BIT (rRALR); });
  for(int nreg = 0; nreg < 8; nreg ++) {
    PROBEns (SNR, nreg, { for (int i = 0; i < 15; i ++) SNR_state [nreg][14 - i] = BIT (SNR[nreg]); });
    PROBEns (RNR, nreg, { for (int i = 0; i < 3; i ++) RNR_state [nreg][2 - i] = BIT (RNR[nreg]); });
    PROBEns (PR_BITNO, nreg, { for (int i = 0; i < 6; i ++) BITNO_state [nreg][5 - i] = BIT (PR_BITNO[nreg]); });
    PROBEns (WORDNO, nreg, { for (int i = 0; i < 18; i ++) WORDNO_state [nreg][17 - i] = BIT (WORDNO[nreg]); });
  }
  PROBE (TRR, { for (int i = 0; i < 3; i ++) TRR_state [2 - i] = BIT (TRR); });
  PROBE (TSR, {for (int i = 0; i < 15; i ++) TSR_state [14 - i] = BIT (TSR); });
  PROBE (TBR, {for (int i = 0; i < 6; i ++) TBR_state [3 - i] = BIT (TBR); });
  PROBE (CA, {for (int i = 0; i < 18; i ++) CA_state [17 - i] = BIT (CA); });

//  if (memcmp (& cpun->BAR, & previous.BAR, sizeof (previous.BAR))) {
//    update = true;
//    for (int i = 0; i < 9; i ++)
//      BASE_state [8 - i] = ((1llu << i) & cpun->BAR.BASE) ? 1 : 0;
//    for (int i = 0; i < 9; i ++)
//      BOUND_state [8 - i] = ((1llu << i) & cpun->BAR.BOUND) ? 1 : 0;
//    //gtk_widget_queue_draw (BAR_STRfault_display);
//  }

  PROBE (ADDR, { for (int i = 0; i < 24; i ++) ADDR_state [23 - i] = BIT (ADDR); });
  PROBE (BND, { for (int i = 0; i < 14; i ++) BND_state [13 - i] = BIT (BND); });
  PROBE (U, { for (int i = 0; i < 1; i ++) U_state [0 - i] = BIT (U); });
  PROBE (STACK, { for (int i = 0; i < 12; i ++) STACK_state [11 - i] = BIT (STACK); });
  PROBE (faultNumber, {
    FAULT_SDF_state = (FAULT_SDF == previous.faultNumber) ? 1 : 0;
    FAULT_STR_state = (FAULT_STR == previous.faultNumber) ? 1 : 0;
    FAULT_MME_state = (FAULT_MME == previous.faultNumber) ? 1 : 0;
    FAULT_F1_state = (FAULT_F1 == previous.faultNumber) ? 1 : 0;
    FAULT_TRO_state = (FAULT_TRO == previous.faultNumber) ? 1 : 0;
    FAULT_CMD_state = (FAULT_CMD == previous.faultNumber) ? 1 : 0;
    FAULT_DRL_state = (FAULT_DRL == previous.faultNumber) ? 1 : 0;
    FAULT_LUF_state = (FAULT_LUF == previous.faultNumber) ? 1 : 0;
    FAULT_CON_state = (FAULT_CON == previous.faultNumber) ? 1 : 0;
    FAULT_PAR_state = (FAULT_PAR == previous.faultNumber) ? 1 : 0;
    FAULT_IPR_state = (FAULT_IPR == previous.faultNumber) ? 1 : 0;
    FAULT_ONC_state = (FAULT_ONC == previous.faultNumber) ? 1 : 0;
    FAULT_SUF_state = (FAULT_SUF == previous.faultNumber) ? 1 : 0;
    FAULT_OFL_state = (FAULT_OFL == previous.faultNumber) ? 1 : 0;
    FAULT_DIV_state = (FAULT_DIV == previous.faultNumber) ? 1 : 0;
    FAULT_EXF_state = (FAULT_EXF == previous.faultNumber) ? 1 : 0;
    FAULT_DF0_state = (FAULT_DF0 == previous.faultNumber) ? 1 : 0;
    FAULT_DF1_state = (FAULT_DF1 == previous.faultNumber) ? 1 : 0;
    FAULT_DF2_state = (FAULT_DF2 == previous.faultNumber) ? 1 : 0;
    FAULT_DF3_state = (FAULT_DF3 == previous.faultNumber) ? 1 : 0;
    FAULT_ACV_state = (FAULT_ACV == previous.faultNumber) ? 1 : 0;
    FAULT_MME2_state = (FAULT_MME2 == previous.faultNumber) ? 1 : 0;
    FAULT_MME3_state = (FAULT_MME3 == previous.faultNumber) ? 1 : 0;
    FAULT_MME4_state = (FAULT_MME4 == previous.faultNumber) ? 1 : 0;
    FAULT_F2_state = (FAULT_F2 == previous.faultNumber) ? 1 : 0;
    FAULT_F3_state = (FAULT_F3 == previous.faultNumber) ? 1 : 0;
    FAULT_TRB_state = (FAULT_TRB == previous.faultNumber) ? 1 : 0;
  });
#if 0

  if (memcmp (& cpun->faultNumber, & previous.faultNumber, sizeof (previous.faultNumber))) {
    update = true;
    //FAULT_oob_state = (oob_fault == cpun->faultNumber) ? 1 : 0;

    //gtk_widget_queue_draw (fault_display[0]);
    //gtk_widget_queue_draw (fault_display[1]);
  }
#endif

  if (update)
    gtk_widget_queue_draw (widget);
  return TRUE;
}

#define XSTR_EMAXLEN 32767

static const char
*xstrerror_l(int errnum)
{
  int saved = errno;
  const char *ret = NULL;
  static /* __thread */ char buf[XSTR_EMAXLEN];

#if defined(__APPLE__) || defined(_AIX) || \
      defined(__MINGW32__) || defined(__MINGW64__) || \
        defined(CROSS_MINGW32) || defined(CROSS_MINGW64)
# if defined(__MINGW32__) || defined(__MINGW64__) || \
        defined(CROSS_MINGW32) || defined(CROSS_MINGW64)
  if (strerror_s(buf, sizeof(buf), errnum) == 0) ret = buf; /*LINTOK: xstrerror_l*/
# else
  if (strerror_r(errnum, buf, sizeof(buf)) == 0) ret = buf; /*LINTOK: xstrerror_l*/
# endif
#else
# if defined(__NetBSD__)
  locale_t loc = LC_GLOBAL_LOCALE;
# else
  locale_t loc = uselocale((locale_t)0);
# endif
  locale_t copy = loc;
  if (copy == LC_GLOBAL_LOCALE)
    copy = duplocale(copy);

  if (copy != (locale_t)0)
    {
      ret = strerror_l(errnum, copy); /*LINTOK: xstrerror_l*/
      if (loc == LC_GLOBAL_LOCALE)
        {
          freelocale(copy);
        }
    }
#endif

  if (!ret)
    {
      (void)snprintf(buf, sizeof(buf), "Unknown error %d", errnum);
      ret = buf;
    }

  errno = saved;
  return ret;
}

static void * openShm (char * key) {
  void * p;
  char buf [256];
  (void)sprintf (buf, "dps8m.%s", key);
  int fd = open (buf, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    (void)fprintf(stderr, "FATAL: open failed: %s (Error %d)\r\n", xstrerror_l(errno), errno);
    exit(EXIT_FAILURE);
  }

  struct stat sb;
  if (fstat(fd, &sb) == -1) { /* To obtain file size */
    (void)fprintf(stderr, "FATAL: fstat failed: %s (Error %d)\r\n", xstrerror_l(errno), errno);
    exit(EXIT_FAILURE);
  }

  p = mmap (NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);

  if (p == MAP_FAILED) {
    (void)fprintf(stderr, "FATAL: mmap failed: %s (Error %d)\r\n", xstrerror_l(errno), errno);
    exit(EXIT_FAILURE);
  }

  return p;
}

int main (int argc, char * argv []) {
  (void)setlocale(LC_ALL, "");

  struct sigaction quit_action;
  quit_action.sa_handler = SIG_IGN;
  quit_action.sa_flags = SA_RESTART;
  sigaction (SIGQUIT, & quit_action, NULL);

  int cpunum = 0;
  if (argc > 1 && strlen (argv [1])) {
    char * end;
    long p = strtol (argv [1], & end, 0);
    if (* end == 0) {
      cpunum = (int)p;
      argv [1] [0] = 0;
    }
  }

#if !defined(N_CPU_UNITS_MAX)
# define N_CPU_UNITS_MAX 8
#endif

  if (cpunum < 0 || cpunum > N_CPU_UNITS_MAX - 1) {
    (void)fprintf (stderr, "FATAL: Invalid CPU number %ld: expected 0..%ld\r\n", (long)cpunum, (long)N_CPU_UNITS_MAX);
    return EXIT_FAILURE;
  }

  system_state = (struct system_state_s *) openShm ("state");
  if (! system_state) {
    (void)fprintf (stderr, "FATAL: system state open_shm failed: %s (Error %d)\r\n", xstrerror_l(errno), errno);
    return EXIT_FAILURE;
  }

  ss_p = (byte *) system_state;

  uint32_t symbolType, valueType, value;

  cpu0_p = ss_p + lookup (system_state, SYM_STATE_OFFSET, "cpus[]", & symbolType, & valueType, & value);
  sizeof_cpu = (size_t) lookup (system_state, SYM_STRUCT_SZ, "sizeof(*cpus)", & symbolType, & valueType, & value);

#define ENUM32(n) n = (uint32_t) lookup (system_state, SYM_ENUM, #n, & symbolType, & valueType, & value);
  ENUM32 (FAULT_SDF);
  ENUM32 (FAULT_STR);
  ENUM32 (FAULT_MME);
  ENUM32 (FAULT_F1);
  ENUM32 (FAULT_TRO);
  ENUM32 (FAULT_CMD);
  ENUM32 (FAULT_DRL);
  ENUM32 (FAULT_LUF);
  ENUM32 (FAULT_CON);
  ENUM32 (FAULT_PAR);
  ENUM32 (FAULT_IPR);
  ENUM32 (FAULT_ONC);
  ENUM32 (FAULT_SUF);
  ENUM32 (FAULT_OFL);
  ENUM32 (FAULT_DIV);
  ENUM32 (FAULT_EXF);
  ENUM32 (FAULT_DF0);
  ENUM32 (FAULT_DF1);
  ENUM32 (FAULT_DF2);
  ENUM32 (FAULT_DF3);
  ENUM32 (FAULT_ACV);
  ENUM32 (FAULT_MME2);
  ENUM32 (FAULT_MME3);
  ENUM32 (FAULT_MME4);
  ENUM32 (FAULT_F2);
  ENUM32 (FAULT_F3);
  ENUM32 (FAULT_TRB);

  cpun = cpu0_p + cpunum * sizeof_cpu;
  PPR_p = cpun + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].PPR", & symbolType, & valueType, & value);
  PRR_p = (uint8_t *) (PPR_p + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].PPR.PRR", & symbolType, & valueType, & value));
  PSR_p = (uint16_t *) (PPR_p + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].PPR.PSR", & symbolType, & valueType, & value));
  P_p   = (uint8_t *) (PPR_p + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].PPR.P",   & symbolType, & valueType, & value));
  IC_p  = (uint32_t *) (PPR_p + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].PPR.IC",  & symbolType, & valueType, & value));
  cu_p  = cpun + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].cu", & symbolType, & valueType, & value);
  IWB_p = (uint64_t *) (cu_p + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].cu.IWB", & symbolType, & valueType, & value));
  IR_p = (uint32_t *) (cu_p + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].cu.IR", & symbolType, & valueType, & value));
  rA_p = (uint64_t *) (cpun + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].rA", & symbolType, & valueType, & value));
  rQ_p = (uint64_t *) (cpun + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].rQ", & symbolType, & valueType, & value));
  rE_p = (uint8_t *) (cpun + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].rE", & symbolType, & valueType, & value));
  rX0_p = cpun + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].rX[]", & symbolType, & valueType, & value);
  sizeof_rX = (size_t) lookup (system_state, SYM_STRUCT_SZ, "sizeof(*rX)", & symbolType, & valueType, & value);
  for (int nreg = 0; nreg < 8; nreg ++) {
    rX_p[nreg] = (uint32_t *) \
                 (rX0_p + nreg * sizeof_rX + lookup (system_state, SYM_STRUCT_OFFSET,
                                                     "cpus[].rX", & symbolType, & valueType, & value));
  }
  rTR_p = (uint32_t *) (cpun + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].rTR", & symbolType, & valueType, & value));
  rRALR_p = (uint8_t *) (cpun + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].rRALR", & symbolType, & valueType, & value));
  PAR_p = (uint8_t *) (cpun + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].PAR[]", & symbolType, & valueType, & value));
  sizeof_PAR = (size_t) lookup (system_state, SYM_STRUCT_SZ, "sizeof(*PAR)", & symbolType, & valueType, & value);
  for (int nreg = 0; nreg < 8; nreg ++) {
    SNR_p[nreg] = (uint16_t *) \
                  (PAR_p + nreg * sizeof_PAR + lookup (system_state, SYM_STRUCT_OFFSET,
                                                       "cpus[].PAR[].SNR", & symbolType, & valueType, & value));
    RNR_p[nreg] = (uint8_t *) \
                  (PAR_p + nreg * sizeof_PAR + lookup (system_state, SYM_STRUCT_OFFSET,
                                                       "cpus[].PAR[].RNR", & symbolType, & valueType, & value));
    PR_BITNO_p[nreg] = (uint8_t *) \
                       (PAR_p + nreg * sizeof_PAR + lookup (system_state, SYM_STRUCT_OFFSET,
                                                            "cpus[].PAR[].PR_BITNO", & symbolType, & valueType, & value));
    WORDNO_p[nreg] = (uint32_t *) \
                     (PAR_p + nreg * sizeof_PAR + lookup (system_state, SYM_STRUCT_OFFSET,
                                                          "cpus[].PAR[].WORDNO", & symbolType, & valueType, & value));
  }
  //BAR_p = cpun + lookup \
  //        (system_state, SYM_STRUCT_OFFSET, "cpus[].BAR", & symbolType, & valueType, & value);
  //BASE_p = (uint8_t *) \
  //         (BAR_p + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].BAR.BASE", & symbolType, & valueType, & value));
  //BOUND_p = (uint8_t *) \
  //          (BAR_p + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].BAR.BOUND", & symbolType, & valueType, & value));
  TPR_p = cpun + lookup \
          (system_state, SYM_STRUCT_OFFSET, "cpus[].TPR", & symbolType, & valueType, & value);
  TRR_p = (uint8_t *) \
          (TPR_p + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].TPR.TRR", & symbolType, & valueType, & value));
  TSR_p = (uint16_t *) \
          (TPR_p + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].TPR.TSR", & symbolType, & valueType, & value));
  TBR_p = (uint8_t *) \
          (TPR_p + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].TPR.TBR", & symbolType, & valueType, & value));
  CA_p = (uint32_t *) \
         (TPR_p + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].TPR.CA", & symbolType, & valueType, & value));
  DSBR_p = cpun + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].DSBR", & symbolType, & valueType, & value);
  ADDR_p = (uint32_t *) \
           (DSBR_p + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].DSBR.ADDR", & symbolType, & valueType, & value));
  BND_p = (uint16_t *) \
          (DSBR_p + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].DSBR.BND", & symbolType, & valueType, & value));
  U_p = (uint8_t *) \
        (DSBR_p + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].DSBR.U", & symbolType, & valueType, & value));
  STACK_p = (uint16_t *) \
            (DSBR_p + lookup (system_state, SYM_STRUCT_OFFSET, "cpus[].DSBR.STACK", & symbolType, & valueType, & value));
  faultNumber_p = (uint32_t *) cpun + lookup \
                  (system_state, SYM_STRUCT_OFFSET, "cpus[].faultNumber", & symbolType, & valueType, & value);

  gdk_rgba_parse (& lightOn, "white");
  gdk_rgba_parse (& lightOff, "black");

  GtkWidget * window;

  gtk_init (& argc, & argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget * window_rows = gtk_grid_new ();

// PPR

  PPR_display = gtk_grid_new ();

  (void)memset (PRR_state, 0, sizeof (PRR_state));
  (void)memset (PSR_state, 0, sizeof (PSR_state));
  (void)memset (P_state, 0, sizeof (P_state));
  (void)memset (IC_state, 0, sizeof (IC_state));

  GtkWidget * PRR_lights = createLightArray (3, PRR_state);
  GtkWidget * PSR_lights = createLightArray (15, PSR_state);
  GtkWidget * P_lights = createLightArray (1, P_state);
  GtkWidget * IC_lights = createLightArray (18, IC_state);

  GtkWidget * PPR_label = gtk_label_new ("PPR ");
  GtkWidget * PRR_label = gtk_label_new (" PRR ");
  GtkWidget * PSR_label = gtk_label_new (" PSR ");
  GtkWidget * P_label = gtk_label_new (" P ");
  GtkWidget * IC_label = gtk_label_new (" IC ");

  gtk_grid_attach (GTK_GRID (PPR_display), PPR_label,  0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (PPR_display), PRR_label,  1, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (PPR_display), PRR_lights, 2, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (PPR_display), PSR_label,  3, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (PPR_display), PSR_lights, 4, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (PPR_display), P_label,    5, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (PPR_display), P_lights,   6, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (PPR_display), IC_label,   7, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (PPR_display), IC_lights,  8, 0, 1, 1);

// instr

  inst_display = gtk_grid_new ();
  (void)memset (inst_state, 0, sizeof (inst_state));
  GtkWidget * inst_lights = createLightArray (36, inst_state);
  GtkWidget * inst_label = gtk_label_new ("INSTRUCTION ");
  gtk_grid_attach (GTK_GRID (inst_display), inst_label,  0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (inst_display), inst_lights, 1, 0, 1, 1);

// A

  A_display = gtk_grid_new ();
  (void)memset (A_state, 0, sizeof (A_state));
  GtkWidget * A_lights = createLightArray (36, A_state);
  GtkWidget * A_label = gtk_label_new ("A ");
  gtk_grid_attach (GTK_GRID (A_display), A_label,  0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (A_display), A_lights, 1, 0, 1, 1);

// Q

  Q_display = gtk_grid_new ();
  (void)memset (Q_state, 0, sizeof (Q_state));
  GtkWidget * Q_lights = createLightArray (36, Q_state);
  GtkWidget * Q_label = gtk_label_new ("Q ");
  gtk_grid_attach (GTK_GRID (Q_display), Q_label,  0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (Q_display), Q_lights, 1, 0, 1, 1);

// E

  E_display = gtk_grid_new ();
  (void)memset (E_state, 0, sizeof (E_state));
  GtkWidget * E_lights = createLightArray (8, E_state);
  GtkWidget * E_label = gtk_label_new ("E ");
  gtk_grid_attach (GTK_GRID (E_display), E_label,  0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (E_display), E_lights, 1, 0, 1, 1);

// X

  GtkWidget * X_lights[8];
  GtkWidget * X_label[8];
  for(int nreg = 0; nreg < 8; nreg ++) {
    char X_text[4] = "Xn ";
    X_display[nreg] = gtk_grid_new ();

    (void)memset (X_state[nreg], 0, sizeof (X_state[nreg]));

    X_lights[nreg] = createLightArray (18, X_state[nreg]);

    (void)snprintf(X_text, sizeof(X_text), "X%d ", nreg);
    X_label[nreg] = gtk_label_new (X_text);

    gtk_grid_attach (GTK_GRID (X_display[nreg]), X_label[nreg],  0, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (X_display[nreg]), X_lights[nreg], 1, 0, 1, 1);
  }

// IR

  IR_display = gtk_grid_new ();
  (void)memset (IR_state, 0, sizeof (IR_state));
  GtkWidget * IR_lights = createLightArray (18, IR_state);
  GtkWidget * IR_label = gtk_label_new ("IR ");
  gtk_grid_attach (GTK_GRID (IR_display), IR_label,  0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (IR_display), IR_lights, 1, 0, 1, 1);

// TR

  TR_display = gtk_grid_new ();
  (void)memset (TR_state, 0, sizeof (TR_state));
  GtkWidget * TR_lights = createLightArray (27, TR_state);
  GtkWidget * TR_label = gtk_label_new ("TR ");
  gtk_grid_attach (GTK_GRID (TR_display), TR_label,  0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (TR_display), TR_lights, 1, 0, 1, 1);

// RALR

  RALR_display = gtk_grid_new ();
  (void)memset (RALR_state, 0, sizeof (RALR_state));
  GtkWidget * RALR_lights = createLightArray (3, RALR_state);
  GtkWidget * RALR_label = gtk_label_new ("RALR ");
  gtk_grid_attach (GTK_GRID (RALR_display), RALR_label,  0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (RALR_display), RALR_lights, 1, 0, 1, 1);

// PAR

  GtkWidget * SNR_lights[8];
  GtkWidget * RNR_lights[8];
  GtkWidget * BITNO_lights[8];
  GtkWidget * WORDNO_lights[8];

  GtkWidget * PAR_label[8];
  GtkWidget * SNR_label[8];
  GtkWidget * RNR_label[8];
  GtkWidget * BITNO_label[8];
  GtkWidget * WORDNO_label[8];

  for(int nreg = 0; nreg < 8; nreg ++) {
    char PAR_text[6] = "PARn ";

    PAR_display[nreg] = gtk_grid_new ();
    (void)memset (SNR_state[nreg], 0, sizeof (SNR_state[nreg]));
    (void)memset (RNR_state[nreg], 0, sizeof (RNR_state[nreg]));
    (void)memset (BITNO_state[nreg], 0, sizeof (BITNO_state[nreg]));
    (void)memset (WORDNO_state[nreg], 0, sizeof (WORDNO_state[nreg]));

    SNR_lights[nreg] = createLightArray (15, SNR_state[nreg]);
    RNR_lights[nreg] = createLightArray (3, RNR_state[nreg]);
    BITNO_lights[nreg] = createLightArray (6, BITNO_state[nreg]);
    WORDNO_lights[nreg] = createLightArray (18, WORDNO_state[nreg]);

    (void)snprintf(PAR_text, sizeof(PAR_text), "PAR%d ", nreg);
    PAR_label[nreg] = gtk_label_new (PAR_text);
    SNR_label[nreg] = gtk_label_new (" SNR ");
    RNR_label[nreg] = gtk_label_new (" RNR ");
    BITNO_label[nreg] = gtk_label_new (" BITNO ");
    WORDNO_label[nreg] = gtk_label_new (" WORDNO ");

    gtk_grid_attach (GTK_GRID (PAR_display[nreg]), PAR_label[nreg],  0, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (PAR_display[nreg]), SNR_label[nreg],  1, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (PAR_display[nreg]), SNR_lights[nreg], 2, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (PAR_display[nreg]), RNR_label[nreg],  3, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (PAR_display[nreg]), RNR_lights[nreg], 4, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (PAR_display[nreg]), BITNO_label[nreg],    5, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (PAR_display[nreg]), BITNO_lights[nreg],   6, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (PAR_display[nreg]), WORDNO_label[nreg],   7, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (PAR_display[nreg]), WORDNO_lights[nreg],  8, 0, 1, 1);
  }

  fault_display[0] = gtk_grid_new ();
  fault_display[1] = gtk_grid_new ();

  (void)memset (&FAULT_SDF_state, 0, sizeof (FAULT_SDF_state));
  GtkWidget * FAULT_SDF_lights = createLightArray (1, &FAULT_SDF_state);
  GtkWidget * FAULT_SDF_label = gtk_label_new ("SDF ");
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_SDF_label, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_SDF_lights, 1, 0, 1, 1);

  (void)memset (&FAULT_STR_state, 0, sizeof (FAULT_STR_state));
  GtkWidget * FAULT_STR_lights = createLightArray (1, &FAULT_STR_state);
  GtkWidget * FAULT_STR_label = gtk_label_new ("STR ");
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_STR_label, 2, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_STR_lights, 3, 0, 1, 1);

  (void)memset (&FAULT_MME_state, 0, sizeof (FAULT_MME_state));
  GtkWidget * FAULT_MME_lights = createLightArray (1, &FAULT_MME_state);
  GtkWidget * FAULT_MME_label = gtk_label_new ("MME ");
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_MME_label, 4, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_MME_lights, 5, 0, 1, 1);

  (void)memset (&FAULT_F1_state, 0, sizeof (FAULT_F1_state));
  GtkWidget * FAULT_F1_lights = createLightArray (1, &FAULT_F1_state);
  GtkWidget * FAULT_F1_label = gtk_label_new ("F1 ");
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_F1_label, 6, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_F1_lights, 7, 0, 1, 1);

  (void)memset (&FAULT_TRO_state, 0, sizeof (FAULT_TRO_state));
  GtkWidget * FAULT_TRO_lights = createLightArray (1, &FAULT_TRO_state);
  GtkWidget * FAULT_TRO_label = gtk_label_new ("TRO ");
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_TRO_label, 8, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_TRO_lights, 9, 0, 1, 1);

  (void)memset (&FAULT_CMD_state, 0, sizeof (FAULT_CMD_state));
  GtkWidget * FAULT_CMD_lights = createLightArray (1, &FAULT_CMD_state);
  GtkWidget * FAULT_CMD_label = gtk_label_new ("CMD ");
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_CMD_label, 10, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_CMD_lights, 11, 0, 1, 1);

  (void)memset (&FAULT_DRL_state, 0, sizeof (FAULT_DRL_state));
  GtkWidget * FAULT_DRL_lights = createLightArray (1, &FAULT_DRL_state);
  GtkWidget * FAULT_DRL_label = gtk_label_new ("DRL ");
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_DRL_label, 12, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_DRL_lights, 13, 0, 1, 1);

  (void)memset (&FAULT_LUF_state, 0, sizeof (FAULT_LUF_state));
  GtkWidget * FAULT_LUF_lights = createLightArray (1, &FAULT_LUF_state);
  GtkWidget * FAULT_LUF_label = gtk_label_new ("LUF ");
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_LUF_label, 14, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_LUF_lights, 15, 0, 1, 1);

  (void)memset (&FAULT_CON_state, 0, sizeof (FAULT_CON_state));
  GtkWidget * FAULT_CON_lights = createLightArray (1, &FAULT_CON_state);
  GtkWidget * FAULT_CON_label = gtk_label_new ("CON ");
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_CON_label, 16, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_CON_lights, 17, 0, 1, 1);

  (void)memset (&FAULT_PAR_state, 0, sizeof (FAULT_PAR_state));
  GtkWidget * FAULT_PAR_lights = createLightArray (1, &FAULT_PAR_state);
  GtkWidget * FAULT_PAR_label = gtk_label_new ("PAR ");
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_PAR_label, 18, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_PAR_lights, 19, 0, 1, 1);

  (void)memset (&FAULT_IPR_state, 0, sizeof (FAULT_IPR_state));
  GtkWidget * FAULT_IPR_lights = createLightArray (1, &FAULT_IPR_state);
  GtkWidget * FAULT_IPR_label = gtk_label_new ("IPR ");
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_IPR_label, 20, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_IPR_lights, 21, 0, 1, 1);

  (void)memset (&FAULT_ONC_state, 0, sizeof (FAULT_ONC_state));
  GtkWidget * FAULT_ONC_lights = createLightArray (1, &FAULT_ONC_state);
  GtkWidget * FAULT_ONC_label = gtk_label_new ("ONC ");
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_ONC_label, 22, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_ONC_lights, 23, 0, 1, 1);

  (void)memset (&FAULT_SUF_state, 0, sizeof (FAULT_SUF_state));
  GtkWidget * FAULT_SUF_lights = createLightArray (1, &FAULT_SUF_state);
  GtkWidget * FAULT_SUF_label = gtk_label_new ("SUF ");
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_SUF_label, 24, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_SUF_lights, 25, 0, 1, 1);

  (void)memset (&FAULT_OFL_state, 0, sizeof (FAULT_OFL_state));
  GtkWidget * FAULT_OFL_lights = createLightArray (1, &FAULT_OFL_state);
  GtkWidget * FAULT_OFL_label = gtk_label_new ("OFL ");
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_OFL_label, 26, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_OFL_lights, 27, 0, 1, 1);

  (void)memset (&FAULT_DIV_state, 0, sizeof (FAULT_DIV_state));
  GtkWidget * FAULT_DIV_lights = createLightArray (1, &FAULT_DIV_state);
  GtkWidget * FAULT_DIV_label = gtk_label_new ("DIV ");
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_DIV_label, 28, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_DIV_lights, 29, 0, 1, 1);

  (void)memset (&FAULT_EXF_state, 0, sizeof (FAULT_EXF_state));
  GtkWidget * FAULT_EXF_lights = createLightArray (1, &FAULT_EXF_state);
  GtkWidget * FAULT_EXF_label = gtk_label_new ("EXF ");
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_EXF_label, 30, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[0]), FAULT_EXF_lights, 31, 0, 1, 1);

  (void)memset (&FAULT_DF0_state, 0, sizeof (FAULT_DF0_state));
  GtkWidget * FAULT_DF0_lights = createLightArray (1, &FAULT_DF0_state);
  GtkWidget * FAULT_DF0_label = gtk_label_new ("DF0 ");
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_DF0_label, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_DF0_lights, 1, 0, 1, 1);

  (void)memset (&FAULT_DF1_state, 0, sizeof (FAULT_DF1_state));
  GtkWidget * FAULT_DF1_lights = createLightArray (1, &FAULT_DF1_state);
  GtkWidget * FAULT_DF1_label = gtk_label_new ("DF1 ");
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_DF1_label, 2, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_DF1_lights, 3, 0, 1, 1);

  (void)memset (&FAULT_DF2_state, 0, sizeof (FAULT_DF2_state));
  GtkWidget * FAULT_DF2_lights = createLightArray (1, &FAULT_DF2_state);
  GtkWidget * FAULT_DF2_label = gtk_label_new ("DF2 ");
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_DF2_label, 4, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_DF2_lights, 5, 0, 1, 1);

  (void)memset (&FAULT_DF3_state, 0, sizeof (FAULT_DF3_state));
  GtkWidget * FAULT_DF3_lights = createLightArray (1, &FAULT_DF3_state);
  GtkWidget * FAULT_DF3_label = gtk_label_new ("DF3 ");
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_DF3_label, 6, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_DF3_lights, 7, 0, 1, 1);

  (void)memset (&FAULT_ACV_state, 0, sizeof (FAULT_ACV_state));
  GtkWidget * FAULT_ACV_lights = createLightArray (1, &FAULT_ACV_state);
  GtkWidget * FAULT_ACV_label = gtk_label_new ("ACV ");
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_ACV_label, 8, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_ACV_lights, 9, 0, 1, 1);

  (void)memset (&FAULT_MME2_state, 0, sizeof (FAULT_MME2_state));
  GtkWidget * FAULT_MME2_lights = createLightArray (1, &FAULT_MME2_state);
  GtkWidget * FAULT_MME2_label = gtk_label_new ("MME2 ");
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_MME2_label, 10, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_MME2_lights, 11, 0, 1, 1);

  (void)memset (&FAULT_MME3_state, 0, sizeof (FAULT_MME3_state));
  GtkWidget * FAULT_MME3_lights = createLightArray (1, &FAULT_MME3_state);
  GtkWidget * FAULT_MME3_label = gtk_label_new ("MME3 ");
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_MME3_label, 12, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_MME3_lights, 13, 0, 1, 1);

  (void)memset (&FAULT_MME4_state, 0, sizeof (FAULT_MME4_state));
  GtkWidget * FAULT_MME4_lights = createLightArray (1, &FAULT_MME4_state);
  GtkWidget * FAULT_MME4_label = gtk_label_new ("MME4 ");
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_MME4_label, 14, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_MME4_lights, 15, 0, 1, 1);

  (void)memset (&FAULT_F2_state, 0, sizeof (FAULT_F2_state));
  GtkWidget * FAULT_F2_lights = createLightArray (1, &FAULT_F2_state);
  GtkWidget * FAULT_F2_label = gtk_label_new ("F2 ");
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_F2_label, 16, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_F2_lights, 17, 0, 1, 1);

  (void)memset (&FAULT_F3_state, 0, sizeof (FAULT_F3_state));
  GtkWidget * FAULT_F3_lights = createLightArray (1, &FAULT_F3_state);
  GtkWidget * FAULT_F3_label = gtk_label_new ("F3 ");
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_F3_label, 18, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_F3_lights, 19, 0, 1, 1);

  (void)memset (&FAULT_TRB_state, 0, sizeof (FAULT_TRB_state));
  GtkWidget * FAULT_TRB_lights = createLightArray (1, &FAULT_TRB_state);
  GtkWidget * FAULT_TRB_label = gtk_label_new ("TRB ");
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_TRB_label, 20, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_TRB_lights, 21, 0, 1, 1);

//  (void)memset (&FAULT_oob_state, 0, sizeof (FAULT_oob_state));
//  GtkWidget * FAULT_oob_lights = createLightArray (1, &FAULT_oob_state);
//  GtkWidget * FAULT_oob_label = gtk_label_new ("oob ");
//  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_oob_label, 22, 0, 1, 1);
//  gtk_grid_attach (GTK_GRID (fault_display[1]), FAULT_oob_lights, 23, 0, 1, 1);

// window_rows

  gtk_grid_attach (GTK_GRID (window_rows), PPR_display,  0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (window_rows), inst_display, 0, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (window_rows), A_display,    0, 2, 1, 1);
  gtk_grid_attach (GTK_GRID (window_rows), Q_display,    0, 3, 1, 1);
  gtk_grid_attach (GTK_GRID (window_rows), E_display,    0, 4, 1, 1);
  for(int nreg = 0; nreg < 8; nreg ++) {
    gtk_grid_attach (GTK_GRID (window_rows), X_display[nreg],    0, 5 + nreg, 1, 1);
  }
  gtk_grid_attach (GTK_GRID (window_rows), IR_display,    0, 13, 1, 1);
  gtk_grid_attach (GTK_GRID (window_rows), TR_display,    0, 14, 1, 1);
  gtk_grid_attach (GTK_GRID (window_rows), RALR_display,  0, 15, 1, 1);
  for(int nreg = 0; nreg < 8; nreg ++) {
    gtk_grid_attach (GTK_GRID (window_rows), PAR_display[nreg],    0, 16 + nreg, 1, 1);
  }
  gtk_grid_attach (GTK_GRID (window_rows), fault_display[0], 0, 24, 1, 1);
  gtk_grid_attach (GTK_GRID (window_rows), fault_display[1], 0, 25, 1, 1);

  gtk_container_add (GTK_CONTAINER (window), window_rows);

  // 100 = 10Hz
  // 10 = 100Hz
  g_timeout_add (10, (GSourceFunc) time_handler, (gpointer) window);

  g_signal_connect (window, "delete-event", G_CALLBACK (window_delete), NULL);
  gtk_widget_show_all  (window);

  time_handler (window);

  gtk_main ();

  return EXIT_SUCCESS;
}
