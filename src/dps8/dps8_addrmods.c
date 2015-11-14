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
#include "dps8_cpu.h"
#include "dps8_append.h"
#include "dps8_ins.h"
#include "dps8_utils.h"
#include "dps8_iefp.h"
#include "dps8_faults.h"
#include "dps8_opcodetable.h"

// Computed Address Formation Flowcharts


//
// return contents of register indicated by Td
//

static word18 getCr (word4 Tdes)
  {
    CPU -> directOperandFlag = false;

    if (Tdes == 0)
      return 0;

    if (Tdes & 010) // Xn
      return CPU -> rX [X (Tdes)];

    switch (Tdes)
      {
        case TD_N:  // rY = address from opcode
          return 0;

        case TD_AU: // rY + C(A)0,17
          return GETHI (CPU -> rA);

        case TD_QU: // rY + C(Q)0,17
          return GETHI (CPU -> rQ);

        case TD_DU: // none; operand has the form y || (00...0)18
          CPU -> directOperand = 0;
          SETHI (CPU -> directOperand, CPU -> rY);
          CPU -> directOperandFlag = true;

          sim_debug (DBG_ADDRMOD, & cpu_dev,
                    "getCr(TD_DU): rY=%06o directOperand=%012llo\n",
                    CPU -> rY, CPU -> directOperand);

          return 0;

        case TD_IC: // rY + C (PPR.IC)
            return CPU -> PPR . IC;

        case TD_AL: // rY + C(A)18,35
            return GETLO (CPU -> rA);

        case TD_QL: // rY + C(Q)18,35
            return GETLO (CPU -> rQ);

        case TD_DL: // none; operand has the form (00...0)18 || y
            CPU -> directOperand = 0;
            SETLO (CPU -> directOperand, CPU -> rY);
            CPU -> directOperandFlag = true;

            sim_debug (DBG_ADDRMOD, & cpu_dev,
                       "getCr(TD_DL): rY=%06o directOperand=%012llo\n",
                       CPU -> rY, CPU -> directOperand);

            return 0;
      }
    return 0;
  }

#ifndef SPEED
// Warning: returns ptr to static buffer.

