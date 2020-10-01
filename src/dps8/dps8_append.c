/*
 Copyright 2012-2016 by Harry Reed
 Copyright 2013-2017 by Charles Anthony
 Copyright 2017 by Michal Tomek

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

/**
 * \file dps8_append.c
 * \project dps8
 * \date 10/28/12
 * \copyright Copyright (c) 2012 Harry Reed. All rights reserved.
*/

#include <stdio.h>
#include "dps8.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_iom.h"
//#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_append.h"
#include "dps8_addrmods.h"
#include "dps8_utils.h"
#include "threadz.h"

#define DBG_CTR cpu.cycleCnt

/**
 * The appending unit ...
 */

#ifdef TESTING
#define DBG_CTR cpu.cycleCnt
#define DBGAPP(...) sim_debug (DBG_APPENDING, & cpu_dev, __VA_ARGS__)
#else
#define DBGAPP(...)
#endif

#if 0
void set_apu_status (apuStatusBits status)
  {
#if 1
    word12 FCT = cpu.cu.APUCycleBits & MASK3;
    cpu.cu.APUCycleBits = (status & 07770) | FCT;
#else
    cpu.cu.PI_AP = 0;
    cpu.cu.DSPTW = 0;
    cpu.cu.SDWNP = 0;
    cpu.cu.SDWP  = 0;
    cpu.cu.PTW   = 0;
    cpu.cu.PTW2  = 0;
    cpu.cu.FAP   = 0;
    cpu.cu.FANP  = 0;
    cpu.cu.FABS  = 0;
    switch (status)
      {
        case apuStatus_PI_AP:
          cpu.cu.PI_AP = 1;
          break;
        case apuStatus_DSPTW:
        case apuStatus_MDSPTW: // XXX this doesn't seem like the right solution.
                               // XXX there is a MDSPTW bit in the APU history
                               // register, but not in the CU.
          cpu.cu.DSPTW = 1;
          break;
        case apuStatus_SDWNP:
          cpu.cu.SDWNP = 1;
          break;
        case apuStatus_SDWP:
          cpu.cu.SDWP  = 1;
          break;
        case apuStatus_PTW:
        case apuStatus_MPTW: // XXX this doesn't seem like the right solution.
                             // XXX there is a MPTW bit in the APU history
                             // XXX register, but not in the CU.
          cpu.cu.PTW   = 1;
          break;
        case apuStatus_PTW2:
          cpu.cu.PTW2  = 1;
          break;
        case apuStatus_FAP:
          cpu.cu.FAP   = 1;
          break;
        case apuStatus_FANP:
          cpu.cu.FANP  = 1;
          break;
        case apuStatus_FABS:
          cpu.cu.FABS  = 1;
          break;
      }
#endif
  }
#endif

#ifdef TESTING
#ifdef WAM
static char *str_sdw (char * buf, sdw_s *SDW);
#endif
#endif

//
// 
//  The Use of Bit 29 in the Instruction Word
//  The reader is reminded that there is a preliminary step of loading TPR.CA
//  with the ADDRESS field of the instruction word during instruction decode.
//  If bit 29 of the instruction word is set to 1, modification by pointer
//  register is invoked and the preliminary step is executed as follows:
//  1. The ADDRESS field of the instruction word is interpreted as shown in
//  Figure 6-7 below.
//  2. C(PRn.SNR) -> C(TPR.TSR)
//  3. maximum of ( C(PRn.RNR), C(TPR.TRR), C(PPR.PRR) ) -> C(TPR.TRR)
//  4. C(PRn.WORDNO) + OFFSET -> C(TPR.CA) (NOTE: OFFSET is a signed binary
//  number.)
//  5. C(PRn.BITNO) -> TPR.BITNO
//


// Define this to do error detection on the PTWAM table.
// Useful if PTWAM reports an error message, but it slows the emulator
// down 50%

#ifdef do_selftestPTWAM
static void selftest_ptwaw (void)
  {
    int usages[N_WAM_ENTRIES];
    for (int i = 0; i < N_WAM_ENTRIES; i ++)
      usages[i] = -1;

    for (int i = 0; i < N_WAM_ENTRIES; i ++)
      {
        ptw_s * p = cpu.PTWAM + i;
        if (p->USE > N_WAM_ENTRIES - 1)
          sim_printf ("PTWAM[%d].USE is %d; > %d!\n",
                      i, p->USE, N_WAM_ENTRIES - 1);
        if (usages[p->USE] != -1)
          sim_printf ("PTWAM[%d].USE is equal to PTWAM[%d].USE; %d\n",
                      i, usages[p->USE], p->USE);
        usages[p->USE] = i;
      }
    for (int i = 0; i < N_WAM_ENTRIES; i ++)
      {
        if (usages[i] == -1)
          sim_printf ("No PTWAM had a USE of %d\n", i);
      }
  }
