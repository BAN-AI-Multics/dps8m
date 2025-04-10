/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 1414c8ee-f62f-11ec-885a-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

#include "dps8.h"
#include "dps8_sys.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_ins.h"
#include "dps8_opcodetable.h"
#include "dps8_utils.h"

#define DBG_CTR 1

#define FREE(p) do  \
  {                 \
    free((p));      \
    (p) = NULL;     \
  } while(0)

/*
 * misc utility routines used by simulator
 */

char * dump_flags(char * buffer, word18 flags)
{
    (void)sprintf(buffer, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
                  flags & I_HEX   ? "Hex "   : "",  // L68 will never have I_HEX set, so no need to DPS8M-only
                  flags & I_ABS   ? "Abs "   : "",
                  flags & I_MIF   ? "MIF "   : "",
                  flags & I_TRUNC ? "Trunc " : "",
                  flags & I_NBAR  ? "~BAR "  : "",
                  flags & I_PMASK ? "PMask " : "",
                  flags & I_PERR  ? "PErr"   : "",
                  flags & I_TALLY ? "Tally " : "",
                  flags & I_OMASK ? "OMASK " : "",
                  flags & I_EUFL  ? "EUFL "  : "",
                  flags & I_EOFL  ? "EOFL "  : "",
                  flags & I_OFLOW ? "Ovr "   : "",
                  flags & I_CARRY ? "Carry " : "",
                  flags & I_NEG   ? "Neg "   : "",
                  flags & I_ZERO  ? "Zero "  : ""
                 );
    return buffer;
}

static char * dps8_strupr(char *str)
{
    char *s;

    for(s = str; *s; s++)
        *s = (char) toupper((unsigned char)*s);
    return str;
}

//! get instruction info for IWB ...

static struct opcode_s unimplented = {"(unimplemented)", 0, 0, 0, 0};

struct opcode_s * get_iwb_info  (DCDstruct * i)
  {
    struct opcode_s * p = &opcodes10[i->opcode10];
    return p->mne ? p : &unimplented;
  }

char *disassemble(char * result, word36 instruction)
{
    uint32 opcode   = GET_OP(instruction);   ///< get opcode
    uint32 opcodeX  = GET_OPX(instruction);  ///< opcode extension
    uint32 opcode10 = opcode | (opcodeX ? 01000 : 0);
    word18 address  = GET_ADDR(instruction);
    word1  a        = GET_A(instruction);
    //int32 i       = GET_I(instruction);
    word6  tag      = GET_TAG(instruction);

    //static char result[132] = "???";
    strcpy(result, "???");

    // get mnemonic ...
    if (opcodes10[opcode10].mne)
        strcpy(result, opcodes10[opcode10].mne);

    // XXX need to reconstruct multi-word EIS instruction.

    char buff[256];

    if (a)
    {
        int n = (address >> 15) & 07;
        int offset = address & 077777;

        (void)sprintf (buff, " pr%d|%o", n, offset);
        strcat (result, buff);
        // return dps8_strupr(result);
    } else {
        (void)sprintf (buff, " %06o", address);
        strcat (result, buff);
    }
    // get mod
    strcpy(buff, "");
    for(uint n = 0 ; n < 0100 ; n++)
        if (extMods[n].mod)
            if(n == tag)
            {
                strncpy(buff, extMods[n].mod, sizeof(buff) - 1);
                buff[sizeof(buff) - 1] = '\0';
                break;
            }

    if (strlen(buff))
    {
        strcat(result, ",");
        strcat(result, buff);
    }

    return dps8_strupr(result);
}

/*
 * get_mod__string ()
 *
 * Convert instruction address modifier tag to printable string
 * WARNING: returns pointer to statically allocated string
 */

char *get_mod_string(char * msg, word6 tag)
{
    strcpy(msg, "none");

    if (tag >= 0100) //-V536
    {
        (void)sprintf (msg, "getModReg(tag out-of-range %o)", tag);
    } else {
        for(uint n = 0 ; n < 0100 ; n++) //-V536
            if (extMods[n].mod)
                if(n == tag)
                {
                    strcpy(msg, extMods[n].mod);
                    break;
                }

    }
    return msg;
}

/*
 * 36-bit arithmetic stuff ...
 */
/* Single word integer routines */

word36 Add36b (cpu_state_t * cpup, word36 op1, word36 op2, word1 carryin, word18 flagsToSet, word18 * flags, bool * ovf)
  {
    CPT (cpt2L, 17); // Add36b
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "Add36b op1 %012"PRIo64" op2 %012"PRIo64" carryin %o flagsToSet %06o flags %06o\r\n",
               op1, op2, carryin, flagsToSet, * flags);

// See: https://en.wikipedia.org/wiki/Two%27s_complement#Addition

    // 37 bit arithmetic for the above N+1 algorithm
    word38 op1e = op1 & MASK36;
    word38 op2e = op2 & MASK36;
    word38 ci   = carryin ? 1 : 0;

    // extend sign bits
    if (op1e & SIGN36)
      op1e |= BIT37;
    if (op2e & SIGN36)
      op2e |= BIT37;

    // Do the math
    word38 res = op1e + op2e + ci;

    // Extract the overflow bits
    bool r37 = res & BIT37 ? true : false;
    bool r36 = res & SIGN36 ? true : false;

    // Extract the carry bit
    bool r38 = res & BIT38 ? true : false;

    // Check for overflow
    * ovf = r37 ^ r36;

    // Check for carry
    bool cry = r38;

    // Truncate the result
    res &= MASK36;

#if defined(PANEL68)
    if (cry) CPT (cpt2L, 28); // carry
    if (ovf) CPT (cpt2L, 29); // ovf
    if (!res) CPT (cpt2L, 30); // zero
    if (res & SIGN36) CPT (cpt2L, 31); // neg
#endif

    if (flagsToSet & I_CARRY)
      {
        if (cry)
          SETF (* flags, I_CARRY);
        else
          CLRF (* flags, I_CARRY);
      }

    if (chkOVF (cpup) && (flagsToSet & I_OFLOW))
      {
        if (* ovf)
          SETF (* flags, I_OFLOW);      // overflow
      }

    if (flagsToSet & I_ZERO)
      {
        if (res)
          CLRF (* flags, I_ZERO);
        else
          SETF (* flags, I_ZERO);       // zero result
      }

    if (flagsToSet & I_NEG)
      {
        if (res & SIGN36)
          SETF (* flags, I_NEG);
        else
          CLRF (* flags, I_NEG);
      }

    sim_debug (DBG_TRACEEXT, & cpu_dev, "Add36b res %012"PRIo64" flags %06o ovf %o\r\n", res, * flags, * ovf);
    return res;
  }