static char * opDescSTR (void)
  {
    static char temp [256];
    DCDstruct * i = & CPU -> currentInstruction;

    strcpy (temp, "");

    if (READOP (i))
      {
        switch (OPSIZE ())
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

        switch (OPSIZE ())
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

    return temp;    //"opDescSTR(???)";
  }
#endif

#ifndef QUIET_UNUSED
static char * operandSTR (void)
  {
    DCDstruct * i = & CPU -> currentInstruction;
    if (i -> info -> ndes > 0)
      return "operandSTR(): MWEIS not handled yet";

    static char temp [1024];

    int n = OPSIZE ();
    switch (n)
      {
        case 1:
          sprintf (temp, "CY=%012llo", CY);
          break;
        case 2:
          sprintf (temp, "CYpair[0]=%012llo CYpair[1]=%012llo",
                   Ypair [0], Ypair [1]);
          break;
        case 8:
        case 16:
        case 32:
        default:
          sprintf (temp, "Unhandled size: %d", n);
          break;
      }
    return temp;
}
#endif


static void doITP (word36 itxPair [2])
  {
    sim_debug (DBG_APPENDING, & cpu_dev,
               "ITP Pair: PRNUM=%o BITNO=%o WORDNO=%o MOD=%o\n",
               GET_ITP_PRNUM (itxPair), GET_ITP_WORDNO (itxPair),
               GET_ITP_BITNO (itxPair), GET_ITP_MOD (itxPair));
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

    word3 n = GET_ITP_PRNUM (itxPair);
    CPU -> TPR . TSR = CPU -> PR [n] . SNR;
    //CPU -> TPR . TRR = max3 (CPU -> CPU -> PR [n] . RNR, SDW -> R1, CPU -> TPR . TRR);
    CPU -> TPR . TRR = max3 (CPU -> PR [n] . RNR, CPU -> RSDWH_R1, CPU -> TPR . TRR);
    CPU -> TPR . TBR = GET_ITP_BITNO (itxPair);
    CPU -> TPR . CA = CPU -> PAR [n] . WORDNO + GET_ITP_WORDNO (itxPair);
    CPU -> TPR . CA &= AMASK;
    CPU -> rY = CPU -> TPR.CA;

    CPU -> rTAG = GET_ITP_MOD (itxPair);
    return;
  }

static void doITS (word36 itxPair [2])
  {
    sim_debug (DBG_APPENDING, & cpu_dev,
               "ITS Pair: SEGNO=%o RN=%o WORDNO=%o BITNO=%o MOD=%o\n",
               GET_ITS_SEGNO (itxPair), GET_ITS_RN (itxPair),
               GET_ITS_WORDNO (itxPair), GET_ITS_BITNO (itxPair),
               GET_ITS_MOD (itxPair));
    // C(ITS.SEGNO) -> C(TPR.TSR)
    // maximum of ( C(ITS. RN), C(SDW.R1), C(TPR.TRR) ) -> C(TPR.TRR)
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

    CPU -> TPR . TSR = GET_ITS_SEGNO (itxPair);

    sim_debug (DBG_APPENDING, & cpu_dev,
               "ITS Pair Ring: RN %o RSDWH_R1 %o TRR %o max %o\n",
               GET_ITS_RN (itxPair), CPU -> RSDWH_R1, CPU -> TPR . TRR,
               max3 (GET_ITS_RN (itxPair), CPU -> RSDWH_R1, CPU -> TPR . TRR));

    //CPU -> TPR . TRR = max3 (GET_ITS_RN (itxPair), SDW -> R1, CPU -> TPR . TRR);
    CPU -> TPR . TRR = max3 (GET_ITS_RN (itxPair), CPU -> RSDWH_R1, CPU -> TPR . TRR);
    CPU -> TPR . TBR = GET_ITS_BITNO (itxPair);
    CPU -> TPR . CA = GET_ITS_WORDNO (itxPair);
    CPU -> TPR . CA &= AMASK;

    CPU -> rY = CPU -> TPR . CA;

    CPU -> rTAG = GET_ITS_MOD (itxPair);

    return;
  }


// CANFAULT
static void doITSITP (word18 address, word36 indword, word6 Tag)
  {
    DCDstruct * i = & CPU -> currentInstruction;
    word6 indTag = GET_TAG  (indword);

    sim_debug  (DBG_APPENDING, &  cpu_dev,
                "doITS/ITP: indword:%012llo Tag:%o\n",
                indword, Tag);

    if (! ((GET_TM (Tag) == TM_IR || GET_TM (Tag) == TM_RI) &&
           (ISITP(indword) || ISITS(indword))))
      {
        sim_debug (DBG_APPENDING, & cpu_dev, "doITSITP: faulting\n");
        doFault (FAULT_IPR, ill_mod, "Incorrect address modifier");
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

    word36 itxPair [2];

    itxPair [0] = indword;

    Read (address + 1, & itxPair [1], INDIRECT_WORD_FETCH, i -> a);

    sim_debug (DBG_APPENDING, & cpu_dev,
               "doITS/ITP: YPair= %012llo %012llo\n",
               itxPair [0], itxPair [1]);

    if (ISITS (indTag))
        doITS (itxPair);
    else
        doITP (itxPair);

    //didITSITP = true;
    set_went_appending ();
  }


static void updateIWB (word18 addr, word6 tag)
  {
    sim_debug (DBG_ADDRMOD, & cpu_dev,
               "updateIWB: IWB was %012llo %06o %s\n",
               CPU -> cu . IWB, GET_ADDR (CPU -> cu . IWB),
               extMods [GET_TAG (CPU -> cu . IWB)] . mod);

    putbits36 (& CPU -> cu . IWB,  0, 18, addr);
    putbits36 (& CPU -> cu . IWB, 30,  6, tag);

    sim_debug (DBG_ADDRMOD, & cpu_dev,
               "updateIWB: IWB now %012llo %06o %s\n",
               CPU -> cu . IWB, GET_ADDR (CPU -> cu . IWB),
               extMods [GET_TAG (CPU -> cu . IWB)] . mod);

    decodeInstruction (CPU -> cu . IWB, & CPU -> currentInstruction);
  }

//
// Input:
//   CPU -> cu . IWB
//   currentInstruction
//   TPR . TSR
//   TPR . TRR
//   TPR . TBR // XXX check to see if this is initialized
//   TPR . CA
//
//  Output:
//   TPR . CA
//   directOperandFlag
//
// CANFAULT

t_stat doComputedAddressFormation (void)
  {

    sim_debug (DBG_ADDRMOD, & cpu_dev,
               "%s(Entry): operType:%s TPR.CA=%06o\n",
                __func__, opDescSTR (), CPU -> TPR . CA);

    DCDstruct * i = & CPU -> currentInstruction;

// This must happen before the restart entry
#if 0
    // NB: The currentInstrucion . a bit has been qualified with the IGN_B9 bit
    if (i -> a)
      {
        word3 n = GET_PRN (CPU -> cu . IWB);  // get PRn
        word15 offset = GET_OFFSET (CPU -> cu . IWB);

        CPU -> TPR . TSR = CPU -> PAR [n] . SNR;
        CPU -> TPR . TRR = max3 (CPU -> PAR [n] . RNR, CPU -> TPR . TRR, CPU -> PPR . PRR);

        CPU -> TPR . CA = (CPU -> PAR [n] . WORDNO + SIGNEXT15_18 (offset)) & 0777777;
        CPU -> TPR . TBR = CPU -> PAR [n] . BITNO;

        // Clear the A bit
        //updateIWB (CPU -> TPR . CA, i -> tag);
      }
#endif
    word6 Tm = 0;
    word6 Td = 0;
    //didITSITP = false;

#if 0 // Unused
    eCAFoper operType;
    if (RMWOP (i))
      operType = rmwCY;           // r/m/w cycle
    else if (READOP (i))
      operType = readCY;          // read cycle
    else if (WRITEOP (i))
      operType = writeCY;         // write cycle
    else
      operType = prepareCA;
#endif

    int iTAG;   // tag of word preceeding an indirect fetch

    CPU -> directOperandFlag = false;
    CPU -> characterOperandFlag = false;

    if (i -> info -> flags & NO_TAG) // for instructions line STCA/STCQ
      CPU -> rTAG = 0;
    else
      CPU -> rTAG = GET_TAG (CPU -> cu . IWB);

    int lockupCnt = 0;
#define lockupLimit 4096 // approx. 2 ms

startCA:;

    if (++ lockupCnt > lockupLimit)
      {
        doFault (FAULT_LUF, 0,
                 "Lockup in addrmod");
      }

    Td = GET_TD (CPU -> rTAG);
    Tm = GET_TM (CPU -> rTAG);

    sim_debug (DBG_ADDRMOD, & cpu_dev,
               "%s(startCA): TAG=%02o(%s) Tm=%o Td=%o\n",
               __func__, CPU -> rTAG, getModString (CPU -> rTAG), Tm, Td);

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
                __func__, GET_TM (CPU -> rTAG));
    sim_err ("(startCA): unknown Tm\n");


        //! Register modification. Fig 6-3
    R_MOD:;
      {
        if (Td == 0) // TPR.CA = address from opcode
          {
            //updateIWB (identity)
            return SCPE_OK;
          }

        word18 Cr = getCr (Td);

        sim_debug (DBG_ADDRMOD, & cpu_dev, "R_MOD: Cr=%06o\n", Cr);

        if (CPU -> directOperandFlag)
          {
            sim_debug (DBG_ADDRMOD, & cpu_dev,
                       "R_MOD: directOperand = %012llo\n", CPU -> directOperand);

            //updateIWB (identity)
            return SCPE_OK;
          }

        if (CPU -> cu . rpt || CPU -> cu . rd)
          {
            word6 Td = GET_TD (i -> tag);
            uint Xn = X (Td);  // Get Xn of next instruction
            CPU -> TPR . CA = CPU -> rX [Xn];
            if (i -> a)
              {
                word3 PRn = (i -> address >> 15) & MASK3;
                CPU -> TPR . CA += CPU -> PR [PRn] . WORDNO;
                CPU -> TPR . CA &= AMASK;
              }
          }
        else
          {
            CPU -> TPR . CA  += Cr;
            CPU -> TPR . CA  &= MASK18;   // keep to 18-bits
          }
        sim_debug (DBG_ADDRMOD, & cpu_dev, "R_MOD: TPR.CA=%06o\n", CPU -> TPR . CA);

        // If repeat, the indirection chain is limited, so it is not necessary
        // to clear the tag; the delta code later on needs the tag to know
        // which X register to update
        if (! (CPU -> cu . rpt || CPU -> cu . rd))
          updateIWB (CPU -> TPR . CA, 0);
        return SCPE_OK;
      } // R_MOD


        //! Figure 6-4. Register Then Indirect Modification Flowchart
    RI_MOD:;
      {
        sim_debug (DBG_ADDRMOD, & cpu_dev, "RI_MOD: Td=%o\n", Td);

        if (Td == TD_DU || Td == TD_DL)
          doFault (FAULT_IPR, ill_mod,
                   "RI_MOD: Td == TD_DU || Td == TD_DL");

        word18 tmpCA = CPU -> TPR . CA;

        if (Td != 0)
          {
            word18 Cr = getCr (Td);  // C(r)

            // We don''t need to worry about direct operand here, since du
            // and dl are disallowed above

            sim_debug (DBG_ADDRMOD, & cpu_dev,
                       "RI_MOD: Cr=%06o tmpCA(Before)=%06o\n", Cr, tmpCA);

            if (CPU -> cu . rpt || CPU -> cu . rd)
              {
                 word6 Td = GET_TD (i -> tag);
                 uint Xn = X (Td);  // Get Xn of next instruction
                 tmpCA = CPU -> rX [Xn];
              }
            else
              {
                tmpCA += Cr;
                tmpCA &= MASK18;   // keep to 18-bits
              }
            sim_debug (DBG_ADDRMOD, & cpu_dev,
                       "RI_MOD: tmpCA(After)=%06o\n", tmpCA);
          }

        // If the indirect word faults, on restart the CA will be the post
        // register modification value, so we want to prevent it from 
        // happening again on restart

        updateIWB (CPU -> TPR . CA, TM_RI | TD_N);

        // in case it turns out to be a ITS/ITP
        iTAG = CPU -> rTAG;

        word36 indword;
        Read (tmpCA, & indword, INDIRECT_WORD_FETCH, i -> a); //TM_RI);

        // "In the case of RI modification, only one indirect reference is made
        // per repeated execution. The TAG field of the indirect word is not
        // interpreted.  The indirect word is treated as though it had R
        // modification with R = N."

        if (CPU -> cu . rpt || CPU -> cu . rd)
          {
             indword &= ~ INST_M_TAG;
             indword |= TM_R | GET_TD (iTAG);
           }

        // (Closed) Ticket 15: Check for fault causing tags before updating
        // the IWB, so the instruction restart will reload the offending
        // indirect word.

        if (GET_TM (GET_TAG(indword)) == TM_IT)
          {
            if (GET_TD (GET_TAG(indword)) == IT_F2)
              {
                CPU -> TPR . CA = tmpCA;
                doFault (FAULT_F2, 0, "RI_MOD: IT_F2 (0)");
              }
            if (GET_TD (GET_TAG(indword)) == IT_F3)
              {
                CPU -> TPR . CA = tmpCA;
                doFault (FAULT_F3, 0, "RI_MOD: IT_F3");
              }
          }

        if (ISITP (indword) || ISITS (indword))
          {
            doITSITP (tmpCA, indword, iTAG);
          }
        else
          {
            CPU -> TPR . CA = GETHI (indword);
            CPU -> rTAG = GET_TAG (indword);
            CPU -> rY = CPU -> TPR . CA;
          }

        sim_debug (DBG_ADDRMOD, & cpu_dev,
                   "RI_MOD: indword=%012llo TPR.CA=%06o rTAG=%02o\n",
                   indword, CPU -> TPR . CA, CPU -> rTAG);
        // If repeat, the indirection chain is limited, so it is not needed
        // to clear the tag; the delta code later on needs the tag to know
        // which X register to update
        if (! (CPU -> cu . rpt || CPU -> cu . rd))
          updateIWB (CPU -> TPR . CA, CPU -> rTAG);
        goto startCA;
      } // RI_MOD

        //! Figure 6-5. Indirect Then Register Modification Flowchart
    IR_MOD:;
      {
        CPU -> cu . CT_HOLD = Td;

        sim_debug (DBG_ADDRMOD, & cpu_dev,
                   "IR_MOD: CT_HOLD=%o %o\n", CPU -> cu . CT_HOLD, Td);

        IR_MOD_1:

        sim_debug (DBG_ADDRMOD, & cpu_dev,
                   "IR_MOD: fetching indirect word from %06o\n", CPU -> TPR . CA);

        // in case it turns out to be a ITS/ITP
        iTAG = CPU -> rTAG;

        word36 indword;
        word18 saveCA = CPU -> TPR . CA;
        Read (CPU -> TPR . CA, & indword, INDIRECT_WORD_FETCH, i -> a);

        if (ISITP (indword) || ISITS (indword))
          {
            doITSITP (CPU -> TPR . CA, indword, iTAG);
          }
        else
          {
            CPU -> TPR.CA = GETHI(indword);
            CPU -> rY = CPU -> TPR.CA;
            CPU -> rTAG = GET_TAG (indword);
          }

        Td = GET_TD(CPU -> rTAG);
        Tm = GET_TM(CPU -> rTAG);

        // IR_MOD_2:;

        sim_debug (DBG_ADDRMOD, & cpu_dev,
                   "IR_MOD1: indword=%012llo TPR.CA=%06o Tm=%o Td=%02o (%s)\n",
                   indword, CPU -> TPR . CA, Tm, Td, getModString (GET_TAG (indword)));

        switch (Tm)
          {
            case TM_IT:
              {
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IR_MOD(TM_IT): Td=%02o => %02o\n",
                           Td, CPU -> cu . CT_HOLD);
                if (Td == IT_F2 || Td == IT_F3)
                {
                    // Abort. FT2 or 3
                    switch (Td)
                    {
                        case IT_F2:
                            CPU -> TPR . CA = saveCA;
                            doFault (FAULT_F2, 0, "TM_IT: IT_F2 (1)");

                        case IT_F3:
                            CPU -> TPR . CA = saveCA;
                            doFault( FAULT_F3, 0, "TM_IT: IT_F3");
                    }
                }
                // fall through to TM_R
              } // TM_IT

            case TM_R:
              {
                word18 Cr = getCr (CPU -> cu . CT_HOLD);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IR_MOD(TM_R): Cr=%06o\n", Cr);

#if 0
                if (CPU -> directOperandFlag)
                  {
                    sim_debug (DBG_ADDRMOD, & cpu_dev,
                      "IR_MOD(TM_R:directOperandFlag): operand=%012llo\n",
                      CPU -> directOperand);

                    CPU -> TPR . CA = CPU -> directOperand;

                    updateIWB (CPU -> TPR . CA, CPU -> cu . CT_HOLD);

                    return SCPE_OK;
                }

                CPU -> TPR . CA += Cr;
                CPU -> TPR . CA &= MASK18;   // keep to 18-bits

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IR_MOD(TM_R): TPR.CA=%06o\n", CPU -> TPR . CA);

                updateIWB (CPU -> TPR . CA, CPU -> cu . CT_HOLD);
#else
#define ABUSE_CT_HOLD
#ifdef ABUSE_CT_HOLD
                if (CPU -> directOperandFlag)
                  {
                    CPU -> TPR . CA += CPU -> directOperand;
                    CPU -> TPR . CA &= MASK18;   // keep to 18-bits

                    sim_debug (DBG_ADDRMOD, & cpu_dev,
                               "IR_MOD(TM_R): DO TPR.CA=%06o\n", CPU -> TPR . CA);

                    updateIWB (CPU -> TPR . CA, TM_R | TD_DL);
                    CPU -> cu . CT_HOLD = 0;
                  }
                else
                  {
                    CPU -> TPR . CA += Cr;
                    CPU -> TPR . CA &= MASK18;   // keep to 18-bits

                    sim_debug (DBG_ADDRMOD, & cpu_dev,
                               "IR_MOD(TM_R): TPR.CA=%06o\n", CPU -> TPR . CA);

                    updateIWB (CPU -> TPR . CA, 0);
                    CPU -> cu . CT_HOLD = 0;
                  }
#else
                if (CPU -> directOperandFlag)
                  {
                    CPU -> TPR . CA += CPU -> directOperand;
                  }
                else
                  {
                    CPU -> TPR . CA += Cr;
                  }
                CPU -> TPR . CA &= MASK18;   // keep to 18-bits

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IR_MOD(TM_R): TPR.CA=%06o\n", CPU -> TPR . CA);

                updateIWB (CPU -> TPR . CA, 0);
#endif
#endif
                return SCPE_OK;
              } // TM_R

            case TM_RI:
              {
                word18 Cr = getCr(Td);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IR_MOD(TM_RI): Td=%o Cr=%06o TPR.CA(Before)=%06o\n",
                           Td, Cr, CPU -> TPR . CA);

                CPU -> TPR . CA += Cr;
                CPU -> TPR . CA &= MASK18;   // keep to 18-bits

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IR_MOD(TM_RI): TPR.CA(After)=%06o\n",
                           CPU -> TPR . CA);

                updateIWB (CPU -> TPR . CA, CPU -> rTAG); // XXX guessing here...
                goto IR_MOD_1;
              } // TM_RI

            case TM_IR:
              {
                updateIWB (CPU -> TPR . CA, CPU -> rTAG); // XXX guessing here...
                goto IR_MOD;
              } // TM_IR
          } // Tm

        sim_printf ("%s(IR_MOD): unknown Tm??? %o\n", 
                    __func__, GET_TM (CPU -> rTAG));
        return SCPE_OK;
      } // IR_MOD

    IT_MOD:;
      {
        //    IT_SD     = 004,
        //    IT_SCR        = 005,
        //    IT_CI     = 010,
        //    IT_I      = 011,
        //    IT_SC     = 012,
        //    IT_AD     = 013,
        //    IT_DI     = 014,
        //    IT_DIC        = 015,
        //    IT_ID     = 016,
        //    IT_IDC        = 017
        word12 tally;
        word6 idwtag, delta;
        word24 Yi = -1;

        switch (Td)
          {
            // XXX this is probably wrong. ITS/ITP are not standard addr mods
            case SPEC_ITP:
            case SPEC_ITS:
              {
                doFault(FAULT_IPR, ill_mod, "ITx in IT_MOD)");
              }

            case 2:
              {
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(): illegal procedure, illegal modifier, "
                           "fault Td=%o\n", Td);
                doFault (FAULT_IPR, ill_mod,
                         "IT_MOD(): illegal procedure, illegal modifier, "
                         "fault");
              }

            case IT_F1:
              {
                doFault(FAULT_F1, 0, "IT_MOD: IT_F1");
              }

            case IT_F2:
              {
                doFault(FAULT_F2, 0, "IT_MOD: IT_F2 (2)");
              }

            case IT_F3:
              {
                doFault(FAULT_F3, 0, "IT_MOD: IT_F3");
              }

            case IT_CI: ///< Character indirect (Td = 10)
              {
#ifdef ABUSE_CT_HOLD
                // Check for instruction restart
                if (CPU -> cu . CT_HOLD)
                  {
                    CPU -> characterOperandFlag = true;
                    CPU -> characterOperandSize = CPU -> cu . CT_HOLD & 010 ? TB9 : TB6;
                    CPU -> characterOperandOffset = CPU -> cu . CT_HOLD & 007;
//sim_printf ("XXX CI\n");
                    return SCPE_OK;
                  }
#endif

                // Bit 30 of the TAG field of the indirect word is interpreted
                // as a character size flag, tb, with the value 0 indicating
                // 6-bit characters and the value 1 indicating 9-bit bytes.
                // Bits 33-35 of the TAG field are interpreted as a 3-bit
                // character/byte position value, cf. Bits 31-32 of the TAG
                // field must be zero.  If the character position value is
                // greater than 5 for 6-bit characters or greater than 3 for 9-
                // bit bytes, an illegal procedure, illegal modifier, fault
                // will occur. The TALLY field is ignored. The computed address
                // is the value of the ADDRESS field of the indirect word. The
                // effective character/byte number is the value of the
                // character position count, cf, field of the indirect word.

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_CI): reading indirect word from %06o\n",
                            CPU -> TPR . CA);

                word36 indword;
                Read (CPU -> TPR . CA, & indword, OPERAND_READ, i -> a);  //TM_IT);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_CI): indword=%012llo\n", indword);

                CPU -> TPR.CA = GET_ADDR (indword);
                CPU -> characterOperandFlag = true;
                CPU -> characterOperandSize = GET_TB (GET_TAG (indword));
                CPU -> characterOperandOffset = GET_CF (GET_TAG (indword));

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_CI): size=%o offset=%o TPR.CA=%06o\n",
                           CPU -> characterOperandSize, CPU -> characterOperandOffset,
                           CPU -> TPR . CA);

                if (CPU -> characterOperandSize == TB6 && CPU -> characterOperandOffset > 5)
                  // generate an illegal procedure, illegal modifier fault
                  doFault (FAULT_IPR, ill_mod,
                           "co size == TB6 && offset > 5");

                if (CPU -> characterOperandSize == TB9 && CPU -> characterOperandOffset > 3)
                  // generate an illegal procedure, illegal modifier fault
                  doFault (FAULT_IPR, ill_mod,
                           "co size == TB9 && offset > 3");