#endif

/**
 * implement ldbr instruction
 */

void do_ldbr (word36 * Ypair)
  {
    CPTUR (cptUseDSBR);
#ifdef WAM
    if (cpu.switches.enable_wam) 
      {
        if (cpu.cu.SD_ON) 
          {
            // If SDWAM is enabled, then
            //   0 -> C(SDWAM(i).FULL) for i = 0, 1, ..., 15
            //   i -> C(SDWAM(i).USE) for i = 0, 1, ..., 15
            for (uint i = 0; i < N_WAM_ENTRIES; i ++)
              {
                cpu.SDWAM[i].FE = 0;
                cpu.SDWAM[i].USE = (word4) i;
              }
          }

        if (cpu.cu.PT_ON) 
          {
            // If PTWAM is enabled, then
            //   0 -> C(PTWAM(i).FULL) for i = 0, 1, ..., 15
            //   i -> C(PTWAM(i).USE) for i = 0, 1, ..., 15
            for (uint i = 0; i < N_WAM_ENTRIES; i ++)
              {
                cpu.PTWAM[i].FE = 0;
                cpu.PTWAM[i].USE = (word4) i;
              }
#ifdef do_selftestPTWAM
            selftest_ptwaw ();
#endif
          }
      }
#else
    cpu.SDW0.FE = 0;
    cpu.SDW0.USE = 0;
    cpu.PTW0.FE = 0;
    cpu.PTW0.USE = 0;
#endif // WAM

    // If cache is enabled, reset all cache column and level full flags
    // XXX no cache

    // C(Y-pair) 0,23 -> C(DSBR.ADDR)
    cpu.DSBR.ADDR = (Ypair[0] >> (35 - 23)) & PAMASK;

    // C(Y-pair) 37,50 -> C(DSBR.BOUND)
    cpu.DSBR.BND = (Ypair[1] >> (71 - 50)) & 037777;

    // C(Y-pair) 55 -> C(DSBR.U)
    cpu.DSBR.U = (Ypair[1] >> (71 - 55)) & 01;

    // C(Y-pair) 60,71 -> C(DSBR.STACK)
    cpu.DSBR.STACK = (Ypair[1] >> (71 - 71)) & 07777;
    DBGAPP ("ldbr 0 -> SDWAM/PTWAM[*].F, i -> SDWAM/PTWAM[i].USE, "
            "DSBR.ADDR %088o, DSBR.BND %05o, DSBR.U %o, DSBR.STACK %04o\n",
            cpu.DSBR.ADDR, cpu.DSBR.BND, cpu.DSBR.U, cpu.DSBR.STACK); 
  }


    


#ifdef WAM

static sdw_s * fetch_sdw_from_sdwam (word15 segno)
  {
    DBGAPP ("%s(0):segno=%05o\n", __func__, segno);
    
    if (! cpu.switches.enable_wam)
      {
        DBGAPP ("%s(0): WAM disabled\n", __func__);
        return NULL;
      }

    if (! cpu.cu.SD_ON)
      {
        DBGAPP ("%s(0): SDWAM disabled\n", __func__);
        return NULL;
      }

    int nwam = N_WAM_ENTRIES;
    for (int _n = 0; _n < nwam; _n++)
      {
        // make certain we initialize SDWAM prior to use!!!
        if (cpu.SDWAM[_n].FE && segno == cpu.SDWAM[_n].POINTER)
          {
            DBGAPP ("%s(1):found match for segno %05o "
                    "at _n=%d\n",
                     __func__, segno, _n);
            
            cpu.cu.SDWAMM = 1;
            cpu.SDWAMR = (word4) _n;
            cpu.SDW = & cpu.SDWAM[_n];
            
            // If the SDWAM match logic circuitry indicates a hit, all usage
            // counts (SDWAM.USE) greater than the usage count of the register
            // hit are decremented by one, the usage count of the register hit
            // is set to 15, and the contents of the register hit are read out
            // into the address preparation circuitry. 

            for (int _h = 0; _h < nwam; _h++)
              {
                if (cpu.SDWAM[_h].USE > cpu.SDW->USE)
                  cpu.SDWAM[_h].USE -= 1;
              }
            cpu.SDW->USE = N_WAM_ENTRIES - 1;
 
            char buf[256];
#ifdef TESTING
            DBGAPP ("%s(2):SDWAM[%d]=%s\n",
                     __func__, _n, str_sdw (buf, cpu.SDW));
#endif

            return cpu.SDW;
          }
      }
    DBGAPP ("%s(3):SDW for segment %05o not found in SDWAM\n",
            __func__, segno);
    cpu.cu.SDWAMM = 0;
    return NULL;    // segment not referenced in SDWAM
  }
