/*
 Copyright (c) 2007-2013 Michael Mondy
 Copyright 2012-2016 by Harry Reed
 Copyright 2013-2018 by Charles Anthony

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

/**
 * \file dps8_addrmods.c
 * \project dps8
 * \author Harry Reed
 * \date 9/25/12
 * \copyright Copyright (c) 2012 Harry Reed. All rights reserved.
 */

#include <stdio.h>
#include "dps8.h"
#include "dps8_addrmods.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_append.h"
#include "dps8_ins.h"
#include "dps8_iefp.h"
#include "dps8_opcodetable.h"
#include "dps8_utils.h"
#if defined(THREADZ) || defined(LOCKLESS)
#include "threadz.h"
#endif

#define DBG_CTR cpu.cycleCnt

// Computed Address Formation Flowcharts

//
// return contents of register indicated by Td
//

static word18 get_Cr (word4 Tdes)
  {
    cpu.ou.directOperandFlag = false;

    if (Tdes == 0)
      return 0;

    if (Tdes & 010) // Xn
      return cpu.rX [X (Tdes)];

    switch (Tdes)
      {
        case TD_N:  // rY = address from opcode
          return 0;

        case TD_AU: // rY + C(A)0,17
          return GETHI (cpu.rA);

        case TD_QU: // rY + C(Q)0,17
          return GETHI (cpu.rQ);

        case TD_DU: // none; operand has the form y || (00...0)18
          cpu.ou.directOperand = 0;
          SETHI (cpu.ou.directOperand, cpu.rY);
          cpu.ou.directOperandFlag = true;

          sim_debug (DBG_ADDRMOD, & cpu_dev,
                    "%s(TD_DU): rY=%06o directOperand=%012"PRIo64"\n",
                    __func__, cpu.rY, cpu.ou.directOperand);

          return 0;

        case TD_IC: // rY + C (PPR.IC)
            return cpu.PPR.IC;

        case TD_AL: // rY + C(A)18,35
            return GETLO (cpu.rA);

        case TD_QL: // rY + C(Q)18,35
            return GETLO (cpu.rQ);

        case TD_DL: // none; operand has the form (00...0)18 || y
            cpu.ou.directOperand = 0;
            SETLO (cpu.ou.directOperand, cpu.rY);
            cpu.ou.directOperandFlag = true;

            sim_debug (DBG_ADDRMOD, & cpu_dev,
                       "%s(TD_DL): rY=%06o directOperand=%012"PRIo64"\n",
                       __func__, cpu.rY, cpu.ou.directOperand);

            return 0;
      }
    return 0;
  }

static char * op_desc_str (char * temp)
  {
    DCDstruct * i = & cpu.currentInstruction;

    strcpy (temp, "");

    if (READOP (i))
      {
        switch (operand_size ())
          {
            case 1:
              strcat (temp, "readCY");
              break;

            case 2:
              strcat (temp, "readCYpair2");
              break;

            case 8:
              strcat (temp, "readCYblock8");
              break;

            case 16:
              strcat (temp, "readCYblock16");
              break;

            case 32:
              strcat (temp, "readCYblock32");
              break;
          }
      }

    if (WRITEOP (i))
      {
        if (strlen (temp))
          strcat (temp, "/");

        switch (operand_size ())
          {
            case 1:
              strcat (temp, "writeCY");
              break;

            case 2:
              strcat (temp, "writeCYpair2");
              break;

            case 8:
              strcat (temp, "writeCYblock8");
              break;

            case 16:
              strcat (temp, "writeCYblock16");
              break;

            case 32:
              strcat (temp, "writeCYblock32");
              break;
          }
      }

    if (TRANSOP (i))
      {
        if (strlen (temp))
          strcat (temp, "/");

        strcat (temp, "prepareCA (TRA)");
      }

    if (! READOP (i) && ! WRITEOP (i) && ! TRANSOP (i) &&
        i -> info -> flags & PREPARE_CA)
      {
        if (strlen (temp))
          strcat (temp, "/");

        strcat (temp, "prepareCA");
      }

    return temp;    //"op_desc_str (???)";
  }

bool b29clr;

static void do_ITP (word4 Tag)
  {
    sim_debug (DBG_APPENDING, & cpu_dev,
               "ITP Pair: PRNUM=%o BITNO=%o WORDNO=%o MOD=%o TAG=%o\n",
               GET_ITP_PRNUM (cpu.itxPair), GET_ITP_WORDNO (cpu.itxPair),
               GET_ITP_BITNO (cpu.itxPair), GET_ITP_MOD (cpu.itxPair), Tag);
    //
    // For n = C(ITP.PRNUM):
    // C(PRn.SNR) -> C(TPR.TSR)
    // maximum of ( C(PRn.RNR), C(SDW.R1), C(TPR.TRR) ) -> C(TPR.TRR)
    // C(ITP.BITNO) -> C(TPR.TBR)
    // C(PRn.WORDNO) + C(ITP.WORDNO) + C(r) -> C(TPR.CA)

    // Notes:
    // 1. r = C(CT-HOLD) if the instruction word or preceding indirect word
    //    specified indirect then register modification, or
    // 2. r = C(ITP.MOD.Td) if the instruction word or preceding indirect word
    //   specified register then indirect modification and ITP.MOD.Tm specifies
    //    either register or register then indirect modification.
    // 3. SDW.R1 is the upper limit of the read/write ring bracket for the
    // segment C(TPR.TSR) (see Section 8).
    //

    word3 n = GET_ITP_PRNUM (cpu.itxPair);
    CPTUR (cptUsePRn + n);
    cpu.TPR.TSR = cpu.PR[n].SNR;
    cpu.TPR.TRR = max3 (cpu.PR[n].RNR, cpu.RSDWH_R1, cpu.TPR.TRR);
    cpu.TPR.TBR = GET_ITP_BITNO (cpu.itxPair);
    cpu.TPR.CA = cpu.PAR[n].WORDNO + GET_ITP_WORDNO (cpu.itxPair);
    //if (GET_TM (Tag) == TM_IR || (cpu.switches.fault_tag_indirect && Tag == 020))
    if (GET_TM (Tag) == TM_IR && Tag != 060)
      {
        cpu.TPR.CA += get_Cr(GET_TD(cpu.cu.CT_HOLD));
        sim_debug (DBG_APPENDING, & cpu_dev, "ITP sets CA to %06o\n", cpu.TPR.CA);
        //if (b29clr)
          {
            sim_debug (DBG_ADDRMOD, & cpu_dev, "ITP clears CT_HOLD\n");
            cpu.cu.CT_HOLD = 0;
          }
      }
    else if (GET_TM (Tag) == TM_RI)
      {
        if (GET_ITP_MOD (cpu.itxPair) == TM_R || GET_ITP_MOD (cpu.itxPair) == TM_RI)
            cpu.TPR.CA += get_Cr(GET_TAG (GET_ITP_MOD (cpu.itxPair)));
      }

    cpu.TPR.CA &= AMASK;
    cpu.rY = cpu.TPR.CA;

    cpu.rTAG = GET_ITP_MOD (cpu.itxPair);

    cpu.cu.itp = 1;
    cpu.cu.TSN_PRNO[0] = n;
    cpu.cu.TSN_VALID[0] = 1;

    return;
  }

