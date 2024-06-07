/*
 * sim_defs.h: simulator definitions
 *
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: MIT
 * scspell-id: ae37b35b-f62a-11ec-8c79-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 1993-2008 Robert M. Supnik
 * Copyright (c) 2021-2024 The DPS8M Development Team
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

/*
 * The interface between the simulator control package (SCP) and the
 * simulator consists of the following routines and data structures
 *
 *      sim_name                simulator name string
 *      sim_devices[]           array of pointers to simulated devices
 *      sim_PC                  pointer to saved PC register descriptor
 *      sim_interval            simulator interval to next event
 *      sim_stop_messages[]     array of pointers to stop messages
 *      sim_instr()             instruction execution routine
 *      sim_emax                maximum number of words in an instruction
 *
 * In addition, the simulator must supply routines to print and parse
 * architecture specific formats
 *
 *      print_sym               print symbolic output
 *      parse_sym               parse symbolic input
 */

#if !defined(SIM_DEFS_H_)
# define SIM_DEFS_H_    0

# include <stddef.h>
# include <stdlib.h>
# include <stdio.h>
# if defined(_MSC_VER) && (_MSC_VER < 1900)
#  define snprintf _snprintf
# endif
# include <stdarg.h>
# include <string.h>
# include <errno.h>
# include <limits.h>
# include <ctype.h>
# include <math.h>
# include <setjmp.h>

# if defined(_WIN32)
#  include <winsock2.h>
#  undef PACKED                       /* avoid macro name collision */
#  undef ERROR                        /* avoid macro name collision */
#  undef MEM_MAPPED                   /* avoid macro name collision */
#  include <process.h>
# endif /* if defined(_WIN32) */

# include <sys/param.h>               /* MAXPATHLEN */

/* avoid internal macro name collisions */

# if defined(PMASK)
#  undef PMASK
# endif /* if defined(PMASK) */

# if defined(RS)
#  undef RS
# endif /* if defined(RS) */

# if defined(PAGESIZE)
#  undef PAGESIZE
# endif /* if defined(PAGESIZE) */

# if !defined(TRUE)
#  define TRUE 1
# endif /* if !defined(TRUE) */

# if !defined(FALSE)
#  undef FALSE
#  define FALSE 0
# endif /* if !defined(FALSE) */

# if defined(__CYGWIN__)
#  if !defined(USE_FLOCK)
#   define USE_FLOCK 1
#  endif /* if !defined(USE_FLOCK) */
#  if !defined(USE_FCNTL)
#   define USE_FCNTL 1
#  endif /* if !defined(USE_FCNTL) */
# endif /* if defined(__CYGWIN__) */

/*
 * SCP API shim.
 *
 * The SCP API for version 4.0 introduces a number of "pointer-to-const"
 * parameter qualifiers that were not present in the 3.x versions.  To maintain
 * compatibility with the earlier versions, the new qualifiers are expressed as
 * "CONST" rather than "const".  This allows macro removal of the qualifiers
 * when compiling for SIMH 3.x.
 */

# if !defined(CONST)
#  define CONST const
# endif /* if !defined(CONST) */

/* Length specific integer declarations */

typedef signed char     int8;
typedef signed short    int16;
typedef signed int      int32;
typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned int    uint32;
typedef int             t_stat;                         /* status */
typedef int             t_bool;                         /* boolean */

/* 64b integers */

# if defined (__GNUC__)                                  /* GCC */
typedef signed long long        t_int64;
typedef unsigned long long      t_uint64;
# elif defined (_WIN32)                                  /* Windows */
typedef signed __int64          t_int64;
typedef unsigned __int64        t_uint64;
# else                                                   /* default */
#  define t_int64                 signed long long
#  define t_uint64                unsigned long long
# endif                                                  /* end 64b */
# include <stdint.h>
# if !defined(INT64_C)
#  define INT64_C(x)      x ## LL
# endif /* if !defined(INT64_C) */

typedef t_int64         t_svalue;                       /* signed value */
typedef t_uint64        t_value;                        /* value */
typedef uint32          t_addr;
# define T_ADDR_W        32
# define T_ADDR_FMT      ""

# if defined (_WIN32)
#  undef vsnprintf
#  define vsnprintf _vsnprintf
# endif
# define STACKBUFSIZE 2048

# if defined (_WIN32) /* Actually, a GCC issue */
#  define LL_FMT "I64"
# else
#  define LL_FMT "ll"
# endif

# if defined (_WIN32)
#  define NULL_DEVICE "NUL:"
# else
#  define NULL_DEVICE "/dev/null"
# endif

/* Stubs for inlining */

# if defined(_MSC_VER)
#  define SIM_INLINE _inline
# elif defined(__GNUC__) || defined(__clang_version__) || defined(__xlc__)
#  define SIM_INLINE inline
# else
#  define SIM_INLINE
# endif

/* System independent definitions */

# define FLIP_SIZE       (1 << 16)                       /* flip buf size */
# if !defined (PATH_MAX)                                 /* usually in limits */
#  define PATH_MAX        512
# endif
# if (PATH_MAX >= 128)
#  define CBUFSIZE        (4096 + PATH_MAX)                /* string buf size */
# else
#  define CBUFSIZE        4224
# endif

/* Breakpoint spaces definitions */