#endif // WAM


#ifdef WAM
static char *str_sdw (char * buf, sdw_s *SDW)
  {
    if (! SDW->FE)
      sprintf (buf, "*** SDW Uninitialized ***");
    else
      sprintf (buf,
               "ADDR:%06o R1:%o R2:%o R3:%o BOUND:%o R:%o E:%o W:%o P:%o "
               "U:%o G:%o C:%o CL:%o DF:%o FC:%o POINTER=%o USE=%d",
               SDW->ADDR,
               SDW->R1,
               SDW->R2,
               SDW->R3,
               SDW->BOUND,
               SDW->R,
               SDW->E,
               SDW->W,
               SDW->P,
               SDW->U,
               SDW->G,
               SDW->C,
               SDW->EB,
               SDW->DF,
               SDW->FC,
               SDW->POINTER,
               SDW->USE);
    return buf;
  }
#endif // WAM

#ifdef WAM

/**
 * dump SDWAM...
 */

t_stat dump_sdwam (void)
  {
    char buf[256];
    for (int _n = 0; _n < N_WAM_ENTRIES; _n++)
      {
        sdw_s *p = & cpu.SDWAM[_n];
        
        if (p->FE)
          sim_printf ("SDWAM n:%d %s\n", _n, str_sdw (buf, p));
      }
    return SCPE_OK;
  }

#endif


#ifdef WAM
static ptw_s * fetch_ptw_from_ptwam (word15 segno, word18 CA)
  {
    if ((! cpu.switches.enable_wam) || ! cpu.cu.PT_ON)
      {
        DBGAPP ("%s: PTWAM disabled\n", __func__);
        return NULL;
      }
    
    int nwam = N_WAM_ENTRIES;
    for (int _n = 0; _n < nwam; _n++)
      {
        if (cpu.PTWAM[_n].FE && ((CA >> 6) & 07760) == cpu.PTWAM[_n].PAGENO &&
            cpu.PTWAM[_n].POINTER == segno)   //_initialized)
          {
            DBGAPP ("%s: found match for segno=%o pageno=%o "
                    "at _n=%d\n",
                    __func__, segno, cpu.PTWAM[_n].PAGENO, _n);
            cpu.cu.PTWAMM = 1;
            cpu.PTWAMR = (word4) _n;
            cpu.PTW = & cpu.PTWAM[_n];
            
            // If the PTWAM match logic circuitry indicates a hit, all usage
            // counts (PTWAM.USE) greater than the usage count of the register
            // hit are decremented by one, the usage count of the register hit
            // is set to 15, and the contents of the register hit are read out
            // into the address preparation circuitry.

            for (int _h = 0; _h < nwam; _h++)
              {
                if (cpu.PTWAM[_h].USE > cpu.PTW->USE)
                  cpu.PTWAM[_h].USE -= 1; //PTW->USE -= 1;
              }
            cpu.PTW->USE = N_WAM_ENTRIES - 1;
#ifdef do_selftestPTWAM
            selftest_ptwaw ();
#endif
            DBGAPP ("%s: ADDR 0%06o U %o M %o F %o FC %o\n",
                    __func__, cpu.PTW->ADDR, cpu.PTW->U, cpu.PTW->M,
                    cpu.PTW->DF, cpu.PTW->FC);
            return cpu.PTW;
          }
      }

    cpu.cu.PTWAMM = 0;
    return NULL;    // page not referenced in PTWAM
  }
#endif // WAM




/**
 * Is the instruction a SToRage OPeration ?
 */