static void do_ITS (word4 Tag)
  {
    sim_debug (DBG_APPENDING, & cpu_dev,
               "ITS Pair: SEGNO=%o RN=%o WORDNO=%o BITNO=%o MOD=%o TAG=%o\n",
               GET_ITS_SEGNO (cpu.itxPair), GET_ITS_RN (cpu.itxPair),
               GET_ITS_WORDNO (cpu.itxPair), GET_ITS_BITNO (cpu.itxPair),
               GET_ITS_MOD (cpu.itxPair), Tag);
    // C(ITS.SEGNO) -> C(TPR.TSR)
    // maximum of ( C(ITS.RN), C(SDW.R1), C(TPR.TRR) ) -> C(TPR.TRR)
    // C(ITS.BITNO) -> C(TPR.TBR)
    // C(ITS.WORDNO) + C(r) -> C(TPR.CA)
    //
    // 1. r = C(CT-HOLD) if the instruction word or preceding indirect word
    //    specified indirect then register modification, or
    // 2. r = C(ITS.MOD.Td) if the instruction word or preceding indirect word
    //    specified register then indirect modification and ITS.MOD.Tm specifies
    //    either register or register then indirect modification.
    // 3. SDW.R1 is the upper limit of the read/write ring bracket for the
    //    segment C(TPR.TSR) (see Section 8).

    cpu.TPR.TSR = GET_ITS_SEGNO (cpu.itxPair);

    sim_debug (DBG_APPENDING, & cpu_dev,
               "ITS Pair Ring: RN %o RSDWH_R1 %o TRR %o max %o\n",
               GET_ITS_RN (cpu.itxPair), cpu.RSDWH_R1, cpu.TPR.TRR,
               max3 (GET_ITS_RN (cpu.itxPair), cpu.RSDWH_R1, cpu.TPR.TRR));

    cpu.TPR.TRR = max3 (GET_ITS_RN (cpu.itxPair), cpu.RSDWH_R1, cpu.TPR.TRR);
    cpu.TPR.TBR = GET_ITS_BITNO (cpu.itxPair);
    cpu.TPR.CA = GET_ITS_WORDNO (cpu.itxPair);
    //if (GET_TM (Tag) == TM_IR || (cpu.switches.fault_tag_indirect && Tag == 020))
    if (GET_TM (Tag) == TM_IR && Tag != 060)
      {
        cpu.TPR.CA += get_Cr(GET_TD(cpu.cu.CT_HOLD));
        sim_debug (DBG_APPENDING, & cpu_dev, "ITS sets CA to %06o\n", cpu.TPR.CA);
        //if (b29clr)
          {
            sim_debug (DBG_ADDRMOD, & cpu_dev, "ITS clears CT_HOLD\n");
            cpu.cu.CT_HOLD = 0;
          }
      }
    else if (GET_TM (Tag) == TM_RI)
      {
        if (GET_ITS_MOD (cpu.itxPair) == TM_R || GET_ITS_MOD (cpu.itxPair) == TM_RI)
            cpu.TPR.CA += get_Cr(GET_TD (GET_ITS_MOD (cpu.itxPair)));
      }

    cpu.TPR.CA &= AMASK;

    cpu.rY = cpu.TPR.CA;

    cpu.rTAG = GET_ITS_MOD (cpu.itxPair);

    cpu.cu.its = 1;

    return;
  }


// CANFAULT
static void do_ITS_ITP (word6 Tag, word6 * newtag)
  {
    word6 ind_tag = GET_TAG (cpu.itxPair [0]);

    sim_debug (DBG_APPENDING, & cpu_dev,
               "do_ITS/ITP: %012"PRIo64" %012"PRIo64" Tag:%o\n",
               cpu.itxPair[0], cpu.itxPair[1], Tag);

    if (! ((GET_TM (Tag) == TM_IR || GET_TM (Tag) == TM_RI) &&
           (ISITP (cpu.itxPair[0]) || ISITS (cpu.itxPair[0]))))
      {
        sim_debug (DBG_APPENDING, & cpu_dev, "do_ITS_ITP: faulting\n");
        doFault (FAULT_IPR, fst_ill_mod, "Incorrect address modifier");
      }

    // Whenever the processor is forming a virtual address two special address
    // modifiers may be specified and are effective under certain restrictive
    // conditions. The special address modifiers are shown in Table 6-4 and
    // discussed in the paragraphs below.
    //
    //  The conditions for which the special address modifiers are effective
    //  are as follows:
    //
    // 1. The instruction word (or preceding indirect word) must specify
    // indirect then register or register then indirect modification.
    //
    // 2. The computed address for the indirect word must be even.
    //
    // If these conditions are satisfied, the processor examines the indirect
    // word TAG field for the special address modifiers.
    //
    // If either condition is violated, the indirect word TAG field is
    // interpreted as a normal address modifier and the presence of a special
    // address modifier will cause an illegal procedure, illegal modifier,
    // fault.


    if (ISITS (ind_tag))
        do_ITS (Tag);
    else
        do_ITP (Tag);

    * newtag = GET_TAG (cpu.itxPair [1]);
    //set_went_appending ();
    cpu.cu.XSF = 1;
    sim_debug (DBG_APPENDING, & cpu_dev, "do_ITS_ITP sets XSF to 1\n");
  }


void updateIWB (word18 addr, word6 tag)
  {
    word36 * wb;
    if (USE_IRODD)
      wb = & cpu.cu.IRODD;
    else
      wb = & cpu.cu.IWB;
    sim_debug (DBG_ADDRMOD, & cpu_dev,
               "updateIWB: IWB was %012"PRIo64" %06o %s\n",
               * wb, GET_ADDR (* wb),
               extMods [GET_TAG (* wb)].mod);

    putbits36_18 (wb,  0, addr);
    putbits36_6 (wb, 30, tag);
    putbits36_1 (wb, 29,  0);

    sim_debug (DBG_ADDRMOD, & cpu_dev,
               "updateIWB: IWB now %012"PRIo64" %06o %s\n",
               * wb, GET_ADDR (* wb),
               extMods [GET_TAG (* wb)].mod);

    decode_instruction (IWB_IRODD, & cpu.currentInstruction);
  }

//
// Input:
//   cu.IWB
//   currentInstruction
//   TPR.TSR
//   TPR.TRR
//   TPR.TBR // XXX check to see if this is initialized
//   TPR.CA
//
//  Output:
//   TPR.CA
//   directOperandFlag
//
// CANFAULT