word36 Sub36b (cpu_state_t * cpup, word36 op1, word36 op2, word1 carryin, word18 flagsToSet, word18 * flags, bool * ovf)
  {
    CPT (cpt2L, 18); // Sub36b

// See: https://en.wikipedia.org/wiki/Two%27s_complement

// AL39:
//
//  If carry indicator ON, then C(A) - C(Y) -> C(A)
//  If carry indicator OFF, then C(A) - C(Y) - 1 -> C(A)

    // 38 bit arithmetic for the above N+1 algorithm
    word38 op1e = op1 & MASK36;
    word38 op2e = op2 & MASK36;
    // Note that carryin has an inverted sense for borrow
    word38 ci   = carryin ? 0 : 1;

    // extend sign bits
    if (op1e & SIGN36)
      op1e |= BIT37;
    if (op2e & SIGN36)
      op2e |= BIT37;

    // Do the math
    // word38 res = op1e - op2e - ci;
    // ubsan
    word38 res = (word38) (((word38s) op1e) - ((word38s) op2e) - ((word38) ci));

    // Extract the overflow bits
    bool r37 = (res & BIT37) ? true : false;
    bool r36 = (res & SIGN36) ? true : false;

    // Extract the carry bit
    bool r38 = res & BIT38 ? true : false;

    // Truncate the result
    res &= MASK36;

    // Check for overflow
    * ovf = r37 ^ r36;

    // Check for carry
    bool cry = r38;

#if defined(PANEL68)
    if (cry) CPT (cpt2L, 28); // carry
    if (ovf) CPT (cpt2L, 29); // ovf
    if (!res) CPT (cpt2L, 30); // zero
    if (res & SIGN36) CPT (cpt2L, 31); // neg
#endif

    if (flagsToSet & I_CARRY)
      {
        if (cry) // Note inverted logic for subtraction
          CLRF (* flags, I_CARRY);
        else
          SETF (* flags, I_CARRY);
      }

    if (chkOVF (cpup) && (flagsToSet & I_OFLOW))
      {
        if (* ovf)
          SETF (* flags, I_OFLOW);      // overflow
      }

    if (flagsToSet & I_ZERO)
      {
        if (res)
          CLRF (* flags, I_ZERO);
        else
          SETF (* flags, I_ZERO);       // zero result
      }

    if (flagsToSet & I_NEG)
      {
        if (res & SIGN36)
          SETF (* flags, I_NEG);
        else
          CLRF (* flags, I_NEG);
      }

    return res;
  }

word18 Add18b (cpu_state_t * cpup, word18 op1, word18 op2, word1 carryin, word18 flagsToSet, word18 * flags, bool * ovf)
  {
    CPT (cpt2L, 19); // Add18b

// See: https://en.wikipedia.org/wiki/Two%27s_complement#Addition

    // 19 bit arithmetic for the above N+1 algorithm
    word20 op1e = op1 & MASK18;
    word20 op2e = op2 & MASK18;
    word20 ci   = carryin ? 1 : 0;

    // extend sign bits
    if (op1e & SIGN18)
      op1e |= BIT19;
    if (op2e & SIGN18)
      op2e |= BIT19;

    // Do the math
    word20 res = op1e + op2e + ci;

    // Extract the overflow bits
    bool r19 = (res & BIT19)  ? true : false;
    bool r18 = (res & SIGN18) ? true : false;

    // Extract the carry bit
    bool r20 = res & BIT20 ? true : false;

    // Truncate the result
    res &= MASK18;

    // Check for overflow
    * ovf = r19 ^ r18;

    // Check for carry
    bool cry = r20;

#if defined(PANEL68)
    if (cry) CPT (cpt2L, 28); // carry
    if (ovf) CPT (cpt2L, 29); // ovf
    if (!res) CPT (cpt2L, 30); // zero
    if (res & SIGN36) CPT (cpt2L, 31); // neg
#endif

    if (flagsToSet & I_CARRY)
      {
        if (cry)
          SETF (* flags, I_CARRY);
        else
          CLRF (* flags, I_CARRY);
      }

    if (chkOVF (cpup) && (flagsToSet & I_OFLOW))
      {
        if (* ovf)
          SETF (* flags, I_OFLOW);      // overflow
      }

    if (flagsToSet & I_ZERO)
      {
        if (res)
          CLRF (* flags, I_ZERO);
        else
          SETF (* flags, I_ZERO);       // zero result
      }

    if (flagsToSet & I_NEG)
      {
        if (res & SIGN18)
          SETF (* flags, I_NEG);
        else
          CLRF (* flags, I_NEG);
      }

    return (word18) res;
  }

word18 Sub18b (cpu_state_t * cpup, word18 op1, word18 op2, word1 carryin, word18 flagsToSet, word18 * flags, bool * ovf)
  {
    CPT (cpt2L, 20); // Sub18b

// See: https://en.wikipedia.org/wiki/Two%27s_complement

// AL39:
//
//  If carry indicator ON, then C(A) - C(Y) -> C(A)
//  If carry indicator OFF, then C(A) - C(Y) - 1 -> C(A)

    // 19 bit arithmetic for the above N+1 algorithm
    word20 op1e = op1 & MASK18;
    word20 op2e = op2 & MASK18;
    // Note that carryin has an inverted sense for borrow
    word20 ci   = carryin ? 0 : 1;

    // extend sign bits
    if (op1e & SIGN18)
      op1e |= BIT19;
    if (op2e & SIGN18)
      op2e |= BIT19;

    // Do the math
    // word20 res = op1e - op2e - ci;
    // ubsan
    word20 res = (word20) (((word20s) op1e) - ((word20s) op2e) - ((word20s) ci));

    // Extract the overflow bits
    bool r19 = res & BIT19  ? true : false;
    bool r18 = res & SIGN18 ? true : false;

    // Extract the carry bit
    bool r20 = res & BIT20 ? true : false;

    // Truncate the result
    res &= MASK18;

    // Check for overflow
    * ovf = r19 ^ r18;

    // Check for carry
    bool cry = r20;

#if defined(PANEL68)
    if (cry) CPT (cpt2L, 28); // carry
    if (ovf) CPT (cpt2L, 29); // ovf
    if (!res) CPT (cpt2L, 30); // zero
    if (res & SIGN36) CPT (cpt2L, 31); // neg
#endif

    if (flagsToSet & I_CARRY)
      {
        if (cry) // Note inverted logic for subtraction
          CLRF (* flags, I_CARRY);
        else
          SETF (* flags, I_CARRY);
      }

    if (chkOVF (cpup) && (flagsToSet & I_OFLOW))
      {
        if (* ovf)
          SETF (* flags, I_OFLOW);      // overflow
      }

    if (flagsToSet & I_ZERO)
      {
        if (res)
          CLRF (* flags, I_ZERO);
        else
          SETF (* flags, I_ZERO);       // zero result
      }

    if (flagsToSet & I_NEG)
      {
        if (res & SIGN18)
          SETF (* flags, I_NEG);
        else
          CLRF (* flags, I_NEG);
      }

    return res;
  }