#ifndef QUIET_UNUSED
static char *str_access_type (MemoryAccessType accessType)
  {
    switch (accessType)
      {
        case UnknownMAT:        return "Unknown";
        case OperandRead:       return "OperandRead";
        case OperandWrite:      return "OperandWrite";
        default:                return "???";
      }
  }
#endif

#ifndef QUIET_UNUSED
static char *str_acv (_fault_subtype acv)
  {
    switch (acv)
      {
        case ACV0:  return "Illegal ring order (ACV0=IRO)";
        case ACV1:  return "Not in execute bracket (ACV1=OEB)";
        case ACV2:  return "No execute permission (ACV2=E-OFF)";
        case ACV3:  return "Not in read bracket (ACV3=ORB)";
        case ACV4:  return "No read permission (ACV4=R-OFF)";
        case ACV5:  return "Not in write bracket (ACV5=OWB)";
        case ACV6:  return "No write permission (ACV6=W-OFF)";
        case ACV7:  return "Call limiter fault (ACV7=NO GA)";
        case ACV8:  return "Out of call brackets (ACV8=OCB)";
        case ACV9:  return "Outward call (ACV9=OCALL)";
        case ACV10: return "Bad outward call (ACV10=BOC)";
        case ACV11: return "Inward return (ACV11=INRET) XXX ??";
        case ACV12: return "Invalid ring crossing (ACV12=CRT)";
        case ACV13: return "Ring alarm (ACV13=RALR)";
        case ACV14: return "Associative memory error XXX ??";
        case ACV15: return "Out of segment bounds (ACV15=OOSB)";
        //case ACDF0: return "Directed fault 0";
        //case ACDF1: return "Directed fault 1";
        //case ACDF2: return "Directed fault 2";
        //case ACDF3: return "Directed fault 3";
        default:
            break;
      }
  return "unhandled acv in str_acv";
  }
#endif

// Translate a segno:offset to a absolute address.
// Return 0 if successful.

int dbgLookupAddress (word18 segno, word18 offset, word24 * finalAddress,
                      char * * msg)
  {
    // Local copies so we don't disturb machine state

    ptw_s PTW1;
    sdw_s SDW1;

   if (2u * segno >= 16u * (cpu.DSBR.BND + 1u))
     {
       if (msg)
         * msg = "DSBR boundary violation.";
       return 1;
     }

    if (cpu.DSBR.U == 0)
      {
        // fetch_dsptw

        word24 y1 = (2 * segno) % 1024;
        word24 x1 = (2 * segno) / 1024; // floor

        word36 PTWx1;
        core_read ((cpu.DSBR.ADDR + x1) & PAMASK, & PTWx1, __func__);
        
        PTW1.ADDR = GETHI (PTWx1);
        PTW1.U = TSTBIT (PTWx1, 9);
        PTW1.M = TSTBIT (PTWx1, 6);
        PTW1.DF = TSTBIT (PTWx1, 2);
        PTW1.FC = PTWx1 & 3;
    
        if (! PTW1.DF)
          {
            if (msg)
              * msg = "!PTW0.F";
            return 2;
          }

        // fetch_psdw

        y1 = (2 * segno) % 1024;
    
        word36 SDWeven, SDWodd;
    
        core_read2 (((((word24)PTW1. ADDR & 0777760) << 6) + y1) & PAMASK, 
                    & SDWeven, & SDWodd, __func__);
    
        // even word
        SDW1.ADDR = (SDWeven >> 12) & 077777777;
        SDW1.R1 = (SDWeven >> 9) & 7;
        SDW1.R2 = (SDWeven >> 6) & 7;
        SDW1.R3 = (SDWeven >> 3) & 7;
        SDW1.DF = TSTBIT (SDWeven, 2);
        SDW1.FC = SDWeven & 3;
    
        // odd word
        SDW1.BOUND = (SDWodd >> 21) & 037777;
        SDW1.R = TSTBIT (SDWodd, 20);
        SDW1.E = TSTBIT (SDWodd, 19);
        SDW1.W = TSTBIT (SDWodd, 18);
        SDW1.P = TSTBIT (SDWodd, 17);
        SDW1.U = TSTBIT (SDWodd, 16);
        SDW1.G = TSTBIT (SDWodd, 15);
        SDW1.C = TSTBIT (SDWodd, 14);
        SDW1.EB = SDWodd & 037777;
      }
    else // ! DSBR.U
      {
        // fetch_nsdw

        word36 SDWeven, SDWodd;
        
        core_read2 ((cpu.DSBR.ADDR + 2 * segno) & PAMASK, 
                    & SDWeven, & SDWodd, __func__);
        
        // even word
        SDW1.ADDR = (SDWeven >> 12) & 077777777;
        SDW1.R1 = (SDWeven >> 9) & 7;
        SDW1.R2 = (SDWeven >> 6) & 7;
        SDW1.R3 = (SDWeven >> 3) & 7;
        SDW1.DF = TSTBIT (SDWeven, 2);
        SDW1.FC = SDWeven & 3;
        
        // odd word
        SDW1.BOUND = (SDWodd >> 21) & 037777;
        SDW1.R = TSTBIT (SDWodd, 20);
        SDW1.E = TSTBIT (SDWodd, 19);
        SDW1.W = TSTBIT (SDWodd, 18);
        SDW1.P = TSTBIT (SDWodd, 17);
        SDW1.U = TSTBIT (SDWodd, 16);
        SDW1.G = TSTBIT (SDWodd, 15);
        SDW1.C = TSTBIT (SDWodd, 14);
        SDW1.EB = SDWodd & 037777;
    
      }

    if (SDW1.DF == 0)
      {
        if (msg)
          * msg = "!SDW0.F != 0";
        return 3;
      }

    if (((offset >> 4) & 037777) > SDW1.BOUND)
      {
        if (msg)
          * msg = "C(TPR.CA)0,13 > SDW.BOUND";
        return 4;
      }

    // is segment C(TPR.TSR) paged?
    if (SDW1.U)
      {
        * finalAddress = (SDW1.ADDR + offset) & PAMASK;
      }
    else
      {
        // fetch_ptw
        word24 y2 = offset % 1024;
        word24 x2 = (offset) / 1024; // floor
    
        word36 PTWx2;
    
        core_read ((SDW1.ADDR + x2) & PAMASK, & PTWx2, __func__);
    
        PTW1.ADDR = GETHI (PTWx2);
        PTW1.U = TSTBIT (PTWx2, 9);
        PTW1.M = TSTBIT (PTWx2, 6);
        PTW1.DF = TSTBIT (PTWx2, 2);
        PTW1.FC = PTWx2 & 3;

        if (! PTW1.DF)
          {
            if (msg)
              * msg = "!PTW0.F";
            return 5;
          }

        y2 = offset % 1024;
    
        * finalAddress = ((((word24)PTW1.ADDR & 0777760) << 6) + y2) & PAMASK;
      }
    if (msg)
      * msg = "";
    return 0;
  }