void do_caf (void)
  {
    if (cpu.currentInstruction.b29 == 0)
      {
        cpu.TPR.CA = GET_ADDR (IWB_IRODD);
      }
    else
      {
        word3 n = GET_PRN(IWB_IRODD);  // get PRn
        word15 offset = GET_OFFSET(IWB_IRODD);
        cpu.TPR.CA = (cpu.PAR[n].WORDNO + SIGNEXT15_18 (offset))
                      & MASK18;
      }

    char buf [256];
    sim_debug (DBG_ADDRMOD, & cpu_dev,
               "%s(Entry): operType:%s TPR.CA=%06o\n",
                __func__, op_desc_str (buf), cpu.TPR.CA);
    sim_debug (DBG_ADDRMOD, & cpu_dev,
               "%s(Entry): CT_HOLD %o\n",
                __func__, cpu.cu.CT_HOLD);

    DCDstruct * i = & cpu.currentInstruction;

    word6 Tm = 0;
    word6 Td = 0;

    word6 iTAG;   // tag of word preceeding an indirect fetch

    cpu.ou.directOperandFlag = false;

    if (i -> info -> flags & NO_TAG) // for instructions line STCA/STCQ
      cpu.rTAG = 0;
    else
      cpu.rTAG = GET_TAG (IWB_IRODD);

    int lockupCnt = 0;
#define lockupLimit 4096 // approx. 2 ms

    if (cpu.instr_restart)
      {
       if (cpu.cu.CT_HOLD == (TM_IT | IT_DIC) &&
           cpu.cu.pot == 1 && GET_ADDR (IWB_IRODD) == cpu.TPR.CA)
         {
           cpu.TPR.CA--;
	 cpu.TPR.CA &= MASK18;
           sim_debug (DBG_ADDRMOD, & cpu_dev, "correct CA\n");
         }
      }

startCA:;

    sim_debug (DBG_ADDRMOD, & cpu_dev,
               "startCA: TAG=%02o(%s) CT_HOLD %02o CA:%06o\n",
               cpu.rTAG, get_mod_string (buf, cpu.rTAG), cpu.cu.CT_HOLD, cpu.TPR.CA);

    // Try to get ISOLTS 885 POT usage right
    if (! cpu.instr_restart)
      {
        cpu.cu.pot = 0;
        cpu.cu.its = 0;
        cpu.cu.itp = 0;
      }

    if (++ lockupCnt > lockupLimit)
      {
        doFault (FAULT_LUF, fst_zero, "Lockup in addrmod");
      }

    Td = GET_TD (cpu.rTAG);
    Tm = GET_TM (cpu.rTAG);

    // Try to get ISOLTS 885 POT usage right
    if (! cpu.instr_restart)
      {
        cpu.cu.pot = 0;
      }

    switch (Tm)
      {
        case TM_R:
          goto R_MOD;
        case TM_RI:
          goto RI_MOD;
        case TM_IT:
          goto IT_MOD;
        case TM_IR:
          goto IR_MOD;
        default:
          break;
      }

    sim_printf ("%s(startCA): unknown Tm??? %o\n",
                __func__, GET_TM (cpu.rTAG));
    sim_warn ("(startCA): unknown Tmi; can't happen!\n");
    return;


    // Register modification. Fig 6-3

    R_MOD:;
      {
        if (Td == 0) // TPR.CA = address from opcode
          {
            //updateIWB (identity) // known that Td is 0.
            return;
          }

        word18 Cr = get_Cr (Td);

        sim_debug (DBG_ADDRMOD, & cpu_dev, "R_MOD: Cr=%06o\n", Cr);

        if (cpu.ou.directOperandFlag)
          {
            sim_debug (DBG_ADDRMOD, & cpu_dev,
                       "R_MOD: directOperand = %012"PRIo64"\n",
                       cpu.ou.directOperand);
            return;
          }

        // For the case of RPT/RPD, the instruction decoder has
        // verified that Tm is R or RI, and Td is X1..X7.
        if (cpu.cu.rpt || cpu.cu.rd | cpu.cu.rl)
          {
            if (cpu.currentInstruction.b29)
              {
                word3 PRn = GET_PRN(IWB_IRODD);
                CPTUR (cptUsePRn + PRn);
                cpu.TPR.CA = (Cr & MASK15) + cpu.PR [PRn].WORDNO;
                cpu.TPR.CA &= AMASK;
              }
            else
              {
                cpu.TPR.CA = Cr;
              }
          }
        else
          {
            cpu.TPR.CA += Cr;
            cpu.TPR.CA &= MASK18;   // keep to 18-bits
          }
        sim_debug (DBG_ADDRMOD, & cpu_dev, "R_MOD: TPR.CA=%06o\n",
                   cpu.TPR.CA);

        return;
      } // R_MOD


    // Figure 6-4. Register Then Indirect Modification Flowchart

    RI_MOD:;
      {
        sim_debug (DBG_ADDRMOD, & cpu_dev, "RI_MOD: Td=%o\n", Td);

        if (Td == TD_DU || Td == TD_DL)
          doFault (FAULT_IPR, fst_ill_mod,
                   "RI_MOD: Td == TD_DU || Td == TD_DL");

        if (Td != 0)
          {
            word18 Cr = get_Cr (Td);  // C(r)

            // We don''t need to worry about direct operand here, since du
            // and dl are disallowed above

            sim_debug (DBG_ADDRMOD, & cpu_dev,
                       "RI_MOD: Cr=%06o CA(Before)=%06o\n", Cr, cpu.TPR.CA);

            if (cpu.cu.rpt || cpu.cu.rd || cpu.cu.rl)
              {
                 word6 Td_ = GET_TD (i -> tag);
                 uint Xn = X (Td_);  // Get Xn of next instruction
                 cpu.TPR.CA = cpu.rX [Xn];
              }
            else
              {
                cpu.TPR.CA += Cr;
                cpu.TPR.CA &= MASK18;   // keep to 18-bits
              }
            sim_debug (DBG_ADDRMOD, & cpu_dev,
                       "RI_MOD: CA(After)=%06o\n", cpu.TPR.CA);
          }

        // If the indirect word faults, on restart the CA will be the post
        // register modification value, so we want to prevent it from 
        // happening again on restart

        // in case it turns out to be a ITS/ITP
        iTAG = cpu.rTAG;

        word18 saveCA = cpu.TPR.CA;
        ReadIndirect ();

        // "In the case of RI modification, only one indirect reference is made
        // per repeated execution. The TAG field of the indirect word is not
        // interpreted.  The indirect word is treated as though it had R
        // modification with R = N."

        if (cpu.cu.rpt || cpu.cu.rd || cpu.cu.rl)
          {
             cpu.itxPair[0] &= ~ INST_M_TAG;
             cpu.itxPair[0] |= TM_R | GET_TD (iTAG);
           }

        // (Closed) Ticket 15: Check for fault causing tags before updating
        // the IWB, so the instruction restart will reload the offending
        // indirect word.

        if (GET_TM (GET_TAG (cpu.itxPair[0])) == TM_IT)
          {
            if (GET_TD (GET_TAG (cpu.itxPair[0])) == IT_F2)
              {
                doFault (FAULT_F2, fst_zero, "RI_MOD: IT_F2 (0)");
              }
            if (GET_TD (GET_TAG (cpu.itxPair[0])) == IT_F3)
              {
                doFault (FAULT_F3, fst_zero, "RI_MOD: IT_F3");
              }
          }

        if ((saveCA & 1) == 1 && (ISITP (cpu.itxPair[0]) || ISITS (cpu.itxPair[0])))
            {
              sim_warn ("%s: itp/its at odd address\n", __func__);
#ifdef TESTING
              traceInstruction (0);
#endif
            }

          if ((saveCA & 1) == 0 && (ISITP (cpu.itxPair[0]) || ISITS (cpu.itxPair[0])))
          {
            do_ITS_ITP (iTAG, & cpu.rTAG);
          }
        else
          {
            cpu.TPR.CA = GETHI (cpu.itxPair[0]);
            cpu.rTAG = GET_TAG (cpu.itxPair[0]);
            cpu.rY = cpu.TPR.CA;
          }

        sim_debug (DBG_ADDRMOD, & cpu_dev,
                   "RI_MOD: cpu.itxPair[0]=%012"PRIo64
                   " TPR.CA=%06o rTAG=%02o\n",
                   cpu.itxPair[0], cpu.TPR.CA, cpu.rTAG);
        // If repeat, the indirection chain is limited, so it is not needed
        // to clear the tag; the delta code later on needs the tag to know
        // which X register to update
        if (cpu.cu.rpt || cpu.cu.rd || cpu.cu.rl)
          return;

        sim_debug (DBG_ADDRMOD, & cpu_dev, "RI_MOD updateIWB\n");
        updateIWB (cpu.TPR.CA, cpu.rTAG);
        goto startCA;
      } // RI_MOD

    // Figure 6-5. Indirect Then Register Modification Flowchart
    IR_MOD:;
      {
        sim_debug (DBG_ADDRMOD, & cpu_dev, "IR_MOD\n");

// ISOLTS 885: The indirect read may fault; CT-HOLD must be
// left alone on fault as the restart comes in at the top of CAF.
// Instead of updating CT-HOLD at IR_MOD_1, as in the AL39 flowchart,
// create a stash which will be copied in after the indirection.

        IR_MOD_1:;

        // Set up the stash
        // cpu.cu.CT_HOLD = Td;
        // ISOLTS 885 expects the entire tag, not just Td as indicated
        // by AL39.
        word6 hold_to_be = cpu.rTAG;
        sim_debug (DBG_ADDRMOD, & cpu_dev,
                   "IR_MOD: CT_HOLD will be %o\n", hold_to_be);

        IR_MOD_2:;

        if (++ lockupCnt > lockupLimit)
          {
            doFault (FAULT_LUF, fst_zero, "Lockup in addrmod IR mode");
          }

        
        sim_debug (DBG_ADDRMOD, & cpu_dev,
                   "IR_MOD: fetching indirect word from %06o\n",
                   cpu.TPR.CA);

        // in case it turns out to be a ITS/ITP
        iTAG = cpu.rTAG;

        word18 saveCA = cpu.TPR.CA;
        ReadIndirect ();

        // Update CT-HOLD it the indirect did not fault
        cpu.cu.CT_HOLD = hold_to_be;
        sim_debug (DBG_ADDRMOD, & cpu_dev, "IR_MOD sets CT_HOLD to %02o\n", cpu.cu.CT_HOLD);

        if ((saveCA & 1) == 1 && (ISITP (cpu.itxPair[0]) || ISITS (cpu.itxPair[0])))
            {
              sim_warn ("%s: itp/its at odd address\n", __func__);
#ifdef TESTING
              traceInstruction (0);
#endif
            }

          if ((saveCA & 1) == 0 && (ISITP (cpu.itxPair[0]) || ISITS (cpu.itxPair[0])))
          {
            do_ITS_ITP (iTAG, & cpu.rTAG);
          }
        else
          {
            cpu.TPR.CA = GETHI (cpu.itxPair[0]);
            cpu.rY = cpu.TPR.CA;
            cpu.rTAG = GET_TAG (cpu.itxPair[0]);
          }

        Td = GET_TD (cpu.rTAG);
        Tm = GET_TM (cpu.rTAG);

        sim_debug (DBG_ADDRMOD, & cpu_dev,
                   "IR_MOD1: cpu.itxPair[0]=%012"PRIo64
                   " TPR.CA=%06o Tm=%o Td=%02o (%s)\n",
                   cpu.itxPair[0], cpu.TPR.CA, Tm, Td,
                   get_mod_string (buf, GET_TAG (cpu.itxPair[0])));

        switch (Tm)
          {
            case TM_IT:
              {
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IR_MOD(TM_IT): Td=%02o => %02o\n",
                           Td, cpu.cu.CT_HOLD);

                if (Td == IT_F2 || Td == IT_F3)
                  {
                    // Abort. FT2 or 3
                    switch (Td)
                      {
                        case IT_F2:
                          cpu.TPR.CA = saveCA;
                          doFault (FAULT_F2, fst_zero, "TM_IT: IT_F2 (1)"); 

                        case IT_F3:
                          cpu.TPR.CA = saveCA;
                          doFault (FAULT_F3, fst_zero, "TM_IT: IT_F3");
                      }
                  }
                // fall through to TM_R
                } // TM_IT

            case TM_R:
              {
                word18 Cr = get_Cr (GET_TD (cpu.cu.CT_HOLD));

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IR_MOD(TM_R): CT_HOLD %o Cr=%06o\n",
                           GET_TD (cpu.cu.CT_HOLD), Cr);

                if (cpu.ou.directOperandFlag)
                  {
                    sim_debug (DBG_ADDRMOD, & cpu_dev,
                               "IR_MOD(TM_R): CT_HOLD DO %012"PRIo64"\n",
                               cpu.ou.directOperand);
                    // CT_HOLD has *DU or *DL; convert to DU or DL
                    word6 tag = TM_R | GET_TD (cpu.cu.CT_HOLD);
                    sim_debug (DBG_ADDRMOD, & cpu_dev, "IR_MOD(TM_R) direct operand updateIWB\n");
                    updateIWB (cpu.TPR.CA, tag); // Known to be DL or DU
                  }
                else
                  {
                    cpu.TPR.CA += Cr;
                    cpu.TPR.CA &= MASK18;   // keep to 18-bits

                    sim_debug (DBG_ADDRMOD, & cpu_dev,
                               "IR_MOD(TM_R): TPR.CA=%06o\n", cpu.TPR.CA);

                    sim_debug (DBG_ADDRMOD, & cpu_dev, "IR_MOD(TM_R) not direct operand updateIWB\n");
                    updateIWB (cpu.TPR.CA, 0);
                  }
                return;
              } // TM_R

            case TM_RI:
              {
                word18 Cr = get_Cr (Td);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IR_MOD(TM_RI): Td=%o Cr=%06o TPR.CA(Before)=%06o\n",
                           Td, Cr, cpu.TPR.CA);

                if (cpu.ou.directOperandFlag)
                  {
                    // keep to 18-bits
                    cpu.TPR.CA = (word18) cpu.ou.directOperand & MASK18;

                    sim_debug (DBG_ADDRMOD, & cpu_dev,
                               "IR_MOD(TM_RI): DO TPR.CA=%06o\n",
                               cpu.TPR.CA);
                  }
                else
                  {
                    cpu.TPR.CA += Cr;
                    cpu.TPR.CA &= MASK18;   // keep to 18-bits

                    sim_debug (DBG_ADDRMOD, & cpu_dev,
                               "IR_MOD(TM_RI): TPR.CA=%06o\n", cpu.TPR.CA);

                  }

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IR_MOD(TM_RI): TPR.CA(After)=%06o\n",
                           cpu.TPR.CA);

                sim_debug (DBG_ADDRMOD, & cpu_dev, "IR_MOD(TM_RI) updateIWB\n");
                updateIWB (cpu.TPR.CA, cpu.rTAG); // XXX guessing here...
                goto IR_MOD_2;
              } // TM_RI

            case TM_IR:
              {
                sim_debug (DBG_ADDRMOD, & cpu_dev, "IR_MOD(TM_IR) updateIWB\n");
                updateIWB (cpu.TPR.CA, cpu.rTAG); // XXX guessing here...
                goto IR_MOD_1;
              } // TM_IR
          } // Tm

        sim_printf ("%s(IR_MOD): unknown Tm??? %o\n", 
                    __func__, GET_TM (cpu.rTAG));
        return;
      } // IR_MOD

    IT_MOD:;
      {
        //    IT_SD     = 004,
        //    IT_SCR    = 005,
        //    IT_CI     = 010,
        //    IT_I      = 011,
        //    IT_SC     = 012,
        //    IT_AD     = 013,
        //    IT_DI     = 014,
        //    IT_DIC    = 015,
        //    IT_ID     = 016,
        //    IT_IDC    = 017
        word6 idwtag, delta;
        word24 Yi = (word24) -1;

        switch (Td)
          {
            // XXX this is probably wrong. ITS/ITP are not standard addr mods
            case SPEC_ITP:
            case SPEC_ITS:
              {
                doFault(FAULT_IPR, fst_ill_mod, "ITx in IT_MOD)");
              }

            case 2:
              {
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(): illegal procedure, illegal modifier, "
                           "fault Td=%o\n", Td);
                doFault (FAULT_IPR, fst_ill_mod,
                         "IT_MOD(): illegal procedure, illegal modifier, "
                         "fault");
              }

            case IT_F1:
              {
                doFault(FAULT_F1, fst_zero, "IT_MOD: IT_F1");
              }

            case IT_F2:
              {
                doFault(FAULT_F2, fst_zero, "IT_MOD: IT_F2 (2)");
              }

            case IT_F3:
              {
                doFault(FAULT_F3, fst_zero, "IT_MOD: IT_F3");
              }


            case IT_CI:  // Character indirect (Td = 10)
            case IT_SC:  // Sequence character (Td = 12)
            case IT_SCR: // Sequence character reverse (Td = 5)
              {
                // There is complexity with managing page faults and tracking
                // the indirect word address and the operand address.
                //
                // To address this, we force those pages in during PREPARE_CA,
                // so that they won't fault during operand read/write.

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD CI/SC/SCR reading indirect word from %06o\n",
                           cpu.TPR.CA);

                //
                // Get the indirect word
                //

                word36 indword;
                word18 indaddr = cpu.TPR.CA;
                Read (indaddr, & indword, APU_DATA_READ);
#ifdef LOCKLESS
                word24 phys_address = cpu.iefpFinalAddress;
#endif

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD CI/SC/SCR indword=%012"PRIo64"\n", indword);

                //
                // Parse and validate the indirect word
                //

                Yi = GET_ADDR (indword);
                word6 sz = GET_TB (GET_TAG (indword));
                word3 os = GET_CF (GET_TAG (indword));
                word12 tally = GET_TALLY (indword);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD CI/SC/SCR size=%o offset=%o Yi=%06o\n",
                           sz, os, Yi);

                if (sz == TB6 && os > 5)
                  // generate an illegal procedure, illegal modifier fault
                  doFault (FAULT_IPR, fst_ill_mod,
                           "co size == TB6 && offset > 5");

                if (sz == TB9 && os > 3)
                  // generate an illegal procedure, illegal modifier fault
                  doFault (FAULT_IPR, fst_ill_mod,
                           "co size == TB9 && offset > 3");

                // Save data in OU registers for readOperands/writeOperands

                cpu.TPR.CA = Yi;
                cpu.ou.character_address = Yi;
                cpu.ou.characterOperandSize = sz;
                cpu.ou.characterOperandOffset = os;

                // CI uses the address, and SC uses the pre-increment address;
                // but SCR use the post-decrement address
                if (Td == IT_SCR)
                  {
                    // For each reference to the indirect word, the character
                    // counter, cf, is reduced by 1 and the TALLY field is
                    // increased by 1 before the computed address is formed.
                    //
                    // Character count arithmetic is modulo 6 for 6-bit
                    // characters and modulo 4 for 9-bit bytes. If the
                    // character count, cf, underflows to -1, it is reset to 5
                    // for 6-bit characters or to 3 for 9-bit bytes and ADDRESS
                    // is reduced by 1. ADDRESS arithmetic is modulo 2^18.
                    // TALLY arithmetic is modulo 4096.
                    //
                    // If the TALLY field overflows to 0, the tally runout
                    // indicator is set ON, otherwise it is set OFF. The
                    // computed address is the (possibly) decremented value of
                    // the ADDRESS field of the indirect word. The effective
                    // character/byte number is the decremented value of the
                    // character position count, cf, field of the indirect
                    // word.

                    if (os == 0)
                      {
                        if (sz == TB6)
                            os = 5;
                        else
                            os = 3;
                        Yi -= 1;
                        Yi &= MASK18;
                      }
                    else
                      {
                        os -= 1;
                      }

                    CPT (cpt2L, 5); // Update IT Tally; SCR
                    tally ++;
                    tally &= 07777; // keep to 12-bits

                    // Update saved values

                    cpu.TPR.CA = Yi;
                    cpu.ou.character_address = Yi;
                    cpu.ou.characterOperandSize = sz;
                    cpu.ou.characterOperandOffset = os;
                  }