word72 Add72b (cpu_state_t * cpup, word72 op1, word72 op2, word1 carryin, word18 flagsToSet, word18 * flags, bool * ovf)
  {
    CPT (cpt2L, 21); // Add72b

// See: https://en.wikipedia.org/wiki/Two%27s_complement#Addition

    // 73 bit arithmetic for the above N+1 algorithm
#if defined(NEED_128)
    word74 op1e = and_128 (op1, MASK72);
    word74 op2e = and_128 (op2, MASK72);
    word74 ci   = construct_128 (0, carryin ? 1 : 0);
#else
    word74 op1e = op1 & MASK72;
    word74 op2e = op2 & MASK72;
    word74 ci   = carryin ? 1 : 0;
#endif

    // extend sign bits
#if defined(NEED_128)
    if (isnonzero_128 (and_128 (op1e, SIGN72)))
      op1e = or_128 (op1e, BIT73);
    if (isnonzero_128 (and_128 (op2e, SIGN72)))
      op2e = or_128 (op2e, BIT73);
#else
    if (op1e & SIGN72)
      op1e |= BIT73;
    if (op2e & SIGN72)
      op2e |= BIT73;
#endif

    // Do the math
#if defined(NEED_128)
    word74 res = add_128 (op1e, add_128 (op2e, ci));
#else
    word74 res = op1e + op2e + ci;
#endif

    // Extract the overflow bits
#if defined(NEED_128)
    bool r73 = isnonzero_128 (and_128 (res, BIT73));
    bool r72 = isnonzero_128 (and_128 (res, SIGN72));
#else
    bool r73 = res & BIT73  ? true : false;
    bool r72 = res & SIGN72 ? true : false;
#endif

    // Extract the carry bit
#if defined(NEED_128)
    bool r74 = isnonzero_128 (and_128 (res, BIT74));
#else
    bool r74 = res & BIT74 ? true : false;
#endif

    // Truncate the result
#if defined(NEED_128)
    res = and_128 (res, MASK72);
#else
    res &= MASK72;
#endif

    // Check for overflow
    * ovf = r73 ^ r72;

    // Check for carry
    bool cry = r74;

#if defined(PANEL68)
    if (cry) CPT (cpt2L, 28); // carry
    if (ovf) CPT (cpt2L, 29); // ovf
    if (!res) CPT (cpt2L, 30); // zero
    if (res & SIGN36) CPT (cpt2L, 31); // neg
#endif

    if (flagsToSet & I_CARRY)
      {
        if (cry)
          SETF (* flags, I_CARRY);
        else
          CLRF (* flags, I_CARRY);
      }

    if (chkOVF (cpup) && (flagsToSet & I_OFLOW))
      {
        if (* ovf)
          SETF (* flags, I_OFLOW);      // overflow
      }

    if (flagsToSet & I_ZERO)
      {
#if defined(NEED_128)
        if (isnonzero_128 (res))
#else
        if (res)
#endif
          CLRF (* flags, I_ZERO);
        else
          SETF (* flags, I_ZERO);       // zero result
      }

    if (flagsToSet & I_NEG)
      {
#if defined(NEED_128)
        if (isnonzero_128 (and_128 (res, SIGN72)))
#else
        if (res & SIGN72)
#endif
          SETF (* flags, I_NEG);
        else
          CLRF (* flags, I_NEG);
      }

    return res;
  }

word72 Sub72b (cpu_state_t * cpup, word72 op1, word72 op2, word1 carryin, word18 flagsToSet, word18 * flags, bool * ovf)
  {
    CPT (cpt2L, 22); // Sub72b
#if defined(NEED_128)
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "Sub72b op1 %012"PRIo64"%012"PRIo64" op2 %012"PRIo64"%012"PRIo64" carryin %o flagsToSet %06o flags %06o\r\n",
               (word36) ((rshift_128 (op1, 36).l) & MASK36),
               (word36) (op1.l & MASK36),
               (word36) (rshift_128 (op2, 36).l & MASK36),
               (word36) (op2.l & MASK36),
               carryin, flagsToSet, * flags);
#else
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "Sub72b op1 %012"PRIo64"%012"PRIo64" op2 %012"PRIo64"%012"PRIo64" carryin %o flagsToSet %06o flags %06o\r\n",
               (word36) ((op1 >> 36) & MASK36),
               (word36) (op1 & MASK36),
               (word36) ((op2 >> 36) & MASK36),
               (word36) (op2 & MASK36),
               carryin, flagsToSet, * flags);
#endif

// See: https://en.wikipedia.org/wiki/Two%27s_complement

// AL39:
//
//  If carry indicator ON, then C(A) - C(Y) -> C(A)
//  If carry indicator OFF, then C(A) - C(Y) - 1 -> C(A)

    // 73 bit arithmetic for the above N+1 algorithm
#if defined(NEED_128)
    word74 op1e = and_128 (op1, MASK72);
    word74 op2e = and_128 (op2, MASK72);
#else
    word74 op1e = op1 & MASK72;
    word74 op2e = op2 & MASK72;
#endif
    // Note that carryin has an inverted sense for borrow
#if defined(NEED_128)
    word74 ci = construct_128 (0, carryin ? 0 : 1);
#else
    word74 ci = carryin ? 0 : 1;
#endif

    // extend sign bits
#if defined(NEED_128)
    if (isnonzero_128 (and_128 (op1e, SIGN72)))
      op1e = or_128 (op1e, BIT73);
    if (isnonzero_128 (and_128 (op2e, SIGN72)))
      op2e = or_128 (op2e, BIT73);
#else
    if (op1e & SIGN72)
      op1e |= BIT73;
    if (op2e & SIGN72)
      op2e |= BIT73;
#endif

    // Do the math
#if defined(NEED_128)
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "Sub72b op1e %012"PRIo64"%012"PRIo64" op2e %012"PRIo64"%012"PRIo64" carryin %o flagsToSet %06o flags %06o\r\n",
               (word36) ((rshift_128 (op1e, 36).l) & MASK36),
               (word36) (op1e.l & MASK36),
               (word36) (rshift_128 (op2e, 36).l & MASK36),
               (word36) (op2e.l & MASK36),
               carryin, flagsToSet, * flags);
#else
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "Sub72b op1e %012"PRIo64"%012"PRIo64" op2e %012"PRIo64"%012"PRIo64" carryin %o flagsToSet %06o flags %06o\r\n",
               (word36) ((op1e >> 36) & MASK36),
               (word36) (op1e & MASK36),
               (word36) ((op2e >> 36) & MASK36),
               (word36) (op2e & MASK36),
               carryin, flagsToSet, * flags);
#endif
#if defined(NEED_128)
    word74 res = subtract_128 (subtract_128 (op1e, op2e), ci);
#else
    // word74 res = op1e - op2e - ci;
    // ubsan
    word74 res = (word72) (((word72s) op1e) - ((word72s) op2e) - ((word72s) ci));
#endif
#if defined(NEED_128)
    sim_debug (DBG_TRACEEXT, & cpu_dev, "Sub72b res %012"PRIo64"%012"PRIo64" flags %06o ovf %o\r\n",
               (word36) (rshift_128 (res, 36).l & MASK36), (word36) (res.l & MASK36), * flags, * ovf);
#else
    sim_debug (DBG_TRACEEXT, & cpu_dev, "Sub72b res %012"PRIo64"%012"PRIo64" flags %06o ovf %o\r\n",
               (word36) ((res >> 36) & MASK36), (word36) (res & MASK36), * flags, * ovf);
#endif

    // Extract the overflow bits
#if defined(NEED_128)
    bool r73 = isnonzero_128 (and_128 (res, BIT73));
    bool r72 = isnonzero_128 (and_128 (res, SIGN72));
#else
    bool r73 = res & BIT73  ? true : false;
    bool r72 = res & SIGN72 ? true : false;
#endif

    // Extract the carry bit
#if defined(NEED_128)
    bool r74 = isnonzero_128 (and_128 (res, BIT74));
#else
    bool r74 = res & BIT74 ? true : false;
#endif

    // Truncate the result
#if defined(NEED_128)
    res = and_128 (res, MASK72);
#else
    res &= MASK72;
#endif

    // Check for overflow
    * ovf = r73 ^ r72;

    // Check for carry
    bool cry = r74;

#if defined(PANEL68)
    if (cry) CPT (cpt2L, 28); // carry
    if (ovf) CPT (cpt2L, 29); // ovf
    if (!res) CPT (cpt2L, 30); // zero
    if (res & SIGN36) CPT (cpt2L, 31); // neg
