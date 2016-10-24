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
#include "dps8_cpu.h"
#include "dps8_utils.h"
#include "dps8_append.h"

/**
 * \brief the appending unit ...
 */

#if 0
typedef enum apuStatusBits
  {
    apuStatus_PI_AP = 1llu < (36 - 24),
    apuStatus_DSPTW = 1llu < (36 - 25),
    apuStatus_SDWNP = 1llu < (36 - 26),
    apuStatus_SDWP  = 1llu < (36 - 27),
    apuStatus_PTW   = 1llu < (36 - 28),
    apuStatus_PTW2  = 1llu < (36 - 29),
    apuStatus_FAP   = 1llu < (36 - 30),
    apuStatus_FANP  = 1llu < (36 - 31),
    apuStatus_FABS  = 1llu < (36 - 32),
  } apuStatusBits;

static const apuStatusBits apuStatusAll =
    apuStatus_PI_AP |
    apuStatus_DSPTW |
    apuStatus_SDWNP |
    apuStatus_SDWP  |
    apuStatus_PTW   |
    apuStatus_PTW2  |
    apuStatus_FAP   |
    apuStatus_FANP  |
    apuStatus_FABS;
#endif

void setAPUStatus (apuStatusBits status)
  {
#if 1
    word12 FCT = cpu.cu.APUCycleBits & MASK3;
    cpu.cu.APUCycleBits = (status & 07770) | FCT;
#else
    cpu . cu . PI_AP = 0;
    cpu . cu . DSPTW = 0;
    cpu . cu . SDWNP = 0;
    cpu . cu . SDWP  = 0;
    cpu . cu . PTW   = 0;
    cpu . cu . PTW2  = 0;
    cpu . cu . FAP   = 0;
    cpu . cu . FANP  = 0;
    cpu . cu . FABS  = 0;
    switch (status)
      {
        case apuStatus_PI_AP:
          cpu . cu . PI_AP = 1;
          break;
        case apuStatus_DSPTW:
        case apuStatus_MDSPTW: // XXX this doesn't seem like the right solution.
                               // XXX there is a MDSPTW bit in the APU history
                               // register, but not in the CU.
          cpu . cu . DSPTW = 1;
          break;
        case apuStatus_SDWNP:
          cpu . cu . SDWNP = 1;
          break;
        case apuStatus_SDWP:
          cpu . cu . SDWP  = 1;
          break;
        case apuStatus_PTW:
        case apuStatus_MPTW: // XXX this doesn't seem like the right solution.
                             // XXX there is a MPTW bit in the APU history
                             // XXX register, but not in the CU.
          cpu . cu . PTW   = 1;
          break;
        case apuStatus_PTW2:
          cpu . cu . PTW2  = 1;
          break;
        case apuStatus_FAP:
          cpu . cu . FAP   = 1;
          break;
        case apuStatus_FANP:
          cpu . cu . FANP  = 1;
          break;
        case apuStatus_FABS:
          cpu . cu . FABS  = 1;
          break;
      }
#endif
  }

#ifndef SPEED
static char *strSDW(_sdw *SDW);
#endif

static enum _appendingUnit_cycle_type appendingUnitCycleType = apuCycle_APPUNKNOWN;

/**

 The Use of Bit 29 in the Instruction Word
 The reader is reminded that there is a preliminary step of loading TPR.CA with the ADDRESS field of the instruction word during instruction decode.
 If bit 29 of the instruction word is set to 1, modification by pointer register is invoked and the preliminary step is executed as follows:
 1. The ADDRESS field of the instruction word is interpreted as shown in Figure 6-7 below.
 2. C(PRn.SNR) → C(TPR.TSR)
 3. maximum of ( C(PRn.RNR), C(TPR.TRR), C(PPR.PRR) ) → C(TPR.TRR)
 4. C(PRn.WORDNO) + OFFSET → C(TPR.CA) (NOTE: OFFSET is a signed binary number.)
 5. C(PRn.BITNO) → TPR.BITNO
 */


void doPtrReg(void)
{
    word3 n = GET_PRN(IWB_IRODD);  // get PRn
    word15 offset = GET_OFFSET(IWB_IRODD);
    
    sim_debug(DBG_APPENDING, &cpu_dev, "doPtrReg(): PR[%o] SNR=%05o RNR=%o WORDNO=%06o BITNO=%02o\n", n, cpu . PAR[n].SNR, cpu . PAR[n].RNR, cpu . PAR[n].WORDNO, GET_PR_BITNO (n));
    cpu . TPR.TSR = cpu . PAR[n].SNR;
    cpu . TPR.TRR = max3(cpu . PAR[n].RNR, cpu . TPR.TRR, cpu . PPR.PRR);
    
    cpu . TPR.CA = (cpu . PAR[n].WORDNO + SIGNEXT15_18(offset)) & 0777777;
    cpu . TPR.TBR = GET_PR_BITNO (n);
    
    set_went_appending ();
    sim_debug(DBG_APPENDING, &cpu_dev, "doPtrReg(): n=%o offset=%05o TPR.CA=%06o TPR.TBR=%o TPR.TSR=%05o TPR.TRR=%o\n", n, offset, cpu . TPR.CA, cpu . TPR.TBR, cpu . TPR.TSR, cpu . TPR.TRR);
}

// Define this to do error detection on the PTWAM table.
// Useful if PTWAM reports an error message, but it slows the emulator
// down 50%

#ifdef do_selftestPTWAM
static void selftestPTWAM (void)
  {
    int usages [N_WAM_ENTRIES];
    for (int i = 0; i < N_WAM_ENTRIES; i ++)
      usages [i] = -1;

    for (int i = 0; i < N_WAM_ENTRIES; i ++)
      {
        _ptw * p = cpu . PTWAM + i;
        if (p -> USE > N_WAM_ENTRIES - 1)
          sim_printf ("PTWAM[%d].USE is %d; > %d!\n", i, p -> USE, N_WAM_ENTRIES - 1);
        if (usages [p -> USE] != -1)
          sim_printf ("PTWAM[%d].USE is equal to PTWAM[%d].USE; %d\n",
                      i, usages [p -> USE], p -> USE);
        usages [p -> USE] = i;
      }
    for (int i = 0; i < N_WAM_ENTRIES; i ++)
      {
        if (usages [i] == -1)
          sim_printf ("No PTWAM had a USE of %d\n", i);
      }
  }
#endif

/**
 * implement ldbr instruction
 */

void do_ldbr (word36 * Ypair)
  {
#ifndef SPEED
    // XXX is it enabled?

    // If SDWAM is enabled, then
    //   0 → C(SDWAM(i).FULL) for i = 0, 1, ..., 15
    //   i → C(SDWAM(i).USE) for i = 0, 1, ..., 15
    for (uint i = 0; i < N_WAM_ENTRIES; i ++)
      {
        cpu . SDWAM [i] . DF = 0;
        cpu . SDWAM [i] . USE = (word6) i;
      }

    // If PTWAM is enabled, then
    //   0 → C(PTWAM(i).FULL) for i = 0, 1, ..., 15
    //   i → C(PTWAM(i).USE) for i = 0, 1, ..., 15
    for (uint i = 0; i < N_WAM_ENTRIES; i ++)
      {
        cpu . PTWAM [i] . FE = 0;
        cpu . PTWAM [i] . USE = (word6) i;
      }
#ifdef do_selftestPTWAM
    selftestPTWAM ();
#endif
#else
    cpu.SDWAM0.FE = 0;
    cpu.SDWAM0.USE = 0;
    cpu.PTWAM0.FE = 0;
    cpu.PTWAM0.USE = 0;
#endif // SPEED

    // If cache is enabled, reset all cache column and level full flags
    // XXX no cache

    // C(Y-pair) 0,23 → C(DSBR.ADDR)
    cpu . DSBR . ADDR = (Ypair [0] >> (35 - 23)) & PAMASK;

    // C(Y-pair) 37,50 → C(DSBR.BOUND)
    cpu . DSBR . BND = (Ypair [1] >> (71 - 50)) & 037777;

    // C(Y-pair) 55 → C(DSBR.U)
    cpu . DSBR . U = (Ypair [1] >> (71 - 55)) & 01;

    // C(Y-pair) 60,71 → C(DSBR.STACK)
    cpu . DSBR . STACK = (Ypair [1] >> (71 - 71)) & 07777;
    sim_debug (DBG_APPENDING, &cpu_dev, "ldbr 0 -> SDWAM/PTWAM[*].F, i -> SDWAM/PTWAM[i].USE, DSBR.ADDR 0%o, DSBR.BND 0%o, DSBR.U 0%o, DSBR.STACK 0%o\n", cpu . DSBR.ADDR, cpu . DSBR.BND, cpu . DSBR.U, cpu . DSBR.STACK); 
    //sim_printf ("ldbr %012llo %012llo\n", Ypair [0], Ypair [1]);
    //sim_printf ("ldbr DSBR.ADDR %08o, DSBR.BND %05o, DSBR.U %o, DSBR.STACK %04o\n", cpu . DSBR.ADDR, cpu . DSBR.BND, cpu . DSBR.U, cpu . DSBR.STACK); 
  }