// What if readOperands and/of writeOperands fault? On restart, doCAF will be
// called again and the indirect word would incorrectly be updated a second
// time. 
//
// We don't care about read/write access violations; in general, they are not
// restarted.
//
// We can avoid page faults by preemptively fetching the data word.

                //
                // Get the data word
                //

                cpu.cu.pot = 1;

#ifdef LOCKLESSXXX
                // gives warnings as another lock is aquired in between
                Read (cpu.TPR.CA, & cpu.ou.character_data, (i->info->flags & RMW) == STORE_OPERAND ? OPERAND_RMW : OPERAND_READ);
#else
                Read (cpu.TPR.CA, & cpu.ou.character_data, OPERAND_READ);
#endif
#ifdef LOCKLESS
                cpu.char_word_address = cpu.iefpFinalAddress;
#endif

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD CI/SC/SCR data=%012"PRIo64"\n",
                           cpu.ou.character_data);

                //cpu.cu.pot = 0;

                if (Td == IT_SC)
                  {
                    // For each reference to the indirect word, the character
                    // counter, cf, is increased by 1 and the TALLY field is
                    // reduced by 1 after the computed address is formed.
                    // Character count arithmetic is modulo 6 for 6-bit
                    // characters and modulo 4 for 9-bit bytes. If the
                    // character count, cf, overflows to 6 for 6-bit characters
                    // or to 4 for 9-bit bytes, it is reset to 0 and ADDRESS is
                    // increased by 1. ADDRESS arithmetic is modulo 2^18. TALLY
                    // arithmetic is modulo 4096. If the TALLY field is reduced
                    // to 0, the tally runout indicator is set ON, otherwise it
                    // is set OFF.

                    os ++;

                    if (((sz == TB6) && (os > 5)) ||
                        ((sz == TB9) && (os > 3)))
                      {
                        os = 0;
                        Yi += 1;
                        Yi &= MASK18;
                      }
                    CPT (cpt2L, 6); // Update IT Tally; SC
                    tally --;
                    tally &= 07777; // keep to 12-bits
                  }

                if (Td == IT_SC || Td == IT_SCR)
                  {
                    sim_debug (DBG_ADDRMOD, & cpu_dev,
                                   "update IT tally now %o\n", tally);

                    //word36 new_indword = (word36) (((word36) Yi << 18) |
                    //                    (((word36) tally & 07777) << 6) |
                    //                    cpu.ou.characterOperandSize |
                    //                    cpu.ou.characterOperandOffset);
                    //Write (cpu.TPR.CA,  new_indword, APU_DATA_STORE);
#ifdef LOCKLESS
                    word36 indword_new;
                    core_read_lock(phys_address, &indword_new, __func__);
                    if (indword_new != indword)
                      sim_warn("indword changed from %llo to %llo\n", indword, indword_new);
#endif
                    putbits36_18 (& indword, 0, Yi);
                    putbits36_12 (& indword, 18, tally);
                    putbits36_3  (& indword, 33, os);
#ifdef LOCKLESS
                    core_write_unlock(phys_address, indword, __func__);
#else
                    Write (indaddr, indword, APU_DATA_STORE);
#endif

                    SC_I_TALLY (tally == 0);

                    sim_debug (DBG_ADDRMOD, & cpu_dev,
                               "update IT wrote tally word %012"PRIo64
                               " to %06o\n",
                               indword, cpu.TPR.CA);
                  }



                // readOperand and writeOperand will not use cpu.TPR.CA; they
                // will use the saved address, size, offset and data.
                cpu.TPR.CA = cpu.ou.character_address;
                return;
              } // IT_CI, IT_SC, IT_SCR
   
            case IT_I: // Indirect (Td = 11)
              {
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_I): reading indirect word from %06o\n",
                           cpu.TPR.CA);

                //Read2 (cpu.TPR.CA, cpu.itxPair, INDIRECT_WORD_FETCH);
                ReadIndirect ();

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_I): indword=%012"PRIo64"\n",
                           cpu.itxPair[0]);

                cpu.TPR.CA = GET_ADDR (cpu.itxPair[0]);
                return;
              } // IT_I

            case IT_AD: ///< Add delta (Td = 13)
              {
                // The TAG field of the indirect word is interpreted as a
                // 6-bit, unsigned, positive address increment value, delta.
                // For each reference to the indirect word, the ADDRESS field
                // is increased by delta and the TALLY field is reduced by 1
                // after the computed address is formed. ADDRESS arithmetic is
                // modulo 2^18. TALLY arithmetic is modulo 4096. If the TALLY
                // field is reduced to 0, the tally runout indicator is set ON,
                // otherwise it is set OFF. The computed address is the value
                // of the unmodified ADDRESS field of the indirect word.

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_AD): reading indirect word from %06o\n",
                           cpu.TPR.CA);

