
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

static bool directOperandFlag;
static word36 directOperand;

static bool characterOperandFlag;
static int characterOperandSize;
static int characterOperandOffset;

//
// return contents of register indicated by Td
//

static word18 getCr (word4 Tdes)
  {
    directOperandFlag = false;

    if (Tdes == 0)
      return 0;

    if (Tdes & 010) // Xn
      return rX [X (Tdes)];

    switch (Tdes)
      {
        case TD_N:  // rY = address from opcode
          return 0;

        case TD_AU: // rY + C(A)0,17
          return GETHI (rA);

        case TD_QU: // rY + C(Q)0,17
          return GETHI (rQ);

        case TD_DU: // none; operand has the form y || (00...0)18
          directOperand = 0;
          SETHI (directOperand, rY);
          directOperandFlag = true;

          sim_debug (DBG_ADDRMOD, & cpu_dev,
                    "getCr(TD_DU): rY=%06o directOperand=%012llo\n",
                    rY, directOperand);

          return 0;

        case TD_IC: // rY + C (PPR.IC)
            return PPR . IC;

        case TD_AL: // rY + C(A)18,35
            return GETLO (rA);

        case TD_QL: // rY + C(Q)18,35
            return GETLO (rQ);

        case TD_DL: // none; operand has the form (00...0)18 || y
            directOperand = 0;
            SETLO (directOperand, rY);
            directOperandFlag = true;

            sim_debug (DBG_ADDRMOD, & cpu_dev,
                       "getCr(TD_DL): rY=%06o directOperand=%012llo\n",
                       rY, directOperand);

            return 0;
      }
    return 0;
  }

// Warning: returns ptr to static buffer.

static char * opDescSTR (void)
  {
    static char temp [256];
    DCDstruct * i = & currentInstruction;

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

#ifndef QUIET_UNUSED
static char * operandSTR (void)
  {
    DCDstruct * i = & currentInstruction;
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


// Y-pair for ITS/ITP operations (so we don't have to muck with the real Ypair)
static word36 itxPair[2];

static void doITP (void)
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
    TPR . TSR = PR [n] . SNR;
    //TPR . TRR = max3 (PR [n] . RNR, SDW -> R1, TPR . TRR);
    TPR . TRR = max3 (PR [n] . RNR, RSDWH_R1, TPR . TRR);
    TPR . TBR = GET_ITP_BITNO (itxPair);
    TPR . CA = PAR [n] . WORDNO + GET_ITP_WORDNO (itxPair);
    TPR . CA &= AMASK;
    rY = TPR.CA;

    rTAG = GET_ITP_MOD (itxPair);
    return;
}

static void doITS(void)
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

    TPR . TSR = GET_ITS_SEGNO (itxPair);

    sim_debug (DBG_APPENDING, & cpu_dev,
               "ITS Pair Ring: RN %o RSDWH_R1 %o TRR %o max %o\n",
               GET_ITS_RN (itxPair), RSDWH_R1, TPR . TRR,
               max3 (GET_ITS_RN (itxPair), RSDWH_R1, TPR . TRR));

    //TPR . TRR = max3 (GET_ITS_RN (itxPair), SDW -> R1, TPR . TRR);
    TPR . TRR = max3 (GET_ITS_RN (itxPair), RSDWH_R1, TPR . TRR);
    TPR . TBR = GET_ITS_BITNO (itxPair);
    TPR . CA = GET_ITS_WORDNO (itxPair);
    TPR . CA &= AMASK;

    rY = TPR . CA;

    rTAG = GET_ITS_MOD (itxPair);

    return;
  }


// CANFAULT
static void doITSITP (word18 address, word36 indword, word6 Tag, word6 * newtag)
  {
    DCDstruct * i = & currentInstruction;
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

    itxPair [0] = indword;

    Read (address + 1, & itxPair [1], INDIRECT_WORD_FETCH, i -> a);

    sim_debug (DBG_APPENDING, & cpu_dev,
               "doITS/ITP: YPair= %012llo %012llo\n",
               itxPair [0], itxPair [1]);

    if (ISITS (indTag))
        doITS ();
    else
        doITP ();

    * newtag = GET_TAG (itxPair [1]);
    //didITSITP = true;
    set_went_appending ();
  }