#endif

    if (flagsToSet & I_CARRY)
      {
        if (cry) // Note inverted logic for subtraction
          CLRF (* flags, I_CARRY);
        else
          SETF (* flags, I_CARRY);
      }

    if (chkOVF (cpup) && (flagsToSet & I_OFLOW))
      {
        if (* ovf)
          SETF (* flags, I_OFLOW);      // overflow
      }

    if (flagsToSet & I_ZERO)
      {
#if defined(NEED_128)
        if (isnonzero_128 (res))
#else
        if (res)
#endif
          CLRF (* flags, I_ZERO);
        else
          SETF (* flags, I_ZERO);       // zero result
      }

    if (flagsToSet & I_NEG)
      {
#if defined(NEED_128)
        if (isnonzero_128 (and_128 (res, SIGN72)))
#else
        if (res & SIGN72)
#endif
          SETF (* flags, I_NEG);
        else
          CLRF (* flags, I_NEG);
      }

#if defined(NEED_128)
    sim_debug (DBG_TRACEEXT, & cpu_dev, "Sub72b res %012"PRIo64"%012"PRIo64" flags %06o ovf %o\r\n",
               (word36) (rshift_128 (res, 36).l & MASK36), (word36) (res.l & MASK36), * flags, * ovf);
#else
    sim_debug (DBG_TRACEEXT, & cpu_dev, "Sub72b res %012"PRIo64"%012"PRIo64" flags %06o ovf %o\r\n",
               (word36) ((res >> 36) & MASK36), (word36) (res & MASK36), * flags, * ovf);
#endif
    return res;
  }

// CANFAULT
word36 compl36(cpu_state_t * cpup, word36 op1, word18 *flags, bool * ovf)
{
    CPT (cpt2L, 23); // compl36
    //(void)printf("op1 = %"PRIo64" %"PRIo64"\r\n", op1, (-op1) & DMASK);

    op1 &= DMASK;

    // word36 res = -op1 & DMASK;
    // ubsan
    word36 res = ((word36) (- ((word36s) op1))) & DMASK;

    * ovf = op1 == MAXNEG;

#if defined(PANEL68)
    if (* ovf) CPT (cpt2L, 29); // ovf
    if (!res) CPT (cpt2L, 30); // zero
    if (res & SIGN36) CPT (cpt2L, 31); // neg
#endif

    if (chkOVF (cpup) && * ovf)
        SETF(*flags, I_OFLOW);

    if (res & SIGN36)
        SETF(*flags, I_NEG);
    else
        CLRF(*flags, I_NEG);

    if (res == 0)
        SETF(*flags, I_ZERO);
    else
        CLRF(*flags, I_ZERO);

    return res;
}

// CANFAULT
word18 compl18(cpu_state_t * cpup, word18 op1, word18 *flags, bool * ovf)
{
    CPT (cpt2L, 24); // compl18
    //(void)printf("op1 = %"PRIo64" %"PRIo64"\r\n", op1, (-op1) & DMASK);

    op1 &= MASK18;

    // word18 res = -op1 & MASK18;
    // ubsan
    word18 res = ((word18) (- (word18s) op1)) & MASK18;

    * ovf = op1 == MAX18NEG;
#if defined(PANEL68)
    if (* ovf) CPT (cpt2L, 29); // ovf
    if (!res) CPT (cpt2L, 30); // zero
    if (res & SIGN18) CPT (cpt2L, 31); // neg
#endif

    if (chkOVF (cpup) && * ovf)
        SETF(*flags, I_OFLOW);
    if (res & SIGN18)
        SETF(*flags, I_NEG);
    else
        CLRF(*flags, I_NEG);

    if (res == 0)
        SETF(*flags, I_ZERO);
    else
        CLRF(*flags, I_ZERO);

    return res;
}

void copyBytes(int posn, word36 src, word36 *dst)
{
    word36 mask = 0;

    if (posn & 8) // bit 30 - byte 0 - (bits 0-8)
        mask |= 0777000000000LL;

    if (posn & 4) // bit 31 - byte 1 - (bits 9-17)
        mask |= 0000777000000LL;

    if (posn & 2) // bit 32 - byte 2 - (bits 18-26)
        mask |= 0000000777000LL;

    if (posn & 1) // bit 33 - byte 3 - (bits 27-35)
        mask |= 0000000000777LL;

    word36 byteVals = src & mask;   // get byte bits

    // clear the bits in dst
    *dst &= ~mask;

    // and set the bits in dst
    *dst |= byteVals;
}

void copyChars(int posn, word36 src, word36 *dst)
{
    word36 mask = 0;

    if (posn & 32) // bit 30 - char 0 - (bits 0-5)
        mask |= 0770000000000LL;

    if (posn & 16) // bit 31 - char 1 - (bits 6-11)
        mask |= 0007700000000LL;

    if (posn & 8) // bit 32 - char 2 - (bits 12-17)
        mask |= 0000077000000LL;

    if (posn & 4) // bit 33 - char 3 - (bits 18-23)
        mask |= 0000000770000LL;

    if (posn & 2) // bit 34 - char 4 - (bits 24-29)
        mask |= 0000000007700LL;

    if (posn & 1) // bit 35 - char 5 - (bits 30-35)
        mask |= 0000000000077LL;

    word36 byteVals = src & mask;   // get byte bits

    // clear the bits in dst
    *dst &= ~mask;

    // and set the bits in dst
    *dst |= byteVals;
}

/*!
 * write 9-bit byte into 36-bit word....
 */
void putByte(word36 *dst, word9 data, int posn)
{
    // XXX which is faster switch() or calculation?

//    int offset = 27 - (9 * posn);//    0;
//    switch (posn)
//    {
//        case 0:
//            offset = 27;
//            break;
//        case 1:
//            offset = 18;
//            break;
//        case 2:
//            offset = 9;
//            break;
//        case 3:
//            offset = 0;
//            break;
//    }
    putbits36_9 (dst, (uint) posn * 9, data);
}

void putChar(word36 *dst, word6 data, int posn)
{
    // XXX which is faster switch() or calculation?

//    int offset = 30 - (6 * posn);   //0;
//    switch (posn)
//    {
//        case 0:
//            offset = 30;
//            break;
//        case 1:
//            offset = 24;
//            break;
//        case 2:
//            offset = 18;
//            break;
//        case 3:
//            offset = 12;
//            break;
//        case 4:
//            offset = 6;
//            break;
//        case 5:
//            offset = 0;
//            break;
//    }
    putbits36_6 (dst, (uint) posn * 6, data);
}

word72 convert_to_word72(word36 even, word36 odd)
{
#if defined(NEED_128)
    return or_128 (lshift_128 (construct_128 (0, even), 36), construct_128 (0, odd));
#else
    return ((word72)even << 36) | (word72)odd;
#endif
}

void convert_to_word36 (word72 src, word36 *even, word36 *odd)
{
#if defined(NEED_128)
    *even = rshift_128 (src, 36).l & DMASK;
    *odd  = src.l & DMASK;
#else
    *even = (word36)(src >> 36) & DMASK;
    *odd  = (word36)src & DMASK;
#endif
}