#ifdef THREADZ
                lock_rmw ();
#endif

                word18 saveCA = cpu.TPR.CA;
                word36 indword;
                Read (cpu.TPR.CA, & indword, APU_DATA_RMW);

                cpu.AM_tally = GET_TALLY (indword); // 12-bits
                delta = GET_DELTA (indword); // 6-bits
                Yi = GETHI (indword);        // from where data live

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_AD): indword=%012"PRIo64"\n",
                           indword);
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_AD): address:%06o tally:%04o "
                           "delta:%03o\n",
                           Yi, cpu.AM_tally, delta);

                cpu.TPR.CA = Yi;
                word18 computedAddress = cpu.TPR.CA;

                Yi += delta;
                Yi &= MASK18;

                cpu.AM_tally -= 1;
                cpu.AM_tally &= 07777; // keep to 12-bits
                // breaks emacs
                //if (cpu.AM_tally == 0)
                  //SET_I_TALLY;
                SC_I_TALLY (cpu.AM_tally == 0);
                indword = (word36) (((word36) Yi << 18) |
                                    (((word36) cpu.AM_tally & 07777) << 6) |
                                    delta);
#ifdef LOCKLESS
                core_write_unlock(cpu.iefpFinalAddress, indword, __func__);
#else
                Write (saveCA, indword, APU_DATA_STORE);