#ifdef ABUSE_CT_HOLD
                CPU -> cu . CT_HOLD = 020 | // flag = true
                               (CPU -> characterOperandSize == TB9 ? 010 : 000) |
                               (CPU -> characterOperandOffset & 007);

                updateIWB (CPU -> TPR . CA, CPU -> rTAG);
#else
                //updateIWB (identity)
#endif
                return SCPE_OK;
               } // IT_CI

            case IT_SC: ///< Sequence character (Td = 12)
              {
#ifdef ABUSE_CT_HOLD
                // Check for instruction restart
                if (CPU -> cu . CT_HOLD)
                  {
                    CPU -> characterOperandFlag = true;
                    CPU -> characterOperandSize = CPU -> cu . CT_HOLD & 010 ? TB9 : TB6;
                    CPU -> characterOperandOffset = CPU -> cu . CT_HOLD & 007;
                    return SCPE_OK;
                  }
#endif

                // Bit 30 of the TAG field of the indirect word is interpreted
                // as a character size flag, tb, with the value 0 indicating
                // 6-bit characters and the value 1 indicating 9-bit bytes.
                // Bits 33-35 of the TAG field are interpreted as a 3-bit
                // character/byte position counter, cf. Bits 31-32 of the TAG
                // field must be zero.

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SC): reading indirect word from %06o\n",
                           CPU -> TPR . CA);

                word18 saveCA = CPU -> TPR . CA;
                word36 indword;
                Read (CPU -> TPR . CA, & indword, OPERAND_READ, i -> a);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SC): indword=%012llo\n", indword);

                tally = GET_TALLY (indword);    // 12-bits
                idwtag = GET_TAG (indword);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SC): tally=%04o idwtag=%02o\n",
                           tally, idwtag);

                // The computed address is the unmodified value of the ADDRESS
                // field. The effective character/byte number is the unmodified
                // value of the character position counter, cf, field of the
                // indirect word.

                CPU -> characterOperandFlag = true;
                CPU -> characterOperandSize = GET_TB (idwtag);
                CPU -> characterOperandOffset = GET_CF (idwtag);
                Yi = GETHI (indword);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                          "IT_MOD(IT_SC): co size=%o offset=%o Yi=%06o\n",
                          CPU -> characterOperandSize, CPU -> characterOperandOffset, Yi);

                CPU -> TPR . CA = Yi;

                word18 computedAddress = CPU -> TPR . CA;

                // For each reference to the indirect word, the character
                // counter, cf, is increased by 1 and the TALLY field is
                // reduced by 1 after the computed address is formed. Character
                // count arithmetic is modulo 6 for 6-bit characters and modulo
                // 4 for 9-bit bytes. If the character count, cf, overflows to
                // 6 for 6-bit characters or to 4 for 9-bit bytes, it is reset
                // to 0 and ADDRESS is increased by 1. ADDRESS arithmetic is
                // modulo 2^18. TALLY arithmetic is modulo 4096. If the TALLY
                // field is reduced to 0, the tally runout indicator is set ON,
                // otherwise it is set OFF.

                int cos = CPU -> characterOperandOffset + 1;
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SC): characterOperandOffset = %o\n", cos);

                if (((CPU -> characterOperandSize == TB6) && (cos > 5)) ||
                    ((CPU -> characterOperandSize == TB9) && (cos > 3)))
                  {
                    cos = 0;
                    Yi += 1;
                    Yi &= MASK18;

                    sim_debug (DBG_ADDRMOD, &cpu_dev,
                               "IT_MOD(IT_SC): reset characterOperandOffset. "
                               "Yi now %06o\n", Yi);
                  }

                tally -= 1;
                tally &= 07777; // keep to 12-bits

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SC): tally now %o\n", tally);

                SCF (tally == 0, CPU -> cu . IR, I_TALLY);

                indword = (word36) (((word36) Yi << 18) |
                                    (((word36) tally & 07777) << 6) |
                                    CPU -> characterOperandSize |
                                    cos);
                Write (saveCA, indword, OPERAND_STORE, i -> a);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SC): wrote tally word %012llo to %06o\n",
                           indword, saveCA);

                CPU -> TPR . CA = computedAddress;
