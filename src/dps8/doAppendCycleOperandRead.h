/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 3b4b8be2-171d-11ee-84fd-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2023 Charles Anthony
 * Copyright (c) 2022-2023 Jeffrey H. Johnson
 * Copyright (c) 2022-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

//
//        A:   fetch sdw
//        B:   check RB consistency
//        B1:  Is CALL6? Yes---------------------------------------+
//               No                                                |
//             Is transfer? Yes---------------+                E: Check RB
//               No                           |                    |
//             Check Read RB             F: Check Execute RB       |
//                |                      D: Check RALR             |
//                |                           |                    |
//                +<--------------------------+<-------------------+
//                |
//                V
//        G:   Check bound
//             Paged? No----------------------+
//               Yes                          |
//             Fetch PTW                      |
//        I:   Compute final address     H:   Compute Final Address
//                |                           |
//                +<--------------------------+
//                |
//                V
//        HI:  Read operand
//             CALL6? Yes--------------------------------------+
//               No                                            |
//             Transfer? Yes -----------------+                |
//               No                           |                |
//                |                           |                |
//                |                      L:  Handle TSPn   N: Handle CALL6
//                |                      KL: Set IC           Set IC
//                |                           |                |
//                |                           +<---------------+
//                |                           |
//                |                      M: Set P
//                |                           |
//                +<--------------------------+
//                |
//                V
//              Exit