#endif

#ifdef THREADZ
                unlock_rmw ();
#endif

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_AD): wrote tally word %012"PRIo64
                           " to %06o\n",
                           indword, saveCA);

                cpu.TPR.CA = computedAddress;
                // The address and tally have been updated; change the tag
                // so that if the indirect fetches, the address and tally
                // will be left alone. 
                sim_debug (DBG_ADDRMOD, & cpu_dev, "IT_MOD(IT_AD) updateIWB\n");
                updateIWB (cpu.TPR.CA, 0);
                return;
              } // IT_AD

            case IT_SD: ///< Subtract delta (Td = 4)
              {
                // The TAG field of the indirect word is interpreted as a
                // 6-bit, unsigned, positive address increment value, delta.
                // For each reference to the indirect word, the ADDRESS field
                // is reduced by delta and the TALLY field is increased by 1
                // before the computed address is formed. ADDRESS arithmetic is
                // modulo 2^18. TALLY arithmetic is modulo 4096. If the TALLY
                // field overflows to 0, the tally runout indicator is set ON,
                // otherwise it is set OFF. The computed address is the value
                // of the decremented ADDRESS field of the indirect word.

#ifdef THREADZ
                lock_rmw ();
#endif

                word18 saveCA = cpu.TPR.CA;
                word36 indword;
                Read (cpu.TPR.CA, & indword, APU_DATA_RMW);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SD): reading indirect word from %06o\n",
                           cpu.TPR.CA);
                cpu.AM_tally = GET_TALLY (indword); // 12-bits
                delta = GET_DELTA (indword); // 6-bits
                Yi    = GETHI (indword);     // from where data live

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SD): indword=%012"PRIo64"\n",
                           indword);
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SD): address:%06o tally:%04o "
                           "delta:%03o\n",
                           Yi, cpu.AM_tally, delta);

                Yi -= delta;
                Yi &= MASK18;
                cpu.TPR.CA = Yi;

                cpu.AM_tally += 1;
                cpu.AM_tally &= 07777; // keep to 12-bits
                if (cpu.AM_tally == 0)
                  SET_I_TALLY;

                // write back out indword
                indword = (word36) (((word36) Yi << 18) |
                                    (((word36) cpu.AM_tally & 07777) << 6) |
                                    delta);
#ifdef LOCKLESS
                core_write_unlock(cpu.iefpFinalAddress, indword, __func__);
#else
                Write (saveCA, indword, APU_DATA_STORE);
#endif

#ifdef THREADZ
                unlock_rmw ();