/**
 * implement sdbr instruction
 */

void do_sdbr (word36 * Ypair)
  {
    // C(DSBR.ADDR) → C(Y-pair) 0,23
    // 00...0 → C(Y-pair) 24,36
    Ypair [0] = ((word36) (cpu . DSBR . ADDR & PAMASK)) << (35 - 23); 

    // C(DSBR.BOUND) → C(Y-pair) 37,50
    // 0000 → C(Y-pair) 51,54
    // C(DSBR.U) → C(Y-pair) 55
    // 000 → C(Y-pair) 56,59
    // C(DSBR.STACK) → C(Y-pair) 60,71
    Ypair [1] = ((word36) (cpu . DSBR . BND & 037777)) << (71 - 50) |
                ((word36) (cpu . DSBR . U & 1)) << (71 - 55) |
                ((word36) (cpu . DSBR . STACK & 07777)) << (71 - 71);
    //sim_printf ("sdbr DSBR.ADDR %08o, DSBR.BND %05o, DSBR.U %o, DSBR.STACK %04o\n", cpu . DSBR.ADDR, cpu . DSBR.BND, cpu . DSBR.U, cpu . DSBR.STACK); 
    //sim_printf ("sdbr %012llo %012llo\n", Ypair [0], Ypair [1]);
  }

/**
 * implement camp instruction
 */

void do_camp (UNUSED word36 Y)
  {
    // C(TPR.CA) 16,17 control disabling or enabling the associative memory.
    // This may be done to either or both halves.
    // The full/empty bit of cache PTWAM register is set to zero and the LRU
    // counters are initialized.
    // XXX enable/disable and LRU don't seem to be implemented; punt
    // XXX ticket #1
#ifndef SPEED
    for (uint i = 0; i < N_WAM_ENTRIES; i ++)
      {
        cpu.PTWAM[i].FE = 0;
        cpu.PTWAM[i].USE = (word6) i;
      }
#else
    cpu.PTWAM0.FE = 0;
    cpu.PTWAM0.USE = 0;
#endif
// 8009997-040 A level of the associative memory is disabled if
// C(TPR.CA) 16,17 = 01
// 8009997-040 A level of the associative memory is enabled if
// C(TPR.CA) 16,17 = 10
// Level j is selected to be enabled/disable if
// C(TPR.CA) 10+j = 1; j=1,2,3,4
// All levels are selected to be enabled/disabled if
// C(TPR.CA) 11,14 = 0
    if (cpu.TPR.CA != 0000002 && (cpu.TPR.CA & 3) != 0)
      sim_warn ("CAMP ignores enable/disable %06o\n", cpu.TPR.CA);
  }

/**
 * implement cams instruction
 */

void do_cams (UNUSED word36 Y)
  {
    // The full/empty bit of each SDWAM register is set to zero and the LRU
    // counters are initialized. The remainder of the contents of the registers
    // are unchanged. If the associative memory is disabled, F and LRU are
    // unchanged.
    // C(TPR.CA) 16,17 control disabling or enabling the associative memory.
    // This may be done to either or both halves.
    // XXX enable/disable and LRU don't seem to be implemented; punt
    // XXX ticket #2
#ifndef SPEED
    for (uint i = 0; i < N_WAM_ENTRIES; i ++)
      {
        cpu.SDWAM[i].DF = 0;
        cpu.SDWAM[i].USE = (word6) i;
#ifdef ISOLTS
if (currentRunningCPUnum)
sim_printf ("CAMS cleared it\n");
#endif
      }
#else
    cpu.SDWAM0.FE = 0;
    cpu.SDWAM0.USE = 0;
#endif
// 8009997-040 A level of the associative memory is disabled if
// C(TPR.CA) 16,17 = 01
// 8009997-040 A level of the associative memory is enabled if
// C(TPR.CA) 16,17 = 10
// Level j is selected to be enabled/disable if
// C(TPR.CA) 10+j = 1; j=1,2,3,4
// All levels are selected to be enabled/disabled if
// C(TPR.CA) 11,14 = 0
    if (cpu.TPR.CA != 0000006 && (cpu.TPR.CA & 3) != 0)
      sim_warn ("CAMS ignores enable/disable %06o\n", cpu.TPR.CA);
  }

    
/**
 * fetch descriptor segment PTW ...
 */
// CANFAULT
static void fetchDSPTW(word15 segno)
{
    sim_debug (DBG_APPENDING, & cpu_dev, "fetchDSPTW segno 0%o\n", segno);
    if (2 * segno >= 16 * (cpu . DSBR.BND + 1))
        // generate access violation, out of segment bounds fault
        doFault(FAULT_ACV, (_fault_subtype) {.fault_acv_subtype=ACV15}, "acvFault: fetchDSPTW out of segment bounds fault");
        
    setAPUStatus (apuStatus_DSPTW);

    word24 y1 = (2 * segno) % 1024;
    word24 x1 = (2 * segno - y1) / 1024;

    word36 PTWx1;
    core_read((cpu . DSBR.ADDR + x1) & PAMASK, &PTWx1, __func__);
    
    cpu . PTW0.ADDR = GETHI(PTWx1);
    cpu . PTW0.U = TSTBIT(PTWx1, 9);
    cpu . PTW0.M = TSTBIT(PTWx1, 6);
    cpu . PTW0.DF = TSTBIT(PTWx1, 2);
    cpu . PTW0.FC = PTWx1 & 3;
    
    sim_debug (DBG_APPENDING, & cpu_dev, "fetchDSPTW x1 0%o y1 0%o DSBR.ADDR 0%o PTWx1 0%012llo PTW0: ADDR 0%o U %o M %o F %o FC %o\n", x1, y1, cpu . DSBR.ADDR, PTWx1, cpu . PTW0.ADDR, cpu . PTW0.U, cpu . PTW0.M, cpu . PTW0.DF, cpu . PTW0.FC);
}


/**
 * modify descriptor segment PTW (Set U=1) ...
 */
// CANFAULT
static void modifyDSPTW(word15 segno)
{
    if (2 * segno >= 16 * (cpu . DSBR.BND + 1))
        // generate access violation, out of segment bounds fault
        doFault(FAULT_ACV, (_fault_subtype) {.fault_acv_subtype=ACV15}, "acvFault: modifyDSPTW out of segment bounds fault");

    setAPUStatus (apuStatus_MDSPTW); 

    word24 y1 = (2 * segno) % 1024;
    word24 x1 = (2 * segno - y1) / 1024;
    
    word36 PTWx1;
    core_read((cpu . DSBR.ADDR + x1) & PAMASK, &PTWx1, __func__);
    PTWx1 = SETBIT(PTWx1, 9);
    core_write((cpu . DSBR.ADDR + x1) & PAMASK, PTWx1, __func__);
    
    cpu . PTW0.U = 1;
}