void cmp36(cpu_state_t * cpup, word36 oP1, word36 oP2, word18 *flags)
  {
    CPT (cpt2L, 25); // cmp36
    L68_ (cpu.ou.cycle |= ou_GOS;)
    t_int64 op1 = SIGNEXT36_64(oP1 & DMASK);
    t_int64 op2 = SIGNEXT36_64(oP2 & DMASK);

    word36 sign1 = (word36) op1 & SIGN36;
    word36 sign2 = (word36) op2 & SIGN36;

    if ((! sign1) && sign2)  // op1 > 0, op2 < 0 :: op1 > op2
      CLRF (* flags, I_ZERO | I_NEG | I_CARRY);

    else if (sign1 == sign2) // both operands have the same sign
      {
         if (op1 > op2)
           {
             CPT (cpt2L, 28); // carry
             SETF (* flags, I_CARRY);
             CLRF (* flags, I_ZERO | I_NEG);
           }
         else if (op1 == op2)
           {
             CPT (cpt2L, 28); // carry
             CPT (cpt2L, 30); // zero
             SETF (* flags, I_ZERO | I_CARRY);
             CLRF (* flags, I_NEG);
           }
         else //  op1 < op2
          {
            CPT (cpt2L, 31); // neg
            SETF (* flags, I_NEG);
            CLRF (* flags, I_ZERO | I_CARRY);
          }
      }
    else // op1 < 0, op2 > 0 :: op1 < op2
      {
        CPT  (cpt2L, 28); // carry
        CPT  (cpt2L, 31); // neg
        SETF (* flags, I_CARRY | I_NEG);
        CLRF (* flags, I_ZERO);
      }
  }

void cmp18(cpu_state_t * cpup, word18 oP1, word18 oP2, word18 *flags)
  {
    CPT (cpt2L, 26); // cmp18
    L68_ (cpu.ou.cycle |= ou_GOS;)
    int32 op1 = SIGNEXT18_32 (oP1 & MASK18);
    int32 op2 = SIGNEXT18_32 (oP2 & MASK18);

    word18 sign1 = (word18) op1 & SIGN18;
    word18 sign2 = (word18) op2 & SIGN18;

    if ((! sign1) && sign2)  // op1 > 0, op2 < 0 :: op1 > op2
      CLRF (* flags, I_ZERO | I_NEG | I_CARRY);

    else if (sign1 == sign2) // both operands have the same sign
      {
        if (op1 > op2)
          {
            CPT (cpt2L, 28); // carry
            SETF (* flags, I_CARRY);
            CLRF (* flags, I_ZERO | I_NEG);
          }
        else if (op1 == op2)
          {
            CPT (cpt2L, 28); // carry
            CPT (cpt2L, 30); // zero
            SETF (* flags, I_ZERO | I_CARRY);
            CLRF (* flags, I_NEG);
          }
        else //  op1 < op2
          {
            CPT (cpt2L, 31); // neg
            SETF (* flags, I_NEG);
            CLRF (* flags, I_ZERO | I_CARRY);
          }
      }
    else // op1 < 0, op2 > 0 :: op1 < op2
      {
        CPT (cpt2L, 28); // carry
        CPT (cpt2L, 31); // neg
        SETF (* flags, I_CARRY | I_NEG);
        CLRF (* flags, I_ZERO);
      }
  }

void cmp36wl(cpu_state_t * cpup, word36 A, word36 Y, word36 Q, word18 *flags)
{
    CPT (cpt2L, 26); // cmp36wl
    // This is wrong; signed math is needed.

    //bool Z = (A <= Y && Y <= Q) || (A >= Y && Y >= Q);

    L68_ (cpu.ou.cycle |= ou_GOS;)
    t_int64 As = (word36s) SIGNEXT36_64(A & DMASK);
    t_int64 Ys = (word36s) SIGNEXT36_64(Y & DMASK);
    t_int64 Qs = (word36s) SIGNEXT36_64(Q & DMASK);
    bool Z = (As <= Ys && Ys <= Qs) || (As >= Ys && Ys >= Qs);

    SCF(Z, *flags, I_ZERO);

    if (!(Q & SIGN36) && (Y & SIGN36) && (Qs > Ys))
        CLRF(*flags, I_NEG | I_CARRY);
    else if (((Q & SIGN36) == (Y & SIGN36)) && (Qs >= Ys))
    {
        CPT (cpt2L, 28); // carry
        SETF(*flags, I_CARRY);
        CLRF(*flags, I_NEG);
    } else if (((Q & SIGN36) == (Y & SIGN36)) && (Qs < Ys))
    {
        CPT (cpt2L, 31); // neg
        CLRF(*flags, I_CARRY);
        SETF(*flags, I_NEG);
    } else if ((Q & SIGN36) && !(Y & SIGN36) && (Qs < Ys))
    {
        CPT (cpt2L, 28); // carry
        CPT (cpt2L, 31); // neg
        SETF(*flags, I_NEG | I_CARRY);
    }
}

void cmp72(cpu_state_t * cpup, word72 op1, word72 op2, word18 *flags)
{
    CPT (cpt2L, 27); // cmp72
   // The case of op1 == 400000000000000000000000 and op2 == 0 falls through
   // this code.
    L68_ (cpu.ou.cycle |= ou_GOS;)
#if defined(NEED_128)
sim_debug (DBG_TRACEEXT, & cpu_dev, "op1 %016"PRIx64"%016"PRIx64"\r\n", op1.h, op1.l);
sim_debug (DBG_TRACEEXT, & cpu_dev, "op2 %016"PRIx64"%016"PRIx64"\r\n", op2.h, op2.l);
    int128 op1s =  SIGNEXT72_128 (and_128 (op1, MASK72));
    int128 op2s =  SIGNEXT72_128 (and_128 (op2, MASK72));
sim_debug (DBG_TRACEEXT, & cpu_dev, "op1s %016"PRIx64"%016"PRIx64"\r\n", op1s.h, op1s.l);
sim_debug (DBG_TRACEEXT, & cpu_dev, "op2s %016"PRIx64"%016"PRIx64"\r\n", op2s.h, op2s.l);
#else
sim_debug (DBG_TRACEEXT, & cpu_dev, "op1 %016"PRIx64"%016"PRIx64"\r\n", (uint64_t) (op1>>64), (uint64_t) op1);
sim_debug (DBG_TRACEEXT, & cpu_dev, "op2 %016"PRIx64"%016"PRIx64"\r\n", (uint64_t) (op2>>64), (uint64_t) op2);
    int128 op1s =  SIGNEXT72_128 (op1 & MASK72);
    int128 op2s =  SIGNEXT72_128 (op2 & MASK72);
sim_debug (DBG_TRACEEXT, & cpu_dev, "op1s %016"PRIx64"%016"PRIx64"\r\n", (uint64_t) (op1s>>64), (uint64_t) op1s);
sim_debug (DBG_TRACEEXT, & cpu_dev, "op2s %016"PRIx64"%016"PRIx64"\r\n", (uint64_t) (op2s>>64), (uint64_t) op2s);
#endif
#if defined(NEED_128)
    if (isgt_s128 (op1s, op2s))
#else
    if (op1s > op2s)
#endif
      {
#if defined(NEED_128)
        if (isnonzero_128 (and_128 (op2, SIGN72)))
#else
        if (op2 & SIGN72)
#endif
          CLRF (* flags, I_CARRY);
        else
          {
            CPT (cpt2L, 28); // carry
            SETF (* flags, I_CARRY);
          }
        CLRF (* flags, I_ZERO | I_NEG);
      }
#if defined(NEED_128)
    else if (iseq_128 (cast_128 (op1s), cast_128 (op2s)))
#else
    else if (op1s == op2s)
#endif
      {
        CPT (cpt2L, 28); // carry
        CPT (cpt2L, 30); // zero
        SETF (* flags, I_CARRY | I_ZERO);
        CLRF (* flags, I_NEG);
      }
    else /* op1s < op2s */
      {
        CPT (cpt2L, 31); // neg
#if defined(NEED_128)
        if (isnonzero_128 (and_128 (op1, SIGN72)))
#else
        if (op1 & SIGN72)
#endif
          {
            CPT (cpt2L, 28); // carry
            SETF (* flags, I_CARRY);
          }
        else
          CLRF (* flags, I_CARRY);
        CLRF (* flags, I_ZERO);
        SETF (* flags, I_NEG);
      }
}

