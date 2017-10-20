/*
 Copyright (c) 2007-2013 Michael Mondy
 Copyright 2012-2016 by Harry Reed
 Copyright 2013-2016 by Charles Anthony

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

#ifndef DPS8_HW_CONSTS_H
#define DPS8_HW_CONSTS_H

#include "dps8_math128.h"

/////////////////////////////////////
//
// SCU/Memory
//

#define XXX_TEMP_SCU_SUBPORT 0

enum { N_SCU_PORTS = 8 };
enum { N_SCU_SUBPORTS = 4 };
enum { N_ASSIGNMENTS = 2 };
// Number of interrupts in an interrupt cell register
enum { N_CELL_INTERRUPTS = 32 };

#define PASIZE          24                       /* phys addr width */
#define PAMASK          ((1U << PASIZE) - 1U)
// Mask out low bit
#define PAEVEN          (((1U << (PASIZE - 1)) - 1U) << 1)
#define MEM_SIZE_MAX    (1U << PASIZE)           /* maximum memory */

// The minimum allocation size of a SCU is 64K (2^16) 
// (2 banks of 32K). Call it an SCBANK
#define SCBANK          (1U << 16)

// Maximum memory size is MEM_SIZE_MAX, number of
// scbanks is:
#define N_SCBANKS ((MEM_SIZE_MAX) / (SCBANK))

//
// Memory addressing
//


#define VASIZE          18                       // virtual addr width
#define AMASK           ((1U << VASIZE) - 1U)    // virtual addr mask
#define SEGSIZE         (1U << VASIZE)           // size of segment in words


/////////////////////////////////////
//
// Append unit memory paging
//

#define PGSZ 1024u
#define PGMK 1023u

//
// IOM
//


enum { MAX_CHANNELS = 64 };
enum { N_IOM_PORTS = 8 };
// The number of devices that a dev_code can address (6 bit number)
enum { N_DEV_CODES = 64 };
enum { IOM_CONNECT_CHAN = 2 };

/////////////////////////////////////
//
// Words
//


#define MAX18           0777777U
#define MAX18POS        0377777U                 //  2**17-1 
#define MAX18NEG        0400000U                 // -2**17 
#define SIGN18          0400000U
// NB. these 2 use the wrong bit number convention
#define BIT19          01000000U                 // carry out bit from 18 bit arithmetic
#define BIT20          02000000U                 // carry out bit from 19 bit arithmetic
#define MASK36         0777777777777LLU          // data mask 
#define DMASK           MASK36
#define MASK10          0001777U                 // 10-bit data mask
#define MASK14          0037777U                 // 14-bit data mask
#define MASK16          0177777U                 // 16-bit data mask
#define MASK17          0377777U                 // 17-bit data mask
#define MASK18          0777777U                 // 18-bit data mask
#define WMASK           MASK18                   // WORDNO mask
#define MASKLO18        0000000777777LLU
#define MASKHI18        0777777000000LLU
#define MASK20          03777777U                // 20-bit data mask
#define MASK24          077777777U               // 24-bit data mask
#define MASK28          01777777777U             // 28-bit data mask
#define SIGN24          040000000U
#define SIGN36          0400000000000LLU         // sign bit of a 36-bit word
// NB. these 3 use the wrong bit number convention
#define BIT37          01000000000000LLU         // carry out bit from 36 bit arithmetic
#define BIT38          02000000000000LLU         // carry out bit from 37 bit arithmetic
#define BIT35           0200000000000LLU         // next to the sign bit
#define MASK32          037777777777U
#define MASK15          077777U
#define SMASK           MASK15                   // Segment number mask
#define SIGN15          040000U                  // sign mask 15-bit number
#define MAGMASK         0377777777777LLU         // magnitude mask 
#define ONES            0777777777777LLU
#define NEG136          0777777777777LLU         // -1
#define MAXPOS          0377777777777LLU         //  2**35-1
#define MAXNEG          0400000000000LLU         // -2**35
#define MAX36           0777777777777LLU         // 2**36
#define MAX72           (((word72)1U << 72) - 1U)  // 72 1's

#define CARRY           01000000000000LLU        // carry from 2 36-bit additions/subs
#define ZEROEXT         0777777777777LLU         // mask to zero extend a 36 => 64-bit int
#define ZEROEXT18       0777777U                 // mask to zero extend a 18 => 32-bit int

#ifdef NEED_128

#define SIGN72          (construct_128 (0200U, 0U))
// NB. these use the wrong bit number convention
#define BIT68           (construct_128 (010U, 0U))
#define BIT69           (construct_128 (020U, 0U))
#define BIT70           (construct_128 (040U, 0U))
#define BIT71           (construct_128 (0100U, 0U)) // next to the sign bit
#define BIT73           (construct_128 (0400U, 0U)) // carry out bit from 72 bit arithmetic
#define BIT74           (construct_128 (01000U, 0U)) // carry out bit from 73 bit arithmetic
#define MASK63          0x7FFFFFFFFFFFFFFF
#define MASK64          0xFFFFFFFFFFFFFFFF
#define MASK68          (construct_128 (017U, MASK64)) // Hex mode mantissa normalization mask
#define MASK70          (construct_128 (0077U, MASK64))
#define MASK71          (construct_128 (0177U, MASK64))
#define MASK72          (construct_128 (0377U, MASK64))

#else

#define SIGN72          ((word72)1U << 71)
// NB. these use the wrong bit number convention
#define BIT68           ((word72)1U << 67)
#define BIT69           ((word72)1U << 68)
#define BIT70           ((word72)1U << 69)
#define BIT71           ((word72)1U << 70)  // next to the sign bit
#define BIT73           ((word72)1U << 72)       // carry out bit from 72 bit arithmetic
#define BIT74           ((word72)1U << 73)       // carry out bit from 73 bit arithmetic

#define MASK68          (((word72)1U << 68) - 1U) // Hex mode mantissa normalization mask
#define MASK70          (((word72)1U << 70) - 1U)
#define MASK71          (((word72)1U << 71) - 1U)
#define MASK72          (((word72)1U << 72) - 1U)
#define ZEROEXT72       (((word72)1U << 72) - 1U)  // mask to zero extend a 72 => 128 int

#endif

#define SIGN64          ((uint64)1U << 63)

#define MASK2           03U
#define MASK3           07U
#define MASK4           017U
#define MASK5           037U
#define MASK6           077U
#define MASK7           0177U

#define SIGN8           0200U                    // sign mask 8-bit number
#define MASK8           0377U                    // 8-bit mask
#define MASK9           0777U                    // 9-bit mask

#define MASK11          03777U

#define SIGN12          0x800U                   // sign mask 12-bit number
#define MASK12          07777U

#define SIGN6           0040U                    // sign bit of 6-bit signed numfer (e.g. Scaling Factor)

#define MASK35          0377777777777llu

#define MASKBITS(x)     ( ~(~((uint64)0)<<x) )   // lower (x) bits all ones
#define MASKBITS18(x)   ( ~(~((word18)0)<<x) )   // lower (x) bits all ones
#define MASKBITS72(x)   ( ~(~((word72)0)<<x) )   // lower (x) bits all ones

#define GETHI36(a)      ((word18) (((a) >> 18) & MASK18))
#define GETLO36(a)      ((word18) ((a) & MASK18))
#define SETHI36(a,b)    (((a) &= MASKLO18), ((a) |= ((((word36)(b) & MASKLO18) << 18))))
#define SETLO36(a,b)    (((a) &= MASKHI18), ((a) |= ((word36)(b) & MASKLO18)))
#define GETHI(a)        GETHI36((a))
#define GETLO(a)        GETLO36((a))
#define SETHI(a,b)      SETHI36((a),(b))
#define SETLO(a,b)      SETLO36((a),(b))

#define GETHI72(a)      ((word36) (((a) >> 36) & MASK36))
#define GETLO72(a)      ((word36) ((a) & MASK36))
#define SETHI72(a,b)    ((a) &= MASK36, (a) |= ((((word72)(b) & MASK36)) << 36))
#define SETLO72(a,b)    ((a) &= MASK36 << 36, (a) |= ((word72)(b) & MASK36))

#define GET24(a)        ((word24) ((a) & MASK24))
#define MASK21          07777777llu
#define SIGN21          04000000llu
#define MASK22          017777777llu
#define SIGN22          010000000llu
#define MASK27          0777777777llu


// Sign extend DPS8M words into host words

static inline int SIGNEXT6_int (word6 w)
  {
    if (w & SIGN6)
      {
        return ((int) w) | (int) (((uint) -1) << 6);
      }
    return w & MASK6;
  }

static inline int SIGNEXT8_int (word8 w)
  {
    if (w & SIGN8)
      {
        return ((int) w) | (int) (((uint) -1) << 8);
      }
    return w & MASK8;
  }

static inline int32 SIGNEXT15_32 (word15 w)
  {
    if (w & SIGN15)
      {
        return ((int32) w) | (int32) (((uint32) -1) << 15);
      }
    return w & MASK15;
  }

static inline int32 SIGNEXT18_32 (word18 w)
  {
    if (w & SIGN18)
      {
        return ((int32) w) | (int32) (((uint32) -1) << 18);
      }
    return w & MASK18;
  }

static inline int32 SIGNEXT21_32 (word21 w)
  {
    if (w & SIGN21)
      {
        return ((int32) w) | (int32) (((uint32) -1) << 21);
      }
    return w & MASK21;
  }

static inline int32 SIGNEXT22_32 (word22 w)
  {
    if (w & SIGN22)
      {
        return ((int32) w) | (int32) (((uint32) -1) << 22);
      }
    return w & MASK22;
  }

static inline int32 SIGNEXT24_32 (word24 w)
  {
    if (w & SIGN24)
      {
        return ((int32) w) | (int32) (((uint32) -1) << 24);
      }
    return w & MASK24;
  }