#ifdef ABUSE_CT_HOLD
                CPU -> cu . CT_HOLD = 020 | // flag = true
                               (CPU -> characterOperandSize == TB9 ? 010 : 000) |
                               (CPU -> characterOperandOffset & 007);

                updateIWB (computedAddress, CPU -> rTAG);
#else
                updateIWB (computedAddress, 0); // XXX guessing here...
#endif
                return SCPE_OK;
              } // IT_SC

            case IT_SCR: // Sequence character reverse (Td = 5)
              {
#ifdef ABUSE_CT_HOLD
                // Check for instruction restart
                if (CPU -> cu . CT_HOLD)
                  {
                    CPU -> characterOperandFlag = true;
                    CPU -> characterOperandSize = CPU -> cu . CT_HOLD & 010 ? TB9 : TB6;
                    CPU -> characterOperandOffset = CPU -> cu . CT_HOLD & 007;
                    return SCPE_OK;
                  }
#endif

                // Bit 30 of the TAG field of the indirect word is interpreted
                // as a character size flag, tb, with the value 0 indicating
                // 6-bit characters and the value 1 indicating 9-bit bytes.
                // Bits 33-35 of the TAG field are interpreted as a 3-bit
                // character position counter, cf. Bits 31-32 of the TAG field
                // must be zero.

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SCR): reading indirect word from %06o\n",
                           CPU -> TPR.CA);

                word18 saveCA = CPU -> TPR . CA;
                word36 indword;
                Read (CPU -> TPR . CA, & indword, OPERAND_READ, i -> a);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SCR): indword=%012llo\n", indword);

                tally = GET_TALLY (indword);    // 12-bits
                idwtag = GET_TAG (indword);
                Yi = GETHI (indword);           // where chars/bytes live

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SCR): tally=%04o idwtag=%02o\n",
                           tally, idwtag);

                CPU -> characterOperandFlag = true;
                CPU -> characterOperandSize = GET_TB (idwtag);   //GET_TAG(indword));
                CPU -> characterOperandOffset = GET_CF (idwtag);   //GET_TAG(indword));

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                          "IT_MOD(IT_SCR): characterOperandSize=%o "
                           "characterOperandOffset=%o Yi=%06o\n",
                           CPU -> characterOperandSize, CPU -> characterOperandOffset, Yi);

                // For each reference to the indirect word, the character
                // counter, cf, is reduced by 1 and the TALLY field is
                // increased by 1 before the computed address is formed.
                // Character count arithmetic is modulo 6 for 6-bit characters
                // and modulo 4 for 9-bit bytes. If the character count, cf,
                // underflows to -1, it is reset to 5 for 6-bit characters or
                // to 3 for 9-bit bytes and ADDRESS is reduced by 1. ADDRESS
                // arithmetic is modulo 2^18. TALLY arithmetic is modulo 4096.
                // If the TALLY field overflows to 0, the tally runout
                // indicator is set ON, otherwise it is set OFF. The computed
                // address is the (possibly) decremented value of the ADDRESS
                // field of the indirect word. The effective character/byte
                // number is the decremented value of the character position
                // count, cf, field of the indirect word.

                if (CPU -> characterOperandOffset == 0)
                  {
                    if (CPU -> characterOperandSize == TB6)
                        CPU -> characterOperandOffset = 5;
                    else
                        CPU -> characterOperandOffset = 3;
                    Yi -= 1;
                    Yi &= MASK18;
                  }
                else
                  {
                    CPU -> characterOperandOffset -= 1;
                  }

                tally += 1;
                tally &= 07777; // keep to 12-bits
                SCF (tally == 0, CPU -> cu . IR, I_TALLY);

                indword = (word36) (((word36) Yi << 18) |
                                    (((word36) tally & 07777) << 6) |
                                    CPU -> characterOperandSize |
                                    CPU -> characterOperandOffset);
                Write (saveCA, indword, OPERAND_STORE, i -> a);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SCR): wrote tally word %012llo to %06o\n",
                           indword, saveCA);

                CPU -> TPR.CA = Yi;