/*
 * String utilities ...
 */

char * strlower(char *q)
{
        char *s = q;

        while (*s) {
                if (isupper((unsigned char)*s))
                        *s = (char) tolower(*s);
                s++;
        }
        return q;
}

/*  state definitions  */
#define STAR    0
#define NOTSTAR 1
#define RESET   2

int strmask (char * str, char * mask)
/*!
 Tests string 'str' against mask string 'mask'
 Returns TRUE if the string matches the mask.

 The mask can contain '?' and '*' wild card characters.
 '?' matches any        single character.
 '*' matches any number of any characters.

 For example:
 strmask("Hello", "Hello");     ---> TRUE
 strmask("Hello", "Jello");     ---> FALSE
 strmask("Hello", "H*o");       ---> TRUE
 strmask("Hello", "H*g");       ---> FALSE
 strmask("Hello", "?ello");     ---> TRUE
 strmask("Hello", "H????");     ---> TRUE
 strmask("H", "H????");         ---> FALSE
 */
  {
    char * sp, * mp, * reset_string, * reset_mask, * sn;
    int state;

    sp = str;
    mp = mask;

    while (1)
      {
        switch (* mp)
          {
            case '\0':
              return * sp ? false : true;

            case '?':
              sp ++;
              mp ++;
              break;

            default:
              if (* mp == * sp)
                {
                  sp ++;
                  mp ++;
                  break;
                }
              else
                {
                  return false;
                }

            case '*':
              if (* (mp + 1) == '\0')
                {
                  return true;
                }
              if ((sn = strchr (sp, * (mp + 1))) == NULL)
                {
                  return false;
                }

              /* save place -- match rest of string */
              /* if fail, reset to here */
              reset_mask = mp;
              reset_string = sn + 1;

              mp = mp + 2;
              sp = sn + 1;
              state = NOTSTAR;
              while (state == NOTSTAR)
                {
                  switch (* mp)
                    {
                      case '\0':
                        if (* sp == '\0')
                          return false;
                        else
                          state = RESET;
                        break;
                      case '?':
                        sp ++;
                        mp ++;
                        break;
                      default:
                        if (* mp == * sp)
                          {
                            sp ++;
                            mp ++;
                          }
                        else
                          state = RESET;
                        break;
                      case '*':
                        state = STAR;
                        break;
                    }
                } // while STATE == NOTSTAR
              /* we've reach a new star or should reset to last star */
              if (state == RESET)
                {
                  sp = reset_string;
                  mp = reset_mask;
                }
              break;
          } // switch (* mp)
      } // while (1)
#if defined(SUNLINT) || !defined(__SUNPRO_C) && !defined(__SUNPRO_CC)
    /*NOTREACHED*/ /* unreachable */
    return false;
#endif /* if defined(SUNLINT) || !defined(__SUNPRO_C) && !defined(__SUNPRO_CC) */
  }

#if 0
/*
 * strtok() with string quoting...
 * (implemented as a small fsm, kind of...
 * (add support for embedded " later, much later...)
 */
# define NORMAL 1
# define IN_STRING 2
# define EOB 3

char * Strtok (char * line, char * sep) {
    static char *p = NULL; // current pointer position in input line; initialized to NULL for robustness.
    static int state = NORMAL;

    char *q; // beginning of current field

    if (line) { // 1st invocation
        p = line;
        state = NORMAL;
    }

    if (! p) {
      // Called without proper initialization
      sim_warn ("Strtok called without initialization\r\n");
      // Treat it as an empty line
      state = NORMAL;
      return NULL;
    }

    q = p;
    while (state != EOB) {
        switch (state) {
            case NORMAL:
                switch (*p) {
                    case 0: // at end of buffer
                        state = EOB; // set state to "End Of Buffer
                        return q;

                    case '"': // beginning of a quoted string
                        state = IN_STRING; // we're in a string
                        p++;
                        continue;

                    default:    // only a few special characters
                        if (strchr(sep, *p) == NULL) { // not a sep
                            p++; // goto next char
                            continue;
                        } else {
                            *p++ = (char)0; /* ... iff >0 */
                            while (*p && strchr(sep, *p)) /* skip over separator(s)*/
                                p++;
                            return q; /* return field */
                        }
                }

            case IN_STRING:
                if (*p == 0) {   /*!< incomplete quoted string */
                    state = EOB;
                    return q;
                }

                if (*p != '"') { // not end of line and still in a string
                    p++;
                    continue;
                }
                state = NORMAL; /* end of quoted string */
                p++;

                continue;

            case EOB: /* just in case */
                state = NORMAL;
                return NULL;

            default:
                (void)fprintf(stderr, "(Strtok):unknown state - %d",state);
                state = EOB;
                return NULL;
        }

    }

    return NULL; /* no more fields in buffer */
}
#endif

#if 0
bool startsWith(const char *str, const char *pre)
{
    size_t lenpre = strlen(pre),
    lenstr = strlen(str);
    return lenstr < lenpre ? false : strncasecmp(pre, str, lenpre) == 0;
}
#endif

/*
 * Removes the trailing spaces from a string.
 */
char *rtrim(char *s)
{
    if (! s)
      return s;
    int index;

    //for (index = (int)strlen(s) - 1; index >= 0 && (s[index] == ' ' || s[index] == '\t'); index--)
    for (index = (int)strlen(s) - 1; index >= 0 && isspace((unsigned char)s[index]); index--)
    {
        s[index] = '\0';
    }
    return(s);
}

char *ltrim(char *s)
/*
 * Removes the leading spaces from a string.
 */
{
    char *p;
    if (s == NULL)
        return NULL;

    //for (p = s; (*p == ' ' || *p == '\t') && *p != '\0'; p++)
    for (p = s; isspace((unsigned char)*p) && *p != '\0'; p++)
        ;

    //strcpy(s, p);
    memmove(s, p, strlen(p) + 1);
    return(s);
}

char *trim(char *s)
{
    return ltrim(rtrim(s));
}

char *
stripquotes(char *s)
{
    if (! s || ! *s)
        return s;
    /*
     char *p;

     while ((p = strchr(s, '"')))
     *p = ' ';
     strchop(s);

     return s;
     */
    int nLast = (int)strlen(s) - 1;
    // trim away leading/trailing "'s
    if (s[0] == '"')
        s[0] = ' ';
    if (s[nLast] == '"')
        s[nLast] = ' ';
    return trim(s);
}

#include <ctype.h>

// XXX what about config=addr7=123, where clist has a "addr%"?