#ifndef SPEED
// XXX SDW0 is the in-core representation of a SDW. Need to have a SDWAM struct as current SDW!!!
static _sdw* fetchSDWfromSDWAM(word15 segno)
{
    sim_debug(DBG_APPENDING, &cpu_dev, "fetchSDWfromSDWAM(0):segno=%05o\n", segno);
    
    int nwam = N_WAM_ENTRIES;
    if (cpu . switches . disable_wam)
    {
        sim_debug(DBG_APPENDING, &cpu_dev, "fetchSDWfromSDWAM(0): SDWAM disabled\n");
        nwam = 1;
        return NULL;
    }
    
    for(int _n = 0 ; _n < nwam ; _n++)
    {
        // make certain we initialize SDWAM prior to use!!!
        //if (SDWAM[_n]._initialized && segno == SDWAM[_n].POINTER)
        if (cpu . SDWAM[_n].DF && segno == cpu . SDWAM[_n].POINTER)
        {
            sim_debug(DBG_APPENDING, &cpu_dev, "fetchSDWfromSDWAM(1):found match for segno %05o at _n=%d\n", segno, _n);
            
            cpu . SDW = &cpu . SDWAM[_n];
            
            /*
             If the SDWAM match logic circuitry indicates a hit, all usage counts (SDWAM.USE) greater than the usage count of the register hit are decremented by one, the usage count of the register hit is set to 15 (63?), and the contents of the register hit are read out into the address preparation circuitry. 
             */
            for(int _h = 0 ; _h < nwam ; _h++)
            {
                if (cpu . SDWAM[_h].USE > cpu . SDW->USE)
                    cpu . SDWAM[_h].USE -= 1;
            }
            cpu . SDW->USE = N_WAM_ENTRIES - 1;
            
            sim_debug(DBG_APPENDING, &cpu_dev, "fetchSDWfromSDWAM(2):SDWAM[%d]=%s\n", _n, strSDW(cpu . SDW));
            return cpu . SDW;
        }
    }
    sim_debug(DBG_APPENDING, &cpu_dev, "fetchSDWfromSDWAM(3):SDW for segment %05o not found in SDWAM\n", segno);
    return NULL;    // segment not referenced in SDWAM
}
#endif // SPEED
/**
 * Fetches an SDW from a paged descriptor segment.
 */
// CANFAULT
static void fetchPSDW(word15 segno)
{
    sim_debug(DBG_APPENDING, &cpu_dev, "fetchPSDW(0):segno=%05o\n", segno);
    
    setAPUStatus (apuStatus_SDWP);
    word24 y1 = (2 * segno) % 1024;
    
    word36 SDWeven, SDWodd;
    
    core_read2(((cpu . PTW0.ADDR << 6) + y1) & PAMASK, &SDWeven, &SDWodd, __func__);
    
    // even word
    cpu . SDW0.ADDR = (SDWeven >> 12) & 077777777;
    cpu . SDW0.R1 = (SDWeven >> 9) & 7;
    cpu . SDW0.R2 = (SDWeven >> 6) & 7;
    cpu . SDW0.R3 = (SDWeven >> 3) & 7;
    cpu . SDW0.DF = TSTBIT(SDWeven, 2);
    cpu . SDW0.FC = SDWeven & 3;
    
    // odd word
    cpu . SDW0.BOUND = (SDWodd >> 21) & 037777;
    cpu . SDW0.R = TSTBIT(SDWodd, 20);
    cpu . SDW0.E = TSTBIT(SDWodd, 19);
    cpu . SDW0.W = TSTBIT(SDWodd, 18);
    cpu . SDW0.P = TSTBIT(SDWodd, 17);
    cpu . SDW0.U = TSTBIT(SDWodd, 16);
    cpu . SDW0.G = TSTBIT(SDWodd, 15);
    cpu . SDW0.C = TSTBIT(SDWodd, 14);
    cpu . SDW0.EB = SDWodd & 037777;
    
    //cpu . PPR.P = (cpu . SDW0.P && cpu . PPR.PRR == 0);   // set priv bit (if OK)

    sim_debug (DBG_APPENDING, & cpu_dev, "fetchPSDW y1 0%o p->ADDR 0%o SDW 0%012llo 0%012llo ADDR 0%o BOUND 0%o U %o F %o\n",
 y1, cpu . PTW0.ADDR, SDWeven, SDWodd, cpu . SDW0.ADDR, cpu . SDW0.BOUND, cpu . SDW0.U, cpu . SDW0.DF);
}

/// \brief Nonpaged SDW Fetch
/// Fetches an SDW from an unpaged descriptor segment.
// CANFAULT
static void fetchNSDW(word15 segno)
{
    sim_debug(DBG_APPENDING, &cpu_dev, "fetchNSDW(0):segno=%05o\n", segno);

    setAPUStatus (apuStatus_SDWNP);

    if (2 * segno >= 16 * (cpu . DSBR.BND + 1))
    {
        sim_debug(DBG_APPENDING, &cpu_dev, "fetchNSDW(1):Access Violation, out of segment bounds for segno=%05o DSBR.BND=%d\n", segno, cpu . DSBR.BND);
        // generate access violation, out of segment bounds fault
        doFault(FAULT_ACV, (_fault_subtype) {.fault_acv_subtype=ACV15}, "acvFault fetchNSDW: out of segment bounds fault");
    }
    sim_debug(DBG_APPENDING, &cpu_dev, "fetchNSDW(2):fetching SDW from %05o\n", cpu . DSBR.ADDR + 2 * segno);
    word36 SDWeven, SDWodd;
    
    core_read2((cpu . DSBR.ADDR + 2 * segno) & PAMASK, &SDWeven, &SDWodd, __func__);
    
    // even word
    cpu . SDW0.ADDR = (SDWeven >> 12) & 077777777;
    cpu . SDW0.R1 = (SDWeven >> 9) & 7;
    cpu . SDW0.R2 = (SDWeven >> 6) & 7;
    cpu . SDW0.R3 = (SDWeven >> 3) & 7;
    cpu . SDW0.DF = TSTBIT(SDWeven, 2);
    cpu . SDW0.FC = SDWeven & 3;
    
    // odd word
    cpu . SDW0.BOUND = (SDWodd >> 21) & 037777;
    cpu . SDW0.R = TSTBIT(SDWodd, 20);
    cpu . SDW0.E = TSTBIT(SDWodd, 19);
    cpu . SDW0.W = TSTBIT(SDWodd, 18);
    cpu . SDW0.P = TSTBIT(SDWodd, 17);
    cpu . SDW0.U = TSTBIT(SDWodd, 16);
    cpu . SDW0.G = TSTBIT(SDWodd, 15);
    cpu . SDW0.C = TSTBIT(SDWodd, 14);
    cpu . SDW0.EB = SDWodd & 037777;
    
    //cpu . PPR.P = (cpu . SDW0.P && cpu . PPR.PRR == 0);   // set priv bit (if OK)
    
    sim_debug(DBG_APPENDING, &cpu_dev, "fetchNSDW(2):SDW0=%s\n", strSDW0(&cpu . SDW0));
}

#ifndef SPEED
static char *strSDW(_sdw *SDW)
{
    static char buff[256];
    
    //if (SDW->ADDR == 0 && SDW->BOUND == 0) // need a better test
    //if (!SDW->_initialized)
    if (!SDW->DF)
        sprintf(buff, "*** SDW Uninitialized ***");
    else
        sprintf(buff, "ADDR:%06o R1:%o R2:%o R3:%o BOUND:%o R:%o E:%o W:%o P:%o U:%o G:%o C:%o CL:%o POINTER=%o USE=%d",
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
                SDW->POINTER,
                SDW->USE);
    return buff;
}
#endif

#ifndef SPEED
/**
 * dump SDWAM...
 */
t_stat dumpSDWAM (void)
{
    for(int _n = 0 ; _n < N_WAM_ENTRIES ; _n++)
    {
        _sdw *p = &cpu . SDWAM[_n];
        
        //if (p->_initialized)
        if (p->DF)
            sim_printf("SDWAM n:%d %s\n", _n, strSDW(p));
    }
    return SCPE_OK;
}
#endif


/**
 * load the current in-core SDW0 into the SDWAM ...
 */