# define SIM_BKPT_N_SPC  (1 << (32 - SIM_BKPT_V_SPC))    /* max number spaces */
# define SIM_BKPT_V_SPC  (BRK_TYP_MAX + 1)               /* location in arg */

/* Extended switch definitions (bits >= 26) */

# define SIM_SW_HIDE     (1u << 26)                      /* enable hiding */
# define SIM_SW_REST     (1u << 27)                      /* attach/restore */
# define SIM_SW_REG      (1u << 28)                      /* register value */
# define SIM_SW_STOP     (1u << 29)                      /* stop message */
# define SIM_SW_SHUT     (1u << 30)                      /* shutdown */

/*
 * Simulator status codes
 *
 * 0                    ok
 * 1 - (SCPE_BASE - 1)  simulator specific
 * SCPE_BASE - n        general
 */

# define SCPE_OK         0                               /* normal return */
# define SCPE_BASE       64                              /* base for messages */
# define SCPE_NXM        (SCPE_BASE + 0)                 /* nxm */
# define SCPE_UNATT      (SCPE_BASE + 1)                 /* no file */
# define SCPE_IOERR      (SCPE_BASE + 2)                 /* I/O error */
# define SCPE_CSUM       (SCPE_BASE + 3)                 /* loader cksum */
# define SCPE_FMT        (SCPE_BASE + 4)                 /* loader format */
# define SCPE_NOATT      (SCPE_BASE + 5)                 /* not attachable */
# define SCPE_OPENERR    (SCPE_BASE + 6)                 /* open error */
# define SCPE_MEM        (SCPE_BASE + 7)                 /* alloc error */
# define SCPE_ARG        (SCPE_BASE + 8)                 /* argument error */
# define SCPE_STEP       (SCPE_BASE + 9)                 /* step expired */
# define SCPE_UNK        (SCPE_BASE + 10)                /* unknown command */
# define SCPE_RO         (SCPE_BASE + 11)                /* read only */
# define SCPE_INCOMP     (SCPE_BASE + 12)                /* incomplete */
# define SCPE_STOP       (SCPE_BASE + 13)                /* sim stopped */
# define SCPE_EXIT       (SCPE_BASE + 14)                /* sim exit */
# define SCPE_TTIERR     (SCPE_BASE + 15)                /* console tti err */
# define SCPE_TTOERR     (SCPE_BASE + 16)                /* console tto err */
# define SCPE_EOF        (SCPE_BASE + 17)                /* end of file */
# define SCPE_REL        (SCPE_BASE + 18)                /* relocation error */
# define SCPE_NOPARAM    (SCPE_BASE + 19)                /* no parameters */
# define SCPE_ALATT      (SCPE_BASE + 20)                /* already attached */
# define SCPE_TIMER      (SCPE_BASE + 21)                /* hwre timer err */
# define SCPE_SIGERR     (SCPE_BASE + 22)                /* signal err */
# define SCPE_TTYERR     (SCPE_BASE + 23)                /* tty setup err */
# define SCPE_SUB        (SCPE_BASE + 24)                /* subscript err */
# define SCPE_NOFNC      (SCPE_BASE + 25)                /* func not imp */
# define SCPE_UDIS       (SCPE_BASE + 26)                /* unit disabled */
# define SCPE_NORO       (SCPE_BASE + 27)                /* rd only not ok */
# define SCPE_INVSW      (SCPE_BASE + 28)                /* invalid switch */
# define SCPE_MISVAL     (SCPE_BASE + 29)                /* missing value */
# define SCPE_2FARG      (SCPE_BASE + 30)                /* too few arguments */
# define SCPE_2MARG      (SCPE_BASE + 31)                /* too many arguments */
# define SCPE_NXDEV      (SCPE_BASE + 32)                /* nx device */
# define SCPE_NXUN       (SCPE_BASE + 33)                /* nx unit */
# define SCPE_NXREG      (SCPE_BASE + 34)                /* nx register */
# define SCPE_NXPAR      (SCPE_BASE + 35)                /* nx parameter */
# define SCPE_NEST       (SCPE_BASE + 36)                /* nested DO */
# define SCPE_IERR       (SCPE_BASE + 37)                /* internal error */
# define SCPE_MTRLNT     (SCPE_BASE + 38)                /* tape rec lnt error */
# define SCPE_LOST       (SCPE_BASE + 39)                /* Telnet conn lost */
# define SCPE_TTMO       (SCPE_BASE + 40)                /* Telnet conn timeout */
# define SCPE_STALL      (SCPE_BASE + 41)                /* Telnet conn stall */
# define SCPE_AFAIL      (SCPE_BASE + 42)                /* assert failed */
# define SCPE_INVREM     (SCPE_BASE + 43)                /* invalid remote console command */
# define SCPE_NOTATT     (SCPE_BASE + 44)                /* not attached */
# define SCPE_EXPECT     (SCPE_BASE + 45)                /* expect matched */
# define SCPE_REMOTE     (SCPE_BASE + 46)                /* remote console command */

# define SCPE_MAX_ERR    (SCPE_BASE + 47)                /* Maximum SCPE Error Value */
# define SCPE_KFLAG      0x1000                          /* tti data flag */
# define SCPE_BREAK      0x2000                          /* tti break flag */
# define SCPE_NOMESSAGE  0x10000000                      /* message display suppression flag */
# define SCPE_BARE_STATUS(stat) ((stat) & ~(SCPE_NOMESSAGE|SCPE_KFLAG|SCPE_BREAK))

