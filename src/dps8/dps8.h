/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 6965b612-f62e-11ec-a432-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2012 Dave Jordan
 * Copyright (c) 2013-2018 Charles Anthony
 * Copyright (c) 2016 Jean-Michel Merliot
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#if !defined(DPS8_H)
# define DPS8_H

# if defined(__FAST_MATH__)
#  error __FAST_MATH__ is unsupported
# endif /* if defined(__FAST_MATH__) */

# include <stdio.h>
# include <stdbool.h>
# if defined(THREADZ) || defined(LOCKLESS)
#  include <stdatomic.h>
# endif /* defined(THREADZ) || defined(LOCKLESS) */
# include <errno.h>
# include <inttypes.h>
# include <sys/stat.h>
# include <sys/time.h>
# include <setjmp.h>  // for setjmp/longjmp used by interrupts & faults

# if (defined(__APPLE__) && defined(__MACH__)) || defined(__ANDROID__)
#  include <libgen.h>  // needed for macOS and Android
# endif

# include "dps8_sir.h"

# if defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__) || defined(__x86_64__) || defined(__AMD64) || \
     defined(_M_IX86) || defined(__i386) || defined(__i486) || defined(__i586) || defined(__i686) || defined(__ix86)
#  if HAS_INCLUDE(<immintrin.h>)
#   include <immintrin.h>
#  endif
# endif

# undef HAS_ATTRIBUTE
# if defined __has_attribute && (defined(__clang__) || defined(__GNUC__))
#  define HAS_ATTRIBUTE(atr) __has_attribute(atr)
# else
#  define HAS_ATTRIBUTE(atr) 0
# endif

# undef HOT
# if HAS_ATTRIBUTE(hot)
#  define HOT __attribute__((hot))
# endif
# if !defined(HOT)
#  define HOT
# endif

# undef HAS_BUILTIN
# if defined __has_builtin
#  define HAS_BUILTIN(builtin) __has_builtin(builtin)
# else
#  define HAS_BUILTIN(builtin) 0
# endif

# if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || (defined(__APPLE__) && defined(__MACH__)) || defined(__ANDROID__) || defined(_AIX)
#  undef setjmp
#  define setjmp _setjmp
#  undef longjmp
#  define longjmp _longjmp
# endif

typedef int64_t __int64_t;

# if defined(NEED_128)
typedef struct { uint64_t h; uint64_t l; } x__uint128_t;
typedef struct { int64_t h;  uint64_t l; } x__int128_t;
#  define construct_128(h, l) ((uint128) { (h), (l) })
#  define construct_s128(h, l) ((int128) { (h), (l) })
# endif /* if defined(NEED_128) */

// Quiet compiler unused warnings
# define QUIET_UNUSED

// Enable speed over debugging if not TESTING
# if !defined(TESTING)
#  define SPEED
# endif /* if !defined(TESTING) */

// Experimental dial_out line disconnect delay
// FNP polled ~100Hz; 2 secs. is 200 polls
# define DISC_DELAY 200

// Micro-cache
# undef OLDAPP
# define UCACHE_STATS

// Shift/rotate instruction barrel shifter
# define BARREL_SHIFTER 1

//
// Dependencies
//

# if defined(PANEL68)
#  define PNL(x) x
# else
#  define PNL(x)
# endif /* if defined(PANEL68) */

# define L68_(x) if (cpu.tweaks.l68_mode) { x }
# define DPS8M_(x) if (! cpu.tweaks.l68_mode) { x }

// Debugging tool
# if defined(TESTING)
#  define IF1 if (cpu.tweaks.isolts_mode)
# else
#  define IF1 if (0)
# endif /* if defined(TESTING) */

// DPS8-M supports Hex Mode Floating Point
# define HEX_MODE

// Instruction profiler
// #define MATRIX

// Run TR on work done, not wall clock.
// Define one of these; tied to memory access (MEM)
//  or to instruction execution (EXEC)
//# define TR_WORK_MEM
# define TR_WORK_EXEC