static inline t_int64 SIGNEXT36_64 (word36 w)
  {
    if (w & SIGN36)
      {
        return ((t_int64) w) | (t_int64) (((t_uint64) -1ll) << 36);
      }
    return w & MASK36;
  }

static inline t_int64 SIGNEXT18_64 (word36 w)
  {
    if (w & SIGN18)
      {
        return ((t_int64) w) | (t_int64) (((t_uint64) -1ll) << 18);
      }
    return w & MASK18;
  }

static inline t_int64 SIGNEXT21_64 (word36 w)
  {
    if (w & SIGN21)
      {
        return ((t_int64) w) | (t_int64) (((t_uint64) -1ll) << 21);
      }
    return w & MASK21;
  }

static inline t_int64 SIGNEXT22_64 (word36 w)
  {
    if (w & SIGN22)
      {
        return ((t_int64) w) | (t_int64) (((t_uint64) -1ll) << 22);
      }
    return w & MASK22;
  }

static inline t_int64 SIGNEXT24_64 (word36 w)
  {
    if (w & SIGN24)
      {
        return ((t_int64) w) | (t_int64) (((t_uint64) -1ll) << 24);
      }
    return w & MASK24;
  }

static inline int128 SIGNEXT72_128 (word72 w)
  {
#ifdef NEED_128
    if (isnonzero_128 (and_128 (w, SIGN72)))
      {
        uint128 v = or_128 (w, construct_128 (0xFFFFFFFFFFFFFF80, 0));
        return cast_s128 (v);
      }
    uint128 v =  and_128 (w, MASK72);
    return (int128) { (__int64_t) v.h, v.l};
#else
    if (w & SIGN72)
      {
        return ((int128) w) | (int128) (((uint128) -1ll) << 72);
      }
    return w & MASK72;
#endif
  }

#ifdef NEED_128
static inline int128 SIGNEXT36_128 (word36 w)
  {
    if (w & SIGN36)
      {
        return construct_s128 ((int64_t) MASK64, w | 0xFFFFFFF000000000);
      }
    return construct_s128 (0, w);
  }
#endif

// Sign extend DPS8M words into DPS8M words
// NB: The high order bits in the host container will
// set to 0; you cannot do host math with
// these results.

static inline word18 SIGNEXT15_18 (word15 w)
  {
    if (w & SIGN15)
      {
        return (w | ((word18) -1) << 15) & MASK18;
      }
    return w & MASK15;
  }

static inline word24 SIGNEXT18_24 (word18 w)
  {
    if (w & SIGN18)
      {
        return (w | ((word24) -1) << 18) & MASK24;
      }
    return w & MASK18;
  }

static inline word72 SIGNEXT36_72 (word36 w)
  {
#ifdef NEED_128
    if (w & SIGN36)
      {
        //return (w | ((word72) DMASK) << 36) & MASK72;
        return construct_128 (0377U, (w & MASK36) | 0xFFFFFFF000000000);
      }
    //return w & MASK36;
    return construct_128 (0, w & MASK36);
#else
    if (w & SIGN36)
      {
        return (w | ((word72) DMASK) << 36) & MASK72;
      }
    return w & MASK36;
#endif
  }

#define SETS36(x)       ((x) | SIGN36)
#define CLRS36(x)       ((x) & ~SIGN36)
#define TSTS36(x)       ((x) & SIGN36)

/////////////////////////////////////
//
// Instruction format
//

#define INST_V_TAG      0                        // Tag
#define INST_M_TAG      077U
#define INST_V_A        6                        // Indirect via pointer
#define INST_M_A        1U
#define INST_V_I        7                        // Interrupt Inhibit
#define INST_M_I        1U
#define INST_V_OP       9                        // opcode
#define INST_M_OP       0777U
#define INST_V_OPX      8                        // opcode etension 
#define INST_M_OPX      1U

#define INST_V_ADDR     18                       // Address
#define INST_M_ADDR     0777777U
#define INST_V_OFFSET   18                       // Offset (Bit29=1)
#define INST_M_OFFSET   077777U
#define INST_V_PRN      33                       // n of PR[n] (Bit29=1)
#define INST_M_PRN      07U
#define INST_V_ARN      33                       // n of AR[n] (Bit29=1)
#define INST_M_ARN      07U


#define GET_TAG(x)      ((word6)  ( (x)                   & INST_M_TAG ))
#define GET_A(x)        ((word1)  (((x) >> INST_V_A)      & INST_M_A   ))
#define GET_I(x)        ((int32)  (((x) >> INST_V_I)      & INST_M_I   ))
#define GET_OP(x)       ((word9)  (((x) >> INST_V_OP)     & INST_M_OP ))
#define GET_OPX(x)      ((bool)   (((x) >> INST_V_OPX)    & INST_M_OPX))

#define GET_OFFSET(x)   ((word15) (((x) >> INST_V_OFFSET) & INST_M_OFFSET))
#define GET_PRN(x)      ((word3)  (((x) >> INST_V_PRN)    & INST_M_PRN))
#define GET_ARN(x)      ((word3)  (((x) >> INST_V_ARN)    & INST_M_ARN))

#define GET_TM(x)       (       (GET_TAG(x) & 060U))
#define GET_TD(x)       (       (GET_TAG(x) & 017U))

#define GET_ADDR(x)     ((uint32) (((x) >> INST_V_ADDR) & INST_M_ADDR))

// tag defines ...
#define TAG_R           0U
#define TAG_RI          1U
#define TAG_IT          2U
#define TAG_IR          3U


#define _TD(tag)        ((tag) & 017U)
#define _TM(tag)        ((tag) & 060U)

enum {
    TD_N        = 000U,
    TD_AU       = 001U,
    TD_QU       = 002U,
    TD_DU       = 003U,
    TD_IC       = 004U,
    TD_AL       = 005U,
    TD_QL       = 006U,
    TD_DL       = 007U,
    TD_X0       = 010U,
    TD_X1       = 011U,
    TD_X2       = 012U,
    TD_X3       = 013U,
    TD_X4       = 014U,
    TD_X5       = 015U,
    TD_X6       = 016U,
    TD_X7       = 017U
};

enum {
    TM_R        = 000U,
    TM_RI       = 020U,
    TM_IT       = 040U,  // HWR - next 2 had values swapped
    TM_IR       = 060U
};

/* see AL39, pp 6-13, tbl 6-3 */
enum {
    IT_F1       = 000U,
    IT_SD       = 004U,
    IT_SCR      = 005U,
    IT_F2       = 006U,
    IT_F3       = 007U,
    IT_CI       = 010U,
    IT_I        = 011U,
    IT_SC       = 012U,
    IT_AD       = 013U,
    IT_DI       = 014U,
    IT_DIC      = 015U,
    IT_ID       = 016U,
    IT_IDC      = 017U,
    
    // not really IT, but they're in it's namespace
    SPEC_ITP  = 001U,
    SPEC_ITS  = 003U
};

#define GET_TB(tag)     ((tag) & 040U)
#define GET_CF(tag)     ((tag) & 007U)

#define _TB(tag)        GET_TB((tag))
#define _CF(tag)        GET_CF((tag))

#define TB6            000U // 6-bit characters
#define TB9            040U // 9-bit characters

/////////////////////////////////////
//
// ITS/ITP
//

#define ISITP(x)                (((x) & INST_M_TAG) == 041U)
#define GET_ITP_PRNUM(Ypair)    ((word3)  (((Ypair)[0] >> 33) & 07U))
#define GET_ITP_WORDNO(Ypair)   ((word18) (((Ypair)[1] >> 18) & WMASK))
#define GET_ITP_BITNO(Ypair)    ((word6)  (((Ypair)[1] >>  9) & 077U))
#define GET_ITP_MOD(Ypair)      (GET_TAG((Ypair)[1]))

#define ISITS(x)                (((x) & INST_M_TAG) == 043U)
#define GET_ITS_SEGNO(Ypair)    ((word15) (((Ypair)[0] >> 18) & SMASK))
#define GET_ITS_RN(Ypair)       ((word3)  (((Ypair)[0] >> 15) & 07))
#define GET_ITS_WORDNO(Ypair)   ((word18) (((Ypair)[1] >> 18) & WMASK))
#define GET_ITS_BITNO(Ypair)    ((word6)  (((Ypair)[1] >>  9) & 077))
#define GET_ITS_MOD(Ypair)      (GET_TAG((Ypair)[1]))

/////////////////////////////////////
//
// Indicator register bits
//

#define F_V_A           17      // Zero
#define F_V_B           16      // Negative
#define F_V_C           15      // Carry
#define F_V_D           14      // Overflow
#define F_V_E           13      // Exponent Overflow
#define F_V_F           12      // Exponent Underflow
#define F_V_G           11      // Overflow Mask
#define F_V_H           10      // Tally Runout
#define F_V_I            9      // Parity Error
#define F_V_J            8      // Parity Mask
#define F_V_K            7      // Not BAR mode
#define F_V_L            6      // Truncation
#define F_V_M            5      // Mid Instruction Interrupt Fault
#define F_V_N            4      // Absolute Mode
#define F_V_O            3      // Hex Mode

#define F_A             (1LLU << F_V_A)
#define F_B             (1LLU << F_V_B)
#define F_C             (1LLU << F_V_C)
#define F_D             (1LLU << F_V_D)
#define F_E             (1LLU << F_V_E)
#define F_F             (1LLU << F_V_F)
#define F_G             (1LLU << F_V_G)
#define F_H             (1LLU << F_V_H)
#define F_I             (1LLU << F_V_I)
#define F_J             (1LLU << F_V_J)
#define F_K             (1LLU << F_V_K)
#define F_L             (1LLU << F_V_L)
#define F_M             (1LLU << F_V_M)
#define F_N             (1LLU << F_V_N)
#define F_O             (1LLU << F_V_O)