/* Print value format codes */

# define PV_RZRO         0                               /* right, zero fill */
# define PV_LEFT         3                               /* left justify */

/* Default timing parameters */

# define KBD_POLL_WAIT   5000                            /* keyboard poll */
# define KBD_MAX_WAIT    500000
# define SERIAL_IN_WAIT  100                             /* serial in time */
# define SERIAL_OUT_WAIT 100                             /* serial output */
# define NOQUEUE_WAIT    1000000                         /* min check time */
# define KBD_LIM_WAIT(x) (((x) > KBD_MAX_WAIT)? KBD_MAX_WAIT: (x))
# define KBD_WAIT(w,s)   ((w)? w: KBD_LIM_WAIT (s))

/* Convert switch letter to bit mask */

# define SWMASK(x) (1u << (((int) (x)) - ((int) 'A')))

/* String match - at least one character required */

# define MATCH_CMD(ptr,cmd) ((NULL == (ptr)) || (!*(ptr)) || strncmp ((ptr), (cmd), strlen (ptr)))

/* End of Linked List/Queue value                           */
/* Chosen for 2 reasons:                                    */
/*     1 - to not be NULL, this allowing the NULL value to  */
/*         indicate inclusion on a list                     */
/* and                                                      */
/*     2 - to not be a valid/possible pointer (alignment)   */
# define QUEUE_LIST_END ((UNIT *)1)

/* Typedefs for principal structures */

typedef struct DEVICE DEVICE;
typedef struct UNIT UNIT;
typedef struct REG REG;
typedef struct CTAB CTAB;
typedef struct C1TAB C1TAB;
typedef struct SHTAB SHTAB;
typedef struct MTAB MTAB;
typedef struct SCHTAB SCHTAB;
typedef struct BRKTAB BRKTAB;
typedef struct BRKTYPTAB BRKTYPTAB;
typedef struct EXPTAB EXPTAB;
typedef struct EXPECT EXPECT;
typedef struct SEND SEND;
typedef struct DEBTAB DEBTAB;
typedef struct FILEREF FILEREF;
typedef struct BITFIELD BITFIELD;

typedef t_stat (*ACTIVATE_API)(UNIT *unit, int32 interval);

/* Device data structure */

struct DEVICE {
    const char          *name;                          /* name */
    UNIT                *units;                         /* units */
    REG                 *registers;                     /* registers */
    MTAB                *modifiers;                     /* modifiers */
    uint32              numunits;                       /* #units */
    uint32              aradix;                         /* address radix */
    uint32              awidth;                         /* address width */
    uint32              aincr;                          /* addr increment */
    uint32              dradix;                         /* data radix */
    uint32              dwidth;                         /* data width */
    t_stat              (*examine)(t_value *v, t_addr a, UNIT *up,
                            int32 sw);                  /* examine routine */
    t_stat              (*deposit)(t_value v, t_addr a, UNIT *up,
                            int32 sw);                  /* deposit routine */
    t_stat              (*reset)(DEVICE *dp);           /* reset routine */
    t_stat              (*boot)(int32 u, DEVICE *dp);
                                                        /* boot routine */
    t_stat              (*attach)(UNIT *up, CONST char *cp);
                                                        /* attach routine */
    t_stat              (*detach)(UNIT *up);            /* detach routine */
    void                *ctxt;                          /* context */
    uint32              flags;                          /* flags */
    uint32              dctrl;                          /* debug control */
    DEBTAB              *debflags;                      /* debug flags */
    t_stat              (*msize)(UNIT *up, int32 v, CONST char *cp, void *dp);
                                                        /* mem size routine */
    char                *lname;                         /* logical name */
    t_stat              (*help)(FILE *st, DEVICE *dptr,
                            UNIT *uptr, int32 flag, const char *cptr);
                                                        /* help */
    t_stat              (*attach_help)(FILE *st, DEVICE *dptr,
                            UNIT *uptr, int32 flag, const char *cptr);
                                                        /* attach help */
    void *help_ctx;                                     /* Context available to help routines */
    const char          *(*description)(DEVICE *dptr);  /* Device Description */
    BRKTYPTAB           *brk_types;                     /* Breakpoint types */
    };

/* Device flags */

# define DEV_V_DIS       0                               /* dev disabled */
# define DEV_V_DISABLE   1                               /* dev disable-able */
# define DEV_V_DYNM      2                               /* mem size dynamic */
# define DEV_V_DEBUG     3                               /* debug capability */
# define DEV_V_TYPE      4                               /* Attach type */
# define DEV_S_TYPE      3                               /* Width of Type Field */
# define DEV_V_SECTORS   7                               /* Unit Capacity is in 512byte sectors */
# define DEV_V_DONTAUTO  8                               /* Do not auto detach already attached units */
# define DEV_V_FLATHELP  9                               /* Use traditional (unstructured) help */
# define DEV_V_NOSAVE    10                              /* Don't save device state */
# define DEV_V_UF_31     12                              /* user flags, V3.1 */
# define DEV_V_UF        16                              /* user flags */
# define DEV_V_RSV       31                              /* reserved */