// Storage for main thread id
extern pthread_t main_thread_id;

// Multi-threading may require 'volatile' in some places
# if defined(THREADZ) || defined(LOCKLESS)
#  define vol volatile
#  define volAtomic volatile _Atomic
# else
#  define vol
#  define volAtomic
# endif /* if defined(THREADZ) || defined(LOCKLESS) */

# if !defined(NEED_128)
#  if defined(PRIu64)
#   undef PRIu64
#  endif /* if defined(PRIu64) */
#  if !defined(PRIu64)
#   define PRIu64 "llu"
#  endif /* if !defined(PRIu64) */
#  if defined(PRId64)
#   undef PRId64
#  endif /* if defined(PRId64) */
#  if !defined(PRId64)
#   define PRId64 "lld"
#  endif /* if !defined(PRId64) */
#  if defined(PRIo64)
#   undef PRIo64
#  endif /* if defined(PRIo64) */
#  if !defined(PRIo64)
#   if defined(__HAIKU__)
#    define PRIo64 "lo"
#    undef llo
#    define llo "lo"
#   else
#    define PRIo64 "llo"
#   endif /* if defined(__HAIKU__) */
#  endif /* if !defined(PRIo64) */
# endif /* if !defined(NEED_128) */

# include "../simh/sim_defs.h"           /* simulator defns */
# include "../simh/sim_tape.h"

# if defined(__MINGW32__)
#  include <stdint.h>
typedef t_uint64    u_int64_t;
# endif /* if defined(__MINGW32__) */
# if defined(__HAIKU__)
#  include <stdint.h>
typedef long int64;
typedef unsigned long uint64;
# endif /* if defined(__HAIKU__) */
# if !defined(__HAIKU__)
typedef t_uint64    uint64;
# endif /* if !defined(__HAIKU__) */
# if !defined(_AIX)
#  if !defined(__HAIKU__)
typedef t_int64     int64;
#  endif /* if !defined(__HAIKU__) */
# else
typedef long        int64;
# endif /* if !defined(_AIX) */

/* Data types */

typedef uint8        word1;
typedef uint8        word2;
typedef uint8        word3;
typedef uint8        word4;
typedef uint8        word5;
typedef uint8        word6;
typedef uint8        word7;
typedef uint8        word8;
typedef int8         word8s; // signed 8-bit quantity
typedef uint16       word9;
typedef uint16       word10;
typedef uint16       word11;
typedef uint16       word12;
typedef int16        word12s;
typedef uint16       word13;
typedef uint16       word14;
typedef uint16       word15;
typedef uint16       word16;
typedef uint32       word17;
typedef uint32       word18;
typedef uint32       word19;
typedef int32        word18s;
typedef uint32       word20;
typedef int32        word20s;
typedef uint32       word21;
typedef uint32       word22;
typedef uint32       word23;
typedef uint32       word24;
typedef uint32       word27;
typedef int32        word27s;
typedef uint32       word28;
typedef uint32       word32;
typedef uint64       word34;
typedef uint64       word36;
typedef uint64       word37;
typedef uint64       word38;
typedef int64        word38s;
typedef int64        word36s;
# if !defined(NEED_128)
typedef __uint128_t  word72;
typedef __int128_t   word72s;
typedef __uint128_t  word73;
typedef __uint128_t  word74;
typedef __uint128_t  uint128;
typedef __int128_t   int128;
# else
typedef x__uint128_t word72;
typedef x__int128_t  word72s;
typedef x__uint128_t word73;
typedef x__uint128_t word74;
typedef x__uint128_t uint128;
typedef x__int128_t  int128;
# endif /* if !defined(NEED_128) */

typedef word36       float36;   // single precision float
typedef word72       float72;   // double precision float

typedef unsigned int uint;   //-V677   // efficient unsigned int, at least 32 bits

# include "dps8_simh.h"
# include "dps8_sys.h"
# include "dps8_math128.h"
# include "dps8_hw_consts.h"
# include "dps8_em_consts.h"