#ifdef DPS8M
#define I_HEX   F_O     // base-16 exponent                 0000010
#endif
#define I_ABS   F_N     // absolute mode                    0000020
#define I_MIF   F_M     // mid-instruction interrupt fault  0000040
#define I_TRUNC F_L     // truncation                       0000100
#define I_NBAR  F_K     // not BAR mode                     0000200
#define I_PMASK F_J     // parity mask                      0000400
#define I_PERR  F_I     // parity error                     0001000
#define I_TALLY F_H     // tally runout                     0002000
#define I_OMASK F_G     // overflow mask                    0004000
#define I_EUFL  F_F     // exponent underflow               0010000
#define I_EOFL  F_E     // exponent overflow                0020000
#define I_OFLOW F_D     // overflow                         0040000
#define I_CARRY F_C     // carry                            0100000
#define I_NEG   F_B     // negative                         0200000
#define I_ZERO  F_A     // zero                             0400000

#define I_ZNOC (I_ZERO | I_NEG | I_OFLOW | I_CARRY)
#define I_ZNC (I_ZERO | I_NEG | I_CARRY)

#define CLR_I_ABS   CLRF (cpu.cu.IR, I_ABS)
#define CLR_I_MIF   CLRF (cpu.cu.IR, I_MIF)
#define CLR_I_TRUNC CLRF (cpu.cu.IR, I_TRUNC)
#define CLR_I_NBAR  CLRF (cpu.cu.IR, I_NBAR)
#define CLR_I_TALLY CLRF (cpu.cu.IR, I_TALLY)
#define CLR_I_PMASK CLRF (cpu.cu.IR, I_PMASK)
#define CLR_I_EOFL  CLRF (cpu.cu.IR, I_EOFL)
#define CLR_I_EUFL  CLRF (cpu.cu.IR, I_EUFL)
#define CLR_I_OFLOW CLRF (cpu.cu.IR, I_OFLOW)
#define CLR_I_CARRY CLRF (cpu.cu.IR, I_CARRY)
#define CLR_I_NEG   CLRF (cpu.cu.IR, I_NEG)
#define CLR_I_ZERO  CLRF (cpu.cu.IR, I_ZERO)

#define SET_I_ABS   SETF (cpu.cu.IR, I_ABS)
#define SET_I_NBAR  SETF (cpu.cu.IR, I_NBAR)
#define SET_I_TRUNC SETF (cpu.cu.IR, I_TRUNC)
#define SET_I_TALLY SETF (cpu.cu.IR, I_TALLY)
#define SET_I_EOFL  SETF (cpu.cu.IR, I_EOFL)
#define SET_I_EUFL  SETF (cpu.cu.IR, I_EUFL)
#define SET_I_OFLOW SETF (cpu.cu.IR, I_OFLOW)
#define SET_I_CARRY SETF (cpu.cu.IR, I_CARRY)
#define SET_I_NEG   SETF (cpu.cu.IR, I_NEG)
#define SET_I_ZERO  SETF (cpu.cu.IR, I_ZERO)

#define TST_I_ABS   TSTF (cpu.cu.IR, I_ABS)
#define TST_I_MIF   TSTF (cpu.cu.IR, I_MIF)
#define TST_I_NBAR  TSTF (cpu.cu.IR, I_NBAR)
#define TST_I_PMASK TSTF (cpu.cu.IR, I_PMASK)
#define TST_I_TRUNC TSTF (cpu.cu.IR, I_TRUNC)
#define TST_I_TALLY TSTF (cpu.cu.IR, I_TALLY)
#define TST_I_OMASK TSTF (cpu.cu.IR, I_OMASK)
#define TST_I_EUFL  TSTF (cpu.cu.IR, I_EUFL )
#define TST_I_EOFL  TSTF (cpu.cu.IR, I_EOFL )
#define TST_I_OFLOW TSTF (cpu.cu.IR, I_OFLOW)
#define TST_I_CARRY TSTF (cpu.cu.IR, I_CARRY)
#define TST_I_NEG   TSTF (cpu.cu.IR, I_NEG)
#define TST_I_ZERO  TSTF (cpu.cu.IR, I_ZERO)
#define TST_I_HEX   TSTF (cpu.cu.IR, I_HEX)

#ifdef DPS8M
#define SC_I_HEX(v)   SCF (v, cpu.cu.IR, I_HEX)
#endif
#define SC_I_MIF(v)   SCF (v, cpu.cu.IR, I_MIF)
#define SC_I_TALLY(v) SCF (v, cpu.cu.IR, I_TALLY)
#define SC_I_NEG(v)   SCF (v, cpu.cu.IR, I_NEG)
#define SC_I_ZERO(v)  SCF (v, cpu.cu.IR, I_ZERO)
#define SC_I_CARRY(v) SCF (v, cpu.cu.IR, I_CARRY);
#define SC_I_OFLOW(v) SCF (v, cpu.cu.IR, I_OFLOW);
#define SC_I_EOFL(v)  SCF (v, cpu.cu.IR, I_EOFL);
#define SC_I_EUFL(v)  SCF (v, cpu.cu.IR, I_EUFL);
#define SC_I_OMASK(v) SCF (v, cpu.cu.IR, I_OMASK);
#define SC_I_PERR(v)  SCF (v, cpu.cu.IR, I_PERR);
#define SC_I_PMASK(v) SCF (v, cpu.cu.IR, I_PMASK);
#define SC_I_TRUNC(v) SCF (v, cpu.cu.IR, I_TRUNC);



/////////////////////////////////////
//
//  floating-point constants
//

#define FLOAT36MASK     01777777777LLU               ///< user to extract mantissa from single precision C(CEAQ)
#define FLOAT72MASK     01777777777777777777777LLU   ///< use to extract mastissa from double precision C(EAQ)
#define FLOAT72SIGN     (1LLU << 63)                 ///< mantissa sign mask for full precision C(EAQ)
// XXX beware the 72's are not what they seem!


/////////////////////////////////////
//
// Faults
//

#define N_FAULT_GROUPS 7
#define N_FAULTS 32

enum _fault
  {
    FAULT_SDF  =  0U, // shutdown fault
    FAULT_STR  =  1U, // store fault
    FAULT_MME  =  2U, // master mode entry
    FAULT_F1   =  3U, // fault tag 1
    FAULT_TRO  =  4U, // timer runout fault
    FAULT_CMD  =  5U, // command
    FAULT_DRL  =  6U, // derail
    FAULT_LUF  =  7U, // lockup
    FAULT_CON  =  8U, // connect
    FAULT_PAR  =  9U, // parity
    FAULT_IPR  = 10U, // illegal proceedure
    FAULT_ONC  = 11U, // operation not complete
    FAULT_SUF  = 12U, // startup
    FAULT_OFL  = 13U, // overflow
    FAULT_DIV  = 14U, // divide check
    FAULT_EXF  = 15U, // execute
    FAULT_DF0  = 16U, // directed fault 0
    FAULT_DF1  = 17U, // directed fault 1
    FAULT_DF2  = 18U, // directed fault 2
    FAULT_DF3  = 19U, // directed fault 3
    FAULT_ACV  = 20U, // access violation
    FAULT_MME2 = 21U, // Master mode entry 2
    FAULT_MME3 = 22U, // Master mode entry 3
    FAULT_MME4 = 23U, // Master mode entry 4
    FAULT_F2   = 24U, // fault tag 2
    FAULT_F3   = 25U, // fault tag 3
    FAULT_UN1  = 26U, // unassigned
    FAULT_UN2  = 27U, // unassigned
    FAULT_UN3  = 28U, // unassigned
    FAULT_UN4  = 29U, // unassigned
    FAULT_UN5  = 30U, // unassigned
    FAULT_TRB  = 31U  // Trouble
  };

#define FAULTBASE_MASK  07740U       ///< mask off all but top 7 msb


typedef enum _fault _fault;

#if 0
    //no_fault_subtype = 0,

    // FAULT_IPR

    //flt_ipr_ill_op,     // An illegal operation code has been detected.
    //flt_ipr_ill_mod,    // An illegal address modifier has been detected.
    //flt_ipr_ill_slv,    // An illegal BAR mode procedure has been encountered.
    //flt_ipr_ill_dig,    // An illegal decimal digit or sign has been detected by the decimal unit.
    //flt_ipr_ill_proc,   // An illegal procedure other than the four above has been encountered.


    // FAULT_PAR

    //proc_paru,  // A parity error has been detected in the upper 36 bits of data. (Yeah, right)
    //proc_parl,  // A parity error has been detected in the lower 36 bits of data. (Yeah, right)



    // FAULT_ONC 

    //da_err,     // Operation not complete. Processor/system controller interface sequence error 1 has been detected. (Yeah, right)
    //da_err2,    // Operation not completed. Processor/system controller interface sequence error 2 has been detected.

    // Misc

    //cpar_dir,   // A parity error has been detected in the cache memory directory. (Not likely)
    //cpar_str,   // PAR fault. A data parity error has been detected in the cache memory.
    //cpar_ia,    // PAR fault. An illegal action has been received from a system controller during a store operation with cache memory enabled.
    //cpar_blk,   // PAR fault. A cache memory parity error has occurred during a cache memory data block load.
    
    // odd word
    //      Cache Duplicate Directory WNO Buffer Overflow
    //port_a,
    //port_b,
    //port_c,
    //port_d,
    
    //cpd,  // Cache Primary Directory WNO Buffer Overflow
    // Write Notify (WNO) Parity Error on Port A, B, C, or D.
    
    //      Cache Duplicate Directory Parity Error
    //level_0,
    ////level_1,
    ////level_2,
    ////level_3,
    
    // Cache Duplicate Directory Multiple Match
    //cdd,
    
    //par_sdwam,  // A parity error has been detected in the SDWAM.
    //par_ptwam,  // A parity error has been detected in the PTWAM.
    

};
typedef enum _fault_subtype _fault_subtype;
#endif

typedef enum fault_onc_subtype_
  {
    flt_onc_nem,        // A nonexistent main memory address has been requested.
    flt_onc_FORCE  = 0400000000000llu // Force enum size to 36 bits.
  } fault_onc_subtype_;