# define DEV_DIS         (1 << DEV_V_DIS)                /* device is currently disabled */
# define DEV_DISABLE     (1 << DEV_V_DISABLE)            /* device can be set enabled or disabled */
# define DEV_DYNM        (1 << DEV_V_DYNM)               /* device requires call on msize routine to change memory size */
# define DEV_DEBUG       (1 << DEV_V_DEBUG)              /* device supports SET DEBUG command */
# define DEV_SECTORS     (1 << DEV_V_SECTORS)            /* capacity is 512 byte sectors */
# define DEV_DONTAUTO    (1 << DEV_V_DONTAUTO)           /* Do not auto detach already attached units */
# define DEV_FLATHELP    (1 << DEV_V_FLATHELP)           /* Use traditional (unstructured) help */
# define DEV_NOSAVE      (1 << DEV_V_NOSAVE)             /* Don't save device state */
# define DEV_NET         0                               /* Deprecated - meaningless */

# define DEV_TYPEMASK    (((1 << DEV_S_TYPE) - 1) << DEV_V_TYPE)
# define DEV_DISK        (1 << DEV_V_TYPE)               /* sim_disk Attach */
# define DEV_TAPE        (2 << DEV_V_TYPE)               /* sim_tape Attach */
# define DEV_MUX         (3 << DEV_V_TYPE)               /* sim_tmxr Attach */
# define DEV_ETHER       (4 << DEV_V_TYPE)               /* Ethernet Device */
# define DEV_DISPLAY     (5 << DEV_V_TYPE)               /* Display Device */
# define DEV_TYPE(dptr)  ((dptr)->flags & DEV_TYPEMASK)

# define DEV_UFMASK_31   (((1u << DEV_V_RSV) - 1) & ~((1u << DEV_V_UF_31) - 1))
# define DEV_UFMASK      (((1u << DEV_V_RSV) - 1) & ~((1u << DEV_V_UF) - 1))
# define DEV_RFLAGS      (DEV_UFMASK|DEV_DIS)            /* restored flags */

/* Unit data structure

   Parts of the unit structure are device specific, that is, they are
   not referenced by the simulator control package and can be freely
   used by device simulators.  Fields starting with 'buf', and flags
   starting with 'UF', are device specific.  The definitions given here
   are for a typical sequential device.
*/

struct UNIT {
    UNIT                *next;                          /* next active */
    t_stat              (*action)(UNIT *up);            /* action routine */
    char                *filename;                      /* open file name */
    FILE                *fileref;                       /* file reference */
    void                *filebuf;                       /* memory buffer */
    uint32              hwmark;                         /* high water mark */
    int32               time;                           /* time out */
    uint32              flags;                          /* flags */
    uint32              dynflags;                       /* dynamic flags */
    t_addr              capac;                          /* capacity */
    t_addr              pos;                            /* file position */
    void                (*io_flush)(UNIT *up);          /* io flush routine */
    uint32              iostarttime;                    /* I/O start time */
    int32               buf;                            /* buffer */
    int32               wait;                           /* wait */
    int32               u3;                             /* device specific */
    int32               u4;                             /* device specific */
    int32               u5;                             /* device specific */
    int32               u6;                             /* device specific */
    void                *up7;                           /* device specific */
    void                *up8;                           /* device specific */
    void                *tmxr;                          /* TMXR linkage */
    void                (*cancel)(UNIT *);
    };

/* Unit flags */

# define UNIT_V_UF_31    12              /* dev spec, V3.1 */
# define UNIT_V_UF       16              /* device specific */
# define UNIT_V_RSV      31              /* reserved!! */

# define UNIT_ATTABLE    0000001         /* attachable */
# define UNIT_RO         0000002         /* read only */
# define UNIT_FIX        0000004         /* fixed capacity */
# define UNIT_SEQ        0000010         /* sequential */
# define UNIT_ATT        0000020         /* attached */
# define UNIT_BINK       0000040         /* K = power of 2 */
# define UNIT_BUFABLE    0000100         /* bufferable */
# define UNIT_MUSTBUF    0000200         /* must buffer */
# define UNIT_BUF        0000400         /* buffered */
# define UNIT_ROABLE     0001000         /* read only ok */
# define UNIT_DISABLE    0002000         /* disable-able */
# define UNIT_DIS        0004000         /* disabled */
# define UNIT_IDLE       0040000         /* idle eligible */

/* Unused/meaningless flags */
# define UNIT_TEXT       0000000         /* text mode - no effect */

# define UNIT_UFMASK_31  (((1u << UNIT_V_RSV) - 1) & ~((1u << UNIT_V_UF_31) - 1))
# define UNIT_UFMASK     (((1u << UNIT_V_RSV) - 1) & ~((1u << UNIT_V_UF) - 1))
# define UNIT_RFLAGS     (UNIT_UFMASK|UNIT_DIS)          /* restored flags */

/* Unit dynamic flags (dynflags) */

/* These flags are only set dynamically */

