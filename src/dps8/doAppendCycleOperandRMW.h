/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 339b75b0-171d-11ee-bb9f-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2023 Charles Anthony
 * Copyright (c) 2022-2023 Jeffrey H. Johnson
 * Copyright (c) 2022-2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

word24 doAppendCycleOperandRMW (cpu_state_t * cpup, word36 * data, uint nWords) {
  DCDstruct * i = & cpu.currentInstruction;
  DBGAPP ("doAppendCycleOperandRMW(Entry) thisCycle=OPERAND_RMW\r\n");
  DBGAPP ("doAppendCycleOperandRMW(Entry) lastCycle=%s\r\n", str_pct (cpu.apu.lastCycle));
  DBGAPP ("doAppendCycleOperandRMW(Entry) CA %06o\r\n", cpu.TPR.CA);
  DBGAPP ("doAppendCycleOperandRMW(Entry) n=%2u\r\n", nWords);
  DBGAPP ("doAppendCycleOperandRMW(Entry) PPR.PRR=%o PPR.PSR=%05o\r\n", cpu.PPR.PRR, cpu.PPR.PSR);
  DBGAPP ("doAppendCycleOperandRMW(Entry) TPR.TRR=%o TPR.TSR=%05o\r\n", cpu.TPR.TRR, cpu.TPR.TSR);

  if (i->b29) {
    DBGAPP ("doAppendCycleOperandRMW(Entry) isb29 PRNO %o\r\n", GET_PRN (IWB_IRODD));
  }

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
  cpu.apu.lastCycle = OPERAND_RMW;

  DBGAPP ("doAppendCycleOperandRMW(Entry) XSF %o\r\n", cpu.cu.XSF);

  PNL (L68_ (cpu.apu.state = 0;))

  cpu.RSDWH_R1 = 0;

  cpu.acvFaults = 0;

//#define FMSG(x) x
#define FMSG(x)
  FMSG (char * acvFaultsMsg = "<unknown>";)

  word24 finalAddress = (word24) -1;  // not everything requires a final
                                      // address

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

  DBGAPP ("doAppendCycleOperandRMW(A)\r\n");

  // is SDW for C(TPR.TSR) in SDWAM?
  if (nomatch || ! fetch_sdw_from_sdwam (cpup, cpu.TPR.TSR)) {
    // No
    DBGAPP ("doAppendCycleOperandRMW(A):SDW for segment %05o not in SDWAM\r\n", cpu.TPR.TSR);
    DBGAPP ("doAppendCycleOperandRMW(A):DSBR.U=%o\r\n", cpu.DSBR.U);

    if (cpu.DSBR.U == 0) {
      fetch_dsptw (cpup, cpu.TPR.TSR);

      if (! cpu.PTW0.DF)
        doFault (FAULT_DF0 + cpu.PTW0.FC, fst_zero, "doAppendCycleOperandRMW(A): PTW0.F == 0");

      if (! cpu.PTW0.U)
        modify_dsptw (cpup, cpu.TPR.TSR);

      fetch_psdw (cpup, cpu.TPR.TSR);
    } else
      fetch_nsdw (cpup, cpu.TPR.TSR); // load SDW0 from descriptor segment table.

    if (cpu.SDW0.DF == 0) {
      DBGAPP ("doAppendCycleOperandRMW(A): SDW0.F == 0! Initiating directed fault\r\n");
      // initiate a directed fault ...
      doFault (FAULT_DF0 + cpu.SDW0.FC, fst_zero, "SDW0.F == 0");
    }
    // load SDWAM .....
    load_sdwam (cpup, cpu.TPR.TSR, nomatch);
  }
  DBGAPP ("doAppendCycleOperandRMW(A) R1 %o R2 %o R3 %o E %o\r\n", cpu.SDW->R1, cpu.SDW->R2, cpu.SDW->R3, cpu.SDW->E);

  // Yes...
  cpu.RSDWH_R1 = cpu.SDW->R1;

////////////////////////////////////////
//
// Sheet 3: "B"
//
////////////////////////////////////////

//
// B: Check the ring
//

  DBGAPP ("doAppendCycleOperandRMW(B)\r\n");

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
    sim_warn ("%s: lastCycle == RTCD_OPERAND_FETCH opcode %0#o\r\n", __func__, i->opcode);

  //
  // B1: The operand is one of: an instruction, data to be read or data to be
  //     written
  //

  //
  // check read bracket for read access
  //

  DBGAPP ("doAppendCycleOperandRMW(B):!STR-OP\r\n");

  // No
  // C(TPR.TRR) > C(SDW .R2)?
  if (cpu.TPR.TRR > cpu.SDW->R2) {
    DBGAPP ("ACV3\r\n");
    DBGAPP ("doAppendCycleOperandRMW(B) ACV3\r\n");
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
      DBGAPP ("ACV4\r\n");
      DBGAPP ("doAppendCycleOperandRMW(B) ACV4\r\n");
      //Set fault ACV4 = R-OFF
      cpu.acvFaults |= ACV4;
      PNL (L68_ (cpu.apu.state |= apu_FLT;))
      FMSG (acvFaultsMsg = "acvFaults(B) C(PPR.PSR) = C(TPR.TSR)";)
    //} else {
      // sim_warn ("doAppendCycleOperandRMW(B) SDW->R == 0 && cpu.PPR.PSR == cpu.TPR.TSR: %0#o\r\n", cpu.PPR.PSR);
    }
  }

  //
  // check write bracket for write access
  //
  DBGAPP ("doAppendCycleOperandRMW(B):STR-OP\r\n");

  // isolts 870
  if (cpu.TPR.TSR == cpu.PPR.PSR)
    cpu.TPR.TRR = cpu.PPR.PRR;

  // C(TPR.TRR) > C(SDW .R1)? Note typo in AL39, R2 should be R1
  if (cpu.TPR.TRR > cpu.SDW->R1) {
    DBGAPP ("ACV5 TRR %o R1 %o\r\n", cpu.TPR.TRR, cpu.SDW->R1);
    //Set fault ACV5 = OWB
    cpu.acvFaults |= ACV5;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(B) C(TPR.TRR) > C(SDW .R1)";)
  }

  if (! cpu.SDW->W) {
    // isolts 870
    cpu.TPR.TRR = cpu.PPR.PRR;

    DBGAPP ("ACV6\r\n");
    // Set fault ACV6 = W-OFF
    cpu.acvFaults |= ACV6;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(B) ACV6 = W-OFF";)
  }

  goto G;