typedef enum fault_str_subtype_
  {
    flt_str_oob,        // A BAR mode boundary violation has occurred.
    flt_str_ill_ptr,    // SPRPn illegal ptr.
    flt_str_nea, // non-existent address
    flt_str_FORCE  = 0400000000000llu // Force enum size to 36 bits.
  } fault_str_subtype_;

typedef enum fault_con_subtype_
  {
    con_a = 0,      // A $CONNECT signal has been received through port A.
    con_b = 1,      // A $CONNECT signal has been received through port B.
    con_c = 2,      // A $CONNECT signal has been received through port C.
    con_d = 3,      // A $CONNECT signal has been received through port D.
    flt_con_FORCE  = 0400000000000llu // Force enum size to 36 bits.
  } fault_con_subtype_;

typedef enum fault_acv_subtype_
  {
    ACV0  = (1U << 15),   ///< 15.Illegal ring order (ACV0=IRO)
    ACV1  = (1U << 14),   ///< 3. Not in execute bracket (ACV1=OEB)
    ACV2  = (1U << 13),   ///< 6. No execute permission (ACV2=E-OFF)
    ACV3  = (1U << 12),   ///< 1. Not in read bracket (ACV3=ORB)
    ACV4  = (1U << 11),   ///< 4. No read permission (ACV4=R-OFF)
    ACV5  = (1U << 10),   ///< 2. Not in write bracket (ACV5=OWB)
    ACV6  = (1U <<  9),   ///< 5. No write permission (ACV6=W-OFF)
    ACV7  = (1U <<  8),   ///< 8. Call limiter fault (ACV7=NO GA)
    ACV8  = (1U <<  7),   ///< 16.Out of call brackets (ACV8=OCB)
    ACV9  = (1U <<  6),   ///< 9. Outward call (ACV9=OCALL)
    ACV10 = (1U <<  5),   ///< 10.Bad outward call (ACV10=BOC)
    ACV11 = (1U <<  4),   ///< 11.Inward return (ACV11=INRET)
    ACV12 = (1U <<  3),   ///< 7. Invalid ring crossing (ACV12=CRT)
    ACV13 = (1U <<  2),   ///< 12.Ring alarm (ACV13=RALR)
    ACV14 = (1U <<  1), ///< 13.Associative memory error 
    ACV15 = (1U <<  0), ///< 14.Out of segment bounds (ACV15=OOSB)
    flt_acv_FORCE  = 0400000000000llu // Force enum size to 36 bits.
  } fault_acv_subtype_;

typedef enum fault_ipr_subtype_
  {
    FR_ILL_OP    = 0400000000000llu, //  0 a ILL OP
    FR_ILL_MOD   = 0200000000000llu, //  1 b ILL MOD
    FR_ILL_SLV   = 0100000000000llu, //  2 c ILL SLV
    FR_ILL_PROC  = 0040000000000llu, //  3 d ILL PROC
    FR_ILL_PROC_MOD  = 0240000000000llu, //  1,3 d ILL PROC | ILL MOD
    FR_NEM       = 0020000000000llu, //  4 e NEM
    FR_OOB       = 0010000000000llu, //  5 f OOB
    FR_ILL_DIG   = 0004000000000llu, //  6 g ILL DIG
    FR_PROC_PARU = 0002000000000llu, //  7 h PROC PARU
    FR_PROC_PARL = 0001000000000llu, //  8 i PROC PARU
    FR_CON_A     = 0000400000000llu, //  9 j $CON A
    FR_CON_B     = 0000200000000llu, // 10 k $CON B
    FR_CON_C     = 0000100000000llu, // 11 l $CON C
    FR_CON_D     = 0000040000000llu, // 12 m $CON D
    FR_DA_ERR    = 0000020000000llu, // 13 n DA ERR
    FR_DA_ERR2   = 0000010000000llu  // 14 o DA ERR2
  } fault_ipr_subtype_;

typedef enum fault_cmd_subtype_
  {
    flt_cmd_lprpn_bits,  // illegal bits in lprpn instruction
    flt_cmd_not_control,  // not control
    flt_cmd_FORCE  = 0400000000000llu // Force enum size to 36 bits.
  } fault_cmd_subtype_;

typedef union _fault_subtype
  {
    fault_onc_subtype_ fault_onc_subtype;
    fault_str_subtype_ fault_str_subtype;
    fault_con_subtype_ fault_con_subtype;
    fault_acv_subtype_ fault_acv_subtype;
    fault_ipr_subtype_ fault_ipr_subtype;
    fault_cmd_subtype_ fault_cmd_subtype;
    word36 bits;
  } _fault_subtype;

// Fault Register bits
enum _faultRegisterBits0
  {
    FR_IA_MASK   = 017,
    FR_IAA_SHIFT = 16, // 0000003600000llu,
    FR_IAB_SHIFT = 12, // 0000000170000llu,
    FR_IAC_SHIFT = 8,  // 0000000007400llu,
    FR_IAD_SHIFT = 4,  // 0000000000360llu,

    FR_CPAR_DIR  = 0000000000010llu, // 32 p CPAR DIR
    FR_CPAR_STR  = 0000000000004llu, // 33 q CPAR STR
    FR_CPAR_IA   = 0000000000002llu, // 34 r CPAR IA
    FR_CPAR_BLK  = 0000000000001llu  // 35 s CPAR BLK
  };

enum _faultRegisterBits1
  {
     FR_PORT_A    = 0400000000000llu, //  0 t PORT A
     FR_PORT_B    = 0200000000000llu, //  1 u PORT B
     FR_PORT_C    = 0100000000000llu, //  2 v PORT C
     FR_PORT_D    = 0040000000000llu, //  3 w PORT D
     FR_WNO_BO    = 0020000000000llu, //  4 x Cache Primary Directory WNO Buffer Overflow
     FR_WNO_PAR   = 0010000000000llu, //  5 y Write Notify (WNO) Parity Error on Port A, B, C or D.
     FR_LEVEL_0   = 0004000000000llu, //  6 z Level 0
     FR_LEVEL_1   = 0002000000000llu, //  7 A Level 1
     FR_LEVEL_2   = 0001000000000llu, //  8 B Level 2
     FR_LEVEL_3   = 0000400000000llu, //  0 C Level 3
     FR_CDDMM     = 0000200000000llu, // 10 D Cache Duplicate Directory Multiple Match
     FR_PAR_SDWAM = 0000100000000llu, // 11 E SDWAM parity error
     FR_PAR_PTWAM = 0000040000000llu  // 12 F PTWAM parity error
  };

enum _systemControllerIllegalActionCodes
  {
    SCIAC_NONE =    000,
    SCIAC_NEA =     002,
    SCIAC_SOC =     003,
    SCIAC_PAR5 =    005,
    SCIAC_PAR6 =    006,
    SCIAC_PAR7 =    007,
    SCIAC_NC =      010,
    SCIAC_PNE =     011,
    SCIAC_ILL_CMD = 012,
    SCIAC_NR =      013,
    SCIAC_PAR14 =   014,
    SCIAC_PAR15 =   015,
    SCIAC_PAR16 =   016,
    SCIAC_PAR17 =   017
  };


/////////////////////////////////////
//
// Interrupts
//

#define N_INTERRUPTS 32

/////////////////////////////////////
//
// Memory map
//

#define IOM_MBX_LOW 01200
#define IOM_MBX_LEN 02200
#define DN355_MBX_LOW 03400
#define DN355_MBX_LEN 03000

/////////////////////////////////////
//
// Opcodes
//

// MM's opcode stuff ...