# define UNIT_ATTMULT    0000001         /* Allow multiple attach commands */
# define UNIT_TM_POLL    0000002         /* TMXR Polling unit */
# define UNIT_NO_FIO     0000004         /* fileref is NOT a FILE * */
# define UNIT_DISK_CHK   0000010         /* disk data debug checking (sim_disk) */
# define UNIT_TMR_UNIT   0000020         /* Unit registered as a calibrated timer */
# define UNIT_V_DF_TAPE  5               /* Bit offset for Tape Density reservation */
# define UNIT_S_DF_TAPE  3               /* Bits Reserved for Tape Density */

struct BITFIELD {
    const char      *name;                               /* field name */
    uint32          offset;                              /* starting bit */
    uint32          width;                               /* width */
    const char      **valuenames;                        /* map of values to strings */
    const char      *format;                             /* value format string */
    };

/* Register data structure */

struct REG {
    CONST char          *name;                           /* name */
    void                *loc;                            /* location */
    uint32              radix;                           /* radix */
    uint32              width;                           /* width */
    uint32              offset;                          /* starting bit */
    uint32              depth;                           /* save depth */
    const char          *desc;                           /* description */
    BITFIELD            *fields;                         /* bit fields */
    uint32              flags;                           /* flags */
    uint32              qptr;                            /* circ q ptr */
    size_t              str_size;                        /* structure size */
    };

/* Register flags */

# define REG_FMT         00003                           /* see PV_x */
# define REG_RO          00004                           /* read only */
# define REG_HIDDEN      00010                           /* hidden */
# define REG_NZ          00020                           /* must be non-zero */
# define REG_UNIT        00040                           /* in unit struct */
# define REG_STRUCT      00100                           /* in structure array */
# define REG_CIRC        00200                           /* circular array */
# define REG_VMIO        00400                           /* use VM data print/parse */
# define REG_VMAD        01000                           /* use VM addr print/parse */
# define REG_FIT         02000                           /* fit access to size */
# define REG_HRO         (REG_RO | REG_HIDDEN)           /* hidden, read only */

# define REG_V_UF        16                              /* device specific */
# define REG_UFMASK      (~((1u << REG_V_UF) - 1))       /* user flags mask */
# define REG_VMFLAGS     (REG_VMIO | REG_UFMASK)         /* call VM routine if any of these are set */

/* Command tables, base and alternate formats */

struct CTAB {
    const char          *name;                           /* name */
    t_stat              (*action)(int32 flag, CONST char *cptr);
                                                         /* action routine */
    int32               arg;                             /* argument */
    const char          *help;                           /* help string/structured locator */
    const char          *help_base;                      /* structured help base*/
    void                (*message)(const char *unechoed_cmdline, t_stat stat);
                                                         /* message printing routine */
    };

struct C1TAB {
    const char          *name;                           /* name */
    t_stat              (*action)(DEVICE *dptr, UNIT *uptr,
                            int32 flag, CONST char *cptr);/* action routine */
    int32               arg;                             /* argument */
    const char          *help;                           /* help string */
    };

struct SHTAB {
    const char          *name;                           /* name */
    t_stat              (*action)(FILE *st, DEVICE *dptr,
                            UNIT *uptr, int32 flag, CONST char *cptr);
    int32               arg;                             /* argument */
    const char          *help;                           /* help string */
    };

/* Modifier table - only extended entries have disp, reg, or flags */

struct MTAB {
    uint32              mask;                            /* mask */
    uint32              match;                           /* match */
    const char          *pstring;                        /* print string */
    const char          *mstring;                        /* match string */
    t_stat              (*valid)(UNIT *up, int32 v, CONST char *cp, void *dp);
                                                         /* validation routine */
    t_stat              (*disp)(FILE *st, UNIT *up, int32 v, CONST void *dp);
                                                         /* display routine */
    void                *desc;                           /* value descriptor */
                                                         /* REG * if MTAB_VAL */
                                                         /* int * if not */
    const char          *help;                           /* help string */
    };

/* mtab mask flag bits */
/* NOTE: MTAB_VALR and MTAB_VALO are only used to display help */
# define MTAB_XTD        (1u << UNIT_V_RSV)              /* ext entry flag */
# define MTAB_VDV        (0001 | MTAB_XTD)               /* valid for dev */
# define MTAB_VUN        (0002 | MTAB_XTD)               /* valid for unit */
# define MTAB_VALR       (0004 | MTAB_XTD)               /* takes a value (required) */
# define MTAB_VALO       (0010 | MTAB_XTD)               /* takes a value (optional) */
# define MTAB_NMO        (0020 | MTAB_XTD)               /* only if named */
# define MTAB_NC         (0040 | MTAB_XTD)               /* no UC conversion */
# define MTAB_QUOTE      (0100 | MTAB_XTD)               /* quoted string */
# define MTAB_SHP        (0200 | MTAB_XTD)               /* show takes parameter */
# define MODMASK(mptr,flag) (((mptr)->mask & (uint32)(flag)) == (uint32)(flag))/* flag mask test */

/* Search table */

struct SCHTAB {
    size_t              logic;                           /* logical operator */
    size_t              boolop;                          /* boolean operator */
    uint32              count;                           /* value count in mask and comp arrays */
    t_value             *mask;                           /* mask for logical */
    t_value             *comp;                           /* comparison for boolean */
    };

/* Breakpoint table */