// return -2: error; -1: done; >= 0 option found
int cfg_parse (const char * tag, const char * cptr, config_list_t * clist, config_state_t * state, int64_t * result)
  {
    if (! cptr)
      return -2;
    char * start = NULL;
    if (! state -> copy)
      {
        state -> copy            = strdup (cptr);
        if (! state->copy)
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
        start                    = state -> copy;
        state ->  statement_save = NULL;
      }

    int ret = -2; // error

    // grab every thing up to the next semicolon
    char * statement;
    statement = strtok_r (start, ";", & state -> statement_save);
    start = NULL;
    if (! statement)
      {
        ret = -1; // done
        goto done;
      }

    // extract name
    char * name_start = statement;
    char * name_save  = NULL;
    char * name;
    name = strtok_r (name_start, "=", & name_save);
    if (! name)
      {
        sim_printf ("error: %s: can't parse name\r\n", tag);
        goto done;
      }

    // lookup name
    config_list_t * p = clist;
    while (p -> name)
      {
        if (strcasecmp (name, p -> name) == 0)
          break;
        p ++;
      }
    if (! p -> name)
      {
        sim_printf ("error: %s: don't know name <%s>\r\n", tag, name);
        goto done;
      }

    // extract value
    char * value;
    value = strtok_r (NULL, "", & name_save);
    if (! value)
      {
        // Special case; min>max and no value list
        // means that a missing value is ok
        if (p -> min > p -> max && ! p -> value_list)
          {
            return (int) (p - clist);
          }
        sim_printf ("error: %s: can't parse value\r\n", tag);
        goto done;
      }

    // first look to value in the value list
    config_value_list_t * v = p -> value_list;
    if (v)
      {
        while (v -> value_name)
          {
            if (strcasecmp (value, v -> value_name) == 0)
              break;
            v ++;
          }

        // Hit?
        if (v -> value_name)
          {
            * result = v -> value;
            return (int) (p - clist);
          }
      }

    // Must be a number
    if (p -> min > p -> max)
      {
        sim_printf ("error: %s: can't parse value\r\n", tag);
        goto done;
      }

    if (strlen (value) == 0)
      {
         sim_printf ("error: %s: missing value\r\n", tag);
         goto done;
      }
    char * endptr;
    int64_t n = strtoll (value, & endptr, 0);
    if (* endptr)
      {
        sim_printf ("error: %s: can't parse value\r\n", tag);
        goto done;
      }

// XXX small bug; doesn't check for junk after number...
    if (n < p -> min || n > p -> max)
      {
        sim_printf ("error: %s: value out of range\r\n", tag);
        goto done;
      }

    * result = n;
    return (int) (p - clist);

done:
    FREE (state -> copy);
    state -> copy = NULL;
    return ret;
  }

void cfg_parse_done (config_state_t * state)
  {
    if (state -> copy)
      FREE (state -> copy);
    state -> copy = NULL;
  }

// strdup with limited C-style escape processing
//
//  strdupesc ("foo\nbar") --> 'f' 'o' 'o' 012 'b' 'a' 'r'
//
//  Handles:
//   \\  backslash
//   \n  newline
//   \t  tab
//   \f  formfeed
//   \r  carriage return
//   \0  null              // doesn't work, commented out.
//
// \\ doesn't seem to work...
//  Also, a scp specific:
//
//   \e  (end simulation)
//
//  the scp parser doesn't handle these very well...
//
//   \_  space
//   \c  comma
//   \s  semicolon
//   \d  dollar
//   \q  double quote
//   \w  <backslash>
//   \z  ^D eof (DECism)
//   \^  caret
//   \x  expect; used by the autoinput parser
//
// And a special case:
//
//   \TZ replaced with the timezone string. Three characters are used
//       to allow for space in the buffer.
//
//  all others silently ignored and left unprocessed
//

char * strdupesc (const char * str)
  {
    char * buf = strdup (str);
    if (!buf)
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
    char * p = buf;
    while (* p)
      {
        if (* p != '\\')
          {
            p ++;
            continue;
          }
        if (p [1] == '\\')           //   \\    backslash
          * p = '\\'; //-V1048
        else if (p [1] == 'a')       //   \a    ^A
          * p = '\001';
        else if (p [1] == 'w')       //   \w    backslash
          * p = '\\'; //-V1048
        else if (p [1] == 'n')       //   \n    newline
          * p = '\n';
        else if (p [1] == 't')       //  \t    tab
          * p = '\t';
        else if (p [1] == 'f')       //  \f    formfeed
          * p = '\f';
        else if (p [1] == 'r')       //  \r    carriage return
          * p = '\r';
        else if (p [1] == 'e')       //  \e    ^E; Multics escape char.
          * p = '\005';
        else if (p [1] == '_')       //  \_    space; needed for leading or
                                     //        trailing spaces (scp parser
                                     //        issue)
          * p = ' ';
        else if (p [1] == 'c')       //  \c    comma (scp parser issue)
          * p = ',';
        else if (p [1] == 's')       //  \s    semicolon (scp parser issue)
          * p = ';';
        else if (p [1] == 'd')       //  \d    dollar sign (scp parser issue)
          * p = '$';
        else if (p [1] == 'q')       //  \q    double quote (scp parser issue)
          * p = '"';
        else if (p [1] == 'z')       //  \z    ^D eof (VAXism)
          * p = '\004';
        else if (p [1] == 'k')       //  \k    caret
          * p = '^';
        else if (p [1] == 'x')       //  \x    expect
          * p = '\030';
        else if (p [1] == 'y')       //  \y    expect
          * p = '\031';
        //else if (p [1] == '0')       //  \0    null; used as end of expect string
          //* p = 0;

#if 0
        else if (p [1] == 'T' && p [2] == 'Z')  // \TZ   time zone
          {
            strncpy (p, "pst", 3);
            time_t t = time (NULL);
            struct tm * lt = localtime (& t);
            if (strlen (lt -> tm_zone) == 3)
              {
                //strncpy (p, lt -> tm_zone, 3);
                p [0] = tolower (lt -> tm_zone [0]);
                p [1] = tolower (lt -> tm_zone [1]);
                p [2] = tolower (lt -> tm_zone [2]);
              }
            p += 2;
          }
#endif
        else
          {
            p ++;
            continue;
          }
        p ++;
        memmove (p, p + 1, strlen (p + 1) + 1);
      }
    return buf;
  }

// Layout of data as read from tape file format
//
//   bits: buffer of bits from tape. The data is
//   packed as 2 36 bit words in 9 eight bit bytes
//     (2 * 36 == 7 * 9)
//   The of the bytes in bits is
//      byte     value
//       0       most significant byte in word 0
//       1       2nd msb in word 0
//       2       3rd msb in word 0
//       3       4th msb in word 0
//       4       upper half is 4 least significant bits in word 0
//               lower half is 4 most significant bit in word 1
//       5       5th to 13th most significant bits in word 1
//       6       ...
//       7       ...
//       8       least significant byte in word 1
//

// Multics humor: this is idiotic

// Data conversion routines
//
//  'bits' is the packed bit stream read from the tape
//    it is assumed to start at an even word36 address
//
//   extr36
//     extract the word36 at woffset
//

static word36 extrASCII36 (uint8 * bits, uint woffset)
  {
    uint8 * p = bits + woffset * 4;

    uint64 w;
    w  = ((uint64) p [0]) << 27;
    w |= ((uint64) p [1]) << 18;
    w |= ((uint64) p [2]) << 9;
    w |= ((uint64) p [3]);
    // mask shouldn't be necessary but is robust
    return (word36) (w & MASK36);
  }

// Data conversion routines
//
//  'bits' is the packed bit stream read from the tape
//    it is assumed to start at an even word36 address
//
//   extr36
//     extract the word36 at woffset
//

word36 extr36 (uint8 * bits, uint woffset)
  {
    uint isOdd = woffset % 2;
    uint dwoffset = woffset / 2;
    uint8 * p = bits + dwoffset * 9;

    uint64 w;
    if (isOdd)
      {
        w  = (((uint64) p [4]) & 0xf) << 32;
        w |=  ((uint64) p [5]) << 24;
        w |=  ((uint64) p [6]) << 16;
        w |=  ((uint64) p [7]) << 8;
        w |=  ((uint64) p [8]);
      }
    else
      {
        w  =  ((uint64) p [0]) << 28;
        w |=  ((uint64) p [1]) << 20;
        w |=  ((uint64) p [2]) << 12;
        w |=  ((uint64) p [3]) << 4;
        w |= (((uint64) p [4]) >> 4) & 0xf;
      }
    // mask shouldn't be necessary but is robust
    return (word36) (w & MASK36);
  }