// Opcodes with low bit (bit 27) == 0.  Enum value is value of upper 9 bits.
typedef enum {
        opcode0_mme    = 0001U, // (1 decimal)
        opcode0_drl    = 0002U, // (2 decimal)
        opcode0_mme2   = 0004U, // (4 decimal)
        opcode0_mme3   = 0005U, // (5 decimal)
        opcode0_mme4   = 0007U, // (7 decimal)
        opcode0_nop    = 0011U, // (9 decimal)
        opcode0_puls1  = 0012U, // (10 decimal)
        opcode0_puls2  = 0013U, // (11 decimal)
        opcode0_cioc   = 0015U, // (13 decimal)
        opcode0_adlx0  = 0020U, // (16 decimal)
        opcode0_adlx1  = 0021U, // (17 decimal)
        opcode0_adlx2  = 0022U, // (18 decimal)
        opcode0_adlx3  = 0023U, // (19 decimal)
        opcode0_adlx4  = 0024U, // (20 decimal)
        opcode0_adlx5  = 0025U, // (21 decimal)
        opcode0_adlx6  = 0026U, // (22 decimal)
        opcode0_adlx7  = 0027U, // (23 decimal)
        opcode0_ldqc   = 0032U, // (26 decimal)
        opcode0_adl    = 0033U, // (27 decimal)
        opcode0_ldac   = 0034U, // (28 decimal)
        opcode0_adla   = 0035U, // (29 decimal)
        opcode0_adlq   = 0036U, // (30 decimal)
        opcode0_adlaq  = 0037U, // (31 decimal)
        opcode0_asx0   = 0040U, // (32 decimal)
        opcode0_asx1   = 0041U, // (33 decimal)
        opcode0_asx2   = 0042U, // (34 decimal)
        opcode0_asx3   = 0043U, // (35 decimal)
        opcode0_asx4   = 0044U, // (36 decimal)
        opcode0_asx5   = 0045U, // (37 decimal)
        opcode0_asx6   = 0046U, // (38 decimal)
        opcode0_asx7   = 0047U, // (39 decimal)
        opcode0_adwp0  = 0050U, // (40 decimal)
        opcode0_adwp1  = 0051U, // (41 decimal)
        opcode0_adwp2  = 0052U, // (42 decimal)
        opcode0_adwp3  = 0053U, // (43 decimal)
        opcode0_aos    = 0054U, // (44 decimal)
        opcode0_asa    = 0055U, // (45 decimal)
        opcode0_asq    = 0056U, // (46 decimal)
        opcode0_sscr   = 0057U, // (47 decimal)
        opcode0_adx0   = 0060U, // (48 decimal)
        opcode0_adx1   = 0061U, // (49 decimal)
        opcode0_adx2   = 0062U, // (50 decimal)
        opcode0_adx3   = 0063U, // (51 decimal)
        opcode0_adx4   = 0064U, // (52 decimal)
        opcode0_adx5   = 0065U, // (53 decimal)
        opcode0_adx6   = 0066U, // (54 decimal)
        opcode0_adx7   = 0067U, // (55 decimal)
        opcode0_awca   = 0071U, // (57 decimal)
        opcode0_awcq   = 0072U, // (58 decimal)
        opcode0_lreg   = 0073U, // (59 decimal)
        opcode0_ada    = 0075U, // (61 decimal)
        opcode0_adq    = 0076U, // (62 decimal)
        opcode0_adaq   = 0077U, // (63 decimal)
        opcode0_cmpx0  = 0100U, // (64 decimal)
        opcode0_cmpx1  = 0101U, // (65 decimal)
        opcode0_cmpx2  = 0102U, // (66 decimal)
        opcode0_cmpx3  = 0103U, // (67 decimal)
        opcode0_cmpx4  = 0104U, // (68 decimal)
        opcode0_cmpx5  = 0105U, // (69 decimal)
        opcode0_cmpx6  = 0106U, // (70 decimal)
        opcode0_cmpx7  = 0107U, // (71 decimal)
        opcode0_cwl    = 0111U, // (73 decimal)
        opcode0_cmpa   = 0115U, // (77 decimal)
        opcode0_cmpq   = 0116U, // (78 decimal)
        opcode0_cmpaq  = 0117U, // (79 decimal)
        opcode0_sblx0  = 0120U, // (80 decimal)
        opcode0_sblx1  = 0121U, // (81 decimal)
        opcode0_sblx2  = 0122U, // (82 decimal)
        opcode0_sblx3  = 0123U, // (83 decimal)
        opcode0_sblx4  = 0124U, // (84 decimal)
        opcode0_sblx5  = 0125U, // (85 decimal)
        opcode0_sblx6  = 0126U, // (86 decimal)
        opcode0_sblx7  = 0127U, // (87 decimal)
        opcode0_sbla   = 0135U, // (93 decimal)
        opcode0_sblq   = 0136U, // (94 decimal)
        opcode0_sblaq  = 0137U, // (95 decimal)
        opcode0_ssx0   = 0140U, // (96 decimal)
        opcode0_ssx1   = 0141U, // (97 decimal)
        opcode0_ssx2   = 0142U, // (98 decimal)
        opcode0_ssx3   = 0143U, // (99 decimal)
        opcode0_ssx4   = 0144U, // (100 decimal)
        opcode0_ssx5   = 0145U, // (101 decimal)
        opcode0_ssx6   = 0146U, // (102 decimal)
        opcode0_ssx7   = 0147U, // (103 decimal)
        opcode0_adwp4  = 0150U, // (104 decimal)
        opcode0_adwp5  = 0151U, // (105 decimal)
        opcode0_adwp6  = 0152U, // (106 decimal)
        opcode0_adwp7  = 0153U, // (107 decimal)
        opcode0_sdbr   = 0154U, // (108 decimal)
        opcode0_ssa    = 0155U, // (109 decimal)
        opcode0_ssq    = 0156U, // (110 decimal)
        opcode0_sbx0   = 0160U, // (112 decimal)
        opcode0_sbx1   = 0161U, // (113 decimal)
        opcode0_sbx2   = 0162U, // (114 decimal)
        opcode0_sbx3   = 0163U, // (115 decimal)
        opcode0_sbx4   = 0164U, // (116 decimal)
        opcode0_sbx5   = 0165U, // (117 decimal)
        opcode0_sbx6   = 0166U, // (118 decimal)
        opcode0_sbx7   = 0167U, // (119 decimal)
        opcode0_swca   = 0171U, // (121 decimal)
        opcode0_swcq   = 0172U, // (122 decimal)
        opcode0_lpri   = 0173U, // (123 decimal)
        opcode0_sba    = 0175U, // (125 decimal)
        opcode0_sbq    = 0176U, // (126 decimal)
        opcode0_sbaq   = 0177U, // (127 decimal)
        opcode0_cnax0  = 0200U, // (128 decimal)
        opcode0_cnax1  = 0201U, // (129 decimal)
        opcode0_cnax2  = 0202U, // (130 decimal)
        opcode0_cnax3  = 0203U, // (131 decimal)
        opcode0_cnax4  = 0204U, // (132 decimal)
        opcode0_cnax5  = 0205U, // (133 decimal)
        opcode0_cnax6  = 0206U, // (134 decimal)
        opcode0_cnax7  = 0207U, // (135 decimal)
        opcode0_cmk    = 0211U, // (137 decimal)
        opcode0_absa   = 0212U, // (138 decimal)
        opcode0_epaq   = 0213U, // (139 decimal)
        opcode0_sznc   = 0214U, // (140 decimal)
        opcode0_cnaa   = 0215U, // (141 decimal)
        opcode0_cnaq   = 0216U, // (142 decimal)
        opcode0_cnaaq  = 0217U, // (143 decimal)
        opcode0_ldx0   = 0220U, // (144 decimal)
        opcode0_ldx1   = 0221U, // (145 decimal)
        opcode0_ldx2   = 0222U, // (146 decimal)
        opcode0_ldx3   = 0223U, // (147 decimal)
        opcode0_ldx4   = 0224U, // (148 decimal)
        opcode0_ldx5   = 0225U, // (149 decimal)
        opcode0_ldx6   = 0226U, // (150 decimal)
        opcode0_ldx7   = 0227U, // (151 decimal)
        opcode0_lbar   = 0230U, // (152 decimal)
        opcode0_rsw    = 0231U, // (153 decimal)
        opcode0_ldbr   = 0232U, // (154 decimal)
        opcode0_rmcm   = 0233U, // (155 decimal)
        opcode0_szn    = 0234U, // (156 decimal)
        opcode0_lda    = 0235U, // (157 decimal)
        opcode0_ldq    = 0236U, // (158 decimal)
        opcode0_ldaq   = 0237U, // (159 decimal)
        opcode0_orsx0  = 0240U, // (160 decimal)
        opcode0_orsx1  = 0241U, // (161 decimal)
        opcode0_orsx2  = 0242U, // (162 decimal)
        opcode0_orsx3  = 0243U, // (163 decimal)
        opcode0_orsx4  = 0244U, // (164 decimal)
        opcode0_orsx5  = 0245U, // (165 decimal)
        opcode0_orsx6  = 0246U, // (166 decimal)
        opcode0_orsx7  = 0247U, // (167 decimal)
        opcode0_spri0  = 0250U, // (168 decimal)
        opcode0_spbp1  = 0251U, // (169 decimal)
        opcode0_spri2  = 0252U, // (170 decimal)
        opcode0_spbp3  = 0253U, // (171 decimal)
        opcode0_spri   = 0254U, // (172 decimal)
        opcode0_orsa   = 0255U, // (173 decimal)
        opcode0_orsq   = 0256U, // (174 decimal)
        opcode0_lsdp   = 0257U, // (175 decimal)
        opcode0_orx0   = 0260U, // (176 decimal)
        opcode0_orx1   = 0261U, // (177 decimal)
        opcode0_orx2   = 0262U, // (178 decimal)
        opcode0_orx3   = 0263U, // (179 decimal)
        opcode0_orx4   = 0264U, // (180 decimal)
        opcode0_orx5   = 0265U, // (181 decimal)
        opcode0_orx6   = 0266U, // (182 decimal)
        opcode0_orx7   = 0267U, // (183 decimal)
        opcode0_tsp0   = 0270U, // (184 decimal)
        opcode0_tsp1   = 0271U, // (185 decimal)
        opcode0_tsp2   = 0272U, // (186 decimal)
        opcode0_tsp3   = 0273U, // (187 decimal)
        opcode0_ora    = 0275U, // (189 decimal)
        opcode0_orq    = 0276U, // (190 decimal)
        opcode0_oraq   = 0277U, // (191 decimal)
        opcode0_canx0  = 0300U, // (192 decimal)
        opcode0_canx1  = 0301U, // (193 decimal)
        opcode0_canx2  = 0302U, // (194 decimal)
        opcode0_canx3  = 0303U, // (195 decimal)
        opcode0_canx4  = 0304U, // (196 decimal)
        opcode0_canx5  = 0305U, // (197 decimal)
        opcode0_canx6  = 0306U, // (198 decimal)
        opcode0_canx7  = 0307U, // (199 decimal)
        opcode0_eawp0  = 0310U, // (200 decimal)
        opcode0_easp0  = 0311U, // (201 decimal)
        opcode0_eawp2  = 0312U, // (202 decimal)
        opcode0_easp2  = 0313U, // (203 decimal)
        opcode0_cana   = 0315U, // (205 decimal)
        opcode0_canq   = 0316U, // (206 decimal)
        opcode0_canaq  = 0317U, // (207 decimal)
        opcode0_lcx0   = 0320U, // (208 decimal)
        opcode0_lcx1   = 0321U, // (209 decimal)
        opcode0_lcx2   = 0322U, // (210 decimal)
        opcode0_lcx3   = 0323U, // (211 decimal)
        opcode0_lcx4   = 0324U, // (212 decimal)
        opcode0_lcx5   = 0325U, // (213 decimal)
        opcode0_lcx6   = 0326U, // (214 decimal)
        opcode0_lcx7   = 0327U, // (215 decimal)
        opcode0_eawp4  = 0330U, // (216 decimal)
        opcode0_easp4  = 0331U, // (217 decimal)
        opcode0_eawp6  = 0332U, // (218 decimal)
        opcode0_easp6  = 0333U, // (219 decimal)
        opcode0_lca    = 0335U, // (221 decimal)
        opcode0_lcq    = 0336U, // (222 decimal)
        opcode0_lcaq   = 0337U, // (223 decimal)
        opcode0_ansx0  = 0340U, // (224 decimal)
        opcode0_ansx1  = 0341U, // (225 decimal)
        opcode0_ansx2  = 0342U, // (226 decimal)
        opcode0_ansx3  = 0343U, // (227 decimal)
        opcode0_ansx4  = 0344U, // (228 decimal)
        opcode0_ansx5  = 0345U, // (229 decimal)
        opcode0_ansx6  = 0346U, // (230 decimal)
        opcode0_ansx7  = 0347U, // (231 decimal)
        opcode0_epp0   = 0350U, // (232 decimal)
        opcode0_epbp1  = 0351U, // (233 decimal)
        opcode0_epp2   = 0352U, // (234 decimal)
        opcode0_epbp3  = 0353U, // (235 decimal)
        opcode0_stac   = 0354U, // (236 decimal)
        opcode0_ansa   = 0355U, // (237 decimal)
        opcode0_ansq   = 0356U, // (238 decimal)
        opcode0_stcd   = 0357U, // (239 decimal)
        opcode0_anx0   = 0360U, // (240 decimal)
        opcode0_anx1   = 0361U, // (241 decimal)
        opcode0_anx2   = 0362U, // (242 decimal)
        opcode0_anx3   = 0363U, // (243 decimal)
        opcode0_anx4   = 0364U, // (244 decimal)
        opcode0_anx5   = 0365U, // (245 decimal)
        opcode0_anx6   = 0366U, // (246 decimal)
        opcode0_anx7   = 0367U, // (247 decimal)
        opcode0_epp4   = 0370U, // (248 decimal)
        opcode0_epbp5  = 0371U, // (249 decimal)
        opcode0_epp6   = 0372U, // (250 decimal)
        opcode0_epbp7  = 0373U, // (251 decimal)
        opcode0_ana    = 0375U, // (253 decimal)
        opcode0_anq    = 0376U, // (254 decimal)
        opcode0_anaq   = 0377U, // (255 decimal)
        opcode0_mpf    = 0401U, // (257 decimal)
        opcode0_mpy    = 0402U, // (258 decimal)
        opcode0_cmg    = 0405U, // (261 decimal)
        opcode0_lde    = 0411U, // (265 decimal)
        opcode0_rscr   = 0413U, // (267 decimal)
        opcode0_ade    = 0415U, // (269 decimal)
        opcode0_ufm    = 0421U, // (273 decimal)
        opcode0_dufm   = 0423U, // (275 decimal)
        opcode0_fcmg   = 0425U, // (277 decimal)
        opcode0_dfcmg  = 0427U, // (279 decimal)
        opcode0_fszn   = 0430U, // (280 decimal)
        opcode0_fld    = 0431U, // (281 decimal)
        opcode0_dfld   = 0433U, // (283 decimal)
        opcode0_ufa    = 0435U, // (285 decimal)
        opcode0_dufa   = 0437U, // (287 decimal)
        opcode0_sxl0   = 0440U, // (288 decimal)
        opcode0_sxl1   = 0441U, // (289 decimal)
        opcode0_sxl2   = 0442U, // (290 decimal)
        opcode0_sxl3   = 0443U, // (291 decimal)
        opcode0_sxl4   = 0444U, // (292 decimal)
        opcode0_sxl5   = 0445U, // (293 decimal)
        opcode0_sxl6   = 0446U, // (294 decimal)
        opcode0_sxl7   = 0447U, // (295 decimal)
        opcode0_stz    = 0450U, // (296 decimal)
        opcode0_smic   = 0451U, // (297 decimal)
        opcode0_scpr   = 0452U, // (298 decimal)
        opcode0_stt    = 0454U, // (300 decimal)
        opcode0_fst    = 0455U, // (301 decimal)
        opcode0_ste    = 0456U, // (302 decimal)
        opcode0_dfst   = 0457U, // (303 decimal)
        opcode0_fmp    = 0461U, // (305 decimal)
        opcode0_dfmp   = 0463U, // (307 decimal)
        opcode0_fstr   = 0470U, // (312 decimal)
        opcode0_frd    = 0471U, // (313 decimal)
        opcode0_dfstr  = 0472U, // (314 decimal)
        opcode0_dfrd   = 0473U, // (315 decimal)
        opcode0_fad    = 0475U, // (317 decimal)
        opcode0_dfad   = 0477U, // (319 decimal)
        opcode0_rpl    = 0500U, // (320 decimal)
        opcode0_bcd    = 0505U, // (325 decimal)
        opcode0_div    = 0506U, // (326 decimal)
        opcode0_dvf    = 0507U, // (327 decimal)
        opcode0_fneg   = 0513U, // (331 decimal)
        opcode0_fcmp   = 0515U, // (333 decimal)
        opcode0_dfcmp  = 0517U, // (335 decimal)
        opcode0_rpt    = 0520U, // (336 decimal)
        opcode0_fdi    = 0525U, // (341 decimal)
        opcode0_dfdi   = 0527U, // (343 decimal)
        opcode0_neg    = 0531U, // (345 decimal)
        opcode0_cams   = 0532U, // (346 decimal)
        opcode0_negl   = 0533U, // (347 decimal)
        opcode0_ufs    = 0535U, // (349 decimal)
        opcode0_dufs   = 0537U, // (351 decimal)
        opcode0_sprp0  = 0540U, // (352 decimal)
        opcode0_sprp1  = 0541U, // (353 decimal)
        opcode0_sprp2  = 0542U, // (354 decimal)
        opcode0_sprp3  = 0543U, // (355 decimal)
        opcode0_sprp4  = 0544U, // (356 decimal)
        opcode0_sprp5  = 0545U, // (357 decimal)
        opcode0_sprp6  = 0546U, // (358 decimal)
        opcode0_sprp7  = 0547U, // (359 decimal)
        opcode0_sbar   = 0550U, // (360 decimal)
        opcode0_stba   = 0551U, // (361 decimal)
        opcode0_stbq   = 0552U, // (362 decimal)
        opcode0_smcm   = 0553U, // (363 decimal)
        opcode0_stc1   = 0554U, // (364 decimal)
        opcode0_ssdp   = 0557U, // (367 decimal)
        opcode0_rpd    = 0560U, // (368 decimal)
        opcode0_fdv    = 0565U, // (373 decimal)
        opcode0_dfdv   = 0567U, // (375 decimal)
        opcode0_fno    = 0573U, // (379 decimal)
        opcode0_fsb    = 0575U, // (381 decimal)
        opcode0_dfsb   = 0577U, // (383 decimal)
        opcode0_tze    = 0600U, // (384 decimal)
        opcode0_tnz    = 0601U, // (385 decimal)
        opcode0_tnc    = 0602U, // (386 decimal)
        opcode0_trc    = 0603U, // (387 decimal)
        opcode0_tmi    = 0604U, // (388 decimal)
        opcode0_tpl    = 0605U, // (389 decimal)
        opcode0_ttf    = 0607U, // (391 decimal)
        opcode0_rtcd   = 0610U, // (392 decimal)
        opcode0_rcu    = 0613U, // (395 decimal)
        opcode0_teo    = 0614U, // (396 decimal)
        opcode0_teu    = 0615U, // (397 decimal)
        opcode0_dis    = 0616U, // (398 decimal)
        opcode0_tov    = 0617U, // (399 decimal)
        opcode0_eax0   = 0620U, // (400 decimal)
        opcode0_eax1   = 0621U, // (401 decimal)
        opcode0_eax2   = 0622U, // (402 decimal)
        opcode0_eax3   = 0623U, // (403 decimal)
        opcode0_eax4   = 0624U, // (404 decimal)
        opcode0_eax5   = 0625U, // (405 decimal)
        opcode0_eax6   = 0626U, // (406 decimal)
        opcode0_eax7   = 0627U, // (407 decimal)
        opcode0_ret    = 0630U, // (408 decimal)
        opcode0_rccl   = 0633U, // (411 decimal)
        opcode0_ldi    = 0634U, // (412 decimal)
        opcode0_eaa    = 0635U, // (413 decimal)
        opcode0_eaq    = 0636U, // (414 decimal)
        opcode0_ldt    = 0637U, // (415 decimal)
        opcode0_ersx0  = 0640U, // (416 decimal)
        opcode0_ersx1  = 0641U, // (417 decimal)
        opcode0_ersx2  = 0642U, // (418 decimal)
        opcode0_ersx3  = 0643U, // (419 decimal)
        opcode0_ersx4  = 0644U, // (420 decimal)
        opcode0_ersx5  = 0645U, // (421 decimal)
        opcode0_ersx6  = 0646U, // (422 decimal)
        opcode0_ersx7  = 0647U, // (423 decimal)
        opcode0_spri4  = 0650U, // (424 decimal)
        opcode0_spbp5  = 0651U, // (425 decimal)
        opcode0_spri6  = 0652U, // (426 decimal)
        opcode0_spbp7  = 0653U, // (427 decimal)
        opcode0_stacq  = 0654U, // (428 decimal)
        opcode0_ersa   = 0655U, // (429 decimal)
        opcode0_ersq   = 0656U, // (430 decimal)
        opcode0_scu    = 0657U, // (431 decimal)
        opcode0_erx0   = 0660U, // (432 decimal)
        opcode0_erx1   = 0661U, // (433 decimal)
        opcode0_erx2   = 0662U, // (434 decimal)
        opcode0_erx3   = 0663U, // (435 decimal)
        opcode0_erx4   = 0664U, // (436 decimal)
        opcode0_erx5   = 0665U, // (437 decimal)
        opcode0_erx6   = 0666U, // (438 decimal)
        opcode0_erx7   = 0667U, // (439 decimal)
        opcode0_tsp4   = 0670U, // (440 decimal)
        opcode0_tsp5   = 0671U, // (441 decimal)
        opcode0_tsp6   = 0672U, // (442 decimal)
        opcode0_tsp7   = 0673U, // (443 decimal)
        opcode0_lcpr   = 0674U, // (444 decimal)
        opcode0_era    = 0675U, // (445 decimal)
        opcode0_erq    = 0676U, // (446 decimal)
        opcode0_eraq   = 0677U, // (447 decimal)
        opcode0_tsx0   = 0700U, // (448 decimal)
        opcode0_tsx1   = 0701U, // (449 decimal)
        opcode0_tsx2   = 0702U, // (450 decimal)
        opcode0_tsx3   = 0703U, // (451 decimal)
        opcode0_tsx4   = 0704U, // (452 decimal)
        opcode0_tsx5   = 0705U, // (453 decimal)
        opcode0_tsx6   = 0706U, // (454 decimal)
        opcode0_tsx7   = 0707U, // (455 decimal)
        opcode0_tra    = 0710U, // (456 decimal)
        opcode0_call6  = 0713U, // (459 decimal)
        opcode0_tss    = 0715U, // (461 decimal)
        opcode0_xec    = 0716U, // (462 decimal)
        opcode0_xed    = 0717U, // (463 decimal)
        opcode0_lxl0   = 0720U, // (464 decimal)
        opcode0_lxl1   = 0721U, // (465 decimal)
        opcode0_lxl2   = 0722U, // (466 decimal)
        opcode0_lxl3   = 0723U, // (467 decimal)
        opcode0_lxl4   = 0724U, // (468 decimal)
        opcode0_lxl5   = 0725U, // (469 decimal)
        opcode0_lxl6   = 0726U, // (470 decimal)
        opcode0_lxl7   = 0727U, // (471 decimal)
        opcode0_ars    = 0731U, // (473 decimal)
        opcode0_qrs    = 0732U, // (474 decimal)
        opcode0_lrs    = 0733U, // (475 decimal)
        opcode0_als    = 0735U, // (477 decimal)
        opcode0_qls    = 0736U, // (478 decimal)
        opcode0_lls    = 0737U, // (479 decimal)
        opcode0_stx0   = 0740U, // (480 decimal)
        opcode0_stx1   = 0741U, // (481 decimal)
        opcode0_stx2   = 0742U, // (482 decimal)
        opcode0_stx3   = 0743U, // (483 decimal)
        opcode0_stx4   = 0744U, // (484 decimal)
        opcode0_stx5   = 0745U, // (485 decimal)
        opcode0_stx6   = 0746U, // (486 decimal)
        opcode0_stx7   = 0747U, // (487 decimal)
        opcode0_stc2   = 0750U, // (488 decimal)
        opcode0_stca   = 0751U, // (489 decimal)
        opcode0_stcq   = 0752U, // (490 decimal)
        opcode0_sreg   = 0753U, // (491 decimal)
        opcode0_sti    = 0754U, // (492 decimal)
        opcode0_sta    = 0755U, // (493 decimal)
        opcode0_stq    = 0756U, // (494 decimal)
        opcode0_staq   = 0757U, // (495 decimal)
        opcode0_lprp0  = 0760U, // (496 decimal)
        opcode0_lprp1  = 0761U, // (497 decimal)
        opcode0_lprp2  = 0762U, // (498 decimal)
        opcode0_lprp3  = 0763U, // (499 decimal)
        opcode0_lprp4  = 0764U, // (500 decimal)
        opcode0_lprp5  = 0765U, // (501 decimal)
        opcode0_lprp6  = 0766U, // (502 decimal)
        opcode0_lprp7  = 0767U, // (503 decimal)
        opcode0_arl    = 0771U, // (505 decimal)
        opcode0_qrl    = 0772U, // (506 decimal)
        opcode0_lrl    = 0773U, // (507 decimal)
        opcode0_gtb    = 0774U, // (508 decimal)
        opcode0_alr    = 0775U, // (509 decimal)
        opcode0_qlr    = 0776U, // (510 decimal)
        opcode0_llr    = 0777U  // (511 decimal)
} opcode0_t;