# define SETF(flags, x)       flags = ((flags) |  (x))
# define CLRF(flags, x)       flags = ((flags) & ~(x))
# define TSTF(flags, x)       (((flags) & (x)) ? 1 : 0)
# define SCF(cond, flags, x)  { if (cond) SETF((flags), x); else CLRF((flags), x); }

# define SETBIT(dst, bitno)   ((dst)  |  (1LLU << (bitno)))
# define CLRBIT(dst, bitno)   ((dst)  & ~(1LLU << (bitno)))
# define TSTBIT(dst, bitno)   (((dst) &  (1LLU << (bitno))) ? 1: 0)

typedef enum
  {
    UNKNOWN_CYCLE = 0,
    OPERAND_STORE,
    OPERAND_READ,
    INDIRECT_WORD_FETCH,
    RTCD_OPERAND_FETCH,
    INSTRUCTION_FETCH,
    APU_DATA_READ,
    APU_DATA_STORE,
    ABSA_CYCLE,
# if defined(LOCKLESS)
    OPERAND_RMW,
    APU_DATA_RMW,
# endif /* if defined(LOCKLESS) */
  } processor_cycle_type;

# if !defined(LOCKLESS)
#  define OPERAND_RMW   OPERAND_READ
#  define APU_DATA_RMW  APU_DATA_READ
# endif /* if !defined(LOCKLESS) */

# if !defined(EIS_PTR4)
// some breakpoint stuff ...
typedef enum
  {
    UnknownMAT       = 0,
    OperandRead,
    OperandWrite,
    viaPR
  } MemoryAccessType;
# endif

// get 6-bit char @ pos
# define GETCHAR(src, pos) (word6)(((word36)src >> (word36)((5 - pos) * 6)) & 077)
// get 9-bit byte @ pos
# define GETBYTE(src, pos) (word9)(((word36)src >> (word36)((3 - pos) * 9)) & 0777)

# if defined(NEED_128)
#  define YPAIRTO72(ypair) construct_128 ((ypair[0] >> 28) & MASK8,    \
                                         ((ypair[0] & MASK28) << 36) | \
                                          (ypair[1] & MASK36));
# else
#  define YPAIRTO72(ypair)    (((((word72)(ypair[0] & DMASK)) << 36) | \
                                          (ypair[1] & DMASK)) & MASK72)
# endif /* if defined(NEED_128) */

# define GET_TALLY(src) (((src) >> 6) & MASK12)   // 12-bits
# define GET_DELTA(src)  ((src) & MASK6)          // 6-bits

# if !defined(max)
#  define max(a,b)   max2((a),(b))
# endif /* if !defined(max) */
# define max2(a,b)   ((a) > (b) ? (a) : (b))
# define max3(a,b,c) max((a), max((b),(c)))

# if !defined(min)
#  define min(a,b)   min2((a),(b))
# endif /* if !defined(min) */
# define min2(a,b)   ((a) < (b) ? (a) : (b))
# define min3(a,b,c) min((a), min((b),(c)))