#endif

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SD): wrote tally word %012"PRIo64
                           " to %06o\n",
                           indword, saveCA);


                cpu.TPR.CA = Yi;
                // The address and tally have been updated; change the tag
                // so that if the indirect fetches, the address and tally
                // will be left alone. 
                sim_debug (DBG_ADDRMOD, & cpu_dev, "IT_MOD(IT_SD) updateIWB\n");
                updateIWB (cpu.TPR.CA, 0);
                return;
              } // IT_SD

            case IT_DI: ///< Decrement address, increment tally (Td = 14)
              {
                // For each reference to the indirect word, the ADDRESS field
                // is reduced by 1 and the TALLY field is increased by 1 before
                // the computed address is formed. ADDRESS arithmetic is modulo
                // 2^18. TALLY arithmetic is modulo 4096. If the TALLY field
                // overflows to 0, the tally runout indicator is set ON,
                // otherwise it is set OFF. The TAG field of the indirect word
                // is ignored. The computed address is the value of the
                // decremented ADDRESS field.

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_DI): reading indirect word from %06o\n",
                           cpu.TPR.CA);

#ifdef THREADZ
                lock_rmw ();
#endif

                word18 saveCA = cpu.TPR.CA;
                word36 indword;
                Read (cpu.TPR.CA, & indword, APU_DATA_RMW);

                Yi = GETHI (indword);
                cpu.AM_tally = GET_TALLY (indword); // 12-bits
                word6 junk = GET_TAG (indword); // get tag field, but ignore it

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_DI): indword=%012"PRIo64"\n",
                           indword);
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_DI): address:%06o tally:%04o\n",
                           Yi, cpu.AM_tally);

                Yi -= 1;
                Yi &= MASK18;
                cpu.TPR.CA = Yi;

                cpu.AM_tally += 1;
                cpu.AM_tally &= 07777; // keep to 12-bits
                SC_I_TALLY (cpu.AM_tally == 0);

                // write back out indword

                indword = (word36) (((word36) cpu.TPR.CA << 18) |
                                    ((word36) cpu.AM_tally << 6) |
                                    junk);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_DI): writing indword=%012"PRIo64" to "
                           "addr %06o\n",
                           indword, saveCA);

#ifdef LOCKLESS
                core_write_unlock(cpu.iefpFinalAddress, indword, __func__);
#else
                Write (saveCA, indword, APU_DATA_STORE);
#endif

#ifdef THREADZ
                unlock_rmw ();
#endif

// Confusing... CA is set to Yi above, so this should be redundant, but
// commenting it out causes early boot hang...
                cpu.TPR.CA = Yi;
                // The address and tally have been updated; change the tag
                // so that if the indirect fetches, the address and tally
                // will be left alone. 
                sim_debug (DBG_ADDRMOD, & cpu_dev, "IT_MOD(IT_DI) updateIWB\n");
                updateIWB (cpu.TPR.CA, 0);
                return;
              } // IT_DI

            case IT_ID: ///< Increment address, decrement tally (Td = 16)
              {
                // For each reference to the indirect word, the ADDRESS field
                // is increased by 1 and the TALLY field is reduced by 1 after
                // the computed address is formed. ADDRESS arithmetic is modulo
                // 2^18. TALLY arithmetic is modulo 4096. If the TALLY field is
                // reduced to 0, the tally runout indicator is set ON,
                // otherwise it is set OFF. The TAG field of the indirect word
                // is ignored. The computed address is the value of the
                // unmodified ADDRESS field.

                word18 saveCA = cpu.TPR.CA;

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_ID): fetching indirect word from %06o\n",
                           cpu.TPR.CA);

#ifdef THREADZ
                lock_rmw ();
#endif

                word36 indword;
                Read (cpu.TPR.CA, & indword, APU_DATA_RMW);

                Yi = GETHI (indword);
                cpu.AM_tally = GET_TALLY (indword); // 12-bits
                // get tag field, but ignore it
                word6 junk = GET_TAG (indword);
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_ID): indword=%012"PRIo64
                           " Yi=%06o tally=%04o\n",
                           indword, Yi, cpu.AM_tally);

                cpu.TPR.CA = Yi;
                word18 computedAddress = cpu.TPR.CA;

                Yi += 1;
                Yi &= MASK18;

                cpu.AM_tally -= 1;
                cpu.AM_tally &= 07777; // keep to 12-bits

                // XXX Breaks boot?
                //if (cpu.AM_tally == 0)
                  //SET_I_TALLY;
                SC_I_TALLY (cpu.AM_tally == 0);

                // write back out indword
                indword = (word36) (((word36) Yi << 18) |
                                    ((word36) cpu.AM_tally << 6) |
                                    junk);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_ID): writing indword=%012"PRIo64" to "
                           "addr %06o\n",
                           indword, saveCA);

#ifdef LOCKLESS
                core_write_unlock(cpu.iefpFinalAddress, indword, __func__);
#else
                Write (saveCA, indword, APU_DATA_STORE);
#endif

#ifdef THREADZ
                unlock_rmw ();
#endif

                cpu.TPR.CA = computedAddress;
                return;
              } // IT_ID

            // Decrement address, increment tally, and continue (Td = 15)
            case IT_DIC:
              {
                // The action for this variation is identical to that for the
                // decrement address, increment tally variation except that the
                // TAG field of the indirect word is interpreted and
                // continuation of the indirect chain is possible. If the TAG
                // of the indirect word invokes a register, that is, specifies
                // r, ri, or ir modification, the effective Td value for the
                // register is forced to "null" before the next computed
                // address is formed.

                // a:RJ78/idc1
                // The address and tally fields are used as described under the
                // ID variation. The tag field uses the set of variations for
                // instruction address modification under the following
                // restrictions: no variation is permitted that requires an
                // indexing modification in the DIC cycle since the indexing
                // adder is being used by the tally phase of the operation.
                // Thus, permissible variations are any allowable form of IT or
                // IR, but if RI or R is used, R must equal N (RI and R forced
                // to N).

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_DIC): fetching indirect word from %06o\n",
                           cpu.TPR.CA);

#ifdef THREADZ
                lock_rmw ();
#endif

                word18 saveCA = cpu.TPR.CA;
                word36 indword;
                // ISOLTS 885 test-02a second level
                cpu.cu.pot = 1;
                Read (cpu.TPR.CA, & indword, APU_DATA_RMW);

                //cpu.cu.pot = 0;

                Yi = GETHI (indword);
                cpu.AM_tally = GET_TALLY (indword); // 12-bits
                idwtag = GET_TAG (indword);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_DIC): indword=%012"PRIo64" Yi=%06o "
                           "tally=%04o idwtag=%02o\n", 
                           indword, Yi, cpu.AM_tally, idwtag);

                // ISOLTS 885 test-02a
                word24 YiSafe2 = Yi; // save indirect address for later use

                Yi -= 1;
                Yi &= MASK18;

                cpu.AM_tally += 1;
                cpu.AM_tally &= 07777; // keep to 12-bits
// Set the tally after the indirect word is processed; if it faults, the IR
// should be unchanged. ISOLTS ps791 test-02g
                //SC_I_TALLY (cpu.AM_tally == 0);
                //if (cpu.AM_tally == 0)
                  //SET_I_TALLY;

                // write back out indword
                indword = (word36) (((word36) Yi << 18) |
                          ((word36) cpu.AM_tally << 6) | idwtag);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_DIC): writing indword=%012"PRIo64" to "
                           "addr %06o\n", indword, saveCA);