////////////////////////////////////////
//
// Sheet 7: "G"
//
////////////////////////////////////////

G:;

  DBGAPP ("doAppendCycleOperandRMW(G)\r\n");

  //C(TPR.CA)0,13 > SDW.BOUND?
  if (((cpu.TPR.CA >> 4) & 037777) > cpu.SDW->BOUND) {
    DBGAPP ("ACV15\r\n");
    DBGAPP ("doAppendCycleOperandRMW(G) ACV15\r\n");
    cpu.acvFaults |= ACV15;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(G) C(TPR.CA)0,13 > SDW.BOUND";)
    DBGAPP ("acvFaults(G) C(TPR.CA)0,13 > SDW.BOUND\r\n" "   CA %06o CA>>4 & 037777 %06o SDW->BOUND %06o",
            cpu.TPR.CA, ((cpu.TPR.CA >> 4) & 037777), cpu.SDW->BOUND);
  }

  if (cpu.acvFaults) {
    DBGAPP ("doAppendCycleOperandRMW(G) acvFaults\r\n");
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    // Initiate an access violation fault
    doFault (FAULT_ACV, (_fault_subtype) {.fault_acv_subtype=cpu.acvFaults}, "ACV fault");
  }

  // is segment C(TPR.TSR) paged?
  if (cpu.SDW->U)
    goto H; // Not paged

  // Yes. segment is paged ...
  // is PTW for C(TPR.CA) in PTWAM?

  DBGAPP ("doAppendCycleOperandRMW(G) CA %06o\r\n", cpu.TPR.CA);
  if (nomatch || ! fetch_ptw_from_ptwam (cpup, cpu.SDW->POINTER, cpu.TPR.CA))  {
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
  if (i->opcodeX && ((i->opcode & 0770)== 0200|| (i->opcode & 0770) == 0220
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
  DBGAPP ("doAppendCycleOperandRMW(H): FANP\r\n");

  PNL (L68_ (cpu.apu.state |= apu_FANP;))
#if 0
  // ISOLTS pa865 test-01a 101232
  if (get_bar_mode ()) {
    set_apu_status (apuStatus_FABS);
  } else
    ....
#endif
  set_apu_status (cpup, apuStatus_FANP);

  DBGAPP ("doAppendCycleOperandRMW(H): SDW->ADDR=%08o CA=%06o\r\n", cpu.SDW->ADDR, cpu.TPR.CA);

  finalAddress = (cpu.SDW->ADDR & 077777760) + cpu.TPR.CA;
  finalAddress &= 0xffffff;
  PNL (cpu.APUMemAddr = finalAddress;)

  DBGAPP ("doAppendCycleOperandRMW(H:FANP): (%05o:%06o) finalAddress=%08o\r\n", cpu.TPR.TSR, cpu.TPR.CA, finalAddress);

  goto HI;

I:;

// Set PTW.M

  DBGAPP ("doAppendCycleOperandRMW(I): FAP\r\n");
  if (cpu.PTW->M == 0)  // is this the right way to do this?
    modify_ptw (cpup, cpu.SDW, cpu.TPR.CA);

  // final address paged
  set_apu_status (cpup, apuStatus_FAP);
  PNL (L68_ (cpu.apu.state |= apu_FAP;))

  word24 y2 = cpu.TPR.CA % 1024;

  // AL39: The hardware ignores low order bits of the main memory page
  // address according to page size
  finalAddress = (((word24)cpu.PTW->ADDR & 0777760) << 6) + y2;
  finalAddress &= 0xffffff;
  PNL (cpu.APUMemAddr = finalAddress;)

  L68_ (
    if (cpu.MR_cache.emr && cpu.MR_cache.ihr)
      add_l68_APU_history (cpup, APUH_FAP);
  )

  DBGAPP ("doAppendCycleOperandRMW(H:FAP): (%05o:%06o) finalAddress=%08o\r\n", cpu.TPR.TSR, cpu.TPR.CA, finalAddress);

  goto HI;

HI:
  DBGAPP ("doAppendCycleOperandRMW(HI)\r\n");

  // isolts 870
  cpu.cu.XSF = 1;
  sim_debug (DBG_TRACEEXT, & cpu_dev, "loading of cpu.TPR.TSR sets XSF to 1\r\n");

  if (nWords == 1) {
    core_read_lock (cpup, finalAddress, data, "OPERAND_RMW");
  } else {
    sim_warn("doAppendCycleOperandRMW: RMW nWords %d != 1\r\n", nWords);
    core_readN (cpup, finalAddress, data, nWords, "OPERAND_RMW");
  }

////////////////////////////////////////
//
// Sheet 11: "O", "P"
//
////////////////////////////////////////

//Exit:;

  PNL (cpu.APUDataBusOffset = cpu.TPR.CA;)
  PNL (cpu.APUDataBusAddr = finalAddress;)

  PNL (L68_ (cpu.apu.state |= apu_FA;))

  DBGAPP ("doAppendCycleOperandRMW (Exit) PRR %o PSR %05o P %o IC %06o\r\n", cpu.PPR.PRR, cpu.PPR.PSR, cpu.PPR.P, cpu.PPR.IC);
  DBGAPP ("doAppendCycleOperandRMW (Exit) TRR %o TSR %05o TBR %02o CA %06o\r\n", cpu.TPR.TRR, cpu.TPR.TSR, cpu.TPR.TBR, cpu.TPR.CA);

  return finalAddress;
}