// opcode metadata (flag) ...
typedef enum
  {
    READ_OPERAND    = (1U <<  0),  // fetches/reads operand (CA) from memory
    STORE_OPERAND   = (1U <<  1),  // stores/writes operand to memory (its a STR-OP)
# define RMW             (READ_OPERAND | STORE_OPERAND) // a Read-Modify-Write instruction
    READ_YPAIR      = (1U <<  2),  // fetches/reads Y-pair operand (CA) from memory
    STORE_YPAIR     = (1U <<  3),  // stores/writes Y-pair operand to memory
    READ_YBLOCK8    = (1U <<  4),  // fetches/reads Y-block8 operand (CA) from memory
    NO_RPT          = (1U <<  5),  // Repeat instructions not allowed
//#define NO_RPD          (1U << 6)
    NO_RPL          = (1U <<  7),
//#define NO_RPX          (NO_RPT | NO_RPD | NO_RPL)
    READ_YBLOCK16   = (1U <<  8),  // fetches/reads Y-block16 operands from memory
    STORE_YBLOCK16  = (1U <<  9),  // fetches/reads Y-block16 operands from memory
    TRANSFER_INS    = (1U << 10),  // a transfer instruction
    TSPN_INS        = (1U << 11),  // a TSPn instruction
    CALL6_INS       = (1U << 12),  // a call6 instruction
    PREPARE_CA      = (1U << 13),  // prepare TPR.CA for instruction
    STORE_YBLOCK8   = (1U << 14),  // stores/writes Y-block8 operand to memory
    IGN_B29         = (1U << 15),  // Bit-29 has an instruction specific meaning. Ignore.
    NO_TAG          = (1U << 16),  // tag is interpreted differently and for addressing purposes is effectively 0
    PRIV_INS        = (1U << 17),  // privileged instruction
    NO_BAR          = (1U << 18),  // not allowed in BAR mode
//  NO_XEC          = (1U << 19),  // can't be executed via xec/xed
    NO_XED          = (1U << 20),  // No execution via XEC/XED instruction

// EIS operand types

# define EOP_ALPHA 1U

// bits 21, 22
    EOP1_ALPHA      = (EOP_ALPHA << 21),
    EOP1_MASK       = (3U << 21),
# define EOP1_SHIFT 21

// bits 23, 24
    EOP2_ALPHA      = (EOP_ALPHA << 23),
    EOP2_MASK       = (3U << 23),
# define EOP2_SHIFT 23

// bits 25, 26
    EOP3_ALPHA      = (EOP_ALPHA << 25),
    EOP3_MASK       = (3U << 25),
# define EOP3_SHIFT 25

    READ_YBLOCK32   = (1U << 27),  // fetches/reads Y-block16 operands from memory
    STORE_YBLOCK32  = (1U << 28),  // fetches/reads Y-block16 operands from memory
  } opc_flag;

// opcode metadata (disallowed) modifications
enum opc_mod
  {
    NO_DU           = (1U << 0),   // No DU modification allowed (Can these 2 be combined into 1?)
    NO_DL           = (1U << 1),   // No DL modification allowed
# define NO_DUDL    (NO_DU | NO_DL)

    NO_CI           = (1U << 2),   // No character indirect modification (can these next 3 be combined?
    NO_SC           = (1U << 3),   // No sequence character modification
    NO_SCR          = (1U << 4),   // No sequence character reverse modification
# define NO_CSS     (NO_CI | NO_SC | NO_SCR)

# define NO_DLCSS   (NO_DU   | NO_CSS)
# define NO_DDCSS   (NO_DUDL | NO_CSS)

    ONLY_AU_QU_AL_QL_XN = (1U << 5)    // None except au, qu, al, ql, xn
  };

// None except au, qu, al, ql, xn for MF1 and REG
// None except du, au, qu, al, ql, xn for MF2
// None except au, qu, al, ql, xn for MF1, MF2, and MF3

# define IS_NONE(tag) (!(tag))
/*! non-tally: du or dl */
# define IS_DD(tag) ((_TM(tag) != 040U) && \
    ((_TD(tag) == 003U) || (_TD(tag) == 007U)))
/*! tally: ci, sc, or scr */
# define IS_CSS(tag) ((_TM(tag) == 040U) && \
    ((_TD(tag) == 050U) || (_TD(tag) == 052U) || \
    (_TD(tag) == 045U)))
# define IS_DDCSS(tag) (IS_DD(tag) || IS_CSS(tag))
/*! just dl or css */
# define IS_DCSS(tag) (((_TM(tag) != 040U) && (_TD(tag) == 007U)) || IS_CSS(tag))

