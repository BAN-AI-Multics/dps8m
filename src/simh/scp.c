/*
 * scp.c: simulator control program
 *
 * vim: filetype=c:tabstop=4:ai:colorcolumn=84:expandtab
 * SPDX-License-Identifier: MIT
 * scspell-id: 7cde852c-f62a-11ec-8444-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 1993-2022 Robert M. Supnik
 * Copyright (c) 2021-2023 Jeffrey H. Johnson
 * Copyright (c) 2006-2024 The DPS8M Development Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Robert M. Supnik shall not
 * be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Robert M. Supnik.
 *
 * ---------------------------------------------------------------------------
 */

//-V::701

#if !defined(__EXTENSIONS__)
# define __EXTENSIONS__
#endif

#if !defined(__STDC_WANT_LIB_EXT1__)
# define __STDC_WANT_LIB_EXT1__ 1
#endif

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE
#endif

/* Macros and data structures */

#include "sim_defs.h"
#include "sim_disk.h"
#include "sim_tape.h"
#include "sim_sock.h"

#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <stddef.h>
#if defined(_WIN32)
# if !defined(WIN32_LEAN_AND_MEAN)
#  define WIN32_LEAN_AND_MEAN
# endif /* if !defined(WIN32_LEAN_AND_MEAN) */
# if defined(_MSC_VER)
#  pragma warning(push, 3)
# endif
# include <direct.h>
# include <io.h>
# include <fcntl.h>
#else
# include <unistd.h>
# define HAVE_UNISTD 1
#endif
#include <sys/stat.h>
#include <sys/types.h>
#if !defined(_WIN32) && !defined(__MINGW32__) && !defined(__MINGW64__) && !defined(CROSS_MINGW32) && !defined(CROSS_MINGW64)
# include <sys/resource.h>
#endif
#include <setjmp.h>
#include <stdint.h>
#include <limits.h>
#if defined(__APPLE__)
# include <xlocale.h>
#endif
#include <locale.h>

#include "linehistory.h"

#if defined(__APPLE__)
# include <sys/sysctl.h>
#endif /* if defined(_APPLE_) */

#if ( defined(__linux__) || defined(__linux) || defined(_linux) || defined(linux) ) //-V1040
# include <sys/sysinfo.h>
# define LINUX_OS
#endif

#include <uv.h>

#if !defined(HAVE_UNISTD)
# undef USE_BACKTRACE
#endif /* if !defined(HAVE_UNISTD) */

#if defined(USE_BACKTRACE)
# include <string.h>
# include <signal.h>
#endif /* if defined(USE_BACKTRACE) */

#if defined(__HAIKU__)
# include <OS.h>
#endif /* if defined(__HAIKU__) */

#if !defined(__CYGWIN__)
# if !defined(__APPLE__)
#  if !defined(_AIX)
#   if !defined(__MINGW32__)
#    if !defined(__MINGW64__)
#     if !defined(CROSS_MINGW32)
#      if !defined(CROSS_MINGW64)
#       if !defined(_WIN32)
#        if !defined(__HAIKU__)
#         include <link.h>
#        endif
#       endif
#      endif
#     endif
#    endif
#   endif
#  endif
# endif
#endif

#if defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(CROSS_MINGW32) || defined(CROSS_MINGW64)
# include <windows.h>
#endif

#if defined(__CYGWIN__)
# include <windows.h>
# include <sys/utsname.h>
# include <sys/cygwin.h>
# include <cygwin/version.h>
#endif

#define DBG_CTR 0

#include "../dps8/dps8.h"
#include "../dps8/dps8_cpu.h"
#include "../dps8/ver.h"

#include "../dps8/dps8_iom.h"
#include "../dps8/dps8_fnp2.h"

#include "../decNumber/decContext.h"
#include "../decNumber/decNumberLocal.h"

#include "../dps8/dps8_math128.h"

#if !defined(__CYGWIN__)
# if !defined(__APPLE__)
#  if !defined(_AIX)
#   if !defined(__MINGW32__)
#    if !defined(__MINGW64__)
#     if !defined(CROSS_MINGW32)
#      if !defined(CROSS_MINGW64)
#       if !defined(_WIN32)
#        if !defined(__HAIKU__)
static unsigned int dl_iterate_phdr_callback_called = 0;
#        endif
#       endif
#      endif
#     endif
#    endif
#   endif
#  endif
# endif
#endif

#if defined(MAX)
# undef MAX
#endif /* if defined(MAX) */
#define MAX(a,b)  (((a) >= (b)) ? (a) : (b))

#if defined(FREE)
# undef FREE
#endif /* if defined(FREE) */
#define FREE(p) do  \
  {                 \
    free((p));      \
    (p) = NULL;     \
  } while(0)

/* search logical and boolean ops */

#define SCH_OR          0                               /* search logicals */
#define SCH_AND         1
#define SCH_XOR         2
#define SCH_E           0                               /* search booleans */
#define SCH_N           1
#define SCH_G           2
#define SCH_L           3
#define SCH_EE          4
#define SCH_NE          5
#define SCH_GE          6
#define SCH_LE          7

#define MAX_DO_NEST_LVL 20                              /* DO cmd nesting level */
#define SRBSIZ          1024                            /* save/restore buffer */
#define SIM_BRK_INILNT  4096                            /* bpt tbl length */
#define SIM_BRK_ALLTYP  0xFFFFFFFB

#define UPDATE_SIM_TIME                                         \
    if (1) {                                                    \
        int32 _x;                                               \
        if (sim_clock_queue == QUEUE_LIST_END)                  \
            _x = noqueue_time;                                  \
        else                                                    \
            _x = sim_clock_queue->time;                         \
        sim_time = sim_time + (_x - sim_interval);              \
        sim_rtime = sim_rtime + ((uint32) (_x - sim_interval)); \
        if (sim_clock_queue == QUEUE_LIST_END)                  \
            noqueue_time = sim_interval;                        \
        else                                                    \
            sim_clock_queue->time = sim_interval;               \
        }                                                       \
    else                                                        \
        (void)0

#define SZ_D(dp) (size_map[((dp)->dwidth + CHAR_BIT - 1) / CHAR_BIT])

#define SZ_R(rp) \
    (size_map[((rp)->width + (rp)->offset + CHAR_BIT - 1) / CHAR_BIT])

#define SZ_LOAD(sz,v,mb,j)                                                 \
    if (sz == sizeof (uint8)) v = *(((uint8 *) mb) + ((uint32) j));        \
    else if (sz == sizeof (uint16)) v = *(((uint16 *) mb) + ((uint32) j)); \
    else if (sz == sizeof (uint32)) v = *(((uint32 *) mb) + ((uint32) j)); \
    else v = *(((t_uint64 *) mb) + ((uint32) j));

#define SZ_STORE(sz,v,mb,j)                                                         \
    if (sz == sizeof (uint8)) *(((uint8 *) mb) + j) = (uint8) v;                    \
    else if (sz == sizeof (uint16)) *(((uint16 *) mb) + ((uint32) j)) = (uint16) v; \
    else if (sz == sizeof (uint32)) *(((uint32 *) mb) + ((uint32) j)) = (uint32) v; \
    else *(((t_uint64 *) mb) + ((uint32) j)) = v;

#define GET_SWITCHES(cp) \
    if ((cp = get_sim_sw (cp)) == NULL) return SCPE_INVSW

#define GET_RADIX(val,dft)                          \
    if (sim_switches & SWMASK ('O')) val = 8;       \
    else if (sim_switches & SWMASK ('D')) val = 10; \
    else if (sim_switches & SWMASK ('H')) val = 16; \
    else val = dft;

/*
 * The per-simulator init routine is a weak global that defaults to NULL
 * The other per-simulator pointers can be overridden by the init routine
 */

t_bool sim_asynch_enabled = FALSE;
t_stat tmxr_locate_line_send (const char *dev_line, SEND **snd);
t_stat tmxr_locate_line_expect (const char *dev_line, EXPECT **exp);
extern void (*sim_vm_init) (void);
extern void (*sim_vm_exit) (void);
char* (*sim_vm_read) (char *ptr, int32 size, FILE *stream) = NULL;
void (*sim_vm_post) (t_bool from_scp) = NULL;
CTAB *sim_vm_cmd = NULL;
void (*sim_vm_sprint_addr) (char *buf, DEVICE *dptr, t_addr addr) = NULL;
void (*sim_vm_fprint_addr) (FILE *st, DEVICE *dptr, t_addr addr) = NULL;
t_addr (*sim_vm_parse_addr) (DEVICE *dptr, CONST char *cptr, CONST char **tptr) = NULL;
t_value (*sim_vm_pc_value) (void) = NULL;
t_bool (*sim_vm_is_subroutine_call) (t_addr **ret_addrs) = NULL;
t_bool (*sim_vm_fprint_stopped) (FILE *st, t_stat reason) = NULL;

/* Prototypes */

/* Set and show command processors */

t_stat set_dev_radix (DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat set_dev_enbdis (DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat set_dev_debug (DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat set_unit_enbdis (DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat ssh_break (FILE *st, const char *cptr, int32 flg);
t_stat show_cmd_fi (FILE *ofile, int32 flag, CONST char *cptr);
t_stat show_config (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_queue (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_time (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_mod_names (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_show_commands (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_log_names (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_dev_radix (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_dev_debug (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_dev_logicals (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_dev_modifiers (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_dev_show_commands (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_version (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_buildinfo (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cprr);
t_stat show_prom (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_default_base_system_script (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_default (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_break (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_on (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat sim_show_send (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat sim_show_expect (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_device (FILE *st, DEVICE *dptr, int32 flag);
t_stat show_unit (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag);
t_stat show_all_mods (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flg, int32 *toks);
t_stat show_one_mod (FILE *st, DEVICE *dptr, UNIT *uptr, MTAB *mptr, CONST char *cptr, int32 flag);
t_stat sim_save (FILE *sfile);
t_stat sim_rest (FILE *rfile);

/* Breakpoint package */

t_stat sim_brk_init (void);
t_stat sim_brk_set (t_addr loc, int32 sw, int32 ncnt, CONST char *act);
t_stat sim_brk_clr (t_addr loc, int32 sw);
t_stat sim_brk_clrall (int32 sw);
t_stat sim_brk_show (FILE *st, t_addr loc, int32 sw);
t_stat sim_brk_showall (FILE *st, int32 sw);
CONST char *sim_brk_getact (char *buf, int32 size);
BRKTAB *sim_brk_new (t_addr loc, uint32 btyp);
char *sim_brk_clract (void);

FILE *stdnul;

/* Command support routines */

SCHTAB *get_rsearch (CONST char *cptr, int32 radix, SCHTAB *schptr);
SCHTAB *get_asearch (CONST char *cptr, int32 radix, SCHTAB *schptr);
int32 test_search (t_value *val, SCHTAB *schptr);
static const char *get_glyph_gen (const char *iptr, char *optr, char mchar, t_bool uc, t_bool quote, char escape_char);
int32 get_switches (const char *cptr);
CONST char *get_sim_sw (CONST char *cptr);
t_stat get_aval (t_addr addr, DEVICE *dptr, UNIT *uptr);
t_value get_rval (REG *rptr, uint32 idx);
void put_rval (REG *rptr, uint32 idx, t_value val);
void fprint_help (FILE *st);
void fprint_stopped (FILE *st, t_stat r);
void fprint_capac (FILE *st, DEVICE *dptr, UNIT *uptr);
void fprint_sep (FILE *st, int32 *tokens);
char *read_line (char *ptr, int32 size, FILE *stream);
char *read_line_p (const char *prompt, char *ptr, int32 size, FILE *stream);
REG *find_reg_glob (CONST char *ptr, CONST char **optr, DEVICE **gdptr);
char *sim_trim_endspc (char *cptr);

/* Forward references */

t_stat scp_attach_unit (DEVICE *dptr, UNIT *uptr, const char *cptr);
t_stat scp_detach_unit (DEVICE *dptr, UNIT *uptr);
t_bool qdisable (DEVICE *dptr);
t_stat attach_err (UNIT *uptr, t_stat stat);
t_stat detach_all (int32 start_device, t_bool shutdown);
t_stat assign_device (DEVICE *dptr, const char *cptr);
t_stat deassign_device (DEVICE *dptr);
t_stat ssh_break_one (FILE *st, int32 flg, t_addr lo, int32 cnt, CONST char *aptr);
t_stat exdep_reg_loop (FILE *ofile, SCHTAB *schptr, int32 flag, CONST char *cptr,
    REG *lowr, REG *highr, uint32 lows, uint32 highs);
t_stat ex_reg (FILE *ofile, t_value val, int32 flag, REG *rptr, uint32 idx);
t_stat dep_reg (int32 flag, CONST char *cptr, REG *rptr, uint32 idx);
t_stat exdep_addr_loop (FILE *ofile, SCHTAB *schptr, int32 flag, const char *cptr,
    t_addr low, t_addr high, DEVICE *dptr, UNIT *uptr);
t_stat ex_addr (FILE *ofile, int32 flag, t_addr addr, DEVICE *dptr, UNIT *uptr);
t_stat dep_addr (int32 flag, const char *cptr, t_addr addr, DEVICE *dptr,
    UNIT *uptr, int32 dfltinc);
void fprint_fields (FILE *stream, t_value before, t_value after, BITFIELD* bitdefs);
t_stat step_svc (UNIT *ptr);
t_stat expect_svc (UNIT *ptr);
t_stat set_on (int32 flag, CONST char *cptr);
t_stat set_verify (int32 flag, CONST char *cptr);
t_stat set_message (int32 flag, CONST char *cptr);
t_stat set_quiet (int32 flag, CONST char *cptr);
t_stat set_localopc (int32 flag, CONST char *cptr);
t_stat set_asynch (int32 flag, CONST char *cptr);
t_stat sim_show_asynch (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat do_cmd_label (int32 flag, CONST char *cptr, CONST char *label);
void int_handler (int signal);
t_stat set_prompt (int32 flag, CONST char *cptr);
t_stat sim_set_asynch (int32 flag, CONST char *cptr);
t_stat sim_set_environment (int32 flag, CONST char *cptr);
static const char *get_dbg_verb (uint32 dbits, DEVICE* dptr);

/* Global data */

DEVICE *sim_dflt_dev             = NULL;
UNIT *sim_clock_queue            = QUEUE_LIST_END;
int32 sim_interval               = 0;
int32 sim_switches               = 0;
FILE *sim_ofile                  = NULL;
SCHTAB *sim_schrptr              = FALSE;
SCHTAB *sim_schaptr              = FALSE;
DEVICE *sim_dfdev                = NULL;
UNIT *sim_dfunit                 = NULL;
DEVICE **sim_internal_devices    = NULL;
uint32 sim_internal_device_count = 0;
int32 sim_opt_out                = 0;
int32 sim_is_running             = 0;
t_bool sim_processing_event      = FALSE;
uint32 sim_brk_summ              = 0;
uint32 sim_brk_types             = 0;
BRKTYPTAB *sim_brk_type_desc     = NULL;         /* type descriptions */
uint32 sim_brk_dflt              = 0;
uint32 sim_brk_match_type;
t_addr sim_brk_match_addr;
char *sim_brk_act[MAX_DO_NEST_LVL];
char *sim_brk_act_buf[MAX_DO_NEST_LVL];
BRKTAB **sim_brk_tab             = NULL;
int32 sim_brk_ent                = 0;
int32 sim_brk_lnt                = 0;
int32 sim_brk_ins                = 0;
int32 sim_iglock                 = 0;
int32 sim_nolock                 = 0;
int32 sim_quiet                  = 0;
int32 sim_localopc               = 1;
int32 sim_randompst              = 0;
int32 sim_randstate              = 0;
int32 sim_step                   = 0;
int nodist                       = 0;
#if defined(PERF_STRIP)
int32 sim_nostate                = 1;
#else
int32 sim_nostate                = 0;
#endif /* if defined(PERF_STRIP) */
static double sim_time;
static uint32 sim_rtime;
static int32 noqueue_time;
volatile int32 stop_cpu          = 0;
t_value *sim_eval                = NULL;
static t_value sim_last_val;
static t_addr sim_last_addr;
FILE *sim_log                    = NULL;         /* log file */
FILEREF *sim_log_ref             = NULL;         /* log file file reference */
FILE *sim_deb                    = NULL;         /* debug file */
FILEREF *sim_deb_ref             = NULL;         /* debug file file reference */
int32 sim_deb_switches           = 0;            /* debug switches */
struct timespec sim_deb_basetime;                /* debug timestamp relative base time */
char *sim_prompt                 = NULL;         /* prompt string */
static FILE *sim_gotofile;                       /* the currently open do file */
static int32 sim_goto_line[MAX_DO_NEST_LVL+1];   /* the current line number in the currently open do file */
static int32 sim_do_echo         = 0;            /* the echo status of the currently open do file */
static int32 sim_show_message    = 1;            /* the message display status of the currently open do file */
static int32 sim_on_inherit      = 0;            /* the inherit status of on state and conditions when executing do files */
static int32 sim_do_depth        = 0;

static int32 sim_on_check[MAX_DO_NEST_LVL+1];
static char *sim_on_actions[MAX_DO_NEST_LVL+1][SCPE_MAX_ERR+1];
static char sim_do_filename[MAX_DO_NEST_LVL+1][CBUFSIZE];
static const char *sim_do_ocptr[MAX_DO_NEST_LVL+1];
static const char *sim_do_label[MAX_DO_NEST_LVL+1];

t_stat sim_last_cmd_stat;                        /* Command Status */

static SCHTAB sim_stabr;                         /* Register search specifier */
static SCHTAB sim_staba;                         /* Memory search specifier */

static UNIT sim_step_unit   = { UDATA (&step_svc, 0, 0)  };
static UNIT sim_expect_unit = { UDATA (&expect_svc, 0, 0)  };

/* Tables and strings */

const char save_vercur[] = "V4.1";
const char save_ver40[]  = "V4.0";
const char save_ver35[]  = "V3.5";
const char save_ver32[]  = "V3.2";
const char save_ver30[]  = "V3.0";
const struct scp_error {
    const char *code;
    const char *message;
    } scp_errors[1+SCPE_MAX_ERR-SCPE_BASE] =
        {{"NXM",     "Address space exceeded"},
         {"UNATT",   "Unit not attached"},
         {"IOERR",   "I/O error"},
         {"CSUM",    "Checksum error"},
         {"FMT",     "Format error"},
         {"NOATT",   "Unit not attachable"},
         {"OPENERR", "File open error"},
         {"MEM",     "Memory exhausted"},
         {"ARG",     "Invalid argument"},
         {"STEP",    "Step expired"},
         {"UNK",     "Unknown command"},
         {"RO",      "Read only argument"},
         {"INCOMP",  "Command not completed"},
         {"STOP",    "Simulation stopped"},
         {"EXIT",    "Goodbye"},
         {"TTIERR",  "Console input I/O error"},
         {"TTOERR",  "Console output I/O error"},
         {"EOF",     "End of file"},
         {"REL",     "Relocation error"},
         {"NOPARAM", "No settable parameters"},
         {"ALATT",   "Unit already attached"},
         {"TIMER",   "Hardware timer error"},
         {"SIGERR",  "Signal handler setup error"},
         {"TTYERR",  "Console terminal setup error"},
         {"SUB",     "Subscript out of range"},
         {"NOFNC",   "Command not allowed"},
         {"UDIS",    "Unit disabled"},
         {"NORO",    "Read only operation not allowed"},
         {"INVSW",   "Invalid switch"},
         {"MISVAL",  "Missing value"},
         {"2FARG",   "Too few arguments"},
         {"2MARG",   "Too many arguments"},
         {"NXDEV",   "Non-existent device"},
         {"NXUN",    "Non-existent unit"},
         {"NXREG",   "Non-existent register"},
         {"NXPAR",   "Non-existent parameter"},
         {"NEST",    "Nested DO command limit exceeded"},
         {"IERR",    "Internal error"},
         {"MTRLNT",  "Invalid magtape record length"},
         {"LOST",    "Console Telnet connection lost"},
         {"TTMO",    "Console Telnet connection timed out"},
         {"STALL",   "Console Telnet output stall"},
         {"AFAIL",   "Assertion failed"},
         {"INVREM",  "Invalid remote console command"},
         {"NOTATT",  "Not attached"},
         {"EXPECT",  "Expect matched"},
         {"REMOTE",  "Remote console command"},
    };

const size_t size_map[] = { sizeof (int8),
    sizeof (int8), sizeof (int16), sizeof (int32), sizeof (int32)
    , sizeof (t_int64), sizeof (t_int64), sizeof (t_int64), sizeof (t_int64)
};

const t_value width_mask[] = { 0,
    0x1, 0x3, 0x7, 0xF,
    0x1F, 0x3F, 0x7F, 0xFF,
    0x1FF, 0x3FF, 0x7FF, 0xFFF,
    0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
    0x1FFFF, 0x3FFFF, 0x7FFFF, 0xFFFFF,
    0x1FFFFF, 0x3FFFFF, 0x7FFFFF, 0xFFFFFF,
    0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF, 0xFFFFFFF,
    0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
    0x1FFFFFFFF, 0x3FFFFFFFF, 0x7FFFFFFFF, 0xFFFFFFFFF,
    0x1FFFFFFFFF, 0x3FFFFFFFFF, 0x7FFFFFFFFF, 0xFFFFFFFFFF,
    0x1FFFFFFFFFF, 0x3FFFFFFFFFF, 0x7FFFFFFFFFF, 0xFFFFFFFFFFF,
    0x1FFFFFFFFFFF, 0x3FFFFFFFFFFF, 0x7FFFFFFFFFFF, 0xFFFFFFFFFFFF,
    0x1FFFFFFFFFFFF, 0x3FFFFFFFFFFFF, 0x7FFFFFFFFFFFF, 0xFFFFFFFFFFFFF,
    0x1FFFFFFFFFFFFF, 0x3FFFFFFFFFFFFF, 0x7FFFFFFFFFFFFF, 0xFFFFFFFFFFFFFF,
    0x1FFFFFFFFFFFFFF, 0x3FFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFF,
    0x1FFFFFFFFFFFFFFF, 0x3FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF
    };

static const char simh_help[] =
       /***************** 80 character line width template *************************/
      "1Commands\n"
#define HLP_RESET       "*Commands Resetting Devices"
       /***************** 80 character line width template *************************/
      "2Resetting Devices\n"
      " The `RESET` command (*abbreviated* `RE`) resets a device or the entire\n"
      " simulator to a predefined condition.  If the switch \"`-p`\" is specified,\n"
      " the device is reset to its initial power-on state:\n\n"
      "++RESET                  resets all devices\n"
      "++RESET -p               power-cycle all devices\n"
      "++RESET ALL              resets all devices\n"
      "++RESET <device>         resets the specified <device>\n\n"
      " * Typically, `RESET` *aborts* in-progress I/O operations, *clears* any\n"
      " interrupt requests, and returns the device to a quiescent state.\n\n"
      " * It does **NOT** clear the main memory or affect associated I/O\n"
      " connections.\n"
#define HLP_EXAMINE     "*Commands Examining_and_Changing_State"
#define HLP_IEXAMINE    "*Commands Examining_and_Changing_State"
#define HLP_DEPOSIT     "*Commands Examining_and_Changing_State"
#define HLP_IDEPOSIT    "*Commands Examining_and_Changing_State"
       /***************** 80 character line width template *************************/
      "2Examining and Changing State\n"
      " There are four commands to examine and change state:\n\n"
      " * `EXAMINE` (*abbreviated* `E`) examines state\n"
      " * `DEPOSIT` (*abbreviated* `D`) changes state\n"
      " * `IEXAMINE` (\"interactive examine\", *abbreviated* `IE`) examines state\n"
      "    and allows the user to interactively change it\n"
      " * `IDEPOSIT` (interactive deposit, *abbreviated* `ID`) allows the user to\n"
      "    interactively change state\n\n"
      " All four commands take the form:\n\n"
      "++command {modifiers} <object list>\n\n"
      " The `DEPOSIT` command requires the deposit value at the end of the command.\n\n"
      " There are four kinds of modifiers: **switches**, **device/unit name**,\n"
      " **search specifier**, and for `EXAMINE`, **output file**.\n\n"
      " * **Switches** have been described previously.\n"
      " * A **device/unit name** identifies the device and unit whose address\n"
      " space is to be examined or modified. If no device is specified, the CPU\n"
      " main memory is selected. If a device but no unit is specified, unit `0`\n"
      " of the specified device is selected automatically.\n"
      " * The **search specifier** provides criteria for testing addresses or\n"
      " registers to see if they should be processed.  The search specifier\n"
      " consists of a \"<`logical operator`>\", a \"<`relational operator`>\", or\n"
      " both, optionally separated by spaces:\n\n"
      "++{ < logical op >  < value > }  < relational op >  < value >\n\n"
       /***************** 80 character line width template *************************/
      " * * The \"<`logical operator`>\" may be \"`&`\" (*and*), \"`|`\" (*or*),\n"
      " or \"`^`\" (*exclusive or*), and the \"<`relational operator`>\" may\n"
      " be \"`=`\" or \"`==`\" (*equal*), \"`!`\" or \"`!=`\" (*not\n"
      " equal*), \">=\" (*greater than or equal*), \">\" (*greater\n"
      " than*), \"<=\" (*less than or equal*), or \"<\" (*less than*).\n"
      " * * If any \"<`logical operator`>\" is specified without\n"
      " a \"<`relational operator`>\", it is ignored.\n"
      " * * If any \"<`relational operator`>\" is specified without\n"
      " a \"<`logical operator`>\", no logical operation is performed.\n"
      " * * All comparisons are unsigned.\n\n"
      " * The **output file** modifier redirects the command output to a file\n"
      " instead of the console.  The **output file** modifier is specified with\n"
      " the \"`@`\" (*commercial-at*) character, followed by a valid file name.\n\n"
      " **NOTE**: Modifiers may be specified in any order.  If multiple\n"
      " modifiers of the same type are specified, later modifiers override earlier\n"
      " modifiers. If the **device/unit name** comes *after* the search specifier,\n"
      " the search values will interpreted in the *radix of the CPU*, rather than\n"
      " of the device/unit.\n\n"
      " The \"<`object list`>\" argument consists of one or more of the following,\n"
      " separated by commas:\n\n"
       /***************** 80 character line width template *************************/
      "++register                the specified register\n"
      "++register[sub1-sub2]     the specified register array locations,\n"
      "++++++++                  starting at location sub1 up to and\n"
      "++++++++                  including location sub2\n"
      "++register[sub1/length]   the specified register array locations,\n"
      "++++++++                  starting at location sub1 up to but\n"
      "++++++++                  not including sub1+length\n"
      "++register[ALL]           all locations in the specified register\n"
      "++++++++                  array\n"
      "++register1-register2     all the registers starting at register1\n"
      "++++++++                  up to and including register2\n"
      "++address                 the specified location\n"
      "++address1-address2       all locations starting at address1 up to\n"
      "++++++++                  and including address2\n"
      "++address/length          all location starting at address up to\n"
      "++++++++                  but not including address+length\n"
      "++STATE                   all registers in the device\n"
      "++ALL                     all locations in the unit\n"
      "++$                       the last value displayed by an EXAMINE\n"
      "++++++++                  command interpreted as an address\n"
      "3Switches\n"
      "4Formatting Control\n"
      " Switches can be used to control the format of the displayed information:\n\n"
       /***************** 80 character line width template *************************/
      "5-a\n"
      " display as ASCII\n"
      "5-c\n"
      " display as character string\n"
      "5-m\n"
      " display as instruction mnemonics\n"
      "5-o\n"
      " display as octal\n"
      "5-d\n"
      " display as decimal\n"
      "5-h\n"
      " display as hexadecimal\n\n"
      "3Examples\n"
      "++ex 1000-1100                examine 1000 to 1100\n"
      "++de PC 1040                  set PC to 1040\n"
      "++ie 40-50                    interactively examine 40:50\n"
      "++ie >1000 40-50              interactively examine the subset\n"
      "+++++++++                     of locations 40:50 that are >1000\n"
      "++ex rx0 50060                examine 50060, RX unit 0\n"
      "++ex rx sbuf[3-6]             examine SBUF[3] to SBUF[6] in RX\n"
      "++de all 0                    set main memory to 0\n"
      "++de &77>0 0                  set all addresses whose low order\n"
      "+++++++++                     bits are non-zero to 0\n"
      "++ex -m @memdump.txt 0-7777   dump memory to file\n\n"
      " * **NOTE**: To terminate an interactive command, simply type any bad value\n"
      "           (*e.g.* `XYZ`) when input is requested.\n"
#define HLP_EVALUATE    "*Commands Evaluating_Instructions"
       /***************** 80 character line width template *************************/
      "2Evaluating Instructions\n"
      " The `EVAL` command evaluates a symbolic expression and returns the\n"
      " equivalent numeric value.\n\n"
       /***************** 80 character line width template *************************/
      "2Running A Simulated Program\n"
#define HLP_RUN         "*Commands Running_A_Simulated_Program RUN"
      "3RUN\n"
      " The `RUN` command (*abbreviated* `RU`) resets all devices, deposits its\n"
      " argument, if given, in the PC (program counter), and starts execution.\n"
      " If no argument is given execution starts at the current PC.\n"
#define HLP_GO          "*Commands Running_A_Simulated_Program GO"
      "3GO\n"
      " The `GO` command does *not* reset devices, deposits its argument (if\n"
      " given) in the PC, and starts execution.  If no argument is given,\n"
      " execution starts at the current PC (program counter).\n"
#define HLP_CONTINUE    "*Commands Running_A_Simulated_Program Continuing_Execution"
      "3Continuing Execution\n"
      " The `CONTINUE` command (*abbreviated* `CONT` or `CO`) resumes execution\n"
      " (if execution was stopped, possibly due to hitting a breakpoint) at the\n"
      " current program counter without resetting any devices.\n"
#define HLP_STEP        "*Commands Running_A_Simulated_Program Step_Execution"
      "3Step Execution\n"
      " The `STEP` command (*abbreviated* `S`) resumes execution at the current\n"
      " PC for the number of instructions given by its argument.  If no argument\n"
      " is supplied, one instruction is executed.\n"
      "4Switches\n"
      "5`-T`\n"
      " If the `STEP` command is invoked with the \"`-T`\" switch, the step\n"
      " command will cause execution to run for *microseconds* rather than\n"
      " instructions.\n"
#define HLP_NEXT        "*Commands Running_A_Simulated_Program NEXT"
      "3NEXT\n"
      " The `NEXT` command (*abbreviated* `N`) resumes execution at the current PC\n"
      " for one instruction, attempting to execute *through* subroutine calls.\n"
      " If the next instruction to be executed is *not* a subroutine call, then\n"
      " one instruction is executed.\n"
#define HLP_BOOT        "*Commands Running_A_Simulated_Program Booting_the_system"
      "3Booting the system\n"
      " The `BOOT` command (*abbreviated* `BO`) resets all devices and bootstraps\n"
      " the device and unit given by its argument. If no unit is supplied,\n"
      " unit `0` is bootstrapped.  The specified unit must be `ATTACH`'ed.\n\n"
      " When booting Multics, the boot device should always be `iom0`.\n"
      " Assuming a tape is attached to the `tape0` device, it will be bootstrapped\n"
      " into memory and the system will transfer control to the boot record.\n\n"
      " **Example**\n\n"
      "++; Boot Multics using iom0\n"
      "++boot iom0\n\n"
       /***************** 80 character line width template *************************/
      "2Stopping The Simulator\n"
      " The simulator runs until the simulated hardware encounters an error, or\n"
      " until the user forces a stop condition.\n"
      "3Simulator Detected Stop Conditions\n"
      " These simulator-detected conditions stop simulation:\n\n"
      "++-  HALT instruction.  If a HALT instruction is decoded, simulation stops.\n\n"
      "++-  I/O error.  If an I/O error occurs during simulation of an I/O\n"
      "+++operation, and the device stop-on-I/O-error flag is set, simulation\n"
      "+++usually stops.\n\n"
      "++-  Processor condition.  Certain processor conditions can stop\n"
      "+++the simulation.\n"
      "3User Specified Stop Conditions\n"
      " Typing the interrupt character stops simulation.  The interrupt character\n"
      " is defined by the `WRU` (*Where aRe yoU*) console option, and is initially\n"
      " set to `005` (`^E`).\n\n"
       /***************** 80 character line width template *************************/
#define HLP_BREAK       "*Commands Stopping_The_Simulator User_Specified_Stop_Conditions BREAK"
#define HLP_NOBREAK     "*Commands Stopping_The_Simulator User_Specified_Stop_Conditions BREAK"
      "4Breakpoints\n"
      " The simulator offers breakpoint capability for debugging. Users may define\n"
      " breakpoints of different types, identified by letter (for example, `E`\n"
      " for *execution*, `R` for *read*, `W` for *write*, etc).\n\n"
      " Associated with each breakpoint is a count and, optionally, one or more\n"
      " actions.  Each time a breakpoint occurs, the associated count\n"
      " is *decremented*.  If the count is less than or equal to `0`, the breakpoint\n"
      " occurs; otherwise, it is deferred.  When the breakpoint occurs, any\n"
      " optional actions are automatically executed.\n\n"
      " A breakpoint is set by the `BREAK` (or `SET BREAK`) command:\n\n"
      "++BREAK {-types} {<addr range>{[count]},{addr range...}}{;action;action...}\n\n"
      " If no type is specified, the default breakpoint type (`E`, *execution*) is\n"
      " used.  If no address range is specified, the current PC is used.  As\n"
      " with `EXAMINE` and `DEPOSIT`, an address range may be a single address, a\n"
      " range of addresses low-high, or a relative range of address/length.\n"
       /***************** 80 character line width template *************************/
      "5Displaying Breakpoints\n"
      " Currently set breakpoints can be displayed with the `SHOW BREAK` command:\n\n"
      "++SHOW {-C} {-types} BREAK {ALL|<addr range>{,<addr range>...}}\n\n"
      " Locations with breakpoints of the specified type are displayed.\n\n"
      " The \"`-C`\" switch displays the selected breakpoint(s) formatted as\n"
      " commands which may be subsequently used to establish the same\n"
      " breakpoint(s).\n\n"
      "5Removing Breakpoints\n"
      " Breakpoints can be cleared by the `NOBREAK` or the `SET NOBREAK` commands.\n"
      "5Examples\n"
      " The following examples illustrate breakpoint usage:\n\n"
      "++BREAK                      set E break at current PC\n"
      "++BREAK -e 200               set E break at 200\n"
      "++BREAK 2000/2[2]            set E breaks at 2000,2001 with count = 2\n"
      "++BREAK 100;EX AC;D MQ 0     set E break at 100 with actions EX AC and\n"
      "+++++++++D MQ 0\n"
      "++BREAK 100;                 delete action on break at 100\n\n"
       /***************** 80 character line width template *************************/
      "2Connecting and Disconnecting Devices\n"
      " Units are simulated as files on the host file system.  Before using any\n"
      " simulated unit, the user must specify the file to be accessed by that unit.\n"
#define HLP_ATTACH      "*Commands Connecting_and_Disconnecting_Devices Attaching_devices"
      "3Attaching devices\n"
      " The `ATTACH` (*abbreviation* `AT`) command associates a unit and a file:\n\n"
      "++ATTACH <unit> <filename>\n\n"
      " Some devices have more detailed or specific help available with:\n\n"
      "++HELP <device> ATTACH\n\n"
      "4Switches\n"
      "5-n\n"
      " If the \"`-n`\" switch is specified when `ATTACH` is executed, a new\n"
      " file will be created when the filename specified does not exist, or an\n"
      " existing file will have it's size truncated to zero, and an appropriate\n"
      " message is printed.\n"
      "5-e\n"
      " If the file does not exist, and the \"`-e`\" switch *was not* specified,\n"
      " a new file is created, and an appropriate message is printed.  If\n"
      " the \"`-e`\" switch *was* specified, a new file is *not* created, and an\n"
      " error message is printed.\n"
      "5-r\n"
      " If the \"`-r`\" switch is specified, or the file is write protected by\n"
      " host operating system, `ATTACH` tries to open the file in read only mode.\n"
      " If the file does not exist, or the unit does not support read only\n"
      " operation, an error occurs.  Input-only devices, such as card readers, or\n"
      " storage devices with write locking switches, such as disks or tapes,\n"
      " support read only operation - other devices do not.  If a file is\n"
      " attached read only, its contents can be examined but not modified.\n"
      "5-q\n"
      " If the \"`-q`\" switch is specified when creating a new file (\"`-n`\")\n"
      " or opening one read only (\"`-r`\"), the message announcing this fact\n"
      " is suppressed.\n"
      "5-f\n"
      " For simulated magnetic tapes, the `ATTACH` command can specify the format\n"
      " of the attached tape image file:\n\n"
      "++ATTACH -f <tape_unit> <format> <filename>\n\n"
      " * The currently supported magnetic tape image file formats are:\n\n"
      " |                  |                                                      |\n"
      " | ----------------:|:---------------------------------------------------- |\n"
      " | \"**`SIMH`**\"   | The **SIMH** / **DPS8M** native portable tape format |\n"
      " | \"**`E11`**\"    | The *D Bit* **Ersatz-11** simulator format           |\n"
      " | \"**`TPC`**\"    | The **TPC** format (*used by _SIMH_ prior to V2.3*)  |\n"
      " | \"**`P7B`**\"    | The **Paul Pierce** `7`-track tape archive format    |\n\n"
       /***************** 80 character line width template *************************/
      " * The default tape format can also be specified with the `SET` command\n"
      " prior to using the `ATTACH` command:\n\n"
      "++SET <tape_unit> FORMAT=<format>\n"
      "++ATTACH <tape_unit> <filename>\n\n"
      " * The format of a currently attached tape image can be displayed with\n"
      "   the `SHOW FORMAT` command:\n\n"
      "++SHOW <unit> FORMAT\n\n"
      " **Examples**\n\n"
      " The following example illustrates common `ATTACH` usage:\n"
      "++; Associate the tape image file \"12.7MULTICS.tap\" with the tape0 unit\n"
      "++; in read-only mode, where tape0 corresponds to the first tape device.\n"
      "++ATTACH -r tape0 12.7MULTICS.tap\n\n"
      "++; Associate the disk image file \"root.dsk\" with the disk0 unit.\n"
      "++; The disk0 unit corresponds to the first disk device.\n"
      "++ATTACH disk0 root.dsk\n\n"
       /***************** 80 character line width template *************************/
#define HLP_DETACH      "*Commands Connecting_and_Disconnecting_Devices Detaching_devices"
      "3Detaching devices\n"
      " The `DETACH` (*abbreviation* `DET`) command breaks the association between\n"
      " a unit and its backing file or device:\n\n"
      "++DETACH ALL             Detach all units\n"
      "++DETACH <unit>          Detach specified unit\n\n"
      " * **NOTE:** The `EXIT` command performs an automatic `DETACH ALL`.\n"
#define HLP_SET         "*Commands SET"
      "2SET\n"
       /***************** 80 character line width template *************************/
#define HLP_SET_LOG    "*Commands SET Logging"
      "3Logging\n"
      " Interactions with the simulator session can be recorded to a log file.\n\n"
      "+SET LOG log_file            Specify the log destination\n"
      "++++++++                     (STDOUT, DEBUG, or filename)\n"
      "+SET NOLOG                   Disables any currently active logging\n"
      "4Switches\n"
      "5`-N`\n"
      " By default, log output is written at the *end* of the specified log file.\n"
      " A new log file can created if the \"`-N`\" switch is used on the command\n"
      " line.\n\n"
      "5`-B`\n"
      " By default, log output is written in *text* mode.  The log file can be\n"
      " opened for *binary* mode writing if the \"`-B`\" switch is used on the\n"
      " command line.\n"
#define HLP_SET_DEBUG  "*Commands SET Debug_Messages"
       /***************** 80 character line width template *************************/
      "3Debug Messages\n"
      "+SET DEBUG debug_file        Specify the debug destination\n"
      "++++++++                     (STDOUT, STDERR, LOG, or filename)\n"
      "+SET NODEBUG                 Disables any currently active debug output\n"
      "4Switches\n"
      " Debug message output contains a timestamp which indicates the number of\n"
      " simulated instructions which have been executed prior to the debug event.\n\n"
      " Debug message output can be enhanced to contain additional, potentially\n"
      " useful information.\n\n\n"
      " **NOTE**: If neither \"`-T`\" or \"`-A`\" is specified, \"`-T`\" is implied.\n"
      "5-T\n"
      " The \"`-T`\" switch causes debug output to contain a time of day displayed\n"
      " as `hh:mm:ss.msec`.\n"
      "5-A\n"
      " The \"`-A`\" switch causes debug output to contain a time of day displayed\n"
      " as `seconds.msec`.\n"
      "5-R\n"
      " The \"`-R`\" switch causes timing to be relative to the start of debugging.\n"
      "5-P\n"
      " The \"`-P`\" switch adds the output of the PC (program counter) to each\n"
      " debug message.\n"
      "5-N\n"
      " The \"`-N`\" switch causes a new (empty) file to be written to.\n"
      " (The default is to append to an existing debug log file).\n"
      "5-D\n"
      " The \"`-D`\" switch causes data blob output to also display the data\n"
      " as **`RADIX-50`** characters.\n"
      "5-E\n"
      " The \"`-E`\" switch causes data blob output to also display the data\n"
      " as \"**EBCDIC**\" characters.\n"
       /***************** 80 character line width template *************************/
#define HLP_SET_ENVIRON "*Commands SET Environment_Variables"
      "3Environment Variables\n"
      "+SET ENVIRONMENT NAME=val    Set environment variable\n"
      "+SET ENVIRONMENT NAME        Clear environment variable\n"
#define HLP_SET_ON      "*Commands SET Command_Status_Trap_Dispatching"
      "3Command Status Trap Dispatching\n"
      "+SET ON                      Enables error checking command execution\n"
      "+SET NOON                    Disables error checking command execution\n"
      "+SET ON INHERIT              Enables inheritance of ON state and actions\n"
      "+SET ON NOINHERIT            Disables inheritance of ON state and actions\n"
#define HLP_SET_VERIFY "*Commands SET Command_Execution_Display"
      "3Command Execution Display\n"
      "+SET VERIFY                  Enables display of processed script commands\n"
      "+SET VERBOSE                 Enables display of processed script commands\n"
      "+SET NOVERIFY                Disables display of processed script commands\n"
      "+SET NOVERBOSE               Disables display of processed script commands\n"
#define HLP_SET_MESSAGE "*Commands SET Command_Error_Status_Display"
      "3Command Error Status Display\n"
      "+SET MESSAGE                 Re-enables display of script error messages\n"
      "+SET NOMESSAGE               Disables display of script error messages\n"
#define HLP_SET_QUIET "*Commands SET Command_Output_Display"
      "3Command Output Display\n"
      "+SET QUIET                   Disables suppression of some messages\n"
      "+SET NOQUIET                 Re-enables suppression of some messages\n"
#define HLP_SET_LOCALOPC "*Commands SET Local_Operator_Console"
      "3Local Operator Console\n"
      "+SET LOCALOPC                Enables local operator console\n"
      "+SET NOLOCALOPC              Disables local operator console\n"
#define HLP_SET_PROMPT "*Commands SET Command_Prompt"
      "3Command Prompt\n"
      "+SET PROMPT \"string\"         Sets an alternate simulator prompt string\n"
      "3Device and Unit Settings\n"
      "+SET <dev> OCT|DEC|HEX       Set device display radix\n"
      "+SET <dev> ENABLED           Enable device\n"
      "+SET <dev> DISABLED          Disable device\n"
      "+SET <dev> DEBUG{=arg}       Set device debug flags\n"
      "+SET <dev> NODEBUG={arg}     Clear device debug flags\n"
      "+SET <dev> arg{,arg...}      Set device parameters\n"
      "+SET <unit> ENABLED          Enable unit\n"
      "+SET <unit> DISABLED         Disable unit\n"
      "+SET <unit> arg{,arg...}     Set unit parameters\n"
      "+HELP <dev> SET              Displays any device specific SET commands\n"
      " \n\n"
      " See the Omnibus documentation for a complete SET command reference.\n"
       /***************** 80 character line width template *************************/
#define HLP_SHOW        "*Commands SHOW"
      "2SHOW\n"
      "+SH{OW} B{UILDINFO}               Show build-time compilation information\n"
      "+SH{OW} CL{OCKS}                  Show wall clock and timer information\n"
      "+SH{OW} C{ONFIGURATION}           Show simulator configuration\n"
      "+SH{OW} D{EFAULT_BASE_SYSTEM}     Show default base system script\n"
      "+SH{OW} DEV{ICES}                 Show devices\n"
      "+SH{OW} M{ODIFIERS}               Show SET commands for all devices\n"
      "+SH{OW} O{N}                      Show ON condition actions\n"
      "+SH{OW} P{ROM}                    Show CPU ID PROM initialization data\n"
      "+SH{OW} Q{UEUE}                   Show event queue\n"
      "+SH{OW} S{HOW}                    Show SHOW commands for all devices\n"
      "+SH{OW} T{IME}                    Show simulated timer\n"
      "+SH{OW} VE{RSION}                 Show simulator version\n"
      "+H{ELP} <dev> SHOW                Show device-specific SHOW commands\n"
      "+SH{OW} <dev> {arg,...}           Show device parameters\n"
      "+SH{OW} <dev> DEBUG               Show device debug flags\n"
      "+SH{OW} <dev> MODIFIERS           Show device modifiers\n"
      "+SH{OW} <dev> RADIX               Show device display radix\n"
      "+SH{OW} <dev> SHOW                Show device SHOW commands\n"
      "+SH{OW} <unit> {arg,...}          Show unit parameters\n\n"
      " See the Omnibus documentation for a complete SHOW command reference.\n\n"
#define HLP_SHOW_CONFIG         "*Commands SHOW"
#define HLP_SHOW_DEVICES        "*Commands SHOW"
#define HLP_SHOW_FEATURES       "*Commands SHOW"
#define HLP_SHOW_QUEUE          "*Commands SHOW"
#define HLP_SHOW_TIME           "*Commands SHOW"
#define HLP_SHOW_MODIFIERS      "*Commands SHOW"
#define HLP_SHOW_NAMES          "*Commands SHOW"
#define HLP_SHOW_SHOW           "*Commands SHOW"
#define HLP_SHOW_VERSION        "*Commands SHOW"
#define HLP_SHOW_BUILDINFO      "*Commands SHOW"
#define HLP_SHOW_PROM           "*Commands SHOW"
#define HLP_SHOW_DBS            "*Commands SHOW"
#define HLP_SHOW_DEFAULT        "*Commands SHOW"
#define HLP_SHOW_CONSOLE        "*Commands SHOW"
#define HLP_SHOW_REMOTE         "*Commands SHOW"
#define HLP_SHOW_BREAK          "*Commands SHOW"
#define HLP_SHOW_LOG            "*Commands SHOW"
#define HLP_SHOW_DEBUG          "*Commands SHOW"
#define HLP_SHOW_CLOCKS         "*Commands SHOW"
#define HLP_SHOW_ON             "*Commands SHOW"
#define HLP_SHOW_SEND           "*Commands SHOW"
#define HLP_SHOW_EXPECT         "*Commands SHOW"
#define HLP_HELP                "*Commands HELP"
       /***************** 80 character line width template *************************/
      "2HELP\n"
      "+H{ELP}                      Show this message\n"
      "+H{ELP} <command>            Show help for command\n"
      "+H{ELP} <dev>                Show help for device\n"
      "+H{ELP} <dev> REGISTERS      Show help for device register variables\n"
      "+H{ELP} <dev> ATTACH         Show help for device specific ATTACH command\n"
      "+H{ELP} <dev> SET            Show help for device specific SET commands\n"
      "+H{ELP} <dev> SHOW           Show help for device specific SHOW commands\n"
      "+H{ELP} <dev> <command>      Show help for device specific <command> command\n"
       /***************** 80 character line width template *************************/
      "2Altering The Simulated Configuration\n"
      " The \"SET <device> DISABLED\" command removes a device from the configuration.\n"
      " A `DISABLED` device is invisible to running programs.  The device can still\n"
      " be `RESET`, but it cannot be `ATTACH`ed, `DETACH`ed, or `BOOT`ed.\n\n"
      " The \"SET <device> ENABLED\" command restores a disabled device to a\n"
      " configuration.\n\n"
      " Most multi-unit devices allow units to be enabled or disabled:\n\n"
      "++SET <unit> ENABLED\n"
      "++SET <unit> DISABLED\n\n"
      " When a unit is disabled, it will not be displayed by SHOW DEVICE.\n\n"
       /***************** 80 character line width template *************************/
#define HLP_DO          "*Commands Executing_Command_Files Processing_Command_Files"
      "2Executing Command Files\n"
      "3Processing Command Files\n"
      " The simulator can invoke another script file with the \"`DO`\" command:\n\n"
      "++DO <filename> {arguments...}       execute commands in specified file\n\n"
      " The \"`DO`\" command allows command files to contain substitutable\n"
      " arguments. The string \"`%%n`\", where \"`n`\" is a number\n"
      " between \"`1`\" and \"`9`\", is replaced with argument \"`n`\" from\n"
      " the \"`DO`\" command line. (*i.e.* \"`%%0`\", \"`%%1`\", \"`%%2`\", etc.).\n"
      " The string \"`%%0`\" is replaced with \"<`filename`>\".\n The\n"
      " sequences \"`\\%%`\" and \"`\\\\`\" are replaced with the literal\n"
      " characters \"`%%`\" and \"`\\`\", respectively. Arguments with spaces must\n"
      " be enclosed in matching single or double quotation marks.\n\n"
      " * **NOTE**: Nested \"`DO`\" commands are supported, up to ten invocations\n"
      " deep.\n\n"
      "4Switches\n"
      "5`-v`\n\n"
      " If the switch \"`-v`\" is specified, commands in the command file are\n"
      " echoed *before* they are executed.\n\n"
      "5`-e`\n\n"
      " If the switch \"`-e`\" is specified, command processing (including nested\n"
      " command invocations) will be aborted if any command error is encountered.\n"
      " (A simulation stop **never** aborts processing; use `ASSERT` to catch\n"
      " unexpected stops.) Without this switch, all errors except `ASSERT` failures\n"
      " will be ignored, and command processing will continue.\n\n"
      "5`-o`\n\n"
      " If the switch \"`-o`\" is specified, the `ON` conditions and actions from\n"
      " the calling command file will be inherited by the command file being\n"
      " invoked.\n"
      "5`-q`\n\n"
      " If the switch \"`-q`\" is specified, *quiet mode* will be explicitly\n"
      " enabled for the called command file, otherwise the *quiet mode* setting\n"
      " is inherited from the calling context.\n"
       /***************** 80 character line width template *************************/
#define HLP_GOTO        "*Commands Executing_Command_Files GOTO"
      "3GOTO\n"
      " Commands in a command file execute in sequence until either an error\n"
      " trap occurs (when a command completes with an error status), or when an\n"
      " explicit request is made to start command execution elsewhere with\n"
      " the `GOTO` command:\n\n"
      "++GOTO <label>\n\n"
      " * Labels are lines in a command file which the first non-whitespace\n"
      " character is a \"`:`\".\n"
      " * The target of a `GOTO` is the first matching label in the current `DO`\n"
      " command file which is encountered.\n\n"
      " **Example**\n\n"
      " The following example illustrates usage of the `GOTO` command (by\n"
      " creating an infinite loop):\n\n"
      "++:Label\n"
      "++:: This is a loop.\n"
      "++GOTO Label\n\n"
#define HLP_RETURN      "*Commands Executing_Command_Files RETURN"
       /***************** 80 character line width template *************************/
      "3RETURN\n"
      " The `RETURN` command causes the current procedure call to be restored to\n"
      " the calling context, possibly returning a specific return status.\n"
      " If no return status is specified, the return status from the last command\n"
      " executed will be returned.  The calling context may have `ON` traps defined\n"
      " which may redirect command flow in that context.\n\n"
      "++RETURN                 return from command file with last command status\n"
      "++RETURN {-Q} <status>   return from command file with specific status\n\n"
      " * The status return can be any numeric value or one of the standard SCPE_\n"
      " condition names.\n\n"
      " * The \"`-Q`\" switch on the `RETURN` command will cause the specified\n"
      " status to be returned, but normal error status message printing to be\n"
      " suppressed.\n\n"
      " **Condition Names**\n\n"
      " The available standard SCPE_ condition names and their meanings are:\n\n"
      " | Name    | Meaning                         | Name    | Meaning                             |\n"
      " | ------- | --------------------------------| ------- | ----------------------------------- |\n"
      " | NXM     | Address space exceeded          | UNATT   | Unit not attached                   |\n"
      " | IOERR   | I/O error                       | CSUM    | Checksum error                      |\n"
      " | FMT     | Format error                    | NOATT   | Unit not attachable                 |\n"
      " | OPENERR | File open error                 | MEM     | Memory exhausted                    |\n"
      " | ARG     | Invalid argument                | STEP    | Step expired                        |\n"
      " | UNK     | Unknown command                 | RO      | Read only argument                  |\n"
      " | INCOMP  | Command not completed           | STOP    | Simulation stopped                  |\n"
      " | EXIT    | Goodbye                         | TTIERR  | Console input I/O error             |\n"
      " | TTOERR  | Console output I/O error        | EOF     | End of file                         |\n"
      " | REL     | Relocation error                | NOPARAM | No settable parameters              |\n"
      " | ALATT   | Unit already attached           | TIMER   | Hardware timer error                |\n"
      " | SIGERR  | Signal handler setup error      | TTYERR  | Console terminal setup error        |\n"
      " | NOFNC   | Command not allowed             | UDIS    | Unit disabled                       |\n"
      " | NORO    | Read only operation not allowed | INVSW   | Invalid switch                      |\n"
      " | MISVAL  | Missing value                   | 2FARG   | Too few arguments                   |\n"
      " | 2MARG   | Too many arguments              | NXDEV   | Non-existent device                 |\n"
      " | NXUN    | Non-existent unit               | NXREG   | Non-existent register               |\n"
      " | NXPAR   | Non-existent parameter          | NEST    | Nested DO command limit exceeded    |\n"
      " | IERR    | Internal error                  | MTRLNT  | Invalid magtape record length       |\n"
      " | LOST    | Console Telnet connection lost  | TTMO    | Console Telnet connection timed out |\n"
      " | STALL   | Console Telnet output stall     | AFAIL   | Assertion failed                    |\n"
      " | INVREM  | Invalid remote console command  |         |                                     |\n"
      "\n\n"
#define HLP_SHIFT       "*Commands Executing_Command_Files Shift_Parameters"
      "3Shift Parameters\n"
      " Shift the command files positional parameters\n"
#define HLP_CALL        "*Commands Executing_Command_Files Call_a_subroutine"
      "3Call a subroutine\n"
      " Control can be transferred to a labeled subroutine using `CALL`.\n\n"
      " **Example**\n\n"
      "++CALL routine\n"
      "++BYE\n"
      "++\n"
      "++:routine\n"
      "++ECHO routine called\n"
      "++RETURN\n\n"
#define HLP_ON          "*Commands Executing_Command_Files ON"
      "3ON\n"
      " The `ON` command performs actions after a condition, or clears a condition.\n"
      "++ON <condition> <action>  Perform action after condition\n"
      "++ON <condition>           Clears action of specified condition\n"
#define HLP_PROCEED     "*Commands Executing_Command_Files PROCEED_or_IGNORE"
#define HLP_IGNORE      "*Commands Executing_Command_Files PROCEED_or_IGNORE"
       /***************** 80 character line width template *************************/
      "3PROCEED or IGNORE\n"
      " The `PROCEED` (or `IGNORE`) command does nothing.  It is potentially\n"
      " useful as a placeholder for any `ON` action condition that should be\n"
      " explicitly ignored, allowing command file execution to continue without\n"
      " taking any specific action.\n"
#define HLP_ECHO        "*Commands Executing_Command_Files Displaying_Arbitrary_Text"
       /***************** 80 character line width template *************************/
      "3Displaying Arbitrary Text\n"
      " The `ECHO` command is a useful way of annotating command files.  `ECHO`\n"
      " prints out its arguments to the console (and to any applicable log file):\n\n"
      "++ECHO <string>      Output string to console\n\n"
      " **NOTE**: If no arguments are specified, `ECHO` prints a blank line.\n"
      " This may be used to provide spacing for console messages or log file\n"
      " output.\n"
       /***************** 80 character line width template *************************/
#define HLP_ASSERT      "*Commands Executing_Command_Files Testing_Assertions"
      "3Testing Assertions\n"
      " The `ASSERT` command tests a simulator state condition and halts command\n"
      " file execution if the condition is false:\n\n"
      "++ASSERT <Simulator State Expressions>\n\n"
      " * If the indicated expression evaluates to false, the command completes\n"
      " with an `AFAIL` condition.  By default, when a command file encounters a\n"
      " command which returns the `AFAIL` condition, it will exit the running\n"
      " command file with the `AFAIL` status to the calling command file.  This\n"
      " behavior can be changed with the `ON` command as well as switches to the\n"
      " invoking `DO` command.\n\n"
      " **Examples**\n\n"
      " The command file below might be used to bootstrap a hypothetical system\n"
      " that halts after the initial load from disk. The `ASSERT` command can then\n"
      " be used to confirm that the load completed successfully by examining the\n"
      " CPU's \"`A`\" register for the expected value:\n\n"
      "++; Example INI file\n"
      "++BOOT\n"
      "++; A register contains error code; 0 = good boot\n"
      "++ASSERT A=0\n"
      "++RUN\n\n"
       /***************** 80 character line width template *************************/
      " * In the above example, if the \"`A`\" register is *not* `0`,\n"
      " the \"`ASSERT A=0`\" command will be displayed to the user, and the\n"
      " command file will be aborted with an \"`Assertion failed`\" message.\n"
      " Otherwise, the command file will continue to bring up the system.\n\n"
      " * See the **`IF`** command documentation for more information and details\n"
      " regarding simulator state expressions.\n\n"
#define HLP_IF          "*Commands Executing_Command_Files Testing_Conditions"
      "3Testing Conditions\n"
      " The `IF` command tests a simulator state condition and executes additional\n"
      " commands if the condition is true:\n\n"
      "++IF <Simulator State Expressions> commandtoprocess{; additionalcommand}...\n\n"
      " **Examples**\n\n"
      " The command file below might be used to bootstrap a hypothetical system\n"
      " that halts after the initial load from disk. The `IF` command can then\n"
      " be used to confirm that the load completed successfully by examining the\n"
      " CPU's \"`A`\" register for an expected value:\n\n"
      "++; Example INI file\n"
      "++BOOT\n"
      "++; A register contains error code; 0 = good boot\n"
      "++IF NOT A=0 echo Boot failed - Failure Code ; EX A; exit AFAIL\n"
      "++RUN\n\n"
       /***************** 80 character line width template *************************/
      " * In the above example, if the \"`A`\" register is *not* `0`, the\n"
      " message \"`Boot failed - Failure Code `\" will be displayed, the contents\n"
      " of the \"`A`\" register will be displayed, and the command file will be\n"
      " aborted with an \"`Assertion failed`\" message.  Otherwise, the command\n"
      " file will continue to bring up the system.\n"
      "4Conditional Expressions\n"
      " The `IF` and `ASSERT` commands evaluate the following two different forms\n"
      " of conditional expressions.\n\n"
      "5Simulator State Expressions\n"
      "  &nbsp;\n \n"
      " The values of simulator registers can be evaluated with:\n\n"
      "++{NOT} {<dev>} <reg>|<addr>{<logical-op><value>}<conditional-op><value>\n\n"
      " * If \"<`dev`>\" is not specified, `CPU` is assumed.  \"<`reg`>\" is a\n"
      " register belonging to the indicated device.\n"
      " * The \"<`addr`>\" is an address in the address space of the indicated\n"
      " device.\n"
      " * The \"<`conditional-op`>\" and optional \"<`logical-op`>\" are\n"
      " the same as those used for \"search specifiers\" by the `EXAMINE` and\n"
      " `DEPOSIT` commands.\n"
      " The \"<`value`>\" is expressed in the radix specified for \"<`reg`>\",\n"
      " not in the radix for the device when referencing a register; when an\n"
      " address is referenced the device radix is used as the default.\n\n"
      " * If \"<`logical-op`>\" and \"<`value`>\" are specified, the target\n"
      " register value is first altered as indicated.  The result is then compared\n"
      " to the \"<`value`>\" via the \"<`conditional-op`>\".\n"
      " * * If the result is *true*, the command(s) are executed before proceeding\n"
      " to the next line in the command file.\n"
      " * * If the result is *false*, the next command in the command file is\n"
      " processed.\n\n"
      "5String Comparison Expressions\n"
      "  &nbsp;\n \n"
      " String Values can be compared with:\n\n"
      "++{-i} {NOT} \"<string1>\" <compare-op> \"<string2>\"\n\n"
      " * The \"`-i`\" switch, if present, causes a comparison to be case\n"
      "   insensitive.\n"
      " * The \"<`string1`>\" and \"<`string2`>\" arguments are quoted string\n"
      "   values which may have environment variables substituted as desired.\n"
      " * The \"<`compare-op`>\" may be one of:\n\n"
      " |                |                        |\n"
      " | --------------:|:---------------------- |\n"
      " | \"**`==`**\"     |  equal                 |\n"
      " | \"**`EQU`**\"    |  equal                 |\n"
      " | \"**`!=`**\"     |  not equal             |\n"
      " | \"**`NEQ`**\"    |  not equal             |\n"
      " | \"**<**\"        |  less than             |\n"
      " | \"**`LSS`**\"    |  less than             |\n"
      " | \"**<=**\"       |  less than or equal    |\n"
      " | \"**`LEQ`**\"    |  less than or equal    |\n"
      " | \"**>**\"        |  greater than          |\n"
      " | \"**`GTR`**\"    |  greater than          |\n"
      " | \"**>=**\"       |  greater than or equal |\n"
      " | \"**`GEQ`**\"    |  greater than or equal |\n"
      " * **NOTE**: Comparisons are *generic*.  This means that if\n"
      " both \"<`string1`>\" and \"<`string2`>\" are comprised of all numeric\n"
      " digits, then the strings are converted to numbers and a numeric\n"
      " comparison is performed.  For example, the comparison\n"
      " '`\"+1\"` `EQU` `\"1\"`' evaluates to *true*.\n"
       /***************** 80 character line width template *************************/
#define HLP_EXIT        "*Commands Exiting_the_Simulator"
      "2Exiting the Simulator\n"
      " The `EXIT` command (*synonyms* `QUIT` *and* `BYE`) exits the simulator,\n"
      " returning control to the host operating system.\n"
       /***************** 80 character line width template *************************/
#define HLP_SPAWN       "*Commands Executing_System_Commands"
      "2Executing System Commands\n"
      " * The simulator can execute host operating system commands with\n"
      " the \"`!`\" (*spawn*) command.\n\n"
      " |                         |                                             |\n"
      " |:----------------------- |:------------------------------------------- |\n"
      " | \"**`!`**\"             | Spawn the hosts default command interpreter |\n"
      " | \"**`!`** <`command`>\" | Execute the host operating system `command` |\n\n"
      " * **NOTE**: The *exit status* from the command which was executed is set\n"
      " as the *command completion status* for the \"`!`\" command.  This may\n"
      " influence any enabled `ON` condition traps.\n" ;
       /***************** 80 character line width template *************************/

static CTAB cmd_table[] = {
    { "RESET",    &reset_cmd,  0,         HLP_RESET     },
    { "EXAMINE",  &exdep_cmd,  EX_E,      HLP_EXAMINE   },
    { "IEXAMINE", &exdep_cmd,  EX_E+EX_I, HLP_IEXAMINE  },
    { "DEPOSIT",  &exdep_cmd,  EX_D,      HLP_DEPOSIT   },
    { "IDEPOSIT", &exdep_cmd,  EX_D+EX_I, HLP_IDEPOSIT  },
    { "EVALUATE", &eval_cmd,   0,         HLP_EVALUATE  },
    { "RUN",      &run_cmd,    RU_RUN,    HLP_RUN,      NULL, &run_cmd_message },
    { "GO",       &run_cmd,    RU_GO,     HLP_GO,       NULL, &run_cmd_message },
    { "STEP",     &run_cmd,    RU_STEP,   HLP_STEP,     NULL, &run_cmd_message },
    { "NEXT",     &run_cmd,    RU_NEXT,   HLP_NEXT,     NULL, &run_cmd_message },
    { "CONTINUE", &run_cmd,    RU_CONT,   HLP_CONTINUE, NULL, &run_cmd_message },
    { "BOOT",     &run_cmd,    RU_BOOT,   HLP_BOOT,     NULL, &run_cmd_message },
    { "BREAK",    &brk_cmd,    SSH_ST,    HLP_BREAK     },
    { "NOBREAK",  &brk_cmd,    SSH_CL,    HLP_NOBREAK   },
    { "ATTACH",   &attach_cmd, 0,         HLP_ATTACH    },
    { "DETACH",   &detach_cmd, 0,         HLP_DETACH    },
    { "EXIT",     &exit_cmd,   0,         HLP_EXIT      },
    { "QUIT",     &exit_cmd,   0,         NULL          },
    { "BYE",      &exit_cmd,   0,         NULL          },
    { "SET",      &set_cmd,    0,         HLP_SET       },
    { "SHOW",     &show_cmd,   0,         HLP_SHOW      },
    { "DO",       &do_cmd,     1,         HLP_DO        },
    { "GOTO",     &goto_cmd,   1,         HLP_GOTO      },
    { "RETURN",   &return_cmd, 0,         HLP_RETURN    },
    { "SHIFT",    &shift_cmd,  0,         HLP_SHIFT     },
    { "CALL",     &call_cmd,   0,         HLP_CALL      },
    { "ON",       &on_cmd,     0,         HLP_ON        },
    { "IF",       &assert_cmd, 0,         HLP_IF        },
    { "PROCEED",  &noop_cmd,   0,         HLP_PROCEED   },
    { "IGNORE",   &noop_cmd,   0,         HLP_IGNORE    },
    { "ECHO",     &echo_cmd,   0,         HLP_ECHO      },
    { "ASSERT",   &assert_cmd, 1,         HLP_ASSERT    },
    { "SEND",     &send_cmd,   0          },            /* deprecated */
    { "EXPECT",   &expect_cmd, 1          },            /* deprecated */
    { "NOEXPECT", &expect_cmd, 0          },            /* deprecated */
    { "!",        &spawn_cmd,  0,         HLP_SPAWN     },
    { "HELP",     &help_cmd,   0,         HLP_HELP      },
    { NULL,       NULL,        0          }
    };

static CTAB set_glob_tab[] = {
    { "CONSOLE",     &sim_set_console,        0      }, /* deprecated */
    { "REMOTE",      &sim_set_remote_console, 0      }, /* deprecated */
    { "BREAK",       &brk_cmd,                SSH_ST }, /* deprecated */
    { "NOBREAK",     &brk_cmd,                SSH_CL }, /* deprecated */
    { "TELNET",      &sim_set_telnet,         0      }, /* deprecated */
    { "NOTELNET",    &sim_set_notelnet,       0      }, /* deprecated */
    { "LOG",         &sim_set_logon,          0,     HLP_SET_LOG      },
    { "NOLOG",       &sim_set_logoff,         0,     HLP_SET_LOG      },
    { "DEBUG",       &sim_set_debon,          0,     HLP_SET_DEBUG    },
    { "NODEBUG",     &sim_set_deboff,         0,     HLP_SET_DEBUG    },
    { "ENVIRONMENT", &sim_set_environment,    1,     HLP_SET_ENVIRON  },
    { "ON",          &set_on,                 1,     HLP_SET_ON       },
    { "NOON",        &set_on,                 0,     HLP_SET_ON       },
    { "VERIFY",      &set_verify,             1,     HLP_SET_VERIFY   },
    { "VERBOSE",     &set_verify,             1,     HLP_SET_VERIFY   },
    { "NOVERIFY",    &set_verify,             0,     HLP_SET_VERIFY   },
    { "NOVERBOSE",   &set_verify,             0,     HLP_SET_VERIFY   },
    { "MESSAGE",     &set_message,            1,     HLP_SET_MESSAGE  },
    { "NOMESSAGE",   &set_message,            0,     HLP_SET_MESSAGE  },
    { "QUIET",       &set_quiet,              1,     HLP_SET_QUIET    },
    { "NOQUIET",     &set_quiet,              0,     HLP_SET_QUIET    },
    { "LOCALOPC",    &set_localopc,           1,     HLP_SET_LOCALOPC },
    { "NOLOCALOPC",  &set_localopc,           0,     HLP_SET_LOCALOPC },
    { "PROMPT",      &set_prompt,             0,     HLP_SET_PROMPT   },
    { NULL,          NULL,                    0      }
    };

static C1TAB set_dev_tab[] = {
    { "OCTAL",    &set_dev_radix,   8  },
    { "DECIMAL",  &set_dev_radix,  10  },
    { "HEX",      &set_dev_radix,  16  },
    { "ENABLED",  &set_dev_enbdis,  1  },
    { "DISABLED", &set_dev_enbdis,  0  },
    { "DEBUG",    &set_dev_debug,   1  },
    { "NODEBUG",  &set_dev_debug,   0  },
    { NULL,       NULL,             0  }
    };

static C1TAB set_unit_tab[] = {
    { "ENABLED",  &set_unit_enbdis, 1 },
    { "DISABLED", &set_unit_enbdis, 0 },
    { NULL,       NULL,             0 }
    };

static SHTAB show_glob_tab[] = {
    { "CONFIGURATION",  &show_config,        0, HLP_SHOW_CONFIG    },
    { "DEFAULT_BASE_SYSTEM_SCRIPT",
           &show_default_base_system_script, 0, HLP_SHOW_DBS       },
    { "DEVICES",   &show_config,             1, HLP_SHOW_DEVICES   },
    { "FEATURES",  &show_config,             2, HLP_SHOW_FEATURES  },
    { "QUEUE",     &show_queue,              0, HLP_SHOW_QUEUE     },
    { "TIME",      &show_time,               0, HLP_SHOW_TIME      },
    { "MODIFIERS", &show_mod_names,          0, HLP_SHOW_MODIFIERS },
    { "NAMES",     &show_log_names,          0, HLP_SHOW_NAMES     },
    { "SHOW",      &show_show_commands,      0, HLP_SHOW_SHOW      },
    { "VERSION",   &show_version,            1, HLP_SHOW_VERSION   },
    { "BUILDINFO", &show_buildinfo,          1, HLP_SHOW_BUILDINFO },
    { "PROM",      &show_prom,               0, HLP_SHOW_PROM      },
    { "CONSOLE",   &sim_show_console,        0, HLP_SHOW_CONSOLE   },
    { "REMOTE",    &sim_show_remote_console, 0, HLP_SHOW_REMOTE    },
    { "BREAK",     &show_break,              0, HLP_SHOW_BREAK     },
    { "LOG",       &sim_show_log,            0, HLP_SHOW_LOG       },
    { "TELNET",    &sim_show_telnet,         0  },  /* deprecated */
    { "DEBUG",     &sim_show_debug,          0, HLP_SHOW_DEBUG     },
    { "CLOCKS",    &sim_show_timers,         0, HLP_SHOW_CLOCKS    },
    { "SEND",      &sim_show_send,           0, HLP_SHOW_SEND      },
    { "EXPECT",    &sim_show_expect,         0, HLP_SHOW_EXPECT    },
    { "ON",        &show_on,                 0, HLP_SHOW_ON        },
    { NULL,        NULL,                     0  }
    };

static SHTAB show_dev_tab[] = {
    { "RADIX",     &show_dev_radix,         0 },
    { "DEBUG",     &show_dev_debug,         0 },
    { "MODIFIERS", &show_dev_modifiers,     0 },
    { "NAMES",     &show_dev_logicals,      0 },
    { "SHOW",      &show_dev_show_commands, 0 },
    { NULL,        NULL,                    0 }
    };

static SHTAB show_unit_tab[] = {
    { NULL, NULL, 0 }
    };

#if defined(_WIN32)
static
int setenv(const char *envname, const char *envval, int overwrite)
{
char *envstr = (char *)malloc(strlen(envname)+strlen(envval)+2);
int r;

(void)sprintf(envstr, "%s=%s", envname, envval);
r = _putenv(envstr);
FREE(envstr);
return r;
}

static
int unsetenv(const char *envname)
{
setenv(envname, "", 1);
return 0;
}
#endif /* if defined(_WIN32) */

#define XSTR_EMAXLEN 32767

const char
*xstrerror_l(int errnum)
{
  int saved = errno;
  const char *ret = NULL;
  static __thread char buf[XSTR_EMAXLEN];

#if defined(__APPLE__) || defined(_AIX) || defined(__MINGW32__) || \
    defined(__MINGW64__) || defined(CROSS_MINGW32) || defined(CROSS_MINGW64)
# if defined(__MINGW32__) || defined(__MINGW64__) || defined(CROSS_MINGW32) || defined(CROSS_MINGW64)
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

t_stat process_stdin_commands (t_stat stat, char *argv[]);

/* Check if running on Rosetta 2 */

#if defined(__APPLE__)
int processIsTranslated(void)
{
    int ret = 0;
    size_t size = sizeof(ret);
    if (sysctlbyname("sysctl.proc_translated", &ret, &size, NULL, 0) == -1) {
        if (errno == ENOENT)
            return 0;
        return -1; }
    return ret;
}
#endif /* if defined(_APPLE_) */

/* Substring removal hack */

char *strremove(char *str, const char *sub)
{
    char *p, *q, *r;
    if (*sub && (q = r = strstr(str, sub)) != NULL) {
        size_t len = strlen(sub);
        while ((r = strstr(p = r + len, sub)) != NULL) {
            while (p < r)
                *q++ = *p++;
        }
        while ((*q++ = *p++) != '\0')
            continue;
    }
    return str;
}

/* Trim whitespace */

void strtrimspace (char *str_trimmed, const char *str_untrimmed)
{
    while (*str_untrimmed != '\0') {
      if(!isspace((unsigned char)*str_untrimmed)) {
        *str_trimmed = (char)*str_untrimmed;
        str_trimmed++;
      }
      str_untrimmed++;
    }
    *str_trimmed = '\0';
}

#if !defined(_WIN32) && !defined(__MINGW32__) && !defined(__MINGW64__) && !defined(CROSS_MINGW32) && !defined(CROSS_MINGW64)
void allowCores(void)
{
  int ret;
  struct rlimit limit;
# if defined(RLIMIT_CORE)
  ret = getrlimit(RLIMIT_CORE, &limit);
  (void)ret;
#  if defined(TESTING)
  if (ret != 0)
    {
      sim_warn ("Failed to query core dump configuration.");
      return;
    }
#  endif /* if defined(TESTING) */
  limit.rlim_cur = limit.rlim_max;
  ret = setrlimit(RLIMIT_CORE, &limit);
#  if defined(TESTING)
  if (ret != 0)
    {
      sim_warn ("Failed to enable unlimited core dumps.");
      return;
    }
#  endif /* if defined(TESTING) */
# else
#  if defined(TESTING)
  sim_warn ("Unable to query core dump configuration.");
#  endif /* if defined(TESTING) */
# endif /* if defined(RLIMIT_CORE) */
  return;
}
#endif

#if defined(USE_DUMA)
void CleanDUMA(void)
{
  (void)fflush(stdout);
  DUMA_CHECKALL();
  (void)fflush(stderr);
}
# undef USE_DUMA
# define USE_DUMA 1
#endif /* if defined(USE_DUMA) */

#if !defined(SIG_SETMASK)
# undef USE_BACKTRACE
#endif /* if !defined(SIG_SETMASK) */

#if defined(PERF_STRIP)
# undef USE_BACKTRACE
#endif /* if defined(PERF_STRIP) */

#if defined(USE_BACKTRACE)
# include <backtrace.h>
# include <backtrace-supported.h>
# define BACKTRACE_SKIP 1
# define BACKTRACE_MAIN "main"
# undef USE_BACKTRACE
# define USE_BACKTRACE 1
#endif /* if defined(USE_BACKTRACE) */

#if defined(BACKTRACE_SUPPORTED)
# if defined(BACKTRACE_SUPPORTS_THREADS)
#  if !(BACKTRACE_SUPPORTED)
#   undef USE_BACKTRACE
#  endif /* if !(BACKTRACE_SUPPORTED) */
# else  /* if defined(BACKTRACE_SUPPORTS_THREADS) */
#  undef USE_BACKTRACE
# endif /* if defined(BACKTRACE_SUPPORTS_THREADS) */
#else  /* if defined(BACKTRACE_SUPPORTED) */
# undef USE_BACKTRACE
#endif /* if defined(BACKTRACE_SUPPORTED) */

#if defined(USE_BACKTRACE)
# if defined(BACKTRACE_SUPPORTED)
#  include "backtrace_func.c"
# endif /* if defined(BACKTRACE_SUPPORTED) */
#endif /* if defined(USE_BACKTRACE) */

#if !defined(__CYGWIN__)
# if !defined(__APPLE__)
#  if !defined(_AIX)
#   if !defined(__MINGW32__)
#    if !defined(__MINGW64__)
#     if !defined(CROSS_MINGW32)
#      if !defined(CROSS_MINGW64)
#       if !defined(_WIN32)
#        if !defined(__HAIKU__)
static int
dl_iterate_phdr_callback (struct dl_phdr_info *info, size_t size, void *data)
{
  (void)size;
  (void)data;

  if (strlen(info->dlpi_name) >= 2) {
      if (!dl_iterate_phdr_callback_called)
          (void)printf ("\r\n Loaded shared objects: ");

      dl_iterate_phdr_callback_called++;
      (void)printf ("%s ", info->dlpi_name);
  }

  return 0;
}
#        endif
#       endif
#      endif
#     endif
#    endif
#   endif
#  endif
# endif
#endif

#if defined(__MINGW32__) || defined(__MINGW64__) || defined(CROSS_MINGW32) || defined(CROSS_MINGW64)
# if defined(_UCRT)
#  define MINGW_CRT "UCRT"
# else
#  define MINGW_CRT "MSVCRT"
# endif
#endif

#if defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || defined(CROSS_MINGW32) || defined(CROSS_MINGW64)
struct UCRTVersion
{
  uint16_t ProductVersion[4];
};

int
GetUCRTVersion (struct UCRTVersion *ucrtversion)
{
# ifdef _DEBUG
  static const wchar_t *DllName = L"ucrtbased.dll";
# else
  static const wchar_t *DllName = L"ucrtbase.dll";
# endif

  HMODULE ucrt = GetModuleHandleW (DllName);
  if (!ucrt)
    return GetLastError ();

  wchar_t path[MAX_PATH];
  if (!GetModuleFileNameW (ucrt, path, MAX_PATH))
    return GetLastError ();

  DWORD versionInfoSize = GetFileVersionInfoSizeW (path, NULL);
  if (!versionInfoSize)
    return GetLastError ();

  uint8_t versionInfo[versionInfoSize];

  if (!GetFileVersionInfoW (path, 0, versionInfoSize, versionInfo))
    return GetLastError ();

  VS_FIXEDFILEINFO *fixedFileInfo;
  UINT fixedFileInfoSize;
  if (!VerQueryValueW (versionInfo, L"\\", (void **)&fixedFileInfo, &fixedFileInfoSize))
    return GetLastError ();

  memcpy (ucrtversion->ProductVersion, &fixedFileInfo->dwProductVersionMS, sizeof (uint32_t) * 2);

  return 0;
}
#endif

/* Main command loop */

#if !defined(PERF_STRIP)
int main (int argc, char *argv[])
{
char *cptr, *cptr2;
char nbuf[PATH_MAX + 7];
char cbuf[4*CBUFSIZE];
char **targv = NULL;
# if defined(USE_BACKTRACE)
#  if defined(BACKTRACE_SUPPORTED)
#   if defined(_INC_BACKTRACE_FUNC)
bt_pid = (long)getpid();
(void)bt_pid;
#   endif /* if defined(_INC_BACKTRACE_FUNC) */
#  endif /* if defined(BACKTRACE_SUPPORTED) */
# endif /* if defined(USE_BACKTRACE) */
int32 i, sw;
t_bool lookswitch;
t_stat stat;

# if defined(__MINGW32__)
#  undef IS_WINDOWS
#  define IS_WINDOWS 1
#  if !defined(NEED_CONSOLE_SETUP)
#   define NEED_CONSOLE_SETUP
#  endif /* if !defined(NEED_CONSOLE_SETUP) */
# endif /* if defined(__MINGW32__)*/

# if defined(CROSS_MINGW32)
#  undef IS_WINDOWS
#  define IS_WINDOWS 1
#  if !defined(NEED_CONSOLE_SETUP)
#   define NEED_CONSOLE_SETUP
#  endif /* if !defined(NEED_CONSOLE_SETUP) */
# endif /* if defined(CROSS_MINGW32) */

# if defined(__MINGW64__)
#  undef IS_WINDOWS
#  define IS_WINDOWS 1
#  if !defined(NEED_CONSOLE_SETUP)
#   define NEED_CONSOLE_SETUP
#  endif /* if !defined(NEED_CONSOLE_SETUP) */
# endif /* if defined(__MINGW64__) */

# if defined(CROSS_MINGW64)
#  undef IS_WINDOWS
#  define IS_WINDOWS 1
#  if !defined(NEED_CONSOLE_SETUP)
#   define NEED_CONSOLE_SETUP
#  endif /* if !defined(NEED_CONSOLE_SETUP */
# endif /* if defined(CROSS_MINGW64) */

# if defined(__CYGWIN__)
#  if defined(IS_WINDOWS)
#   undef IS_WINDOWS
#  endif /* if defined(IS_WINDOWS) */
# endif /* if defined(__CYGWIN__) */

# if defined(USE_DUMA)
#  if defined(DUMA_EXPLICIT_INIT)
duma_init();
(void)fflush(stderr);
#  endif /* if defined(DUMA_EXPLICIT_INIT) */
#  if defined(DUMA_MIN_ALIGNMENT)
#   if DUMA_MIN_ALIGNMENT > 0
DUMA_SET_ALIGNMENT(DUMA_MIN_ALIGNMENT);
#   endif /* if DUMA_MIN_ALIGNMENT > 0 */
#  endif /* if defined(DUMA_MIN_ALIGNMENT) */
DUMA_SET_FILL(0x2E);
(void)fflush(stderr);
(void)atexit(CleanDUMA);
# endif /* if defined(USE_DUMA) */

# if defined(USE_BACKTRACE)
#  if defined(BACKTRACE_SUPPORTED)
#   include "backtrace_main.c"
#  endif /* if defined(BACKTRACE_SUPPORTED) */
# endif /* if defined(USE_BACKTRACE) */

(void)setlocale(LC_ALL, "");

# if defined(NEED_CONSOLE_SETUP) && defined(_WIN32)
#  if !defined(ENABLE_VIRTUAL_TERMINAL_PROCESSING)
#   define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#  endif /* if !defined(ENABLE_VIRTUAL_TERMINAL_PROCESSING) */
# endif /* if defined(NEED_CONSOLE_SETUP) && defined(_WIN32) */

# if defined(NEED_CONSOLE_SETUP)
HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
if (handle != INVALID_HANDLE_VALUE)
  {
    DWORD mode = 0;
    if (GetConsoleMode(handle, &mode))
      {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(handle, mode);
      }
  }
puts ("\e[0m");
# endif /* if defined(NEED_CONSOLE_SETUP) */

# if defined(__HAIKU__)
(void)disable_debugger(1);
# endif /* if defined(__HAIKU__) */

/* sanity checks */

# if defined(__clang_analyzer__)
(void)fprintf (stderr, "Error: Attempting to execute a Clang Analyzer build!\n");
return 1;
# endif /* if defined(__clang_analyzer__) */

if (argc == 0) {
    (void)fprintf (stderr, "Error: main() called directly!\n");
    return 1;
}

/* Enable unlimited core dumps */
# if !defined(_WIN32) && !defined(__MINGW32__) && !defined(__MINGW64__) && !defined(CROSS_MINGW32) && !defined(CROSS_MINGW64)
allowCores();
# endif

int testEndian = decContextTestEndian();
if (testEndian != 0) {
  if (testEndian == 1) {
    (void)fprintf (stderr,
                   "Error: Compiled for big-endian, but little-endian ordering detected; aborting.\n");
    return 1;
  }
  if (testEndian == -1) {
    (void)fprintf (stderr,
                   "Error: Compiled for little-endian, but big-endian ordering detected; aborting.\n");
    return 1;
  }
  (void)fprintf (stderr,
                 "Error: Unable to determine system byte order; aborting.\n");
  return 1;
}
# if defined(NEED_128)
test_math128();
# endif /* if defined(NEED_128) */

/* Make sure that argv has at least 10 elements and that it ends in a NULL pointer */
targv = (char **)calloc (1+MAX(10, argc), sizeof(*targv));
if (!targv)
  {
    (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                   __func__, __FILE__, __LINE__);
# if defined(USE_BACKTRACE)
#  if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
#  endif /* if defined(SIGUSR2) */
# endif /* if defined(USE_BACKTRACE) */
    abort();
  }
for (i=0; i<argc; i++)
    targv[i] = argv[i];
argv = targv;

/* setup defaults */
set_prompt (0, "sim>");                                 /* start with set standard prompt */
*cbuf = 0;                                              /* init arg buffer */
sim_switches = 0;                                       /* init switches */
lookswitch = TRUE;
stdnul = fopen(NULL_DEVICE,"wb");

/* process arguments */
for (i = 1; i < argc; i++) {                            /* loop thru args */
    if (argv[i] == NULL)                                /* paranoia */
        continue;

# if defined(THREADZ) || defined(LOCKLESS)
/* performance test */
    int perftestflag  = strcmp(argv[i], "--perftest");
    if (perftestflag == 0) {
      char * testName = NULL;
      if (i + 1 < argc)
        testName = argv[i + 1];
      perfTest (testName);
      return 0;
    }
# endif

/* requested only version? */
    int onlyvers  = strcmp(argv[i], "--version");
    if (onlyvers == 0) {
# if defined(VER_H_GIT_VERSION)
#  if defined(VER_H_GIT_PATCH) && defined(VER_H_GIT_PATCH_INT)
#   if VER_H_GIT_PATCH_INT < 1
        (void)fprintf (stdout, "%s simulator %s\n",
                       sim_name, VER_H_GIT_VERSION);
#   else
        (void)fprintf (stdout, "%s simulator %s+%s\n",
                       sim_name, VER_H_GIT_VERSION, VER_H_GIT_PATCH);
#   endif /* if VER_H_GIT_PATCH_INT < 1 */
#  else
        (void)fprintf (stdout, "%s simulator %s\n",
                       sim_name, VER_H_GIT_VERSION);
#  endif /* if defined(VER_H_GIT_PATCH) && defined(VER_H_GIT_PATCH_INT) */
# else
        (void)fprintf (stdout, "%s simulator\n", sim_name);
# endif /* if defined(VER_H_GIT_VERSION) */
        FREE (targv);
        return 0;
    }

/* requested short or long help? */
    int longhelp  = strcmp(argv[i], "--help");
    int shorthelp = strcmp(argv[i], "-h");
    if (shorthelp != 0) shorthelp = strcmp(argv[i], "-H");
    if (longhelp == 0 || shorthelp == 0) {
# if defined(VER_H_GIT_VERSION)
#  if defined(VER_H_GIT_PATCH) && defined(VER_H_GIT_PATCH_INT)
#   if VER_H_GIT_PATCH_INT < 1
        (void)fprintf (stdout, "%s simulator %s", sim_name, VER_H_GIT_VERSION);
#   else
        (void)fprintf (stdout, "%s simulator %s+%s", sim_name, VER_H_GIT_VERSION, VER_H_GIT_PATCH);
#   endif /* if VER_H_GIT_PATCH_INT < 1 */
#  else
        (void)fprintf (stdout, "%s simulator %s", sim_name, VER_H_GIT_VERSION);
#  endif /* if defined(VER_H_GIT_PATCH) && defined(VER_H_GIT_PATCH_INT) */
# else
        (void)fprintf (stdout, "%s simulator", sim_name);
# endif /* if defined(VER_H_GIT_VERSION) */
        (void)fprintf (stdout, "\n");
        (void)fprintf (stdout, "\n USAGE: %s { [ SWITCHES ] ... } { < SCRIPT > }", argv[0]);
        (void)fprintf (stdout, "\n");
        (void)fprintf (stdout, "\n Invokes the %s simulator, with optional switches and/or script file.", sim_name);
        (void)fprintf (stdout, "\n");
        (void)fprintf (stdout, "\n Switches:");
        (void)fprintf (stdout, "\n  -e, -E            Aborts script processing immediately upon any error");
        (void)fprintf (stdout, "\n  -h, -H, --help    Prints only this informational help text and exit");
        (void)fprintf (stdout, "\n  -k, -K            Disables all support for exclusive file locking");
        (void)fprintf (stdout, "\n  -l, -L            Reports but ignores all exclusive file locking errors");
        (void)fprintf (stdout, "\n  -o, -O            Makes scripting ON conditions and actions inheritable");
        (void)fprintf (stdout, "\n  -q, -Q            Disables printing of non-fatal informational messages");
        (void)fprintf (stdout, "\n  -r, -R            Enables an unlinked ephemeral system state file");
        (void)fprintf (stdout, "\n  -s, -S            Enables a randomized persistent system state file");
        (void)fprintf (stdout, "\n  -t, -T            Disables fsync and creation/usage of system state file");
        (void)fprintf (stdout, "\n  -v, -V            Prints commands read from script file before execution");
        (void)fprintf (stdout, "\n  --version         Prints only the simulator identification text and exit");
        (void)fprintf (stdout, "\n");
# if defined(USE_DUMA)
        nodist++;
# endif /* if defined(USE_DUMA) */
if (!nodist) {
        (void)fprintf (stdout, "\n This software is made available under the terms of the ICU License.");
        (void)fprintf (stdout, "\n For complete license details, see the LICENSE file included with the");
        (void)fprintf (stdout, "\n software or https://gitlab.com/dps8m/dps8m/-/blob/master/LICENSE.md\n");
}
else
{
        (void)fprintf (stdout, "\n********** LICENSE RESTRICTED BUILD ****** NOT FOR REDISTRIBUTION **********");
}
        (void)fprintf (stdout, "\n");
        FREE(argv); //-V726
        return 0;
    }
    /* invalid arguments? */
    if ((*argv[i] == '-') && lookswitch) {              /* switch? */
        if ((sw = get_switches (argv[i])) < 0) {
            (void)fprintf (stderr, "Invalid switch \"%s\".\nTry \"%s -h\" for help.\n", argv[i], argv[0]);
            FREE(argv); //-V726
            return 1;
            }
        sim_switches = sim_switches | sw;
        }
    /* parse arguments */
    else {
        if ((strlen (argv[i]) + strlen (cbuf) + 3) >= sizeof(cbuf)) {
            (void)fprintf (stderr, "Argument string too long\n");
            FREE(argv); //-V726
            return 1;
            }
        if (*cbuf)                                  /* concat args */
            strcat (cbuf, " ");
        (void)sprintf(&cbuf[strlen(cbuf)], "%s%s%s", //-V755
                      strchr(argv[i], ' ') ? "\"" : "", argv[i], strchr(argv[i], ' ') ? "\"" : ""); //-V755
        lookswitch = FALSE;                         /* no more switches */
        }
    }                                               /* end for */
sim_nolock = sim_switches & SWMASK ('K');           /* -k means skip locking     */
sim_iglock = sim_switches & SWMASK ('L');           /* -l means ignore locking   */
sim_randompst = sim_switches & SWMASK ('S');        /* -s means persist random   */
sim_quiet = sim_switches & SWMASK ('Q');            /* -q means quiet            */
sim_randstate = sim_switches & SWMASK ('R');        /* -r means random sys_state */
if (sim_randompst) sim_randstate = 1;               /*    and is implied with -s */
sim_nostate = sim_switches & SWMASK ('T');          /* -t means no sys_state     */
if (sim_nostate)                                    /*    and disables -s and -r */
  {
    sim_randompst = 0;
    sim_randstate = 0;
  }
sim_on_inherit = sim_switches & SWMASK ('O');       /* -o means inherit on state */

sim_init_sock ();                                   /* init socket capabilities */
if (sim_dflt_dev == NULL)                           /* if no default */
    sim_dflt_dev = sim_devices[0];
if (sim_vm_init != NULL)                            /* call once only */
    (*sim_vm_init)();
sim_finit ();                                       /* init fio package */
for (i = 0; cmd_table[i].name; i++) {
    size_t alias_len = strlen (cmd_table[i].name);
    char *cmd_name = (char *)calloc (1 + alias_len, sizeof (*cmd_name));
    if (!cmd_name)
      {
        (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                       __func__, __FILE__, __LINE__);
# if defined(USE_BACKTRACE)
#  if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
#  endif /* if defined(SIGUSR2) */
# endif /* if defined(USE_BACKTRACE) */
        abort();
      }

    strcpy (cmd_name, cmd_table[i].name);
    while (alias_len > 1) {
        cmd_name[alias_len] = '\0';                 /* Possible short form command name */
        --alias_len;
        if (getenv (cmd_name))                      /* Externally defined command alias? */
            unsetenv (cmd_name);                    /* Remove it to protect against possibly malicious aliases */
        }
    FREE (cmd_name);
    }
stop_cpu = 0;
sim_interval = 0;
sim_time = sim_rtime = 0;
noqueue_time = 0;
sim_clock_queue = QUEUE_LIST_END;
sim_is_running = 0;
sim_log = NULL;
if (sim_emax <= 0)
    sim_emax = 1;
sim_timer_init ();

if ((stat = sim_ttinit ()) != SCPE_OK) {
    (void)fprintf (stderr, "Fatal terminal initialization error\n%s\n",
                   sim_error_text (stat));
    FREE(argv); //-V726
    return 1;
    }
if ((sim_eval = (t_value *) calloc (sim_emax, sizeof (t_value))) == NULL) {
    (void)fprintf (stderr, "Unable to allocate examine buffer\n");
    FREE(argv); //-V726
    return 1;
    };
if ((stat = reset_all_p (0)) != SCPE_OK) {
    (void)fprintf (stderr, "Fatal simulator initialization error\n%s\n",
                   sim_error_text (stat));
    FREE(argv); //-V726
    return 1;
    }
if ((stat = sim_brk_init ()) != SCPE_OK) {
    (void)fprintf (stderr, "Fatal breakpoint table initialization error\n%s\n",
                   sim_error_text (stat));
    FREE(argv); //-V726
    return 1;
    }
if (!sim_quiet) {
    (void)printf ("\n");
    show_version (stdout, NULL, NULL, 0, NULL);
    }

cptr = getenv("HOME");
if (cptr == NULL) {
    cptr = getenv("HOMEPATH");
    cptr2 = getenv("HOMEDRIVE");
    }
else
    cptr2 = NULL;
(void)cptr2;
if ( (*cbuf) && (strcmp(cbuf, "")) )                    /* cmd file arg? */
    stat = do_cmd (0, cbuf);                            /* proc cmd file */
else if (*argv[0]) {                                    /* sim name arg? */
    char *np;                                           /* "path.ini" */
    nbuf[0] = '"';                                      /* starting " */
    stat = do_cmd (-1, nbuf) & ~SCPE_NOMESSAGE;         /* proc default cmd file */
    if (stat == SCPE_OPENERR) {                         /* didn't exist/can't open? */
        np = strrchr (nbuf, '/');                       /* strip path and try again in cwd */
        if (np == NULL)
            np = strrchr (nbuf, '\\');                  /* windows path separator */
        if (np != NULL) {
            *np = '"';
            stat = do_cmd (-1, np) & ~SCPE_NOMESSAGE;   /* proc default cmd file */
            }
        }
    }

stat = process_stdin_commands (SCPE_BARE_STATUS(stat), argv);

if (sim_vm_exit != NULL)                                /* call once only */
    (*sim_vm_exit)();

detach_all (0, TRUE);                                   /* close files */
sim_set_deboff (0, NULL);                               /* close debug */
sim_set_logoff (0, NULL);                               /* close log */
sim_set_notelnet (0, NULL);                             /* close Telnet */
sim_ttclose ();                                         /* close console */
sim_cleanup_sock ();                                    /* cleanup sockets */
fclose (stdnul);                                        /* close bit bucket file handle */
FREE (targv);                                           /* release any argv copy that was made */
FREE (sim_prompt);
FREE (sim_eval);
FREE (sim_internal_devices);
FREE (sim_brk_tab);
FREE (sim_staba.comp);
FREE (sim_staba.mask);
FREE (sim_stabr.comp);
FREE (sim_stabr.mask);
return 0;
}
#endif

t_stat process_stdin_commands (t_stat stat, char *argv[])
{
char cbuf[4*CBUFSIZE], gbuf[CBUFSIZE];
CONST char *cptr;
t_stat stat_nomessage;
CTAB *cmdp = NULL;

stat = SCPE_BARE_STATUS(stat);                          /* remove possible flag */
while (stat != SCPE_EXIT) {                             /* in case exit */
    if ((cptr = sim_brk_getact (cbuf, sizeof(cbuf))))   /* pending action? */
        (void)printf ("%s%s\n", sim_prompt, cptr);      /* echo */
    else if (sim_vm_read != NULL) {                     /* sim routine? */
        (void)printf ("%s", sim_prompt);                /* prompt */
        cptr = (*sim_vm_read) (cbuf, sizeof(cbuf), stdin);
        }
    else cptr = read_line_p (sim_prompt, cbuf, sizeof(cbuf), stdin);/* read with prompt*/
    if (cptr == NULL) {                                 /* EOF? */
        if (sim_ttisatty()) continue;                   /* ignore tty EOF */
        else break;                                     /* otherwise exit */
        }
    if (*cptr == 0)                                     /* ignore blank */
        continue;
    sim_sub_args (cbuf, sizeof(cbuf), argv);
    if (sim_log)                                        /* log cmd */
        (void)fprintf (sim_log, "%s%s\n", sim_prompt, cptr);
    if (sim_deb && (sim_deb != sim_log) && (sim_deb != stdout))
        (void)fprintf (sim_deb, "%s%s\n", sim_prompt, cptr);
    cptr = get_glyph_cmd (cptr, gbuf);                  /* get command glyph */
    sim_switches = 0;                                   /* init switches */
    if ((cmdp = find_cmd (gbuf)))                       /* lookup command */
        stat = cmdp->action (cmdp->arg, cptr);          /* if found, exec */
    else
        stat = SCPE_UNK;
    stat_nomessage = stat & SCPE_NOMESSAGE;             /* extract possible message suppression flag */
    stat_nomessage = stat_nomessage || (!sim_show_message);/* Apply global suppression */
    stat = SCPE_BARE_STATUS(stat);                      /* remove possible flag */
    sim_last_cmd_stat = stat;                           /* save command error status */
    if (!stat_nomessage) {                              /* displaying message status? */
        if (cmdp && (cmdp->message))                    /* special message handler? */
            cmdp->message (NULL, stat);                 /* let it deal with display */
        else
            if (stat >= SCPE_BASE)                      /* error? */
                sim_printf ("%s\n", sim_error_text (stat));
        }
    if (sim_vm_post != NULL)
        (*sim_vm_post) (TRUE);
    }                                                   /* end while */
return stat;
}

/* Set prompt routine */

t_stat set_prompt (int32 flag, CONST char *cptr)
{
char gbuf[CBUFSIZE], *gptr;

if ((NULL == cptr) || (*cptr == '\0'))
    return SCPE_ARG;

cptr = get_glyph_nc (cptr, gbuf, '"');                  /* get quote delimited token */
if (gbuf[0] == '\0') {                                  /* Token started with quote */
    gbuf[sizeof (gbuf)-1] = '\0';
    strncpy (gbuf, cptr, sizeof (gbuf)-1);
    gptr = strchr (gbuf, '"');
    if (NULL != gptr)
        *gptr = '\0';
    }
sim_prompt = (char *)realloc (sim_prompt, strlen (gbuf) + 2);   /* nul terminator and trailing blank */
if (!sim_prompt)
  {
    (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                   __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
(void)sprintf (sim_prompt, "%s ", gbuf);
return SCPE_OK;
}

/* Find command routine */

CTAB *find_cmd (const char *gbuf)
{
CTAB *cmdp = NULL;

if (sim_vm_cmd)                                         /* try ext commands */
    cmdp = find_ctab (sim_vm_cmd, gbuf);
if (cmdp == NULL)                                       /* try regular cmds */
    cmdp = find_ctab (cmd_table, gbuf);
return cmdp;
}

/* Exit command */

t_stat exit_cmd (int32 flag, CONST char *cptr)
{
return SCPE_EXIT;
}

/* Help command */

/* Used when sorting a list of command names */
static int _cmd_name_compare (const void *pa, const void *pb)
{
CTAB * const *a = (CTAB * const *)pa;
CTAB * const *b = (CTAB * const *)pb;

return strcmp((*a)->name, (*b)->name);
}

void fprint_help (FILE *st)
{
CTAB *cmdp;
CTAB **hlp_cmdp = NULL;
size_t cmd_cnt = 0;
size_t cmd_size = 0;
size_t max_cmdname_size = 0;
size_t i, line_offset;

for (cmdp = sim_vm_cmd; cmdp && (cmdp->name != NULL); cmdp++) {
    if (cmdp->help) {
        if (cmd_cnt >= cmd_size) {
            cmd_size += 20;
            hlp_cmdp = (CTAB **)realloc (hlp_cmdp, sizeof(*hlp_cmdp)*cmd_size);
            if (!hlp_cmdp)
              {
                (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                               __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
                (void)raise(SIGUSR2);
                /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
                abort();
              }
            }
        hlp_cmdp[cmd_cnt] = cmdp;
        ++cmd_cnt;
        if (strlen(cmdp->name) > max_cmdname_size)
            max_cmdname_size = strlen(cmdp->name);
        }
    }
for (cmdp = cmd_table; cmdp && (cmdp->name != NULL); cmdp++) {
    if (cmdp->help && (NULL == sim_vm_cmd || NULL == find_ctab (sim_vm_cmd, cmdp->name))) {
        if (cmd_cnt >= cmd_size) {
            cmd_size += 20;
            hlp_cmdp = (CTAB **)realloc (hlp_cmdp, sizeof(*hlp_cmdp)*cmd_size);
            if (!hlp_cmdp)
              {
                (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                               __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
                (void)raise(SIGUSR2);
                /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
                abort();
              }
            }
        hlp_cmdp[cmd_cnt] = cmdp;
        ++cmd_cnt;
        if (strlen (cmdp->name) > max_cmdname_size)
            max_cmdname_size = strlen(cmdp->name);
        }
    }
(void)fprintf (st, "HELP is available for the following commands:\n\n    ");
if (hlp_cmdp)
  qsort (hlp_cmdp, cmd_cnt, sizeof(*hlp_cmdp), _cmd_name_compare);
line_offset = 4;
for ( i = 0 ; i < cmd_cnt ; ++i ) {
    fputs (hlp_cmdp[i]->name, st);
    line_offset += 5 + max_cmdname_size;
    if (line_offset + max_cmdname_size > 79) {
        line_offset = 4;
        (void)fprintf (st, "\n    ");
        }
    else
        (void)fprintf (st, "%*s", (int)(max_cmdname_size + 5 - strlen (hlp_cmdp[i]->name)), "");
    }
FREE (hlp_cmdp);
(void)fprintf (st, "\n");
return;
}

static void fprint_header (FILE *st, t_bool *pdone, char *context)
{
if (!*pdone)
    (void)fprintf (st, "%s", context);
*pdone = TRUE;
}

void fprint_reg_help_ex (FILE *st, DEVICE *dptr, t_bool silent)
{
REG *rptr, *trptr;
t_bool found = FALSE;
t_bool all_unique = TRUE;
size_t max_namelen = 0;
DEVICE *tdptr;
CONST char *tptr;
char *namebuf;
char rangebuf[32];

if (dptr->registers)
    for (rptr = dptr->registers; rptr->name != NULL; rptr++) {
        if (rptr->flags & REG_HIDDEN)
            continue;
        if (rptr->depth > 1)
            (void)sprintf (rangebuf, "[%d:%d]", 0, rptr->depth-1);
        else
            strcpy (rangebuf, "");
        if (max_namelen < (strlen(rptr->name) + strlen (rangebuf)))
            max_namelen = strlen(rptr->name) + strlen (rangebuf);
        found = TRUE;
        trptr = find_reg_glob (rptr->name, &tptr, &tdptr);
        if ((trptr == NULL) || (tdptr != dptr))
            all_unique = FALSE;
        }
if (!found) {
    if (!silent)
        (void)fprintf (st, "No register HELP available for the %s device\n",
                       dptr->name);
    }
else {
    namebuf = (char *)calloc (max_namelen + 1, sizeof (*namebuf));
    if (!namebuf)
      {
        (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                       __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
        abort();
      }
    (void)fprintf (st, "\nThe %s device implements these registers:\n\n",
                   dptr->name);
    for (rptr = dptr->registers; rptr->name != NULL; rptr++) {
        if (rptr->flags & REG_HIDDEN)
            continue;
        if (rptr->depth <= 1)
            (void)sprintf (namebuf, "%*s",
                           -((int)max_namelen),
                           rptr->name);
        else {
            (void)sprintf (rangebuf, "[%d:%d]",
                           0,
                           rptr->depth-1);
            (void)sprintf (namebuf, "%s%*s",
                           rptr->name,
                           (int)(strlen(rptr->name))-((int)max_namelen),
                           rangebuf);
            }
        if (all_unique) {
            (void)fprintf (st, "  %s %4d  %s\n",
                           namebuf,
                           rptr->width,
                           rptr->desc ? rptr->desc : "");
            continue;
            }
        trptr = find_reg_glob (rptr->name, &tptr, &tdptr);
        if ((trptr == NULL) || (tdptr != dptr))
            (void)fprintf (st, "  %s %s %4d  %s\n",
                           dptr->name,
                           namebuf,
                           rptr->width,
                           rptr->desc ? rptr->desc : "");
        else
            (void)fprintf (st, "  %*s %s %4d  %s\n",
                           (int)strlen(dptr->name), "",
                           namebuf,
                           rptr->width,
                           rptr->desc ? rptr->desc : "");
        }
    FREE (namebuf);
    }
}

void fprint_reg_help (FILE *st, DEVICE *dptr)
{
fprint_reg_help_ex (st, dptr, TRUE);
}

void fprint_attach_help_ex (FILE *st, DEVICE *dptr, t_bool silent)
{
if (dptr->attach_help) {
    (void)fprintf (st, "\n%s device ATTACH commands:\n\n", dptr->name);
    dptr->attach_help (st, dptr, NULL, 0, NULL);
    return;
    }
if (DEV_TYPE(dptr) == DEV_DISK) {
    (void)fprintf (st, "\n%s device ATTACH commands:\n\n", dptr->name);
    sim_disk_attach_help (st, dptr, NULL, 0, NULL);
    return;
    }
if (DEV_TYPE(dptr) == DEV_TAPE) {
    (void)fprintf (st, "\n%s device ATTACH commands:\n\n", dptr->name);
    sim_tape_attach_help (st, dptr, NULL, 0, NULL);
    return;
    }
if (!silent) {
    (void)fprintf (st, "No ATTACH help is available for the %s device\n", dptr->name);
    if (dptr->help)
        dptr->help (st, dptr, NULL, 0, NULL);
    }
}

void fprint_set_help_ex (FILE *st, DEVICE *dptr, t_bool silent)
{
MTAB *mptr;
DEBTAB *dep;
t_bool found = FALSE;
char buf[CBUFSIZE], header[CBUFSIZE];
uint32 enabled_units = dptr->numunits;
uint32 unit;

(void)sprintf (header, "\n%s device SET commands:\n\n", dptr->name);
for (unit=0; unit < dptr->numunits; unit++)
    if (dptr->units[unit].flags & UNIT_DIS)
        --enabled_units;
if (dptr->modifiers) {
    for (mptr = dptr->modifiers; mptr->mask != 0; mptr++) {
        if (!MODMASK(mptr,MTAB_VDV) && MODMASK(mptr,MTAB_VUN) && (dptr->numunits != 1))
            continue;                                       /* skip unit only extended modifiers */
        if ((enabled_units != 1) && !(mptr->mask & MTAB_XTD))
            continue;                                       /* skip unit only simple modifiers */
        if (mptr->mstring) {
            fprint_header (st, &found, header);
            (void)sprintf (buf, "SET %s %s%s", sim_dname (dptr),
                           mptr->mstring,
                           (strchr(mptr->mstring, '=')) \
                               ? ""       : (MODMASK(mptr,MTAB_VALR) \
                               ? "=val"   : (MODMASK(mptr,MTAB_VALO) \
                               ? "{=val}" : "")));
            if ((strlen (buf) < 30) || (NULL == mptr->help))
                (void)fprintf (st, "%-30s\t%s\n", buf, mptr->help ? mptr->help : "");
            else
                (void)fprintf (st, "%s\n%-30s\t%s\n", buf, "", mptr->help);
            }
        }
    }
if (dptr->flags & DEV_DISABLE) {
    fprint_header (st, &found, header);
    (void)sprintf (buf, "SET %s ENABLE", sim_dname (dptr));
    (void)fprintf (st,  "%-30s\tEnables device %s\n", buf, sim_dname (dptr));
    (void)sprintf (buf, "SET %s DISABLE", sim_dname (dptr));
    (void)fprintf (st,  "%-30s\tDisables device %s\n", buf, sim_dname (dptr));
    }
if (dptr->flags & DEV_DEBUG) {
    fprint_header (st, &found, header);
    (void)sprintf (buf, "SET %s DEBUG", sim_dname (dptr));
    (void)fprintf (st,  "%-30s\tEnables debugging for device %s\n", buf, sim_dname (dptr));
    (void)sprintf (buf, "SET %s NODEBUG", sim_dname (dptr));
    (void)fprintf (st,  "%-30s\tDisables debugging for device %s\n", buf, sim_dname (dptr));
    if (dptr->debflags) {
        t_bool desc_available = FALSE;
        strcpy (buf, "");
        (void)fprintf (st, "SET %s DEBUG=", sim_dname (dptr));
        for (dep = dptr->debflags; dep->name != NULL; dep++) {
            (void)fprintf (st, "%s%s", ((dep == dptr->debflags) ? "" : ";"), dep->name);
            desc_available |= ((dep->desc != NULL) && (dep->desc[0] != '\0'));
            }
        (void)fprintf (st, "\n");
        (void)fprintf (st,  "%-30s\tEnables specific debugging for device %s\n", buf, sim_dname (dptr));
        (void)fprintf (st, "SET %s NODEBUG=", sim_dname (dptr));
        for (dep = dptr->debflags; dep->name != NULL; dep++)
            (void)fprintf (st, "%s%s", ((dep == dptr->debflags) ? "" : ";"), dep->name);
        (void)fprintf (st, "\n");
        (void)fprintf (st,  "%-30s\tDisables specific debugging for device %s\n", buf, sim_dname (dptr));
        if (desc_available) {
            (void)fprintf (st, "\n*%s device DEBUG settings:\n", sim_dname (dptr));
            for (dep = dptr->debflags; dep->name != NULL; dep++)
                (void)fprintf (st, "%4s%-12s%s\n", "", dep->name, dep->desc ? dep->desc : "");
            }
        }
    }
if ((dptr->modifiers) && (dptr->units) && (enabled_units != 1)) {
    if (dptr->units->flags & UNIT_DISABLE) {
        fprint_header (st, &found, header);
        (void)sprintf (buf, "SET %sn ENABLE", sim_dname (dptr));
        (void)fprintf (st,  "%-30s\tEnables unit %sn\n", buf, sim_dname (dptr));
        (void)sprintf (buf, "SET %sn DISABLE", sim_dname (dptr));
        (void)fprintf (st,  "%-30s\tDisables unit %sn\n", buf, sim_dname (dptr));
        }
    for (mptr = dptr->modifiers; mptr->mask != 0; mptr++) {
        if ((!MODMASK(mptr,MTAB_VUN)) && MODMASK(mptr,MTAB_XTD))
            continue;                                           /* skip device only modifiers */
        if ((NULL == mptr->valid) && MODMASK(mptr,MTAB_XTD))
            continue;                                           /* skip show only modifiers */
        if (mptr->mstring) {
            fprint_header (st, &found, header);
            (void)sprintf (buf, "SET %s%s %s%s", sim_dname (dptr),
                           (dptr->numunits > 1) ? "n" : "0", mptr->mstring,
                           (strchr(mptr->mstring, '=')) \
                               ? ""       : (MODMASK(mptr,MTAB_VALR) \
                               ? "=val"   : (MODMASK(mptr,MTAB_VALO) \
                               ? "{=val}" : "")));
            (void)fprintf (st, "%-30s\t%s\n", buf,
                           (strchr(mptr->mstring, '=')) \
                               ? "" : (mptr->help ? mptr->help : ""));
            }
        }
    }
if (!found && !silent)
    (void)fprintf (st, "No SET help is available for the %s device\n", dptr->name);
}

void fprint_set_help (FILE *st, DEVICE *dptr)
{
  fprint_set_help_ex (st, dptr, TRUE);
}

void fprint_show_help_ex (FILE *st, DEVICE *dptr, t_bool silent)
{
MTAB *mptr;
t_bool found = FALSE;
char buf[CBUFSIZE], header[CBUFSIZE];
uint32 enabled_units = dptr->numunits;
uint32 unit;

(void)sprintf (header, "\n%s device SHOW commands:\n\n", dptr->name);
for (unit=0; unit < dptr->numunits; unit++)
    if (dptr->units[unit].flags & UNIT_DIS)
        --enabled_units;
if (dptr->modifiers) {
    for (mptr = dptr->modifiers; mptr->mask != 0; mptr++) {
        if (!MODMASK(mptr,MTAB_VDV) && MODMASK(mptr,MTAB_VUN) && (dptr->numunits != 1))
            continue;                                       /* skip unit only extended modifiers */
        if ((enabled_units != 1) && !(mptr->mask & MTAB_XTD))
            continue;                                       /* skip unit only simple modifiers */
        if ((!mptr->disp) || (!mptr->pstring) || !(*mptr->pstring))
            continue;
        fprint_header (st, &found, header);
        (void)sprintf (buf, "SHOW %s %s%s", sim_dname (dptr),
                       mptr->pstring, MODMASK(mptr,MTAB_SHP) ? "{=arg}" : "");
        (void)fprintf (st, "%-30s\t%s\n", buf, mptr->help ? mptr->help : "");
        }
    }
if (dptr->flags & DEV_DEBUG) {
    fprint_header (st, &found, header);
    (void)sprintf (buf, "SHOW %s DEBUG", sim_dname (dptr));
    (void)fprintf (st, "%-30s\tDisplays debugging status for device %s\n", buf, sim_dname (dptr));
    }
if ((dptr->modifiers) && (dptr->units) && (enabled_units != 1)) {
    for (mptr = dptr->modifiers; mptr->mask != 0; mptr++) {
        if ((!MODMASK(mptr,MTAB_VUN)) && MODMASK(mptr,MTAB_XTD))
            continue;                                           /* skip device only modifiers */
        if ((!mptr->disp) || (!mptr->pstring))
            continue;
        fprint_header (st, &found, header);
        (void)sprintf (buf, "SHOW %s%s %s%s", sim_dname (dptr),
                       (dptr->numunits > 1) ? "n" : "0", mptr->pstring,
                       MODMASK(mptr,MTAB_SHP) ? "=arg" : "");
        (void)fprintf (st, "%-30s\t%s\n", buf, mptr->help ? mptr->help : "");
        }
    }
if (!found && !silent)
    (void)fprintf (st, "No SHOW help is available for the %s device\n", dptr->name);
}

void fprint_show_help (FILE *st, DEVICE *dptr)
    {
    fprint_show_help_ex (st, dptr, TRUE);
    }

void fprint_brk_help_ex (FILE *st, DEVICE *dptr, t_bool silent)
{
BRKTYPTAB *brkt = dptr->brk_types;
char gbuf[CBUFSIZE];

if (sim_brk_types == 0) {
    if ((dptr != sim_dflt_dev) && (!silent)) {
        (void)fprintf (st, "Breakpoints are not supported in the %s simulator\n", sim_name);
        if (dptr->help)
            dptr->help (st, dptr, NULL, 0, NULL);
        }
    return;
    }
if (brkt == NULL) {
    int i;

    if (dptr == sim_dflt_dev) {
        if (sim_brk_types & ~sim_brk_dflt) {
            (void)fprintf (st, "%s supports the following breakpoint types:\n", sim_dname (dptr));
            for (i=0; i<26; i++) {
                if (sim_brk_types & (1<<i))
                    (void)fprintf (st, "  -%c\n", 'A'+i);
                }
            }
        (void)fprintf (st, "The default breakpoint type is: %s\n", put_switches (gbuf, sizeof(gbuf), sim_brk_dflt));
        }
    return;
    }
(void)fprintf (st, "%s supports the following breakpoint types:\n", sim_dname (dptr));
while (brkt->btyp) {
    (void)fprintf (st, "  %s     %s\n", put_switches (gbuf, sizeof(gbuf), brkt->btyp), brkt->desc);
    ++brkt;
    }
(void)fprintf (st, "The default breakpoint type is: %s\n", put_switches (gbuf, sizeof(gbuf), sim_brk_dflt));
}

t_stat help_dev_help (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, const char *cptr)
{
char gbuf[CBUFSIZE];
CTAB *cmdp;

if (*cptr) {
    (void)get_glyph (cptr, gbuf, 0);
    if ((cmdp = find_cmd (gbuf))) {
        if (cmdp->action == &exdep_cmd) {
            if (dptr->help) /* Shouldn't this pass cptr so the device knows which command invoked? */
                return dptr->help (st, dptr, uptr, flag, cptr);
            else
                (void)fprintf (st, "No HELP available for the %s %s command\n", cmdp->name, sim_dname(dptr));
            return SCPE_OK;
            }
        if (cmdp->action == &set_cmd) {
            fprint_set_help_ex (st, dptr, FALSE);
            return SCPE_OK;
            }
        if (cmdp->action == &show_cmd) {
            fprint_show_help_ex (st, dptr, FALSE);
            return SCPE_OK;
            }
        if (cmdp->action == &attach_cmd) {
            fprint_attach_help_ex (st, dptr, FALSE);
            return SCPE_OK;
            }
        if (cmdp->action == &brk_cmd) {
            fprint_brk_help_ex (st, dptr, FALSE);
            return SCPE_OK;
            }
        if (dptr->help)
            return dptr->help (st, dptr, uptr, flag, cptr);
        (void)fprintf (st, "No %s HELP is available for the %s device\n", cmdp->name, dptr->name);
        return SCPE_OK;
        }
    if (MATCH_CMD (gbuf, "REGISTERS") == 0) {
        fprint_reg_help_ex (st, dptr, FALSE);
        return SCPE_OK;
        }
    if (dptr->help)
        return dptr->help (st, dptr, uptr, flag, cptr);
    (void)fprintf (st, "No %s HELP is available for the %s device\n", gbuf, dptr->name);
    return SCPE_OK;
    }
if (dptr->help) {
    return dptr->help (st, dptr, uptr, flag, cptr);
    }
if (dptr->description)
    (void)fprintf (st, "%s %s HELP\n", dptr->description (dptr), dptr->name);
else
    (void)fprintf (st, "%s HELP\n", dptr->name);
fprint_set_help_ex    (st, dptr, TRUE);
fprint_show_help_ex   (st, dptr, TRUE);
fprint_attach_help_ex (st, dptr, TRUE);
fprint_reg_help_ex    (st, dptr, TRUE);
fprint_brk_help_ex    (st, dptr, TRUE);
return SCPE_OK;
}

t_stat help_cmd_output (int32 flag, const char *help, const char *help_base)
{
switch (help[0]) {
    case '*':
        scp_help (stdout, NULL, NULL, flag, help_base ? help_base : simh_help, help+1);
        if (sim_log)
            scp_help (sim_log, NULL, NULL, flag | SCP_HELP_FLAT, help_base ? help_base : simh_help, help+1);
        break;
    default:
        fputs (help, stdout);
        if (sim_log)
            fputs (help, sim_log);
        break;
    }
return SCPE_OK;
}

t_stat help_cmd (int32 flag, CONST char *cptr)
{
char gbuf[CBUFSIZE];
CTAB *cmdp;

GET_SWITCHES (cptr);
if (sim_switches & SWMASK ('F'))
    flag = flag | SCP_HELP_FLAT;
if (*cptr) {
    cptr = get_glyph (cptr, gbuf, 0);
    if ((cmdp = find_cmd (gbuf))) {
        if (*cptr) {
            if ((cmdp->action == &set_cmd) || (cmdp->action == &show_cmd)) {
                DEVICE *dptr;
                UNIT *uptr;
                t_stat r;
                cptr = get_glyph (cptr, gbuf, 0);
                dptr = find_unit (gbuf, &uptr);
                if (dptr == NULL)
                    dptr = find_dev (gbuf);
                if (dptr != NULL) {
                    r = help_dev_help (stdout, dptr, uptr, flag, (cmdp->action == &set_cmd) ? "SET" : "SHOW");
                    if (sim_log)
                        help_dev_help (sim_log, dptr, uptr, flag | SCP_HELP_FLAT, (cmdp->action == &set_cmd) ? "SET" : "SHOW");
                    return r;
                    }
                if (cmdp->action == &set_cmd) { /* HELP SET xxx (not device or unit) */
                    /*LINTED E_EQUALITY_NOT_ASSIGNMENT*/
                    if ((cmdp = find_ctab (set_glob_tab, gbuf)) &&
                         (cmdp->help))
                        return help_cmd_output (flag, cmdp->help, cmdp->help_base);
                    }
                else { /* HELP SHOW xxx (not device or unit) */
                    SHTAB *shptr = find_shtab (show_glob_tab, gbuf);
                    if ((shptr == NULL) || (shptr->help == NULL) || (*shptr->help == '\0'))
                        return SCPE_ARG;
                    return help_cmd_output (flag, shptr->help, NULL);
                    }
                return SCPE_ARG;
                }
            else
                return SCPE_2MARG;
            }
        if (cmdp->help) {
            if (strcmp (cmdp->name, "HELP") == 0) {
#if 0
                DEVICE *dptr;
                int i;
                /*LINTED E_END_OF_LOOP_CODE_NOT_REACHED*/
                for (i = 0; (dptr = sim_devices[i]) != NULL; i++) {
                    break;
                    /*NOTREACHED*/ /* unreachable */ /* XXX(jhj): NOTREACHED below */
                    if (dptr->help)
                        sim_printf ("H{ELP} %-17s Display help for device %s\n",
                                    dptr->name, dptr->name);
                    if (dptr->attach_help ||
                        (DEV_TYPE(dptr) == DEV_MUX) ||
                        (DEV_TYPE(dptr) == DEV_DISK) ||
                        (DEV_TYPE(dptr) == DEV_TAPE)) {
                        sim_printf ("H{ELP} %s ATTACH\t Display help for device %s ATTACH command\n",
                                    dptr->name, dptr->name);
                        }
                    if (dptr->registers) {
                        if (dptr->registers->name != NULL)
                            sim_printf ("H{ELP} %s REGISTERS\t Display help for device %s register variables\n",
                                        dptr->name, dptr->name);
                        }
                    if (dptr->modifiers) {
                        MTAB *mptr;
                        for (mptr = dptr->modifiers; mptr->pstring != NULL; mptr++) {
                            if (mptr->help) {
                                sim_printf ("H{ELP} %s SET\t\t Display help for device %s SET commands\n",
                                            dptr->name, dptr->name);
                                break;
                                }
                            }
                        }
                    }
#endif /* 0 */
                }
            else {
                if (((cmdp->action == &exdep_cmd) || (0 == strcmp(cmdp->name, "BOOT"))) &&
                    sim_dflt_dev && sim_dflt_dev->help) {
                        sim_dflt_dev->help (stdout, sim_dflt_dev, sim_dflt_dev->units, 0, cmdp->name);
                        if (sim_log)
                            sim_dflt_dev->help (sim_log, sim_dflt_dev, sim_dflt_dev->units, 0, cmdp->name);
                    }
                }
            help_cmd_output (flag, cmdp->help, cmdp->help_base);
            }
        else { /* no help so it is likely a command alias */
            CTAB *cmdpa;
            for (cmdpa=cmd_table; cmdpa->name != NULL; cmdpa++)
                if ((cmdpa->action == cmdp->action) && (cmdpa->help)) {
                    sim_printf ("%s is an alias for the %s command:\n%s",
                                cmdp->name, cmdpa->name, cmdpa->help);
                    break;
                    }
            if (cmdpa->name == NULL)                /* not found? */
                sim_printf ("No help available for the %s command\n", cmdp->name);
            }
        }
    else {
        DEVICE *dptr;
        UNIT *uptr;
        t_stat r;
        dptr = find_unit (gbuf, &uptr);
        if (dptr == NULL) {
            dptr = find_dev (gbuf);
            if (dptr == NULL)
                return SCPE_ARG;
            if (dptr->flags & DEV_DIS)
                sim_printf ("Device %s is currently disabled\n", dptr->name);
            }
        r = help_dev_help (stdout, dptr, uptr, flag, cptr);
        if (sim_log)
            help_dev_help (sim_log, dptr, uptr, flag | SCP_HELP_FLAT, cptr);
        return r;
        }
    }
else {
    fprint_help (stdout);
    if (sim_log)
        fprint_help (sim_log);
    }
return SCPE_OK;
}

/* Spawn command */

t_stat spawn_cmd (int32 flag, CONST char *cptr)
{
t_stat status;
if ((cptr == NULL) || (strlen (cptr) == 0))
    cptr = getenv("SHELL");
if ((cptr == NULL) || (strlen (cptr) == 0))
    cptr = getenv("ComSpec");
(void)fflush(stdout);                                   /* flush stdout */
if (sim_log)                                            /* flush log if enabled */
    (void)fflush (sim_log);
if (sim_deb)                                            /* flush debug if enabled */
    (void)fflush (sim_deb);
status = system (cptr);

return status;
}

/* Echo command */

t_stat echo_cmd (int32 flag, CONST char *cptr)
{
sim_printf ("%s\n", cptr);
return SCPE_OK;
}

/*
 * DO command
 *
 * Note that SCPE_STEP ("Step expired") is considered a note and
 * not an error; it does not abort command execution when using -E.
 *
 * Inputs:
 *      flag    =   caller and nesting level indicator
 *      fcptr   =   filename and optional arguments, space-separated
 * Outputs:
 *      status  =   error status
 *
 * The "flag" input value indicates the source of the call, as follows:
 *
 *      -1      =   initialization file (no error if not found)
 *       0      =   command line file
 *       1      =   "DO" command
 *      >1      =   nested "DO" command
 */

t_stat do_cmd (int32 flag, CONST char *fcptr)
{
return do_cmd_label (flag, fcptr, NULL);
}

static char *do_position(void)
{
static char cbuf[4*CBUFSIZE];

(void)snprintf (cbuf, sizeof (cbuf), "%s%s%s-%d", sim_do_filename[sim_do_depth],
                sim_do_label[sim_do_depth] ? "::" : "",
                sim_do_label[sim_do_depth] ? sim_do_label[sim_do_depth] : "",
                sim_goto_line[sim_do_depth]);
return cbuf;
}

t_stat do_cmd_label (int32 flag, CONST char *fcptr, CONST char *label)
{
char cbuf[4*CBUFSIZE], gbuf[CBUFSIZE], abuf[4*CBUFSIZE], quote, *c, *do_arg[11];
CONST char *cptr;
FILE *fpin;
CTAB *cmdp = NULL;
int32 echo, nargs, errabort, i;
int32 saved_sim_do_echo = sim_do_echo,
      saved_sim_show_message = sim_show_message,
      saved_sim_on_inherit = sim_on_inherit,
      saved_sim_quiet = sim_quiet;
t_bool staying;
t_stat stat, stat_nomessage;

stat = SCPE_OK;
staying = TRUE;
if (flag > 0)                                           /* need switches? */
    GET_SWITCHES (fcptr);                               /* get switches */
echo = (sim_switches & SWMASK ('V')) || sim_do_echo;    /* -v means echo */
sim_quiet = (sim_switches & SWMASK ('Q')) || sim_quiet; /* -q means quiet */
sim_on_inherit =(sim_switches & SWMASK ('O')) || sim_on_inherit; /* -o means inherit ON condition actions */
errabort = sim_switches & SWMASK ('E');                 /* -e means abort on error */

abuf[sizeof(abuf)-1] = '\0';
strncpy (abuf, fcptr, sizeof(abuf)-1);
c = abuf;
do_arg[10] = NULL;                                      /* make sure the argument list always ends with a NULL */
for (nargs = 0; nargs < 10; ) {                         /* extract arguments */
    while (sim_isspace (*c))                            /* skip blanks */
        c++;
    if (*c == 0)                                        /* all done? */
        do_arg [nargs++] = NULL;                        /* null argument */
    else {
        if (*c == '\'' || *c == '"')                    /* quoted string? */
            quote = *c++;
        else quote = 0;
        do_arg[nargs++] = c;                            /* save start */
        while (*c && (quote ? (*c != quote) : !sim_isspace (*c)))
            c++;
        if (*c)                                         /* term at quote/spc */
            *c++ = 0;
        }
    }                                                   /* end for */

if (do_arg [0] == NULL)                                 /* need at least 1 */
    return SCPE_2FARG;
if ((fpin = fopen (do_arg[0], "r")) == NULL) {          /* file failed to open? */
    strcat (strcpy (cbuf, do_arg[0]), ".ini");          /* try again with .ini extension */
    if ((fpin = fopen (cbuf, "r")) == NULL) {           /* failed a second time? */
        if (flag == 0)                                  /* cmd line file? */
             (void)fprintf (stderr, "Can't open file %s\n", do_arg[0]);
        return SCPE_OPENERR;                            /* return failure */
        }
    }
if (flag >= 0) {                                        /* Only bump nesting from command or nested */
    ++sim_do_depth;
    if (sim_on_inherit) {                               /* inherit ON condition actions? */
        sim_on_check[sim_do_depth] = sim_on_check[sim_do_depth-1]; /* inherit On mode */
        for (i=0; i<SCPE_MAX_ERR; i++) {                /* replicate any on commands */
            if (sim_on_actions[sim_do_depth-1][i]) {
                sim_on_actions[sim_do_depth][i] = (char *)malloc(1+strlen(sim_on_actions[sim_do_depth-1][i]));
                if (NULL == sim_on_actions[sim_do_depth][i]) {
                    while (--i >= 0) {
                        FREE(sim_on_actions[sim_do_depth][i]);
                        sim_on_actions[sim_do_depth][i] = NULL;
                        }
                    sim_on_check[sim_do_depth] = 0;
                    sim_brk_clract ();                  /* defang breakpoint actions */
                    --sim_do_depth;                     /* unwind nesting */
                    fclose(fpin);
                    return SCPE_MEM;
                    }
                strcpy(sim_on_actions[sim_do_depth][i], sim_on_actions[sim_do_depth-1][i]);
                }
            }
        }
    }

strcpy( sim_do_filename[sim_do_depth], do_arg[0]);      /* stash away do file name for possible use by 'call' command */
sim_do_label[sim_do_depth] = label;                     /* stash away do label for possible use in messages */
sim_goto_line[sim_do_depth] = 0;
if (label) {
    sim_gotofile = fpin;
    sim_do_echo = echo;
    stat = goto_cmd (0, label);
    if (stat != SCPE_OK) {
        strcpy(cbuf, "RETURN SCPE_ARG");
        cptr = get_glyph (cbuf, gbuf, 0);               /* get command glyph */
        cmdp = find_cmd (gbuf);                         /* return the errorStage things to the stat will be returned */
        goto Cleanup_Return;
        }
    }
if (errabort)                                           /* -e flag? */
    set_on (1, NULL);                                   /* equivalent to ON ERROR RETURN */

do {
    sim_do_ocptr[sim_do_depth] = cptr = sim_brk_getact (cbuf, sizeof(cbuf)); /* get bkpt action */
    if (!sim_do_ocptr[sim_do_depth]) {                  /* no pending action? */
        sim_do_ocptr[sim_do_depth] = cptr = read_line (cbuf, sizeof(cbuf), fpin);/* get cmd line */
        sim_goto_line[sim_do_depth] += 1;
        }
    sim_sub_args (cbuf, sizeof(cbuf), do_arg);          /* substitute args */
    if (cptr == NULL) {                                 /* EOF? */
        stat = SCPE_OK;                                 /* set good return */
        break;
        }
    if (*cptr == 0)                                     /* ignore blank */
        continue;
    if (echo)                                           /* echo if -v */
        sim_printf("%s> %s\n", do_position(), cptr);
    if (*cptr == ':')                                   /* ignore label */
        continue;
    cptr = get_glyph_cmd (cptr, gbuf);                  /* get command glyph */
    sim_switches = 0;                                   /* init switches */
    sim_gotofile = fpin;
    sim_do_echo = echo;
    if ((cmdp = find_cmd (gbuf))) {                     /* lookup command */
        if (cmdp->action == &return_cmd)                /* RETURN command? */
            break;                                      /*    done! */
        if (cmdp->action == &do_cmd) {                  /* DO command? */
            if (sim_do_depth >= MAX_DO_NEST_LVL)        /* nest too deep? */
                stat = SCPE_NEST;
            else
                stat = do_cmd (sim_do_depth+1, cptr);   /* exec DO cmd */
            }
        else
            stat = cmdp->action (cmdp->arg, cptr);      /* exec other cmd */
        }
    else stat = SCPE_UNK;                               /* bad cmd given */
    echo = sim_do_echo;                                 /* Allow for SET VERIFY */
    stat_nomessage = stat & SCPE_NOMESSAGE;             /* extract possible message suppression flag */
    stat_nomessage = stat_nomessage || (!sim_show_message);/* Apply global suppression */
    stat = SCPE_BARE_STATUS(stat);                      /* remove possible flag */
    if (cmdp)
      if (((stat != SCPE_OK) && (stat != SCPE_EXPECT)) ||
          ((cmdp->action != &return_cmd) &&
           (cmdp->action != &goto_cmd) &&
           (cmdp->action != &on_cmd) &&
           (cmdp->action != &echo_cmd)))
        sim_last_cmd_stat = stat;                       /* save command error status */
    switch (stat) {
        case SCPE_AFAIL:
            staying = (sim_on_check[sim_do_depth] &&        /* if trap action defined */
                       sim_on_actions[sim_do_depth][stat]); /* use it, otherwise exit */
            break;
        case SCPE_EXIT:
            staying = FALSE;
            break;
        case SCPE_OK:
        case SCPE_STEP:
            break;
        default:
            break;
        }
    if ((stat >= SCPE_BASE) && (stat != SCPE_EXIT) &&   /* error from cmd? */
        (stat != SCPE_STEP)) {
        if (!echo && !sim_quiet &&                      /* report if not echoing */
            !stat_nomessage &&                          /* and not suppressing messages */
            !(cmdp && cmdp->message)) {                 /* and not handling them specially */
            sim_printf("%s> %s\n", do_position(), sim_do_ocptr[sim_do_depth]);
            }
        }
    if (!stat_nomessage) {                              /* report error if not suppressed */
        if (cmdp && cmdp->message)                      /* special message handler */
            cmdp->message ((!echo && !sim_quiet) ? sim_do_ocptr[sim_do_depth] : NULL, stat);
        else
            if (stat >= SCPE_BASE)                      /* report error if not suppressed */
                sim_printf ("%s\n", sim_error_text (stat));
        }
    if (stat == SCPE_EXPECT)                            /* EXPECT status is non actionable */
        stat = SCPE_OK;                                 /* so adjust it to SCPE_OK */
    if (staying &&
        (sim_on_check[sim_do_depth]) &&
        (stat != SCPE_OK) &&
        (stat != SCPE_STEP)) {
        if ((stat <= SCPE_MAX_ERR) && sim_on_actions[sim_do_depth][stat])
            sim_brk_setact (sim_on_actions[sim_do_depth][stat]);
        else
            sim_brk_setact (sim_on_actions[sim_do_depth][0]);
        }
    if (sim_vm_post != NULL)
        (*sim_vm_post) (TRUE);
    } while (staying);
Cleanup_Return:
if (fpin) //-V547
    fclose (fpin);                                      /* close file */
sim_gotofile = NULL;
if (flag >= 0) {
    sim_do_echo = saved_sim_do_echo;                    /* restore echo state we entered with */
    sim_show_message = saved_sim_show_message;          /* restore message display state we entered with */
    sim_on_inherit = saved_sim_on_inherit;              /* restore ON inheritance state we entered with */
    }
sim_quiet = saved_sim_quiet;                            /* restore quiet mode we entered with */
if ((flag >= 0) || (!sim_on_inherit)) {
    for (i=0; i<SCPE_MAX_ERR; i++) {                    /* release any on commands */
        FREE (sim_on_actions[sim_do_depth][i]);
        sim_on_actions[sim_do_depth][i] = NULL;
        }
    sim_on_check[sim_do_depth] = 0;                     /* clear on mode */
    }
if (flag >= 0)
    --sim_do_depth;                                     /* unwind nesting */
sim_brk_clract ();                                      /* defang breakpoint actions */
if (cmdp && (cmdp->action == &return_cmd) && (0 != *cptr)) { /* return command with argument? */
    sim_string_to_stat (cptr, &stat);
    sim_last_cmd_stat = stat;                           /* save explicit status as command error status */
    if (sim_switches & SWMASK ('Q'))
        stat |= SCPE_NOMESSAGE;                         /* suppress error message display (in caller) if requested */
    return stat;                                        /* return with explicit return status */
    }
return stat | SCPE_NOMESSAGE;                           /* suppress message since we've already done that here */
}

/*
 * Substitute_args - replace %n tokens in 'instr' with the do command's arguments
 *                   and other environment variables
 *
 * Calling sequence
 * instr        =       input string
 * instr_size   =       sizeof input string buffer
 * do_arg[10]   =       arguments
 *
 * Token "%0" expands to the command file name.
 * Token %n (n being a single digit) expands to the n'th argument
 * Tonen %* expands to the whole set of arguments (%1 ... %9)
 *
 * The input sequence "\%" represents a literal "%", and "\\" represents a
 * literal "\".  All other character combinations are rendered literally.
 *
 * Omitted parameters result in null-string substitutions.
 *
 * A Tokens preceded and followed by % characters are expanded as environment
 * variables, and if one isn't found then can be one of several special
 * variables:
 *   %DATE%              yyyy-mm-dd
 *   %TIME%              hh:mm:ss
 *   %STIME%             hh_mm_ss
 *   %CTIME%             Www Mmm dd hh:mm:ss yyyy
 *   %STATUS%            Status value from the last command executed
 *   %TSTATUS%           The text form of the last status value
 *   %SIM_VERIFY%        The Verify/Verbose mode of the current Do command file
 *   %SIM_VERBOSE%       The Verify/Verbose mode of the current Do command file
 *   %SIM_QUIET%         The Quiet mode of the current Do command file
 *   %SIM_MESSAGE%       The message display status of the current Do command file
 * Environment variable lookups are done first with the precise name between
 * the % characters and if that fails, then the name between the % characters
 * is upcased and a lookup of that value is attempted.

 * The first Space delimited token on the line is extracted in uppercase and
 * then looked up as an environment variable.  If found it the value is
 * substituted for the original string before expanding everything else.  If
 * it is not found, then the original beginning token on the line is left
 * untouched.
 */

void sim_sub_args (char *instr, size_t instr_size, char *do_arg[])
{
char gbuf[CBUFSIZE];
char *ip = instr, *op, *oend, *tmpbuf;
const char *ap;
char rbuf[CBUFSIZE];
int i;
time_t now;
struct tm *tmnow;

time(&now);
tmnow = localtime(&now);
tmpbuf = (char *)malloc(instr_size);
if (!tmpbuf)
  {
     (void)fprintf(stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                   __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
     (void)raise(SIGUSR2);
     /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
     abort();
  }
op = tmpbuf;
oend = tmpbuf + instr_size - 2;
while (sim_isspace (*ip))                               /* skip leading spaces */
    *op++ = *ip++;
for (; *ip && (op < oend); ) {
    if ((ip [0] == '\\') &&                             /* literal escape? */
        ((ip [1] == '%') || (ip [1] == '\\'))) {        /*   and followed by '%' or '\'? */
        ip++;                                           /* skip '\' */
        *op++ = *ip++;                                  /* copy escaped char */
        }
    else
        if ((*ip == '%') &&
            (sim_isalnum(ip[1]) || (ip[1] == '*') || (ip[1] == '_'))) {/* sub? */
            if ((ip[1] >= '0') && (ip[1] <= ('9'))) {   /* %n = sub */
                ap = do_arg[ip[1] - '0'];
                for (i=0; i<ip[1] - '0'; ++i)           /* make sure we're not past the list end */
                    if (do_arg[i] == NULL) {
                        ap = NULL;
                        break;
                        }
                ip = ip + 2;
                }
            else if (ip[1] == '*') {                    /* %1 ... %9 = sub */
                (void)memset (rbuf, '\0', sizeof(rbuf));
                ap = rbuf;
                for (i=1; i<=9; ++i)
                    if (do_arg[i] == NULL)
                        break;
                    else
                        if ((sizeof(rbuf)-strlen(rbuf)) < (2 + strlen(do_arg[i]))) {
                            if (strchr(do_arg[i], ' ')) { /* need to surround this argument with quotes */
                                char quote = '"';
                                if (strchr(do_arg[i], quote))
                                    quote = '\'';
                                (void)sprintf(&rbuf[strlen(rbuf)], "%s%c%s%c\"",
                                              (i != 1) ? " " : "", quote,
                                              do_arg[i], quote);
                                }
                            else
                                (void)sprintf(&rbuf[strlen(rbuf)], "%s%s",
                                              (i != 1) ? " " : "", do_arg[i]);
                            }
                        else
                            break;
                ip = ip + 2;
                }
            else {                                      /* environment variable */
                ap = NULL;
                (void)get_glyph_nc (ip+1, gbuf, '%');   /* first try using the literal name */
                ap = getenv(gbuf);
                if (!ap) {
                    (void)get_glyph (ip+1, gbuf, '%');  /* now try using the upcased name */
                    ap = getenv(gbuf);
                    }
                ip += 1 + strlen (gbuf);
                if (*ip == '%') ++ip;
                if (!ap) {
                    /* ISO 8601 format date/time info */
                    if (!strcmp ("DATE", gbuf)) {
                        (void)sprintf (rbuf, "%4d-%02d-%02d",
                                       tmnow->tm_year + 1900,
                                       tmnow->tm_mon  + 1,
                                       tmnow->tm_mday);
                        ap = rbuf;
                        }
                    else if (!strcmp ("TIME", gbuf)) {
                        (void)sprintf (rbuf, "%02d:%02d:%02d",
                                       tmnow->tm_hour,
                                       tmnow->tm_min,
                                       tmnow->tm_sec);
                        ap = rbuf;
                        }
                    else if (!strcmp ("DATETIME", gbuf)) {
                        (void)sprintf (rbuf, "%04d-%02d-%02dT%02d:%02d:%02d",
                                       tmnow->tm_year + 1900,
                                       tmnow->tm_mon  + 1,
                                       tmnow->tm_mday,
                                       tmnow->tm_hour,
                                       tmnow->tm_min,
                                       tmnow->tm_sec);
                        ap = rbuf;
                        }
                    /* Locale oriented formatted date/time info */
                    if (!strcmp ("LDATE", gbuf)) {
                        strftime (rbuf, sizeof(rbuf), "%x", tmnow);
                        ap = rbuf;
                        }
                    else if (!strcmp ("LTIME", gbuf)) {
                        strftime (rbuf, sizeof(rbuf), "%I:%M:%S %p", tmnow);
                        ap = rbuf;
                        }
                    else if (!strcmp ("CTIME", gbuf)) {
                        strftime (rbuf, sizeof(rbuf), "%c", tmnow);
                        ap = rbuf;
                        }
                    /* Separate Date/Time info */
                    else if (!strcmp ("DATE_YYYY", gbuf)) {/* Year (0000-9999) */
                        strftime (rbuf, sizeof(rbuf), "%Y", tmnow);
                        ap = rbuf;
                        }
                    else if (!strcmp ("DATE_YY", gbuf)) {/* Year (00-99) */
                        strftime (rbuf, sizeof(rbuf), "%y", tmnow);
                        ap = rbuf;
                        }
                    else if (!strcmp ("DATE_YC", gbuf)) {/* Century (year/100) */
                        (void)sprintf (rbuf, "%d", (tmnow->tm_year + 1900)/100);
                        ap = rbuf;
                        }
                    else if ((!strcmp ("DATE_19XX_YY", gbuf)) || /* Year with same calendar */
                             (!strcmp ("DATE_19XX_YYYY", gbuf))) {
                        int year = tmnow->tm_year + 1900;
                        int days = year - 2001;
                        int leaps = days/4 - days/100 + days/400;
                        int lyear = ((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0));
                        int selector = ((days + leaps + 7) % 7) + lyear * 7;
                        static int years[] = {90, 91, 97, 98, 99, 94, 89,
                                              96, 80, 92, 76, 88, 72, 84};
                        int cal_year = years[selector];

                        if (!strcmp ("DATE_19XX_YY", gbuf))
                            (void)sprintf (rbuf, "%d", cal_year);        /* 2 digit year */
                        else
                            (void)sprintf (rbuf, "%d", cal_year + 1900); /* 4 digit year */
                        ap = rbuf;
                        }
                    else if (!strcmp ("DATE_MM", gbuf)) {/* Month number (01-12) */
                        strftime (rbuf, sizeof(rbuf), "%m", tmnow);
                        ap = rbuf;
                        }
                    else if (!strcmp ("DATE_MMM", gbuf)) {/* Month number (01-12) */
                        strftime (rbuf, sizeof(rbuf), "%b", tmnow);
                        ap = rbuf;
                        }
                    else if (!strcmp ("DATE_DD", gbuf)) {/* Day of Month (01-31) */
                        strftime (rbuf, sizeof(rbuf), "%d", tmnow);
                        ap = rbuf;
                        }
                    else if (!strcmp ("DATE_D", gbuf)) { /* ISO 8601 weekday number (1-7) */
                        (void)sprintf (rbuf, "%d", (tmnow->tm_wday ? tmnow->tm_wday : 7));
                        ap = rbuf;
                        }
                    else if ((!strcmp ("DATE_WW", gbuf)) ||   /* ISO 8601 week number (01-53) */
                             (!strcmp ("DATE_WYYYY", gbuf))) {/* ISO 8601 week year number (0000-9999) */
                        int iso_yr = tmnow->tm_year + 1900;
                        int iso_wk = (tmnow->tm_yday + 11 - (tmnow->tm_wday ? tmnow->tm_wday : 7))/7;;

                        if (iso_wk == 0) {
                            iso_yr = iso_yr - 1;
                            tmnow->tm_yday += 365 + (((iso_yr % 4) == 0) ? 1 : 0);  /* Adjust for Leap Year (Correct thru 2099) */
                            iso_wk = (tmnow->tm_yday + 11 - (tmnow->tm_wday ? tmnow->tm_wday : 7))/7;
                            }
                        else
                            if ((iso_wk == 53) && (((31 - tmnow->tm_mday) + tmnow->tm_wday) < 4)) {
                                ++iso_yr;
                                iso_wk = 1;
                                }
                        if (!strcmp ("DATE_WW", gbuf))
                            (void)sprintf (rbuf, "%02d", iso_wk);
                        else
                            (void)sprintf (rbuf, "%04d", iso_yr);
                        ap = rbuf;
                        }
                    else if (!strcmp ("DATE_JJJ", gbuf)) {/* day of year (001-366) */
                        strftime (rbuf, sizeof(rbuf), "%j", tmnow);
                        ap = rbuf;
                        }
                    else if (!strcmp ("TIME_HH", gbuf)) {/* Hour of day (00-23) */
                        strftime (rbuf, sizeof(rbuf), "%H", tmnow);
                        ap = rbuf;
                        }
                    else if (!strcmp ("TIME_MM", gbuf)) {/* Minute of hour (00-59) */
                        strftime (rbuf, sizeof(rbuf), "%M", tmnow);
                        ap = rbuf;
                        }
                    else if (!strcmp ("TIME_SS", gbuf)) {/* Second of minute (00-59) */
                        strftime (rbuf, sizeof(rbuf), "%S", tmnow);
                        ap = rbuf;
                        }
                    else if (!strcmp ("STATUS", gbuf)) {
                        (void)sprintf (rbuf, "%08X", sim_last_cmd_stat);
                        ap = rbuf;
                        }
                    else if (!strcmp ("TSTATUS", gbuf)) {
                        (void)sprintf (rbuf, "%s", sim_error_text (sim_last_cmd_stat));
                        ap = rbuf;
                        }
                    else if (!strcmp ("SIM_VERIFY", gbuf)) {
                        (void)sprintf (rbuf, "%s", sim_do_echo ? "-V" : "");
                        ap = rbuf;
                        }
                    else if (!strcmp ("SIM_VERBOSE", gbuf)) {
                        (void)sprintf (rbuf, "%s", sim_do_echo ? "-V" : "");
                        ap = rbuf;
                        }
                    else if (!strcmp ("SIM_LOCALOPC", gbuf)) {
                        (void)sprintf (rbuf, "%s", sim_localopc ? "1" : "");
                        ap = rbuf;
                        }
                    else if (!strcmp ("SIM_QUIET", gbuf)) {
                        (void)sprintf (rbuf, "%s", sim_quiet ? "-Q" : "");
                        ap = rbuf;
                        }
                    else if (!strcmp ("SIM_MESSAGE", gbuf)) {
                        (void)sprintf (rbuf, "%s", sim_show_message ? "" : "-Q");
                        ap = rbuf;
                        }
                    else if (!strcmp ("HOSTID", gbuf)) {
#if defined(HAVE_UNISTD) && !defined(__HAIKU__) && !defined(__ANDROID__) && !defined(__serenity__)
                        (void)sprintf (rbuf, "%ld", (long)gethostid());
#else
                        (void)sprintf (rbuf, "00000000");
#endif /* if defined(HAVE_UNISTD) && !defined(__HAIKU__) && !defined(__ANDROID__) && !defined(__serenity__) */
                        ap = rbuf;
                        }
                    else if (!strcmp ("UID", gbuf)) {
#if defined(HAVE_UNISTD)
                        (void)sprintf (rbuf, "%ld", (long)getuid());
#else
                        (void)sprintf (rbuf, "0");
#endif /* if defined(HAVE_UNISTD) */
                        ap = rbuf;
                        }
                    else if (!strcmp ("GID", gbuf)) {
#if defined(HAVE_UNISTD)
                        (void)sprintf (rbuf, "%ld", (long)getgid());
#else
                        (void)sprintf (rbuf, "0");
#endif /* if defined(HAVE_UNISTD) */
                        ap = rbuf;
                        }
                    else if (!strcmp ("EUID", gbuf)) {
#if defined(HAVE_UNISTD)
                        (void)sprintf (rbuf, "%ld", (long)geteuid());
#else
                        (void)sprintf (rbuf, "0");
#endif /* if defined(HAVE_UNISTD) */
                        ap = rbuf;
                        }
                    else if (!strcmp ("EGID", gbuf)) {
#if defined(HAVE_UNISTD)
                        (void)sprintf (rbuf, "%ld", (long)getegid());
#else
                        (void)sprintf (rbuf, "0");
#endif /* if defined(HAVE_UNISTD) */
                        ap = rbuf;
                        }
                    else if (!strcmp ("PID", gbuf)) {
#if defined(HAVE_UNISTD)
                        (void)sprintf (rbuf, "%ld", (long)getpid());
#else
                        (void)sprintf (rbuf, "0");
#endif /* if defined(HAVE_UNISTD) */
                        ap = rbuf;
                        }
                    else if (!strcmp ("PPID", gbuf)) {
#if defined(HAVE_UNISTD)
                        (void)sprintf (rbuf, "%ld", (long)getppid());
#else
                        (void)sprintf (rbuf, "0");
#endif /* if defined(HAVE_UNISTD) */
                        ap = rbuf;
                        }
                    else if (!strcmp ("PGID", gbuf)) {
#if defined(HAVE_UNISTD)
                        (void)sprintf (rbuf, "%ld", (long)getpgid(getpid()));
#else
                        (void)sprintf (rbuf, "0");
#endif /* if defined(HAVE_UNISTD) */
                        ap = rbuf;
                        }
                    else if (!strcmp ("SID", gbuf)) {
#if defined(HAVE_UNISTD)
                        (void)sprintf (rbuf, "%ld", (long)getsid(getpid()));
#else
                        (void)sprintf (rbuf, "0");
#endif /* if defined(HAVE_UNISTD) */
                        ap = rbuf;
                        }
                    else if (!strcmp ("ENDIAN", gbuf)) {
#if ( defined(DECLITEND) && DECLITEND == 1 )
                        (void)sprintf (rbuf, "LITTLE");
#elif ( defined(DECLITEND) && DECLITEND == 0 )
                        (void)sprintf (rbuf, "BIG");
#else
                        (void)sprintf (rbuf, "UNKNOWN");
#endif /* if ( defined(DECLITEND) && DECLITEND == 1 ) */
                        ap = rbuf;
                        }
                    else if (!strcmp("SIM_NAME", gbuf)) {
                        (void)sprintf (rbuf, "%s", sim_name);
                        ap = rbuf;
                        }
                    else if (!strcmp("SIM_VERSION", gbuf)) {
#if defined(VER_H_GIT_VERSION)
                        (void)sprintf (rbuf, "%s", VER_H_GIT_VERSION);
#else
                        (void)sprintf (rbuf, "UNKNOWN");
#endif /* if defined(VER_H_GIT_VERSION) */
                        ap = rbuf;
                        }
                    else if (!strcmp("SIM_HASH", gbuf)) {
#if defined(VER_H_GIT_HASH)
                        (void)sprintf (rbuf, "%s", VER_H_GIT_HASH);
#else
                        (void)sprintf (rbuf, "0000000000000000000000000000000000000000");
#endif /* if defined(VER_H_GIT_HASH) */
                        ap = rbuf;
                        }
                    else if (!strcmp("SIM_RELT", gbuf)) {
#if defined(VER_H_GIT_RELT)
                        (void)sprintf (rbuf, "%s", VER_H_GIT_RELT);
#else
                        (void)sprintf (rbuf, "X");
#endif /* if defined(VER_H_GIT_RELT) */
                        ap = rbuf;
                        }
                    else if (!strcmp("SIM_DATE", gbuf)) {
#if defined(VER_H_GIT_DATE)
                        (void)sprintf (rbuf, "%s", VER_H_GIT_DATE);
#else
                        (void)sprintf (rbuf, "UNKNOWN");
#endif /* if defined(VER_H_GIT_DATE) */
                        ap = rbuf;
                        }
                    else if ( (!strcmp("CPUS", gbuf)) \
                      || (!strcmp("PROCESSORS", gbuf) ) ) {
#if defined(LINUX_OS) && !defined(__ANDROID__)
                        (void)sprintf(rbuf, "%ld", (long)get_nprocs());
#elif defined(__HAIKU__)
                        system_info hinfo;
                        get_system_info(&hinfo);
                        (void)sprintf (rbuf, "%llu",
                                       (long long unsigned int)hinfo.cpu_count);
#else
                        (void)sprintf(rbuf, "1");
#endif /* if defined(LINUX_OS) && !defined(__ANDROID__) */
                        ap = rbuf;
                        }
                    }
                }
            if (ap) {                                   /* non-null arg? */
                while (*ap && (op < oend))              /* copy the argument */
                    *op++ = *ap++;
                }
            }
        else
            *op++ = *ip++;
    }
*op = 0;                                                /* term buffer */
strcpy (instr, tmpbuf);
FREE (tmpbuf);
return;
}

static
int sim_cmp_string (const char *s1, const char *s2)
{
long int v1, v2;
char *ep1, *ep2;

v1 = strtol(s1+1, &ep1, 0);
v2 = strtol(s2+1, &ep2, 0);
if ((ep1 != s1 + strlen (s1) - 1) ||
    (ep2 != s2 + strlen (s2) - 1))
    return strcmp (s1, s2);
if (v1 == v2)
    return 0;
if (v1 < v2)
    return -1;
return 1;
}

/* Assert command */

t_stat assert_cmd (int32 flag, CONST char *cptr)
{
char gbuf[CBUFSIZE], gbuf2[CBUFSIZE];
CONST char *tptr, *gptr;
REG *rptr;
uint32 idx = 0;
t_value val;
t_stat r;
t_bool Not = FALSE;
t_bool result;
t_addr addr = 0;
t_stat reason;

cptr = (CONST char *)get_sim_opt (CMD_OPT_SW|CMD_OPT_DFT, (CONST char *)cptr, &r);
                                                        /* get sw, default */
#if defined(__NVCOMPILER) || defined(__NVCOMPILER_LLVM__) || defined(__PGI) || defined(__PGLLVM__)
# pragma diagnostic push
# pragma diag_suppress = integer_sign_change
#endif
sim_stabr.boolop = sim_staba.boolop = -1;               /* no relational op dflt */
#if defined(__NVCOMPILER) || defined(__NVCOMPILER_LLVM__) || defined(__PGI) || defined(__PGLLVM__)
# pragma diagnostic pop
#endif
if (*cptr == 0)                                         /* must be more */
    return SCPE_2FARG;
tptr = get_glyph (cptr, gbuf, 0);                       /* get token */
if (!strcmp (gbuf, "NOT")) {                            /* Conditional Inversion? */
    Not = TRUE;                                         /* remember that, and */
    cptr = (CONST char *)tptr;
    }
if (*cptr == '"') {                                     /* quoted string comparison? */
    char op[CBUFSIZE];
    static struct {
        const char *op;
        int aval;
        int bval;
        t_bool invert;
        } *optr, compare_ops[] =
        {
            { "==",   0,  0, FALSE },
            { "EQU",  0,  0, FALSE },
            { "!=",   0,  0, TRUE  },
            { "NEQ",  0,  0, TRUE  },
            { "<",   -1, -1, FALSE },
            { "LSS", -1, -1, FALSE },
            { "<=",   0, -1, FALSE },
            { "LEQ",  0, -1, FALSE },
            { ">",    1,  1, FALSE },
            { "GTR",  1,  1, FALSE },
            { ">=",   0,  1, FALSE },
            { "GEQ",  0,  1, FALSE },
            { NULL }
        };

    tptr = (CONST char *)get_glyph_gen (cptr, gbuf, '=', (sim_switches & SWMASK ('I')), TRUE, '\\');
                                                    /* get first string */
    if (!*tptr)
        return SCPE_2FARG;
    cptr += strlen (gbuf);
    while (sim_isspace (*cptr))                     /* skip spaces */
        ++cptr;
    (void)get_glyph (cptr, op, '"');
    for (optr = compare_ops; optr->op; optr++)
        if (0 == strcmp (op, optr->op))
            break;
    if (!optr->op)
        return sim_messagef (SCPE_ARG, "Invalid operator: %s\n", op);
    cptr += strlen (op);
    while (sim_isspace (*cptr))                         /* skip spaces */
        ++cptr;
    cptr = (CONST char *)get_glyph_gen (cptr, gbuf2, 0, (sim_switches & SWMASK ('I')), TRUE, '\\');
                                                        /* get second string */
    if (*cptr) {                                        /* more? */
        if (flag)                                       /* ASSERT has no more args */
            return SCPE_2MARG;
        }
    else {
        if (!flag)
            return SCPE_2FARG;                          /* IF needs actions! */
        }
    result = sim_cmp_string (gbuf, gbuf2);
    result = ((result == optr->aval) || (result == optr->bval));
    if (optr->invert)
        result = !result;
    }
else {
    cptr = get_glyph (cptr, gbuf, 0);                   /* get register */
    rptr = find_reg (gbuf, &gptr, sim_dfdev);           /* parse register */
    if (rptr) {                                         /* got register? */
        if (*gptr == '[') {                             /* subscript? */
            if (rptr->depth <= 1)                       /* array register? */
                return SCPE_ARG;
            idx = (uint32) strtotv (++gptr, &tptr, 10); /* convert index */
            if ((gptr == tptr) || (*tptr++ != ']'))
                return SCPE_ARG;
            gptr = tptr;                                /* update */
            }
        else idx = 0;                                   /* not array */
        if (idx >= rptr->depth)                         /* validate subscript */
            return SCPE_SUB;
        }
    else {                                              /* not reg, check for memory */
        if (sim_dfdev && sim_vm_parse_addr)             /* get addr */
            addr = sim_vm_parse_addr (sim_dfdev, gbuf, &gptr);
        else
            addr = (t_addr) strtotv (gbuf, &gptr, sim_dfdev ? sim_dfdev->dradix : sim_dflt_dev->dradix);
        if (gbuf == gptr)                               /* error? */
            return SCPE_NXREG;
        }
    if (*gptr != 0)                                     /* more? must be search */
        (void)get_glyph (gptr, gbuf, 0);
    else {
        if (*cptr == 0)                                 /* must be more */
            return SCPE_2FARG;
        cptr = get_glyph (cptr, gbuf, 0);               /* get search cond */
        }
    if (*cptr) {                                        /* more? */
        if (flag)                                       /* ASSERT has no more args */
            return SCPE_2MARG;
        }
    else {
        if (!flag)
            return SCPE_2FARG;                          /* IF needs actions! */
        }
    if (rptr) {                                         /* Handle register case */
#if defined(__NVCOMPILER) || defined(__NVCOMPILER_LLVM__) || defined(__PGI) || defined(__PGLLVM__)
# pragma diagnostic push
# pragma diag_suppress = integer_sign_change
#endif
        if (!get_rsearch (gbuf, rptr->radix, &sim_stabr) ||  /* parse condition */
            (sim_stabr.boolop == -1))                   /* relational op reqd */
            return SCPE_MISVAL;
#if defined(__NVCOMPILER) || defined(__NVCOMPILER_LLVM__) || defined(__PGI) || defined(__PGLLVM__)
# pragma diagnostic pop
#endif
        val = get_rval (rptr, idx);                     /* get register value */
        result = test_search (&val, &sim_stabr);        /* test condition */
        }
    else {                                              /* Handle memory case */
#if defined(__NVCOMPILER) || defined(__NVCOMPILER_LLVM__) || defined(__PGI) || defined(__PGLLVM__)
# pragma diagnostic push
# pragma diag_suppress = integer_sign_change
#endif
        if (!get_asearch (gbuf, sim_dfdev->dradix, &sim_staba) ||  /* parse condition */
            (sim_staba.boolop == -1))                    /* relational op reqd */
            return SCPE_MISVAL;
#if defined(__NVCOMPILER) || defined(__NVCOMPILER_LLVM__) || defined(__PGI) || defined(__PGLLVM__)
# pragma diagnostic pop
#endif
        reason = get_aval (addr, sim_dfdev, sim_dfunit);/* get data */
        if (reason != SCPE_OK)                          /* return if error */
            return reason;
        result = test_search (sim_eval, &sim_staba);    /* test condition */
        }
    }
if (Not ^ result) {
    if (!flag)
        sim_brk_setact (cptr);                          /* set up IF actions */
    }
else
    if (flag)
        return SCPE_AFAIL;                              /* return assert status */
return SCPE_OK;
}

/* Send command */

t_stat send_cmd (int32 flag, CONST char *cptr)
{
char gbuf[CBUFSIZE];
CONST char *tptr;
uint8 dbuf[CBUFSIZE];
uint32 dsize = 0;
uint32 delay = 0;
uint32 after = 0;
t_stat r;
SEND *snd = NULL;

GET_SWITCHES (cptr);                                    /* get switches */
tptr = get_glyph (cptr, gbuf, ',');
if (sim_isalpha(gbuf[0]) && (strchr (gbuf, ':'))) {
    r = tmxr_locate_line_send (gbuf, &snd);
    if (r != SCPE_OK)
      return r;
    cptr = tptr;
    tptr = get_glyph (tptr, gbuf, ',');
    }
else
    snd = sim_cons_get_send ();

while (*cptr) {
    if ((!strncmp(gbuf, "DELAY=", 6)) && (gbuf[6])) {
        delay = (uint32)get_uint (&gbuf[6], 10, 2000000000, &r);
        if (r != SCPE_OK)
            return sim_messagef (SCPE_ARG, "Invalid Delay Value\n");
        cptr = tptr;
        tptr = get_glyph (cptr, gbuf, ',');
        continue;
        }
    if ((!strncmp(gbuf, "AFTER=", 6)) && (gbuf[6])) {
        after = (uint32)get_uint (&gbuf[6], 10, 2000000000, &r);
        if (r != SCPE_OK)
            return sim_messagef (SCPE_ARG, "Invalid After Value\n");
        cptr = tptr;
        tptr = get_glyph (cptr, gbuf, ',');
        continue;
        }
    if ((*cptr == '"') || (*cptr == '\''))
        break;
    return SCPE_ARG;
    }
if (*cptr) {
    if ((*cptr != '"') && (*cptr != '\'')) //-V560
        return sim_messagef (SCPE_ARG, "String must be quote delimited\n");
    cptr = get_glyph_quoted (cptr, gbuf, 0);
    if (*cptr != '\0')
        return SCPE_2MARG;                  /* No more arguments */

    if (SCPE_OK != sim_decode_quoted_string (gbuf, dbuf, &dsize))
        return sim_messagef (SCPE_ARG, "Invalid String\n");
    }
if ((dsize == 0) && (delay == 0) && (after == 0))
    return SCPE_2FARG;
return sim_send_input (snd, dbuf, dsize, after, delay);
}

t_stat sim_show_send (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
char gbuf[CBUFSIZE];
CONST char *tptr;
t_stat r;
SEND *snd = NULL;

tptr = get_glyph (cptr, gbuf, ',');
if (sim_isalpha(gbuf[0]) && (strchr (gbuf, ':'))) {
    r = tmxr_locate_line_send (gbuf, &snd);
    if (r != SCPE_OK)
      return r;
    cptr = tptr;
    }
else
    snd = sim_cons_get_send ();
if (*cptr)
    return SCPE_2MARG;
return sim_show_send_input (st, snd);
}

t_stat expect_cmd (int32 flag, CONST char *cptr)
{
char gbuf[CBUFSIZE];
CONST char *tptr;
EXPECT *exp = NULL;

GET_SWITCHES (cptr);                                    /* get switches */
tptr = get_glyph (cptr, gbuf, ',');
if (sim_isalpha(gbuf[0]) && (strchr (gbuf, ':'))) {
    cptr = tptr;
    }
else
    exp = sim_cons_get_expect ();
if (flag)
    return sim_set_expect (exp, cptr);
else
    return sim_set_noexpect (exp, cptr);
}

t_stat sim_show_expect (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
char gbuf[CBUFSIZE];
CONST char *tptr;
EXPECT *exp = NULL;
t_stat r;

tptr = get_glyph (cptr, gbuf, ',');
if (sim_isalpha(gbuf[0]) && (strchr (gbuf, ':'))) {
    r = tmxr_locate_line_expect (gbuf, &exp);
    if (r != SCPE_OK)
        return r;
    cptr = tptr;
    }
else
    exp = sim_cons_get_expect ();
if (*cptr && (*cptr != '"') && (*cptr != '\''))
    return SCPE_ARG;            /* String must be quote delimited */
tptr = get_glyph_quoted (cptr, gbuf, 0);
if (*tptr != '\0')
    return SCPE_2MARG;          /* No more arguments */
if (*cptr && (cptr[strlen(cptr)-1] != '"') && (cptr[strlen(cptr)-1] != '\''))
    return SCPE_ARG;            /* String must be quote delimited */
return sim_exp_show (st, exp, gbuf);
}

/* Goto command */

t_stat goto_cmd (int32 flag, CONST char *fcptr)
{
char cbuf[CBUFSIZE], gbuf[CBUFSIZE], gbuf1[CBUFSIZE];
const char *cptr;
long fpos;
int32 saved_do_echo = sim_do_echo;
int32 saved_goto_line = sim_goto_line[sim_do_depth];

if (NULL == sim_gotofile) return SCPE_UNK;              /* only valid inside of do_cmd */
(void)get_glyph (fcptr, gbuf1, 0);
if ('\0' == gbuf1[0]) return SCPE_ARG;                  /* unspecified goto target */
fpos = ftell(sim_gotofile);                             /* Save start position */
rewind(sim_gotofile);                                   /* start search for label */
sim_goto_line[sim_do_depth] = 0;                        /* reset line number */
sim_do_echo = 0;                                        /* Don't echo while searching for label */
while (1) {
    cptr = read_line (cbuf, sizeof(cbuf), sim_gotofile);/* get cmd line */
    if (cptr == NULL) break;                            /* exit on eof */
    sim_goto_line[sim_do_depth] += 1;                   /* record line number */
    if (*cptr == 0) continue;                           /* ignore blank */
    if (*cptr != ':') continue;                         /* ignore non-labels */
    ++cptr;                                             /* skip : */
    while (sim_isspace (*cptr)) ++cptr;                 /* skip blanks */
    cptr = get_glyph (cptr, gbuf, 0);                   /* get label glyph */
    if (0 == strcmp(gbuf, gbuf1)) {
        sim_brk_clract ();                              /* goto defangs current actions */
        sim_do_echo = saved_do_echo;                    /* restore echo mode */
        if (sim_do_echo)                                /* echo if -v */
            sim_printf("%s> %s\n", do_position(), cbuf);
        return SCPE_OK;
        }
    }
sim_do_echo = saved_do_echo;                       /* restore echo mode         */
fseek(sim_gotofile, fpos, SEEK_SET);               /* restore start position    */
sim_goto_line[sim_do_depth] = saved_goto_line;     /* restore start line number */
return SCPE_ARG;
}

/* Return command */

/* The return command is invalid unless encountered in a do_cmd context,    */
/* and in that context, it is handled as a special case inside of do_cmd()  */
/* and not dispatched here, so if we get here a return has been issued from */
/* interactive input */

t_stat return_cmd (int32 flag, CONST char *fcptr)
{
return SCPE_UNK;                                 /* only valid inside of do_cmd */
}

/* Shift command */

/* The shift command is invalid unless encountered in a do_cmd context,    */
/* and in that context, it is handled as a special case inside of do_cmd() */
/* and not dispatched here, so if we get here a shift has been issued from */
/* interactive input (it is not valid interactively since it would have to */
/* mess with the program's argv which is owned by the C runtime library    */

t_stat shift_cmd (int32 flag, CONST char *fcptr)
{
return SCPE_UNK;                                 /* only valid inside of do_cmd */
}

/* Call command */

/* The call command is invalid unless encountered in a do_cmd context,     */
/* and in that context, it is handled as a special case inside of do_cmd() */
/* and not dispatched here, so if we get here a call has been issued from  */
/* interactive input                                                       */

t_stat call_cmd (int32 flag, CONST char *fcptr)
{
char cbuf[2*CBUFSIZE], gbuf[CBUFSIZE];
const char *cptr;

if (NULL == sim_gotofile) return SCPE_UNK;              /* only valid inside of do_cmd */
cptr = get_glyph (fcptr, gbuf, 0);
if ('\0' == gbuf[0]) return SCPE_ARG;                   /* unspecified goto target */
(void)snprintf(cbuf, sizeof (cbuf), "%s %s", sim_do_filename[sim_do_depth], cptr);
sim_switches |= SWMASK ('O');                           /* inherit ON state and actions */
return do_cmd_label (flag, cbuf, gbuf);
}

/* On command */

t_stat on_cmd (int32 flag, CONST char *cptr)
{
char gbuf[CBUFSIZE];
t_stat cond;

cptr = get_glyph (cptr, gbuf, 0);
if ('\0' == gbuf[0]) return SCPE_ARG;                   /* unspecified condition */
if (0 == strcmp("ERROR", gbuf))
    cond = 0;
else
    if (SCPE_OK != sim_string_to_stat (gbuf, &cond))
        return SCPE_ARG;
if ((NULL == cptr) || ('\0' == *cptr)) {                /* Empty Action */
    FREE(sim_on_actions[sim_do_depth][cond]);           /* Clear existing condition */
    sim_on_actions[sim_do_depth][cond] = NULL; }
else {
    sim_on_actions[sim_do_depth][cond] =
        (char *)realloc(sim_on_actions[sim_do_depth][cond], 1+strlen(cptr));
    if (!sim_on_actions[sim_do_depth][cond])
      {
        (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                       __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
        abort();
      }
    strcpy(sim_on_actions[sim_do_depth][cond], cptr);
    }
return SCPE_OK;
}

/* noop command */

t_stat noop_cmd (int32 flag, CONST char *cptr)
{
if (cptr && (*cptr != 0))                               /* now eol? */
    return SCPE_2MARG;
return SCPE_OK;                                         /* we're happy doing nothing */
}

/* Set on/noon routine */

t_stat set_on (int32 flag, CONST char *cptr)
{
if ((flag) && (cptr) && (*cptr)) {                      /* Set ON with arg */
    char gbuf[CBUFSIZE];

    cptr = get_glyph (cptr, gbuf, 0);                   /* get command glyph */
    if (((MATCH_CMD(gbuf,"INHERIT")) &&
         (MATCH_CMD(gbuf,"NOINHERIT"))) || //-V600
        (*cptr))
        return SCPE_2MARG;
    if ((gbuf[0]) && (0 == MATCH_CMD(gbuf,"INHERIT"))) //-V560
        sim_on_inherit = 1;
    if ((gbuf[0]) && (0 == MATCH_CMD(gbuf,"NOINHERIT"))) //-V560
        sim_on_inherit = 0;
    return SCPE_OK;
    }
if (cptr && (*cptr != 0))                               /* now eol? */
    return SCPE_2MARG;
sim_on_check[sim_do_depth] = flag;
if ((sim_do_depth != 0) &&
    (NULL == sim_on_actions[sim_do_depth][0])) {        /* default handler set? */
    sim_on_actions[sim_do_depth][0] =                   /* No, so make "RETURN" */
        (char *)malloc(1+strlen("RETURN"));             /* be the default action */
    strcpy(sim_on_actions[sim_do_depth][0], "RETURN");
    }
if ((sim_do_depth != 0) &&
    (NULL == sim_on_actions[sim_do_depth][SCPE_AFAIL])) {/* handler set for AFAIL? */
    sim_on_actions[sim_do_depth][SCPE_AFAIL] =          /* No, so make "RETURN" */
        (char *)malloc(1+strlen("RETURN"));             /* be the action */
    strcpy(sim_on_actions[sim_do_depth][SCPE_AFAIL], "RETURN");
    }
return SCPE_OK;
}

/* Set verify/noverify routine */

t_stat set_verify (int32 flag, CONST char *cptr)
{
if (cptr && (*cptr != 0))                               /* now eol? */
    return SCPE_2MARG;
if (flag == sim_do_echo)                                /* already set correctly? */
    return SCPE_OK;
sim_do_echo = flag;
return SCPE_OK;
}

/* Set message/nomessage routine */

t_stat set_message (int32 flag, CONST char *cptr)
{
if (cptr && (*cptr != 0))                               /* now eol? */
    return SCPE_2MARG;
if (flag == sim_show_message)                           /* already set correctly? */
    return SCPE_OK;
sim_show_message = flag;
return SCPE_OK;
}

/* Set localopc/nolocalopc routine */

t_stat set_localopc (int32 flag, CONST char *cptr)
{
if (cptr && (*cptr != 0))                               /* now eol? */
    return SCPE_2MARG;
if (flag == sim_localopc)                               /* already set correctly? */
    return SCPE_OK;
sim_localopc = flag;
return SCPE_OK;
}
/* Set quiet/noquiet routine */

t_stat set_quiet (int32 flag, CONST char *cptr)
{
if (cptr && (*cptr != 0))                               /* now eol? */
    return SCPE_2MARG;
if (flag == sim_quiet)                                  /* already set correctly? */
    return SCPE_OK;
sim_quiet = flag;
return SCPE_OK;
}

/* Set environment routine */

t_stat sim_set_environment (int32 flag, CONST char *cptr)
{
char varname[CBUFSIZE];

if ((NULL == cptr) || (*cptr == 0))                            /* now eol? */
    return SCPE_2FARG;
cptr = get_glyph (cptr, varname, '=');                  /* get environment variable name */
setenv(varname, cptr, 1);
return SCPE_OK;
}

/* Set command */

t_stat set_cmd (int32 flag, CONST char *cptr)
{
uint32 lvl = 0;
t_stat r;
char gbuf[CBUFSIZE], *cvptr;
CONST char *svptr;
DEVICE *dptr;
UNIT *uptr;
MTAB *mptr;
CTAB *gcmdp;
C1TAB *ctbr = NULL, *glbr;

GET_SWITCHES (cptr);                                    /* get switches */
if ((NULL == cptr) || (*cptr == 0)) //-V560             /* must be more */
    return SCPE_2FARG;
cptr = get_glyph (svptr = cptr, gbuf, 0);               /* get glob/dev/unit */

if ((dptr = find_dev (gbuf))) {                         /* device match? */
    uptr = dptr->units;                                 /* first unit */
    ctbr = set_dev_tab;                                 /* global table */
    lvl = MTAB_VDV;                                     /* device match */
    GET_SWITCHES (cptr);                                /* get more switches */
    }
else if ((dptr = find_unit (gbuf, &uptr))) {            /* unit match? */
    if (uptr == NULL)                                   /* invalid unit */
        return SCPE_NXUN;
    ctbr = set_unit_tab;                                /* global table */
    lvl = MTAB_VUN;                                     /* unit match */
    GET_SWITCHES (cptr);                                /* get more switches */
    }
else if ((gcmdp = find_ctab (set_glob_tab, gbuf))) {    /* global? */
    GET_SWITCHES (cptr);                                /* get more switches */
    return gcmdp->action (gcmdp->arg, cptr);            /* do the rest */
    }
else {
    if (sim_dflt_dev && sim_dflt_dev->modifiers) {
        if ((cvptr = strchr (gbuf, '=')))               /* = value? */
            *cvptr++ = 0;
        for (mptr = sim_dflt_dev->modifiers; mptr->mask != 0; mptr++) {
            if (mptr->mstring && (MATCH_CMD (gbuf, mptr->mstring) == 0)) {
                dptr = sim_dflt_dev;
                cptr = svptr;
                while (sim_isspace(*cptr))
                    ++cptr;
                break;
                }
            }
        }
    if (!dptr)
        return SCPE_NXDEV;                              /* no match */
    lvl = MTAB_VDV;                                     /* device match */
    uptr = dptr->units;                                 /* first unit */
    }
if ((*cptr == 0) || (*cptr == ';') || (*cptr == '#'))   /* must be more */
    return SCPE_2FARG;
GET_SWITCHES (cptr);                                    /* get more switches */

while (*cptr != 0) {                                    /* do all mods */
    cptr = get_glyph (svptr = cptr, gbuf, ',');         /* get modifier */
    if (0 == strcmp (gbuf, ";"))
        break;
    if ((cvptr = strchr (gbuf, '=')))                   /* = value? */
        *cvptr++ = 0;
    for (mptr = dptr->modifiers; mptr && (mptr->mask != 0); mptr++) {
        if ((mptr->mstring) &&                          /* match string */
            (MATCH_CMD (gbuf, mptr->mstring) == 0)) {   /* matches option? */
            if (mptr->mask & MTAB_XTD) {                /* extended? */
                if (((lvl & mptr->mask) & ~MTAB_XTD) == 0)
                    return SCPE_ARG;
                if ((lvl == MTAB_VUN) && (uptr->flags & UNIT_DIS))
                    return SCPE_UDIS;                   /* unit disabled? */
                if (mptr->valid) {                      /* validation rtn? */
                    if (cvptr && MODMASK(mptr,MTAB_QUOTE)) {
                        svptr = get_glyph_quoted (svptr, gbuf, ',');
                        if ((cvptr = strchr (gbuf, '='))) {
                            *cvptr++ = 0;
                            cptr = svptr;
                            }
                        }
                    else {
                        if (cvptr && MODMASK(mptr,MTAB_NC)) {
                            (void)get_glyph_nc (svptr, gbuf, ',');
                            if ((cvptr = strchr (gbuf, '=')))
                                *cvptr++ = 0;
                            }
                        }
                    r = mptr->valid (uptr, mptr->match, cvptr, mptr->desc);
                    if (r != SCPE_OK)
                        return r;
                    }
                else if (!mptr->desc)                   /* value desc? */
                    break;
                else if (cvptr)                         /* = value? */
                    return SCPE_ARG;
                else *((int32 *) mptr->desc) = mptr->match;
                }                                       /* end if xtd */
            else {                                      /* old style */
                if (cvptr)                              /* = value? */
                    return SCPE_ARG;
                if (uptr->flags & UNIT_DIS)             /* disabled? */
                     return SCPE_UDIS;
                if ((mptr->valid) &&                    /* invalid? */
                    ((r = mptr->valid (uptr, mptr->match, cvptr, mptr->desc)) != SCPE_OK))
                    return r;
                uptr->flags = (uptr->flags & ~(mptr->mask)) |
                    (mptr->match & mptr->mask);         /* set new value */
                }                                       /* end else xtd */
            break;                                      /* terminate for */
            }                                           /* end if match */
        }                                               /* end for */
    if (!mptr || (mptr->mask == 0)) {                   /* no match? */
        if ((glbr = find_c1tab (ctbr, gbuf))) {         /* global match? */
            r = glbr->action (dptr, uptr, glbr->arg, cvptr);    /* do global */
            if (r != SCPE_OK)
                return r;
            }
        else if (!dptr->modifiers)                      /* no modifiers? */
            return SCPE_NOPARAM;
        else return SCPE_NXPAR;
        }                                               /* end if no mat */
    }                                                   /* end while */
return SCPE_OK;                                         /* done all */
}

/* Match CTAB/CTAB1 name */

CTAB *find_ctab (CTAB *tab, const char *gbuf)
{
if (!tab)
    return NULL;
for (; tab->name != NULL; tab++) {
    if (MATCH_CMD (gbuf, tab->name) == 0)
        return tab;
    }
return NULL;
}

C1TAB *find_c1tab (C1TAB *tab, const char *gbuf)
{
if (!tab)
    return NULL;
for (; tab->name != NULL; tab++) {
    if (MATCH_CMD (gbuf, tab->name) == 0)
        return tab;
    }
return NULL;
}

/* Set device data radix routine */

t_stat set_dev_radix (DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
if (cptr)
    return SCPE_ARG;
dptr->dradix = flag & 037;
return SCPE_OK;
}

/* Set device enabled/disabled routine */

t_stat set_dev_enbdis (DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
UNIT *up;
uint32 i;

if (cptr)
    return SCPE_ARG;
if ((dptr->flags & DEV_DISABLE) == 0)                   /* allowed? */
    return SCPE_NOFNC;
if (flag) {                                             /* enable? */
    if ((dptr->flags & DEV_DIS) == 0)                   /* already enb? ok */
        return SCPE_OK;
    dptr->flags = dptr->flags & ~DEV_DIS;               /* no, enable */
    }
else {
    if (dptr->flags & DEV_DIS)                          /* already dsb? ok */
        return SCPE_OK;
    for (i = 0; i < dptr->numunits; i++) {              /* check units */
        up = (dptr->units) + i;                         /* att or active? */
        if ((up->flags & UNIT_ATT) || sim_is_active (up))
            return SCPE_NOFNC;                          /* can't do it */
        }
    dptr->flags = dptr->flags | DEV_DIS;                /* disable */
    }
if (dptr->reset)                                        /* reset device */
    return dptr->reset (dptr);
else return SCPE_OK;
}

/* Set unit enabled/disabled routine */

t_stat set_unit_enbdis (DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
if (cptr)
    return SCPE_ARG;
if (!(uptr->flags & UNIT_DISABLE))                      /* allowed? */
    return SCPE_NOFNC;
if (flag)                                               /* enb? enable */
    uptr->flags = uptr->flags & ~UNIT_DIS;
else {
    if ((uptr->flags & UNIT_ATT) ||                     /* dsb */
        sim_is_active (uptr))                           /* more tests */
        return SCPE_NOFNC;
    uptr->flags = uptr->flags | UNIT_DIS;               /* disable */
    }
return SCPE_OK;
}

/* Set device debug enabled/disabled routine */

t_stat set_dev_debug (DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
char gbuf[CBUFSIZE];
DEBTAB *dep;

if ((dptr->flags & DEV_DEBUG) == 0)
    return SCPE_NOFNC;
if (cptr == NULL) {                                     /* no arguments? */
    dptr->dctrl = flag ? (dptr->debflags ? flag : 0xFFFFFFFF) : 0;/* disable/enable w/o table */
    if (flag && dptr->debflags) {                       /* enable with table? */
        for (dep = dptr->debflags; dep->name != NULL; dep++)
            dptr->dctrl = dptr->dctrl | dep->mask;      /* set all */
        }
    return SCPE_OK;
    }
if (dptr->debflags == NULL)                             /* must have table */
    return SCPE_ARG;
while (*cptr) {
    cptr = get_glyph (cptr, gbuf, ';');                 /* get debug flag */
    for (dep = dptr->debflags; dep->name != NULL; dep++) {
        if (strcmp (dep->name, gbuf) == 0) {            /* match? */
            if (flag)
                dptr->dctrl = dptr->dctrl | dep->mask;
            else dptr->dctrl = dptr->dctrl & ~dep->mask;
            break;
            }
        }                                               /* end for */
    if (dep->mask == 0)                                 /* no match? */
        return SCPE_ARG;
    }                                                   /* end while */
return SCPE_OK;
}

/* Show command */

t_stat show_cmd (int32 flag, CONST char *cptr)
{
t_stat r = SCPE_IERR;

cptr = get_sim_opt (CMD_OPT_SW|CMD_OPT_OF, cptr, &r);
                                                        /* get sw, ofile */
if (NULL == cptr)                                              /* error? */
    return r;
if (sim_ofile) {                                        /* output file? */
    r = show_cmd_fi (sim_ofile, flag, cptr);            /* do show */
    fclose (sim_ofile);
    }
else {
    r = show_cmd_fi (stdout, flag, cptr);               /* no, stdout, log */
    if (sim_log && (sim_log != stdout))
        show_cmd_fi (sim_log, flag, cptr);
    if (sim_deb && (sim_deb != stdout) && (sim_deb != sim_log))
        show_cmd_fi (sim_deb, flag, cptr);
    }
return r;
}

t_stat show_cmd_fi (FILE *ofile, int32 flag, CONST char *cptr)
{
uint32 lvl = 0xFFFFFFFF;
char gbuf[CBUFSIZE], *cvptr;
CONST char *svptr;
DEVICE *dptr;
UNIT *uptr;
MTAB *mptr;
SHTAB *shtb = NULL, *shptr;

GET_SWITCHES (cptr);                                    /* get switches */
if ((*cptr == 0) || (*cptr == ';') || (*cptr == '#'))   /* must be more */
    return SCPE_2FARG;
cptr = get_glyph (svptr = cptr, gbuf, 0);               /* get next glyph */

if ((dptr = find_dev (gbuf))) {                         /* device match? */
    uptr = dptr->units;                                 /* first unit */
    shtb = show_dev_tab;                                /* global table */
    lvl = MTAB_VDV;                                     /* device match */
    GET_SWITCHES (cptr);                                /* get more switches */
    }
else if ((dptr = find_unit (gbuf, &uptr))) {            /* unit match? */
    if (uptr == NULL)                                   /* invalid unit */
        return sim_messagef (SCPE_NXUN, "Non-existent unit: %s\n", gbuf);
    if (uptr->flags & UNIT_DIS)                         /* disabled? */
        return sim_messagef (SCPE_UDIS, "Unit disabled: %s\n", gbuf);
    shtb = show_unit_tab;                               /* global table */
    lvl = MTAB_VUN;                                     /* unit match */
    GET_SWITCHES (cptr);                                /* get more switches */
    }
else if ((shptr = find_shtab (show_glob_tab, gbuf))) {  /* global? */
    GET_SWITCHES (cptr);                                /* get more switches */
    return shptr->action (ofile, NULL, NULL, shptr->arg, cptr);
    }
else {
    if (sim_dflt_dev && sim_dflt_dev->modifiers) {
        if ((cvptr = strchr (gbuf, '=')))               /* = value? */
            *cvptr++ = 0;
        for (mptr = sim_dflt_dev->modifiers; mptr && (mptr->mask != 0); mptr++) {
            if ((((mptr->mask & MTAB_VDV) == MTAB_VDV) &&
                 (mptr->pstring && (MATCH_CMD (gbuf, mptr->pstring) == 0))) || //-V600
                (!(mptr->mask & MTAB_VDV) && (mptr->mstring && (MATCH_CMD (gbuf, mptr->mstring) == 0)))) {
                dptr = sim_dflt_dev;
                lvl = MTAB_VDV;                         /* device match */
                cptr = svptr;
                while (sim_isspace(*cptr))
                    ++cptr;
                break;
                }
            }
        }
    if (!dptr) {
        if (sim_dflt_dev && (shptr = find_shtab (show_dev_tab, gbuf)))  /* global match? */
            return shptr->action (ofile, sim_dflt_dev, uptr, shptr->arg, cptr);
        else
            return sim_messagef (SCPE_NXDEV, "Non-existent device: %s\n", gbuf);/* no match */
        }
    }

if ((*cptr == 0) || (*cptr == ';') || (*cptr == '#')) { /* now eol? */
    return (lvl == MTAB_VDV)?
        show_device (ofile, dptr, 0):
        show_unit (ofile, dptr, uptr, -1);
    }
GET_SWITCHES (cptr);                                    /* get more switches */

while (*cptr != 0) {                                    /* do all mods */
    cptr = get_glyph (cptr, gbuf, ',');                 /* get modifier */
    if ((cvptr = strchr (gbuf, '=')))                   /* = value? */
        *cvptr++ = 0;
    for (mptr = dptr->modifiers; mptr && (mptr->mask != 0); mptr++) {
        if (((mptr->mask & MTAB_XTD)?                   /* right level? */
            ((mptr->mask & lvl) == lvl): (MTAB_VUN & lvl)) &&
            ((mptr->disp && mptr->pstring &&            /* named disp? */
            (MATCH_CMD (gbuf, mptr->pstring) == 0))
            )) {
            if (cvptr && !MODMASK(mptr,MTAB_SHP))
                return sim_messagef (SCPE_ARG, "Invalid Argument: %s=%s\n", gbuf, cvptr);
            show_one_mod (ofile, dptr, uptr, mptr, cvptr, 1);
            break;
            }                                           /* end if */
        }                                               /* end for */
    if (!mptr || (mptr->mask == 0)) {                   /* no match? */
        if (shtb && (shptr = find_shtab (shtb, gbuf))) {/* global match? */
            t_stat r;

            r = shptr->action (ofile, dptr, uptr, shptr->arg, cptr);
            if (r != SCPE_OK)
                return r;
            }
        else {
            if (!dptr->modifiers)                       /* no modifiers? */
                return sim_messagef (SCPE_NOPARAM, "%s device has no parameters\n", dptr->name);
            else
                return sim_messagef (SCPE_NXPAR, "Non-existent parameter: %s\n", gbuf);
            }
        }                                               /* end if */
    }                                                   /* end while */
return SCPE_OK;
}

SHTAB *find_shtab (SHTAB *tab, const char *gbuf)
{
if (!tab)
    return NULL;
for (; tab->name != NULL; tab++) {
    if (MATCH_CMD (gbuf, tab->name) == 0)
        return tab;
    }
return NULL;
}

/* Show device and unit */

t_stat show_device (FILE *st, DEVICE *dptr, int32 flag)
{
uint32 j, udbl, ucnt;
UNIT *uptr;
int32 toks = 0;

if (strcmp(sim_dname (dptr),"SYS") != 0) {
(void)fprintf (st, "%s", sim_dname (dptr));                   /* print dev name */
if ((flag == 2) && dptr->description) {
    (void)fprintf (st, "\t%s\n", dptr->description(dptr));
    }
else {
    if ((sim_switches & SWMASK ('D')) && dptr->description)
        (void)fprintf (st, "\t%s\n", dptr->description(dptr));
    }
if (qdisable (dptr)) {                                  /* disabled? */
    (void)fprintf (st, "\tdisabled\n");
    return SCPE_OK;
    }
for (j = ucnt = udbl = 0; j < dptr->numunits; j++) {    /* count units */
    uptr = dptr->units + j;
    if (!(uptr->flags & UNIT_DIS))                      /* count enabled units */
        ucnt++;
    else if (uptr->flags & UNIT_DISABLE)
        udbl++;                                         /* count user-disabled */
    }
//show_all_mods (st, dptr, dptr->units, MTAB_VDV, &toks); /* show dev mods */
if (dptr->numunits == 0) {
    if (toks) //-V547
        (void)fprintf (st, "\n");
    }
else {
    if (ucnt == 0) {
        fprint_sep (st, &toks);
        (void)fprintf (st, "all units disabled\n");
        }
    else if ((ucnt + udbl) == 1) {
        fprint_sep (st, &toks);
        (void)fprintf (st, " 1 unit\n");
        }
    else if ((ucnt > 1) || (udbl > 0)) {
        fprint_sep (st, &toks);
        (void)fprintf (st, "%2.d units\n", ucnt + udbl);
        }
    else
        if ((flag != 2) || !dptr->description || toks)
            (void)fprintf (st, "\n");
    toks = 0;
    }
if (flag)                                               /* dev only? */
    return SCPE_OK;
for (j = 0; j < dptr->numunits; j++) {                  /* loop thru units */
    uptr = dptr->units + j;
    if ((uptr->flags & UNIT_DIS) == 0)
        show_unit (st, dptr, uptr, ucnt + udbl);
    }
}
return SCPE_OK;
}

void fprint_sep (FILE *st, int32 *tokens)
{
(void)fprintf (st, "%s", (*tokens > 0) ? "" : "\t");
*tokens += 1;
}

t_stat show_unit (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag)
{
int32 u = (int32)(uptr - dptr->units);
int32 toks = 0;

if (flag > 1)
    (void)fprintf (st, "   %s%d \n", sim_dname (dptr), u);
else if (flag < 0)
    (void)fprintf (st, " %s%d ", sim_dname (dptr), u);
if (uptr->flags & UNIT_ATT) {
    fprint_sep (st, &toks);
    (void)fprintf (st, "status   : attached to %s", uptr->filename);
    if (uptr->flags & UNIT_RO)
        (void)fprintf (st, ", read only");
    }
else {
    if (uptr->flags & UNIT_ATTABLE) {
        fprint_sep (st, &toks);
        (void)fprintf (st, "status   : not attached");
        }
    }
if ((uptr->capac > 0) && (uptr->flags & UNIT_FIX)) {
    fprint_sep (st, &toks);
    fprint_capac (st, dptr, uptr);
    }
show_all_mods (st, dptr, uptr, MTAB_VUN, &toks);        /* show unit mods */
if (toks || (flag < 0) || (flag > 1))
    (void)fprintf (st, "\n");
return SCPE_OK;
}

const char *sprint_capac (DEVICE *dptr, UNIT *uptr)
{
static char capac_buf[((CHAR_BIT * sizeof (t_value) * 4 + 3)/3) + 8];
t_offset kval = (t_offset)((uptr->flags & UNIT_BINK) ? 1024: 1000);
t_offset mval;
t_offset psize = (t_offset)uptr->capac;
char *scale, *width;

if (sim_switches & SWMASK ('B'))
    kval = 1024;
mval = kval * kval;
if (dptr->flags & DEV_SECTORS) {
    kval = kval / 512;
    mval = mval / 512;
    }
if ((dptr->dwidth / dptr->aincr) > 8)
    width = "W";
else
    width = "B";
if ((psize < (kval * 10)) &&
    (0 != (psize % kval))) {
    scale = "";
    }
else if ((psize < (mval * 10)) &&
         (0 != (psize % mval))){
    scale = "K";
    psize = psize / kval;
    }
else {
    scale = "M";
    psize = psize / mval;
    }
sprint_val (capac_buf, (t_value) psize, 10, T_ADDR_W, PV_LEFT);
(void)sprintf (&capac_buf[strlen (capac_buf)], "%s%s", scale, width);
return capac_buf;
}

void fprint_capac (FILE *st, DEVICE *dptr, UNIT *uptr)
{
(void)fprintf (st, " %s", sprint_capac (dptr, uptr));
}

/* Show <global name> processors  */

extern void print_default_base_system_script (void);
t_stat show_default_base_system_script (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
#if !defined(PERF_STRIP)
  print_default_base_system_script();
#endif /* if !defined(PERF_STRIP) */
  return 0;
}

static void printp (unsigned char * PROM, char * label, int offset, int length) {
  sim_printf (" %s ", label);
  sim_printf ("   %2d     %3o(8)     '", length, offset);
  for (int l = 0; l < length; l ++)
    {
      unsigned int byte = PROM[offset + l];
      if (byte == 255)
        {
          byte = ' ';
        }
      sim_printf (isprint (byte) ? "%c" : "\\%03o", byte);
    }
  sim_printf ("'\r\n");
}

static void strip_spaces(char* str) {
  int i, x;
  for (i=x=0; str[i]; ++i)
    {
      if (!isspace((int)str[i]) || (i > 0 && !isspace((int)str[(int)(i-1)]))) //-V781
        {
          str[x++] = str[i];
        }
    }
  str[x] = '\0';
  i = -1;
  x = 0;
  while (str[x] != '\0')
    {
      if (str[x] != ' ' && str[x] != '\t' && str[x] != '\n')
        {
          i=x;
        }
      x++;
    }
  str[i+1] = '\0';
}

static void printpq (unsigned char * PROM, FILE * st, int offset, int length) {
  char sx[1024];
  sx[1023] = '\0';
  unsigned int lastbyte = 0;
  for (int l = 0; l < length; l ++)
    {
      unsigned int byte = PROM[offset + l];
      if (byte == 255)
        {
          byte = 20;
        }
      if ((lastbyte != 20) && (byte != 20))
        {
          (void)sprintf(&sx[l], isprint (byte) ? "%c" : " ", byte);
        }
      lastbyte = byte;
    }
  strip_spaces(sx);
  (void)fprintf (st, "%s", sx);
}

t_stat show_prom (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
  unsigned char PROM[1024];
  setupPROM (0, PROM);

  sim_printf (" PROM size: %llu bytes\r\n",
              (long long unsigned)sizeof(PROM));
  sim_printf (" PROM initialization data:\r\n\r\n");

  sim_printf ("     Field Description      Length   Offset              Contents\r\n");
  sim_printf (" ========================= ======== ======== ==================================\r\n");
  sim_printf ("\r\n");

  //                      Field                 Offset       Length
  //             -------------------------    ----------   ----------
  printp (PROM, "CPU Model                ",       0,          11);
  printp (PROM, "CPU Serial               ",      11,          11);
  printp (PROM, "Ship Date                ",      22,           6);
  printp (PROM, "PROM Layout Version      ",      60,           1);
  printp (PROM, "Release Git Commit Date  ",      70,          10);
  printp (PROM, "Release Major            ",      80,           3);
  printp (PROM, "Release Minor            ",      83,           3);
  printp (PROM, "Release Patch            ",      86,           3);
  printp (PROM, "Release Iteration        ",      89,           3);
  printp (PROM, "Release Build Number     ",      92,           8);  /* Reserved */
  printp (PROM, "Release Type             ",     100,           1);
  printp (PROM, "Release Version Text     ",     101,          29);
  printp (PROM, "Build Architecture       ",     130,          20);
  printp (PROM, "Build Operating System   ",     150,          20);
  printp (PROM, "Target Architecture      ",     170,          20);
  printp (PROM, "Target Operating System  ",     190,          20);

  sim_printf("\r\n");
  return 0;
}

t_stat show_buildinfo (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
    (void)fprintf (st, "\r Build Information:\n");
#if defined(BUILDINFO_scp)
    (void)fprintf (st, "\r\n      Compilation info: %s\n", BUILDINFO_scp );
#else
    (void)fprintf (st, "\r\n      Compilation info: Not available\n" );
#endif
#if !defined(__CYGWIN__)
# if !defined(__APPLE__)
#  if !defined(_AIX)
#   if !defined(__MINGW32__)
#    if !defined(__MINGW64__)
#     if !defined(CROSS_MINGW32)
#      if !defined(CROSS_MINGW64)
#       if !defined(_WIN32)
#        if !defined(__HAIKU__)
    (void)dl_iterate_phdr (dl_iterate_phdr_callback, NULL);
    if (dl_iterate_phdr_callback_called)
        (void)fprintf (st, "\n");
#        endif
#       endif
#      endif
#     endif
#    endif
#   endif
#  endif
# endif
#endif
#if defined(UV_VERSION_MAJOR) && \
    defined(UV_VERSION_MINOR) && \
    defined(UV_VERSION_PATCH)
# if defined(UV_VERSION_MAJOR)
#  if !defined(UV_VERSION_MINOR) && \
      !defined(UV_VERSION_PATCH) && \
      !defined(UV_VERSION_SUFFIX)
    (void)fprintf (st, "\r\n    Event loop library: Built with libuv v%d", UV_VERSION_MAJOR);
#  endif /* if !defined(UV_VERSION_MINOR) && !defined(UV_VERSION_PATCH) && defined(UV_VERSION_SUFFIX) */
#  if defined(UV_VERSION_MINOR)
#   if !defined(UV_VERSION_PATCH) && !defined(UV_VERSION_SUFFIX)
    (void)fprintf (st, "\r\n    Event loop library: Built with libuv %d.%d", UV_VERSION_MAJOR,
                   UV_VERSION_MINOR);
#   endif /* if !defined(UV_VERSION_PATCH) && !defined(UV_VERSION_SUFFIX) */
#   if defined(UV_VERSION_PATCH)
#    if !defined(UV_VERSION_SUFFIX)
    (void)fprintf (st, "\r\n    Event loop library: Built with libuv %d.%d.%d", UV_VERSION_MAJOR,
                   UV_VERSION_MINOR, UV_VERSION_PATCH);
#    endif /* if !defined(UV_VERSION_SUFFIX) */
#    if defined(UV_VERSION_SUFFIX)
    (void)fprintf (st, "\r\n    Event loop library: Built with libuv %d.%d.%d", UV_VERSION_MAJOR,
                   UV_VERSION_MINOR, UV_VERSION_PATCH);
#     if defined(UV_VERSION_IS_RELEASE)
#      if UV_VERSION_IS_RELEASE == 1
#       define UV_RELEASE_TYPE " (release)"
#      endif /* if UV_VERSION_IS_RELEASE == 1 */
#      if UV_VERSION_IS_RELEASE == 0
#       define UV_RELEASE_TYPE "-dev"
#      endif /* if UV_VERSION_IS_RELEASE == 0 */
#      if !defined(UV_RELEASE_TYPE)
#       define UV_RELEASE_TYPE ""
#      endif /* if !defined(UV_RELEASE_TYPE) */
#      if defined(UV_RELEASE_TYPE)
    (void)fprintf (st, "%s", UV_RELEASE_TYPE);
#      endif /* if defined(UV_RELEASE_TYPE) */
#     endif /* if defined(UV_VERSION_IS_RELEASE) */
#    endif /* if defined(UV_VERSION_SUFFIX) */
#   endif /* if defined(UV_VERSION_PATCH) */
#  endif /* if defined(UV_VERSION_MINOR) */
    unsigned int CurrentUvVersion = uv_version();
    if (CurrentUvVersion > 0)
        if (uv_version_string() != NULL)
            (void)fprintf (st, "; %s in use", uv_version_string());
# endif /* if defined(UV_VERSION_MAJOR) */
#else
    (void)fprintf (st, "\r\n    Event loop library: Using libuv (or compatible) library, unknown version");
#endif /* if defined(UV_VERSION_MAJOR) &&  \
        *    defined(UV_VERSION_MINOR) &&  \
        *    defined(UV_VERSION_PATCH)     \
        */
#if defined(DECNUMBERLOC)
# if defined(DECVERSION)
#  if defined(DECVERSEXT)
    (void)fprintf (st, "\r\n          Math library: %s-%s", DECVERSION, DECVERSEXT);
#  else
#   if defined(DECNLAUTHOR)
    (void)fprintf (st, "\r\n          Math library: %s (%s and contributors)", DECVERSION, DECNLAUTHOR);
#   else
    (void)fprintf (st, "\r\n          Math library: %s", DECVERSION);
#   endif /* if defined(DECNLAUTHOR) */
#  endif /* if defined(DECVERSEXT) */
# else
    (void)fprintf (st, "\r\n          Math library: decNumber, unknown version");
# endif /* if defined(DECVERSION) */
#endif /* if defined(DECNUMBERLOC) */
#if defined(LOCKLESS)
    (void)fprintf (st, "\r\n     Atomic operations: ");
# if defined(AIX_ATOMICS)
    (void)fprintf (st, "IBM AIX-style");
# elif defined(BSD_ATOMICS)
    (void)fprintf (st, "FreeBSD-style");
# elif defined(GNU_ATOMICS)
    (void)fprintf (st, "GNU-style");
# elif defined(SYNC_ATOMICS)
    (void)fprintf (st, "GNU sync-style");
# elif defined(ISO_ATOMICS)
    (void)fprintf (st, "ISO/IEC 9899:2011 (C11) standard");
# elif defined(NT_ATOMICS)
    (void)fprintf (st, "Windows NT interlocked operations");
# endif
#endif /* if defined(LOCKLESS) */
    (void)fprintf (st, "\r\n          File locking: ");
#if defined(USE_FCNTL) && defined(USE_FLOCK)
    (void)fprintf (st, "POSIX-style fcntl() and BSD-style flock() locking");
#endif
#if defined(USE_FCNTL) && !defined(USE_FLOCK)
    (void)fprintf (st, "POSIX-style fcntl() locking");
#endif
#if defined(USE_FLOCK) && !defined(USE_FCNTL)
    (void)fprintf (st, "BSD-style flock() locking");
#endif
#if !defined(USE_FLOCK) && !defined(USE_FCNTL)
    (void)fprintf (st, "No file locking available");
#endif
    (void)fprintf (st, "\r\n     Backtrace support: ");
#if defined(USE_BACKTRACE)
    (void)fprintf (st, "Enabled (libbacktrace)");
#else
    (void)fprintf (st, "Disabled");
#endif /* if defined(USE_BACKTRACE) */
    (void)fprintf (st, "\r\n       Windows support: ");
#if defined(__MINGW32__) || defined(__MINGW64__) || defined(CROSS_MINGW32) || defined(CROSS_MINGW64)
# if defined(__MINGW64_VERSION_STR)
    (void)fprintf (st, "Built with MinGW-w64 %s", __MINGW64_VERSION_STR);
# elif defined(__MINGW32_MAJOR_VERSION) && defined(__MINGW32_MINOR_VERSION)
    (void)fprintf (st, "Built with MinGW %d.%d", __MINGW32_MAJOR_VERSION, __MINGW32_MINOR_VERSION);
# else
    (void)fprintf (st, "Built with MinGW");
# endif

# if defined(MINGW_CRT)
    (void)fprintf (st, "; %s", MINGW_CRT);
#  if !defined(_UCRT)
#   if defined(__MSVCRT_VERSION__)
#    if __MSVCRT_VERSION__ > 0x00
    (void)fprintf (st, " %d.%d", (__MSVCRT_VERSION__ >> CHAR_BIT) & UCHAR_MAX, __MSVCRT_VERSION__ & UCHAR_MAX);
#    endif
#   endif
#  else

    struct UCRTVersion ucrtversion;
    int result = GetUCRTVersion (&ucrtversion);

    if (result == 0)
      (void)fprintf (st, " %u.%u.%u.%u",
                     ucrtversion.ProductVersion[1], ucrtversion.ProductVersion[0],
                     ucrtversion.ProductVersion[3], ucrtversion.ProductVersion[2]);
#  endif
    (void)fprintf (st, " in use");
# endif
#elif defined(__CYGWIN__)
    struct utsname utsname;
    (void)fprintf (st, "Built with Cygwin %d.%d.%d",
                   CYGWIN_VERSION_DLL_MAJOR / 1000,
                   CYGWIN_VERSION_DLL_MAJOR % 1000,
                   CYGWIN_VERSION_DLL_MINOR);
    if (uname(&utsname) == 0)
      fprintf (st, "; %s in use", utsname.release);
#else
    (void)fprintf (st, "Disabled");
#endif

    (void)fprintf (st, "\r\n");
    return 0;
}

t_stat show_version (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
const char *arch = "";
char *whydirty = " ";
int dirty = 0;

if (cptr && (*cptr != 0))
    return SCPE_2MARG;
if (flag) {
        (void)fprintf (st, " %s Simulator:", sim_name);
#if defined(USE_DUMA)
# undef NO_SUPPORT_VERSION
# define NO_SUPPORT_VERSION 1
        nodist++;
#endif /* if defined(USE_DUMA) */
#if defined(NO_SUPPORT_VERSION) ||  \
    defined(WITH_SOCKET_DEV)    ||  \
    defined(WITH_ABSI_DEV)      ||  \
    defined(WITH_MGP_DEV)       ||  \
    defined(TESTING)            ||  \
    defined(ISOLTS)             ||  \
    defined(USE_DUMA)
# if !defined(NO_SUPPORT_VERSION)
#  define NO_SUPPORT_VERSION 1
# endif /* if !defined(NO_SUPPORT_VERSION) */
#endif
#if defined(NO_SUPPORT_VERSION)
        dirty++;
#endif
#if defined(GENERATED_MAKE_VER_H)
# if defined(VER_H_GIT_VERSION)

        /* Dirty if git source is dirty */
        if (strstr(VER_H_GIT_VERSION, "*"))
          {
                dirty++;
          }

        /* Dirty if version contains "X", "D", "A", or "B" */
        if ((strstr(VER_H_GIT_VERSION, "X")) ||  \
            (strstr(VER_H_GIT_VERSION, "D")) ||  \
            (strstr(VER_H_GIT_VERSION, "A")) ||  \
            (strstr(VER_H_GIT_VERSION, "B")))
          {
                dirty++;
          }

        /* Why? */
        if (dirty) //-V547
          {
            if ((strstr(VER_H_GIT_VERSION, "X")))
              {
                    whydirty = " ";
              }
            else if ((strstr(VER_H_GIT_VERSION, "D")))
              {
                    whydirty = " DEV ";
              }
            else if ((strstr(VER_H_GIT_VERSION, "A")))
              {
                    whydirty = " ALPHA ";
              }
            else if ((strstr(VER_H_GIT_VERSION, "B")))
              {
                    whydirty = " BETA ";
              }
          }

#  if defined(VER_H_GIT_PATCH) && defined(VER_H_GIT_PATCH_INT)
#   if defined(VER_H_GIT_HASH)
#    if VER_H_GIT_PATCH_INT < 1
    (void)fprintf (st, "\n   Version: %s (%ld-bit)\n    Commit: %s",
                   VER_H_GIT_VERSION,
                   (long)(CHAR_BIT*sizeof(void *)),
                   VER_H_GIT_HASH);
#    else
#     define NO_SUPPORT_VERSION 1
    (void)fprintf (st, "\n   Version: %s+%s (%ld-bit)\n    Commit: %s",
                   VER_H_GIT_VERSION, VER_H_GIT_PATCH,
                   (long)(CHAR_BIT*sizeof(void *)),
                   VER_H_GIT_HASH);
#    endif
#   else
#    if VER_H_GIT_PATCH_INT < 1
        (void)fprintf (st, "\n   Version: %s (%ld-bit)",
                       VER_H_GIT_VERSION,
                       (long)(CHAR_BIT*sizeof(void *)));
#    else
#     define NO_SUPPORT_VERSION 1
        (void)fprintf (st, "\n   Version: %s+%s (%ld-bit)",
                       VER_H_GIT_VERSION, VER_H_GIT_PATCH,
                       (long)(CHAR_BIT*sizeof(void *)));
#    endif
#   endif
#  else
#   if defined(VER_H_GIT_HASH)
        (void)fprintf (st, "\n   Version: %s (%ld-bit)\n    Commit: %s",
                       VER_H_GIT_VERSION,
                       (long)(CHAR_BIT*sizeof(void *)),
                       VER_H_GIT_HASH);
#   else
        (void)fprintf (st, "\n   Version: %s (%ld-bit)",
                       VER_H_GIT_VERSION,
                       (long)(CHAR_BIT*sizeof(void *)));
#   endif
#  endif
# endif
#endif

/* TESTING */
#if defined(TESTING)
    (void)fprintf (st, "\n   Options: ");
# if !defined(HAVE_DPSOPT)
#  define HAVE_DPSOPT 1
# endif
    (void)fprintf (st, "TESTING");
#endif /* if defined(TESTING) */

/* ISOLTS */
#if defined(ISOLTS)
# if defined(HAVE_DPSOPT)
    (void)fprintf (st, ", ");
# else
    (void)fprintf (st, "\n   Options: ");
# endif
# if !defined(HAVE_DPSOPT)
#  define HAVE_DPSOPT 1
# endif
    (void)fprintf (st, "ISOLTS");
#endif /* if defined(ISOLTS) */

/* NO_UCACHE */
#if defined(NO_UCACHE)
# if defined(HAVE_DPSOPT)
    (void)fprintf (st, ", ");
# else
    (void)fprintf (st, "\n   Options: ");
# endif
# if !defined(HAVE_DPSOPT)
#  define HAVE_DPSOPT 1
# endif
    (void)fprintf (st, "NO_UCACHE");
#endif /* if defined(NO_UCACHE) */

/* NEED_128 */
#if defined(NEED_128)
# if defined(HAVE_DPSOPT)
    (void)fprintf (st, ", ");
# else
    (void)fprintf (st, "\n   Options: ");
# endif
# if !defined(HAVE_DPSOPT)
#  define HAVE_DPSOPT 1
# endif
    (void)fprintf (st, "NEED_128");
#endif /* if defined(NEED_128) */

/* WAM */
#if defined(WAM)
# if defined(HAVE_DPSOPT)
    (void)fprintf (st, ", ");
# else
    (void)fprintf (st, "\n   Options: ");
# endif
# if !defined(HAVE_DPSOPT)
#  define HAVE_DPSOPT 1
# endif
    (void)fprintf (st, "WAM");
#endif /* if defined(WAM) */

/* ROUND_ROBIN */
#if defined(ROUND_ROBIN)
# if defined(HAVE_DPSOPT)
    (void)fprintf (st, ", ");
# else
    (void)fprintf (st, "\n   Options: ");
# endif
# if !defined(HAVE_DPSOPT)
#  define HAVE_DPSOPT 1
# endif
    (void)fprintf (st, "ROUND_ROBIN");
#endif /* if defined(ROUND_ROBIN) */

/* NO_LOCKLESS */
#if !defined(LOCKLESS)
# if defined(HAVE_DPSOPT)
    (void)fprintf (st, ", ");
# else
    (void)fprintf (st, "\n   Options: ");
# endif
# if !defined(HAVE_DPSOPT)
#  define HAVE_DPSOPT 1
# endif
    (void)fprintf (st, "NO_LOCKLESS");
#endif /* if !defined(LOCKLESS) */

/* ABSI */  /* XXX: Change to NO_ABSI once code is non-experimental */
#if defined(WITH_ABSI_DEV)
# if defined(HAVE_DPSOPT)
    (void)fprintf (st, ", ");
# else
    (void)fprintf (st, "\n   Options: ");
# endif
# if !defined(HAVE_DPSOPT)
#  define HAVE_DPSOPT 1
# endif
    (void)fprintf (st, "ABSI");
#endif /* if defined(WITH_ABSI_DEV) */

/* SOCKET */  /* XXX: Change to NO_SOCKET once code is non-experimental */
#if defined(WITH_SOCKET_DEV)
# if defined(HAVE_DPSOPT)
    (void)fprintf (st, ", ");
# else
    (void)fprintf (st, "\n   Options: ");
# endif
# if !defined(HAVE_DPSOPT)
#  define HAVE_DPSOPT 1
# endif
    (void)fprintf (st, "SOCKET");
#endif /* if defined(WITH_SOCKET_DEV) */

/* CHAOSNET */  /* XXX: Change to NO_CHAOSNET once code is non-experimental */
#if defined(WITH_MGP_DEV)
# if defined(HAVE_DPSOPT)
    (void)fprintf (st, ", ");
# else
    (void)fprintf (st, "\n   Options: ");
# endif
# if !defined(HAVE_DPSOPT)
#  define HAVE_DPSOPT 1
# endif
    (void)fprintf (st, "CHAOSNET");
# if USE_SOCKET_DEV_APPROACH
    (void)fprintf (st, "-S");
# endif /* if USE_SOCKET_DEV_APPROACH */
#endif /* if defined(WITH_MGP_DEV) */

/* DUMA */
#if defined(USE_DUMA)
# if defined(HAVE_DPSOPT)
    (void)fprintf (st, ", ");
# else
    (void)fprintf (st, "\n   Options: ");
# endif
# if !defined(HAVE_DPSOPT)
#  define HAVE_DPSOPT 1
# endif
    (void)fprintf (st, "DUMA");
#endif /* if defined(USE_DUMA) */

#if defined(GENERATED_MAKE_VER_H) && defined(VER_H_GIT_DATE)
# if defined(NO_SUPPORT_VERSION)
    (void)fprintf (st, "\n  Modified: %s", VER_H_GIT_DATE);
# else
    (void)fprintf (st, "\n  Released: %s", VER_H_GIT_DATE);
# endif
#endif
#if defined(GENERATED_MAKE_VER_H) && defined(VER_H_GIT_DATE) && defined(VER_H_PREP_DATE)
    (void)fprintf (st, " - Kit Prepared: %s", VER_H_PREP_DATE);
#endif
#if defined(VER_CURRENT_TIME)
    (void)fprintf (st, "\n  Compiled: %s", VER_CURRENT_TIME);
#endif
    if (dirty) //-V547
      {
        (void)fprintf (st, "\r\n\r\n ****** THIS%sBUILD IS NOT SUPPORTED BY THE DPS8M DEVELOPMENT TEAM ******", whydirty);
      }
    (void)fprintf (st, "\r\n\r\n Build Information:");
#if defined(BUILD_PROM_OSV_TEXT) && defined(BUILD_PROM_OSA_TEXT)
    char build_os_version_raw[255];
    char build_os_arch_raw[255];
    (void)sprintf(build_os_version_raw, "%.254s", BUILD_PROM_OSV_TEXT);
    (void)sprintf(build_os_arch_raw, "%.254s", BUILD_PROM_OSA_TEXT);
    char *build_os_version = strdup(build_os_version_raw);
    if (!build_os_version)
      {
        (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                       __func__, __FILE__, __LINE__);
# if defined(USE_BACKTRACE)
#  if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
#  endif /* if defined(SIGUSR2) */
# endif /* if defined(USE_BACKTRACE) */
        abort();
      }
    char *build_os_arch = strdup(build_os_arch_raw);
    if (!build_os_arch)
      {
        (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                       __func__, __FILE__, __LINE__);
# if defined(USE_BACKTRACE)
#  if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
#  endif /* if defined(SIGUSR2) */
# endif /* if defined(USE_BACKTRACE) */
        abort();
      }
    unsigned char SPROM[1024];
    setupPROM (0, SPROM);
    (void)fprintf (st, "\n    Target: ");
    printpq (SPROM, st, 190, 20);
    if (SPROM[170] != 20)
      {
        if (SPROM[170] != 255)
          {
            (void)fprintf (st, " on ");
            printpq (SPROM, st, 170, 20);
          }
      }
    strtrimspace(build_os_version, build_os_version_raw);
    strtrimspace(build_os_arch, build_os_arch_raw);
    (void)fprintf (st, "\n  Build OS: %s %s", build_os_version, build_os_arch);
    FREE(build_os_version);
    FREE(build_os_arch);
#endif
#if defined(__VERSION__)
    char gnumver[2];
    char postver[1024];
    (void)sprintf(gnumver, "%.1s", __VERSION__);
    (void)sprintf(postver, "%.1023s", __VERSION__);
    strremove(postver, "(TM)");
    strremove(postver, "(R)");
    strremove(postver, "git://github.com/OpenIndiana/oi-userland.git ");
    strremove(postver, "https://github.com/OpenIndiana/oi-userland.git ");
    strremove(postver, " gcc 4.9 mode");
    strremove(postver, "4.2.1 Compatible ");
    strremove(postver, "git@github.com:llvm/llvm-project.git ");
    strremove(postver, "https://github.com/llvm/llvm-project.git ");
    strremove(postver, " (https://github.com/yrnkrn/zapcc)");
    strremove(postver, "https://github.com/yrnkrn/zapcc ");
    strremove(postver, "(experimental) ");
    strremove(postver, ".module+el8.7.0+20823+214a699d");
    strremove(postver, "17.1.1 (5725-C72, 5765-J20), version ");
    strremove(postver, "17.1.1 (5725-C72, 5765-J18), version ");
    strremove(postver, "17.1.2 (5725-C72, 5765-J20), version ");
    strremove(postver, "17.1.2 (5725-C72, 5765-J18), version ");
    strremove(postver, "llvmorg-16.0.6-0-");
    strremove(postver, " Clang 15.0.0 (build 760095e)");
    strremove(postver, " Clang 15.0.0 (build 6af5742)");
    strremove(postver, " Clang 15.0.0 (build ca7115e)");
    strremove(postver, " Clang 15.0.0 (build 232543c)");
    strremove(postver, " Clang 17.0.6 (build 19a779f)");
    strremove(postver, "CLANG: ");
#endif
#if ( defined(__GNUC__) && defined(__VERSION__) ) && !defined(__EDG__)
# if !defined(__clang_version__)
    if (isdigit((unsigned char)gnumver[0])) {
        (void)fprintf (st, "\n  Compiler: GCC %s", postver);
    } else {
        (void)fprintf (st, "\n  Compiler: %s", postver);
    }
# endif
# if defined(__clang_analyzer__ )
    (void)fprintf (st, "\n  Compiler: Clang C/C++ Static Analyzer");
# elif defined(__clang_version__) && defined(__VERSION__)
    char clangllvmver[1024];
    (void)sprintf(clangllvmver, "%.1023s", __clang_version__);
    strremove(clangllvmver, "git://github.com/OpenIndiana/oi-userland.git ");
    strremove(clangllvmver, "https://github.com/OpenIndiana/oi-userland.git ");
    strremove(clangllvmver, "https://github.com/llvm/llvm-project.git ");
    strremove(clangllvmver, "c13b7485b87909fcf739f62cfa382b55407433c0");
    strremove(clangllvmver, "e6c3289804a67ea0bb6a86fadbe454dd93b8d855");
    strremove(clangllvmver, "https://github.com/llvm/llvm-project.git");
    strremove(clangllvmver, " ( )");
    strremove(clangllvmver, " ()");
    if (gnumver[0] == 'c' || gnumver[0] == 'C') {
        (void)fprintf (st, "\n  Compiler: Clang %s", clangllvmver);
    } else {
        (void)fprintf (st, "\n  Compiler: %s", postver);
    }
# elif defined(__clang_version__)
    (void)fprintf (st, "\n  Compiler: %s", postver);
# endif
#elif defined(__PGI) && !defined(__NVCOMPILER)
    (void)fprintf (st, "\n  Compiler: Portland Group, Inc. (PGI) C Compiler ");
# if defined(__PGIC__)
    (void)fprintf (st, "%d", __PGIC__);
#  if defined(__PGIC_MINOR__)
    (void)fprintf (st, ".%d", __PGIC_MINOR__);
#   if defined(__PGIC_PATCHLEVEL__)
    (void)fprintf (st, ".%d", __PGIC_PATCHLEVEL__);
#   endif
#  endif
# endif
#elif defined(__NVCOMPILER)
    (void)fprintf (st, "\n  Compiler: NVIDIA HPC SDK C Compiler ");
# if defined(__NVCOMPILER_MAJOR__)
    (void)fprintf (st, "%d", __NVCOMPILER_MAJOR__);
#  if defined(__NVCOMPILER_MINOR__)
    (void)fprintf (st, ".%d", __NVCOMPILER_MINOR__);
#   if defined(__NVCOMPILER_PATCHLEVEL__)
    (void)fprintf (st, ".%d", __NVCOMPILER_PATCHLEVEL__);
#   endif
#  endif
# endif
#elif defined(_MSC_FULL_VER) && defined(_MSC_BUILD)
    (void)fprintf (st, "\n  Compiler: Microsoft C %d.%02d.%05d.%02d",
                   _MSC_FULL_VER/10000000,
                   (_MSC_FULL_VER/100000)%100,
                   _MSC_FULL_VER%100000,
                   _MSC_BUILD);
#elif ( defined(__xlc__) && !defined(__clang_version__) )
# if defined(_AIX) && defined(__PASE__)
    (void)fprintf (st, "\n  Compiler: IBM XL C/C++ V%s (PASE for IBM i)", __xlc__);
# endif
# if defined(_AIX) && !defined(__PASE__)
    (void)fprintf (st, "\n  Compiler: IBM XL C/C++ for AIX V%s", __xlc__);
# endif
# if defined(__linux__) && ( !defined(_AIX) || !defined(__PASE__) )
    (void)fprintf (st, "\n  Compiler: IBM XL C/C++ for Linux V%s", __xlc__);
# endif
# if ( !defined(_AIX) && !defined(__clang_version__) && !defined(__PASE__) && !defined(__linux__) && defined(__xlc__) )
#  if defined(__PPC__) && defined(__APPLE__)
    (void)fprintf (st, "\n  Compiler: IBM XL C/C++ V%s for Mac OS X", __xlc__);
#  else
    (void)fprintf (st, "\n  Compiler: IBM XL C/C++ V%s", __xlc__);
#  endif
# endif
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC) || defined(__SUNPRO_CC_COMPAT)
# define VER_ENC(maj, min, rev) \
  (((maj) * 1000000) + ((min) * 1000) + (rev))
# define VER_DEC_MAJ(ver) \
  ((ver) / 1000000)
# define VER_DEC_MIN(ver) \
  (((ver) % 1000000) / 1000)
# define VER_DEC_REV(ver) \
  ((ver) % 1000)
# if defined(__SUNPRO_C) && (__SUNPRO_C > 0x1000)
#  define COMP_VER VER_ENC(                                        \
   (((__SUNPRO_C >> 16) & 0xf) * 10) + ((__SUNPRO_C >> 12) & 0xf), \
   (((__SUNPRO_C >>  8) & 0xf) * 10) + ((__SUNPRO_C >>  4) & 0xf), \
     (__SUNPRO_C & 0xf) * 10)
# elif defined(__SUNPRO_C)
#  define COMP_VER VER_ENC(    \
     (__SUNPRO_C >>  8) & 0xf, \
     (__SUNPRO_C >>  4) & 0xf, \
     (__SUNPRO_C) & 0xf)
# elif defined(__SUNPRO_CC) && (__SUNPRO_CC > 0x1000)
#  define COMP_VER VER_ENC(                                          \
   (((__SUNPRO_CC >> 16) & 0xf) * 10) + ((__SUNPRO_CC >> 12) & 0xf), \
   (((__SUNPRO_CC >>  8) & 0xf) * 10) + ((__SUNPRO_CC >>  4) & 0xf), \
     (__SUNPRO_CC & 0xf) * 10)
# elif defined(__SUNPRO_CC)
#  define COMP_VER VER_ENC(     \
     (__SUNPRO_CC >>  8) & 0xf, \
     (__SUNPRO_CC >>  4) & 0xf, \
     (__SUNPRO_CC) & 0xf)
# endif
# if !defined(COMP_VER)
#  define COMP_VER 0
# endif
    (void)fprintf (st, "\n  Compiler: Oracle Developer Studio C/C++ %d.%d.%d",
                   VER_DEC_MAJ(COMP_VER),
                   VER_DEC_MIN(COMP_VER),
                   VER_DEC_REV(COMP_VER));
#elif defined(__DMC__)
    (void)fprintf (st, "\n  Compiler: Digital Mars C/C++");
#elif defined(__PCC__)
    (void)fprintf (st, "\n  Compiler: Portable C Compiler");
#elif defined(KENC) || defined(KENCC) || defined(__KENC__) || defined(__KENCC__)
    (void)fprintf (st, "\n  Compiler: Plan 9 Compiler Suite");
#elif defined(__ACK__)
    (void)fprintf (st, "\n  Compiler: Amsterdam Compiler Kit");
#elif defined(__COMO__)
    (void)fprintf (st, "\n  Compiler: Comeau C++");
#elif defined(__COMPCERT__)
    (void)fprintf (st, "\n  Compiler: CompCert C");
#elif defined(__COVERITY__)
    (void)fprintf (st, "\n  Compiler: Coverity C/C++ Static Analyzer");
#elif defined(__LCC__)
    (void)fprintf (st, "\n  Compiler: Local C Compiler (lcc)");
#elif defined(sgi) || defined(__sgi) || defined(_sgi) || defined(_SGI_COMPILER_VERSION)
    (void)fprintf (st, "\n  Compiler: SGI MIPSpro");
#elif defined(__OPEN64__)
    (void)fprintf (st, "\n  Compiler: Open64 %s", __OPEN64__);
#elif defined(__PGI) || defined(__PGIC__)
    (void)fprintf (st, "\n  Compiler: Portland Group/PGI C/C++");
#elif defined(__VBCC__)
    (void)fprintf (st, "\n  Compiler: Volker Barthelmann C Compiler (vbcc)");
#elif defined(__WATCOMC__)
    (void)fprintf (st, "\n  Compiler: Watcom C/C++ %d.%d",
                   __WATCOMC__ / 100,
                   __WATCOMC__ % 100);
#elif defined(__xlC__)
    (void)fprintf (st, "\n  Compiler: IBM XL C/C++");
#elif defined(__INTEL_COMPILER) || defined(__ICC)
# if defined(__INTEL_COMPILER_UPDATE)
#  if defined(__INTEL_COMPILER_BUILD_DATE)
    (void)fprintf (st, "\n  Compiler: Intel C++ Compiler %d.%d (%d)",
                   __INTEL_COMPILER, __INTEL_COMPILER_UPDATE,
                   __INTEL_COMPILER_BUILD_DATE);
#  else
    (void)fprintf (st, "\n  Compiler: Intel C++ Compiler %d.%d",
                   __INTEL_COMPILER, __INTEL_COMPILER_UPDATE);
#  endif
# else
    (void)fprintf (st, "\n  Compiler: Intel C++ Compiler %d",
                   __INTEL_COMPILER);
# endif
#elif defined(SIM_COMPILER)
# define S_xstr(a) S_str(a)
# define S_str(a) #a
    (void)fprintf (st, "\n  Compiler: %s", S_xstr(SIM_COMPILER));
# undef S_str
# undef S_xstr
#else
    (void)fprintf (st, "\n  Compiler: Unknown");
#endif

#if defined(__ppc64__) || defined(__PPC64__) || defined(__ppc64le__) || defined(__PPC64LE__) || defined(__powerpc64__) || \
    defined(__POWERPC64__) || defined(_M_PPC64) || defined(__PPC64) || defined(_ARCH_PPC64)
# define SC_IS_PPC64 1
#else
# define SC_IS_PPC64 0
#endif

#if defined(__ppc__) || defined(__PPC__) || defined(__powerpc__) || defined(__POWERPC__) || defined(_M_PPC) || defined(__PPC) || \
    defined(__ppc32__) || defined(__PPC32__) || defined(__powerpc32__) || defined(__POWERPC32__) || defined(_M_PPC32) || \
    defined(__PPC32)
# define SC_IS_PPC32 1
#else
# define SC_IS_PPC32 0
#endif

#if defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__) || defined(__x86_64__) || defined(__AMD64)
    arch = " x86_64";
#elif defined(_M_IX86) || defined(__i386) || defined(__i486) || defined(__i586) || defined(__i686) || defined(__ix86)
    arch = " x86";
#elif defined(_M_ARM64) || defined(__aarch64__) || defined(__arm64__)
    arch = " arm64";
#elif defined(_M_ARM) || defined(__arm__)
    arch = " arm";
#elif defined(__ia64__) || defined(_M_IA64) || defined(__itanium__)
    arch = " ia64";
#elif SC_IS_PPC64
    arch = " powerpc64";
#elif SC_IS_PPC32
    arch = " powerpc";
#elif defined(__s390x__)
    arch = " s390x";
#elif defined(__s390__)
    arch = " s390";
#elif defined(__J2__) || defined(__J2P__) || defined(__j2__) || defined(__j2p__)
    arch = " j2";
#elif defined(__SH4__) || defined(__sh4__) || defined(__SH4) || defined(__sh4)
    arch = " sh4";
#elif defined(__SH2__) || defined(__sh2__) || defined(__SH2) || defined(__sh2)
    arch = " sh2";
#elif defined(__alpha__)
    arch = " alpha";
#elif defined(__hppa__) || defined(__HPPA__) || defined(__PARISC__) || defined(__parisc__)
    arch = " hppa";
#elif defined(__ICE9__) || defined(__ice9__) || defined(__ICE9) || defined(__ice9)
    arch = " ice9";
#elif defined(mips64) || defined(__mips64__) || defined(MIPS64) || defined(_MIPS64_) || defined(__mips64)
    arch = " mips64";
#elif defined(mips) || defined(__mips__) || defined(MIPS) || defined(_MIPS_) || defined(__mips)
    arch = " mips";
#elif defined(__OpenRISC__) || defined(__OPENRISC__) || defined(__openrisc__) || \
      defined(__OR1K__) || defined(__JOR1K__) || defined(__OPENRISC1K__) || defined(__OPENRISC1200__)
    arch = " openrisc";
#elif defined(__sparc64) || defined(__SPARC64) || defined(__SPARC64__) || defined(__sparc64__)
    arch = " sparc64";
#elif defined(__sparc) || defined(__SPARC) || defined(__SPARC__) || defined(__sparc__)
    arch = " sparc";
#elif defined(__riscv) || defined(__riscv__)
    arch = " riscv";
#elif defined(__myriad2__)
    arch = " myriad2";
#elif defined(__loongarch64) || defined(__loongarch__)
    arch = " loongarch";
#elif defined(_m68851) || defined(__m68k__) || defined(__m68000__) || defined(__M68K)
    arch = " m68k";
#elif defined(__m88k__) || defined(__m88000__) || defined(__M88K)
    arch = " m88k";
#elif defined(__VAX__) || defined(__vax__)
    arch = " vax";
#elif defined(__NIOS2__) || defined(__nios2__)
    arch = " nios2";
#elif defined(__MICROBLAZE__) || defined(__microblaze__)
    arch = " microblaze";
#else
    arch = " ";
#endif
    (void)fprintf (st, "%s", arch);
#if defined(BUILD_BY_USER)
        (void)fprintf (st, "\n  Built by: %s", BUILD_BY_USER);
#else
# if defined(GENERATED_MAKE_VER_H) && defined(VER_H_PREP_USER)
        (void)fprintf (st, "\n  Built by: %s", VER_H_PREP_USER);
# endif
#endif
                (void)fprintf (st, "\n\n Host System Information:");
#if defined(_WIN32)
    if (1) {
        char *arch = getenv ("PROCESSOR_ARCHITECTURE");
        char *proc_arch3264 = getenv ("PROCESSOR_ARCHITEW6432");
        char osversion[PATH_MAX+1] = "";
        FILE *f;

        if ((f = _popen ("ver", "r"))) {
            (void)memset (osversion, 0, sizeof(osversion));
            do {
                if (NULL == fgets (osversion, sizeof(osversion)-1, f))
                    break;
                sim_trim_endspc (osversion);
                } while (osversion[0] == '\0');
            _pclose (f);
            }
        (void)fprintf (st, "\n   Host OS: %s", osversion);
        (void)fprintf (st, " %s%s%s", arch, proc_arch3264 ? " on " : "", proc_arch3264 ? proc_arch3264  : "");
        }
#else
    if (1) {
        char osversion[2*PATH_MAX+1] = "";
        FILE *f;
# if !defined(_AIX)
        if ((f = popen \
             ("uname -mrs 2> /dev/null", "r"))) {
# else
        if ((f = popen \
             ("sh -c 'echo \"$(command -p env uname -v \
               2> /dev/null).$(command -p env uname -r \
               2> /dev/null) $(command -p env uname -p \
               2> /dev/null)\"' 2> /dev/null", "r"))) {
# endif /* if !defined(_AIX) */
            (void)memset (osversion, 0, sizeof(osversion));
            do {
              if (NULL == fgets (osversion, sizeof(osversion)-1, f)) {
                    break;
              }
            sim_trim_endspc (osversion);
            } while (osversion[0] == '\0');
            pclose (f);
            strremove(osversion, "0000000000000000 ");
            strremove(osversion, " 0000000000000000");
            strremove(osversion, "000000000000 ");
            strremove(osversion, " 000000000000");
            strremove(osversion, "IBM ");
            strremove(osversion, " (emulated by qemu)");
            strremove(osversion, " (emulated by QEMU)");
        }
# if !defined(_AIX)
            (void)fprintf (st, "\n   Host OS: %s", osversion);
# else
            strremove(osversion, "AIX ");
#  if !defined(__PASE__)
            (void)fprintf (st, "\n   Host OS: IBM AIX %s", osversion);
#  else
            (void)fprintf (st, "\n   Host OS: IBM OS/400 (PASE) %s", osversion);
#  endif /* if !defined(__PASE__) */
# endif /* if !defined(_AIX) */
    } else {
# if !defined(_AIX)
        (void)fprintf (st, "\n   Host OS: Unknown");
# else
#  if !defined(__PASE__)
        (void)fprintf (st, "\n   Host OS: IBM AIX");
#  else
        (void)fprintf (st, "\n   Host OS: IBM OS/400 (PASE)");
#  endif /* if !defined(__PASE__) */
# endif /* if !defined(_AIX) */
    }
#endif
#if defined(__APPLE__)
    int isRosetta = processIsTranslated();
    if (isRosetta == 1) {
        sim_printf ("\n\n  ****** RUNNING UNDER APPLE ROSETTA 2, EXPECT REDUCED PERFORMANCE ******");
    }
#endif
    if (nodist)
      {
        sim_printf ("\n\n ********* LICENSE RESTRICTED BUILD *** NOT FOR REDISTRIBUTION *********\n");
      }
    else
      {
        (void)fprintf (st, "\n");
        (void)fprintf (st, "\n This software is made available under the terms of the ICU License.");
        (void)fprintf (st, "\n For complete license details, see the LICENSE file included with the");
        (void)fprintf (st, "\n software or https://gitlab.com/dps8m/dps8m/-/blob/master/LICENSE.md");
      }
        (void)fprintf (st, "\n");
    }
return SCPE_OK;
}

t_stat show_config (FILE *st, DEVICE *dnotused, UNIT *unotused, int32 flag, CONST char *cptr)
{
size_t i;
DEVICE *dptr;
t_bool only_enabled = (sim_switches & SWMASK ('E'));

if (cptr && (*cptr != 0))
    return SCPE_2MARG;
(void)fprintf (st, "%s simulator configuration%s\n\n", sim_name, only_enabled ? " (enabled devices)" : "");
for (i = 0; (dptr = sim_devices[i]) != NULL; i++)
    if (!only_enabled || !qdisable (dptr))
        show_device (st, dptr, flag);
return SCPE_OK;
}

t_stat show_log_names (FILE *st, DEVICE *dnotused, UNIT *unotused, int32 flag, CONST char *cptr)
{
int32 i;
DEVICE *dptr;

if (cptr && (*cptr != 0))
    return SCPE_2MARG;
for (i = 0; (dptr = sim_devices[i]) != NULL; i++)
    show_dev_logicals (st, dptr, NULL, 1, cptr);
return SCPE_OK;
}

t_stat show_dev_logicals (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
if (dptr->lname)
    (void)fprintf (st, "%s -> %s\n", dptr->lname, dptr->name);
else if (!flag)
    fputs ("no logical name assigned\n", st);
return SCPE_OK;
}

t_stat show_queue (FILE *st, DEVICE *dnotused, UNIT *unotused, int32 flag, CONST char *cptr)
{
DEVICE *dptr;
UNIT *uptr;
int32 accum;

if (cptr && (*cptr != 0))
    return SCPE_2MARG;
if (sim_clock_queue == QUEUE_LIST_END)
    (void)fprintf (st, "%s event queue empty, time = %.0f, executing %.0f instructions/sec\n",
                   sim_name, sim_time, sim_timer_inst_per_sec ());
else {
    const char *tim;

    (void)fprintf (st, "%s event queue status, time = %.0f, executing %.0f instructions/sec\n",
                   sim_name, sim_time, sim_timer_inst_per_sec ());
    accum = 0;
    for (uptr = sim_clock_queue; uptr != QUEUE_LIST_END; uptr = uptr->next) {
        if (uptr == &sim_step_unit)
            (void)fprintf (st, "  Step timer");
        else
            if (uptr == &sim_expect_unit)
                (void)fprintf (st, "  Expect fired");
            else
                if ((dptr = find_dev_from_unit (uptr)) != NULL) {
                    (void)fprintf (st, "  %s", sim_dname (dptr));
                    if (dptr->numunits > 1)
                        (void)fprintf (st, " unit %d", (int32) (uptr - dptr->units));
                    }
                else
                    (void)fprintf (st, "  Unknown");
        tim = sim_fmt_secs((accum + uptr->time)/sim_timer_inst_per_sec ());
        (void)fprintf (st, " at %d%s%s%s%s\n", accum + uptr->time,
                       (*tim) ? " (" : "", tim, (*tim) ? ")" : "",
                       (uptr->flags & UNIT_IDLE) ? " (Idle capable)" : "");
        accum = accum + uptr->time;
        }
    }
sim_show_clock_queues (st, dnotused, unotused, flag, cptr);
return SCPE_OK;
}

t_stat show_time (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
if (cptr && (*cptr != 0))
    return SCPE_2MARG;
(void)fprintf (st, "Time:\t%.0f\n", sim_gtime());
return SCPE_OK;
}

t_stat show_break (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
t_stat r;

if (cptr && (*cptr != 0))
    r = ssh_break (st, cptr, 1);  /* more? */
else
    r = sim_brk_showall (st, sim_switches);
return r;
}

t_stat show_dev_radix (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
(void)fprintf (st, "Radix=%d\n", dptr->dradix);
return SCPE_OK;
}

t_stat show_dev_debug (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
int32 any = 0;
DEBTAB *dep;

if (dptr->flags & DEV_DEBUG) {
    if (dptr->dctrl == 0)
        fputs ("Debugging disabled", st);
    else if (dptr->debflags == NULL)
        fputs ("Debugging enabled", st);
    else {
        uint32 dctrl = dptr->dctrl;

        fputs ("Debug=", st);
        for (dep = dptr->debflags; (dctrl != 0) && (dep->name != NULL); dep++) {
            if ((dctrl & dep->mask) == dep->mask) {
                dctrl &= ~dep->mask;
                if (any)
                    fputc (';', st);
                fputs (dep->name, st);
                any = 1;
                }
            }
        }
    fputc ('\n', st);
    return SCPE_OK;
    }
else return SCPE_NOFNC;
}

/* Show On actions */

t_stat show_on (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
int32 lvl, i;

if (cptr && (*cptr != 0)) return SCPE_2MARG;            /* now eol? */
for (lvl=sim_do_depth; lvl >= 0; --lvl) {
    if (lvl > 0)
        (void)fprintf(st, "On Processing at Do Nest Level: %d", lvl);
    else
        (void)fprintf(st, "On Processing for input commands");
    (void)fprintf(st, " is %s\n", (sim_on_check[lvl]) ? "enabled" : "disabled");
    for (i=1; i<SCPE_BASE; ++i) {
        if (sim_on_actions[lvl][i])
            (void)fprintf(st, "    on %5d    %s\n", i, sim_on_actions[lvl][i]); }
    for (i=SCPE_BASE; i<=SCPE_MAX_ERR; ++i) {
        if (sim_on_actions[lvl][i])
            (void)fprintf(st, "    on %-5s    %s\n", scp_errors[i-SCPE_BASE].code, sim_on_actions[lvl][i]); }
    if (sim_on_actions[lvl][0])
        (void)fprintf(st, "    on ERROR    %s\n", sim_on_actions[lvl][0]);
    (void)fprintf(st, "\n");
    }
if (sim_on_inherit)
    (void)fprintf(st, "on state and actions are inherited by nested do commands and subroutines\n");
return SCPE_OK;
}

/* Show modifiers */

t_stat show_mod_names (FILE *st, DEVICE *dnotused, UNIT *unotused, int32 flag, CONST char *cptr)
{
int32 i;
DEVICE *dptr;

if (cptr && (*cptr != 0))                               /* now eol? */
    return SCPE_2MARG;
for (i = 0; (dptr = sim_devices[i]) != NULL; i++)
    show_dev_modifiers (st, dptr, NULL, flag, cptr);
for (i = 0; sim_internal_device_count && (dptr = sim_internal_devices[i]); ++i)
    show_dev_modifiers (st, dptr, NULL, flag, cptr);
return SCPE_OK;
}

t_stat show_dev_modifiers (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
fprint_set_help (st, dptr);
return SCPE_OK;
}

t_stat show_all_mods (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, int32 *toks)
{
MTAB *mptr;
t_stat r = SCPE_OK;

if (dptr->modifiers == NULL)
    return SCPE_OK;
for (mptr = dptr->modifiers; mptr->mask != 0; mptr++) {
    if (mptr->pstring &&
        ((mptr->mask & MTAB_XTD)?
            (MODMASK(mptr,flag) && !MODMASK(mptr,MTAB_NMO)):
            ((MTAB_VUN == (uint32)flag) && ((uptr->flags & mptr->mask) == mptr->match)))) {
        if (*toks > 0) {
            (void)fprintf (st, "\n");
            *toks = 0;
            }
        if (r == SCPE_OK)
            fprint_sep (st, toks);
        r = show_one_mod (st, dptr, uptr, mptr, NULL, 0);
        }
    }
return SCPE_OK;
}

t_stat show_one_mod (FILE *st, DEVICE *dptr, UNIT *uptr, MTAB *mptr,
    CONST char *cptr, int32 flag)
{
t_stat r = SCPE_OK;

if (mptr->disp)
    r = mptr->disp (st, uptr, mptr->match, (CONST void *)(cptr? cptr: mptr->desc));
else
    fputs (mptr->pstring, st);
if ((r == SCPE_OK) && (flag && !((mptr->mask & MTAB_XTD) && MODMASK(mptr,MTAB_NMO))))
    fputc ('\n', st);
return r;
}

/* Show show commands */

t_stat show_show_commands (FILE *st, DEVICE *dnotused, UNIT *unotused, int32 flag, CONST char *cptr)
{
int32 i;
DEVICE *dptr;

if (cptr && (*cptr != 0))                               /* now eol? */
    return SCPE_2MARG;
for (i = 0; (dptr = sim_devices[i]) != NULL; i++)
    show_dev_show_commands (st, dptr, NULL, flag, cptr);
for (i = 0; sim_internal_device_count && (dptr = sim_internal_devices[i]); ++i)
    show_dev_show_commands (st, dptr, NULL, flag, cptr);
return SCPE_OK;
}

t_stat show_dev_show_commands (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
fprint_show_help (st, dptr);
return SCPE_OK;
}

/* Breakpoint commands */

t_stat brk_cmd (int32 flg, CONST char *cptr)
{
GET_SWITCHES (cptr);                                    /* get switches */
return ssh_break (NULL, cptr, flg);                     /* call common code */
}

t_stat ssh_break (FILE *st, const char *cptr, int32 flg)
{
char gbuf[CBUFSIZE], *aptr, abuf[4*CBUFSIZE];
CONST char *tptr, *t1ptr;
DEVICE *dptr = sim_dflt_dev;
UNIT *uptr;
t_stat r;
t_addr lo, hi, max;
int32 cnt;

if (sim_brk_types == 0)
    return sim_messagef (SCPE_NOFNC, "No breakpoint support in this simulator\n");
if (dptr == NULL)
    return SCPE_IERR;
uptr = dptr->units;
if (uptr == NULL)
    return SCPE_IERR;
max = uptr->capac - 1;
abuf[sizeof(abuf)-1] = '\0';
strncpy (abuf, cptr, sizeof(abuf)-1);
cptr = abuf;
if ((aptr = strchr (abuf, ';'))) {                      /* ;action? */
    if (flg != SSH_ST)                                  /* only on SET */
        return sim_messagef (SCPE_ARG, "Invalid argument: %s\n", aptr);
    *aptr++ = 0;                                        /* separate strings */
    }
if (*cptr == 0) {                                       /* no argument? */
    lo = (t_addr) get_rval (sim_PC, 0);                 /* use PC */
    return ssh_break_one (st, flg, lo, 0, aptr);
    }
while (*cptr) {
    cptr = get_glyph (cptr, gbuf, ',');
    tptr = get_range (dptr, gbuf, &lo, &hi, dptr->aradix, max, 0);
    if (tptr == NULL)
        return sim_messagef (SCPE_ARG, "Invalid address specifier: %s\n", gbuf);
    if (*tptr == '[') {
        cnt = (int32) strtotv (tptr + 1, &t1ptr, 10);
        if ((tptr == t1ptr) || (*t1ptr != ']') || (flg != SSH_ST))
            return sim_messagef (SCPE_ARG, "Invalid repeat count specifier: %s\n", tptr + 1);
        tptr = t1ptr + 1;
        }
    else cnt = 0;
    if (*tptr != 0)
        return sim_messagef (SCPE_ARG, "Unexpected argument: %s\n", tptr);
    if ((lo == 0) && (hi == max)) {
        if (flg == SSH_CL)
            sim_brk_clrall (sim_switches);
        else
            if (flg == SSH_SH)
                sim_brk_showall (st, sim_switches);
            else
                return SCPE_ARG;
        }
    else {
        for ( ; lo <= hi; lo = lo + 1) {
            r = ssh_break_one (st, flg, lo, cnt, aptr);
            if (r != SCPE_OK)
                return r;
            }
        }
    }
return SCPE_OK;
}

t_stat ssh_break_one (FILE *st, int32 flg, t_addr lo, int32 cnt, CONST char *aptr)
{
if (!sim_brk_types)
    return sim_messagef (SCPE_NOFNC, "No breakpoint support in this simulator\n");
switch (flg) {
    case SSH_ST:
        return sim_brk_set (lo, sim_switches, cnt, aptr);
        /*NOTREACHED*/ /* unreachable */
        break;

    case SSH_CL:
        return sim_brk_clr (lo, sim_switches);
        /*NOTREACHED*/ /* unreachable */
        break;

    case SSH_SH:
        return sim_brk_show (st, lo, sim_switches);
        /*NOTREACHED*/ /* unreachable */
        break;

    default:
        return SCPE_ARG;
    }
}

/* Reset command and routines */

static t_bool run_cmd_did_reset = FALSE;

t_stat reset_cmd (int32 flag, CONST char *cptr)
{
char gbuf[CBUFSIZE];
DEVICE *dptr;

GET_SWITCHES (cptr);                                    /* get switches */
run_cmd_did_reset = FALSE;
if (*cptr == 0)                                         /* reset(cr) */
    return (reset_all (0));
cptr = get_glyph (cptr, gbuf, 0);                       /* get next glyph */
if (*cptr != 0)                                         /* now eol? */
    return SCPE_2MARG;
if (strcmp (gbuf, "ALL") == 0)
    return (reset_all (0));
dptr = find_dev (gbuf);                                 /* locate device */
if (dptr == NULL)                                       /* found it? */
    return SCPE_NXDEV;
if (dptr->reset != NULL)
    return dptr->reset (dptr);
else return SCPE_OK;
}

/* Reset devices start..end

   Inputs:
        start   =       number of starting device
   Outputs:
        status  =       error status
*/

t_stat reset_all (uint32 start)
{
DEVICE *dptr;
uint32 i;
t_stat reason;

for (i = 0; i < start; i++) {
    if (sim_devices[i] == NULL)
        return SCPE_IERR;
    }
for (i = start; (dptr = sim_devices[i]) != NULL; i++) {
    if (dptr->reset != NULL) {
        reason = dptr->reset (dptr);
        if (reason != SCPE_OK)
            return reason;
        }
    }
for (i = 0; sim_internal_device_count && (dptr = sim_internal_devices[i]); ++i) {
    if (dptr->reset != NULL) {
        reason = dptr->reset (dptr);
        if (reason != SCPE_OK)
            return reason;
        }
    }
return SCPE_OK;
}

/* Reset to powerup state

   Inputs:
        start   =       number of starting device
   Outputs:
        status  =       error status
*/

t_stat reset_all_p (uint32 start)
{
t_stat r;
int32 old_sw = sim_switches;

sim_switches = SWMASK ('P');
r = reset_all (start);
sim_switches = old_sw;
return r;
}

/* Attach command */

t_stat attach_cmd (int32 flag, CONST char *cptr)
{
char gbuf[4*CBUFSIZE];
DEVICE *dptr;
UNIT *uptr;
t_stat r;

GET_SWITCHES (cptr);                                    /* get switches */
if ((NULL == cptr) || (*cptr == 0)) //-V560             /* must be more */
    return SCPE_2FARG;
cptr = get_glyph (cptr, gbuf, 0);                       /* get next glyph */
GET_SWITCHES (cptr);                                    /* get switches */
if ((NULL == cptr) || (*cptr == 0)) //-V560             /* now eol? */
    return SCPE_2FARG;
dptr = find_unit (gbuf, &uptr);                         /* locate unit */
if (dptr == NULL)                                       /* found dev? */
    return SCPE_NXDEV;
if (uptr == NULL)                                       /* valid unit? */
    return SCPE_NXUN;
if (uptr->flags & UNIT_ATT) {                           /* already attached? */
    if (!(uptr->dynflags & UNIT_ATTMULT) &&             /* and only single attachable */
        !(dptr->flags & DEV_DONTAUTO)) {                /* and auto detachable */
        r = scp_detach_unit (dptr, uptr);               /* detach it */
        if (r != SCPE_OK)                               /* error? */
            return r; }
    else {
        if (!(uptr->dynflags & UNIT_ATTMULT))
            return SCPE_ALATT;                          /* Already attached */
        }
    }
gbuf[sizeof(gbuf)-1] = '\0';
strncpy (gbuf, cptr, sizeof(gbuf)-1);
sim_trim_endspc (gbuf);                                 /* trim trailing spc */
return scp_attach_unit (dptr, uptr, gbuf);              /* attach */
}

/* Call device-specific or file-oriented attach unit routine */

t_stat scp_attach_unit (DEVICE *dptr, UNIT *uptr, const char *cptr)
{
if (dptr->attach != NULL)                               /* device routine? */
    return dptr->attach (uptr, (CONST char *)cptr);     /* call it */
return attach_unit (uptr, (CONST char *)cptr);          /* no, std routine */
}

/* Attach unit to file */

t_stat attach_unit (UNIT *uptr, CONST char *cptr)
{
DEVICE *dptr;

if (uptr->flags & UNIT_DIS)                             /* disabled? */
    return SCPE_UDIS;
if (!(uptr->flags & UNIT_ATTABLE))                      /* not attachable? */
    return SCPE_NOATT;
if ((dptr = find_dev_from_unit (uptr)) == NULL)
    return SCPE_NOATT;
uptr->filename = (char *) calloc (CBUFSIZE, sizeof (char)); /* alloc name buf */
if (uptr->filename == NULL)
    return SCPE_MEM;
strncpy (uptr->filename, cptr, CBUFSIZE-1);             /* save name */
if ((sim_switches & SWMASK ('R')) ||                    /* read only? */
    ((uptr->flags & UNIT_RO) != 0)) {
    if (((uptr->flags & UNIT_ROABLE) == 0) &&           /* allowed? */
        ((uptr->flags & UNIT_RO) == 0))
        return attach_err (uptr, SCPE_NORO);            /* no, error */
    uptr->fileref = sim_fopen (cptr, "rb");             /* open rd only */
    if (uptr->fileref == NULL)                          /* open fail? */
        return attach_err (uptr, SCPE_OPENERR);         /* yes, error */
    uptr->flags = uptr->flags | UNIT_RO;                /* set rd only */
    if (!sim_quiet && !(sim_switches & SWMASK ('Q'))) {
        sim_printf ("%s: unit is read only (%s)\n", sim_dname (dptr), cptr);
        }
    }
else {
    if (sim_switches & SWMASK ('N')) {                  /* new file only? */
        uptr->fileref = sim_fopen (cptr, "wb+");        /* open new file */
        if (uptr->fileref == NULL)                      /* open fail? */
            return attach_err (uptr, SCPE_OPENERR);     /* yes, error */
        if (!sim_quiet && !(sim_switches & SWMASK ('Q'))) {
            sim_printf ("%s: creating new file (%s)\n", sim_dname (dptr), cptr);
            }
        }
    else {                                              /* normal */
        uptr->fileref = sim_fopen (cptr, "rb+");        /* open r/w */
        if (uptr->fileref == NULL) {                    /* open fail? */
#if defined(EWOULDBLOCK)
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
#else
            if ((errno == EAGAIN))
#endif
                return attach_err (uptr, SCPE_OPENERR); /* yes, error */

#if defined(EPERM)
            if ((errno == EROFS) || (errno == EACCES) || (errno == EPERM)) {/* read only? */
#else
            if ((errno == EROFS) || (errno == EACCES)) {/* read only? */
#endif
                if ((uptr->flags & UNIT_ROABLE) == 0)   /* allowed? */
                    return attach_err (uptr, SCPE_NORO);/* no error */
                uptr->fileref = sim_fopen (cptr, "rb"); /* open rd only */
                if (uptr->fileref == NULL)              /* open fail? */
                    return attach_err (uptr, SCPE_OPENERR); /* yes, error */
                uptr->flags = uptr->flags | UNIT_RO;    /* set rd only */
                if (!sim_quiet) {
                    sim_printf ("%s: unit is read only (%s)\n", sim_dname (dptr), cptr);
                    }
                }
            else {                                      /* doesn't exist */
                if (sim_switches & SWMASK ('E'))        /* must exist? */
                    return attach_err (uptr, SCPE_OPENERR); /* yes, error */
                uptr->fileref = sim_fopen (cptr, "wb+");/* open new file */
                if (uptr->fileref == NULL)              /* open fail? */
                    return attach_err (uptr, SCPE_OPENERR); /* yes, error */
                if (!sim_quiet) {
                    sim_printf ("%s: creating new file (%s)\n", sim_dname (dptr), cptr);
                    }
                }
            }                                           /* end if null */
        }                                               /* end else */
    }
if (uptr->flags & UNIT_BUFABLE) {                       /* buffer? */
    uint32 cap = ((uint32) uptr->capac) / dptr->aincr;  /* effective size */
    if (uptr->flags & UNIT_MUSTBUF)                     /* dyn alloc? */
        uptr->filebuf = calloc (cap, SZ_D (dptr));      /* allocate */
    if (uptr->filebuf == NULL)                          /* no buffer? */
        return attach_err (uptr, SCPE_MEM);             /* error */
    if (!sim_quiet) {
        sim_printf ("%s: buffering file in memory\n", sim_dname (dptr));
        }
    uptr->hwmark = (uint32)sim_fread (uptr->filebuf,    /* read file */
        SZ_D (dptr), cap, uptr->fileref);
    uptr->flags = uptr->flags | UNIT_BUF;               /* set buffered */
    }
uptr->flags = uptr->flags | UNIT_ATT;
uptr->pos = 0;
return SCPE_OK;
}

t_stat attach_err (UNIT *uptr, t_stat stat)
{
FREE (uptr->filename);
uptr->filename = NULL;
return stat;
}

/* Detach command */

t_stat detach_cmd (int32 flag, CONST char *cptr)
{
char gbuf[CBUFSIZE];
DEVICE *dptr;
UNIT *uptr;

GET_SWITCHES (cptr);                                    /* get switches */
if ((NULL == cptr) || (*cptr == 0)) //-V560             /* must be more */
    return SCPE_2FARG;
cptr = get_glyph (cptr, gbuf, 0);                       /* get next glyph */
if (*cptr != 0)                                         /* now eol? */
    return SCPE_2MARG;
if (strcmp (gbuf, "ALL") == 0)
    return (detach_all (0, FALSE));
dptr = find_unit (gbuf, &uptr);                         /* locate unit */
if (dptr == NULL)                                       /* found dev? */
    return SCPE_NXDEV;
if (uptr == NULL)                                       /* valid unit? */
    return SCPE_NXUN;
return scp_detach_unit (dptr, uptr);                    /* detach */
}

/* Detach devices start..end

   Inputs:
        start   =       number of starting device
        shutdown =      TRUE if simulator shutting down
   Outputs:
        status  =       error status

   Note that during shutdown, detach routines for non-attachable devices
   will be called.  These routines can implement simulator shutdown.  Error
   returns during shutdown are ignored.
*/

t_stat detach_all (int32 start, t_bool shutdown)
{
uint32 i, j;
DEVICE *dptr;
UNIT *uptr;
t_stat r;

if ((start < 0) || (start > 1))
    return SCPE_IERR;
if (shutdown)
    sim_switches = sim_switches | SIM_SW_SHUT;          /* flag shutdown */
for (i = start; (dptr = sim_devices[i]) != NULL; i++) { /* loop thru dev */
    for (j = 0; j < dptr->numunits; j++) {              /* loop thru units */
        uptr = (dptr->units) + j;
        if ((uptr->flags & UNIT_ATT) ||                 /* attached? */
            (shutdown && dptr->detach &&                /* shutdown, spec rtn, */
            !(uptr->flags & UNIT_ATTABLE))) {           /* !attachable? */
            r = scp_detach_unit (dptr, uptr);           /* detach unit */

            if ((r != SCPE_OK) && !shutdown)            /* error and not shutting down? */
                return r;                               /* bail out now with error status */
            }
        }
    }
return SCPE_OK;
}

/* Call device-specific or file-oriented detach unit routine */

t_stat scp_detach_unit (DEVICE *dptr, UNIT *uptr)
{
if (dptr->detach != NULL)                               /* device routine? */
    return dptr->detach (uptr);
return detach_unit (uptr);                              /* no, standard */
}

/* Detach unit from file */

t_stat detach_unit (UNIT *uptr)
{
DEVICE *dptr;

if (uptr == NULL)
    return SCPE_IERR;
if (!(uptr->flags & UNIT_ATTABLE))                      /* attachable? */
    return SCPE_NOATT;
if (!(uptr->flags & UNIT_ATT)) {                        /* not attached? */
    if (sim_switches & SIM_SW_REST)                     /* restoring? */
        return SCPE_OK;                                 /* allow detach */
    else
        return SCPE_NOTATT;                             /* complain */
    }
if ((dptr = find_dev_from_unit (uptr)) == NULL)
    return SCPE_OK;
if (uptr->flags & UNIT_BUF) {
    uint32 cap = (uptr->hwmark + dptr->aincr - 1) / dptr->aincr;
    if (uptr->hwmark && ((uptr->flags & UNIT_RO) == 0)) {
        if (!sim_quiet) {
            sim_printf ("%s: writing buffer to file\n", sim_dname (dptr));
            }
        rewind (uptr->fileref);
        sim_fwrite (uptr->filebuf, SZ_D (dptr), cap, uptr->fileref);
        if (ferror (uptr->fileref))
            sim_printf ("%s: I/O error - %s (Error %d)",
                        sim_dname (dptr), xstrerror_l(errno), errno);
        }
    if (uptr->flags & UNIT_MUSTBUF) {                   /* dyn alloc? */
        FREE (uptr->filebuf);                           /* free buf */
        uptr->filebuf = NULL;
        }
    uptr->flags = uptr->flags & ~UNIT_BUF;
    }
uptr->flags = uptr->flags & ~(UNIT_ATT | UNIT_RO);
FREE (uptr->filename);
uptr->filename = NULL;
if (fclose (uptr->fileref) == EOF)
    return SCPE_IOERR;
return SCPE_OK;
}

/* Get device display name */

const char *sim_dname (DEVICE *dptr)
{
return (dptr ? (dptr->lname? dptr->lname: dptr->name) : "");
}

/* Get unit display name */

const char *sim_uname (UNIT *uptr)
{
DEVICE *d = find_dev_from_unit(uptr);
static char uname[CBUFSIZE];

if (!d)
    return "";
if (d->numunits == 1)
    return sim_dname (d);
(void)sprintf (uname, "%s%d", sim_dname (d), (int)(uptr-d->units));
return uname;
}

/* Run, go, boot, cont, step, next commands */

t_stat run_cmd (int32 flag, CONST char *cptr)
{
char gbuf[CBUFSIZE] = "";
CONST char *tptr;
uint32 i, j;
int32 sim_next = 0;
int32 unitno;
t_value pcv, orig_pcv;
t_stat r;
DEVICE *dptr;
UNIT *uptr;

GET_SWITCHES (cptr);                                    /* get switches */
sim_step = 0;
if ((flag == RU_RUN) || (flag == RU_GO)) {              /* run or go */
    orig_pcv = get_rval (sim_PC, 0);                    /* get current PC value */
    if (*cptr != 0) {                                   /* argument? */
        cptr = get_glyph (cptr, gbuf, 0);               /* get next glyph */
        if (MATCH_CMD (gbuf, "UNTIL") != 0) {
            if (sim_dflt_dev && sim_vm_parse_addr)      /* address parser? */
                pcv = sim_vm_parse_addr (sim_dflt_dev, gbuf, &tptr);
            else pcv = strtotv (gbuf, &tptr, sim_PC->radix);/* parse PC */
            if ((tptr == gbuf) || (*tptr != 0) ||       /* error? */
                (pcv > width_mask[sim_PC->width]))
                return SCPE_ARG;
            put_rval (sim_PC, 0, pcv);                  /* Save in PC */
            }
        }
    if ((flag == RU_RUN) &&                             /* run? */
        ((r = sim_run_boot_prep (flag)) != SCPE_OK)) {  /* reset sim */
        put_rval (sim_PC, 0, orig_pcv);                 /* restore original PC */
        return r;
        }
    if ((*cptr) || (MATCH_CMD (gbuf, "UNTIL") == 0)) { //-V600 /* should be end */
        int32 saved_switches = sim_switches;

        if (MATCH_CMD (gbuf, "UNTIL") != 0)
            cptr = get_glyph (cptr, gbuf, 0);           /* get next glyph */
        if (MATCH_CMD (gbuf, "UNTIL") != 0)
            return sim_messagef (SCPE_2MARG, "Unexpected %s command argument: %s %s\n",
                                             (flag == RU_RUN) ? "RUN" : "GO", gbuf, cptr);
        sim_switches = 0;
        GET_SWITCHES (cptr);
        if ((*cptr == '\'') || (*cptr == '"')) {        /* Expect UNTIL condition */
            r = expect_cmd (1, cptr);
            if (r != SCPE_OK)
                return r;
            }
        else {                                          /* BREAK UNTIL condition */
            if (sim_switches == 0)
                sim_switches = sim_brk_dflt;
            sim_switches |= BRK_TYP_TEMP;               /* make this a one-shot breakpoint */
            sim_brk_types |= BRK_TYP_TEMP;
            r = ssh_break (NULL, cptr, SSH_ST);
            if (r != SCPE_OK)
                return sim_messagef (r, "Unable to establish breakpoint at: %s\n", cptr);
            }
        sim_switches = saved_switches;
        }
    }

else if ((flag == RU_STEP) ||
         ((flag == RU_NEXT) && !sim_vm_is_subroutine_call)) { /* step */
    static t_bool not_implemented_message = FALSE;

    if ((!not_implemented_message) && (flag == RU_NEXT)) {
        not_implemented_message = TRUE;
        flag = RU_STEP;
        }
    if (*cptr != 0) {                                   /* argument? */
        cptr = get_glyph (cptr, gbuf, 0);               /* get next glyph */
        if (*cptr != 0)                                 /* should be end */
            return SCPE_2MARG;
        sim_step = (int32) get_uint (gbuf, 10, INT_MAX, &r);
        if ((r != SCPE_OK) || (sim_step <= 0))          /* error? */
            return SCPE_ARG;
        }
    else sim_step = 1;
    if ((flag == RU_STEP) && (sim_switches & SWMASK ('T')))
        sim_step = (int32)((sim_timer_inst_per_sec ()*sim_step)/1000000.0);
    }
else if (flag == RU_NEXT) {                             /* next */
    t_addr *addrs;

    if (*cptr != 0) {                                   /* argument? */
        cptr = get_glyph (cptr, gbuf, 0);               /* get next glyph */
        if (*cptr != 0)                                 /* should be end */
            return SCPE_2MARG;
        sim_next = (int32) get_uint (gbuf, 10, INT_MAX, &r);
        if ((r != SCPE_OK) || (sim_next <= 0))          /* error? */
            return SCPE_ARG;
        }
    else sim_next = 1;
    if (sim_vm_is_subroutine_call(&addrs)) {
        sim_brk_types |= BRK_TYP_DYN_STEPOVER;
        for (i=0; addrs[i]; i++)
            sim_brk_set (addrs[i], BRK_TYP_DYN_STEPOVER, 0, NULL);
        }
    else
        sim_step = 1;
    }
else if (flag == RU_BOOT) {                             /* boot */
    if (*cptr == 0)                                     /* must be more */
        return SCPE_2FARG;
    cptr = get_glyph (cptr, gbuf, 0);                   /* get next glyph */
    if (*cptr != 0)                                     /* should be end */
        return SCPE_2MARG;
    dptr = find_unit (gbuf, &uptr);                     /* locate unit */
    if (dptr == NULL)                                   /* found dev? */
        return SCPE_NXDEV;
    if (uptr == NULL)                                   /* valid unit? */
        return SCPE_NXUN;
    if (dptr->boot == NULL)                             /* can it boot? */
        return SCPE_NOFNC;
    if (uptr->flags & UNIT_DIS)                         /* disabled? */
        return SCPE_UDIS;
    if ((uptr->flags & UNIT_ATTABLE) &&                 /* if attable, att? */
        !(uptr->flags & UNIT_ATT))
        return SCPE_UNATT;
    unitno = (int32) (uptr - dptr->units);              /* recover unit# */
    if ((r = sim_run_boot_prep (flag)) != SCPE_OK)      /* reset sim */
        return r;
    if ((r = dptr->boot (unitno, dptr)) != SCPE_OK)     /* boot device */
        return r;
    }

else
    if (flag != RU_CONT)                                /* must be cont */
        return SCPE_IERR;
    else                                                /* CONTINUE command */
        if (*cptr != 0)                                 /* should be end (no arguments allowed) */
            return sim_messagef (SCPE_2MARG, "CONTINUE command takes no arguments\n");

if (sim_switches & SIM_SW_HIDE)                         /* Setup only for Remote Console Mode */
    return SCPE_OK;

for (i = 1; (dptr = sim_devices[i]) != NULL; i++) {     /* reposition all */
    for (j = 0; j < dptr->numunits; j++) {              /* seq devices */
        uptr = dptr->units + j;
        if ((uptr->flags & (UNIT_ATT + UNIT_SEQ)) == (UNIT_ATT + UNIT_SEQ))
            sim_fseek (uptr->fileref, uptr->pos, SEEK_SET);
        }
    }
stop_cpu = 0;
sim_is_running = 1;                                     /* flag running */
if (sim_ttrun () != SCPE_OK) {                          /* set console mode */
    sim_is_running = 0;                                 /* flag idle */
    sim_ttcmd ();
    return SCPE_TTYERR;
    }
if ((r = sim_check_console (30)) != SCPE_OK) {          /* check console, error? */
    sim_is_running = 0;                                 /* flag idle */
    sim_ttcmd ();
    return r;
    }
#if !defined(IS_WINDOWS)
# if defined(SIGINT)
if (signal (SIGINT, int_handler) == SIG_ERR) {          /* set WRU */
    sim_is_running = 0;                                 /* flag idle */
    sim_ttcmd ();
    return SCPE_SIGERR;
    }
# endif
#endif
#if !defined(IS_WINDOWS)
# if defined(SIGHUP)
if (signal (SIGHUP, int_handler) == SIG_ERR) {          /* set WRU */
    sim_is_running = 0;                                 /* flag idle */
    sim_ttcmd ();
    return SCPE_SIGERR;
    }
# endif
#endif
#if !defined(IS_WINDOWS)
# if defined(SIGTERM)
if (signal (SIGTERM, int_handler) == SIG_ERR) {         /* set WRU */
    sim_is_running = 0;                                 /* flag idle */
    sim_ttcmd ();
    return SCPE_SIGERR;
    }
# endif
#endif
if (sim_step)                                           /* set step timer */
    sim_activate (&sim_step_unit, sim_step);
(void)fflush(stdout);                                   /* flush stdout */
if (sim_log)                                            /* flush log if enabled */
    (void)fflush (sim_log);
sim_rtcn_init_all ();                                   /* re-init clocks */
sim_start_timer_services ();                            /* enable wall clock timing */

do {
    t_addr *addrs;

    while (1) {
        r = sim_instr();
        if (r != SCPE_REMOTE)
            break;
        sim_remote_process_command ();                  /* Process the command and resume processing */
        }
    if ((flag != RU_NEXT) ||                            /* done if not doing NEXT */
        (--sim_next <=0))
        break;
    if (sim_step == 0) {                                /* doing a NEXT? */
        t_addr val;
        BRKTAB *bp;

        if (SCPE_BARE_STATUS(r) >= SCPE_BASE)           /* done if an error occurred */
            break;
        if (sim_vm_pc_value)                            /* done if didn't stop at a dynamic breakpoint */
            val = (t_addr)(*sim_vm_pc_value)();
        else
            val = (t_addr)get_rval (sim_PC, 0);
        if ((!(bp = sim_brk_fnd (val))) || (!(bp->typ & BRK_TYP_DYN_STEPOVER)))
            break;
        sim_brk_clrall (BRK_TYP_DYN_STEPOVER);          /* cancel any step/over subroutine breakpoints */
        }
    else {
        if (r != SCPE_STEP)                             /* done if step didn't complete with step expired */
            break;
        }
    /* setup another next/step */
    sim_step = 0;
    if (sim_vm_is_subroutine_call(&addrs)) {
        sim_brk_types |= BRK_TYP_DYN_STEPOVER;
        for (i=0; addrs[i]; i++)
            sim_brk_set (addrs[i], BRK_TYP_DYN_STEPOVER, 0, NULL);
        }
    else
        sim_step = 1;
    if (sim_step)                                       /* set step timer */
        sim_activate (&sim_step_unit, sim_step);
    } while (1);

sim_is_running = 0;                                     /* flag idle */
sim_stop_timer_services ();                             /* disable wall clock timing */
sim_ttcmd ();                                           /* restore console */
sim_brk_clrall (BRK_TYP_DYN_STEPOVER);                  /* cancel any step/over subroutine breakpoints */
signal (SIGINT, SIG_DFL);                               /* cancel WRU */
#if defined(SIGHUP)
signal (SIGHUP, SIG_DFL);                               /* cancel WRU */
#endif
signal (SIGTERM, SIG_DFL);                              /* cancel WRU */
if (sim_log)                                            /* flush console log */
    (void)fflush (sim_log);
if (sim_deb)                                            /* flush debug log */
    sim_debug_flush ();
for (i = 1; (dptr = sim_devices[i]) != NULL; i++) {     /* flush attached files */
    for (j = 0; j < dptr->numunits; j++) {              /* if not buffered in mem */
        uptr = dptr->units + j;
        if (uptr->flags & UNIT_ATT) {                   /* attached, */
            if (uptr->io_flush)                         /* unit specific flush routine */
                uptr->io_flush (uptr);                  /* call it */
            else {
                if (!(uptr->flags & UNIT_BUF) &&        /* not buffered, */
                    (uptr->fileref) &&                  /* real file, */
                    !(uptr->dynflags & UNIT_NO_FIO) &&  /* is FILE *, */
                    !(uptr->flags & UNIT_RO))           /* not read only? */
                    (void)fflush (uptr->fileref);
                }
            }
        }
    }
sim_cancel (&sim_step_unit);                            /* cancel step timer */
UPDATE_SIM_TIME;                                        /* update sim time */
return r | ((sim_switches & SWMASK ('Q')) ? SCPE_NOMESSAGE : 0);
}

/* run command message handler */

void
run_cmd_message (const char *unechoed_cmdline, t_stat r)
{
if (unechoed_cmdline && (r >= SCPE_BASE) && (r != SCPE_STEP) && (r != SCPE_STOP) && (r != SCPE_EXPECT))
    sim_printf("%s> %s\n", do_position(), unechoed_cmdline);
#if defined(WIN_STDIO)
(void)fflush(stderr);
(void)fflush(stdout);
#endif /* if defined(WIN_STDIO) */
fprint_stopped (stdout, r);                         /* print msg */
if (sim_log && (sim_log != stdout))                 /* log if enabled */
    fprint_stopped (sim_log, r);
if (sim_deb && (sim_deb != stdout) && (sim_deb != sim_log))/* debug if enabled */
    fprint_stopped (sim_deb, r);
#if defined(WIN_STDIO)
(void)fflush(stderr);
(void)fflush(stdout);
#endif /* if defined(WIN_STDIO) */
}

/* Common setup for RUN or BOOT */

t_stat sim_run_boot_prep (int32 flag)
{
UNIT *uptr;
t_stat r;

sim_interval = 0;                                       /* reset queue */
sim_time = sim_rtime = 0;
noqueue_time = 0;
for (uptr = sim_clock_queue; uptr != QUEUE_LIST_END; uptr = sim_clock_queue) {
    sim_clock_queue = uptr->next;
    uptr->next = NULL;
    }
r = reset_all (0);
if ((r == SCPE_OK) && (flag == RU_RUN)) {
    if ((run_cmd_did_reset) && (0 == (sim_switches & SWMASK ('Q')))) {
        sim_printf ("Resetting all devices...  This may not have been your intention.\n");
        sim_printf ("The GO and CONTINUE commands do not reset devices.\n");
        }
    run_cmd_did_reset = TRUE;
    }
return r;
}

/* Print stopped message
 * For VM stops, if a VM-specific "sim_vm_fprint_stopped" pointer is defined,
 * call the indicated routine to print additional information after the message
 * and before the PC value is printed.  If the routine returns FALSE, skip
 * printing the PC and its related instruction.
 */

void fprint_stopped_gen (FILE *st, t_stat v, REG *pc, DEVICE *dptr)
{
int32 i;
t_stat r = 0;
t_addr k;
t_value pcval;

fputc ('\n', st);                                       /* start on a new line */

if (v >= SCPE_BASE)                                     /* SCP error? */
    fputs (sim_error_text (v), st);                     /* print it from the SCP list */
else {                                                  /* VM error */
    fputs (sim_stop_messages [v], st);                  /* print the VM-specific message */

    if ((sim_vm_fprint_stopped != NULL) &&              /* if a VM-specific stop handler is defined */
        (!sim_vm_fprint_stopped (st, v)))               /*   call it; if it returned FALSE, */
        return;                                         /*     we're done */
    }

(void)fprintf (st, ", %s: ", pc->name);                       /* print the name of the PC register */

pcval = get_rval (pc, 0);
if ((pc->flags & REG_VMAD) && sim_vm_fprint_addr)       /* if reg wants VM-specific printer */
    sim_vm_fprint_addr (st, dptr, (t_addr) pcval);      /*   call it to print the PC address */
else fprint_val (st, pcval, pc->radix, pc->width,       /* otherwise, print as a numeric value */
    pc->flags & REG_FMT);                               /*   with the radix and formatting specified */
if ((dptr != NULL) && (dptr->examine != NULL)) {
    for (i = 0; i < sim_emax; i++)
        sim_eval[i] = 0;
    for (i = 0, k = (t_addr) pcval; i < sim_emax; i++, k = k + dptr->aincr) {
        if ((r = dptr->examine (&sim_eval[i], k, dptr->units, SWMASK ('V')|SIM_SW_STOP)) != SCPE_OK)
            break;
        }
    if ((r == SCPE_OK) || (i > 0)) {
        (void)fprintf (st, " (");
        if (fprint_sym (st, (t_addr) pcval, sim_eval, NULL, SWMASK('M')|SIM_SW_STOP) > 0)
            fprint_val (st, sim_eval[0], dptr->dradix, dptr->dwidth, PV_RZRO);
        (void)fprintf (st, ")");
        }
    }
(void)fprintf (st, "\n");
return;
}

void fprint_stopped (FILE *st, t_stat v)
{
#if defined(WIN_STDIO)
(void)fflush(stderr);
(void)fflush(stdout);
#endif /* if defined(WIN_STDIO) */
fprint_stopped_gen (st, v, sim_PC, sim_dflt_dev);
return;
}

/* Unit service for step timeout, originally scheduled by STEP n command
   Return step timeout SCP code, will cause simulation to stop */

t_stat step_svc (UNIT *uptr)
{
return SCPE_STEP;
}

/* Unit service to facilitate expect matching to stop simulation.
   Return expect SCP code, will cause simulation to stop */

t_stat expect_svc (UNIT *uptr)
{
return SCPE_EXPECT | (sim_do_echo ? 0 : SCPE_NOMESSAGE);
}

/* Signal handler for ^C signal - set stop simulation flag */

void int_handler (int sig)
{
stop_cpu = 1;
return;
}

/* Examine/deposit commands */

t_stat exdep_cmd (int32 flag, CONST char *cptr)
{
char gbuf[CBUFSIZE];
CONST char *gptr;
CONST char *tptr = NULL;
int32 opt;
t_addr low, high;
t_stat reason = SCPE_IERR;
DEVICE *tdptr;
REG *lowr, *highr;
FILE *ofile;

opt = CMD_OPT_SW|CMD_OPT_SCH|CMD_OPT_DFT;               /* options for all */
if (flag == EX_E)                                       /* extra for EX */
    opt = opt | CMD_OPT_OF;
cptr = get_sim_opt (opt, cptr, &reason);                /* get cmd options */
if (NULL == cptr)                                       /* error? */
    return reason;
if (*cptr == 0)                                         /* must be more */
    return SCPE_2FARG;
if (sim_dfunit == NULL)                                 /* got a unit? */
    return SCPE_NXUN;
cptr = get_glyph (cptr, gbuf, 0);                       /* get list */
if ((flag == EX_D) && (*cptr == 0))                     /* deposit needs more */

    return SCPE_2FARG;
ofile = sim_ofile? sim_ofile: stdout;                   /* no ofile? use stdout */

for (gptr = gbuf, reason = SCPE_OK;
    (*gptr != 0) && (reason == SCPE_OK); gptr = tptr) {
    tdptr = sim_dfdev;                                  /* working dptr */
    if (strncmp (gptr, "STATE", strlen ("STATE")) == 0) {
        tptr = gptr + strlen ("STATE");
        if (*tptr && (*tptr++ != ','))
            return SCPE_ARG;
        if ((lowr = sim_dfdev->registers) == NULL)
            return SCPE_NXREG;
        for (highr = lowr; highr->name != NULL; highr++) ;
        sim_switches = sim_switches | SIM_SW_HIDE;
        reason = exdep_reg_loop (ofile, sim_schrptr, flag, cptr,
            lowr, --highr, 0, 0);
        continue;
        }

    /* LINTED E_EQUALITY_NOT_ASSIGNMENT*/
    if ((lowr = find_reg (gptr, &tptr, tdptr)) ||       /* local reg or */
        (!(sim_opt_out & CMD_OPT_DFT) &&                /* no dflt, global? */
        (lowr = find_reg_glob (gptr, &tptr, &tdptr)))) {
        low = high = 0;
        if ((*tptr == '-') || (*tptr == ':')) {
            highr = find_reg (tptr + 1, &tptr, tdptr);
            if (highr == NULL)
                return SCPE_NXREG;
            }
        else {
            highr = lowr;
            if (*tptr == '[') {
                if (lowr->depth <= 1)
                    return SCPE_ARG;
                tptr = get_range (NULL, tptr + 1, &low, &high,
                    10, lowr->depth - 1, ']');
                if (tptr == NULL)
                    return SCPE_ARG;
                }
            }
        if (*tptr && (*tptr++ != ','))
            return SCPE_ARG;
        reason = exdep_reg_loop (ofile, sim_schrptr, flag, cptr,
            lowr, highr, (uint32) low, (uint32) high);
        continue;
        }

    tptr = get_range (sim_dfdev, gptr, &low, &high, sim_dfdev->aradix,
        (((sim_dfunit->capac == 0) || (flag == EX_E))? 0:
        sim_dfunit->capac - sim_dfdev->aincr), 0);
    if (tptr == NULL)
        return SCPE_ARG;
    if (*tptr && (*tptr++ != ','))
        return SCPE_ARG;
    reason = exdep_addr_loop (ofile, sim_schaptr, flag, cptr, low, high,
        sim_dfdev, sim_dfunit);
    }                                                   /* end for */
if (sim_ofile)                                          /* close output file */
    fclose (sim_ofile);
return reason;
}

/* Loop controllers for examine/deposit

   exdep_reg_loop       examine/deposit range of registers
   exdep_addr_loop      examine/deposit range of addresses
*/

t_stat exdep_reg_loop (FILE *ofile, SCHTAB *schptr, int32 flag, CONST char *cptr,
    REG *lowr, REG *highr, uint32 lows, uint32 highs)
{
t_stat reason;
uint32 idx, val_start=lows;
t_value val, last_val;
REG *rptr;

if ((lowr == NULL) || (highr == NULL))
    return SCPE_IERR;
if (lowr > highr)
    return SCPE_ARG;
for (rptr = lowr; rptr <= highr; rptr++) {
    if ((sim_switches & SIM_SW_HIDE) &&
        (rptr->flags & REG_HIDDEN))
        continue;
    val = last_val = 0;
    for (idx = lows; idx <= highs; idx++) {
        if (idx >= rptr->depth)
            return SCPE_SUB;
        val = get_rval (rptr, idx);
        if (schptr && !test_search (&val, schptr))
            continue;
        if (flag == EX_E) {
            if ((idx > lows) && (val == last_val))
                continue;
            if (idx > val_start+1) {
                if (idx-1 == val_start+1) {
                    reason = ex_reg (ofile, val, flag, rptr, idx-1);
                    if (reason != SCPE_OK)
                        return reason;
                    if (sim_log && (ofile == stdout))
                        ex_reg (sim_log, val, flag, rptr, idx-1);
                    }
                else {
                    if (val_start+1 != idx-1) {
                        (void)Fprintf (ofile, "%s[%d]-%s[%d]: same as above\n", rptr->name, val_start+1, rptr->name, idx-1);
                        if (sim_log && (ofile == stdout))
                            (void)Fprintf (sim_log, "%s[%d]-%s[%d]: same as above\n", rptr->name, val_start+1, rptr->name, idx-1);
                        }
                    else {
                        (void)Fprintf (ofile, "%s[%d]: same as above\n", rptr->name, val_start+1);
                        if (sim_log && (ofile == stdout))
                            (void)Fprintf (sim_log, "%s[%d]: same as above\n", rptr->name, val_start+1);
                        }
                    }
                }
            sim_last_val = last_val = val;
            val_start = idx;
            reason = ex_reg (ofile, val, flag, rptr, idx);
            if (reason != SCPE_OK)
                return reason;
            if (sim_log && (ofile == stdout))
                ex_reg (sim_log, val, flag, rptr, idx);
            }
        if (flag != EX_E) {
            reason = dep_reg (flag, cptr, rptr, idx);
            if (reason != SCPE_OK)
                return reason;
            }
        }
    if ((flag == EX_E) && (val_start != highs)) {
        if (highs == val_start+1) {
            reason = ex_reg (ofile, val, flag, rptr, highs);
            if (reason != SCPE_OK)
                return reason;
            if (sim_log && (ofile == stdout))
                ex_reg (sim_log, val, flag, rptr, highs);
            }
        else {
            if (val_start+1 != highs) {
                (void)Fprintf (ofile, "%s[%d]-%s[%d]: same as above\n", rptr->name, val_start+1, rptr->name, highs);
                if (sim_log && (ofile == stdout))
                    (void)Fprintf (sim_log, "%s[%d]-%s[%d]: same as above\n", rptr->name, val_start+1, rptr->name, highs);
                }
            else {
                (void)Fprintf (ofile, "%s[%d]: same as above\n", rptr->name, val_start+1);
                if (sim_log && (ofile == stdout))
                    (void)Fprintf (sim_log, "%s[%d]: same as above\n", rptr->name, val_start+1);
                }
            }
        }
    }
return SCPE_OK;
}

t_stat exdep_addr_loop (FILE *ofile, SCHTAB *schptr, int32 flag, const char *cptr,
    t_addr low, t_addr high, DEVICE *dptr, UNIT *uptr)
{
t_addr i, mask;
t_stat reason;

if (uptr->flags & UNIT_DIS)                             /* disabled? */
    return SCPE_UDIS;
mask = (t_addr) width_mask[dptr->awidth];
if ((low > mask) || (high > mask) || (low > high))
    return SCPE_ARG;
for (i = low; i <= high; ) {                            /* all paths must incr!! */
    reason = get_aval (i, dptr, uptr);                  /* get data */
    if (reason != SCPE_OK)                              /* return if error */
        return reason;
    if (schptr && !test_search (sim_eval, schptr))
        i = i + dptr->aincr;                            /* sch fails, incr */
    else {                                              /* no sch or success */
        if (flag != EX_D) {                             /* ex, ie, or id? */
            reason = ex_addr (ofile, flag, i, dptr, uptr);
            if (reason > SCPE_OK)
                return reason;
            if (sim_log && (ofile == stdout))
                ex_addr (sim_log, flag, i, dptr, uptr);
            }
        else reason = 1 - dptr->aincr;                  /* no, dflt incr */
        if (flag != EX_E) {                             /* ie, id, or d? */
            reason = dep_addr (flag, cptr, i, dptr, uptr, reason);
            if (reason > SCPE_OK)
                return reason;
            }
        i = i + (1 - reason);                           /* incr */
        }
    }
return SCPE_OK;
}

/* Examine register routine

   Inputs:
        ofile   =       output stream
        val     =       current register value
        flag    =       type of ex/mod command (ex, iex, idep)
        rptr    =       pointer to register descriptor
        idx     =       index
   Outputs:
        return  =       error status
*/

t_stat ex_reg (FILE *ofile, t_value val, int32 flag, REG *rptr, uint32 idx)
{
int32 rdx;

if (rptr == NULL)
    return SCPE_IERR;
if (rptr->depth > 1)
    (void)Fprintf (ofile, "%s[%d]:\t", rptr->name, idx);
else
    (void)Fprintf (ofile, "%s:\t", rptr->name);
if (!(flag & EX_E))
    return SCPE_OK;
GET_RADIX (rdx, rptr->radix);
if ((rptr->flags & REG_VMAD) && sim_vm_fprint_addr && sim_dflt_dev)
    sim_vm_fprint_addr (ofile, sim_dflt_dev, (t_addr) val);
else if (!(rptr->flags & REG_VMFLAGS) ||
    (fprint_sym (ofile, (rptr->flags & REG_UFMASK) | rdx, &val,
                 NULL, sim_switches | SIM_SW_REG) > 0)) {
        fprint_val (ofile, val, rdx, rptr->width, rptr->flags & REG_FMT);
        if (rptr->fields) {
            (void)Fprintf (ofile, "\t");
            fprint_fields (ofile, val, val, rptr->fields);
            }
        }
if (flag & EX_I)
    (void)Fprintf (ofile, "\t");
else
    (void)Fprintf (ofile, "\n");
return SCPE_OK;
}

/* Get register value

   Inputs:
        rptr    =       pointer to register descriptor
        idx     =       index
   Outputs:
        return  =       register value
*/

t_value get_rval (REG *rptr, uint32 idx)
{
size_t sz;
t_value val;
uint32 *ptr;

sz = SZ_R (rptr);
if ((rptr->depth > 1) && (rptr->flags & REG_CIRC)) {
    idx = idx + rptr->qptr;
    if (idx >= rptr->depth) idx = idx - rptr->depth;
    }
if ((rptr->depth > 1) && (rptr->flags & REG_UNIT)) {
    ptr = (uint32 *)(((UNIT *) rptr->loc) + idx);
    if (sz <= sizeof (uint32))
        val = *ptr;
    else val = *((t_uint64 *) ptr); //-V1032
    }
else if ((rptr->depth > 1) && (rptr->flags & REG_STRUCT)) {
    ptr = (uint32 *)(((size_t) rptr->loc) + (idx * rptr->str_size));
    if (sz <= sizeof (uint32))
        val = *ptr;
    else val = *((t_uint64 *) ptr);
    }
else if (((rptr->depth > 1) || (rptr->flags & REG_FIT)) &&
    (sz == sizeof (uint8)))
    val = *(((uint8 *) rptr->loc) + idx);
else if (((rptr->depth > 1) || (rptr->flags & REG_FIT)) &&
    (sz == sizeof (uint16)))
    val = *(((uint16 *) rptr->loc) + idx);
else if (sz <= sizeof (uint32))
     val = *(((uint32 *) rptr->loc) + idx);
else val = *(((t_uint64 *) rptr->loc) + idx);
val = (val >> rptr->offset) & width_mask[rptr->width];
return val;
}

/* Deposit register routine

   Inputs:
        flag    =       type of deposit (normal/interactive)
        cptr    =       pointer to input string
        rptr    =       pointer to register descriptor
        idx     =       index
   Outputs:
        return  =       error status
*/

t_stat dep_reg (int32 flag, CONST char *cptr, REG *rptr, uint32 idx)
{
t_stat r;
t_value val, mask;
int32 rdx;
CONST char *tptr;
char gbuf[CBUFSIZE];

if ((cptr == NULL) || (rptr == NULL))
    return SCPE_IERR;
if (rptr->flags & REG_RO)
    return SCPE_RO;
if (flag & EX_I) {
    cptr = read_line (gbuf, sizeof(gbuf), stdin);
    if (sim_log)
        (void)fprintf (sim_log, "%s\n", cptr? cptr: "");
    if (cptr == NULL)                                   /* force exit */
        return 1;
    if (*cptr == 0)                                     /* success */
        return SCPE_OK;
    }
mask = width_mask[rptr->width];
GET_RADIX (rdx, rptr->radix);
if ((rptr->flags & REG_VMAD) && sim_vm_parse_addr && sim_dflt_dev) {    /* address form? */
    val = sim_vm_parse_addr (sim_dflt_dev, cptr, &tptr);
    if ((tptr == cptr) || (*tptr != 0) || (val > mask))
        return SCPE_ARG;
    }
else
    if (!(rptr->flags & REG_VMFLAGS) ||                 /* don't use sym? */
        (parse_sym ((CONST char *)cptr, (rptr->flags & REG_UFMASK) | rdx, NULL,
                    &val, sim_switches | SIM_SW_REG) > SCPE_OK)) {
    val = get_uint (cptr, rdx, mask, &r);
    if (r != SCPE_OK)
        return SCPE_ARG;
    }
if ((rptr->flags & REG_NZ) && (val == 0))
    return SCPE_ARG;
put_rval (rptr, idx, val);
return SCPE_OK;
}

/* Put register value

   Inputs:
        rptr    =       pointer to register descriptor
        idx     =       index
        val     =       new value
        mask    =       mask
   Outputs:
        none
*/

void put_rval (REG *rptr, uint32 idx, t_value val)
{
size_t sz;
t_value mask;
uint32 *ptr;

#define PUT_RVAL(sz,rp,id,v,m)            \
    *(((sz *) rp->loc) + id) =            \
        (sz)((*(((sz *) rp->loc) + id) &  \
            ~((m) << (rp)->offset)) | ((v) << (rp)->offset))

if (rptr == sim_PC)
    sim_brk_npc (0);
sz = SZ_R (rptr);
mask = width_mask[rptr->width];
if ((rptr->depth > 1) && (rptr->flags & REG_CIRC)) {
    idx = idx + rptr->qptr;
    if (idx >= rptr->depth)
        idx = idx - rptr->depth;
    }
if ((rptr->depth > 1) && (rptr->flags & REG_UNIT)) {
    ptr = (uint32 *)(((UNIT *) rptr->loc) + idx);
    if (sz <= sizeof (uint32))
        *ptr = (*ptr &
        ~(((uint32) mask) << rptr->offset)) |
        (((uint32) val) << rptr->offset);
    else *((t_uint64 *) ptr) = (*((t_uint64 *) ptr) //-V1032
        & ~(mask << rptr->offset)) | (val << rptr->offset);
    }
else if ((rptr->depth > 1) && (rptr->flags & REG_STRUCT)) {
    ptr = (uint32 *)(((size_t) rptr->loc) + (idx * rptr->str_size));
    if (sz <= sizeof (uint32))
        *((uint32 *) ptr) = (*((uint32 *) ptr) &
        ~(((uint32) mask) << rptr->offset)) |
        (((uint32) val) << rptr->offset);
    else *((t_uint64 *) ptr) = (*((t_uint64 *) ptr)
        & ~(mask << rptr->offset)) | (val << rptr->offset);
    }
else if (((rptr->depth > 1) || (rptr->flags & REG_FIT)) &&
    (sz == sizeof (uint8)))
    PUT_RVAL (uint8, rptr, idx, (uint32) val, (uint32) mask);
else if (((rptr->depth > 1) || (rptr->flags & REG_FIT)) &&
    (sz == sizeof (uint16)))
    PUT_RVAL (uint16, rptr, idx, (uint32) val, (uint32) mask);
else if (sz <= sizeof (uint32))
    PUT_RVAL (uint32, rptr, idx, (int32) val, (uint32) mask);
else PUT_RVAL (t_uint64, rptr, idx, val, mask);
return;
}

/* Examine address routine

   Inputs: (sim_eval is an implicit argument)
        ofile   =       output stream
        flag    =       type of ex/mod command (ex, iex, idep)
        addr    =       address to examine
        dptr    =       pointer to device
        uptr    =       pointer to unit
   Outputs:
        return  =       if > 0, error status
                        if <= 0,-number of extra addr units retired
*/

t_stat ex_addr (FILE *ofile, int32 flag, t_addr addr, DEVICE *dptr, UNIT *uptr)
{
t_stat reason;
int32 rdx;

if (sim_vm_fprint_addr)
    sim_vm_fprint_addr (ofile, dptr, addr);
else fprint_val (ofile, addr, dptr->aradix, dptr->awidth, PV_LEFT);
(void)Fprintf (ofile, ":\t");
if (!(flag & EX_E))
    return (1 - dptr->aincr);

GET_RADIX (rdx, dptr->dradix);
if ((reason = fprint_sym (ofile, addr, sim_eval, uptr, sim_switches)) > 0) {
    fprint_val (ofile, sim_eval[0], rdx, dptr->dwidth, PV_RZRO);
    reason = 1 - dptr->aincr;
    }
if (flag & EX_I)
    (void)Fprintf (ofile, "\t");
else
    (void)Fprintf (ofile, "\n");
return reason;
}

/* Get address routine

   Inputs:
        flag    =       type of ex/mod command (ex, iex, idep)
        addr    =       address to examine
        dptr    =       pointer to device
        uptr    =       pointer to unit
   Outputs: (sim_eval is an implicit output)
        return  =       error status
*/

t_stat get_aval (t_addr addr, DEVICE *dptr, UNIT *uptr)
{
int32 i;
t_value mask;
t_addr j, loc;
size_t sz;
t_stat reason = SCPE_OK;

if ((dptr == NULL) || (uptr == NULL))
    return SCPE_IERR;
mask = width_mask[dptr->dwidth];
for (i = 0; i < sim_emax; i++)
    sim_eval[i] = 0;
for (i = 0, j = addr; i < sim_emax; i++, j = j + dptr->aincr) {
    if (dptr->examine != NULL) {
        reason = dptr->examine (&sim_eval[i], j, uptr, sim_switches);
        if (reason != SCPE_OK)
            break;
        }
    else {
        if (!(uptr->flags & UNIT_ATT))
            return SCPE_UNATT;
        if (uptr->dynflags & UNIT_NO_FIO)
            return SCPE_NOFNC;
        if ((uptr->flags & UNIT_FIX) && (j >= uptr->capac)) {
            reason = SCPE_NXM;
            break;
            }
        sz = SZ_D (dptr);
        loc = j / dptr->aincr;
        if (uptr->flags & UNIT_BUF) {
            SZ_LOAD (sz, sim_eval[i], uptr->filebuf, loc);
            }
        else {
            sim_fseek (uptr->fileref, (t_addr)(sz * loc), SEEK_SET);
            sim_fread (&sim_eval[i], sz, 1, uptr->fileref);
            if ((feof (uptr->fileref)) &&
               !(uptr->flags & UNIT_FIX)) {
                reason = SCPE_EOF;
                break;
                }
            else if (ferror (uptr->fileref)) {
                clearerr (uptr->fileref);
                reason = SCPE_IOERR;
                break;
                }
            }
        }
    sim_last_val = sim_eval[i] = sim_eval[i] & mask;
    }
if ((reason != SCPE_OK) && (i == 0))
    return reason;
return SCPE_OK;
}

/* Deposit address routine

   Inputs:
        flag    =       type of deposit (normal/interactive)
        cptr    =       pointer to input string
        addr    =       address to examine
        dptr    =       pointer to device
        uptr    =       pointer to unit
        dfltinc =       value to return on cr input
   Outputs:
        return  =       if > 0, error status
                        if <= 0, -number of extra address units retired
*/

t_stat dep_addr (int32 flag, const char *cptr, t_addr addr, DEVICE *dptr,
    UNIT *uptr, int32 dfltinc)
{
int32 i, count, rdx;
t_addr j, loc;
t_stat r, reason;
t_value mask;
size_t sz;
char gbuf[CBUFSIZE];

if (dptr == NULL)
    return SCPE_IERR;
if (flag & EX_I) {
    cptr = read_line (gbuf, sizeof(gbuf), stdin);
    if (sim_log)
        (void)fprintf (sim_log, "%s\n", cptr? cptr: "");
    if (cptr == NULL)                                   /* force exit */
        return 1;
    if (*cptr == 0)                                     /* success */
        return dfltinc;
    }
if (uptr->flags & UNIT_RO)                              /* read only? */
    return SCPE_RO;
mask = width_mask[dptr->dwidth];

GET_RADIX (rdx, dptr->dradix);
if ((reason = parse_sym ((CONST char *)cptr, addr, uptr, sim_eval, sim_switches)) > 0) {
    sim_eval[0] = get_uint (cptr, rdx, mask, &reason);
    if (reason != SCPE_OK)
        return reason;
    reason = dfltinc;
    }
count = (1 - reason + (dptr->aincr - 1)) / dptr->aincr;

for (i = 0, j = addr; i < count; i++, j = j + dptr->aincr) {
    sim_eval[i] = sim_eval[i] & mask;
    if (dptr->deposit != NULL) {
        r = dptr->deposit (sim_eval[i], j, uptr, sim_switches);
        if (r != SCPE_OK)
            return r;
        }
    else {
        if (!(uptr->flags & UNIT_ATT))
            return SCPE_UNATT;
        if (uptr->dynflags & UNIT_NO_FIO)
            return SCPE_NOFNC;
        if ((uptr->flags & UNIT_FIX) && (j >= uptr->capac))
            return SCPE_NXM;
        sz = SZ_D (dptr);
        loc = j / dptr->aincr;
        if (uptr->flags & UNIT_BUF) {
            SZ_STORE (sz, sim_eval[i], uptr->filebuf, loc);
            if (loc >= uptr->hwmark)
                uptr->hwmark = (uint32) loc + 1;
            }
        else {
            sim_fseek (uptr->fileref, (t_addr)(sz * loc), SEEK_SET);
            sim_fwrite (&sim_eval[i], sz, 1, uptr->fileref);
            if (ferror (uptr->fileref)) {
                clearerr (uptr->fileref);
                return SCPE_IOERR;
                }
            }
        }
    }
return reason;
}

/* Evaluate command */

t_stat eval_cmd (int32 flg, CONST char *cptr)
{
if (!sim_dflt_dev)
  return SCPE_ARG;
DEVICE *dptr = sim_dflt_dev;
int32 i, rdx, a, lim;
t_stat r;

GET_SWITCHES (cptr);
GET_RADIX (rdx, dptr->dradix);
for (i = 0; i < sim_emax; i++)
sim_eval[i] = 0;
if (*cptr == 0)
    return SCPE_2FARG;
if ((r = parse_sym ((CONST char *)cptr, 0, dptr->units, sim_eval, sim_switches)) > 0) {
    sim_eval[0] = get_uint (cptr, rdx, width_mask[dptr->dwidth], &r);
    if (r != SCPE_OK)
        return r;
    }
lim = 1 - r;
for (i = a = 0; a < lim; ) {
    sim_printf ("%d:\t", a);
    if ((r = fprint_sym (stdout, a, &sim_eval[i], dptr->units, sim_switches)) > 0)
        r = fprint_val (stdout, sim_eval[i], rdx, dptr->dwidth, PV_RZRO);
    if (sim_log) {
        if ((r = fprint_sym (sim_log, a, &sim_eval[i], dptr->units, sim_switches)) > 0)
            r = fprint_val (sim_log, sim_eval[i], rdx, dptr->dwidth, PV_RZRO);
        }
    sim_printf ("\n");
    if (r < 0)
        a = a + 1 - r;
    else a = a + dptr->aincr;
    i = a / dptr->aincr;
    }
return SCPE_OK;
}

/* String processing routines

   read_line            read line

   Inputs:
        cptr    =       pointer to buffer
        size    =       maximum size
        stream  =       pointer to input stream
   Outputs:
        optr    =       pointer to first non-blank character
                        NULL if EOF
*/

char *read_line (char *cptr, int32 size, FILE *stream)
{
return read_line_p (NULL, cptr, size, stream);
}

/* read_line_p          read line with prompt

   Inputs:
        prompt  =       pointer to prompt string
        cptr    =       pointer to buffer
        size    =       maximum size
        stream  =       pointer to input stream
   Outputs:
        optr    =       pointer to first non-blank character
                        NULL if EOF
*/

char *read_line_p (const char *prompt, char *cptr, int32 size, FILE *stream)
{
char *tptr;

if (prompt) {                                           /* interactive? */
#if defined(HAVE_LINEHISTORY)
        char *tmpc = linenoise (prompt);                /* get cmd line */
        if (tmpc == NULL)                               /* bad result? */
            cptr = NULL;
        else {
            strncpy (cptr, tmpc, size-1);               /* copy result */
            linenoiseHistoryAdd (tmpc);                 /* add to history */
            FREE (tmpc);                                /* free temp */
            }
        }
#else
        (void)fflush (stdout);                          /* flush output */
        (void)printf ("%s", prompt);                    /* display prompt */
        (void)fflush (stdout);                          /* flush output */
        cptr = fgets (cptr, size, stream);              /* get cmd line */
        }
#endif /* if defined(HAVE_LINEHISTORY) */
else cptr = fgets (cptr, size, stream);                 /* get cmd line */

if (cptr == NULL) {
    clearerr (stream);                                  /* clear error */
    return NULL;                                        /* ignore EOF */
    }
for (tptr = cptr; tptr < (cptr + size); tptr++) {       /* remove cr or nl */
    if ((*tptr == '\n') || (*tptr == '\r') ||
        (tptr == (cptr + size - 1))) {                  /* str max length? */
        *tptr = 0;                                      /* terminate */
        break;
        }
    }
if (0 == memcmp (cptr, "\xEF\xBB\xBF", 3))              /* Skip/ignore UTF8_BOM */
    memmove (cptr, cptr + 3, strlen (cptr + 3));
while (sim_isspace (*cptr))                             /* trim leading spc */
    cptr++;
if ((*cptr == ';') || (*cptr == '#')) {                 /* ignore comment */
    if (sim_do_echo)                                    /* echo comments if -v */
        sim_printf("%s> %s\n", do_position(), cptr);
    *cptr = 0;
    }

return cptr;
}

/* get_glyph            get next glyph (force upper case)
   get_glyph_nc         get next glyph (no conversion)
   get_glyph_quoted     get next glyph (potentially enclosed in quotes, no conversion)
   get_glyph_cmd        get command glyph (force upper case, extract leading !)
   get_glyph_gen        get next glyph (general case)

   Inputs:
        iptr        =   pointer to input string
        optr        =   pointer to output string
        mchar       =   optional end of glyph character
        uc          =   TRUE for convert to upper case (_gen only)
        quote       =   TRUE to allow quote enclosing values (_gen only)
        escape_char =   optional escape character within quoted strings (_gen only)

   Outputs
        result      =   pointer to next character in input string
*/

static const char *get_glyph_gen (const char *iptr, char *optr, char mchar, t_bool uc, t_bool quote, char escape_char)
{
t_bool quoting = FALSE;
t_bool escaping = FALSE;
char quote_char = 0;

while ((*iptr != 0) &&
       ((quote && quoting) || ((sim_isspace (*iptr) == 0) && (*iptr != mchar)))) {
    if (quote) {
        if (quoting) {
            if (!escaping) {
                if (*iptr == escape_char)
                    escaping = TRUE;
                else
                    if (*iptr == quote_char)
                        quoting = FALSE;
                }
            else
                escaping = FALSE;
            }
        else {
            if ((*iptr == '"') || (*iptr == '\'')) {
                quoting = TRUE;
                quote_char = *iptr;
                }
            }
        }
    if (sim_islower (*iptr) && uc)
        *optr = (char)toupper (*iptr);
    else *optr = *iptr;
    iptr++; optr++;
    }
if (mchar && (*iptr == mchar))              /* skip input terminator */
    iptr++;
*optr = 0;                                  /* terminate result string */
while (sim_isspace (*iptr))                 /* absorb additional input spaces */
    iptr++;
return iptr;
}

CONST char *get_glyph (const char *iptr, char *optr, char mchar)
{
return (CONST char *)get_glyph_gen (iptr, optr, mchar, TRUE, FALSE, 0);
}

CONST char *get_glyph_nc (const char *iptr, char *optr, char mchar)
{
return (CONST char *)get_glyph_gen (iptr, optr, mchar, FALSE, FALSE, 0);
}

CONST char *get_glyph_quoted (const char *iptr, char *optr, char mchar)
{
return (CONST char *)get_glyph_gen (iptr, optr, mchar, FALSE, TRUE, '\\');
}

CONST char *get_glyph_cmd (const char *iptr, char *optr)
{
/* Tolerate "!subprocess" vs. requiring "! subprocess" */
if ((iptr[0] == '!') && (!sim_isspace(iptr[1]))) {
    strcpy (optr, "!");                     /* return ! as command glyph */
    return (CONST char *)(iptr + 1);        /* and skip over the leading ! */
    }
return (CONST char *)get_glyph_gen (iptr, optr, 0, TRUE, FALSE, 0);
}

/* Trim trailing spaces from a string

    Inputs:
        cptr    =       pointer to string
    Outputs:
        cptr    =       pointer to string
*/

char *sim_trim_endspc (char *cptr)
{
char *tptr;

tptr = cptr + strlen (cptr);
while ((--tptr >= cptr) && sim_isspace (*tptr))
    *tptr = 0;
return cptr;
}

int sim_isspace (char c)
{
return (c & 0x80) ? 0 : isspace (c);
}

int sim_islower (char c)
{
return (c & 0x80) ? 0 : islower (c);
}

int sim_isalpha (char c)
{
return (c & 0x80) ? 0 : isalpha (c);
}

int sim_isprint (char c)
{
return (c & 0x80) ? 0 : isprint (c);
}

int sim_isdigit (char c)
{
return (c & 0x80) ? 0 : isdigit (c);
}

int sim_isgraph (char c)
{
return (c & 0x80) ? 0 : isgraph (c);
}

int sim_isalnum (char c)
{
return (c & 0x80) ? 0 : isalnum (c);
}

/* get_uint             unsigned number

   Inputs:
        cptr    =       pointer to input string
        radix   =       input radix
        max     =       maximum acceptable value
        *status =       pointer to error status
   Outputs:
        val     =       value
*/

t_value get_uint (const char *cptr, uint32 radix, t_value max, t_stat *status)
{
t_value val;
CONST char *tptr;

*status = SCPE_OK;
val = strtotv ((CONST char *)cptr, &tptr, radix);
if ((cptr == tptr) || (val > max))
    *status = SCPE_ARG;
else {
    while (sim_isspace (*tptr)) tptr++;
    if (*tptr != 0)
        *status = SCPE_ARG;
    }
return val;
}

/* get_range            range specification

   Inputs:
        dptr    =       pointer to device (NULL if none)
        cptr    =       pointer to input string
        *lo     =       pointer to low result
        *hi     =       pointer to high result
        aradix  =       radix
        max     =       default high value
        term    =       terminating character, 0 if none
   Outputs:
        tptr    =       input pointer after processing
                        NULL if error
*/

CONST char *get_range (DEVICE *dptr, CONST char *cptr, t_addr *lo, t_addr *hi,
    uint32 rdx, t_addr max, char term)
{
CONST char *tptr;

if (max && strncmp (cptr, "ALL", strlen ("ALL")) == 0) {    /* ALL? */
    tptr = cptr + strlen ("ALL");
    *lo = 0;
    *hi = max;
    }
else {
    if ((strncmp (cptr, ".", strlen (".")) == 0) &&             /* .? */
        ((cptr[1] == '\0') ||
         (cptr[1] == '-')  ||
         (cptr[1] == ':')  ||
         (cptr[1] == '/'))) {
        tptr = cptr + strlen (".");
        *lo = *hi = sim_last_addr;
        }
    else {
        if (strncmp (cptr, "$", strlen ("$")) == 0) {           /* $? */
            tptr = cptr + strlen ("$");
            *hi = *lo = (t_addr)sim_last_val;
            }
        else {
            if (dptr && sim_vm_parse_addr)                      /* get low */
                *lo = sim_vm_parse_addr (dptr, cptr, &tptr);
            else
                *lo = (t_addr) strtotv (cptr, &tptr, rdx);
            if (cptr == tptr)                                   /* error? */
                    return NULL;
            }
        }
    if ((*tptr == '-') || (*tptr == ':')) {             /* range? */
        cptr = tptr + 1;
        if (dptr && sim_vm_parse_addr)                  /* get high */
            *hi = sim_vm_parse_addr (dptr, cptr, &tptr);
        else *hi = (t_addr) strtotv (cptr, &tptr, rdx);
        if (cptr == tptr)
            return NULL;
        if (*lo > *hi)
            return NULL;
        }
    else if (*tptr == '/') {                            /* relative? */
        cptr = tptr + 1;
        *hi = (t_addr) strtotv (cptr, &tptr, rdx);      /* get high */
        if ((cptr == tptr) || (*hi == 0))
            return NULL;
        *hi = *lo + *hi - 1;
        }
    else *hi = *lo;
    }
sim_last_addr = *hi;
if (term && (*tptr++ != term))
    return NULL;
return tptr;
}

/* sim_decode_quoted_string

   Inputs:
        iptr        =   pointer to input string
        optr        =   pointer to output buffer
                        the output buffer must be allocated by the caller
                        and to avoid overrunat it must be at least as big
                        as the input string.

   Outputs
        result      =   status of decode SCPE_OK when good, SCPE_ARG otherwise
        osize       =   size of the data in the optr buffer

   The input string must be quoted.  Quotes may be either single or
   double but the opening anc closing quote characters must match.
   Within quotes C style character escapes are allowed.
*/

t_stat sim_decode_quoted_string (const char *iptr, uint8 *optr, uint32 *osize)
{
char quote_char;
uint8 *ostart = optr;

*osize = 0;
if ((strlen(iptr) == 1) ||
    (iptr[0] != iptr[strlen(iptr)-1]) ||
    ((iptr[strlen(iptr)-1] != '"') && (iptr[strlen(iptr)-1] != '\'')))
    return SCPE_ARG;            /* String must be quote delimited */
quote_char = *iptr++;           /* Save quote character */
while (iptr[1]) {               /* Skip trailing quote */
    if (*iptr != '\\') {
        if (*iptr == quote_char)
            return SCPE_ARG;    /* Embedded quotes must be escaped */
        *(optr++) = (uint8)(*(iptr++));
        continue;
        }
    ++iptr; /* Skip backslash */
    switch (*iptr) {
        case 'r':   /* ASCII Carriage Return character (Decimal value 13) */
            *(optr++) = 13; ++iptr;
            break;
        case 'n':   /* ASCII Linefeed character (Decimal value 10) */
            *(optr++) = 10; ++iptr;
            break;
        case 'f':   /* ASCII Formfeed character (Decimal value 12) */
            *(optr++) = 12; ++iptr;
            break;
        case 't':   /* ASCII Horizontal Tab character (Decimal value 9) */
            *(optr++) = 9; ++iptr;
            break;
        case 'v':   /* ASCII Vertical Tab character (Decimal value 11) */
            *(optr++) = 11; ++iptr;
            break;
        case 'b':   /* ASCII Backspace character (Decimal value 8) */
            *(optr++) = 8; ++iptr;
            break;
        case '\\':   /* ASCII Backslash character (Decimal value 92) */
            *(optr++) = 92; ++iptr;
            break;
        case 'e':   /* ASCII Escape character (Decimal value 27) */
            *(optr++) = 27; ++iptr;
            break;
        case '\'':   /* ASCII Single Quote character (Decimal value 39) */
            *(optr++) = 39; ++iptr;
            break;
        case '"':   /* ASCII Double Quote character (Decimal value 34) */
            *(optr++) = 34; ++iptr;
            break;
        case '?':   /* ASCII Question Mark character (Decimal value 63) */
            *(optr++) = 63; ++iptr;
            break;
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
            *optr = *(iptr++) - '0';
            if ((*iptr >= '0') && (*iptr <= '7'))
                *optr = ((*optr)<<3) + (*(iptr++) - '0');
            if ((*iptr >= '0') && (*iptr <= '7'))
                *optr = ((*optr)<<3) + (*(iptr++) - '0');
            ++optr;
            break;
        case 'x':
            if (1) {
                static const char *hex_digits = "0123456789ABCDEF";
                const char *c;

                ++iptr;
                *optr = 0;
                c = strchr (hex_digits, toupper(*iptr));
                if (c) {
                    *optr = ((*optr)<<4) + (uint8)(c-hex_digits);
                    ++iptr;
                    }
                c = strchr (hex_digits, toupper(*iptr));
                if (c) {
                    *optr = ((*optr)<<4) + (uint8)(c-hex_digits);
                    ++iptr;
                    }
                ++optr;
                }
            break;
        default:
            return SCPE_ARG;    /* Invalid escape */
        }
    }
*optr = '\0';
*osize = (uint32)(optr-ostart);
return SCPE_OK;
}

/* sim_encode_quoted_string

   Inputs:
        iptr        =   pointer to input buffer
        size        =   number of bytes of data in the buffer

   Outputs
        optr        =   pointer to output buffer
                        the output buffer must be freed by the caller

   The input data will be encoded into a simply printable form.
*/

char *sim_encode_quoted_string (const uint8 *iptr, size_t size)
{
size_t i;
t_bool double_quote_found = FALSE;
t_bool single_quote_found = FALSE;
char quote = '"';
char *tptr, *optr;

optr = (char *)malloc (4*size + 3);
if (optr == NULL)
    return NULL;
tptr = optr;
for (i=0; i<size; i++)
    switch ((char)iptr[i]) {
        case '"':
            double_quote_found = TRUE;
            break;
        case '\'':
            single_quote_found = TRUE;
            break;
        }
if (double_quote_found && (!single_quote_found))
    quote = '\'';
*tptr++ = quote;
while (size--) {
    switch (*iptr) {
        case '\r':
            *tptr++ = '\\'; *tptr++ = 'r'; break;
        case '\n':
            *tptr++ = '\\'; *tptr++ = 'n'; break;
        case '\f':
            *tptr++ = '\\'; *tptr++ = 'f'; break;
        case '\t':
            *tptr++ = '\\'; *tptr++ = 't'; break;
        case '\v':
            *tptr++ = '\\'; *tptr++ = 'v'; break;
        case '\b':
            *tptr++ = '\\'; *tptr++ = 'b'; break;
        case '\\':
            *tptr++ = '\\'; *tptr++ = '\\'; break;
        case '"':
        case '\'':
            if (quote == *iptr)
                *tptr++ = '\\';
        /*FALLTHRU*/ /* fall through */ /* fallthrough */
        default:
            if (sim_isprint (*iptr))
                *tptr++ = *iptr;
            else {
                (void)sprintf (tptr, "\\%03o", *iptr);
                tptr += 4;
                }
            break;
        }
    ++iptr;
    }
*tptr++ = quote;
*tptr++ = '\0';
return optr;
}

void fprint_buffer_string (FILE *st, const uint8 *buf, size_t size)
{
char *string;

string = sim_encode_quoted_string (buf, size);
(void)fprintf (st, "%s", string);
FREE (string);
}

/* Find_device          find device matching input string

   Inputs:
        cptr    =       pointer to input string
   Outputs:
        result  =       pointer to device
*/

DEVICE *find_dev (const char *cptr)
{
int32 i;
DEVICE *dptr;

if (cptr == NULL)
    return NULL;
for (i = 0; (dptr = sim_devices[i]) != NULL; i++) {
    if ((strcmp (cptr, dptr->name) == 0) ||
        (dptr->lname &&
        (strcmp (cptr, dptr->lname) == 0)))
        return dptr;
    }
for (i = 0; sim_internal_device_count && (dptr = sim_internal_devices[i]); ++i) {
    if ((strcmp (cptr, dptr->name) == 0) ||
        (dptr->lname &&
        (strcmp (cptr, dptr->lname) == 0)))
        return dptr;
    }
return NULL;
}

/* Find_unit            find unit matching input string

   Inputs:
        cptr    =       pointer to input string
        uptr    =       pointer to unit pointer
   Outputs:
        result  =       pointer to device (null if no dev)
        *iptr   =       pointer to unit (null if nx unit)

*/

DEVICE *find_unit (const char *cptr, UNIT **uptr)
{
uint32 i, u;
const char *nptr;
const char *tptr;
t_stat r;
DEVICE *dptr;

if (uptr == NULL)                                       /* arg error? */
    return NULL;
*uptr = NULL;
if ((dptr = find_dev (cptr))) {                         /* exact match? */
    if (qdisable (dptr))                                /* disabled? */
        return NULL;
    *uptr = dptr->units;                                /* unit 0 */
    return dptr;
    }

for (i = 0; (dptr = sim_devices[i]) != NULL; i++) {     /* base + unit#? */
    if (qdisable (dptr))                                /* device disabled? */
        continue;
    if (dptr->numunits && /* LINTED E_EQUALITY_NOT_ASSIGNMENT*/ /* any units? */
        (((nptr = dptr->name) &&
          (strncmp (cptr, nptr, strlen (nptr)) == 0)) || /* LINTED E_EQUALITY_NOT_ASSIGNMENT*/
         ((nptr = dptr->lname) &&
          (strncmp (cptr, nptr, strlen (nptr)) == 0)))) {
        tptr = cptr + strlen (nptr);
        if (sim_isdigit (*tptr)) {
            if (qdisable (dptr))                        /* disabled? */
                return NULL;
            u = (uint32) get_uint (tptr, 10, dptr->numunits - 1, &r);
            if (r != SCPE_OK)                           /* error? */
                *uptr = NULL;
            else
                *uptr = dptr->units + u;
            return dptr;
            }
        }
    for (u = 0; u < dptr->numunits; u++) {
        if (0 == strcmp (cptr, sim_uname (&dptr->units[u]))) {
            *uptr = &dptr->units[u];
            return dptr;
            }
        }
    }
return NULL;
}

/* sim_register_internal_device   Add device to internal device list

   Inputs:
        dptr    =       pointer to device
*/

t_stat sim_register_internal_device (DEVICE *dptr)
{
uint32 i;

for (i = 0; (sim_devices[i] != NULL); i++)
    if (sim_devices[i] == dptr)
        return SCPE_OK;
for (i = 0; i < sim_internal_device_count; i++)
    if (sim_internal_devices[i] == dptr)
        return SCPE_OK;
++sim_internal_device_count;
sim_internal_devices = (DEVICE **)realloc(sim_internal_devices, (sim_internal_device_count+1)*sizeof(*sim_internal_devices));
if (!sim_internal_devices)
  {
    (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                   __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
sim_internal_devices[sim_internal_device_count-1] = dptr;
sim_internal_devices[sim_internal_device_count] = NULL;
return SCPE_OK;
}

/* Find_dev_from_unit   find device for unit

   Inputs:
        uptr    =       pointer to unit
   Outputs:
        result  =       pointer to device
*/

DEVICE *find_dev_from_unit (UNIT *uptr)
{
DEVICE *dptr;
uint32 i, j;

if (uptr == NULL)
    return NULL;
for (i = 0; (dptr = sim_devices[i]) != NULL; i++) {
    for (j = 0; j < dptr->numunits; j++) {
        if (uptr == (dptr->units + j))
            return dptr;
        }
    }
for (i = 0; i<sim_internal_device_count; i++) {
    dptr = sim_internal_devices[i];
    for (j = 0; j < dptr->numunits; j++) {
        if (uptr == (dptr->units + j))
            return dptr;
        }
    }
return NULL;
}

/* Test for disabled device */

t_bool qdisable (DEVICE *dptr)
{
return (dptr->flags & DEV_DIS? TRUE: FALSE);
}

/* find_reg_glob        find globally unique register

   Inputs:
        cptr    =       pointer to input string
        optr    =       pointer to output pointer (can be null)
        gdptr   =       pointer to global device
   Outputs:
        result  =       pointer to register, NULL if error
        *optr   =       pointer to next character in input string
        *gdptr  =       pointer to device where found
*/

REG *find_reg_glob (CONST char *cptr, CONST char **optr, DEVICE **gdptr)
{
int32 i;
DEVICE *dptr;
REG *rptr, *srptr = NULL;

*gdptr = NULL;
for (i = 0; (dptr = sim_devices[i]) != 0; i++) {        /* all dev */
    if (dptr->flags & DEV_DIS)                          /* skip disabled */
        continue;
    if ((rptr = find_reg (cptr, optr, dptr))) {         /* found? */
        if (srptr)                                      /* ambig? err */
            return NULL;
        srptr = rptr;                                   /* save reg */
        *gdptr = dptr;                                  /* save unit */
        }
    }
return srptr;
}

/* find_reg             find register matching input string

   Inputs:
        cptr    =       pointer to input string
        optr    =       pointer to output pointer (can be null)
        dptr    =       pointer to device
   Outputs:
        result  =       pointer to register, NULL if error
        *optr   =       pointer to next character in input string
*/

REG *find_reg (CONST char *cptr, CONST char **optr, DEVICE *dptr)
{
CONST char *tptr;
REG *rptr;
size_t slnt;

if ((cptr == NULL) || (dptr == NULL) || (dptr->registers == NULL))
    return NULL;
tptr = cptr;
do {
    tptr++;
    } while (sim_isalnum (*tptr) || (*tptr == '*') || (*tptr == '_') || (*tptr == '.'));
slnt = tptr - cptr;
for (rptr = dptr->registers; rptr->name != NULL; rptr++) {
    if ((slnt == strlen (rptr->name)) &&
        (strncmp (cptr, rptr->name, slnt) == 0)) {
        if (optr != NULL)
            *optr = tptr;
        return rptr;
        }
    }
return NULL;
}

/* get_switches         get switches from input string

   Inputs:
        cptr    =       pointer to input string
   Outputs:
        sw      =       switch bit mask
                        0 if no switches, -1 if error
*/

int32 get_switches (const char *cptr)
{
int32 sw;

if (*cptr != '-')
    return 0;
sw = 0;
for (cptr++; (sim_isspace (*cptr) == 0) && (*cptr != 0); cptr++) {
    if (sim_isalpha (*cptr) == 0)
        return -1;
    sw = sw | SWMASK (toupper (*cptr));
    }
return sw;
}

/* get_sim_sw           accumulate sim_switches

   Inputs:
        cptr    =       pointer to input string
   Outputs:
        ptr     =       pointer to first non-string glyph
                        NULL if error
*/

CONST char *get_sim_sw (CONST char *cptr)
{
int32 lsw;
char gbuf[CBUFSIZE];

while (*cptr == '-') {                                  /* while switches */
    cptr = get_glyph (cptr, gbuf, 0);                   /* get switch glyph */
    lsw = get_switches (gbuf);                          /* parse */
    if (lsw <= 0)                                       /* invalid? */
        return NULL;
    sim_switches = sim_switches | lsw;                  /* accumulate */
    }
return cptr;
}

/* get_sim_opt          get simulator command options

   Inputs:
        opt     =       command options
        cptr    =       pointer to input string
   Outputs:
        ptr     =       pointer to next glyph, NULL if error
        *stat   =       error status
*/

CONST char *get_sim_opt (int32 opt, CONST char *cptr, t_stat *st)
{
int32 t;
char gbuf[CBUFSIZE];
CONST char *svptr;
DEVICE *tdptr;
UNIT *tuptr;

sim_switches = 0;                                       /* no switches */
sim_ofile = NULL;                                       /* no output file */
sim_schrptr = NULL;                                     /* no search */
sim_schaptr = NULL;                                     /* no search */
sim_stabr.logic = sim_staba.logic = SCH_OR;             /* default search params */
sim_stabr.boolop = sim_staba.boolop = SCH_GE;
sim_stabr.count = 1;
sim_stabr.mask = (t_value *)realloc (sim_stabr.mask, sim_emax * sizeof(*sim_stabr.mask));
if (!sim_stabr.mask)
  {
    (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                   __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
(void)memset (sim_stabr.mask, 0, sim_emax * sizeof(*sim_stabr.mask));
sim_stabr.comp = (t_value *)realloc (sim_stabr.comp, sim_emax * sizeof(*sim_stabr.comp));
if (!sim_stabr.comp)
  {
    (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                   __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
(void)memset (sim_stabr.comp, 0, sim_emax * sizeof(*sim_stabr.comp));
sim_staba.count = sim_emax;
sim_staba.mask = (t_value *)realloc (sim_staba.mask, sim_emax * sizeof(*sim_staba.mask));
if (!sim_staba.mask)
  {
    (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                   __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
(void)memset (sim_staba.mask, 0, sim_emax * sizeof(*sim_staba.mask));
sim_staba.comp = (t_value *)realloc (sim_staba.comp, sim_emax * sizeof(*sim_staba.comp));
if (!sim_staba.comp)
  {
    (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                   __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
(void)memset (sim_staba.comp, 0, sim_emax * sizeof(*sim_staba.comp));
if (! sim_dflt_dev)
  return NULL;
sim_dfdev = sim_dflt_dev;
sim_dfunit = sim_dfdev->units;
sim_opt_out = 0;                                        /* no options yet */
*st = SCPE_OK;
while (*cptr) {                                         /* loop through modifiers */
    svptr = cptr;                                       /* save current position */
    if ((opt & CMD_OPT_OF) && (*cptr == '@')) {         /* output file spec? */
        if (sim_ofile) {                                /* already got one? */
            fclose (sim_ofile);                         /* one per customer */
            *st = SCPE_ARG;
            return NULL;
            }
        cptr = get_glyph (cptr + 1, gbuf, 0);
        sim_ofile = sim_fopen (gbuf, "a");              /* open for append */
        if (sim_ofile == NULL) {                        /* open failed? */
            *st = SCPE_OPENERR;
            return NULL;
            }
        sim_opt_out |= CMD_OPT_OF;                      /* got output file */
        continue;
        }
    cptr = get_glyph (cptr, gbuf, 0);
    if ((t = get_switches (gbuf)) != 0) {               /* try for switches */
        if (t < 0) {                                    /* err if bad switch */
            *st = SCPE_INVSW;
            return NULL;
            }
        sim_switches = sim_switches | t;                /* or in new switches */
        }
    else if ((opt & CMD_OPT_SCH) &&                     /* if allowed, */
        get_rsearch (gbuf, sim_dfdev->dradix, &sim_stabr)) { /* try for search */
        sim_schrptr = &sim_stabr;                       /* set search */
        sim_schaptr = get_asearch (gbuf, sim_dfdev->dradix, &sim_staba);/* populate memory version of the same expression */
        sim_opt_out |= CMD_OPT_SCH;                     /* got search */
        }
    else if ((opt & CMD_OPT_DFT) &&                     /* default allowed? */
        ((sim_opt_out & CMD_OPT_DFT) == 0) &&           /* none yet? */
        (tdptr = find_unit (gbuf, &tuptr)) &&           /* try for default */
        (tuptr != NULL)) {
        sim_dfdev = tdptr;                              /* set as default */
        sim_dfunit = tuptr;
        sim_opt_out |= CMD_OPT_DFT;                     /* got default */
        }
    else return svptr;                                  /* not rec, break out */
    }
return cptr;
}

/* put_switches         put switches into string

   Inputs:
        buf     =       pointer to string buffer
        bufsize =       size of string buffer
        sw      =       switch bit mask
   Outputs:
        buf     =       buffer with switches converted to text
*/

const char *put_switches (char *buf, size_t bufsize, uint32 sw)
{
char *optr = buf;
int32 bit;

(void)memset (buf, 0, bufsize);
if ((sw == 0) || (bufsize < 3))
    return buf;
--bufsize;                          /* leave room for terminating NUL */
*optr++ = '-';
for (bit=0; bit <= ('Z'-'A'); bit++)
    if (sw & (1 << bit))
        if ((size_t)(optr - buf) < bufsize)
            *optr++ = 'A' + bit;
return buf;
}

/* Get register search specification

   Inputs:
        cptr    =       pointer to input string
        radix   =       radix for numbers
        schptr =        pointer to search table
   Outputs:
        return =        NULL if error
                        schptr if valid search specification
*/

SCHTAB *get_rsearch (CONST char *cptr, int32 radix, SCHTAB *schptr)
{
int32 c;
size_t logop, cmpop;
t_value logval, cmpval;
const char *sptr;
CONST char *tptr;
const char logstr[] = "|&^", cmpstr[] = "=!><";
/* Using a const instead of a #define. */
const size_t invalid_op = (size_t) -1;

logval = cmpval = 0;
if (*cptr == 0)                                         /* check for clause */
    return NULL;
/*LINTED E_EQUALITY_NOT_ASSIGNMENT*/
for (logop = cmpop = invalid_op; (c = *cptr++); ) {     /* loop thru clauses */
    if ((sptr = strchr (logstr, c))) {                  /* check for mask */
        logop = sptr - logstr;
        logval = strtotv (cptr, &tptr, radix);
        if (cptr == tptr)
            return NULL;
        cptr = tptr;
        }
    else if ((sptr = strchr (cmpstr, c))) {             /* check for boolop */
        cmpop = sptr - cmpstr;
        if (*cptr == '=') {
            cmpop = cmpop + strlen (cmpstr);
            cptr++;
            }
        cmpval = strtotv (cptr, &tptr, radix);
        if (cptr == tptr)
            return NULL;
        cptr = tptr;
        }
    else return NULL;
    }                                                   /* end for */
if (schptr->count != 1) {
    FREE (schptr->mask);
    schptr->mask = (t_value *)calloc (sim_emax, sizeof(*schptr->mask));
    FREE (schptr->comp);
    schptr->comp = (t_value *)calloc (sim_emax, sizeof(*schptr->comp));
    }
if (logop != invalid_op) {
    schptr->logic = logop;
    schptr->mask[0] = logval;
    }
if (cmpop != invalid_op) {
    schptr->boolop = cmpop;
    schptr->comp[0] = cmpval;
    }
schptr->count = 1;
return schptr;
}

/* Get memory search specification

   Inputs:
        cptr    =       pointer to input string
        radix   =       radix for numbers
        schptr =        pointer to search table
   Outputs:
        return =        NULL if error
                        schptr if valid search specification
*/

SCHTAB *get_asearch (CONST char *cptr, int32 radix, SCHTAB *schptr)
{
int32 c;
size_t logop, cmpop;
t_value *logval, *cmpval;
t_stat reason = SCPE_OK;
CONST char *ocptr = cptr;
const char *sptr;
char gbuf[CBUFSIZE];
const char logstr[] = "|&^", cmpstr[] = "=!><";
/* Using a const instead of a #define. */
const size_t invalid_op = (size_t) -1;

if (*cptr == 0)                                         /* check for clause */
    return NULL;
logval = (t_value *)calloc (sim_emax, sizeof(*logval));
cmpval = (t_value *)calloc (sim_emax, sizeof(*cmpval));
/*LINTED E_EQUALITY_NOT_ASSIGNMENT*/
for (logop = cmpop = invalid_op; (c = *cptr++); ) {     /* loop thru clauses */
    if (NULL != (sptr = strchr (logstr, c))) {          /* check for mask */
        logop = sptr - logstr;
        cptr = get_glyph (cptr, gbuf, 0);
        reason = parse_sym (gbuf, 0, sim_dfunit, logval, sim_switches);
        if (reason > 0) {
            FREE (logval);
            FREE (cmpval);
            return get_rsearch (ocptr, radix, schptr);
            }
        }
    else if (NULL != (sptr = strchr (cmpstr, c))) {     /* check for boolop */
        cmpop = sptr - cmpstr;
        if (*cptr == '=') {
            cmpop = cmpop + strlen (cmpstr);
            cptr++;
            }
        cptr = get_glyph (cptr, gbuf, 0);
        reason = parse_sym (gbuf, 0, sim_dfunit, cmpval, sim_switches);
        if (reason > 0) {
            FREE (logval);
            FREE (cmpval);
            return get_rsearch (ocptr, radix, schptr);
            }
        }
    else {
        FREE (logval);
        FREE (cmpval);
        return NULL;
        }
    }                                                   /* end for */
if (schptr->count != (uint32)(1 - reason)) {
    schptr->count = 1 - reason;
    FREE (schptr->mask);
    schptr->mask = (t_value *)calloc (sim_emax, sizeof(*schptr->mask));
    FREE (schptr->comp);
    schptr->comp = (t_value *)calloc (sim_emax, sizeof(*schptr->comp));
    }
if (logop != invalid_op) {
    schptr->logic = logop;
    FREE (schptr->mask);
    schptr->mask = logval;
    }
else {
    FREE (logval);
    }
if (cmpop != invalid_op) {
    schptr->boolop = cmpop;
    FREE (schptr->comp);
    schptr->comp = cmpval;
    }
else {
    FREE (cmpval);
    }
return schptr;
}

/* Test value against search specification

   Inputs:
        val    =        value list to test
        schptr =        pointer to search table
   Outputs:
        return =        1 if value passes search criteria, 0 if not
*/

int32 test_search (t_value *values, SCHTAB *schptr)
{
t_value *val = NULL;
int32 i, updown;
int32 ret = 0;

if (schptr == NULL)
    return ret;

val = (t_value *)malloc (schptr->count * sizeof (*values));
if (!val)
  {
    (void)fprintf(stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                  __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
for (i=0; i<(int32)schptr->count; i++) {
    val[i] = values[i];
    switch (schptr->logic) {                            /* case on logical */

        case SCH_OR:
            val[i] = val[i] | schptr->mask[i];
            break;

        case SCH_AND:
            val[i] = val[i] & schptr->mask[i];
            break;

        case SCH_XOR:
            val[i] = val[i] ^ schptr->mask[i];
            break;
            }
    }

ret = 1;
if (1) {    /* Little Endian VM */
    updown = -1;
    i=schptr->count-1;
    }
else {      /* Big Endian VM */
    updown = 1;
    i=0;
    }
for (; (i>=0) && (i<(int32)schptr->count) && ret; i += updown) {
    switch (schptr->boolop) {                           /* case on comparison */

        case SCH_E: case SCH_EE:
            if (val[i] != schptr->comp[i])
                ret = 0;
            break;

        case SCH_N: case SCH_NE:
            if (val[i] == schptr->comp[i])
                ret = 0;
            break;

        case SCH_G:
            if (val[i] <= schptr->comp[i])
                ret = 0;
            break;

        case SCH_GE:
            if (val[i] < schptr->comp[i])
                ret = 0;
            break;

        case SCH_L:
            if (val[i] >= schptr->comp[i])
                ret = 0;
            break;

        case SCH_LE:
            if (val[i] > schptr->comp[i])
                ret = 0;
            break;
        }
    }
FREE (val);
return ret;
}

/* Radix independent input/output package

   strtotv - general radix input routine

   Inputs:
        inptr   =       string to convert
        endptr  =       pointer to first unconverted character
        radix   =       radix for input
   Outputs:
        value   =       converted value

   On an error, the endptr will equal the inptr.
*/

t_value strtotv (CONST char *inptr, CONST char **endptr, uint32 radix)
{
int32 nodigit;
t_value val;
uint32 c, digit;

*endptr = inptr;                                        /* assume fails */
if ((radix < 2) || (radix > 36))
    return 0;
while (sim_isspace (*inptr))                            /* bypass white space */
    inptr++;
val = 0;
nodigit = 1;
for (c = *inptr; sim_isalnum(c); c = *++inptr) {        /* loop through char */
    if (sim_islower (c))
        c = toupper (c);
    if (sim_isdigit (c))                                /* digit? */
        digit = c - (uint32) '0';
    else if (radix <= 10)                               /* stop if not expected */
        break;
    else digit = c + 10 - (uint32) 'A';                 /* convert letter */
    if (digit >= radix)                                 /* valid in radix? */
        return 0;
    val = (val * radix) + digit;                        /* add to value */
    nodigit = 0;
    }
if (nodigit)                                            /* no digits? */
    return 0;
*endptr = inptr;                                        /* result pointer */
return val;
}

/* fprint_val - general radix printing routine

   Inputs:
        stream  =       stream designator
        val     =       value to print
        radix   =       radix to print
        width   =       width to print
        format  =       leading zeroes format
   Outputs:
        status  =       error status
        if stream is NULL, returns length of output that would
        have been generated.
*/

t_stat sprint_val (char *buffer, t_value val, uint32 radix,
                   size_t width, uint32 format)
{
#define MAX_WIDTH ((CHAR_BIT * sizeof (t_value) * 4 + 3) / 3)
t_value owtest, wtest;
size_t d;
size_t digit;
size_t ndigits;
size_t commas = 0;
char dbuf[MAX_WIDTH + 1];

for (d = 0; d < MAX_WIDTH; d++)
    dbuf[d] = (format == PV_RZRO)? '0': ' ';
dbuf[MAX_WIDTH] = 0;
d = MAX_WIDTH;
do {
    d = d - 1;
    digit = val % radix;
    val = val / radix;
    dbuf[d] = (char)((digit <= 9)? '0' + digit: 'A' + (digit - 10));
    } while ((d > 0) && (val != 0));

switch (format) {
    case PV_LEFT:
        break;
    case PV_RZRO:
        wtest = owtest = radix;
        ndigits = 1;
        while ((wtest < width_mask[width]) && (wtest >= owtest)) {
            owtest = wtest;
            wtest = wtest * radix;
            ndigits = ndigits + 1;
            }
        if ((MAX_WIDTH - (ndigits + commas)) < d)
            d = MAX_WIDTH - (ndigits + commas);
        break;
    }
if (NULL == buffer)                   /* Potentially unsafe wraparound if */
    return ((t_stat) strlen(dbuf+d)); /*  sizeof(t_stat) < sizeof(size_t) */
*buffer = '\0';
if (width < strlen(dbuf+d))
    return SCPE_IOERR;
strcpy(buffer, dbuf+d);
return SCPE_OK;
}

t_stat fprint_val (FILE *stream, t_value val, uint32 radix,
    uint32 width, uint32 format)
{
char dbuf[MAX_WIDTH + 1];

if (!stream)
    return sprint_val (NULL, val, radix, width, format);
if (width > MAX_WIDTH)
    width = MAX_WIDTH;
sprint_val (dbuf, val, radix, width, format);
if (Fprintf (stream, "%s", dbuf) < 0)
    return SCPE_IOERR;
return SCPE_OK;
}

const char *sim_fmt_secs (double seconds)
{
static char buf[60];
char frac[16] = "";
const char *sign = "";
double val = seconds;
double days, hours, mins, secs, msecs, usecs;

if (val == 0.0)
    return "";
if (val < 0.0) {
    sign = "-";
    val = -val;
    }
days = floor (val / (24.0*60.0*60.0));
val -= (days * 24.0*60.0*60.0);
hours = floor (val / (60.0*60.0));
val -= (hours * 60.0 * 60.0);
mins = floor (val / 60.0);
val -= (mins * 60.0);
secs = floor (val);
val -= secs;
val *= 1000.0;
msecs = floor (val);
val -= msecs;
val *= 1000.0;
usecs = floor (val+0.5);
if (usecs == 1000.0) {
    usecs = 0.0;
    msecs += 1;
    }
if ((msecs > 0.0) || (usecs > 0.0)) {
    (void)sprintf (frac, ".%03.0f%03.0f", msecs, usecs);
    while (frac[strlen (frac) - 1] == '0') //-V557
        frac[strlen (frac) - 1] = '\0'; //-V557
    if (strlen (frac) == 1)
        frac[0] = '\0';
    }
if (days > 0)
    (void)sprintf (buf, "%s%.0f day%s %02.0f:%02.0f:%02.0f%s hour%s",
                   sign, days, (days != 1) ? "s" : "", hours, mins,
                   secs, frac, (days == 1) ? "s" : "");
else
    if (hours > 0)
        (void)sprintf (buf, "%s%.0f:%02.0f:%02.0f%s hour",
                       sign, hours, mins, secs, frac);
    else
        if (mins > 0)
            (void)sprintf (buf, "%s%.0f:%02.0f%s minute",
                           sign, mins, secs, frac);
        else
            if (secs > 0)
                (void)sprintf (buf, "%s%.0f%s second",
                               sign, secs, frac);
            else
                if (msecs > 0) {
                    if (usecs > 0)
                        (void)sprintf (buf, "%s%.0f.%s msec",
                                       sign, msecs, frac+4);
                    else
                        (void)sprintf (buf, "%s%.0f msec",
                                       sign, msecs);
                    }
                else
                    (void)sprintf (buf, "%s%.0f usec",
                                   sign, usecs);
if (0 != strncmp ("1 ", buf, 2))
    strcpy (&buf[strlen (buf)], "s");
return buf;
}

/*
 * Event queue package
 *
 *      sim_activate            add entry to event queue
 *      sim_activate_abs        add entry to event queue even if event already scheduled
 *      sim_activate_after      add entry to event queue after a specified amount of wall time
 *      sim_cancel              remove entry from event queue
 *      sim_process_event       process entries on event queue
 *      sim_is_active           see if entry is on event queue
 *      sim_activate_time       return time until activation
 *      sim_atime               return absolute time for an entry
 *      sim_gtime               return global time
 *      sim_qcount              return event queue entry count
 */

t_stat sim_process_event (void)
{
UNIT *uptr;
t_stat reason;
#if defined(TESTING)
cpu_state_t * cpup = _cpup;
#endif

if (stop_cpu)                                           /* stop CPU? */
  {
#if defined(WIN_STDIO)
    (void)fflush(stdout);
    (void)fflush(stderr);
#endif /* if defined(WIN_STDIO) */
    return SCPE_STOP;
  }
UPDATE_SIM_TIME;                                        /* update sim time */

if (sim_clock_queue == QUEUE_LIST_END) {                /* queue empty? */
    sim_interval = noqueue_time = NOQUEUE_WAIT;         /* flag queue empty */
    sim_debug (SIM_DBG_EVENT, sim_dflt_dev,
               "Queue Empty New Interval = %d\n",
               sim_interval);
    return SCPE_OK;
    }
sim_processing_event = TRUE;
do {
    uptr = sim_clock_queue;                             /* get first */
    sim_clock_queue = uptr->next;                       /* remove first */
    uptr->next = NULL;                                  /* hygiene */
    sim_interval -= uptr->time;
    uptr->time = 0;
    if (sim_clock_queue != QUEUE_LIST_END)
        sim_interval = sim_clock_queue->time;
    else
        sim_interval = noqueue_time = NOQUEUE_WAIT;
    sim_debug (SIM_DBG_EVENT, sim_dflt_dev,
               "Processing Event for %s\n", sim_uname (uptr));
    if (uptr->action != NULL)
        reason = uptr->action (uptr);
    else
        reason = SCPE_OK;
    } while ((reason == SCPE_OK) &&
             (sim_interval <= 0) &&
             (sim_clock_queue != QUEUE_LIST_END) &&
             (!stop_cpu));

if (sim_clock_queue == QUEUE_LIST_END) {                /* queue empty? */
    sim_interval = noqueue_time = NOQUEUE_WAIT;         /* flag queue empty */
    sim_debug (SIM_DBG_EVENT, sim_dflt_dev,
               "Processing Queue Complete New Interval = %d\n",
               sim_interval);
    }
else
    sim_debug (SIM_DBG_EVENT, sim_dflt_dev,
               "Processing Queue Complete New Interval = %d(%s)\n",
               sim_interval, sim_uname(sim_clock_queue));

if ((reason == SCPE_OK) && stop_cpu)
  {
#if defined(WIN_STDIO)
    (void)fflush(stdout);
    (void)fflush(stderr);
#endif /* if defined(WIN_STDIO) */
    reason = SCPE_STOP;
  }
sim_processing_event = FALSE;
return reason;
}

/* sim_activate - activate (queue) event

   Inputs:
        uptr    =       pointer to unit
        event_time =    relative timeout
   Outputs:
        reason  =       result (SCPE_OK if ok)
*/

t_stat sim_activate (UNIT *uptr, int32 event_time)
{
if (uptr->dynflags & UNIT_TMR_UNIT)
    return sim_timer_activate (uptr, event_time);
return _sim_activate (uptr, event_time);
}

t_stat _sim_activate (UNIT *uptr, int32 event_time)
{
UNIT *cptr, *prvptr;
int32 accum;
#if defined(TESTING)
cpu_state_t * cpup = _cpup;
#endif

if (sim_is_active (uptr))                               /* already active? */
    return SCPE_OK;
UPDATE_SIM_TIME;                                        /* update sim time */

sim_debug (SIM_DBG_ACTIVATE, sim_dflt_dev, "Activating %s delay=%d\n", sim_uname (uptr), event_time);

prvptr = NULL;
accum = 0;
for (cptr = sim_clock_queue; cptr != QUEUE_LIST_END; cptr = cptr->next) {
    if (event_time < (accum + cptr->time))
        break;
    accum = accum + cptr->time;
    prvptr = cptr;
    }
if (prvptr == NULL) {                                   /* insert at head */
    cptr = uptr->next = sim_clock_queue;
    sim_clock_queue = uptr;
    }
else {
    cptr = uptr->next = prvptr->next;                   /* insert at prvptr */
    prvptr->next = uptr;
    }
uptr->time = event_time - accum;
if (cptr != QUEUE_LIST_END)
    cptr->time = cptr->time - uptr->time;
sim_interval = sim_clock_queue->time;
return SCPE_OK;
}

/* sim_activate_abs - activate (queue) event even if event already scheduled

   Inputs:
        uptr    =       pointer to unit
        event_time =    relative timeout
   Outputs:
        reason  =       result (SCPE_OK if ok)
*/

t_stat sim_activate_abs (UNIT *uptr, int32 event_time)
{
sim_cancel (uptr);
return _sim_activate (uptr, event_time);
}

/* sim_activate_after - activate (queue) event

   Inputs:
        uptr    =       pointer to unit
        usec_delay =    relative timeout (in microseconds)
   Outputs:
        reason  =       result (SCPE_OK if ok)
*/

t_stat sim_activate_after (UNIT *uptr, uint32 usec_delay)
{
return _sim_activate_after (uptr, usec_delay);
}

t_stat _sim_activate_after (UNIT *uptr, uint32 usec_delay)
{
if (sim_is_active (uptr))                               /* already active? */
    return SCPE_OK;
return sim_timer_activate_after (uptr, usec_delay);
}

/* sim_cancel - cancel (dequeue) event

   Inputs:
        uptr    =       pointer to unit
   Outputs:
        reason  =       result (SCPE_OK if ok)

*/

t_stat sim_cancel (UNIT *uptr)
{
UNIT *cptr, *nptr;
#if defined(TESTING)
cpu_state_t * cpup = _cpup;
#endif

if (sim_clock_queue == QUEUE_LIST_END)
    return SCPE_OK;
if (!sim_is_active (uptr))
    return SCPE_OK;
UPDATE_SIM_TIME;                                        /* update sim time */
sim_debug (SIM_DBG_EVENT, sim_dflt_dev, "Canceling Event for %s\n", sim_uname(uptr));
nptr = QUEUE_LIST_END;

if (sim_clock_queue == uptr) {
    nptr = sim_clock_queue = uptr->next;
    uptr->next = NULL;                                  /* hygiene */
    }
else {
    for (cptr = sim_clock_queue; cptr != QUEUE_LIST_END; cptr = cptr->next) {
        if (cptr->next == uptr) {
            nptr = cptr->next = uptr->next;
            uptr->next = NULL;                          /* hygiene */
            break;                                      /* end queue scan */
            }
        }
    }
if (nptr != QUEUE_LIST_END)
    nptr->time += (uptr->next) ? 0 : uptr->time;
if (!uptr->next)
    uptr->time = 0;
if (sim_clock_queue != QUEUE_LIST_END)
    sim_interval = sim_clock_queue->time;
else sim_interval = noqueue_time = NOQUEUE_WAIT;
if (uptr->next) {
    sim_printf ("\rCancel failed for '%s'!\r\n", sim_uname(uptr));
    if (sim_deb)
        fclose(sim_deb);
    (void)fprintf (stderr, "\rFATAL: Bugcheck! Aborting at %s[%s:%d]\r\n",
                   __func__, __FILE__, __LINE__);
    abort ();
    }
return SCPE_OK;
}

/* sim_is_active - test for entry in queue

   Inputs:
        uptr    =       pointer to unit
   Outputs:
        result =        TRUE if unit is busy, FALSE inactive
*/

t_bool sim_is_active (UNIT *uptr)
{
if (uptr->next == NULL)
  return FALSE;
else
return TRUE;
}

/* sim_activate_time - return activation time

   Inputs:
        uptr    =       pointer to unit
   Outputs:
        result =        absolute activation time + 1, 0 if inactive
*/

int32 sim_activate_time (UNIT *uptr)
{
UNIT *cptr;
int32 accum = 0;

for (cptr = sim_clock_queue; cptr != QUEUE_LIST_END; cptr = cptr->next) {
    if (cptr == sim_clock_queue) {
        if (sim_interval > 0)
            accum = accum + sim_interval;
        }
    else
        accum = accum + cptr->time;
    if (cptr == uptr)
        return accum + 1;
    }
return 0;
}

/* sim_gtime - return global time

   Inputs: none
   Outputs:
        time    =       global time
*/

double sim_gtime (void)
{
return sim_time;
}

/* sim_qcount - return queue entry count

   Inputs: none
   Outputs:
        count   =       number of entries on the queue
*/

int32 sim_qcount (void)
{
int32 cnt;
UNIT *uptr;

cnt = 0;
for (uptr = sim_clock_queue; uptr != QUEUE_LIST_END; uptr = uptr->next)
    cnt++;
return cnt;
}

/* Breakpoint package.  This module replaces the VM-implemented one
   instruction breakpoint capability.

   Breakpoints are stored in table sim_brk_tab, which is ordered by address for
   efficient binary searching.  A breakpoint consists of a six entry structure:

        addr                    address of the breakpoint
        type                    types of breakpoints set on the address
                                a bit mask representing letters A-Z
        cnt                     number of iterations before breakp is taken
        action                  pointer command string to be executed
                                when break is taken
        next                    list of other breakpoints with the same addr specifier
        time_fired              array of when this breakpoint was fired for each class

   sim_brk_summ is a summary of the types of breakpoints that are currently set (it
   is the bitwise OR of all the type fields).  A simulator need only check for
   a breakpoint of type X if bit SWMASK('X') is set in sim_brk_summ.

   The package contains the following public routines:

        sim_brk_init            initialize
        sim_brk_set             set breakpoint
        sim_brk_clr             clear breakpoint
        sim_brk_clrall          clear all breakpoints
        sim_brk_show            show breakpoint
        sim_brk_showall         show all breakpoints
        sim_brk_test            test for breakpoint
        sim_brk_npc             PC has been changed
        sim_brk_getact          get next action
        sim_brk_clract          clear pending actions

   Initialize breakpoint system.
*/

t_stat sim_brk_init (void)
{
int32 i;

for (i=0; i<sim_brk_lnt; i++) {
    BRKTAB *bp;
    if (sim_brk_tab)
      {
        bp = sim_brk_tab[i];

        while (bp)
          {
            BRKTAB *bpt = bp->next;

            FREE (bp->act);
            FREE (bp);
            bp = bpt;
          }
      }
}
if (sim_brk_tab != NULL)
    (void)memset (sim_brk_tab, 0, sim_brk_lnt*sizeof (BRKTAB*));
sim_brk_lnt = SIM_BRK_INILNT;
sim_brk_tab = (BRKTAB **) realloc (sim_brk_tab, sim_brk_lnt*sizeof (BRKTAB*));
if (sim_brk_tab == NULL)
    return SCPE_MEM;
(void)memset (sim_brk_tab, 0, sim_brk_lnt*sizeof (BRKTAB*));
sim_brk_ent = sim_brk_ins = 0;
sim_brk_clract ();
sim_brk_npc (0);
return SCPE_OK;
}

/* Search for a breakpoint in the sorted breakpoint table */

BRKTAB *sim_brk_fnd (t_addr loc)
{
int32 lo, hi, p;
BRKTAB *bp;

if (sim_brk_ent == 0) {                                 /* table empty? */
    sim_brk_ins = 0;                                    /* insrt at head */
    return NULL;                                        /* sch fails */
    }
lo = 0;                                                 /* initial bounds */
hi = sim_brk_ent - 1;
do {
    p = (lo + hi) >> 1;                                 /* probe */
    bp = sim_brk_tab[p];                                /* table addr */
    if (loc == bp->addr) {                              /* match? */
        sim_brk_ins = p;
        return bp;
        }
    else if (loc < bp->addr)                            /* go down? p is upper */
        hi = p - 1;
    else lo = p + 1;                                    /* go up? p is lower */
    } while (lo <= hi);
if (loc < bp->addr)                                     /* insrt before or */
    sim_brk_ins = p;
else sim_brk_ins = p + 1;                               /* after last sch */
return NULL;
}

BRKTAB *sim_brk_fnd_ex (t_addr loc, uint32 btyp, t_bool any_typ, uint32 spc)
{
BRKTAB *bp = sim_brk_fnd (loc);

while (bp) {
    if (any_typ ? ((bp->typ & btyp) && (bp->time_fired[spc] != sim_gtime())) :
                  (bp->typ == btyp))
        return bp;
    bp = bp->next;
    }
return bp;
}

/* Insert a breakpoint */

BRKTAB *sim_brk_new (t_addr loc, uint32 btyp)
{
int32 i, t;
BRKTAB *bp, **newp;

if (sim_brk_ins < 0)
    return NULL;
if (sim_brk_ent >= sim_brk_lnt) {                       /* out of space? */
    t = sim_brk_lnt + SIM_BRK_INILNT;                   /* new size */
    newp = (BRKTAB **) calloc (t, sizeof (BRKTAB*));    /* new table */
    if (newp == NULL)                                   /* can't extend */
        return NULL;
    memcpy (newp, sim_brk_tab, sim_brk_lnt * sizeof (*sim_brk_tab));/* copy table */
    (void)memset (newp + sim_brk_lnt, 0, SIM_BRK_INILNT * sizeof (*newp));/* zero new entries */
    FREE (sim_brk_tab);                                 /* free old table */
    sim_brk_tab = newp;                                 /* new base, lnt */
    sim_brk_lnt = t;
    }
if ((sim_brk_ins == sim_brk_ent) ||
    ((sim_brk_ins != sim_brk_ent) && //-V728
     (sim_brk_tab[sim_brk_ins]->addr != loc))) {        /* need to open a hole? */
    for (i = sim_brk_ent; i > sim_brk_ins; --i)
        sim_brk_tab[i] = sim_brk_tab[i - 1];
    sim_brk_tab[sim_brk_ins] = NULL;
    }
bp = (BRKTAB *)calloc (1, sizeof (*bp));
if (!bp)
  {
    (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                   __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
bp->next = sim_brk_tab[sim_brk_ins];
sim_brk_tab[sim_brk_ins] = bp;
if (bp->next == NULL)
    sim_brk_ent += 1;
bp->addr = loc;
bp->typ = btyp;
bp->cnt = 0;
bp->act = NULL;
for (i = 0; i < SIM_BKPT_N_SPC; i++)
    bp->time_fired[i] = -1.0;
return bp;
}

/* Set a breakpoint of type sw */

t_stat sim_brk_set (t_addr loc, int32 sw, int32 ncnt, CONST char *act)
{
BRKTAB *bp;

if ((sw == 0) || (sw == BRK_TYP_DYN_STEPOVER))
    sw |= sim_brk_dflt;
if (~sim_brk_types & sw) {
    char gbuf[CBUFSIZE];

    return sim_messagef (SCPE_NOFNC, "Unknown breakpoint type; %s\n", put_switches(gbuf, sizeof(gbuf), sw & ~sim_brk_types));
    }
if ((sw & BRK_TYP_DYN_ALL) && act)                      /* can't specify an action with a dynamic breakpoint */
    return SCPE_ARG;
bp = sim_brk_fnd (loc);                                 /* loc present? */
if (!bp)                                                /* no, allocate */
    bp = sim_brk_new (loc, sw);
else {
    while (bp && (bp->typ != (uint32)sw))
        bp = bp->next;
    if (!bp)
        bp = sim_brk_new (loc, sw);
    }
if (!bp)                                                /* still no? mem err */
    return SCPE_MEM;
bp->cnt = ncnt;                                         /* set count */
if ((!(sw & BRK_TYP_DYN_ALL)) &&                        /* Not Dynamic and */
    (bp->act != NULL) && (act != NULL)) {               /* replace old action? */
    FREE (bp->act);                                     /* deallocate */
    bp->act = NULL;                                     /* now no action */
    }
if ((act != NULL) && (*act != 0)) {                     /* new action? */
    char *newp = (char *) calloc (CBUFSIZE+1, sizeof (char)); /* alloc buf */
    if (newp == NULL)                                   /* mem err? */
        return SCPE_MEM;
    strncpy (newp, act, CBUFSIZE);                      /* copy action */
    bp->act = newp;                                     /* set pointer */
    }
sim_brk_summ = sim_brk_summ | (sw & ~BRK_TYP_TEMP);
return SCPE_OK;
}

/* Clear a breakpoint */

t_stat sim_brk_clr (t_addr loc, int32 sw)
{
BRKTAB *bpl = NULL;
BRKTAB *bp = sim_brk_fnd (loc);
int32 i;

if (!bp)                                                /* not there? ok */
    return SCPE_OK;
if (sw == 0)
    sw = SIM_BRK_ALLTYP;

#if !defined(__clang_analyzer__)
while (bp) {
    if (bp->typ == (bp->typ & sw)) {
        FREE (bp->act);                                 /* deallocate action */
        if (bp == sim_brk_tab[sim_brk_ins])
            bpl = sim_brk_tab[sim_brk_ins] = bp->next;
        else
          {
            if (bpl)
              bpl->next = bp->next;
          }
        FREE (bp);
        bp = bpl;
        }
    else {
        bpl = bp;
        bp = bp->next;
        }
    }
#endif /* if !defined(__clang_analyzer__) */
if (sim_brk_tab[sim_brk_ins] == NULL) {                 /* erased entry */
    sim_brk_ent = sim_brk_ent - 1;                      /* decrement count */
    for (i = sim_brk_ins; i < sim_brk_ent; i++)         /* shuffle remaining entries */
        sim_brk_tab[i] = sim_brk_tab[i+1];
    }
sim_brk_summ = 0;                                       /* recalc summary */
for (i = 0; i < sim_brk_ent; i++) {
    bp = sim_brk_tab[i];
    while (bp) {
        sim_brk_summ |= (bp->typ & ~BRK_TYP_TEMP);
        bp = bp->next;
        }
    }
return SCPE_OK;
}

/* Clear all breakpoints */

t_stat sim_brk_clrall (int32 sw)
{
int32 i;

if (sw == 0)
    sw = SIM_BRK_ALLTYP;
for (i = 0; i < sim_brk_ent;) {
    t_addr loc = sim_brk_tab[i]->addr;
    sim_brk_clr (loc, sw);
    if ((i < sim_brk_ent) &&
        (loc == sim_brk_tab[i]->addr))
        ++i;
    }
return SCPE_OK;
}

/* Show a breakpoint */

t_stat sim_brk_show (FILE *st, t_addr loc, int32 sw)
{
BRKTAB *bp = sim_brk_fnd_ex (loc, sw & (~SWMASK ('C')), FALSE, 0);
DEVICE *dptr;
int32 i, any;

if ((sw == 0) || (sw == SWMASK ('C'))) {
    sw = SIM_BRK_ALLTYP | ((sw == SWMASK ('C')) ? SWMASK ('C') : 0); }
if (!bp || (!(bp->typ & sw))) {
    return SCPE_OK; }
dptr = sim_dflt_dev;
if (dptr == NULL) {
    return SCPE_OK; }
if (sw & SWMASK ('C')) {
    if (st != NULL) {
        (void)fprintf (st, "SET BREAK "); }
} else {
    if (sim_vm_fprint_addr) {
        sim_vm_fprint_addr
            (st, dptr, loc);
    } else {
        fprint_val
            (st, loc, dptr->aradix, dptr->awidth, PV_LEFT); }
    if (st != NULL) {
        (void)fprintf (st, ":\t"); }
    }
for (i = any = 0; i < 26; i++) {
    if ((bp->typ >> i) & 1) {
        if ((sw & SWMASK ('C')) == 0) {
            if (any) {
                if (st != NULL) {
                    (void)fprintf (st, ", "); } }
            if (st != NULL) {
                fputc (i + 'A', st); }
            }
        } else {
            if (st != NULL) {
                (void)fprintf (st, "-%c", i + 'A'); }
        any = 1;
        }
    }
if (sw & SWMASK ('C')) {
    if (st != NULL) {
        (void)fprintf (st, " "); }
    if (sim_vm_fprint_addr) {
        if (st != NULL) {
            sim_vm_fprint_addr (st, dptr, loc); }
    } else {
        fprint_val
            (st, loc, dptr->aradix, dptr->awidth, PV_LEFT); }
    }
if (bp->cnt > 0) {
    if (st != NULL) {
        (void)fprintf (st, "[%d]", bp->cnt); } }
if (bp->act != NULL) {
    if (st != NULL) {
        (void)fprintf (st, "; %s", bp->act); } }
(void)fprintf (st, "\n");
return SCPE_OK;
}

/* Show all breakpoints */

t_stat sim_brk_showall (FILE *st, int32 sw)
{
int32 bit, mask, types;
BRKTAB **bpt;

if ((sw == 0) || (sw == SWMASK ('C')))
    sw = SIM_BRK_ALLTYP | ((sw == SWMASK ('C')) ? SWMASK ('C') : 0);
for (types=bit=0; bit <= ('Z'-'A'); bit++)
    if (sim_brk_types & (1 << bit))
        ++types;
if ((!(sw & SWMASK ('C'))) && sim_brk_types && (types > 1)) {
    (void)fprintf (st, "Supported Breakpoint Types:");
    for (bit=0; bit <= ('Z'-'A'); bit++)
        if (sim_brk_types & (1 << bit))
            (void)fprintf (st, " -%c", 'A' + bit);
    (void)fprintf (st, "\n");
    }
if (((sw & sim_brk_types) != sim_brk_types) && (types > 1)) {
    mask = (sw & sim_brk_types);
    (void)fprintf (st, "Displaying Breakpoint Types:");
    for (bit=0; bit <= ('Z'-'A'); bit++)
        if (mask & (1 << bit))
            (void)fprintf (st, " -%c", 'A' + bit);
    (void)fprintf (st, "\n");
    }
for (bpt = sim_brk_tab; bpt < (sim_brk_tab + sim_brk_ent); bpt++) {
    BRKTAB *prev = NULL;
    BRKTAB *cur = *bpt;
    BRKTAB *next;
    /* First reverse the list */
    while (cur) {
        next = cur->next;
        cur->next = prev;
        prev = cur;
        cur = next;
        }
    /* save reversed list in the head pointer so lookups work */
    *bpt = prev;
    /* Walk the reversed list and print it in the order it was defined in */
    cur = prev;
    while (cur) {
        if (cur->typ & sw)
            sim_brk_show (st, cur->addr, cur->typ | ((sw & SWMASK ('C')) ? SWMASK ('C') : 0));
        cur = cur->next;
        }
    /* reversing the list again */
    cur = prev;
    prev = NULL;
    while (cur) {
        next = cur->next;
        cur->next = prev;
        prev = cur;
        cur = next;
        }
    /* restore original list */
    *bpt = prev;
    }
return SCPE_OK;
}

/* Test for breakpoint */

uint32 sim_brk_test (t_addr loc, uint32 btyp)
{
BRKTAB *bp;
uint32 spc = (btyp >> SIM_BKPT_V_SPC) & (SIM_BKPT_N_SPC - 1);

if (sim_brk_summ & BRK_TYP_DYN_ALL)
    btyp |= BRK_TYP_DYN_ALL;

if ((bp = sim_brk_fnd_ex (loc, btyp, TRUE, spc))) {     /* in table, and type match? */
    double s_gtime = sim_gtime ();                      /* get time now */

    if (bp->time_fired[spc] == s_gtime)                 /* already taken?  */
        return 0;
    bp->time_fired[spc] = s_gtime;                      /* remember match time */
    if (--bp->cnt > 0)                                  /* count > 0? */
        return 0;
    bp->cnt = 0;                                        /* reset count */
    sim_brk_setact (bp->act);                           /* set up actions */
    sim_brk_match_type = btyp & bp->typ;                /* set return value */
    if (bp->typ & BRK_TYP_TEMP)
        sim_brk_clr (loc, bp->typ);                     /* delete one-shot breakpoint */
    sim_brk_match_addr = loc;
    return sim_brk_match_type;
    }
return 0;
}

/* Get next pending action, if any */

CONST char *sim_brk_getact (char *buf, int32 size)
{
char *ep;
size_t lnt;

if (sim_brk_act[sim_do_depth] == NULL)                  /* any action? */
    return NULL;
while (sim_isspace (*sim_brk_act[sim_do_depth]))        /* skip spaces */
    sim_brk_act[sim_do_depth]++;
if (*sim_brk_act[sim_do_depth] == 0) {                  /* now empty? */
    return sim_brk_clract ();
    }
if ((ep = strchr (sim_brk_act[sim_do_depth], ';'))) {   /* cmd delimiter? */
    lnt = ep - sim_brk_act[sim_do_depth];               /* cmd length */
    memcpy (buf, sim_brk_act[sim_do_depth], lnt + 1);   /* copy with ; */
    buf[lnt] = 0;                                       /* erase ; */
    sim_brk_act[sim_do_depth] += lnt + 1;               /* adv ptr */
    }
else {
    strncpy (buf, sim_brk_act[sim_do_depth], size);     /* copy action */
    sim_brk_clract ();                                  /* no more */
    }
return buf;
}

/* Clear pending actions */

char *sim_brk_clract (void)
{
FREE (sim_brk_act_buf[sim_do_depth]);
return sim_brk_act[sim_do_depth] = sim_brk_act_buf[sim_do_depth] = NULL;
}

/* Set up pending actions */

void sim_brk_setact (const char *action)
{
if (action) {
    sim_brk_act_buf[sim_do_depth] = (char *)realloc (sim_brk_act_buf[sim_do_depth], strlen (action) + 1);
    if (!sim_brk_act_buf[sim_do_depth])
      {
        (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                       __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
        abort();
      }
    strcpy (sim_brk_act_buf[sim_do_depth], action);
    sim_brk_act[sim_do_depth] = sim_brk_act_buf[sim_do_depth];
    }
else
    sim_brk_clract ();
}

/* New PC */

void sim_brk_npc (uint32 cnt)
{
uint32 spc;
BRKTAB **bpt, *bp;

if ((cnt == 0) || (cnt > SIM_BKPT_N_SPC))
    cnt = SIM_BKPT_N_SPC;
for (bpt = sim_brk_tab; bpt < (sim_brk_tab + sim_brk_ent); bpt++) {
    for (bp = *bpt; bp; bp = bp->next) {
        for (spc = 0; spc < cnt; spc++)
            bp->time_fired[spc] = -1.0;
        }
    }
}

/* Clear breakpoint space */

void sim_brk_clrspc (uint32 spc, uint32 btyp)
{
BRKTAB **bpt, *bp;

if (spc < SIM_BKPT_N_SPC) {
    for (bpt = sim_brk_tab; bpt < (sim_brk_tab + sim_brk_ent); bpt++) {
        for (bp = *bpt; bp; bp = bp->next) {
            if (bp->typ & btyp)
                bp->time_fired[spc] = -1.0;
            }
        }
    }
}

const char *sim_brk_message(void)
{
static char msg[256];
char addr[65];
char buf[32];

msg[0] = '\0';
if (sim_dflt_dev) {
  if (sim_vm_sprint_addr)
    sim_vm_sprint_addr (addr, sim_dflt_dev, (t_value)sim_brk_match_addr);
  else sprint_val (addr, (t_value)sim_brk_match_addr, sim_dflt_dev->aradix, sim_dflt_dev->awidth, PV_LEFT);
}
if (sim_brk_type_desc) {
    BRKTYPTAB *brk = sim_brk_type_desc;

    while (2 == strlen (put_switches (buf, sizeof(buf), brk->btyp))) {
        if (brk->btyp == sim_brk_match_type) {
            (void)sprintf (msg, "%s: %s", brk->desc, addr);
            break;
            }
        brk++;
        }
    }
if (!msg[0])
    (void)sprintf (msg, "%s Breakpoint at: %s\n",
                   put_switches (buf, sizeof(buf), sim_brk_match_type), addr);

return msg;
}

/* Expect package.  This code provides a mechanism to stop and control simulator
   execution based on traffic coming out of simulated ports and as well as a means
   to inject data into those ports.  It can conceptually viewed as a string
   breakpoint package.

   Expect rules are stored in tables associated with each port which can use this
   facility.  An expect rule consists of a five entry structure:

        match                   the expect match string
        size                    the number of bytes in the match string
        match_pattern           the expect match string in display format
        cnt                     number of iterations before match is declared
        action                  command string to be executed when match occurs

   All active expect rules are contained in an expect match context structure.

        rules                   the match rules
        size                    the count of match rules
        buf                     the buffer of output data which has been produced
        buf_ins                 the buffer insertion point for the next output data
        buf_size                the buffer size

   The package contains the following public routines:

        sim_set_expect          expect command parser and initializer
        sim_set_noexpect        noexpect command parser
        sim_exp_set             set or add an expect rule
        sim_exp_clr             clear or delete an expect rule
        sim_exp_clrall          clear all expect rules
        sim_exp_show            show an expect rule
        sim_exp_showall         show all expect rules
        sim_exp_check           test for rule match
*/

/* Set expect */

t_stat sim_set_expect (EXPECT *exp, CONST char *cptr)
{
char gbuf[CBUFSIZE];
CONST char *tptr;
CONST char *c1ptr;
t_bool after_set = FALSE;
uint32 after;
int32 cnt = 0;
t_stat r;

if (exp == NULL)
    return sim_messagef (SCPE_ARG, "Null exp!\n");
after = exp->after;

if ((cptr == NULL) || (*cptr == 0))
    return SCPE_2FARG;
if (*cptr == '[') {
    cnt = (int32) strtotv (cptr + 1, &c1ptr, 10);
    if ((cptr == c1ptr) || (*c1ptr != ']'))
        return sim_messagef (SCPE_ARG, "Invalid Repeat count specification\n");
    cptr = c1ptr + 1;
    while (sim_isspace(*cptr))
        ++cptr;
    }
tptr = get_glyph (cptr, gbuf, ',');
if ((!strncmp(gbuf, "HALTAFTER=", 10)) && (gbuf[10])) {
    after = (uint32)get_uint (&gbuf[10], 10, 2000000000, &r);
    if (r != SCPE_OK)
        return sim_messagef (SCPE_ARG, "Invalid Halt After Value\n");
    after_set = TRUE;
    cptr = tptr;
    }
if ((*cptr != '"') && (*cptr != '\''))
    return sim_messagef (SCPE_ARG, "String must be quote delimited\n");
cptr = get_glyph_quoted (cptr, gbuf, 0);

return sim_exp_set (exp, gbuf, cnt, (after_set ? after : exp->after), sim_switches, cptr);
}

/* Clear expect */

t_stat sim_set_noexpect (EXPECT *exp, const char *cptr)
{
char gbuf[CBUFSIZE];

if (NULL == cptr || !*cptr)
    return sim_exp_clrall (exp);                    /* clear all rules */
if ((*cptr != '"') && (*cptr != '\''))
    return sim_messagef (SCPE_ARG, "String must be quote delimited\n");
cptr = get_glyph_quoted (cptr, gbuf, 0);
if (*cptr != '\0')
    return SCPE_2MARG;                              /* No more arguments */
return sim_exp_clr (exp, gbuf);                     /* clear one rule */
}

/* Search for an expect rule in an expect context */

CONST EXPTAB *sim_exp_fnd (CONST EXPECT *exp, const char *match, size_t start_rule)
{
size_t i;

if (NULL == exp->rules)
    return NULL;
for (i=start_rule; i<exp->size; i++)
    if (!strcmp (exp->rules[i].match_pattern, match))
        return &exp->rules[i];
return NULL;
}

/* Clear (delete) an expect rule */

t_stat sim_exp_clr_tab (EXPECT *exp, EXPTAB *ep)
{
size_t i;

if (NULL == ep)                                         /* not there? ok */
    return SCPE_OK;
FREE (ep->match);                                       /* deallocate match string */
FREE (ep->match_pattern);                               /* deallocate the display format match string */
FREE (ep->act);                                         /* deallocate action */
exp->size -= 1;                                         /* decrement count */
#if !defined(__clang_analyzer__)
for (i=ep-exp->rules; i<exp->size; i++)                 /* shuffle up remaining rules */
    exp->rules[i] = exp->rules[i+1];
if (exp->size == 0) {                                   /* No rules left? */
    FREE (exp->rules);
    exp->rules = NULL;
    }
#endif /* if !defined(__clang_analyzer__) */
return SCPE_OK;
}

t_stat sim_exp_clr (EXPECT *exp, const char *match)
{
EXPTAB *ep = (EXPTAB *)sim_exp_fnd (exp, match, 0);

while (ep) {
    sim_exp_clr_tab (exp, ep);
    ep = (EXPTAB *)sim_exp_fnd (exp, match, ep - exp->rules);
    }
return SCPE_OK;
}

/* Clear all expect rules */

t_stat sim_exp_clrall (EXPECT *exp)
{
int32 i;

for (i=0; i<exp->size; i++) {
    FREE (exp->rules[i].match);                         /* deallocate match string */
    FREE (exp->rules[i].match_pattern);                 /* deallocate display format match string */
    FREE (exp->rules[i].act);                           /* deallocate action */
    }
FREE (exp->rules);
exp->rules = NULL;
exp->size = 0;
FREE (exp->buf);
exp->buf = NULL;
exp->buf_size = 0;
exp->buf_ins = 0;
return SCPE_OK;
}

/* Set/Add an expect rule */

t_stat sim_exp_set (EXPECT *exp, const char *match, int32 cnt, uint32 after, int32 switches, const char *act)
{
EXPTAB *ep;
uint8 *match_buf;
uint32 match_size;
size_t i;

/* Validate the match string */
match_buf = (uint8 *)calloc (strlen (match) + 1, 1);
if (!match_buf)
    return SCPE_MEM;
if (switches & EXP_TYP_REGEX) {
    FREE (match_buf);
    return sim_messagef (SCPE_ARG, "RegEx support not available\n");
    }
else {
    if (switches & EXP_TYP_REGEX_I) {
        FREE (match_buf);
        return sim_messagef (SCPE_ARG, "Case independent matching is only valid for RegEx expect rules\n");
        }
    sim_data_trace(exp->dptr, exp->dptr->units, (const uint8 *)match, "", strlen(match)+1, "Expect Match String", exp->dbit);
    if (SCPE_OK != sim_decode_quoted_string (match, match_buf, &match_size)) {
        FREE (match_buf);
        return sim_messagef (SCPE_ARG, "Invalid quoted string\n");
        }
    }
FREE (match_buf);
for (i=0; i<exp->size; i++) {                           /* Make sure this rule won't be occluded */
    if ((0 == strcmp (match, exp->rules[i].match_pattern)) &&
        (exp->rules[i].switches & EXP_TYP_PERSIST))
        return sim_messagef (SCPE_ARG, "Persistent Expect rule with identical match string already exists\n");
    }
if (after && exp->size)
    return sim_messagef (SCPE_ARG, "Multiple concurrent EXPECT rules aren't valid when a HALTAFTER parameter is non-zero\n");
exp->rules = (EXPTAB *) realloc (exp->rules, sizeof (*exp->rules)*(exp->size + 1));
if (!exp->rules)
  {
    (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                   __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
ep = &exp->rules[exp->size];
exp->size += 1;
exp->after = after;                                     /* set halt after value */
(void)memset (ep, 0, sizeof(*ep));
ep->match_pattern = (char *)malloc (strlen (match) + 1);
if (ep->match_pattern)
    strcpy (ep->match_pattern, match);
ep->cnt = cnt;                                          /* set proceed count */
ep->switches = switches;                                /* set switches */
match_buf = (uint8 *)calloc (strlen (match) + 1, 1);
if ((match_buf == NULL) || (ep->match_pattern == NULL)) {
    sim_exp_clr_tab (exp, ep);                          /* clear it */
    FREE (match_buf);                                   /* release allocation */
    return SCPE_MEM;
    }
if (switches & EXP_TYP_REGEX) {
    FREE (match_buf);
    match_buf = NULL;
    }
else {
    sim_data_trace(exp->dptr, exp->dptr->units, (const uint8 *)match, "", strlen(match)+1, "Expect Match String", exp->dbit);
    sim_decode_quoted_string (match, match_buf, &match_size);
    ep->match = match_buf;
    ep->size = match_size;
    }
ep->match_pattern = (char *)malloc (strlen (match) + 1);
if (!ep->match_pattern)
  {
    (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                   __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
strcpy (ep->match_pattern, match);
if (ep->act) {                                          /* replace old action? */
    FREE (ep->act);                                     /* deallocate */
    ep->act = NULL;                                     /* now no action */
    }
if (act) while (sim_isspace(*act)) ++act;                   /* skip leading spaces in action string */
if ((act != NULL) && (*act != 0)) {                     /* new action? */
    char *newp = (char *) calloc (strlen (act)+1, sizeof (*act)); /* alloc buf */
    if (newp == NULL)                                   /* mem err? */
        return SCPE_MEM;
    strcpy (newp, act);                                 /* copy action */
    ep->act = newp;                                     /* set pointer */
    }
/* Make sure that the production buffer is large enough to detect a match for all rules including a NUL termination byte */
for (i=0; i<exp->size; i++) {
    size_t compare_size = (exp->rules[i].switches & EXP_TYP_REGEX) ? MAX(10 * strlen(ep->match_pattern), 1024) : exp->rules[i].size;
    if (compare_size >= exp->buf_size) {
        exp->buf = (uint8 *)realloc (exp->buf, compare_size + 2); /* Extra byte to null terminate regex compares */
        exp->buf_size = compare_size + 1;
        }
    }
return SCPE_OK;
}

/* Show an expect rule */

t_stat sim_exp_show_tab (FILE *st, const EXPECT *exp, const EXPTAB *ep)
{
if (!ep)
    return SCPE_OK;
(void)fprintf (st, "EXPECT");
if (ep->switches & EXP_TYP_PERSIST)
    (void)fprintf (st, " -p");
if (ep->switches & EXP_TYP_CLEARALL)
    (void)fprintf (st, " -c");
if (ep->switches & EXP_TYP_REGEX)
    (void)fprintf (st, " -r");
if (ep->switches & EXP_TYP_REGEX_I)
    (void)fprintf (st, " -i");
(void)fprintf (st, " %s", ep->match_pattern);
if (ep->cnt > 0)
    (void)fprintf (st, " [%d]", ep->cnt);
if (ep->act)
    (void)fprintf (st, " %s", ep->act);
(void)fprintf (st, "\n");
return SCPE_OK;
}

t_stat sim_exp_show (FILE *st, CONST EXPECT *exp, const char *match)
{
CONST EXPTAB *ep = (CONST EXPTAB *)sim_exp_fnd (exp, match, 0);

if (exp->buf_size) {
    char *bstr = sim_encode_quoted_string (exp->buf, exp->buf_ins);

    (void)fprintf (st, "Match Buffer Size: %lld\n",
                   (long long)exp->buf_size);
    (void)fprintf (st, "Buffer Insert Offset: %lld\n",
                   (long long)exp->buf_ins);
    (void)fprintf (st, "Buffer Contents: %s\n",
                   bstr);
    FREE (bstr);
    }
if (exp->after)
    (void)fprintf (st, "Halt After: %lld instructions\n",
                   (long long)exp->after);
if (exp->dptr && exp->dbit)
    (void)fprintf (st, "Debugging via: SET %s DEBUG%s%s\n",
                   sim_dname(exp->dptr), exp->dptr->debflags ? "=" : "",
                   exp->dptr->debflags ? get_dbg_verb (exp->dbit, exp->dptr) : "");
(void)fprintf (st, "Match Rules:\n");
if (!*match)
    return sim_exp_showall (st, exp);
if (!ep) {
    (void)fprintf (st, "No Rules match '%s'\n", match);
    return SCPE_ARG;
    }
do {
    sim_exp_show_tab (st, exp, ep);
    ep = (CONST EXPTAB *)sim_exp_fnd (exp, match, 1 + (ep - exp->rules));
    } while (ep);
return SCPE_OK;
}

/* Show all expect rules */

t_stat sim_exp_showall (FILE *st, const EXPECT *exp)
{
size_t i;

for (i=0; i < exp->size; i++)
    sim_exp_show_tab (st, exp, &exp->rules[i]);
return SCPE_OK;
}

/* Test for expect match */

t_stat sim_exp_check (EXPECT *exp, uint8 data)
{
size_t i;
EXPTAB *ep = NULL;
char *tstr = NULL;
#if defined(TESTING)
cpu_state_t * cpup = _cpup;
#endif

if ((!exp) || (!exp->rules))                            /* Anything to check? */
    return SCPE_OK;

exp->buf[exp->buf_ins++] = data;                        /* Save new data */
exp->buf[exp->buf_ins] = '\0';                          /* Nul terminate for RegEx match */

for (i=0; i < exp->size; i++) {
    ep = &exp->rules[i];
    if (ep == NULL)
        break;
    if (ep->switches & EXP_TYP_REGEX) {
        }
    else {
        if (exp->buf_ins < ep->size) {                          /* Match stradle end of buffer */
            /*
             * First compare the newly deposited data at the beginning
             * of buffer with the end of the match string
             */
            if (exp->buf_ins > 0) {
                if (sim_deb && exp->dptr && (exp->dptr->dctrl & exp->dbit)) {
                    char *estr = sim_encode_quoted_string (exp->buf, exp->buf_ins);
                    char *mstr = sim_encode_quoted_string (&ep->match[ep->size-exp->buf_ins], exp->buf_ins);

                    sim_debug (exp->dbit, exp->dptr, "Checking String[0:%lld]: %s\n",
                               (long long)exp->buf_ins, estr);
                    sim_debug (exp->dbit, exp->dptr, "Against Match Data: %s\n", mstr);
                    FREE (estr);
                    FREE (mstr);
                    }
                if (memcmp (exp->buf, &ep->match[ep->size-exp->buf_ins], exp->buf_ins))
                    continue;
                }
            if (sim_deb && exp->dptr && (exp->dptr->dctrl & exp->dbit)) {
                char *estr = sim_encode_quoted_string (&exp->buf[exp->buf_size-(ep->size-exp->buf_ins)], ep->size-exp->buf_ins);
                char *mstr = sim_encode_quoted_string (ep->match, ep->size-exp->buf_ins);

                sim_debug (exp->dbit, exp->dptr, "Checking String[%lld:%lld]: %s\n",
                           (long long)exp->buf_size-(ep->size-exp->buf_ins),
                           (long long)ep->size-exp->buf_ins, estr);
                sim_debug (exp->dbit, exp->dptr, "Against Match Data: %s\n", mstr);
                FREE (estr);
                FREE (mstr);
                }
            if (memcmp (&exp->buf[exp->buf_size-(ep->size-exp->buf_ins)], ep->match, ep->size-exp->buf_ins))
                continue;
            break;
            }
        else {
            if (sim_deb && exp->dptr && (exp->dptr->dctrl & exp->dbit)) {
                char *estr = sim_encode_quoted_string (&exp->buf[exp->buf_ins-ep->size], ep->size);
                char *mstr = sim_encode_quoted_string (ep->match, ep->size);

                sim_debug (exp->dbit, exp->dptr, "Checking String[%lld:%lld]: %s\n",
                           (long long)exp->buf_ins-ep->size,
                           (long long)ep->size, estr);
                sim_debug (exp->dbit, exp->dptr, "Against Match Data: %s\n", mstr);
                FREE (estr);
                FREE (mstr);
                }
            if (memcmp (&exp->buf[exp->buf_ins-ep->size], ep->match, ep->size))
                continue;
            break;
            }
        }
    }
if (exp->buf_ins == exp->buf_size) {                    /* At end of match buffer? */
        exp->buf_ins = 0;                               /* wrap around to beginning */
        sim_debug (exp->dbit, exp->dptr, "Buffer wrapping\n");
    }
if ((ep != NULL) && (i != exp->size)) {                 /* Found? */
    sim_debug (exp->dbit, exp->dptr, "Matched expect pattern!\n");
    if (ep->cnt > 0) {
        ep->cnt -= 1;
        sim_debug (exp->dbit, exp->dptr, "Waiting for %lld more match%s before stopping\n",
                   (long long)ep->cnt, (ep->cnt == 1) ? "" : "es");
        }
    else {
        uint32 after   = exp->after;
        int32 switches = ep->switches;
        if (ep->act && *ep->act) {
            sim_debug (exp->dbit, exp->dptr, "Initiating actions: %s\n", ep->act);
            }
        else {
            sim_debug (exp->dbit, exp->dptr, "No actions specified, stopping...\n");
            }
        sim_brk_setact (ep->act);                       /* set up actions */
        if (ep->switches & EXP_TYP_CLEARALL)            /* Clear-all expect rule? */
            sim_exp_clrall (exp);                       /* delete all rules */
        else {
            if (!(ep->switches & EXP_TYP_PERSIST))      /* One shot expect rule? */
                sim_exp_clr_tab (exp, ep);              /* delete it */
            }
        sim_activate (&sim_expect_unit,                 /* schedule simulation stop when indicated */
                      (switches & EXP_TYP_TIME) ?
                            (uint32)((sim_timer_inst_per_sec ()*exp->after)/1000000.0) :
                             after);
        }
    /* Matched data is no longer available for future matching */
    exp->buf_ins = 0;
    }
if (tstr)  //-V547
  FREE (tstr);
return SCPE_OK;
}

/* Queue input data for sending */

t_stat sim_send_input (SEND *snd, uint8 *data, size_t size, uint32 after, uint32 delay)
{
if (snd->extoff != 0) {
    if (snd->insoff > snd->extoff)
        memmove(snd->buffer, snd->buffer+snd->extoff, snd->insoff-snd->extoff);
    snd->insoff -= snd->extoff;
    snd->extoff  = 0;
    }
if (snd->insoff+size > snd->bufsize) {
    snd->bufsize = snd->insoff+size;
    snd->buffer  = (uint8 *)realloc(snd->buffer, snd->bufsize);
    if (!snd->buffer)
      {
        (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                       __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
        abort();
      }
    }
memcpy(snd->buffer+snd->insoff, data, size);
snd->insoff += size;
if (delay)
    snd->delay = (sim_switches & SWMASK ('T')) ? (uint32)((sim_timer_inst_per_sec()*delay)/1000000.0) : delay;
if (after)
    snd->after = (sim_switches & SWMASK ('T')) ? (uint32)((sim_timer_inst_per_sec()*after)/1000000.0) : after;
if (snd->after == 0)
    snd->after = snd->delay;
snd->next_time = sim_gtime() + snd->after;
return SCPE_OK;
}

/* Cancel Queued input data */
t_stat sim_send_clear (SEND *snd)
{
snd->insoff = 0;
snd->extoff = 0;
return SCPE_OK;
}

/* Display console Queued input data status */

t_stat sim_show_send_input (FILE *st, const SEND *snd)
{
if (snd->extoff < snd->insoff) {
    (void)fprintf (st, "%lld bytes of pending input Data:\n    ",
                   (long long)snd->insoff-snd->extoff);
    fprint_buffer_string (st, snd->buffer+snd->extoff, snd->insoff-snd->extoff);
    (void)fprintf (st, "\n");
    }
else
    (void)fprintf (st, "No Pending Input Data\n");
if ((snd->next_time - sim_gtime()) > 0) {
    if ((snd->next_time - sim_gtime()) > (sim_timer_inst_per_sec()/1000000.0))
        (void)fprintf (st, "Minimum of %d instructions (%d microseconds) before sending first character\n",
                       (int)(snd->next_time - sim_gtime()),
        (int)((snd->next_time - sim_gtime())/(sim_timer_inst_per_sec()/1000000.0)));
    else
        (void)fprintf (st, "Minimum of %d instructions before sending first character\n",
                       (int)(snd->next_time - sim_gtime()));
    }
if (snd->delay > (sim_timer_inst_per_sec()/1000000.0))
    (void)fprintf (st, "Minimum of %d instructions (%d microseconds) between characters\n",
                   (int)snd->delay, (int)(snd->delay/(sim_timer_inst_per_sec()/1000000.0)));
else
    (void)fprintf (st, "Minimum of %d instructions between characters\n",
                   (int)snd->delay);
if (snd->dptr && snd->dbit)
    (void)fprintf (st, "Debugging via: SET %s DEBUG%s%s\n",
                   sim_dname(snd->dptr), snd->dptr->debflags ? "=" : "",
                   snd->dptr->debflags ? get_dbg_verb (snd->dbit, snd->dptr) : "");
return SCPE_OK;
}

/* Poll for Queued input data */

t_bool sim_send_poll_data (SEND *snd, t_stat *stat)
{
#if defined(TESTING)
cpu_state_t * cpup = _cpup;
#endif
if ((NULL != snd) && (snd->extoff < snd->insoff)) {     /* pending input characters available? */
    if (sim_gtime() < snd->next_time) {                 /* too soon? */
        *stat = SCPE_OK;
        sim_debug (snd->dbit, snd->dptr, "Too soon to inject next byte\n");
        }
    else {
        char dstr[8] = "";
        *stat = snd->buffer[snd->extoff++] | SCPE_KFLAG;/* get one */
        snd->next_time = sim_gtime() + snd->delay;
        if (sim_isgraph(*stat & 0xFF) || ((*stat & 0xFF) == ' '))
            (void)sprintf (dstr, " '%c'", *stat & 0xFF);
        sim_debug (snd->dbit, snd->dptr, "Byte value: 0x%02X%s injected\n", *stat & 0xFF, dstr);
        }
    return TRUE;
    }
return FALSE;
}

/* Message Text */

const char *sim_error_text (t_stat stat)
{
static char msgbuf[64];

stat &= ~(SCPE_KFLAG|SCPE_BREAK|SCPE_NOMESSAGE);        /* remove any flags */
if (stat == SCPE_OK)
    return "No Error";
if ((stat >= SCPE_BASE) && (stat <= SCPE_MAX_ERR))
    return scp_errors[stat-SCPE_BASE].message;
(void)sprintf(msgbuf, "Error %d", stat);
return msgbuf;
}

t_stat sim_string_to_stat (const char *cptr, t_stat *stat)
{
char gbuf[CBUFSIZE];
size_t cond;

*stat = SCPE_ARG;
cptr = get_glyph (cptr, gbuf, 0);
if (0 == memcmp("SCPE_", gbuf, 5))
    memmove (gbuf, gbuf + 5, 1 + strlen (gbuf + 5));  /* skip leading SCPE_ */
for (cond=0; cond < (SCPE_MAX_ERR-SCPE_BASE); cond++)
    if (0 == strcmp(scp_errors[cond].code, gbuf)) {
        cond += SCPE_BASE;
        break;
        }
if (0 == strcmp(gbuf, "OK"))
    cond = SCPE_OK;
if (cond == (SCPE_MAX_ERR-SCPE_BASE)) {       /* not found? */
    unsigned long numeric_cond = strtol(gbuf, NULL, 0);
    if (0 == numeric_cond)                    /* try explicit number */
        return SCPE_ARG;
    cond = (t_stat) numeric_cond;
    }
if (cond > SCPE_MAX_ERR)
    return SCPE_ARG;
*stat = cond;
return SCPE_OK;
}

/* Debug printout routines, from Dave Hittner */

const char* debug_bstates = "01_^";
char debug_line_prefix[256];
int32 debug_unterm  = 0;

/* Finds debug phrase matching bitmask from from device DEBTAB table */

static const char *get_dbg_verb (uint32 dbits, DEVICE* dptr)
{
static const char *debtab_none    = "DEBTAB_ISNULL";
static const char *debtab_nomatch = "DEBTAB_NOMATCH";
const char *some_match = NULL;
int32 offset = 0;

if (dptr->debflags == 0)
    return debtab_none;

dbits &= dptr->dctrl;                           /* Look for just the bits that matched */

/* Find matching words for bitmask */

while ((offset < 32) && dptr->debflags[offset].name) {
    if (dptr->debflags[offset].mask == dbits)   /* All Bits Match */
        return dptr->debflags[offset].name;
    if (dptr->debflags[offset].mask & dbits)
        some_match = dptr->debflags[offset].name;
    offset++;
    }
return some_match ? some_match : debtab_nomatch;
}

/* Prints standard debug prefix unless previous call unterminated */

static const char *sim_debug_prefix (uint32 dbits, DEVICE* dptr)
{
const char* debug_type = get_dbg_verb (dbits, dptr);
char tim_t[32] = "";
char tim_a[32] = "";
char  pc_s[64] = "";
struct timespec time_now;

if (sim_deb_switches & (SWMASK ('T') | SWMASK ('R') | SWMASK ('A'))) {
    clock_gettime(CLOCK_REALTIME, &time_now);
    if (sim_deb_switches & SWMASK ('R'))
        sim_timespec_diff (&time_now, &time_now, &sim_deb_basetime);
    if (sim_deb_switches & SWMASK ('T')) {
        time_t tnow = (time_t)time_now.tv_sec;
        struct tm *now = gmtime(&tnow);
        (void)sprintf(tim_t, "%02d:%02d:%02d.%03ld ",
                      (int)now->tm_hour,
                      (int)now->tm_min,
                      (int)now->tm_sec,
                      (long)(time_now.tv_nsec / 1000000));
        }
    if (sim_deb_switches & SWMASK ('A')) {
        (void)sprintf(tim_t, "%d.%03ld ",
                      (int)(time_now.tv_sec),
                      (long)(time_now.tv_nsec / 1000000));
        }
    }
if (sim_deb_switches & SWMASK ('P')) {
    t_value val;

    if (sim_vm_pc_value)
        val = (*sim_vm_pc_value)();
    else
        val = get_rval (sim_PC, 0);
    (void)sprintf(pc_s, "-%s:", sim_PC->name);
    sprint_val (&pc_s[strlen(pc_s)], val, sim_PC->radix, sim_PC->width, sim_PC->flags & REG_FMT);
    }
(void)sprintf(debug_line_prefix, "DBG(%s%s%.0f%s)%s> %s %s: ",
              tim_t, tim_a, sim_gtime(), pc_s,
              "", dptr->name, debug_type);
return debug_line_prefix;
}

void fprint_fields (FILE *stream, t_value before, t_value after, BITFIELD* bitdefs)
{
int32 i, fields, offset;
uint32 value, beforevalue, mask;

for (fields=offset=0; bitdefs[fields].name; ++fields) {
    if (bitdefs[fields].offset == 0xffffffff)       /* fixup uninitialized offsets */
        bitdefs[fields].offset = offset;
    offset += bitdefs[fields].width;
    }
for (i = fields-1; i >= 0; i--) {                   /* print xlation, transition */
    if (bitdefs[i].name[0] == '\0')
        continue;
    if ((bitdefs[i].width == 1) && (bitdefs[i].valuenames == NULL)) {
        int off = ((after >> bitdefs[i].offset) & 1) + (((before ^ after) >> bitdefs[i].offset) & 1) * 2;
        (void)Fprintf(stream, "%s%c ", bitdefs[i].name, debug_bstates[off]);
        }
    else {
        const char *delta = "";
        mask = 0xFFFFFFFF >> (32-bitdefs[i].width);
        value = (uint32)((after >> bitdefs[i].offset) & mask);
        beforevalue = (uint32)((before >> bitdefs[i].offset) & mask);
        if (value < beforevalue)
            delta = "_";
        if (value > beforevalue)
            delta = "^";
        if (bitdefs[i].valuenames)
            (void)Fprintf(stream, "%s=%s%s ", bitdefs[i].name, delta, bitdefs[i].valuenames[value]);
        else
            if (bitdefs[i].format) {
                (void)Fprintf(stream, "%s=%s", bitdefs[i].name, delta);
                (void)Fprintf(stream, bitdefs[i].format, value);
                (void)Fprintf(stream, " ");
                }
            else
                (void)Fprintf(stream, "%s=%s0x%X ", bitdefs[i].name, delta, value);
        }
    }
}

/* Prints state of a register: bit translation + state (0,1,_,^)
   indicating the state and transition of the bit and bitfields. States:
   0=steady(0->0), 1=steady(1->1), _=falling(1->0), ^=rising(0->1) */

void sim_debug_bits_hdr(uint32 dbits, DEVICE* dptr, const char *header,
    BITFIELD* bitdefs, uint32 before, uint32 after, int terminate)
{
if (sim_deb && dptr && (dptr->dctrl & dbits)) {
    if (!debug_unterm)
        (void)fprintf(sim_deb, "%s", sim_debug_prefix(dbits, dptr));         /* print prefix if required */
    if (header)
        (void)fprintf(sim_deb, "%s: ", header);
    fprint_fields (sim_deb, (t_value)before, (t_value)after, bitdefs); /* print xlation, transition */
    if (terminate)
        (void)fprintf(sim_deb, "\r\n");
    debug_unterm = terminate ? 0 : 1;                   /* set unterm for next */
    }
}
void sim_debug_bits(uint32 dbits, DEVICE* dptr, BITFIELD* bitdefs,
    uint32 before, uint32 after, int terminate)
{
sim_debug_bits_hdr(dbits, dptr, NULL, bitdefs, before, after, terminate);
}

/* Print message to stdout, sim_log (if enabled) and sim_deb (if enabled) */
void sim_printf (const char* fmt, ...)
{
char stackbuf[STACKBUFSIZE];
int32 bufsize = sizeof(stackbuf);
char *buf = stackbuf;
int32 len;
va_list arglist;

while (1) {                                         /* format passed string, args */
    va_start (arglist, fmt);
    len = vsnprintf (buf, bufsize-1, fmt, arglist);
    va_end (arglist);

/* If the formatted result didn't fit into the buffer, then grow the buffer and try again */

    if ((len < 0) || (len >= bufsize-1)) {
        if (buf != stackbuf)
            FREE (buf);
        bufsize = bufsize * 2;
        if (bufsize < len + 2)
            bufsize = len + 2;
        buf = (char *) malloc (bufsize);
        if (buf == NULL)                            /* out of memory */
            return;
        buf[bufsize-1] = '\0';
        continue;
        }
    break;
    }

if (sim_is_running) {
    char *c, *remnant = buf;
    while ((c = strchr(remnant, '\n'))) {
        if ((c != buf) && (*(c - 1) != '\r'))
            (void)printf("%.*s\r\n", (int)(c-remnant), remnant);
        else
            (void)printf("%.*s\n", (int)(c-remnant), remnant);
        remnant = c + 1;
        }
    (void)printf("%s", remnant);
    }
else
    (void)printf("%s", buf);
if (sim_log && (sim_log != stdout))
    (void)fprintf (sim_log, "%s", buf);
if (sim_deb && (sim_deb != stdout) && (sim_deb != sim_log))
    (void)fprintf (sim_deb, "%s", buf);

if (buf != stackbuf)
    FREE (buf);
}

/* Print command result message to stdout, sim_log (if enabled) and sim_deb (if enabled) */
t_stat sim_messagef (t_stat stat, const char* fmt, ...)
{
char stackbuf[STACKBUFSIZE];
size_t bufsize = sizeof(stackbuf);
char *buf = stackbuf;
size_t len;
va_list arglist;
t_bool inhibit_message = (!sim_show_message || (stat & SCPE_NOMESSAGE));

while (1) {                                         /* format passed string, args */
    va_start (arglist, fmt);
    len = vsnprintf (buf, bufsize-1, fmt, arglist);
    va_end (arglist);

/* If the formatted result didn't fit into the buffer, then grow the buffer and try again */

    if (len >= bufsize - 1) {
        if (buf != stackbuf)
            FREE (buf);
        bufsize = bufsize * 2;
        if (bufsize < len + 2)
            bufsize = len + 2;
        buf = (char *) malloc (bufsize);
        if (buf == NULL)                            /* out of memory */
            return SCPE_MEM;
        buf[bufsize-1] = '\0';
        continue;
        }
    break;
    }

if (sim_do_ocptr[sim_do_depth]) {
    if (!sim_do_echo && !sim_quiet && !inhibit_message)
        sim_printf("%s> %s\n", do_position(), sim_do_ocptr[sim_do_depth]);
    else {
        if (sim_deb)                        /* Always put context in debug output */
            (void)fprintf (sim_deb, "%s> %s\n", do_position(), sim_do_ocptr[sim_do_depth]);
        }
    }
if (sim_is_running && !inhibit_message) {
    char *c, *remnant = buf;
    while ((c = strchr(remnant, '\n'))) {
        if ((c != buf) && (*(c - 1) != '\r'))
            (void)printf("%.*s\r\n", (int)(c-remnant), remnant);
        else
            (void)printf("%.*s\n", (int)(c-remnant), remnant);
        remnant = c + 1;
        }
    (void)printf("%s", remnant);
    }
else {
    if (!inhibit_message)
        (void)printf("%s", buf);
    }
if (sim_log && (sim_log != stdout) && !inhibit_message)
    (void)fprintf (sim_log, "%s", buf);
if (sim_deb && (((sim_deb != stdout) && (sim_deb != sim_log)) || inhibit_message))/* Always display messages in debug output */
    (void)fprintf (sim_deb, "%s", buf);

if (buf != stackbuf)
    FREE (buf);
return stat | SCPE_NOMESSAGE;
}

/* Inline debugging - will print debug message if debug file is
   set and the bitmask matches the current device debug options.
   Extra returns are added for un*x systems, since the output
   device is set into 'raw' mode when the cpu is booted,
   and the extra returns don't hurt any other systems.
   Callers should be calling sim_debug() which is a macro
   defined in scp.h which evaluates the action condition before
   incurring call overhead. */
void _sim_debug (uint32 dbits, DEVICE* vdptr, const char* fmt, ...)
{
DEVICE *dptr = (DEVICE *)vdptr;
if (sim_deb && dptr && (dbits == 0 || (dptr->dctrl & dbits))) {
    char stackbuf[STACKBUFSIZE];
    int32 bufsize = sizeof(stackbuf);
    char *buf = stackbuf;
    va_list arglist;
    int32 i, j, len;
    const char* debug_prefix = sim_debug_prefix(dbits, dptr);   /* prefix to print if required */

    buf[bufsize-1] = '\0';
    while (1) {                                         /* format passed string, args */
        va_start (arglist, fmt);
        len = vsnprintf (buf, bufsize-1, fmt, arglist);
        va_end (arglist);

/* If the formatted result didn't fit into the buffer, then grow the buffer and try again */

        if ((len < 0) || (len >= bufsize-1)) {
            if (buf != stackbuf)
                FREE (buf);
            bufsize = bufsize * 2;
            if (bufsize < len + 2)
                bufsize = len + 2;
            buf = (char *) malloc (bufsize);
            if (buf == NULL)                            /* out of memory */
                return;
            buf[bufsize-1] = '\0';
            continue;
            }
        break;
        }

/* Output the formatted data expanding newlines where they exist */

    for (i = j = 0; i < len; ++i) {
        if ('\n' == buf[i]) {
            if (i >= j) {
                if ((i != j) || (i == 0)) {
                    if (debug_unterm)
                        (void)fprintf (sim_deb, "%.*s\r\n", i-j, &buf[j]);
                    else                                /* print prefix when required */
                        (void)fprintf (sim_deb, "%s%.*s\r\n", debug_prefix, i-j, &buf[j]);
                    }
                debug_unterm = 0;
                }
            j = i + 1;
            }
        }
    if (i > j) {
        if (debug_unterm)
            (void)fprintf (sim_deb, "%.*s", i-j, &buf[j]);
        else                                        /* print prefix when required */
            (void)fprintf (sim_deb, "%s%.*s", debug_prefix, i-j, &buf[j]);
        }

/* Set unterminated flag for next time */

    debug_unterm = len ? (((buf[len-1]=='\n')) ? 0 : 1) : debug_unterm;
    if (buf != stackbuf)
        FREE (buf);
    }
return;
}

void sim_data_trace(DEVICE *dptr, UNIT *uptr, const uint8 *data, const char *position, size_t len, const char *txt, uint32 reason)
{
#if defined(TESTING)
cpu_state_t * cpup = _cpup;
#endif
if (sim_deb && (dptr->dctrl & reason)) {
    sim_debug (reason, dptr, "%s %s %slen: %08X\n", sim_uname(uptr), txt, position, (unsigned int)len);
    if (data && len) {
        size_t i, same, group, sidx, oidx, ridx, eidx, soff;
        char outbuf[80], strbuf[28], rad50buf[36], ebcdicbuf[32];
        static char hex[] = "0123456789ABCDEF";
        static char rad50[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ$._0123456789";
        static unsigned char ebcdic2ascii[] = {
            0000, 0001, 0002, 0003, 0234, 0011, 0206, 0177,
            0227, 0215, 0216, 0013, 0014, 0015, 0016, 0017,
            0020, 0021, 0022, 0023, 0235, 0205, 0010, 0207,
            0030, 0031, 0222, 0217, 0034, 0035, 0036, 0037,
            0200, 0201, 0202, 0203, 0204, 0012, 0027, 0033,
            0210, 0211, 0212, 0213, 0214, 0005, 0006, 0007,
            0220, 0221, 0026, 0223, 0224, 0225, 0226, 0004,
            0230, 0231, 0232, 0233, 0024, 0025, 0236, 0032,
            0040, 0240, 0241, 0242, 0243, 0244, 0245, 0246,
            0247, 0250, 0133, 0056, 0074, 0050, 0053, 0041,
            0046, 0251, 0252, 0253, 0254, 0255, 0256, 0257,
            0260, 0261, 0135, 0044, 0052, 0051, 0073, 0136,
            0055, 0057, 0262, 0263, 0264, 0265, 0266, 0267,
            0270, 0271, 0174, 0054, 0045, 0137, 0076, 0077,
            0272, 0273, 0274, 0275, 0276, 0277, 0300, 0301,
            0302, 0140, 0072, 0043, 0100, 0047, 0075, 0042,
            0303, 0141, 0142, 0143, 0144, 0145, 0146, 0147,
            0150, 0151, 0304, 0305, 0306, 0307, 0310, 0311,
            0312, 0152, 0153, 0154, 0155, 0156, 0157, 0160,
            0161, 0162, 0313, 0314, 0315, 0316, 0317, 0320,
            0321, 0176, 0163, 0164, 0165, 0166, 0167, 0170,
            0171, 0172, 0322, 0323, 0324, 0325, 0326, 0327,
            0330, 0331, 0332, 0333, 0334, 0335, 0336, 0337,
            0340, 0341, 0342, 0343, 0344, 0345, 0346, 0347,
            0173, 0101, 0102, 0103, 0104, 0105, 0106, 0107,
            0110, 0111, 0350, 0351, 0352, 0353, 0354, 0355,
            0175, 0112, 0113, 0114, 0115, 0116, 0117, 0120,
            0121, 0122, 0356, 0357, 0360, 0361, 0362, 0363,
            0134, 0237, 0123, 0124, 0125, 0126, 0127, 0130,
            0131, 0132, 0364, 0365, 0366, 0367, 0370, 0371,
            0060, 0061, 0062, 0063, 0064, 0065, 0066, 0067,
            0070, 0071, 0372, 0373, 0374, 0375, 0376, 0377,
            };

        for (i=same=0; i<len; i += 16) {
            if ((i > 0) && (0 == memcmp (&data[i], &data[i-16], 16))) {
                ++same;
                continue;
                }
            if (same > 0) {
                sim_debug (reason, dptr, "%04lx thru %04lx same as above\n",
                           i - (16*same),
                           i - 1);
                same = 0;
                }
            group = (((len - i) > 16) ? 16 : (len - i));
            strcpy (ebcdicbuf, (sim_deb_switches & SWMASK ('E')) ? " EBCDIC:" : "");
            eidx = strlen(ebcdicbuf);
            strcpy (rad50buf, (sim_deb_switches & SWMASK ('D')) ? " RAD50:" : "");
            ridx = strlen(rad50buf);
            strcpy (strbuf, (sim_deb_switches & (SWMASK ('E') | SWMASK ('D'))) ? "ASCII:" : "");
            soff = strlen(strbuf);
            for (sidx=oidx=0; sidx<group; ++sidx) {
                outbuf[oidx++] = ' ';
                outbuf[oidx++] = hex[(data[i+sidx]>>4)&0xf];
                outbuf[oidx++] = hex[data[i+sidx]&0xf];
                if (sim_isprint (data[i+sidx]))
                    strbuf[soff+sidx] = data[i+sidx];
                else
                    strbuf[soff+sidx] = '.';
                if (ridx && ((sidx&1) == 0)) {
                    uint16 word = data[i+sidx] + (((uint16)data[i+sidx+1]) << 8);

                    if (word >= 64000) {
                        rad50buf[ridx++] = '|'; /* Invalid RAD-50 character */
                        rad50buf[ridx++] = '|'; /* Invalid RAD-50 character */
                        rad50buf[ridx++] = '|'; /* Invalid RAD-50 character */
                        }
                    else {
                        rad50buf[ridx++] = rad50[word/1600];
                        rad50buf[ridx++] = rad50[(word/40)%40];
                        rad50buf[ridx++] = rad50[word%40];
                        }
                    }
                if (eidx) {
                    if (sim_isprint (ebcdic2ascii[data[i+sidx]]))
                        ebcdicbuf[eidx++] = ebcdic2ascii[data[i+sidx]];
                    else
                        ebcdicbuf[eidx++] = '.';
                    }
                }
            outbuf[oidx] = '\0';
            strbuf[soff+sidx] = '\0';
            ebcdicbuf[eidx] = '\0';
            rad50buf[ridx] = '\0';
            sim_debug (reason, dptr, "%04lx%-48s %s%s%s\n", i, outbuf, strbuf, ebcdicbuf, rad50buf);
            }
        if (same > 0) {
            sim_debug (reason, dptr, "%04lx thru %04lx same as above\n", i-(16*same), (long unsigned int)(len-1));
            }
        }
    }
}

int Fprintf (FILE *f, const char* fmt, ...)
{
int ret = 0;
va_list args;

va_start (args, fmt);
    ret = vfprintf (f, fmt, args);
va_end (args);
return ret;
}

/* Hierarchical help presentation
 *
 * Device help can be presented hierarchically by calling
 *
 * t_stat scp_help (FILE *st, DEVICE *dptr,
 *                  UNIT *uptr, int flag, const char *help, char *cptr)
 *
 * or one of its three cousins from the device HELP routine.
 *
 * *help is the pointer to the structured help text to be displayed.
 *
 * The format and usage, and some helper macros can be found in scp_help.h
 * If you don't use the macros, it is not necessary to #include "scp_help.h".
 *
 * Actually, if you don't specify a DEVICE pointer and don't include
 * other device references, it can be used for non-device help.
 */

#define blankch(x) ((x) == ' ' || (x) == '\t')

typedef struct topic {
    size_t         level;
    char          *title;
    char          *label;
    struct topic  *parent;
    struct topic **children;
    uint32         kids;
    char          *text;
    size_t         len;
    uint32         flags;
    size_t         kidwid;
#define HLP_MAGIC_TOPIC  1
    } TOPIC;

static volatile struct {
    const char *error;
    const char *prox;
    size_t block;
    size_t line;
    } help_where = { "", NULL, 0, 0 };
jmp_buf help_env;

#define FAIL(why,text,here)        \
  {                                \
    help_where.error = #text;      \
    help_where.prox = here;        \
    longjmp ( help_env, (why) );   \
    /*LINTED E_STMT_NOT_REACHED*/  \
  }

/*
 * Add to topic text.
 * Expands text buffer as necessary.
 */

static void appendText (TOPIC *topic, const char *text, size_t len)
{
char *newt;

if (!len)
    return;

newt = (char *)realloc (topic->text, topic->len + len +1);
if (!newt) {
#if !defined(SUNLINT)
    FAIL (SCPE_MEM, No memory, NULL);
#endif /* if !defined(SUNLINT) */
    }
topic->text = newt;
memcpy (newt + topic->len, text, len);
topic->len +=len;
newt[topic->len] = '\0';
return;
}

/* Release memory held by a topic and its children.
 */
static void cleanHelp (TOPIC *topic)
{
TOPIC *child;
size_t i;

FREE (topic->title);
FREE (topic->text);
FREE (topic->label);
for (i = 0; i < topic->kids; i++) {
    child = topic->children[i];
    cleanHelp (child);
    FREE (child);
    }
FREE (topic->children);
return;
}

/* Build a help tree from a string.
 * Handles substitutions, formatting.
 */
static TOPIC *buildHelp (TOPIC *topic, DEVICE *dptr,
                         UNIT *uptr, const char *htext, va_list ap)
{
char *end;
size_t n, ilvl;
#define VSMAX 100
char *vstrings[VSMAX];
size_t vsnum = 0;
char * astrings[VSMAX+1];
size_t asnum = 0;
char *const *hblock;
const char *ep;
t_bool excluded = FALSE;

/* variable arguments consumed table.
 * The scheme used allows arguments to be accessed in random
 * order, but for portability, all arguments must be char *.
 * If you try to violate this, there ARE machines that WILL break.
 */

(void)memset (vstrings, 0, sizeof (vstrings));
(void)memset (astrings, 0, sizeof (astrings));
astrings[asnum++] = (char *) htext;

for (hblock = astrings; (htext = *hblock) != NULL; hblock++) {
    help_where.block = hblock - astrings;
    help_where.line = 0;
    while (*htext) {
        const char *start;

        help_where.line++;
        if (sim_isspace (*htext) || *htext == '+') {/* Topic text, indented topic text */
            if (excluded) {                     /* Excluded topic text */
                while (*htext && *htext != '\n')
                    htext++;
                if (*htext)
                    ++htext;
                continue;
                }
            ilvl = 1;
            appendText (topic, "    ", 4);      /* Basic indentation */
            if (*htext == '+') {                /* More for each + */
                while (*htext == '+') {
                    ilvl++;
                    appendText (topic, "    ", 4);
                    htext++;
                    }
                }
            while (*htext && *htext != '\n' && sim_isspace (*htext))
                htext++;
            if (!*htext)                        /* Empty after removing leading spaces */
                break;
            start = htext;
            while (*htext) {                    /* Process line for substitutions */
                if (*htext == '%') {
                    appendText (topic, start, htext - start); /* Flush up to escape */
                    switch (*++htext) {         /* Evaluate escape */
                        case 'U':
                            if (dptr) {
                                char buf[129];
                                n = uptr? uptr - dptr->units: 0;
                                (void)sprintf (buf, "%s%u", dptr->name, (int)n);
                                appendText (topic, buf, strlen (buf));
                                }
                            break;
                        case 'D':
                            if (dptr != NULL)
                                appendText (topic, dptr->name, strlen (dptr->name));
                            break;
                        case 'S':
                            appendText (topic, sim_name, strlen (sim_name));
                            break;
                        case '%':
                            appendText (topic, "%", 1);
                            break;
                        case '+':
                            appendText (topic, "+", 1);
                            break;
                        default:                    /* Check for vararg # */
                            if (sim_isdigit (*htext)) {
                                n = 0;
                                while (sim_isdigit (*htext))
                                    n += (n * 10) + (*htext++ - '0');
                                if (( *htext != 'H' && *htext != 's') ||
                                    n == 0 || n >= VSMAX) {
#if !defined(SUNLINT)
                                    FAIL (SCPE_ARG, Invalid escape, htext);
#endif /* if !defined(SUNLINT) */
                                    }
                                while (n > vsnum)   /* Get arg pointer if not cached */
                                    vstrings[vsnum++] = va_arg (ap, char *);
                                start = vstrings[n-1]; /* Insert selected string */
                                if (*htext == 'H') {   /* Append as more input */
                                    if (asnum >= VSMAX) {
#if !defined(SUNLINT)
                                        FAIL (SCPE_ARG, Too many blocks, htext);
#endif /* if !defined(SUNLINT) */
                                        }
                                    astrings[asnum++] = (char *)start;
                                    break;
                                    }
                                ep = start;
                                while (*ep) {
                                    if (*ep == '\n') {
                                        ep++;       /* Segment to \n */
                                        appendText (topic, start, ep - start);
                                        if (*ep) {  /* More past \n, indent */
                                            size_t i;
                                            for (i = 0; i < ilvl; i++)
                                                appendText (topic, "    ", 4);
                                            }
                                        start = ep;
                                        }
                                    else
                                        ep++;
                                    }
                                appendText (topic, start, ep-start);
                                break;
                                }
#if !defined(SUNLINT)
                            FAIL (SCPE_ARG, Invalid escape, htext);
#endif /* if !defined(SUNLINT) */
                        } /* switch (escape) */
                    start = ++htext;
                    continue;                   /* Current line */
                    } /* if (escape) */
                if (*htext == '\n') {           /* End of line, append last segment */
                    htext++;
                    appendText (topic, start, htext - start);
                    break;                      /* To next line */
                    }
                htext++;                        /* Regular character */
                }
            continue;
            } /* topic text line */
        if (sim_isdigit (*htext)) {             /* Topic heading */
            TOPIC **children;
            TOPIC *newt;
            char nbuf[100];

            n = 0;
            start = htext;
            while (sim_isdigit (*htext))
                n += (n * 10) + (*htext++ - '0');
            if ((htext == start) || !n) {
#if !defined(SUNLINT)
                FAIL (SCPE_ARG, Invalid topic heading, htext);
#endif /* if !defined(SUNLINT) */
                }
            if (n <= topic->level) {            /* Find level for new topic */
                while (n <= topic->level)
                    topic = topic->parent;
                }
            else {
                if (n > topic->level + 1) {     /* Skipping down more than 1 */
#if !defined(SUNLINT)
                    FAIL (SCPE_ARG, Level not contiguous, htext); /* E.g. 1 3, not reasonable */
#endif /* if !defined(SUNLINT) */
                    }
                }
            while (*htext && (*htext != '\n') && sim_isspace (*htext))
                htext++;
            if (!*htext || (*htext == '\n')) {  /* Name missing */
#if !defined(SUNLINT)
                FAIL (SCPE_ARG, Missing topic name, htext);
#endif /* if !defined(SUNLINT) */
                }
            start = htext;
            while (*htext && (*htext != '\n'))
                htext++;
            if (start == htext) {               /* Name NULL */
#if !defined(SUNLINT)
                FAIL (SCPE_ARG, Null topic name, htext);
#endif /* if !defined(SUNLINT) */
                }
            excluded = FALSE;
            if (*start == '?') {                /* Conditional topic? */
                size_t n = 0;
                start++;
                while (sim_isdigit (*start))    /* Get param # */
                    n += (n * 10) + (*start++ - '0');
                if (!*start || *start == '\n'|| n == 0 || n >= VSMAX) {
#if !defined(SUNLINT)
                    FAIL (SCPE_ARG, Invalid parameter number, start);
#endif /* if !defined(SUNLINT) */
                    }
                while (n > vsnum)               /* Get arg pointer if not cached */
                    vstrings[vsnum++] = va_arg (ap, char *);
                end = vstrings[n-1];            /* Check for True */
                if (!end || !(toupper (*end) == 'T' || *end == '1')) {
                    excluded = TRUE;            /* False, skip topic this time */
                    if (*htext)
                        htext++;
                    continue;
                    }
                }
            newt = (TOPIC *) calloc (sizeof (TOPIC), 1);
            if (!newt) {
#if !defined(SUNLINT)
                FAIL (SCPE_MEM, No memory, NULL);
#endif /* if !defined(SUNLINT) */
                }
            newt->title = (char *) malloc ((htext - start)+1);
            if (!newt->title) {
                FREE (newt);
#if !defined(SUNLINT)
                FAIL (SCPE_MEM, No memory, NULL);
#endif /* if !defined(SUNLINT) */
                }
            memcpy (newt->title, start, htext - start);
            newt->title[htext - start] = '\0';
            if (*htext)
                htext++;

            if (newt->title[0] == '$')
                newt->flags |= HLP_MAGIC_TOPIC;

            children = (TOPIC **) realloc (topic->children,
                                           (topic->kids +1) * sizeof (TOPIC *));
            if (NULL == children) {
                FREE (newt->title);
                FREE (newt);
#if !defined(SUNLINT)
                FAIL (SCPE_MEM, No memory, NULL);
#endif /* if !defined(SUNLINT) */
                }
            topic->children = children;
            topic->children[topic->kids++] = newt;
            newt->level = n;
            newt->parent = topic;
            n = strlen (newt->title);
            if (n > topic->kidwid)
                topic->kidwid = n;
            (void)sprintf (nbuf, ".%u", topic->kids);
            n = strlen (topic->label) + strlen (nbuf) + 1;
            newt->label = (char *) malloc (n);
            if (NULL == newt->label) {
                FREE (newt->title);
                topic->children[topic->kids -1] = NULL;
                FREE (newt);
#if !defined(SUNLINT)
                FAIL (SCPE_MEM, No memory, NULL);
#endif /* if !defined(SUNLINT) */
                }
            (void)sprintf (newt->label, "%s%s", topic->label, nbuf);
            topic = newt;
            continue;
            } /* digits introducing a topic */
        if (*htext == ';') {                    /* Comment */
            while (*htext && *htext != '\n')
                htext++;
            continue;
            }
#if !defined(SUNLINT)
        FAIL (SCPE_ARG, Unknown line type, htext);     /* Unknown line */
#endif /* if !defined(SUNLINT) */
        } /* htext not at end */
    (void)memset (vstrings, 0, VSMAX * sizeof (char *));
    vsnum = 0;
    } /* all strings */

return topic;
}

/*
 * Create prompt string - top thru current topic
 * Add prompt at end.
 */
static char *helpPrompt ( TOPIC *topic, const char *pstring, t_bool oneword )
{
char *prefix;
char *newp, *newt;

if (topic->level == 0) {
    prefix = (char *) calloc (2,1);
    if (!prefix) {
#if !defined(SUNLINT)
        FAIL (SCPE_MEM, No memory, NULL);
#endif /* if !defined(SUNLINT) */
        }
    prefix[0] = '\n';
    }
else
    prefix = helpPrompt (topic->parent, "", oneword);

newp = (char *) malloc (strlen (prefix) + 1 + strlen (topic->title) + 1 +
                        strlen (pstring) +1);
if (!newp) {
    FREE (prefix);
#if !defined(SUNLINT)
    FAIL (SCPE_MEM, No memory, NULL);
#endif /* if !defined(SUNLINT) */
    }
strcpy (newp, prefix);
if (topic->children) {
    if (topic->level != 0)
        strcat (newp, " ");
    newt = (topic->flags & HLP_MAGIC_TOPIC)?
            topic->title+1: topic->title;
    if (oneword) {
        char *np = newp + strlen (newp);
        while (*newt) {
            *np++ = blankch (*newt)? '_' : *newt;
            newt++;
            }
        *np = '\0';
        }
    else
        strcat (newp, newt);
    if (*pstring && *pstring != '?')
        strcat (newp, " ");
    }
strcat (newp, pstring);
FREE (prefix);
return newp;
}

static void displayMagicTopic (FILE *st, DEVICE *dptr, TOPIC *topic)
{
char tbuf[CBUFSIZE];
size_t i, skiplines;
#if defined(_WIN32)
FILE *tmp;
char *tmpnam;

do {
    int fd;
    tmpnam = _tempnam (NULL, "simh");
    fd = _open (tmpnam, _O_CREAT | _O_RDWR | _O_EXCL, _S_IREAD | _S_IWRITE);
    if (fd != -1) {
        tmp = _fdopen (fd, "w+");
        break;
        }
    } while (1);
#else
FILE *tmp = tmpfile();
#endif /* if defined(_WIN32) */

if (!tmp) {
    (void)fprintf (st, "Unable to create temporary file: %s (Error %d)\n",
                   xstrerror_l(errno), errno);
    return;
    }

if (topic->title)
    (void)fprintf (st, "%s\n", topic->title+1);

skiplines = 0;
if (topic->title) {
  if (!strcmp (topic->title+1, "Registers")) {
      fprint_reg_help (tmp, dptr) ;
      skiplines = 1;
      }
  else
      if (!strcmp (topic->title+1, "Set commands")) {
          fprint_set_help (tmp, dptr);
          skiplines = 3;
          }
      else
          if (!strcmp (topic->title+1, "Show commands")) {
              fprint_show_help (tmp, dptr);
              skiplines = 3;
              }
  }
rewind (tmp);
if (errno) {
    (void)fprintf (st, "rewind: error %d\r\n", errno);
}

/* Discard leading blank lines/redundant titles */

for (i =0; i < skiplines; i++)
    if (fgets (tbuf, sizeof (tbuf), tmp)) {};

while (fgets (tbuf, sizeof (tbuf), tmp)) {
    if (tbuf[0] != '\n')
        fputs ("    ", st);
    fputs (tbuf, st);
    }
fclose (tmp);
#if defined(_WIN32)
remove (tmpnam);
FREE (tmpnam);
#endif /* if defined(_WIN32) */
return;
}
/* Flatten and display help for those who say they prefer it. */

static t_stat displayFlatHelp (FILE *st, DEVICE *dptr,
                               UNIT *uptr, int32 flag,
                               TOPIC *topic, va_list ap )
{
size_t i;

if (topic->flags & HLP_MAGIC_TOPIC) {
    (void)fprintf (st, "\n%s ", topic->label);
    displayMagicTopic (st, dptr, topic);
    }
else
    (void)fprintf (st, "\n%s %s\n", topic->label, topic->title);

/*
 * Topic text (for magic topics, follows for explanations)
 * It's possible/reasonable for a magic topic to have no text.
 */

if (topic->text)
    fputs (topic->text, st);

for (i = 0; i < topic->kids; i++)
    displayFlatHelp (st, dptr, uptr, flag, topic->children[i], ap);

return SCPE_OK;
}

#define HLP_MATCH_AMBIGUOUS (~0u)
#define HLP_MATCH_WILDCARD  (~1U)
#define HLP_MATCH_NONE      0
static size_t matchHelpTopicName (TOPIC *topic, const char *token)
{
size_t i, match;
char cbuf[CBUFSIZE], *cptr;

if (!strcmp (token, "*"))
    return HLP_MATCH_WILDCARD;

match = 0;
for (i = 0; i < topic->kids; i++) {
    strcpy (cbuf,topic->children[i]->title +
            ((topic->children[i]->flags & HLP_MAGIC_TOPIC)? 1 : 0));
    cptr = cbuf;
    while (*cptr) {
        if (blankch (*cptr)) {
            *cptr++ = '_';
            }
        else {
            *cptr = (char)toupper (*cptr);
            cptr++;
            }
        }
    if (!strcmp (cbuf, token))      /* Exact Match */
        return i+1;
    if (!strncmp (cbuf, token, strlen (token))) {
        if (match)
            return HLP_MATCH_AMBIGUOUS;
        match = i+1;
        }
    }
return match;
}

/* Main help routine */

t_stat scp_vhelp (FILE *st, DEVICE *dptr,
                  UNIT *uptr, int32 flag,
                  const char *help, const char *cptr, va_list ap)
{
TOPIC top;
TOPIC *topic = &top;
int failed;
size_t match;
size_t i;
const char *p;
t_bool flat_help = FALSE;
char cbuf [CBUFSIZE], gbuf[CBUFSIZE];

static const char attach_help[] = { " ATTACH" };
static const char  brief_help[] = { "%s help.  Type <CR> to exit, HELP for navigation help.\n" };
static const char onecmd_help[] = { "%s help.\n" };
static const char   help_help[] = {
    /****|***********************80 column width guide********************************/
    "    To see more HELP information, type the listed subtopic name.  To move\n"
    "    up a level, just type <CR>.  To review the current subtopic, type \"?\".\n"
    "    To view all subtopics, type \"*\".  To exit type \"EXIT\", \"^C\", or \"^D\".\n\n"
    };

(void)memset (&top, 0, sizeof(top));
top.parent = &top;
if ((failed = setjmp (help_env)) != 0) {
    (void)fprintf (stderr, "\nHELP was unable to process HELP for this device.\n"
                           "Error in block %u line %u: %s\n"
                           "%s%*.*s%s\n",
                   (int)help_where.block, (int)help_where.line, help_where.error,
                   help_where.prox ? "Near '" : "",
                   help_where.prox ? 15 : 0, help_where.prox ? 15 : 0,
                   help_where.prox ? help_where.prox : "",
                   help_where.prox ? "'" : "");
    cleanHelp (&top);
    return failed;
    }

/* Compile string into navigation tree */

/* Root */

if (dptr) {
    p = dptr->name;
    flat_help = (dptr->flags & DEV_FLATHELP) != 0;
    }
else
    p = sim_name;
top.title = (char *) malloc (strlen (p) + ((flag & SCP_HELP_ATTACH)? sizeof (attach_help)-1: 0) +1);
if (!top.title)
  {
    (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                   __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
for (i = 0; p[i]; i++ )
    top.title[i] = (char)toupper (p[i]);
top.title[i] = '\0';
if (flag & SCP_HELP_ATTACH)
    strcpy (top.title+i, attach_help);

top.label = (char *) malloc (sizeof ("1"));
if (!top.label)
  {
    (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                   __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
strcpy (top.label, "1");

flat_help = flat_help || !sim_ttisatty() || (flag & SCP_HELP_FLAT);

if (flat_help) {
    flag |= SCP_HELP_FLAT;
    if (sim_ttisatty())
        (void)fprintf (st, "%s help.\nThis help is also available in hierarchical form.\n", top.title);
    else
        (void)fprintf (st, "%s help.\n", top.title);
    }
else
    (void)fprintf (st, ((flag & SCP_HELP_ONECMD)? onecmd_help: brief_help), top.title);

/* Add text and subtopics */

(void) buildHelp (&top, dptr, uptr, help, ap);

/* Go to initial topic if provided */

while (cptr && *cptr) {
    cptr = get_glyph (cptr, gbuf, 0);
    if (!gbuf[0])
        break;
    if (!strcmp (gbuf, "HELP")) {           /* HELP (about help) */
        (void)fprintf (st, "\n");
        fputs (help_help, st);
        break;
        }
    match =  matchHelpTopicName (topic, gbuf);
    if (match == HLP_MATCH_WILDCARD) {
        displayFlatHelp (st, dptr, uptr, flag, topic, ap);
        cleanHelp (&top);
        return SCPE_OK;
        }
    if (match == HLP_MATCH_AMBIGUOUS) {
        (void)fprintf (st, "\n%s is ambiguous in %s\n", gbuf, topic->title);
        break;
        }
    if (match == HLP_MATCH_NONE) {
        (void)fprintf (st, "\n%s is not available in %s\n", gbuf, topic->title);
        break;
        }
    topic = topic->children[match-1];
    }
cptr = NULL;

if (flat_help) {
    displayFlatHelp (st, dptr, uptr, flag, topic, ap);
    cleanHelp (&top);
    return SCPE_OK;
    }

/* Interactive loop displaying help */

while (TRUE) {
    char *pstring;
    const char *prompt[2] = {"? ", "Subtopic? "};

    /* Some magic topic names for help from data structures */

    if (topic->flags & HLP_MAGIC_TOPIC) {
        fputc ('\n', st);
        displayMagicTopic (st, dptr, topic);
        }
    else
        (void)fprintf (st, "\n%s\n", topic->title);

    /* Topic text (for magic topics, follows for explanations)
     * It's possible/reasonable for a magic topic to have no text.
     */

    if (topic->text)
        fputs (topic->text, st);

    if (topic->kids) {
        size_t w = 0;
        char *p;
        char tbuf[CBUFSIZE];

        (void)fprintf (st, "\n    Additional information available:\n\n");
        for (i = 0; i < topic->kids; i++) {
            strcpy (tbuf, topic->children[i]->title +
                    ((topic->children[i]->flags & HLP_MAGIC_TOPIC)? 1 : 0));
            for (p = tbuf; *p; p++) {
                if (blankch (*p))
                    *p = '_';
                }
            w += 4 + topic->kidwid;
            if (w > 80) {
                w = 4 + topic->kidwid;
                fputc ('\n', st);
                }
            (void)fprintf (st, "    %-*s", (int32_t)topic->kidwid, tbuf);
            }
        (void)fprintf (st, "\n\n");
        if (flag & SCP_HELP_ONECMD) {
            pstring = helpPrompt (topic, "", TRUE);
            (void)fprintf (st, "To view additional topics, type HELP %s topicname\n", pstring+1);
            FREE (pstring);
            break;
            }
        }

    if (!sim_ttisatty() || (flag & SCP_HELP_ONECMD))
        break;

  reprompt:
    if (NULL == cptr || !*cptr) {
        if (topic->kids == 0)
            topic = topic->parent;
        pstring = helpPrompt (topic, prompt[topic->kids != 0], FALSE);

        cptr = read_line_p (pstring+1, cbuf, sizeof (cbuf), stdin);
        FREE (pstring);
        if ((cptr != NULL) &&                   /* Got something? */
            ((0 == strcmp (cptr, "\x04")) ||    /* was it a bare ^D? */
             (0 == strcmp (cptr, "\x1A"))))     /* was it a bare ^Z? */
            cptr = NULL;                        /* These are EOF synonyms */
        }

    if (NULL == cptr)                           /* EOF, exit help */
        break;

    cptr = get_glyph (cptr, gbuf, 0);
    if (!strcmp (gbuf, "*")) {              /* Wildcard */
        displayFlatHelp (st, dptr, uptr, flag, topic, ap);
        gbuf[0] = '\0';                     /* Displayed all subtopics, go up */
        }
    if (!gbuf[0]) {                         /* Blank, up a level */
        if (topic->level == 0)
            break;
        topic = topic->parent;
        continue;
        }
    if (!strcmp (gbuf, "?"))                /* ?, repaint current topic */
        continue;
    if (!strcmp (gbuf, "HELP")) {           /* HELP (about help) */
        fputs (help_help, st);
        goto reprompt;
        }
    if (!strcmp (gbuf, "EXIT") || !strcmp (gbuf, "QUIT"))   /* EXIT (help) */
        break;

    /* String - look for that topic */

    if (!topic->kids) {
        (void)fprintf (st, "No additional help at this level.\n");
        cptr = NULL;
        goto reprompt;
        }
    match = matchHelpTopicName (topic, gbuf);
    if (match == HLP_MATCH_AMBIGUOUS) {
        (void)fprintf (st, "%s is ambiguous, please type more of the topic name\n", gbuf);
        cptr = NULL;
        goto reprompt;
        }

    if (match == HLP_MATCH_NONE) {
        (void)fprintf (st, "Help for %s is not available\n", gbuf);
        cptr = NULL;
        goto reprompt;
        }
    /* Found, display subtopic */

    topic = topic->children[match-1];
    }

/* Free structures and return */

cleanHelp (&top);

return SCPE_OK;
}

/* variable argument list shell - most commonly used */

t_stat scp_help (FILE *st, DEVICE *dptr,
                 UNIT *uptr, int32 flag,
                 const char *help, const char *cptr, ...)
{
t_stat r;
va_list ap;

va_start (ap, cptr);
r = scp_vhelp (st, dptr, uptr, flag, help, cptr, ap);
va_end (ap);

return r;
}

#if defined(_MSC_VER)
# pragma warning(pop)
#endif
