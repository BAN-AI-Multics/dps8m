word24 do_append_rtcd_operand_fetch (word36 * data, uint nWords)
  {
    DCDstruct * i = & cpu.currentInstruction;
    DBGAPP ("do_append_cycle(Entry) thisCycle=RTCD_OPERAND_FETCH\n");
    DBGAPP ("do_append_cycle(Entry) lastCycle=%s\n",
            str_pct (cpu.apu.lastCycle));
    DBGAPP ("do_append_cycle(Entry) CA %06o\n",
            cpu.TPR.CA);
    DBGAPP ("do_append_cycle(Entry) n=%2u\n",
            nWords);
    DBGAPP ("do_append_cycle(Entry) PPR.PRR=%o PPR.PSR=%05o\n",
            cpu.PPR.PRR, cpu.PPR.PSR);
    DBGAPP ("do_append_cycle(Entry) TPR.TRR=%o TPR.TSR=%05o\n",
            cpu.TPR.TRR, cpu.TPR.TSR);

    if (i->b29)
      {
        DBGAPP ("do_append_cycle(Entry) isb29 PRNO %o\n",
                GET_PRN (IWB_IRODD));
      }

#ifdef WAM
    // AL39: The associative memory is ignored (forced to "no match") during
    // address preparation.
    // lptp,lptr,lsdp,lsdr,sptp,sptr,ssdp,ssdr
    // Unfortunately, ISOLTS doesn't try to execute any of these in append mode.
    // XXX should this be only for OPERAND_READ and OPERAND_STORE?
    bool nomatch = i->opcode10 == 01232 ||
                   i->opcode10 == 01254 ||
                   i->opcode10 == 01154 ||
                   i->opcode10 == 01173 ||
                   i->opcode10 == 00557 ||
                   i->opcode10 == 00257;
#else
    const bool nomatch = true;
#endif

    processor_cycle_type lastCycle = cpu.apu.lastCycle;
    cpu.apu.lastCycle = RTCD_OPERAND_FETCH;

    DBGAPP ("do_append_cycle(Entry) XSF %o\n", cpu.cu.XSF);

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
// Sheet 1: "START APPEND"
//
////////////////////////////////////////

// START APPEND

    // If the rtcd instruction is executed with the processor in absolute
    // mode with bit 29 of the instruction word set OFF and without
    // indirection through an ITP or ITS pair, then:
    //
    //   appending mode is entered for address preparation for the
    //   rtcd operand and is retained if the instruction executes
    //   successfully, and the effective segment number generated for
    //   the SDW fetch and subsequent loading into C(TPR.TSR) is equal
    //   to C(PPR.PSR) and may be undefined in absolute mode, and the
    //   effective ring number loaded into C(TPR.TRR) prior to the SDW
    //   fetch is equal to C(PPR.PRR) (which is 0 in absolute mode)
    //   implying that control is always transferred into ring 0.
    //
    if (/*thisCycle == RTCD_OPERAND_FETCH &&*/
        get_addr_mode() == ABSOLUTE_mode &&
        ! (cpu.cu.XSF || cpu.currentInstruction.b29) /*get_went_appending()*/)
      { 
        cpu.TPR.TSR = 0;
        DBGAPP ("RTCD_OPERAND_FETCH ABSOLUTE mode set TSR %05o TRR %o\n", cpu.TPR.TSR, cpu.TPR.TRR);
      }

    goto A;

////////////////////////////////////////
//
// Sheet 2: "A"
//
////////////////////////////////////////

//
//  A:
//    Get SDW

A:;

    //PNL (cpu.APUMemAddr = address;)
    PNL (cpu.APUMemAddr = cpu.TPR.CA;)

    DBGAPP ("do_append_cycle(A)\n");
    
#ifdef WAM
    // is SDW for C(TPR.TSR) in SDWAM?
    if (nomatch || ! fetch_sdw_from_sdwam (cpu.TPR.TSR))
#endif
      {
        // No
        DBGAPP ("do_append_cycle(A):SDW for segment %05o not in SDWAM\n",
                 cpu.TPR.TSR);
        
        DBGAPP ("do_append_cycle(A):DSBR.U=%o\n",
                cpu.DSBR.U);
        
        if (cpu.DSBR.U == 0)
          {
            fetch_dsptw (cpu.TPR.TSR);
            
            if (! cpu.PTW0.DF)
              doFault (FAULT_DF0 + cpu.PTW0.FC, fst_zero,
                       "do_append_cycle(A): PTW0.F == 0");
            
            if (! cpu.PTW0.U)
              modify_dsptw (cpu.TPR.TSR);
            
            fetch_psdw (cpu.TPR.TSR);
          }
        else
          fetch_nsdw (cpu.TPR.TSR); // load SDW0 from descriptor segment table.
        
        if (cpu.SDW0.DF == 0)
          {
            DBGAPP ("do_append_cycle(A): SDW0.F == 0! "
                    "Initiating directed fault\n");
            // initiate a directed fault ...
            doFault (FAULT_DF0 + cpu.SDW0.FC, fst_zero, "SDW0.F == 0");
          }
        // load SDWAM .....
        load_sdwam (cpu.TPR.TSR, nomatch);
      }
    DBGAPP ("do_append_cycle(A) R1 %o R2 %o R3 %o E %o\n",
            cpu.SDW->R1, cpu.SDW->R2, cpu.SDW->R3, cpu.SDW->E);

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

    DBGAPP ("do_append_cycle(B)\n");

    // check ring bracket consistency
    
    //C(SDW.R1) <= C(SDW.R2) <= C(SDW .R3)?
    if (! (cpu.SDW->R1 <= cpu.SDW->R2 && cpu.SDW->R2 <= cpu.SDW->R3))
      {
        // Set fault ACV0 = IRO
        cpu.acvFaults |= ACV0;
        PNL (L68_ (cpu.apu.state |= apu_FLT;))
        FMSG (acvFaultsMsg = "acvFaults(B) C(SDW.R1) <= C(SDW.R2) <= "
                              "C(SDW .R3)";)
      }

    // lastCycle == RTCD_OPERAND_FETCH
    // if a fault happens between the RTCD_OPERAND_FETCH and the INSTRUCTION_FETCH
    // of the next instruction - this happens about 35 time for just booting  and
    // shutting down multics -- a stored lastCycle is useless.
    // the opcode is preserved accross faults and only replaced as the
    // INSTRUCTION_FETCH succeeds.
    if (lastCycle == RTCD_OPERAND_FETCH)
      sim_warn ("%s: lastCycle == RTCD_OPERAND_FETCH opcode %0#o\n", __func__, i->opcode10);

    //
    // B1: The operand is one of: an instruction, data to be read or data to be
    //     written
    //

    //
    // check read bracket for read access
    //

    DBGAPP ("do_append_cycle(B):!STR-OP\n");
        
    // No
    // C(TPR.TRR) > C(SDW .R2)?
    if (cpu.TPR.TRR > cpu.SDW->R2)
      {
        DBGAPP ("ACV3\n");
        DBGAPP ("do_append_cycle(B) ACV3\n");
        //Set fault ACV3 = ORB
        cpu.acvFaults |= ACV3;
        PNL (L68_ (cpu.apu.state |= apu_FLT;))
        FMSG (acvFaultsMsg = "acvFaults(B) C(TPR.TRR) > C(SDW .R2)";)
      }
        
    if (cpu.SDW->R == 0)
      {
        // isolts 870
        cpu.TPR.TRR = cpu.PPR.PRR;

        //C(PPR.PSR) = C(TPR.TSR)?
        if (cpu.PPR.PSR != cpu.TPR.TSR)
          {
            DBGAPP ("ACV4\n");
            DBGAPP ("do_append_cycle(B) ACV4\n");
            //Set fault ACV4 = R-OFF
            cpu.acvFaults |= ACV4;
            PNL (L68_ (cpu.apu.state |= apu_FLT;))
            FMSG (acvFaultsMsg = "acvFaults(B) C(PPR.PSR) = C(TPR.TSR)";)
          }
        else
          {
            // sim_warn ("do_append_cycle(B) SDW->R == 0 && cpu.PPR.PSR == cpu.TPR.TSR: %0#o\n", cpu.PPR.PSR);
          }
      }

    //goto G;
    
////////////////////////////////////////
//
// Sheet 7: "G"
//
////////////////////////////////////////

//G:;
    
    DBGAPP ("do_append_cycle(G)\n");
    
    //C(TPR.CA)0,13 > SDW.BOUND?
    if (((cpu.TPR.CA >> 4) & 037777) > cpu.SDW->BOUND)
      {
        DBGAPP ("ACV15\n");
        DBGAPP ("do_append_cycle(G) ACV15\n");
        cpu.acvFaults |= ACV15;
        PNL (L68_ (cpu.apu.state |= apu_FLT;))
        FMSG (acvFaultsMsg = "acvFaults(G) C(TPR.CA)0,13 > SDW.BOUND";)
        DBGAPP ("acvFaults(G) C(TPR.CA)0,13 > SDW.BOUND\n"
                "   CA %06o CA>>4 & 037777 %06o SDW->BOUND %06o",
                cpu.TPR.CA, ((cpu.TPR.CA >> 4) & 037777), cpu.SDW->BOUND);
      }
    
    if (cpu.acvFaults)
      {
        DBGAPP ("do_append_cycle(G) acvFaults\n");
        PNL (L68_ (cpu.apu.state |= apu_FLT;))
        // Initiate an access violation fault
        doFault (FAULT_ACV, (_fault_subtype) {.fault_acv_subtype=cpu.acvFaults},
                 "ACV fault");
      }

    // is segment C(TPR.TSR) paged?
    if (cpu.SDW->U)
      goto H; // Not paged
    
    // Yes. segment is paged ...
    // is PTW for C(TPR.CA) in PTWAM?
    
    DBGAPP ("do_append_cycle(G) CA %06o\n", cpu.TPR.CA);
#ifdef WAM
    if (nomatch ||
        ! fetch_ptw_from_ptwam (cpu.SDW->POINTER, cpu.TPR.CA))  //TPR.CA))
#endif
      {
        fetch_ptw (cpu.SDW, cpu.TPR.CA);
        if (! cpu.PTW0.DF)
          {
            // initiate a directed fault
            doFault (FAULT_DF0 + cpu.PTW0.FC, (_fault_subtype) {.bits=0},
                     "PTW0.F == 0");
          }
        loadPTWAM (cpu.SDW->POINTER, cpu.TPR.CA, nomatch); // load PTW0 to PTWAM
      }
    
    // Prepage mode?
    // check for "uninterruptible" EIS instruction
    // ISOLTS-878 02: mvn,cmpn,mvne,ad3d; obviously also
    // ad2/3d,sb2/3d,mp2/3d,dv2/3d
    // DH03 p.8-13: probably also mve,btd,dtb
    if ((i->opcode10 & 01750) == 01200 || // ad2d sb2d mp2d dv2d ad3d sb3d mp3d dv3d
        (i->opcode10 & 01770) == 01020 || // mve mvne
        (i->opcode10 & 01770) == 01300)   // mvn btd cmpn dtb
      {
        do_ptw2 (cpu.SDW, cpu.TPR.CA);
      } 
    goto I;
    
////////////////////////////////////////
//
// Sheet 8: "H", "I"
//
////////////////////////////////////////

H:;
    DBGAPP ("do_append_cycle(H): FANP\n");

    PNL (L68_ (cpu.apu.state |= apu_FANP;))
    set_apu_status (apuStatus_FANP);

    DBGAPP ("do_append_cycle(H): SDW->ADDR=%08o CA=%06o \n",
            cpu.SDW->ADDR, cpu.TPR.CA);

    if (/*thisCycle == RTCD_OPERAND_FETCH &&*/
        get_addr_mode () == ABSOLUTE_mode &&
        ! (cpu.cu.XSF || cpu.currentInstruction.b29) /*get_went_appending ()*/)
      { 
        finalAddress = cpu.TPR.CA;
      }
    else
      {
        finalAddress = (cpu.SDW->ADDR & 077777760) + cpu.TPR.CA;
        finalAddress &= 0xffffff;
      }
    PNL (cpu.APUMemAddr = finalAddress;)
    
    DBGAPP ("do_append_cycle(H:FANP): (%05o:%06o) finalAddress=%08o\n",
            cpu.TPR.TSR, cpu.TPR.CA, finalAddress);
    
    goto HI;
    
I:;

// Set PTW.M

    DBGAPP ("do_append_cycle(I): FAP\n");
    
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
    DBGAPP ("do_append_cycle(H:FAP): (%05o:%06o) finalAddress=%08o\n",
            cpu.TPR.TSR, cpu.TPR.CA, finalAddress);

    goto HI;

HI:
    DBGAPP ("do_append_cycle(HI)\n");

    // isolts 870
    cpu.cu.XSF = 1;
    sim_debug (DBG_TRACEEXT, & cpu_dev, "loading of cpu.TPR.TSR sets XSF to 1\n");

    core_readN (finalAddress, data, nWords, "RTCD_OPERAND_FETCH");

    // goto K;

////////////////////////////////////////
//
// Sheet 10: "K", "L", "M", "N"
//
////////////////////////////////////////

// K:; // RTCD operand fetch
    DBGAPP ("do_append_cycle(K)\n");

    word3 y = GET_ITS_RN (data);

    // C(Y-pair)3,17 -> C(PPR.PSR)
    // We set TSR here; TSR will be copied to PSR at KL
    cpu.TPR.TSR = GET_ITS_SEGNO (data);

    // Maximum of
    // C(Y-pair)18,20; C(TPR.TRR); C(SDW.R1) -> C(PPR.PRR)
    // We set TRR here as well
    cpu.PPR.PRR = cpu.TPR.TRR = max3 (y, cpu.TPR.TRR, cpu.RSDWH_R1);

    // C(Y-pair)36,53 -> C(PPR.IC)
    // We set CA here; copied to IC  at KL
    cpu.TPR.CA = GET_ITS_WORDNO (data);

    // If C(PPR.PRR) = 0 then C(SDW.P) -> C(PPR.P);
    //     otherwise 0 -> C(PPR.P)
    // Done at M

    // goto KL;

// KL:
    DBGAPP ("do_append_cycle(KL)\n");

    // C(TPR.TSR) -> C(PPR.PSR)
    cpu.PPR.PSR = cpu.TPR.TSR;
    // C(TPR.CA) -> C(PPR.IC) 
    cpu.PPR.IC = cpu.TPR.CA;

    // goto M;

// M: // Set P
    DBGAPP ("do_append_cycle(M)\n");

    // C(TPR.TRR) = 0?
    if (cpu.TPR.TRR == 0)
      {
        // C(SDW.P) -> C(PPR.P) 
        cpu.PPR.P = cpu.SDW->P;
      }
    else
      {
        // 0 C(PPR.P)
        cpu.PPR.P = 0;
      }

    // goto Exit; 

// Exit:;

    PNL (cpu.APUDataBusOffset = cpu.TPR.CA;)
    PNL (cpu.APUDataBusAddr = finalAddress;)

    PNL (L68_ (cpu.apu.state |= apu_FA;))

    DBGAPP ("do_append_cycle (Exit) PRR %o PSR %05o P %o IC %06o\n",
            cpu.PPR.PRR, cpu.PPR.PSR, cpu.PPR.P, cpu.PPR.IC);
    DBGAPP ("do_append_cycle (Exit) TRR %o TSR %05o TBR %02o CA %06o\n",
            cpu.TPR.TRR, cpu.TPR.TSR, cpu.TPR.TBR, cpu.TPR.CA);

    return finalAddress;    // or 0 or -1???
  }