static void updateIWB (word18 addr, word6 tag)
  {
    sim_debug (DBG_ADDRMOD, & cpu_dev,
               "updateIWB: IWB was %012llo %06o %s\n",
               cu . IWB, GET_ADDR (cu . IWB),
               extMods [GET_TAG (cu . IWB)] . mod);

    putbits36 (& cu . IWB,  0, 18, addr);
    putbits36 (& cu . IWB, 30,  6, tag);
    putbits36 (& cu . IWB, 29,  1, 0);

    sim_debug (DBG_ADDRMOD, & cpu_dev,
               "updateIWB: IWB now %012llo %06o %s\n",
               cu . IWB, GET_ADDR (cu . IWB),
               extMods [GET_TAG (cu . IWB)] . mod);

    decodeInstruction (cu . IWB, & currentInstruction);
  }

//
// Input:
//   cu . IWB
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
                __func__, opDescSTR (), TPR . CA);
    sim_debug (DBG_ADDRMOD, & cpu_dev,
               "%s(Entry): CT_HOLD %o\n",
                __func__, cu . CT_HOLD);

    DCDstruct * i = & currentInstruction;

    word6 Tm = 0;
    word6 Td = 0;

    int iTAG;   // tag of word preceeding an indirect fetch

    directOperandFlag = false;
    characterOperandFlag = false;

    if (i -> info -> flags & NO_TAG) // for instructions line STCA/STCQ
      rTAG = 0;
    else
      rTAG = GET_TAG (cu . IWB);

    int lockupCnt = 0;
#define lockupLimit 4096 // approx. 2 ms