static void loadSDWAM(word15 segno)
{
#ifdef SPEED
#if 0
    cpu . SDWAM0 . ADDR = cpu . SDW0.ADDR;
    cpu . SDWAM0 . R1 = cpu . SDW0.R1;
    cpu . SDWAM0 . R2 = cpu . SDW0.R2;
    cpu . SDWAM0 . R3 = cpu . SDW0.R3;
    cpu . SDWAM0 . BOUND = cpu . SDW0.BOUND;
    cpu . SDWAM0 . R = cpu . SDW0.R;
    cpu . SDWAM0 . E = cpu . SDW0.E;
    cpu . SDWAM0 . W = cpu . SDW0.W;
    cpu . SDWAM0 . P = cpu . SDW0.P;
    cpu . SDWAM0 . U = cpu . SDW0.U;
    cpu . SDWAM0 . G = cpu . SDW0.G;
    cpu . SDWAM0 . C = cpu . SDW0.C;
    cpu . SDWAM0 . EB = cpu . SDW0.EB;
#else
    cpu.SDWAM0 = cpu.SDW0;
#endif
    cpu . SDWAM0 . POINTER = segno;
    cpu . SDWAM0 . USE = 0;
            
    cpu . SDWAM0 . FE = true;     // in use by SDWAM
            
    cpu . SDW = & cpu . SDWAM0;
            
#else
    if (cpu . switches . disable_wam)
    {
        sim_debug(DBG_APPENDING, &cpu_dev, "loadSDWAM: SDWAM disabled\n");
        _sdw *p = &cpu . SDWAM[0];
#if 0
        p->ADDR = cpu . SDW0.ADDR;
        p->R1 = cpu . SDW0.R1;
        p->R2 = cpu . SDW0.R2;
        p->R3 = cpu . SDW0.R3;
        p->BOUND = cpu . SDW0.BOUND;
        p->R = cpu . SDW0.R;
        p->E = cpu . SDW0.E;
        p->W = cpu . SDW0.W;
        p->P = cpu . SDW0.P;
        p->U = cpu . SDW0.U;
        p->G = cpu . SDW0.G;
        p->C = cpu . SDW0.C;
        p->EB = cpu . SDW0.EB;
#else
        * p = cpu.SDW0;
#endif
        p->POINTER = segno;
        p->USE = 0;
            
        //p->_initialized = true;     // in use by SDWAM
        p->DF = true;     // in use by SDWAM
            
        cpu . SDW = p;
            
        return;
    }
    
    /* If the SDWAM match logic does not indicate a hit, the SDW is fetched from the descriptor segment in main memory and loaded into the SDWAM register with usage count 0 (the oldest), all usage counts are decremented by one with the newly loaded register rolling over from 0 to 15 (63?), and the newly loaded register is read out into the address preparation circuitry.
     */
    for(int _n = 0 ; _n < N_WAM_ENTRIES ; _n++)
    {
        _sdw *p = &cpu . SDWAM[_n];
        //if (!p->_initialized || p->USE == 0)
        if (!p->DF || p->USE == 0)
        {
            sim_debug(DBG_APPENDING, &cpu_dev, "loadSDWAM(1):SDWAM[%d] p->USE=0\n", _n);
            
#if 0
            p->ADDR = cpu . SDW0.ADDR;
            p->R1 = cpu . SDW0.R1;
            p->R2 = cpu . SDW0.R2;
            p->R3 = cpu . SDW0.R3;
            p->BOUND = cpu . SDW0.BOUND;
            p->R = cpu . SDW0.R;
            p->E = cpu . SDW0.E;
            p->W = cpu . SDW0.W;
            p->P = cpu . SDW0.P;
            p->U = cpu . SDW0.U;
            p->G = cpu . SDW0.G;
            p->C = cpu . SDW0.C;
            p->EB = cpu . SDW0.EB;
#else
            *p = cpu.SDW0;
#endif
            p->POINTER = segno;
            p->USE = 0;
            
            //p->_initialized = true;     // in use by SDWAM
            p->DF = true;     // in use by SDWAM
            
            for(int _h = 0 ; _h < N_WAM_ENTRIES ; _h++)
            {
                _sdw *q = &cpu . SDWAM[_h];
                //if (!q->_initialized)
                if (!q->DF)
                    continue;
                
                q->USE -= 1;
                q->USE &= 077;
            }
            
            cpu . SDW = p;
            
            sim_debug(DBG_APPENDING, &cpu_dev, "loadSDWAM(2):SDWAM[%d]=%s\n", _n, strSDW(p));
            
            return;
        }
    }
    sim_debug(DBG_APPENDING, &cpu_dev, "loadSDWAM(3) no USE=0 found for segment=%d\n", segno);
    
    sim_printf("loadSDWAM(%05o): no USE=0 found!\n", segno);
    dumpSDWAM();
#endif
}

#ifndef SPEED
static _ptw* fetchPTWfromPTWAM(word15 segno, word18 CA)
{
    int nwam = N_WAM_ENTRIES;
    if (cpu . switches . disable_wam)
    {
        sim_debug(DBG_APPENDING, &cpu_dev, "fetchPTWfromPTWAM: PTWAM disabled\n");
        nwam = 1;
        return NULL;
    }
    
    for(int _n = 0 ; _n < nwam ; _n++)
    {
        if (((CA >> 10) & 0377) == ((cpu . PTWAM[_n].PAGENO >> 4) & 0377) && cpu . PTWAM[_n].POINTER == segno && cpu . PTWAM[_n].FE)   //_initialized)
        {
            cpu . PTW = &cpu . PTWAM[_n];
            
            /*
             * If the PTWAM match logic circuitry indicates a hit, all usage counts (PTWAM.USE) greater than the usage count of the register hit are decremented by one, the usage count of the register hit is set to 15 (63?), and the contents of the register hit are read out into the address preparation circuitry.
             */
            for(int _h = 0 ; _h < nwam ; _h++)
            {
                if (cpu . PTWAM[_h].USE > cpu . PTW->USE)
                    cpu . PTWAM[_h].USE -= 1; //PTW->USE -= 1;
            }
            cpu . PTW->USE = N_WAM_ENTRIES - 1;
#ifdef do_selftestPTWAM
            selftestPTWAM ();
#endif
            return cpu . PTW;
        }
    }
    return NULL;    // segment not referenced in SDWAM
}
#endif

static void fetchPTW(_sdw *sdw, word18 offset)
{

    setAPUStatus (apuStatus_PTW);

    word24 y2 = offset % 1024;
    word24 x2 = (offset - y2) / 1024;
    
    word36 PTWx2;
    
    sim_debug (DBG_APPENDING,& cpu_dev, "fetchPTW address %08o\n", sdw->ADDR + x2);

    core_read((sdw->ADDR + x2) & PAMASK, &PTWx2, __func__);
    
    cpu . PTW0.ADDR = GETHI(PTWx2);
    cpu . PTW0.U = TSTBIT(PTWx2, 9);
    cpu . PTW0.M = TSTBIT(PTWx2, 6);
    cpu . PTW0.DF = TSTBIT(PTWx2, 2);
    cpu . PTW0.FC = PTWx2 & 3;
    
    sim_debug (DBG_APPENDING, & cpu_dev, "fetchPTW x2 0%o y2 0%o sdw->ADDR 0%o PTWx2 0%012llo PTW0: ADDR 0%o U %o M %o F %o FC %o\n", x2, y2, sdw->ADDR, PTWx2, cpu . PTW0.ADDR, cpu . PTW0.U, cpu . PTW0.M, cpu . PTW0.DF, cpu . PTW0.FC);
}

