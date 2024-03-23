/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * SPDX-License-Identifier: Multics
 * scspell-id: 6421764a-f62d-11ec-b542-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 *
 * This source file may contain code comments that adapt, include, and/or
 * incorporate Multics program code and/or documentation distributed under
 * the Multics License.  In the event of any discrepancy between code
 * comments herein and the original Multics materials, the original Multics
 * materials should be considered authoritative unless otherwise noted.
 * For more details and historical background, see the LICENSE.md file at
 * the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#ifndef __MINGW64__
# ifndef __MINGW32__
#  include <termios.h>
# endif /* ifndef __MINGW32__ */
#endif /* ifndef __MINGW64__ */
#include <ctype.h>

#include "dps8.h"
#include "dps8_iom.h"
#include "dps8_sys.h"
#include "dps8_console.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_mt.h"  // attachTape
#include "dps8_disk.h"  // attachDisk
#include "dps8_utils.h"
#ifdef LOCKLESS
# include "threadz.h"
#endif /* ifdef LOCKLESS */

#include "libtelnet.h"
#ifdef CONSOLE_FIX
# include "threadz.h"
#endif /* ifdef CONSOLE_FIX */

#ifdef SIM_NAME
# undef SIM_NAME
#endif /* SIM_NAME */
#define SIM_NAME "DPS8M"

#define DBG_CTR 1
#define ASSUME0 0

#ifdef TESTING
# undef realloc
# undef FREE
# define FREE(p) free(p)
# define realloc trealloc
#endif /* ifdef TESTING */

// config switch -- The bootload console has a 30-second timer mechanism. When
// reading from the console, if no character is typed within 30 seconds, the
// read operation is terminated. The timer is controlled by an enable switch,
// must be set to enabled during Multics and BCE

static t_stat opc_reset (DEVICE * dptr);
static t_stat opc_show_nunits (FILE *st, UNIT *uptr, int val,
                               const void *desc);
static t_stat opc_set_nunits (UNIT * uptr, int32 value, const char * cptr,
                              void * desc);
static t_stat opc_autoinput_set (UNIT *uptr, int32 val, const char *cptr,
                                 void *desc);
static t_stat opc_autoinput_show (FILE *st, UNIT *uptr, int val,
                                  const void *desc);
static t_stat opc_set_config (UNUSED UNIT *  uptr, UNUSED int32 value,
                              const char * cptr, UNUSED void * desc);
static t_stat opc_show_config (UNUSED FILE * st, UNUSED UNIT * uptr,
                               UNUSED int  val, UNUSED const void * desc);
static t_stat opc_set_console_port (UNIT * uptr, UNUSED int32 value,
                                    const char * cptr, UNUSED void * desc);
static t_stat opc_show_console_port (UNUSED FILE * st, UNIT * uptr,
                                       UNUSED int val, UNUSED const void * desc);
static t_stat opc_set_console_address (UNIT * uptr, UNUSED int32 value,
                                    const char * cptr, UNUSED void * desc);
static t_stat opc_show_console_address (UNUSED FILE * st, UNIT * uptr,
                                       UNUSED int val, UNUSED const void * desc);
static t_stat opc_set_console_pw (UNIT * uptr, UNUSED int32 value,
                                    const char * cptr, UNUSED void * desc);
static t_stat opc_show_console_pw (UNUSED FILE * st, UNIT * uptr,
                                       UNUSED int val, UNUSED const void * desc);
static t_stat opc_set_device_name (UNIT * uptr, UNUSED int32 value,
                                   const char * cptr, UNUSED void * desc);
static t_stat opc_show_device_name (UNUSED FILE * st, UNIT * uptr,
                                    UNUSED int val, UNUSED const void * desc);

static MTAB opc_mtab[] =
  {
    {
       MTAB_unit_nouc,     /* Mask          */
       0,                  /* Match         */
       "AUTOINPUT",        /* Print string  */
       "AUTOINPUT",        /* Match pstring */
       opc_autoinput_set,
       opc_autoinput_show,
       NULL,
       NULL
    },

    {
      MTAB_dev_valr,                        /* Mask               */
      0,                                    /* Match              */
      "NUNITS",                             /* Print string       */
      "NUNITS",                             /* Match string       */
      opc_set_nunits,                       /* Validation routine */
      opc_show_nunits,                      /* Display routine    */
      "Number of OPC units in the system",  /* Value descriptor   */
      NULL                                  /* Help               */
    },

    {
      MTAB_unit_uc,                   /* Mask               */
      0,                              /* Match              */
      (char *) "CONFIG",              /* Print string       */
      (char *) "CONFIG",              /* Match string       */
      opc_set_config,                 /* Validation routine */
      opc_show_config,                /* Display routine    */
      NULL,                           /* Value descriptor   */
      NULL,                           /* Help               */
    },
    {
      MTAB_XTD | MTAB_VUN | \
      MTAB_VALR | MTAB_NC,            /* Mask               */
      0,                              /* Match              */
      "NAME",                         /* Print string       */
      "NAME",                         /* Match string       */
      opc_set_device_name,            /* Validation routine */
      opc_show_device_name,           /* Display routine    */
      "Set the device name",          /* Value descriptor   */
      NULL                            /* Help               */
    },

    {
      MTAB_unit_valr_nouc,            /* Mask               */
      0,                              /* Match              */
      "PORT",                         /* Print string       */
      "PORT",                         /* Match string       */
      opc_set_console_port,           /* validation routine */
      opc_show_console_port,          /* Display routine    */
      "Set the console port number",  /* Value descriptor   */
      NULL                            /* Help               */
    },

    {
      MTAB_unit_valr_nouc,            /* Mask               */
      0,                              /* Match              */
      "ADDRESS",                      /* Print string       */
      "ADDRESS",                      /* Match string       */
      opc_set_console_address,        /* Validation routine */
      opc_show_console_address,       /* Display routine    */
      "Set the console IP Address",   /* Value descriptor   */
      NULL                            /* Help               */
    },

    {
      MTAB_unit_valr_nouc,            /* Mask               */
      0,                              /* Match              */
      "PW",                           /* Print string       */
      "PW",                           /* Match string       */
      opc_set_console_pw,             /* Validation routine */
      opc_show_console_pw,            /* Display routine    */
      "Set the console password",     /* Value descriptor   */
      NULL                            /* Help               */
    },

    MTAB_eol
};

static DEBTAB opc_dt[] =
  {
    { "NOTIFY", DBG_NOTIFY, NULL },
    { "INFO",   DBG_INFO,   NULL },
    { "ERR",    DBG_ERR,    NULL },
    { "WARN",   DBG_WARN,   NULL },
    { "DEBUG",  DBG_DEBUG,  NULL },
    { "ALL",    DBG_ALL,    NULL }, // Don't move as it messes up DBG message
    { NULL,     0,          NULL }
  };

// Multics only supports a single operator console; but
// it is possible to run multiple Multics instances in a
// cluster. It is also possible to route message output to
// alternate console(s) as I/O devices.

#define N_OPC_UNITS 1 // default
#define OPC_UNIT_IDX(uptr) ((uptr) - opc_unit)

// sim_activate counts in instructions, is dependent on the execution
// model
#ifdef LOCKLESS
// The sim_activate calls are done by the controller thread, which
// has a 1000Hz cycle rate.
// 1K ~= 1 sec
# define ACTIVATE_1SEC 1000
#else
// The sim_activate calls are done by the only thread, with a 4 MHz
// cycle rate.
// 4M ~= 1 sec
# define ACTIVATE_1SEC 4000000
#endif

static t_stat opc_svc (UNIT * unitp);