static void putASCII36 (word36 val, uint8 * bits, uint woffset)
  {
    uint8 * p = bits + woffset * 4;
    p [0]  = (val >> 27) & 0xff;
    p [1]  = (val >> 18) & 0xff;
    p [2]  = (val >>  9) & 0xff;
    p [3]  = (val      ) & 0xff;
  }

void put36 (word36 val, uint8 * bits, uint woffset)
  {
    uint isOdd = woffset % 2;
    uint dwoffset = woffset / 2;
    uint8 * p = bits + dwoffset * 9;

    if (isOdd)
      {
        p [4] &=               0xf0;
        p [4] |= (val >> 32) & 0x0f;
        p [5]  = (val >> 24) & 0xff;
        p [6]  = (val >> 16) & 0xff;
        p [7]  = (val >>  8) & 0xff;
        p [8]  = (val >>  0) & 0xff;
        //w  = ((uint64) (p [4] & 0xf)) << 32;
        //w |=  (uint64) (p [5]) << 24;
        //w |=  (uint64) (p [6]) << 16;
        //w |=  (uint64) (p [7]) << 8;
        //w |=  (uint64) (p [8]);
      }
    else
      {
        p [0]  = (val >> 28) & 0xff;
        p [1]  = (val >> 20) & 0xff;
        p [2]  = (val >> 12) & 0xff;
        p [3]  = (val >>  4) & 0xff;
        p [4] &=               0x0f;
        p [4] |= (val <<  4) & 0xf0;
        //w  =  (uint64) (p [0]) << 28;
        //w |=  (uint64) (p [1]) << 20;
        //w |=  (uint64) (p [2]) << 12;
        //w |=  (uint64) (p [3]) << 4;
        //w |= ((uint64) (p [4]) >> 4) & 0xf;
      }
    // mask shouldn't be necessary but is robust
  }

int extractASCII36FromBuffer (uint8 * bufp, t_mtrlnt tbc, uint * words_processed, word36 *wordp)
  {
    uint wp = * words_processed; // How many words have been processed

    // 1 dps8m word == 4 bytes

    uint bytes_processed = wp * 4;
    if (bytes_processed >= tbc)
      return 1;
    //sim_printf ("store 0%08lo@0%012"PRIo64"\r\n", wordp - M, extr36 (bufp, wp));

    * wordp = extrASCII36 (bufp, wp);
    //if (* wordp & ~MASK36) sim_printf (">>>>>>> extr %012"PRIo64"\r\n", * wordp);
    //sim_printf ("* %06lo = %012"PRIo64"\r\n", wordp - M, * wordp);
    (* words_processed) ++;

    return 0;
  }

int extractWord36FromBuffer (uint8 * bufp, t_mtrlnt tbc, uint * words_processed, word36 *wordp)
  {
    uint wp = * words_processed; // How many words have been processed

    // 2 dps8m words == 9 bytes

    uint bytes_processed = (wp * 9 + 1) / 2;
    if (bytes_processed >= tbc)
      return 1;
    //sim_printf ("store 0%08lo@0%012"PRIo64"\r\n", wordp - M, extr36 (bufp, wp));

    * wordp = extr36 (bufp, wp);
    //if (* wordp & ~MASK36) sim_printf (">>>>>>> extr %012"PRIo64"\r\n", * wordp);
    //sim_printf ("* %06lo = %012"PRIo64"\r\n", wordp - M, * wordp);
    (* words_processed) ++;

    return 0;
  }

int insertASCII36toBuffer (uint8 * bufp, t_mtrlnt tbc, uint * words_processed, word36 word)
  {
    uint wp = * words_processed; // How many words have been processed

    // 1 dps8m word == 4 bytes

    uint bytes_processed = wp * 4;
    if (bytes_processed >= tbc)
      return 1;
    //sim_printf ("store 0%08lo@0%012"PRIo64"\r\n", wordp - M, extr36 (bufp, wp));

    putASCII36 (word, bufp, wp);
    //sim_printf ("* %06lo = %012"PRIo64"\r\n", wordp - M, * wordp);
    (* words_processed) ++;

    return 0;
  }

int insertWord36toBuffer (uint8 * bufp, t_mtrlnt tbc, uint * words_processed, word36 word)
  {
    uint wp = * words_processed; // How many words have been processed

    // 2 dps8m words == 9 bytes

    uint bytes_processed = (wp * 9 + 1) / 2;
    if (bytes_processed >= tbc)
      return 1;
    //sim_printf ("store 0%08lo@0%012"PRIo64"\r\n", wordp - M, extr36 (bufp, wp));

    put36 (word, bufp, wp);
    //sim_printf ("* %06lo = %012"PRIo64"\r\n", wordp - M, * wordp);
    (* words_processed) ++;

    return 0;
  }

#if !defined(NEED_128)
static void print_uint128o_r (uint128 n, char * p)
  {
    if (n == 0)
      return;

    print_uint128o_r(n / 8, p);
    if (p)
      {
        char s [2];
        s [0] = n % 8 + '0';
        s [1] = '\0';
        strcat (p, s);
      }
    else
      sim_printf("%c", (int) (n%8+0x30));
  }

char * print_int128o (int128 n, char * p)
  {
    if (n == 0)
      {
        if (p)
          strcat (p, "0");
        else
          sim_printf ("0");
        return p;
      }
    print_uint128o_r ((uint128) n, p);
    return p;
  }

static void print_uint128_r (uint128 n, char * p)
  {
    if (n == 0)
      return;

    print_uint128_r(n / 10, p);
    if (p)
      {
        char s [2];
        s [0] = n % 10 + '0';
        s [1] = '\0';
        strcat (p, s);
      }
    else
      sim_printf("%c", (int) (n%10+0x30));
  }

void print_int128 (int128 n, char * p)
  {
    if (n == 0)
      {
        if (p)
          strcat (p, "0");
        else
          sim_printf ("0");
        return;
      }
    if (n < 0)
      {
        if (p)
          strcat (p, "-");
        else
          sim_printf ("-");
        n = -n;
      }
    print_uint128_r ((uint128) n, p);
  }
#endif

void timespec_diff(struct timespec * start, struct timespec * stop,
                   struct timespec * result)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000L;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}

#if 0
// Calculate current TR value

void currentTR (word27 * trunits, bool * ovf)
  {
    struct timespec now, delta;
    clock_gettime (CLOCK_BOOTTIME, & now);
    timespec_diff (& cpu.rTRTime, & now, & delta);
    if (delta.tv_sec > 263)
      {
        // The elapsed time is manifestly larger than the TR range
        * trunits = (~0llu) & MASK27;
        * ovf = true;
        return;
      }
    // difference in nSecs
    unsigned long dns = (unsigned long) delta.tv_sec * 1000000000 +
                        (unsigned long) delta.tv_nsec;
    // in Timer ticks
    unsigned long ticks = dns / 1953 /* 1953.125 */;

    // Runout?
    if (ticks >= cpu.rTR)
      {
        * trunits = (~0llu) & MASK27;
        * ovf = true;
        return;
      }
    * trunits = (cpu.rTR - ticks) & MASK27;
    //sim_printf ("time left %f\r\n", (float) (* trunits) / 5120000);
    * ovf = false;
  }
#endif