static void loadPTWAM(word15 segno, word18 offset)
{
#ifdef SPEED
#if 0
    cpu . PTWAM0 . ADDR = cpu . PTW0.ADDR;
    cpu . PTWAM0 . M = cpu . PTW0.M;
#else
    cpu.PTWAM0 = cpu.PTW0;
#endif
    cpu.PTWAM0.PAGENO = (offset >> 6) & 07777;
    cpu.PTWAM0.POINTER = segno;
    cpu.PTWAM0.USE = 0;
    cpu.PTWAM0.FE = true;
            
    cpu.PTW = & cpu.PTWAM0;
#else
    if (cpu . switches . disable_wam)
    {
        sim_debug(DBG_APPENDING, &cpu_dev, "loadPTWAM: PTWAM disabled\n");
        _ptw *p = &cpu . PTWAM[0];
#if 0
        p->ADDR = cpu . PTW0.ADDR;
        p->M = cpu . PTW0.M;
#else
        *p = cpu.PTW0;
#endif
        p->PAGENO = (offset >> 6) & 07777;
        p->POINTER = segno;
        p->USE = 0;
        p->FE = true;
            
        cpu . PTW = p;
        return;
    }
    
    /*
     * If the PTWAM match logic does not indicate a hit, the PTW is fetched from main memory and loaded into the PTWAM register with usage count 0 (the oldest), all usage counts are decremented by one with the newly loaded register rolling over from 0 to 15 (63), and the newly loaded register is read out into the address preparation circuitry.
     */
    for(int _n = 0 ; _n < N_WAM_ENTRIES ; _n++)
    {
        _ptw *p = &cpu . PTWAM[_n];
        //if (!p->_initialized || p->USE == 0)
        if (!p->FE || p->USE == 0)
        {
#if 0
            p->ADDR = cpu . PTW0.ADDR;
            p->M = cpu . PTW0.M;
#else
            *p = cpu.PTW0;
#endif
            p->PAGENO = (offset >> 6) & 07777;
            p->POINTER = segno;
            p->USE = 0;
            p->FE = true;
            
            for(int _h = 0 ; _h < N_WAM_ENTRIES ; _h++)
            {
                _ptw *q = &cpu . PTWAM[_h];
                //if (!q->_initialized)
                //if (!q->F)
                    //continue;
                
                q->USE -= 1;
                q->USE &= 077;
            }
            
            cpu . PTW = p;
#ifdef do_selftestPTWAM
            selftestPTWAM ();
#endif
            return;
        }
    }
    sim_printf("loadPTWAM(segno=%05o, offset=%012o): no USE=0 found!\n", segno, offset);
#endif // SPEED
}

/**
 * modify target segment PTW (Set M=1) ...
 */
static void modifyPTW(_sdw *sdw, word18 offset)
{
    word24 y2 = offset % 1024;
    word24 x2 = (offset - y2) / 1024;
    
    word36 PTWx2;
    
    setAPUStatus (apuStatus_MPTW);

    core_read((sdw->ADDR + x2) & PAMASK, &PTWx2, __func__);
    PTWx2 = SETBIT(PTWx2, 6);
    core_write((sdw->ADDR + x2) & PAMASK, PTWx2, __func__);
//if_sim_debug (DBG_TRACE, & cpu_dev)
//sim_printf ("modifyPTW 0%o %012llo ADDR %o U %llo M %llo F %llo FC %llo\n",
            //sdw -> ADDR + x2, PTWx2, GETHI (PTWx2), TSTBIT(PTWx2, 9), 
            //TSTBIT(PTWx2, 6), TSTBIT(PTWx2, 2), PTWx2 & 3);
   
    cpu . PTW->M = 1;
}



/**
 * Is the instruction a SToRage OPeration ?
 */
#ifndef QUIET_UNUSED
static bool isAPUDataMovement(word36 inst)
{
    // XXX: implement - when we figure out what it is
    return false;
}
#endif

#ifndef QUIET_UNUSED
static char *strAccessType(MemoryAccessType accessType)
{
    switch (accessType)
    {
        case Unknown:           return "Unknown";
        case InstructionFetch:  return "InstructionFetch";
        case IndirectRead:      return "IndirectRead";
        //case DataRead:          return "DataRead";
        //case DataWrite:         return "DataWrite";
        case OperandRead:       return "Operand/Data-Read";
        case OperandWrite:      return "Operand/Data-Write";
        case Call6Operand:      return "Call6Operand";
        case RTCDOperand:       return "RTCDOperand";
        default:                return "???";
    }
}
#endif

#ifndef QUIET_UNUSED
static char *strACV(_fault_subtype acv)
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
  return "unhandled acv in strACV";
}
#endif

static fault_acv_subtype_  acvFaults;   ///< pending ACV faults

static void acvFault(fault_acv_subtype_ acvfault, char * msg)
{
    acvFaults |= acvfault;
    sim_debug(DBG_APPENDING, &cpu_dev,
              "doAppendCycle(acvFault): acvFault=%llo acvFaults=%llo: %s",
              (word36) acvfault, (word36) acvFaults, msg);
}

static char *strPCT(_processor_cycle_type t)
{
    switch (t)
    {
        case UNKNOWN_CYCLE: return "UNKNOWN_CYCLE";
        case APPEND_CYCLE: return "APPEND_CYCLE";
        case CA_CYCLE: return "CA_CYCLE";
        case OPERAND_STORE : return "OPERAND_STORE";
        case OPERAND_READ : return "OPERAND_READ";
        case DIVIDE_EXECUTION: return "DIVIDE_EXECUTION";
        case FAULT: return "FAULT";
        case INDIRECT_WORD_FETCH: return "INDIRECT_WORD_FETCH";
        case RTCD_OPERAND_FETCH: return "RTCD_OPERAND_FETCH";
        //case SEQUENTIAL_INSTRUCTION_FETCH: return "SEQUENTIAL_INSTRUCTION_FETCH";
        case INSTRUCTION_FETCH: return "INSTRUCTION_FETCH";
        case APU_DATA_MOVEMENT: return "APU_DATA_MOVEMENT";
        case ABORT_CYCLE: return "ABORT_CYCLE";
        case FAULT_CYCLE: return "FAULT_CYCLE";
        case EIS_OPERAND_DESCRIPTOR : return "EIS_OPERAND_DESCRIPTOR";
        case EIS_OPERAND_STORE : return "EIS_OPERAND_STORE";
        case EIS_OPERAND_READ : return "EIS_OPERAND_READ";

        default:
            return "Unhandled _processor_cycle_type";
    }
  
}

// CANFAULT
_sdw0 * getSDW (word15 segno)
  {
     sim_debug (DBG_APPENDING, & cpu_dev, "getSDW for segment %05o\n", segno);
     sim_debug (DBG_APPENDING, & cpu_dev, "getSDW DSBR.U=%o\n", cpu . DSBR . U);
        
    if (cpu . DSBR . U == 0)
      {
        fetchDSPTW (segno);
            
        if (! cpu . PTW0 . DF)
          doFault (FAULT_DF0 + cpu . PTW0.FC, (_fault_subtype) {.bits=0}, "getSDW PTW0.F == 0");
            
        if (! cpu . PTW0 . U)
          modifyDSPTW (segno);
            
        fetchPSDW (segno);
      }
    else
      fetchNSDW (segno);
        
    if (cpu . SDW0 . DF == 0)
      {
        sim_debug (DBG_APPENDING, & cpu_dev,
                   "getSDW SDW0.F == 0! Initiating directed fault\n");
        doFault (FAULT_DF0 + cpu . SDW0 . FC, (_fault_subtype) {.bits=0}, "SDW0.F == 0");
     }
   return & cpu . SDW0;
  }

//static bool bPrePageMode = false;

/*
 * recoding APU functions to more closely match Fig 5,6 & 8 ...
 * Returns final address suitable for core_read/write
 */

// Usage notes:
//   Checks for the following conditions:
//     thisCycle == INSTRUCTION_FETCH, OPERAND_STORE, EIS_OPERAND_STORE, 
//                  RTCD_OPERAND_FETCH
//     thisCycle == OPERAND_READ && i->info->flags & CALL6_INS
//     thisCycle == OPERAND_READ && (i->info && i->info->flags & TRANSFER_INS