// Opcodes with low bit (bit 27) == 1.  Enum value is value of upper 9 bits.
typedef enum {
        opcode1_mve    = 0020U, // (16 decimal)
        opcode1_mvne   = 0024U, // (20 decimal)
        opcode1_csl    = 0060U, // (48 decimal)
        opcode1_csr    = 0061U, // (49 decimal)
        opcode1_sztl   = 0064U, // (52 decimal)
        opcode1_sztr   = 0065U, // (53 decimal)
        opcode1_cmpb   = 0066U, // (54 decimal)
        opcode1_mlr    = 0100U, // (64 decimal)
        opcode1_mrl    = 0101U, // (65 decimal)
        opcode1_cmpc   = 0106U, // (70 decimal)
        opcode1_scd    = 0120U, // (80 decimal)
        opcode1_scdr   = 0121U, // (81 decimal)
        opcode1_scm    = 0124U, // (84 decimal)
        opcode1_scmr   = 0125U, // (85 decimal)
        opcode1_sptr   = 0154U, // (108 decimal)
        opcode1_mvt    = 0160U, // (112 decimal)
        opcode1_tct    = 0164U, // (116 decimal)
        opcode1_tctr   = 0165U, // (117 decimal)
        opcode1_lptr   = 0173U, // (123 decimal)
        opcode1_ad2d   = 0202U, // (130 decimal)
        opcode1_sb2d   = 0203U, // (131 decimal)
        opcode1_mp2d   = 0206U, // (134 decimal)
        opcode1_dv2d   = 0207U, // (135 decimal)
        opcode1_ad3d   = 0222U, // (146 decimal)
        opcode1_sb3d   = 0223U, // (147 decimal)
        opcode1_mp3d   = 0226U, // (150 decimal)
        opcode1_dv3d   = 0227U, // (151 decimal)
        opcode1_lsdr   = 0232U, // (154 decimal)
        opcode1_spbp0  = 0250U, // (168 decimal)
        opcode1_spri1  = 0251U, // (169 decimal)
        opcode1_spbp2  = 0252U, // (170 decimal)
        opcode1_spri3  = 0253U, // (171 decimal)
        opcode1_ssdr   = 0254U, // (172 decimal)
        opcode1_lptp   = 0257U, // (175 decimal)
        opcode1_mvn    = 0300U, // (192 decimal)
        opcode1_btd    = 0301U, // (193 decimal)
        opcode1_cmpn   = 0303U, // (195 decimal)
        opcode1_dtb    = 0305U, // (197 decimal)
        opcode1_easp1  = 0310U, // (200 decimal)
        opcode1_eawp1  = 0311U, // (201 decimal)
        opcode1_easp3  = 0312U, // (202 decimal)
        opcode1_eawp3  = 0313U, // (203 decimal)
        opcode1_easp5  = 0330U, // (216 decimal)
        opcode1_eawp5  = 0331U, // (217 decimal)
        opcode1_easp7  = 0332U, // (218 decimal)
        opcode1_eawp7  = 0333U, // (219 decimal)
        opcode1_epbp0  = 0350U, // (232 decimal)
        opcode1_epp1   = 0351U, // (233 decimal)
        opcode1_epbp2  = 0352U, // (234 decimal)
        opcode1_epp3   = 0353U, // (235 decimal)
        opcode1_epbp4  = 0370U, // (248 decimal)
        opcode1_epp5   = 0371U, // (249 decimal)
        opcode1_epbp6  = 0372U, // (250 decimal)
        opcode1_epp7   = 0373U, // (251 decimal)
        opcode1_sareg  = 0443U, // (291 decimal)
        opcode1_spl    = 0447U, // (295 decimal)
        opcode1_lareg  = 0463U, // (307 decimal)
        opcode1_lpl    = 0467U, // (311 decimal)
        opcode1_a9bd   = 0500U, // (320 decimal)
        opcode1_a6bd   = 0501U, // (321 decimal)
        opcode1_a4bd   = 0502U, // (322 decimal)
        opcode1_abd    = 0503U, // (323 decimal)
        opcode1_awd    = 0507U, // (327 decimal)
        opcode1_s9bd   = 0520U, // (336 decimal)
        opcode1_s6bd   = 0521U, // (337 decimal)
        opcode1_s4bd   = 0522U, // (338 decimal)
        opcode1_sbd    = 0523U, // (339 decimal)
        opcode1_swd    = 0527U, // (343 decimal)
        opcode1_camp   = 0532U, // (346 decimal)
        opcode1_ara0   = 0540U, // (352 decimal)
        opcode1_ara1   = 0541U, // (353 decimal)
        opcode1_ara2   = 0542U, // (354 decimal)
        opcode1_ara3   = 0543U, // (355 decimal)
        opcode1_ara4   = 0544U, // (356 decimal)
        opcode1_ara5   = 0545U, // (357 decimal)
        opcode1_ara6   = 0546U, // (358 decimal)
        opcode1_ara7   = 0547U, // (359 decimal)
        opcode1_sptp   = 0557U, // (367 decimal)
        opcode1_aar0   = 0560U, // (368 decimal)
        opcode1_aar1   = 0561U, // (369 decimal)
        opcode1_aar2   = 0562U, // (370 decimal)
        opcode1_aar3   = 0563U, // (371 decimal)
        opcode1_aar4   = 0564U, // (372 decimal)
        opcode1_aar5   = 0565U, // (373 decimal)
        opcode1_aar6   = 0566U, // (374 decimal)
        opcode1_aar7   = 0567U, // (375 decimal)
        opcode1_trtn   = 0600U, // (384 decimal)
        opcode1_trtf   = 0601U, // (385 decimal)
        opcode1_tmoz   = 0604U, // (388 decimal)
        opcode1_tpnz   = 0605U, // (389 decimal)
        opcode1_ttn    = 0606U, // (390 decimal)
        opcode1_arn0   = 0640U, // (416 decimal)
        opcode1_arn1   = 0641U, // (417 decimal)
        opcode1_arn2   = 0642U, // (418 decimal)
        opcode1_arn3   = 0643U, // (419 decimal)
        opcode1_arn4   = 0644U, // (420 decimal)
        opcode1_arn5   = 0645U, // (421 decimal)
        opcode1_arn6   = 0646U, // (422 decimal)
        opcode1_arn7   = 0647U, // (423 decimal)
        opcode1_spbp4  = 0650U, // (424 decimal)
        opcode1_spri5  = 0651U, // (425 decimal)
        opcode1_spbp6  = 0652U, // (426 decimal)
        opcode1_spri7  = 0653U, // (427 decimal)
        opcode1_nar0   = 0660U, // (432 decimal)
        opcode1_nar1   = 0661U, // (433 decimal)
        opcode1_nar2   = 0662U, // (434 decimal)
        opcode1_nar3   = 0663U, // (435 decimal)
        opcode1_nar4   = 0664U, // (436 decimal)
        opcode1_nar5   = 0665U, // (437 decimal)
        opcode1_nar6   = 0666U, // (438 decimal)
        opcode1_nar7   = 0667U, // (439 decimal)
        opcode1_sar0   = 0740U, // (480 decimal)
        opcode1_sar1   = 0741U, // (481 decimal)
        opcode1_sar2   = 0742U, // (482 decimal)
        opcode1_sar3   = 0743U, // (483 decimal)
        opcode1_sar4   = 0744U, // (484 decimal)
        opcode1_sar5   = 0745U, // (485 decimal)
        opcode1_sar6   = 0746U, // (486 decimal)
        opcode1_sar7   = 0747U, // (487 decimal)
        opcode1_sra    = 0754U, // (492 decimal)
        opcode1_lar0   = 0760U, // (496 decimal)
        opcode1_lar1   = 0761U, // (497 decimal)
        opcode1_lar2   = 0762U, // (498 decimal)
        opcode1_lar3   = 0763U, // (499 decimal)
        opcode1_lar4   = 0764U, // (500 decimal)
        opcode1_lar5   = 0765U, // (501 decimal)
        opcode1_lar6   = 0766U, // (502 decimal)
        opcode1_lar7   = 0767U, // (503 decimal)
        opcode1_lra    = 0774U  // (508 decimal)
} opcode1_t;