// !%WRD  ~0200000  017
// !%9    ~0100000  027
// !%6    ~0040000  033
// !%4    ~0020000  035
// !%1    ~0010000  036
enum reg_use { is_WRD =  0174000,
               is_9  =   0274000,
               is_6  =   0334000,
               is_4  =   0354000,
               is_1  =   0364000,
               is_DU =   04000,
               is_OU =   02000,
               ru_A  =   02000 | 01000,
               ru_Q  =   02000 |  0400,
               ru_X0 =   02000 |  0200,
               ru_X1 =   02000 |  0100,
               ru_X2 =   02000 |   040,
               ru_X3 =   02000 |   020,
               ru_X4 =   02000 |   010,
               ru_X5 =   02000 |    04,
               ru_X6 =   02000 |    02,
               ru_X7 =   02000 |    01,
               ru_none = 02000 |     0 };
//, ru_notou = 1024 };

# define ru_AQ (ru_A | ru_Q)
# define ru_Xn(n) (1 << (7 - (n)))

// Basic + EIS opcodes .....
struct opcode_s {
    const char *mne;       // mnemonic
    opc_flag flags;        // various and sundry flags
    enum opc_mod mods;          // disallowed addr mods
    uint ndes;             // number of operand descriptor words for instruction (mw EIS)
    enum reg_use reg_use;  // register usage
};

// operations stuff

/*! Y of instruc word */
# define Y(i) (i & MASKHI18)
/*! X from opcodes in instruc word */
# define OPSX(i) ((i & 0007000LLU) >> 9)
/*! X from OP_* enum, and X from  */
# define X(i) (i & 07U)

enum { OP_1     = 00001U,
    OP_E        = 00002U,
    OP_BAR      = 00003U,
    OP_IC       = 00004U,
    OP_A        = 00005U,
    OP_Q        = 00006U,
    OP_AQ       = 00007U,
    OP_IR       = 00010U,
    OP_TR       = 00011U,
    OP_REGS     = 00012U,

    /* 645/6180 */
    OP_CPR      = 00021U,
    OP_DBR      = 00022U,
    OP_PTP      = 00023U,
    OP_PTR      = 00024U,
    OP_RA       = 00025U,
    OP_SDP      = 00026U,
    OP_SDR      = 00027U,

    OP_X        = 01000U
};

enum eCAFoper {
    unknown = 0,
    readCY,
    writeCY,
    rmwCY,      // Read-Modify-Write
//    readCYpair,
//    writeCYpair,
//    readCYblock8,
//    writeCYblock8,
//    readCYblock16,
//    writeCYblock16,

    prepareCA,
};
typedef enum eCAFoper eCAFoper;

# define READOP(i) ((bool) (i->info->flags    &  \
                           (READ_OPERAND      |  \
                            READ_YPAIR        |  \
                            READ_YBLOCK8      |  \
                            READ_YBLOCK16     |  \
                            READ_YBLOCK32)))

# define WRITEOP(i) ((bool) (i->info->flags   &  \
                            (STORE_OPERAND    |  \
                             STORE_YPAIR      |  \
                             STORE_YBLOCK8    |  \
                             STORE_YBLOCK16   |  \
                             STORE_YBLOCK32)))

// if it's both read and write it's a RMW
# define RMWOP(i) ((bool) READOP(i) && WRITEOP(i))

# define TRANSOP(i) ((bool) (i->info->flags & (TRANSFER_INS) ))

//
// EIS stuff ...
//

// Numeric operand descriptors

// AL39 Table 4-3. Alphanumeric Data Type (TA) Codes
enum
  {
    CTA9   = 0U, // 9-bit bytes
    CTA6   = 1U, // 6-bit characters
    CTA4   = 2U, // 4-bit decimal
    CTAILL = 3U  // Illegal
  };

// TN - Type Numeric AL39 Table 4-3. Alphanumeric Data Type (TN) Codes
enum
  {
    CTN9 = 0U,   // 9-bit
    CTN4 = 1U    // 4-bit
  };

// S - Sign and Decimal Type (AL39 Table 4-4. Sign and Decimal Type (S) Codes)