// CANFAULT
word24 doAppendCycle (word18 address, _processor_cycle_type thisCycle)
{
    DCDstruct * i = & cpu . currentInstruction;
    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(Entry) thisCycle=%s\n", strPCT(thisCycle));
    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(Entry) Address=%06o\n", address);
    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(Entry) PPR.PRR=%o PPR.PSR=%05o\n", cpu . PPR.PRR, cpu . PPR.PSR);
    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(Entry) TPR.TRR=%o TPR.TSR=%05o\n", cpu . TPR.TRR, cpu . TPR.TSR);

    bool instructionFetch = (thisCycle == INSTRUCTION_FETCH);
    bool StrOp = (thisCycle == OPERAND_STORE || thisCycle == EIS_OPERAND_STORE);
    
    cpu . RSDWH_R1 = 0;
    
    acvFaults = 0;
    char * acvFaultsMsg = "<unknown>";

    word24 finalAddress = (word24) -1;  // not everything requires a final address
    
//
//  A:
//    Get SDW
#ifndef QUIET_UNUSED
A:;
#endif

    cpu . TPR . CA = address;
//
// Phase 1:
//
//     A: Get the SDW
//
//     B: Check the ring
//
// Phase 2:
//
//     B1: If CALL6 operand
//           goto E
//         If instruction fetch or transfer instruction operand
//           goto F
//         If write
//           check write permission
//         else
//           check read permission
//         goto G
//
//     E: -- CALL6 operand handling
//        Check execute and gate bits
//        Get the ring
//        goto G
//
//     F: -- instruction fetch or transfer instruction operand
//        Check execute bit and ring
//        goto D
//
//     D: Check RALR
//        goto G
//
// Phase 3
//
//     G: Check BOUND
//        If not paged
//          goto H
//        Fetch PTW
//        Fetch prepage PTW
//        Goto I
//
//     H: Compute final address
//        Goto HI
//
//     I: If write
//          set PTW.M
//        Compute final address
//        Goto HI
//
// Phase 4
//
//     HI: --
//         If indirect word fetch
//           goto Exit
//         if rtcd operand fetch
//           goto KL
//         If instruction fetch or transfer instruction operand
//           goto KL
//         "APU data movement"
//         Goto Exit
//
//    KL: Set PPR.P
//         Goto Exit
//
//    Exit: return
//

    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(A)\n");
    
#ifdef SPEED
    if (cpu . DSBR.U == 0)
    {
        fetchDSPTW(cpu . TPR.TSR);
        
        if (!cpu . PTW0.DF)
            doFault(FAULT_DF0 + cpu . PTW0.FC, (_fault_subtype) {.bits=0}, "doAppendCycle(A): PTW0.F == 0");
        
        if (!cpu . PTW0.U)
            modifyDSPTW(cpu . TPR.TSR);
        
        fetchPSDW(cpu . TPR.TSR);
    }
    else
        fetchNSDW(cpu . TPR.TSR); // load SDW0 from descriptor segment table.
    
    if (cpu . SDW0.DF == 0)
    {
        sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(A): SDW0.F == 0! Initiating directed fault\n");
        // initiate a directed fault ...
        doFault(FAULT_DF0 + cpu . SDW0.FC, (_fault_subtype) {.bits=0}, "SDW0.F == 0");
    }
    loadSDWAM(cpu . TPR.TSR);
#else
    // is SDW for C(TPR.TSR) in SDWAM?
    if (!fetchSDWfromSDWAM(cpu . TPR.TSR))
    {
        // No
        sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(A):SDW for segment %05o not in SDWAM\n", cpu . TPR.TSR);
        
        sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(A):DSBR.U=%o\n", cpu . DSBR.U);
        
        if (cpu . DSBR.U == 0)
        {
            fetchDSPTW(cpu . TPR.TSR);
            
            if (!cpu . PTW0.DF)
                doFault(FAULT_DF0 + cpu . PTW0.FC, (_fault_subtype) {.bits=0}, "doAppendCycle(A): PTW0.F == 0");
            
            if (!cpu . PTW0.U)
                modifyDSPTW(cpu . TPR.TSR);
            
            fetchPSDW(cpu . TPR.TSR);
        }
        else
            fetchNSDW(cpu . TPR.TSR); // load SDW0 from descriptor segment table.
        
        if (cpu . SDW0.DF == 0)
        {
            sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(A): SDW0.F == 0! Initiating directed fault\n");
            // initiate a directed fault ...
            doFault(FAULT_DF0 + cpu . SDW0.FC, (_fault_subtype) {.bits=0}, "SDW0.F == 0");
        }
        else
            // load SDWAM .....
            loadSDWAM(cpu . TPR.TSR);
    }
#endif
    sim_debug (DBG_APPENDING, & cpu_dev,
               "doAppendCycle(A) R1 %o R2 %o R3 %o\n", cpu . SDW -> R1, cpu . SDW -> R2, cpu . SDW -> R3);
    // Yes...
    cpu . RSDWH_R1 = cpu . SDW->R1;

//
// B: Check the ring
//

    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(B)\n");
    
    //C(SDW.R1) ≤ C(SDW.R2) ≤ C(SDW .R3)?
    if (!(cpu.SDW->R1 <= cpu.SDW->R2 && cpu.SDW->R2 <= cpu.SDW->R3))
        // Set fault ACV0 = IRO
        acvFault(ACV0, "doAppendCycle(B) C(SDW.R1) ≤ C(SDW.R2) ≤ C(SDW .R3)");

    // No

//
// B1: The operand is one of: an instruction, data to be read or data to be
//     written
//

    // CALL6 must check the call gate. Also it has the feature of checking 
    // the target address for execution permission (instead of allowing the 
    // instruction fetch of the next cycle to do it). This allows the fault 
    // to occur in the calling instruction, which is easier to debug.

    // Is OPCODE call6?
    if (thisCycle == OPERAND_READ && i->info->flags & CALL6_INS)
      goto E;
 

    // If the instruction is a transfer operand or we are doing an instruction
    // fetch, the operand is destined to be executed. Verify that the operand
    // is executable

    // Transfer or instruction fetch?
    if (instructionFetch || (thisCycle == OPERAND_READ && (i->info && i->info->flags & TRANSFER_INS)))
        goto F;
    
    // Not executed, therefore it is data. Read or Write?

    if (StrOp)
    {
        sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(B):STR-OP\n");
        
        // C(TPR.TRR) > C(SDW .R2)?
        if (cpu . TPR.TRR > cpu . SDW->R2)
            //Set fault ACV5 = OWB
            acvFault(ACV5, "doAppendCycle(B) C(TPR.TRR) > C(SDW .R2)");
        
        if (!cpu . SDW->W)
            // Set fault ACV6 = W-OFF
            acvFault(ACV6, "doAppendCycle(B) ACV6 = W-OFF");
        
    } else {
        sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(B):!STR-OP\n");
        
        // No
        // C(TPR.TRR) > C(SDW .R2)?
        if (cpu . TPR.TRR > cpu . SDW->R2) {
            //Set fault ACV3 = ORB
            acvFaults |= ACV3;
            acvFaultsMsg = "acvFaults(B) C(TPR.TRR) > C(SDW .R2)";
        }
        
        if (cpu . SDW->R == 0)
        {
            //C(PPR.PSR) = C(TPR.TSR)?
            if (cpu . PPR.PSR != cpu . TPR.TSR) {
                //Set fault ACV4 = R-OFF
                acvFaults |= ACV4;
                acvFaultsMsg = "acvFaults(B) C(PPR.PSR) = C(TPR.TSR)";
            }
        }
        
    }
    goto G;
    
D:;
    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(D)\n");
    
    // transfer or instruction fetch

    // AL39, pg 31, RING ALARM REGISTER:
    // "...and the instruction for which an absolute main memory address is 
    //  being prepared is a transfer instruction..."
    if (instructionFetch)
      goto G;

    if (cpu . rRALR == 0)
        goto G;
    
    // C(PPR.PRR) < RALR?
    if (!(cpu . PPR.PRR < cpu . rRALR)) {
        sim_debug (DBG_APPENDING, & cpu_dev,
                   "acvFaults(D) C(PPR.PRR) %o < RALR %o\n", cpu . PPR . PRR, cpu . rRALR);
        acvFaults |= ACV13;
        acvFaultsMsg = "acvFaults(D) C(PPR.PRR) < RALR";
    }
    
    goto G;
    
E:;

//
// E: CALL6
//

    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(E): CALL6\n");

    //SDW .E set ON?
    if (!cpu . SDW->E) {
        // Set fault ACV2 = E-OFF
        acvFaults |= ACV2;
        acvFaultsMsg = "acvFaults(E) SDW .E set OFF";
    }
    
    //SDW .G set ON?
    if (cpu . SDW->G)
        goto E1;
    
    // C(PPR.PSR) = C(TPR.TSR)?
    if (cpu . PPR.PSR == cpu . TPR.TSR)
        goto E1;
    
    // XXX This doesn't seem right
    // TPR.CA4-17 ≥ SDW.CL?
    //if ((cpu . TPR.CA & 0037777) >= SDW->CL)
    if ((address & 0037777) >= cpu . SDW->EB) {
        // Set fault ACV7 = NO GA
        acvFaults |= ACV7;
        acvFaultsMsg = "acvFaults(E) TPR.CA4-17 ≥ SDW.CL";
    }
    
E1:
    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(E1): CALL6 (cont'd)\n");

    // C(TPR.TRR) > SDW.R3?
    if (cpu . TPR.TRR > cpu . SDW->R3) {
        //Set fault ACV8 = OCB
        acvFaults |= ACV8;
        acvFaultsMsg = "acvFaults(E1) C(TPR.TRR) > SDW.R3";
    }
    
    // C(TPR.TRR) < SDW.R1?
    if (cpu . TPR.TRR < cpu . SDW->R1) {
        // Set fault ACV9 = OCALL
        acvFaults |= ACV9;
        acvFaultsMsg = "acvFaults(E1) C(TPR.TRR) < SDW.R1";
    }
    
    
    // C(TPR.TRR) > C(PPR.PRR)?
    if (cpu . TPR.TRR > cpu . PPR.PRR)
        // C(PPR.PRR) < SDW.R2?
        if (cpu . PPR.PRR < cpu . SDW->R2) {
            // Set fault ACV10 = BOC
            acvFaults |= ACV10;
            acvFaultsMsg = "acvFaults(E1) C(TPR.TRR) > C(PPR.PRR) && C(PPR.PRR) < SDW.R2";
        }
    
    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(E1): CALL6 TPR.TRR %o SDW->R2 %o\n", cpu . TPR . TRR, cpu . SDW -> R2);
    // C(TPR.TRR) > SDW.R2?
    if (cpu . TPR.TRR > cpu . SDW->R2)
        // ￼SDW.R2 → C(TPR.TRR)
        cpu . TPR.TRR = cpu . SDW->R2;
    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(E1): CALL6 TPR.TRR %o\n", cpu . TPR . TRR);
    
    goto G;
    
F:;
    
    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(F): transfer or instruction fetch\n");

    // C(TPR.TRR) < C(SDW .R1)?
    if (cpu . TPR.TRR < cpu . SDW->R1) {
        sim_debug (DBG_APPENDING, & cpu_dev,
                   "acvFaults(F) C(TPR.TRR) %o < C(SDW .R1) %o\n", cpu . TPR . TRR, cpu . SDW -> R1);
        acvFaults |= ACV1;
        acvFaultsMsg = "acvFaults(F) C(TPR.TRR) < C(SDW .R1)";
    }
    
    //C(TPR.TRR) > C(SDW .R2)?
    if (cpu . TPR.TRR > cpu . SDW->R2) {
        sim_debug (DBG_TRACE, & cpu_dev,
                   "acvFaults(F) C(TPR.TRR) %o > C(SDW .R2) %o\n", cpu . TPR . TRR, cpu . SDW -> R2);
        acvFaults |= ACV1;
        acvFaultsMsg = "acvFaults(F) C(TPR.TRR) > C(SDW .R2)";
    }
    
    //SDW .E set ON?
    if (!cpu . SDW->E) {
        acvFaults |= ACV2;
        acvFaultsMsg = "acvFaults(F) SDW .E set OFF";
    }
    
    //C(PPR.PRR) = C(TPR.TRR)?
    if (cpu . PPR.PRR != cpu . TPR.TRR) {
        //Set fault ACV12 = CRT
        acvFaults |= ACV12;
        acvFaultsMsg = "acvFaults(F) C(PPR.PRR) != C(TPR.TRR)";
    }
    
    goto D;

G:;
    
    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(G)\n");
    
    //C(TPR.CA)0,13 > SDW.BOUND?
    //if (((TPR.CA >> 4) & 037777) > SDW->BOUND)
    if (((address >> 4) & 037777) > cpu . SDW->BOUND) {
        acvFaults |= ACV15;
        acvFaultsMsg = "acvFaults(G) C(TPR.CA)0,13 > SDW.BOUND";
        sim_debug (DBG_FAULT, & cpu_dev, "acvFaults(G) C(TPR.CA)0,13 > SDW.BOUND\n    address %06o address>>4&037777 %06o SDW->BOUND %06o",
                    address, ((address >> 4) & 037777), cpu . SDW->BOUND);
    }
    
    if (acvFaults)
        // Initiate an access violation fault
        doFault(FAULT_ACV, (_fault_subtype) {.fault_acv_subtype=acvFaults}, "ACV fault");
    
    // is segment C(TPR.TSR) paged?
    if (cpu . SDW->U)
        goto H; // Not paged
    
    // Yes. segment is paged ...
    // is PTW for C(TPR.CA) in PTWAM?
    
#ifdef SPEED
    fetchPTW(cpu . SDW, address);
    if (!cpu . PTW0.DF)
    {
        //cpu . TPR.CA = address;
        // initiate a directed fault
        doFault(FAULT_DF0 + cpu . PTW0.FC, (_fault_subtype) {.bits=0}, "PTW0.F == 0");
    }
    loadPTWAM(cpu . SDW->POINTER, address);    // load PTW0 to PTWAM

#else
    if (!fetchPTWfromPTWAM(cpu . SDW->POINTER, address))  //TPR.CA))
    {
        appendingUnitCycleType = apuCycle_PTWfetch;
        //fetchPTW(cpu . SDW, cpu . TPR.CA);
        fetchPTW(cpu . SDW, address);
        if (!cpu . PTW0.DF)
        {
            //cpu . TPR.CA = address;
            // initiate a directed fault
            doFault(FAULT_DF0 + cpu . PTW0.FC, (_fault_subtype) {.bits=0}, "PTW0.F == 0");
        }

        //loadPTWAM(cpu . SDW->POINTER, cpu . TPR.CA);    // load PTW0 to PTWAM
        loadPTWAM(cpu . SDW->POINTER, address);    // load PTW0 to PTWAM
    }
#endif
    
    // is prepage mode???
    // XXX: don't know what todo with this yet ...
    // XXX: ticket #11
    // The MVT, TCT, TCTR, and CMPCT instruction have a prepage check. The size of the translate table is determined by the TA1 data type as shown in the table below. Before the instruction is executed, a check is made for allocation in memory for the page for the translate table. If the page is not in memory, a Missing Page fault occurs before execution of the instruction. (cf. Bull, RJ78, p.7-75, sec 7.14.15)
#if 0
    if (bPrePageMode)
    {
        //Is PTW.F set ON?
       if (!PTW0.F)
          // initiate a directed fault
         doFault(FAULT_DF0 + PTW0.FC, 0, "PTW0.F == 0");
        
    }
    bPrePageMode = false;   // done with this
#endif
    goto I;
    
H:;
    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(H): FANP\n");
#if 1
    appendingUnitCycleType = apuCycle_FANP;
    setAPUStatus (apuStatus_FANP);
#else
    // ISOLTS pa865 test-01a 101232
    if (get_bar_mode ())
      {
        appendingUnitCycleType = apuCycle_FANP;
        setAPUStatus (apuStatus_FABS);
      }
    else
      {
        appendingUnitCycleType = apuCycle_FANP;
        setAPUStatus (apuStatus_FANP);
      }
#endif
    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(H): SDW->ADDR=%08o TPR.CA=%06o \n", cpu . SDW->ADDR, address);

    finalAddress = cpu . SDW->ADDR + address;
    finalAddress &= 0xffffff;
    
    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(H:FANP): (%05o:%06o) finalAddress=%08o\n",cpu . TPR.TSR, address, finalAddress);
    
    goto HI;
    
I:;

// Set PTW.M

    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(I)\n");
    //if (isSTROP(i) && PTW->M == 0)
    //if (thisCycle == OPERAND_STORE && PTW->M == 0)  // is this the right way to do this?
    if (StrOp && cpu . PTW->M == 0)  // is this the right way to do this?
    {
#if 0
        // Modify PTW -  Sets the page modified bit (PTW.M) in the PTW for a page in other than a descriptor segment page table.
        appendingUnitCycleType = MPTW;
        cpu . PTW->M = 1;
        
#else
       modifyPTW(cpu . SDW, address);
#endif
    }
    
    // final address paged
    appendingUnitCycleType = apuCycle_FAP;
    setAPUStatus (apuStatus_FAP);
    
    //word24 y2 = TPR.CA % 1024;
    word24 y2 = address % 1024;
    
    finalAddress = ((cpu . PTW->ADDR & 0777777) << 6) + y2;
    finalAddress &= 0xffffff;
    
    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(H:FAP): (%05o:%06o) finalAddress=%08o\n",cpu . TPR.TSR, address, finalAddress);
    goto HI;

HI:

// Check for conditions that change the PPR.P bit

    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(HI)\n");
    
    //if (thisCycle == INSTRUCTION_FETCH)
    if (thisCycle == INDIRECT_WORD_FETCH)
        goto Exit;
    
    if (thisCycle == RTCD_OPERAND_FETCH)
        goto KL;
    
    //if (i && ((i->info->flags & TRANSFER_INS) || instructionFetch))
    if (instructionFetch || (i && (i->info->flags & TRANSFER_INS)))
        goto KL;
    
// XXX "APU data movement; Load/store APU data" not implemented"
// XXX ticket 12
    // load/store data .....

    
    goto Exit;
 
KL:;

// We end up here if the operand data is destined for the IC.
//  Set PPR.P

    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(KLM)\n");
    
    if (cpu . TPR.TRR == 0)
        // C(SDW.P) → C(PPR.P)
        cpu . PPR.P = cpu . SDW->P;
    else
        // 0 → C(PPR.P)
        cpu . PPR.P = 0;
    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(KLM) TPR.TSR %05o TPR.CA %06o\n", cpu.TPR.TSR, cpu.TPR.CA);
    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(KLM) TPR.TRR %o SDW.P %o PPR.P %o\n", cpu.TPR.TRR, cpu.SDW->P, cpu.PPR.P);
    
    goto Exit;    // this may not be setup or right
    

    
    
   
Exit:;
//    sim_debug(DBG_APPENDING, &cpu_dev, "doAppendCycle(Exit): lastCycle: %s => %s\n", strPCT(lastCycle), strPCT(thisCycle));

    cpu . TPR . CA = address;
    return finalAddress;    // or 0 or -1???
}

// Translate a segno:offset to a absolute address.
// Return 0 if successful.

int dbgLookupAddress (word18 segno, word18 offset, word24 * finalAddress,
                      char * * msg)
  {
    // Local copies so we don't disturb machine state

    _ptw0 PTW1;
    _sdw0 SDW1;

   if (2u * segno >= 16u * (cpu . DSBR.BND + 1u))
     {
       if (msg)
         * msg = "DSBR boundary violation.";
       return 1;
     }

    if (cpu . DSBR . U == 0)
      {
        // fetchDSPTW

        word24 y1 = (2 * segno) % 1024;
        word24 x1 = (2 * segno - y1) / 1024;

        word36 PTWx1;
        core_read ((cpu . DSBR . ADDR + x1) & PAMASK, & PTWx1, __func__);
        
        PTW1 . ADDR = GETHI (PTWx1);
        PTW1 . U = TSTBIT (PTWx1, 9);
        PTW1 . M = TSTBIT (PTWx1, 6);
        PTW1 . DF = TSTBIT (PTWx1, 2);
        PTW1 . FC = PTWx1 & 3;
    
        if (! PTW1 . DF)
          {
            if (msg)
              * msg = "!PTW0.F";
            return 2;
          }

        // fetchPSDW

        y1 = (2 * segno) % 1024;
    
        word36 SDWeven, SDWodd;
    
        core_read2 (((PTW1 .  ADDR << 6) + y1) & PAMASK, & SDWeven, & SDWodd, __func__);
    
        // even word
        SDW1 . ADDR = (SDWeven >> 12) & 077777777;
        SDW1 . R1 = (SDWeven >> 9) & 7;
        SDW1 . R2 = (SDWeven >> 6) & 7;
        SDW1 . R3 = (SDWeven >> 3) & 7;
        SDW1 . DF = TSTBIT(SDWeven, 2);
        SDW1 . FC = SDWeven & 3;
    
        // odd word
        SDW1 . BOUND = (SDWodd >> 21) & 037777;
        SDW1 . R = TSTBIT (SDWodd, 20);
        SDW1 . E = TSTBIT (SDWodd, 19);
        SDW1 . W = TSTBIT (SDWodd, 18);
        SDW1 . P = TSTBIT (SDWodd, 17);
        SDW1 . U = TSTBIT (SDWodd, 16);
        SDW1 . G = TSTBIT (SDWodd, 15);
        SDW1 . C = TSTBIT (SDWodd, 14);
        SDW1 . EB = SDWodd & 037777;
      }
    else // ! DSBR . U
      {
        // fetchNSDW

        word36 SDWeven, SDWodd;
        
        core_read2 ((cpu . DSBR . ADDR + 2 * segno) & PAMASK, & SDWeven, & SDWodd, __func__);
        
        // even word
        SDW1 . ADDR = (SDWeven >> 12) & 077777777;
        SDW1 . R1 = (SDWeven >> 9) & 7;
        SDW1 . R2 = (SDWeven >> 6) & 7;
        SDW1 . R3 = (SDWeven >> 3) & 7;
        SDW1 . DF = TSTBIT (SDWeven, 2);
        SDW1 . FC = SDWeven & 3;
        
        // odd word
        SDW1 . BOUND = (SDWodd >> 21) & 037777;
        SDW1 . R = TSTBIT(SDWodd, 20);
        SDW1 . E = TSTBIT(SDWodd, 19);
        SDW1 . W = TSTBIT(SDWodd, 18);
        SDW1 . P = TSTBIT(SDWodd, 17);
        SDW1 . U = TSTBIT(SDWodd, 16);
        SDW1 . G = TSTBIT(SDWodd, 15);
        SDW1 . C = TSTBIT(SDWodd, 14);
        SDW1 . EB = SDWodd & 037777;
    
      }

    if (SDW1 . DF == 0)
      {
        if (msg)
          * msg = "!SDW0.F != 0";
        return 3;
      }

    if (((offset >> 4) & 037777) > SDW1 . BOUND)
      {
        if (msg)
          * msg = "C(TPR.CA)0,13 > SDW.BOUND";
        return 4;
      }

    // is segment C(TPR.TSR) paged?
    if (SDW1 . U)
      {
        * finalAddress = (SDW1 . ADDR + offset) & PAMASK;
      }
    else
      {
        // fetchPTW
        word24 y2 = offset % 1024;
        word24 x2 = (offset - y2) / 1024;
    
        word36 PTWx2;
    
        core_read ((SDW1 . ADDR + x2) & PAMASK, & PTWx2, __func__);
    
        PTW1 . ADDR = GETHI (PTWx2);
        PTW1 . U = TSTBIT (PTWx2, 9);
        PTW1 . M = TSTBIT (PTWx2, 6);
        PTW1 . DF = TSTBIT (PTWx2, 2);
        PTW1 . FC = PTWx2 & 3;

        if ( !PTW1 . DF)
          {
            if (msg)
              * msg = "!PTW0.F";
            return 5;
          }

        y2 = offset % 1024;
    
        * finalAddress = (((PTW1 . ADDR & 0777777) << 6) + y2) & PAMASK;
      }
    if (msg)
      * msg = "";
    return 0;
  }