#ifdef ABUSE_CT_HOLD
                CPU -> cu . CT_HOLD = 020 | // flag = true
                               (CPU -> characterOperandSize == TB9 ? 010 : 000) |
                               (CPU -> characterOperandOffset & 007);

                updateIWB (CPU -> TPR . CA, CPU -> rTAG);
#else
                updateIWB (CPU -> TPR . CA, 0); // XXX guessing here...
#endif
                return SCPE_OK;
              } // IT_SCR

            case IT_I: // Indirect (Td = 11)
              {
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_I): reading indirect word from %06o\n",
                           CPU -> TPR.CA);

                word36 indword;
                Read (CPU -> TPR . CA, & indword, INDIRECT_WORD_FETCH, 0);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_I): indword=%012llo\n",
                           indword);

                CPU -> TPR . CA = GET_ADDR (indword);

                updateIWB (CPU -> TPR . CA, 0); // XXX guessing here...

                return SCPE_OK;
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
                           CPU -> TPR . CA);

                word18 saveCA = CPU -> TPR . CA;
                word36 indword;
                Read (CPU -> TPR . CA, & indword, OPERAND_READ, i -> a);

                tally = GET_TALLY (indword); // 12-bits
                delta = GET_DELTA (indword); // 6-bits
                Yi = GETHI (indword);        // from where data live

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_AD): indword=%012llo\n",
                           indword);
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_AD): address:%06o tally:%04o delta:%03o\n",
                           Yi, tally, delta);

                CPU -> TPR . CA = Yi;
                word18 computedAddress = CPU -> TPR . CA;

                Yi += delta;
                Yi &= MASK18;

                tally -= 1;
                tally &= 07777; // keep to 12-bits
                SCF(tally == 0, CPU -> cu . IR, I_TALLY);

                indword = (word36) (((word36) Yi << 18) |
                                    (((word36) tally & 07777) << 6) |
                                    delta);
                Write (saveCA, indword, OPERAND_STORE, i -> a);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_AD): wrote tally word %012llo to %06o\n",
                            indword, saveCA);

                CPU -> TPR . CA = computedAddress;

                updateIWB (CPU -> TPR . CA, 0); // XXX guessing here...

                return SCPE_OK;
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

                word18 saveCA = CPU -> TPR . CA;
                word36 indword;
                Read (CPU -> TPR . CA, & indword, OPERAND_READ, i -> a);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SD): reading indirect word from %06o\n",
                           CPU -> TPR . CA);
                tally = GET_TALLY (indword); // 12-bits
                delta = GET_DELTA (indword); // 6-bits
                Yi    = GETHI (indword);     // from where data live

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SD): indword=%012llo\n",
                           indword);
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SD): address:%06o tally:%04o delta:%03o\n",
                           Yi, tally, delta);

                Yi -= delta;
                Yi &= MASK18;
                CPU -> TPR.CA = Yi;

                tally += 1;
                tally &= 07777; // keep to 12-bits
                SCF(tally == 0, CPU -> cu.IR, I_TALLY);

                // write back out indword
                indword = (word36) (((word36) Yi << 18) |
                                    (((word36) tally & 07777) << 6) |
                                    delta);
                Write (saveCA, indword, OPERAND_STORE, i -> a);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SD): wrote tally word %012llo to %06o\n",
                           indword, saveCA);


                CPU -> TPR . CA = Yi;
                updateIWB (CPU -> TPR . CA, 0); // XXX guessing here...

                return SCPE_OK;
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
                           CPU -> TPR . CA);

                word18 saveCA = CPU -> TPR . CA;
                word36 indword;
                Read (CPU -> TPR . CA, & indword, OPERAND_READ, i -> a);

                Yi = GETHI (indword);
                tally = GET_TALLY (indword); // 12-bits
                word6 junk = GET_TAG (indword); // get tag field, but ignore it

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_DI): indword=%012llo\n",
                           indword);
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_DI): address:%06o tally:%04o\n",
                           Yi, tally);

                Yi -= 1;
                Yi &= MASK18;
                CPU -> TPR.CA = Yi;

                tally += 1;
                tally &= 07777; // keep to 12-bits
                SCF(tally == 0, CPU -> cu.IR, I_TALLY);

                // write back out indword

                indword = (word36) (((word36) CPU -> TPR . CA << 18) |
                                    ((word36) tally << 6) |
                                    junk);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_DI): writing indword=%012llo to "
                           "addr %06o\n",
                           indword, saveCA);

                Write (saveCA, indword, OPERAND_STORE, i -> a);

                CPU -> TPR . CA = Yi;
                updateIWB (CPU -> TPR . CA, 0); // XXX guessing here...

                return SCPE_OK;
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

                word18 saveCA = CPU -> TPR . CA;

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_ID): fetching indirect word from %06o\n",
                           CPU -> TPR . CA);

                word36 indword;
                Read (CPU -> TPR . CA, & indword, OPERAND_READ, i -> a);

                Yi = GETHI (indword);
                tally = GET_TALLY (indword); // 12-bits
                // get tag field, but ignore it
                word6 junk = GET_TAG (indword);
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_ID): indword=%012llo Yi=%06o tally=%04o\n",
                           indword, Yi, tally);

                CPU -> TPR . CA = Yi;
                word18 computedAddress = CPU -> TPR . CA;

                Yi += 1;
                Yi &= MASK18;

                tally -= 1;
                tally &= 07777; // keep to 12-bits
                SCF(tally == 0, CPU -> cu.IR, I_TALLY);

                // write back out indword
                indword = (word36) (((word36) Yi << 18) |
                                    ((word36) tally << 6) |
                                    junk);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_ID): writing indword=%012llo to "
                           "addr %06o\n",
                           indword, saveCA);

                Write (saveCA, indword, OPERAND_STORE, i->a);

                CPU -> TPR . CA = computedAddress;
                updateIWB (CPU -> TPR . CA, 0); // XXX guessing here...

                return SCPE_OK;
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
                // address is formed .

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_DIC): fetching indirect word from %06o\n",
                           CPU -> TPR . CA);

                word18 saveCA = CPU -> TPR . CA;
                word36 indword;
                Read (CPU -> TPR . CA, & indword, OPERAND_READ, i -> a);

                Yi = GETHI (indword);
                tally = GET_TALLY (indword); // 12-bits
                idwtag = GET_TAG (indword);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_DIC): indword=%012llo Yi=%06o "
                           "tally=%04o idwtag=%02o\n", 
                           indword, Yi, tally, idwtag);

                Yi -= 1;
                Yi &= MASK18;

                word24 YiSafe2 = Yi; // save indirect address for later use

                CPU -> TPR.CA = Yi;

                tally += 1;
                tally &= 07777; // keep to 12-bits
                SCF(tally == 0, CPU -> cu.IR, I_TALLY);

                // write back out indword
                indword = (word36) (((word36) Yi << 18) |
                          ((word36) tally << 6) | idwtag);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_DIC): writing indword=%012llo to "
                           "addr %06o\n", indword, saveCA);

                Write (saveCA, indword, OPERAND_STORE, i->a);

                // If the TAG of the indirect word invokes a register, that is,
                // specifies r, ri, or ir modification, the effective Td value
                // for the register is forced to "null" before the next
                // computed address is formed.

                CPU -> TPR.CA = YiSafe2;

                CPU -> rTAG = idwtag;
                Tm = GET_TM(CPU -> rTAG);

                if (Tm != TM_IT)
                  {
                    CPU -> rTAG = idwtag & 0x70; // force R to 0
                  }
                updateIWB (CPU -> TPR . CA, CPU -> rTAG);
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

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_IDC): fetching indirect word from %06o\n",
                           CPU -> TPR . CA);

                word18 saveCA = CPU -> TPR . CA;
                word36 indword;
                Read (CPU -> TPR . CA, & indword, OPERAND_READ, i -> a);

                Yi = GETHI (indword);
                tally = GET_TALLY (indword); // 12-bits
                idwtag = GET_TAG (indword);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_IDC): indword=%012llo Yi=%06o "
                           "tally=%04o idwtag=%02o\n",
                           indword, Yi, tally, idwtag);

                word24 YiSafe = Yi; // save indirect address for later use

                Yi += 1;
                Yi &= MASK18;

                tally -= 1;
                tally &= 07777; // keep to 12-bits
                SCF(tally == 0, CPU -> cu . IR, I_TALLY);

                // write back out indword
                indword = (word36) (((word36) Yi << 18) |
                                    ((word36) tally << 6) |
                                    idwtag);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_IDC): writing indword=%012llo"
                           " to addr %06o\n",
                           indword, saveCA);

                Write (saveCA, indword, OPERAND_STORE, i -> a);

                // If the TAG of the indirect word invokes a register, that is,
                // specifies r, ri, or ir modification, the effective Td value
                // for the register is forced to "null" before the next
                // computed address is formed.
                // But for the dps88 you can use everything but ir/ri.

                // force R to 0 (except for IT)
                CPU -> TPR.CA = YiSafe;

                CPU -> rTAG = idwtag;
                Tm = GET_TM(CPU -> rTAG);

                if (Tm != TM_IT)
                  {
                    CPU -> rTAG = idwtag & 0x70; // force R to 0
                  }
                updateIWB (CPU -> TPR . CA, CPU -> rTAG);
                goto startCA;
              } // IT_IDC
          } // Td
        sim_printf ("IT_MOD/Td how did we get here?\n");
        return SCPE_OK;
     } // IT_MOD
  } // doComputedAddressFormation


