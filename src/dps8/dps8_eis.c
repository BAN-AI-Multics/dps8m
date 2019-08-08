/*
 Copyright (c) 2007-2013 Michael Mondy
 Copyright 2012-2016 by Harry Reed
 Copyright 2013-2016 by Charles Anthony
 Copyright 2015-2016 by Craig Ruff
 Copyright 2016 by Michal Tomek

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

/**
 * file dps8_eis.c
 * project dps8
 * date 12/31/12
 * copyright Copyright (c) 2012 Harry Reed. All rights reserved.
 * brief EIS support code...
*/

#ifdef EIS_PTR
// Cached operand data...
//  Alphanumeric Operand
//    Address 18 bits  --> DkW
//    CN 3 bits --> DkB
//    TA 2 bits
//    Length 12 bits
//  Numeric Operand
//    Address 18 bits  --> DkW
//    CN 3 bits --> DkB
//    TN 1 bits
//    S  2 bits
//    SF 6 bits
//    Length 6 bits
//  Bit-string Operand
//    Address 18 bits  --> DkW
//    C  2 bits --> DkB
//    B  4 bits --> DkB
//    Length 12 bits
#endif

#include <ctype.h>

#include "dps8.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_iefp.h"
#include "dps8_decimal.h"
#include "dps8_ins.h"
#include "dps8_eis.h"
#include "dps8_utils.h"

#define DBG_CTR cpu.cycleCnt

//  Restart status
//
//  a6bd   n/a
//  a4bd  n/a
//  a9bd  n/a
//  abd  n/a
//  awd  n/a
//  s4bd  n/a
//  s6bd  n/a
//  s9bd  n/a
//  sbd  n/a
//  swd  n/a
//  cmpc   done
//  scd   done
//  scdr   done
//  scm   done
//  scmr   done
//  tct   done
//  tctr   done
//  mlr   done
//  mrl   done
//  mve
//  mvne
//  mvt   done
//  cmpn
//  mvn
//  csl done
//  csr done
//  cmpb
//  btd
//  dtb
//  ad2d
//  ad3d
//  sb2d
//  sb3d
//  mp2d
//  mp3d
//  dv2d
//  dv3d

// Local optimization
#define ABD_BITS

// Enable EIS operand setup refactoring code -- crashes Multics late in boot.
//#define EIS_SETUP

//  EISWriteCache  -- flush the cache
//
//
//  EISWriteIdx (p, n, data, flush); -- write to cache at p->addr [n]; 
//  EISRead (p) -- read to cache from p->addr
//  EISReadIdx (p, n)  -- read to cache from p->addr[n]; 
//  EISReadN (p, n, dst) -- read N words to dst; 
 
//  EISget469 (k, i)
//  EISput469 (k, i, c)

//  EISget49 (k, *pos, tn) get p->addr[*pos++]

// AL39, Figure 2-3
static word4 get4 (word36 w, int pos)
  {
    switch (pos)
      {
        case 0:
          return getbits36_4 (w, 1);

        case 1:
          return getbits36_4 (w, 5);

        case 2:
          return getbits36_4 (w, 10);

        case 3:
          return getbits36_4 (w, 14);

        case 4:
          return getbits36_4 (w, 19);

        case 5:
          return getbits36_4 (w, 23);

        case 6:
          return getbits36_4 (w, 28);

        case 7:
          return getbits36_4 (w, 32);

      }
    sim_printf ("get4(): How'd we get here?\n");
    return 0;
}

// AL39, Figure 2-4
static word4 get6 (word36 w, int pos)
  {
    switch (pos)
      {
        case 0:
         return getbits36_6 (w, 0);

        case 1:
         return getbits36_6 (w, 6);

        case 2:
         return getbits36_6 (w, 12);

        case 3:
         return getbits36_6 (w, 18);

        case 4:
         return getbits36_6 (w, 24);

        case 5:
         return getbits36_6 (w, 30);

      }
    sim_printf ("get6(): How'd we get here?\n");
    return 0;
  }

// AL39, Figure 2-5
static word9 get9(word36 w, int pos)
  {
    
    switch (pos)
      {
        case 0:
         return getbits36_9 (w, 0);

        case 1:
         return getbits36_9 (w, 9);

        case 2:
         return getbits36_9 (w, 18);

        case 3:
         return getbits36_9 (w, 27);

      }
    sim_printf ("get9(): How'd we get here?\n");
    return 0;
  }

// AL39, Figure 2-3
static word36 put4 (word36 w, int pos, word4 c)
  {

// AL-39 pg 13: "The 0 bits at it positions 0, 9, 18, and 27 are forced to be 0
// by the processor on data transfers to main memory ..."
//
// The code uses 5 bit sets for the even bytes to force the 0 writes.

    c &= MASK4;
    switch (pos)
      {
        case 0:
          //return setbits36_4 (w, 1, c);
          return setbits36_5 (w, 0, c);

        case 1:
          return setbits36_4 (w, 5, c);

        case 2:
          //return setbits36_4 (w, 10, c);
          return setbits36_5 (w, 9, c);

        case 3:
          return setbits36_4 (w, 14, c);

        case 4:
          //return setbits36_4 (w, 19, c);
          return setbits36_5 (w, 18, c);

        case 5:
          return setbits36_4 (w, 23, c);

        case 6:
          //return setbits36_4 (w, 28, c);
          return setbits36_5 (w, 27, c);

        case 7:
          return setbits36_4 (w, 32, c);
      }
    sim_printf ("put4(): How'd we get here?\n");
    return 0;
  }

// AL39, Figure 2-4
static word36 put6 (word36 w, int pos, word6 c)
  {
    switch (pos)
      {
        case 0:
          //return bitfieldInsert36 (w, c, 30, 6);
          return setbits36_6 (w, 0, c);

        case 1:
          //`return bitfieldInsert36 (w, c, 24, 6);
          return setbits36_6 (w, 6, c);

        case 2:
          //return bitfieldInsert36 (w, c, 18, 6);
          return setbits36_6 (w, 12, c);

        case 3:
          //return bitfieldInsert36 (w, c, 12, 6);
          return setbits36_6 (w, 18, c);

        case 4:
          //return bitfieldInsert36 (w, c, 6, 6);
          return setbits36_6 (w, 24, c);

        case 5:
          //return bitfieldInsert36 (w, c, 0, 6);
          return setbits36_6 (w, 30, c);

      }
    sim_printf ("put6(): How'd we get here?\n");
    return 0;
  }

// AL39, Figure 2-5
static word36 put9 (word36 w, int pos, word9 c)
  {
    
    switch (pos)
      {
        case 0:
          //return bitfieldInsert36 (w, c, 27, 9);
          return setbits36_9 (w, 0, c);

        case 1:
          //return bitfieldInsert36 (w, c, 18, 9);
          return setbits36_9 (w, 9, c);

        case 2:
          //return bitfieldInsert36 (w, c, 9, 9);
          return setbits36_9 (w, 18, c);

        case 3:
          //return bitfieldInsert36 (w, c, 0, 9);
          return setbits36_9 (w, 27, c);

      }
    sim_printf ("put9(): How'd we get here?\n");
    return 0;
  }

/**
 * get register value indicated by reg for Address Register operations
 * (not for use with address modifications)
 */

static word36 getCrAR (word4 reg)
  {
    if (reg == 0)
      return 0;
    
    if (reg & 010) /* Xn */
      return cpu.rX [X (reg)];
    
    switch (reg)
      {
        case TD_N:
          return 0;

        case TD_AU: // C(A)0,17
          return GETHI (cpu.rA);

        case TD_QU: //  C(Q)0,17
          return GETHI (cpu.rQ);

        case TD_IC: // C(PPR.IC)
          return cpu.PPR.IC;

        case TD_AL: // C(A)18,35
          return cpu.rA; // See AL36, Table 4-1

        case TD_QL: // C(Q)18,35
          return cpu.rQ; // See AL36, Table 4-1
      }
    return 0;
  }

// getMFReg
//  RType reflects the AL-39 R-type and C(op. desc.)32,35 columns
//
//  Table 4-1. R-type Modifiers for REG Fields
//  
//                   Meaning as used in:
//
//  Octal  R-type  MF.REG   Indirect operand    C(operand descriptor)32,35
//  Code                    decriptor-pointer
//  00         n       n          n                      IPR
//  01        au      au          au                      au
//  02        qu      qu          qu                      qu
//  03        du     IPR         IPR                      du (a)
//  04        ic      ic          ic                      ic (b)
//  05        al       a (c)      al                       a (c)
//  06        ql       q (c)      ql                       a (c)
//  07        dl     IPR         IPR                     IPR
//  1n        xn      xn          xn                      xn
//

static word18 getMFReg18 (uint n, bool allowDU, bool allowNIC, fault_ipr_subtype_ *mod_fault)
  {
    switch (n)
      {
        case 0: // n
          if (! allowNIC)
            {
              //sim_printf ("getMFReg18 n\n");
              *mod_fault |= FR_ILL_MOD;
              //doFault (FAULT_IPR, fst_ill_mod, "getMFReg18 n");
            }
          return 0;

        case 1: // au
          return GETHI (cpu.rA);

        case 2: // qu
          return GETHI (cpu.rQ);

        case 3: // du
          // du is a special case for SCD, SCDR, SCM, and SCMR
// XXX needs attention; doesn't work with old code; triggered by
// XXX parseOperandDescriptor;
          if (! allowDU)
            {
#if 0 // fixes first fail
sim_printf ("getMFReg18 %012"PRIo64"\n", IWB_IRODD);
              if (cpu.currentInstruction.opcode == 0305 && // dtb
                  cpu.currentInstruction.opcodeX == 1)
                {
                  sim_printf ("dtb special case 2\n");
                  doFault (FAULT_IPR,
                    (_fault_subtype) (((_fault_subtype) {.fault_ipr_subtype=FR_ILL_MOD}).bits |
                                      ((_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC}).bits),
                    "getMFReg18 du");
                }
#endif
              //sim_printf ("getMFReg18 du\n");
              *mod_fault |= FR_ILL_MOD;
              //doFault (FAULT_IPR, fst_ill_mod, "getMFReg18 du");
            }
          return 0;

        case 4: // ic - The ic modifier is permitted in 
                // C (od)32,35 only if MFk.RL = 0, that is, if the contents of 
                // the register is an address offset, not the designation of 
                // a register containing the operand length.
                // Note that AL39 is wrong saying "is permitted in MFk.REG and C(od)32,35". Only C(od)32,35 is correct. cf RJ78
          if (! allowNIC)
            {
              //sim_printf ("getMFReg18 n\n");
              *mod_fault |= FR_ILL_MOD;
              //doFault (FAULT_IPR, fst_ill_mod, "getMFReg18 ic");
            }
          return cpu.PPR.IC;

        case 5: // al / a
          return GETLO (cpu.rA);

        case 6: // ql / a
          return GETLO (cpu.rQ);

        case 7: // dl
          *mod_fault |= FR_ILL_MOD;
          //doFault (FAULT_IPR, fst_ill_mod, "getMFReg18 dl");
          return 0;

        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
          return cpu.rX [n - 8];
      }
    sim_printf ("getMFReg18(): How'd we get here? n=%d\n", n);
    return 0;
  }

static word36 getMFReg36 (uint n, bool allowDU, bool allowNIC, fault_ipr_subtype_ *mod_fault)
  {
    switch (n)
      {
        case 0: // n
         if (! allowNIC)
           {
             //sim_printf ("getMFReg36 n\n");
             *mod_fault |= FR_ILL_MOD;
             //doFault (FAULT_IPR, fst_ill_mod, "getMFReg36 n");
           }
          return 0;
        case 1: // au
          return GETHI (cpu.rA);

        case 2: // qu
          return GETHI (cpu.rQ);

        case 3: // du
          // du is a special case for SCD, SCDR, SCM, and SCMR
          if (! allowDU)
           *mod_fault |= FR_ILL_MOD;
           //doFault (FAULT_IPR, fst_ill_mod, "getMFReg36 du");
          return 0;

        case 4: // ic - The ic modifier is permitted in MFk.REG and 
                // C (od)32,35 only if MFk.RL = 0, that is, if the contents of 
                // the register is an address offset, not the designation of 
                // a register containing the operand length.
                // Note that AL39 is wrong saying "is permitted in MFk.REG and C(od)32,35". Only C(od)32,35 is correct. cf RJ78
          if (! allowNIC)
            {
              //sim_printf ("getMFReg36 n\n");
              *mod_fault |= FR_ILL_MOD;
              //doFault (FAULT_IPR, fst_ill_mod, "getMFReg36 ic");
            }
          return cpu.PPR.IC;

        case 5: // al / a
          return cpu.rA;

        case 6: // ql / a
            return cpu.rQ;

        case 7: // dl
             *mod_fault |= FR_ILL_MOD;
             //doFault (FAULT_IPR, fst_ill_mod, "getMFReg36 dl");
             return 0;

        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            return cpu.rX [n - 8];
      }
    sim_printf ("getMFReg36(): How'd we get here? n=%d\n", n);
    return 0;
  }

#define EISADDR_IDX(p) ((p) - cpu.currentEISinstruction.addr)

static void EISWriteCache (EISaddr * p)
  {
    sim_debug (DBG_TRACEEXT, & cpu_dev, "EISWriteCache addr %06o\n", p->cachedAddr);
    word3 saveTRR = cpu.TPR.TRR;

    if (p -> cacheValid && p -> cacheDirty)
      {
        if (p -> mat == viaPR)
          {
            cpu.TPR.TRR = p -> RNR;
            cpu.TPR.TSR = p -> SNR;
	    cpu.cu.XSF = 0;
            if_sim_debug (DBG_TRACEEXT, & cpu_dev)
              {
                for (uint i = 0; i < 8; i ++)
#ifdef CWO
                  if (p->wordDirty[i])
                    {
#endif
                  sim_debug (DBG_TRACEEXT, & cpu_dev, 
                             "%s: writeCache (PR) %012"PRIo64"@%o:%06o\n", 
                             __func__, p -> cachedParagraph [i], p -> SNR, p -> cachedAddr + i);
#ifdef CWO
                   }
#endif
              }
{ long eisaddr_idx = EISADDR_IDX (p);
sim_debug (DBG_TRACEEXT, & cpu_dev, "EIS %ld Write8 TRR %o TSR %05o\n", eisaddr_idx, cpu.TPR.TRR, cpu.TPR.TSR); }
#ifdef CWO
            for (uint i = 0; i < 8; i ++)
              if (p->wordDirty[i])
                {
                  Write1 (p->cachedAddr+i, p -> cachedParagraph[i], true);
                  p->wordDirty[i] = false;
                }
#else
            Write8 (p->cachedAddr, p -> cachedParagraph, true);
#endif
          }
        else
          {
            //if (get_addr_mode() == APPEND_mode)
              //{
            cpu.TPR.TRR = cpu.PPR.PRR;
            cpu.TPR.TSR = cpu.PPR.PSR;
	    cpu.cu.XSF = 0;
              //}
        
            if_sim_debug (DBG_TRACEEXT, & cpu_dev)
              {
                for (uint i = 0; i < 8; i ++)
#ifdef CWO
                  if (p->wordDirty[i])
                    {
#endif
                  sim_debug (DBG_TRACEEXT, & cpu_dev, 
                             "%s: writeCache %012"PRIo64"@%o:%06o\n", 
                             __func__, p -> cachedParagraph [i], cpu.TPR.TSR, p -> cachedAddr + i);
#ifdef CWO
                     }
#endif
              }
{ long eisaddr_idx = EISADDR_IDX (p);
sim_debug (DBG_TRACEEXT, & cpu_dev, "EIS %ld Write8 NO PR TRR %o TSR %05o\n", eisaddr_idx, cpu.TPR.TRR, cpu.TPR.TSR); }
#ifdef CWO
            for (uint i = 0; i < 8; i ++)
              if (p->wordDirty[i])
                {
                  Write1 (p->cachedAddr+i, p -> cachedParagraph[i], false);
                  p->wordDirty[i] = false;
                }
#else
            Write8 (p->cachedAddr, p -> cachedParagraph, false);
#endif
          }
      }
    p -> cacheDirty = false;
    cpu.TPR.TRR = saveTRR;
  }

static void EISReadCache (EISaddr * p, word18 address)
  {
    sim_debug (DBG_TRACEEXT, & cpu_dev, "EISReadCache addr %06o\n", address);
    word3 saveTRR = cpu.TPR.TRR;

    address &= AMASK;

    word18 paragraphAddress = address & paragraphMask;
    //word3 paragraphOffset = address & paragraphOffsetMask;

    if (p -> cacheValid && p -> cachedAddr == paragraphAddress)
      {
        return;
      }

    if (p -> cacheValid && p -> cacheDirty && p -> cachedAddr != paragraphAddress)
      {
        EISWriteCache (p);
      }

    if (p -> mat == viaPR)
      {
        cpu.TPR.TRR = p -> RNR;
        cpu.TPR.TSR = p -> SNR;
	cpu.cu.XSF = 0;
{ long eisaddr_idx = EISADDR_IDX (p);
sim_debug (DBG_TRACEEXT, & cpu_dev, "EIS %ld Read8 TRR %o TSR %05o\n", eisaddr_idx, cpu.TPR.TRR, cpu.TPR.TSR); }
        Read8 (paragraphAddress, p -> cachedParagraph, true);

        if_sim_debug (DBG_TRACEEXT, & cpu_dev)
          {
            for (uint i = 0; i < 8; i ++)
              sim_debug (DBG_TRACEEXT, & cpu_dev, 
                         "%s: readCache (PR) %012"PRIo64"@%o:%06o\n", 
                           __func__, p -> cachedParagraph [i], p -> SNR, paragraphAddress + i);
          }
      }
    else
      {
        //if (get_addr_mode() == APPEND_mode)
          //{
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.TPR.TSR = cpu.PPR.PSR;
	cpu.cu.XSF = 0;
          //}
        
{ long eisaddr_idx = EISADDR_IDX (p);
sim_debug (DBG_TRACEEXT, & cpu_dev, "EIS %ld Read8 NO PR TRR %o TSR %05o\n", eisaddr_idx, cpu.TPR.TRR, cpu.TPR.TSR); }
        Read8 (paragraphAddress, p -> cachedParagraph, false);
        if_sim_debug (DBG_TRACEEXT, & cpu_dev)
          {
            for (uint i = 0; i < 8; i ++)
              sim_debug (DBG_TRACEEXT, & cpu_dev, 
                         "%s: readCache %012"PRIo64"@%o:%06o\n", 
                         __func__, p -> cachedParagraph [i], cpu.TPR.TSR, paragraphAddress + i);
          }
      }
    p -> cacheValid = true;
    p -> cacheDirty = false;
#ifdef CWO
    for (uint i = 0; i < 8; i ++)
      p->wordDirty[i] = false;
#endif
    p -> cachedAddr = paragraphAddress;
    cpu.TPR.TRR = saveTRR;
  }

static void EISWriteIdx (EISaddr *p, uint n, word36 data, bool flush)
{
#ifdef EIS_PTR
    long eisaddr_idx = EISADDR_IDX (p);
if (eisaddr_idx < 0 || eisaddr_idx > 2) { sim_warn ("IDX1"); return }
    sim_debug (DBG_TRACEEXT, & cpu_dev, "EISWriteIdx addr %06o n %u\n", cpu.du.Dk_PTR_W[eisaddr_idx], n);
    word18 addressN = (cpu.du.Dk_PTR_W[eisaddr_idx] + n) & AMASK;
#else
    sim_debug (DBG_TRACEEXT, & cpu_dev, "EISWriteIdx addr %06o n %u\n", p->address, n);
    word18 addressN = p -> address + n;
#endif
    addressN &= AMASK;

    word18 paragraphAddress = addressN & paragraphMask;
    word3 paragraphOffset = addressN & paragraphOffsetMask;

    if (p -> cacheValid && p -> cacheDirty && p -> cachedAddr != paragraphAddress)
      {
        EISWriteCache (p);
      }
    if ((! p -> cacheValid) || p -> cachedAddr != paragraphAddress)
      {
        EISReadCache (p, paragraphAddress);
      }
    p -> cacheDirty = true;
#ifdef CWO
    p -> wordDirty[paragraphOffset] = true;
#endif
    p -> cachedParagraph [paragraphOffset] = data;
    p -> cachedAddr = paragraphAddress;
// XXX ticket #31
// This a little brute force; it we fault on the next read, the cached value
// is lost. There might be a way to logic it up so that when the next read
// word offset changes, then we write the cache before doing the read. For
// right now, be pessimistic. Sadly, since this is a bit loop, it is very.
    if (flush)
      {
        EISWriteCache (p);
      }
}

static word36 EISReadIdx (EISaddr * p, uint n)
  {
#ifdef EIS_PTR
    long eisaddr_idx = EISADDR_IDX (p);
if (eisaddr_idx < 0 || eisaddr_idx > 2) { sim_warn ("IDX1"); return }

    sim_debug (DBG_TRACEEXT, & cpu_dev, "EISReadIdx addr %06o n %u\n", cpu.du.Dk_PTR_W[eisaddr_idx], n);
    word18 addressN = (cpu.du.Dk_PTR_W[eisaddr_idx] + n) & AMASK;
#else
    long eisaddr_idx = EISADDR_IDX (p);
    sim_debug (DBG_TRACEEXT, & cpu_dev, "EISReadIdx %ld addr %06o n %u\n", eisaddr_idx, p->address, n);
    word18 addressN = p -> address + n;
#endif
    addressN &= AMASK;

    word18 paragraphAddress = addressN & paragraphMask;
    word3 paragraphOffset = addressN & paragraphOffsetMask;

    if (p -> cacheValid && p -> cachedAddr == paragraphAddress)
      {
        return p -> cachedParagraph [paragraphOffset];
      }
    if (p -> cacheValid && p -> cacheDirty)
      {
        EISWriteCache (p);
      }
    EISReadCache (p, paragraphAddress);
    return p -> cachedParagraph [paragraphOffset];
  }

static word36 EISRead (EISaddr * p)
  {
#ifdef EIS_PTR
    long eisaddr_idx = EISADDR_IDX (p);
if (eisaddr_idx < 0 || eisaddr_idx > 2) { sim_warn ("IDX1"); return }

    sim_debug (DBG_TRACEEXT, & cpu_dev, "EISRead addr %06o\n", cpu.du.Dk_PTR_W[eisaddr_idx]);
#else
    sim_debug (DBG_TRACEEXT, & cpu_dev, "EISRead addr %06o\n", p->address);
#endif
    return EISReadIdx (p, 0);
  }

#if 0
static void EISReadN (EISaddr * p, uint N, word36 *dst)
  {
#ifdef EIS_PTR
    long eisaddr_idx = EISADDR_IDX (p);
if (eisaddr_idx < 0 || eisaddr_idx > 2) { sim_warn ("IDX1"); return }
    sim_debug (DBG_TRACEEXT, & cpu_dev, "EISReadN addr %06o N %u\n", cpu.du.Dk_PTR_W[eisaddr_idx], N);
#else
    sim_debug (DBG_TRACEEXT, & cpu_dev, "EISReadN addr %06o N %u\n", p->address, N);
#endif
    for (uint n = 0; n < N; n ++)
      {
        * dst ++ = EISReadIdx (p, n);
      }
  }
#endif

static void EISReadPage (EISaddr * p, uint n, word36 * data)
  {
#ifdef EIS_PTR
    long eisaddr_idx = EISADDR_IDX (p);
if (eisaddr_idx < 0 || eisaddr_idx > 2) { sim_warn ("IDX1"); return }
    word18 addressN = (cpu.du.Dk_PTR_W[eisaddr_idx] + n) & AMASK;
#else
    word18 addressN = p -> address + n;
#endif
    addressN &= AMASK;

    sim_debug (DBG_TRACEEXT, & cpu_dev, "%s addr %06o\n", __func__, addressN);
    if ((addressN & PGMK) != 0)
      {
        sim_warn ("EISReadPage not aligned %06o\n", addressN);
        addressN &= (word18) ~PGMK;
      }

    word3 saveTRR = cpu.TPR.TRR;

    if (p -> mat == viaPR)
      {
        cpu.TPR.TRR = p -> RNR;
        cpu.TPR.TSR = p -> SNR;
	cpu.cu.XSF = 0;
        ReadPage (addressN, data, true);

        if_sim_debug (DBG_TRACEEXT, & cpu_dev)
          {
            for (uint i = 0; i < PGSZ; i ++)
#ifdef EIS_PTR
              sim_debug (DBG_TRACEEXT, & cpu_dev, 
                         "%s: (PR) %012"PRIo64"@%o:%06o\n", 
                           __func__, data [i], cpu.TPR.TSR, addressN + i);
#else
              sim_debug (DBG_TRACEEXT, & cpu_dev, 
                         "%s: (PR) %012"PRIo64"@%o:%06o\n", 
                           __func__, data [i], p -> SNR, addressN + i);
#endif
          }
      }
    else
      {
        //if (get_addr_mode() == APPEND_mode)
          //{
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.TPR.TSR = cpu.PPR.PSR;
	cpu.cu.XSF = 0;
          //}
        
        ReadPage (addressN, data, false);
        if_sim_debug (DBG_TRACEEXT, & cpu_dev)
          {
            for (uint i = 0; i < PGSZ; i ++)
              sim_debug (DBG_TRACEEXT, & cpu_dev, 
                         "%s: %012"PRIo64"@%o:%06o\n", 
                         __func__, data [i], cpu.TPR.TSR, addressN + i);
          }
      }
    cpu.TPR.TRR = saveTRR;
  }

static void EISWritePage (EISaddr * p, uint n, word36 * data)
  {
#ifdef EIS_PTR
    long eisaddr_idx = EISADDR_IDX (p);
if (eisaddr_idx < 0 || eisaddr_idx > 2) { sim_warn ("IDX1"); return }
    word18 addressN = (cpu.du.Dk_PTR_W[eisaddr_idx] + n) & AMASK;
#else
    word18 addressN = p -> address + n;
#endif
    addressN &= AMASK;

    sim_debug (DBG_TRACEEXT, & cpu_dev, "%s addr %06o\n", __func__, addressN);
    if ((addressN & PGMK) != 0)
      {
        sim_warn ("EISWritePage not aligned %06o\n", addressN);
        addressN &= (uint) ~PGMK;
      }

    word3 saveTRR = cpu.TPR.TRR;

    if (p -> mat == viaPR)
      {
        cpu.TPR.TRR = p -> RNR;
        cpu.TPR.TSR = p -> SNR;
	cpu.cu.XSF = 0;
        WritePage (addressN, data, true);

        if_sim_debug (DBG_TRACEEXT, & cpu_dev)
          {
            for (uint i = 0; i < PGSZ; i ++)
#ifdef EIS_PTR
              sim_debug (DBG_TRACEEXT, & cpu_dev, 
                         "%s: (PR) %012"PRIo64"@%o:%06o\n", 
                           __func__, data [i], cpu.TPR.TSR, addressN + i);
#else
              sim_debug (DBG_TRACEEXT, & cpu_dev, 
                         "%s: (PR) %012"PRIo64"@%o:%06o\n", 
                           __func__, data [i], p -> SNR, addressN + i);
#endif
          }
      }
    else
      {
        //if (get_addr_mode() == APPEND_mode)
          //{
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.TPR.TSR = cpu.PPR.PSR;
	cpu.cu.XSF = 0;
          //}
        
        WritePage (addressN, data, false);
        if_sim_debug (DBG_TRACEEXT, & cpu_dev)
          {
            for (uint i = 0; i < PGSZ; i ++)
              sim_debug (DBG_TRACEEXT, & cpu_dev, 
                         "%s: %012"PRIo64"@%o:%06o\n", 
                         __func__, data [i], cpu.TPR.TSR, addressN + i);
          }
      }
    cpu.TPR.TRR = saveTRR;
  }

static word9 EISget469 (int k, uint i)
  {
    EISstruct * e = & cpu.currentEISinstruction;
    
    uint nPos = 4; // CTA9
#ifdef EIS_PTR3
    switch (cpu.du.TAk[k-1])
#else
    switch (e -> TA [k - 1])
#endif
      {
        case CTA4:
            nPos = 8;
            break;
            
        case CTA6:
            nPos = 6;
            break;
      }
    
    word18 address = e -> WN [k - 1];
    uint nChars = i + e -> CN [k - 1];

    address += nChars / nPos;
    uint residue = nChars % nPos;

    PNL (cpu.du.Dk_PTR_W[k-1] = address);
#ifdef EIS_PTR
    cpu.du.Dk_PTR_W[k-1] = address;
#else
    e -> addr [k - 1].address = address;
#endif
    word36 data = EISRead (& e -> addr [k - 1]);    // read it from memory

    word9 c = 0;
#ifdef EIS_PTR3
    switch (cpu.du.TAk[k-1])
#else
    switch (e -> TA [k - 1])
#endif
      {
        case CTA4:
          c = (word9) get4 (data, (int) residue);
          break;

        case CTA6:
          c = (word9) get6 (data, (int) residue);
          break;

        case CTA9:
          c = get9 (data, (int) residue);
          break;
      }
#ifdef EIS_PTR3
    sim_debug (DBG_TRACEEXT, & cpu_dev, "EISGet469 : k: %u TAk %u coffset %u c %o \n", k, cpu.du.TAk[k - 1], residue, c);
#else
    sim_debug (DBG_TRACEEXT, & cpu_dev, "EISGet469 : k: %u TAk %u coffset %u c %o \n", k, e -> TA [k - 1], residue, c);
#endif
    
    return c;
  }

static void EISput469 (int k, uint i, word9 c469)
  {
    EISstruct * e = & cpu.currentEISinstruction;

    uint nPos = 4; // CTA9
#ifdef EIS_PTR3
    switch (cpu.du.TAk[k-1])
#else
    switch (e -> TA [k - 1])
#endif
      { 
        case CTA4:
          nPos = 8;
          break;
            
        case CTA6:
          nPos = 6;
          break;
      }

    word18 address = e -> WN [k - 1];
    uint nChars = i + e -> CN [k - 1];

    address += nChars / nPos;
    uint residue = nChars % nPos;

    PNL (cpu.du.Dk_PTR_W[k-1] = address);
#ifdef EIS_PTR
    cpu.du.Dk_PTR_W[k-1] = address;
#else
    e -> addr [k - 1].address = address;
#endif
    word36 data = EISRead (& e -> addr [k - 1]);    // read it from memory

    word36 w = 0;
#ifdef EIS_PTR3
    switch (cpu.du.TAk[k-1])
#else
    switch (e -> TA [k - 1])
#endif
      {
        case CTA4:
          w = put4 (data, (int) residue, (word4) c469);
          break;

        case CTA6:
          w = put6 (data, (int) residue, (word6) c469);
          break;

        case CTA9:
          w = put9 (data, (int) residue, c469);
          break;
      }
    EISWriteIdx (& e -> addr [k - 1], 0, w, true);
  }

/*
 * return a 4- or 9-bit character at memory "*address" and position "*pos". 
 * Increment pos (and address if necesary)
 */

static word9 EISget49 (EISaddr * p, int * pos, int tn)
  {
    int maxPos = tn == CTN4 ? 7 : 3;

    if (* pos > maxPos)        // overflows to next word?
      {   // yep....
        * pos = 0;        // reset to 1st byte
        // bump source to next address
#ifdef EIS_PTR
        long eisaddr_idx = EISADDR_IDX (p);
if (eisaddr_idx < 0 || eisaddr_idx > 2) { sim_warn ("IDX1"); return }
        cpu.du.Dk_PTR_W[eisaddr_idx] = (cpu.du.Dk_PTR_W[eisaddr_idx] + 1) & AMASK;
#else
        p -> address = (p -> address + 1) & AMASK;
#endif
        p -> data = EISRead (p);    // read it from memory
      }
    else
      {
        p -> data = EISRead (p);   // read data word from memory
      }

    word9 c = 0;
    switch (tn)
      {
        case CTN4:
          c = get4 (p -> data, * pos);
          break;
        case CTN9:
          c = get9 (p -> data, * pos);
          break;
      }

    (* pos) ++;
    return c;
  }

static bool EISgetBitRWN (EISaddr * p, bool flush)
  {
#ifdef EIS_PTR
    long eisaddr_idx = EISADDR_IDX (p);
if (eisaddr_idx < 0 || eisaddr_idx > 2) { sim_warn ("IDX1"); return }

#endif
    int baseCharPosn = (p -> cPos * 9);     // 9-bit char bit position
    int baseBitPosn = baseCharPosn + p -> bPos;
    baseBitPosn += (int) cpu.du.CHTALLY;

    int bitPosn = baseBitPosn % 36;
    int woff = baseBitPosn / 36;

#ifdef EIS_PTR
    word18 saveAddr = cpu.du.Dk_PTR_W[eisaddr_idx];
    cpu.du.Dk_PTR_W[eisaddr_idx] += (uint) woff;
    cpu.du.Dk_PTR_W[eisaddr_idx] &= AMASK;
#else
    word18 saveAddr = p -> address;
    p -> address += (uint) woff;
#endif

    p -> data = EISRead (p); // read data word from memory
    
    if (p -> mode == eRWreadBit)
      {
        p -> bit = getbits36_1 (p -> data, (uint) bitPosn);
      } 
    else if (p -> mode == eRWwriteBit)
      {
        p -> data = setbits36_1 (p -> data, (uint) bitPosn, p -> bit);
        
        EISWriteIdx (p, 0, p -> data, flush); // write data word to memory
      }

    p->last_bit_posn = bitPosn;

#ifdef EIS_PTR
    cpu.du.Dk_PTR_W[eisaddr_idx] = saveAddr;
#else
    p -> address = saveAddr;
#endif
    return p -> bit;
  }

static void setupOperandDescriptorCache (int k)
  {
    EISstruct * e = & cpu.currentEISinstruction;
    e -> addr [k - 1]. cacheValid = false;
  }

//
// 5.2.10.5  Operand Descriptor Address Preparation Flowchart
//
// A flowchart of the operations involved in operand descriptor address
// preparation is shown in Figure 5-2. The chart depicts the address
// preparation for operand descriptor 1 of a multiword instruction as described
// by modification field 1 (MF1). A similar type address preparation would be
// carried out for each operand descriptor as specified by its MF code.
//    (Bull Nova 9000 pg 5-40  67 A2 RJ78 REV02)
//
// 1. The multiword instruction is obtained from memory.
//
// 2. The indirect (ID) bit of MF1 is queried to determine if the descriptor
// for operand 1 is present or is an indirect word.
//
// 3. This step is reached only if an indirect word was in the operand
// descriptor location. Address modification for the indirect word is now
// performed. If the AR bit of the indirect word is 1, address register
// modification step 4 is performed.
//
// 4. The y field of the indirect word is added to the contents of the
// specified address register.
//
// 5. A check is now made to determine if the REG field of the indirect word
// specifies that a register type modification be performed.
//
// 6. The indirect address as modified by the address register is now modified
// by the contents of the specified register, producing the effective address
// of the operand descriptor.
//
// 7. The operand descriptor is obtained from the location determined by the
// generated effective address in item 6.
//

static void setupOperandDescriptor (int k, fault_ipr_subtype_ *mod_fault)
  {
    EISstruct * e = & cpu.currentEISinstruction;
    switch (k)
      {
        case 1:
          PNL (L68_ (DU_CYCLE_FA_I1;))
          PNL (L68_ (DU_CYCLE_GDLDA;))
          e -> MF1 = getbits36_7 (cpu.cu.IWB, 29);
          break;
        case 2:
          PNL (L68_ (DU_CYCLE_FA_I2;))
          PNL (L68_ (DU_CYCLE_GDLDB;))
          e -> MF2 = getbits36_7 (cpu.cu.IWB, 11);
          break;
        case 3:
          PNL (L68_ (DU_CYCLE_FA_I3;))
          PNL (L68_ (DU_CYCLE_GDLDC;))
          e -> MF3 = getbits36_7 (cpu.cu.IWB,  2);
          break;
      }
    
    word18 MFk = e -> MF [k - 1];
    
    if (MFk & MFkID)
    {
        PNL (L68_ (if (k == 1)
                     DU_CYCLE_LDWRT1;
                   if (k == 2)
                     DU_CYCLE_LDWRT2;))

        word36 opDesc = e -> op [k - 1];
        
// 18-28 MBZ check breaks Multics; line 161 of sys_trouble.alm contains
//
// 000103 aa 040100 1006 20    160   mlr     (id),(pr),fill(040) copy error message
// 000104 0a 000126 0002 05    161   arg     trouble_messages-1,al
// 000105 aa 300063 200063     162   desc9a  bb|fgbx.message+3(1),64-13
//
// bit 28 of 000104 is set
//

#if 0
        // Bits 18-28,30, 31 MBZ
        if (opDesc & 0000000777660)
          {
#if 0 // fix 2nd fail
sim_printf ("setupOperandDescriptor %012"PRIo64"\n", opDesc);
sim_printf ("setupOperandDescriptor %012"PRIo64"\n", IWB_IRODD);
            if (cpu.currentInstruction.opcode == 0305 && // dtb
                cpu.currentInstruction.opcodeX == 1)
              {
                sim_printf ("dtb special case\n");
                doFault (FAULT_IPR,
                  (_fault_subtype) (((_fault_subtype) {.fault_ipr_subtype=FR_ILL_MOD}).bits |
                                    ((_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC}).bits),
                  "setupOperandDescriptor 18-28,30, 31 MBZ");
              }
#endif
            doFault (FAULT_IPR, fst_ill_mod, "setupOperandDescriptor 18-28,30, 31 MBZ");
          }
#endif 

        // Bits 30, 31 MBZ
        // RJ78 p. 5-39, ISOLTS 840 07a,07b
        if (opDesc & 060)
          {
            *mod_fault |= FR_ILL_MOD;
            //doFault (FAULT_IPR, fst_ill_mod, "setupOperandDescriptor 30,31 MBZ");
          }

        // fill operand according to MFk....
        word18 address = GETHI (opDesc);
        PNL (cpu.du.Dk_PTR_W[k-1] = address);
#ifdef EIS_PTR
        cpu.du.Dk_PTR_W[k-1] = address;
#else
        e -> addr [k - 1].address = address;
#endif
        
        // Indirect descriptor control. If ID = 1 for Mfk, then the kth word
        // following the instruction word is an indirect pointer to the operand
        // descriptor for the kth operand; otherwise, that word is the operand
        // descriptor.
        //
        // If MFk.ID = 1, then the kth word following an EIS multiword
        // instruction word is not an operand descriptor, but is an indirect
        // pointer to an operand descriptor and is interpreted as shown in
        // Figure 4-5.
        
        
        // Mike Mondy michael.mondy@coffeebird.net sez' ...
        // EIS indirect pointers to operand descriptors use PR registers.
        // However, operand descriptors use AR registers according to the
        // description of the AR registers and the description of EIS operand
        // descriptors. However, the description of the MF field
        // claims that operands use PR registers. The AR doesn't have a
        // segment field. Emulation confirms that operand descriptors
        // need to be fetched via segments given in PR registers.

        bool a = opDesc & (1 << 6); 
        if (a)
          {
            // A 3-bit pointer register number (n) and a 15-bit offset relative
            // to C(PRn.WORDNO) if A = 1 (all modes)
            word3 n = (word3) getbits18 (address, 0, 3);
            CPTUR (cptUsePRn + n);
            word15 offset = address & MASK15;  // 15-bit signed number
            address = (cpu.AR [n].WORDNO + SIGNEXT15_18 (offset)) & AMASK;

            PNL (cpu.du.Dk_PTR_W[k-1] = address);
#ifdef EIS_PTR
            cpu.du.Dk_PTR_W[k-1] = address;
#else
            e -> addr [k - 1].address = address;
#endif
            cpu.cu.TSN_PRNO[k-1] = n;
            cpu.cu.TSN_VALID[k-1] = 1;
            e -> addr [k - 1].SNR = cpu.PR [n].SNR;
            e -> addr [k - 1].RNR = max3 (cpu.PR [n].RNR,
                                            cpu.TPR.TRR,
                                            cpu.PPR.PRR);
                
            e -> addr [k - 1].mat = viaPR;   // ARs involved
sim_debug (DBG_TRACEEXT, & cpu_dev, "AR n %u k %u\n", n, k - 1);
          }
        else
          {
            e->addr [k - 1].mat = OperandRead;      // no ARs involved yet
sim_debug (DBG_TRACEEXT, & cpu_dev, "No ARb %u\n", k - 1);
          }

        // Address modifier for ADDRESS. All register modifiers except du and
        // dl may be used. If the ic modifier is used, then ADDRESS is an
        // 18-bit offset relative to value of the instruction counter for the
        // instruction word. C(REG) is always interpreted as a word offset. REG 

        uint reg = opDesc & 017;
        // XXX RH03/RJ78 say a,q modifiers are also available here. AL39 says al/ql only
        address += getMFReg18 (reg, false, true, mod_fault); // ID=1: disallow du, allow n,ic
        address &= AMASK;

        PNL (cpu.du.Dk_PTR_W[k-1] = address);

#ifdef EIS_PTR
        cpu.du.Dk_PTR_W[k-1] = address;
#else
        e -> addr [k - 1].address = address;
#endif
        
        // read EIS operand .. this should be an indirectread
        e -> op [k - 1] = EISRead (& e -> addr [k - 1]); 
    }
    else
    {
          e->addr [k - 1].mat = OperandRead;      // no ARs involved yet
sim_debug (DBG_TRACEEXT, & cpu_dev, "No ARa %u\n", k - 1);
    }
    setupOperandDescriptorCache (k);
}

void setupEISoperands (void)
  {
    PNL (cpu.du.POP = 0);
    PNL (cpu.du.POL = 0);

#ifdef EIS_SETUP
    for (int i = 0; i < 3; i ++)
      {
        if (i < cpu.currentInstruction.info -> ndes)
          setupOperandDescriptor (i + 1);
        else
          setupOperandDescriptorCache (i + 1);
      }
#endif
  }

static void parseAlphanumericOperandDescriptor (uint k, uint useTA, bool allowDU, fault_ipr_subtype_ *mod_fault)
  {
    EISstruct * e = & cpu.currentEISinstruction;
    word18 MFk = e -> MF [k - 1];
    
    PNL (L68_ (if (k == 1)
      DU_CYCLE_ANLD1;
    else if (k == 2)
      DU_CYCLE_ANLD2;
    else if (k == 3)
      DU_CYCLE_ANSTR;))

    PNL (cpu.du.POP = 1);

    word36 opDesc = e -> op [k - 1];
    
    word8 ARn_CHAR = 0;
    word6 ARn_BITNO = 0;
    
    word18 address = GETHI (opDesc);
    
#ifdef EIS_PTR3
    if (useTA != k)
      cpu.du.TAk[k-1] = cpu.du.TAk[useTA-1];
    else
      cpu.du.TAk[k-1] = getbits36_2 (opDesc, 21);    // type alphanumeric
#else
    if (useTA != k)
      e -> TA [k - 1] = e -> TA [useTA - 1];
    else
      e -> TA [k - 1] = getbits36_2 (opDesc, 21);    // type alphanumeric
#endif

#ifdef PANEL
    if (k == 1) // Use data from first operand
      {
        switch (e->TA[0])
          {
            case CTA9:
              cpu.dataMode = 0102; // 9 bit an
              cpu.ou.opsz = is_9 >> 12;
              break;
            case CTA6:
              cpu.dataMode = 0042; // 6 bit an
              cpu.ou.opsz = is_6 >> 12;
              break;
            case CTA4:
              cpu.dataMode = 0022; // 4 bit an
              cpu.ou.opsz = is_4 >> 12;
              break;
          }
      }
#endif


// 8. Modification of the operand descriptor address begins. This step is
// reached directly from 2 if no indirection is involved. The AR bit of MF1 is
// checked to determine if address register modification is specified.
//
// 9. Address register modification is performed on the operand descriptor as
// described under "Address Modification with Address Registers" above. The
// character and bit positions of the specified address register are used in
// one of two ways, depending on the type of operand descriptor, i.e., whether
// the type is a bit string, a numeric, or an alphanumeric descriptor.
//
// 10. The REG field of MF1 is checked for a legal code. If DU is specified in
// the REG field of MF2 in one of the four multiword instructions (SCD, SCDR,
// SCM, or SCMR) for which DU is legal, the CN field is ignored and the
// character or characters are arranged within the 18 bits of the word address
// portion of the operand descriptor.
//
// 11. The count contained in the register specified by the REG field code is
// appropriately converted and added to the operand address.
//
// 12. The operand is retrieved from the calculated effective address location.
//

    if (MFk & MFkAR)
      {
        // if MKf contains ar then it Means Y-charn is not the memory address
        // of the data but is a reference to a pointer register pointing to the
        // data.
        word3 n = (word3) getbits18 (address, 0, 3);
        CPTUR (cptUsePRn + n);
        word18 offset = SIGNEXT15_18 ((word15) address);  // 15-bit signed number
        address = (cpu.AR [n].WORDNO + offset) & AMASK;
        
        ARn_CHAR = GET_AR_CHAR (n); // AR[n].CHAR;
        ARn_BITNO = GET_AR_BITNO (n); // AR[n].BITNO;
        
        cpu.cu.TSN_PRNO[k-1] = n;
        cpu.cu.TSN_VALID[k-1] = 1;
        e -> addr [k - 1].SNR = cpu.PR [n].SNR;
        e -> addr [k - 1].RNR = max3 (cpu.PR [n].RNR, cpu.TPR.TRR, cpu.PPR.PRR);

        e -> addr [k - 1].mat = viaPR;   // ARs involved
sim_debug (DBG_TRACEEXT, & cpu_dev, "AR n %u k %u\n", n, k - 1);
      }

    PNL (cpu.du.POL = 1);

    uint CN = getbits36_3 (opDesc, 18);    // character number

    sim_debug (DBG_TRACEEXT, & cpu_dev, "initial CN%u %u\n", k, CN);
    
    if (MFk & MFkRL)
    {
        uint reg = opDesc & 017;
// XXX Handle N too big intelligently....
        e -> N [k - 1] = (uint) getMFReg36 (reg, false, false, mod_fault); // RL=1: disallow du,n,ic
#ifdef EIS_PTR3
        switch (cpu.du.TAk[k-1])
#else
        switch (e -> TA [k - 1])
#endif
          {
            case CTA4:
              e -> N [k - 1] &= 017777777; // 22-bits of length
              break;

            case CTA6:
            case CTA9:
              e -> N [k - 1] &= 07777777;  // 21-bits of length.
              break;

            default:
#ifdef L68
              doFault (FAULT_IPR, fst_ill_proc, "parseAlphanumericOperandDescriptor TA 3");
#else
              *mod_fault |= FR_ILL_PROC;
#endif
              //sim_printf ("parseAlphanumericOperandDescriptor(ta=%d) How'd we get here 1?\n", e->TA[k-1]);
              break;
          }
      }
    else
      e -> N [k - 1] = opDesc & 07777;
    
    //if (e->N [k - 1] == 0)
      //doFault (FAULT_IPR, FR_ILL_PROC, "parseAlphanumericOperandDescriptor N 0");

    sim_debug (DBG_TRACEEXT, & cpu_dev, "N%u %o\n", k, e->N[k-1]);

    word36 r = getMFReg36 (MFk & 017, allowDU, true, mod_fault); // allow du based on instruction, allow n,ic
    
    if ((MFk & 017) == 4)   // reg == IC ?
      {
        address += r;
        address &= AMASK;
        r = 0;
      }

    // If seems that the effect address calcs given in AL39 p.6-27 are not 
    // quite right. E.g. For CTA4/CTN4 because of the 4 "slop" bits you need 
    // to do 32-bit calcs not 36-bit!
    uint effBITNO = 0;
    uint effCHAR = 0;
    uint effWORDNO = 0;

#ifdef EIS_PTR3
    switch (cpu.du.TAk[k-1])
#else
    switch (e -> TA [k - 1])
#endif
      {
        case CTA4:
          {
            // Calculate character number of ARn CHAR and BITNO
            uint bitoffset = ARn_CHAR * 9u + ARn_BITNO;
            uint arn_char4 = bitoffset * 2 / 9; // / 4.5
            // 8 chars per word plus the number of chars in r, plus the 
            // number of chars in ARn CHAR/BITNO plus the CN from the operand
// XXX Handle 'r' too big intelligently...
            uint nchars = address * 8 + (uint) r + arn_char4 + CN;

            effWORDNO = nchars / 8; // 8 chars/word
            effCHAR = nchars % 8; // effCHAR is the 4 bit char number, not 
                                  // the 9-bit char no
            effBITNO = (nchars & 1) ? 5 : 0;

            effWORDNO &= AMASK;
            
            e -> CN [k - 1] = effCHAR;
            e -> WN [k - 1] = effWORDNO;

            sim_debug (DBG_TRACEEXT, & cpu_dev, "CN%d set to %d by CTA4\n",
                       k, e -> CN [k - 1]);
          }
          break;

        case CTA6:
          if (CN >= 6)
#ifdef L68
            doFault (FAULT_IPR, fst_ill_proc, "parseAlphanumericOperandDescriptor TAn CTA6 CN >= 6");
#else
            *mod_fault |= FR_ILL_PROC;
#endif
          effBITNO = (9u * ARn_CHAR + 6u * r + ARn_BITNO) % 9u;
          effCHAR = ((6u * CN +
                      9u * ARn_CHAR +
                      6u * r + ARn_BITNO) % 36u) / 6u;//9;
          effWORDNO = (uint) (address +
                           (6u * CN +
                            9u * ARn_CHAR +
                            6u * r +
                            ARn_BITNO) / 36u);
          effWORDNO &= AMASK;
            
          e -> CN [k - 1] = effCHAR;   // ??????
          e -> WN [k - 1] = effWORDNO;
          sim_debug (DBG_TRACEEXT, & cpu_dev, "CN%d set to %d by CTA6\n",
                     k, e -> CN [k - 1]);
          break;

        case CTA9:
          if (CN & 01)
#ifdef L68
            doFault(FAULT_IPR, fst_ill_proc, "parseAlphanumericOperandDescriptor CTA9 & CN odd");
#else
            *mod_fault |= FR_ILL_PROC;
#endif
          CN = (CN >> 1);
            
          effBITNO = 0;
          effCHAR = (CN + ARn_CHAR + r) % 4;
          sim_debug (DBG_TRACEEXT, & cpu_dev, 
                     "effCHAR %d = (CN %d + ARn_CHAR %d + r %"PRId64") %% 4)\n",
                     effCHAR, CN, ARn_CHAR, r);
          effWORDNO = (uint) (address +
                           ((9u * CN +
                             9u * ARn_CHAR +
                             9u * r +
                             ARn_BITNO) / 36u));
          effWORDNO &= AMASK;
            
          e -> CN [k - 1] = effCHAR;   // ??????
          e -> WN [k - 1] = effWORDNO;
          sim_debug (DBG_TRACEEXT, & cpu_dev, "CN%d set to %d by CTA9\n",
                     k, e -> CN [k - 1]);
          break;

        default:
#ifdef L68
          doFault (FAULT_IPR, fst_ill_proc, "parseAlphanumericOperandDescriptor TA1 3");
#else
          *mod_fault |= FR_ILL_PROC;
#endif
          //sim_printf ("parseAlphanumericOperandDescriptor(ta=%d) How'd we get here 2?\n", e->TA[k-1]);
          break;
    }
    
    EISaddr * a = & e -> addr [k - 1];
    PNL (cpu.du.Dk_PTR_W[k-1] = effWORDNO);
#ifdef EIS_PTR
    cpu.du.Dk_PTR_W[k-1] = effWORDNO;
#else
    a -> address = effWORDNO;
#endif
    a -> cPos= (int) effCHAR;
    a -> bPos = (int) effBITNO;
    
#ifndef EIS_PTR3
    // a->_type = eisTA;
    a -> TA = (int) e -> TA [k - 1];
#endif
  }

static void parseArgOperandDescriptor (uint k, fault_ipr_subtype_ *mod_fault)
  {
    PNL (L68_ (if (k == 1)
      DU_CYCLE_NLD1;
    else if (k == 2)
      DU_CYCLE_NLD2;
    else if (k == 3)
      DU_CYCLE_GSTR;))

    EISstruct * e = & cpu.currentEISinstruction;
    word36 opDesc = e -> op [k - 1];
    word18 y = GETHI (opDesc);
    word1 yA = GET_A (opDesc);

    uint yREG = opDesc & 0xf;
    
    word36 r = getMFReg36 (yREG, false, true, mod_fault); // disallow du, allow n,ic
    
    word8 ARn_CHAR = 0;
    word6 ARn_BITNO = 0;

    PNL (cpu.du.POP = 1);

    if (yA)
      {
        // if operand contains A (bit-29 set) then it Means Y-char9n is not
        // the memory address of the data but is a reference to a pointer
        // register pointing to the data.
        word3 n = GET_ARN (opDesc);
        CPTUR (cptUsePRn + n);
        word15 offset = y & MASK15;  // 15-bit signed number
        y = (cpu.AR [n].WORDNO + SIGNEXT15_18 (offset)) & AMASK;
        
        ARn_CHAR = GET_AR_CHAR (n); // AR[n].CHAR;
        ARn_BITNO = GET_AR_BITNO (n); // AR[n].BITNO;
        
        cpu.cu.TSN_PRNO[k-1] = n;
        cpu.cu.TSN_VALID[k-1] = 1;
        e -> addr [k - 1].SNR = cpu.PR[n].SNR;
        e -> addr [k - 1].RNR = max3 (cpu.PR [n].RNR, cpu.TPR.TRR, cpu.PPR.PRR);
        e -> addr [k - 1].mat = viaPR;
      }
    
    y += ((9u * ARn_CHAR + 36u * r + ARn_BITNO) / 36u);
    y &= AMASK;
    
    PNL (cpu.du.Dk_PTR_W[k-1] = y);

#ifdef EIS_PTR
    cpu.du.Dk_PTR_W[k-1] = y;
#else
    e -> addr [k - 1].address = y;
#endif
  }

static void parseNumericOperandDescriptor (int k, fault_ipr_subtype_ *mod_fault)
{
    PNL (L68_ (if (k == 1)
      DU_CYCLE_NLD1;
    else if (k == 2)
      DU_CYCLE_NLD2;
    else if (k == 3)
      DU_CYCLE_GSTR;))

    EISstruct * e = & cpu.currentEISinstruction;
    word18 MFk = e->MF[k-1];

    PNL (cpu.du.POP = 1);

    word36 opDesc = e->op[k-1];

    word8 ARn_CHAR = 0;
    word6 ARn_BITNO = 0;

    word18 address = GETHI(opDesc);
    if (MFk & MFkAR)
    {
        // if MKf contains ar then it Means Y-charn is not the memory address
        // of the data but is a reference to a pointer register pointing to the
        // data.
        word3 n = (word3) getbits18 (address, 0, 3);
        CPTUR (cptUsePRn + n);
        word15 offset = address & MASK15;  // 15-bit signed number
        address = (cpu.AR[n].WORDNO + SIGNEXT15_18(offset)) & AMASK;

        ARn_CHAR = GET_AR_CHAR (n); // AR[n].CHAR;
        ARn_BITNO = GET_AR_BITNO (n); // AR[n].BITNO;

        cpu.cu.TSN_PRNO[k-1] = n;
        cpu.cu.TSN_VALID[k-1] = 1;
        e->addr[k-1].SNR = cpu.PR[n].SNR;
        e->addr[k-1].RNR = max3(cpu.PR[n].RNR, cpu.TPR.TRR, cpu.PPR.PRR);

        e->addr[k-1].mat = viaPR;   // ARs involved
    }

    PNL (cpu.du.POL = 1);

    word3 CN = getbits36_3 (opDesc, 18);    // character number
    e->TN[k-1] = getbits36_1 (opDesc, 21); // type numeric

#ifdef PANEL
    if (k == 1)
      {
        if (e->TN[0])
          cpu.dataMode = 0021; // 4 bit numeric
        else
          cpu.dataMode = 0101; // 9 bit numeric
      }
#endif

    e->S[k-1]  = getbits36_2 (opDesc, 22);    // Sign and decimal type of data
    e->SF[k-1] = SIGNEXT6_int (getbits36_6 (opDesc, 24));    // Scaling factor.

    // Operand length. If MFk.RL = 0, this field contains the operand length in
    // digits. If MFk.RL = 1, it contains the REG code for the register holding
    // the operand length and C(REG) is treated as a 0 modulo 64 number. See
    // Table 4-1 and EIS modification fields (MF) above for a discussion of
    // register codes.

    if (MFk & MFkRL)
    {
        uint reg = opDesc & 017;
        e->N[k-1] = getMFReg18(reg, false, false, mod_fault) & 077; // RL=1: disallow du,n,ic
    }
    else
        e->N[k-1] = opDesc & 077;

    sim_debug (DBG_TRACEEXT, & cpu_dev, "parseNumericOperandDescriptor(): N%u %0o\n", k, e->N[k-1]);

    word36 r = getMFReg36(MFk & 017, false, true, mod_fault); // disallow du, allow n, ic
    if ((MFk & 017) == 4)   // reg == IC ?
    {
        address += r;
        address &= AMASK;
        r = 0;
    }

#ifdef ISOLTS
#if 0
    uint TN = e->TN[k-1];
    uint S = e->S[k-1];  // This is where MVNE gets really nasty.
#endif
#endif
// handled in numeric instructions
#if 0
    uint N = e->N[k-1];  // number of chars in string
    // I spit on the designers of this instruction set (and of COBOL.) >Ptui!<

    if (N == 0)
      {
        doFault (FAULT_IPR, fst_ill_proc, "parseNumericOperandDescriptor N=0");
      }
#endif

// Causes:
//DBG(662088814)> CPU0 FAULT: Fault 10(012), sub 4294967296(040000000000), dfc N, 'parseNumericOperandDescriptor N=1 S=0|1|2'^M
//DBG(662088814)> CPU0 FAULT: 00257:004574 bound_process_env_:command_query_+04574^M
//DBG(662088814)> CPU0 FAULT:       664 end print_question;^M
//DBG(662088814)> CPU0 FAULT: 00257:004574 4 000100301500 (BTD PR0|100) 000100 301(1) 0 0 0 00^M

#ifdef ISOLTS
#if 0 
// This test does not hold true for BTD operand 1; the S field is ignored by
// the instruction

    if (N == 1 && (S == 0 || S == 1 || S == 2))
      {
sim_printf ("k %d N %d S %d\n", k, N, S);
        doFault (FAULT_IPR, fst_ill_proc, "parseNumericOperandDescriptor N=1 S=0|1|2");
      }
#endif

// This breaks eis_tester 631 dtb; the S field in OP2 is ignored by the instruction.
#if 0
    if (N == 2 && S == 0)
      {
        doFault (FAULT_IPR, fst_ill_proc, "parseNumericOperandDescriptor N=2 S=0");
      }
#endif

// handled in numeric instructions
#if 0
    if (N == 3 && S == 0 && TN == 1)
      {
        doFault (FAULT_IPR, fst_ill_proc, "parseNumericOperandDescriptor N=3 S=0 TN 1");
      }
#endif
#endif


    uint effBITNO = 0;
    uint effCHAR = 0;
    uint effWORDNO = 0;

    // If seems that the effect address calcs given in AL39 p.6-27 are not
    // quite right.
    // E.g. For CTA4/CTN4 because of the 4 "slop" bits you need to do 32-bit
    // calcs not 36-bit!

    switch (e->TN[k-1])
    {
        case CTN4:
          {
            // Calculate character number of ARn CHAR and BITNO
            uint bitoffset = ARn_CHAR * 9u + ARn_BITNO;
            uint arn_char4 = bitoffset * 2u / 9u; // / 4.5
            //// The odd chars start at the 6th bit, not the 5th
            //if (bitoffset & 1) // if odd
            //  arn_char4 ++;
            // 8 chars per word plus the number of chars in r, plus the number of chars in ARn CHAR/BITNO
// XXX Handle 'r' too big intelligently...
            uint nchars = (uint) (address * 8u + r + arn_char4 + CN);

            effWORDNO = nchars / 8u; // 8 chars/word
            effCHAR = nchars % 8u; // effCHAR is the 4 bit char number, not the 9-bit char no
            effBITNO = (nchars & 1u) ? 5u : 0u;
            effWORDNO &= AMASK;

            e->CN[k-1] = effCHAR;        // ?????
          }
          break;

        case CTN9:
            if (CN & 1u)
#ifdef L68
              doFault(FAULT_IPR, fst_ill_proc, "parseNumericOperandDescriptor CTA9 & CN odd");
#else
              *mod_fault |= FR_ILL_PROC;
#endif
            CN = (CN >> 1u) & 03u;

            effBITNO = 0;
            effCHAR = ((word36) CN + (word36) ARn_CHAR + r) % 4u;
            effWORDNO = (uint) (address + (9u*CN + 9u*ARn_CHAR + 9u*r + ARn_BITNO) / 36);
            effWORDNO &= AMASK;

            e->CN[k-1] = effCHAR;        // ?????

            break;
        default:
#ifdef EIS_PTR3
            sim_printf ("parseNumericOperandDescriptor(ta=%d) How'd we get here 2?\n", cpu.du.TAk[k-1]);
#else
            sim_printf ("parseNumericOperandDescriptor(ta=%d) How'd we get here 2?\n", e->TA[k-1]);
#endif
            break;
    }

    EISaddr *a = &e->addr[k-1];
    PNL (cpu.du.Dk_PTR_W[k-1] = effWORDNO);
#ifdef EIS_PTR
    cpu.du.Dk_PTR_W[k-1] = effWORDNO;
#else
    a->address = effWORDNO;
#endif
    a->cPos = (int) effCHAR;
    a->bPos = (int) effBITNO;

    // a->_type = eisTN;
    a->TN = (int) e->TN[k-1];

#ifdef EIS_PTR
    sim_debug (DBG_TRACEEXT, & cpu_dev, "parseNumericOperandDescriptor(): address:%06o cPos:%d bPos:%d N%u %u\n", cpu.du.Dk_PTR_W[k-1], a->cPos, a->bPos, k, e->N[k-1]);
#else
    sim_debug (DBG_TRACEEXT, & cpu_dev, "parseNumericOperandDescriptor(): address:%06o cPos:%d bPos:%d N%u %u\n", a->address, a->cPos, a->bPos, k, e->N[k-1]);
#endif

}

static void parseBitstringOperandDescriptor (int k, fault_ipr_subtype_ *mod_fault)
{
    PNL (L68_ (if (k == 1)
      DU_CYCLE_ANLD1;
    else if (k == 2)
      DU_CYCLE_ANLD2;
    else if (k == 3)
      DU_CYCLE_ANSTR;))

    EISstruct * e = & cpu.currentEISinstruction;
    word18 MFk = e->MF[k-1];
    word36 opDesc = e->op[k-1];
    
#ifdef PANEL
    if (k == 1)
      cpu.dataMode = 0010; // 1 bit not alpha, not alpha numeric
#endif
    word8 ARn_CHAR = 0;
    word6 ARn_BITNO = 0;
    
    PNL (cpu.du.POP = 1);

    word18 address = GETHI(opDesc);
    if (MFk & MFkAR)
    {
        // if MKf contains ar then it Means Y-charn is not the memory address
        // of the data but is a reference to a pointer register pointing to the
        // data.
        word3 n = (word3) getbits18 (address, 0, 3);
        CPTUR (cptUsePRn + n);
        word15 offset = address & MASK15;  // 15-bit signed number
        address = (cpu.AR[n].WORDNO + SIGNEXT15_18(offset)) & AMASK;

        sim_debug (DBG_TRACEEXT, & cpu_dev, "bitstring k %d AR%d\n", k, n);
        
        ARn_CHAR = GET_AR_CHAR (n); // AR[n].CHAR;
        ARn_BITNO = GET_AR_BITNO (n); // AR[n].BITNO;
        cpu.cu.TSN_PRNO[k-1] = n;
        cpu.cu.TSN_VALID[k-1] = 1;
        e->addr[k-1].SNR = cpu.PR[n].SNR;
        e->addr[k-1].RNR = max3(cpu.PR[n].RNR, cpu.TPR.TRR, cpu.PPR.PRR);
        e->addr[k-1].mat = viaPR;   // ARs involved
    }
    PNL (cpu.du.POL = 1);

    //Operand length. If MFk.RL = 0, this field contains the string length of
    //the operand. If MFk.RL = 1, this field contains the code for a register
    //holding the operand string length. See Table 4-1 and EIS modification
    //fields (MF) above for a discussion of register codes.
    if (MFk & MFkRL)
    {
        uint reg = opDesc & 017;
        e->N[k-1] = getMFReg36(reg, false, false, mod_fault) & 077777777;  // RL=1: disallow du,n,ic
        sim_debug (DBG_TRACEEXT, & cpu_dev, "bitstring k %d RL reg %u val %"PRIo64"\n", k, reg, (word36)e->N[k-1]);
    }
    else
    {
        e ->N[k-1] = opDesc & 07777;
    }

    sim_debug (DBG_TRACEEXT, & cpu_dev, "bitstring k %d opdesc %012"PRIo64"\n", k, opDesc);
    sim_debug (DBG_TRACEEXT, & cpu_dev, "N%u %u\n", k, e->N[k-1]);
    
    
    word4 B = getbits36_4(opDesc, 20);    // bit# from descriptor
    word2 C = getbits36_2 (opDesc, 18);     // char# from descriptor

    if (B >= 9)
#ifdef L68
      doFault (FAULT_IPR, fst_ill_proc, "parseBitstringOperandDescriptor B >= 9");
#else
      *mod_fault |= FR_ILL_PROC;
#endif
     
    word36 r = getMFReg36(MFk & 017, false, true, mod_fault);  // disallow du, allow n,ic
    if ((MFk & 017) == 4)   // reg == IC ?
    {
        // If reg == IC, then R is in words, not bits.
        //r *= 36;
        address += r;
        address &= AMASK;
        r = 0;
    }

    uint effBITNO = (9u*ARn_CHAR + r + ARn_BITNO + B + 9u*C) % 9u;
    uint effCHAR = ((9u*ARn_CHAR + r + ARn_BITNO + B + 9u*C) % 36u) / 9u;
    uint effWORDNO = (uint) (address + (9u*ARn_CHAR + r + ARn_BITNO + B + 9u*C) / 36u);
    effWORDNO &= AMASK;
    
    e->B[k-1] = effBITNO;
    e->C[k-1] = effCHAR;
    
    EISaddr *a = &e->addr[k-1];
    PNL (cpu.du.Dk_PTR_W[k-1] = effWORDNO);
#ifdef EIS_PTR
    cpu.du.Dk_PTR_W[k-1] = effWORDNO;
#else
    a->address = effWORDNO;
#endif
    a->cPos = (int) effCHAR;
    a->bPos = (int) effBITNO;
}

static void cleanupOperandDescriptor (int k)
  {
    EISstruct * e = & cpu.currentEISinstruction;
    if (e -> addr [k - 1].cacheValid && e -> addr [k - 1].cacheDirty)
      {
        EISWriteCache(& e -> addr [k - 1]);
      }
    e -> addr [k - 1].cacheDirty = false;
  }

// For a4bd/s4bd, the world is made of 32 bit words, so the address space
// is 2^18 * 32 bits
#define n4bits (1 << 23)
// For a4bd/s4bd, the world is made of 8 4-bitcharacter words, so the address space
// is 2^18 * 8 characters
#define n4chars (1 << 21)
// For axbd/sxbd, the world is made of 36 bits words, so the address space
// is 2^18 * 36 bits
#define nxbits ((1 << 18) * 36)

// 2 * (s->BITNO / 9) + (s->BITNO % 9) / 4;
static unsigned int cntFromBit[36] = {
    0, 0, 0, 0, 0, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 7, 7, 7, 7
};

static word6 bitFromCnt[8] = {1, 5, 10, 14, 19, 23, 28, 32};

void a4bd (void)
  {
    // 8 4-bit characters/word

    uint ARn = GET_ARN (cpu.cu.IWB);
    CPTUR (cptUsePRn + ARn);
    int32_t address = SIGNEXT15_32 (GET_OFFSET (cpu.cu.IWB));
//if (current_running_cpu_idx)
//sim_printf ("a4bd address %o %d.\n", address, address);

    word4 reg = GET_TD (cpu.cu.IWB); // 4-bit register modification (None except 
                                     // au, qu, al, ql, xn)
    // r is the count of 4bit characters
    word36 ur = getCrAR (reg);
    int32 r = SIGNEXT22_32 ((word22) ur);
//if (current_running_cpu_idx)
//sim_printf ("a4bd r %o %d.\n", r, r);

    uint augend = 0; // in 4bit characters
    if (GET_A (cpu.cu.IWB))
       {
//if (current_running_cpu_idx)
//sim_printf ("a4bd AR%d WORDNO %o %d. CHAR %o BITNO %o\n", cpu.AR[ARn].WORDNO, cpu.AR[ARn].WORDNO, cpu.AR[ARn].WORDNO, cpu.AR[ARn].CHAR, cpu.AR[ARn].BITNO);

         //augend = cpu.AR[ARn].WORDNO * 32u + cntFromBit [GET_AR_BITNO (ARn)];
         // force to 4 bit character boundary
         //augend = augend & ~3;
         //augend = cpu.AR[ARn].WORDNO * 8 + cpu.AR[ARn].CHAR * 2;
         augend = cpu.AR[ARn].WORDNO * 8u + GET_AR_CHAR (ARn) * 2u;

         //if (cpu.AR[ARn].BITNO >= 5)
         if (GET_AR_BITNO (ARn) >= 5u)
           augend ++;
       }

//if (current_running_cpu_idx)
//sim_printf ("a4bd augend %o %d.\n", augend, augend);

    int32_t addend = address * 8 + r;  // in characters

//if (current_running_cpu_idx)
//sim_printf ("a4bd addend %o %d.\n", addend, addend);

    int32_t sum = (int32_t) augend + addend;
//if (current_running_cpu_idx)
//sim_printf ("a4bd sum %o %d.\n", sum, sum);


    // Handle over/under flow
    while (sum < 0)
      sum += n4chars;
    sum = sum % n4chars;
//if (current_running_cpu_idx)
//sim_printf ("a4bd sum %o %d.\n", sum, sum);


    cpu.AR[ARn].WORDNO = (word18) (sum / 8) & AMASK;
//if (current_running_cpu_idx)
//sim_printf ("a4bd WORDNO %o %d.\n", cpu.AR[ARn].WORDNO, cpu.AR[ARn].WORDNO);

//    // 0aaaabbbb0ccccdddd0eeeeffff0gggghhhh
//    //             111111 11112222 22222233
//    //  01234567 89012345 67890123 45678901   // 4 bit notation offset
//    static int tab [32] = { 1,  2,  3,  4,  5,  6,  7,  8,
//                           10, 11, 12, 13, 14, 15, 16, 17,
//                           19, 20, 21, 22, 23, 24, 25, 26,
//                           28, 29, 30, 31, 32, 33, 34, 35};
//
    //uint bitno = sum % 32;
//    AR [ARn].BITNO = tab [bitno];
    //cpu.AR [ARn].BITNO = bitFromCnt[bitno % 8];
    //SET_PR_BITNO (ARn, bitFromCnt[bitno % 8]);
    uint char4no = (uint) (sum % 8);
//if (current_running_cpu_idx)
//sim_printf ("a4bd char4no %d.\n", char4no);

    SET_AR_CHAR_BITNO (ARn, (word2) (char4no / 2), (char4no % 2) ? 5 : 0);
    HDBGRegAR (ARn);
//if (current_running_cpu_idx)
//sim_printf ("a4bd CHAR %o %d.\n", cpu.AR[ARn].CHAR, cpu.AR[ARn].CHAR);
//if (current_running_cpu_idx)
//sim_printf ("a4bd BITNO %o %d.\n", cpu.AR[ARn].BITNO, cpu.AR[ARn].BITNO);
  }


void s4bd (void)
  {
    uint ARn = GET_ARN (cpu.cu.IWB);
    CPTUR (cptUsePRn + ARn);
    int32_t address = SIGNEXT15_32 (GET_OFFSET (cpu.cu.IWB));
    word4 reg = GET_TD (cpu.cu.IWB); // 4-bit register modification (None except 
                                  // au, qu, al, ql, xn)
    // r is the count of characters
    word36 ur = getCrAR (reg);
    int32 r = SIGNEXT22_32 ((word22) ur);

    uint minuend = 0;
    if (GET_A (cpu.cu.IWB))
       {
         //minuend = cpu.AR [ARn].WORDNO * 32 + cntFromBit [GET_PR_BITNO (ARn)];
         minuend = cpu.AR [ARn].WORDNO * 32 + cntFromBit [GET_AR_CHAR (ARn) * 9 + GET_AR_BITNO (ARn)];
         // force to 4 bit character boundary
         minuend = minuend & (unsigned int) ~3;
       }
    int32_t subtractend = address * 32 + r * 4;
    int32_t difference = (int32_t) minuend - subtractend;

    // Handle over/under flow
    while (difference < 0)
      difference += n4bits;
    difference = difference % n4bits;

    cpu.AR [ARn].WORDNO = (word18) (difference / 32) & AMASK;

//    // 0aaaabbbb0ccccdddd0eeeeffff0gggghhhh
//    //             111111 11112222 22222233
//    //  01234567 89012345 67890123 45678901   // 4 bit notation offset
//    static int tab [32] = { 1,  2,  3,  4,  5,  6,  7,  8,
//                       10, 11, 12, 13, 14, 15, 16, 17,
//                       19, 20, 21, 22, 23, 24, 25, 26,
//                       28, 29, 30, 31, 32, 33, 34, 35};
//

    uint bitno = (uint) (difference % 32);
//    cpu.AR [ARn].BITNO = tab [bitno];
    // SET_PR_BITNO (ARn, bitFromCnt[bitno % 8]);
    SET_AR_CHAR_BITNO (ARn, bitFromCnt[bitno % 8] / 9, bitFromCnt[bitno % 8] % 9);
    HDBGRegAR (ARn);
  }

void axbd (uint sz)
  {
    uint ARn = GET_ARN (cpu.cu.IWB);
    CPTUR (cptUsePRn + ARn);
    int32_t address = SIGNEXT15_32 (GET_OFFSET (cpu.cu.IWB));
    word6 reg = GET_TD (cpu.cu.IWB); // 4-bit register modification (None except 
                                  // au, qu, al, ql, xn)
    // r is the count of characters
    word36 rcnt = getCrAR (reg);
    int32_t r;

    if (sz == 1)
      r = SIGNEXT24_32 ((word24) rcnt);
    else if (sz == 4)
      r = SIGNEXT22_32 ((word22) rcnt);
    else if (sz == 6)
      r = SIGNEXT21_32 ((word21) rcnt);
    else if (sz == 9)
      r = SIGNEXT21_32 ((word21) rcnt);
    else // if (sz == 36)
      r = SIGNEXT18_32 ((word18) rcnt);

    sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "axbd sz %d ARn 0%o address 0%o reg 0%o r 0%o\n", sz, ARn, address, reg, r);

  
    uint augend = 0;
    if (GET_A (cpu.cu.IWB))
      {
       sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "axbd ARn %d WORDNO %o CHAR %o BITNO %0o %d.\n", ARn, cpu.PAR[ARn].WORDNO, GET_AR_CHAR (ARn), GET_AR_BITNO (ARn), GET_AR_BITNO (ARn));
       augend = cpu.AR[ARn].WORDNO * 36u + GET_AR_CHAR (ARn) * 9u + GET_AR_BITNO (ARn);
      }
    sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "axbd augend 0%o\n", augend);
    // force to character boundary
    //if (sz == 9 || sz == 36 || GET_A (cpu.cu.IWB))
    if (sz == 9 || GET_A (cpu.cu.IWB))
      {
        augend = (augend / sz) * sz;
        sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "axbd force augend 0%o\n", augend);
      }
// If sz == 9, this is an a9bd instruction; ISOLTS says that r is in characters, not bits.
// wow. That breaks the boot bad.
//    if (sz == 9)
//      {
//        r *= 9;
//if (current_running_cpu_idx)
//sim_printf ("axbd force chars 0%o %d. bits\n", r, r);
//      }

    int32_t addend = address * 36 + r * (int32_t) sz;
    int32_t sum = (int32_t) augend + addend;

    // Handle over/under flow
    while (sum < 0)
      sum += nxbits;
    sum = sum % nxbits;

    sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "axbd augend 0%o addend 0%o sum 0%o\n", augend, addend, sum);

    cpu.AR [ARn].WORDNO = (word18) (sum / 36) & AMASK;
    //SET_PR_BITNO (ARn, sum % 36);
    SET_AR_CHAR_BITNO (ARn, (sum % 36) / 9, sum % 9);
    HDBGRegAR (ARn);
  }

#if 1
void abd (void)
  {
    uint ARn = GET_ARN (cpu.cu.IWB);
    CPTUR (cptUsePRn + ARn);

    word18 address = SIGNEXT15_18 (GET_OFFSET (cpu.cu.IWB));
//if (current_running_cpu_idx)
//sim_printf ("address %o\n", address);
    word4 reg = (word4) GET_TD (cpu.cu.IWB);
    // r is the count of bits (0 - 2^18 * 36 -1); 24 bits
    word24 r = getCrAR ((word4) reg) & MASK24;
//if (current_running_cpu_idx)
//sim_printf ("r 0%o %d.\n", r, r);
//if (current_running_cpu_idx)
//sim_printf ("abd WORDNO 0%o %d. CHAR %o BITNO 0%o %d.\n", cpu.AR[ARn].WORDNO, cpu.AR[ARn].WORDNO, cpu.AR[ARn].CHAR, cpu.AR[ARn].BITNO, cpu.AR[ARn].BITNO);

    //if (cpu.AR[ARn].BITNO > 8)
      //cpu.AR[ARn].BITNO = 8;
    if (GET_AR_BITNO (ARn) > 8)
      SET_AR_CHAR_BITNO (ARn, GET_AR_CHAR (ARn), 8);

    if (GET_A (cpu.cu.IWB))
      {
//if (current_running_cpu_idx)
//sim_printf ("A 1\n");
        //word24 bits = 9 * cpu.AR[ARn].CHAR + cpu.AR[ARn].BITNO + r;
        word24 bits = 9u * GET_AR_CHAR (ARn) + GET_AR_BITNO (ARn) + r;
//if (current_running_cpu_idx)
//sim_printf ("bits 0%o %d.\n", bits, bits);
        cpu.AR[ARn].WORDNO = (cpu.AR[ARn].WORDNO + address +
                              bits / 36) & MASK18;
        if (r % 36)
          {
            //cpu.AR[ARn].CHAR = (bits % 36) / 9;
            //cpu.AR[ARn].BITNO = bits % 9;
            SET_AR_CHAR_BITNO (ARn, (bits % 36) / 9,
                                    bits % 9);
          }
      }
    else
      {
//if (current_running_cpu_idx)
//sim_printf ("A 0\n");
        cpu.AR[ARn].WORDNO = (address + r / 36) & MASK18;
        if (r % 36)
          {
            //cpu.AR[ARn].CHAR = (r % 36) / 9;
            //cpu.AR[ARn].BITNO = r % 9;
            SET_AR_CHAR_BITNO (ARn, (r % 36) / 9,
                                    r % 9);
          }
      }
    HDBGRegAR (ARn);
 
//if (current_running_cpu_idx)
//sim_printf ("abd WORDNO 0%o %d. CHAR %o BITNO 0%o %d.\n", cpu.AR[ARn].WORDNO, cpu.AR[ARn].WORDNO, cpu.AR[ARn].CHAR, cpu.AR[ARn].BITNO, cpu.AR[ARn].BITNO);
  }
#else
void abd (void)
  {
    uint ARn = GET_ARN (cpu.cu.IWB);
    CPTUR (cptUsePRn + ARn);
    int32_t address = SIGNEXT15_32 (GET_OFFSET (cpu.cu.IWB));

if (current_running_cpu_idx)
sim_printf ("abd address 0%o %d.\n", address, address);

    // 4-bit register modification (None except 
    // au, qu, al, ql, xn)
    uint reg = GET_TD (cpu.cu.IWB);

    // r is the count of bits
    int32_t r = getCrAR (reg);

if (current_running_cpu_idx)
sim_printf ("abd r 0%o %d.\n", r, r);

    r = SIGNEXT24_32 (r);

if (current_running_cpu_idx)
sim_printf ("abd r 0%o %d.\n", r, r);
 
#define SEPARATE

    uint augend = 0; // in bits
#ifdef SEPARATE
    uint bitno = 0;
#endif

    if (GET_A (cpu.cu.IWB))
      {

if (current_running_cpu_idx)
sim_printf ("abd ARn %d WORDNO %o CHAR %o BITNO %0o %d. PR_BITNO %0o %d.\n", ARn, cpu.PAR[ARn].WORDNO, cpu.PAR[ARn].CHAR, cpu.PAR[ARn].BITNO, cpu.PAR[ARn].BITNO, GET_AR_BITNO (ARn), GET_AR_BITNO (ARn));
       sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "abd ARn %d WORDNO %o BITNO %0o %d.\n", ARn, cpu.PAR[ARn].WORDNO, GET_AR_BITNO (ARn), GET_AR_BITNO (ARn));

#ifdef SEPARATE
        //augend = cpu.AR[ARn].WORDNO * 36 + cpu.AR[ARn].CHAR * 9;
        //bitno = cpu.AR[ARn].BITNO;
        augend = cpu.AR[ARn].WORDNO * 36 + GET_AR_CHAR (ARn) * 9;
        bitno = GET_AR_BITNO (ARn);
#else
        augend = cpu.AR [ARn].WORDNO * 36 + GET_AR_CHARNO (ARn) * 9 + GET_AR_BITNO (ARn);
#endif
      }

if (current_running_cpu_idx)
sim_printf ("abd augend 0%o %d.\n", augend, augend);

#ifdef SEPARATE
    if (GET_A (cpu.cu.IWB))
      {

if (current_running_cpu_idx)
sim_printf ("abd bitno 0%o %d.\n", bitno, bitno);

        int32_t rBitcnt = r % 36;

if (current_running_cpu_idx)
sim_printf ("abd rBitcnt 0%o %d.\n", rBitcnt, rBitcnt);

        r -= rBitcnt;

if (current_running_cpu_idx)
sim_printf ("abd r 0%o %d.\n", r, r);
    sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "abd augend 0%o\n", augend);


        // BITNO overflows oddly; handle separately

        int32_t deltaBits = rBitcnt + bitno;

if (current_running_cpu_idx)
sim_printf ("abd deltaBits 0%o %d.\n", deltaBits, deltaBits);

        while (deltaBits < 0)
          {
            deltaBits += 9;
            r -= 9;
          }
        while (deltaBits > 15)
          {
            deltaBits -= 9;
            r += 9;
          }
        cpu.AR[ARn].BITNO = deltaBits;

if (current_running_cpu_idx)
sim_printf ("abd deltaBits 0%o %d.\n", deltaBits, deltaBits);
if (current_running_cpu_idx)
sim_printf ("abd r 0%o %d.\n", r, r);

      }
    else
      {
        cpu.AR[ARn].BITNO = (r % 9) & MASK4;
      }
#endif

    int32_t addend = address * 36 + r;

if (current_running_cpu_idx)
sim_printf ("abd addend 0%o %d.\n", addend, addend);

    int32_t sum = augend + addend;

if (current_running_cpu_idx)
sim_printf ("abd sum 0%o %d.\n", sum, sum);



    // Handle over/under flow
    while (sum < 0)
      sum += nxbits;
    sum = sum % nxbits;

if (current_running_cpu_idx)
sim_printf ("abd sum 0%o %d.\n", sum, sum);

    sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "abd augend 0%o addend 0%o sum 0%o\n", augend, addend, sum);

    cpu.AR[ARn].WORDNO = (sum / 36) & AMASK;
#ifdef SEPARATE
    //cpu.AR[ARn].CHAR = (sum / 9) & MASK2;
    SET_AR_CHAR_BITNO (ARn, (sum / 9) & MASK2, GET_AR_BITNO (ARn));
#else
    // Fails ISOLTS
    //SET_PR_BITNO (ARn, sum % 36);
    SET_AR_CHAR_BITNO (ARn, (sum % 36) / 9, sum % 9);
#endif
    HDBGRegAR (ARn);

    // Fails boot
    //uint bitno = sum % 36;
    //cpu.AR[ARn].CHAR = (bitno >> 4) & MASK2;
    //cpu.AR[ARn].BITNO = bitno & MASK4;

    
if (current_running_cpu_idx)
sim_printf ("abd WORDNO 0%o %d. CHAR %o BITNO 0%o %d.\n", cpu.AR[ARn].WORDNO, cpu.AR[ARn].WORDNO, cpu.AR[ARn].CHAR, cpu.AR[ARn].BITNO, cpu.AR[ARn].BITNO);
  }
#endif

void awd (void)
  {
    uint ARn = GET_ARN (cpu.cu.IWB);
    CPTUR (cptUsePRn + ARn);
    int32_t address = SIGNEXT15_32 (GET_OFFSET (cpu.cu.IWB));
    // 4-bit register modification (None except 
    // au, qu, al, ql, xn)
    word4 reg = (word4) GET_TD (cpu.cu.IWB);
    // r is the count of characters
// XXX This code is assuming that 'r' has 18 bits of data....
    int32_t r = (int32_t) (getCrAR (reg) & MASK18);
    r = SIGNEXT18_32 ((word18) r);

    sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev,
               "awd ARn 0%o address 0%o reg 0%o r 0%o\n", ARn, address, reg, r);

  
    uint augend = 0;
    if (GET_A (cpu.cu.IWB))
      {
       sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "awd ARn %d WORDNO %o CHAR %o BITNO %0o %d.\n", ARn, cpu.PAR[ARn].WORDNO, GET_AR_CHAR (ARn), GET_AR_BITNO (ARn), GET_AR_BITNO (ARn));

       //augend = cpu.AR [ARn].WORDNO * 36 + GET_AR_CHAR (ARn) * 9 + GET_AR_BITNO (ARn);
       augend = cpu.AR [ARn].WORDNO;
      }

    sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "awd augend 0%o\n", augend);

    int32_t addend = address + r;
    int32_t sum = (int32_t) augend + addend;

    sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "awd augend 0%o addend 0%o sum 0%o\n", augend, addend, sum);

    cpu.AR[ARn].WORDNO = (word18) sum & AMASK;
    SET_AR_CHAR_BITNO (ARn, 0, 0);
    HDBGRegAR (ARn);
  }

void sbd (void)
  {
    uint ARn = GET_ARN (cpu.cu.IWB);

    word18 address = SIGNEXT15_18 (GET_OFFSET (cpu.cu.IWB));
    word4 reg = (word4) GET_TD (cpu.cu.IWB);
    // r is the count of bits (0 - 2^18 * 36 -1); 24 bits
    word24 r = getCrAR ((word4) reg) & MASK24;
    if (GET_AR_BITNO (ARn) > 8)
      SET_AR_CHAR_BITNO (ARn, GET_AR_CHAR (ARn), 8);

    if (GET_A (cpu.cu.IWB))
      {
        word24 bits = 9u * GET_AR_CHAR (ARn) + GET_AR_BITNO (ARn) - r;
        cpu.AR[ARn].WORDNO = (cpu.AR[ARn].WORDNO - 
                             address + bits / 36) & MASK18;
        if (r % 36)
          {
            SET_AR_CHAR_BITNO (ARn, (- ((bits % 36) / 9)) & MASK2,
                                    (- (bits % 9)) & MASK4);
          }
      }
    else
      {
        cpu.AR[ARn].WORDNO = (- (address + r / 36)) & MASK18;
        if (r % 36)
          {
            SET_AR_CHAR_BITNO (ARn, (- ((r % 36) / 9)) & MASK2,
                                    (- (r % 9)) & MASK4);
          }
      }
    HDBGRegAR (ARn);
  }

void swd (void)
  {
    uint ARn = GET_ARN (cpu.cu.IWB);
    CPTUR (cptUsePRn + ARn);
    int32_t address = SIGNEXT15_32 (GET_OFFSET (cpu.cu.IWB));
    // 4-bit register modification (None except 
    // au, qu, al, ql, xn)
    word4 reg = (word4) GET_TD (cpu.cu.IWB);
    // r is the count of characters
// XXX This code is assuming that 'r' has 18 bits of data....
    int32_t r = (int32_t) (getCrAR (reg) & MASK18);
    r = SIGNEXT18_32 ((word18) r);

    sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "swd ARn 0%o address 0%o reg 0%o r 0%o\n", ARn, address, reg, r);

    uint minued = 0;
    if (GET_A (cpu.cu.IWB))
      {
       sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "swd ARn %d WORDNO %o CHAR %o BITNO %0o %d.\n", ARn, cpu.PAR[ARn].WORDNO, GET_AR_CHAR (ARn), GET_AR_BITNO (ARn), GET_AR_BITNO (ARn));

       //minued = cpu.AR [ARn].WORDNO * 36 + GET_AR_BITNO (ARn);
       minued = cpu.AR [ARn].WORDNO;
      }

    sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "swd minued 0%o\n", minued);

    int32_t subtractend = address + r;
    int32_t difference = (int32_t) minued - subtractend;

    sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "swd minued 0%o subtractend 0%o difference 0%o\n", minued, subtractend, difference);

    cpu.AR [ARn].WORDNO = (word18) difference & AMASK;
    SET_AR_CHAR_BITNO (ARn, 0, 0);
    HDBGRegAR (ARn);
  }

void s9bd (void)
  {
    uint ARn = GET_ARN (cpu.cu.IWB);
    CPTUR (cptUsePRn + ARn);
    word18 address = SIGNEXT15_18 (GET_OFFSET (cpu.cu.IWB));
    // 4-bit register modification (None except 
    // au, qu, al, ql, xn)
    word4 reg = (word4) GET_TD (cpu.cu.IWB);

    // r is the count of 9-bit characters
    word21 r = getCrAR (reg) & MASK21;;

    sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "s9bd r 0%o\n", r);

    sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "s9bd ARn 0%o address 0%o reg 0%o r 0%o\n", ARn, address, reg, r);

    if (GET_A (cpu.cu.IWB))
      {
        //cpu.AR[ARn].WORDNO = (cpu.AR[ARn].WORDNO - 
        //                      address + 
        //                      (cpu.AR[ARn].CHAR - r) / 4) & MASK18;
        cpu.AR[ARn].WORDNO = (cpu.AR[ARn].WORDNO - 
                              address + 
                              (GET_AR_CHAR (ARn) - r) / 4) & MASK18;
        //if (r % 36)
          //{
            //cpu.AR[ARn].CHAR = ((cpu.AR[ARn].CHAR - r) % 4) & MASK2;
            //cpu.AR[ARn].CHAR = (cpu.AR[ARn].CHAR - r)  & MASK2;
            SET_AR_CHAR_BITNO (ARn, (GET_AR_CHAR (ARn) - r)  & MASK2, 0);
          //}
      }
    else
      {
        cpu.AR[ARn].WORDNO = (- (address + (r + 3) / 4)) & MASK18;
        //if (r % 36)
          //{
            //cpu.AR[ARn].CHAR = (-r) & MASK2;
            SET_AR_CHAR_BITNO (ARn, (-r) & MASK2, 0);
          //}
      }
    //cpu.AR[ARn].BITNO = 0;
    HDBGRegAR (ARn);
 
//if (current_running_cpu_idx)
//sim_printf ("s9bd WORDNO 0%o %d. CHAR %o BITNO 0%o %d.\n", cpu.AR[ARn].WORDNO, cpu.AR[ARn].WORDNO, cpu.AR[ARn].CHAR, cpu.AR[ARn].BITNO, cpu.AR[ARn].BITNO);
  }


//
// Address Register arithmetic
//
// This code handles Address Register arithmetic
//
// asxbd (  ,       )
// ABD     1   false
// A4BD    4   false
// A6BD    6   false
// A9BD    9   false
// AWD    36   false
// SBD     1   true
// S4BD    4   true
// S6BD    6   true
// S9BD    9   true
// SWD    36   true
//

// The general approach is do all of the math as unsigned number of bits,
// modulo 2^18 * 36 (the number of bits in a segment).
//
// To handle subtraction underflow, a preemptive borrow is done if the 
// the operation will underflow.
//
// Notes:
//   According to ISOLTS 805, WORDNO is unsigned; this disagrees with AL-39

void asxbd (uint sz, bool sub)
  {
    // Map charno:bitno to bit offset for 4 bit char set
    uint map4 [64] =
      {      // 9-bit    4-bit
          0, // 0  0      0 
          0, // 0  1      0 
          0, // 0  2      0
          0, // 0  3      0
          0, // 0  4      0
          5, // 0  5      1
          5, // 0  6      1
          5, // 0  7      1
          5, // 0  8      1
          5, // 0  9  ill     guess
          5, // 0 10  ill     guess
          5, // 0 11  ill     guess
          5, // 0 12  ill     guess
          5, // 0 13  ill     guess
          5, // 0 14  ill     guess
          5, // 0 15  ill     guess
          9, // 1  0      2
          9, // 1  1      2
          9, // 1  2      2
          9, // 1  3      2
          9, // 1  4      2
         14, // 1  5      3
         14, // 1  6      3
         14, // 1  7      3
         14, // 1  8      3
         14, // 1  9  ill     guess
         14, // 1 10  ill ISOLTS 805 loop point 010226 sxbd test no 28 (4bit)
         14, // 1 11  ill     guess
         14, // 1 12  ill     guess
         14, // 1 13  ill     guess
         14, // 1 14  ill     guess
         14, // 1 15  ill     guess
         18, // 2  0      4
         18, // 2  1      4
         18, // 2  2      4
         18, // 2  3      4
         18, // 2  4      4
         23, // 2  5      5
         23, // 2  6      5
         23, // 2  7      5
         23, // 2  8      5
         23, // 2  9  ill     guess
         23, // 2 10  ill     guess
         23, // 2 11  ill     guess
         23, // 2 12  ill   ISOLTS 805 loop point 010226 sxbd test no 26 (4bit)
         23, // 2 13  ill     guess
         23, // 2 14  ill     guess
         23, // 2 15  ill     guess
         27, // 3  0      6
         27, // 3  1      6
         27, // 3  2      6
         27, // 3  3      6
         27, // 3  4      6
         32, // 3  5      7
         32, // 3  6      7
         32, // 3  7      7
         32, // 3  8      7
         32, // 3  9  ill     guess
         32, // 3 10  ill     guess
         32, // 3 11  ill     guess
         32, // 3 12  ill     guess
         32, // 3 13  ill     guess
         32, // 3 14  ill   ISOLTS 805 loop point 010226 sxbd test no 24 (4bit)
         32  // 3 15  ill     guess
      };
    // Map charno:bitno to bit offset for 6 bit char set
    uint map6 [64] =
      {      // 9-bit    6-bit
          0, // 0  0      0 
          1, // 0  1      0 
          2, // 0  2      0
          3, // 0  3      0
          4, // 0  4      0
          5, // 0  5      0
          6, // 0  6      1
          7, // 0  7      1
          8, // 0  8      1
          6, // 0  9  ill  ISOLTS 805 loop point 010100 sxbd test no 12
          6, // 0 10  ill     guess
          6, // 0 11  ill     guess
          6, // 0 12  ill     guess
          6, // 0 13  ill     guess
          6, // 0 14  ill     guess
          6, // 0 15  ill     guess
          9, // 1  0      1
         10, // 1  1      1
         11, // 1  2      1
         12, // 1  3      2
         13, // 1  4      2
         14, // 1  5      2
         15, // 1  6      2
         16, // 1  7      2
         17, // 1  8      2
         12, // 1  9  ill     guess
         12, // 1 10  ill     guess
         12, // 1 11  ill ISOLTS 805 loop point 010100 sxbd test no 12
         12, // 1 12  ill     guess
         12, // 1 13  ill     guess
         12, // 1 14  ill     guess
         12, // 1 15  ill     guess
         18, // 2  0      3
         19, // 2  1      3
         20, // 2  2      3
         21, // 2  3      3
         22, // 2  4      3
         23, // 2  5      3
         24, // 2  6      4
         25, // 2  7      4
         26, // 2  8      4
         24, // 2  9  ill     guess
         24, // 2 10  ill     guess
         24, // 2 11  ill     guess
         24, // 2 12  ill     guess
         24, // 2 13  ill   ISOLTS 805 loop point 010100 sxbd test no 10
         24, // 2 14  ill     guess
         24, // 2 15  ill     guess
         27, // 3  0      4
         28, // 3  1      4
         29, // 3  2      4
         30, // 3  3      5
         31, // 3  4      5
         32, // 3  5      5
         33, // 3  6      5
         34, // 3  7      5
         35, // 3  8      5
         30, // 3  9  ill     guess
         30, // 3 10  ill     guess
         30, // 3 11  ill     guess
         30, // 3 12  ill     guess
         30, // 3 13  ill     guess
         30, // 3 14  ill     guess
         30  // 3 15  ill   ISOLTS 805 loop point 010100 sxbd test no 8 (6bit)
      };
    // Map charno:bitno to 1 and 9 bit offset
    uint map9 [64] =
      {      // 9-bit 
          0, // 0  0
          1, // 0  1
          2, // 0  2
          3, // 0  3
          4, // 0  4
          5, // 0  5
          6, // 0  6
          7, // 0  7
          8, // 0  8
          8, // 0  9  ill     guess
          8, // 0 10  ill     guess
          8, // 0 11  ill     guess
          8, // 0 12  ill     guess
          8, // 0 13  ill     guess
          8, // 0 14  ill     guess
          8, // 0 15  ill     guess
          9, // 1  0
         10, // 1  1
         11, // 1  2
         12, // 1  3
         13, // 1  4
         14, // 1  5
         15, // 1  6
         16, // 1  7
         17, // 1  8
         17, // 1  9  ill     guess
         17, // 1 10  ill     guess
         17, // 1 11  ill     guess
         17, // 1 12  ill     guess
         17, // 1 13  ill     guess
         17, // 1 14  ill     guess
         17, // 1 15  ill     guess
         18, // 2  0
         19, // 2  1
         20, // 2  2
         21, // 2  3
         22, // 2  4
         23, // 2  5
         24, // 2  6
         25, // 2  7
         26, // 2  8
         26, // 2  9  ill     guess
         26, // 2 10  ill     guess
         26, // 2 11  ill     guess
         26, // 2 12  ill     guess
         26, // 2 13  ill     guess
         26, // 2 14  ill     guess
         26, // 2 15  ill     guess
         27, // 3  0
         28, // 3  1
         29, // 3  2
         30, // 3  3
         31, // 3  4
         32, // 3  5
         33, // 3  6
         34, // 3  7
         35, // 3  8
         35, // 3  9  ill     guess
         35, // 3 10  ill     guess
         35, // 3 11  ill     guess
         35, // 3 12  ill     guess
         35, // 3 13  ill     guess
         35, // 3 14  ill     guess
         35  // 3 15  ill     guess
      };

//
// Extract the operand data from the instruction
//

    uint ARn = GET_ARN (cpu.cu.IWB);
    uint address = SIGNEXT15_18 (GET_OFFSET (cpu.cu.IWB));
    word4 reg = (word4) GET_TD (cpu.cu.IWB); // 4-bit register modification (None except 
                                  // au, qu, al, ql, xn)

//
// Calculate r
//

    // r is the count of characters (or bits if sz is 1; words if sz == 36)
    word36 rcnt = getCrAR (reg);

    sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "asxbd sz %d r 0%"PRIo64"\n", sz, rcnt);

    // Crop rcnt into r based on the operand size.
    uint r = 0;

    if (sz == 1)
      r = (uint) (rcnt & MASK24);
    else if (sz == 4)
      r = (uint) (rcnt & MASK22);
    else if (sz == 6)
      r = (uint) (rcnt & MASK21);
    else if (sz == 9)
      r = (uint) (rcnt & MASK21);
    else // if (sz == 36)
      r = (uint) (rcnt & MASK18);

    sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "asxbd sz %d ARn 0%o address 0%o reg 0%o r 0%o\n", sz, ARn, address, reg, r);

//
// Calculate augend
//

    // If A is set, the instruction is AR = AR op operand; if not, AR = 0 op operand.
    uint augend = 0;
    if (GET_A (cpu.cu.IWB))
      {
        // For AWD/SWD, leave CHAR/BITNO alone
        if (sz == 36)
          {
            augend = cpu.AR[ARn].WORDNO * 36u;
          }
        else
          {
            uint bitno = GET_AR_BITNO (ARn);
            uint charno = GET_AR_CHAR (ARn);

            // The behavior of the DU for cases of CHAR > 9 is not defined; some values
            // are tested by ISOLTS, and have been recorded in the mapx tables; the 
            // missing values are guessed at.

            uint * map;
            if (sz == 4)
              map = map4;
            else if (sz == 6)
              map = map6;
            else
              map = map9;

            augend = cpu.AR[ARn].WORDNO * 36u + map [charno * 16 + bitno];
            augend = augend % nxbits;
          }
      }

//
// Calculate addend
//

    uint addend = 0;
    if (sz == 4)
      {
        // r is the number of 4 bit characters; each character is actually
        // 4.5 bits long
        addend = address * 36u + (r * 9) / 2;

        // round the odd character up one bit
        // (the odd character starts at bit n/2 + 1, not n/2.)
        if ((! sub) && r % 2) // r is odd
          addend ++;
      }
    else
      addend = address * 36u + r * sz;

    // Handle overflow
    addend = addend % nxbits;

//
// Calculate sum or difference
//

    uint sum = 0;
    if (sub)
      {
        // Prevent underflow
        if (addend > augend)
          augend += nxbits;
        sum = augend - addend;
      }
    else
      {
        sum = augend + addend;
        sum %= nxbits;
      }

    sim_debug (DBG_TRACEEXT|DBG_CAC, & cpu_dev, "asxbd augend 0%o addend 0%o sum 0%o\n", augend, addend, sum);

//
// Adjust to character boundary
//

    if (sz == 6 || sz == 9)
      {
        sum = (sum / sz) * sz;
      }

//
// Convert sum to WORDNO/CHAR/BITNO
//

    cpu.AR [ARn].WORDNO = (word18) (sum / 36u) & AMASK;

    // If AWD/SWD clear CHAR/BITNO

    if (sz == 36)
      {
        SET_AR_CHAR_BITNO (ARn, 0, 0);
      }
    else
      {
        if (sz == 4)
          {
            static uint tab [36] [2] =
              {
                // char bitno  offset  4-bit charno
                { 0, 0 }, // 0   0
                { 0, 0 }, // 1
                { 0, 0 }, // 2
                { 0, 0 }, // 3
                { 0, 0 }, // 4

                { 0, 5 }, // 5   1
                { 0, 5 }, // 6
                { 0, 5 }, // 7
                { 0, 5 }, // 8

                { 1, 0 }, // 9   2
                { 1, 0 }, // 10
                { 1, 0 }, // 11
                { 1, 0 }, // 12
                { 1, 0 }, // 13

                { 1, 5 }, // 15  3
                { 1, 5 }, // 15
                { 1, 5 }, // 16
                { 1, 5 }, // 17

                { 2, 0 }, // 18  4
                { 2, 0 }, // 19
                { 2, 0 }, // 20
                { 2, 0 }, // 21
                { 2, 0 }, // 22

                { 2, 5 }, // 23  5
                { 2, 5 }, // 24
                { 2, 5 }, // 25
                { 2, 5 }, // 26

                { 3, 0 }, // 27  6
                { 3, 0 }, // 28
                { 3, 0 }, // 29
                { 3, 0 }, // 30
                { 3, 0 }, // 31

                { 3, 5 }, // 32  7
                { 3, 5 }, // 33
                { 3, 5 }, // 34
                { 3, 5 }  // 35
              };
            uint charno = tab [sum % 36u] [0];
            uint bitno = tab [sum % 36u] [1];
            SET_AR_CHAR_BITNO (ARn, (word2) charno, (word4) bitno);
          }
        else
          {
            uint charno = (sum % 36u) / 9;
            uint bitno = sum % 9;
            SET_AR_CHAR_BITNO (ARn, (word2) charno, (word4) bitno);
          }
      }
    HDBGRegAR (ARn);
  }

void cmpc (void)
  {
    EISstruct * e = & cpu.currentEISinstruction;

    // For i = 1, 2, ..., minimum (N1,N2)
    //    C(Y-charn1)i-1 :: C(Y-charn2)i-1
    // If N1 < N2, then for i = N1+1, N1+2, ..., N2
    //    C(FILL) :: C(Y-charn2)i-1
    // If N1 > N2, then for i = N2+1, N2+2, ..., N1
    //    C(Y-charn1)i-1 :: C(FILL)
    //
    // Indicators:
    //     Zero: If C(Y-charn1)i-1 = C(Y-charn2)i-1 for all i, then ON; 
    //       otherwise, OFF
    //     Carry: If C(Y-charn1)i-1 < C(Y-charn2)i-1 for any i, then OFF; 
    //       otherwise ON
    
    // Both strings are treated as the data type given for the left-hand
    // string, TA1. A data type given for the right-hand string, TA2, is
    // ignored.
    //
    // Comparison is made on full 9-bit fields. If the given data type is not
    // 9-bit (TA1 ≠ 0), then characters from C(Y-charn1) and C(Y-charn2) are
    // high- order zero filled. All 9 bits of C(FILL) are used.
    //
    // Instruction execution proceeds until an inequality is found or the
    // larger string length count is exhausted.

    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor (1, &mod_fault);
    setupOperandDescriptor (2, &mod_fault);
#endif
    parseAlphanumericOperandDescriptor (1, 1, false, &mod_fault);
    parseAlphanumericOperandDescriptor (2, 1, false, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bits 9-10 MBZ
    if (IWB_IRODD & 0000600000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "cmpc 9-10 MBZ");

    // Bit 23 of OP1 MBZ
    if (!(e->MF[0] & MFkID) && e -> op [0]  & 0000000010000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "cmpc op1 23 MBZ");

// ISOLTS ps846    test-07a    dec add test
// Sets TA2 to the same as TA1. AL39 says TA2 ignored.
// Try only check bit 23.
// ISOLTS 880 test-04c sets bit 23.
#if 0
    // // Bits 21-23 of OP2 MBZ
    // if (e -> op [1]  & 0000000070000)
    // Bit 23 of OP2 MBZ
    if (!(e->MF[1] & MFkID) && e -> op [1]  & 0000000010000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "cmpc op2 23 MBZ");
#endif

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    word9 fill = getbits36_9 (cpu.cu.IWB, 0);
    
    SET_I_ZERO;  // set ZERO flag assuming strings are equal ...
    SET_I_CARRY; // set CARRY flag assuming strings are equal ...
    
    PNL (L68_ (if (max (e->N1, e->N2) < 128)
      DU_CYCLE_FLEN_128;))

    for (; cpu.du.CHTALLY < min (e->N1, e->N2); cpu.du.CHTALLY ++)
      {
        word9 c1 = EISget469 (1, cpu.du.CHTALLY); // get Y-char1n
        word9 c2 = EISget469 (2, cpu.du.CHTALLY); // get Y-char2n
sim_debug (DBG_TRACEEXT, & cpu_dev, "cmpc tally %d c1 %03o c2 %03o\n", cpu.du.CHTALLY, c1, c2);
        if (c1 != c2)
          {
            CLR_I_ZERO;  // an inequality found
            SC_I_CARRY (c1 > c2);
            cleanupOperandDescriptor (1);
            cleanupOperandDescriptor (2);
            return;
          }
      }

    if (e -> N1 < e -> N2)
      {
        for( ; cpu.du.CHTALLY < e->N2; cpu.du.CHTALLY ++)
          {
            word9 c1 = fill;     // use fill for Y-char1n
            word9 c2 = EISget469 (2, cpu.du.CHTALLY); // get Y-char2n
            
            if (c1 != c2)
              {
                CLR_I_ZERO;  // an inequality found
                SC_I_CARRY (c1 > c2);
                cleanupOperandDescriptor (1);
                cleanupOperandDescriptor (2);
                return;
              }
          }
      }
    else if (e->N1 > e->N2)
      {
        for ( ; cpu.du.CHTALLY < e->N1; cpu.du.CHTALLY ++)
          {
            word9 c1 = EISget469 (1, cpu.du.CHTALLY); // get Y-char1n
            word9 c2 = fill;   // use fill for Y-char2n
            
            if (c1 != c2)
              {
                CLR_I_ZERO;  // an inequality found
                SC_I_CARRY (c1 > c2);
                cleanupOperandDescriptor (1);
                cleanupOperandDescriptor (2);
                return;
              }
          }
      }
    // else ==
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
  }


/*
 * SCD - Scan Characters Double
 */

void scd ()
  {
    EISstruct * e = & cpu.currentEISinstruction;

    // For i = 1, 2, ..., N1-1
    //   C(Y-charn1)i-1,i :: C(Y-charn2)0,1
    // On instruction completion, if a match was found:
    //   00...0 → C(Y3)0,11
    //   i-1 → C(Y3)12,35
    // If no match was found:
    //   00...0 → C(Y3)0,11
    //      N1-1→ C(Y3)12,35
    //
    
    // The REG field of MF1 is checked for a legal code. If DU is specified in
    // the REG field of MF2 in one of the four multiword instructions (SCD,
    // SCDR, SCM, or SCMR) for which DU is legal, the CN field is ignored and
    // the character or characters are arranged within the 18 bits of the word
    // address portion of the operand descriptor.

    fault_ipr_subtype_ mod_fault = 0;

#ifndef EIS_SETUP
    setupOperandDescriptor (1, &mod_fault);
    setupOperandDescriptor (2, &mod_fault);
    setupOperandDescriptorCache (3);
#endif
    
    parseAlphanumericOperandDescriptor (1, 1, false, &mod_fault);
    parseAlphanumericOperandDescriptor (2, 1, true, &mod_fault); // use TA1
    parseArgOperandDescriptor (3, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bits 0-10 MBZ
    if (IWB_IRODD & 0777600000000)
      {
        //sim_printf ("scd %12"PRIo64"\n", IWB_IRODD);
        doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "scd 0-10 MBZ");
      }

    // Bit 23 of OP1 MBZ
    if (!(e->MF[0] & MFkID) && e -> op [0]  & 0000000010000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "scd op1 23 MBZ");

    // Bits 18-28. 30-31 of OP3 MBZ
    if (!(e->MF[2] & MFkID) && e -> op [2]  & 0000000777660)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "scd op3 18-28. 30-31 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    // Both the string and the test character pair are treated as the data type
    // given for the string, TA1. A data type given for the test character
    // pair, TA2, is ignored.
    
    // fetch 'test' char - double
    // If MF2.ID = 0 and MF2.REG = du, then the second word following the
    // instruction word does not contain an operand descriptor for the test
    // character; instead, it contains the test character as a direct upper
    // operand in bits 0,8.
    
    word9 c1 = 0;
    word9 c2 = 0;
    
    if (! (e -> MF2 & MFkID) && ((e -> MF2 & MFkREGMASK) == 3))  // MF2.du
      {
        // per Bull RJ78, p. 5-45
#ifdef EIS_PTR3
        switch (TA1) // Use TA1, not TA2
#else
        switch (e -> TA1) // Use TA1, not TA2
#endif
        {
            case CTA4:
#ifdef EIS_PTR
              c1 = (cpu.du.D2_PTR_W >> 13) & 017;
              c2 = (cpu.du.D2_PTR_W >>  9) & 017;
#else
              c1 = (e -> ADDR2.address >> 13) & 017;
              c2 = (e -> ADDR2.address >>  9) & 017;
#endif
              break;

            case CTA6:
#ifdef EIS_PTR
              c1 = (cpu.du.D2_PTR_W >> 12) & 077;
              c2 = (cpu.du.D2_PTR_W >>  6) & 077;
#else
              c1 = (e -> ADDR2.address >> 12) & 077;
              c2 = (e -> ADDR2.address >>  6) & 077;
#endif
              break;

            case CTA9:
#ifdef EIS_PTR
              c1 = (cpu.du.D2_PTR_W >> 9) & 0777;
              c2 = (cpu.du.D2_PTR_W     ) & 0777;
#else
              c1 = (e -> ADDR2.address >> 9) & 0777;
              c2 = (e -> ADDR2.address     ) & 0777;
#endif
              break;
          }
      }
    else
      {  
        c1 = EISget469 (2, 0);
        c2 = EISget469 (2, 1);
      }

#ifdef EIS_PTR3
    switch (TA1) // Use TA1, not TA2
#else
    switch (e -> TA1) // Use TA1, not TA2
#endif
      {
        case CTA4:
          c1 &= 017;    // keep 4-bits
          c2 &= 017;    // keep 4-bits
          break;

        case CTA6:
          c1 &= 077;    // keep 6-bits
          c2 &= 077;    // keep 6-bits
          break;

        case CTA9:
          c1 &= 0777;   // keep 9-bits
          c2 &= 0777;   // keep 9-bits
          break;
      }
    

    PNL (L68_ (if (e->N1 < 128)
      DU_CYCLE_FLEN_128;))

    word9 yCharn11;
    word9 yCharn12;
    if (e -> N1)
      {
        uint limit = e -> N1 - 1;
        for ( ; cpu.du.CHTALLY < limit; cpu.du.CHTALLY ++)
          {
            yCharn11 = EISget469 (1, cpu.du.CHTALLY);
            yCharn12 = EISget469 (1, cpu.du.CHTALLY + 1);
            if (yCharn11 == c1 && yCharn12 == c2)
              break;
          }
        SC_I_TALLY (cpu.du.CHTALLY == limit);
      }
    else
      {
        SET_I_TALLY;
      }
    
    //word36 CY3 = bitfieldInsert36 (0, cpu.du.CHTALLY, 0, 24);
    word36 CY3 = setbits36_24 (0, 12, cpu.du.CHTALLY);
    EISWriteIdx (& e -> ADDR3, 0, CY3, true);

    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    cleanupOperandDescriptor (3);
 }

/*
 * SCDR - Scan Characters Double Reverse
 */

void scdr (void)
  {
    EISstruct * e = & cpu.currentEISinstruction;

    // For i = 1, 2, ..., N1-1
    //   C(Y-charn1)N1-i-1,N1-i :: C(Y-charn2)0,1
    // On instruction completion, if a match was found:
    //   00...0 → C(Y3)0,11
    //   i-1 → C(Y3)12,35
    // If no match was found:
    //   00...0 → C(Y3)0,11
    //      N1-1→ C(Y3)12,35
    //
    
    // The REG field of MF1 is checked for a legal code. If DU is specified in
    // the REG field of MF2 in one of the four multiword instructions (SCD,
    // SCDR, SCM, or SCMR) for which DU is legal, the CN field is ignored and
    // the character or characters are arranged within the 18 bits of the word
    // address portion of the operand descriptor.

    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor(1, &mod_fault);
    setupOperandDescriptor(2, &mod_fault);
    setupOperandDescriptorCache(3);
#endif

    parseAlphanumericOperandDescriptor(1, 1, false, &mod_fault);
    parseAlphanumericOperandDescriptor(2, 1, true, &mod_fault); // Use TA1
    parseArgOperandDescriptor (3, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bits 0-10 MBZ
    if (IWB_IRODD & 0777600000000)
      {
        //sim_printf ("scdr %12"PRIo64"\n", IWB_IRODD);
        doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "scdr 0-10 MBZ");
      }

    // Bit 23 of OP1 MBZ
    if (!(e->MF[0] & MFkID) && e -> op [0]  & 0000000010000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "scdr op1 23 MBZ");

    // Bits 18-28. 30-31 of OP3 MBZ
    if (!(e->MF[2] & MFkID) && e -> op [2]  & 0000000777660)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "scdr op3 18-28. 30-31 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    // Both the string and the test character pair are treated as the data type
    // given for the string, TA1. A data type given for the test character
    // pair, TA2, is ignored.

    // fetch 'test' char - double
    // If MF2.ID = 0 and MF2.REG = du, then the second word following the
    // instruction word does not contain an operand descriptor for the test
    // character; instead, it contains the test character as a direct upper
    // operand in bits 0,8.
    
    word9 c1 = 0;
    word9 c2 = 0;
    
    if (! (e -> MF2 & MFkID) && ((e -> MF2 & MFkREGMASK) == 3))  // MF2.du
      {
        // per Bull RJ78, p. 5-45
#ifdef EIS_PTR3
        switch (TA1) // Use TA1, not TA2
#else
        switch (e -> TA1)
#endif
          {
            case CTA4:
#ifdef EIS_PTR
              c1 = (cpu.du.D2_PTR_W >> 13) & 017;
              c2 = (cpu.du.D2_PTR_W >>  9) & 017;
#else
              c1 = (e -> ADDR2.address >> 13) & 017;
              c2 = (e -> ADDR2.address >>  9) & 017;
#endif
              break;

            case CTA6:
#ifdef EIS_PTR
              c1 = (cpu.du.D2_PTR_W >> 12) & 077;
              c2 = (cpu.du.D2_PTR_W >>  6) & 077;
#else
              c1 = (e -> ADDR2.address >> 12) & 077;
              c2 = (e -> ADDR2.address >>  6) & 077;
#endif
              break;

            case CTA9:
#ifdef EIS_PTR
              c1 = (cpu.du.D2_PTR_W >> 9) & 0777;
              c2 = (cpu.du.D2_PTR_W     ) & 0777;
#else
              c1 = (e -> ADDR2.address >> 9) & 0777;
              c2 = (e -> ADDR2.address     ) & 0777;
#endif
              break;
          }
      }
    else
      {
        c1 = EISget469 (2, 0);
        c2 = EISget469 (2, 1);
      }

#ifdef EIS_PTR3
    switch (TA1) // Use TA1, not TA2
#else
    switch (e -> TA1) // Use TA1, not TA2
#endif
      {
        case CTA4:
          c1 &= 017;    // keep 4-bits
          c2 &= 017;    // keep 4-bits
          break;

        case CTA6:
          c1 &= 077;    // keep 6-bits
          c2 &= 077;    // keep 6-bits
          break;

        case CTA9:
          c1 &= 0777;   // keep 9-bits
          c2 &= 0777;   // keep 9-bits
          break;
      }
    
    word9 yCharn11;
    word9 yCharn12;

    PNL (L68_ (if (e->N1 < 128)
      DU_CYCLE_FLEN_128;))

    if (e -> N1)
      {
        uint limit = e -> N1 - 1;
    
        for ( ; cpu.du.CHTALLY < limit; cpu.du.CHTALLY ++)
          {
            yCharn11 = EISget469 (1, limit - cpu.du.CHTALLY - 1);
            yCharn12 = EISget469 (1, limit - cpu.du.CHTALLY);
        
            if (yCharn11 == c1 && yCharn12 == c2)
                break;
          }
        SC_I_TALLY (cpu.du.CHTALLY == limit);
      }
    else
      {
        SET_I_TALLY;
      }

    //word36 CY3 = bitfieldInsert36(0, cpu.du.CHTALLY, 0, 24);
    word36 CY3 = setbits36_24 (0, 12, cpu.du.CHTALLY);
    EISWriteIdx (& e -> ADDR3, 0, CY3, true);

    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    cleanupOperandDescriptor (3);
  }


/*
 * SCM - Scan with Mask
 */

void scm (void)
  {
    EISstruct * e = & cpu.currentEISinstruction;

    // For characters i = 1, 2, ..., N1
    //   For bits j = 0, 1, ..., 8
    //      C(Z)j = ~C(MASK)j & ((C(Y-charn1)i-1 )j ⊕ (C(Y-charn2)0)j)
    //      If C(Z)0,8 = 00...0, then
    //           00...0 → C(Y3)0,11
    //           i-1 → C(Y3)12,35
    //      otherwise, continue scan of C(Y-charn1)
    // If a masked character match was not found, then
    //   00...0 → C(Y3)0,11
    //   N1 → C(Y3)12,35
    
    // Starting at location YC1, the L1 type TA1 characters are masked and
    // compared with the assumed type TA1 character contained either in
    // location YC2 or in bits 0-8 or 0-5 of the address field of operand
    // descriptor 2 (when the REG field of MF2 specifies DU modification). The
    // mask is right-justified in bit positions 0-8 of the instruction word.
    // Each bit position of the mask that is a 1 prevents that bit position in
    // the two characters from entering into the compare.

    // The masked compare operation continues until either a match is found or
    // the tally (L1) is exhausted. For each unsuccessful match, a count is
    // incremented by 1. When a match is found or when the L1 tally runs out,
    // this count is stored right-justified in bits 12-35 of location Y3 and
    // bits 0-11 of Y3 are zeroed. The contents of location YC2 and the source
    // string remain unchanged. The RL bit of the MF2 field is not used.
    
    // The REG field of MF1 is checked for a legal code. If DU is specified in
    // the REG field of MF2 in one of the four multiword instructions (SCD,
    // SCDR, SCM, or SCMR) for which DU is legal, the CN field is ignored and
    // the character or characters are arranged within the 18 bits of the word
    // address portion of the operand descriptor.
    
    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor (1, &mod_fault);
    setupOperandDescriptor (2, &mod_fault);
    setupOperandDescriptorCache (3);
#endif

    parseAlphanumericOperandDescriptor (1, 1, false, &mod_fault);
    parseAlphanumericOperandDescriptor (2, 1, true, &mod_fault);
    parseArgOperandDescriptor (3, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bits 9-10 MBZ
    if (IWB_IRODD & 0000600000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "scm 9-10 MBZ");

    // Bit 23 of OP1 MBZ
    if (!(e->MF[0] & MFkID) && e -> op [0]  & 0000000010000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "scm op1 23 MBZ");

    // Bits 18-28, 39-31 of OP3 MBZ
    if (!(e->MF[2] & MFkID) && e -> op [2]  & 0000000777660)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "scm op3 18-28, 39-31 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    // Both the string and the test character pair are treated as the data type
    // given for the string, TA1. A data type given for the test character
    // pair, TA2, is ignored.

    // get 'mask'
    uint mask = (uint) getbits36_9 (cpu.cu.IWB, 0);
    
    // fetch 'test' char
    // If MF2.ID = 0 and MF2.REG = du, then the second word following the
    // instruction word does not contain an operand descriptor for the test
    // character; instead, it contains the test character as a direct upper
    // operand in bits 0,8.
    
    word9 ctest = 0;
    if (! (e -> MF2 & MFkID) && ((e -> MF2 & MFkREGMASK) == 3))  // MF2.du
      {
        word18 duo = GETHI (e -> OP2);
        // per Bull RJ78, p. 5-45
#ifdef EIS_PTR3
        switch (TA1) // Use TA1, not TA2
#else
        switch (e -> TA1)
#endif
          {
            case CTA4:
              ctest = (duo >> 13) & 017;
              break;
            case CTA6:
              ctest = (duo >> 12) & 077;
              break;
            case CTA9:
              ctest = (duo >> 9) & 0777;
              break;
          }
      }
    else
      {
        ctest = EISget469 (2, 0);
      }

#ifdef EIS_PTR3
    switch (TA1) // Use TA1, not TA2
#else
    switch (e -> TA1) // use TA1, not TA2
#endif
      {
        case CTA4:
            ctest &= 017;    // keep 4-bits
            break;
        case CTA6:
            ctest &= 077;    // keep 6-bits
            break;
        case CTA9:
            ctest &= 0777;   // keep 9-bits
      }

    PNL (L68_ (if (e->N1 < 128)
      DU_CYCLE_FLEN_128;))

    uint limit = e -> N1;

    for ( ; cpu.du.CHTALLY < limit; cpu.du.CHTALLY ++)
      {
        word9 yCharn1 = EISget469 (1, cpu.du.CHTALLY);
        word9 c = ((~mask) & (yCharn1 ^ ctest)) & 0777;
        if (c == 0)
          {
            //00...0 → C(Y3)0,11
            //i-1 → C(Y3)12,35
            //Y3 = bitfieldInsert36(Y3, cpu.du.CHTALLY, 0, 24);
            break;
          }
      }
    //word36 CY3 = bitfieldInsert36 (0, cpu.du.CHTALLY, 0, 24);
    word36 CY3 = setbits36_24 (0, 12, cpu.du.CHTALLY);
    
    SC_I_TALLY (cpu.du.CHTALLY == limit);
    
    EISWriteIdx (& e -> ADDR3, 0, CY3, true);

    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    cleanupOperandDescriptor (3);
  }
/*
 * SCMR - Scan with Mask in Reverse
 */

void scmr (void)
  {
    EISstruct * e = & cpu.currentEISinstruction;

    // For characters i = 1, 2, ..., N1
    //   For bits j = 0, 1, ..., 8
    //      C(Z)j = ~C(MASK)j & ((C(Y-charn1)i-1 )j ⊕ (C(Y-charn2)0)j)
    //      If C(Z)0,8 = 00...0, then
    //           00...0 → C(Y3)0,11
    //           i-1 → C(Y3)12,35
    //      otherwise, continue scan of C(Y-charn1)
    // If a masked character match was not found, then
    //   00...0 → C(Y3)0,11
    //   N1 → C(Y3)12,35
    
    // Starting at location YC1, the L1 type TA1 characters are masked and
    // compared with the assumed type TA1 character contained either in
    // location YC2 or in bits 0-8 or 0-5 of the address field of operand
    // descriptor 2 (when the REG field of MF2 specifies DU modification). The
    // mask is right-justified in bit positions 0-8 of the instruction word.
    // Each bit position of the mask that is a 1 prevents that bit position in
    // the two characters from entering into the compare.
 
    // The masked compare operation continues until either a match is found or
    // the tally (L1) is exhausted. For each unsuccessful match, a count is
    // incremented by 1. When a match is found or when the L1 tally runs out,
    // this count is stored right-justified in bits 12-35 of location Y3 and
    // bits 0-11 of Y3 are zeroed. The contents of location YC2 and the source
    // string remain unchanged. The RL bit of the MF2 field is not used.
    
    // The REG field of MF1 is checked for a legal code. If DU is specified in
    // the REG field of MF2 in one of the four multiword instructions (SCD,
    // SCDR, SCM, or SCMR) for which DU is legal, the CN field is ignored and
    // the character or characters are arranged within the 18 bits of the word
    // address portion of the operand descriptor.
    
    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor (1, &mod_fault);
    setupOperandDescriptor (2, &mod_fault);
    setupOperandDescriptorCache (3);
#endif

    parseAlphanumericOperandDescriptor (1, 1, false, &mod_fault);
    parseAlphanumericOperandDescriptor (2, 1, true, &mod_fault);
    parseArgOperandDescriptor (3, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bits 9-10 MBZ
    if (IWB_IRODD & 0000600000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "scmr 9-10 MBZ");

    // Bit 23 of OP1 MBZ
    if (!(e->MF[0] & MFkID) && e -> op [0]  & 0000000010000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "scmr op1 23 MBZ");

    // Bits 18 of OP3 MBZ
    //if (!(e->MF[2] & MFkID) && e -> op [2]  & 0000000400000)
    //  doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "scmr op3 18 MBZ");

    // Bits 18-28, 39-31 of OP3 MBZ
    if (!(e->MF[2] & MFkID) && e -> op [2]  & 0000000777660)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "scmr op3 18-28, 39-31 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    // Both the string and the test character pair are treated as the data type
    // given for the string, TA1. A data type given for the test character
    // pair, TA2, is ignored.

    // get 'mask'
    uint mask = (uint) getbits36_9 (cpu.cu.IWB, 0);
    
    // fetch 'test' char
    // If MF2.ID = 0 and MF2.REG = du, then the second word following the
    // instruction word does not contain an operand descriptor for the test
    // character; instead, it contains the test character as a direct upper
    // operand in bits 0,8.
    
    word9 ctest = 0;
    if (! (e -> MF2 & MFkID) && ((e -> MF2 & MFkREGMASK) == 3))  // MF2.du
      {
        word18 duo = GETHI (e -> OP2);
        // per Bull RJ78, p. 5-45
#ifdef EIS_PTR3
        switch (TA1) // Use TA1, not TA2
#else
        switch (e -> TA1)
#endif
          {
            case CTA4:
              ctest = (duo >> 13) & 017;
              break;
            case CTA6:
              ctest = (duo >> 12) & 077;
              break;
            case CTA9:
              ctest = (duo >> 9) & 0777;
              break;
          }
      }
    else
      {
        ctest = EISget469 (2, 0);
      }

#ifdef EIS_PTR3
    switch (TA1) // Use TA1, not TA2
#else
    switch (e -> TA1) // use TA1, not TA2
#endif
      {
        case CTA4:
            ctest &= 017;    // keep 4-bits
            break;
        case CTA6:
            ctest &= 077;    // keep 6-bits
            break;
        case CTA9:
            ctest &= 0777;   // keep 9-bits
      }

    PNL (L68_ (if (e->N1 < 128)
      DU_CYCLE_FLEN_128;))

    uint limit = e -> N1;
    for ( ; cpu.du.CHTALLY < limit; cpu.du.CHTALLY ++)
      {
        word9 yCharn1 = EISget469 (1, limit - cpu.du.CHTALLY - 1);
        word9 c = ((~mask) & (yCharn1 ^ ctest)) & 0777;
        if (c == 0)
          {
            //00...0 → C(Y3)0,11
            //i-1 → C(Y3)12,35
            //Y3 = bitfieldInsert36(Y3, cpu.du.CHTALLY, 0, 24);
            break;
          }
      }
    //word36 CY3 = bitfieldInsert36 (0, cpu.du.CHTALLY, 0, 24);
    word36 CY3 = setbits36_24 (0, 12, cpu.du.CHTALLY);
    
    SC_I_TALLY (cpu.du.CHTALLY == limit);
    
    EISWriteIdx (& e -> ADDR3, 0, CY3, true);

    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    cleanupOperandDescriptor (3);
  }

/*
 * TCT - Test Character and Translate
 */

#if 0
static word9 xlate (word36 * xlatTbl, uint dstTA, uint c)
  {
    uint idx = (c / 4) & 0177;      // max 128-words (7-bit index)
    word36 entry = xlatTbl [idx];

    uint pos9 = c % 4;      // lower 2-bits
    word9 cout = GETBYTE (entry, pos9);
    switch (dstTA)
      {
        case CTA4:
          return cout & 017;
        case CTA6:
          return cout & 077;
        case CTA9:
          return cout;
      }
    return 0;
  }
#endif

static word9 xlate (EISaddr * xlatTbl, uint dstTA, uint c)
  {
    uint idx = (c / 4) & 0177;      // max 128-words (7-bit index)
    word36 entry = EISReadIdx(xlatTbl, idx);

    uint pos9 = c % 4;      // lower 2-bits
    word9 cout = GETBYTE (entry, pos9);
    switch (dstTA)
      {
        case CTA4:
          return cout & 017;
        case CTA6:
          return cout & 077;
        case CTA9:
          return cout;
      }
    return 0;
  }


void tct (void)
  {
    EISstruct * e = & cpu.currentEISinstruction;

    // For i = 1, 2, ..., N1
    //   m = C(Y-charn1)i-1
    //   If C(Y-char92)m ≠ 00...0, then
    //     C(Y-char92)m → C(Y3)0,8
    //     000 → C(Y3)9,11
    //     i-1 → C(Y3)12,35
    //   otherwise, continue scan of C(Y-charn1)
    // If a non-zero table entry was not found, then 00...0 → C(Y3)0,11
    // N1 → C(Y3)12,35
    //
    // Indicators: Tally Run Out. If the string length count exhausts, then ON;
    // otherwise, OFF
    //
    // If the data type of the string to be scanned is not 9-bit (TA1 ≠ 0),
    // then characters from C(Y-charn1) are high-order zero filled in forming
    // the table index, m.
    // Instruction execution proceeds until a non-zero table entry is found or
    // the string length count is exhausted.
    // The character number of Y-char92 must be zero, i.e., Y-char92 must start
    // on a word boundary.
    
    
    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor (1, &mod_fault);
    setupOperandDescriptorCache (2);
    setupOperandDescriptorCache (3);
#endif
    
    parseAlphanumericOperandDescriptor (1, 1, false, &mod_fault);
    parseArgOperandDescriptor (2, &mod_fault);
    parseArgOperandDescriptor (3, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bits 0-17 MBZ
    if (IWB_IRODD & 0777777000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "tct 0-17 MBZ");

    // Bit 23 of OP1 MBZ
    if (!(e->MF[0] & MFkID) && e -> op [0]  & 0000000010000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "tct op1 23 MBZ");

    // Bits 18-28, 39-31 of OP2 MBZ
    if (!(e->MF[1] & MFkID) && e -> op [1]  & 0000000777660)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "tct op2 18-28, 39-31 MBZ");

    // Bits 18-28, 39-31 of OP3 MBZ
    if (!(e->MF[2] & MFkID) && e -> op [2]  & 0000000777660)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "tct op3 18-28, 39-31 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

#ifdef EIS_PTR3
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "TCT CN1: %d TA1: %d\n", e -> CN1, TA1);
#else
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "TCT CN1: %d TA1: %d\n", e -> CN1, e -> TA1);
#endif

    uint srcSZ = 0;

#ifdef EIS_PTR3
    switch (TA1)
#else
    switch (e -> TA1)
#endif
      {
        case CTA4:
            srcSZ = 4;
            break;
        case CTA6:
            srcSZ = 6;
            break;
        case CTA9:
            srcSZ = 9;
            break;
      }
    
    // ISOLTS-878 01h asserts no prepaging
#if 0    
    //  Prepage Check in a Multiword Instruction
    //  The MVT, TCT, TCTR, and CMPCT instruction have a prepage check. The
    //  size of the translate table is determined by the TA1 data type as shown
    //  in the table below. Before the instruction is executed, a check is made
    //  for allocation in memory for the page for the translate table. If the
    //  page is not in memory, a Missing Page fault occurs before execution of
    //  the instruction. (Bull RJ78 p.7-75)
    
    // TA1              TRANSLATE TABLE SIZE
    // 4-BIT CHARACTER      4 WORDS
    // 6-BIT CHARACTER     16 WORDS
    // 9-BIT CHARACTER    128 WORDS
    
    uint xlatSize = 0;   // size of xlation table in words .....
#ifdef EIS_PTR3
    switch (TA1)
#else
    switch(e -> TA1)
#endif
    {
        case CTA4:
            xlatSize = 4;
            break;
        case CTA6:
            xlatSize = 16;
            break;
        case CTA9:
            xlatSize = 128;
            break;
    }
    
    word36 xlatTbl [128];
    memset (xlatTbl, 0, sizeof (xlatTbl));    // 0 it out just in case
    
    EISReadN (& e -> ADDR2, xlatSize, xlatTbl);
#endif
    
    word36 CY3 = 0;
    
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "TCT N1 %d\n", e -> N1);

    PNL (L68_ (if (e->N1 < 128)
      DU_CYCLE_FLEN_128;))

    for ( ; cpu.du.CHTALLY < e -> N1; cpu.du.CHTALLY ++)
      {
        word9 c = EISget469 (1, cpu.du.CHTALLY); // get src char

        uint m = 0;
        
        switch (srcSZ)
          {
            case 4:
              m = c & 017;    // truncate upper 2-bits
              break;
            case 6:
              m = c & 077;    // truncate upper 3-bits
              break;
            case 9:
              m = c;          // keep all 9-bits
              break;              // should already be 0-filled
          }
        
        word9 cout = xlate (&e->ADDR2, CTA9, m);

        sim_debug (DBG_TRACEEXT, & cpu_dev,
                   "TCT c %03o %c cout %03o %c\n",
                   m, isprint ((int) m) ? '?' : (char) m, 
                   cout, isprint ((int) cout) ? '?' : (char) cout);

        if (cout)
          {
            // CY3 = bitfieldInsert36 (0, cout, 27, 9); // C(Y-char92)m -> C(Y3)0,8
            CY3 = setbits36_9 (0, 0, cout);
            break;
          }
      }
    
    SC_I_TALLY (cpu.du.CHTALLY == e -> N1);
    
    //CY3 = bitfieldInsert36 (CY3, cpu.du.CHTALLY, 0, 24);
    putbits36_24 (& CY3, 12, cpu.du.CHTALLY);
    EISWriteIdx (& e -> ADDR3, 0, CY3, true);
    
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    cleanupOperandDescriptor (3);
  }

void tctr (void)
  {
    EISstruct * e = & cpu.currentEISinstruction;

    // For i = 1, 2, ..., N1
    //   m = C(Y-charn1)N1-i
    //   If C(Y-char92)m ≠ 00...0, then
    //     C(Y-char92)m → C(Y3)0,8
    //     000 → C(Y3)9,11
    //     i-1 → C(Y3)12,35
    //   otherwise, continue scan of C(Y-charn1) If a non-zero table entry was
    //   not found, then
    // 00...0 → C(Y3)0,11
    // N1 → C(Y3)12,35
    //
    // Indicators: Tally Run Out. If the string length count exhausts, then ON;
    // otherwise, OFF
    //
    // If the data type of the string to be scanned is not 9-bit (TA1 ≠ 0),
    // then characters from C(Y-charn1) are high-order zero filled in forming
    // the table index, m.

    // Instruction execution proceeds until a non-zero table entry is found or
    // the string length count is exhausted.

    // The character number of Y-char92 must be zero, i.e., Y-char92 must start
    // on a word boundary.
 
    
    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor (1, &mod_fault);
    setupOperandDescriptorCache (2);
    setupOperandDescriptorCache (3);
#endif
    
    parseAlphanumericOperandDescriptor (1, 1, false, &mod_fault);
    parseArgOperandDescriptor (2, &mod_fault);
    parseArgOperandDescriptor (3, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bits 0-17 MBZ
    if (IWB_IRODD & 0777777000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "tctr 0-17 MBZ");

    // Bit 23 of OP1 MBZ
    if (!(e->MF[0] & MFkID) && e -> op [0]  & 0000000010000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "tctr op1 23 MBZ");

    // Bits 18-28, 39-31 of OP2 MBZ
    if (!(e->MF[1] & MFkID) && e -> op [1]  & 0000000777660)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "tctr op2 18-28, 39-31 MBZ");

    // Bits 18-28, 39-31 of OP3 MBZ
    if (!(e->MF[2] & MFkID) && e -> op [2]  & 0000000777660)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "tctr op3 18-28, 39-31 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

#ifdef EIS_PTR3
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "TCTR CN1: %d TA1: %d\n", e -> CN1, TA1);
#else
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "TCTR CN1: %d TA1: %d\n", e -> CN1, e -> TA1);
#endif

    uint srcSZ = 0;

#ifdef EIS_PTR3
    switch (TA1)
#else
    switch (e -> TA1)
#endif
      {
        case CTA4:
            srcSZ = 4;
            break;
        case CTA6:
            srcSZ = 6;
            break;
        case CTA9:
            srcSZ = 9;
            break;
      }
    
    // ISOLTS-878 01i asserts no prepaging    
#if 0    
    //  Prepage Check in a Multiword Instruction
    //  The MVT, TCT, TCTR, and CMPCT instruction have a prepage check. The
    //  size of the translate table is determined by the TA1 data type as shown
    //  in the table below. Before the instruction is executed, a check is made
    //  for allocation in memory for the page for the translate table. If the
    //  page is not in memory, a Missing Page fault occurs before execution of
    //  the instruction. (Bull RJ78 p.7-75)
    
    // TA1              TRANSLATE TABLE SIZE
    // 4-BIT CHARACTER      4 WORDS
    // 6-BIT CHARACTER     16 WORDS
    // 9-BIT CHARACTER    128 WORDS
    
    uint xlatSize = 0;   // size of xlation table in words .....
#ifdef EIS_PTR3
    switch (TA1)
#else
    switch(e -> TA1)
#endif
    {
        case CTA4:
            xlatSize = 4;
            break;
        case CTA6:
            xlatSize = 16;
            break;
        case CTA9:
            xlatSize = 128;
            break;
    }
    
    word36 xlatTbl [128];
    memset (xlatTbl, 0, sizeof (xlatTbl));    // 0 it out just in case
    
    EISReadN (& e -> ADDR2, xlatSize, xlatTbl);
#endif
    
    word36 CY3 = 0;
    
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "TCT N1 %d\n", e -> N1);

    PNL (L68_ (if (e->N1 < 128)
      DU_CYCLE_FLEN_128;))

    uint limit = e -> N1;
    for ( ; cpu.du.CHTALLY < limit; cpu.du.CHTALLY ++)
      {
        word9 c = EISget469 (1, limit - cpu.du.CHTALLY - 1); // get src char

        uint m = 0;
        
        switch (srcSZ)
          {
            case 4:
              m = c & 017;    // truncate upper 2-bits
              break;
            case 6:
              m = c & 077;    // truncate upper 3-bits
              break;
            case 9:
              m = c;          // keep all 9-bits
              break;              // should already be 0-filled
          }
        
        word9 cout = xlate (&e->ADDR2, CTA9, m);

        sim_debug (DBG_TRACEEXT, & cpu_dev,
                   "TCT c %03o %c cout %03o %c\n",
                   m, isprint ((int) m) ? '?' : (char) m, 
                   cout, isprint ((int) cout) ? '?' : (char) cout);

        if (cout)
          {
            //CY3 = bitfieldInsert36 (0, cout, 27, 9); // C(Y-char92)m -> C(Y3)0,8
            CY3 = setbits36_9 (0, 0, cout);
            break;
          }
      }
    
    SC_I_TALLY (cpu.du.CHTALLY == e -> N1);
    
    //CY3 = bitfieldInsert36 (CY3, cpu.du.CHTALLY, 0, 24);
    putbits36_24 (& CY3, 12, cpu.du.CHTALLY);
    EISWriteIdx (& e -> ADDR3, 0, CY3, true);
    
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    cleanupOperandDescriptor (3);
  }

/*
 * MLR - Move Alphanumeric Left to Right
 *
 * (Nice, simple instruction if it weren't for the stupid overpunch stuff that ruined it!!!!)
 */

/*
 * does 6-bit char represent a GEBCD negative overpuch? if so, whice numeral?
 * Refer to Bull NovaScale 9000 RJ78 Rev2 p11-178
 */

#if 0
static bool isOvp (uint c, word9 * on)
  {
    // look for GEBCD -' 'A B C D E F G H I (positive overpunch)
    // look for GEBCD - ^ J K L M N O P Q R (negative overpunch)
    
    word9 c2 = c & 077;   // keep to 6-bits
    * on = 0;
    
    if (c2 >= 020 && c2 <= 031)   // positive overpunch
      {
        * on = c2 - 020;          // return GEBCD number 0-9 (020 is +0)
        return false;             // well, it's not a negative overpunch is it?
      }
    if (c2 >= 040 && c2 <= 052)   // negative overpunch
      {
        * on = c2 - 040;  // return GEBCD number 0-9 
                         // (052 is just a '-' interpreted as -0)
        return true;
      }
    return false;
}
#endif

// RJ78 p.11-178,D-6
static bool isGBCDOvp (uint c, bool * isNeg)
  {
    if (c & 020)
      {
        * isNeg = false;
        return true;
      }
    if (c & 040)
      {
        * isNeg = true;
        return true;
      }
    return false;
  }

// Applies to both MLR and MRL
// 
// If L1 is greater than L2, the least significant (L1-L2) characters are not moved and
// the Truncation indicator is set. If L1 is less than L2, bits 0-8, 3-8, or 5-8 of the FILL
// character (depending on TA2) are inserted as the least significant (L2-L1)
// characters. If L1 is less than L2, bit 0 of C(FILL) = 1, TA1 = 01, and TA2 = 10
// (6-4 move); the hardware looks for a 6-bit overpunched sign. If a negative
// overpunch sign is found, a negative sign (octal 15) is inserted as the last FILL
// character. If a negative overpunch sign is not found, a positive sign (octal 14) is
// inserted as the last FILL character

void mlr (void)
  {
    EISstruct * e = & cpu.currentEISinstruction;

    // For i = 1, 2, ..., minimum (N1,N2)
    //     C(Y-charn1)N1-i → C(Y-charn2)N2-i
    // If N1 < N2, then for i = N1+1, N1+2, ..., N2
    //    C(FILL) → C(Y-charn2)N2-i
    // Indicators: Truncation. If N1 > N2 then ON; otherwise OFF
    
    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor (1, &mod_fault);
    setupOperandDescriptor (2, &mod_fault);
    //setupOperandDescriptorCache (3);
#endif
    
    parseAlphanumericOperandDescriptor(1, 1, false, &mod_fault);
    parseAlphanumericOperandDescriptor(2, 2, false, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bit 10 MBZ
    if (IWB_IRODD & 0000200000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "mlr 10 MBZ");

    // Bit 23 of OP1 MBZ
    if (!(e->MF[0] & MFkID) && e -> op [0]  & 0000000010000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "mlr op1 23 MBZ");

    // Bit 23 of OP2 MBZ
    if (!(e->MF[1] & MFkID) && e -> op [1]  & 0000000010000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "mlr op2 23 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    int srcSZ = 0, dstSZ = 0;

#ifdef EIS_PTR3
    switch (TA1)
#else
    switch (e -> TA1)
#endif
      {
        case CTA4:
          srcSZ = 4;
          break;
        case CTA6:
          srcSZ = 6;
          break;
        case CTA9:
          srcSZ = 9;
          break;
      }
    
#ifdef EIS_PTR3
    switch (TA2)
#else
    switch (e -> TA2)
#endif
      {
        case CTA4:
          dstSZ = 4;
          break;
        case CTA6:
          dstSZ = 6;
          break;
        case CTA9:
          dstSZ = 9;
          break;
      }
    
    word1 T = getbits36_1 (cpu.cu.IWB, 9);
    
    word9 fill = getbits36_9 (cpu.cu.IWB, 0);
    word9 fillT = fill;  // possibly truncated fill pattern

    // play with fill if we need to use it
    switch (dstSZ)
      {
        case 4:
          fillT = fill & 017;    // truncate upper 5-bits
          break;
        case 6:
          fillT = fill & 077;    // truncate upper 3-bits
          break;
      }
    
    // If N1 > N2, then (N1-N2) leading characters of C(Y-charn1) are not moved
    // and the truncation indicator is set ON.

    // If N1 < N2 and TA2 = 2 (4-bit data) or 1 (6-bit data), then FILL
    // characters are high-order truncated as they are moved to C(Y-charn2). No
    // character conversion takes place.

    // The user of string replication or overlaying is warned that the decimal
    // unit addresses the main memory in unaligned (not on modulo 8 boundary)
    // units of Y-block8 words and that the overlayed string, C(Y-charn2), is
    // not returned to main memory until the unit of Y-block8 words is filled or
    // the instruction completes.

    // If T = 1 and the truncation indicator is set ON by execution of the
    // instruction, then a truncation (overflow) fault occurs.

    // Attempted execution with the xed instruction causes an illegal procedure
    // fault.

    // Attempted repetition with the rpt, rpd, or rpl instructions causes an
    // illegal procedure fault.
    
    PNL (L68_ (if (max (e->N1, e->N2) < 128)
      DU_CYCLE_FLEN_128;))

#ifdef EIS_PTR3
    bool ovp = (e -> N1 < e -> N2) && (fill & 0400) && (TA1 == 1) &&
               (TA2 == 2); // (6-4 move)
#else
    bool ovp = (e -> N1 < e -> N2) && (fill & 0400) && (e -> TA1 == 1) &&
               (e -> TA2 == 2); // (6-4 move)
#endif
    //word9 on;     // number overpunch represents (if any)
    bool isNeg = false;
    //bool bOvp = false;  // true when a negative overpunch character has been 
                        // found @ N1-1 

    
#ifdef EIS_PTR3
    sim_debug (DBG_TRACEEXT, & cpu_dev, "MLR TALLY %u TA1 %u TA2 %u N1 %u N2 %u CN1 %u CN2 %u\n", cpu.du.CHTALLY, TA1, TA2, e -> N1, e -> N2, e -> CN1, e -> CN2);
#else
    sim_debug (DBG_TRACEEXT, & cpu_dev, "MLR TALLY %u TA1 %u TA2 %u N1 %u N2 %u CN1 %u CN2 %u\n", cpu.du.CHTALLY, e -> TA1, e -> TA2, e -> N1, e -> N2, e -> CN1, e -> CN2);
#endif
    
//
// Multics frequently uses certain code sequences which are easily detected
// and optimized; eg. it uses the MLR instruction to copy or zero segments.
//
// The MLR implementation is correct, not efficent. Copy invokes 12 append
// cycles per word, and fill 8.
//


//
// Page copy
//

    if ((cpu.du.CHTALLY % PGSZ) == 0 &&
#ifdef EIS_PTR3
        TA1 == CTA9 &&  // src and dst are both char 9
        TA2 == CTA9 &&
#else
        e -> TA1 == CTA9 &&  // src and dst are both char 9
        e -> TA2 == CTA9 &&
#endif
        (e -> N1 % (PGSZ * 4)) == 0 &&  // a page
        e -> N2 == e -> N1 && // the src is the same size as the dest.
        e -> CN1 == 0 &&  // and it starts at a word boundary // BITNO?
        e -> CN2 == 0 &&
#ifdef EIS_PTR
        (cpu.du.D1_PTR_W & PGMK) == 0 &&
        (cpu.du.D2_PTR_W & PGMK) == 0)
#else
        (e -> ADDR1.address & PGMK) == 0 &&
        (e -> ADDR2.address & PGMK) == 0)
#endif
      {
        sim_debug (DBG_TRACEEXT, & cpu_dev, "MLR special case #3\n");
        while (cpu.du.CHTALLY < e -> N1)
          {
            word36 pg [PGSZ];
            EISReadPage (& e -> ADDR1, cpu.du.CHTALLY / 4, pg);
            EISWritePage (& e -> ADDR2, cpu.du.CHTALLY / 4, pg);
            cpu.du.CHTALLY += PGSZ * 4;
          }
        cleanupOperandDescriptor (1);
        cleanupOperandDescriptor (2);
        // truncation fault check does need to be checked for here since 
        // it is known that N1 == N2
        CLR_I_TRUNC;
        return;
      }

//
// Page zero
//

    if ((cpu.du.CHTALLY % PGSZ) == 0 &&
#ifdef EIS_PTR3
        TA1 == CTA9 &&  // src and dst are both char 9
        TA2 == CTA9 &&
#else
        e -> TA1 == CTA9 &&  // src and dst are both char 9
        e -> TA2 == CTA9 &&
#endif
        e -> N1 == 0 && // the source is entirely fill
        (e -> N2 % (PGSZ * 4)) == 0 &&  // a page
        e -> CN1 == 0 &&  // and it starts at a word boundary // BITNO?
        e -> CN2 == 0 &&
#ifdef EIS_PTR
        (cpu.du.D1_PTR_W & PGMK) == 0 &&
        (cpu.du.D2_PTR_W& PGMK) == 0)
#else
        (e -> ADDR1.address & PGMK) == 0 &&
        (e -> ADDR2.address & PGMK) == 0)
#endif
      {
        sim_debug (DBG_TRACEEXT, & cpu_dev, "MLR special case #4\n");
        word36 pg [PGSZ];
        if (fill)
          {
            word36 w = (word36) fill | ((word36) fill << 9) | ((word36) fill << 18) | ((word36) fill << 27);
            for (uint i = 0; i < PGSZ; i ++)
              pg [i] = w;
          }
        else
          {
           memset (pg, 0, sizeof (pg));
          }
        while (cpu.du.CHTALLY < e -> N2)
          {
            EISWritePage (& e -> ADDR2, cpu.du.CHTALLY / 4, pg);
            cpu.du.CHTALLY += PGSZ * 4;
          }
        cleanupOperandDescriptor (1);
        cleanupOperandDescriptor (2);
        // truncation fault check does need to be checked for here since 
        // it is known that N1 == N2
        CLR_I_TRUNC;
        return;
      }

// Test for the case of aligned word move; and do things a word at a time,
// instead of a byte at a time...

#ifdef EIS_PTR3
    if (TA1 == CTA9 &&  // src and dst are both char 9
        TA2 == CTA9 &&
#else
    if (e -> TA1 == CTA9 &&  // src and dst are both char 9
        e -> TA2 == CTA9 &&
#endif
        e -> N1 % 4 == 0 &&  // a whole number of words in the src
        e -> N2 == e -> N1 && // the src is the same size as the dest.
        e -> CN1 == 0 &&  // and it starts at a word boundary // BITNO?
        e -> CN2 == 0)
      {
        sim_debug (DBG_TRACEEXT, & cpu_dev, "MLR special case #1\n");
        for ( ; cpu.du.CHTALLY < e -> N2; cpu.du.CHTALLY += 4)
          {
            uint n = cpu.du.CHTALLY / 4;
            word36 w = EISReadIdx (& e -> ADDR1, n);
            EISWriteIdx (& e -> ADDR2, n, w, true);
          }
        cleanupOperandDescriptor (1);
        cleanupOperandDescriptor (2);
        // truncation fault check does need to be checked for here since 
        // it is known that N1 == N2
        CLR_I_TRUNC;
        return;
      }

// Test for the case of aligned word fill; and do things a word at a time,
// instead of a byte at a time...

#ifdef EIS_PTR3
    if (TA1 == CTA9 && // src and dst are both char 9
        TA2 == CTA9 &&
#else
    if (e -> TA1 == CTA9 && // src and dst are both char 9
        e -> TA2 == CTA9 &&
#endif
        e -> N1 == 0 && // the source is entirely fill
        e -> N2 % 4 == 0 && // a whole number of words in the dest
        e -> CN1 == 0 &&  // and it starts at a word boundary // BITNO?
        e -> CN2 == 0)
      {
        sim_debug (DBG_TRACEEXT, & cpu_dev, "MLR special case #2\n");
        word36 w = (word36) fill | ((word36) fill << 9) | ((word36) fill << 18) | ((word36) fill << 27);
        for ( ; cpu.du.CHTALLY < e -> N2; cpu.du.CHTALLY += 4)
          {
            uint n = cpu.du.CHTALLY / 4;
            EISWriteIdx (& e -> ADDR2, n, w, true);
          }
        cleanupOperandDescriptor (1);
        cleanupOperandDescriptor (2);
        // truncation fault check does need to be checked for here since 
        // it is known that N1 <= N2
        CLR_I_TRUNC;
        return;
      }

    for ( ; cpu.du.CHTALLY < min (e->N1, e->N2); cpu.du.CHTALLY ++)
      {
        word9 c = EISget469 (1, cpu.du.CHTALLY); // get src char
        word9 cout = 0;
        
#ifdef EIS_PTR3
        if (TA1 == TA2) 
#else
        if (e -> TA1 == e -> TA2) 
#endif
          EISput469 (2, cpu.du.CHTALLY, c);
        else
          {
            // If data types are dissimilar (TA1 ≠ TA2), each character is
            // high-order truncated or zero filled, as appropriate, as it is
            // moved. No character conversion takes place.
            cout = c;
            switch (srcSZ)
              {
                case 6:
                  switch(dstSZ)
                    {
                      case 4:
                        cout = c & 017;    // truncate upper 2-bits
                        break;
                      case 9:
                        break;              // should already be 0-filled
                    }
                  break;
                case 9:
                  switch(dstSZ)
                    {
                      case 4:
                        cout = c & 017;    // truncate upper 5-bits
                        break;
                      case 6:
                        cout = c & 077;    // truncate upper 3-bits
                        break;
                    }
                  break;
              }

            // If N1 < N2, C(FILL)0 = 1, TA1 = 1, and TA2 = 2 (6-4 move), then
            // C(Y-charn1)N1-1 is examined for a GBCD overpunch sign. If a
            // negative overpunch sign is found, then the minus sign character
            // is placed in C(Y-charn2)N2-1; otherwise, a plus sign character
            // is placed in C(Y-charn2)N2-1.
            
            if (ovp && (cpu.du.CHTALLY == e -> N1 - 1))
              {
                // C(FILL)0 = 1 means that there *is* an overpunch char here.
                // ISOLTS-838 01e, RJ78 p. 11-126
                isGBCDOvp (c, & isNeg);
              }
            EISput469 (2, cpu.du.CHTALLY, cout);
          }
      }
    
    // If N1 < N2, then for i = N1+1, N1+2, ..., N2
    //    C(FILL) → C(Y-charn2)N2-i
    // If N1 < N2 and TA2 = 2 (4-bit data) or 1 (6-bit data), then FILL
    // characters are high-order truncated as they are moved to C(Y-charn2). No
    // character conversion takes place.

    if (e -> N1 < e -> N2)
      {
        for ( ; cpu.du.CHTALLY < e -> N2 ; cpu.du.CHTALLY ++)
          {
            // if there's an overpunch then the sign will be the last of the fill
            if (ovp && (cpu.du.CHTALLY == e -> N2 - 1))
              {
                if (isNeg)
                  EISput469 (2, cpu.du.CHTALLY, 015); // 015 is decimal -
                else
                  EISput469 (2, cpu.du.CHTALLY, 014); // 014 is decimal +
              }
            else
              EISput469 (2, cpu.du.CHTALLY, fillT);
          }
    }
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);

    if (e -> N1 > e -> N2)
      {
        SET_I_TRUNC;
        if (T && ! TST_I_OMASK)
          doFault (FAULT_OFL, fst_zero, "mlr truncation fault");
      }
    else
      CLR_I_TRUNC;
  } 


void mrl (void)
  {
    EISstruct * e = & cpu.currentEISinstruction;

    // For i = 1, 2, ..., minimum (N1,N2)
    //     C(Y-charn1)N1-i → C(Y-charn2)N2-i
    // If N1 < N2, then for i = N1+1, N1+2, ..., N2
    //    C(FILL) → C(Y-charn2)N2-i
    // Indicators: Truncation. If N1 > N2 then ON; otherwise OFF
    
    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor (1, &mod_fault);
    setupOperandDescriptor (2, &mod_fault);
    //setupOperandDescriptorCache (3);
#endif
    
    parseAlphanumericOperandDescriptor(1, 1, false, &mod_fault);
    parseAlphanumericOperandDescriptor(2, 2, false, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bit 10 MBZ
    if (IWB_IRODD & 0000200000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "mrl 10 MBZ");

    // Bit 23 of OP1 MBZ
    if (!(e->MF[0] & MFkID) && e -> op [0]  & 0000000010000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "mrl op1 23 MBZ");

    // Bit 23 of OP2 MBZ
    if (!(e->MF[1] & MFkID) && e -> op [1]  & 0000000010000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "mrl op2 23 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    int srcSZ = 0, dstSZ = 0;

#ifdef EIS_PTR3
    switch (TA1)
#else
    switch (e -> TA1)
#endif
      {
        case CTA4:
          srcSZ = 4;
          break;
        case CTA6:
          srcSZ = 6;
          break;
        case CTA9:
          srcSZ = 9;
          break;
      }
    
#ifdef EIS_PTR3
    switch (TA2)
#else
    switch (e -> TA2)
#endif
      {
        case CTA4:
          dstSZ = 4;
          break;
        case CTA6:
          dstSZ = 6;
          break;
        case CTA9:
          dstSZ = 9;
          break;
      }
    
    word1 T = getbits36_1 (cpu.cu.IWB, 9);
    
    word9 fill = getbits36_9 (cpu.cu.IWB, 0);
    word9 fillT = fill;  // possibly truncated fill pattern

    // play with fill if we need to use it
    switch (dstSZ)
      {
        case 4:
          fillT = fill & 017;    // truncate upper 5-bits
          break;
        case 6:
          fillT = fill & 077;    // truncate upper 3-bits
          break;
      }
    
    // If N1 > N2, then (N1-N2) leading characters of C(Y-charn1) are not moved
    // and the truncation indicator is set ON.

    // If N1 < N2 and TA2 = 2 (4-bit data) or 1 (6-bit data), then FILL
    // characters are high-order truncated as they are moved to C(Y-charn2). No
    // character conversion takes place.

    // The user of string replication or overlaying is warned that the decimal
    // unit addresses the main memory in unaligned (not on modulo 8 boundary)
    // units of Y-block8 words and that the overlayed string, C(Y-charn2), is
    // not returned to main memory until the unit of Y-block8 words is filled or
    // the instruction completes.

    // If T = 1 and the truncation indicator is set ON by execution of the
    // instruction, then a truncation (overflow) fault occurs.

    // Attempted execution with the xed instruction causes an illegal procedure
    // fault.

    // Attempted repetition with the rpt, rpd, or rpl instructions causes an
    // illegal procedure fault.
    
#ifdef EIS_PTR3
    bool ovp = (e -> N1 < e -> N2) && (fill & 0400) && (TA1 == 1) &&
               (TA2 == 2); // (6-4 move)
#else
    bool ovp = (e -> N1 < e -> N2) && (fill & 0400) && (e -> TA1 == 1) &&
               (e -> TA2 == 2); // (6-4 move)
#endif
    bool isNeg = false;
    //bool bOvp = false;  // true when a negative overpunch character has been 
                        // found @ N1-1 

    PNL (L68_ (if (max (e->N1, e->N2) < 128)
      DU_CYCLE_FLEN_128;))

//
// Test for the case of aligned word move; and do things a word at a time,
// instead of a byte at a time...

#ifdef EIS_PTR3
    if (TA1 == CTA9 &&  // src and dst are both char 9
        TA2 == CTA9 &&
#else
    if (e -> TA1 == CTA9 &&  // src and dst are both char 9
        e -> TA2 == CTA9 &&
#endif
        e -> N1 % 4 == 0 &&  // a whole number of words in the src
        e -> N2 == e -> N1 && // the src is the same size as the dest.
        e -> CN1 == 0 &&  // and it starts at a word boundary // BITNO?
        e -> CN2 == 0)
      {
        sim_debug (DBG_TRACEEXT, & cpu_dev, "MRL special case #1\n");
        uint limit = e -> N2;
        for ( ; cpu.du.CHTALLY < limit; cpu.du.CHTALLY += 4)
          {
            uint n = (limit - cpu.du.CHTALLY - 1) / 4;
            word36 w = EISReadIdx (& e -> ADDR1, n);
            EISWriteIdx (& e -> ADDR2, n, w, true);
          }
        cleanupOperandDescriptor (1);
        cleanupOperandDescriptor (2);
        // truncation fault check does need to be checked for here since 
        // it is known that N1 == N2
        CLR_I_TRUNC;
        return;
      }

// Test for the case of aligned word fill; and do things a word at a time,
// instead of a byte at a time...

#ifdef EIS_PTR3
    if (TA1 == CTA9 && // src and dst are both char 9
        TA2 == CTA9 &&
#else
    if (e -> TA1 == CTA9 && // src and dst are both char 9
        e -> TA2 == CTA9 &&
#endif
        e -> N1 == 0 && // the source is entirely fill
        e -> N2 % 4 == 0 && // a whole number of words in the dest
        e -> CN1 == 0 &&  // and it starts at a word boundary // BITNO?
        e -> CN2 == 0)
      {
        sim_debug (DBG_TRACEEXT, & cpu_dev, "MRL special case #2\n");
        word36 w = (word36) fill |
                  ((word36) fill << 9) |
                  ((word36) fill << 18) |
                  ((word36) fill << 27);
        uint limit = e -> N2;
        for ( ; cpu.du.CHTALLY < e -> N2; cpu.du.CHTALLY += 4)
          {
            uint n = (limit - cpu.du.CHTALLY - 1) / 4;
            EISWriteIdx (& e -> ADDR2, n, w, true);
          }
        cleanupOperandDescriptor (1);
        cleanupOperandDescriptor (2);
        // truncation fault check does need to be checked for here since 
        // it is known that N1 <= N2
        CLR_I_TRUNC;
        return;
      }

    for ( ; cpu.du.CHTALLY < min (e -> N1, e -> N2); cpu.du.CHTALLY ++)
      {
        word9 c = EISget469 (1, e -> N1 - cpu.du.CHTALLY - 1); // get src char
        word9 cout = 0;
        
#ifdef EIS_PTR3
        if (TA1 == TA2) 
#else
        if (e -> TA1 == e -> TA2) 
#endif
          EISput469 (2, e -> N2 - cpu.du.CHTALLY - 1, c);
        else
          {
            // If data types are dissimilar (TA1 ≠ TA2), each character is
            // high-order truncated or zero filled, as appropriate, as it is
            // moved. No character conversion takes place.
            cout = c;
            switch (srcSZ)
              {
                case 6:
                  switch(dstSZ)
                    {
                      case 4:
                        cout = c & 017;    // truncate upper 2-bits
                        break;
                      case 9:
                        break;              // should already be 0-filled
                    }
                  break;
                case 9:
                  switch(dstSZ)
                    {
                      case 4:
                        cout = c & 017;    // truncate upper 5-bits
                        break;
                      case 6:
                        cout = c & 077;    // truncate upper 3-bits
                        break;
                    }
                  break;
              }

            // If N1 < N2, C(FILL)0 = 1, TA1 = 1, and TA2 = 2 (6-4 move), then
            // C(Y-charn1)N1-1 is examined for a GBCD overpunch sign. If a
            // negative overpunch sign is found, then the minus sign character
            // is placed in C(Y-charn2)N2-1; otherwise, a plus sign character
            // is placed in C(Y-charn2)N2-1.
            
// ISOLTS 838 01f, RJ78 p.11-154 - the rightmost digit is examined for overpunch.
            if (ovp && (cpu.du.CHTALLY == 0))
              {
                // C(FILL)0 = 1 means that there *is* an overpunch char here.
                isGBCDOvp (c, & isNeg);
              }
            EISput469 (2, e -> N2 - cpu.du.CHTALLY - 1, cout);
          }
      }
    
    // If N1 < N2, then for i = N1+1, N1+2, ..., N2
    //    C(FILL) → C(Y-charn2)N2-i
    // If N1 < N2 and TA2 = 2 (4-bit data) or 1 (6-bit data), then FILL
    // characters are high-order truncated as they are moved to C(Y-charn2). No
    // character conversion takes place.

    if (e -> N1 < e -> N2)
      {
        for ( ; cpu.du.CHTALLY < e -> N2 ; cpu.du.CHTALLY ++)
          {
            // if there's an overpunch then the sign will be the last of the fill
            if (ovp && (cpu.du.CHTALLY == e -> N2 - 1))
              {
                if (isNeg)
                  EISput469 (2, e -> N2 - cpu.du.CHTALLY - 1, 015); // 015 is decimal -
                else
                  EISput469 (2, e -> N2 - cpu.du.CHTALLY - 1, 014); // 014 is decimal +
              }
            else
              {
                 EISput469 (2, e -> N2 - cpu.du.CHTALLY - 1, fillT);
              }
          }
    }
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);

    if (e -> N1 > e -> N2)
      {
        SET_I_TRUNC;
        if (T && ! TST_I_OMASK)
          doFault (FAULT_OFL, fst_zero, "mrl truncation fault");
      }
    else
      CLR_I_TRUNC;
  } 

// decimalZero
//
//  Try 1:
//
// This makes MVE 1-28 and all of MVNE work
// Fails MVE 29-37 (6->6)
//    #define decimalZero (e->srcTA != CTA4 ? '0' : 0)
//
// Try 2
// This makes MVE 1-10 and 20-37 and all of MVNE work
// Fails MVE 11-19 (6->9)
//    #define decimalZero (e->srcTA == CTA9 ? '0' : 0)
//
// Try 4
//
#define isDecimalZero(c) ((e->srcTA == CTA9) ? \
                          ((c) == '0') : \
                          (((c) & 017) == 0)) 

/*
 * Load the entire sending string number (maximum length 63 characters) into
 * the decimal unit input buffer as 4-bit digits (high-order truncating 9-bit
 * data). Strip the sign and exponent characters (if any), put them aside into
 * special holding registers and decrease the input buffer count accordingly.
 */

static void EISloadInputBufferNumeric (int k)
{
    EISstruct * e = & cpu.currentEISinstruction;

    word9 *p = e->inBuffer; // p points to position in inBuffer where 4-bit chars are stored
    memset(e->inBuffer, 0, sizeof(e->inBuffer));   // initialize to all 0's

    int pos = (int) e->CN[k-1];

    int TN = (int) e->TN[k-1];
    int S = (int) e->S[k-1];  // This is where MVNE gets really nasty.
    // I spit on the designers of this instruction set (and of COBOL.) >Ptui!<

    int N = (int) e->N[k-1];  // number of chars in src string

    EISaddr *a = &e->addr[k-1];

    e->sign = 1;
    e->exponent = 0;

    for(int n = 0 ; n < N ; n += 1)
    {
        word9 c = EISget49(a, &pos, TN);
        sim_debug (DBG_TRACEEXT, & cpu_dev, "src: %d: %o\n", n, c);

        /*
         * Here we need to distinguish between 4 type of numbers.
         *
         * CSFL - Floating-point, leading sign
         * CSLS - Scaled fixed-point, leading sign
         * CSTS - Scaled fixed-point, trailing sign
         * CSNS - Scaled fixed-point, unsigned
         */
        switch(S)
        {
            case CSFL:  // this is the real evil one ....
                // Floating-point:
                // [sign=c0] c1×10(n-3) + c2×10(n-4) + ... + c(n-3) [exponent=8
                // bits]
                //
                // where:
                //
                //  ci is the decimal value of the byte in the ith byte
                //  position.
                //
                //  [sign=ci] indicates that ci is interpreted as a sign byte.
                //
                //  [exponent=8 bits] indicates that the exponent value is
                //  taken from the last 8 bits of the string. If the data is in
                //  9-bit bytes, the exponent is bits 1-8 of c(n-1). If the
                //  data is in 4- bit bytes, the exponent is the binary value
                //  of the concatenation of c(n-2) and c(n-1).
 
                if (n == 0) // first had better be a sign ....
                {
                    c &= 0xf;   // hack off all but lower 4 bits

                    if (c < 012 || c > 017)
                      doFault (FAULT_IPR,
                               fst_ill_dig,
                               "loadInputBufferNumeric(1): illegal char in "
                               "input");

                    if (c == 015)   // '-'
                        e->sign = -1;

                    e->srcTally -= 1;   // 1 less source char
                }
                else if (TN == CTN9 && n == N-1)    // the 9-bit exponent (of which only 8-bits are used)
                {
                    e->exponent = (signed char)(c & 0377); // want to do a sign extend
                    e->srcTally -= 1;   // 1 less source char
                }
                else if (TN == CTN4 && n == N-2)    // the 1st 4-chars of the 8-bit exponent
                {
                    e->exponent = (c & 0xf);// << 4;
                    e->exponent <<= 4;
                    e->srcTally -= 1;   // 1 less source char
                }
                else if (TN == CTN4 && n == N-1)    // the 2nd 4-chars of the 8-bit exponent
                {
                    e->exponent |= (c & 0xf);

                    signed char ce = (signed char) (e->exponent & 0xff);
                    e->exponent = ce;

                    e->srcTally -= 1;   // 1 less source char
                }
                else
                {
                    c &= 0xf;   // hack off all but lower 4 bits
                    if (c > 011)
                        doFault(FAULT_IPR, fst_ill_dig, "loadInputBufferNumeric(2): illegal char in input"); // TODO: generate ill proc fault

                    *p++ = c; // store 4-bit char in buffer
                }
                break;

            case CSLS:
                // Only the byte values [0,11]8 are legal in digit positions and only the byte values [12,17]8 are legal in sign positions. Detection of an illegal byte value causes an illegal procedure fault
                c &= 0xf;   // hack off all but lower 4 bits

                if (n == 0) // first had better be a sign ....
                {
                    if (c < 012 || c > 017)
                        doFault(FAULT_IPR, fst_ill_dig, "loadInputBufferNumeric(3): illegal char in input"); // TODO: generate ill proc fault

                    if (c == 015)   // '-'
                        e->sign = -1;
                    e->srcTally -= 1;   // 1 less source char
                }
                else
                {
                    if (c > 011)
                        doFault(FAULT_IPR, fst_ill_dig, "loadInputBufferNumeric(4): illegal char in input");
                    *p++ = c; // store 4-bit char in buffer
                }
                break;

            case CSTS:
                c &= 0xf;   // hack off all but lower 4 bits

                if (n == N-1) // last had better be a sign ....
                {
                    if (c < 012 || c > 017)
                         doFault(FAULT_IPR, fst_ill_dig, "loadInputBufferNumeric(5): illegal char in input");
                    if (c == 015)   // '-'
                        e->sign = -1;
                    e->srcTally -= 1;   // 1 less source char
                }
                else
                {
                    if (c > 011)
                        doFault(FAULT_IPR, fst_ill_dig, "loadInputBufferNumeric(6): illegal char in input");
                    *p++ = c; // store 4-bit char in buffer
                }
                break;

            case CSNS:
                c &= 0xf; // hack off all but lower 4 bits

                *p++ = c; // the "easy" one
                break;
        }
    }
    if_sim_debug (DBG_TRACEEXT, & cpu_dev)
      {
        sim_debug (DBG_TRACEEXT, & cpu_dev, "inBuffer:");
        for (word9 *q = e->inBuffer; q < p; q ++)
          sim_debug (DBG_TRACEEXT, & cpu_dev, " %02o", * q);
        sim_debug (DBG_TRACEEXT, & cpu_dev, "\n");
      }
}


/*
 * Load decimal unit input buffer with sending string characters. Data is read
 * from main memory in unaligned units (not modulo 8 boundary) of Y-block8
 * words. The number of characters loaded is the minimum of the remaining
 * sending string count, the remaining receiving string count, and 64.
 */

static void EISloadInputBufferAlphnumeric (int k)
  {
    EISstruct * e = & cpu.currentEISinstruction;
    // p points to position in inBuffer where 4-bit chars are stored
    word9 * p = e -> inBuffer;
    memset (e -> inBuffer, 0, sizeof (e -> inBuffer));// initialize to all 0's

    // minimum of the remaining sending string count, the remaining receiving
    // string count, and 64.
    // uint N = min3 (e -> N1, e -> N3, 64);
    // The above AL39 rule doesn't work with IGN and possibly other MOPs as
    // well. Workaround: Load all source, but max 63 characters (as RJ78
    // suggests).
    uint N = min (e-> N1, 63);

    for (uint n = 0 ; n < N ; n ++)
      {
        word9 c = EISget469 (k, n);
        * p ++ = c;
      }
}

static void EISwriteOutputBufferToMemory (int k)
  {
    EISstruct * e = & cpu.currentEISinstruction;

    for (uint n = 0 ; n < (uint) e -> dstTally; n ++)
      {
        word9 c49 = e -> outBuffer [n];
        EISput469 (k, n, c49);
      }
  }


static void writeToOutputBuffer (word9 **dstAddr, int szSrc, int szDst, word9 c49)
{
    EISstruct * e = & cpu.currentEISinstruction;
    // 4. If an edit insertion table entry or MOP insertion character is to be
    // stored, ANDed, or ORed into a receiving string of 4- or 6-bit
    // characters, high-order truncate the character accordingly.
    // 5. (MVNE) If the receiving string is 9-bit characters, high-order fill
    // the (4-bit) digits from the input buffer with bits 0-4 of character 8 of
    // the edit insertion table. If the receiving string is 6-bit characters,
    // high-order fill the digits with "00"b.

    if (e -> mvne)
      {
        switch (szSrc)   // XXX according to rule 1 of Numeric Edit, 
                         // input should be 4bits. this needs cleaning
          {
            case 4:
              switch (szDst)
                {
                  case 4:
                    ** dstAddr = c49 & 0xf;
                    break;
                  case 6:
                    ** dstAddr = c49 & 077;   // high-order fill the digits with "00"b.
                    break;
                  case 9:
                    ** dstAddr = c49 | (e -> editInsertionTable [7] & 0760);
                    break;
                }
              break;
            case 6:
              switch (szDst)
                {
                  case 4:
                    ** dstAddr = c49 & 0xf;    // write only-4-bits
                    break;
                  case 6:
                    ** dstAddr = c49; // XXX is this safe? shouldn't it be & 077?
                    break;
                  case 9:
                    ** dstAddr = c49;
                    break;
                }
              break;
            // Note: case szSrc == 9 is also used for writing edit table
            // entries and MOP insertion characters which shall NOT be
            // transformed by rule 5
            case 9:
              switch(szDst)
                {
                  case 4:
                    ** dstAddr = c49 & 0xf;    // write only-4-bits
                    break;
                  case 6:
                    ** dstAddr = c49 & 077;   // write only 6-bits
                    break;
                  case 9:
                    ** dstAddr = c49;
                    break;
                }
              break;
          }
      }
    else // mve
      {
        switch(szDst)
          {
            case 4:
              ** dstAddr = c49 & 0xf;    // write only-4-bits
              break;
            case 6:
              ** dstAddr = c49 & 077;   // write only 6-bits
              break;
            case 9:
              ** dstAddr = c49;
              break;
          }
      }
    e->dstTally -= 1;
    *dstAddr += 1; 
}

/*!
 * This is the Micro Operation Executor/Interpreter
 */

static char* defaultEditInsertionTable = " *+-$,.0";

// Edit Flags
//
// The processor provides the following four edit flags for use by the micro
// operations.  
//
// bool mopES = false; // End Suppression flag; initially OFF, set ON by a
// micro operation when zero-suppression ends.
//
// bool mopSN = false; 
// Sign flag; initially set OFF if the sending string has an alphanumeric
// descriptor or an unsigned numeric descriptor. If the sending string has a
// signed numeric descriptor, the sign is initially read from the sending
// string from the digit position defined by the sign and the decimal type
// field (S); SN is set OFF if positive, ON if negative. If all digits are
// zero, the data is assumed positive and the SN flag is set OFF, even when the
// sign is negative.
//
//bool mopBZ = false; i
// Blank-when-zero flag; initially set OFF and set ON by either the ENF or SES
// micro operation. If, at the completion of a move (L1 exhausted), both the Z
// and BZ flags are ON, the receiving string is filled with character 1 of the
// edit insertion table.

/*!
 * CHT Micro Operation - Change Table
 * EXPLANATION: The edit insertion table is replaced by the string of eight
 * 9-bit characters immediately following the CHT micro operation.
 * FLAGS: None affected
 * NOTE: C(IF) is not interpreted for this operation.
 */

static int mopCHT (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
    memset(&e->editInsertionTable, 0, sizeof(e->editInsertionTable)); // XXX do we really need this?
    for(int i = 0 ; i < 8 ; i += 1)
    {
        if (e->mopTally == 0)
        {
            e->_faults |= FAULT_IPR;
            break;
        }
#ifdef EIS_PTR2
        word9 entry = EISget49(&e->ADDR2, &e->mopPos, CTN9);  // get mop table entries
#else
        word9 entry = EISget49(e->mopAddress, &e->mopPos, CTN9);  // get mop table entries
#endif
        e->editInsertionTable[i] = entry & 0777;            // keep to 9-bits
        e->mopTally -= 1;
    }
    return 0;
}

/*!
 * ENF Micro Operation - End Floating Suppression
 * EXPLANATION:
 *  Bit 0 of IF, IF(0), specifies the nature of the floating suppression. Bit 1
 *  of IF, IF(1), specifies if blank when zero option is used.
 * For IF(0) = 0 (end floating-sign operation),
 * − If ES is OFF and SN is OFF, then edit insertion table entry 3 is moved to
 * the receiving field and ES is set ON.
 * − If ES is OFF and SN is ON, then edit insertion table entry 4 is moved to
 * the receiving field and ES is set ON.
 * − If ES is ON, no action is taken.
 * For IF(0) = 1 (end floating currency symbol operation),
 * − If ES is OFF, then edit insertion table entry 5 is moved to the receiving
 * field and ES is set ON.
 * − If ES is ON, no action is taken.
 * For IF(1) = 1 (blank when zero): the BZ flag is set ON. For IF(1) = 0 (no
 * blank when zero): no action is taken.
 * FLAGS: (Flags not listed are not affected)
 *      ES - If OFF, then set ON
 *      BZ - If bit 1 of C(IF) = 1, then set ON; otherwise, unchanged
 */

static int mopENF (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
    // For IF(0) = 0 (end floating-sign operation),
    if (!(e->mopIF & 010))
    {
        // If ES is OFF and SN is OFF, then edit insertion table entry 3 is moved to the receiving field and ES is set ON.
        if (!e->mopES && !e->mopSN)
        {
            writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[2]);
            e->mopES = true;
        }
        // If ES is OFF and SN is ON, then edit insertion table entry 4 is moved to the receiving field and ES is set ON.
        if (!e->mopES && e->mopSN)
        {
            writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[3]);
            e->mopES = true;
        }
        // If ES is ON, no action is taken.
    } else { // IF(0) = 1 (end floating currency symbol operation),
        if (!e->mopES)
        {
            // If ES is OFF, then edit insertion table entry 5 is moved to the receiving field and ES is set ON.
            writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[4]);
            e->mopES = true;
        }
        // If ES is ON, no action is taken.
    }
    
    // For IF(1) = 1 (blank when zero): the BZ flag is set ON. For IF(1) = 0 (no blank when zero): no action is taken.
    if (e->mopIF & 04)
        e->mopBZ = true;
    
    return 0;
}

/*!
 * IGN Micro Operation - Ignore Source Characters
 * EXPLANATION:
 * IF specifies the number of characters to be ignored, where IF = 0 specifies
 * 16 characters.
 * The next IF characters in the source data field are ignored and the sending
 * tally is reduced accordingly.
 * FLAGS: None affected
 */

static int mopIGN (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
// AL-39 dosen't specify the == 0 test, but NovaScale does;
// also ISOLTS ps830 test-04a seems to rely on it.
    if (e->mopIF == 0)
        e->mopIF = 16;

    for(int n = 0 ; n < e->mopIF ; n += 1)
    {
        if (e->dstTally == 0)
            break;
        if (e->srcTally == 0)
        {
            // IGN doesn't allow BZ, raise IPR straight away
            e->_faults |= FAULT_IPR;
            break;
        }
        
        e->srcTally -= 1;
        e->in += 1;
    }
    return 0;
}

/*!
 * INSA Micro Operation - Insert Asterisk on Suppression
 * EXPLANATION:
 * This MOP is the same as INSB except that if ES is OFF, then edit insertion
 * table entry 2 is moved to the receiving field.
 * FLAGS: None affected
 * NOTE: If C(IF) = 9-15, an IPR fault occurs.
 */

static int mopINSA (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
    // If C(IF) = 9-15, an IPR fault occurs.
    if (e->mopIF >= 9 && e->mopIF <= 15)
    {
        e->_faults |= FAULT_IPR;
        return 0;
    }
    
#if 1
    // If ES is OFF, then edit insertion table entry 1 is moved to the
    // receiving field. If IF = 0, then the next 9 bits are also skipped. If IF
    // is not 0, the next 9 bits are treated as a MOP.

    if (!e->mopES)
      {
        writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[1]);
           
        if (e->mopIF == 0)
          {
            if (e->mopTally == 0)
              {
                e->_faults |= FAULT_IPR;
                return 0;
              }
#ifdef EIS_PTR2
            EISget49(&e->ADDR2, &e->mopPos, CTN9);
#else
            EISget49(e->mopAddress, &e->mopPos, CTN9);
#endif
            e->mopTally -= 1;
          }
      }

    // If ES is ON and IF = 0, then the 9-bit character immediately following
    // the INSB micro-instruction is moved to the receiving field
    else
      {
        if (e->mopIF == 0)
          {
            if (e->mopTally == 0)
              {
                e->_faults |= FAULT_IPR;
                return 0;
              }
#ifdef EIS_PTR2
            word9 c = EISget49(&e->ADDR2, &e->mopPos, CTN9);
#else
            word9 c = EISget49(e->mopAddress, &e->mopPos, CTN9);
#endif
            writeToOutputBuffer(&e->out, 9, e->dstSZ, c);
            e->mopTally -= 1;
          }
    // If ES is ON and IF<>0, then IF specifies which edit insertion table
    // entry (1-8) is to be moved to the receiving field.
        else
          {
            writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[e->mopIF-1]);
          }
      }
#else
    // If IF = 0, the 9 bits immediately following the INSB micro operation are
    // treated as a 9-bit character (not a MOP) and are moved or skipped
    // according to ES.
    if (e->mopIF == 0)
    {
        // If ES is OFF, then edit insertion table entry 2 is moved to the
        // receiving field. If IF = 0, then the next 9 bits are also skipped.
        // If IF is not 0, the next 9 bits are treated as a MOP.
        if (!e->mopES)
        {
            writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[1]);
           
#ifdef EIS_PTR2
            EISget49(&e->ADDR2, &e->mopPos, CTN9);
#else
            EISget49(e->mopAddress, &e->mopPos, CTN9);
#endif
            e->mopTally -= 1;
        } else {
            // If ES is ON and IF = 0, then the 9-bit character immediately
            // following the INSB micro-instruction is moved to the receiving
            // field.
#if 1
#ifdef EIS_PTR2
            word9 c = EISget49(&e->ADDR2, &e->mopPos, CTN9);
#else
            word9 c = EISget49(e->mopAddress, &e->mopPos, CTN9);
#endif
            writeToOutputBuffer(&e->out, 9, e->dstSZ, c);
#else
#ifdef EIS_PTR2
            writeToOutputBuffer(&e->out, 9, e->dstSZ, EISget49(&e->ADDR2, &e->mopPos, CTN9));
#else
            writeToOutputBuffer(&e->out, 9, e->dstSZ, EISget49(e->mopAddress, &e->mopPos, CTN9));
#endif
#endif
            e->mopTally -= 1;
        }
        
    } else {
        // If ES is ON and IF<>0, then IF specifies which edit insertion table
        // entry (1-8) is to be moved to the receiving field.
        if (e->mopES)
        {
            writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[e->mopIF-1]);
        }
    }
#endif
    return 0;
}

/*!
 * INSB Micro Operation - Insert Blank on Suppression
 * EXPLANATION:
 * IF specifies which edit insertion table entry is inserted.
 * If IF = 0, the 9 bits immediately following the INSB micro operation are
 * treated as a 9-bit character (not a MOP) and are moved or skipped according
 * to ES.
 * − If ES is OFF, then edit insertion table entry 1 is moved to the receiving
 * field. If IF = 0, then the next 9 bits are also skipped. If IF is not 0, the
 * next 9 bits are treated as a MOP.
 * − If ES is ON and IF = 0, then the 9-bit character immediately following the
 * INSB micro-instruction is moved to the receiving field.
 * − If ES is ON and IF<>0, then IF specifies which edit insertion table entry
 * (1-8) is to be moved to the receiving field.
 * FLAGS: None affected
 * NOTE: If C(IF) = 9-15, an IPR fault occurs.
 */

static int mopINSB (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
    // If C(IF) = 9-15, an IPR fault occurs.
    if (e->mopIF >= 9 && e->mopIF <= 15)
    {
        e->_faults |= FAULT_IPR;
        return 0;
    }
    
    if (!e->mopES)
    {
        // If ES is OFF, then edit insertion table entry 1 is moved to the
        // receiving field. If IF = 0, then the next 9 bits are also skipped.
        // If IF is not 0, the next 9 bits are treated as a MOP.
        writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[0]);

        if (e->mopIF == 0)
        {
            if (e->mopTally == 0)
            {
                e->_faults |= FAULT_IPR;
                return 0;
            }
#ifdef EIS_PTR2
            EISget49(&e->ADDR2, &e->mopPos, CTN9);
#else
            EISget49(e->mopAddress, &e->mopPos, CTN9);
#endif
            e->mopTally -= 1;
        }

    } else {

        // ES is ON

        // If C(IF) != 0
        if (e->mopIF)
        {
            // If ES is ON and IF<>0, then IF specifies which edit
            // insertion table entry (1-8) is to be moved to the receiving
            // field.
            writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[e->mopIF - 1]);
        } else {
            // If ES is ON and IF = 0, then the 9-bit character immediately
            // following the INSB micro-instruction is moved to the
            // receiving field.
            if (e->mopTally == 0)
            {
                e->_faults |= FAULT_IPR;
                return 0;
            }
#ifdef EIS_PTR2
            writeToOutputBuffer(&e->out, 9, e->dstSZ, EISget49(&e->ADDR2, &e->mopPos, CTN9));
            //EISget49(&e->ADDR2, &e->mopPos, CTN9);
#else
            writeToOutputBuffer(&e->out, 9, e->dstSZ, EISget49(e->mopAddress, &e->mopPos, CTN9));
            //EISget49(e->mopAddress, &e->mopPos, CTN9);
#endif
            e->mopTally -= 1;

        }
    }
    return 0;
}

/*!
 * INSM Micro Operation - Insert Table Entry One Multiple
 * EXPLANATION:
 * IF specifies the number of receiving characters affected, where IF = 0
 * specifies 16 characters.
 * Edit insertion table entry 1 is moved to the next IF (1-16) receiving field
 * characters.
 * FLAGS: None affected
 */

static int mopINSM (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
    if (e->mopIF == 0)
        e->mopIF = 16;
    for(int n = 0 ; n < e->mopIF ; n += 1)
    {
        if (e->dstTally == 0)
          break;
        writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[0]);
    }
    return 0;
}

/*!
 * INSN Micro Operation - Insert on Negative
 * EXPLANATION:
 * IF specifies which edit insertion table entry is inserted. If IF = 0, the 9
 * bits immediately following the INSN micro operation are treated as a 9-bit
 * character (not a MOP) and are moved or skipped according to SN.
 * − If SN is OFF, then edit insertion table entry 1 is moved to the receiving
 * field. If IF = 0, then the next 9 bits are also skipped. If IF is not 0, the
 * next 9 bits are treated as a MOP.
 * − If SN is ON and IF = 0, then the 9-bit character immediately following the
 * INSN micro-instruction is moved to the receiving field.
 * − If SN is ON and IF <> 0, then IF specifies which edit insertion table
 * entry (1-8) is to be moved to the receiving field.
 * FLAGS: None affected
 * NOTE: If C(IF) = 9-15, an IPR fault occurs.
 */

static int mopINSN (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
    // If C(IF) = 9-15, an IPR fault occurs.
    if (e->mopIF >= 9 && e->mopIF <= 15)
    {
        e->_faults |= FAULT_IPR;
        return 0;
    }
    
    // If IF = 0, the 9 bits immediately following the INSN micro operation are
    // treated as a 9-bit character (not a MOP) and are moved or skipped
    // according to SN.
    
    if (e->mopIF == 0)
    {
        if (e->mopTally == 0)
        {
            e->_faults |= FAULT_IPR;
            return 0;
        }
      if (!e->mopSN)
        {
            //If SN is OFF, then edit insertion table entry 1 is moved to the
            //receiving field. If IF = 0, then the next 9 bits are also
            //skipped. If IF is not 0, the next 9 bits are treated as a MOP.
#ifdef EIS_PTR2
            EISget49(&e->ADDR2, &e->mopPos, CTN9);
#else
            EISget49(e->mopAddress, &e->mopPos, CTN9);
#endif
            writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[0]);
            e->mopTally -= 1;
        } else {
            // If SN is ON and IF = 0, then the 9-bit character immediately
            // following the INSN micro-instruction is moved to the receiving
            // field.
#ifdef EIS_PTR2
            writeToOutputBuffer(&e->out, 9, e->dstSZ, EISget49(&e->ADDR2, &e->mopPos, CTN9));
#else
            writeToOutputBuffer(&e->out, 9, e->dstSZ, EISget49(e->mopAddress, &e->mopPos, CTN9));
#endif

            e->mopTally -= 1;
        }
    }
    else
    {
        if (e->mopSN)
        {
            //If SN is ON and IF <> 0, then IF specifies which edit insertion
            //table entry (1-8) is to be moved to the receiving field.
            writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[e->mopIF - 1]);
        } else {
            writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[0]);
        }
    }
    return 0;
}

/*!
 * INSP Micro Operation - Insert on Positive
 * EXPLANATION:
 * INSP is the same as INSN except that the responses for the SN values are
 * reversed.
 * FLAGS: None affected
 * NOTE: If C(IF) = 9-15, an IPR fault occurs.
 */

static int mopINSP (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
    // If C(IF) = 9-15, an IPR fault occurs.
    if (e->mopIF >= 9 && e->mopIF <= 15)
    {
        e->_faults |= FAULT_IPR;
        return 0;
    }
    
    if (e->mopIF == 0)
    {
        if (e->mopTally == 0)
        {
            e->_faults |= FAULT_IPR;
            return 0;
        }
        if (e->mopSN)
        {
#ifdef EIS_PTR2
            EISget49(&e->ADDR2, &e->mopPos, CTN9);
#else
            EISget49(e->mopAddress, &e->mopPos, CTN9);
#endif
            writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[0]);
            e->mopTally -= 1;
        } else {
#ifdef EIS_PTR2
            writeToOutputBuffer(&e->out, 9, e->dstSZ, EISget49(&e->ADDR2, &e->mopPos, CTN9));
#else
            writeToOutputBuffer(&e->out, 9, e->dstSZ, EISget49(e->mopAddress, &e->mopPos, CTN9));
#endif
            e->mopTally -= 1;
        }
    }
    else
    {
        if (!e->mopSN)
        {
            writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[e->mopIF - 1]);
        } else {
            writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[0]);
        }
    }

    return 0;
}

/*!
 * LTE Micro Operation - Load Table Entry
 * EXPLANATION:
 * IF specifies the edit insertion table entry to be replaced.
 * The edit insertion table entry specified by IF is replaced by the 9-bit
 * character immediately following the LTE microinstruction.
 * FLAGS: None affected
 * NOTE: If C(IF) = 0 or C(IF) = 9-15, an Illegal Procedure fault occurs.
 */

static int mopLTE (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
    if (e->mopIF == 0 || (e->mopIF >= 9 && e->mopIF <= 15))
    {
        e->_faults |= FAULT_IPR;
        return 0;
    }
    if (e->mopTally == 0)
    {
        e->_faults |= FAULT_IPR;
        return 0;
    }
#ifdef EIS_PTR2
    word9 next = EISget49(&e->ADDR2, &e->mopPos, CTN9);
#else
    word9 next = EISget49(e->mopAddress, &e->mopPos, CTN9);
#endif
    e->mopTally -= 1;
    
    e->editInsertionTable[e->mopIF - 1] = next;
    sim_debug (DBG_TRACEEXT, & cpu_dev, "LTE IT[%d]<=%d\n", e -> mopIF - 1, next);    
    return 0;
}

/*!
 * MFLC Micro Operation - Move with Floating Currency Symbol Insertion
 * EXPLANATION:
 * IF specifies the number of characters of the sending field upon which the
 * operation is performed, where IF = 0 specifies 16 characters.
 * Starting with the next available sending field character, the next IF
 * characters are individually fetched and the following conditional actions
 * occur.
 * − If ES is OFF and the character is zero, edit insertion table entry 1 is
 * moved to the receiving field in place of the character.
 * − If ES is OFF and the character is not zero, then edit insertion table
 * entry 5 is moved to the receiving field, the character is also moved to the
 * receiving field, and ES is set ON.
 * − If ES is ON, the character is moved to the receiving field.
 * The number of characters placed in the receiving field is data-dependent. If
 * the entire sending field is zero, IF characters are placed in the receiving
 * field. However, if the sending field contains a nonzero character, IF+1
 * characters (the insertion character plus the characters from the sending
 * field) are placed in the receiving field.
 * An IPR fault occurs when the sending field is exhausted before the receiving
 * field is filled. In order to provide space in the receiving field for an
 * inserted currency symbol, the receiving field must have a string length one
 * character longer than the sending field. When the sending field is all
 * zeros, no currency symbol is inserted by the MFLC micro operation and the
 * receiving field is not filled when the sending field is exhausted. The user
 * should provide an ENF (ENF,12) micro operation after a MFLC micro operation
 * that has as its character count the number of characters in the sending
 * field. The ENF micro operation is engaged only when the MFLC micro operation
 * fails to fill the receiving field. Then it supplies a currency symbol to
 * fill the receiving field and blanks out the entire field.
 * FLAGS: (Flags not listed are not affected.)
 * ES If OFF and any of C(Y) is less than decimal zero, then ON; otherwise, it
 * is unchanged.
 * NOTE: Since the number of characters moved to the receiving string is
 * data-dependent, a possible IPR fault may be avoided by ensuring that the Z
 * and BZ flags are ON.
 */

static int mopMFLC (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
    if (e->mopIF == 0)
        e->mopIF = 16;

    //  Starting with the next available sending field character, the next IF
    //  characters are individually fetched and the following conditional
    //  actions occur.
    sim_debug (DBG_TRACEEXT, & cpu_dev, "MFLC IF %d, srcTally %d, dstTally %d\n", e->mopIF, e->srcTally, e->dstTally);
    for(int n = 0 ; n < e->mopIF ; n += 1)
    {
        sim_debug (DBG_TRACEEXT, & cpu_dev, "MFLC n %d, srcTally %d, dstTally %d\n", n, e->srcTally, e->dstTally);
        if (e->dstTally == 0)
            break;
        if (e->srcTally == 0)
            return -1;
        // If ES is OFF and the character is zero, edit insertion table entry 1
        // is moved to the receiving field in place of the character.
        // If ES is OFF and the character is not zero, then edit insertion
        // table entry 5 is moved to the receiving field, the character is also
        // moved to the receiving field, and ES is set ON.
        
        word9 c = *(e->in);
        sim_debug (DBG_TRACEEXT, & cpu_dev, "MFLC c %d (0%o)\n", c, c);
        if (!e->mopES) { // e->mopES is OFF


            sim_debug (DBG_TRACEEXT, & cpu_dev, "MFLC ES off\n");
            if (isDecimalZero (c)) {
                sim_debug (DBG_TRACEEXT, & cpu_dev, "MFLC is zero\n");
                // edit insertion table entry 1 is moved to the receiving field
                // in place of the character.
                writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[0]);
                e->in += 1;
                e->srcTally -= 1;
            } else {
                sim_debug (DBG_TRACEEXT, & cpu_dev, "MFLC is not zero\n");
                // then edit insertion table entry 5 is moved to the receiving
                // field, the character is also moved to the receiving field,
                // and ES is set ON.
                writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[4]);

                writeToOutputBuffer(&e->out, e->srcSZ, e->dstSZ, c);
                e->mopZ = false; // iszero() tested above.
                e->in += 1;
                e->srcTally -= 1;
                
                e->mopES = true;
            }
        } else {
            sim_debug (DBG_TRACEEXT, & cpu_dev, "MFLC ES on\n");
            // If ES is ON, the character is moved to the receiving field.
            writeToOutputBuffer(&e->out, e->srcSZ, e->dstSZ, c);
            
            if (! isDecimalZero (c))
                e->mopZ = false;
            e->in += 1;
            e->srcTally -= 1;
        }
    }

    return 0;
}

/*!
 * MFLS Micro Operation - Move with Floating Sign Insertion
 * EXPLANATION:
 * IF specifies the number of characters of the sending field upon which the
 * operation is performed, where IF = 0 specifies 16 characters.
 * Starting with the next available sending field character, the next IF
 * characters are individually fetched and the following conditional actions
 * occur.
 * − If ES is OFF and the character is zero, edit insertion table entry 1 is
 * moved to the receiving field in place of the character.
 * − If ES is OFF, the character is not zero, and SN is OFF; then edit
 * insertion table entry 3 is moved to the receiving field; the character is
 * also moved to the receiving field, and ES is set ON.
 * − If ES is OFF, the character is nonzero, and SN is ON; edit insertion table
 * entry 4 is moved to the receiving field; the character is also moved to the
 * receiving field, and ES is set ON.
 * − If ES is ON, the character is moved to the receiving field.
 * The number of characters placed in the receiving field is data-dependent. If
 * the entire sending field is zero, IF characters are placed in the receiving
 * field. However, if the sending field contains a nonzero character, IF+1
 * characters (the insertion character plus the characters from the sending
 * field) are placed in the receiving field.
 * An IPR fault occurs when the sending field is exhausted before the receiving
 * field is filled. In order to provide space in the receiving field for an
 * inserted sign, the receiving field must have a string length one character
 * longer than the sending field. When the sending field is all zeros, no sign
 * is inserted by the MFLS micro operation and the receiving field is not
 * filled when the sending field is exhausted. The user should provide an ENF
 * (ENF,4) micro operation after a MFLS micro operation that has as its
 * character count the number of characters in the sending field. The ENF micro
 * operation is engaged only when the MFLS micro operation fails to fill the
 * receiving field; then, it supplies a sign character to fill the receiving
 * field and blanks out the entire field.
 *
 * FLAGS: (Flags not listed are not affected.)
 *     ES If OFF and any of C(Y) is less than decimal zero, then ON; otherwise,
 *     it is unchanged.
 * NOTE: Since the number of characters moved to the receiving string is
 * data-dependent, a possible Illegal Procedure fault may be avoided by
 * ensuring that the Z and BZ flags are ON.
 */

static int mopMFLS (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
    if (e->mopIF == 0)
        e->mopIF = 16;
    
    for(int n = 0 ; n < e->mopIF; n += 1)
    {
        if (e->dstTally == 0)
            break;
        if (e->srcTally == 0)
            return -1;
        
        word9 c = *(e->in);
        sim_debug (DBG_TRACEEXT, & cpu_dev, "MFLS n %d c %o\n", n, c);
        if (!e->mopES) { // e->mopES is OFF
            if (isDecimalZero (c))
            {
                // edit insertion table entry 1 is moved to the receiving field
                // in place of the character.
                sim_debug (DBG_TRACEEXT, & cpu_dev, "ES is off, c is zero; edit insertion table entry 1 is moved to the receiving field in place of the character.\n");
                writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[0]);
                e->in += 1;
                e->srcTally -= 1;
            } else {
                // c is non-zero
                if (!e->mopSN)
                {
                    // then edit insertion table entry 3 is moved to the
                    // receiving field; the character is also moved to the
                    // receiving field, and ES is set ON.
                    sim_debug (DBG_TRACEEXT, & cpu_dev, "ES is off, c is non-zero, SN is off; edit insertion table entry 3 is moved to the receiving field; the character is also moved to the receiving field, and ES is set ON.\n");
                    writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[2]);

                    e->in += 1;
                    e->srcTally -= 1;
                    e->mopZ = false; // iszero tested above
#if 0
                    if (e->srcTally == 0 && e->dstTally > 1)
                    {
                        e->_faults |= FAULT_IPR;
                        return -1;
                    }
#endif
                    
                    writeToOutputBuffer(&e->out, e->srcSZ, e->dstSZ, c);

                    e->mopES = true;
                } else {
                    //  SN is ON; edit insertion table entry 4 is moved to the
                    //  receiving field; the character is also moved to the
                    //  receiving field, and ES is set ON.
                    sim_debug (DBG_TRACEEXT, & cpu_dev, "ES is off, c is non-zero, SN is OFF; edit insertion table entry 4 is moved to the receiving field; the character is also moved to the receiving field, and ES is set ON.\n");
                    writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[3]);
                    
                    e->in += 1;
                    e->srcTally -= 1;
                    e->mopZ = false; // iszero tested above
                    
                    writeToOutputBuffer(&e->out, e->srcSZ, e->dstSZ, c);

                    e->mopES = true;
                }
            }
        } else {
            // If ES is ON, the character is moved to the receiving field.
            sim_debug (DBG_TRACEEXT, & cpu_dev, "ES is ON, the character is moved to the receiving field.\n");
            writeToOutputBuffer(&e->out, e->srcSZ, e->dstSZ, c);

            if (! isDecimalZero (c))
                e->mopZ = false;
            e->in += 1;
            e->srcTally -= 1;
        }
    }
    
    // NOTE: Since the number of characters moved to the receiving string is
    // data-dependent, a possible Illegal Procedure fault may be avoided by
    // ensuring that the Z and BZ flags are ON.

    return 0;
}

/*!
 * MORS Micro Operation - Move and OR Sign
 * EXPLANATION:
 * IF specifies the number of characters of the sending field upon which the
 * operation is performed, where IF = 0 specifies 16 characters.
 * Starting with the next available sending field character, the next IF
 * characters are individually fetched and the following conditional actions
 * occur.
 * − If SN is OFF, the next IF characters in the source data field are moved to
 * the receiving data field and, during the move, edit insertion table entry 3
 * is ORed to each character.
 * − If SN is ON, the next IF characters in the source data field are moved to
 * the receiving data field and, during the move, edit insertion table entry 4
 * is ORed to each character.
 * MORS can be used to generate a negative overpunch for a receiving field to
 * be used later as a sending field.
 * FLAGS: None affected
 */

static int mopMORS (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
    if (e->mopIF == 0)
        e->mopIF = 16;
    
    sim_debug (DBG_TRACEEXT, & cpu_dev, "MORS mopIF %d src %d dst %d\n", e->mopIF, e->srcTally, e->dstTally);
    for(int n = 0 ; n < e->mopIF ; n += 1)
    {
// The micro operation sequence is terminated normally when the receiving
// string length becomes exhausted. The micro operation sequence is terminated
// abnormally (with an illegal procedure fault) if a move from an exhausted
// sending string or the use of an exhausted MOP string is attempted.

        if (e->dstTally == 0)
            break;
        if (e->srcTally == 0)
            return -1;
        
        // XXX this is probably wrong regarding the ORing, but it's a start ....
        word9 c = (*e->in | (!e->mopSN ? e->editInsertionTable[2] : e->editInsertionTable[3]));
        if (! isDecimalZero (*e->in))
            e->mopZ = false;
        e->in += 1;
        e->srcTally -= 1;
        
        writeToOutputBuffer(&e->out, e->srcSZ, e->dstSZ, c);
    }

    return 0;
}

/*!
 * MVC Micro Operation - Move Source Characters
 * EXPLANATION:
 * IF specifies the number of characters to be moved, where IF = 0 specifies 16
 * characters.
 * The next IF characters in the source data field are moved to the receiving
 * data field.
 * FLAGS: None affected
 */

static int mopMVC (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
    if (e->mopIF == 0)
        e->mopIF = 16;
    
    sim_debug (DBG_TRACEEXT, & cpu_dev, "MVC mopIF %d\n", e->mopIF);

    for(int n = 0 ; n < e->mopIF ; n += 1)
    {
        sim_debug (DBG_TRACEEXT, & cpu_dev, "MVC n %d srcTally %d dstTally %d\n", n, e->srcTally, e->dstTally);
// GD's test_float shows that data exhaustion is not a fault.
//#if 0
        if (e->dstTally == 0)
            break;
        if (e->srcTally == 0)
            return -1;
        
        sim_debug (DBG_TRACEEXT, & cpu_dev, "MVC write to output buffer %o\n", *e->in);
        writeToOutputBuffer(&e->out, e->srcSZ, e->dstSZ, *e->in);
        if (! isDecimalZero (*e->in))
            e->mopZ = false;
        e->in += 1;
        
        e->srcTally -= 1;
    }
    
    sim_debug (DBG_TRACEEXT, & cpu_dev, "MVC done\n");
    return 0;
}

/*!
 * MSES Micro Operation - Move and Set Sign
 * EXPLANATION:
 * IF specifies the number of characters of the sending field upon which the
 * operation is performed, where IF = 0 specifies 16 characters. For MVE,
 * starting with the next available sending field character, the next IF
 * characters are individually fetched and the following conditional actions
 * occur.
 * Starting with the first character during the move, a comparative AND is made
 * first with edit insertion table entry 3. If the result is nonzero, the first
 * character and the rest of the characters are moved without further
 * comparative ANDs. If the result is zero, a comparative AND is made between
 * the character being moved and edit insertion table entry 4 If that result is
 * nonzero, the SN indicator is set ON (indicating negative) and the first
 * character and the rest of the characters are moved without further
 * comparative ANDs. If the result is zero, the second character is treated
 * like the first. This process continues until one of the comparative AND
 * results is nonzero or until all characters are moved.
 * For MVNE instruction, the sign (SN) flag is already set and IF characters
 * are moved to the destination field (MSES is equivalent to the MVC
 * instruction).
 * FLAGS: (Flags not listed are not affected.)
 * SN If edit insertion table entry 4 is found in C(Y-1), then ON; otherwise,
 * it is unchanged.
 */

static int mopMSES (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
    if (e->mvne == true)
        return mopMVC ();   // XXX I think!
        
        
    if (e->mopIF == 0)
        e->mopIF = 16;

    int overpunch = false;
    
    for(int n = 0 ; n < e->mopIF ; n += 1)
    {
        if (e->dstTally == 0)
            break;
        if (e->srcTally == 0)
            return -1;

        //Starting with the first character during the move, a comparative AND
        //is made first with edit insertion table entry 3. If the result is
        //nonzero, the first character and the rest of the characters are moved
        //without further comparative ANDs. If the result is zero, a
        //comparative AND is made between the character being moved and edit
        //insertion table entry 4 If that result is nonzero, the SN indicator
        //is set ON (indicating negative) and the first character and the rest
        //of the characters are moved without further comparative ANDs. If the
        //result is zero, the second character is treated like the first. This
        //process continues until one of the comparative AND results is nonzero
        //or until all characters are moved.
        
        word9 c = *(e->in);

        if (!overpunch) {
            if (c & e->editInsertionTable[2])  // XXX only lower 4-bits are considered
                overpunch = true;
        
            else if (c & e->editInsertionTable[3])  // XXX only lower 4-bits are considered
            {
                e->mopSN = true;
                overpunch = true;
            }
        }

        e->in += 1;
        e->srcTally -= 1;   // XXX is this correct? No chars have been consumed, but ......
        if (! isDecimalZero (c))
            e->mopZ = false;
        writeToOutputBuffer(&e->out, e->srcSZ, e->dstSZ, c);
    }
    
    return 0;
}

/*!
 * MVZA Micro Operation - Move with Zero Suppression and Asterisk Replacement
 * EXPLANATION:
 * MVZA is the same as MVZB except that if ES is OFF and the character is zero,
 * then edit insertion table entry 2 is moved to the receiving field.
 * FLAGS: (Flags not listed are not affected.)
 * ES If OFF and any of C(Y) is less than decimal zero, then ON; otherwise, it
 * is unchanged.
 */

static int mopMVZA (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
    if (e->mopIF == 0)
        e->mopIF = 16;
    
    for(int n = 0 ; n < e->mopIF ; n += 1)
    {
        if (e->dstTally == 0)
            break;
        if (e->srcTally == 0)
            return -1;
        
        word9 c = *e->in;
        e->in += 1;
        e->srcTally -= 1;
        
        //if (!e->mopES && c == 0)
        // XXX See srcTA comment in MVNE
        if (!e->mopES && isDecimalZero (c))
        {
            //If ES is OFF and the character is zero, then edit insertion table
            //entry 2 is moved to the receiving field in place of the
            //character.
            writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[1]);
        //} else if (!e->mopES && c != 0)
        // XXX See srcTA comment in MVNE
        }
        else if ((! e->mopES) && (! isDecimalZero (c)))
        {
            //If ES is OFF and the character is not zero, then the character is
            //moved to the receiving field and ES is set ON.
            e->mopZ = false;
            writeToOutputBuffer(&e->out, e->srcSZ, e->dstSZ, c);

            e->mopES = true;
        } else if (e->mopES)
        {
            //If ES is ON, the character is moved to the receiving field.
            if (! isDecimalZero (c))
                e->mopZ = false;
            writeToOutputBuffer(&e->out, e->srcSZ, e->dstSZ, c);
        }
    }
    
    return 0;
}

/*!
 * MVZB Micro Operation - Move with Zero Suppression and Blank Replacement
 * EXPLANATION:
 * IF specifies the number of characters of the sending field upon which the
 * operation is performed, where IF = 0 specifies 16 characters.
 * Starting with the next available sending field character, the next IF
 * characters are individually fetched and the following conditional actions
 * occur.
 * − If ES is OFF and the character is zero, then edit insertion table entry 1
 * is moved to the receiving field in place of the character.
 * − If ES is OFF and the character is not zero, then the character is moved to
 * the receiving field and ES is set ON.
 * − If ES is ON, the character is moved to the receiving field. 
 * FLAGS: (Flags not listed are not affected.)
 *   ES If OFF and any of C(Y) is less than decimal zero, then ON; otherwise,
 *   it is unchanged.
 */

static int mopMVZB (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
    if (e->mopIF == 0)
        e->mopIF = 16;
    
    for(int n = 0 ; n < e->mopIF ; n += 1)
    {
        if (e->dstTally == 0)
            break;
        if (e->srcTally == 0)
          return -1;

        word9 c = *e->in;
        e->srcTally -= 1;
        e->in += 1;
        
        //if (!e->mopES && c == 0)
        // XXX See srcTA comment in MVNE
        if ((!e->mopES) && isDecimalZero (c))
        {
            //If ES is OFF and the character is zero, then edit insertion table
            //entry 1 is moved to the receiving field in place of the
            //character.
            writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[0]);
        //} else if (!e->mopES && c != 0)
        // XXX See srcTA comment in MVNE
        }
        if ((! e->mopES) && (! isDecimalZero (c)))
        {
            //If ES is OFF and the character is not zero, then the character is
            //moved to the receiving field and ES is set ON.
            e->mopZ = false;
            writeToOutputBuffer(&e->out, e->srcSZ, e->dstSZ, c);

            e->mopES = true;
        } else if (e->mopES)
        {
            //If ES is ON, the character is moved to the receiving field.
            if (! isDecimalZero (c))
                e->mopZ = false;
            writeToOutputBuffer(&e->out, e->srcSZ, e->dstSZ, c);
        }
    }

    return 0;
}

/*!
 * SES Micro Operation - Set End Suppression
 * EXPLANATION:
 * Bit 0 of IF (IF(0)) specifies the setting of the ES switch.
 * If IF(0) = 0, the ES flag is set OFF. If IF(0) = 1, the ES flag is set ON.
 * Bit 1 of IF (IF(1)) specifies the setting of the blank-when-zero option.
 * If IF(1) = 0, no action is taken.
 * If IF(1) = 1, the BZ flag is set ON.
 * FLAGS: (Flags not listed are not affected.)
 * ES set by this micro operation
 * BZ If bit 1 of C(IF) = 1, then ON; otherwise, it is unchanged.
 */

static int mopSES (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
    if (e->mopIF & 010)
        e->mopES = true;
    else
        e->mopES = false;
    
    if (e->mopIF & 04)
        e->mopBZ = true;
    
    return 0;
}

// Table 4-9. Micro Operation Code Assignment Map
#ifndef QUIET_UNUSED 
static char * mopCodes [040] =
  {
    //        0       1       2       3       4       5       6       7
    /* 00 */  0,     "insm", "enf",  "ses",  "mvzb", "mvza", "mfls", "mflc",
    /* 10 */ "insb", "insa", "insn", "insp", "ign",  "mvc",  "mses", "mors",
    /* 20 */ "lte",  "cht",   0,      0,      0,      0,      0,      0,
    /* 30 */   0,      0,     0,      0,      0,      0,      0,      0
  };
#endif


static MOP_struct mopTab[040] = {
    {NULL, 0},
    {"insm", mopINSM },
    {"enf",  mopENF  },
    {"ses",  mopSES  },
    {"mvzb", mopMVZB },
    {"mvza", mopMVZA },
    {"mfls", mopMFLS },
    {"mflc", mopMFLC },
    {"insb", mopINSB },
    {"insa", mopINSA },
    {"insn", mopINSN },
    {"insp", mopINSP },
    {"ign",  mopIGN  },
    {"mvc",  mopMVC  },
    {"mses", mopMSES },
    {"mors", mopMORS },
    {"lte",  mopLTE  },
    {"cht",  mopCHT  },
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0}
};


/*!
 * fetch MOP from e->mopAddr/e->mopPos ...
 */

static MOP_struct* EISgetMop (void)
{
    EISstruct * e = & cpu.currentEISinstruction;
    //static word18 lastAddress;  // try to keep memory access' down
    //static word36 data;
    
    
    if (e == NULL)
    //{
    //    p->lastAddress = -1;
    //    p->data = 0;
        return NULL;
    //}
   
#ifdef EIS_PTR2
    EISaddr *p = &e->ADDR2;
#else
    EISaddr *p = e->mopAddress;
#endif
    
    //if (p->lastAddress != p->address)                 // read from memory if different address
        p->data = EISRead(p);   // read data word from memory
    
    if (e->mopPos > 3)   // overflows to next word?
    {   // yep....
        e->mopPos = 0;   // reset to 1st byte
#ifdef EIS_PTR2
        cpu.du.Dk_PTR_W[KMOP] = (cpu.du.Dk_PTR_W[KMOP] + 1) & AMASK;     // bump source to next address
        p->data = EISRead(&e->ADDR2);   // read it from memory
#else
        PNL (cpu.du.Dk_PTR_W[1] = (cpu.du.Dk_PTR_W[1] + 1) & AMASK);     // bump source to next address
        PNL (p->data = EISRead(e->mopAddress));   // read it from memory
#ifdef EIS_PTR
        cpu.du.Dk_PTR_W[1] = (cpu.du.Dk_PTR_W[1] + 1) & AMASK;     // bump source to next address
        p->data = EISRead(e->mopAddress);   // read it from memory
#else
        e->mopAddress->address = (e->mopAddress->address + 1) & AMASK;     // bump source to next address
        p->data = EISRead(e->mopAddress);   // read it from memory
#endif
#endif
    }
    
    word9 mop9  = (word9) get9 (p -> data, e -> mopPos); // get 9-bit mop
    word5 mop   = (mop9 >> 4) & 037;
    e->mopIF = mop9 & 0xf;
    
    MOP_struct *m = &mopTab[mop];
    sim_debug (DBG_TRACEEXT, & cpu_dev, "MOP %s(%o) %o\n", m -> mopName, mop, e->mopIF);
    e->m = m;
    if (e->m == NULL || e->m->f == NULL)
    {
        sim_debug (DBG_TRACEEXT, & cpu_dev, "getMop(e->m == NULL || e->m->f == NULL): mop:%d IF:%d\n", mop, e->mopIF);
        return NULL;
    }
    
    e->mopPos += 1;
    e->mopTally -= 1;
    
    //p->lastAddress = p->address;
    
    return m;
}


#ifdef EIS_PTR2
static void mopExecutor (void)
#else
static void mopExecutor (int kMop)
#endif
  {
    EISstruct * e = & cpu.currentEISinstruction;
    PNL (L68_ (DU_CYCLE_FEXOP;))
#ifdef EIS_PTR2
    e->mopTally = (int) e->N[KMOP];        // number of micro-ops
    e->mopPos   = (int) e->CN[KMOP];        // starting at char pos CN
#else
    e->mopAddress = &e->addr[kMop-1];
    e->mopTally = (int) e->N[kMop-1];        // number of micro-ops
    e->mopPos   = (int) e->CN[kMop-1];        // starting at char pos CN
#endif
    
    word9 *p9 = e->editInsertionTable; // re-initilize edit insertion table
    char *q = defaultEditInsertionTable;
    while((*p9++ = (word9) (*q++)))
        ;
    
    e->in = e->inBuffer;    // reset input buffer pointer
    e->out = e->outBuffer;  // reset output buffer pointer
    
    e->_faults = 0; // No faults (yet!)
    
    // execute dstTally micro operations
    // The micro operation sequence is terminated normally when the receiving
    // string length becomes exhausted. The micro operation sequence is
    // terminated abnormally (with an illegal procedure fault) if a move from
    // an exhausted sending string or the use of an exhausted MOP string is
    // attempted.
    
    while (e->dstTally)
    {
        sim_debug (DBG_TRACEEXT, & cpu_dev, "mopExecutor srcTally %d dstTally %d mopTally %d\n", e->srcTally, e->dstTally, e->mopTally);
        MOP_struct *m = EISgetMop();
        if (! m)
          {
            sim_debug (DBG_TRACEEXT, & cpu_dev, "mopExecutor EISgetMop forced break\n");
            e->_faults |= FAULT_IPR;   // XXX ill proc fault
            break;        
          } 
        int mres = m->f();    // execute mop

        if (e->_faults & FAULT_IPR) // hard IPR raised by a MOP
            break;

        // RJ78 specifies "if at completion of a move (L1 exhausted)", AL39
        // doesn't define "completion of a move". 
        // But ISOLTS-845 asserts a case where L1 is NOT exhausted. Therefore I
        // assume "L2 exhausted" or "L1 or L2 exhausted" is the correct
        // interpretation. Both these options pass ISOLTS-845.
        // "L3 exhausted" is also an option but unlikely since that would fire
        // the BZ check even upon normal termination.
        // XXX DH03 7-295 suggests that there might be a difference between MVE
        // and MVNE. It might well be that DPS88/9000 behaves differently than
        // DPS8.

#if 0
        // delayed (L2) srcTally test
        if (e->mopTally == 0)
          {
            sim_debug (DBG_TRACEEXT, & cpu_dev, "mopExecutor N2 exhausted\n");
#else
        // immediate (L1 or L2) srcTally test
        // perhaps more adherent to documentation
        if (e->mopTally == 0 || mres)
          {
            sim_debug (DBG_TRACEEXT, & cpu_dev,
                       "mopExecutor N1 or N2 exhausted\n");
#endif
            if (e->mopZ && e->mopBZ)
              {
                e->out = e->outBuffer;  // reset output buffer pointer
                e->dstTally = (int) e->N3;  // number of chars in dst (max 63)
                while (e->dstTally)
                  {
                    writeToOutputBuffer(&e->out, 9, e->dstSZ, e->editInsertionTable[0]);
                  }
              }
            else if (mres || e->dstTally)
              { // N1 or N2 exhausted and BZ wasn't enabled
                e->_faults |= FAULT_IPR;
              } // otherwise normal termination
            break;        
          }
    }
    
    sim_debug (DBG_TRACEEXT, & cpu_dev, "mop faults %o src %d dst %d mop %d\n", e->_faults, e->srcTally, e->dstTally, e->mopTally);

//"The micro-operation sequence is terminated normally when the receiving string
// length is exhausted. The micro-operation sequence is terminated abnormally (with
// an IPR fault) if an attempt is made to move from an exhausted sending string or to
// use an exhausted MOP string.

// ISOLTS 845 is happy with no check at all
#if 0
// ISOLTS ps841
// testing for ipr fault when micro-op tally runs out
// prior to the sending or receiving field tally.
// ISOLTS ps830 test-04a
//  mopTally 0 srcTally 32 dstTally 0 is not a fault
    //if (e->mopTally < e->srcTally || e->mopTally < e->dstTally)
    if (e->mopTally < e->dstTally)
      {
        sim_debug (DBG_TRACEEXT, & cpu_dev, "mop executor IPR fault; mopTally %d srcTally %d dstTally %d\n", e->mopTally, e->srcTally, e->dstTally);
        e->_faults |= FAULT_IPR;   // XXX ill proc fault
      }
#endif
#if 0
    // dst string not exhausted?
    if (e->dstTally != 0)
      {
        e->_faults |= FAULT_IPR;   // XXX ill proc fault
      }
#endif

#if 0
    // mop string not exhausted?
    if (e->mopTally != 0)
      {
        sim_debug (DBG_TRACEEXT, & cpu_dev, "mop executor IPR fault; mopTally %d\n", e->mopTally);
        e->_faults |= FAULT_IPR;   // XXX ill proc fault
      }
#endif
 
#if 0
    // src string not exhausted?
    if (e->srcTally != 0)
      {
        e->_faults |= FAULT_IPR;   // XXX ill proc fault
      }
#endif

    if (e -> _faults)
      doFault (FAULT_IPR, fst_ill_proc, "mopExecutor");
}


void mve (void)
  {
    EISstruct * e = & cpu.currentEISinstruction;

    sim_debug(DBG_TRACEEXT, & cpu_dev, "mve\n");

    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor(1, &mod_fault);
    setupOperandDescriptor(2, &mod_fault);
    setupOperandDescriptor(3, &mod_fault);
#endif
    
    parseAlphanumericOperandDescriptor(1, 1, false, &mod_fault);
    parseAlphanumericOperandDescriptor(2, 2, false, &mod_fault);
    parseAlphanumericOperandDescriptor(3, 3, false, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bits 0, 1, 9, and 10 MBZ
    // According to RJ78, bit 9 is T, but is not mentioned in the text.
    if (IWB_IRODD & 0600600000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "mve: 0, 1, 9, 10 MBZ");

    // Bit 23 of OP1 MBZ
    if (!(e->MF[0] & MFkID) && e -> op [0]  & 0000000010000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "mve op1 23 MBZ");

#if 0
    // Bits 21-23 of OP1 MBZ
    if (!(e->MF[1] & MFkID) && e -> op [1]  & 0000000070000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "mve op2 21-23 MBZ");
#endif
    // only bit 23 according to RH03. this was fixed in DPS9000
    if (!(e->MF[1] & MFkID) && e -> op [1]  & 0000000010000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "mve op2 23 MBZ");

    // Bit 23 of OP3 MBZ
    if (!(e->MF[2] & MFkID) && e -> op [2]  & 0000000010000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "mve op3 23 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    // initialize mop flags. Probably best done elsewhere.
    e->mopES = false; // End Suppression flag
    e->mopSN = false; // Sign flag
    e->mopBZ = false; // Blank-when-zero flag
    e->mopZ  = true;  // Zero flag
    
    e->srcTally = (int) e->N1;  // number of chars in src (max 63)
    e->dstTally = (int) e->N3;  // number of chars in dst (max 63)
    
#ifdef EIS_PTR3
    e->srcTA = (int) TA1;    // type of chars in src
#else
    e->srcTA = (int) e->TA1;    // type of chars in src
#endif

    switch (e -> srcTA)
      {
        case CTA4:
          e -> srcSZ = 4;
          break;
        case CTA6:
          e -> srcSZ = 6;
          break;
        case CTA9:
          e -> srcSZ = 9;
          break;
      }
    
#ifdef EIS_PTR3
    uint dstTA = TA3;    // type of chars in dst
#else
    uint dstTA = e -> TA3;    // type of chars in dst
#endif

    switch (dstTA)
      {
        case CTA4:
          e -> dstSZ = 4;
          break;
        case CTA6:
          e -> dstSZ = 6;
          break;
        case CTA9:
          e -> dstSZ = 9;
          break;
      }
    
    // 1. load sending string into inputBuffer
    EISloadInputBufferAlphnumeric (1);   // according to MF1
    
    // 2. Execute micro operation string, starting with first (4-bit) digit.
    e -> mvne = false;
    
#ifdef EIS_PTR2
    mopExecutor ();
#else
    mopExecutor (2);
#endif
    
    e -> dstTally = (int) e -> N3;  // restore dstTally for output
    
    EISwriteOutputBufferToMemory (3);
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    cleanupOperandDescriptor (3);
  }

void mvne (void)
  {
    EISstruct * e = & cpu.currentEISinstruction;

    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor (1, &mod_fault);
    setupOperandDescriptor (2, &mod_fault);
    setupOperandDescriptor (3, &mod_fault);
#endif
    
    parseNumericOperandDescriptor (1, &mod_fault);
    parseAlphanumericOperandDescriptor (2, 2, false, &mod_fault);
    parseAlphanumericOperandDescriptor (3, 3, false, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bits 0, 1, 9, and 10 MBZ
    if (IWB_IRODD & 0600600000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "mvne: 0, 1, 9, 10 MBZ");


    // Bit 24-29 of OP1 MBZ
    // Multics has been observed to use 600162017511, cf RJ78
    //if (!(e->MF[0] & MFkID) && e -> op [0]  & 0000000007700)
      //doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "mvne op1 24-29 MBZ");

#if 0
    // Bits 21-29 of OP1 MBZ
    if (!(e->MF[1] & MFkID) && e -> op [1]  & 0000000077700)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "mvne op2 21-29 MBZ");
#endif
    // only bits 21-23 according to RJ78, maybe even less on DPS8
    if (!(e->MF[1] & MFkID) && e -> op [1]  & 0000000070000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "mvne op2 21-23 MBZ");

#if 0
    // Bits 23-29 of OP3 MBZ
    if (!(e->MF[2] & MFkID) && e -> op [2]  & 0000000017700)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "mvne op3 23-29 MBZ");
#endif
    // only bit 23 according to RJ78
    if (!(e->MF[2] & MFkID) && e -> op [2]  & 0000000010000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "mvne op3 23 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    uint srcTN = e -> TN1;    // type of chars in src

    int n1 = 0;
    
    /*
     * Here we need to distinguish between 4 type of numbers.
     *
     * CSFL - Floating-point, leading sign
     * CSLS - Scaled fixed-point, leading sign
     * CSTS - Scaled fixed-point, trailing sign
     * CSNS - Scaled fixed-point, unsigned
     */
    
    // determine precision
    switch(e->S1)
    {
        case CSFL:
            n1 = (int) e->N1 - 1; // need to account for the - sign
            if (srcTN == CTN4)
                n1 -= 2;    // 2 4-bit digits exponent
            else
                n1 -= 1;    // 1 9-bit digit exponent
            break;
        
        case CSLS:
        case CSTS:
            n1 = (int) e->N1 - 1; // only 1 sign
            break;
        
        case CSNS:
            n1 = (int) e->N1;     // no sign
            break;  // no sign wysiwyg
    }

    if (n1 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "mvne adjusted n1<1");

    // Putting this check in pAOD breaks Multics boot
    // From DH03 it seems that DPS8 does not check this explicitly, but L2 exhaust occurs which raises IPR anyway
    // So we may as well keep it here.
    if (e->N[1] == 0)
      doFault (FAULT_IPR, fst_ill_proc, "mvne N2 0");

    // this is a flaw in DPS8/70 which was corrected in DPS88 and later
    // ISOLTS-841 07h, RH03 p.7-295
    if (e->N[2] == 0)
      doFault (FAULT_IPR, fst_ill_proc, "mvne N3 0");

//if ((e -> op [0]  & 0000000007700) ||
//    (e -> op [1]  & 0000000077700) ||
//    (e -> op [2]  & 0000000017700))
//sim_printf ("%012"PRIo64"\n%012"PRIo64"\n%012"PRIo64"\n%012"PRIo64"\n", cpu.cu.IWB, e->op[0], e->op[1], e-> op[2]);
//if (e -> op [0]  & 0000000007700) sim_printf ("op1\n");
//if (e -> op [1]  & 0000000077700) sim_printf ("op2\n");
//if (e -> op [2]  & 0000000017700) sim_printf ("op3\n");
//000140  aa  100 004 024 500   mvne      (pr),(ic),(pr)
//000141  aa  6 00162 01 7511   desc9ls   pr6|114,9,-3
//000142  aa   000236 00 0007   desc9a    158,7               000376 = 403040144040
//000143  aa  6 00134 00 0012   desc9a    pr6|92,10           vcpu
//
// The desc8ls is sign-extending the -3.




    // initialize mop flags. Probably best done elsewhere.
    e->mopES = false; // End Suppression flag
    e->mopSN = false; // Sign flag
    e->mopBZ = false; // Blank-when-zero flag
    e->mopZ  = true;  // Zero flag
    
    e -> srcTally = (int) e -> N1;  // number of chars in src (max 63)
    e -> dstTally = (int) e -> N3;  // number of chars in dst (max 63)
    
// XXX Temp hack to get MOP to work. Merge TA/TN?
// The MOP operators look at srcTA to make 9bit/not 9-bit decisions about
// the contents of inBuffer; parseNumericOperandDescriptor() always puts
// 4 bit data in inBuffer, so signal the MOPS code of that.
    e->srcTA = CTA4;    // type of chars in src

    switch(srcTN)
    {
        case CTN4:
            //e->srcAddr = e->YChar41;
            e->srcSZ = 4;   // stored as 4-bit decimals
            break;
        case CTN9:
            //e->srcAddr = e->YChar91;
            e->srcSZ = 4;   // 'cause everything is stored as 4-bit decimals
            break;
    }

#ifdef EIS_PTR3
    uint dstTA = TA3;    // type of chars in dst
#else
    uint dstTA = e->TA3;    // type of chars in dst
#endif
    switch(dstTA)
    {
        case CTA4:
            //e->dstAddr = e->YChar43;
            e->dstSZ = 4;
            break;
        case CTA6:
            //e->dstAddr = e->YChar63;
            e->dstSZ = 6;
            break;
        case CTA9:
            //e->dstAddr = e->YChar93;
            e->dstSZ = 9;
            break;
    }

#ifdef EIS_PTR3
    sim_debug (DBG_TRACEEXT, & cpu_dev,
      "mvne N1 %d N2 %d N3 %d TN1 %d CN1 %d TA3 %d CN3 %d\n",
      e->N1, e->N2, e->N3, e->TN1, e->CN1, TA3, e->CN3);
#else
    sim_debug (DBG_TRACEEXT, & cpu_dev,
      "mvne N1 %d N2 %d N3 %d TN1 %d CN1 %d TA3 %d CN3 %d\n",
      e->N1, e->N2, e->N3, e->TN1, e->CN1, e->TA3, e->CN3);
#endif

    // 1. load sending string into inputBuffer
    EISloadInputBufferNumeric (1);   // according to MF1
    
    // 2. Test sign and, if required, set the SN flag. (Sign flag; initially
    // set OFF if the sending string has an alphanumeric descriptor or an
    // unsigned numeric descriptor. If the sending string has a signed numeric
    // descriptor, the sign is initially read from the sending string from the
    // digit position defined by the sign and the decimal type field (S); SN is
    // set OFF if positive, ON if negative. If all digits are zero, the data is
    // assumed positive and the SN flag is set OFF, even when the sign is
    // negative.)

    int sum = 0;
    for(int n = 0 ; n < e -> srcTally ; n ++)
        sum += e -> inBuffer [n];
    if ((e -> sign == -1) && sum)
        e -> mopSN = true;
    
    // 3. Execute micro operation string, starting with first (4-bit) digit.
    e -> mvne = true;
    
#ifdef EIS_PTR2
    mopExecutor ();
#else
    mopExecutor (2);
#endif

    e -> dstTally = (int) e -> N3;  // restore dstTally for output
    
    EISwriteOutputBufferToMemory (3);
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    cleanupOperandDescriptor (3);
  }

/*  
 * MVT - Move Alphanumeric with Translation
 */

void mvt (void)
  {
    EISstruct * e = & cpu.currentEISinstruction;

    // For i = 1, 2, ..., minimum (N1,N2)
    //    m = C(Y-charn1)i-1
    //    C(Y-char93)m → C(Y-charn2)i-1
    // If N1 < N2, then for i = N1+1, N1+2, ..., N2
    //    m = C(FILL)
    //    C(Y-char93)m → C(Y-charn2)i-1
    
    // Indicators: Truncation. If N1 > N2 then ON; otherwise OFF
    
    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor (1, &mod_fault);
    setupOperandDescriptor (2, &mod_fault);
    setupOperandDescriptorCache (3);
#endif
    
    parseAlphanumericOperandDescriptor (1, 1, false, &mod_fault);
    parseAlphanumericOperandDescriptor (2, 2, false, &mod_fault);
    parseArgOperandDescriptor (3, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

// ISOLTS 808 test-03b sets bit 0, 1    
// ISOLTS 808 test-03b sets bit 0, 1, 9
#if 1
    // Bits 10 MBZ 
    if (IWB_IRODD & 0000200000000)
      {
        //sim_printf ("mvt %012"PRIo64"\n", IWB_IRODD);
        doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "mvt 10 MBZ");
      }
#else
    // Bits 0,1,9,10 MBZ 
    if (IWB_IRODD & 0600600000000)
      {
        //sim_printf ("mvt %012"PRIo64"\n", IWB_IRODD);
        doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "mvt 0,1,9,10 MBZ");
      }
#endif

    // Bit 23 of OP1 MBZ
    if (!(e->MF[0] & MFkID) && e -> op [0]  & 0000000010000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "mvt op1 23 MBZ");

// This breaks eis_tester mvt 110
#if 0
    // Bits 18 of OP2 MBZ
    if (!(e->MF[1] & MFkID) && e -> op [1]  & 0000000400000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "mvt op2 18 MBZ");
#endif

    // Bits 18-28 of OP3 MBZ
    if (!(e->MF[2] & MFkID) && e -> op [2]  & 0000000777600)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "mvt op3 18-28 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

#ifdef EIS_PTR3
    e->srcTA = (int) TA1;
    uint dstTA = TA2;
    
    switch (TA1)
#else
    e->srcTA = (int) e->TA1;
    uint dstTA = e->TA2;
    
    switch (e -> TA1)
#endif
      {
        case CTA4:
          e -> srcSZ = 4;
          break;
        case CTA6:
          e -> srcSZ = 6;
          break;
        case CTA9:
          e -> srcSZ = 9;
         break;
      }
    
#ifdef EIS_PTR3
    switch (TA2)
#else
    switch (e -> TA2)
#endif
      {
        case CTA4:
          e -> dstSZ = 4;
          break;
        case CTA6:
          e -> dstSZ = 6;
          break;
        case CTA9:
          e -> dstSZ = 9;
          break;
      }
    
    //  Prepage Check in a Multiword Instruction
    //  The MVT, TCT, TCTR, and CMPCT instruction have a prepage check. The
    //  size of the translate table is determined by the TA1 data type as shown
    //  in the table below. Before the instruction is executed, a check is made
    //  for allocation in memory for the page for the translate table. If the
    //  page is not in memory, a Missing Page fault occurs before execution of
    //  the instruction. (Bull RJ78 p.7-75)
        
    // TA1              TRANSLATE TABLE SIZE
    // 4-BIT CHARACTER      4 WORDS
    // 6-BIT CHARACTER     16 WORDS
    // 9-BIT CHARACTER    128 WORDS
    
    uint xlatSize = 0;   // size of xlation table in words .....
#ifdef EIS_PTR3
    switch(TA1)
#else
    switch(e->TA1)
#endif
    {
        case CTA4:
            xlatSize = 4;
            break;
        case CTA6:
            xlatSize = 16;
            break;
        case CTA9:
            xlatSize = 128;
            break;
    }
    
#if 0    
    word36 xlatTbl[128];
    memset(xlatTbl, 0, sizeof(xlatTbl));    // 0 it out just in case
    
    // XXX here is where we probably need to to the prepage thang...
    EISReadN(&e->ADDR3, xlatSize, xlatTbl);
#endif

    // ISOLTS 878 01c - op1 and xlate table are prepaged, in that order
    // prepage op1
    int lastpageidx = ((int)e->N1 + (int)e->CN1 -1) / e->srcSZ;
    if (lastpageidx>0)
        EISReadIdx(&e->ADDR1, (uint)lastpageidx);
    // prepage xlate table
    EISReadIdx(&e->ADDR3, 0);
    EISReadIdx(&e->ADDR3, xlatSize-1);
    
    word1 T = getbits36_1 (cpu.cu.IWB, 9);
    
    word9 fill = getbits36_9 (cpu.cu.IWB, 0);
    word9 fillT = fill;  // possibly truncated fill pattern
    // play with fill if we need to use it
    switch(e->srcSZ)
    {
        case 4:
            fillT = fill & 017;    // truncate upper 5-bits
            break;
        case 6:
            fillT = fill & 077;    // truncate upper 3-bits
            break;
    }
    
    sim_debug (DBG_TRACEEXT, & cpu_dev, 
      "%s srcCN:%d dstCN:%d srcSZ:%d dstSZ:%d T:%d fill:%03o/%03o N1:%d N2:%d\n",
      __func__, e -> CN1, e -> CN2, e -> srcSZ, e -> dstSZ, T,
      fill, fillT, e -> N1, e -> N2);

    PNL (L68_ (if (max (e->N1, e->N2) < 128)
      DU_CYCLE_FLEN_128;))

    for ( ; cpu.du.CHTALLY < min(e->N1, e->N2); cpu.du.CHTALLY ++)
    {
        word9 c = EISget469(1, cpu.du.CHTALLY); // get src char
        int cidx = 0;
    
#ifdef EIS_PTR3
        if (TA1 == TA2)
#else
        if (e->TA1 == e->TA2)
#endif
            EISput469(2, cpu.du.CHTALLY, xlate (&e->ADDR3, dstTA, c));
        else
        {
            // If data types are dissimilar (TA1 ≠ TA2), each character is high-order truncated or zero filled, as appropriate, as it is moved. No character conversion takes place.
            cidx = c;
            
            word9 cout = xlate(&e->ADDR3, dstTA, (uint) cidx);

//            switch(e->dstSZ)
//            {
//                case 4:
//                    cout &= 017;    // truncate upper 5-bits
//                    break;
//                case 6:
//                    cout &= 077;    // truncate upper 3-bits
//                    break;
//            }

            switch (e->srcSZ)
            {
                case 6:
                    switch(e->dstSZ)
                    {
                        case 4:
                            cout &= 017;    // truncate upper 2-bits
                            break;
                        case 9:
                            break;              // should already be 0-filled
                    }
                    break;
                case 9:
                    switch(e->dstSZ)
                    {
                        case 4:
                            cout &= 017;    // truncate upper 5-bits
                            break;
                        case 6:
                            cout &= 077;    // truncate upper 3-bits
                            break;
                    }
                    break;
            }
            
            EISput469 (2, cpu.du.CHTALLY, cout);
        }
    }
    
    // If N1 < N2, then for i = N1+1, N1+2, ..., N2
    //    m = C(FILL)
    //    C(Y-char93)m → C(Y-charn2)N2-i
    
    if (e->N1 < e->N2)
    {
        word9 cfill = xlate(&e->ADDR3, dstTA, fillT);
        switch (e->srcSZ)
        {
            case 6:
                switch(e->dstSZ)
                {
                    case 4:
                        cfill &= 017;    // truncate upper 2-bits
                        break;
                    case 9:
                        break;              // should already be 0-filled
                }
                break;
            case 9:
                switch(e->dstSZ)
                {
                    case 4:
                        cfill &= 017;    // truncate upper 5-bits
                        break;
                    case 6:
                        cfill &= 077;    // truncate upper 3-bits
                        break;
                }
                break;
        }
        
        for( ; cpu.du.CHTALLY < e->N2 ; cpu.du.CHTALLY ++)
            EISput469 (2, cpu.du.CHTALLY, cfill);
    }

    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    cleanupOperandDescriptor (3);

    if (e->N1 > e->N2)
      {
        SET_I_TRUNC;
        if (T && ! TST_I_OMASK)
          doFault(FAULT_OFL, fst_zero, "mvt truncation fault");
      }
    else
      CLR_I_TRUNC;
  }


/*
 * cmpn - Compare Numeric
 */

void cmpn (void)
{
    EISstruct * e = & cpu.currentEISinstruction;

    // C(Y-charn1) :: C(Y-charn2) as numeric values
    
    // Zero If C(Y-charn1) = C(Y-charn2), then ON; otherwise OFF
    // Negative If C(Y-charn1) > C(Y-charn2), then ON; otherwise OFF
    // Carry If | C(Y-charn1) | > | C(Y-charn2) | , then OFF, otherwise ON
    
    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor(1, &mod_fault);
    setupOperandDescriptor(2, &mod_fault);
#endif
    
    parseNumericOperandDescriptor(1, &mod_fault);
    parseNumericOperandDescriptor(2, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bits 0-10 MBZ
    if (IWB_IRODD & 0777600000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "cmpn 0-10 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    uint srcTN = e->TN1;    // type of chars in src
    
    int n1 = 0, n2 = 0, sc1 = 0, sc2 = 0;
    
    /*
     * Here we need to distinguish between 4 type of numbers.
     *
     * CSFL - Floating-point, leading sign
     * CSLS - Scaled fixed-point, leading sign
     * CSTS - Scaled fixed-point, trailing sign
     * CSNS - Scaled fixed-point, unsigned
     */
    
    // determine precision
    switch(e->S1)
    {
        case CSFL:
            n1 = (int) e->N1 - 1; // need to account for the - sign
            if (srcTN == CTN4)
                n1 -= 2;    // 2 4-bit digits exponent
            else
                n1 -= 1;    // 1 9-bit digit exponent
            sc1 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n1 = (int) e->N1 - 1; // only 1 sign
            sc1 = -e->SF1;
            break;
            
        case CSNS:
            n1 = (int) e->N1;     // no sign
            sc1 = -e->SF1;
            break;  // no sign wysiwyg
    }

    if (n1 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "cmpn adjusted n1<1");

    switch(e->S2)
    {
        case CSFL:
            n2 = (int) e->N2 - 1; // need to account for the sign
            if (e->TN2 == CTN4)
                n2 -= 2;    // 2 4-bit digit exponent
            else
                n2 -= 1;    // 1 9-bit digit exponent
            sc2 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n2 = (int) e->N2 - 1; // 1 sign
            sc2 = -e->SF2;
            break;
            
        case CSNS:
            n2 = (int) e->N2;     // no sign
            sc2 = -e->SF2;
            break;  // no sign wysiwyg
    }

    if (n2 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "cmpn adjusted n2<1");


    decContext set;
    //decContextDefault(&set, DEC_INIT_BASE);         // initialize
    decContextDefaultDPS8(&set);

    set.traps=0;
    
    decNumber _1, _2, _3;
    
    EISloadInputBufferNumeric (1);   // according to MF1
    
    decNumber *op1 = decBCD9ToNumber(e->inBuffer, n1, sc1, &_1);
    if (e->sign == -1)
        op1->bits |= DECNEG;
    if (e->S1 == CSFL)
        op1->exponent = e->exponent;
    
    EISloadInputBufferNumeric (2);   // according to MF2
    
    decNumber *op2 = decBCD9ToNumber(e->inBuffer, n2, sc2, &_2);
    if (e->sign == -1)
        op2->bits |= DECNEG;
    if (e->S2 == CSFL)
        op2->exponent = e->exponent;
    
    // signed-compare
    decNumber *cmp = decNumberCompare(&_3, op1, op2, &set); // compare signed op1 :: op2
    int cSigned = decNumberToInt32(cmp, &set);
    
    // take absolute value of operands
    op1 = decNumberAbs(op1, op1, &set);
    op2 = decNumberAbs(op2, op2, &set);

    // magnitude-compare
    decNumber *mcmp = decNumberCompare(&_3, op1, op2, &set); // compare signed op1 :: op2
    int cMag = decNumberToInt32(mcmp, &set);
    
    // Zero If C(Y-charn1) = C(Y-charn2), then ON; otherwise OFF
    // Negative If C(Y-charn1) > C(Y-charn2), then ON; otherwise OFF
    // Carry If | C(Y-charn1) | > | C(Y-charn2) | , then OFF, otherwise ON

    SC_I_ZERO (cSigned == 0);
    SC_I_NEG (cSigned == 1);
    SC_I_CARRY (cMag != 1);

    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);

}

/*
 * mvn - move numeric (initial version was deleted by house gnomes)
 */

/*
 * write 4-bit chars to memory @ pos ...
 */

static void EISwrite4(EISaddr *p, int *pos, word4 char4)
{
    word36 w;
    if (*pos > 7)    // out-of-range?
    {
        *pos = 0;    // reset to 1st byte
#ifdef EIS_PTR
        long eisaddr_idx = EISADDR_IDX (p);
if (eisaddr_idx < 0 || eisaddr_idx > 2) { sim_warn ("IDX1"); return }
        cpu.du.Dk_PTR_W[eisaddr_idx] = (cpu.du.Dk_PTR_W[eisaddr_idx] + 1) & AMASK;     // bump source to next address
#else
        p->address = (p->address + 1) & AMASK;        // goto next dstAddr in memory
#endif
    }

    w = EISRead(p);      // read dst memory into w

// AL39, Figure 2-3
    switch (*pos)
    {
        case 0: 
            //w = bitfieldInsert36(w, char4, 31, 5);
            w = setbits36_4 (w, 1, char4);
            break;
        case 1: 
            //w = bitfieldInsert36(w, char4, 27, 4);
            w = setbits36_4 (w, 5, char4);
            break;
        case 2: 
            //w = bitfieldInsert36(w, char4, 22, 5);
            w = setbits36_4 (w, 10, char4);
            break;
        case 3: 
            //w = bitfieldInsert36(w, char4, 18, 4);
            w = setbits36_4 (w, 14, char4);
            break;
        case 4: 
            //w = bitfieldInsert36(w, char4, 13, 5);
            w = setbits36_4 (w, 19, char4);
            break;
        case 5: 
            //w = bitfieldInsert36(w, char4, 9, 4);
            w = setbits36_4 (w, 23, char4);
            break;
        case 6: 
            //w = bitfieldInsert36(w, char4, 4, 5);
            w = setbits36_4 (w, 28, char4);
            break;
        case 7: 
            //w = bitfieldInsert36(w, char4, 0, 4);
            w = setbits36_4 (w, 32, char4);
            break;
    }


    EISWriteIdx(p, 0, w, true); // XXX this is the inefficient part!

    *pos += 1;       // to next char.
}


/*
 * write 9-bit bytes to memory @ pos ...
 */

static void EISwrite9(EISaddr *p, int *pos, word9 char9)
{
    word36 w;
    if (*pos > 3)    // out-of-range?
    {
        *pos = 0;    // reset to 1st byte
#ifdef EIS_PTR
        long eisaddr_idx = EISADDR_IDX (p);
if (eisaddr_idx < 0 || eisaddr_idx > 2) { sim_warn ("IDX1"); return }
        cpu.du.Dk_PTR_W[eisaddr_idx] = (cpu.du.Dk_PTR_W[eisaddr_idx] + 1) & AMASK;     // bump source to next address
#else
        p->address = (p->address + 1) & AMASK;       // goto next dstAddr in memory
#endif
    }

    w = EISRead(p);      // read dst memory into w

// AL39, Figure 2-5
    switch (*pos)
    {
        case 0: 
            //w = bitfieldInsert36(w, char9, 27, 9);
            w = setbits36_9 (w, 0, char9);
            break;
        case 1: 
            //w = bitfieldInsert36(w, char9, 18, 9);
            w = setbits36_9 (w, 9, char9);
            break;
        case 2: 
            //w = bitfieldInsert36(w, char9, 9, 9);
            w = setbits36_9 (w, 18, char9);
            break;
        case 3: 
            //w = bitfieldInsert36(w, char9, 0, 9);
            w = setbits36_9 (w, 27, char9);
            break;
    }

    EISWriteIdx (p, 0, w, true); // XXX this is the inefficient part!

    *pos += 1;       // to next byte.
}

/*
 * write a 4-, or 9-bit numeric char to dstAddr ....
 */

static void EISwrite49(EISaddr *p, int *pos, int tn, word9 c49)
{
    switch(tn)
    {
        case CTN4:
            EISwrite4(p, pos, (word4) c49);
            return;
        case CTN9:
            EISwrite9(p, pos, c49);
            return;
    }
}


void mvn (void)
{
    /*
     * EXPLANATION:
     * Starting at location YC1, the decimal number of data type TN1 and sign
     * and decimal type S1 is moved, properly scaled, to the decimal number of
     * data type TN2 and sign and decimal type S2 that starts at location YC2.
     * If S2 indicates a fixed-point format, the results are stored as L2
     * digits using scale factor SF2, and thereby may cause
     * most-significant-digit overflow and/or least- significant-digit
     * truncation.
     * If P = 1, positive signed 4-bit results are stored using octal 13 as the
     * plus sign. Rounding is legal for both fixed-point and floating-point
     * formats. If P = 0, positive signed 4-bit results are stored using octal
     * 14 as the plus sign.
     * Provided that string 1 and string 2 are not overlapped, the contents of
     * the decimal number that starts in location YC1 remain unchanged.
     */

    EISstruct * e = & cpu.currentEISinstruction;
    
    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor(1, &mod_fault);
    setupOperandDescriptor(2, &mod_fault);
#endif
    
    parseNumericOperandDescriptor(1, &mod_fault);
    parseNumericOperandDescriptor(2, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bits 2-8 MBZ
    if (IWB_IRODD & 0377000000000)
     doFault (FAULT_IPR,
              (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault},
              "mvn 2-8 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    e->P = getbits36_1 (cpu.cu.IWB, 0) != 0;  // 4-bit data sign character
                                              //  control
    word1 T = getbits36_1 (cpu.cu.IWB, 9);
    bool R = getbits36_1 (cpu.cu.IWB, 10) != 0;  // rounding bit
    PNL (L68_ (if (R)
      DU_CYCLE_FRND;))

    uint srcTN = e->TN1;    // type of chars in src
    
    uint dstTN = e->TN2;    // type of chars in dst
    uint dstCN = e->CN2;    // starting at char pos CN
    
    sim_debug (DBG_CAC, & cpu_dev,
               "mvn(1): TN1 %d CN1 %d N1 %d TN2 %d CN2 %d N2 %d\n",
               e->TN1, e->CN1, e->N1, e->TN2, e->CN2, e->N2);
    sim_debug (DBG_CAC, & cpu_dev,
               "mvn(2): SF1 %d              SF2 %d\n",
               e->SF1, e->SF2);
    sim_debug (DBG_CAC, & cpu_dev,
               "mvn(3): OP1 %012"PRIo64" OP2 %012"PRIo64"\n",
               e->OP1, e->OP2);

    int n1 = 0, n2 = 0, sc1 = 0;
    
    /*
     * Here we need to distinguish between 4 type of numbers.
     *
     * CSFL - Floating-point, leading sign
     * CSLS - Scaled fixed-point, leading sign
     * CSTS - Scaled fixed-point, trailing sign
     * CSNS - Scaled fixed-point, unsigned
     */
    
    // determine precision
    switch(e->S1)
    {
        case CSFL:
            n1 = (int) e->N1 - 1; // need to account for the - sign
            if (srcTN == CTN4)
                n1 -= 2;    // 2 4-bit digits exponent
            else
                n1 -= 1;    // 1 9-bit digit exponent
            sc1 = 0;        // no scaling factor
            break;
        
        case CSLS:
        case CSTS:
            n1 = (int) e->N1 - 1; // only 1 sign
            sc1 = -e->SF1;
            break;
        
        case CSNS:
            n1 = (int) e->N1;     // no sign
            sc1 = -e->SF1;
            break;  // no sign wysiwyg
    }

    sim_debug (DBG_CAC, & cpu_dev, "n1 %d sc1 %d\n", n1, sc1);

    // RJ78: An Illegal Procedure fault occurs if:
    // The values for the number of characters (N1 or N2) of the data
    // descriptors are not large enough to hold the number of characters
    // required for the specified sign and/or exponent, plus at least one
    // digit. 

    if (n1 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "mvn adjusted n1<1");

    switch(e->S2)
    {
        case CSFL:
            n2 = (int) e->N2 - 1; // need to account for the sign
            if (dstTN == CTN4)
                n2 -= 2;    // 2 4-bit digit exponent
            else
                n2 -= 1;    // 1 9-bit digit exponent
            break;
        
        case CSLS:
        case CSTS:
            n2 = (int) e->N2 - 1; // 1 sign
            break;
        
        case CSNS:
            n2 = (int) e->N2;     // no sign
            break;          // no sign wysiwyg
    }
    
    sim_debug (DBG_CAC, & cpu_dev, "n2 %d\n", n2);

    if (n2 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "mvn adjusted n2<1");

    decContext set;
    decContextDefaultDPS8(&set);
    set.traps=0;
    
    decNumber _1;

    EISloadInputBufferNumeric (1);   // according to MF1
    
    decNumber *op1 = decBCD9ToNumber (e->inBuffer, n1, sc1, &_1);
    
    if (e->sign == -1)
        op1->bits |= DECNEG;
    if (e->S1 == CSFL)
        op1->exponent = e->exponent;
    if (decNumberIsZero (op1))
        op1->exponent = 127;
   
    if_sim_debug (DBG_CAC, & cpu_dev)
    {
        PRINTDEC ("mvn input (op1)", op1);
    }
   
    bool Ovr = false, EOvr = false, Trunc = false;
    

    uint8_t out [256];
    char * res = formatDecimal (out, & set, op1, n2, (int) e->S2, e->SF2, R,
                                & Ovr, & Trunc);
    
    sim_debug (DBG_CAC, & cpu_dev, "mvn res: '%s'\n", res);
    
    // now write to memory in proper format.....

    //word18 dstAddr = e->dstAddr;
    int pos = (int) dstCN;
    
    // 1st, take care of any leading sign .......
    switch(e->S2)
    {
        case CSFL:  // floating-point, leading sign.
        case CSLS:  // fixed-point, leading sign
            switch(dstTN)
            {
                case CTN4:
 // If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus
 // sign character is placed appropriately if the result of the operation is
 // positive.
                    if (e->P)
                        // special +
                        EISwrite49 (& e->ADDR2, & pos, (int) dstTN,
                                   (decNumberIsNegative (op1) &&
                                    ! decNumberIsZero(op1)) ? 015 : 013);
                    else
                        // default +
                        EISwrite49 (& e->ADDR2, & pos, (int) dstTN, 
                                    (decNumberIsNegative (op1) &&
                                     ! decNumberIsZero (op1)) ? 015 : 014);
                    break;
                case CTN9:
                    EISwrite49 (& e->ADDR2, & pos, (int) dstTN,
                                (decNumberIsNegative (op1) &&
                                 ! decNumberIsZero (op1)) ? '-' : '+');
                    break;
            }
            break;
        
        case CSTS:  // nuttin' to do here .....
        case CSNS:
            break;  // no sign wysiwyg
    }
    
    // 2nd, write the digits .....
    for (int i = 0 ; i < n2 ; i ++)
        switch (dstTN)
        {
            case CTN4:
                EISwrite49 (& e->ADDR2, & pos, (int) dstTN,
                            (word9) (res[i] - '0'));
                break;
            case CTN9:
                EISwrite49 (& e->ADDR2, & pos, (int) dstTN, (word9) res[i]);
                break;
        }
    
    // 3rd, take care of any trailing sign or exponent ...
    switch(e->S2)
    {
        case CSTS:  // write trailing sign ....
            switch(dstTN)
            {
                case CTN4:
// If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus
// sign character is placed appropriately if the result of the operation is
// positive.
                    if (e->P)
                        // special +
                        EISwrite49 (& e->ADDR2, & pos, (int) dstTN,
                                    (decNumberIsNegative (op1) &&
                                     ! decNumberIsZero(op1)) ? 015 :  013);
                    else
                        // default +
                        EISwrite49 (& e->ADDR2, & pos, (int) dstTN,
                                    (decNumberIsNegative (op1) &&
                                     ! decNumberIsZero (op1)) ? 015 :  014);
                    break;
            
                case CTN9:
                    EISwrite49 (& e->ADDR2, & pos, (int) dstTN,
                                (decNumberIsNegative (op1) &&
                                 ! decNumberIsZero(op1)) ? '-' : '+');
                    break;
            }
            break;
        
        case CSFL:  // floating-point, leading sign.
            // write the exponent
            switch(dstTN)
            {
                case CTN4:
                    EISwrite49 (& e->ADDR2, & pos, (int) dstTN,
                                (op1->exponent >> 4) & 0xf); // upper 4-bits
                    EISwrite49 (& e->ADDR2, & pos, (int) dstTN,
                                op1->exponent       & 0xf); // lower 4-bits
                    break;
                case CTN9:
                    EISwrite49 (& e->ADDR2, & pos, (int) dstTN,
                                 op1->exponent & 0xff); // write 8-bit exponent
                break;
            }
            break;
        
        case CSLS:  // fixed point, leading sign - already done
        case CSNS:  // fixed point, unsigned - nuttin' needed to do
            break;
    }
    
    // set flags, etc ...
    if (e->S2 == CSFL)
    {
        if (op1->exponent > 127)
        {
            SET_I_EOFL;
            EOvr = true;
        }
        if (op1->exponent < -128)
        {
            SET_I_EUFL;
            EOvr = true;
        }
    }
    
sim_debug (DBG_CAC, & cpu_dev, "is neg %o\n", decNumberIsNegative(op1));
sim_debug (DBG_CAC, & cpu_dev, "is zero %o\n", decNumberIsZero(op1));
sim_debug (DBG_CAC, & cpu_dev, "R %o\n", R);
sim_debug (DBG_CAC, & cpu_dev, "Trunc %o\n", Trunc);
sim_debug (DBG_CAC, & cpu_dev, "TRUNC %o\n", TST_I_TRUNC);
sim_debug (DBG_CAC, & cpu_dev, "OMASK %o\n", TST_I_OMASK);
sim_debug (DBG_CAC, & cpu_dev, "tstOVFfault %o\n", tstOVFfault ());
sim_debug (DBG_CAC, & cpu_dev, "T %o\n", T);
sim_debug (DBG_CAC, & cpu_dev, "EOvr %o\n", EOvr);
sim_debug (DBG_CAC, & cpu_dev, "Ovr %o\n", Ovr);
    // set negative indicator if op3 < 0
    SC_I_NEG (decNumberIsNegative(op1) && !decNumberIsZero(op1));

    // set zero indicator if op3 == 0
    SC_I_ZERO (decNumberIsZero(op1));
    
    // If the truncation condition exists without rounding, then ON; 
    // otherwise OFF
    SC_I_TRUNC (!R && Trunc);

    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    
    if (TST_I_TRUNC && T && tstOVFfault ())
        doFault (FAULT_OFL, fst_zero, "mvn truncation(overflow) fault");
    if (EOvr && tstOVFfault ())
        doFault (FAULT_OFL, fst_zero, "mvn over/underflow fault");
    if (Ovr)
    {
        SET_I_OFLOW;
        if (tstOVFfault ())
          doFault (FAULT_OFL, fst_zero, "mvn overflow fault");
    }
}


void csl (void)
  {
    EISstruct * e = & cpu.currentEISinstruction;

    // For i = bits 1, 2, ..., minimum (N1,N2)
    //   m = C(Y-bit1)i-1 || C(Y-bit2)i-1 (a 2-bit number)
    //   C(BOLR)m → C(Y-bit2)i-1
    // If N1 < N2, then for i = N1+l, N1+2, ..., N2
    //   m = C(F) || C(Y-bit2)i-1 (a 2-bit number)
    //   C(BOLR)m → C(Y-bit2)i-1
    //
    // INDICATORS: (Indicators not listed are not affected)
    //     Zero If C(Y-bit2) = 00...0, then ON; otherwise OFF
    //     Truncation If N1 > N2, then ON; otherwise OFF
    //
    // NOTES: If N1 > N2, the low order (N1-N2) bits of C(Y-bit1) are not
    // processed and the truncation indicator is set ON.
    //
    // If T = 1 and the truncation indicator is set ON by execution of the
    // instruction, then a truncation (overflow) fault occurs.
    //
    // BOLR
    // If first operand    and    second operand    then result
    // bit is:                    bit is:           is from bit:
    //        0                          0                      5
    //        0                          1                      6
    //        1                          0                      7
    //        1                          1                      8
    //
    // The Boolean operations most commonly used are
    //                  BOLR Field Bits
    // Operation        5      6      7      8
    //
    // MOVE             0      0      1      1
    // AND              0      0      0      1
    // OR               0      1      1      1
    // NAND             1      1      1      0
    // EXCLUSIVE OR     0      1      1      0
    // Clear            0      0      0      0
    // Invert           1      1      0      0
    //
    
// 0 0 0 0  Clear
// 0 0 0 1  a AND b
// 0 0 1 0  a AND !b
// 0 0 1 1  a
// 0 1 0 0  !a AND b
// 0 1 0 1  b
// 0 1 1 0  a XOR b
// 0 1 1 1  a OR b
// 1 0 0 0  !a AND !b     !(a OR b)
// 1 0 0 1  a == b        !(a XOR b)
// 1 0 1 0  !b
// 1 0 1 1  !b OR A 
// 1 1 0 0  !a
// 1 1 0 1  !b AND a
// 1 1 1 0  a NAND b
// 1 1 1 1  Set

    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor (1, & mod_fault);
    setupOperandDescriptor (2, & mod_fault);
#endif
    
    parseBitstringOperandDescriptor (1, & mod_fault);
    parseBitstringOperandDescriptor (2, & mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
#endif
    
    // Bits 1-4 and 10 MBZ
    if (IWB_IRODD & 0360200000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "csl 1-4,10 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
#endif

    e->ADDR1.cPos = (int) e->C1;
    e->ADDR2.cPos = (int) e->C2;
    
    e->ADDR1.bPos = (int) e->B1;
    e->ADDR2.bPos = (int) e->B2;
    
    bool F = getbits36_1 (cpu.cu.IWB, 0) != 0;   // fill bit
    bool T = getbits36_1 (cpu.cu.IWB, 9) != 0;   // T (enablefault) bit
    
    uint BOLR = getbits36_4 (cpu.cu.IWB, 5);   // T (enablefault) bit
    bool B5 = !! (BOLR & 8);
    bool B6 = !! (BOLR & 4);
    bool B7 = !! (BOLR & 2);
    bool B8 = !! (BOLR & 1);
    
    e->ADDR1.mode = eRWreadBit;

#ifndef EIS_PTR
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "CSL N1 %d N2 %d\n"
               "CSL C1 %d C2 %d B1 %d B2 %d F %o T %d\n"
               "CSL BOLR %u%u%u%u\n"
               "CSL op1 SNR %06o WORDNO %06o CHAR %d BITNO %d\n"
               "CSL op2 SNR %06o WORDNO %06o CHAR %d BITNO %d\n",
               e -> N1, e -> N2,
               e -> C1, e -> C2, e -> B1, e -> B2, F, T,
               B5, B6, B7, B8,
               e -> addr [0].SNR, e -> addr [0].address, 
               e -> addr [0].cPos, e -> addr [0].bPos,
               e -> addr [1].SNR, e -> addr [1].address, 
               e -> addr [1].cPos, e -> addr [1].bPos);
#endif

    bool bR = false; // result bit

    PNL (L68_ (if (max (e->N1, e->N2) < 128)
                 DU_CYCLE_FLEN_128;))

    for( ; cpu.du.CHTALLY < min(e->N1, e->N2); cpu.du.CHTALLY += 1)
      {
        bool b1 = EISgetBitRWN(&e->ADDR1, true);
        e->ADDR2.mode = eRWreadBit;
        bool b2 = EISgetBitRWN(&e->ADDR2, true);
        
        if (b1) if (b2) bR = B8; else bR = B7; else if (b2) bR = B6; else bR = B5;

        if (bR)
          {
            //CLR_I_ZERO);
            cpu.du.Z = 0;
          }

        // write out modified bit
        e->ADDR2.bit = bR ? 1 : 0;              // set bit contents to write
        e->ADDR2.mode = eRWwriteBit;    // we want to write the bit
        // if ADDR1 is on a word boundary, it might fault on the next loop,
        // so we flush the write in case.
        EISgetBitRWN(&e->ADDR2, e->ADDR1.last_bit_posn == 35);    // write bit w/ addr increment to memory
    }
    
    if (e->N1 < e->N2)
      {
        for(; cpu.du.CHTALLY < e->N2; cpu.du.CHTALLY += 1)
          {
            bool b1 = F;
            
            e->ADDR2.mode = eRWreadBit;
            bool b2 = EISgetBitRWN(&e->ADDR2, true);
            
            if (b1) if (b2) bR = B8; else bR = B7; else if (b2) bR = B6; else bR = B5;

            if (bR)
              {
                //CLR_I_ZERO;
                cpu.du.Z = 0;
              }
        
            // write out modified bit
            e->ADDR2.bit = bR ? 1 : 0;
            e->ADDR2.mode = eRWwriteBit;
            // if ADDR1 is on a word boundary, it might fault on the next loop,
            // so we flush the write in case.
            EISgetBitRWN(&e->ADDR2, e->ADDR1.last_bit_posn == 35);    // write bit w/ addr increment to memory
          }
      }

    EISWriteCache (&e->ADDR2);

    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);

    SC_I_ZERO (cpu.du.Z);
    if (e->N1 > e->N2)
      {
        // NOTES: If N1 > N2, the low order (N1-N2) bits of C(Y-bit1) are not
        // processed and the truncation indicator is set ON.
        //
        // If T = 1 and the truncation indicator is set ON by execution of the
        // instruction, then a truncation (overflow) fault occurs.
        
        SET_I_TRUNC;
        if (T && tstOVFfault ())
          doFault(FAULT_OFL, fst_zero, "csl truncation fault");
      }
    else
      {
        CLR_I_TRUNC;
      }
  }

/*
 * return B (bit position), C (char position) and word offset given:
 *  'length' # of bits, etc ....
 *  'initC' initial char position (C)
 *  'initB' initial bit position
 */

static void getBitOffsets(int length, int initC, int initB, int *nWords, int *newC, int *newB)
{
    if (length == 0)
        return;
    
    int endBit = (length + 9 * initC + initB - 1) % 36;
    
    //int numWords = (length + 35) / 36;  // how many additional words will the bits take up?
    int numWords = (length + 9 * initC + initB + 35) / 36;  // how many additional words will the bits take up?
    int lastWordOffset = numWords - 1;
    
    if (lastWordOffset > 0)          // more that the 1 word needed?
        *nWords = lastWordOffset;  // # of additional words
    else
        *nWords = 0;    // no additional words needed
    
    *newC = endBit / 9; // last character number
    *newB = endBit % 9; // last bit number
}

static bool EISgetBitRWNR (EISaddr * p, bool flush)
  {
    int baseCharPosn = (p -> cPos * 9);     // 9-bit char bit position
    int baseBitPosn = baseCharPosn + p -> bPos;
    baseBitPosn -= (int) cpu.du.CHTALLY;

    int bitPosn = baseBitPosn % 36;
    int woff = baseBitPosn / 36;
    while (bitPosn < 0)
      {
        bitPosn += 36;
        woff -= 1;
      }
if (bitPosn < 0) {
sim_printf ("cPos %d bPos %d\n", p->cPos, p->bPos);
sim_printf ("baseCharPosn %d baseBitPosn %d\n", baseCharPosn, baseBitPosn);
sim_printf ("CHTALLY %d baseBitPosn %d\n", cpu.du.CHTALLY, baseBitPosn);
sim_printf ("bitPosn %d woff %d\n", bitPosn, woff);
sim_warn ("EISgetBitRWNR oops\n");
return false;
}

#ifdef EIS_PTR
    long eisaddr_idx = EISADDR_IDX (p);
if (eisaddr_idx < 0 || eisaddr_idx > 2) { sim_warn ("IDX1"); return }
    word18 saveAddr = cpu.du.Dk_PTR_W[eisaddr_idx];
    cpu.du.Dk_PTR_W[eisaddr_idx] += (word18) woff;
    cpu.du.Dk_PTR_W[eisaddr_idx] &= AMASK;
#else
    word18 saveAddr = p -> address;
    p -> address += (word18) woff;
#endif

    p -> data = EISRead (p); // read data word from memory
    
    if (p -> mode == eRWreadBit)
      {
        p -> bit = getbits36_1 (p -> data, (uint) bitPosn);
      } 
    else if (p -> mode == eRWwriteBit)
      {
        //p -> data = bitfieldInsert36 (p -> data, p -> bit, bitPosn, 1);
        p -> data = setbits36_1 (p -> data, (uint) bitPosn, p -> bit);
        
        EISWriteIdx (p, 0, p -> data, flush); // write data word to memory
      }

    p->last_bit_posn = bitPosn;

#ifdef EIS_PTR
    cpu.du.Dk_PTR_W[eisaddr_idx] = saveAddr;
#else
    p -> address = saveAddr;
#endif
    return p -> bit;
  }

void csr (void)
  {
    EISstruct * e = & cpu.currentEISinstruction;

    // For i = bits 1, 2, ..., minimum (N1,N2)
    //   m = C(Y-bit1)N1-i || C(Y-bit2)N2-i (a 2-bit number)
    //   C(BOLR)m → C( Y-bit2)N2-i
    // If N1 < N2, then for i = N1+i, N1+2, ..., N2
    //   m = C(F) || C(Y-bit2)N2-i (a 2-bit number)
    //    C(BOLR)m → C( Y-bit2)N2-i
    // INDICATORS: (Indicators not listed are not affected)
    //     Zero If C(Y-bit2) = 00...0, then ON; otherwise OFF
    //     Truncation If N1 > N2, then ON; otherwise OFF
    //
    // NOTES: If N1 > N2, the low order (N1-N2) bits of C(Y-bit1) are not
    // processed and the truncation indicator is set ON.
    //
    // If T = 1 and the truncation indicator is set ON by execution of the
    // instruction, then a truncation (overflow) fault occurs.
    //
    // BOLR
    // If first operand    and    second operand    then result
    // bit is:                    bit is:           is from bit:
    //        0                          0                      5
    //        0                          1                      6
    //        1                          0                      7
    //        1                          1                      8
    //
    // The Boolean operations most commonly used are
    //                  BOLR Field Bits
    // Operation        5      6      7      8
    //
    // MOVE             0      0      1      1
    // AND              0      0      0      1
    // OR               0      1      1      1
    // NAND             1      1      1      0
    // EXCLUSIVE OR     0      1      1      0
    // Clear            0      0      0      0
    // Invert           1      1      0      0
    //
    
    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor(1, &mod_fault);
    setupOperandDescriptor(2, &mod_fault);
#endif
    
    parseBitstringOperandDescriptor(1, &mod_fault);
    parseBitstringOperandDescriptor(2, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
#endif
    
    // Bits 1-4 and 10 MBZ
    if (IWB_IRODD & 0360200000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "csr 1-4,10 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
#endif

    e->ADDR1.cPos = (int) e->C1;
    e->ADDR2.cPos = (int) e->C2;
    
    e->ADDR1.bPos = (int) e->B1;
    e->ADDR2.bPos = (int) e->B2;
    
    // get new char/bit offsets
    int numWords1=0, numWords2=0;
    
    getBitOffsets((int) e->N1, (int) e->C1, (int) e->B1, &numWords1, &e->ADDR1.cPos, &e->ADDR1.bPos);
    PNL (cpu.du.D1_PTR_W += (word18) numWords1);
    PNL (cpu.du.D1_PTR_W &= AMASK);
#ifdef EIS_PTR
    cpu.du.D1_PTR_W += (word18) numWords1;
    cpu.du.D1_PTR_W &= AMASK;
#else
    e->ADDR1.address += (word18) numWords1;
#endif
        
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "CSR N1 %d C1 %d B1 %d numWords1 %d cPos %d bPos %d\n",
               e->N1, e->C1, e->B1, numWords1, e->ADDR1.cPos, e->ADDR1.bPos);
    getBitOffsets((int) e->N2, (int) e->C2, (int) e->B2, &numWords2, &e->ADDR2.cPos, &e->ADDR2.bPos);
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "CSR N2 %d C2 %d B2 %d numWords2 %d cPos %d bPos %d\n",
               e->N2, e->C2, e->B2, numWords2, e->ADDR2.cPos, e->ADDR2.bPos);
    PNL (cpu.du.D2_PTR_W += (word18) numWords1);
    PNL (cpu.du.D2_PTR_W &= AMASK);
#ifdef EIS_PTR
    cpu.du.D2_PTR_W += (word18) numWords1;
    cpu.du.D2_PTR_W &= AMASK;
#else
    e->ADDR2.address += (word18) numWords2;
#endif
    
    bool F = getbits36_1 (cpu.cu.IWB, 0) != 0;   // fill bit
    bool T = getbits36_1 (cpu.cu.IWB, 9) != 0;   // T (enablefault) bit
    
    uint BOLR = getbits36_4 (cpu.cu.IWB, 5);   // T (enablefault) bit
    bool B5 = !! (BOLR & 8);
    bool B6 = !! (BOLR & 4);
    bool B7 = !! (BOLR & 2);
    bool B8 = !! (BOLR & 1);
    
    e->ADDR1.mode = eRWreadBit;
    
    CLR_I_TRUNC;     // assume N1 <= N2
    
    bool bR = false; // result bit
    
    PNL (L68_ (if (max (e->N1, e->N2) < 128)
                 DU_CYCLE_FLEN_128;))

    for( ; cpu.du.CHTALLY < min(e->N1, e->N2); cpu.du.CHTALLY += 1)
      {
        bool b1 = EISgetBitRWNR(&e->ADDR1, true);
        
        e->ADDR2.mode = eRWreadBit;
        bool b2 = EISgetBitRWNR(&e->ADDR2, true);
        
        if (b1) if (b2) bR = B8; else bR = B7; else if (b2) bR = B6; else bR = B5;
        
        if (bR)
          cpu.du.Z = 0;
        
        // write out modified bit
        e->ADDR2.bit = bR ? 1 : 0;              // set bit contents to write
        e->ADDR2.mode = eRWwriteBit;    // we want to write the bit
        // if ADDR1 is on a word boundary, it might fault on the next loop,
        // so we flush the write in case.
        EISgetBitRWNR(&e->ADDR2, e->ADDR1.last_bit_posn == 0);
      }
    
    if (e->N1 < e->N2)
      {
        for(; cpu.du.CHTALLY < e->N2; cpu.du.CHTALLY += 1)
          {
            bool b1 = F;
            
            e->ADDR2.mode = eRWreadBit;
            bool b2 = EISgetBitRWNR(&e->ADDR2, true);
            
            if (b1) if (b2) bR = B8; else bR = B7; else if (b2) bR = B6; else bR = B5;
            
            if (bR)
              {
                //CLR_I_ZERO;
                cpu.du.Z = 0;
              }
        
            // write out modified bit
            e->ADDR2.bit = bR ? 1 : 0;
            e->ADDR2.mode = eRWwriteBit;
            // if ADDR1 is on a word boundary, it might fault on the next loop,
            // so we flush the write in case.
            EISgetBitRWNR(&e->ADDR2, e->ADDR1.last_bit_posn == 0);
          }
      }

    EISWriteCache (&e->ADDR2);

    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);

    SC_I_ZERO (cpu.du.Z);
    if (e->N1 > e->N2)
      {
        // NOTES: If N1 > N2, the low order (N1-N2) bits of C(Y-bit1) are not
        // processed and the truncation indicator is set ON.
        //
        // If T = 1 and the truncation indicator is set ON by execution of the
        // instruction, then a truncation (overflow) fault occurs.
        
        SET_I_TRUNC;
        if (T && tstOVFfault ())
          doFault(FAULT_OFL, fst_zero, "csr truncation fault");
      }
    else
      {
        CLR_I_TRUNC;
      }
  }

void sztl (void)
  {

    // The execution of this instruction is identical to the Combine
    // Bit Strings Left (csl) instruction except that C(BOLR)m is
    // not placed into C(Y-bit2)i-1.

    EISstruct * e = & cpu.currentEISinstruction;

    // For i = bits 1, 2, ..., minimum (N1,N2)
    //   m = C(Y-bit1)i-1 || C(Y-bit2)i-1 (a 2-bit number)
    //   C(BOLR)m → C(Y-bit2)i-1
    // If N1 < N2, then for i = N1+l, N1+2, ..., N2
    //   m = C(F) || C(Y-bit2)i-1 (a 2-bit number)
    //   C(BOLR)m → C(Y-bit2)i-1
    //
    // INDICATORS: (Indicators not listed are not affected)
    //     Zero If C(Y-bit2) = 00...0, then ON; otherwise OFF
    //     Truncation If N1 > N2, then ON; otherwise OFF
    //
    // NOTES: If N1 > N2, the low order (N1-N2) bits of C(Y-bit1) are not
    // processed and the truncation indicator is set ON.
    //
    // If T = 1 and the truncation indicator is set ON by execution of the
    // instruction, then a truncation (overflow) fault occurs.
    //
    // BOLR
    // If first operand    and    second operand    then result
    // bit is:                    bit is:           is from bit:
    //        0                          0                      5
    //        0                          1                      6
    //        1                          0                      7
    //        1                          1                      8
    //
    // The Boolean operations most commonly used are
    //                  BOLR Field Bits
    // Operation        5      6      7      8
    //
    // MOVE             0      0      1      1
    // AND              0      0      0      1
    // OR               0      1      1      1
    // NAND             1      1      1      0
    // EXCLUSIVE OR     0      1      1      0
    // Clear            0      0      0      0
    // Invert           1      1      0      0
    //
    
// 0 0 0 0  Clear
// 0 0 0 1  a AND b
// 0 0 1 0  a AND !b
// 0 0 1 1  a
// 0 1 0 0  !a AND b
// 0 1 0 1  b
// 0 1 1 0  a XOR b
// 0 1 1 1  a OR b
// 1 0 0 0  !a AND !b     !(a OR b)
// 1 0 0 1  a == b        !(a XOR b)
// 1 0 1 0  !b
// 1 0 1 1  !b OR A 
// 1 1 0 0  !a
// 1 1 0 1  !b AND a
// 1 1 1 0  a NAND b
// 1 1 1 1  Set

    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor (1, &mod_fault);
    setupOperandDescriptor (2, &mod_fault);
#endif
    
    parseBitstringOperandDescriptor (1, &mod_fault);
    parseBitstringOperandDescriptor (2, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
#endif
    
    // Bits 1-4 and 10 MBZ
    if (IWB_IRODD & 0360200000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "csl 1-4,10 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
#endif

    e->ADDR1.cPos = (int) e->C1;
    e->ADDR2.cPos = (int) e->C2;
    
    e->ADDR1.bPos = (int) e->B1;
    e->ADDR2.bPos = (int) e->B2;
    
    bool F = getbits36_1 (cpu.cu.IWB, 0) != 0;   // fill bit
    bool T = getbits36_1 (cpu.cu.IWB, 9) != 0;   // T (enablefault) bit
    
    uint BOLR = getbits36_4 (cpu.cu.IWB, 5);   // T (enablefault) bit
    bool B5 = !! (BOLR & 8);
    bool B6 = !! (BOLR & 4);
    bool B7 = !! (BOLR & 2);
    bool B8 = !! (BOLR & 1);
    
    e->ADDR1.mode = eRWreadBit;
    e->ADDR2.mode = eRWreadBit;

#ifndef EIS_PTR
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "SZTL N1 %d N2 %d\n"
               "SZTL C1 %d C2 %d B1 %d B2 %d F %o T %d\n"
               "SZTL BOLR %u%u%u%u\n"
               "SZTL op1 SNR %06o WORDNO %06o CHAR %d BITNO %d\n"
               "SZTL op2 SNR %06o WORDNO %06o CHAR %d BITNO %d\n",
               e -> N1, e -> N2,
               e -> C1, e -> C2, e -> B1, e -> B2, F, T,
               B5, B6, B7, B8,
               e -> addr [0].SNR, e -> addr [0].address, 
               e -> addr [0].cPos, e -> addr [0].bPos,
               e -> addr [1].SNR, e -> addr [1].address, 
               e -> addr [1].cPos, e -> addr [1].bPos);
#endif

    bool bR = false; // result bit

    PNL (L68_ (if (max (e->N1, e->N2) < 128)
                 DU_CYCLE_FLEN_128;))

    for( ; cpu.du.CHTALLY < min (e->N1, e->N2); cpu.du.CHTALLY += 1)
    {
        bool b1 = EISgetBitRWN (& e->ADDR1, true);
        bool b2 = EISgetBitRWN (& e->ADDR2, true);
        
        if (b1) if (b2) bR = B8; else bR = B7; else if (b2) bR = B6; else bR = B5;
        
        if (bR)
          {
            //CLR_I_ZERO);
            cpu.du.Z = 0;
            break;
          }
      }
    
    if (e->N1 < e->N2)
      {
        for (; cpu.du.CHTALLY < e->N2; cpu.du.CHTALLY += 1)
          {
            bool b1 = F;
            bool b2 = EISgetBitRWN (& e->ADDR2, true);
            
            if (b1) if (b2) bR = B8; else bR = B7; else if (b2) bR = B6; else bR = B5;
            
            if (bR)
              {
                //CLR_I_ZERO;
                cpu.du.Z = 0;
                break;
              }
          }
      }
    
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);

    SC_I_ZERO (cpu.du.Z);
    if (e->N1 > e->N2)
      {
        // NOTES: If N1 > N2, the low order (N1-N2) bits of C(Y-bit1) are not
        // processed and the truncation indicator is set ON.
        //
        // If T = 1 and the truncation indicator is set ON by execution of the
        // instruction, then a truncation (overflow) fault occurs.
        
        SET_I_TRUNC;
        if (T && tstOVFfault ())
          doFault(FAULT_OFL, fst_zero, "csl truncation fault");
      }
    else
      {
        CLR_I_TRUNC;
      }
  }

void sztr (void)
  {

    // The execution of this instruction is identical to the Combine
    // Bit Strings Left (csl) instruction except that C(BOLR)m is
    // not placed into C(Y-bit2)i-1.

    EISstruct * e = & cpu.currentEISinstruction;

    // For i = bits 1, 2, ..., minimum (N1,N2)
    //   m = C(Y-bit1)N1-i || C(Y-bit2)N2-i (a 2-bit number)
    //   C(BOLR)m → C( Y-bit2)N2-i
    // If N1 < N2, then for i = N1+i, N1+2, ..., N2
    //   m = C(F) || C(Y-bit2)N2-i (a 2-bit number)
    //    C(BOLR)m → C( Y-bit2)N2-i
    // INDICATORS: (Indicators not listed are not affected)
    //     Zero If C(Y-bit2) = 00...0, then ON; otherwise OFF
    //     Truncation If N1 > N2, then ON; otherwise OFF
    //
    // NOTES: If N1 > N2, the low order (N1-N2) bits of C(Y-bit1) are not
    // processed and the truncation indicator is set ON.
    //
    // If T = 1 and the truncation indicator is set ON by execution of the
    // instruction, then a truncation (overflow) fault occurs.
    //
    // BOLR
    // If first operand    and    second operand    then result
    // bit is:                    bit is:           is from bit:
    //        0                          0                      5
    //        0                          1                      6
    //        1                          0                      7
    //        1                          1                      8
    //
    // The Boolean operations most commonly used are
    //                  BOLR Field Bits
    // Operation        5      6      7      8
    //
    // MOVE             0      0      1      1
    // AND              0      0      0      1
    // OR               0      1      1      1
    // NAND             1      1      1      0
    // EXCLUSIVE OR     0      1      1      0
    // Clear            0      0      0      0
    // Invert           1      1      0      0
    //
    
    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor(1, &mod_fault);
    setupOperandDescriptor(2, &mod_fault);
#endif
    
    parseBitstringOperandDescriptor(1, &mod_fault);
    parseBitstringOperandDescriptor(2, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
#endif
    
    // Bits 1-4 and 10 MBZ
    if (IWB_IRODD & 0360200000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "csr 1-4,10 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
#endif

    e->ADDR1.cPos = (int) e->C1;
    e->ADDR2.cPos = (int) e->C2;
    
    e->ADDR1.bPos = (int) e->B1;
    e->ADDR2.bPos = (int) e->B2;
    
    // get new char/bit offsets
    int numWords1=0, numWords2=0;
    
    getBitOffsets((int) e->N1, (int) e->C1, (int) e->B1, &numWords1, &e->ADDR1.cPos, &e->ADDR1.bPos);
    PNL (cpu.du.D1_PTR_W += (word18) numWords1);
    PNL (cpu.du.D1_PTR_W &= AMASK);
#ifdef EIS_PTR
    cpu.du.D1_PTR_W += (word18) numWords1;
    cpu.du.D1_PTR_W &= AMASK;
#else
    e->ADDR1.address += (word18) numWords1;
#endif
        
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "CSR N1 %d C1 %d B1 %d numWords1 %d cPos %d bPos %d\n",
               e->N1, e->C1, e->B1, numWords1, e->ADDR1.cPos, e->ADDR1.bPos);
    getBitOffsets((int) e->N2, (int) e->C2, (int) e->B2, &numWords2, &e->ADDR2.cPos, &e->ADDR2.bPos);
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "CSR N2 %d C2 %d B2 %d numWords2 %d cPos %d bPos %d\n",
               e->N2, e->C2, e->B2, numWords2, e->ADDR2.cPos, e->ADDR2.bPos);
    PNL (cpu.du.D2_PTR_W += (word18) numWords1);
    PNL (cpu.du.D2_PTR_W &= AMASK);
#ifdef EIS_PTR
    cpu.du.D2_PTR_W += (word18) numWords1;
    cpu.du.D2_PTR_W &= AMASK;
#else
    e->ADDR2.address += (word18) numWords2;
#endif
    
    bool F = getbits36_1 (cpu.cu.IWB, 0) != 0;   // fill bit
    bool T = getbits36_1 (cpu.cu.IWB, 9) != 0;   // T (enablefault) bit
    
    uint BOLR = getbits36_4 (cpu.cu.IWB, 5);   // T (enablefault) bit
    bool B5 = !! (BOLR & 8);
    bool B6 = !! (BOLR & 4);
    bool B7 = !! (BOLR & 2);
    bool B8 = !! (BOLR & 1);
    
    e->ADDR1.mode = eRWreadBit;
    
    CLR_I_TRUNC;     // assume N1 <= N2
    
    bool bR = false; // result bit
    
    PNL (L68_ (if (max (e->N1, e->N2) < 128)
                 DU_CYCLE_FLEN_128;))

    for( ; cpu.du.CHTALLY < min(e->N1, e->N2); cpu.du.CHTALLY += 1)
      {
        bool b1 = EISgetBitRWNR(&e->ADDR1, true);
        
        e->ADDR2.mode = eRWreadBit;
        bool b2 = EISgetBitRWNR(&e->ADDR2, true);
        
        if (b1) if (b2) bR = B8; else bR = B7; else if (b2) bR = B6; else bR = B5;
        
        if (bR)
          {
            cpu.du.Z = 0;
            break;
          }
        
      }
    
    if (e->N1 < e->N2)
      {
        for(; cpu.du.CHTALLY < e->N2; cpu.du.CHTALLY += 1)
          {
            bool b1 = F;
            
            e->ADDR2.mode = eRWreadBit;
            bool b2 = EISgetBitRWNR(&e->ADDR2, true);
            
            if (b1) if (b2) bR = B8; else bR = B7; else if (b2) bR = B6; else bR = B5;
            
            if (bR)
            {
                //CLR_I_ZERO;
                cpu.du.Z = 0;
                break;
            }
        
          }
      }

    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);

    SC_I_ZERO (cpu.du.Z);
    if (e->N1 > e->N2)
      {
        // NOTES: If N1 > N2, the low order (N1-N2) bits of C(Y-bit1) are not
        // processed and the truncation indicator is set ON.
        //
        // If T = 1 and the truncation indicator is set ON by execution of the
        // instruction, then a truncation (overflow) fault occurs.
        
        SET_I_TRUNC;
        if (T && tstOVFfault ())
          doFault(FAULT_OFL, fst_zero, "csr truncation fault");
      }
    else
      {
        CLR_I_TRUNC;
      }
  }


/*
 * CMPB - Compare Bit Strings
 */

/*
 * get a bit from memory ....
 */
// XXX this is terribly inefficient, but it'll do for now ......

static bool EISgetBit(EISaddr *p, int *cpos, int *bpos)
{
#ifdef EIS_PTR
    long eisaddr_idx = EISADDR_IDX (p);
if (eisaddr_idx < 0 || eisaddr_idx > 2) { sim_warn ("IDX1"); return }
#endif
    
    if (!p)
    {
        //lastAddress = -1;
        return 0;
    }
    
    if (*bpos > 8)      // bits 0-8
    {
        *bpos = 0;
        *cpos += 1;
        if (*cpos > 3)  // chars 0-3
        {
            *cpos = 0;
#ifdef EIS_PTR
            cpu.du.Dk_PTR_W[eisaddr_idx] += 1;
            cpu.du.Dk_PTR_W[eisaddr_idx] &= AMASK;
#else
            p->address += 1;
            p->address &= AMASK;
#endif
        }
    }
    
    p->data = EISRead(p); // read data word from memory
    
    int charPosn = *cpos * 9;
    int bitPosn = charPosn + *bpos;
    bool b = getbits36_1 (p->data, (uint) bitPosn) != 0;
    
    *bpos += 1;
    
    return b;
}

void cmpb (void)
{
    EISstruct * e = & cpu.currentEISinstruction;

    
    // For i = 1, 2, ..., minimum (N1,N2)
    //   C(Y-bit1)i-1 :: C(Y-bit2)i-1
    // If N1 < N2, then for i = N1+1, N1+2, ..., N2
    //   C(FILL) :: C(Y-bit2)i-1
    // If N1 > N2, then for i = N2+l, N2+2, ..., N1
    //   C(Y-bit1)i-1 :: C(FILL)
    //
    // Indicators:
    //    Zero:  If C(Y-bit1)i = C(Y-bit2)i for all i, then ON; otherwise, OFF
    //    Carry: If C(Y-bit1)i < C(Y-bit2)i for any i, then OFF; otherwise ON
    
    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor(1, &mod_fault);
    setupOperandDescriptor(2, &mod_fault);
#endif
    
    parseBitstringOperandDescriptor(1, &mod_fault);
    parseBitstringOperandDescriptor(2, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bits 1-8 and 10 MBZ
    if (IWB_IRODD & 0377200000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "cmpb 1-8,10 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    int charPosn1 = (int) e->C1;
    int charPosn2 = (int) e->C2;
    
    int bitPosn1 = (int) e->B1;
    int bitPosn2 = (int) e->B2;
    
    bool F = getbits36_1 (cpu.cu.IWB, 0) != 0;   // fill bit

    SET_I_ZERO;  // assume all =
    SET_I_CARRY; // assume all >=
    
sim_debug (DBG_TRACEEXT, & cpu_dev, "cmpb N1 %d N2 %d\n", e -> N1, e -> N2);

#if 0
// RJ78: Notes 1:  If L1 or L2 = 0, both the Zero and Carry indicators are 
// turned ON, but no Illegal Procedure fault occurs.

// CAC: This makes sense if you s/or/and/; the behavior for the 'or' 
// condition is well-defined by the text, but the case of 'and' is
// not covered. However, this test is just an optimization -- the 
// code behaves this way for the 'and' case.

    //if (e -> N1 == 0 || e -> N2 == 0)
    if (e -> N1 == 0 && e -> N2 == 0)
      {
        return;
      }
#endif

    PNL (L68_ (if (max (e->N1, e->N2) < 128)
      DU_CYCLE_FLEN_128;))

    uint i;
    for(i = 0 ; i < min(e->N1, e->N2) ; i += 1)
    {
        bool b1 = EISgetBit (&e->ADDR1, &charPosn1, &bitPosn1);
        bool b2 = EISgetBit (&e->ADDR2, &charPosn2, &bitPosn2);
        
sim_debug (DBG_TRACEEXT, & cpu_dev, "cmpb(min(e->N1, e->N2)) i %d b1 %d b2 %d\n", i, b1, b2);
        if (b1 != b2)
        {
            CLR_I_ZERO;
            if (!b1 && b2)  // 0 < 1
                CLR_I_CARRY;

            cleanupOperandDescriptor (1);
            cleanupOperandDescriptor (2);

            return;
        }
        
    }
    if (e->N1 < e->N2)
    {
        for(; i < e->N2 ; i += 1)
        {
            bool b1 = F;
            bool b2 = EISgetBit(&e->ADDR2, &charPosn2, &bitPosn2);
sim_debug (DBG_TRACEEXT, & cpu_dev, "cmpb(e->N1 < e->N2) i %d b1fill %d b2 %d\n", i, b1, b2);
        
            if (b1 != b2)
            {
                CLR_I_ZERO;
                if (!b1 && b2)  // 0 < 1
                    CLR_I_CARRY;

                cleanupOperandDescriptor (1);
                cleanupOperandDescriptor (2);

                return;
            }
        }   
    } else if (e->N1 > e->N2)
    {
        for(; i < e->N1 ; i += 1)
        {
            bool b1 = EISgetBit(&e->ADDR1, &charPosn1, &bitPosn1);
            bool b2 = F;
sim_debug (DBG_TRACEEXT, & cpu_dev, "cmpb(e->N1 > e->N2) i %d b1 %d b2fill %d\n", i, b1, b2);
        
            if (b1 != b2)
            {
                CLR_I_ZERO;
                if (!b1 && b2)  // 0 < 1
                    CLR_I_CARRY;

                cleanupOperandDescriptor (1);
                cleanupOperandDescriptor (2);

                return;
            }
        }
    }
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
}

#if 0
/*
 * write 4-bit digits to memory @ pos (in reverse) ...
 */

static void EISwrite4r(EISaddr *p, int *pos, word4 char4)
{
#ifdef EIS_PTR
    long eisaddr_idx = EISADDR_IDX (p);
if (eisaddr_idx < 0 || eisaddr_idx > 2) { sim_warn ("IDX1"); return }
#endif
    word36 w;
    
    if (*pos < 0)    // out-of-range?
    {
        *pos = 7;    // reset to 1st byte
#ifdef EIS_PTR
        cpu.du.Dk_PTR_W[eisaddr_idx] = (cpu.du.Dk_PTR_W[eisaddr_idx] - 1) & AMASK;         // goto prev dstAddr in memory
#else
        p->address = (p->address - 1) & AMASK;         // goto prev dstAddr in memory
#endif
    }
    w = EISRead(p);
    
// AL39, Figure 2-3
    switch (*pos)
    {
        case 0:
            //w = bitfieldInsert36(w, char4, 31, 5);
            w = setbits36_4 (w, 1, char4);
            break;
        case 1:
            //w = bitfieldInsert36(w, char4, 27, 4);
            w = setbits36_4 (w, 5, char4);
            break;
        case 2:
            //w = bitfieldInsert36(w, char4, 22, 5);
            w = setbits36_4 (w, 10, char4);
            break;
        case 3:
            //w = bitfieldInsert36(w, char4, 18, 4);
            w = setbits36_4 (w, 14, char4);
            break;
        case 4:
            //w = bitfieldInsert36(w, char4, 13, 5);
            w = setbits36_4 (w, 19, char4);
            break;
        case 5:
            //w = bitfieldInsert36(w, char4, 9, 4);
            w = setbits36_4 (w, 23, char4);
            break;
        case 6:
            //w = bitfieldInsert36(w, char4, 4, 5);
            w = setbits36_4 (w, 28, char4);
            break;
        case 7:
            //w = bitfieldInsert36(w, char4, 0, 4);
            w = setbits36_4 (w, 32, char4);
            break;
    }
    
    //Write (*dstAddr, w, OperandWrite, 0); // XXX this is the inefficient part!
    EISWriteIdx(p, 0, w, true); // XXX this is the inefficient part!
    
    *pos -= 1;       // to prev byte.
}

/*
 * write 9-bit bytes to memory @ pos (in reverse)...
 */

static void EISwrite9r(EISaddr *p, int *pos, word9 char9)
{
#ifdef EIS_PTR
    long eisaddr_idx = EISADDR_IDX (p);
if (eisaddr_idx < 0 || eisaddr_idx > 2) { sim_warn ("IDX1"); return }
#endif
    word36 w;
    if (*pos < 0)    // out-of-range?
    {
        *pos = 3;    // reset to 1st byte
#ifdef EIS_PTR
        cpu.du.Dk_PTR_W[eisaddr_idx] = (cpu.du.Dk_PTR_W[eisaddr_idx] - 1) & AMASK;         // goto prev dstAddr in memory
#else
        p->address = (p->address - 1) & AMASK;        // goto next dstAddr in memory
#endif
    }
    
    w = EISRead(p);      // read dst memory into w
    
// AL39, Figure 2-5
    switch (*pos)
    {
        case 0:
            //w = bitfieldInsert36(w, char9, 27, 9);
            w = setbits36_9 (w, 0, char9);
            break;
        case 1:
            //w = bitfieldInsert36(w, char9, 18, 9);
            w = setbits36_9 (w, 9, char9);
            break;
        case 2:
            //w = bitfieldInsert36(w, char9, 9, 9);
            w = setbits36_9 (w, 18, char9);
            break;
        case 3:
            //w = bitfieldInsert36(w, char9, 0, 9);
            w = setbits36_9 (w, 27, char9);
            break;
    }
    
    //Write (*dstAddr, w, OperandWrite, 0); // XXX this is the inefficient part!
    EISWriteIdx(p, 0, w, true); // XXX this is the inefficient part!
    
    *pos -= 1;       // to prev byte.
}

/*
 * write char to output string in Reverse. Right Justified and taking into account string length of destination
 */

static void EISwriteToOutputStringReverse (int k, word9 charToWrite, bool * ovf)
{
    EISstruct * e = & cpu.currentEISinstruction;
    // first thing we need to do is to find out the last position is the buffer we want to start writing to.
    
    static int N = 0;           // length of output buffer in native chars (4, 6 or 9-bit chunks)
    static int CN = 0;          // character number 0-7 (4), 0-5 (6), 0-3 (9)
    static int TN = 0;          // type code
    static int pos = 0;         // current character position
    //static int size = 0;        // size of char
    static int _k = -1;         // k of MFk

    if (k)
    {
        _k = k;
        
        N = (int) e->N[k-1];      // length of output buffer in native chars (4, 9-bit chunks)
        CN = (int) e->CN[k-1];    // character number (position) 0-7 (4), 0-5 (6), 0-3 (9)
        TN = (int) e->TN[k-1];    // type code
        
        //int chunk = 0;
        int maxPos = 4;
        switch (TN)
        {
            case CTN4:
                //address = e->addr[k-1].address;
                //size = 4;
                //chunk = 32;
                maxPos = 8;
                break;
            case CTN9:
                //address = e->addr[k-1].address;
                //size = 9;
                //chunk = 36;
                maxPos = 4;
                break;
        }
        
        // since we want to write the data in reverse (since it's right 
        // justified) we need to determine the final address/CN for the 
        // type and go backwards from there
        
        //int numBits = size * N;      // 8 4-bit digits, 4 9-bit bytes / word
        // (remember there are 4 slop bits in a 36-bit word when dealing with 
        // BCD)

        // how many additional words will the N chars take up?
        //int numWords = numBits / ((TN == CTN4) ? 32 : 36);      

// CN+N    numWords  (CN+N+3)/4   lastChar
//   1       1                      0
//   2       1                      1
//   3       1                      2
//   4       1                      3
//   5       2                      0

        int numWords = (CN + N + (maxPos - 1)) / maxPos;
        int lastWordOffset = numWords - 1;
        int lastChar = (CN + N - 1) % maxPos;   // last character number
        
        if (lastWordOffset > 0)           // more that the 1 word needed?
        {
            //address += lastWordOffset;    // highest memory address
            PNL (cpu.du.Dk_PTR_W[k-1] += (word18) lastWordOffset);
            PNL (cpu.du.Dk_PTR_W[k-1] &= AMASK);
#ifdef EIS_PTR
            cpu.du.Dk_PTR_W[k-1] += (word18) lastWordOffset;
            cpu.du.Dk_PTR_W[k-1] &= AMASK;
#else
            e->addr[k-1].address += (word18) lastWordOffset;
            e->addr[k-1].address &= MASK18;
#endif
        }
        
        pos = lastChar;             // last character number
        return;
    }
    
    // any room left in output string?
    if (N <= 0)
    {
        * ovf = true;
        return;
    }
    
    // we should write character to word/pos in memory .....
    switch(TN)
    {
        case CTN4:
            EISwrite4r(&e->addr[_k-1], &pos, (word4) charToWrite);
            break;
        case CTN9:
            EISwrite9r(&e->addr[_k-1], &pos, charToWrite);
            break;
    }

    N -= 1;
}
#endif

/*
 * determine sign of N*9-bit length word
 */
static bool sign9n(word72 n128, int N)
{
    
    // sign bit of  9-bit is bit 8  (1 << 8)
    // sign bit of 18-bit is bit 17 (1 << 17)
    // .
    // .
    // .
    // sign bit of 72-bit is bit 71 (1 << 71)
    
    if (N < 1 || N > 8) // XXX largest int we'll play with is 72-bits? Makes sense
        return false;
    
#ifdef NEED_128
    word72 sgnmask = lshift_128 (construct_128 (0, 1), (uint) (N * 9 - 1));
    return isnonzero_128 (and_128 (sgnmask, n128));
#else
    word72 sgnmask = (word72)1 << ((N * 9) - 1);
    
    return (bool)(sgnmask & n128);
#endif
}

/*
 * sign extend a N*9 length word to a (word72) 128-bit word
 */
static word72s signExt9(word72 n128, int N)
{
    // ext mask for  9-bit = 037777777777777777777777777777777777777400  8 0's
    // ext mask for 18-bit = 037777777777777777777777777777777777400000 17 0's
    // ext mask for 36-bit = 037777777777777777777777777777400000000000 35 0's
    // etc...
    
    int bits = (N * 9) - 1;
    if (sign9n(n128, N))
    {
#ifdef NEED_128
        uint128 extBits = lshift_128 (construct_128 (MASK64, MASK64), (uint) bits);
        uint128 or = or_128 (n128, extBits);
        return cast_s128 (or);
#else
        uint128 extBits = ((uint128)-1 << bits);
        return (word72s) (n128 | extBits);
#endif
    }
#ifdef NEED_128
    uint128 zeroBits = complement_128 (lshift_128 (construct_128 (MASK64, MASK64), (uint) bits));
    uint128 and = and_128 (n128, zeroBits);
    return cast_s128 (and);
#else
    uint128 zeroBits = ~((uint128)-1 << bits);
    return (word72s) (n128 & zeroBits);
#endif
}

/*
 * load a 9*n bit integer into e->x ...
 */

static void load9x(int n, EISaddr *addr, int pos)
{
    EISstruct * e = & cpu.currentEISinstruction;
#ifdef NEED_128
    word72 x = construct_128 (0, 0);
#else
    word72 x = 0;
#endif
#ifdef EIS_PTR
    long eisaddr_idx = EISADDR_IDX (addr);
if (eisaddr_idx < 0 || eisaddr_idx > 2) { sim_warn ("IDX1"); return }
#endif
    
    word36 data = EISRead(addr);
    
    int m = n;
    while (m)
    {
#ifdef NEED_128
        x = lshift_128 (x, 9);
#else
        x <<= 9;         // make room for next 9-bit byte
#endif
        
        if (pos > 3)        // overflows to next word?
        {   // yep....
            pos = 0;        // reset to 1st byte
#if EIS_PTR
            cpu.du.Dk_PTR_W[eisaddr_idx] = (cpu.du.Dk_PTR_W[eisaddr_idx] + 1) & AMASK;          // bump source to next address
#else
            addr->address = (addr->address + 1) & AMASK;          // bump source to next address
#endif
            data = EISRead(addr);    // read it from memory
        }
        
#ifdef NEED_128
        x = or_128 (x, construct_128 (0, GETBYTE (data, pos)));
#else
        x |= GETBYTE(data, pos);   // fetch byte at position pos and 'or' it in
#endif
        
        pos += 1;           // onto next posotion
        
        m -= 1;             // decrement byte counter
    }
    e->x = signExt9(x, n);  // form proper 2's-complement integer
}

#if 0
/*
 * get sign to buffer position p
 */

static word9 getSign (word72s n128)
{
    EISstruct * e = & cpu.currentEISinstruction;
    // 4- or 9-bit?
    if (e->TN2 == CTN4) // 4-bit
    {
        // If P=1, positive signed 4-bit results are stored using octal 13 as the plus sign.
        // If P=0, positive signed 4-bit results are stored with octal 14 as the plus sign.
        if (n128 >= 0)
        {
            if (e->P)
                return 013;  // alternate + sign
            else
                return 014;  // default + sign
        }
        else
        {
            SETF(e->_flags, I_NEG); 
            return 015;      // - sign
        }
    }
    else
    {   // 9-bit
        if (n128 >= 0)
            return 053;     // default 9-bit +
        else
        {
            SETF(e->_flags, I_NEG);
            return 055;     // default 9-bit -
        }
    }
}


// perform a binary to decimal conversion ...

// Since (according to DH02) we want to "right-justify" the output string it
// might be better to presere the reverse writing and start writing
// characters directly into the output string taking into account the output
// string length.....

static void _btd (bool * ovfp)
{
    EISwriteToOutputStringReverse(2, 0, ovfp);    // initialize output writer .....
    
    PNL (L68_ (DU_CYCLE_DGBD;))

    EISstruct * e = & cpu.currentEISinstruction;
    * ovfp = false;

    word72s n128 = e->x;    // signExt9(e->x, e->N1);          // adjust for +/-

    SC_I_ZERO (n128 == 0);
    SC_I_NEG (n128 == 0);
   
    int sgn = (n128 < 0) ? -1 : 1;  // sgn(x)
    if (n128 < 0)
        n128 = -n128;
    
    int N = (int) e->N2;  // number of chars to write ....
    
    // handle any trailing sign stuff ...
    if (e->S2 == CSTS)  // a trailing sign
    {
        EISwriteToOutputStringReverse(0, getSign(sgn), ovfp);
        if (! * ovfp)
          N -= 1;
    }
    if (! * ovfp)
    {
        do
        {
            int n = n128 % 10;
        
            EISwriteToOutputStringReverse(0, (word9) ((e->TN2 == CTN4) ? n : (n + '0')), ovfp);
        
            if (* ovfp)   // Overflow! Too many chars, not enough room!
                break;
        
            N -= 1;
        
            n128 /= 10;
        } while (n128);
     }
    
    // at this point we've exhausted our digits, but may still have spaces left.
    
    // handle any leading sign stuff ...
    if (! * ovfp)
    {
        if (e->S2 == CSLS)  // a leading sign
        {
            while (N > 1)
            {
                EISwriteToOutputStringReverse(0, (e->TN2 == CTN4) ? 0 : '0', ovfp);
                if (* ovfp)
                    break;
                N -= 1;
            }
            if (! * ovfp)
              EISwriteToOutputStringReverse(0, getSign(sgn), ovfp);
        }
        else
        {
            while (N > 0)
            {
                EISwriteToOutputStringReverse(0, (e->TN2 == CTN4) ? 0 : '0', ovfp);
                if (* ovfp)
                    break;
                N -= 1;
            }
        }
    }
}
#endif

void btd (void)
{
    EISstruct * e = & cpu.currentEISinstruction;

    
    // C(Y-char91) converted to decimal → C(Y-charn2)
    /*!
     * C(Y-char91) contains a twos complement binary integer aligned on 9-bit
     * character boundaries with length 0 < N1 <= 8.
     *
     * If TN2 and S2 specify a 4-bit signed number and P = 1, then if
     * C(Y-char91) is positive (bit 0 of C(Y-char91)0 = 0), then the 13(8) plus
     * sign character is moved to C(Y-charn2) as appropriate.
     *
     *   The scaling factor of C(Y-charn2), SF2, must be 0.
     *
     *   If N2 is not large enough to hold the digits generated by conversion
     *   of C(Y-char91) an overflow condition exists; the overflow indicator is
     *   set ON and an overflow fault occurs. This implies that an unsigned
     *   fixed-point receiving field has a minimum length of 1 character and a
     *   signed fixed- point field, 2 characters.
     *
     * If MFk.RL = 1, then Nk does not contain the operand length; instead; it
     * contains a register code for a register holding the operand length.
     *
     * If MFk.ID = 1, then the kth word following the instruction word does not
     * contain an operand descriptor; instead, it contains an indirect pointer
     * to the operand descriptor.
     *
     * C(Y-char91) and C(Y-charn2) may be overlapping strings; no check is made.
     *
     * Attempted conversion to a floating-point number (S2 = 0) or attempted
     * use of a scaling factor (SF2 ≠ 0) causes an illegal procedure fault.
     *
     * If N1 = 0 or N1 > 8 an illegal procedure fault occurs.
     *
     * Attempted execution with the xed instruction causes an illegal procedure fault.
     *
     * Attempted repetition with the rpt, rpd, or rpl instructions causes an illegal procedure fault.
     *
     */
    
    // C(string 1) -> C(string 2) (converted)
    
    // The two's complement binary integer starting at location YC1 is
    // converted into a signed string of decimal characters of data type TN2,
    // sign and decimal type S2 (S2 = 00 is illegal) and scale factor 0; and is
    // stored, right-justified, as a string of length L2 starting at location
    // YC2. If the string generated is longer than L2, the high-order excess is
    // truncated and the overflow indicator is set. If strings 1 and 2 are not
    // overlapped, the contents of string 1 remain unchanged. The length of
    // string 1 (L1) is given as the number of 9-bit segments that make up the
    // string. L1 is equal to or is less than 8. Thus, the binary string to be
    // converted can be 9, 18, 27, 36, 45, 54, 63, or 72 bits long. CN1
    // designates a 9-bit character boundary. If P=1, positive signed 4-bit
    // results are stored using octal 13 as the plus sign. If P=0, positive
    // signed 4-bit results are stored with octal 14 as the plus sign.

    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor(1, &mod_fault);
    setupOperandDescriptor(2, &mod_fault);
#endif

    parseNumericOperandDescriptor(1, &mod_fault);
    parseNumericOperandDescriptor(2, &mod_fault);
    
#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    // Bits 1-10 MBZ 
    if (IWB_IRODD & 0377600000000)
      {
        //sim_printf ("sb2d %012"PRIo64"\n", IWB_IRODD);
        doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "btd 0-8 MBZ");
      }

    // Bits 21-29 of OP1 MBZ
    if (!(e->MF[0] & MFkID) && e -> op [0]  & 0000000077700)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "btd op1 21-29 MBZ");

    // Bits 24-29 of OP2 MBZ
    if (!(e->MF[1] & MFkID) && e -> op [1]  & 0000000007700)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "btd op2 24-29 MBZ");

    if (e->S[1] == 0)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "btd op2 S=0");

#ifdef DPS8M
    // DPS8 raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    e->P = getbits36_1 (cpu.cu.IWB, 0) != 0;  // 4-bit data sign character control

    if (e->N1 == 0 || e->N1 > 8)
        doFault(FAULT_IPR, fst_ill_proc, "btd(1): N1 == 0 || N1 > 8"); 

    uint dstTN = e->TN2;    // type of chars in dst
    uint dstCN = e->CN2;    // starting at char pos CN

    int n2 = 0;

    switch(e->S2)
    {
        case CSLS:
        case CSTS:
            n2 = (int) e->N2 - 1; // 1 sign
            break;
        
        case CSNS:
            n2 = (int) e->N2;     // no sign
            break;          // no sign wysiwyg
    }
    
    sim_debug (DBG_CAC, & cpu_dev,
      "n2 %d\n",
      n2);

    if (n2 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "btd adjusted n2<1");


    decContext set;
    decContextDefaultDPS8(&set);
    set.traps=0;

    load9x((int) e->N1, &e->ADDR1, (int) e->CN1);

    // handle sign
    e->sign = 1;
#ifdef NEED_128
    word72 x = cast_128 (e->x);
    if (islt_s128 (e->x, construct_s128 (0, 0)))
      {
        e->sign = -1;
        x = and_128 (negate_128 (x), MASK72);

      }

    // convert to decimal string, workaround missing sprintf uint128
    char tmp[32];
    tmp[31] = 0;
    int i;
    for (i=30;i>=0;i--) {
        //tmp[i] = x%10 + '0';
        //x /= 10;
        uint16_t r;
        x = divide_128_16 (x, 10, & r);
        tmp[i] = (char) r + '0';
        if (iszero_128 (x)) 
            break;
    }
#else
    word72 x = (word72)e->x;
    if (e->x < 0) {
        e->sign = -1;
        x = (-x) & MASK72;
    }

    // convert to decimal string, workaround missing sprintf uint128
    char tmp[32];
    tmp[31] = 0;
    int i;
    for (i=30;i>=0;i--) {
        tmp[i] = x%10 + '0';
        x /= 10;
        if (x == 0) 
            break;
    }
#endif

    decNumber _1;
    decNumber *op1 = decNumberFromString(&_1, tmp+i, &set);
    if (e->sign == -1)
        op1->bits |= DECNEG;

    bool Ovr = false, Trunc = false;
    
    uint8_t out [256];
    char * res = formatDecimal (out, &set, op1, n2, (int) e->S2, e->SF2, 0, &Ovr, &Trunc);

    // now write to memory in proper format.....

    //word18 dstAddr = e->dstAddr;
    int pos = (int) dstCN;
    
    // 1st, take care of any leading sign .......
    switch(e->S2)
    {
        case CSLS:  // fixed-point, leading sign
            switch(dstTN)
            {
                case CTN4:
                    if (e->P) //If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus sign character is placed appropriately if the result of the operation is positive.
                        EISwrite49(&e->ADDR2, &pos, (int) dstTN, (decNumberIsNegative(op1) && !decNumberIsZero(op1)) ? 015 : 013);  // special +
                    else
                        EISwrite49(&e->ADDR2, &pos, (int) dstTN, (decNumberIsNegative(op1) && !decNumberIsZero(op1)) ? 015 : 014);  // default +
                    break;
                case CTN9:
                    EISwrite49(&e->ADDR2, &pos, (int) dstTN, (decNumberIsNegative(op1) && !decNumberIsZero(op1)) ? '-' : '+');
                    break;
            }
            break;
        
        case CSTS:  // nuttin' to do here .....
        case CSNS:
            break;  // no sign wysiwyg
    }
    
    // 2nd, write the digits .....
    for(int i = 0 ; i < n2 ; i++)
        switch(dstTN)
        {
            case CTN4:
                EISwrite49(&e->ADDR2, &pos, (int) dstTN, (word9) (res[i] - '0'));
                break;
            case CTN9:
                EISwrite49(&e->ADDR2, &pos, (int) dstTN, (word9) res[i]);
                break;
        }
    
    // 3rd, take care of any trailing sign or exponent ...
    switch(e->S2)
    {
        case CSTS:  // write trailing sign ....
            switch(dstTN)
            {
                case CTN4:
                    if (e->P) //If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus sign character is placed appropriately if the result of the operation is positive.
                        EISwrite49(&e->ADDR2, &pos, (int) dstTN, (decNumberIsNegative(op1) && !decNumberIsZero(op1)) ? 015 :  013);  // special +
                    else
                        EISwrite49(&e->ADDR2, &pos, (int) dstTN, (decNumberIsNegative(op1) && !decNumberIsZero(op1)) ? 015 :  014);  // default +
                    break;
            
                case CTN9:
                    EISwrite49(&e->ADDR2, &pos, (int) dstTN, (decNumberIsNegative(op1) && !decNumberIsZero(op1)) ? '-' : '+');
                    break;
            }
            break;
        
        case CSLS:  // fixed point, leading sign - already done
        case CSNS:  // fixed point, unsigned - nuttin' needed to do
            break;
    }
    
    SC_I_NEG (decNumberIsNegative(op1) && !decNumberIsZero(op1));  // set negative indicator if op3 < 0
    SC_I_ZERO (decNumberIsZero(op1));     // set zero indicator if op3 == 0
    
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    
    if (Ovr)
    {
        SET_I_OFLOW;
        if (tstOVFfault ())
          doFault(FAULT_OFL, fst_zero, "btd overflow fault");
    }
}

#if 0
/*
 * load a decimal number into e->x ...
 */

static int loadDec (EISaddr *p, int pos)
{
    EISstruct * e = & cpu.currentEISinstruction;
    int128 x = 0;
#ifdef EIS_PTR
    long eisaddr_idx = EISADDR_IDX (p);
if (eisaddr_idx < 0 || eisaddr_idx > 2) { sim_warn ("IDX1"); return }
#endif
    
    p->data = EISRead(p);    // read data word from memory
    
    int maxPos = e->TN1 == CTN4 ? 7 : 3;

    int sgn = 1;
    
    sim_debug (DBG_TRACEEXT, & cpu_dev,
      "loadDec: maxPos %d N1 %d\n", maxPos, e->N1);
    for(uint n = 0 ; n < e->N1 ; n += 1)
    {
        if (pos > maxPos)   // overflows to next word?
        {   // yep....
            pos = 0;        // reset to 1st byte
#if EIS_PTR
            cpu.du.Dk_PTR_W[eisaddr_idx] = (cpu.du.Dk_PTR_W[eisaddr_idx] + 1) & AMASK;          // bump source to next address
#else
            p->address = (p->address + 1) & AMASK;      // bump source to next address
#endif
            p->data = EISRead(p);    // read it from memory
        }
        
        int c = 0;
        switch(e->TN1)
        {
            case CTN4:
                c = (word4)get4(p->data, pos);
                break;
            case CTN9:
                c = GETBYTE(p->data, pos);
                break;
        }
        sim_debug (DBG_TRACEEXT, & cpu_dev,
          "loadDec: n %d c %d(%o)\n", n, c, c);
        
        // per CAC suggestion
        if (n == 0 && c == 0)           // treat as +0
        {
            return -1;
        }
        
        if (n == 0 && e->TN1 == CTN4 && e->S1 == CSLS)
        {
            sim_debug (DBG_TRACEEXT, & cpu_dev,
              "loadDec: n 0, TN1 CTN4, S1 CSLS\n");
            switch (c)
            {
                case 015:   // 6-bit - sign
                    SETF(e->_flags, I_NEG);
                    
                    sgn = -1;
                    break;
                case 013:   // alternate 4-bit + sign
                case 014:   // default   4-bit + sign
                    break;
                default:
                    // not a leading sign
                    doFault(FAULT_IPR, fst_ill_proc, "loadDec(): no leading sign (1)");
            }
            pos += 1;           // onto next posotion
            continue;
        }

        if (n == 0 && e->TN1 == CTN9 && e->S1 == CSLS)
        {
            sim_debug (DBG_TRACEEXT, & cpu_dev,
              "loadDec: n 0, TN1 CTN9, S1 CSLS\n");
            switch (c)
            {
                case '-':
                    SETF(e->_flags, I_NEG);
                    
                    sgn = -1;
                    break;
                case '+':
                    break;
                default:
                    // not a leading sign
                    doFault(FAULT_IPR, fst_ill_proc, "loadDec(): no leading sign (2)");
            }
            pos += 1;           // onto next posotion
            continue;
        }

        if (n == e->N1-1 && e->TN1 == CTN4 && e->S1 == CSTS)
        {
            sim_debug (DBG_TRACEEXT, & cpu_dev,
              "loadDec: n N1-1, TN1 CTN4, S1 CSTS\n");
            switch (c)
            {
                case 015:   // 4-bit - sign
                    SETF(e->_flags, I_NEG);
                    
                    sgn = -1;
                    break;
                case 013:   // alternate 4-bit + sign
                case 014:   // default   4-bit + sign
                    break;
                default:
                    // not a trailing sign
                    doFault(FAULT_IPR, fst_ill_proc, "loadDec(): no trailing sign (1)");
            }
            break;
        }

        if (n == e->N1-1 && e->TN1 == CTN9 && e->S1 == CSTS)
        {
            sim_debug (DBG_TRACEEXT, & cpu_dev,
              "loadDec: n N1-1, TN1 CTN9, S1 CSTS\n");
            switch (c)
            {
                case '-':
                    SETF(e->_flags, I_NEG);
                    
                    sgn = -1;
                    break;
                case '+':
                    break;
                default:
                    // not a trailing sign
                    doFault(FAULT_IPR, fst_ill_proc, "loadDec(): no trailing sign (2)");
            }
            break;
        }
        
        x *= 10;
        x += c & 0xf;
        sim_debug (DBG_TRACEEXT, & cpu_dev,
              "loadDec:  x %"PRId64"\n", (int64) x);
        
        pos += 1;           // onto next posotion
    }
    
    e->x = sgn * x;
    sim_debug (DBG_TRACEEXT, & cpu_dev,
      "loadDec:  final x %"PRId64"\n", (int64) x);
    
    return 0;
}

static void EISwriteToBinaryStringReverse(EISaddr *p, int k)
{
    EISstruct * e = & cpu.currentEISinstruction;
#ifdef EIS_PTR
    long eisaddr_idx = EISADDR_IDX (p);
if (eisaddr_idx < 0 || eisaddr_idx > 2) { sim_warn ("IDX1"); return }
#endif
    /// first thing we need to do is to find out the last position is the buffer we want to start writing to.
    
    int N = (int) e->N[k-1];            // length of output buffer in native chars (4, 6 or 9-bit chunks)
    int CN = (int) e->CN[k-1];          // character number 0-3 (9)
    //word18 address  = e->YChar9[k-1]; // current write address
    
    /// since we want to write the data in reverse (since it's right justified) we need to determine
    /// the final address/CN for the type and go backwards from there
    
    int numBits = 9 * N;               // 4 9-bit bytes / word
    //int numWords = numBits / 36;       // how many additional words will the N chars take up?
    //int numWords = (numBits + CN * 9) / 36;       // how many additional words will the N chars take up?
    int numWords = (numBits + CN * 9 + 35) / 36;       // how many additional words will the N chars take up?
    // convert from count to offset
    int lastWordOffset = numWords - 1;
    int lastChar = (CN + N - 1) % 4;   // last character number
    
    if (lastWordOffset > 0)           // more that the 1 word needed?
      {
#ifdef EIS_PTR
        cpu.du.Dk_PTR_W[eisaddr_idx] += (word18) lastWordOffset;    // highest memory address
        cpu.du.Dk_PTR_W[eisaddr_idx] &= AMASK;
#else
        p->address += (word18) lastWordOffset;    // highest memory address
#endif
      }
    int pos = lastChar;             // last character number
    
    int128 x = e->x;
    
    for(int n = 0 ; n < N ; n += 1)
    {
        word9 charToWrite = x & 0777; // get 9-bits of data
        x >>=9;
        
        // we should write character to word/pos in memory .....
        //write9r(e, &address, &pos, charToWrite);
        EISwrite9r(p, &pos, charToWrite);
    }
    
    // anything left in x?. If it's not all 1's we have an overflow!
    if (~x && x != 0)    // if it's all 1's this will be 0
        SETF(e->_flags, I_OFLOW);
}
#endif

void dtb (void)
{
    EISstruct * e = & cpu.currentEISinstruction;

    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor(1, &mod_fault);
    setupOperandDescriptor(2, &mod_fault);
#endif
    
    PNL (L68_ (DU_CYCLE_DGDB;))

    parseNumericOperandDescriptor(1, &mod_fault);
    parseNumericOperandDescriptor(2, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
   
    // Bits 0 to 10 of the instruction Must Be Zero. So Say We ISOLTS.
    uint mbz = (uint) getbits36 (IWB_IRODD, 0, 11);
    if (mbz)
      {
        doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "dtb(): 0-10 MBZ");
      }

    // Bits 24-29 of OP1 MBZ
    if (!(e->MF[0] & MFkID) && e -> op [0]  & 0000000007700)
      {
        doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "dtb op1 24-29 MBZ");
      }

    // Bits 21-29 of OP2 MBZ
    if (!(e->MF[1] & MFkID) && e -> op [1]  & 0000000077700)
      {
        doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "dtb op2 21-29 MBZ");
       }

    // Attempted conversion of a floating-point number (S1 = 0) or attempted 
    // use of a scaling factor (SF1 ≠ 0) causes an illegal procedure fault.
    if (e->S1 == 0 || e->SF1 != 0)
    {
        doFault(FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC|mod_fault}, "dtb():  S1=0 or SF1!=0");
    }

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    // If N2 = 0 or N2 > 8 an illegal procedure fault occurs.
    if (e->N2 == 0 || e->N2 > 8)
    {
        doFault(FAULT_IPR, fst_ill_proc, "dtb():  N2 = 0 or N2 > 8 etc.");
    }


    int n1 = 0;
    
    /*
     * Here we need to distinguish between 4 type of numbers.
     *
     * CSFL - Floating-point, leading sign
     * CSLS - Scaled fixed-point, leading sign
     * CSTS - Scaled fixed-point, trailing sign
     * CSNS - Scaled fixed-point, unsigned
     */
    
    // determine precision
    switch(e->S1)
    {
        case CSLS:
        case CSTS:
            n1 = (int) e->N1 - 1; // only 1 sign
            break;
        
        case CSNS:
            n1 = (int) e->N1;     // no sign
            break;  // no sign wysiwyg
    }
    // RJ78: An Illegal Procedure fault occurs if:
    // N1 is not large enough to specify the number of characters required for the
    // specified sign and/or exponent, plus at least one digit. 

    if (n1 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "dtb adjusted n1<1");


    EISloadInputBufferNumeric (1);   // according to MF1

    // prepare output mask
#ifdef NEED_128
    word72 msk = subtract_128 (lshift_128 (construct_128 (0, 1), (9*e->N2-1)),construct_128 (0, 1));
#else
    word72 msk = ((word72)1<<(9*e->N2-1))-1; // excluding sign
#endif

#if 0
    decNumber _1;
    decNumber *op1 = decBCD9ToNumber(e->inBuffer, n1, 0, &_1);
    if (e->sign == -1)
        op1->bits |= DECNEG;
    if (decNumberIsZero(op1))
        op1->exponent = 127;

sim_printf("dtb: N1 %d N2 %d nin %d CN1 %d CN2 %d msk %012"PRIo64" %012"PRIo64"\n",e->N1,e->N2,n1,e->CN1,e->CN2,(word36)((msk >> 36) & DMASK), (word36)(msk & DMASK));
    PRINTDEC("dtb input (op1)", op1);
#endif

    // input is unscaled fixed point, so just get the digits
    bool Ovr = false;
#ifdef NEED_128
    word72 x = construct_128 (0, 0);
    for (int i = 0; i < n1; i++) {
        //x *= 10;
        x = multiply_128 (x, construct_128 (0, 10));
        //x += e->inBuffer[i];
        x = add_128 (x, construct_128 (0, (uint) e->inBuffer[i]));
        //Ovr |= x>msk?1:0;
        Ovr |= isgt_128 (x, msk) ? 1 : 0;
        //x &= msk; // multiplication and addition mod msk+1
        x = and_128 (x, msk); // multiplication and addition mod msk+1
    }
    if (e->sign == -1)
        //x = -x; // no need to mask it
        x = negate_128 (x); // no need to mask it

#else
    word72 x = 0;
    for (int i = 0; i < n1; i++) {
        x *= 10;
        x += e->inBuffer[i];
        //sim_printf("%d %012"PRIo64" %012"PRIo64"\n",e->inBuffer[i],(word36)((x >> 36) & DMASK), (word36)(x & DMASK));
        Ovr |= x>msk?1:0;
        x &= msk; // multiplication and addition mod msk+1
    }
    if (e->sign == -1)
        x = -x; // no need to mask it

    //sim_printf ("dtb out %012"PRIo64" %012"PRIo64"\n", (word36)((x >> 36) & DMASK), (word36)(x & DMASK));
#endif
    int pos = (int)e->CN2;

    // now write to memory in proper format.....

    int shift = 9*((int)e->N2-1);
    for(int i = 0; i < (int)e->N2; i++) {
#ifdef NEED_128
        EISwrite9(&e->ADDR2, &pos, (word9) rshift_128 (x, (uint) shift).l & 0777);
#else
        EISwrite9(&e->ADDR2, &pos, (word9) (x >> shift )& 0777);
#endif
        shift -= 9;
    }

    SC_I_NEG (e->sign == -1);  // set negative indicator
#ifdef NEED_128
    SC_I_ZERO (iszero_128 (x)); // set zero indicator
#else
    SC_I_ZERO (x==0);     // set zero indicator
#endif
    
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    
    if (Ovr)
    {
        SET_I_OFLOW;
        if (tstOVFfault ())
          doFault(FAULT_OFL, fst_zero, "dtb overflow fault");
    }
}

/*
 * decimal EIS instructions ... 
 */


#define ASC(x)  ((x) + '0')

/*
 * ad2d - Add Using Two Decimal Operands
 */

void ad2d (void)
{
    EISstruct * e = & cpu.currentEISinstruction;

    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor(1, &mod_fault);
    setupOperandDescriptor(2, &mod_fault);
    setupOperandDescriptorCache(3);
#endif
    
    parseNumericOperandDescriptor(1, &mod_fault);
    parseNumericOperandDescriptor(2, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bits 1-8 MBZ
    if (IWB_IRODD & 0377000000000)
      doFault (FAULT_IPR, fst_ill_op, "ad2d 1-8 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    e->P = getbits36_1 (cpu.cu.IWB, 0) != 0;  // 4-bit data sign character control
    bool T = getbits36_1 (cpu.cu.IWB, 9) != 0;  // truncation bit
    bool R = getbits36_1 (cpu.cu.IWB, 10) != 0;  // rounding bit

    PNL (L68_ (if (R)
      DU_CYCLE_FRND;))
    
    uint srcTN = e->TN1;    // type of chars in src
    
    uint dstTN = e->TN2;    // type of chars in dst
    uint dstCN = e->CN2;    // starting at char pos CN

    e->ADDR3 = e->ADDR2;
    
    int n1 = 0, n2 = 0, sc1 = 0, sc2 = 0;
    
    /*
     * Here we need to distinguish between 4 type of numbers.
     *
     * CSFL - Floating-point, leading sign
     * CSLS - Scaled fixed-point, leading sign
     * CSTS - Scaled fixed-point, trailing sign
     * CSNS - Scaled fixed-point, unsigned
     */
    
    // determine precision
    switch(e->S1)
    {
        case CSFL:
            n1 = (int) e->N1 - 1; // need to account for the - sign
            if (srcTN == CTN4)
                n1 -= 2;    // 2 4-bit digits exponent
            else
                n1 -= 1;    // 1 9-bit digit exponent
            sc1 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n1 = (int) e->N1 - 1; // only 1 sign
            sc1 = -e->SF1;
            break;
            
        case CSNS:
            n1 = (int) e->N1;     // no sign
            sc1 = -e->SF1;
            break;  // no sign wysiwyg
    }

    if (n1 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "ad2d adjusted n1<1");

    switch(e->S2)
    {
        case CSFL:
            n2 = (int) e->N2 - 1; // need to account for the sign
            if (e->TN2 == CTN4)
                n2 -= 2;    // 2 4-bit digit exponent
            else
                n2 -= 1;    // 1 9-bit digit exponent
            sc2 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n2 = (int) e->N2 - 1; // 1 sign
            sc2 = -e->SF2;
            break;
            
        case CSNS:
            n2 = (int) e->N2;     // no sign
            sc2 = -e->SF2;
            break;  // no sign wysiwyg
    }

    if (n2 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "ad2d adjusted n2<1");
    

    decContext set;
    //decContextDefault(&set, DEC_INIT_BASE);         // initialize
    decContextDefaultDPS8(&set);
    
    set.traps=0;
    
    decNumber _1, _2, _3;
    
    EISloadInputBufferNumeric (1);   // according to MF1
    
    decNumber *op1 = decBCD9ToNumber(e->inBuffer, n1, sc1, &_1);
    if (e->sign == -1)
        op1->bits |= DECNEG;
    if (e->S1 == CSFL)
        op1->exponent = e->exponent;
    
    EISloadInputBufferNumeric (2);   // according to MF2

    decNumber *op2 = decBCD9ToNumber(e->inBuffer, n2, sc2, &_2);
    if (e->sign == -1)
        op2->bits |= DECNEG;
    if (e->S2 == CSFL)
        op2->exponent = e->exponent;
    
    decNumber *op3 = decNumberAdd(&_3, op1, op2, &set);

    // ISOLTS 846 07c, 10a, 11b internal register overflow - see ad3d
    bool iOvr = 0;
    if (op3->digits > 63) {
        uint8_t pr[256];
        // if sf<=0, trailing zeroes can't be shifted out
        // if sf> 0, (some of) trailing zeroes can be shifted out 
        int sf = e->S3==CSFL?op3->exponent:e->SF3;

        int ctz = 0;
        if (sf>0) {	// optimize: we don't care when sf>0
            decNumberGetBCD(op3,pr);
            for (int i=op3->digits-1;i>=0 && pr[i]==0;i--)
                 ctz ++;
        }

        if (op3->digits - min(max(sf,0),ctz) > 63) {

            enum rounding safeR = decContextGetRounding(&set);         // save rounding mode
            int safe = set.digits;
            decNumber tmp;

            // discard MS digits
            decContextSetRounding(&set, DEC_ROUND_DOWN);     // Round towards 0 (truncation).
            set.digits = op3->digits - min(max(sf,0),ctz) - 63;
            decNumberPlus(&tmp, op3, &set);
            set.digits = safe;

            decNumberSubtract(op3, op3, &tmp, &set); 

            //decNumberToString(op3,(char*)pr); sim_printf("discarded: %s\n",pr);

            decContextSetRounding(&set, safeR);
            iOvr = 1;
        }
    }

    bool Ovr = false, EOvr = false, Trunc = false;

    uint8_t out [256];
    char *res = formatDecimal(out, &set, op3, n2, (int) e->S2, e->SF2, R, &Ovr, &Trunc);

    Ovr |= iOvr;

    if (decNumberIsZero(op3))
        op3->exponent = 127;
    
    //printf("%s\r\n", res);
    
    // now write to memory in proper format.....
    
    //word18 dstAddr = e->dstAddr;
    int pos = (int) dstCN;
    
    // 1st, take care of any leading sign .......
    switch(e->S2)
    {
        case CSFL:  // floating-point, leading sign.
        case CSLS:  // fixed-point, leading sign
            switch(dstTN)
            {
                case CTN4:
                    if (e->P) //If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus sign character is placed appropriately if the result of the operation is positive.
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  013);  // special +
                    else
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  014);  // default +
                    break;
                case CTN9:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? '-' : '+');
                    break;
            }
            break;
            
        case CSTS:  // nuttin' to do here .....
        case CSNS:
            break;  // no sign wysiwyg
    }

    // 2nd, write the digits .....
    for(int j = 0 ; j < n2 ; j++)
        switch(dstTN)
        {
            case CTN4:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, (word9) (res[j] - '0'));
                break;
            case CTN9:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, (word9) res[j]);
                break;
        }
    
    // 3rd, take care of any trailing sign or exponent ...
    switch(e->S2)
    {
        case CSTS:  // write trailing sign ....
            switch(dstTN)
            {
                case CTN4:
                    if (e->P) //If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus sign character is placed appropriately if the result of the operation is positive.
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  013);  // special +
                    else
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  014);  // default +
                    break;
                case CTN9:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? '-' : '+');
                    break;
            }
            break;
            
        case CSFL:  // floating-point, leading sign.
            // write the exponent
            switch(dstTN)
            {
                case CTN4:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (op3->exponent >> 4) & 0xf); // upper 4-bits
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN,  op3->exponent       & 0xf); // lower 4-bits
                    break;
                case CTN9:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, op3->exponent & 0xff);    // write 8-bit exponent
                    break;
            }
            break;
            
        case CSLS:  // fixed point, leading sign - already done
        case CSNS:  // fixed point, unsigned - nuttin' needed to do
            break;
    }
    
    // set flags, etc ...
    if (e->S2 == CSFL)
    {
        if (op3->exponent > 127)
        {
            SET_I_EOFL;
            EOvr = true;
        }
        if (op3->exponent < -128)
        {
            SET_I_EUFL;
            EOvr = true;
        }
    }
    
    SC_I_NEG (decNumberIsNegative(op3) && !decNumberIsZero(op3));  // set negative indicator if op3 < 0
    SC_I_ZERO (decNumberIsZero(op3));     // set zero indicator if op3 == 0
    
    SC_I_TRUNC (!R && Trunc); // If the truncation condition exists without rounding, then ON; otherwise OFF
    
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    cleanupOperandDescriptor (3);

    if (TST_I_TRUNC && T && tstOVFfault ())
      doFault(FAULT_OFL, fst_zero, "ad2d truncation(overflow) fault");
    if (EOvr && tstOVFfault ())
        doFault(FAULT_OFL, fst_zero, "ad2d over/underflow fault");
    if (Ovr)
    {
        SET_I_OFLOW;
        if (tstOVFfault ())
          doFault(FAULT_OFL, fst_zero, "ad2d overflow fault");
    }
}

#if 0
static const char *CS[] = {"CSFL", "CSLS", "CSTS", "CSNS"};
static const char *CTN[] = {"CTN9", "CTN4"};

static int calcSF(int sf1, int sf2, int sf3)
{
    //If the result is given by a fixed-point, operations are performed by justifying the scaling factors (SF1, SF2, and SF3) of the operands 1, 2, and 3:
    //If SF1 > SF2
    //  SF1 > SF2 >= SF3 —> Justify to SF2
    //  SF3 > SF1 > SF2 —> Justify to SF1
    //  SF1 >= SF3 > SF1 —>Justify to SF3-1
    //If SF2 > SF1
    //  SF2 > SF1 >= SF3 —> Justify to SF1
    //  SF3 > SF2 > SF1 —> Justify to SF2
    //  SF2 >= SF3 > SF1 —> Justify to SF3-1
    //
    if (sf1 > sf2)
    {
        if (sf1 > sf2 && sf2 >= sf3)
            return sf2;
        
        if (sf3 > sf1 && sf1 > sf2)
            return sf1;
        
        if (sf1 >= sf3 && sf3 > sf1)
            return sf3-1;
    }
    if (sf2 > sf1)
    {
        if (sf2 > sf1 && sf1 >= sf3)
            return sf1;
        if (sf3 > sf2 && sf2 > sf1)
            return sf2;
        if (sf2 >= sf3 && sf3 > sf1)
            return sf3-1;
    }
    return sf3;
}
#endif

/*
 * ad3d - Add Using Three Decimal Operands
 */

void ad3d (void)
{
    EISstruct * e = & cpu.currentEISinstruction;

    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor(1, &mod_fault);
    setupOperandDescriptor(2, &mod_fault);
    setupOperandDescriptor(3, &mod_fault);
#endif
    
    parseNumericOperandDescriptor(1, &mod_fault);
    parseNumericOperandDescriptor(2, &mod_fault);
    parseNumericOperandDescriptor(3, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bit 1 MBZ
    if (IWB_IRODD & 0200000000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "ad3d(): 1 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    // initialize mop flags. Probably best done elsewhere.
    e->P = getbits36_1 (cpu.cu.IWB, 0) != 0;  // 4-bit data sign character control
    bool T = getbits36_1 (cpu.cu.IWB, 9) != 0;  // truncation bit
    bool R = getbits36_1 (cpu.cu.IWB, 10) != 0;  // rounding bit

    PNL (L68_ (if (R)
      DU_CYCLE_FRND;))
    
    uint srcTN = e->TN1;    // type of chars in src
    
    uint dstTN = e->TN3;    // type of chars in dst
    uint dstCN = e->CN3;    // starting at char pos CN
    
    int n1 = 0, n2 = 0, n3 = 0, sc1 = 0, sc2 = 0;
    
    /*
     * Here we need to distinguish between 4 type of numbers.
     *
     * CSFL - Floating-point, leading sign
     * CSLS - Scaled fixed-point, leading sign
     * CSTS - Scaled fixed-point, trailing sign
     * CSNS - Scaled fixed-point, unsigned
     */
    
    // determine precision
    switch(e->S1)
    {
        case CSFL:
            n1 = (int) e->N1 - 1; // need to account for the - sign
            if (srcTN == CTN4)
                n1 -= 2;    // 2 4-bit digits exponent
            else
                n1 -= 1;    // 1 9-bit digit exponent
            sc1 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n1 = (int) e->N1 - 1; // only 1 sign
            sc1 = -e->SF1;
            break;
            
        case CSNS:
            n1 = (int) e->N1;     // no sign
            sc1 = -e->SF1;
            break;  // no sign wysiwyg
    }

    if (n1 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "ad3d adjusted n1<1");

    switch(e->S2)
    {
        case CSFL:
            n2 = (int) e->N2 - 1; // need to account for the sign
            if (e->TN2 == CTN4)
                n2 -= 2;    // 2 4-bit digit exponent
            else
                n2 -= 1;    // 1 9-bit digit exponent
            sc2 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n2 = (int) e->N2 - 1; // 1 sign
            sc2 = -e->SF2;
            break;
            
        case CSNS:
            n2 = (int) e->N2;     // no sign
            sc2 = -e->SF2;
            break;  // no sign wysiwyg
    }

    if (n2 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "ad3d adjusted n2<1");
    
    switch(e->S3)
    {
        case CSFL:
            n3 = (int) e->N3 - 1; // need to account for the sign
            if (dstTN == CTN4)
                n3 -= 2;    // 2 4-bit digit exponent
            else
                n3 -= 1;    // 1 9-bit digit exponent
            break;
            
        case CSLS:
        case CSTS:
            n3 = (int) e->N3 - 1; // 1 sign
            break;
            
        case CSNS:
            n3 = (int) e->N3;     // no sign
            break;  // no sign wysiwyg
    }

    if (n3 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "ad3d adjusted n3<1");
    

    decContext set;
    //decContextDefault(&set, DEC_INIT_BASE);         // initialize
    decContextDefaultDPS8(&set);
    set.traps=0;
    
    decNumber _1, _2, _3;
    
    EISloadInputBufferNumeric (1);   // according to MF1
    
    decNumber *op1 = decBCD9ToNumber(e->inBuffer, n1, sc1, &_1);
    if (e->sign == -1)
        op1->bits |= DECNEG;
    if (e->S1 == CSFL)
        op1->exponent = e->exponent;
    
    EISloadInputBufferNumeric (2);   // according to MF2

    decNumber *op2 = decBCD9ToNumber(e->inBuffer, n2, sc2, &_2);
    if (e->sign == -1)
        op2->bits |= DECNEG;
    if (e->S2 == CSFL)
        op2->exponent = e->exponent;

    decNumber *op3 = decNumberAdd(&_3, op1, op2, &set);

    // RJ78: significant digits in the result may be lost if:
    // The difference between the scaling factors (exponents) of the source
    // operands is large enough to cause the expected length of the intermediate
    // result to exceed 63 digits after decimal point alignment of source operands, followed by addition.
    // ISOLTS 846 07c, 10a, 11b internal register overflow
    // trailing zeros are not counted towards the limit
    // XXX it is not clear which digits are lost, but I suppose it should be the most significant. ISOLTS doesn't check for this
    // XXX the algorithm should be similar/the same as dv3d NQ? It is to some extent already...
    bool iOvr = 0;
    if (op3->digits > 63) {
        uint8_t pr[256];
        // if sf<=0, trailing zeroes can't be shifted out
        // if sf> 0, (some of) trailing zeroes can be shifted out 
        int sf = e->S3==CSFL?op3->exponent:e->SF3;

        int ctz = 0;
        if (sf>0) {	// optimize: we don't care when sf>0
            decNumberGetBCD(op3,pr);
            for (int i=op3->digits-1;i>=0 && pr[i]==0;i--)
                 ctz ++;
        }

        if (op3->digits - min(max(sf,0),ctz) > 63) {

            enum rounding safeR = decContextGetRounding(&set);         // save rounding mode
            int safe = set.digits;
            decNumber tmp;

            // discard MS digits
            decContextSetRounding(&set, DEC_ROUND_DOWN);     // Round towards 0 (truncation).
            set.digits = op3->digits - min(max(sf,0),ctz) - 63;
            decNumberPlus(&tmp, op3, &set);
            set.digits = safe;

            decNumberSubtract(op3, op3, &tmp, &set); 

            //decNumberToString(op3,(char*)pr); sim_printf("discarded: %s\n",pr);

            decContextSetRounding(&set, safeR);
            iOvr = 1;
        }
    }

    bool Ovr = false, EOvr = false, Trunc = false;
    
    uint8_t out [256];
    char *res = formatDecimal(out, &set, op3, n3, (int) e->S3, e->SF3, R, &Ovr, &Trunc);

    Ovr |= iOvr;

    if (decNumberIsZero(op3))
        op3->exponent = 127;
    
    //printf("%s\r\n", res);
    
    // now write to memory in proper format.....

    //word18 dstAddr = e->dstAddr;
    int pos = (int) dstCN;
    
    // 1st, take care of any leading sign .......
    switch(e->S3)
    {
        case CSFL:  // floating-point, leading sign.
        case CSLS:  // fixed-point, leading sign
            switch(dstTN)
            {
            case CTN4:
                if (e->P) //If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus sign character is placed appropriately if the result of the operation is positive.
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  013);  // special +
                else
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  014);  // default +
                break;
            case CTN9:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? '-' : '+');
                break;
            }
            break;
            
        case CSTS:  // nuttin' to do here .....
        case CSNS:
            break;  // no sign wysiwyg
    }
    
    // 2nd, write the digits .....
    for(int i = 0 ; i < n3 ; i++)
        switch(dstTN)
        {
        case CTN4:
            EISwrite49(&e->ADDR3, &pos, (int) dstTN, (word9) (res[i] - '0'));
            break;
        case CTN9:
            EISwrite49(&e->ADDR3, &pos, (int) dstTN, (word9) res[i]);
            break;
        }
    
    // 3rd, take care of any trailing sign or exponent ...
    switch(e->S3)
    {
        case CSTS:  // write trailing sign ....
            switch(dstTN)
            {
            case CTN4:
                if (e->P) //If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus sign character is placed appropriately if the result of the operation is positive.
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  013);  // special +
                else
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  014);  // default +
                break;
            case CTN9:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? '-' : '+');
                break;
            }
            break;
            
        case CSFL:  // floating-point, leading sign.
            // write the exponent
            switch(dstTN)
            {
            case CTN4:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, (op3->exponent >> 4) & 0xf); // upper 4-bits
                EISwrite49(&e->ADDR3, &pos, (int) dstTN,  op3->exponent       & 0xf); // lower 4-bits
                    
                    break;
            case CTN9:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, op3->exponent & 0xff);    // write 8-bit exponent
                break;
            }
            break;
            
        case CSLS:  // fixed point, leading sign - already done
        case CSNS:  // fixed point, unsigned - nuttin' needed to do
            break;
    }
    
    // set flags, etc ...
    if (e->S3 == CSFL)
    {
        if (op3->exponent > 127)
        {
            SET_I_EOFL;
            EOvr = true;
        }
        if (op3->exponent < -128)
        {
            SET_I_EUFL;
            EOvr = true;
        }
    }
    
    SC_I_NEG (decNumberIsNegative(op3) && !decNumberIsZero(op3));  // set negative indicator if op3 < 0
    SC_I_ZERO (decNumberIsZero(op3));     // set zero indicator if op3 == 0
    
    SC_I_TRUNC (!R && Trunc); // If the truncation condition exists without rounding, then ON; otherwise OFF
    
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    cleanupOperandDescriptor (3);

    if (TST_I_TRUNC && T && tstOVFfault ())
      doFault(FAULT_OFL, fst_zero, "ad3d truncation(overflow) fault");
    if (EOvr && tstOVFfault ())
        doFault(FAULT_OFL, fst_zero, "ad3d over/underflow fault");
    if (Ovr)
    {
        SET_I_OFLOW;
        if (tstOVFfault ())
          doFault(FAULT_OFL, fst_zero, "ad3d overflow fault");
    }
}

/*
 * sb2d - Subtract Using Two Decimal Operands
 */

void sb2d (void)
{
    EISstruct * e = & cpu.currentEISinstruction;

    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor(1, &mod_fault);
    setupOperandDescriptor(2, &mod_fault);
    setupOperandDescriptorCache(3);
#endif
    
    parseNumericOperandDescriptor(1, &mod_fault);
    parseNumericOperandDescriptor(2, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bits 1-8 MBZ 
    if (IWB_IRODD & 0377000000000)
      {
        //sim_printf ("sb2d %012"PRIo64"\n", IWB_IRODD);
        doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "sb2d 0-8 MBZ");
      }

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    e->P = getbits36_1 (cpu.cu.IWB, 0) != 0;  // 4-bit data sign character control
    bool T = getbits36_1 (cpu.cu.IWB, 9) != 0;  // truncation bit
    bool R = getbits36_1 (cpu.cu.IWB, 10) != 0;  // rounding bit

    PNL (L68_ (if (R)
      DU_CYCLE_FRND;))

    
    uint srcTN = e->TN1;    // type of chars in src
    
    uint dstTN = e->TN2;    // type of chars in dst
    uint dstCN = e->CN2;    // starting at char pos CN
    
    e->ADDR3 = e->ADDR2;
    
    int n1 = 0, n2 = 0, sc1 = 0, sc2 = 0;
    
    /*
     * Here we need to distinguish between 4 type of numbers.
     *
     * CSFL - Floating-point, leading sign
     * CSLS - Scaled fixed-point, leading sign
     * CSTS - Scaled fixed-point, trailing sign
     * CSNS - Scaled fixed-point, unsigned
     */
    
    // determine precision
    switch(e->S1)
    {
        case CSFL:
            n1 = (int) e->N1 - 1; // need to account for the - sign
            if (srcTN == CTN4)
                n1 -= 2;    // 2 4-bit digits exponent
            else
                n1 -= 1;    // 1 9-bit digit exponent
            sc1 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n1 = (int) e->N1 - 1; // only 1 sign
            sc1 = -e->SF1;
            break;
            
        case CSNS:
            n1 = (int) e->N1;     // no sign
            sc1 = -e->SF1;
            break;  // no sign wysiwyg
    }

    if (n1 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "sb2d adjusted n1<1");

    switch(e->S2)
    {
        case CSFL:
            n2 = (int) e->N2 - 1; // need to account for the sign
            if (e->TN2 == CTN4)
                n2 -= 2;    // 2 4-bit digit exponent
            else
                n2 -= 1;    // 1 9-bit digit exponent
            sc2 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n2 = (int) e->N2 - 1; // 1 sign
            sc2 = -e->SF2;
            break;
            
        case CSNS:
            n2 = (int) e->N2;     // no sign
            sc2 = -e->SF2;
            break;  // no sign wysiwyg
    }

    if (n2 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "sb2d adjusted n2<1");

    
    decContext set;
    //decContextDefault(&set, DEC_INIT_BASE);         // initialize
    decContextDefaultDPS8(&set);
    set.traps=0;
    
    decNumber _1, _2, _3;
    
    EISloadInputBufferNumeric (1);   // according to MF1
    
    decNumber *op1 = decBCD9ToNumber(e->inBuffer, n1, sc1, &_1);
    if (e->sign == -1)
        op1->bits |= DECNEG;
    if (e->S1 == CSFL)
        op1->exponent = e->exponent;
    
    EISloadInputBufferNumeric (2);   // according to MF2

    decNumber *op2 = decBCD9ToNumber(e->inBuffer, n2, sc2, &_2);
    if (e->sign == -1)
        op2->bits |= DECNEG;
    if (e->S2 == CSFL)
        op2->exponent = e->exponent;
    
    decNumber *op3 = decNumberSubtract(&_3, op2, op1, &set);

    // ISOLTS 846 07c, 10a, 11b internal register overflow - see ad3d
    bool iOvr = 0;
    if (op3->digits > 63) {
        uint8_t pr[256];
        // if sf<=0, trailing zeroes can't be shifted out
        // if sf> 0, (some of) trailing zeroes can be shifted out 
        int sf = e->S3==CSFL?op3->exponent:e->SF3;

        int ctz = 0;
        if (sf>0) {	// optimize: we don't care when sf>0
            decNumberGetBCD(op3,pr);
            for (int i=op3->digits-1;i>=0 && pr[i]==0;i--)
                 ctz ++;
        }

        if (op3->digits - min(max(sf,0),ctz) > 63) {

            enum rounding safeR = decContextGetRounding(&set);         // save rounding mode
            int safe = set.digits;
            decNumber tmp;

            // discard MS digits
            decContextSetRounding(&set, DEC_ROUND_DOWN);     // Round towards 0 (truncation).
            set.digits = op3->digits - min(max(sf,0),ctz) - 63;
            decNumberPlus(&tmp, op3, &set);
            set.digits = safe;

            decNumberSubtract(op3, op3, &tmp, &set); 

            //decNumberToString(op3,(char*)pr); sim_printf("discarded: %s\n",pr);

            decContextSetRounding(&set, safeR);
            iOvr = 1;
        }
    }

    bool Ovr = false, EOvr = false, Trunc = false;
    
    uint8_t out [256];
    char *res = formatDecimal(out, &set, op3, n2, (int) e->S2, e->SF2, R, &Ovr, &Trunc);

    Ovr |= iOvr;
    
    if (decNumberIsZero(op3))
        op3->exponent = 127;
    
    //printf("%s\r\n", res);
    
    // now write to memory in proper format.....
    
    //word18 dstAddr = e->dstAddr;
    int pos = (int) dstCN;
    
    // 1st, take care of any leading sign .......
    switch(e->S2)
    {
        case CSFL:  // floating-point, leading sign.
        case CSLS:  // fixed-point, leading sign
            switch(dstTN)
            {
                case CTN4:
                    if (e->P) //If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus sign character is placed appropriately if the result of the operation is positive.
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  013);  // special +
                    else
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  014);  // default +
                    break;
                case CTN9:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? '-' : '+');
                    break;
            }
            break;
            
        case CSTS:  // nuttin' to do here .....
        case CSNS:
            break;  // no sign wysiwyg
    }
    
    // 2nd, write the digits .....
    for(int i = 0 ; i < n2 ; i++)
        switch(dstTN)
        {
            case CTN4:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, (word9) (res[i] - '0'));
                break;
            case CTN9:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, (word9) res[i]);
                break;
        }
    
    // 3rd, take care of any trailing sign or exponent ...
    switch(e->S2)
    {
        case CSTS:  // write trailing sign ....
            switch(dstTN)
            {
                case CTN4:
                    if (e->P) //If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus sign character is placed appropriately if the result of the operation is positive.
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  013);  // special +
                    else
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  014);  // default +
                    break;
                case CTN9:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? '-' : '+');
                    break;
            }
            break;
            
        case CSFL:  // floating-point, leading sign.
            // write the exponent
            switch(dstTN)
            {
                case CTN4:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (op3->exponent >> 4) & 0xf); // upper 4-bits
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN,  op3->exponent       & 0xf); // lower 4-bits

                    break;
                case CTN9:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, op3->exponent & 0xff);    // write 8-bit exponent
                    break;
            }
            break;
            
        case CSLS:  // fixed point, leading sign - already done
        case CSNS:  // fixed point, unsigned - nuttin' needed to do
            break;
    }
    
    // set flags, etc ...
    if (e->S2 == CSFL)
    {
        if (op3->exponent > 127)
        {
            SET_I_EOFL;
            EOvr = true;
        }
        if (op3->exponent < -128)
        {
            SET_I_EUFL;
            EOvr = true;
        }
    }
    
    SC_I_NEG (decNumberIsNegative(op3) && !decNumberIsZero(op3));  // set negative indicator if op3 < 0
    SC_I_ZERO (decNumberIsZero(op3));     // set zero indicator if op3 == 0
    
    SC_I_TRUNC (!R && Trunc); // If the truncation condition exists without rounding, then ON; otherwise OFF
    
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    cleanupOperandDescriptor (3);

    if (TST_I_TRUNC && T && tstOVFfault ())
      doFault(FAULT_OFL, fst_zero, "sb2d truncation(overflow) fault");
    if (EOvr && tstOVFfault ())
        doFault(FAULT_OFL, fst_zero, "sb2d over/underflow fault");
    if (Ovr)
    {
        SET_I_OFLOW;
        if (tstOVFfault ())
          doFault(FAULT_OFL, fst_zero, "sb2d overflow fault");
    }
}

/*
 * sb3d - Subtract Using Three Decimal Operands
 */

void sb3d (void)
{
    EISstruct * e = & cpu.currentEISinstruction;

    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor(1, &mod_fault);
    setupOperandDescriptor(2, &mod_fault);
    setupOperandDescriptor(3, &mod_fault);
#endif
    
    parseNumericOperandDescriptor(1, &mod_fault);
    parseNumericOperandDescriptor(2, &mod_fault);
    parseNumericOperandDescriptor(3, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bit 1 MBZ
    if (IWB_IRODD & 0200000000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "sb3d(): 1 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    e->P = getbits36_1 (cpu.cu.IWB, 0) != 0;  // 4-bit data sign character control
    bool T = getbits36_1 (cpu.cu.IWB, 9) != 0;  // truncation bit
    bool R = getbits36_1 (cpu.cu.IWB, 10) != 0;  // rounding bit

    PNL (L68_ (if (R)
      DU_CYCLE_FRND;))
    
    uint srcTN = e->TN1;    // type of chars in src
    
    uint dstTN = e->TN3;    // type of chars in dst
    uint dstCN = e->CN3;    // starting at char pos CN
    
    int n1 = 0, n2 = 0, n3 = 0, sc1 = 0, sc2 = 0;
    
    /*
     * Here we need to distinguish between 4 type of numbers.
     *
     * CSFL - Floating-point, leading sign
     * CSLS - Scaled fixed-point, leading sign
     * CSTS - Scaled fixed-point, trailing sign
     * CSNS - Scaled fixed-point, unsigned
     */
    
    // determine precision
    switch(e->S1)
    {
        case CSFL:
            n1 = (int) e->N1 - 1; // need to account for the - sign
            if (srcTN == CTN4)
                n1 -= 2;    // 2 4-bit digits exponent
            else
                n1 -= 1;    // 1 9-bit digit exponent
            sc1 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n1 = (int) e->N1 - 1; // only 1 sign
            sc1 = -e->SF1;
            break;
            
        case CSNS:
            n1 = (int) e->N1;     // no sign
            sc1 = -e->SF1;
            break;  // no sign wysiwyg
    }

    if (n1 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "sb3d adjusted n1<1");

    switch(e->S2)
    {
        case CSFL:
            n2 = (int) e->N2 - 1; // need to account for the sign
            if (e->TN2 == CTN4)
                n2 -= 2;    // 2 4-bit digit exponent
            else
                n2 -= 1;    // 1 9-bit digit exponent
            sc2 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n2 = (int) e->N2 - 1; // 1 sign
            sc2 = -e->SF2;
            break;
            
        case CSNS:
            n2 = (int) e->N2;     // no sign
            sc2 = -e->SF2;
            break;  // no sign wysiwyg
    }

    if (n2 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "sb3d adjusted n2<1");
    
    switch(e->S3)
    {
        case CSFL:
            n3 = (int) e->N3 - 1; // need to account for the sign
            if (dstTN == CTN4)
                n3 -= 2;    // 2 4-bit digit exponent
            else
                n3 -= 1;    // 1 9-bit digit exponent
            break;
            
        case CSLS:
        case CSTS:
            n3 = (int) e->N3 - 1; // 1 sign
            break;
            
        case CSNS:
            n3 = (int) e->N3;     // no sign
            break;  // no sign wysiwyg
    }

    if (n3 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "sb3d adjusted n3<1");
    

    decContext set;
    //decContextDefault(&set, DEC_INIT_BASE);         // initialize
    decContextDefaultDPS8(&set);
    
    set.traps=0;
    
    decNumber _1, _2, _3;
    
    EISloadInputBufferNumeric (1);   // according to MF1
    
    decNumber *op1 = decBCD9ToNumber(e->inBuffer, n1, sc1, &_1);
    if (e->sign == -1)
        op1->bits |= DECNEG;
    if (e->S1 == CSFL)
        op1->exponent = e->exponent;
    
    EISloadInputBufferNumeric (2);   // according to MF2

    decNumber *op2 = decBCD9ToNumber(e->inBuffer, n2, sc2, &_2);
    if (e->sign == -1)
        op2->bits |= DECNEG;
    if (e->S2 == CSFL)
        op2->exponent = e->exponent;

    decNumber *op3 = decNumberSubtract(&_3, op2, op1, &set);

    // ISOLTS 846 07c, 10a, 11b internal register overflow - see ad3d
    bool iOvr = 0;
    if (op3->digits > 63) {
        uint8_t pr[256];
        // if sf<=0, trailing zeroes can't be shifted out
        // if sf> 0, (some of) trailing zeroes can be shifted out 
        int sf = e->S3==CSFL?op3->exponent:e->SF3;

        int ctz = 0;
        if (sf>0) {	// optimize: we don't care when sf>0
            decNumberGetBCD(op3,pr);
            for (int i=op3->digits-1;i>=0 && pr[i]==0;i--)
                 ctz ++;
        }

        if (op3->digits - min(max(sf,0),ctz) > 63) {

            enum rounding safeR = decContextGetRounding(&set);         // save rounding mode
            int safe = set.digits;
            decNumber tmp;

            // discard MS digits
            decContextSetRounding(&set, DEC_ROUND_DOWN);     // Round towards 0 (truncation).
            set.digits = op3->digits - min(max(sf,0),ctz) - 63;
            decNumberPlus(&tmp, op3, &set);
            set.digits = safe;

            decNumberSubtract(op3, op3, &tmp, &set); 

            //decNumberToString(op3,(char*)pr); sim_printf("discarded: %s\n",pr);

            decContextSetRounding(&set, safeR);
            iOvr = 1;
        }
    }
    
    bool Ovr = false, EOvr = false, Trunc = false;

    uint8_t out [256];
    char *res = formatDecimal(out, &set, op3, n3, (int) e->S3, e->SF3, R, &Ovr, &Trunc);

    Ovr |= iOvr;
    
    if (decNumberIsZero(op3))
        op3->exponent = 127;
    
    // now write to memory in proper format.....
    
    //word18 dstAddr = e->dstAddr;
    int pos = (int) dstCN;
    
    // 1st, take care of any leading sign .......
    switch(e->S3)
    {
        case CSFL:  // floating-point, leading sign.
        case CSLS:  // fixed-point, leading sign
            switch(dstTN)
        {
            case CTN4:
                if (e->P) //If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus sign character is placed appropriately if the result of the operation is positive.
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  013);  // special +
                else
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  014);  // default +
                break;
            case CTN9:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? '-' : '+');
                break;
        }
            break;
            
        case CSTS:  // nuttin' to do here .....
        case CSNS:
            break;  // no sign wysiwyg
    }
    
    // 2nd, write the digits .....
    for(int i = 0 ; i < n3 ; i++)
        switch(dstTN)
    {
        case CTN4:
            EISwrite49(&e->ADDR3, &pos, (int) dstTN, (word9) (res[i] - '0'));
            break;
        case CTN9:
            EISwrite49(&e->ADDR3, &pos, (int) dstTN, (word9) res[i]);
            break;
    }
    
    // 3rd, take care of any trailing sign or exponent ...
    switch(e->S3)
    {
        case CSTS:  // write trailing sign ....
            switch(dstTN)
            {
                case CTN4:
                    if (e->P) //If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus sign character is placed appropriately if the result of the operation is positive.
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  013);  // special +
                    else
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  014);  // default +
                    break;
                case CTN9:
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? '-' : '+');
                    break;
            }
            break;
            
        case CSFL:  // floating-point, leading sign.
            // write the exponent
            switch(dstTN)
            {
                case CTN4:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (op3->exponent >> 4) & 0xf); // upper 4-bits
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN,  op3->exponent       & 0xf); // lower 4-bits

                    break;
                case CTN9:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, op3->exponent & 0xff);    // write 8-bit exponent
                    break;
            }
            break;
            
        case CSLS:  // fixed point, leading sign - already done
        case CSNS:  // fixed point, unsigned - nuttin' needed to do
            break;
    }
    
    // set flags, etc ...
    if (e->S3 == CSFL)
    {
        if (op3->exponent > 127)
        {
            SET_I_EOFL;
            EOvr = true;
        }
        if (op3->exponent < -128)
        {
            SET_I_EUFL;
            EOvr = true;
        }
    }
    
    SC_I_NEG (decNumberIsNegative(op3) && !decNumberIsZero(op3));  // set negative indicator if op3 < 0
    SC_I_ZERO (decNumberIsZero(op3));     // set zero indicator if op3 == 0
    
    SC_I_TRUNC (!R && Trunc); // If the truncation condition exists without rounding, then ON; otherwise OFF
    
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    cleanupOperandDescriptor (3);

    if (TST_I_TRUNC && T && tstOVFfault ())
      doFault(FAULT_OFL, fst_zero, "sb3d truncation(overflow) fault");
    if (EOvr && tstOVFfault ())
        doFault(FAULT_OFL, fst_zero, "sb3d over/underflow fault");
    if (Ovr)
    {
        SET_I_OFLOW;
        if (tstOVFfault ())
          doFault(FAULT_OFL, fst_zero, "sb3d overflow fault");
    }
}

/*
 * mp2d - Multiply Using Two Decimal Operands
 */

void mp2d (void)
{
    EISstruct * e = & cpu.currentEISinstruction;

    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor(1, &mod_fault);
    setupOperandDescriptor(2, &mod_fault);
    setupOperandDescriptorCache(3);
#endif
    
    parseNumericOperandDescriptor(1, &mod_fault);
    parseNumericOperandDescriptor(2, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bits 1-8 MBZ
    if (IWB_IRODD & 0377000000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "mp2d 1-8 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    e->P = getbits36_1 (cpu.cu.IWB, 0) != 0;  // 4-bit data sign character control
    bool T = getbits36_1 (cpu.cu.IWB, 9) != 0;  // truncation bit
    bool R = getbits36_1 (cpu.cu.IWB, 10) != 0;  // rounding bit

    PNL (L68_ (if (R)
      DU_CYCLE_FRND;))
    
    uint srcTN = e->TN1;    // type of chars in src
    
    uint dstTN = e->TN2;    // type of chars in dst
    uint dstCN = e->CN2;    // starting at char pos CN
    
    e->ADDR3 = e->ADDR2;
    
    int n1 = 0, n2 = 0, sc1 = 0, sc2 = 0;
    
    /*
     * Here we need to distinguish between 4 type of numbers.
     *
     * CSFL - Floating-point, leading sign
     * CSLS - Scaled fixed-point, leading sign
     * CSTS - Scaled fixed-point, trailing sign
     * CSNS - Scaled fixed-point, unsigned
     */
    
    // determine precision
    switch(e->S1)
    {
        case CSFL:
            n1 = (int) e->N1 - 1; // need to account for the - sign
            if (srcTN == CTN4)
                n1 -= 2;    // 2 4-bit digits exponent
            else
                n1 -= 1;    // 1 9-bit digit exponent
            sc1 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n1 = (int) e->N1 - 1; // only 1 sign
            sc1 = -e->SF1;
            break;
            
        case CSNS:
            n1 = (int) e->N1;     // no sign
            sc1 = -e->SF1;
            break;  // no sign wysiwyg
    }

    if (n1 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "mp2d adjusted n1<1");

    switch(e->S2)
    {
        case CSFL:
            n2 = (int) e->N2 - 1; // need to account for the sign
            if (e->TN2 == CTN4)
                n2 -= 2;    // 2 4-bit digit exponent
            else
                n2 -= 1;    // 1 9-bit digit exponent
            sc2 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n2 = (int) e->N2 - 1; // 1 sign
            sc2 = -e->SF2;
            break;
            
        case CSNS:
            n2 = (int) e->N2;     // no sign
            sc2 = -e->SF2;
            break;  // no sign wysiwyg
    }

    if (n2 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "mp2d adjusted n2<1");
    

    decContext set;
    decContextDefaultDPS8Mul(&set); // 126 digits for multiply
    
    set.traps=0;
    
    decNumber _1, _2, _3;
    
    EISloadInputBufferNumeric (1);   // according to MF1
    
    decNumber *op1 = decBCD9ToNumber(e->inBuffer, n1, sc1, &_1);
    if (e->sign == -1)
        op1->bits |= DECNEG;
    if (e->S1 == CSFL)
        op1->exponent = e->exponent;
    
    EISloadInputBufferNumeric (2);   // according to MF2

    decNumber *op2 = decBCD9ToNumber(e->inBuffer, n2, sc2, &_2);
    if (e->sign == -1)
        op2->bits |= DECNEG;
    if (e->S2 == CSFL)
        op2->exponent = e->exponent;
    
    decNumber *op3 = decNumberMultiply(&_3, op1, op2, &set);
    
    bool Ovr = false, EOvr = false, Trunc = false;
    
    uint8_t out [256];
    char *res = formatDecimal(out, &set, op3, n2, (int) e->S2, e->SF2, R, &Ovr, &Trunc);
    
    if (decNumberIsZero(op3))
        op3->exponent = 127;
    
    // now write to memory in proper format.....
    
    //word18 dstAddr = e->dstAddr;
    int pos = (int) dstCN;
    
    // 1st, take care of any leading sign .......
    switch(e->S2)
    {
        case CSFL:  // floating-point, leading sign.
        case CSLS:  // fixed-point, leading sign
            switch(dstTN)
        {
            case CTN4:
                if (e->P) //If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus sign character is placed appropriately if the result of the operation is positive.
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  013);  // special +
                else
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  014);  // default +
                break;
            case CTN9:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? '-' : '+');
                break;
        }
            break;
            
        case CSTS:  // nuttin' to do here .....
        case CSNS:
            break;  // no sign wysiwyg
    }
    
    // 2nd, write the digits .....
    for(int i = 0 ; i < n2 ; i++)
        switch(dstTN)
    {
        case CTN4:
            EISwrite49(&e->ADDR3, &pos, (int) dstTN, (word9) (res[i] - '0'));
            break;
        case CTN9:
            EISwrite49(&e->ADDR3, &pos, (int) dstTN, (word9) res[i]);
            break;
    }
    
    // 3rd, take care of any trailing sign or exponent ...
    switch(e->S2)
    {
        case CSTS:  // write trailing sign ....
            switch(dstTN)
        {
            case CTN4:
                if (e->P) //If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus sign character is placed appropriately if the result of the operation is positive.
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  013);  // special +
                else
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  014);  // default +
                break;
            case CTN9:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? '-' : '+');
                break;
        }
            break;
            
        case CSFL:  // floating-point, leading sign.
            // write the exponent
            switch(dstTN)
            {
                case CTN4:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (op3->exponent >> 4) & 0xf); // upper 4-bits
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN,  op3->exponent       & 0xf); // lower 4-bits
                    
                    break;
                case CTN9:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, op3->exponent & 0xff);    // write 8-bit exponent
                    break;
            }
            break;
            
        case CSLS:  // fixed point, leading sign - already done
        case CSNS:  // fixed point, unsigned - nuttin' needed to do
            break;
    }
    
    // set flags, etc ...
    if (e->S2 == CSFL)
    {
        if (op3->exponent > 127)
        {
            SET_I_EOFL;
            EOvr = true;
        }
        if (op3->exponent < -128)
        {
            SET_I_EUFL;
            EOvr = true;
        }
    }
    
    SC_I_NEG (decNumberIsNegative(op3) && !decNumberIsZero(op3));  // set negative indicator if op3 < 0
    SC_I_ZERO (decNumberIsZero(op3));     // set zero indicator if op3 == 0
    
    SC_I_TRUNC (!R && Trunc); // If the truncation condition exists without rounding, then ON; otherwise OFF
    
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    cleanupOperandDescriptor (3);

    if (TST_I_TRUNC && T && tstOVFfault ())
      doFault(FAULT_OFL, fst_zero, "mp2d truncation(overflow) fault");
    if (EOvr && tstOVFfault ())
        doFault(FAULT_OFL, fst_zero, "mp2d over/underflow fault");
    if (Ovr)
    {
        SET_I_OFLOW;
        if (tstOVFfault ())
          doFault(FAULT_OFL, fst_zero, "mp2d overflow fault");
    }
}

/*
 * mp3d - Multiply Using Three Decimal Operands
 */

void mp3d (void)
{
    EISstruct * e = & cpu.currentEISinstruction;

    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor(1, &mod_fault);
    setupOperandDescriptor(2, &mod_fault);
    setupOperandDescriptor(3, &mod_fault);
#endif
    
    parseNumericOperandDescriptor(1, &mod_fault);
    parseNumericOperandDescriptor(2, &mod_fault);
    parseNumericOperandDescriptor(3, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bit 1 MBZ
    if (IWB_IRODD & 0200000000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "mp3d(): 1 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    e->P = getbits36_1 (cpu.cu.IWB, 0) != 0;  // 4-bit data sign character control
    bool T = getbits36_1 (cpu.cu.IWB, 9) != 0;  // truncation bit
    bool R = getbits36_1 (cpu.cu.IWB, 10) != 0;  // rounding bit

    PNL (L68_ (if (R)
      DU_CYCLE_FRND;))
    
    uint srcTN = e->TN1;    // type of chars in src
    
    uint dstTN = e->TN3;    // type of chars in dst
    uint dstCN = e->CN3;    // starting at char pos CN
    
    int n1 = 0, n2 = 0, n3 = 0, sc1 = 0, sc2 = 0;
    
    /*
     * Here we need to distinguish between 4 type of numbers.
     *
     * CSFL - Floating-point, leading sign
     * CSLS - Scaled fixed-point, leading sign
     * CSTS - Scaled fixed-point, trailing sign
     * CSNS - Scaled fixed-point, unsigned
     */
    
    // determine precision
    switch(e->S1)
    {
        case CSFL:
            n1 = (int) e->N1 - 1; // need to account for the - sign
            if (srcTN == CTN4)
                n1 -= 2;    // 2 4-bit digits exponent
            else
                n1 -= 1;    // 1 9-bit digit exponent
            sc1 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n1 = (int) e->N1 - 1; // only 1 sign
            sc1 = -e->SF1;
            break;
            
        case CSNS:
            n1 = (int) e->N1;     // no sign
            sc1 = -e->SF1;
            break;  // no sign wysiwyg
    }

    if (n1 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "mp3d adjusted n1<1");

    switch(e->S2)
    {
        case CSFL:
            n2 = (int) e->N2 - 1; // need to account for the sign
            if (e->TN2 == CTN4)
                n2 -= 2;    // 2 4-bit digit exponent
            else
                n2 -= 1;    // 1 9-bit digit exponent
            sc2 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n2 = (int) e->N2 - 1; // 1 sign
            sc2 = -e->SF2;
            break;
            
        case CSNS:
            n2 = (int) e->N2;     // no sign
            sc2 = -e->SF2;
            break;  // no sign wysiwyg
    }

    if (n2 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "mp3d adjusted n2<1");
    
    switch(e->S3)
    {
        case CSFL:
            n3 = (int) e->N3 - 1; // need to account for the sign
            if (dstTN == CTN4)
                n3 -= 2;    // 2 4-bit digit exponent
            else
                n3 -= 1;    // 1 9-bit digit exponent
            break;
            
        case CSLS:
        case CSTS:
            n3 = (int) e->N3 - 1; // 1 sign
            break;
            
        case CSNS:
            n3 = (int) e->N3;     // no sign
            break;  // no sign wysiwyg
    }

    if (n3 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "mp3d adjusted n3<1");


    decContext set;
    //decContextDefault(&set, DEC_INIT_BASE);         // initialize
    decContextDefaultDPS8Mul(&set);	// 126 digits for multiply

    set.traps=0;
    
    decNumber _1, _2, _3;
    
    EISloadInputBufferNumeric (1);   // according to MF1
    
    decNumber *op1 = decBCD9ToNumber(e->inBuffer, n1, sc1, &_1);
    if (e->sign == -1)
        op1->bits |= DECNEG;
    if (e->S1 == CSFL)
        op1->exponent = e->exponent;
    
    EISloadInputBufferNumeric (2);   // according to MF2

    decNumber *op2 = decBCD9ToNumber(e->inBuffer, n2, sc2, &_2);
    if (e->sign == -1)
        op2->bits |= DECNEG;
    if (e->S2 == CSFL)
        op2->exponent = e->exponent;
    
    decNumber *op3 = decNumberMultiply(&_3, op1, op2, &set);
    
//    char c1[1024];
//    char c2[1024];
//    char c3[1024];
//    
//    decNumberToString(op1, c1);
//    sim_printf("c1:%s\n", c1);
//    decNumberToString(op2, c2);
//    sim_printf("c2:%s\n", c2);
//    decNumberToString(op3, c3);
//    sim_printf("c3:%s\n", c3);
    
    bool Ovr = false, EOvr = false, Trunc = false;
    
    uint8_t out [256];
    char *res = formatDecimal(out, &set, op3, n3, (int) e->S3, e->SF3, R, &Ovr, &Trunc);
    
    if (decNumberIsZero(op3))
        op3->exponent = 127;
    
    // now write to memory in proper format.....
    
    //word18 dstAddr = e->dstAddr;
    int pos = (int) dstCN;
    
    // 1st, take care of any leading sign .......
    switch(e->S3)
    {
        case CSFL:  // floating-point, leading sign.
        case CSLS:  // fixed-point, leading sign
            switch(dstTN)
            {
            case CTN4:
	      if (e->P) // If TN2 and S2 specify a 4-bit signed number and P
                          // = 1, then the 13(8) plus sign character is placed
                          // appropriately if the result of the operation is
                          // positive.
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  013);  // special +
                else
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN,  (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  014);  // default +
                break;
            case CTN9:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? '-' : '+');
                break;
            }
            break;
            
        case CSTS:  // nuttin' to do here .....
        case CSNS:
            break;  // no sign wysiwyg
    }
    
    // 2nd, write the digits .....
    for(int i = 0 ; i < n3 ; i++)
        switch(dstTN)
        {
            case CTN4:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, (word9) (res[i] - '0'));
                break;
            case CTN9:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, (word9) res[i]);
                break;
        }
    
        // 3rd, take care of any trailing sign or exponent ...
    switch(e->S3)
    {
        case CSTS:  // write trailing sign ....
            switch(dstTN)
            {
                case CTN4:
                    if (e->P) //If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus sign character is placed appropriately if the result of the operation is positive.
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  013);  // special +
                    else
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  014);  // default +
                    break;
                case CTN9:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? '-' : '+');
                    break;
            }
            break;
            
        case CSFL:  // floating-point, leading sign.
            // write the exponent
            switch(dstTN)
            {
                case CTN4:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (op3->exponent >> 4) & 0xf); // upper 4-bits
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN,  op3->exponent       & 0xf); // lower 4-bits
                    break;
                case CTN9:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, op3->exponent & 0xff);    // write 8-bit exponent
                    break;
            }
            break;
            
        case CSLS:  // fixed point, leading sign - already done
        case CSNS:  // fixed point, unsigned - nuttin' needed to do
            break;
    }
    
    // set flags, etc ...
    if (e->S3 == CSFL)
    {
        if (op3->exponent > 127)
        {
            SET_I_EOFL;
            EOvr = true;
        }
        if (op3->exponent < -128)
        {
            SET_I_EUFL;
            EOvr = true;
        }
    }
    
    SC_I_NEG (decNumberIsNegative(op3) && !decNumberIsZero(op3));  // set negative indicator if op3 < 0
    SC_I_ZERO (decNumberIsZero(op3));     // set zero indicator if op3 == 0
    
    SC_I_TRUNC (!R && Trunc); // If the truncation condition exists without rounding, then ON; otherwise OFF
    
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    cleanupOperandDescriptor (3);

    if (TST_I_TRUNC && T && tstOVFfault ())
      doFault(FAULT_OFL, fst_zero, "mp3d truncation(overflow) fault");
    if (EOvr && tstOVFfault ())
        doFault(FAULT_OFL, fst_zero, "mp3d over/underflow fault");
    if (Ovr)
    {
        SET_I_OFLOW;
        if (tstOVFfault ())
          doFault(FAULT_OFL, fst_zero, "mp3d overflow fault");
    }
}

#if 0
/* ------------------------------------------------------------------ */
/* HWR 2/07 15:49 derived from ......                                 */
/*                                                                    */
/* decPackedFromNumber -- convert decNumber to BCD Packed Decimal     */
/*                                                                    */
/*   bcd    is the BCD bytes                                          */
/*   length is the length of the BCD array                            */
/*   scale  is the scale result                                       */
/*   dn     is the decNumber                                          */
/*   returns bcd, or NULL if error                                    */
/*                                                                    */
/* The number is converted to a BCD decimal byte array,               */
/* right aligned in the bcd array, whose length is indicated by the   */
/* second parameter.                                                  */
/* scale is set to the scale of the number (this is the exponent,     */
/* negated).  To force the number to a specified scale, first use the */
/* decNumberRescale routine, which will round and change the exponent */
/* as necessary.                                                      */
/*                                                                    */
/* If there is an error (that is, the decNumber has too many digits   */
/* to fit in length bytes, or it is a NaN or Infinity), NULL is       */
/* returned and the bcd and scale results are unchanged.  Otherwise   */
/* bcd is returned.                                                   */
/* ------------------------------------------------------------------ */
static uint8_t * decBCDFromNumber(uint8_t *bcd, int length, int *scale, const decNumber *dn) {
    
    //PRINTDEC("decBCDFromNumber()", dn);
    
    const Unit *up=dn->lsu;     // Unit array pointer
    uByte obyte=0, *out;          // current output byte, and where it goes
    Int indigs=dn->digits;      // digits processed
    uInt cut=DECDPUN;           // downcounter per Unit
    uInt u=*up;                 // work
    uInt nib;                   // ..
#if DECDPUN<=4
    uInt temp;                  // ..
#endif
    
    if (dn->digits>length                  // too long ..
        ||(dn->bits & DECSPECIAL)) return NULL;   // .. or special -- hopeless
    
    //if (dn->bits&DECNEG) obyte=DECPMINUS;      // set the sign ..
    //else                obyte=DECPPLUS;
    *scale=-dn->exponent;                      // .. and scale
    
    // loop from lowest (rightmost) byte
    out=bcd+length-1;                          // -> final byte
    for (; out>=bcd; out--) {
        if (indigs>0) {
            if (cut==0) {
                up++;
                u=*up;
                cut=DECDPUN;
            }
#if DECDPUN<=4
            temp=(u*6554)>>16;         // fast /10
            nib=u-X10(temp);
            u=temp;
#else
            nib=u%10;                  // cannot use *6554 trick :-(
            u=u/10;
#endif
            //obyte|=(nib<<4);
            obyte=nib & 255U;
            indigs--;
            cut--;
        }
        *out=obyte;
        obyte=0;                       // assume 0
        //        if (indigs>0) {
        //            if (cut==0) {
        //                up++;
        //                u=*up;
        //                cut=DECDPUN;
        //            }
        //#if DECDPUN<=4
        //            temp=(u*6554)>>16;         // as above
        //            obyte=(uByte)(u-X10(temp));
        //            u=temp;
        //#else
        //            obyte=(uByte)(u%10);
        //            u=u/10;
        //#endif
        //            indigs--;
        //            cut--;
        //        }
    } // loop
    
    return bcd;
} // decBCDFromNumber


static unsigned char *getBCD(decNumber *a)
{
    static uint8_t bcd[256];
    memset(bcd, 0, sizeof(bcd));
    int scale;
    
    decBCDFromNumber(bcd, a->digits, &scale, a);
    for(int i = 0 ; i < a->digits ; i += 1 )
        bcd[i] += '0';
    
    return (unsigned char *) bcd;
}


static char *getBCDn(decNumber *a, int digits)
{
    static uint8_t bcd[256];
    memset(bcd, 0, sizeof(bcd));
    int scale;
    
    decBCDFromNumber(bcd, digits, &scale, a);
    for(int i = 0 ; i < digits ; i += 1 )
        bcd[i] += '0';
    
    return (char *) bcd;
}


static int findFirstDigit(unsigned char *bcd)
{
    int i = 0;
    while (bcd[i] == '0' && bcd[i])
        i += 1;
    
    return i;
}

/* ------------------------------------------------------------------ */
/* HWR 2/07 15:49 derived from ......                                 */
/*                                                                    */
/* decPackedToNumber -- convert BCD Packed Decimal to a decNumber     */
/*                                                                    */
/*   bcd    is the BCD bytes                                          */
/*   length is the length of the BCD array                            */
/*   scale  is the scale associated with the BCD integer              */
/*   dn     is the decNumber [with space for length*2 digits]         */
/*   returns dn, or NULL if error                                     */
/*                                                                    */
/* The BCD decimal byte array, together with an associated scale,     */
/* is converted to a decNumber.  The BCD array is assumed full        */
/* of digits.                                                         */
/* The scale is used (negated) as the exponent of the decNumber.      */
/* Note that zeros may have a scale.                                  */
/*                                                                    */
/* The decNumber structure is assumed to have sufficient space to     */
/* hold the converted number (that is, up to length-1 digits), so     */
/* no error is possible unless the adjusted exponent is out of range. */
/* In these error cases, NULL is returned and the decNumber will be 0.*/
/* ------------------------------------------------------------------ */

static decNumber * decBCDToNumber(const uByte *bcd, Int length, const Int scale, decNumber *dn)
{
    const uByte *last=bcd+length-1;  // -> last byte
    const uByte *first;              // -> first non-zero byte
    uInt  nib;                       // work nibble
    Unit  *up=dn->lsu;               // output pointer
    Int   digits;                    // digits count
    Int   cut=0;                     // phase of output
    
    decNumberZero(dn);               // default result
    last = &bcd[length-1];
    //nib = *last & 0x0f;                // get the sign
    //if (nib==DECPMINUS || nib==DECPMINUSALT) dn->bits=DECNEG;
    //else if (nib<=9) return NULL;   // not a sign nibble
    
    // skip leading zero bytes [final byte is always non-zero, due to sign]
    for (first=bcd; *first==0 && first <= last;) first++;
    digits= (int32_t) (last-first)+1;              // calculate digits ..
    //if ((*first & 0xf0)==0) digits--;     // adjust for leading zero nibble
    if (digits!=0) dn->digits=digits;     // count of actual digits [if 0,
    // leave as 1]
    
    // check the adjusted exponent; note that scale could be unbounded
    dn->exponent=-scale;                 // set the exponent
    if (scale>=0) {                      // usual case
        if ((dn->digits-scale-1)<-DECNUMMAXE) {      // underflow
            decNumberZero(dn);
            return NULL;}
    }
    else { // -ve scale; +ve exponent
        // need to be careful to avoid wrap, here, also BADINT case
        if ((scale<-DECNUMMAXE)            // overflow even without digits
            || ((dn->digits-scale-1)>DECNUMMAXE)) { // overflow
            decNumberZero(dn);
            return NULL;}
    }
    if (digits==0) return dn;             // result was zero
    
    // copy the digits to the number's units, starting at the lsu
    // [unrolled]
    for (;last >= bcd;) {                            // forever
        nib=(unsigned)(*last & 0x0f);
        // got a digit, in nib
        if (nib>9) {decNumberZero(dn); return NULL;}    // bad digit
        
        if (cut==0) *up=(Unit)nib;
        else *up=(Unit)(*up+nib*DECPOWERS[cut]);
        digits--;
        if (digits==0) break;               // got them all
        cut++;
        if (cut==DECDPUN) {
            up++;
            cut=0;
        }
        last--;                             // ready for next
        //        nib = *last & 0x0f;                // get right nibble
        //        if (nib>9) {decNumberZero(dn); return NULL;}
        //
        //        // got a digit, in nib
        //        if (cut==0) *up=(Unit)nib;
        //        else *up=(Unit)(*up+nib*DECPOWERS[cut]);
        //        digits--;
        //        if (digits==0) break;               // got them all
        //        cut++;
        //        if (cut==DECDPUN) {
        //            up++;
        //            cut=0;
        //        }
    } // forever
    
    return dn;
} // decBCDToNumber


static int decCompareMAG(decNumber *lhs, decNumber *rhs, decContext *set)
{
    decNumber _cmpm, *cmpm;
    cmpm = decNumberCompareTotalMag(&_cmpm, lhs, rhs, set);
    
    if (decNumberIsZero(cmpm))
        return 0;   // lhs == rhs
    
    if (decNumberIsNegative(cmpm))
        return -1;  // lhs < rhs
    
    return 1;       // lhs > rhs
}


/*
 * output formatting for DV?X (divide) instructions ....
 */
static char * formatDecimalDIV (decContext * set, decNumber * r, int tn,
                                int n, int s, int sf, bool R, decNumber * num,
                                decNumber * den, bool * OVR, bool * TRUNC)
  {

    bool bDgtN = false;

// this is the sane way to do it.....
//    bool bDgtN = decCompare(num, den, set) == -1;
//    if (s == CSFL && bDgtN)
//        sim_printf("den > num\n");

    // 1) Floating-point quotient
    // NQ = N2, but if the divisor is greater than the dividend after
    // operand alignment, the leading zero digit produced is counted and the
    // effective precision of the result is reduced by one.
    if (s == CSFL)
      {
        
//            decNumber _4, _5, _6a, _6b, *op4, *op5, *op6a, *op6b;
//            
//            // we want to make exponents the same so as to align the operands.
//            // ... which one has priority? dividend or divisor? >punt<
//            
//            op4 = decNumberReduce(&_4, num, set);
//            op5 = decNumberReduce(&_5, den, set);
//
////            op4 = decNumberCopy(&_4, num);
////            op5 = decNumberCopy(&_5, den);
//            
//            op4->exponent = 0;
//            op5->exponent = 0;
//            //op6a = decNumberQuantize(&_6a, op4, op5, set);
//            //op6b = decNumberQuantize(&_6b, op5, op4,  set);
//            
//            //PRINTALL("align 4 (num/dividend)", op4, set);
//            //PRINTALL("align 5 (den/divisor) ", op5, set);
//            
//            PRINTDEC("align 4 (num/dividend)", op4);
//            PRINTDEC("align 5 (den/divisor) ", op5);
//            //PRINTDEC("align 6a (nd)         ", op6a);
//            //PRINTDEC("align 6b (dn)         ", op6b);
//            
//            decNumber _cmp, *cmp;
//            cmp = decNumberCompareTotal(&_cmp, op4, op5, set);
//            bool bAdj2 = decNumberIsNegative(cmp);  // if denominator is > numerator then remove leading 0
//            //    if (bAdj)
//            sim_printf("bAdj2 == %d\n", decNumberToInt32(cmp, set));
//            
//            
        
        //  The dividend mantissa C(AQ) is shifted right and the dividend exponent
        //  C(E) increased accordingly until
        //  | C(AQ)0,63 | < | C(Y-pair)8,71 |
        //  | numerator | < |  denominator  |
        //  | dividend  | < |    divisor    |
        
        // start by determining the characteristic(s) of dividend / divisor
        
        decNumber _dend, _dvsr, *dividend, *divisor;

        dividend = decNumberCopy(&_dend, num);
        divisor = decNumberCopy(&_dvsr, den);
        
        // set exponents to zero to yield the characteristic
        dividend->exponent = 0;
        divisor->exponent = 0;
//        
//        decNumber _one, *one = decNumberFromInt32(&_one, -1);
//        int c = decCompare(dividend, divisor, set);
//        sim_printf("c0 = %d\n", c);

        
        // we want to do a funky fractional alignment here so we can compare the mantissa's
        
        unsigned char *c1 = getBCD(num);
        int f1 = findFirstDigit(c1);
        dividend = decBCDToNumber(c1+f1, 63, 63, &_dend);
        PRINTDEC("aligned dividend", dividend);
        
        unsigned char *c2 = getBCD(den);
        int f2 = findFirstDigit(c2);
        divisor = decBCDToNumber(c2+f2, 63, 63, &_dvsr);
        PRINTDEC("aligned divisor", divisor);
        
        
//        PRINTALL("BCD 1 num/dividend", dividend, set);
//        PRINTALL("BCD 1 den/divisor ", divisor, set);
//
//            decNumberReduce(dividend, dividend, set);
//            decNumberReduce(divisor, divisor, set);
//
//        PRINTDEC("dividend", dividend);
//        PRINTDEC("divisor ", divisor);
//        PRINTALL("BCD 2 num/dividend", dividend, set);
//        PRINTALL("BCD 2 den/divisor ", divisor, set);

        
        if (decCompareMAG(dividend, divisor, set) == -1)
          {
            bDgtN = true;
          }
            
      } // s == CSFL

    if (s == CSFL)
        sf = 0;
    
    // XXX what happens if we try to write a negative number to an unsigned field?????
    // Detection of a character outside the range [0,11]8 in a digit position
    // or a character outside the range [12,17]8 in a sign position causes an
    // illegal procedure fault.
    
    // adjust output length according to type ....
    //This implies that an unsigned fixed-point receiving field has a minimum
    //length of 1 character; a signed fixed-point field, 2 characters; and a
    //floating-point field, haracters.
    
    int adjLen = n;             // adjLen is the adjusted allowed length of the result taking into account signs and/or exponent
    switch (s)
      {
        case CSFL:              // we have a leading sign and a trailing exponent.
          if (tn == CTN9)
            adjLen -= 2;        // a sign and an 1 9-bit exponent
          else
            adjLen -= 3;        // a sign and 2 4-bit digits making up the exponent
          break;
        case CSLS:
        case CSTS:              // take sign into assount. One less char to play with
          adjLen -= 1;
          break;
        case CSNS:
          break;                // no sign to worry about. Use everything
      }
    
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "\nformatDecimal: adjLen=%d SF=%d S=%s TN=%s\n", adjLen, sf, CS[s], CTN[tn]);
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "formatDecimal: %s  r->digits=%d  r->exponent=%d\n", getBCD(r), r->digits, r->exponent);
    
    PRINTDEC("fd(1:r):", r);
    PRINTALL("pa(1:r):", r, set);
    
    if (adjLen < 1)
      {
        // adjusted length is too small for anything but sign and/or exponent
        //*OVR = 1;
        
        // XXX what do we fill in here? Sign and exp?
        *OVR = true;
        return (char *)"";
      }
    
    // scale result (if not floating)
    
    decNumber _r2;
    decNumberZero(&_r2);
    
    decNumber *r2 = &_r2;
    
    decNumber _sf;  // scaling factor
    {
        
        
#ifndef SPEED
      int scale;
      char out[256], out2[256];
      if_sim_debug (DBG_TRACEEXT, & cpu_dev)
        {
          memset (out, 0, sizeof (out));
          memset (out2, 0, sizeof (out2));
           
          decBCDFromNumber((uint8_t *)out, r->digits, &scale, r);
          for(int i = 0 ; i < r->digits ; i += 1 )
              out[i] += '0';
          sim_printf("formatDecimal(DEBUG): out[]: '%s'\n", out);
        }
#endif
        
      if (s != CSFL)// && sf != 0)
        {
          decNumberFromInt32(&_sf, sf);
          sim_debug (DBG_TRACEEXT, & cpu_dev,
                     "formatDecimal(s != CSFL a): %s r->digits=%d r->exponent=%d\n", getBCD(r), r->digits, r->exponent);
          r2 = decNumberRescale(&_r2, r, &_sf, set);
          sim_debug (DBG_TRACEEXT, & cpu_dev,
                     "formatDecimal(s != CSFL b): %s r2->digits=%d r2->exponent=%d\n", getBCD(r2), r2->digits, r2->exponent);
        }
      else
        //*r2 = *r;
        decNumberCopy(r2, r);
        
      PRINTDEC("fd(2:r2):", r2);

#ifndef SPEED
      if_sim_debug (DBG_TRACEEXT, & cpu_dev)
        {
          decBCDFromNumber((uint8_t *)out2, r2->digits, &scale, r2);
          for(int i = 0 ; i < r2->digits ; i += 1 )
            out2[i] += '0';
            
          sim_debug (DBG_TRACEEXT, & cpu_dev,
                     "formatDecimal: adjLen=%d E=%d SF=%d S=%s TN=%s digits(r2)=%s E2=%d\n", adjLen, r->exponent, sf, CS[s], CTN[tn], out2, r2->exponent);
        }
#endif
    }
    
    
    
    int scale;
    
    uint8_t out[256];
    
    memset (out, 0, sizeof (out));
    
    //bool ovr = (r->digits-sf) > adjLen;     // is integer portion too large to fit?
    bool ovr = r2->digits > adjLen;          // is integer portion too large to fit?
    bool trunc = r->digits > r2->digits;     // did we loose something along the way?
    
    
    // now let's check for overflows
    if (!ovr && !trunc)
      {
        sim_debug (DBG_TRACEEXT, & cpu_dev,
                   "formatDecimal(OK): r->digits(%d) <= adjLen(%d) r2->digits(%d)\n", r->digits, adjLen, r2->digits);
        if (s == CSFL)
          {
            if (r2->digits < adjLen)
              {
                PRINTDEC("Value 1a", r2)
                
                decNumber _s, *sc;
                int rescaleFactor = r2->exponent - (adjLen - r2->digits);
                sc = decNumberFromInt32(&_s, rescaleFactor);
                
                PRINTDEC("Value sc", sc)
                if (rescaleFactor > (adjLen - r2->digits))
                    r2 = decNumberRescale(r2, r2, sc, set);
                
                PRINTDEC("Value 2a", r2)
              }
            else
              {
                PRINTDEC("Value 1b", r2)
              }
            
            // if it's floating justify it ...
            /// <remark>
            /// The dps88 and afterwards would generate a quotient with the maximim number of significant digits.
            /// Not so the dps8. According the manuals "if the divisor is greater than the dividend after operand alignment,
            ///    the leading zero digit produced is counted and the effective precision of the result is reduced by one."
            /// No problem. However, according to eis_tester
            ///             desc 1 -sd l -sf 1 -nn 8;
            ///             desc 2 -sd t -sf 2 -nn 8;
            ///             desc 3 -sd f -nn 8;
            ///
            ///             data 1 "+" (5)"0" "58";
            ///             data 2 "000" "1234" "+";
            ///             data 3 "+" "021275" 376;
            ///            +0001234(00) / +0000058(0) = +021275 e-2
            /// by as yet an unknown algorithm
            /// <remark/>
            // ... if bAdj then we leave a (single?) leading 0
            
            if (!decNumberIsZero(r2))
              {
                char *q = getBCDn(r2, adjLen) ;
                int lz = 0; // leading 0's
                while (*q)
                  {
                    if (*q == '0')
                      {
                        lz += 1;
                        q += 1;
                      }
                    else
                      break;
                  }
                
                if (lz)
                  {
                    decNumber _1;
                    decNumberFromInt32(&_1, lz);
                    decNumberShift(r2, r2, &_1, set);
                    r2->exponent -= lz;
                  }
              }
          } // s == CSFL
        
        
        decBCDFromNumber(out, adjLen, &scale, r2);
        
        for(int i = 0 ; i < adjLen ; i += 1 )
          out[i] += '0';
        
        // add leading 0 and reduce precision if needed
        if (bDgtN)
          {
            for(int i = adjLen - 1 ; i >= 0 ; i -= 1 )
                out[i + 1] = out[i];
            out[adjLen] = 0;
            out[0] = '0';
            r2->exponent += 1;
          }
      } // ! ovr && ! trunc
    else
      {
        PRINTDEC("r2(a):", r2);

        ovr = false;
        trunc = false;
        
        // if we get here then we have either overflow or truncation....
        
        sim_debug (DBG_TRACEEXT, & cpu_dev,
                   "formatDecimal(!OK%s): r2->digits %d adjLen %d\n", R ? " R" : "", r2->digits, adjLen);
        
        // so, what do we do?
        if (R)
          {
            // NB even with rounding you can have an overflow...
            
            // if we're in rounding mode then we just make things fit and everything is OK - except if we have an overflow.
            
            decNumber *ro = r2; //(s == CSFL ? r : r2);
            
            int safe = set->digits;
            
            if (ro->digits > adjLen)    //(adjLen + 1))
              {
                //set->digits = ro->digits + sf + 1;
                sim_debug (DBG_TRACEEXT, & cpu_dev,
                           "formatDecimal(!OK R1): ro->digits %d adjLen %d\n", ro->digits, adjLen);
                
                set->digits = adjLen;
                decNumberPlus(ro, ro, set);
                
                decBCDFromNumber(out, set->digits, &scale, ro);
                for(int i = 0 ; i < set->digits ; i += 1 )
                    out[i] += '0';
                
                // HWR 24 Oct 2013
                char temp[256];
                strcpy(temp, (char *) out+set->digits-adjLen);
                strcpy((char *) out, temp);
                
                //strcpy(out, out+set->digits-adjLen); // this generates a SIGABRT - probably because of overlapping strings.
                
                //sim_debug (DBG_TRACEEXT, & cpu_dev, "R OVR\n");
                //ovr = true; breaks ET MVN 5
              } // digits > adjlen
            else
              {
                sim_debug (DBG_TRACEEXT, & cpu_dev,
                           "formatDecimal(!OK R2): ro->digits %d adjLen %d\n", ro->digits, adjLen);
                
                if (s==CSFL)
                  {
                    
                    set->digits = adjLen;
                    decNumberPlus(ro, ro, set);
                    
                    decBCDFromNumber(out, adjLen, &scale, ro);
                    for(int i = 0 ; i < adjLen ; i += 1 )
                        out[i] += '0';
                    out[adjLen] = 0;
                    sim_debug (DBG_TRACEEXT, & cpu_dev, "formatDecimal(!OK R2a): %s\n", out);
                    
                  } // s == CSFL
                else
                  {
                    int dig = set->digits;
                    set->digits = adjLen;
                    ro = decNumberPlus(ro, ro, set);    // round to adjLen digits
                    decBCDFromNumber((uint8_t *)out, adjLen, &scale, ro);
                    set->digits = dig;
                    
                    for(int j = 0 ; j < adjLen; j += 1 )
                        out[j] += '0';
                    
                    sim_debug (DBG_TRACEEXT, & cpu_dev, "formatDecimal(!OK R2b): %s\n", out);
                  } // s != CSFL
                ovr = false;    // since we've rounded we can have no overflow ?????
              } // digits <= adjlen
            sim_debug (DBG_TRACEEXT, & cpu_dev, "formatDecimal(R3): digits:'%s'\n", out);
            
            set->digits = safe;
            
            // display int of number
            
#ifndef SPEED
            if_sim_debug (DBG_TRACEEXT, & cpu_dev)
              {
                decNumber _i;
                decNumber *i = decNumberToIntegralValue(&_i, ro, set);
                char outi[256];
                memset (outi, 0, sizeof (outi));
                decBCDFromNumber((uint8_t *)outi, adjLen, &scale, i);
                for(int j = 0 ; j < adjLen; j += 1 )
                    outi[j] += '0';
                sim_debug (DBG_TRACEEXT, & cpu_dev, "i=%s\n", outi);
              }
#endif
          } // R
        else
          {
            // if we're not in rounding mode then we can either have a truncation or an overflow
            
            if (s == CSFL)
              {
                enum rounding safeR = decContextGetRounding(set);         // save rounding mode
                decContextSetRounding(set, DEC_ROUND_DOWN);     // Round towards 0 (truncation).

                PRINTDEC("out[1a]:", r2);

                int safe = set->digits;
                set->digits = adjLen;
                decNumberPlus(r2, r2, set);

                PRINTDEC("out[1b]:", r2);

                decBCDFromNumber(out, r2->digits, &scale, r2);
                for(int i = 0 ; i < adjLen ; i += 1 )
                    out[i] += '0';
                out[adjLen] = 0;

                
                // 1) Floating-point quotient
                //  NQ = N3, but if the divisor is greater than the dividend after operand alignment, the leading zero digit produced is counted and the effective precision of the result is reduced by one.
                // -or-
                // With the divisor (den) greater than the dividend (num), the algorithm generates a leading zero in the quotient. 

                if (bDgtN)
                  {
                    for(int i = adjLen - 1 ; i >= 0 ; i -= 1 )
                        out[i + 1] = out[i];
                    out[adjLen] = 0;
                    out[0] = '0';
                    r2->exponent += 1;
                  }

                set->digits = safe;
                decContextSetRounding(set, safeR);              // restore rounding mode
                
                sim_debug (DBG_TRACEEXT, & cpu_dev, "CSFL TRUNC\n");
              } // s == CSFL
            else
              {
                if (r2->digits < r->digits)
                  {
                    enum rounding safeR = decContextGetRounding(set);         // save rounding mode
                    decContextSetRounding(set, DEC_ROUND_DOWN);     // Round towards 0 (truncation).
                    
                    // re-rescale r with an eye towards truncation notrounding
                    
                    r2 = decNumberRescale(r2, r, &_sf, set);
                    
                    trunc = true;

                    if (r2->digits <= adjLen)
                      {
                        decBCDFromNumber(out, adjLen, &scale, r2);
                        for(int i = 0 ; i < adjLen; i += 1 )
                            out[i] += '0';
                        out[adjLen] = 0;
                        trunc = false;
                      }
                    else
                      {
                        decBCDFromNumber(out, r2->digits, &scale, r2);
                        for(int i = 0 ; i < r2->digits; i += 1 )
                            out[i] += '0';
                        out[r2->digits] = 0;
                        
                        memcpy(out, out + strlen((char *) out) - adjLen, (unsigned long) adjLen);
                        out[adjLen] = 0;
                        
                        ovr = true;
                        trunc = false;
                      }
                    decContextSetRounding(set, safeR);              // restore rounding mode
                    sim_debug (DBG_TRACEEXT, & cpu_dev, "TRUNC\n");
                    
                    //                else if ((r2->digits-sf) > adjLen)     // HWR 18 July 2014 was (r->digits > adjLen)
                  } // r2 < r
                else if ((r2->digits) > adjLen)     // HWR 18 July 2014 was (r->digits > adjLen)
                  {
                    // OVR
                    decBCDFromNumber(out, r2->digits, &scale, r2);
                    for(int i = 0 ; i < r2->digits ; i += 1 )
                        out[i] += '0';
                    out[r2->digits] = 0;
                    
                    // HWR 24 Oct 2013
                    char temp[256];
                    strcpy(temp, (char *) out+r2->digits-adjLen);
                    strcpy((char *) out, temp);
                    //strcpy(out, out+r->digits-adjLen); // this generates a SIGABRT - probably because of overlapping strings.
                    
                    sim_debug (DBG_TRACEEXT, & cpu_dev, "OVR\n");
                    ovr = true;
                  } // r2 >= r
                else
                  sim_printf("formatDecimal(?): How'd we get here?\n");
              } // s != CSFL
          } // !R
      } // ovr || trunc
    sim_debug (DBG_TRACEEXT, & cpu_dev,
               "formatDecimal(END): ovrflow=%d trunc=%d R=%d out[]='%s'\n", ovr, trunc, R, out);
    *OVR = ovr;
    *TRUNC = trunc;
    
    decNumberCopy(r, r2);
    return (char *) out;
  }
#endif

/*
 * dv2d - Divide Using Two Decimal Operands
 */

void dv2d (void)
{
    EISstruct * e = & cpu.currentEISinstruction;

    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor(1, &mod_fault);
    setupOperandDescriptor(2, &mod_fault);
    setupOperandDescriptorCache(3);
#endif
    
    parseNumericOperandDescriptor(1, &mod_fault);
    parseNumericOperandDescriptor(2, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bits 1-8 MBZ
    // ISOLTS test 840 and RJ78 says bit 9 (T) MBZ as well
    if (IWB_IRODD & 0377400000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "dv2d 1-9 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    e->P = getbits36_1 (cpu.cu.IWB, 0) != 0;  // 4-bit data sign character control
    //bool T = getbits36_1 (cpu.cu.IWB, 9) != 0;  // truncation bit
    bool R = getbits36_1 (cpu.cu.IWB, 10) != 0;  // rounding bit

    PNL (L68_ (if (R)
      DU_CYCLE_FRND;))
    
    uint srcTN = e->TN1;    // type of chars in src
    
    uint dstTN = e->TN2;    // type of chars in dst
    uint dstCN = e->CN2;    // starting at char pos CN
    
    e->ADDR3 = e->ADDR2;
    
    int n1 = 0, n2 = 0, sc1 = 0, sc2 = 0;
    
    /*
     * Here we need to distinguish between 4 type of numbers.
     *
     * CSFL - Floating-point, leading sign
     * CSLS - Scaled fixed-point, leading sign
     * CSTS - Scaled fixed-point, trailing sign
     * CSNS - Scaled fixed-point, unsigned
     */
    
    // determine precision
    switch(e->S1)
    {
        case CSFL:
            n1 = (int) e->N1 - 1; // need to account for the - sign
            if (srcTN == CTN4)
                n1 -= 2;    // 2 4-bit digits exponent
            else
                n1 -= 1;    // 1 9-bit digit exponent
            sc1 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n1 = (int) e->N1 - 1; // only 1 sign
            sc1 = -e->SF1;
            break;
            
        case CSNS:
            n1 = (int) e->N1;     // no sign
            sc1 = -e->SF1;
            break;  // no sign wysiwyg
    }

    if (n1 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "dv2d adjusted n1<1");
    
    switch(e->S2)
    {
        case CSFL:
            n2 = (int) e->N2 - 1; // need to account for the sign
            if (e->TN2 == CTN4)
                n2 -= 2;    // 2 4-bit digit exponent
            else
                n2 -= 1;    // 1 9-bit digit exponent
            sc2 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n2 = (int) e->N2 - 1; // 1 sign
            sc2 = -e->SF2;
            break;
            
        case CSNS:
            n2 = (int) e->N2;     // no sign
            sc2 = -e->SF2;
            break;  // no sign wysiwyg
    }

    if (n2 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "dv2d adjusted n2<1");


    decContext set;
    decContextDefaultDPS8(&set);

    set.traps=0;
    
    decNumber _1, _2, _3;
    
    EISloadInputBufferNumeric (1);   // according to MF1
    
    decNumber *op1 = decBCD9ToNumber(e->inBuffer, n1, sc1, &_1);    // divisor
    if (e->sign == -1)
        op1->bits |= DECNEG;
    if (e->S1 == CSFL)
        op1->exponent = e->exponent;

    // check for divide by 0!
    if (decNumberIsZero(op1))
    {
        doFault(FAULT_DIV, fst_zero, "dv2d division by 0");
    }

    word9   inBufferop1 [64];
    memcpy (inBufferop1,e->inBuffer,64); // save for clz1 calculation later
    
    EISloadInputBufferNumeric (2);   // according to MF2
    
    decNumber *op2 = decBCD9ToNumber(e->inBuffer, n2, sc2, &_2);    // dividend
    if (e->sign == -1)
        op2->bits |= DECNEG;
    if (e->S2 == CSFL)
        op2->exponent = e->exponent;
    
    int NQ;
    if (e->S2 == CSFL)
    {
        NQ = n2;
    } 
    else 
    {
        // count leading zeroes
        // TODO optimize - can these be somehow extracted from decNumbers?
        int clz1, clz2, i;
        for (i=0; i < op1->digits; i++)
            if (inBufferop1[i]!=0)
                break;
        clz1 = i;
        for (i=0; i < op2->digits; i++)
            if (e->inBuffer[i]!=0) // this still holds op2 digits
                break;
        clz2 = i;
        sim_debug (DBG_TRACEEXT, & cpu_dev, "dv2d: clz1 %d clz2 %d\n",clz1,clz2);

        // XXX are clz also valid for CSFL dividend / divisor? probably yes
        // XXX seems that exponents and scale factors are used interchangeably here ? (RJ78)
        NQ = (n2-clz2+1) - (n1-clz1) + (-(e->S1==CSFL?op1->exponent:(int)e->SF1));

sim_debug (DBG_TRACEEXT, & cpu_dev, "dv2d S1 %d S2 %d N1 %d N2 %d clz1 %d clz2 %d E1 %d E2 %d SF2 %d NQ %d\n",e->S1,e->S2,e->N1,e->N2,clz1,clz2,op1->exponent,op2->exponent,e->SF2,NQ);
    }
    if (NQ > 63)
        doFault(FAULT_DIV, fst_zero, "dv2d NQ>63");
    // Note: NQ is currently unused apart from this FAULT_DIV check. decNumber produces more digits than required, but they are then rounded/truncated

    // Yes, they're switched. op1=divisor
    decNumber *op3 = decNumberDivide(&_3, op2, op1, &set); 
    // Note DPS88 and DPS9000 are different when NQ <= 0
    // This is a flaw in the DPS8/70 hardware which was corrected in later models
    // ISOLTS-817 05b

    PRINTDEC("op2", op2);
    PRINTDEC("op1", op1);
    PRINTDEC("op3", op3);
    
    // let's check division results to see for anomalous conditions
    if (
        (set.status & DEC_Division_undefined) ||    // 0/0 will become NaN
        (set.status & DEC_Invalid_operation) ||
        (set.status & DEC_Division_by_zero)
        ) { sim_debug (DBG_TRACEEXT, & cpu_dev, "oops! dv2d anomalous results"); }	// divide by zero has already been checked before

    if (e->S2 == CSFL)
    {
        // justify CSFL left
        // This is a flaw in the DPS8/70 hardware which was corrected in later models
        // Note DPS88 and DPS9000 are different
        // ISOLTS-817 06c,06e

        decNumber _sf;

        if (n2 - op3->digits > 0)
        {
            decNumberFromInt32(&_sf, op3->exponent - (n2 - op3->digits));
            PRINTDEC("Value 1", op3)
            PRINTDEC("Value sf", &_sf)
            op3 = decNumberRescale(op3, op3, &_sf, &set);
            PRINTDEC("Value 2", op3)
        }
    }
    
    bool Ovr = false, EOvr = false, Trunc = false;
    uint8_t out[256];
    
    // CSFL: If the divisor is greater than the dividend after operand
    // alignment, the leading zero digit produced is counted and the effective
    // precision of the result is reduced by one.
    // This is a flaw in the DPS8/70 hardware which was corrected in later models
    // Note DPS88 and DPS9000 are different
    //
    // "greater after operand alignment" means scale until most-significant digits are nonzero, then compare magnitudes ignoring exponents
    // This passes ISOLTS-817 06e, ET 458,461,483,486
    char *res;
    if (e->S2 == CSFL) {
        decNumber _1a;
        decNumber _2a;
        decNumber _sf;
        if (op1->digits >= op2->digits) {
            // scale op2
            decNumberCopy(&_1a, op1);
            decNumberFromInt32(&_sf, op1->digits - op2->digits);
            decNumberShift(&_2a, op2, &_sf, &set);
        } else if (op1->digits < op2->digits) {
            // scale op1
            decNumberFromInt32(&_sf, op2->digits - op1->digits);
            decNumberShift(&_1a, op1, &_sf, &set);
            decNumberCopy(&_2a, op2);
        }
        _1a.exponent = 0;
        _2a.exponent = 0;

        PRINTDEC("dv2d: op1a", &_1a);
        PRINTDEC("dv2d: op2a", &_2a);
        sim_debug (DBG_TRACEEXT, & cpu_dev, "dv2d: exp1 %d exp2 %d digits op1 %d op2 %d op1a %d op2a %d\n",op1->exponent,op2->exponent,op1->digits,op2->digits,_1a.digits,_2a.digits);

        if (decCompareMAG(&_1a, &_2a, &set) > 0) {
            // shorten the result field to get proper rounding
            res = formatDecimal(out, &set, op3, n2 -1, (int) e->S2, e->SF2, R, &Ovr, &Trunc);

            // prepend zero digit
            // ET 458,483 float=float/float, ET 461,486 float=fixed/fixed
            for (int i = n2; i > 0; i--) // incl.zero terminator
                 res[i] = res[i-1];
            res[0] = '0';
            sim_debug (DBG_TRACEEXT, & cpu_dev, "dv2d: addzero n2 %d %s exp %d\n",n2,res,op3->exponent);
        } else {
            // full n2 digits are retured
            res = formatDecimal(out, &set, op3, n2, (int) e->S2, e->SF2, R, &Ovr, &Trunc);
        }
    } else {
        // same as all other decimal instructions
        res = formatDecimal(out, &set, op3, n2, (int) e->S2, e->SF2, R, &Ovr, &Trunc);
    }
    
    if (decNumberIsZero(op3))
        op3->exponent = 127;
    
    // now write to memory in proper format.....
    
    int pos = (int) dstCN;
    
    // 1st, take care of any leading sign .......
    switch(e->S2)
    {
        case CSFL:  // floating-point, leading sign.
        case CSLS:  // fixed-point, leading sign
            switch(dstTN)
            {
                case CTN4:
                    if (e->P) //If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus sign character is placed appropriately if the result of the operation is positive.
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  013);  // special +
                    else
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  014);  // default +
                    break;
                case CTN9:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? '-' : '+');
                    break;
            }
            break;
            
        case CSTS:  // nuttin' to do here .....
        case CSNS:
            break;  // no sign wysiwyg
    }
    
    // 2nd, write the digits .....
    for(int i = 0 ; i < n2 ; i++)
        switch(dstTN)
        {
            case CTN4:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, (word9) (res[i] - '0'));
                break;
            case CTN9:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, (word9) res[i]);
                break;
        }
    
    // 3rd, take care of any trailing sign or exponent ...
    switch(e->S2)
    {
        case CSTS:  // write trailing sign ....
            switch(dstTN)
            {
                case CTN4:
                    if (e->P) //If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus sign character is placed appropriately if the result of the operation is positive.
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  013);  // special +
                    else
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  014);  // default +
                    break;
                case CTN9:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? '-' : '+');
                    break;
            }
            break;
            
        case CSFL:  // floating-point, leading sign.
            // write the exponent
            switch(dstTN)
            {
                case CTN4:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (op3->exponent >> 4) & 0xf); // upper 4-bits
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN,  op3->exponent       & 0xf); // lower 4-bits
                    break;
                case CTN9:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, op3->exponent & 0xff);    // write 8-bit exponent
                    break;
            }
            break;
            
        case CSLS:  // fixed point, leading sign - already done
        case CSNS:  // fixed point, unsigned - nuttin' needed to do
            break;
    }
    
    // set flags, etc ...
    if (e->S2 == CSFL)
    {
        if (op3->exponent > 127)
        {
            SET_I_EOFL;
            EOvr = true;
        }
        if (op3->exponent < -128)
        {
            SET_I_EUFL;
            EOvr = true;
        }
    }
    
    SC_I_NEG (decNumberIsNegative(op3) && !decNumberIsZero(op3));  // set negative indicator if op3 < 0
    SC_I_ZERO (decNumberIsZero(op3));     // set zero indicator if op3 == 0
    
    //SC_I_TRUNC (!R && Trunc); // no truncation flag for divide
    
    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    cleanupOperandDescriptor (3);

    //if (TST_I_TRUNC && T && tstOVFfault ())
    //  doFault(FAULT_OFL, fst_zero, "dv2d truncation(overflow) fault");
    if (EOvr && tstOVFfault ())
        doFault(FAULT_OFL, fst_zero, "dv2d over/underflow fault");
    if (Ovr)
    {
        SET_I_OFLOW;
        if (tstOVFfault ())
          doFault(FAULT_OFL, fst_zero, "dv2d overflow fault");
    }
}

/*
 * dv3d - Divide Using Three Decimal Operands
 */

void dv3d (void)

{
    EISstruct * e = & cpu.currentEISinstruction;

    fault_ipr_subtype_ mod_fault = 0;
    
#ifndef EIS_SETUP
    setupOperandDescriptor(1, &mod_fault);
    setupOperandDescriptor(2, &mod_fault);
    setupOperandDescriptor(3, &mod_fault);
#endif
    
    parseNumericOperandDescriptor(1, &mod_fault);
    parseNumericOperandDescriptor(2, &mod_fault);
    parseNumericOperandDescriptor(3, &mod_fault);

#ifdef L68
    // L68 raises it immediately
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif
    
    // Bit 1 MBZ
    // ISOLTS test 840 and RJ78 says bit 9 (T) MBZ
    if (IWB_IRODD & 0200400000000)
      doFault (FAULT_IPR, (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP|mod_fault}, "dv3d(): 1,9 MBZ");

#ifdef DPS8M
    // DPS8M raises it delayed
    if (mod_fault)
      {
        doFault (FAULT_IPR,
                 (_fault_subtype) {.fault_ipr_subtype=mod_fault},
                 "Illegal modifier");
      }
#endif

    e->P = getbits36_1 (cpu.cu.IWB, 0) != 0;  // 4-bit data sign character control
    //bool T = getbits36_1 (cpu.cu.IWB, 9) != 0;  // truncation bit
    bool R = getbits36_1 (cpu.cu.IWB, 10) != 0;  // rounding bit

    PNL (L68_ (if (R)
      DU_CYCLE_FRND;))
    
    uint srcTN = e->TN1;    // type of chars in src
    
    uint dstTN = e->TN3;    // type of chars in dst
    uint dstCN = e->CN3;    // starting at char pos CN
    
    int n1 = 0, n2 = 0, n3 = 0, sc1 = 0, sc2 = 0;
    
    /*
     * Here we need to distinguish between 4 type of numbers.
     *
     * CSFL - Floating-point, leading sign
     * CSLS - Scaled fixed-point, leading sign
     * CSTS - Scaled fixed-point, trailing sign
     * CSNS - Scaled fixed-point, unsigned
     */
    
    // determine precision
    switch(e->S1)
    {
        case CSFL:
            n1 = (int) e->N1 - 1; // need to account for the - sign
            if (srcTN == CTN4)
                n1 -= 2;    // 2 4-bit digits exponent
            else
                n1 -= 1;    // 1 9-bit digit exponent
            sc1 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n1 = (int) e->N1 - 1; // only 1 sign
            sc1 = -e->SF1;
            break;
            
        case CSNS:
            n1 = (int) e->N1;     // no sign
            sc1 = -e->SF1;
            break;  // no sign wysiwyg
    }

    if (n1 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "dv3d adjusted n1<1");

    switch(e->S2)
    {
        case CSFL:
            n2 = (int) e->N2 - 1; // need to account for the sign
            if (e->TN2 == CTN4)
                n2 -= 2;    // 2 4-bit digit exponent
            else
                n2 -= 1;    // 1 9-bit digit exponent
            sc2 = 0;        // no scaling factor
            break;
            
        case CSLS:
        case CSTS:
            n2 = (int) e->N2 - 1; // 1 sign
            sc2 = -e->SF2;
            break;
            
        case CSNS:
            n2 = (int) e->N2;     // no sign
            sc2 = -e->SF2;
            break;  // no sign wysiwyg
    }

    if (n2 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "dv3d adjusted n2<1");
    
    switch(e->S3)
    {
        case CSFL:
            n3 = (int) e->N3 - 1; // need to account for the sign
            if (dstTN == CTN4)
                n3 -= 2;    // 2 4-bit digit exponent
            else
                n3 -= 1;    // 1 9-bit digit exponent
            break;
            
        case CSLS:
        case CSTS:
            n3 = (int) e->N3 - 1; // 1 sign
            break;
            
        case CSNS:
            n3 = (int) e->N3;     // no sign
            break;  // no sign wysiwyg
    }
    if (n3 < 1)
        doFault (FAULT_IPR, fst_ill_proc, "dv3d adjusted n3<1");


    decContext set;
    decContextDefaultDPS8(&set);
    
    set.traps=0;
    
    decNumber _1, _2, _3;
    
    EISloadInputBufferNumeric (1);   // according to MF1
    
    decNumber *op1 = decBCD9ToNumber(e->inBuffer, n1, sc1, &_1);
    //PRINTDEC("op1", op1);
    if (e->sign == -1)
        op1->bits |= DECNEG;
    if (e->S1 == CSFL)
        op1->exponent = e->exponent;

    // check for divide by 0!
    if (decNumberIsZero(op1))
    {
        doFault(FAULT_DIV, fst_zero, "dv3d division by 0");
    }

    word9   inBufferop1 [64];
    memcpy (inBufferop1,e->inBuffer,64); // save for clz1 calculation later
    
    EISloadInputBufferNumeric (2);   // according to MF2

    decNumber *op2 = decBCD9ToNumber(e->inBuffer, n2, sc2, &_2);
    if (e->sign == -1)
        op2->bits |= DECNEG;
    if (e->S2 == CSFL)
        op2->exponent = e->exponent;


    // The number of required quotient digits, NQ, is determined before
    // division begins as follows:
    //  1) Floating-point quotient
    //      NQ = N3
    //  2) Fixed-point quotient
    //    NQ = (N2-LZ2+1) - (N1-LZ1) + (E2-E1-SF3)
    //    ￼where: Nn = given operand field length
    //        LZn = leading zero count for operand n
    //        En = exponent of operand n
    //        SF3 = scaling factor of quotient
    // 3) Rounding
    //    If rounding is specified (R = 1), then one extra quotient digit is
    //    produced.
    // Note: rule 3 is already handled by formatDecimal rounding
    // Nn doesn't represent full field length, but length without sign and exponent (RJ78/DH03 seems like)

    int NQ;
    if (e->S3 == CSFL)
    {
        NQ = n3;
    } 
    else 
    {
        // count leading zeroes
        // TODO optimize - can these be somehow extracted from decNumbers?
        int clz1, clz2, i;
        for (i=0; i < op1->digits; i++)
            if (inBufferop1[i]!=0)
                break;
        clz1 = i;
        for (i=0; i < op2->digits; i++)
            if (e->inBuffer[i]!=0) // this still holds op2 digits
                break;
        clz2 = i;
        sim_debug (DBG_TRACEEXT, & cpu_dev, "dv3d: clz1 %d clz2 %d\n",clz1,clz2);

        // XXX are clz also valid for CSFL dividend / divisor? probably yes
        // XXX seems that exponents and scale factors are used interchangeably here ? (RJ78)
        NQ = (n2-clz2+1) - (n1-clz1) + ((e->S2==CSFL?op2->exponent:(int)e->SF2)-(e->S1==CSFL?op1->exponent:(int)e->SF1)-(int)e->SF3);

sim_debug (DBG_TRACEEXT, & cpu_dev, "dv3d S1 %d S2 %d N1 %d N2 %d clz1 %d clz2 %d E1 %d E2 %d SF3 %d NQ %d\n",e->S1,e->S2,e->N1,e->N2,clz1,clz2,op1->exponent,op2->exponent,e->SF3,NQ);
    }
    if (NQ > 63)
        doFault(FAULT_DIV, fst_zero, "dv3d NQ>63");
    // Note: NQ is currently unused apart from this FAULT_DIV check. decNumber produces more digits than required, but they are then rounded/truncated

    // Yes, they're switched. op1=divisor
    decNumber *op3 = decNumberDivide(&_3, op2, op1, &set); 
    // Note DPS88 and DPS9000 are different when NQ <= 0
    // This is a flaw in the DPS8/70 hardware which was corrected in later models
    // ISOLTS-817 05b

    PRINTDEC("op2", op2);
    PRINTDEC("op1", op1);
    PRINTDEC("op3", op3);
    
    // let's check division results to see for anomalous conditions
    if (
        (set.status & DEC_Division_undefined) ||    // 0/0 will become NaN
        (set.status & DEC_Invalid_operation) ||
        (set.status & DEC_Division_by_zero)
        ) { sim_debug (DBG_TRACEEXT, & cpu_dev, "oops! dv3d anomalous results"); }	// divide by zero has already been checked before

    if (e->S3 == CSFL)
    {
        // justify CSFL left
        // This is a flaw in the DPS8/70 hardware which was corrected in later models
        // Note DPS88 and DPS9000 are different
        // ISOLTS-817 06c,06e

        decNumber _sf;

        if (n3 - op3->digits > 0)
        {
            decNumberFromInt32(&_sf, op3->exponent - (n3 - op3->digits));
            PRINTDEC("Value 1", op3)
            PRINTDEC("Value sf", &_sf)
            op3 = decNumberRescale(op3, op3, &_sf, &set);
            PRINTDEC("Value 2", op3)
        }
    }

    bool Ovr = false, EOvr = false, Trunc = false;
    uint8_t out[256];

    // CSFL: If the divisor is greater than the dividend after operand
    // alignment, the leading zero digit produced is counted and the effective
    // precision of the result is reduced by one.
    // This is a flaw in the DPS8/70 hardware which was corrected in later models
    // Note DPS88 and DPS9000 are different
    //
    // "greater after operand alignment" means scale until most-significant digits are nonzero, then compare magnitudes ignoring exponents
    // This passes ISOLTS-817 06e, ET 458,461,483,486
    char *res;
    if (e->S3 == CSFL) {
        decNumber _1a;
        decNumber _2a;
        decNumber _sf;
        if (op1->digits >= op2->digits) {
            // scale op2
            decNumberCopy(&_1a, op1);
            decNumberFromInt32(&_sf, op1->digits - op2->digits);
            decNumberShift(&_2a, op2, &_sf, &set);
        } else if (op1->digits < op2->digits) {
            // scale op1
            decNumberFromInt32(&_sf, op2->digits - op1->digits);
            decNumberShift(&_1a, op1, &_sf, &set);
            decNumberCopy(&_2a, op2);
        }
        _1a.exponent = 0;
        _2a.exponent = 0;

        PRINTDEC("dv3d: op1a", &_1a);
        PRINTDEC("dv3d: op2a", &_2a);
        sim_debug (DBG_TRACEEXT, & cpu_dev, "dv3d: exp1 %d exp2 %d digits op1 %d op2 %d op1a %d op2a %d\n",op1->exponent,op2->exponent,op1->digits,op2->digits,_1a.digits,_2a.digits);

        if (decCompareMAG(&_1a, &_2a, &set) > 0) {
            // shorten the result field to get proper rounding
            res = formatDecimal(out, &set, op3, n3 -1, (int) e->S3, e->SF3, R, &Ovr, &Trunc);

            // prepend zero digit
            // ET 458,483 float=float/float, ET 461,486 float=fixed/fixed
            for (int i = n3; i > 0; i--) // incl.zero terminator
                 res[i] = res[i-1];
            res[0] = '0';
            sim_debug (DBG_TRACEEXT, & cpu_dev, "dv3d: addzero n3 %d %s exp %d\n",n3,res,op3->exponent);
        } else {
            // full n3 digits are retured
            res = formatDecimal(out, &set, op3, n3, (int) e->S3, e->SF3, R, &Ovr, &Trunc);
        }
    } else {
        // same as all other decimal instructions
        res = formatDecimal(out, &set, op3, n3, (int) e->S3, e->SF3, R, &Ovr, &Trunc);
    }
    
    if (decNumberIsZero(op3))
        op3->exponent = 127;

    //printf("%s\r\n", res);
    
    // now write to memory in proper format.....
    
    int pos = (int) dstCN;
    
    // 1st, take care of any leading sign .......
    switch(e->S3)
    {
        case CSFL:  // floating-point, leading sign.
        case CSLS:  // fixed-point, leading sign
            switch(dstTN)
            {
                case CTN4:
                    if (e->P) //If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus sign character is placed appropriately if the result of the operation is positive.
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  013);  // special +
                    else
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  014);  // default +
                    break;
                case CTN9:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? '-' : '+');
                    break;
            }
            break;
            
        case CSTS:  // nuttin' to do here .....
        case CSNS:
            break;  // no sign wysiwyg
    }
    
    // 2nd, write the digits .....
    for(int i = 0 ; i < n3 ; i++)
        switch(dstTN)
        {
            case CTN4:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, (word9) (res[i] - '0'));
                break;
            case CTN9:
                EISwrite49(&e->ADDR3, &pos, (int) dstTN, (word9) res[i]);
                break;
            }
    
    // 3rd, take care of any trailing sign or exponent ...
    switch(e->S3)
    {
        case CSTS:  // write trailing sign ....
            switch(dstTN)
            {
                case CTN4:
                    if (e->P) //If TN2 and S2 specify a 4-bit signed number and P = 1, then the 13(8) plus sign character is placed appropriately if the result of the operation is positive.
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  013);  // special +
                    else
                        EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? 015 :  014);  // default +
                    break;
                case CTN9:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (decNumberIsNegative(op3) && !decNumberIsZero(op3)) ? '-' : '+');
                break;
            }
            break;
            
        case CSFL:  // floating-point, leading sign.
            // write the exponent
            switch(dstTN)
            {
                case CTN4:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, (op3->exponent >> 4) & 0xf); // upper 4-bits
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN,  op3->exponent       & 0xf); // lower 4-bits
                    break;
                case CTN9:
                    EISwrite49(&e->ADDR3, &pos, (int) dstTN, op3->exponent & 0xff);    // write 8-bit exponent
                    break;
            }
            break;
            
        case CSLS:  // fixed point, leading sign - already done
        case CSNS:  // fixed point, unsigned - nuttin' needed to do
            break;
    }
    
    // set flags, etc ...
    if (e->S3 == CSFL)
    {
        if (op3->exponent > 127)
        {
            SET_I_EOFL;
            EOvr = true;
        }
        if (op3->exponent < -128)
        {
            SET_I_EUFL;
            EOvr = true;
        }
    }
    
    SC_I_NEG (decNumberIsNegative(op3) && !decNumberIsZero(op3));  // set negative indicator if op3 < 0
    SC_I_ZERO (decNumberIsZero(op3));     // set zero indicator if op3 == 0
    
    // SC_I_TRUNC(!R && Trunc); // no truncation flag for divide

    cleanupOperandDescriptor (1);
    cleanupOperandDescriptor (2);
    cleanupOperandDescriptor (3);
    
    //if (TST_I_TRUNC && T && tstOVFfault ())
    //  doFault(FAULT_OFL, fst_zero, "dv3d truncation(overflow) fault");
    if (EOvr && tstOVFfault ())
        doFault(FAULT_OFL, fst_zero, "dv3d over/underflow fault");
    if (Ovr)
    {
        SET_I_OFLOW;
        if (tstOVFfault ())
          doFault(FAULT_OFL, fst_zero, "dv3d overflow fault");
    }
}
