/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 45d63e9a-171d-11ee-b7ed-80ee73e9b8e7
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
//    A get SDW
//       |
//       V
//    B last cycle = RTCD operand fetch?
//       Yes                            No
//        |                              |
//        V                              V
//    C check ring brackets           F check ring brackets
//        |                              |
//        +------------------------------+
//        |
//        V
//    D check RALR
//        |
//        V
//    G check bound
//      paged?
//       Yes                            No
//        |                              |
//        V                              |
//    G1 get PTW                         |
//        |                              |
//        V                              V
//    I calc. paged address           H calc. unpaged address
//        |                              |
//        +------------------------------+
//        |
//        V
//    HI set XSF
//       read memory
//        |
//        V
//    L lastcyle RTCD operand fetch handling
//        |
//        V
//    KL set PSR, IC
//        |
//        V
//    M  set P
//        |
//        V
//    EXIT  return final address

word24 doAppendCycleInstructionFetch (cpu_state_t * cpup, word36 * data, uint nWords) {
//sim_printf ("doAppendCycleInstructionFetch %05o:%06o\r\n",
static int evcnt = 0;
  DCDstruct * i = & cpu.currentInstruction;
  (void)evcnt;
  DBGAPP ("doAppendCycleInstructionFetch(Entry) thisCycle=INSTRUCTION_FETCH\n");
  DBGAPP ("doAppendCycleInstructionFetch(Entry) lastCycle=%s\n", str_pct (cpu.apu.lastCycle));
  DBGAPP ("doAppendCycleInstructionFetch(Entry) CA %06o\n", cpu.TPR.CA);
  DBGAPP ("doAppendCycleInstructionFetch(Entry) n=%2u\n", nWords);
  DBGAPP ("doAppendCycleInstructionFetch(Entry) PPR.PRR=%o PPR.PSR=%05o\n", cpu.PPR.PRR, cpu.PPR.PSR);
  DBGAPP ("doAppendCycleInstructionFetch(Entry) TPR.TRR=%o TPR.TSR=%05o\n", cpu.TPR.TRR, cpu.TPR.TSR);

  if (i->b29) {
    DBGAPP ("doAppendCycleInstructionFetch(Entry) isb29 PRNO %o\n", GET_PRN (IWB_IRODD));
  }

  uint this = UC_INSTRUCTION_FETCH;

  word24 finalAddress = 0;
  word24 pageAddress = 0;
  word3 RSDWH_R1 = 0;
  word14 bound = 0;
  word1 p = 0;
  bool paged = false;

// ucache logic:
// The cache will hit if:
//   No CAMS/CAMP instruction has been executed.
//   The segment number matches the cached value.
//   The offset is on the same page as the cached value.
//
// doAppendCycle (INSTRUCTION_FETCH) checks:
//   associative memory: Don't Care. If the cache hits, the WAM won't be
//     queried which is the best case condition.
//   lastCycle: Set to INSTRUCTION_FETCH.
//   RSDWH_R1: Restored from cache.
//   lastCycle == RTCD_OPERAND_FETCH. One would think that RTCD would always
//     go to a new page, but that is not guaranteed; skip ucache.
//   rRALR. Since it is before a segment change, the ucache will always miss.
//   Ring brackets.  They will be the same, so recheck is unnecessary.
//   ACVs: They will be the same, so recheck is unnecessary.
//   SDW/PTW : They will be the same, so recheck is unnecessary.
//   Prepage mode:  Skip ucache.
//   History registers... Hm. skip for now; Values could be stashed...

// Is this cycle a candidate for ucache?

//#define TEST_UCACHE
#if defined(TEST_UCACHE)
  bool cacheHit;
  cacheHit = false; // Assume skip...
#endif

  // lastCycle == RTCD_OPERAND_FETCH
  if (i->opcode == 0610  && ! i->opcodeX) {
    //sim_printf ("skip RTCD\r\n");
    goto skip_ucache;
  }

  // RALR
  if (cpu.rRALR) {
    //sim_printf ("skip rRALR\r\n");
    goto skip_ucache;
  }

  // Prepage mode?
  // check for "uninterruptible" EIS instruction
  // ISOLTS-878 02: mvn,cmpn,mvne,ad3d; obviously also
  // ad2/3d,sb2/3d,mp2/3d,dv2/3d
  // DH03 p.8-13: probably also mve,btd,dtb
  if (i->opcodeX && ((i->opcode & 0770)== 0200|| (i->opcode & 0770) == 0220
      || (i->opcode & 0770)== 020|| (i->opcode & 0770) == 0300)) {
    //sim_printf ("skip uninterruptible\r\n");
    goto skip_ucache;
  }

// Yes; check the ucache

//#define TEST_UCACHE
#if defined(TEST_UCACHE)
  word24 cachedAddress;
  word3 cachedR1;
  word14 cachedBound;
  word1 cachedP;
  bool cachedPaged;
  cacheHit =
      ucCacheCheck (cpup, this, cpu.TPR.TSR, cpu.TPR.CA, & cachedBound, & cachedP, & cachedAddress, & cachedR1, & cachedPaged);
  goto miss_ucache;
#else
  if (! ucCacheCheck (cpup, this, cpu.TPR.TSR, cpu.TPR.CA, & bound, & p, & pageAddress, & RSDWH_R1, & paged))
    goto miss_ucache;
#endif

  if (paged) {
    finalAddress = pageAddress + (cpu.TPR.CA & OS18MASK);
  } else {
    finalAddress = pageAddress + cpu.TPR.CA;
  }
  cpu.RSDWH_R1 = RSDWH_R1;

// ucache hit; housekeeping...
  //sim_printf ("hit  %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);

  cpu.apu.lastCycle = INSTRUCTION_FETCH;
  goto HI;

skip_ucache:;
  //sim_printf ("miss %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
#if defined(UCACHE_STATS)
  cpu.uCache.skips[this] ++;
#endif

miss_ucache:;

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
  cpu.apu.lastCycle = INSTRUCTION_FETCH;

  DBGAPP ("doAppendCycleInstructionFetch(Entry) XSF %o\n", cpu.cu.XSF);

  PNL (L68_ (cpu.apu.state = 0;))

  cpu.RSDWH_R1 = 0;

  cpu.acvFaults = 0;

//#define FMSG(x) x
#define FMSG(x)
  FMSG (char * acvFaultsMsg = "<unknown>";)

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

  DBGAPP ("doAppendCycleInstructionFetch(A)\n");

  // is SDW for C(TPR.TSR) in SDWAM?
  if (nomatch || ! fetch_sdw_from_sdwam (cpup, cpu.TPR.TSR)) {
    // No
    DBGAPP ("doAppendCycleInstructionFetch(A):SDW for segment %05o not in SDWAM\n", cpu.TPR.TSR);

    DBGAPP ("doAppendCycleInstructionFetch(A):DSBR.U=%o\n", cpu.DSBR.U);

    if (cpu.DSBR.U == 0) {
      fetch_dsptw (cpup, cpu.TPR.TSR);

      if (! cpu.PTW0.DF)
        doFault (FAULT_DF0 + cpu.PTW0.FC, fst_zero, "doAppendCycleInstructionFetch(A): PTW0.F == 0");

      if (! cpu.PTW0.U)
        modify_dsptw (cpup, cpu.TPR.TSR);

      fetch_psdw (cpup, cpu.TPR.TSR);
    } else
      fetch_nsdw (cpup, cpu.TPR.TSR); // load SDW0 from descriptor segment table.

    if (cpu.SDW0.DF == 0) {
      DBGAPP ("doAppendCycleInstructionFetch(A): SDW0.F == 0! " "Initiating directed fault\n");
      // initiate a directed fault ...
      doFault (FAULT_DF0 + cpu.SDW0.FC, fst_zero, "SDW0.F == 0");
    }
    // load SDWAM .....
    load_sdwam (cpup, cpu.TPR.TSR, nomatch);
  }
  DBGAPP ("doAppendCycleInstructionFetch(A) R1 %o R2 %o R3 %o E %o\n", cpu.SDW->R1, cpu.SDW->R2, cpu.SDW->R3, cpu.SDW->E);

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

  DBGAPP ("doAppendCycleInstructionFetch(B)\n");

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
  if (i->opcode == 0610  && ! i->opcodeX)
    goto C;

  if (lastCycle == RTCD_OPERAND_FETCH)
    sim_warn ("%s: lastCycle == RTCD_OPERAND_FETCH opcode %0#o\n", __func__, i->opcode);

  //
  // B1: The operand is one of: an instruction, data to be read or data to be
  //     written
  //

  // Transfer or instruction fetch?
  goto F;

////////////////////////////////////////
//
// Sheet 4: "C" "D"
//
////////////////////////////////////////

C:;
  DBGAPP ("doAppendCycleInstructionFetch(C)\n");

  //
  // check ring bracket for instruction fetch after rtcd instruction
  //
  //   allow outbound transfers (cpu.TPR.TRR >= cpu.PPR.PRR)
  //

  // C(TPR.TRR) < C(SDW.R1)?
  // C(TPR.TRR) > C(SDW.R2)?
  if (cpu.TPR.TRR < cpu.SDW->R1 || cpu.TPR.TRR > cpu.SDW->R2) {
    DBGAPP ("ACV1 c\n");
    DBGAPP ("acvFaults(C) ACV1 ! ( C(SDW .R1) %o <= C(TPR.TRR) %o <= C(SDW .R2) %o )\n", cpu.SDW->R1, cpu.TPR.TRR, cpu.SDW->R2);
    //Set fault ACV1 = OEB
    cpu.acvFaults |= ACV1;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(C) C(SDW.R1 > C(TPR.TRR) > C(SDW.R2)";)
  }
  // SDW.E set ON?
  if (! cpu.SDW->E) {
    DBGAPP ("ACV2 a\n");
    DBGAPP ("doAppendCycleInstructionFetch(C) ACV2\n");
    //Set fault ACV2 = E-OFF
    cpu.acvFaults |= ACV2;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(C) SDW.E";)
  }
  if (cpu.TPR.TRR > cpu.PPR.PRR)
    sim_warn ("rtcd: outbound call cpu.TPR.TRR %d cpu.PPR.PRR %d\n", cpu.TPR.TRR, cpu.PPR.PRR);
  // C(TPR.TRR) >= C(PPR.PRR)
  if (cpu.TPR.TRR < cpu.PPR.PRR) {
    DBGAPP ("ACV11\n");
    DBGAPP ("doAppendCycleInstructionFetch(C) ACV11\n");
    //Set fault ACV11 = INRET
    cpu.acvFaults |= ACV11;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(C) TRR>=PRR";)
  }

D:;
  DBGAPP ("doAppendCycleInstructionFetch(D)\n");

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
// Sheet 6: "F"
//
////////////////////////////////////////

F:;
  PNL (L68_ (cpu.apu.state |= apu_PIAU;))
  DBGAPP ("doAppendCycleInstructionFetch(F): transfer or instruction fetch\n");

  //
  // check ring bracket for instruction fetch
  //

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
    DBGAPP ("doAppendCycleInstructionFetch(F) ACV2\n");
    cpu.acvFaults |= ACV2;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(F) SDW .E set OFF";)
  }

  // C(PPR.PRR) = C(TPR.TRR)?
  if (cpu.PPR.PRR != cpu.TPR.TRR) {
    DBGAPP ("ACV12\n");
    DBGAPP ("doAppendCycleInstructionFetch(F) ACV12\n");
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

  DBGAPP ("doAppendCycleInstructionFetch(G)\n");

  //C(TPR.CA)0,13 > SDW.BOUND?
  if (((cpu.TPR.CA >> 4) & 037777) > cpu.SDW->BOUND) {
    DBGAPP ("ACV15\n");
    DBGAPP ("doAppendCycleInstructionFetch(G) ACV15\n");
    cpu.acvFaults |= ACV15;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(G) C(TPR.CA)0,13 > SDW.BOUND";)
    DBGAPP ("acvFaults(G) C(TPR.CA)0,13 > SDW.BOUND\n" "   CA %06o CA>>4 & 037777 %06o SDW->BOUND %06o",
            cpu.TPR.CA, ((cpu.TPR.CA >> 4) & 037777), cpu.SDW->BOUND);
  }
  bound = cpu.SDW->BOUND;
  p = cpu.SDW->P;

  if (cpu.acvFaults) {
    DBGAPP ("doAppendCycleInstructionFetch(G) acvFaults\n");
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    // Initiate an access violation fault
    doFault (FAULT_ACV, (_fault_subtype) {.fault_acv_subtype=cpu.acvFaults}, "ACV fault");
  }

  // is segment C(TPR.TSR) paged?
  if (cpu.SDW->U)
    goto H; // Not paged

  // Yes. segment is paged ...
  // is PTW for C(TPR.CA) in PTWAM?

  DBGAPP ("doAppendCycleInstructionFetch(G) CA %06o\n", cpu.TPR.CA);
  if (nomatch ||
      ! fetch_ptw_from_ptwam (cpup, cpu.SDW->POINTER, cpu.TPR.CA)) {
    fetch_ptw (cpup, cpu.SDW, cpu.TPR.CA);
    if (! cpu.PTW0.DF) {
      // initiate a directed fault
      doFault (FAULT_DF0 + cpu.PTW0.FC, (_fault_subtype) {.bits=0}, "PTW0.F == 0");
    }
    loadPTWAM (cpup, cpu.SDW->POINTER, cpu.TPR.CA, nomatch); // load PTW0 to PTWAM
  }

  // Prepage mode?
  // check for "uninterruptible" EIS instruction
  // ISOLTS-878 02: mvn,cmpn,mvne,ad3d; obviously also
  // ad2/3d,sb2/3d,mp2/3d,dv2/3d
  // DH03 p.8-13: probably also mve,btd,dtb

  // XXX: PVS-Studio says that "i->opcodeX" is ALWAYS FALSE in the following check:
  if (i->opcodeX && ((i->opcode & 0770)== 0200|| (i->opcode & 0770) == 0220 //-V560
      || (i->opcode & 0770)== 020|| (i->opcode & 0770) == 0300)) {
      do_ptw2 (cpup, cpu.SDW, cpu.TPR.CA);
    }
  goto I;

////////////////////////////////////////
//
// Sheet 8: "H", "I"
//
////////////////////////////////////////

H:;
  DBGAPP ("doAppendCycleInstructionFetch(H): FANP\n");

  paged = false;

  PNL (L68_ (cpu.apu.state |= apu_FANP;))
#if 0
  // ISOLTS pa865 test-01a 101232
  if (get_bar_mode ()) {
      set_apu_status (apuStatus_FABS);
    }
  else
    ....
#endif
  set_apu_status (cpup, apuStatus_FANP);

  DBGAPP ("doAppendCycleInstructionFetch(H): SDW->ADDR=%08o CA=%06o \n", cpu.SDW->ADDR, cpu.TPR.CA);

  pageAddress = (cpu.SDW->ADDR & 077777760);
  finalAddress = (cpu.SDW->ADDR & 077777760) + cpu.TPR.CA;
  finalAddress &= 0xffffff;
  PNL (cpu.APUMemAddr = finalAddress;)

  DBGAPP ("doAppendCycleInstructionFetch(H:FANP): (%05o:%06o) finalAddress=%08o\n", cpu.TPR.TSR, cpu.TPR.CA, finalAddress);

  goto HI;

I:;

// Set PTW.M

  DBGAPP ("doAppendCycleInstructionFetch(I): FAP\n");

  paged = true;

  // final address paged
  set_apu_status (cpup, apuStatus_FAP);
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
  DBGAPP ("doAppendCycleInstructionFetch(H:FAP): (%05o:%06o) finalAddress=%08o\n", cpu.TPR.TSR, cpu.TPR.CA, finalAddress);

HI:
  DBGAPP ("doAppendCycleInstructionFetch(HI)\n");

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
# endif
      sim_printf ("ins fetch err  %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
      exit (1);
    }
    //sim_printf ("hit  %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
# if defined(HDBG)
    hdbgNote ("doAppendCycleOperandRead.h", "test hit %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
# endif
  } else {
    //sim_printf ("miss %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
# if defined(HDBG)
    hdbgNote ("doAppendCycleOperandRead.h", "test miss %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
# endif
  }
#endif
#if defined(TEST_UCACHE)
if (cacheHit) {
  if (cachedPaged != paged) sim_printf ("cachedPaged %01o != paged %01o\r\n", cachedPaged, paged);
  //sim_printf ("hit  %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
}
else
{
  //sim_printf ("miss %d %05o:%06o\r\n", evcnt, cpu.TPR.TSR, cpu.TPR.CA);
}
#endif

  ucCacheSave (cpup, this, cpu.TPR.TSR, cpu.TPR.CA, bound, p, pageAddress, RSDWH_R1, paged);
evcnt ++;
  // isolts 870
  cpu.cu.XSF = 1;
  sim_debug (DBG_TRACEEXT, & cpu_dev, "loading of cpu.TPR.TSR sets XSF to 1\n");

  core_readN (cpup, finalAddress, data, nWords, "INSTRUCTION_FETCH");

////////////////////////////////////////
//
// Sheet 10: "K", "L", "M", "N"
//
////////////////////////////////////////

//L:; // Transfer or instruction fetch

  DBGAPP ("doAppendCycleInstructionFetch(L)\n");

  // lastCycle == RTCD_OPERAND_FETCH

  if (i->opcode == 0610  && ! i->opcodeX) {
    // C(PPR.PRR) -> C(PRn.RNR) for n = (0, 1, ..., 7)
    // Use TRR here; PRR not set until KL
    CPTUR (cptUsePRn + 0);
    CPTUR (cptUsePRn + 1);
    CPTUR (cptUsePRn + 2);
    CPTUR (cptUsePRn + 3);
    CPTUR (cptUsePRn + 4);
    CPTUR (cptUsePRn + 5);
    CPTUR (cptUsePRn + 6);
    CPTUR (cptUsePRn + 7);
    cpu.PR[0].RNR =
    cpu.PR[1].RNR =
    cpu.PR[2].RNR =
    cpu.PR[3].RNR =
    cpu.PR[4].RNR =
    cpu.PR[5].RNR =
    cpu.PR[6].RNR =
    cpu.PR[7].RNR = cpu.TPR.TRR;
#if defined(TESTING)
    HDBGRegPRW (0, "app rtcd");
    HDBGRegPRW (1, "app rtcd");
    HDBGRegPRW (2, "app rtcd");
    HDBGRegPRW (3, "app rtcd");
    HDBGRegPRW (4, "app rtcd");
    HDBGRegPRW (5, "app rtcd");
    HDBGRegPRW (6, "app rtcd");
    HDBGRegPRW (7, "app rtcd");
#endif
  }
  goto KL;

KL:
  DBGAPP ("doAppendCycleInstructionFetch(KL)\n");

  // C(TPR.TSR) -> C(PPR.PSR)
  cpu.PPR.PSR = cpu.TPR.TSR;
  // C(TPR.CA) -> C(PPR.IC)
  cpu.PPR.IC = cpu.TPR.CA;

  goto M;

M: // Set P
  DBGAPP ("doAppendCycleInstructionFetch(M)\n");

  // C(TPR.TRR) = 0?
  if (cpu.TPR.TRR == 0) {
    // C(SDW.P) -> C(PPR.P)
    //cpu.PPR.P = cpu.SDW->P;
    cpu.PPR.P = p;
  } else {
    // 0 C(PPR.P)
    cpu.PPR.P = 0;
  }

  PNL (cpu.APUDataBusOffset = cpu.TPR.CA;)
  PNL (cpu.APUDataBusAddr = finalAddress;)

  PNL (L68_ (cpu.apu.state |= apu_FA;))

  DBGAPP ("doAppendCycleInstructionFetch (Exit) PRR %o PSR %05o P %o IC %06o\n",
          cpu.PPR.PRR, cpu.PPR.PSR, cpu.PPR.P, cpu.PPR.IC);
  DBGAPP ("doAppendCycleInstructionFetch (Exit) TRR %o TSR %05o TBR %02o CA %06o\n",
          cpu.TPR.TRR, cpu.TPR.TSR, cpu.TPR.TBR, cpu.TPR.CA);

  return finalAddress;
}
#undef TEST_UCACHE