UNIT opc_unit[N_OPC_UNITS_MAX] = {
#ifdef NO_C_ELLIPSIS
  { UDATA (& opc_svc, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (& opc_svc, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (& opc_svc, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (& opc_svc, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (& opc_svc, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (& opc_svc, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (& opc_svc, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (& opc_svc, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL }
#else
  [0 ... N_OPC_UNITS_MAX - 1] = {
    UDATA (& opc_svc, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL
  }
#endif
};

DEVICE opc_dev = {
    "OPC",         /* Name                */
    opc_unit,      /* Units               */
    NULL,          /* Registers           */
    opc_mtab,      /* Modifiers           */
    N_OPC_UNITS,   /* #units              */
    10,            /* Address radix       */
    8,             /* Address width       */
    1,             /* Address increment   */
    8,             /* Address width       */
    8,             /* Data width          */
    NULL,          /* Examine routine     */
    NULL,          /* Deposit routine     */
    opc_reset,     /* Reset routine       */
    NULL,          /* Boot routine        */
    NULL,          /* Attach routine      */
    NULL,          /* Detach routine      */
    NULL,          /* Context             */
    DEV_DEBUG,     /* Flags               */
    0,             /* Debug control flags */
    opc_dt,        /* Debug flag names    */
    NULL,          /* Memory size change  */
    NULL,          /* Logical name        */
    NULL,          /* Help                */
    NULL,          /* Attach help         */
    NULL,          /* Help context        */
    NULL,          /* Description         */
    NULL           /* End                 */
};

enum console_model { m6001 = 0, m6004 = 1, m6601 = 2 };

// Hangs off the device structure
typedef struct opc_state_t
  {
    // Track progress of reads through the autoinput buffer
    unsigned char *tailp;
    unsigned char *readp;

    // Autoinput buffer pointers
    unsigned char *auto_input;
    unsigned char *autop;

    // stuff saved from the Read ASCII command
    time_t startTime;
    UNIT * unitp;

    // telnet connection to console
    uv_access console_access;

    enum console_model model;
    enum console_mode { opc_no_mode, opc_read_mode, opc_write_mode } io_mode;

    // stuff saved from the Read ASCII command
    uint tally;
    uint daddr;
    int chan;

    // ^T
    // Generate "accept" command when dial_ctl announces dialin console
    int autoaccept;
    // Replace empty console input with "@"
    int noempty;
    // ATTN flushes typeahead buffer
    int attn_flush;

    int simh_buffer_cnt;

    // Track the carrier position to allow tab expansion
    // (If the left margin is 1, then the tab stops are 11, 21, 31, 41, ...)
    int carrierPosition;

    bool echo;

    bool attn_pressed;
    bool simh_attn_pressed;

    bool bcd;

    // Handle escape sequence
    bool escapeSequence;

    char device_name [MAX_DEV_NAME_LEN];

// Multics does console reads with a tally of 64 words; so 256 characters + NUL.
// If the tally is smaller than the contents of the buffer, sendConsole will
// issue a warning and discard the excess.
#define bufsize 257
    unsigned char keyboardLineBuffer[bufsize];
    bool tabStops [bufsize];

#define simh_buffer_sz 4096
    char simh_buffer[simh_buffer_sz];
 } opc_state_t;

static opc_state_t console_state[N_OPC_UNITS_MAX];

static char * bcd_code_page =
  "01234567"
  "89[#@;>?"
  " ABCDEFG"
  "HI&.](<\\"
  "^JKLMNOP"
  "QR-$*);'"
  "+/STUVWX"
  "YZ_,%=\"!";

//
// Typeahead buffer
//

#ifndef TA_BUFFER_SIZE
# define TA_BUFFER_SIZE 65536
#endif

static int ta_buffer[TA_BUFFER_SIZE];
static uint ta_cnt  = 0;
static uint ta_next = 0;
static bool ta_ovf  = false;

static void ta_flush (void)
  {
    ta_cnt = ta_next = 0;
    ta_ovf = false;
  }

static void ta_push (int c)
  {
    // discard overflow
    if (ta_cnt >= TA_BUFFER_SIZE)
      {
        if (! ta_ovf)
          sim_print ("typeahead buffer overflow");
        ta_ovf = true;
        return;
      }
    ta_buffer [ta_cnt ++] = c;
  }

static int ta_peek (void)
  {
    if (ta_next >= ta_cnt)
      return SCPE_OK;
    int c = ta_buffer[ta_next];
    return c;
  }

static int ta_get (void)
  {
    if (ta_next >= ta_cnt)
      return SCPE_OK;
    int c = ta_buffer[ta_next ++];
    if (ta_next >= ta_cnt)
      ta_flush ();
    return c;
  }

static t_stat opc_reset (UNUSED DEVICE * dptr)
  {
    for (uint i = 0; i < N_OPC_UNITS_MAX; i ++)
      {
        console_state[i].io_mode         = opc_no_mode;
        console_state[i].tailp           = console_state[i].keyboardLineBuffer;
        console_state[i].readp           = console_state[i].keyboardLineBuffer;
        console_state[i].carrierPosition = 1;
        memset (console_state[i].tabStops, 0, sizeof (console_state[i].tabStops));
        console_state[i].escapeSequence  = false;
      }
    return SCPE_OK;
  }

int check_attn_key (void)
  {
    for (uint i = 0; i < opc_dev.numunits; i ++)
      {
        opc_state_t * csp = console_state + i;
        if (csp->attn_pressed)
          {
             csp->attn_pressed = false;
             sim_usleep (1000);
             return (int) i;
          }
      }
    return -1;
  }

// Once-only initialization

void console_init (void)
  {
    opc_reset (& opc_dev);
    for (uint i = 0; i < N_OPC_UNITS_MAX; i ++)
      {
        opc_state_t * csp      = console_state + i;
        csp->model             = m6001;
        csp->auto_input        = NULL;
        csp->autop             = NULL;
        csp->attn_pressed      = false;
        csp->simh_attn_pressed = false;
        csp->simh_buffer_cnt   = 0;
        strcpy (csp->console_access.pw, "MulticsRulez");

        csp->autoaccept      = 0;
        csp->noempty         = 0;
        csp->attn_flush      = 1;
        csp->carrierPosition = 1;
        csp->escapeSequence  = 1;
        memset (csp->tabStops, 0, sizeof (csp->tabStops));
      }
  }

// Once-only shutdown

void console_exit (void) {
  for (uint i = 0; i < N_OPC_UNITS_MAX; i ++) {
    opc_state_t * csp = console_state + i;
    if (csp->auto_input) {
      FREE (csp->auto_input);
      csp->auto_input = NULL;
    }
    if (csp->console_access.telnetp) {
      sim_warn ("console_exit freeing console %u telnetp %p\r\n", i, csp->console_access.telnetp);
      telnet_free (csp->console_access.telnetp);
      csp->console_access.telnetp = NULL;
    }
  }
}

static int opc_autoinput_set (UNIT * uptr, UNUSED int32 val,
                                const char *  cptr, UNUSED void * desc)
  {
    int devUnitIdx = (int) OPC_UNIT_IDX (uptr);
    opc_state_t * csp = console_state + devUnitIdx;

    if (cptr)
      {
        unsigned char * new = (unsigned char *) strdupesc (cptr);
        if (csp-> auto_input)
          {
            size_t nl = strlen ((char *) new);
            size_t ol = strlen ((char *) csp->auto_input);

            unsigned char * old = realloc (csp->auto_input, nl + ol + 1);
            if (!old)
              {
                fprintf(stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                        __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# ifdef SIGUSR2
                (void)raise(SIGUSR2);
                /*NOTREACHED*/ /* unreachable */
# endif /* ifdef SIGUSR2 */
#endif /* if defined(USE_BACKTRACE) */
                abort();
              }
            strcpy ((char *) old + ol, (char *) new);
            csp->auto_input = old;
            FREE (new);
          }
        else
          csp->auto_input = new;
      }
    else
      {
        if (csp->auto_input)
          FREE (csp->auto_input);
        csp->auto_input = NULL;
      }
    csp->autop = csp->auto_input;
    return SCPE_OK;
  }

int clear_opc_autoinput (int32 flag, UNUSED const char * cptr)
  {
    opc_state_t * csp = console_state + flag;
    if (csp->auto_input)
      FREE (csp->auto_input);
    csp->auto_input = NULL;
    csp->autop = csp->auto_input;
    return SCPE_OK;
  }

int add_opc_autoinput (int32 flag, const char * cptr)
  {
    opc_state_t * csp = console_state + flag;
    unsigned char * new = (unsigned char *) strdupesc (cptr);
    if (csp->auto_input)
      {
        size_t nl = strlen ((char *) new);
        size_t ol = strlen ((char *) csp->auto_input);

        unsigned char * old = realloc (csp->auto_input, nl + ol + 1);
        if (!old)
          {
            fprintf(stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                    __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# ifdef SIGUSR2
            (void)raise(SIGUSR2);
            /*NOTREACHED*/ /* unreachable */
# endif /* ifdef SIGUSR2 */
#endif /* if defined(USE_BACKTRACE) */
            abort();
          }
        strcpy ((char *) old + ol, (char *) new);
        csp->auto_input = old;
        FREE (new);
      }
    else
      csp->auto_input = new;
    csp->autop = csp->auto_input;
    return SCPE_OK;
  }

static int opc_autoinput_show (UNUSED FILE * st, UNIT * uptr,
                                 UNUSED int val, UNUSED const void * desc)
  {
    int conUnitIdx = (int) OPC_UNIT_IDX (uptr);
    opc_state_t * csp = console_state + conUnitIdx;
    if (csp->auto_input)
      sim_print ("autoinput: '%s'", csp->auto_input);
    else
      sim_print ("autoinput: empty");
    return SCPE_OK;
  }

static t_stat console_attn (UNUSED UNIT * uptr);

static UNIT attn_unit[N_OPC_UNITS_MAX] = {
#ifdef NO_C_ELLIPSIS
  { UDATA (& console_attn, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (& console_attn, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (& console_attn, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (& console_attn, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (& console_attn, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (& console_attn, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (& console_attn, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (& console_attn, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL }
#else
  [0 ... N_OPC_UNITS_MAX - 1] = {
    UDATA (& console_attn, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL
  }
#endif
};

static t_stat console_attn (UNUSED UNIT * uptr)
  {
    uint con_unit_idx  = (uint) (uptr - attn_unit);
    uint ctlr_port_num = 0; // Consoles are single ported
    uint iom_unit_idx  = cables->opc_to_iom[con_unit_idx][ctlr_port_num].iom_unit_idx;
    uint chan_num      = cables->opc_to_iom[con_unit_idx][ctlr_port_num].chan_num;
    uint dev_code      = 0; // Only a single console on the controller

    send_special_interrupt (iom_unit_idx, chan_num, dev_code, 0, 0);
    return SCPE_OK;
  }

void console_attn_idx (int conUnitIdx)
  {
    console_attn (attn_unit + conUnitIdx);
  }

#ifndef __MINGW64__
# ifndef __MINGW32__
static struct termios ttyTermios;
static bool ttyTermiosOk = false;

static void newlineOff (void)
  {
    if (! isatty (0))
      return;
    if (! ttyTermiosOk)
      {
        int rc = tcgetattr (0, & ttyTermios); /* get old flags */
        if (rc)
           return;
        ttyTermiosOk = true;
      }
    struct termios runtty;
    runtty = ttyTermios;
    runtty.c_oflag &= (unsigned int) ~OPOST; /* no output edit */
#  if defined(__ANDROID__)
#   define TCSA_TYPE TCSANOW
    (void)fflush(stdout);
    (void)fflush(stderr);
#  else
#   define TCSA_TYPE TCSAFLUSH
#  endif
    tcsetattr (0, TCSA_TYPE, & runtty);
  }

static void newlineOn (void)
  {
    if (! isatty (0))
      return;
    if (! ttyTermiosOk)
      return;
#  if defined(__ANDROID__)
    (void)fflush(stdout);
    (void)fflush(stderr);
#  endif
    tcsetattr (0, TCSA_TYPE, & ttyTermios);
  }
# endif /* ifndef __MINGW32__ */
#endif /* ifndef __MINGW64__ */

static void handleRCP (uint con_unit_idx, char * text)
  {
// It appears that Cygwin doesn't grok "%ms"
#if 0
    char * label = NULL;
    char * with = NULL;
    char * drive = NULL;
// 1750.1  RCP: Mount Reel 12.3EXEC_CF0019_1 without ring on tapa_01
    int rc = sscanf (text, "%*d.%*d RCP: Mount Reel %ms %ms ring on %ms",
                & label, & with, & drive);
#endif
    size_t len = strlen (text);
    char   label [len + 1];
    char   with  [len + 1];
    char   drive [len + 1];
    int rc = sscanf (text, "%*d.%*d RCP: Mount Reel %s %s ring on %s",
                label, with, drive);
    if (rc == 3)
      {
        bool withring = (strcmp (with, "with") == 0);
        char labelDotTap[strlen (label) + strlen (".tap") + 1];
        strcpy (labelDotTap, label);
        strcat (labelDotTap, ".tap");
        attachTape (labelDotTap, withring, drive);
        return;
      }

    rc = sscanf (text, "%*d.%*d RCP: Remount Reel %s %s ring on %s",
                label, with, drive);
    if (rc == 3)
      {
        bool withring = (strcmp (with, "with") == 0);
        char labelDotTap [strlen (label) + strlen (".tap") + 1];
        strcpy (labelDotTap, label);
        strcat (labelDotTap, ".tap");
        attachTape (labelDotTap, withring, drive);
        return;
      }

//  1629  as   dial_ctl_: Channel d.h000 dialed to Initializer

    if (console_state[con_unit_idx].autoaccept)
      {
        rc = sscanf (text, "%*d  as   dial_ctl_: Channel %s dialed to Initializer",
                     label);
        if (rc == 1)
          {
            //sim_printf (" dial system <%s>\r\n", label);
            opc_autoinput_set (opc_unit + con_unit_idx, 0, "accept ", NULL);
            opc_autoinput_set (opc_unit + con_unit_idx, 0, label, NULL);
            opc_autoinput_set (opc_unit + con_unit_idx, 0, "\r", NULL);
// XXX This is subject to race conditions
            if (console_state[con_unit_idx].io_mode != opc_read_mode)
              console_state[con_unit_idx].attn_pressed = true;
            return;
          }
      }
  }

// Send entered text to the IOM.
static void sendConsole (int conUnitIdx, word12 stati)
  {
    opc_state_t * csp   = console_state + conUnitIdx;
    uint tally          = csp->tally;
    uint ctlr_port_num  = 0; // Consoles are single ported
    uint iomUnitIdx     = cables->opc_to_iom[conUnitIdx][ctlr_port_num].iom_unit_idx;
    uint chan_num       = cables->opc_to_iom[conUnitIdx][ctlr_port_num].chan_num;
    iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan_num];

    //ASSURE (csp->io_mode == opc_read_mode);
    if (csp->io_mode != opc_read_mode)
      {
        sim_warn ("%s called with io_mode != opc_read_mode (%d)\n",
                  __func__, csp->io_mode);
        return;
      }

    uint n_chars = (uint) (csp->tailp - csp->readp);
    uint n_words;
    if (csp->bcd)
        // + 2 for the !1 newline
        n_words = ((n_chars+2) + 5) / 6;
      else
        n_words = (n_chars + 3) / 4;
    // The "+1" is for them empty line case below
    word36 buf[n_words + 1];
    // Make Oracle lint happy
    memset (buf, 0, sizeof (word36) * (n_words + 1));
    word36 * bufp = buf;

    // Multics doesn't seem to like empty lines; it the line buffer
    // is empty and there is room in the I/O buffer, send a line kill.
    if ((!csp->bcd) && csp->noempty && n_chars == 0 && tally)
      {
        n_chars = 1;
        n_words = 1;
        putbits36_9 (bufp, 0, '@');
        tally --;
      }
    else
      {
        int bcd_nl_state = 0;
        while (tally && csp->readp < csp->tailp)
          {
            if (csp->bcd)
              {
                //* bufp = 0171717171717ul;
                //* bufp = 0202020202020ul;
                * bufp = 0;
                for (uint charno = 0; charno < 4; ++ charno)
                  {
                    unsigned char c;
                    if (csp->readp >= csp->tailp)
                      {
                        if (bcd_nl_state == 0)
                          {
                            c = '!';
                            bcd_nl_state = 1;
                          }
                        else if (bcd_nl_state == 1)
                          {
                            c = '1';
                            bcd_nl_state = 2;
                          }
                        else
                          break;
                      }
                    else
                      c = (unsigned char) (* csp->readp ++);
                    c = (unsigned char) toupper (c);
                    int i;
                    for (i = 0; i < 64; i ++)
                      if (bcd_code_page[i] == c)
                        break;
                    if (i >= 64)
                      {
                        sim_warn ("Character %o does not map to BCD; replacing with '?'\n", c);
                        i = 017; //-V536
                      }
                    putbits36_6 (bufp, charno * 6, (word6) i);
                  }
              }
            else
              {
                * bufp = 0ul;
                for (uint charno = 0; charno < 4; ++ charno)
                  {
                    if (csp->readp >= csp->tailp)
                      break;
                    unsigned char c = (unsigned char) (* csp->readp ++);
                    c &= 0177;  // Multics get consternated about this
                    putbits36_9 (bufp, charno * 9, c);
                  }
              }
            bufp ++;
            tally --;
          }
        if (csp->readp < csp->tailp)
          {
            sim_warn ("opc_iom_io: discarding %d characters from end of line\n",
                      (int) (csp->tailp - csp->readp));
          }
      }

    iom_indirect_data_service (iomUnitIdx, chan_num, buf, & n_words, true);

    p->charPos = n_chars % 4;
    p->stati   = (word12) stati;

    csp->readp   = csp->keyboardLineBuffer;
    csp->tailp   = csp->keyboardLineBuffer;
    csp->io_mode = opc_no_mode;

    send_terminate_interrupt (iomUnitIdx, chan_num);
  }

static void console_putchar (int conUnitIdx, char ch);
static void console_putstr  (int conUnitIdx, char * str);

// Process characters entered on keyboard or autoinput
static void consoleProcessIdx (int conUnitIdx)
  {
    opc_state_t * csp = console_state + conUnitIdx;
    int c;

//// Move data from keyboard buffers into type-ahead buffer

    for (;;)
      {
        c = sim_poll_kbd ();
        if (c == SCPE_OK)
          c = accessGetChar (& csp->console_access);

        // Check for stop signaled by scp

        if (breakEnable && stop_cpu)
          {
            console_putstr (conUnitIdx,  " Got <sim stop> \r\n");
            return;
          }

        // Check for ^E
        //   (Windows doesn't handle ^E as a signal; need to explicitly test
        //   for it.)

        if (breakEnable && c == SCPE_STOP)
          {
            console_putstr (conUnitIdx,  " Got <sim stop> \r\n");
            stop_cpu = 1;
            return; // User typed ^E to stop simulation
          }

        // Check for scp break

        if (breakEnable && c == SCPE_BREAK)
          {
            console_putstr (conUnitIdx,  " Got <sim stop> \r\n");
            stop_cpu = 1;
            return; // User typed ^E to stop simulation
          }

        // End of available input

        if (c == SCPE_OK)
          break;

        // sanity test

        if (c < SCPE_KFLAG)
          {
            sim_printf ("impossible %d %o\n", c, c);
            continue; // Should be impossible
          }

        // translate to ascii

        int ch = c - SCPE_KFLAG;

        // XXX This is subject to race conditions
        // If the console is not in read mode and ESC or ^A
        // is pressed, signal Multics for ATTN.
        if (csp->io_mode != opc_read_mode)
          {
            if (ch == '\033' || ch == '\001') // escape or ^A
              {
                if (csp->attn_flush)
                  ta_flush ();
                csp->attn_pressed = true;
                continue;
              }
          }

#if 0
        // If Multics has gone seriously awry (eg crash
        // to BCE during boot, the autoinput will become
        // wedged waiting for the expect string 'Ready'
        // Allow the user to signal ATTN when wedged
        // by checking to see if we are waiting on an expect string

        if ((ch == '\033' || ch == '\001') &&  // ESC or ^A
            csp->autop && (*csp->autop == 030 || *csp->autop == 031)) // ^X ^Y
          {
            // User pressed ATTN while expect waiting
            // Clear the autoinput buffer; these will cancel the
            // expect wait and any remaining script, returning
            // control of the console to the user.
            // Assuming opc0.
            clear_opc_autoinput (con_unit_idx, NULL);
            ta_flush ();
            sim_printf ("\r\nAutoinput and typeahead buffers flushed\r\n");
            continue;
          }
#endif
        // ^S

        if (ch == 023) // ^S scp command
          {
            if (! csp->simh_attn_pressed)
              {
                ta_flush ();
                csp->simh_attn_pressed = true;
                csp->simh_buffer_cnt = 0;
                console_putstr (conUnitIdx, "^S\r\n" SIM_NAME "> ");
              }
            continue;
          }

//// ^P  Prompt

        // ATTN is ambiguous; in read mode, it cancels the read.
        // In write mode, is discards output and asks for a prompt.

        // ^P will check the mode, and if it is not input, will do an
        // ATTN signal. There is still a small race window; if Multics
        // is in the process of starting read mode, it may end up canceling
        // the read.

        if (ch == 020) { // ^P
          sim_usleep (1000);
          if (csp->io_mode != opc_read_mode) {
            if (csp->attn_flush)
              ta_flush ();
            csp->attn_pressed = true;
          }
          continue;
        }

//// ^T

        if (ch == 024) // ^T
          {
            char buf[256];
            char cms[3] = "?RW";
            // XXX Assumes console 0
            sprintf (buf, "^T attn %c %c cnt %d next %d\r\n",
                     console_state[0].attn_pressed+'0',
                     cms[console_state[0].io_mode],
                     ta_cnt, ta_next);
            console_putstr (conUnitIdx, buf);
            continue;
          }

//// In ^S mode (accumulating a scp command)?

        if (csp->simh_attn_pressed)
          {
            ta_get ();
            if (ch == '\177' || ch == '\010')  // backspace/del
              {
                if (csp->simh_buffer_cnt > 0)
                  {
                    -- csp->simh_buffer_cnt;
                    csp->simh_buffer[csp->simh_buffer_cnt] = 0;
                    console_putstr (conUnitIdx,  "\b \b");
                  }
                return;
              }

            //// scp ^R

            if (ch == '\022')  // ^R
              {
                console_putstr (conUnitIdx, "^R\r\n" SIM_NAME "> ");
                for (int i = 0; i < csp->simh_buffer_cnt; i ++)
                  console_putchar (conUnitIdx, (char) (csp->simh_buffer[i]));
                return;
              }

            //// scp ^U

            if (ch == '\025')  // ^U
              {
                console_putstr (conUnitIdx, "^U\r\n" SIM_NAME "> ");
                csp->simh_buffer_cnt = 0;
                return;
              }

            //// scp CR/LF

            if (ch == '\012' || ch == '\015')  // CR/LF
              {
                console_putstr (conUnitIdx,  "\r\n");
                csp->simh_buffer[csp->simh_buffer_cnt] = 0;

                char * cptr = csp->simh_buffer;
                char gbuf[simh_buffer_sz];
                cptr = (char *) get_glyph (cptr, gbuf, 0); /* get command glyph */
                if (strlen (gbuf))
                  {
                    CTAB *cmdp;
                    if ((cmdp = find_cmd (gbuf))) /* lookup command */
                      {
                        t_stat stat = cmdp->action (cmdp->arg, cptr);
                           /* if found, exec */
                        if (stat != SCPE_OK)
                          {
                            char buf[4096];
                            sprintf (buf, "\r%s returned %d '%s'\r\n",
                              SIM_NAME, stat, sim_error_text (stat));
                            console_putstr (conUnitIdx,  buf);
                          }
                      }
                    else
                       console_putstr (conUnitIdx,
                         "\rUnrecognized " SIM_NAME " command.\r\n");
                  }
                csp->simh_buffer_cnt   = 0;
                csp->simh_buffer[0]    = 0;
                csp->simh_attn_pressed = false;
                return;
              }

            //// scp ESC/^D/^Z

            if (ch == '\033' || ch == '\004' || ch == '\032')  // ESC/^D/^Z
              {
                console_putstr (conUnitIdx,  "\r\n" SIM_NAME " cancel\r\n");
                // Empty input buffer
                csp->simh_buffer_cnt   = 0;
                csp->simh_buffer[0]    = 0;
                csp->simh_attn_pressed = false;
                return;
              }

            //// scp isprint?

            if (isprint (ch))
              {
                // silently drop buffer overrun
                if (csp->simh_buffer_cnt + 1 >= simh_buffer_sz)
                  return;
                csp->simh_buffer[csp->simh_buffer_cnt ++] = (char) ch;
                console_putchar (conUnitIdx, (char) ch);
                return;
              }
            return;
          } // if (simh_attn_pressed)

        // Save the character

        ta_push (c);
      }

//// Check for stop signaled by scp

    if (breakEnable && stop_cpu)
      {
        console_putstr (conUnitIdx,  " Got <sim stop> \r\n");
        return;
      }

//// Console is reading and autoinput is ready
////   Move line of text from autoinput buffer to console buffer

    if (csp->io_mode == opc_read_mode &&
        csp->autop != NULL)
      {
        int announce = 1;
        for (;;)
          {
            if (csp->tailp >= csp->keyboardLineBuffer + sizeof (csp->keyboardLineBuffer))
             {
                sim_warn ("getConsoleInput: Buffer full; flushing autoinput.\n");
                sendConsole (conUnitIdx, 04000); // Normal status
                return;
              }
            unsigned char c = * (csp->autop);
            if (c == 4) // eot
              {
                FREE (csp->auto_input);
                csp->auto_input = NULL;
                csp->autop      = NULL;
                // Empty input buffer
                csp->readp      = csp->keyboardLineBuffer;
                csp->tailp      = csp->keyboardLineBuffer;
                sendConsole (conUnitIdx, 04310); // Null line, status operator
                                                 // distracted
                console_putstr (conUnitIdx,  "CONSOLE: RELEASED\r\n");
                sim_usleep (1000);
                return;
              }
            if (c == 030 || c == 031) // ^X ^Y
              {
                // an expect string is in the autoinput buffer; wait for it
                // to be processed
                return;
              }
            if (c == 0)
              {
                FREE (csp->auto_input);
                csp->auto_input = NULL;
                csp->autop      = NULL;
                goto eol;
              }
            if (announce)
              {
                console_putstr (conUnitIdx,  "[auto-input] ");
                announce = 0;
              }
            csp->autop ++;

            if (c == '\012' || c == '\015')
              {
eol:
                if (csp->echo)
                  console_putstr (conUnitIdx,  "\r\n");
                sendConsole (conUnitIdx, 04000); // Normal status
                return;
              }
            else
              {
                * csp->tailp ++ = c;
                if (csp->echo)
                  console_putchar (conUnitIdx, (char) c);
              }
          } // for (;;)
      } // if (autop)

//// Read mode and nothing in console buffer
////   Check for timeout

    if (csp->io_mode == opc_read_mode &&
        csp->tailp == csp->keyboardLineBuffer)
      {
        if (csp->startTime + 30 <= time (NULL))
          {
            console_putstr (conUnitIdx,  "CONSOLE: TIMEOUT\r\n");
            sim_usleep (1000);
            csp->readp = csp->keyboardLineBuffer;
            csp->tailp = csp->keyboardLineBuffer;
            sendConsole (conUnitIdx, 04310); // Null line, status operator
                                             // distracted
          }
      }

//// Peek at the character in the typeahead buffer

    c = ta_peek ();

    // No data
    if (c == SCPE_OK)
        return;

    // Convert from scp encoding to ASCII
    if (c < SCPE_KFLAG)
      {
        sim_printf ("impossible %d %o\n", c, c);
        return; // Should be impossible
      }

    // translate to ascii

    int ch = c - SCPE_KFLAG;

// XXX This is subject to race conditions
    if (csp->io_mode != opc_read_mode)
      {
        if (ch == '\033' || ch == '\001') // escape or ^A
          {
            ta_get ();
            csp->attn_pressed = true;
          }
        return;
      }

    if (ch == '\177' || ch == '\010')  // backspace/del
      {
        ta_get ();
        if (csp->tailp > csp->keyboardLineBuffer)
          {
            * csp->tailp = 0;
            -- csp->tailp;
            if (csp->echo)
              console_putstr (conUnitIdx,  "\b \b");
          }
        return;
      }

    if (ch == '\022')  // ^R
      {
        ta_get ();
        if (csp->echo)
          {
            console_putstr (conUnitIdx,  "^R\r\n");
            for (unsigned char * p = csp->keyboardLineBuffer; p < csp->tailp; p ++)
              console_putchar (conUnitIdx, (char) (*p));
            return;
          }
      }

    if (ch == '\025')  // ^U
      {
        ta_get ();
        console_putstr (conUnitIdx,  "^U\r\n");
        csp->tailp = csp->keyboardLineBuffer;
        return;
      }

    if (ch == '\030')  // ^X
      {
        ta_get ();
        console_putstr (conUnitIdx,  "^X\r\n");
        csp->tailp = csp->keyboardLineBuffer;
        return;
      }

    if (ch == '\012' || ch == '\015')  // CR/LF
      {
        ta_get ();
        if (csp->echo)
          console_putstr (conUnitIdx,  "\r\n");
        sendConsole (conUnitIdx, 04000); // Normal status
        return;
      }

    if (ch == '\033' || ch == '\004' || ch == '\032')  // ESC/^D/^Z
      {
        ta_get ();
        console_putstr (conUnitIdx,  "\r\n");
        // Empty input buffer
        csp->readp = csp->keyboardLineBuffer;
        csp->tailp = csp->keyboardLineBuffer;
        sendConsole (conUnitIdx, 04310); // Null line, status operator
                                         // distracted
        console_putstr (conUnitIdx,  "CONSOLE: RELEASED\n");
        sim_usleep (1000);
        return;
      }

    if (isprint (ch))
      {
        // silently drop buffer overrun
        ta_get ();
        if (csp->tailp >= csp->keyboardLineBuffer + sizeof (csp->keyboardLineBuffer))
          return;

        * csp->tailp ++ = (unsigned char) ch;
        if (csp->echo)
          console_putchar (conUnitIdx, (char) ch);
        return;
      }
    // ignore other chars...
    ta_get ();
    return;
  }

void consoleProcess (void)
  {
    for (int conUnitIdx = 0; conUnitIdx < (int) opc_dev.numunits; conUnitIdx ++)
      consoleProcessIdx (conUnitIdx);
  }

/*
 * opc_iom_cmd ()
 *
 * Handle a device command.  Invoked by the IOM while processing a PCW
 * or IDCW.
 */

iom_cmd_rc_t opc_iom_cmd (uint iomUnitIdx, uint chan) {
  iom_cmd_rc_t rc = IOM_CMD_PROCEED;

#ifdef LOCKLESS
  lock_libuv ();
#endif

  iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan];
  uint con_unit_idx   = get_ctlr_idx (iomUnitIdx, chan);
  UNIT * unitp        = & opc_unit[con_unit_idx];
  opc_state_t * csp   = console_state + con_unit_idx;

  p->dev_code = p->IDCW_DEV_CODE;
  p->stati = 0;
  //int conUnitIdx = (int) d->devUnitIdx;

  // The 6001 only executes the PCW DCW command; the 6601 executes
  // the PCW DCW and (at least) the first DCW list item.
  // When Multics uses the 6601, the PCW DCW is always 040 RESET.
  // The 040 RESET will trigger the DCW list read.
  // will change this.

  // IDCW?
  if (IS_IDCW (p)) {
    // IDCW

    switch (p->IDCW_DEV_CMD) {
      case 000: // CMD 00 Request status
        sim_debug (DBG_DEBUG, & opc_dev, "%s: Status request\n", __func__);
        csp->io_mode = opc_no_mode;
        p->stati     = 04000;
        break;

      case 003:               // Read BCD
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Read BCD echoed\n", __func__);
        csp->io_mode = opc_read_mode;
        p->recordResidue --;
        csp->echo    = true;
        csp->bcd     = true;
        p->stati     = 04000;
        break;

      case 013:               // Write BCD
        sim_debug (DBG_DEBUG, & opc_dev, "%s: Write BCD\n", __func__);
        p->isRead    = false;
        csp->bcd     = true;
        csp->io_mode = opc_write_mode;
        p->recordResidue --;
        p->stati     = 04000;
        break;

      case 023:               // Read ASCII
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Read ASCII echoed\n", __func__);
        csp->io_mode = opc_read_mode;
        p->recordResidue --;
        csp->echo    = true;
        csp->bcd     = false;
        p->stati     = 04000;
        break;

      case 033:               // Write ASCII
        sim_debug (DBG_DEBUG, & opc_dev, "%s: Write ASCII\n", __func__);
        p->isRead    = false;
        csp->bcd     = false;
        csp->io_mode = opc_write_mode;
        p->recordResidue --;
        p->stati     = 04000;
        break;

// Model 6001 vs. 6601.
//
// When Multics switches to 6601 mode, the PCW DCW is always 040 RESET; the
// bootload and 6001 code never does that. Therefore, we use the 040
// as an indication that the DCW list should be processed.
// All of the other device commands will return IOM_CMD_DISCONNECT, stopping
// parsing of the channel command program. This one will return IOM_CMD_PROCEED,
// causing the parser to move to the DCW list.

      case 040:               // Reset
        sim_debug (DBG_DEBUG, & opc_dev, "%s: Reset\n", __func__);
        p->stati = 04000;
        // T&D probing
        //if (p->IDCW_DEV_CODE == 077) {
          // T&D uses dev code 77 to test for the console device;
          // it ignores dev code, and so returns OK here.
          //p->stati = 04502; // invalid device code
          // if (p->IDCW_CHAN_CTRL == 0) { sim_warn ("%s: TERMINATE_BUG\n", __func__); return IOM_CMD_DISCONNECT; }
        //}
        break;

      case 043:               // Read ASCII unechoed
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Read ASCII unechoed\n", __func__);
        csp->io_mode = opc_read_mode;
        p->recordResidue --;
        csp->echo    = false;
        csp->bcd     = false;
        p->stati     = 04000;
        break;

      case 051:               // Write Alert -- Ring Bell
        sim_debug (DBG_DEBUG, & opc_dev, "%s: Alert\n", __func__);
        p->isRead = false;
        console_putstr ((int) con_unit_idx,  "CONSOLE: ALERT\r\n");
        console_putchar ((int) con_unit_idx, '\a');
        p->stati  = 04000;
        sim_usleep (1000);
        if (csp->model == m6001 && p->isPCW) {
          rc = IOM_CMD_DISCONNECT;
          goto done;
        }
        break;

      case 057:               // Read ID (according to AN70-1)
        // FIXME: No support for Read ID; appropriate values are not known
        //[CAC] Looking at the bootload console code, it seems more
        // concerned about the device responding, rather than the actual
        // returned value. Make some thing up.
        sim_debug (DBG_DEBUG, & opc_dev, "%s: Read ID\n", __func__);
        p->stati = 04500;
        if (csp->model == m6001 && p->isPCW) {
          rc = IOM_CMD_DISCONNECT;
          goto done;
        }
        break;

      case 060:               // LOCK MCA
        sim_debug (DBG_DEBUG, & opc_dev, "%s: Lock\n", __func__);
        console_putstr ((int) con_unit_idx,  "CONSOLE: LOCK\r\n");
        p->stati = 04000;
        sim_usleep (1000);
        break;

      case 063:               // UNLOCK MCA
        sim_debug (DBG_DEBUG, & opc_dev, "%s: Unlock\n", __func__);
        console_putstr ((int) con_unit_idx,  "CONSOLE: UNLOCK\r\n");
        p->stati = 04000;
        sim_usleep (1000);
        break;

      default:
        sim_debug (DBG_DEBUG, & opc_dev, "%s: Unknown command 0%o\n", __func__, p->IDCW_DEV_CMD);
        p->stati = 04501; // command reject, invalid instruction code
        rc = IOM_CMD_ERROR;
        goto done;
    } // switch IDCW_DEV_CMD
    goto done;
  } // IDCW

  // Not IDCW; TDCW are captured in IOM, so must be IOTD or IOTP
  switch (csp->io_mode) {
    case opc_no_mode:
      sim_warn ("%s: Unexpected IOTx\n", __func__);
      rc = IOM_CMD_ERROR;
      goto done;

    case opc_read_mode: {
        if (csp->tailp != csp->keyboardLineBuffer) {
          sim_warn ("%s: Discarding previously buffered input.\n", __func__);
        }
        uint tally = p->DDCW_TALLY;
        uint daddr = p->DDCW_ADDR;

        if (tally == 0) {
          tally = 4096;
        }

        csp->tailp     = csp->keyboardLineBuffer;
        csp->readp     = csp->keyboardLineBuffer;
        csp->startTime = time (NULL);
        csp->tally     = tally;
        csp->daddr     = daddr;
        csp->unitp     = unitp;
        csp->chan      = (int) chan;

        // If Multics has gone seriously awry (eg crash
        // to BCE during boot), the autoinput will become
        // wedged waiting for the expect string 'Ready'.
        // We just went to read mode; if we are waiting
        // on an expect string, it is never coming because
        // console access is blocked by the expect code.
        // Throw out the script if this happens....

        // If there is autoinput and it is at ^X or ^Y
        if (csp->autop && (*csp->autop == 030 || *csp->autop == 031)) { // ^X ^Y
          // We are wedged.
          // Clear the autoinput buffer; this will cancel the
          // expect wait and any remaining script, returning
          // control of the console to the user.
          // Assuming opc0.
          clear_opc_autoinput (ASSUME0, NULL);
          ta_flush ();
          sim_printf \
              ("\r\nScript wedged and abandoned; autoinput and typeahead buffers flushed\r\n");
        }
        rc = IOM_CMD_PENDING; // command in progress; do not send terminate interrupt
        goto done;
      } // case opc_read_mode

    case opc_write_mode: {
        uint tally = p->DDCW_TALLY;

// We would hope that number of valid characters in the last word
// would be in DCW_18_20_CP, but it seems to reliably be zero.

        if (tally == 0) {
          tally = 4096;
        }

        word36 buf[tally];
        iom_indirect_data_service (iomUnitIdx, chan, buf, & tally, false);
        p->initiate = false;

#if 0
  sim_printf ("\r\n");
  for (uint i = 0; i < tally; i ++) {
    sim_printf ("%012llo  \"", buf[i]);
    for (uint j = 0; j < 36; j += 9) {
      word9 ch = getbits36_9 (buf[i], j);
      if (ch < 256 && isprint ((char) ch))
        sim_printf ("%c", ch);
      else
        sim_printf ("\\%03o", ch);
    }
   sim_printf ("\"\r\n");
  }
  sim_printf ("\r\n");
#endif
#if 0
if (csp->bcd) {
  sim_printf ("\r\n");
  for (uint i = 0; i < tally; i ++) {
    sim_printf ("%012llo  \"", buf[i]);
    for (uint j = 0; j < 36; j += 6) {
      word6 ch = getbits36_6 (buf[i], j);
      sim_printf ("%c", bcd_code_page[ch]);
    }
   sim_printf ("\"\r\n");
  }
  sim_printf ("\r\n");
}
#endif

        // Tally is in words, not chars.
        char text[tally * 4 + 1];
        char * textp = text;
        word36 * bufp = buf;
        * textp = 0;
#ifndef __MINGW64__
        newlineOff ();
#endif
        // 0 == no escape character seen
        // 1 ==  ! seen
        // 2 == !! seen
        int escape_cnt = 0;

        while (tally) {
          word36 datum = * bufp ++;
          tally --;
          if (csp->bcd) {
            // Lifted from tolts_util_.pl1:bci_to_ascii
            for (int i = 0; i < 6; i ++) {
              word36 narrow_char = datum >> 30; // slide leftmost char
                                                //  into low byte
              datum = datum << 6; // lose the leftmost char
              narrow_char &= 077;
              char ch = bcd_code_page [narrow_char];
              if (escape_cnt == 2) {
                console_putchar ((int) con_unit_idx, ch);
                * textp ++ = ch;
                escape_cnt = 0;
              } else if (ch == '!') {
                escape_cnt ++;
              } else if (escape_cnt == 1) {
                uint lp = (uint)narrow_char;
                // !0 is mapped to !1
                // !1 to !9, ![, !#, !@, !;, !>, !?    1 to 15 newlines
                if (lp == 060 /* + */ || lp == 075 /* = */) { // POLTS
                  p->stati = 04320;
                  goto done;
                }
                if (lp == 0)
                  lp = 1;
                if (lp >= 16) {
                  console_putstr ((int) con_unit_idx, "\f");
                  //* textp ++ = '\f';
                } else {
                  for (uint i = 0; i < lp; i ++) {
                    console_putstr ((int) con_unit_idx, "\r\n");
                    //* textp ++ = '\r';
                    //* textp ++ = '\n';
                  }
                }
                escape_cnt = 0;
              } else if (ch == '?') {
                escape_cnt = 0;
              } else {
                console_putchar ((int) con_unit_idx, ch);
                * textp ++ = ch;
                escape_cnt = 0;
              }
            }
          } else {
            for (int i = 0; i < 4; i ++) {
              word36 wide_char = datum >> 27; // slide leftmost char
                                              //  into low byte
              datum = datum << 9; // lose the leftmost char
              char ch = wide_char & 0x7f;
              if (ch != 0177 && ch != 0) {
                console_putchar ((int) con_unit_idx, ch);
                * textp ++ = ch;
              }
            }
          }
        }
        * textp ++ = 0;

        // autoinput expect
        if (csp->autop && * csp->autop == 030) {
          //   ^xstring\0
          //size_t expl = strlen ((char *) (csp->autop + 1));
          //   ^xstring^x
          size_t expl = strcspn ((char *) (csp->autop + 1), "\030");

          if (strncmp (text, (char *) (csp->autop + 1), expl) == 0) {
            csp->autop += expl + 2;
            sim_usleep (1000);
            sim_activate (& attn_unit[con_unit_idx], ACTIVATE_1SEC);
          }
        }
        // autoinput expect
        if (csp->autop && * csp->autop == 031) {
          //   ^ystring\0
          //size_t expl = strlen ((char *) (csp->autop + 1));
          //   ^ystring^y
          size_t expl = strcspn ((char *) (csp->autop + 1), "\031");

          char needle [expl + 1];
          strncpy (needle, (char *) csp->autop + 1, expl);
          needle [expl] = 0;
          if (strstr (text, needle)) {
            csp->autop += expl + 2;
            sim_usleep (1000);
            sim_activate (& attn_unit[con_unit_idx], ACTIVATE_1SEC);
          }
        }
        handleRCP (con_unit_idx, text);
#ifndef __MINGW64__
        newlineOn ();
#endif
        p->stati = 04000;
        goto done;
      } // case opc_write_mode
  } // switch io_mode

done:
#ifdef LOCKLESS
  unlock_libuv ();
#endif
  return rc;
} // opc_iom_cmd

static t_stat opc_svc (UNIT * unitp)
  {
    int con_unit_idx   = (int) OPC_UNIT_IDX (unitp);
    uint ctlr_port_num = 0; // Consoles are single ported
    uint iom_unit_idx  = cables->opc_to_iom[con_unit_idx][ctlr_port_num].iom_unit_idx;
    uint chan_num      = cables->opc_to_iom[con_unit_idx][ctlr_port_num].chan_num;

    opc_iom_cmd (iom_unit_idx, chan_num);
    return SCPE_OK;
  }

static t_stat opc_show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr,
                                 UNUSED int val, UNUSED const void * desc)
  {
    sim_print ("%d units\n", opc_dev.numunits);
    return SCPE_OK;
  }

static t_stat opc_set_nunits (UNUSED UNIT * uptr, int32 UNUSED value,
                                const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    int n = atoi (cptr);
    if (n < 1 || n > N_OPC_UNITS_MAX)
      return SCPE_ARG;
    opc_dev.numunits = (uint32) n;
    return SCPE_OK;
  }

static config_value_list_t cfg_on_off[] =
  {
    { "off",     0 },
    { "on",      1 },
    { "disable", 0 },
    { "enable",  1 },
    { NULL,      0 }
  };

static config_value_list_t cfg_model[] =
  {
    { "m6001", m6001 },
    { "m6004", m6004 },
    { "m6601", m6601 },
    { NULL,    0 }
  };

static config_list_t opc_config_list[] =
  {
   { "autoaccept", 0, 1, cfg_on_off },
   { "noempty",    0, 1, cfg_on_off },
   { "attn_flush", 0, 1, cfg_on_off },
   { "model",      1, 0, cfg_model },
   { NULL,         0, 0, NULL }
  };

static t_stat opc_set_config (UNUSED UNIT *  uptr, UNUSED int32 value,
                              const char * cptr, UNUSED void * desc)
  {
    int devUnitIdx           = (int) OPC_UNIT_IDX (uptr);
    opc_state_t * csp        = console_state + devUnitIdx;
// XXX Minor bug; this code doesn't check for trailing garbage
    config_state_t cfg_state = { NULL, NULL };

    for (;;)
      {
        int64_t v;
        int rc = cfg_parse (__func__, cptr, opc_config_list,
                           & cfg_state, & v);
        if (rc == -1) // done
          break;

        if (rc == -2) // error
          {
            cfg_parse_done (& cfg_state);
            return SCPE_ARG;
          }
        const char * p = opc_config_list[rc].name;

        if (strcmp (p, "autoaccept") == 0)
          {
            csp->autoaccept = (int) v;
            continue;
          }

        if (strcmp (p, "noempty") == 0)
          {
            csp->noempty = (int) v;
            continue;
          }

        if (strcmp (p, "attn_flush") == 0)
          {
            csp->attn_flush = (int) v;
            continue;
          }

        if (strcmp (p, "model") == 0)
          {
            csp->model = (enum console_model) v;
            continue;
          }

        sim_warn ("error: opc_set_config: Invalid cfg_parse rc <%d>\n",
                  rc);
        cfg_parse_done (& cfg_state);
        return SCPE_ARG;
      } // process statements
    cfg_parse_done (& cfg_state);
    return SCPE_OK;
  }

static t_stat opc_show_config (UNUSED FILE * st, UNUSED UNIT * uptr,
                               UNUSED int  val, UNUSED const void * desc)
  {
    int devUnitIdx    = (int) OPC_UNIT_IDX (uptr);
    opc_state_t * csp = console_state + devUnitIdx;
    sim_msg ("flags    : ");
    sim_msg ("autoaccept=%d, ", csp->autoaccept);
    sim_msg ("noempty=%d, ",    csp->noempty);
    sim_msg ("attn_flush=%d",   csp->attn_flush);
    return SCPE_OK;
  }

static t_stat opc_show_device_name (UNUSED FILE * st, UNIT * uptr,
                                    UNUSED int val, UNUSED const void * desc)
  {
    int n = (int) OPC_UNIT_IDX (uptr);
    if (n < 0 || n >= N_OPC_UNITS_MAX)
      return SCPE_ARG;
    sim_printf("name     : OPC%d", n);
    return SCPE_OK;
  }

static t_stat opc_set_device_name (UNIT * uptr, UNUSED int32 value,
                                   const char * cptr, UNUSED void * desc)
  {
    int n = (int) OPC_UNIT_IDX (uptr);
    if (n < 0 || n >= N_OPC_UNITS_MAX)
      return SCPE_ARG;
    if (cptr)
      {
        strncpy (console_state[n].device_name, cptr, MAX_DEV_NAME_LEN-1);
        console_state[n].device_name[MAX_DEV_NAME_LEN-1] = 0;
      }
    else
      console_state[n].device_name[0] = 0;
    return SCPE_OK;
  }

static t_stat opc_set_console_port (UNIT * uptr, UNUSED int32 value,
                                    const char * cptr, UNUSED void * desc)
  {
    int dev_idx = (int) OPC_UNIT_IDX (uptr);
    if (dev_idx < 0 || dev_idx >= N_OPC_UNITS_MAX)
      return SCPE_ARG;

    if (cptr)
      {
        int port = atoi (cptr);
        if (port < 0 || port > 65535) // 0 is 'disable'
          return SCPE_ARG;
        console_state[dev_idx].console_access.port = port;
        if (port != 0)
          sim_msg ("[OPC emulation: TELNET server port set to %d]\r\n", (int)port);
      }
    else
      console_state[dev_idx].console_access.port = 0;
    return SCPE_OK;
  }

static t_stat opc_show_console_port (UNUSED FILE * st, UNIT * uptr,
                                       UNUSED int val, UNUSED const void * desc)
  {
    int dev_idx = (int) OPC_UNIT_IDX (uptr);
    if (dev_idx < 0 || dev_idx >= N_OPC_UNITS_MAX)
      return SCPE_ARG;
    if (console_state[dev_idx].console_access.port)
      sim_printf("port     : %d", console_state[dev_idx].console_access.port);
    else
      sim_printf("port     : disabled");
    return SCPE_OK;
  }

static t_stat opc_set_console_address (UNIT * uptr, UNUSED int32 value,
                                    const char * cptr, UNUSED void * desc)
  {
    int dev_idx = (int) OPC_UNIT_IDX (uptr);
    if (dev_idx < 0 || dev_idx >= N_OPC_UNITS_MAX)
      return SCPE_ARG;

    if (console_state[dev_idx].console_access.address)
      {
        FREE (console_state[dev_idx].console_access.address);
        console_state[dev_idx].console_access.address = NULL;
      }

    if (cptr)
      {
        console_state[dev_idx].console_access.address = strdup (cptr);
        if (!console_state[dev_idx].console_access.address)
          {
            fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                     __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# ifdef SIGUSR2
            (void)raise(SIGUSR2);
            /*NOTREACHED*/ /* unreachable */
# endif /* ifdef SIGUSR2 */
#endif /* if defined(USE_BACKTRACE) */
            abort();
          }
        sim_msg ("\r[OPC emulation: OPC%d server address set to %s]\r\n",
                dev_idx, console_state[dev_idx].console_access.address);
      }

    return SCPE_OK;
  }

static t_stat opc_show_console_address (UNUSED FILE * st, UNIT * uptr,
                                       UNUSED int val, UNUSED const void * desc)
  {
    int dev_idx = (int) OPC_UNIT_IDX (uptr);
    if (dev_idx < 0 || dev_idx >= N_OPC_UNITS_MAX)
      return SCPE_ARG;
    if (console_state[dev_idx].console_access.address)
      sim_printf("address  : %s", console_state[dev_idx].console_access.address);
    else
      sim_printf("address  : any");
    return SCPE_OK;
  }

static t_stat opc_set_console_pw (UNIT * uptr, UNUSED int32 value,
                                    const char * cptr, UNUSED void * desc)
  {
    long dev_idx = (int) OPC_UNIT_IDX (uptr);
    if (dev_idx < 0 || dev_idx >= N_OPC_UNITS_MAX)
      return SCPE_ARG;

    if (cptr && (strlen(cptr) > 0))
      {
        char token[strlen (cptr)+1];
        int rc = sscanf (cptr, "%s", token);
        if (rc != 1)
          return SCPE_ARG;
        if (strlen (token) > PW_SIZE)
          return SCPE_ARG;
        strcpy (console_state[dev_idx].console_access.pw, token);
      }
    else
      {
        sim_msg ("no password\n");
        console_state[dev_idx].console_access.pw[0] = 0;
      }

    return SCPE_OK;
  }

static t_stat opc_show_console_pw (UNUSED FILE * st, UNIT * uptr,
                                       UNUSED int val, UNUSED const void * desc)
  {
    int dev_idx = (int) OPC_UNIT_IDX (uptr);
    if (dev_idx < 0 || dev_idx >= N_OPC_UNITS_MAX)
      return SCPE_ARG;
    sim_printf("password : %s", console_state[dev_idx].console_access.pw);
    return SCPE_OK;
  }

static void console_putstr (int conUnitIdx, char * str)
  {
    size_t l = strlen (str);
    for (size_t i = 0; i < l; i ++)
      sim_putchar (str[i]);
    opc_state_t * csp = console_state + conUnitIdx;
    if (csp->console_access.loggedOn)
      accessStartWrite (csp->console_access.client, str,
                           (ssize_t) l);
  }

static void consolePutchar0 (int conUnitIdx, char ch) {
  opc_state_t * csp = console_state + conUnitIdx;
  sim_putchar (ch);
  if (csp->console_access.loggedOn)
    accessStartWrite (csp->console_access.client, & ch, 1);
}

static void console_putchar (int conUnitIdx, char ch) {
  opc_state_t * csp = console_state + conUnitIdx;
  if (csp->escapeSequence) { // Prior character was an escape
    csp->escapeSequence = false;
    if (ch == '1') { // Set tab
      if (csp->carrierPosition >= 1 && csp->carrierPosition <= 256) {
        csp->tabStops[csp->carrierPosition] = true;
      }
    } else if (ch == '2') { // Clear all tabs
      memset (csp->tabStops, 0, sizeof (csp->tabStops));
    } else { // Unrecognized
      sim_warn ("Unrecognized escape sequence \\033\\%03o\r\n", ch);
    }
  } else if (isprint (ch)) {
    consolePutchar0 (conUnitIdx, ch);
    csp->carrierPosition ++;
  } else if (ch == '\t') {  // Tab
    while (csp->carrierPosition < bufsize - 1) {
      consolePutchar0 (conUnitIdx, ' ');
      csp->carrierPosition ++;
      if (csp->tabStops[csp->carrierPosition])
        break;
    }
  } else if (ch == '\b') { // Backspace
      consolePutchar0 (conUnitIdx, ch);
      csp->carrierPosition --;
  } else if (ch == '\f' || ch == '\r') { // Formfeed, Carriage return
      consolePutchar0 (conUnitIdx, ch);
      csp->carrierPosition = 1;
  } else if (ch == '\033') { //-V536  // Escape
      csp->escapeSequence = true;
  } else { // Non-printing and we don't recognize a carriage motion character, so just print it...
      consolePutchar0 (conUnitIdx, ch);
  }
}

static void consoleConnectPrompt (uv_tcp_t * client)
  {
    accessStartWriteStr (client, "password: \r\n");
    uv_access * console_access = (uv_access *) client->data;
    console_access->pwPos      = 0;
  }

void startRemoteConsole (void)
  {
    for (int conUnitIdx = 0; conUnitIdx < N_OPC_UNITS_MAX; conUnitIdx ++)
      {
        console_state[conUnitIdx].console_access.connectPrompt = consoleConnectPrompt;
        console_state[conUnitIdx].console_access.connected     = NULL;
        console_state[conUnitIdx].console_access.useTelnet     = true;
#ifdef CONSOLE_FIX
# ifdef LOCKLESS
        lock_libuv ();
# endif
#endif
        uv_open_access (& console_state[conUnitIdx].console_access);
#ifdef CONSOLE_FIX
# ifdef LOCKLESS
        unlock_libuv ();
# endif
#endif
      }
  }