struct BRKTAB {
    t_addr              addr;                            /* address */
    uint32              typ;                             /* mask of types */
# define BRK_TYP_USR_TYPES       ((1 << ('Z'-'A'+1)) - 1)/* all types A-Z */
# define BRK_TYP_DYN_STEPOVER    (SWMASK ('Z'+1))
# define BRK_TYP_DYN_USR         (SWMASK ('Z'+2))
# define BRK_TYP_DYN_ALL         (BRK_TYP_DYN_USR|BRK_TYP_DYN_STEPOVER) /* Mask of All Dynamic types */
# define BRK_TYP_TEMP            (SWMASK ('Z'+3))        /* Temporary (one-shot) */
# define BRK_TYP_MAX             (('Z'-'A')+3)           /* Maximum breakpoint type */
    int32               cnt;                             /* proceed count */
    char                *act;                            /* action string */
    double              time_fired[SIM_BKPT_N_SPC];      /* instruction count when match occurred */
    BRKTAB *next;                                        /* list with same address value */
    };

/* Breakpoint table */

struct BRKTYPTAB {
    uint32      btyp;                                    /* type mask */
    const char *desc;                                    /* description */
    };
# define BRKTYPE(typ,descrip) {SWMASK(typ), descrip}

/* Expect rule */

struct EXPTAB {
    uint8               *match;                          /* match string */
    size_t              size;                            /* match string size */
    char                *match_pattern;                  /* match pattern for format */
    int32               cnt;                             /* proceed count */
    int32               switches;                        /* flags */
# define EXP_TYP_PERSIST         (SWMASK ('P'))          /* rule persists after match/default once rule matches, remove it */
# define EXP_TYP_CLEARALL        (SWMASK ('C'))          /* clear rules after matching rule/default once rule matches, remove it */
# define EXP_TYP_REGEX           (SWMASK ('R'))          /* rule pattern is a regular expression */
# define EXP_TYP_REGEX_I         (SWMASK ('I'))          /* regular expression pattern matching should be case independent */
# define EXP_TYP_TIME            (SWMASK ('T'))          /* halt delay is in microseconds instead of instructions */
    char                *act;                            /* action string */
    };

/* Expect Context */

struct EXPECT {
    DEVICE              *dptr;                           /* Device (for Debug) */
    uint32              dbit;                            /* Debugging Bit */
    EXPTAB              *rules;                          /* match rules */
    size_t              size;                            /* count of match rules */
    uint32              after;                           /* delay before halting */
    uint8               *buf;                            /* buffer of output data which has produced */
    size_t              buf_ins;                         /* buffer insertion point for the next output data */
    size_t              buf_size;                        /* buffer size */
    };

/* Send Context */

struct SEND {
    uint32              delay;                           /* instruction delay between sent data */
# define SEND_DEFAULT_DELAY  1000                        /* default delay instruction count */
    DEVICE              *dptr;                           /* Device (for Debug) */
    uint32              dbit;                            /* Debugging Bit */
    uint32              after;                           /* instruction delay before sending any data */
    double              next_time;                       /* execution time when next data can be sent */
    uint8               *buffer;                         /* buffer */
    size_t              bufsize;                         /* buffer size */
    size_t              insoff;                          /* insert offset */
    size_t              extoff;                          /* extra offset */
    };

/* Debug table */

struct DEBTAB {
    const char          *name;                           /* control name */
    uint32              mask;                            /* control bit */
    const char          *desc;                           /* description */
    };

/* Deprecated Debug macros.  Use sim_debug() */

# define DEBUG_PRS(d)    (sim_deb && d.dctrl)
# define DEBUG_PRD(d)    (sim_deb && d->dctrl)
# define DEBUG_PRI(d,m)  (sim_deb && (d.dctrl & (m)))
# define DEBUG_PRJ(d,m)  (sim_deb && ((d)->dctrl & (m)))

# define SIM_DBG_EVENT       0x10000
# define SIM_DBG_ACTIVATE    0x20000
# define SIM_DBG_AIO_QUEUE   0x40000

/* File Reference */
struct FILEREF {
    char                name[CBUFSIZE];                  /* file name */
    FILE                *file;                           /* file handle */
    int32               refcount;                        /* reference count */
    };

/*
   The following macros exist to help populate structure contents

   They are dependent on the declaration order of the fields
   of the structures they exist to populate.

 */

# define UDATA(act,fl,cap) NULL,act,NULL,NULL,NULL,0,0,(fl),0,(cap),0,NULL,0,0

# if defined (__STDC__) || defined (_WIN32) /* Variants which depend on how macro arguments are converted to strings */
/* Generic Register declaration for all fields.
   If the register structure is extended, this macro will be retained and a
   new macro will be provided that populates the new register structure */
#  define REGDATA(nm,loc,rdx,wd,off,dep,desc,flds,fl,qptr,siz) \
    #nm, &(loc), (rdx), (wd), (off), (dep), (desc), (flds), (fl), (qptr), (siz)