word24 doAppendCycleOperandRead (word36 * data, uint nWords) {
static int evcnt = 0;
  DCDstruct * i = & cpu.currentInstruction;
  (void)evcnt;
  DBGAPP ("doAppendCycleOperandRead(Entry) thisCycle=OPERAND_READ\n");
  DBGAPP ("doAppendCycleOperandRead(Entry) lastCycle=%s\n", str_pct (cpu.apu.lastCycle));
  DBGAPP ("doAppendCycleOperandRead(Entry) CA %06o\n", cpu.TPR.CA);
  DBGAPP ("doAppendCycleOperandRead(Entry) n=%2u\n", nWords);
  DBGAPP ("doAppendCycleOperandRead(Entry) PPR.PRR=%o PPR.PSR=%05o\n", cpu.PPR.PRR, cpu.PPR.PSR);
  DBGAPP ("doAppendCycleOperandRead(Entry) TPR.TRR=%o TPR.TSR=%05o\n", cpu.TPR.TRR, cpu.TPR.TSR);

  if (i->b29) {
    DBGAPP ("doAppendCycleOperandRead(Entry) isb29 PRNO %o\n", GET_PRN (IWB_IRODD));
  }

  uint this = UC_OPERAND_READ;
  if (i->info->flags & TRANSFER_INS)
    this = UC_OPERAND_READ_TRA;
  if (i->info->flags & CALL6_INS)
    this = UC_OPERAND_READ_CALL6;

  word24 finalAddress = 0;
  word24 pageAddress = 0;
  word3 RSDWH_R1 = 0;
  word14 bound = 0;
  word1 p = 0;
  bool paged;

// Is this cycle a candidate for ucache?

//#define TEST_UCACHE
#if defined(TEST_UCACHE)
  bool cacheHit;
  cacheHit = false; // Assume skip...
#endif /* if defined(TEST_UCACHE) */

#if 1
  // Is OPCODE call6?

  // See E1; The TRR needs to be checked and set to R2; this will vary across different
  // CALL6 calls.
  if (i->info->flags & CALL6_INS) {
# if defined(UCACHE_STATS)
    cpu.uCache.call6Skips ++;
# endif /* if defined(UCACHE_STATS) */
    goto skip;
  }
#endif

#if 0
  // Transfer or instruction fetch?
  if (i->info->flags & TRANSFER_INS) {
    //cpu.uCache.uc_xfer_skip ++;
    goto skip;
  }
#endif

  // Transfer?
  if (i->info->flags & TRANSFER_INS) {
    // check ring alarm to catch outbound transfers
    if (cpu.rRALR && (cpu.PPR.PRR >= cpu.rRALR)) {
#if defined(UCACHE_STATS)
      cpu.uCache.ralrSkips ++;
#endif /* if defined(UCACHE_STATS) */
      goto skip;
    }
  }

// Yes; check the ucache

#if defined(TEST_UCACHE)
  word24 cachedAddress;
  word3 cachedR1;
  word14 cachedBound;
  word1 cachedP;
  bool cachedPaged;
  cacheHit = ucCacheCheck (this, cpu.TPR.TSR, cpu.TPR.CA, & cachedBound, & cachedP, & cachedAddress, & cachedR1, & cachedPaged);
# if defined(HDBG)
  hdbgNote ("doAppendCycleOperandRead.h", "test cache check %s %d %u %05o:%06o %05o %o %08o %o %o", cacheHit ? "hit" : "miss", evcnt, this, cpu.TPR.TSR, cpu.TPR.CA, cachedBound, cachedP, cachedAddress, cachedR1, cachedPaged);
# endif /* if defined(HDBG) */
  goto miss;
#else
  if (! ucCacheCheck (this, cpu.TPR.TSR, cpu.TPR.CA, & bound, & p, & pageAddress, & RSDWH_R1, & paged)) {
# if defined(HDBG)
    hdbgNote ("doAppendCycleOperandRead.h", "miss %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
# endif /* if defined(HDBG) */
    goto miss;
  }
#endif /* if defined(TEST_UCACHE) */

  if (paged) {
    finalAddress = pageAddress + (cpu.TPR.CA & OS18MASK);
  } else {
    finalAddress = pageAddress + cpu.TPR.CA;
  }
  cpu.RSDWH_R1 = RSDWH_R1;

// ucache hit; housekeeping...
  //sim_printf ("hit  %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
#if defined(HDBG)
  hdbgNote ("doAppendCycleOperandRead.h", "hit  %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
#endif /* if defined(HDBG) */

  cpu.apu.lastCycle = OPERAND_READ;
  goto HI;

#if 1
skip:;
  //sim_printf ("miss %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
# if defined(HDBG)
  hdbgNote ("doAppendCycleOperandRead.h", "skip %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
# endif /* if defined(HDBG) */
# if defined(UCACHE_STATS)
  cpu.uCache.skips[this] ++;
# endif /* if defined(UCACHE_STATS) */
#endif /* if defined(TEST_UCACHE) */

miss:;

  bool nomatch = true;
  if (cpu.tweaks.enable_wam) {
    // AL39: The associative memory is ignored (forced to "no match") during
    // address preparation.
    // lptp,lptr,lsdp,lsdr,sptp,sptr,ssdp,ssdr
    // Unfortunately, ISOLTS doesn't try to execute any of these in append mode.
    // XXX should this be only for OPERAND_READ and OPERAND_STORE?
    nomatch = ((i->opcode == 0232 || i->opcode == 0254 ||
                i->opcode == 0154 || i->opcode == 0173) &&
                i->opcodeX ) ||
               ((i->opcode == 0557 || i->opcode == 0257) &&
                ! i->opcodeX);
  }

  processor_cycle_type lastCycle = cpu.apu.lastCycle;
  cpu.apu.lastCycle = OPERAND_READ;

  DBGAPP ("doAppendCycleOperandRead(Entry) XSF %o\n", cpu.cu.XSF);

  PNL (L68_ (cpu.apu.state = 0;))

  cpu.RSDWH_R1 = 0;

  cpu.acvFaults = 0;

//#define FMSG(x) x
#define FMSG(x)
  FMSG (char * acvFaultsMsg = "<unknown>";)

////////////////////////////////////////
//
// Sheet 1: "START APPEND"
//
////////////////////////////////////////

// START APPEND
  word3 n = 0; // PRn to be saved to TSN_PRNO

////////////////////////////////////////
//
// Sheet 2: "A"
//
////////////////////////////////////////

//
//  A:
//    Get SDW

  //PNL (cpu.APUMemAddr = address;)
  PNL (cpu.APUMemAddr = cpu.TPR.CA;)

  DBGAPP ("doAppendCycleOperandRead(A)\n");

  // is SDW for C(TPR.TSR) in SDWAM?
  if (nomatch || ! fetch_sdw_from_sdwam (cpu.TPR.TSR)) {
    // No
    DBGAPP ("doAppendCycleOperandRead(A):SDW for segment %05o not in SDWAM\n", cpu.TPR.TSR);
    DBGAPP ("doAppendCycleOperandRead(A):DSBR.U=%o\n", cpu.DSBR.U);

    if (cpu.DSBR.U == 0) {
      fetch_dsptw (cpu.TPR.TSR);

      if (! cpu.PTW0.DF)
        doFault (FAULT_DF0 + cpu.PTW0.FC, fst_zero, "doAppendCycleOperandRead(A): PTW0.F == 0");

      if (! cpu.PTW0.U)
        modify_dsptw (cpu.TPR.TSR);

      fetch_psdw (cpu.TPR.TSR);
    } else
      fetch_nsdw (cpu.TPR.TSR); // load SDW0 from descriptor segment table.

    if (cpu.SDW0.DF == 0) {
      DBGAPP ("doAppendCycleOperandRead(A): SDW0.F == 0! " "Initiating directed fault\n");
      // initiate a directed fault ...
      doFault (FAULT_DF0 + cpu.SDW0.FC, fst_zero, "SDW0.F == 0");
    }
    // load SDWAM .....
    load_sdwam (cpu.TPR.TSR, nomatch);
  }
  DBGAPP ("doAppendCycleOperandRead(A) R1 %o R2 %o R3 %o E %o\n", cpu.SDW->R1, cpu.SDW->R2, cpu.SDW->R3, cpu.SDW->E);

  // Yes...
  RSDWH_R1 = cpu.RSDWH_R1 = cpu.SDW->R1;

////////////////////////////////////////
//
// Sheet 3: "B"
//
////////////////////////////////////////

//
// B: Check the ring
//

  DBGAPP ("doAppendCycleOperandRead(B)\n");

  // check ring bracket consistency

  //C(SDW.R1) <= C(SDW.R2) <= C(SDW .R3)?
  if (! (cpu.SDW->R1 <= cpu.SDW->R2 && cpu.SDW->R2 <= cpu.SDW->R3)) {
    // Set fault ACV0 = IRO
    cpu.acvFaults |= ACV0;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(B) C(SDW.R1) <= C(SDW.R2) <= " "C(SDW .R3)";)
  }

  // lastCycle == RTCD_OPERAND_FETCH
  // if a fault happens between the RTCD_OPERAND_FETCH and the INSTRUCTION_FETCH
  // of the next instruction - this happens about 35 time for just booting  and
  // shutting down multics -- a stored lastCycle is useless.
  // the opcode is preserved across faults and only replaced as the
  // INSTRUCTION_FETCH succeeds.
  if (lastCycle == RTCD_OPERAND_FETCH)
    sim_warn ("%s: lastCycle == RTCD_OPERAND_FETCH opcode %0#o\n", __func__, i->opcode);

  //
  // B1: The operand is one of: an instruction, data to be read or data to be
  //     written
  //

  // Is OPCODE call6?
  if (i->info->flags & CALL6_INS)
    goto E;

  // Transfer
  if (i->info->flags & TRANSFER_INS)
    goto F;

  //
  // check read bracket for read access
  //

  DBGAPP ("doAppendCycleOperandRead(B):!STR-OP\n");

  // No
  // C(TPR.TRR) > C(SDW .R2)?
  if (cpu.TPR.TRR > cpu.SDW->R2) {
    DBGAPP ("ACV3\n");
    DBGAPP ("doAppendCycleOperandRead(B) ACV3\n");
    //Set fault ACV3 = ORB
    cpu.acvFaults |= ACV3;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(B) C(TPR.TRR) > C(SDW .R2)";)
  }

  if (cpu.SDW->R == 0) {
    // isolts 870
    cpu.TPR.TRR = cpu.PPR.PRR;

    //C(PPR.PSR) = C(TPR.TSR)?
    if (cpu.PPR.PSR != cpu.TPR.TSR) {
      DBGAPP ("ACV4\n");
      DBGAPP ("doAppendCycleOperandRead(B) ACV4\n");
      //Set fault ACV4 = R-OFF
      cpu.acvFaults |= ACV4;
      PNL (L68_ (cpu.apu.state |= apu_FLT;))
      FMSG (acvFaultsMsg = "acvFaults(B) C(PPR.PSR) = C(TPR.TSR)";)
    //} else {
      // sim_warn ("doAppendCycleOperandRead(B) SDW->R == 0 && cpu.PPR.PSR == cpu.TPR.TSR: %0#o\n", cpu.PPR.PSR);
    }
  }

  goto G;

////////////////////////////////////////
//
// Sheet 4: "C" "D"
//
////////////////////////////////////////

D:;
  DBGAPP ("doAppendCycleOperandRead(D)\n");

  // transfer or instruction fetch

  // check ring alarm to catch outbound transfers

  if (cpu.rRALR == 0)
    goto G;

  // C(PPR.PRR) < RALR?
  if (! (cpu.PPR.PRR < cpu.rRALR)) {
    DBGAPP ("ACV13\n");
    DBGAPP ("acvFaults(D) C(PPR.PRR) %o < RALR %o\n", cpu.PPR.PRR, cpu.rRALR);
    cpu.acvFaults |= ACV13;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(D) C(PPR.PRR) < RALR";)
  }

  goto G;

////////////////////////////////////////
//
// Sheet 5: "E"
//
////////////////////////////////////////

E:;

  //
  // check ring bracket for instruction fetch after call6 instruction
  //   (this is the call6 read operand)
  //

  DBGAPP ("doAppendCycleOperandRead(E): CALL6\n");
  DBGAPP ("doAppendCycleOperandRead(E): E %o G %o PSR %05o TSR %05o CA %06o " "EB %06o R %o%o%o TRR %o PRR %o\n", cpu.SDW->E, cpu.SDW->G, cpu.PPR.PSR, cpu.TPR.TSR, cpu.TPR.CA, cpu.SDW->EB, cpu.SDW->R1, cpu.SDW->R2, cpu.SDW->R3, cpu.TPR.TRR, cpu.PPR.PRR);

  //SDW.E set ON?
  if (! cpu.SDW->E) {
    DBGAPP ("ACV2 b\n");
    DBGAPP ("doAppendCycleOperandRead(E) ACV2\n");
    // Set fault ACV2 = E-OFF
    cpu.acvFaults |= ACV2;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(E) SDW .E set OFF";)
  }

  //SDW .G set ON?
  if (cpu.SDW->G)
    goto E1;

  // C(PPR.PSR) = C(TPR.TSR)?
  if (cpu.PPR.PSR == cpu.TPR.TSR && ! TST_I_ABS)
    goto E1;

  // XXX This doesn't seem right
  // EB is word 15; masking address makes no sense; rather 0-extend EB
  // Fixes ISOLTS 880-01
  if (cpu.TPR.CA >= (word18) cpu.SDW->EB) {
    DBGAPP ("ACV7\n");
    DBGAPP ("doAppendCycleOperandRead(E) ACV7\n");
    // Set fault ACV7 = NO GA
    cpu.acvFaults |= ACV7;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(E) TPR.CA4-17 >= SDW.CL";)
  }

E1:
  DBGAPP ("doAppendCycleOperandRead(E1): CALL6 (cont'd)\n");

  // C(TPR.TRR) > SDW.R3?
  if (cpu.TPR.TRR > cpu.SDW->R3) {
    DBGAPP ("ACV8\n");
    DBGAPP ("doAppendCycleOperandRead(E) ACV8\n");
    //Set fault ACV8 = OCB
    cpu.acvFaults |= ACV8;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(E1) C(TPR.TRR) > SDW.R3";)
  }

  // C(TPR.TRR) < SDW.R1?
  if (cpu.TPR.TRR < cpu.SDW->R1) {
    DBGAPP ("ACV9\n");
    DBGAPP ("doAppendCycleOperandRead(E) ACV9\n");
    // Set fault ACV9 = OCALL
    cpu.acvFaults |= ACV9;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(E1) C(TPR.TRR) < SDW.R1";)
  }

  // C(TPR.TRR) > C(PPR.PRR)?
  if (cpu.TPR.TRR > cpu.PPR.PRR) {
    // C(PPR.PRR) < SDW.R2?
    if (cpu.PPR.PRR < cpu.SDW->R2) {
      DBGAPP ("ACV10\n");
      DBGAPP ("doAppendCycleOperandRead(E) ACV10\n");
      // Set fault ACV10 = BOC
      cpu.acvFaults |= ACV10;
      PNL (L68_ (cpu.apu.state |= apu_FLT;))
      FMSG (acvFaultsMsg = "acvFaults(E1) C(TPR.TRR) > C(PPR.PRR) && " "C(PPR.PRR) < SDW.R2";)
    }
  }

  DBGAPP ("doAppendCycleOperandRead(E1): CALL6 TPR.TRR %o SDW->R2 %o\n", cpu.TPR.TRR, cpu.SDW->R2);

  // C(TPR.TRR) > SDW.R2?
  if (cpu.TPR.TRR > cpu.SDW->R2) {
    // SDW.R2 -> C(TPR.TRR)
    cpu.TPR.TRR = cpu.SDW->R2;
  }

  DBGAPP ("doAppendCycleOperandRead(E1): CALL6 TPR.TRR %o\n", cpu.TPR.TRR);

  goto G;

////////////////////////////////////////
//
// Sheet 6: "F"
//
////////////////////////////////////////

F:;
  PNL (L68_ (cpu.apu.state |= apu_PIAU;))
  DBGAPP ("doAppendCycleOperandRead(F): transfer or instruction fetch\n");

  //
  // check ring bracket for instruction fetch
  //

  // C(TPR.TRR) < C(SDW .R1)?
  // C(TPR.TRR) > C(SDW .R2)?
  if (cpu.TPR.TRR < cpu.SDW->R1 || cpu.TPR.TRR > cpu.SDW->R2) {
    DBGAPP ("ACV1 a/b\n");
    DBGAPP ("acvFaults(F) ACV1 !( C(SDW .R1) %o <= C(TPR.TRR) %o <= C(SDW .R2) %o )\n", cpu.SDW->R1, cpu.TPR.TRR, cpu.SDW->R2);
    cpu.acvFaults |= ACV1;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(F) C(TPR.TRR) < C(SDW .R1)";)
  }
  // SDW .E set ON?
  if (! cpu.SDW->E) {
    DBGAPP ("ACV2 c \n");
    DBGAPP ("doAppendCycleOperandRead(F) ACV2\n");
    cpu.acvFaults |= ACV2;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(F) SDW .E set OFF";)
  }

  // C(PPR.PRR) = C(TPR.TRR)?
  if (cpu.PPR.PRR != cpu.TPR.TRR) {
    DBGAPP ("ACV12\n");
    DBGAPP ("doAppendCycleOperandRead(F) ACV12\n");
    //Set fault ACV12 = CRT
    cpu.acvFaults |= ACV12;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(F) C(PPR.PRR) != C(TPR.TRR)";)
  }

  goto D;

////////////////////////////////////////
//
// Sheet 7: "G"
//
////////////////////////////////////////

G:;

  DBGAPP ("doAppendCycleOperandRead(G)\n");

  //C(TPR.CA)0,13 > SDW.BOUND?
  if (((cpu.TPR.CA >> 4) & 037777) > cpu.SDW->BOUND) {
    DBGAPP ("ACV15\n");
    DBGAPP ("doAppendCycleOperandRead(G) ACV15\n");
    cpu.acvFaults |= ACV15;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(G) C(TPR.CA)0,13 > SDW.BOUND";)
    DBGAPP ("acvFaults(G) C(TPR.CA)0,13 > SDW.BOUND\n" "   CA %06o CA>>4 & 037777 %06o SDW->BOUND %06o", cpu.TPR.CA, ((cpu.TPR.CA >> 4) & 037777), cpu.SDW->BOUND);
  }
  bound = cpu.SDW->BOUND;
  p = cpu.SDW->P;

  if (cpu.acvFaults) {
    DBGAPP ("doAppendCycleOperandRead(G) acvFaults\n");
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    // Initiate an access violation fault
    doFault (FAULT_ACV, (_fault_subtype) {.fault_acv_subtype=cpu.acvFaults}, "ACV fault");
  }

  // is segment C(TPR.TSR) paged?
  if (cpu.SDW->U)
    goto H; // Not paged

  // Yes. segment is paged ...
  // is PTW for C(TPR.CA) in PTWAM?

  DBGAPP ("doAppendCycleOperandRead(G) CA %06o\n", cpu.TPR.CA);
  if (nomatch ||
      ! fetch_ptw_from_ptwam (cpu.SDW->POINTER, cpu.TPR.CA)) {
    fetch_ptw (cpu.SDW, cpu.TPR.CA);
    if (! cpu.PTW0.DF) {
      // initiate a directed fault
      doFault (FAULT_DF0 + cpu.PTW0.FC, (_fault_subtype) {.bits=0}, "PTW0.F == 0");
    }
    loadPTWAM (cpu.SDW->POINTER, cpu.TPR.CA, nomatch); // load PTW0 to PTWAM
  }

  // Prepage mode?
  // check for "uninterruptible" EIS instruction
  // ISOLTS-878 02: mvn,cmpn,mvne,ad3d; obviously also
  // ad2/3d,sb2/3d,mp2/3d,dv2/3d
  // DH03 p.8-13: probably also mve,btd,dtb
  if (i->opcodeX && ((i->opcode & 0770)== 0200|| (i->opcode & 0770) == 0220
      || (i->opcode & 0770)== 020|| (i->opcode & 0770) == 0300)) {
    do_ptw2 (cpu.SDW, cpu.TPR.CA);
  }
  goto I;

////////////////////////////////////////
//
// Sheet 8: "H", "I"
//
////////////////////////////////////////

H:;
  DBGAPP ("doAppendCycleOperandRead(H): FANP\n");

  paged = false;

  PNL (L68_ (cpu.apu.state |= apu_FANP;))
#if 0
  // ISOLTS pa865 test-01a 101232
  if (get_bar_mode ()) {
    set_apu_status (apuStatus_FABS);
  } else
    ....
#endif
  set_apu_status (apuStatus_FANP);
#if defined(HDBG)
  hdbgNote ("doAppendCycleOperandRead", "FANP");
#endif /* if defined(HDBG) */
  DBGAPP ("doAppendCycleOperandRead(H): SDW->ADDR=%08o CA=%06o \n", cpu.SDW->ADDR, cpu.TPR.CA);

  pageAddress = (cpu.SDW->ADDR & 077777760);
  finalAddress = (cpu.SDW->ADDR & 077777760) + cpu.TPR.CA;
  finalAddress &= 0xffffff;
  PNL (cpu.APUMemAddr = finalAddress;)

  DBGAPP ("doAppendCycleOperandRead(H:FANP): (%05o:%06o) finalAddress=%08o\n", cpu.TPR.TSR, cpu.TPR.CA, finalAddress);

  goto HI;

I:;

// Set PTW.M

  DBGAPP ("doAppendCycleOperandRead(I): FAP\n");

  paged = true;

#if defined(HDBG)
  hdbgNote ("doAppendCycleOperandRead", "FAP");
#endif /* if defined(HDBG) */
  // final address paged
  set_apu_status (apuStatus_FAP);
  PNL (L68_ (cpu.apu.state |= apu_FAP;))

  word24 y2 = cpu.TPR.CA % 1024;

  pageAddress = (((word24)cpu.PTW->ADDR & 0777760) << 6);
  // AL39: The hardware ignores low order bits of the main memory page
  // address according to page size
  finalAddress = (((word24)cpu.PTW->ADDR & 0777760) << 6) + y2;
  finalAddress &= 0xffffff;
  PNL (cpu.APUMemAddr = finalAddress;)

#if defined(L68)
  if (cpu.MR_cache.emr && cpu.MR_cache.ihr)
    add_APU_history (APUH_FAP);
#endif /* if defined(L68) */
  DBGAPP ("doAppendCycleOperandRead(H:FAP): (%05o:%06o) finalAddress=%08o\n", cpu.TPR.TSR, cpu.TPR.CA, finalAddress);

  //goto HI;

HI:
  DBGAPP ("doAppendCycleOperandRead(HI)\n");

#if defined(TEST_UCACHE)
  if (cacheHit) {
    bool err = false;
    if (cachedAddress != pageAddress) {
     sim_printf ("cachedAddress %08o != pageAddress %08o\r\n", cachedAddress, pageAddress);
     err = true;
    }
    if (cachedR1 != RSDWH_R1) {
      sim_printf ("cachedR1 %01o != RSDWH_R1 %01o\r\n", cachedR1, RSDWH_R1);
      err = true;
    }
    if (cachedBound != bound) {
      sim_printf ("cachedBound %01o != bound %01o\r\n", cachedBound, bound);
      err = true;
    }
    if (cachedPaged != paged) {
      sim_printf ("cachedPaged %01o != paged %01o\r\n", cachedPaged, paged);
      err = true;
    }
    if (err) {
# if defined(HDBG)
      HDBGPrint ();
# endif /* if defined(HDBG) */
      sim_printf ("oprnd read err  %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
      exit (1);
    }
    //sim_printf ("hit  %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
# if defined(HDBG)
    hdbgNote ("doAppendCycleOperandRead.h", "test hit %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
# endif /* if defined(HDBG) */
  } else {
    //sim_printf ("miss %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
# if defined(HDBG)
    hdbgNote ("doAppendCycleOperandRead.h", "test miss %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
# endif /* if defined(HDBG) */
  }
#endif

  ucCacheSave (this, cpu.TPR.TSR, cpu.TPR.CA, bound, p, pageAddress, RSDWH_R1, paged);
#if defined(TEST_UCACHE)
# if defined(HDBG)
  hdbgNote ("doAppendCycleOperandRead.h", "cache %d %u %05o:%06o %05o %o %08o %o %o", evcnt, this, cpu.TPR.TSR, cpu.TPR.CA, bound, p, pageAddress, RSDWH_R1, paged);
# endif /* if defined(HDBG) */
#endif /* if defined(TEST_UCACHE) */
evcnt ++;

  // isolts 870
  cpu.cu.XSF = 1;
  sim_debug (DBG_TRACEEXT, & cpu_dev, "loading of cpu.TPR.TSR sets XSF to 1\n");

  core_readN (finalAddress, data, nWords, "OPERAND_READ");

  if (i->info->flags & CALL6_INS)
    goto N;

  // Transfer or instruction fetch?
  if (i->info->flags & TRANSFER_INS)
    goto L;

  // APU data movement?
  //  handled above
  goto Exit;

////////////////////////////////////////
//
// Sheet 10: "K", "L", "M", "N"
//
////////////////////////////////////////

L:; // Transfer or instruction fetch

  DBGAPP ("doAppendCycleOperandRead(L)\n");

  // Is OPCODE tspn?
  if (i->info->flags & TSPN_INS) {
    if (i->opcode <= 0273)
      n = (i->opcode & 3);
    else
      n = (i->opcode & 3) + 4;

    // C(PPR.PRR) -> C(PRn .RNR)
    // C(PPR.PSR) -> C(PRn .SNR)
    // C(PPR.IC) -> C(PRn .WORDNO)
    // 000000 -> C(PRn .BITNO)
    cpu.PR[n].RNR = cpu.PPR.PRR;
// According the AL39, the PSR is 'undefined' in absolute mode.
// ISOLTS thinks means don't change the operand
    if (get_addr_mode () == APPEND_mode)
      cpu.PR[n].SNR = cpu.PPR.PSR;
    cpu.PR[n].WORDNO = (cpu.PPR.IC + 1) & MASK18;
    SET_PR_BITNO (n, 0);
#if defined(TESTING)
    HDBGRegPRW (n, "app tspn");
#endif /* if defined(TESTING) */
  }

// KL:
  DBGAPP ("doAppendCycleOperandRead(KL)\n");

  // C(TPR.TSR) -> C(PPR.PSR)
  cpu.PPR.PSR = cpu.TPR.TSR;
  // C(TPR.CA) -> C(PPR.IC)
  cpu.PPR.IC = cpu.TPR.CA;

  //goto M;

M: // Set P
  DBGAPP ("doAppendCycleOperandRead(M)\n");

  // C(TPR.TRR) = 0?
  if (cpu.TPR.TRR == 0) {
    // C(SDW.P) -> C(PPR.P)
    cpu.PPR.P = p;
  } else {
    // 0 C(PPR.P)
    cpu.PPR.P = 0;
  }

  goto Exit;

N: // CALL6
  DBGAPP ("doAppendCycleOperandRead(N)\n");

  // C(TPR.TRR) = C(PPR.PRR)?
  if (cpu.TPR.TRR == cpu.PPR.PRR) {
    // C(PR6.SNR) -> C(PR7.SNR)
    cpu.PR[7].SNR = cpu.PR[6].SNR;
    DBGAPP ("doAppendCycleOperandRead(N) PR7.SNR = PR6.SNR %05o\n", cpu.PR[7].SNR);
  } else {
    // C(DSBR.STACK) || C(TPR.TRR) -> C(PR7.SNR)
    cpu.PR[7].SNR = ((word15) (cpu.DSBR.STACK << 3)) | cpu.TPR.TRR;
    DBGAPP ("doAppendCycleOperandRead(N) STACK %05o TRR %o\n", cpu.DSBR.STACK, cpu.TPR.TRR);
    DBGAPP ("doAppendCycleOperandRead(N) PR7.SNR = STACK||TRR  %05o\n", cpu.PR[7].SNR);
  }

  // C(TPR.TRR) -> C(PR7.RNR)
  cpu.PR[7].RNR = cpu.TPR.TRR;
  // 00...0 -> C(PR7.WORDNO)
  cpu.PR[7].WORDNO = 0;
  // 000000 -> C(PR7.BITNO)
  SET_PR_BITNO (7, 0);
#if defined(TESTING)
  HDBGRegPRW (7, "app call6");
#endif /* if defined(TESTING) */
  // C(TPR.TRR) -> C(PPR.PRR)
  cpu.PPR.PRR = cpu.TPR.TRR;
  // C(TPR.TSR) -> C(PPR.PSR)
  cpu.PPR.PSR = cpu.TPR.TSR;
  // C(TPR.CA) -> C(PPR.IC)
  cpu.PPR.IC = cpu.TPR.CA;

  goto M;

Exit:;

  PNL (cpu.APUDataBusOffset = cpu.TPR.CA;)
  PNL (cpu.APUDataBusAddr = finalAddress;)

  PNL (L68_ (cpu.apu.state |= apu_FA;))

  DBGAPP ("doAppendCycleOperandRead (Exit) PRR %o PSR %05o P %o IC %06o\n", cpu.PPR.PRR, cpu.PPR.PSR, cpu.PPR.P, cpu.PPR.IC);
  DBGAPP ("doAppendCycleOperandRead (Exit) TRR %o TSR %05o TBR %02o CA %06o\n", cpu.TPR.TRR, cpu.TPR.TSR, cpu.TPR.TBR, cpu.TPR.CA);

  return finalAddress;    // or 0 or -1???
}
#undef TEST_UCACHE