#ifdef LOCKLESS
                core_write_unlock(cpu.iefpFinalAddress, indword, __func__);
#else
                Write (saveCA, indword, APU_DATA_STORE);
#endif

#ifdef THREADZ
                unlock_rmw ();
#endif
// ISOLTS 791 test-02e claims:
//      specifying   au  as register designator of an indirect word fetched
//      via 'dic' modification should have faulted
//
// The R/RI handling discussed in AL39 ("forces null") actually means fault.

                if ((GET_TM (idwtag) == TM_R || GET_TM (idwtag) == TM_RI) &&
                    GET_TD (idwtag) != 0)
                  {
                    doFault (FAULT_IPR, fst_ill_mod,
                             "IT_MOD(IT_DIC): indirect TM_R");
                  }

                // If the TAG of the indirect word invokes a register, that is,
                // specifies r, ri, or ir modification, the effective Td value
                // for the register is forced to "null" before the next
                // computed address is formed.

                // Thus, permissible variations are any allowable form of IT or
                // IR, but if RI or R is used, R must equal N (RI and R forced
                // to N).
                cpu.TPR.CA = Yi;

                Tm = GET_TM (cpu.rTAG);
                //if (Tm == TM_IR)
                  {
                    sim_debug (DBG_ADDRMOD, & cpu_dev,
                               "IT_MOD(IT_DIC): new CT_HOLD %02o new TAG %02o\n", 
                               cpu.rTAG, idwtag);
                    cpu.cu.CT_HOLD = cpu.rTAG;
                    sim_debug (DBG_ADDRMOD, & cpu_dev, "IT_DIC sets CT_HOLD to %02o\n", cpu.cu.CT_HOLD);
                  }
                cpu.rTAG = idwtag;

                if (Tm == TM_RI || Tm == TM_R)
                  {
                     if (GET_TD (cpu.rTAG) != 0)
                       {
                         doFault (FAULT_IPR, fst_ill_mod,
                                  "DIC Incorrect address modifier");
                       }
                  }

// Set the tally after the indirect word is processed; if it faults, the IR
// should be unchanged. ISOLTS ps791 test-02g
                SC_I_TALLY (cpu.AM_tally == 0);
                sim_debug (DBG_ADDRMOD, & cpu_dev, "IT_MOD(IT_DIC) updateIWB\n");
                // For DIC, the computed address is the modified address
// fails on ISOLTS 885 test-02a 3rd level at DIC to IDC 
                //updateIWB (Yi, cpu.rTAG);
                updateIWB (YiSafe2, cpu.rTAG);
                cpu.TPR.CA = Yi;

                goto startCA;
              } // IT_DIC

            // Increment address, decrement tally, and continue (Td = 17)
            case IT_IDC: 
              {
                // The action for this variation is identical to that for the
                // increment address, decrement tally variation except that the
                // TAG field of the indirect word is interpreted and
                // continuation of the indirect chain is possible. If the TAG
                // of the indirect word invokes a register, that is, specifies
                // r, ri, or ir modification, the effective Td value for the
                // register is forced to "null" before the next computed
                // address is formed.

                // a:RJ78/idc1
                // The address and tally fields are used as described under the
                // ID variation. The tag field uses the set of variations for
                // instruction address modification under the following
                // restrictions: no variation is permitted that requires an
                // indexing modification in the IDC cycle since the indexing
                // adder is being used by the tally phase of the operation.
                // Thus, permissible variations are any allowable form of IT or
                // IR, but if RI or R is used, R must equal N (RI and R forced
                // to N).

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_IDC): fetching indirect word from %06o\n",
                           cpu.TPR.CA);

#ifdef THREADZ
                lock_rmw ();
#endif

                word18 saveCA = cpu.TPR.CA;
                word36 indword;
#if 0
                if (cpu.cu.CT_HOLD == (TM_IT | IT_DI) ||
                    cpu.cu.CT_HOLD == (TM_IT | IT_DIC))
                  {
                    cpu.TPR.CA --;
                    cpu.TPR.CA &= MASK18;
                  }
#endif
                Read (cpu.TPR.CA, & indword, APU_DATA_RMW);

                //cpu.cu.pot = 0;

                Yi = GETHI (indword);
                cpu.AM_tally = GET_TALLY (indword); // 12-bits
                idwtag = GET_TAG (indword);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_IDC): indword=%012"PRIo64" Yi=%06o "
                           "tally=%04o idwtag=%02o\n",
                           indword, Yi, cpu.AM_tally, idwtag);

                word24 YiSafe = Yi; // save indirect address for later use

                Yi += 1;
                Yi &= MASK18;

                cpu.AM_tally -= 1;
                cpu.AM_tally &= 07777; // keep to 12-bits
// Set the tally after the indirect word is processed; if it faults, the IR
// should be unchanged. ISOLTS ps791 test-02f
                //SC_I_TALLY (cpu.AM_tally == 0);

                // write back out indword
                indword = (word36) (((word36) Yi << 18) |
                                    ((word36) cpu.AM_tally << 6) |
                                    idwtag);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_IDC): writing indword=%012"PRIo64""
                           " to addr %06o\n",
                           indword, saveCA);

#ifdef LOCKLESS
                core_write_unlock(cpu.iefpFinalAddress, indword, __func__);
#else 
                Write (saveCA, indword, APU_DATA_STORE);
#endif

#ifdef THREADZ
                unlock_rmw ();
#endif

                // If the TAG of the indirect word invokes a register, that is,
                // specifies r, ri, or ir modification, the effective Td value
                // for the register is forced to "null" before the next
                // computed address is formed.
                // But for the dps88 you can use everything but ir/ri.

                // Thus, permissible variations are any allowable form of IT or
                // IR, but if RI or R is used, R must equal N (RI and R forced
                // to N).
                cpu.TPR.CA = YiSafe;

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_IDC): new CT_HOLD %02o new TAG %02o\n", 
                           cpu.rTAG, idwtag);
// Experiment to fix ISOLTS 885 test-02a level 4 CT-HOLD error...
// Disabling this breaks level 2.
                cpu.cu.CT_HOLD = cpu.rTAG;
                sim_debug (DBG_ADDRMOD, & cpu_dev, "IT_IDC sets CT_HOLD to %02o\n", cpu.cu.CT_HOLD);
                cpu.rTAG = idwtag;

                Tm = GET_TM (cpu.rTAG);
                if (Tm == TM_RI || Tm == TM_R)
                  {
                     if (GET_TD (cpu.rTAG) != 0)
                       {
                         doFault (FAULT_IPR, fst_ill_mod,
                                  "IDC Incorrect address modifier");
                       }
                  }

// Set the tally after the indirect word is processed; if it faults, the IR
// should be unchanged. ISOLTS ps791 test-02f
                SC_I_TALLY (cpu.AM_tally == 0);
                sim_debug (DBG_ADDRMOD, & cpu_dev, "IT_MOD(IT_IDC) updateIWB\n");
                updateIWB (cpu.TPR.CA, cpu.rTAG);

                goto startCA;
              } // IT_IDC
          } // Td
        sim_printf ("IT_MOD/Td how did we get here?\n");
        return;
     } // IT_MOD
  } // do_caf