/* Right Justified Octal Register Data */
#  define ORDATA(nm,loc,wd) #nm, &(loc), 8, (wd), 0, 1, NULL, NULL
/* Right Justified Decimal Register Data */
#  define DRDATA(nm,loc,wd) #nm, &(loc), 10, (wd), 0, 1, NULL, NULL
/* Right Justified Hexadecimal Register Data */
#  define HRDATA(nm,loc,wd) #nm, &(loc), 16, (wd), 0, 1, NULL, NULL
/* Right Justified Binary Register Data */
#  define BINRDATA(nm,loc,wd) #nm, &(loc), 2, (wd), 0, 1, NULL, NULL
/* One-bit binary flag at an arbitrary offset in a 32-bit word Register */
#  define FLDATA(nm,loc,pos) #nm, &(loc), 2, 1, (pos), 1, NULL, NULL
/* Arbitrary location and Radix Register */
#  define GRDATA(nm,loc,rdx,wd,pos) #nm, &(loc), (rdx), (wd), (pos), 1, NULL, NULL
/* Arrayed register whose data is kept in a standard C array Register */
#  define BRDATA(nm,loc,rdx,wd,dep) #nm, (loc), (rdx), (wd), 0, (dep), NULL, NULL
/* Same as above, but with additional description initializer */
#  define ORDATAD(nm,loc,wd,desc) #nm, &(loc), 8, (wd), 0, 1, (desc), NULL
#  define DRDATAD(nm,loc,wd,desc) #nm, &(loc), 10, (wd), 0, 1, (desc), NULL
#  define HRDATAD(nm,loc,wd,desc) #nm, &(loc), 16, (wd), 0, 1, (desc), NULL
#  define BINRDATAD(nm,loc,wd,desc) #nm, &(loc), 2, (wd), 0, 1, (desc), NULL
#  define FLDATAD(nm,loc,pos,desc) #nm, &(loc), 2, 1, (pos), 1, (desc), NULL
#  define GRDATAD(nm,loc,rdx,wd,pos,desc) #nm, &(loc), (rdx), (wd), (pos), 1, (desc), NULL
#  define BRDATAD(nm,loc,rdx,wd,dep,desc) #nm, (loc), (rdx), (wd), 0, (dep), (desc), NULL
/* Same as above, but with additional description initializer, and bitfields */
#  define ORDATADF(nm,loc,wd,desc,flds) #nm, &(loc), 8, (wd), 0, 1, (desc), (flds)
#  define DRDATADF(nm,loc,wd,desc,flds) #nm, &(loc), 10, (wd), 0, 1, (desc), (flds)
#  define HRDATADF(nm,loc,wd,desc,flds) #nm, &(loc), 16, (wd), 0, 1, (desc), (flds)
#  define BINRDATADF(nm,loc,wd) #nm, &(loc), 2, (wd), 0, 1, NULL, NULL
#  define FLDATADF(nm,loc,pos,desc,flds) #nm, &(loc), 2, 1, (pos), 1, (desc), (flds)
#  define GRDATADF(nm,loc,rdx,wd,pos,desc,flds) #nm, &(loc), (rdx), (wd), (pos), 1, (desc), (flds)
#  define BRDATADF(nm,loc,rdx,wd,dep,desc,flds) #nm, (loc), (rdx), (wd), 0, (dep), (desc), (flds)
#  define BIT(nm)              {#nm, 0xffffffff, 1}             /* Single Bit definition */
#  define BITNC                {"",  0xffffffff, 1}             /* Don't care Bit definition */
#  define BITF(nm,sz)          {#nm, 0xffffffff, sz}            /* Bit Field definition */
#  define BITNCF(sz)           {"",  0xffffffff, sz}            /* Don't care Bit Field definition */
#  define BITFFMT(nm,sz,fmt)   {#nm, 0xffffffff, sz, NULL, #fmt}/* Bit Field definition with Output format */
#  define BITFNAM(nm,sz,names) {#nm, 0xffffffff, sz, names}     /* Bit Field definition with value->name map */
# else /* For non-STD-C compiler which can't stringify macro arguments with # */
#  define REGDATA(nm,loc,rdx,wd,off,dep,desc,flds,fl,qptr,siz) \
    "nm", &(loc), (rdx), (wd), (off), (dep), (desc), (flds), (fl), (qptr), (siz)
