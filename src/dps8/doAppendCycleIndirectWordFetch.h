word24 doAppendCycleIndirectWordFetch (word36 * data, uint nWords) {
  DCDstruct * i = & cpu.currentInstruction;
  DBGAPP ("doAppendCycleIndirectWordFetch(Entry) thisCycle=INDIRECT_WORD_FETCH\n");
  DBGAPP ("doAppendCycleIndirectWordFetch(Entry) lastCycle=%s\n", str_pct (cpu.apu.lastCycle));
  DBGAPP ("doAppendCycleIndirectWordFetch(Entry) CA %06o\n", cpu.TPR.CA);
  DBGAPP ("doAppendCycleIndirectWordFetch(Entry) n=%2u\n", nWords);
  DBGAPP ("doAppendCycleIndirectWordFetch(Entry) PPR.PRR=%o PPR.PSR=%05o\n", cpu.PPR.PRR, cpu.PPR.PSR);
  DBGAPP ("doAppendCycleIndirectWordFetch(Entry) TPR.TRR=%o TPR.TSR=%05o\n", cpu.TPR.TRR, cpu.TPR.TSR);

  if (i->b29) {
    DBGAPP ("doAppendCycleIndirectWordFetch(Entry) isb29 PRNO %o\n", GET_PRN (IWB_IRODD));
  }

  bool nomatch = true;
  if (! cpu.switches.disable_wam) {
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
  cpu.apu.lastCycle = INDIRECT_WORD_FETCH;

  DBGAPP ("doAppendCycleIndirectWordFetch(Entry) XSF %o\n", cpu.cu.XSF);

  PNL (L68_ (cpu.apu.state = 0;))

  cpu.RSDWH_R1 = 0;

  cpu.acvFaults = 0;

//#define FMSG(x) x
#define FMSG(x)
  FMSG (char * acvFaultsMsg = "<unknown>";)

  word24 finalAddress = (word24) -1;  // not everything requires a final address

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

  DBGAPP ("doAppendCycleIndirectWordFetch(A)\n");

  // is SDW for C(TPR.TSR) in SDWAM?
  if (nomatch || ! fetch_sdw_from_sdwam (cpu.TPR.TSR)) {
    // No
    DBGAPP ("doAppendCycleIndirectWordFetch(A):SDW for segment %05o not in SDWAM\n", cpu.TPR.TSR);
    DBGAPP ("doAppendCycleIndirectWordFetch(A):DSBR.U=%o\n", cpu.DSBR.U);

    if (cpu.DSBR.U == 0) {
      fetch_dsptw (cpu.TPR.TSR);

      if (! cpu.PTW0.DF)
        doFault (FAULT_DF0 + cpu.PTW0.FC, fst_zero, "doAppendCycleIndirectWordFetch(A): PTW0.F == 0");

      if (! cpu.PTW0.U)
          modify_dsptw (cpu.TPR.TSR);

      fetch_psdw (cpu.TPR.TSR);
    } else
      fetch_nsdw (cpu.TPR.TSR); // load SDW0 from descriptor segment table.

    if (cpu.SDW0.DF == 0) {
      DBGAPP ("doAppendCycleIndirectWordFetch(A): SDW0.F == 0! " "Initiating directed fault\n");
      // initiate a directed fault ...
      doFault (FAULT_DF0 + cpu.SDW0.FC, fst_zero, "SDW0.F == 0");
    }
    // load SDWAM .....
    load_sdwam (cpu.TPR.TSR, nomatch);
  }
  DBGAPP ("doAppendCycleIndirectWordFetch(A) R1 %o R2 %o R3 %o E %o\n", cpu.SDW->R1, cpu.SDW->R2, cpu.SDW->R3, cpu.SDW->E);

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

  DBGAPP ("doAppendCycleIndirectWordFetch(B)\n");

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
  // the opcode is preserved accross faults and only replaced as the
  // INSTRUCTION_FETCH succeeds.
  if (lastCycle == RTCD_OPERAND_FETCH)
    sim_warn ("%s: lastCycle == RTCD_OPERAND_FETCH opcode %0#o\n", __func__, i->opcode);

  //
  // B1: The operand is one of: an instruction, data to be read or data to be
  //     written
  //

  //
  // check read bracket for read access
  //

  DBGAPP ("doAppendCycleIndirectWordFetch(B):!STR-OP\n");

  // No
  // C(TPR.TRR) > C(SDW .R2)?
  if (cpu.TPR.TRR > cpu.SDW->R2) {
    DBGAPP ("ACV3\n");
    DBGAPP ("doAppendCycleIndirectWordFetch(B) ACV3\n");
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
      DBGAPP ("doAppendCycleIndirectWordFetch(B) ACV4\n");
      //Set fault ACV4 = R-OFF
      cpu.acvFaults |= ACV4;
      PNL (L68_ (cpu.apu.state |= apu_FLT;))
      FMSG (acvFaultsMsg = "acvFaults(B) C(PPR.PSR) = C(TPR.TSR)";)
    } else {
      // sim_warn ("doAppendCycleIndirectWordFetch(B) SDW->R == 0 && cpu.PPR.PSR == cpu.TPR.TSR: %0#o\n", cpu.PPR.PSR);
    }
  }

  goto G;

////////////////////////////////////////
//
// Sheet 7: "G"
//
////////////////////////////////////////

G:;

  DBGAPP ("doAppendCycleIndirectWordFetch(G)\n");

  //C(TPR.CA)0,13 > SDW.BOUND?
  if (((cpu.TPR.CA >> 4) & 037777) > cpu.SDW->BOUND) {
    DBGAPP ("ACV15\n");
    DBGAPP ("doAppendCycleIndirectWordFetch(G) ACV15\n");
    cpu.acvFaults |= ACV15;
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    FMSG (acvFaultsMsg = "acvFaults(G) C(TPR.CA)0,13 > SDW.BOUND";)
    DBGAPP ("acvFaults(G) C(TPR.CA)0,13 > SDW.BOUND\n" "   CA %06o CA>>4 & 037777 %06o SDW->BOUND %06o", cpu.TPR.CA, ((cpu.TPR.CA >> 4) & 037777), cpu.SDW->BOUND);
  }

  if (cpu.acvFaults) {
    DBGAPP ("doAppendCycleIndirectWordFetch(G) acvFaults\n");
    PNL (L68_ (cpu.apu.state |= apu_FLT;))
    // Initiate an access violation fault
    doFault (FAULT_ACV, (_fault_subtype) {.fault_acv_subtype=cpu.acvFaults}, "ACV fault");
  }

  // is segment C(TPR.TSR) paged?
  if (cpu.SDW->U)
    goto H; // Not paged

  // Yes. segment is paged ...
  // is PTW for C(TPR.CA) in PTWAM?

  DBGAPP ("doAppendCycleIndirectWordFetch(G) CA %06o\n", cpu.TPR.CA);
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
  DBGAPP ("doAppendCycleIndirectWordFetch(H): FANP\n");

  PNL (L68_ (cpu.apu.state |= apu_FANP;))
#if 0
  // ISOLTS pa865 test-01a 101232
  if (get_bar_mode ()) {
      set_apu_status (apuStatus_FABS);
    }
  else
    ....
#endif
  set_apu_status (apuStatus_FANP);

  DBGAPP ("doAppendCycleIndirectWordFetch(H): SDW->ADDR=%08o CA=%06o \n", cpu.SDW->ADDR, cpu.TPR.CA);

  finalAddress = (cpu.SDW->ADDR & 077777760) + cpu.TPR.CA;
  finalAddress &= 0xffffff;
  PNL (cpu.APUMemAddr = finalAddress;)

  DBGAPP ("doAppendCycleIndirectWordFetch(H:FANP): (%05o:%06o) finalAddress=%08o\n", cpu.TPR.TSR, cpu.TPR.CA, finalAddress);

  goto HI;

I:;

// Set PTW.M

  DBGAPP ("doAppendCycleIndirectWordFetch(I): FAP\n");

    // final address paged
  set_apu_status (apuStatus_FAP);
  PNL (L68_ (cpu.apu.state |= apu_FAP;))

  word24 y2 = cpu.TPR.CA % 1024;

  // AL39: The hardware ignores low order bits of the main memory page
  // address according to page size
  finalAddress = (((word24)cpu.PTW->ADDR & 0777760) << 6) + y2;
  finalAddress &= 0xffffff;
  PNL (cpu.APUMemAddr = finalAddress;)

#ifdef L68
  if (cpu.MR_cache.emr && cpu.MR_cache.ihr)
    add_APU_history (APUH_FAP);
#endif
  DBGAPP ("doAppendCycleIndirectWordFetch(H:FAP): (%05o:%06o) finalAddress=%08o\n", cpu.TPR.TSR, cpu.TPR.CA, finalAddress);

  goto HI;

HI:
  DBGAPP ("doAppendCycleIndirectWordFetch(HI)\n");

  // isolts 870
  cpu.cu.XSF = 1;
  sim_debug (DBG_TRACEEXT, & cpu_dev, "loading of cpu.TPR.TSR sets XSF to 1\n");

  core_readN (finalAddress, data, nWords, "INDIRECT_WORD_FETCH");


////////////////////////////////////////
//
// Sheet 9: "J"
//
////////////////////////////////////////

// Indirect operand fetch

  DBGAPP ("doAppendCycleIndirectWordFetch(J)\n");

  // ri or ir & TPC.CA even?
  word6 tag = GET_TAG (IWB_IRODD);
  if ((GET_TM (tag) == TM_IR || GET_TM (tag) == TM_RI) && (cpu.TPR.CA & 1) == 0) {
    if (ISITS (* data))
      goto O;
    if (ISITP (* data))
      goto P;
  }

  // C(Y) tag == other indirect?
  //   TM_R never indirects
  //   TM_RI always indirects
  //   TM_IR always indirects
  //   TM_IT always indirects
#if 0
  //     IT_CI, IT_SC, IT_SCR -- address is used for tally word
  //     IT_I indirects
  //     IT_AD -- address is used for tally word
  //     IT_SD -- address is used for tally word
  //     IT_DI -- address is used for tally word
  //     IT_ID -- address is used for tally word
  //     IT_DIC -- address is used for tally word
  //     IT_IDC -- address is used for tally word
  static const bool isInd[64] = {
      // 00-15 R_MOD
      false, false, false, false, false, false, false, false,
      false, false, false, false, false, false, false, false,
      // 16-31 RI_MOD
      true, true, true, true, true, true, true, true,
      true, true, true, true, true, true, true, true,
      // 32-47 IT_MOD
      // f1  und    und    und    sd     scr    f2     f3
      false, false, false, false, true,  true,  false, false,
      // ci  i      sc     ad     di     dic    id     idc
      true,  true,  true,  true,  true,  true,  true,  true,
      // 48-63 IR_MOD
      true, true, true, true, true, true, true, true,
      true, true, true, true, true, true, true, true
    };
  if (isInd[(* data) & MASK6])
#else
  if ((* data) & 060)
#endif
  {
    // C(Y)0,17 -> C(IWB)0,17
    // C(Y)30,35 -> C(IWB)30,35
    // 0 -> C(IWB)29
    //updateIWB (GET_ADDR (* data), (* data) & MASK6);
    //cpu.cu.TSN_PRNO[0] = n;
    //cpu.cu.TSN_VALID[0] = 1;

  }

  goto Exit;

////////////////////////////////////////
//
// Sheet 11: "O", "P"
//
////////////////////////////////////////

O:; // ITS, RTCD
  DBGAPP ("doAppendCycleIndirectWordFetch(O)\n");
  word3 its_RNR = GET_ITS_RN (data);
  DBGAPP ("doAppendCycleIndirectWordFetch(O) TRR %o RSDWH.R1 %o ITS.RNR %o\n", cpu.TPR.TRR, cpu.RSDWH_R1, its_RNR);

  // Maximum of
  //  C(Y)18,20;  C(TPR.TRR); C(SDW.R1) -> C(TPR.TRR)
  cpu.TPR.TRR = max3 (its_RNR, cpu.TPR.TRR, cpu.RSDWH_R1);
  DBGAPP ("doAppendCycleIndirectWordFetch(O) Set TRR to %o\n", cpu.TPR.TRR);

  goto Exit;

P:; // ITP

  DBGAPP ("doAppendCycleIndirectWordFetch(P)\n");

  n = GET_ITP_PRNUM (data);
  DBGAPP ("doAppendCycleIndirectWordFetch(P) TRR %o RSDWH.R1 %o PR[n].RNR %o\n", cpu.TPR.TRR, cpu.RSDWH_R1, cpu.PR[n].RNR);

  // Maximum of
  // cpu.PR[n].RNR;  C(TPR.TRR); C(SDW.R1) -> C(TPR.TRR)
  cpu.TPR.TRR = max3 (cpu.PR[n].RNR, cpu.TPR.TRR, cpu.RSDWH_R1);
  DBGAPP ("doAppendCycleIndirectWordFetch(P) Set TRR to %o\n", cpu.TPR.TRR);

  goto Exit;

Exit:;

  PNL (cpu.APUDataBusOffset = cpu.TPR.CA;)
  PNL (cpu.APUDataBusAddr = finalAddress;)

  PNL (L68_ (cpu.apu.state |= apu_FA;))

  DBGAPP ("doAppendCycleIndirectWordFetch (Exit) PRR %o PSR %05o P %o IC %06o\n", cpu.PPR.PRR, cpu.PPR.PSR, cpu.PPR.P, cpu.PPR.IC);
  DBGAPP ("doAppendCycleIndirectWordFetch (Exit) TRR %o TSR %05o TBR %02o CA %06o\n", cpu.TPR.TRR, cpu.TPR.TSR, cpu.TPR.TBR, cpu.TPR.CA);

  return finalAddress;
}