/////////////////////////////////////
//
// History registers
//

enum { N_HIST_SETS = 4 };
#ifdef DPS8M
enum { N_HIST_SIZE = 64 };
#endif
#ifdef L68
enum { N_HIST_SIZE = 16 };
#endif

// Bit in CU history register word 0

// cu_hist_t flags
enum
  {
    CU_HIST_PIA =      0400000000000,  //  0   Prepare instruction address
    CU_HIST_POA =      0200000000000,  //  1   Prepare operand address
    CU_HIST_RIW =      0100000000000,  //  2   Request indirect word
    CU_HIST_SIW =      0040000000000,  //  3   Restore indirect word
    CU_HIST_POT =      0020000000000,  //  4   Prepare operand tally (indirect tally chain)
    CU_HIST_PON =      0010000000000,  //  5   Prepare operand no tally (as for POT except no chain)
    CU_HIST_RAW =      0004000000000,  //  6   Request read-alter-rewrite word
    CU_HIST_SAW =      0002000000000,  //  7   Restore read-later-rewrite word
    CU_HIST_TRGO =     0001000000000,  //  8   Transfer GO (conditions met)
    CU_HIST_XDE =      0000400000000,  //  9   Execute XED even instruction
    CU_HIST_XDO =      0000200000000,  // 10   Execute XED odd instruction
    CU_HIST_IC =       0000100000000,  // 11   Execute odd instruction of the current pair
    CU_HIST_RPTS =     0000040000000,  // 12   Execute a repeat instruction
    CU_HIST_PORTF =    0000020000000,  // 13   Memory cycle to port on previous cycle
    CU_HIST_INTERNAL = 0000010000000,  // 14   Memory cycle to cache or direct on previous cycle
    CU_HIST_PAI =      0000004000000,  // 15   Prepare interrupt address
    CU_HIST_PFA =      0000002000000,  // 16   Prepare fault address
    CU_HIST_PRIV =     0000001000000   // 17   In privileged mode
  };