enum
  {
    CSFL = 0U,   // Floating-point, leading sign
    CSLS = 1U,   // Scaled fixed-point, leading sign
    CSTS = 2U,   // Scaled fixed-point, trailing sign
    CSNS = 3U    // Scaled fixed-point, unsigned
  };

enum
  {
    // Address register flag. This flag controls interpretation of the ADDRESS
    // field of the operand descriptor just as the "A" flag controls
    // interpretation of the ADDRESS field of the basic and EIS single-word
    // instructions.
    MFkAR = 0x40U,
    // Register length control. If RL = 0, then the length (N) field of the
    // operand descriptor contains the length of the operand. If RL = 1, then
    // the length (N) field of the operand descriptor contains a selector value
    // specifying a register holding the operand length. Operand length is
    // interpreted as units of the data size (1-, 4-, 6-, or 9-bit) given in
    // the associated operand descriptor.
    MFkRL = 0x20U,
    // Indirect descriptor control. If ID = 1 for Mfk, then the kth word
    // following the instruction word is an indirect pointer to the operand
    // descriptor for the kth operand; otherwise, that word is the operand
    // descriptor.
    MFkID = 0x10U,

    MFkREGMASK = 0xfU
  };

// EIS instruction take on a life of their own. Need to take into account
// RNR/SNR/BAR etc.
typedef enum
  {
    eisUnknown = 0,  // uninitialized
    eisTA      = 1,  // type alphanumeric
    eisTN      = 2,  // type numeric
    eisBIT     = 3   // bit string
  } eisDataType;

typedef enum
  {
    eRWreadBit = 0,
    eRWwriteBit
  } eRW;

// Misc constants and macros

# define ARRAY_SIZE(a) ( sizeof(a) / sizeof((a)[0]) )

# if defined(FREE)
#  undef FREE
# endif /* if defined(FREE) */
# define FREE(p) do  \
  {                  \
    free((p));       \
    (p) = NULL;      \
  } while(0)

# if defined(__GNUC__) || defined(__clang_version__)
#  if !defined(LIKELY)
#   define LIKELY(x) __builtin_expect(!!(x), 1)
#  endif
#  if !defined(UNLIKELY)
#   define UNLIKELY(x) __builtin_expect(!!(x), 0)
#  endif
# else
#  if !defined(LIKELY)
#   define LIKELY(x) (x)
#  endif
#  if !defined(UNLIKELY)
#   define UNLIKELY(x) (x)
#  endif
# endif

# if defined (__MINGW64__) || \
    defined (__MINGW32__)  || \
    defined (__GNUC__)     || \
    defined (__clang_version__)
#  define UNUSED    __attribute__ ((unused))
# else
#  define UNUSED
# endif

/* Detect proper "does not return" annotation */
# if !defined(NO_RETURN)
#  if defined(__STDC_VERSION__)
#   if __STDC_VERSION__ >= 202311L
#    define NO_RETURN [[noreturn]] /* C23-style */
#   elif __STDC_VERSION__ >= 201112L
#    define NO_RETURN _Noreturn /* C11-style */
#   endif
#  endif
# endif
# if !defined(NO_RETURN)
#  if defined(__GNUC__) || defined(__SUNPRO_C) || defined(__SUNPRO_CC) || \
      defined(__xlc__) || defined(__ibmxl__)
#   define NO_RETURN __attribute__((noreturn)) /* IBM/Sun/GNU-style */
#  endif
# endif
# if !defined(NO_RETURN)
#  define NO_RETURN /* Fallback */
# endif

# define MAX_DEV_NAME_LEN 64

// Basic STDIO for MinGW
# if !defined(__CYGWIN__)
#  if defined(__MINGW32__) || defined(__MINGW64__) || defined(CROSS_MINGW32) || defined(CROSS_MINGW64) || defined(__QNX__)
#   define WIN_STDIO    1
#  endif
# endif /* if !defined(__CYGWIN__) */

//#define SYNCTEST

#endif // if defined(DPS8_H)