#ifdef CTRACE
word36 fast_append (uint cpun, uint segno, uint offset)
  {
    // prds is segment 072
    //uint segno = 072;
    word24 dsbr_addr = cpus[cpun].DSBR.ADDR;
//printf ("dsbr %08o\n", dsbr_addr);
    if (! dsbr_addr)
      return 1;
    // punt on unpaged ds
    if (cpus[cpun].DSBR.U)
      return 2;
    word24 x1 = (2u * segno) / 1024u; // floor
    word36 PTWx1 = M[dsbr_addr + x1];
//printf ("PTRx1 %012llo\n", PTWx1);
    if (TSTBIT (PTWx1, 2) == 0) // df - page not in memory
      return 3;
    word24 y1 = (2 * segno) % 1024;
    word36 SDWeven = M [(GETHI (PTWx1) << 6) + y1];
    word36 SDWodd = M [(GETHI (PTWx1) << 6) + y1 + 1];
//printf ("SDWeven %012llo\n", SDWeven);
//printf ("SDWodd %012llo\n", SDWodd);
    if (TSTBIT (SDWeven, 2) == 0)
      return 4;
    if (TSTBIT (SDWodd, 16)) // .U
      return 5;
    word24 sdw_addr = (SDWeven >> 12) & 077777777;
//printf ("sdw_addr %08o\n", sdw_addr);
    word24 x2 = (offset) / 1024; // floor

    word24 y2 = offset % 1024;
    word36 PTWx2 = M[sdw_addr + x2];
    word24 PTW1_addr= GETHI (PTWx2);
    word1 PTW0_U = TSTBIT (PTWx2, 9);
    if (! PTW0_U)
      return 6;
    word24 finalAddress = ((((word24)PTW1_addr & 0777760) << 6) + y2) & PAMASK;
    return M[finalAddress];
    //return (word24) (GETHI (PTWx1) << 6);
  }
#endif