// cu_hist_t flags2
enum
  {
    CU_HIST_XINT = 0100,  //  29   Execute instruction
    CU_HIST_IFT =  0040,  //  30   Perform an instruction fetch
    CU_HIST_CRD =  0020,  //  31   Cache read, this CU cycle
    CU_HIST_MRD =  0010,  //  32   Memory read, this CU cycle
    CU_HIST_MSTO = 0004,  //  33   Memory store, this CU cycle
    CU_HIST_PIB =  0002,  //  34   Memory port interface busy
   };

enum
  {
    DU_FANLD1 =  0400000000000,  //   0  Alpha-num load desc l (complemented)
    DU_FANLD2 =  0200000000000,  //   1  Alpha-num load desc 2 (complemented)
    DU_FANSTR =  0100000000000,  //   2  Alpha-num store (complemented)
    DU_FLDWRT1 = 0040000000000,  //   3  Load re-write reg l (complemented)
    DU_FLDWRT2 = 0020000000000,  //   4  Load re-write reg 2 (complemented)
    DU_FNLD1 =   0010000000000,  //   5  Numeric load desc l (complemented)
    DU_FNLD2 =   0004000000000,  //   6  Numeric load desc 2 (complemented)
    DU_NOSEQF =  0002000000000,  //   7  End sequence flag
    DU_FDUD =    0001000000000,  //   8  Decimal unit idle (complemented)
    DU_FGSTR =   0000400000000,  //   9  General store flag (complemented)
    DU_NOSEQ =   0000200000000,  //  10  End of sequence (complemented)
    DU_NINE =    0000100000000,  //  11  9-bit character operation
    DU_SIX =     0000040000000,  //  12  6-bit character operation
    DU_FOUR =    0000020000000,  //  13  4-bit character operation
    DU_DUBIT =   0000010000000,  //  14  Bit operation
    DU_UWORD =   0000004000000,  //  15  Word operation
    DU_PTR1 =    0000002000000,  //  16  Select ptr l
    DU_PTR2 =    0000001000000,  //  17  Select ptr 2
    DU_PRT3 =    0000000400000,  //  18  Select ptr 3
    DU_FPOP =    0000000200000,  //  19  Prepare operand pointer
    DU_GEAM =    0000000100000,  //  20  Add timing gates (complemented)
    DU_LPD12 =   0000000040000,  //  21  Load pointer l or 2 (complemented)
    DU_GEMAE =   0000000020000,  //  22  Multiply gates A E (complemented)
    DU_BTDS =    0000000010000,  //  23  Binary to decimal gates (complemented)
    DU_SP15 =    0000000004000,  //  24  Align cycles (complemented)
    DU_FSWEQ =   0000000002000,  //  25  Single word sequence flag (complemented)
    DU_FGCH =    0000000001000,  //  26  Character cycle (complemented)
    DU_DFRST =   0000000000400,  //  27  Processing descriptor for first time
    DU_EXH =     0000000000200,  //  28  Exhaust
    DU_FGADO =   0000000000100,  //  29  Add cycle (complemented)
    DU_INTRPTD = 0000000000040,  //  30  Interrupted
    DU_GLDP2 =   0000000000020,  //  31  Load DP2
    DU_GEMC =    0000000000010,  //  32  Multiply gate C
    DU_GBDA =    0000000000004,  //  33  Binary to decimal gate A
    DU_GSP5 =    0000000000002   //  34  Final align cycle
  };

// apu_hist_t flags
enum
  {
    APU_PIA_OVF = 04000000,  //  15   PIA Page overflow
    APU_PIA_OOB = 02000000,  //  16   PIA out of segment bounds
    APU_FDSPTW =  01000000,  //  17   Fetch descriptor segment PTW
    APU_MDSPTW =  00400000,  //  18   Descriptor segment PTW is modified
    APU_FSDW =    00200000,  //  19   Fetch SDW
    APU_FPTW =    00100000,  //  20   Fetch PTW
    APU_FPTW2 =   00040000,  //  21   Fetch pre-page PTW
    APU_MPTW =    00020000,  //  22   PTW modified
    APU_FANP =    00010000,  //  23   Final address nonpaged
    APU_FAP =     00004000,  //  24   Final address paged
    APU_MTCHSDW = 00002000,  //  25   SDW match found
    APU_SDWMF =   00001000,  //  26   SDW match found and used
    // BSY Data source for ESN
    APU_BSY_IC =  00000000,  // 27-28  00 = from ppr.ic
    APU_BSY_TSE = 00000200,  // 27-28  01 = from prn.tsr
    APU_BSY_SWR = 00000400,  // 27-28  10 = from tpr.swr
    APU_BSY_CA =  00000600,  // 27-28  11 = from tpr.ca
    APU_MTCHPTW = 00000100,  //  29   PTW match found (AM)
    //APU_PTWMF =   00000000,  // 30-31 PTW match found (AM) and used
    //APU_PTWAM =   00000000   // 32-35 PTW AM direct address (ZCA bits 4-7)
  };

// apu_hist_t flags2
enum
  {
    APU_SDWME =  0400, //   27   SDW match error
    //APU_SDWLVL = 0000, // 28-29  SDW match level count (0 = Level A)
    APU_CACHE =  0040, //   30   Cache used this cycle
    APU_PTW =    0020, //   31   match error
    //APU_PTWLVL = 0000, // 32-33  PTW match level count (0 = level A)
    APU_FLTHLD = 0002, //   34   A directed fault or access violation fault is waiting
    // bit 35 is marked 'w' but undocumented
  };


#ifdef DPS8M
enum { CU_HIST_REG = 0, DU_OU_HIST_REG = 1, APU_HIST_REG = 2, EAPU_HIST_REG = 3 };
#endif
#ifdef L68
enum { CU_HIST_REG = 0, DU_HIST_REG = 1, OU_HIST_REG = 2, APU_HIST_REG = 3 };
#endif


#endif // DPS8_HW_CONSTS_H