startCA:;

    if (++ lockupCnt > lockupLimit)
      {
        doFault (FAULT_LUF, 0,
                 "Lockup in addrmod");
      }

    Td = GET_TD (rTAG);
    Tm = GET_TM (rTAG);

    if (cu . CT_HOLD)
      {
        sim_debug (DBG_ADDRMOD, & cpu_dev,
                   "%s(startCA): IR mode restart; CT_HOLD %02o\n",
                   __func__, cu . CT_HOLD);
        rTAG = cu . CT_HOLD;
        Td = GET_TD (rTAG);
        Tm = GET_TM (rTAG);
        goto IR_MOD_1;
      }
    sim_debug (DBG_ADDRMOD, & cpu_dev,
               "%s(startCA): TAG=%02o(%s) Tm=%o Td=%o\n",
               __func__, rTAG, getModString (rTAG), Tm, Td);

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
                __func__, GET_TM (rTAG));
    sim_err ("(startCA): unknown Tm\n");


        //! Register modification. Fig 6-3
    R_MOD:;
      {
        if (Td == 0) // TPR.CA = address from opcode
          {
            //updateIWB (identity) // known that Td is 0.
            return SCPE_OK;
          }

        word18 Cr = getCr (Td);

        sim_debug (DBG_ADDRMOD, & cpu_dev, "R_MOD: Cr=%06o\n", Cr);

        if (directOperandFlag)
          {
            sim_debug (DBG_ADDRMOD, & cpu_dev,
                       "R_MOD: directOperand = %012llo\n", directOperand);

            //updateIWB (identity) // known that rTag is DL or DU
            return SCPE_OK;
          }

        if (cu . rpt || cu . rd)
          {
            word6 Td = GET_TD (i -> tag);
            uint Xn = X (Td);  // Get Xn of next instruction
            TPR . CA = rX [Xn];
            if (i -> a)
              {
                word3 PRn = (i -> address >> 15) & MASK3;
                TPR . CA += PR [PRn] . WORDNO;
                TPR . CA &= AMASK;
              }
          }
        else
          {
            TPR . CA  += Cr;
            TPR . CA  &= MASK18;   // keep to 18-bits
          }
        sim_debug (DBG_ADDRMOD, & cpu_dev, "R_MOD: TPR.CA=%06o\n", TPR . CA);

        // If repeat, the indirection chain is limited, so it is not necessary
        // to clear the tag; the delta code later on needs the tag to know
        // which X register to update
        if (! (cu . rpt || cu . rd))
          updateIWB (TPR . CA, 0); // Known to be 0 or ,n
        return SCPE_OK;
      } // R_MOD


        //! Figure 6-4. Register Then Indirect Modification Flowchart
    RI_MOD:;
      {
        sim_debug (DBG_ADDRMOD, & cpu_dev, "RI_MOD: Td=%o\n", Td);

        if (Td == TD_DU || Td == TD_DL)
          doFault (FAULT_IPR, ill_mod,
                   "RI_MOD: Td == TD_DU || Td == TD_DL");

        word18 tmpCA = TPR . CA;

        if (Td != 0)
          {
            word18 Cr = getCr (Td);  // C(r)

            // We don''t need to worry about direct operand here, since du
            // and dl are disallowed above

            sim_debug (DBG_ADDRMOD, & cpu_dev,
                       "RI_MOD: Cr=%06o tmpCA(Before)=%06o\n", Cr, tmpCA);

            if (cu . rpt || cu . rd)
              {
                 word6 Td = GET_TD (i -> tag);
                 uint Xn = X (Td);  // Get Xn of next instruction
                 tmpCA = rX [Xn];
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

        updateIWB (TPR . CA, TM_RI | TD_N);

        // in case it turns out to be a ITS/ITP
        iTAG = rTAG;

        word36 indword;
        Read (tmpCA, & indword, INDIRECT_WORD_FETCH, i -> a); //TM_RI);

        // "In the case of RI modification, only one indirect reference is made
        // per repeated execution. The TAG field of the indirect word is not
        // interpreted.  The indirect word is treated as though it had R
        // modification with R = N."

        if (cu . rpt || cu . rd)
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
                TPR . CA = tmpCA;
                doFault (FAULT_F2, 0, "RI_MOD: IT_F2 (0)");
              }
            if (GET_TD (GET_TAG(indword)) == IT_F3)
              {
                TPR . CA = tmpCA;
                doFault (FAULT_F3, 0, "RI_MOD: IT_F3");
              }
          }

        if (ISITP (indword) || ISITS (indword))
          {
            doITSITP (tmpCA, indword, iTAG, & rTAG);
          }
        else
          {
            TPR . CA = GETHI (indword);
            rTAG = GET_TAG (indword);
            rY = TPR . CA;
          }

        sim_debug (DBG_ADDRMOD, & cpu_dev,
                   "RI_MOD: indword=%012llo TPR.CA=%06o rTAG=%02o\n",
                   indword, TPR . CA, rTAG);
        // If repeat, the indirection chain is limited, so it is not needed
        // to clear the tag; the delta code later on needs the tag to know
        // which X register to update
        if (! (cu . rpt || cu . rd))
          updateIWB (TPR . CA, rTAG);
        goto startCA;
      } // RI_MOD

        //! Figure 6-5. Indirect Then Register Modification Flowchart
    IR_MOD:;
      {
        cu . CT_HOLD = rTAG;

        sim_debug (DBG_ADDRMOD, & cpu_dev,
                   "IR_MOD: CT_HOLD=%o %o\n", cu . CT_HOLD, Td);

        IR_MOD_1:

        if (++ lockupCnt > lockupLimit)
          {
            doFault (FAULT_LUF, 0,
                     "Lockup in addrmod IR mode");
          }

        sim_debug (DBG_ADDRMOD, & cpu_dev,
                   "IR_MOD: fetching indirect word from %06o\n", TPR . CA);

        // in case it turns out to be a ITS/ITP
        iTAG = rTAG;

        word36 indword;
        word18 saveCA = TPR . CA;
        Read (TPR . CA, & indword, INDIRECT_WORD_FETCH, i -> a);

        if (ISITP (indword) || ISITS (indword))
          {
            doITSITP (TPR . CA, indword, iTAG, & rTAG);
          }
        else
          {
            TPR.CA = GETHI(indword);
            rY = TPR.CA;
            rTAG = GET_TAG (indword);
          }

        Td = GET_TD(rTAG);
        Tm = GET_TM(rTAG);

        // IR_MOD_2:;

        sim_debug (DBG_ADDRMOD, & cpu_dev,
                   "IR_MOD1: indword=%012llo TPR.CA=%06o Tm=%o Td=%02o (%s)\n",
                   indword, TPR . CA, Tm, Td, getModString (GET_TAG (indword)));

        switch (Tm)
          {
            case TM_IT:
              {
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IR_MOD(TM_IT): Td=%02o => %02o\n",
                           Td, cu . CT_HOLD);
                if (Td == IT_F2 || Td == IT_F3)
                {
                    // Abort. FT2 or 3
                    switch (Td)
                    {
                        case IT_F2:
                            TPR . CA = saveCA;
                            doFault (FAULT_F2, 0, "TM_IT: IT_F2 (1)");

                        case IT_F3:
                            TPR . CA = saveCA;
                            doFault( FAULT_F3, 0, "TM_IT: IT_F3");
                    }
                }
                // fall through to TM_R
              } // TM_IT

            case TM_R:
              {
                word18 Cr = getCr (GET_TD (cu . CT_HOLD));

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IR_MOD(TM_R): CT_HOLD %o Cr=%06o\n", GET_TD (cu . CT_HOLD), Cr);

                if (directOperandFlag)
                  {
                    TPR . CA += directOperand;
                    TPR . CA &= MASK18;   // keep to 18-bits

                    sim_debug (DBG_ADDRMOD, & cpu_dev,
                               "IR_MOD(TM_R): DO TPR.CA=%06o\n", TPR . CA);

                    updateIWB (TPR . CA, cu . CT_HOLD); // Known to be DL or DU
                  }
                else
                  {
                    TPR . CA += Cr;
                    TPR . CA &= MASK18;   // keep to 18-bits

                    sim_debug (DBG_ADDRMOD, & cpu_dev,
                               "IR_MOD(TM_R): TPR.CA=%06o\n", TPR . CA);

                    updateIWB (TPR . CA, 0);
                  }
                cu . CT_HOLD = 0;
                return SCPE_OK;
              } // TM_R

            case TM_RI:
              {
                word18 Cr = getCr(Td);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IR_MOD(TM_RI): Td=%o Cr=%06o TPR.CA(Before)=%06o\n",
                           Td, Cr, TPR . CA);

                TPR . CA += Cr;
                TPR . CA &= MASK18;   // keep to 18-bits

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IR_MOD(TM_RI): TPR.CA(After)=%06o\n",
                           TPR . CA);

                updateIWB (TPR . CA, rTAG); // XXX guessing here...
                goto IR_MOD_1;
              } // TM_RI

            case TM_IR:
              {
                updateIWB (TPR . CA, rTAG); // XXX guessing here...
                goto IR_MOD;
              } // TM_IR
          } // Tm

        sim_printf ("%s(IR_MOD): unknown Tm??? %o\n", 
                    __func__, GET_TM (rTAG));
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
sim_printf ("IT_CI [%lld] %05o:%06o %012llo\n", sim_timell (), PPR . PSR, PPR . IC, cu . IWB);
                return SCPE_OK;
               } // IT_CI

            case IT_SC: ///< Sequence character (Td = 12)
              {
sim_printf ("IT_SC [%lld] %05o:%06o %012llo\n", sim_timell (), PPR . PSR, PPR . IC, cu . IWB);
                return SCPE_OK;
              } // IT_SC

            case IT_SCR: // Sequence character reverse (Td = 5)
              {
sim_printf ("IT_SCR [%lld] %05o:%06o %012llo\n", sim_timell (), PPR . PSR, PPR . IC, cu . IWB);
                return SCPE_OK;
              } // IT_SCR

            case IT_I: // Indirect (Td = 11)
              {
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_I): reading indirect word from %06o\n",
                           TPR.CA);

                word36 indword;
                Read (TPR . CA, & indword, INDIRECT_WORD_FETCH, 0);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_I): indword=%012llo\n",
                           indword);

                TPR . CA = GET_ADDR (indword);

                updateIWB (TPR . CA, 0); // XXX guessing here...

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
                           TPR . CA);

                word18 saveCA = TPR . CA;
                word36 indword;
                Read (TPR . CA, & indword, OPERAND_READ, i -> a);

                tally = GET_TALLY (indword); // 12-bits
                delta = GET_DELTA (indword); // 6-bits
                Yi = GETHI (indword);        // from where data live

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_AD): indword=%012llo\n",
                           indword);
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_AD): address:%06o tally:%04o delta:%03o\n",
                           Yi, tally, delta);

                TPR . CA = Yi;
                word18 computedAddress = TPR . CA;

                Yi += delta;
                Yi &= MASK18;

                tally -= 1;
                tally &= 07777; // keep to 12-bits
                SCF(tally == 0, cu . IR, I_TALLY);

                indword = (word36) (((word36) Yi << 18) |
                                    (((word36) tally & 07777) << 6) |
                                    delta);
                Write (saveCA, indword, OPERAND_STORE, i -> a);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_AD): wrote tally word %012llo to %06o\n",
                            indword, saveCA);

                TPR . CA = computedAddress;

                updateIWB (TPR . CA, 0); // XXX guessing here...

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

                word18 saveCA = TPR . CA;
                word36 indword;
                Read (TPR . CA, & indword, OPERAND_READ, i -> a);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SD): reading indirect word from %06o\n",
                           TPR . CA);
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
                TPR.CA = Yi;

                tally += 1;
                tally &= 07777; // keep to 12-bits
                SCF(tally == 0, cu.IR, I_TALLY);

                // write back out indword
                indword = (word36) (((word36) Yi << 18) |
                                    (((word36) tally & 07777) << 6) |
                                    delta);
                Write (saveCA, indword, OPERAND_STORE, i -> a);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_SD): wrote tally word %012llo to %06o\n",
                           indword, saveCA);


                TPR . CA = Yi;
                updateIWB (TPR . CA, 0); // XXX guessing here...

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
                           TPR . CA);

                word18 saveCA = TPR . CA;
                word36 indword;
                Read (TPR . CA, & indword, OPERAND_READ, i -> a);

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
                TPR.CA = Yi;

                tally += 1;
                tally &= 07777; // keep to 12-bits
                SCF(tally == 0, cu.IR, I_TALLY);

                // write back out indword

                indword = (word36) (((word36) TPR . CA << 18) |
                                    ((word36) tally << 6) |
                                    junk);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_DI): writing indword=%012llo to "
                           "addr %06o\n",
                           indword, saveCA);

                Write (saveCA, indword, OPERAND_STORE, i -> a);

                TPR . CA = Yi;
                updateIWB (TPR . CA, 0); // XXX guessing here...

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

                word18 saveCA = TPR . CA;

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_ID): fetching indirect word from %06o\n",
                           TPR . CA);

                word36 indword;
                Read (TPR . CA, & indword, OPERAND_READ, i -> a);

                Yi = GETHI (indword);
                tally = GET_TALLY (indword); // 12-bits
                // get tag field, but ignore it
                word6 junk = GET_TAG (indword);
                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_ID): indword=%012llo Yi=%06o tally=%04o\n",
                           indword, Yi, tally);

                TPR . CA = Yi;
                word18 computedAddress = TPR . CA;

                Yi += 1;
                Yi &= MASK18;

                tally -= 1;
                tally &= 07777; // keep to 12-bits
                SCF(tally == 0, cu.IR, I_TALLY);

                // write back out indword
                indword = (word36) (((word36) Yi << 18) |
                                    ((word36) tally << 6) |
                                    junk);

                sim_debug (DBG_ADDRMOD, & cpu_dev,
                           "IT_MOD(IT_ID): writing indword=%012llo to "
                           "addr %06o\n",
                           indword, saveCA);

                Write (saveCA, indword, OPERAND_STORE, i->a);

                TPR . CA = computedAddress;
                updateIWB (TPR . CA, 0); // XXX guessing here...

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
                           TPR . CA);

                word18 saveCA = TPR . CA;
                word36 indword;
                Read (TPR . CA, & indword, OPERAND_READ, i -> a);

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

                TPR.CA = Yi;

                tally += 1;
                tally &= 07777; // keep to 12-bits
                SCF(tally == 0, cu.IR, I_TALLY);

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

                TPR.CA = YiSafe2;

                rTAG = idwtag;
                Tm = GET_TM(rTAG);

                if (Tm != TM_IT)
                  {
                    rTAG = idwtag & 0x70; // force R to 0
                  }
                updateIWB (TPR . CA, rTAG);
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
                           TPR . CA);

                word18 saveCA = TPR . CA;
                word36 indword;
                Read (TPR . CA, & indword, OPERAND_READ, i -> a);

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
                SCF(tally == 0, cu . IR, I_TALLY);

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
                TPR.CA = YiSafe;

                rTAG = idwtag;
                Tm = GET_TM(rTAG);

                if (Tm != TM_IT)
                  {
                    rTAG = idwtag & 0x70; // force R to 0
                  }
                updateIWB (TPR . CA, rTAG);
                goto startCA;
              } // IT_IDC
          } // Td
        sim_printf ("IT_MOD/Td how did we get here?\n");
        return SCPE_OK;
     } // IT_MOD
  } // doComputedAddressFormation