#  define ORDATA(nm,loc,wd) "nm", &(loc), 8, (wd), 0, 1, NULL, NULL
#  define DRDATA(nm,loc,wd) "nm", &(loc), 10, (wd), 0, 1, NULL, NULL
#  define HRDATA(nm,loc,wd) "nm", &(loc), 16, (wd), 0, 1, NULL, NULL
#  define BINRDATA(nm,loc,wd) "nm", &(loc), 2, (wd), 0, 1, NULL, NULL
#  define FLDATA(nm,loc,pos) "nm", &(loc), 2, 1, (pos), 1, NULL, NULL
#  define GRDATA(nm,loc,rdx,wd,pos) "nm", &(loc), (rdx), (wd), (pos), 1, NULL, NULL
#  define BRDATA(nm,loc,rdx,wd,dep) "nm", (loc), (rdx), (wd), 0, (dep), NULL, NULL
#  define ORDATAD(nm,loc,wd,desc) "nm", &(loc), 8, (wd), 0, 1, (desc), NULL
#  define DRDATAD(nm,loc,wd,desc) "nm", &(loc), 10, (wd), 0, 1, (desc), NULL
#  define HRDATAD(nm,loc,wd,desc) "nm", &(loc), 16, (wd), 0, 1, (desc), NULL
#  define BINRDATAD(nm,loc,wd,desc) "nm", &(loc), 2, (wd), 0, 1, (desc), NULL
#  define FLDATAD(nm,loc,pos,desc) "nm", &(loc), 2, 1, (pos), 1, (desc), NULL
#  define GRDATAD(nm,loc,rdx,wd,pos,desc) "nm", &(loc), (rdx), (wd), (pos), 1, (desc), NULL
#  define BRDATAD(nm,loc,rdx,wd,dep,desc) "nm", (loc), (rdx), (wd), 0, (dep), (desc), NULL
#  define ORDATADF(nm,loc,wd,desc,flds) "nm", &(loc), 8, (wd), 0, 1, (desc), (flds)
#  define DRDATADF(nm,loc,wd,desc,flds) "nm", &(loc), 10, (wd), 0, 1, (desc), (flds)
#  define HRDATADF(nm,loc,wd,desc,flds) "nm", &(loc), 16, (wd), 0, 1, (desc), (flds)
#  define BINRDATADF(nm,loc,wd,desc,flds) "nm", &(loc), 2, (wd), 0, 1, (desc), (flds)
#  define FLDATADF(nm,loc,pos,desc,flds) "nm", &(loc), 2, 1, (pos), 1, (desc), (flds)
#  define GRDATADF(nm,loc,rdx,wd,pos,desc,flds) "nm", &(loc), (rdx), (wd), (pos), 1, (desc), (flds)
#  define BRDATADF(nm,loc,rdx,wd,dep,desc,flds) "nm", (loc), (rdx), (wd), 0, (dep), (desc), (flds)
#  define BIT(nm)              {"nm", 0xffffffff, 1}              /* Single Bit definition */
#  define BITNC                {"",   0xffffffff, 1}              /* Don't care Bit definition */
#  define BITF(nm,sz)          {"nm", 0xffffffff, sz}             /* Bit Field definition */
#  define BITNCF(sz)           {"",   0xffffffff, sz}             /* Don't care Bit Field definition */
#  define BITFFMT(nm,sz,fmt)   {"nm", 0xffffffff, sz, NULL, "fmt"}/* Bit Field definition with Output format */
#  define BITFNAM(nm,sz,names) {"nm", 0xffffffff, sz, names}      /* Bit Field definition with value->name map */
# endif
# define ENDBITS {NULL}  /* end of bitfield list */

/* Arrayed register whose data is part of the UNIT structure */
# define URDATA(nm,loc,rdx,wd,off,dep,fl) \
    REGDATA(nm,(loc),(rdx),(wd),(off),(dep),NULL,NULL,((fl) | REG_UNIT),0,0)
/* Arrayed register whose data is part of an arbitrary structure */
# define STRDATA(nm,loc,rdx,wd,off,dep,siz,fl) \
    REGDATA(nm,(loc),(rdx),(wd),(off),(dep),NULL,NULL,((fl) | REG_STRUCT),0,(siz))
/* Same as above, but with additional description initializer */
# define URDATAD(nm,loc,rdx,wd,off,dep,fl,desc) \
    REGDATA(nm,(loc),(rdx),(wd),(off),(dep),(desc),NULL,((fl) | REG_UNIT),0,0)
# define STRDATAD(nm,loc,rdx,wd,off,dep,siz,fl,desc) \
    REGDATA(nm,(loc),(rdx),(wd),(off),(dep),(desc),NULL,((fl) | REG_STRUCT),0,(siz))
/* Same as above, but with additional description initializer, and bitfields */
# define URDATADF(nm,loc,rdx,wd,off,dep,fl,desc,flds) \
    REGDATA(nm,(loc),(rdx),(wd),(off),(dep),(desc),(flds),((fl) | REG_UNIT),0,0)
# define STRDATADF(nm,loc,rdx,wd,off,dep,siz,fl,desc,flds) \
    REGDATA(nm,(loc),(rdx),(wd),(off),(dep),(desc),(flds),((fl) | REG_STRUCT),0,(siz))

/* Function prototypes */

# include "scp.h"
# include "sim_console.h"
# include "sim_timer.h"
# include "sim_fio.h"

# if defined(FREE)
#  undef FREE
# endif /* if defined(FREE) */
# define FREE(p) do  \
  {                  \
    free((p));       \
    (p) = NULL;      \
  } while(0)

/* Consistent PATH_MAX */

# if !defined(MAXPATHLEN)
#  if defined(PATH_MAX) && PATH_MAX > 1024
#   define MAXPATHLEN PATH_MAX
#  else
#   define MAXPATHLEN 1024
#  endif /* if defined(PATH_MAX) && PATH_MAX > 1024 */
# endif /* if !defined(MAXPATHLEN) */

# if !defined(PATH_MAX)
#  define PATH_MAX MAXPATHLEN
# endif /* if !defined(PATH_MAX) */

/* Macro to ALWAYS execute the specified expression and fail if it evaluates to false. */

/* This replaces any references to "assert()" which should never be invoked */
/* with an expression which causes side effects (i.e. must be executed for */
/* the program to work correctly) */

# define ASSURE(_Expression) while (!(_Expression))                                         \
         {                                                                                  \
           fprintf(stderr, "%s failed at %s line %d\n", #_Expression, __FILE__, __LINE__);  \
           sim_printf("%s failed at %s line %d\n", #_Expression, __FILE__, __LINE__);       \
           abort();\
         }
#endif
