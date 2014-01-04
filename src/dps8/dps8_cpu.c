 /**
 * \file dps8_cpu.c
 * \project dps8
 * \date 9/17/12
 * \copyright Copyright (c) 2012 Harry Reed. All rights reserved.
*/

#include <stdio.h>

#include "dps8.h"
#include "dps8_utils.h"

// XXX Use this when we assume there is only a single unit
#define ASSUME0 0

void cpu_reset_array (void);

/* CPU data structures
 
 cpu_dev      CPU device descriptor
 cpu_unit     CPU unit
 cpu_reg      CPU register list
 cpu_mod      CPU modifier list
 */

#define N_CPU_UNITS 1
// The DPS8M had only 4 ports
#define N_CPU_PORTS 4

UNIT cpu_unit [N_CPU_UNITS] = {{ UDATA (NULL, UNIT_FIX|UNIT_BINK, MEMSIZE) }};
#define UNIT_NUM(uptr) ((uptr) - cpu_unit)
static t_stat cpu_show_config(FILE *st, UNIT *uptr, int val, void *desc);
static t_stat cpu_set_config (UNIT * uptr, int32 value, char * cptr, void * desc);
/*! CPU modifier list */
MTAB cpu_mod[] = {
    /* { UNIT_V_UF, 0, "STD", "STD", NULL }, */
    //{ MTAB_XTD|MTAB_VDV, 0, "SPECIAL", NULL, NULL, &spec_disp },
    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO /* | MTAB_VALR */, /* mask */
      0,            /* match */
      "CONFIG",     /* print string */
      "CONFIG",         /* match string */
      cpu_set_config,         /* validation routine */
      cpu_show_config, /* display routine */
      NULL          /* value descriptor */
    },
    { 0 }
};

static DEBTAB cpu_dt[] = {
    { "TRACE",      DBG_TRACE       },
    { "TRACEEX",    DBG_TRACEEXT    },
    { "MESSAGES",   DBG_MSG         },

    { "REGDUMPAQI", DBG_REGDUMPAQI  },
    { "REGDUMPIDX", DBG_REGDUMPIDX  },
    { "REGDUMPPR",  DBG_REGDUMPPR   },
    { "REGDUMPADR", DBG_REGDUMPADR  },
    { "REGDUMPPPR", DBG_REGDUMPPPR  },
    { "REGDUMPDSBR",DBG_REGDUMPDSBR },
    { "REGDUMPFLT", DBG_REGDUMPFLT  },
    { "REGDUMP",    DBG_REGDUMP     }, // don't move as it messes up DBG message

    { "ADDRMOD",    DBG_ADDRMOD     },
    { "APPENDING",  DBG_APPENDING   },

    { "WARN",       DBG_WARN        },
    { "DEBUG",      DBG_DEBUG       },
    { "INFO",       DBG_INFO        },
    { "NOTIFY",     DBG_NOTIFY      },
    { "ALL",        DBG_ALL         }, // don't move as it messes up DBG message

    { "FAULT",      DBG_FAULT       },

    { NULL,         0               }
};

// This is part of the simh interface
const char *sim_stop_messages[] = {
    "Unknown error",           // STOP_UNK
    "Unimplemented Opcode",    // STOP_UNIMP
    "DIS instruction",         // STOP_DIS
    "Breakpoint",              // STOP_BKPT
    "Invalid Opcode",          // STOP_INVOP
    "Stop code - 5",           // STOP_5
    "BUG",                     // STOP_BUG
    "WARNING"                  // STOP_WARN
    "Fault cascade",           // STOP_FLT_CASCADE
};

/* End of simh interface */

/* Processor configuration switches 
 *
 * From AM81-04 Multics System Maintainance Procedures
 *
 * "A level 68 IOM system may containa maximum of 7 CPUs, 4 IOMs, 8 SCUs and 16MW of memory
 * [CAC]: but AN87 says multics only supports two IOMs
 * 
 * ASSIGNMENT: 3 toggle switches determine the base address of the SCU connected
 * to the port. The base address (in KW) is the product of this number and the value
 * defined by the STORE SIZE patch plug for the port.
 *
 * ADDRESS RANGE: toggle FULL/HALF. Determines the size of the SCU as full or half
 * of the STORE SIZE patch.
 *
 * PORT ENABLE: (4? toggles)
 *
 * INITIALIZE ENABLE: (4? toggles) These switches enable the receipt of an 
 * initialize signal from the SCU connected to the ports. This signal is used
 * during the first part of bootload to set all CPUs to a known (idle) state.
 * The switch for each port connected to an SCU should be ON, otherwise off.
 *
 * INTERLACE: ... All INTERLACE switches should be OFF for Multics operation.
 *

 */


//t_stat spec_disp (FILE *st, UNIT *uptr, int value, void *desc)
//{
//    printf("In spec_disp()....\n");
//    return SCPE_OK;
//}

/*
 * init_opcodes()
 *
 * This initializes the is_eis[] array which we use to detect whether or
 * not an instruction is an EIS instruction.
 *
 * TODO: Change the array values to show how many operand words are
 * used.  This would allow for better symbolic disassembly.
 *
 * BUG: unimplemented instructions may not be represented
 */

int is_eis[1024];    // hack

void init_opcodes (void)
{
    memset(is_eis, 0, sizeof(is_eis));
    
    is_eis[(opcode1_cmpc<<1)|1] = 1;
    is_eis[(opcode1_scd<<1)|1] = 1;
    is_eis[(opcode1_scdr<<1)|1] = 1;
    is_eis[(opcode1_scm<<1)|1] = 1;
    is_eis[(opcode1_scmr<<1)|1] = 1;
    is_eis[(opcode1_tct<<1)|1] = 1;
    is_eis[(opcode1_tctr<<1)|1] = 1;
    is_eis[(opcode1_mlr<<1)|1] = 1;
    is_eis[(opcode1_mrl<<1)|1] = 1;
    is_eis[(opcode1_mve<<1)|1] = 1;
    is_eis[(opcode1_mvt<<1)|1] = 1;
    is_eis[(opcode1_cmpn<<1)|1] = 1;
    is_eis[(opcode1_mvn<<1)|1] = 1;
    is_eis[(opcode1_mvne<<1)|1] = 1;
    is_eis[(opcode1_csl<<1)|1] = 1;
    is_eis[(opcode1_csr<<1)|1] = 1;
    is_eis[(opcode1_cmpb<<1)|1] = 1;
    is_eis[(opcode1_sztl<<1)|1] = 1;
    is_eis[(opcode1_sztr<<1)|1] = 1;
    is_eis[(opcode1_btd<<1)|1] = 1;
    is_eis[(opcode1_dtb<<1)|1] = 1;
    is_eis[(opcode1_dv3d<<1)|1] = 1;
}


/*!
 * initialize segment table according to the contents of DSBR ...
 */
t_stat dpsCmd_InitUnpagedSegmentTable ()
{
    if (DSBR.U == 0)
    {
        sim_printf("Cannot initialize unpaged segment table because DSBR.U says it is \"paged\"\n");
        return SCPE_OK;    // need a better return value
    }
    
    if (DSBR.ADDR == 0) // DSBR *probably* not initialized. Issue warning and ask....
        if (!get_yn ("DSBR *probably* uninitialized (DSBR.ADDR == 0). Proceed anyway [N]?", FALSE))
            return SCPE_OK;
    
    
    word15 segno = 0;
    while (2 * segno < (16 * (DSBR.BND + 1)))
    {
        //generate target segment SDW for DSBR.ADDR + 2 * segno.
        word24 a = DSBR.ADDR + 2 * segno;
        
        // just fill with 0's for now .....
        core_write(a + 0, 0);
        core_write(a + 1, 0);
        
        segno += 1; // onto next segment SDW
    }
    
    if (!sim_quiet) sim_printf("zero-initialized segments 0 .. %d\n", segno - 1);
    return SCPE_OK;
}

t_stat dpsCmd_InitSDWAM ()
{
    memset(SDWAM, 0, sizeof(SDWAM));
    
    if (!sim_quiet) sim_printf("zero-initialized SDWAM\n");
    return SCPE_OK;
}

_sdw0 *fetchSDW(word15 segno)
{
    word36 SDWeven, SDWodd;
    
    core_read2(DSBR.ADDR + 2 * segno, &SDWeven, &SDWodd);
    
    // even word
    static _sdw0 _s;
    
    _sdw0 *SDW = &_s;   //calloc(1, sizeof(_sdw0));
    memset(SDW, 0, sizeof(_s));
    
    SDW->ADDR = (SDWeven >> 12) & 077777777;
    SDW->R1 = (SDWeven >> 9) & 7;
    SDW->R2 = (SDWeven >> 6) & 7;
    SDW->R3 = (SDWeven >> 3) & 7;
    SDW->F = TSTBIT(SDWeven, 2);
    SDW->FC = SDWeven & 3;
    
    // odd word
    SDW->BOUND = (SDWodd >> 21) & 037777;
    SDW->R = TSTBIT(SDWodd, 20);
    SDW->E = TSTBIT(SDWodd, 19);
    SDW->W = TSTBIT(SDWodd, 18);
    SDW->P = TSTBIT(SDWodd, 17);
    SDW->U = TSTBIT(SDWodd, 16);
    SDW->G = TSTBIT(SDWodd, 15);
    SDW->C = TSTBIT(SDWodd, 14);
    SDW->EB = SDWodd & 037777;
    
    return SDW;
}

char *strDSBR(void)
{
    static char buff[256];
    sprintf(buff, "DSBR: ADDR=%06o BND=%05o U=%o STACK=%04o", DSBR.ADDR, DSBR.BND, DSBR.U, DSBR.STACK);
    return buff;
}

PRIVATE
void printDSBR()
{
    sim_printf("%s\n", strDSBR());
}


char *strSDW0(_sdw0 *SDW)
{
    static char buff[256];
    
    //if (SDW->ADDR == 0 && SDW->BOUND == 0) // need a better test
    if (!SDW->F) 
        sprintf(buff, "*** Uninitialized ***");
    else
        sprintf(buff, "ADDR=%06o R1=%o R2=%o R3=%o F=%o FC=%o BOUND=%o R=%o E=%o W=%o P=%o U=%o G=%o C=%o EB=%o",
                SDW->ADDR, SDW->R1, SDW->R2, SDW->R3, SDW->F, SDW->FC, SDW->BOUND, SDW->R, SDW->E, SDW->W,
                SDW->P, SDW->U, SDW->G, SDW->C, SDW->EB);
    return buff;
}

PRIVATE
void printSDW0(_sdw0 *SDW)
{
    sim_printf("%s\n", strSDW0(SDW));
}

t_stat dpsCmd_DumpSegmentTable()
{
    sim_printf("*** Descriptor Segment Base Register (DSBR) ***\n");
    printDSBR();
    sim_printf("*** Descriptor Segment Table ***\n");
    for(word15 segno = 0; 2 * segno < 16 * (DSBR.BND + 1); segno += 1)
    {
        sim_printf("Seg %d - ", segno);
        _sdw0 *s = fetchSDW(segno);
        printSDW0(s);
        
        //free(s); no longer needed
    }
    return SCPE_OK;
}

//! custom command "dump"
t_stat dpsCmd_Dump (int32 arg, char *buf)
{
    char cmds [256][256];
    memset(cmds, 0, sizeof(cmds));  // clear cmds buffer
    
    int nParams = sscanf(buf, "%s %s %s %s", cmds[0], cmds[1], cmds[2], cmds[3]);
    if (nParams == 2 && !strcasecmp(cmds[0], "segment") && !strcasecmp(cmds[1], "table"))
        return dpsCmd_DumpSegmentTable();
    if (nParams == 1 && !strcasecmp(cmds[0], "sdwam"))
        return dumpSDWAM();
    
    return SCPE_OK;
}

//! custom command "init"
t_stat dpsCmd_Init (int32 arg, char *buf)
{
    char cmds [8][32];
    memset(cmds, 0, sizeof(cmds));  // clear cmds buffer
    
    int nParams = sscanf(buf, "%s %s %s %s", cmds[0], cmds[1], cmds[2], cmds[3]);
    if (nParams == 2 && !strcasecmp(cmds[0], "segment") && !strcasecmp(cmds[1], "table"))
        return dpsCmd_InitUnpagedSegmentTable();
    if (nParams == 1 && !strcasecmp(cmds[0], "sdwam"))
        return dpsCmd_InitSDWAM();
    //if (nParams == 2 && !strcasecmp(cmds[0], "stack"))
    //    return createStack((int)strtoll(cmds[1], NULL, 8));
    
    return SCPE_OK;
}

//! custom command "segment" - stuff to do with deferred segments
t_stat dpsCmd_Segment (int32 arg, char *buf)
{
    char cmds [8][32];
    memset(cmds, 0, sizeof(cmds));  // clear cmds buffer
    
    /*
      cmds   0     1      2     3
     segment ??? remove
     segment ??? segref remove ????
     segment ??? segdef remove ????
     */
    int nParams = sscanf(buf, "%s %s %s %s %s", cmds[0], cmds[1], cmds[2], cmds[3], cmds[4]);
    if (nParams == 2 && !strcasecmp(cmds[0], "remove"))
        return removeSegment(cmds[1]);
    if (nParams == 4 && !strcasecmp(cmds[1], "segref") && !strcasecmp(cmds[2], "remove"))
        return removeSegref(cmds[0], cmds[3]);
    if (nParams == 4 && !strcasecmp(cmds[1], "segdef") && !strcasecmp(cmds[2], "remove"))
        return removeSegdef(cmds[0], cmds[3]);
    
    return SCPE_ARG;
}

//! custom command "segments" - stuff to do with deferred segments
t_stat dpsCmd_Segments (int32 arg, char *buf)
{
    bool bVerbose = !sim_quiet;

    char cmds [8][32];
    memset(cmds, 0, sizeof(cmds));  // clear cmds buffer
    
    /*
     * segments resolve
     * segments load deferred
     * segments remove ???
     */
    int nParams = sscanf(buf, "%s %s %s %s", cmds[0], cmds[1], cmds[2], cmds[3]);
    //if (nParams == 2 && !strcasecmp(cmds[0], "segment") && !strcasecmp(cmds[1], "table"))
    //    return dpsCmd_InitUnpagedSegmentTable();
    //if (nParams == 1 && !strcasecmp(cmds[0], "sdwam"))
    //    return dpsCmd_InitSDWAM();
    if (nParams == 1 && !strcasecmp(cmds[0], "resolve"))
        return resolveLinks(bVerbose);    // resolve external reverences in deferred segments
   
    if (nParams == 2 && !strcasecmp(cmds[0], "load") && !strcasecmp(cmds[1], "deferred"))
        return loadDeferredSegments(bVerbose);    // load all deferred segments
    
    if (nParams == 2 && !strcasecmp(cmds[0], "remove"))
        return removeSegment(cmds[1]);

    if (nParams == 2 && !strcasecmp(cmds[0], "lot") && !strcasecmp(cmds[1], "create"))
        return createLOT(bVerbose);
    if (nParams == 2 && !strcasecmp(cmds[0], "lot") && !strcasecmp(cmds[1], "snap"))
        return snapLOT(bVerbose);

    if (nParams == 3 && !strcasecmp(cmds[0], "create") && !strcasecmp(cmds[1], "stack"))
    {
        int _n = (int)strtoll(cmds[2], NULL, 8);
        return createStack(_n, bVerbose);
    }
    return SCPE_ARG;
}

/*! Reset routine */
t_stat cpu_reset_mm (DEVICE *dptr)
{
    
    sim_debug (DBG_INFO, & cpu_dev, "CPU reset: Running\n");
    
    ic_history_init();
    
    memset(&events, 0, sizeof(events));
    memset(&cpu, 0, sizeof(cpu));
    memset(&cu, 0, sizeof(cu));
    memset(&PPR, 0, sizeof(PPR));
    cu.SD_ON = 1;
    cu.PT_ON = 1;
    cpu.ic_odd = 0;
    
    // TODO: reset *all* other structures to zero
    
    set_addr_mode(ABSOLUTE_mode);
    
    // We statup with either a fault or an interrupt.  So, a trap pair from the
    // appropriate location will end up being the first instructions executed.
#ifdef PUTBACK_WHEN_WE_WANT_TO_StART_NORMALLY
    if (sys_opts.startup_interrupt) {
        // We'll first generate interrupt #4.  The IOM will have initialized
        // memory to have a DIS (delay until interrupt set) instruction at the
        // memory location used to hold the trap words for this interrupt.
        cpu.cycle = INTERRUPT_cycle;
        events.int_pending = 1;
        events.interrupts[4] = 1;   // system fault, IOM zero, channels 0-31 -- MDD-005
    } else {
        // Generate a startup fault.
        // We'll end up in a loop of trouble faults for opcode zero until the IOM
        // finally overwites the trouble fault vector with a DIS from the tape
        // label.
        cpu.cycle = FETCH_cycle;
        fault_gen(startup_fault);   // pressing POWER ON button causes this fault
    }
#endif
    cpu.cycle = FETCH_cycle;

    cpuCycles = 0;

//#if FEAT_INSTR_STATS
    memset(&sys_stats, 0, sizeof(sys_stats));
//#endif
    
    return 0;
}

t_stat cpu_boot (int32 unit_num, DEVICE *dptr)
{
    // The boot button on the cpu is conneted to the boot button on the IOM
    // XXX is this true? Which IOM is it connected to?
    //return iom_boot (ASSUME0, & iom_dev);
    sim_printf ("Try 'BOOT IOMn'\n");
    return SCPE_ARG;
}

t_stat cpu_reset (DEVICE *dptr)
{
    if (M)
        free(M);
    
    M = (word36 *) calloc (MEMSIZE, sizeof (word36));
    if (M == NULL)
        return SCPE_MEM;
    
    rIC = 0;
    rA = 0;
    rQ = 0;
    XECD = 0;
    
    PPR.IC = 0;
    PPR.PRR = 0;
    PPR.PSR = 0;
    PPR.P = 1;
    
    processorCycle = UNKNOWN_CYCLE;
    //processorAddressingMode = ABSOLUTE_MODE;
    set_addr_mode(ABSOLUTE_mode);
    
    sim_brk_types = sim_brk_dflt = SWMASK ('E');

    cpuCycles = 0;
    
    CMR.luf = 3;    // default of 16 mS
    
    // XXX free up previous deferred segments (if any)
    
    
    cpu_reset_mm(dptr);

    cpu_reset_array ();

    initializeTheMatrix();

    return SCPE_OK;
}

/*! Memory examine */
//  t_stat examine_routine (t_val *eval_array, t_addr addr, UNIT *uptr, int32 switches) – Copy 
//  sim_emax consecutive addresses for unit uptr, starting at addr, into eval_array. The switch 
//  variable has bit<n> set if the n’th letter was specified as a switch to the examine command. 
// Not true...

t_stat cpu_ex (t_value *vptr, t_addr addr, UNIT *uptr, int32 sw)
{
    if (addr>= MEMSIZE)
        return SCPE_NXM;
    if (vptr != NULL)
      {
        *vptr = M[addr] & DMASK;
      }
    return SCPE_OK;
}

/*! Memory deposit */
t_stat cpu_dep (t_value val, t_addr addr, UNIT *uptr, int32 sw)
{
    if (addr >= MEMSIZE) return SCPE_NXM;
    M[addr] = val & DMASK;
    return SCPE_OK;
}


enum _processor_cycle_type processorCycle;                  ///< to keep tract of what type of cycle the processor is in
//enum _processor_addressing_mode processorAddressingMode;    ///< what addressing mode are we using
enum _processor_operating_mode processorOperatingMode;      ///< what operating mode

// h6180 stuff
/* [map] designates mapping into 36-bit word from DPS-8 proc manual */

/* GE-625/635 */

word36	rA;	/*!< accumulator */
word36	rQ;	/*!< quotient */
word8	rE;	/*!< exponent [map: rE, 28 0's] */

word18	rX[8];	/*!< index */



//word18	rBAR;	/*!< base address [map: BAR, 18 0's] */
/* format: 9b base, 9b bound */

int XECD; /*!< for out-of-line XEC,XED,faults, etc w/o rIC fetch */
word36 XECD1; /*!< XEC instr / XED instr#1 */
word36 XECD2; /*!< XED instr#2 */


//word18	rIC;	/*!< instruction counter */
// same as PPR.IC
 //word18	rIR;	/*!< indicator [15b] [map: 18 x's, rIR w/ 3 0's] */
//IR_t IR;        // Indicator register   (until I can map MM IR to my rIR)


word27	rTR;	/*!< timer [map: TR, 9 0's] */

word18	ry;     /*!< address operand */
word24	rY;     /*!< address operand */
word8	rTAG;	/*!< instruction tag */

word8	tTB;	/*!< char size indicator (TB6=6-bit,TB9=9-bit) [3b] */
word8	tCF;	/*!< character position field [3b] */

/* GE-645 */


//word36	rDBR;	/*!< descriptor base */
//!<word27	rABR[8];/*!< address base */
//
///* H6180; L68; DPS-8M */
//
word8	rRALR;	/*!< ring alarm [3b] [map: 33 0's, RALR] */
//word36	rPRE[8];/*!< pointer, even word */
//word36	rPRO[8];/*!< pointer, odd word */
//word27	rAR[8];	/*!< address [24b] [map: ARn, 12 0's] */
//word36	rPPRE;	/*!< procedure pointer, even word */
//word36	rPPRO;	/*!< procedure pointer, odd word */
//word36	rTPRE;	/*!< temporary pointer, even word */
//word36	rTPRO;	/*!< temporary pointer, odd word */
//word36	rDSBRE;	/*!< descriptor segment base, even word */
//word36	rDSBRO;	/*!< descriptor segment base, odd word */
//word36	mSE[16];/*!< word-assoc-mem: seg descrip regs, even word */
//word36	mSO[16];/*!< word-assoc-mem: seg descrip regs, odd word */
//word36	mSP[16];/*!< word-assoc-mem: seg descrip ptrs */
//word36	mPR[16];/*!< word-assoc-mem: page tbl regs */
//word36	mPP[16];/*!< word-assoc-mem: page tbl ptrs */
//word36	rFRE;	/*!< fault, even word */
//word36	rFRO;	/*!< fault, odd word */
//word36	rMR;	/*!< mode */
//word36	rCMR;	/*!< cache mode */
//word36	hCE[16];/*!< history: control unit, even word */
//word36	hCO[16];/*!< history: control unit, odd word */
//word36	hOE[16];/*!< history: operations unit, even word */
//word36	hOO[16];/*!< history: operations unit, odd word */
//word36	hDE[16];/*!< history: decimal unit, even word */
//word36	hDO[16];/*!< history: decimal unit, odd word */
//word36	hAE[16];/*!< history: appending unit, even word */
//word36	hAO[16];/*!< history: appending unit, odd word */
//word36	rSW[5];	/*!< switches */

//word12 rFAULTBASE;  ///< fault base (12-bits of which the top-most 7-bits are used)

// end h6180 stuff

struct _tpr TPR;    ///< Temporary Pointer Register
struct _ppr PPR;    ///< Procedure Pointer Register
//struct _prn PR[8];  ///< Pointer Registers
//struct _arn AR[8];  ///< Address Registers
struct _par PAR[8]; ///< pointer/address resisters
struct _bar BAR;    ///< Base Address Register
struct _dsbr DSBR;  ///< Descriptor Segment Base Register


// XXX given this is not real hardware we can eventually remove the SDWAM -- I think. But for now just leave it in.
// For the DPS 8M processor, the SDW associative memory will hold the 64 MRU SDWs and have a 4-way set associative organization with LRU replacement.

struct _sdw  SDWAM[64], *SDW = &SDWAM[0];    ///< Segment Descriptor Word Associative Memory & working SDW
struct _sdw0 SDW0;  ///< a SDW not in SDWAM

struct _ptw PTWAM[64], *PTW = &PTWAM[0];    ///< PAGE TABLE WORD ASSOCIATIVE MEMORY and working PTW
struct _ptw0 PTW0;  ///< a PTW not in PTWAM (PTWx1)

_cache_mode_register CMR;
_mode_register MR;

/*
 * register stuff ...
 */
static const char *z1[] = {"0", "1"};
static BITFIELD dps8_IR_bits[] = {    
    BITNCF(3),
    BITFNAM(HEX,   1, z1),    /*!< base-16 exponent */ ///< 0000010
    BITFNAM(ABS,   1, z1),    /*!< absolute mode */ ///< 0000020
    BITFNAM(MIIF,  1, z1),	  /*!< mid-instruction interrupt fault */ ///< 0000040
    BITFNAM(TRUNC, 1, z1),    /*!< truncation */ ///< 0000100
    BITFNAM(NBAR,  1, z1),	  /*!< not BAR mode */ ///< 0000200
    BITFNAM(PMASK, 1, z1),	  /*!< parity mask */ ///< 0000400
    BITFNAM(PAR,   1, z1),    /*!< parity error */ ///< 0001000
    BITFNAM(TALLY, 1, z1),	  /*!< tally runout */ ///< 0002000
    BITFNAM(OMASK, 1, z1),    /*!< overflow mask */ ///< 0004000
    BITFNAM(EUFL,  1, z1),	  /*!< exponent underflow */ ///< 0010000
    BITFNAM(EOFL,  1, z1),	  /*!< exponent overflow */ ///< 0020000
    BITFNAM(OFLOW, 1, z1),	  /*!< overflow */ ///< 0040000
    BITFNAM(CARRY, 1, z1),	  /*!< carry */ ///< 0100000
    BITFNAM(NEG,   1, z1),    /*!< negative */ ///< 0200000
    BITFNAM(ZERO,  1, z1),	  /*!< zero */ ///< 0400000
    ENDBITS
};

static REG cpu_reg[] = {
    { ORDATA (IC, rIC, VASIZE) },
    //{ ORDATA (IR, rIR, 18) },
    { ORDATADF (IR, rIR, 18, "Indicator Register", dps8_IR_bits) },
    
    //    { FLDATA (Zero, rIR, F_V_A) },
    //    { FLDATA (Negative, rIR, F_V_B) },
    //    { FLDATA (Carry, rIR, F_V_C) },
    //    { FLDATA (Overflow, rIR, F_V_D) },
    //    { FLDATA (ExpOvr, rIR, F_V_E) },
    //    { FLDATA (ExpUdr, rIR, F_V_F) },
    //    { FLDATA (OvrMask, rIR, F_V_G) },
    //    { FLDATA (TallyRunOut, rIR, F_V_H) },
    //    { FLDATA (ParityErr, rIR, F_V_I) }, ///< Yeah, right!
    //    { FLDATA (ParityMask, rIR, F_V_J) },
    //    { FLDATA (NotBAR, rIR, F_V_K) },
    //    { FLDATA (Trunc, rIR, F_V_L) },
    //    { FLDATA (MidInsFlt, rIR, F_V_M) },
    //    { FLDATA (AbsMode, rIR, F_V_N) },
    //    { FLDATA (HexMode, rIR, F_V_O) },
    
    { ORDATA (A, rA, 36) },
    { ORDATA (Q, rQ, 36) },
    { ORDATA (E, rE, 8) },
    
    { ORDATA (X0, rX[0], 18) },
    { ORDATA (X1, rX[1], 18) },
    { ORDATA (X2, rX[2], 18) },
    { ORDATA (X3, rX[3], 18) },
    { ORDATA (X4, rX[4], 18) },
    { ORDATA (X5, rX[5], 18) },
    { ORDATA (X6, rX[6], 18) },
    { ORDATA (X7, rX[7], 18) },
    
    { ORDATA (PPR.IC,  PPR.IC,  18) },
    { ORDATA (PPR.PRR, PPR.PRR,  3) },
    { ORDATA (PPR.PSR, PPR.PSR, 15) },
    { ORDATA (PPR.P,   PPR.P,    1) },
    
    { ORDATA (DSBR.ADDR,  DSBR.ADDR,  24) },
    { ORDATA (DSBR.BND,   DSBR.BND,   14) },
    { ORDATA (DSBR.U,     DSBR.U,      1) },
    { ORDATA (DSBR.STACK, DSBR.STACK, 12) },
    
    { ORDATA (BAR.BASE,  BAR.BASE,  9) },
    { ORDATA (BAR.BOUND, BAR.BOUND, 9) },
    
    { ORDATA (FAULTBASE, rFAULTBASE, 12) }, ///< only top 7-msb are used
    
    { ORDATA (PR0, PR[0], 18) },
    { ORDATA (PR1, PR[1], 18) },
    { ORDATA (PR2, PR[2], 18) },
    { ORDATA (PR3, PR[3], 18) },
    { ORDATA (PR4, PR[4], 18) },
    { ORDATA (PR5, PR[5], 18) },
    { ORDATA (PR6, PR[6], 18) },
    { ORDATA (PR7, PR[7], 18) },
    
    
    /*
     { ORDATA (EBR, ebr, EBR_N_EBR) },
     { FLDATA (PGON, ebr, EBR_V_PGON) },
     { FLDATA (T20P, ebr, EBR_V_T20P) },
     { ORDATA (UBR, ubr, 36) },
     { GRDATA (CURAC, ubr, 8, 3, UBR_V_CURAC), REG_RO },
     { GRDATA (PRVAC, ubr, 8, 3, UBR_V_PRVAC) },
     { ORDATA (SPT, spt, 36) },
     { ORDATA (CST, cst, 36) },
     { ORDATA (PUR, pur, 36) },
     { ORDATA (CSTM, cstm, 36) },
     { ORDATA (HSB, hsb, 36) },
     { ORDATA (DBR1, dbr1, PASIZE) },
     { ORDATA (DBR2, dbr2, PASIZE) },
     { ORDATA (DBR3, dbr3, PASIZE) },
     { ORDATA (DBR4, dbr4, PASIZE) },
     { ORDATA (PCST, pcst, 36) },
     { ORDATA (PIENB, pi_enb, 7) },
     { FLDATA (PION, pi_on, 0) },
     { ORDATA (PIACT, pi_act, 7) },
     { ORDATA (PIPRQ, pi_prq, 7) },
     { ORDATA (PIIOQ, pi_ioq, 7), REG_RO },
     { ORDATA (PIAPR, pi_apr, 7), REG_RO },
     { ORDATA (APRENB, apr_enb, 8) },
     { ORDATA (APRFLG, apr_flg, 8) },
     { ORDATA (APRLVL, apr_lvl, 3) },
     { ORDATA (RLOG, rlog, 10) },
     { FLDATA (F1PR, its_1pr, 0) },
     { BRDATA (PCQ, pcq, 8, VASIZE, PCQ_SIZE), REG_RO+REG_CIRC },
     { ORDATA (PCQP, pcq_p, 6), REG_HRO },
     { DRDATA (INDMAX, ind_max, 8), PV_LEFT + REG_NZ },
     { DRDATA (XCTMAX, xct_max, 8), PV_LEFT + REG_NZ },
     { ORDATA (WRU, sim_int_char, 8) },
     { FLDATA (STOP_ILL, stop_op0, 0) },
     { BRDATA (REG, acs, 8, 36, AC_NUM * AC_NBLK) },
     */
    
    { NULL }
};

/*
 * simh interface
 */

REG *sim_PC = &cpu_reg[0];

/*! CPU device descriptor */
DEVICE cpu_dev = {
    "CPU",          /*!< name */
    cpu_unit,       /*!< units */
    cpu_reg,        /*!< registers */
    cpu_mod,        /*!< modifiers */
    N_CPU_UNITS,    /*!< #units */
    8,              /*!< address radix */
    PASIZE,         /*!< address width */
    1,              /*!< addr increment */
    8,              /*!< data radix */
    36,             /*!< data width */
    &cpu_ex,        /*!< examine routine */
    &cpu_dep,       /*!< deposit routine */
    &cpu_reset,     /*!< reset routine */
    &cpu_boot,           /*!< boot routine */
    NULL,           /*!< attach routine */
    NULL,           /*!< detach routine */
    NULL,           /*!< context */
    DEV_DEBUG,      /*!< device flags */
    0,              /*!< debug control flags */
    cpu_dt,         /*!< debug flag names */
    NULL,           /*!< memory size change */
    NULL            /*!< logical name */
};

//word36 IWB;         ///< instruction working buffer
//opCode *iwb = NULL; ///< opCode *
//int32  opcode;      ///< opcode
//bool   opcodeX;     ///< opcode extension
//word18 address;     ///< bits 0-17 of instruction XXX replace with rY
//bool   a;           ///< bin-29 - indirect addressing mode?
//bool   i;           ///< interrupt inhinit bit.
//word6  tag;         ///< instruction tag XXX replace with rTAG

//word18 stiTally;    ///< for sti instruction


static DCDstruct *newDCDstruct(void);

static t_stat reason;

jmp_buf jmpMain;        ///< This is where we should return to from a fault or interrupt (if necessary)

static DCDstruct _currentInstruction;
DCDstruct *currentInstruction  = &_currentInstruction;;

static EISstruct E;
//EISstruct *e = &E;

events_t events;
switches_t switches;
// the following two should probably be combined
cpu_state_t cpu;
ctl_unit_data_t cu;


// This is an out-of-band flag for the APU. User commands to
// display or modify memory can invoke much the APU. Howeveer, we don't
// want interactive attempts to access non-existant memory locations
// to register a fault.
flag_t fault_gen_no_fault;

int stop_reason; // sim_instr return value for JMP_STOP

/*
 *  cancel_run()
 *
 *  Cancel_run can be called by any portion of the code to let
 *  sim_instr() know that it should stop looping and drop back
 *  to the SIMH command prompt.
 */

static int cancel;

void cancel_run(t_stat reason)
{
    // Maybe we should generate an OOB fault?
    
    (void) sim_cancel_step();
    if (cancel == 0 || reason < cancel)
        cancel = reason;
    //sim_debug (DBG_DEBUG, & cpu_dev, "CU: Cancel requested: %d\n", reason);
}

static uint get_highest_intr (void)
  {
// XXX In theory there needs to be interlocks on this?
    for (int int_num = 32 - 1; int_num >= 0; int_num --) // XXX Magic number
      if (events . interrupts [int_num])
        {
          events . interrupts [int_num] = 0;

          int cnt = 0;
          for (int i = 0; i < 32; i ++) // XXX Magic number
            if (events . interrupts [i])
              cnt ++;
          events . int_pending = !!cnt;

          for (int i = 0; i < 7; i ++) // XXX Magic number
            if (events . fault [i])
              cnt ++;
          events . any = !! cnt;

          return int_num * 2;
        }
    return 1;
  }

static bool sample_interrupts (void)
  {
    return events . int_pending;
  }

static int simh_hooks (void)
  {
    int reason = 0;
    // check clock queue 
    if (sim_interval <= 0)
      {
        reason = sim_process_event ();
        if (reason)
          return reason;
      }
        
    sim_interval --;

    // breakpoint? 
    if (sim_brk_summ && sim_brk_test (rIC, SWMASK ('E')))
        return STOP_BKPT; /* stop simulation */

    return reason;
  }       

//
// Okay, lets treat this as a state machine
//
//  DIS_cycle
//     spining wheels waiting for interrupt
//     if interruot, set INTERRUPT_cycle
//
//  INTERRUPT_cycle
//     clear interrupt, load interrupt pair into instruction buffer
//     set INTERRUPT_EXEC_cycle
//  INTERUPT_EXEC_cycle
//     execute instruction in instruction buffer
//     if (! transfer) set INTERUPT_EXEC2_cycle 
//     else set FETCH_cycle
//  INTERUPT_EXEC2_cycle
//     execute odd instruction in instruction buffer
//     set INTERUPT_EXEC2_cycle 
//
//  FAULT_cycle
//     fetch fault pair into instruction buffer
//     set FAULT_EXEC_cycle
//  FAULT_EXEC_cycle
//     execute instructions in instruction buffer
//     if (! transfer) set FAULT_EXE2_cycle 
//     else set FETCH_cycle
//  FAULT_EXEC2_cycle
//     execute odd instruction in instruction buffer
//     set FETCH_cycle
//
//  FETCH_cycle
//     fetch instruction into instruction buffer
//     set EXEC_cycle
//
//  EXEC_cycle
//     execute instruction in instruction buffer
//     if (repeat conditions) keep cycling
//     if (pair) set EXEC2_cycle
//     else set FETCH_cycle
//  EXEC2_cycle
//     execute odd instruction in instruction buffer
//
//  XEC_cycle
//     load instruction into instruction buffer
//     set EXEC_cyvle
//
//  XED_cycle
//     load instruction pair into instruction buffer
//     set EXEC_cyvle
//  
//  RPT_cycle
//     load instruction into instruction buffer
//     set repeat conditions
//     set EXEC_cycle
//
// other extant cycles:
//  ABORT_cycle

static word36 instr_buf [2];
static enum { IB_EMPTY, IB_SINGLE, IB_PAIR } instr_buf_state;

t_stat sim_instr (void)
  {
    // Heh. This needs to be static; longjmp resets the value to NULL
    static DCDstruct _ci;
    static DCDstruct * ci = & _ci;
    
    adrTrace  = (cpu_dev.dctrl & DBG_ADDRMOD  ) && sim_deb; // perform address mod tracing
    apndTrace = (cpu_dev.dctrl & DBG_APPENDING) && sim_deb; // perform APU tracing
    
    cpu . interrupt_flag = false;
    cpu . g7_flag = false;

    instr_buf_state = IB_EMPTY;
    // cpuCycles = 0; // XXX this should be done by reset(), messes up count on breakpoint

    // This allows long jumping to the top of the state machine
    int val = setjmp(jmpMain);

    switch (val)
    {
        case JMP_ENTRY:
            reason = 0;
            break;
#if 0
        case JMP_NEXT:
            goto jmpNext;
        case JMP_RETRY:
            goto jmpRetry;
        case JMP_TRA:
            goto jmpTra;
#endif
        case JMP_STOP:
            return stop_reason;
    }

    // Main instruction fetch/decode loop 

    do
      {

        reason = 0;

        reason = simh_hooks ();
        if (reason)
          return reason;

        switch (cpu . cycle)
          {
            case DIS_cycle:
                // spining wheels waiting for interrupt
                // if interruot, set INTERRUPT_cycle

                // Are g7 faults addressed in dis? XXX
                // e.g. time run out?

                cpu . interrupt_flag = sample_interrupts ();
                if (cpu . interrupt_flag)
                  cpu . cycle = INTERRUPT_cycle;
                break;

            case INTERRUPT_cycle:
                // clear interrupt, load interrupt pair into instruction buffer
                // set INTERRUPT_EXEC_cycle
                if (cpu . interrupt_flag)
                  {
                    // We should do this later, but doing it now allows us to
                    // avoid clean up for no interrupt pending.

                    uint intr_pair_addr;
                    intr_pair_addr = get_highest_intr ();
                    if (intr_pair_addr != 1) // no interrupts 
                      {

                        // In the INTERRUPT CYCLE, the processor safe-stores
                        // the Control Unit Data (see Section 3) into 
                        // program-invisible holding registers in preparation 
                        // for a Store Control Unit (scu) instruction, enters 
                        // temporary absolute mode, and forces the current 
                        // ring of execution C(PPR.PRR) to
                        // 0. It then issues an XEC system controller command 
                        // to the system controller on the highest priority 
                        // port for which there is a bit set in the interrupt 
                        // present register.  

                        cu_safe_store ();

                        // save address mode
                        // shouldn't this be in safe_store?
                        //addr_modes_t am = get_addr_mode();

                        // Temporary absolute mode
                        set_addr_mode (TEMPORARY_ABSOLUTE_mode);

                        // Set to ring 0
                        PPR . PRR = 0;

                        // get interrupt pair
                        core_read2(intr_pair_addr, instr_buf, instr_buf + 1);
                        instr_buf_state = IB_PAIR;

                        cpu . interrupt_flag = false;
                        cpu . cycle = INTERRUPT_EXEC_cycle;
                        break;
                      } // int_pair != 1
                  } // interrupt_flag
                // If we get here, there was no interrupt
                // The only place we enter INTERRUPT_cycle is in EXEC_cycle,
                // so go back there XXX true? cu_safe_restore() really
                // seems the way to go.
                cpu . cycle = EXEC_cycle;
                break;

            case INTERRUPT_EXEC_cycle:
            case INTERRUPT_EXEC2_cycle:
                //     execute instruction in instruction buffer
                //     if (! transfer) set INTERUPT_EXEC2_cycle 

                if (cpu . cycle == INTERRUPT_EXEC_cycle)
                  ci -> IWB = instr_buf [0];
                else
                  ci -> IWB = instr_buf [1];

                decodeInstruction (ci -> IWB, ci);
                t_stat ret = executeInstruction (ci);

                if (ret == CONT_TRA)
                  {
                     instr_buf_state = IB_EMPTY;
                     cpu . cycle = FETCH_cycle;
                     break;
                  }

                cu_safe_restore ();
                break;

            case FETCH_cycle:

                if (cpu . interrupt_flag)
                  {
                    cpu . cycle = INTERRUPT_cycle;
                    break;
                  }
                if (cpu . g7_flag)
                  {
                    cpu . cycle = FAULT_cycle;
                    break;
                  }

                processorCycle = SEQUENTIAL_INSTRUCTION_FETCH;
        

                ci = fetchInstruction(rIC, currentInstruction);    // fetch next instruction into current instruction struct
        
                // XXX The conditions are more rigorous: see AL39, pg 327
                if (rIC % 1 == 0 && // Even address
                    ci -> i == 0) // Not inhibited
                  {
                    cpu . interrupt_flag = sample_interrupts ();
                    cpu . g7_flag = bG7Pending ();
                  }
                else
                  {
                    cpu . interrupt_flag = false;
                    cpu . g7_flag = false;
                  }
                cpu . cycle = EXEC_cycle;
                break;

            case EXEC_cycle:
              {
                t_stat ret = executeInstruction (ci);
                if (ret > 0)
                  {
                     reason = ret;
                     break;
                  }
                if (ret < 0)
                  {
                    switch (ret)
                      {
                        case CONT_TRA: // Instruction transferred
                          break;   // don't bump rIC, instruction already did it
                        // shouldn't happen; doFault never returns
                        //case CONT_FAULT:
                        default:
                          sim_printf ("execute instruction returned %d?\n", ret);
                          break;
                      }
                  }
                if (ret == 0)
                  rIC ++;
                cpu . cycle = FETCH_cycle;
              }
              break;

            case FAULT_cycle:
              {
                sim_printf ("fault cycle\n");
    
                if (cpu . interrupt_flag)
                  {
                    cpu . cycle = INTERRUPT_cycle;
                    break;
                  }

                int fltAddress = (rFAULTBASE << 5) & 07740;            // (12-bits of which the top-most 7-bits are used)
                word24 addr = fltAddress + _faults [cpu . faultNumber] . fault_address;    // absolute address of fault YPair
  
                // XXX using core_read2 means decode instruction isn't used
                core_read2(addr, instr_buf, instr_buf + 1);



		// In the FAULT CYCLE, the processor safe-stores the Control
		// Unit Data (see Section 3) into program-invisible holding
		// registers in preparation for a Store Control Unit (scu)
		// instruction, then enters temporary absolute mode, forces the
		// current ring of execution C(PPR.PRR) to 0, and generates a
		// computed address for the fault trap pair by concatenating
		// the setting of the FAULT BASE switches on the processor
		// configuration panel with twice the fault number (see Table
		// 7-1). This computed address and the operation code for the
		// Execute Double (xed) instruction are forced into the
		// instruction register and executed as an instruction. Note
		// that the execution of the instruction is not done in a
		// normal EXECUTE CYCLE but in the FAULT CYCLE with the
		// processor in temporary absolute mode.
    
                PPR.PRR = 0;
                set_addr_mode (TEMPORARY_ABSOLUTE_mode);
    
                cpu . cycle = FAULT_EXEC_cycle;
#if 0
    t_stat xrv = doXED(faultPair);
    
    bFaultCycle = false;                // exit FAULT CYCLE
    bTroubleFaultCycle = false;
    if (xrv == CONT_TRA)
        longjmp(jmpMain, JMP_TRA);      // execute transfer instruction
    
    set_addr_mode(am);      // If no transfer of control takes place, the processor returns to the mode in effect at the time of the fault and resumes normal sequential execution with the instruction following the faulting instruction (C(PPR.IC) + 1).
    
    if (xrv == 0)
        longjmp(jmpMain, JMP_NEXT);     // execute next instruction
    else if (0)                         // TODO: need to put test in to retry instruction (i.e. when executing restartable MW EIS?)
        longjmp(jmpMain, JMP_RETRY);    // retry instruction
#endif

                break;
              }

            case FAULT_EXEC_cycle:
            case FAULT_EXEC2_cycle:
              {
                //     execute instruction in instruction buffer
                //     if (! transfer) set INTERUPT_EXEC2_cycle 

                if (cpu . cycle == FAULT_EXEC_cycle)
                  ci -> IWB = instr_buf [0];
                else
                  ci -> IWB = instr_buf [1];

                decodeInstruction (ci -> IWB, ci);

// The normal start state of the CPU is a trouble fault cascade until the
// iom boot generates in interrupt; therefore, despite the fact that AL39
// doesn't mention it, interrupts must be sampled during the fault cycle.

                if (ci -> i == 0) // Not inhibited
                  {
                    cpu . interrupt_flag = sample_interrupts ();
                    cpu . g7_flag = bG7Pending ();
                  }
                else
                  {
                    cpu . interrupt_flag = false;
                    cpu . g7_flag = false;
                  }

                t_stat ret = executeInstruction (ci);

                if (ret == CONT_TRA)
                  {
                     instr_buf_state = IB_EMPTY;
                     cpu . cycle = FETCH_cycle;
                     break;
                  }
		if (cpu . cycle == FAULT_EXEC_cycle)
                  {
                    cpu . cycle = FAULT_EXEC2_cycle;
                    break;
                  }
                // XXX  if (! transfer) set INTERUPT_EXEC2_cycle and go
                // XXX restore IC from safe store
                // XXX need cu_safe_restore
                cu_safe_restore ();
                cpu . cycle = FETCH_cycle;
                break;
              }

            default:
              {
                sim_printf ("cpu . cycle %d?\n", cpu . cycle);
                return SCPE_UNK;
              }
          }  // switch (cpu . cycle)

      } while (reason == 0);

    sim_printf("\r\ncpuCycles = %lld\n", cpuCycles);
    
    return reason;
  }



//--- // This is part of the simh interface
//--- t_stat sim_instr (void)
//--- {
//---     // Heh. This needs to be static; longjmp resets the value to NULL
//---     static DCDstruct *ci = NULL;
//---     
//---     /* Main instruction fetch/decode loop */
//---     adrTrace  = (cpu_dev.dctrl & DBG_ADDRMOD  ) && sim_deb; // perform address mod tracing
//---     apndTrace = (cpu_dev.dctrl & DBG_APPENDING) && sim_deb; // perform APU tracing
//---     
//---     cpu . interrupt_flag = false;
//--- 
//---     //currentInstruction = &_currentInstruction;
//---     currentInstruction->e = &E;
//---     
//---     int val = setjmp(jmpMain);    // here's our main fault/interrupt return. Back to executing instructions....
//---     switch (val)
//---     {
//---         case 0:
//---             reason = 0;
//---             cpuCycles = 0;
//---             break;
//---         case JMP_NEXT:
//---             goto jmpNext;
//---         case JMP_RETRY:
//---             goto jmpRetry;
//---         case JMP_TRA:
//---             goto jmpTra;
//---         case JMP_STOP:
//---             return stop_reason;
//--- 
//---     }
//--- //    if (val)
//--- //    {
//--- //        // if we're here, we're returning from a fault etc and we want to retry an instruction
//--- //        goto retry;
//--- //    } else {
//--- //        reason = 0;
//--- //        cpuCycles = 0;  // XXX This probably needs to be moved so our fault handler won't reset cpuCycles back to 0
//--- //    }
//---     
//---     do {
//--- jmpRetry:;
//---         /* loop until halted */
//---         if (sim_interval <= 0) {                                /* check clock queue */
//---             if ((reason = sim_process_event ()))
//---                 break;
//---         }
//---         
//---         sim_interval --;
//---         if (sim_brk_summ && sim_brk_test (rIC, SWMASK ('E'))) {    /* breakpoint? */
//---             reason = STOP_BKPT;                        /* stop simulation */
//---             break;
//---         }
//---         
//---         if (cpu . cycle == DIS_cycle)
//---           {
//---             cpu . interrupt_flag = sample_interrupts ();
//---             if (cpu . interrupt_flag)
//---               goto dis_entry;
//---             continue;
//---           }
//--- 
//---         // do group 7 fault processing
//---         if (bG7Pending () && (rIR & 1) == 0)    // only process g7 fauts if available and on even instruction boundary
//---             doG7Faults();
//---             
//---         //
//---         // fetch instruction
//---         processorCycle = SEQUENTIAL_INSTRUCTION_FETCH;
//---         
//--- 
//---         ci = fetchInstruction(rIC, currentInstruction);    // fetch next instruction into current instruction struct
//---         
//--- // XXX The conditions are more rigorous: see AL39, pg 327
//---         if (rIC % 1 == 0 && // Even address
//---             ci -> i == 0) // Not inhibited
//---           cpu . interrupt_flag = sample_interrupts ();
//---         else
//---           cpu . interrupt_flag = false;
//--- 
//---         // XXX: what if sim stops during XEC/XED? if user wants to re-step
//---         // instruc, is this logic OK?
//---         if(XECD == 1) {
//---           ci->IWB = XECD1;
//---         } else if(XECD == 2) {
//---           ci->IWB = XECD2;
//---         }
//---         
//---         t_stat ret = executeInstruction(ci);
//---         
//---         if (! ret)
//---          {
//---            if (cpu . interrupt_flag)
//---               {
//---                 // We should do this later, but doing it now allows us to
//---                 // avoid clean up for no interrupt pending.
//--- 
//---                 uint intr_pair_addr;
//--- dis_entry:
//---                 intr_pair_addr = get_highest_intr ();
//---                 if (intr_pair_addr != 1) // no interrupts 
//---                   {
//--- 
//--- 		    // In the INTERRUPT CYCLE, the processor safe-stores the
//--- 		    // Control Unit Data (see Section 3) into program-invisible
//--- 		    // holding registers in preparation for a Store Control
//--- 		    // Unit (scu) instruction, enters temporary absolute mode,
//--- 		    // and forces the current ring of execution C(PPR.PRR) to
//--- 		    // 0. It then issues an XEC system controller command to
//--- 		    // the system controller on the highest priority port for
//--- 		    // which there is a bit set in the interrupt present
//--- 		    // register.  
//--- 
//---                     // XXX safe store
//---                     cu_safe_store ();
//--- 
//---                     addr_modes_t am = get_addr_mode();  // save address mode
//--- 
//--- 		    // Temporary absolute mode
//--- 		    set_addr_mode (ABSOLUTE_mode);
//--- 
//--- 		    // Set to ring 0
//--- 		    PPR . PRR = 0;
//--- 
//---                     // get intr_pair_addr
//--- 
//---                     // get interrupt pair
//---                     word36 faultPair[2];
//---                     core_read2(intr_pair_addr, faultPair, faultPair+1);
//--- 
//---                     t_stat xrv = doXED(faultPair);
//--- 
//---                     cpu . interrupt_flag = false;
//--- 
//---                     set_addr_mode(am);      // If no transfer of control takes place, the processor returns to the mode in effect at the time of the fault and resumes normal sequential execution with the instruction following the faulting instruction (C(PPR.IC) + 1).
//--- 
//---                     sim_debug (DBG_MSG, & cpu_dev, "leaving DIS_cycle\n");
//---                     sim_printf ("leaving DIS_cycle\n");
//---                     cpu . cycle = FETCH_cycle;
//--- 
//---                     if (0 && xrv)                         // TODO: need to put test in to retry instruction (i.e. when executing restartable MW EIS?)
//---                         longjmp(jmpMain, JMP_RETRY);    // retry instruction
//---                     longjmp(jmpMain, JMP_RETRY);     // execute next instruction
//--- 
//---                     ret = xrv;
//---                 } // int_pair != 1
//---             } // interrupt_flag
//---         } // if (!ret)
//--- 
//---         if (ret)
//---         {
//---             if (ret > 0)
//---             {
//---                 reason = ret;
//---                 break;
//---             } else {
//---                 switch (ret)
//---                 {
//---                     case CONT_TRA:
//--- jmpTra:                 continue;   // don't bump rIC, instruction already did it
//---                     case CONT_FAULT:
//---                     {
//---                         // XXX Instruction faulted.
//---                     }
//---                     break;
//---                 }
//---             }
//---         }
//---         
//--- #if 0
//---         // XXX Remove this when we actually can wait for an interrupt
//---         if (ci->opcode == 0616) { // DIS
//---             reason = STOP_DIS;
//---             break;
//---         }
//--- #endif
//--- 
//--- jmpNext:;
//---         // doesn't seem to work as advertized
//---         if (sim_poll_kbd())
//---             reason = STOP_BKPT;
//---        
//---         // XXX: what if sim stops during XEC/XED? if user wants to re-step
//---         // instruc, is this logic OK?
//---         if(XECD == 1) {
//---           XECD = 2;
//---         } else if(XECD == 2) {
//---           XECD = 0;
//---         } else if (cpu . cycle != DIS_cycle) // XXX maybe cycle == FETCH_cycle
//---           rIC += 1;
//---         
//---         // is this a multiword EIS?
//---         // XXX: no multiword EIS for XEC/XED/fault, right?? -MCW
//---         if (ci->iwb->ndes > 0)
//---           rIC += ci->iwb->ndes;
//---         
//---     } while (reason == 0);
//---     
//---     sim_printf("\r\ncpuCycles = %lld\n", cpuCycles);
//---     
//---     return reason;
//--- }


#if 0
static void setDegenerate()
{
    sim_debug (DBG_DEBUG, & cpu_dev, "setDegenerate\n");
    //TPR.TRR = 0;
    //TPR.TSR = 0;
    //TPR.TBR = 0;
    
    //PPR.PRR = 0;
    //PPR.PSR = 0;
    
    //PPR.P = 1;
}
#endif

static uint32 bkpt_type[4] = { SWMASK ('E') , SWMASK ('N'), SWMASK ('R'), SWMASK ('W') };

#define DOITSITP(indword, Tag) ((_TM(Tag) == TM_IR || _TM(Tag) == TM_RI) && (ISITP(indword) || ISITS(indword)))
//bool DOITSITP(indword, Tag)
//{
//    return ((GET_TM(Tag) == TM_IR || GET_TM(Tag) == TM_RI) && (ISITP(indword) || ISITS(indword)));
//}

PRIVATE
t_stat doAbsoluteRead(DCDstruct *i, word24 addr, word36 *dat, MemoryAccessType accessType, int32 Tag)
{
    sim_debug(DBG_TRACE, &cpu_dev, "doAbsoluteRead(Entry): accessType=%d IWB=%012llo A=%d\n", accessType, i->IWB, GET_A(i->IWB));
    
    rY = addr;
    TPR.CA = addr;  //XXX for APU
    
    switch (accessType)
    {
        case InstructionFetch:
            core_read(addr, dat);
            break;
        default:
            //if (i->a)
                doAppendCycle(i, accessType, Tag, -1, dat);
            //else
            //    core_read(addr, dat);
            break;
    }
    return SCPE_OK;
}


/*!
 * the Read, Write functions access main memory, but optionally calls the appending unit to
 * determine the actual memory address
 */
t_stat Read(DCDstruct *i, word24 addr, word36 *dat, enum eMemoryAccessType acctyp, int32 Tag)
{
#if 0
    if (sim_brk_summ && sim_brk_test (addr, bkpt_type[acctyp]))
        return STOP_BKPT;
    else
#endif
         {
        //*dat = M[addr];
        rY = addr;
        TPR.CA = addr;  //XXX for APU


        //switch (processorAddressingMode)
        switch (get_addr_mode())
        {
            case APPEND_MODE:
APPEND_MODE:;
                doAppendCycle(i, acctyp, Tag, -1, dat);
                break;
            case ABSOLUTE_MODE:
                
#if 0
                if (switches . degenerate_mode)
                  {
                    setDegenerate ();
                    doAppendCycle(i, acctyp, Tag, -1, dat);
                    if (acctyp == IndirectRead && DOITSITP(*dat, Tag))
                        goto APPEND_MODE;
                    break;
                  }
#endif
//#if OLDWAY
                // HWR 17 Dec 13. EXPERIMENTAL. an APU read from ABSOLUTE mode?
                // what about MW EIS that use PR addressing, Hm...? Ok, still needs some work
                
                if (i->a && !(i->iwb->flags & IGN_B29) && i->iwb->ndes == 0)
                    doAppendCycle(i, acctyp, Tag, -1, dat);
                else
                    core_read(addr, dat);
                if (acctyp == IndirectRead && DOITSITP(*dat, Tag))
                    goto APPEND_MODE;
                
//#endif
                //doAppendCycle(i, acctyp, Tag, -1, dat);

                break;
            case BAR_MODE:
                // XXX probably not right.
                rY = getBARaddress(addr);
                finalAddress = rY;
                
                core_read(rY, dat);
                return SCPE_OK;
            default:
                sim_printf("Read(): acctyp\n");
                break;
        }
        
    }
    cpu.read_addr = addr;
    
    return SCPE_OK;
}

PRIVATE
t_stat doAbsoluteWrite(DCDstruct *i, word24 addr, word36 dat, MemoryAccessType accessType, int32 Tag)
{
    sim_debug(DBG_ADDRMOD, &cpu_dev, "doAbsoluteWrite (Entry): accessType=%d IWB=%012llo A=%d\n", accessType, i->IWB, GET_A(i->IWB));
    
    switch (accessType)
    {
        case DataWrite:
        case OperandWrite:
            //if (i->a)
                doAppendCycle(i, accessType, Tag, -1, NULL);
            //else
            //    core_write(addr, dat);
            //break;
            
        case APUDataWrite:      // append operations from absolute mode
        case APUOperandWrite:
            doAppendCycle(i, accessType, Tag, -1, NULL);
            break;
            
        default:
            sim_printf("doAbsoluteWrite(Entry): unsupported accessType=%d\n", accessType);
            break;
    }
    doAppendCycle(i, accessType, Tag, -1, NULL);

    return SCPE_OK;
}


t_stat Write (DCDstruct *i, word24 addr, word36 dat, enum eMemoryAccessType acctyp, int32 Tag)
{
#if 0
    if (sim_brk_summ && sim_brk_test (addr, bkpt_type[acctyp]))
        return STOP_BKPT;
    else
#endif
    {
        rY = addr;
        TPR.CA = addr;  //XXX for APU
        
       
        //switch (processorAddressingMode)
        switch (get_addr_mode())
        {
            case APPEND_MODE:
#ifndef QUIET_UNUSED
APPEND_MODE:;
#endif
                doAppendCycle(i, acctyp, Tag, dat, NULL);    // SXXX should we have a tag value here for RI, IR ITS, ITP, etc or is 0 OK
                break;
            case ABSOLUTE_MODE:

#if 0
                if (switches . degenerate_mode)
                  {
                    setDegenerate ();
                    doAppendCycle(i, acctyp, Tag, dat, NULL);    // SXXX should we have a tag value here for RI, IR ITS, ITP, etc or is 0 OK
                    // XXX what kind of dataop can put a write operation into appending mode?
                    //if (DOITSITP(dat, Tag))
                    //    goto APPEND_MODE;
                    break;
                  }
#endif
//#if OLD_WAY
                // HWR 17 Dec 13. EXPERIMENTAL. an APU write from ABSOLUTE mode?
                if (i->a && !(i->iwb->flags & IGN_B29) && i->iwb->ndes == 0)
                    doAppendCycle(i, acctyp, Tag, dat, NULL);
                else
                    core_write(addr, dat);
                //if (doITSITP(dat, GET_TD(Tag)))
                // XXX what kind of dataop can put a write operation into appending mode?
                //if (DOITSITP(dat, Tag))
                //{
                //   processorAddressingMode = APPEND_MODE;
                //    goto APPEND_MODE;   // ???
                //}
//#endif
                // doAppendCycle(i, acctyp, Tag, dat, NULL);
                break;
            case BAR_MODE:
                // XXX probably not right.
                rY = getBARaddress(addr);
                core_write(rY, dat);
                return SCPE_OK;
            default:
                sim_printf("Write(): acctyp\n");
                break;
        }
        
        
    }
    return SCPE_OK;
}

t_stat Read2 (DCDstruct *i, word24 addr, word36 *datEven, word36 *datOdd, enum eMemoryAccessType acctyp, int32 Tag)
{
#if 0
    if (sim_brk_summ && sim_brk_test (addr, bkpt_type[acctyp]))
        return STOP_BKPT;
    else
#endif
    {        
        // need to check for even/odd?
        if (addr & 1)
        {
            addr &= ~1; /* make it an even address */
            addr &= DMASK;
        }
        Read(i, addr + 0, datEven, acctyp, Tag);
        Read(i, addr + 1, datOdd, acctyp, Tag);
        //printf("read2: addr=%06o\n", addr);
    }
    return SCPE_OK;
}
t_stat Write2 (DCDstruct *i, word24 addr, word36 datEven, word36 datOdd, enum eMemoryAccessType acctyp, int32 Tag)
{
    //return SCPE_OK;
    
#if 0
    if (sim_brk_summ && sim_brk_test (addr, bkpt_type[acctyp]))
        return STOP_BKPT;
    else
#endif
    {
        // need to check for even/odd?
        if (addr & 1)
        {
            addr &= ~1; /* make it an even address */
            addr &= DMASK;
        }
        Write(i, addr + 0, datEven, acctyp, Tag);
        Write(i, addr + 1, datOdd,  acctyp, Tag);
        
        //printf("write2: addr=%06o\n", addr);

    }
    return SCPE_OK;
}

t_stat Read72(DCDstruct *i, word24 addr, word72 *dst, enum eMemoryAccessType acctyp, int32 Tag) // needs testing
{
    word36 even, odd;
    t_stat res = Read2(i, addr, &even, &odd, acctyp, Tag);
    if (res != SCPE_OK)
        return res;
    
    *dst = ((word72)even << 36) | (word72)odd;
    return SCPE_OK;
}
t_stat ReadYPair (DCDstruct *i, word24 addr, word36 *Ypair, enum eMemoryAccessType acctyp, int32 Tag)
{
#if 0
    if (sim_brk_summ && sim_brk_test (addr, bkpt_type[acctyp]))
        return STOP_BKPT;
    else
#endif
    {
        // need to check for even/odd?
        if (addr & 1)
            addr &= ~1; /* make it an even address */
        Read(i, addr + 0, Ypair+0, acctyp, Tag);
        Read(i, addr + 1, Ypair+1, acctyp, Tag);
        
    }
    return SCPE_OK;
}

/*!
 cd@libertyhaven.com - sez ....
 If the instruction addresses a block of four words, the target of the instruction is supposed to be an address that is aligned on a four-word boundary (0 mod 4). If not, the processor will grab the four-word block containing that address that begins on a four-word boundary, even if it has to go back 1 to 3 words. Analogous explanation for 8, 16, and 32 cases.
 
 olin@olinsibert.com - sez ...
 It means that the appropriate low bits of the address are forced to zero. So it's the previous words, not the succeeding words, that are used to satisfy the request.
 
 -- Olin

 */
t_stat ReadN (DCDstruct *i, int n, word24 addr, word36 *Yblock, enum eMemoryAccessType acctyp, int32 Tag)
{
#if 0
    if (sim_brk_summ && sim_brk_test (addr, bkpt_type[acctyp]))
        return STOP_BKPT;
    else
#endif
//    int slop = addr % n;
//    if (slop)
//    {
//        addr -= slop;       // back to last n byte boundary;
//        if ((int)addr < 0)  // XXX this is probably not right, but let's see what happens
//            addr = 0;
//    }
    
    for (int j = 0 ; j < n ; j ++)
        Read(i, addr + j, Yblock + j, acctyp, Tag);
    
    return SCPE_OK;
}

//
// read N words in a non-aligned fashion for EIS
//
// XXX here is where we probably need to to the prepage thang...
t_stat ReadNnoalign (DCDstruct *i, int n, word24 addr, word36 *Yblock, enum eMemoryAccessType acctyp, int32 Tag)
{
#if 0
    if (sim_brk_summ && sim_brk_test (addr, bkpt_type[acctyp]))
        return STOP_BKPT;
    else
#endif
        for (int j = 0 ; j < n ; j ++)
            Read(i, addr + j, Yblock + j, acctyp, Tag);
    
    return SCPE_OK;
}

t_stat WriteN (DCDstruct *i, int n, word24 addr, word36 *Yblock, enum eMemoryAccessType acctyp, int32 Tag)
{
#if 0
    if (sim_brk_summ && sim_brk_test (addr, bkpt_type[acctyp]))
        return STOP_BKPT;
    else
#endif
    
//    int slop = addr % n;  
//    if (slop)
//    {
//        addr -= slop;       // back to last n byte boundary;
//        if ((int)addr < 0)  // XXX this is probably not right, but let's see what happens
//            addr = 0;
//    }
//
    for (int j = 0 ; j < n ; j ++)
        Write(i, addr + j, Yblock[j], acctyp, Tag);
    
    return SCPE_OK;
}

/*!
 * "Raw" core interface ....
 */
int32 core_read(word24 addr, word36 *data)
{
    if(addr >= MEMSIZE) {
        *data = 0;
        return -1;
    } else {
        *data = M[addr] & DMASK;
    }
    return 0;
}

int core_write(word24 addr, word36 data) {
    if(addr >= MEMSIZE) {
        return -1;
    } else {
        M[addr] = data & DMASK;
    }
    return 0;
}

int core_read2(word24 addr, word36 *even, word36 *odd) {
    if(addr >= MEMSIZE) {
        *even = 0;
        *odd = 0;
        return -1;
    } else {
        if(addr & 1) {
            sim_debug(DBG_MSG, &cpu_dev,"warning: subtracting 1 from pair at %o in core_read2\n", addr);
            addr &= ~1; /* make it an even address */
        }
        *even = M[addr++] & DMASK;
        *odd = M[addr] & DMASK;
        return 0;
    }
}

//! for working with CY-pairs
int core_read72(word24 addr, word72 *dst) // needs testing
{
    word36 even, odd;
    if (core_read2(addr, &even, &odd) == -1)
        return -1;
    *dst = ((word72)even << 36) | (word72)odd;
    return 0;
}

int core_write2(word24 addr, word36 even, word36 odd) {
    if(addr >= MEMSIZE) {
        return -1;
    } else {
        if(addr & 1) {
            sim_debug(DBG_MSG, &cpu_dev, "warning: subtracting 1 from pair at %o in core_write2\n", addr);
            addr &= ~1; /* make it even a dress, or iron a skirt ;) */
        }
        M[addr++] = even;
        M[addr] = odd;
    }
    return 0;
}
//! for working with CY-pairs
int core_write72(word24 addr, word72 src) // needs testing
{
    word36 even = (word36)(src >> 36) & DMASK;
    word36 odd = ((word36)src) & DMASK;
    
    return core_write2(addr, even, odd);
}

int core_readN(word24 addr, word36 *data, int n)
{
    addr %= n;  // better be an even power of 2, 4, 8, 16, 32, 64, ....
    for(int i = 0 ; i < n ; i++)
        if(addr >= MEMSIZE) {
            *data = 0;
            return -1;
        } else {
            *data++ = M[addr++];
        }
    return 0;
}

int core_writeN(a8 addr, d8 *data, int n)
{
    addr %= n;  // better be an even power of 2, 4, 8, 16, 32, 64, ....
    for(int i = 0 ; i < n ; i++)
        if(addr >= MEMSIZE) {
            return -1;
        } else {
            M[addr++] = *data++;
        }
    return 0;
}

//#define MM
#if 1   //def MM


//=============================================================================

/*
 * encode_instr()
 *
 * Convert an instr_t struct into a  36-bit word.
 *
 */

void encode_instr(const instr_t *ip, t_uint64 *wordp)
{
    *wordp = setbits36(0, 0, 18, ip->addr);
#if 1
    *wordp = setbits36(*wordp, 18, 10, ip->opcode);
#else
    *wordp = setbits36(*wordp, 18, 9, ip->opcode & 0777);
    *wordp = setbits36(*wordp, 27, 1, ip->opcode >> 9);
#endif
    *wordp = setbits36(*wordp, 28, 1, ip->inhibit);
    if (! is_eis[ip->opcode&MASKBITS(10)]) {
        *wordp = setbits36(*wordp, 29, 1, ip->mods.single.pr_bit);
        *wordp = setbits36(*wordp, 30, 6, ip->mods.single.tag);
    } else {
        *wordp = setbits36(*wordp, 29, 1, ip->mods.mf1.ar);
        *wordp = setbits36(*wordp, 30, 1, ip->mods.mf1.rl);
        *wordp = setbits36(*wordp, 31, 1, ip->mods.mf1.id);
        *wordp = setbits36(*wordp, 32, 4, ip->mods.mf1.reg);
    }
}



#endif // MM
    
static DCDstruct *newDCDstruct(void)
{
    DCDstruct *p = malloc(sizeof(DCDstruct));
    p->e = malloc(sizeof(EISstruct));
    
    return p;
}

void freeDCDstruct(DCDstruct *p)
{
    if (!p)
        return; // Uh-Uh...
    
    if (p->e)
        free(p->e);
    free(p);
}

/*
 * instruction fetcher ...
 * fetch + decode instruction at 18-bit address 'addr'
 */
DCDstruct *fetchInstruction(word18 addr, DCDstruct *i)  // fetch instrcution at address
{
    DCDstruct *p = (i == NULL) ? newDCDstruct() : i;

// XXX experimental code; there may be a better way to do this, especially
// if a pointer to a malloc is getting zapped
// Yep, I was right
// HWR doesn't make sense. DCDstruct * is not really malloc()'d .. it's a global that needs to be cleared before each use. Why does the memset break gcc code?
    
    //memset (p, 0, sizeof (struct DCDstruct));
// Try the obivous ones
    p->opcode  = 0;
    p->opcodeX = 0;
    p->address = 0;
    p->a       = 0;
    p->i       = 0;
    p->tag     = 0;
    
    p->iwb = 0;
    p->IWB = 0;
    
    Read(p, addr, &p->IWB, InstructionFetch, 0);
    
    cpu.read_addr = addr;
    
    return decodeInstruction(p->IWB, p);
}

/*
 * instruction decoder .....
 *
 * if dst is not NULL place results into dst, if dst is NULL plae results into global currentInstruction
 */
DCDstruct *decodeInstruction(word36 inst, DCDstruct *dst)     // decode instruction into structure
{
    DCDstruct *p = dst == NULL ? newDCDstruct() : dst;
    
    p->opcode  = GET_OP(inst);  // get opcode
    p->opcodeX = GET_OPX(inst); // opcode extension
    p->address = GET_ADDR(inst);// address field from instruction
    p->a       = GET_A(inst);   // "A" - the indirect via pointer register flag
    p->i       = GET_I(inst);   // "I" - inhibit interrupt flag
    p->tag     = GET_TAG(inst); // instruction tag
    
    p->iwb = getIWBInfo(p);     // get info for IWB instruction
    
    // HWR 18 June 2013 
    p->iwb->opcode = p->opcode;
    p->IWB = inst;
    
    // HWR 21 Dec 2013
    if (p->iwb->flags & IGN_B29)
        p->a = 0;   // make certain 'a' bit is valid always

    if (p->iwb->ndes > 0)
    {
        p->a = 0;
        p->tag = 0;
        if (p->iwb->ndes > 1)
        {
            memset(p->e, 0, sizeof(EISstruct)); // clear out e
            p->e->op0 = p->IWB;
            // XXX: for XEC/XED/faults, this should trap?? I think -MCW
            for(int n = 0 ; n < p->iwb->ndes; n += 1)
                Read(p, rIC + 1 + n, &p->e->op[n], OperandRead, 0); // I think.
        }
    }
    return p;
}

// MM stuff ...

    /*
     * is_priv_mode()
     *
     * Report whether or or not the CPU is in privileged mode.
     * True if in absolute mode or if priv bit is on in segment TPR.TSR
     * The processor executes instructions in privileged mode when forming addresses in absolute mode
     * or when forming addresses in append mode and the segment descriptor word (SDW) for the segment in execution specifies a privileged procedure
     * and the execution ring is equal to zero.
     *
     */
    
    int is_priv_mode(void)
    {
        // TODO: fix this when time permits
        
        switch (get_addr_mode())
        {
            case ABSOLUTE_mode:
                return 1;
            case APPEND_mode:
                // XXX This is probably too simplistic, but it's a start
                
                if (SDW0.P && PPR.PRR == 0)
                    return 1;
                break;
            default:
                break;
        }
        
        //if (!TSTF(rIR, I_ABS))       //IR.abs_mode)
        //    return 1;
        
//        SDW_t *SDWp = get_sdw();    // Get SDW for segment TPR.TSR
//        if (SDWp == NULL) {
//            if (cpu.apu_state.fhld) {
//                // TODO: Do we need to check cu.word1flags.oosb and other flags to
//                // know what kind of fault to gen?
//                fault_gen(acc_viol_fault);
//                cpu.apu_state.fhld = 0;
//            }
//            sim_debug (DGB_WARN, & cpu_dev, "APU is-priv-mode: Segment does not exist?!?\n");
//            cancel_run(STOP_BUG);
//            return 0;   // arbitrary
//        }
//        if (SDWp->priv)
//            return 1;
//        if(opt_debug>0)
//            sim_debug (DBG_DEBUG, & cpu_dev, "APU: Priv check fails for segment %#o.\n", TPR.TSR);
//        return 0;
        
        return 0;
    }


static addr_modes_t secret_addressing_mode;
/*
 * addr_modes_t get_addr_mode()
 *
 * Report what mode the CPU is in.
 * This is determined by examining a couple of IR flags.
 *
 * TODO: get_addr_mode() probably belongs in the CPU source file.
 *
 */

addr_modes_t get_addr_mode(void)
{
    if (secret_addressing_mode == TEMPORARY_ABSOLUTE_mode)
        return ABSOLUTE_mode; // This is not the mode you are looking for

    //if (IR.abs_mode)
    if (TSTF(rIR, I_ABS))
        return ABSOLUTE_mode;
    
    //if (IR.not_bar_mode == 0)
    if (! TSTF(rIR, I_NBAR))
        return BAR_mode;
    
    return APPEND_mode;
}


/*
 * set_addr_mode()
 *
 * Put the CPU into the specified addressing mode.   This involves
 * setting a couple of IR flags and the PPR priv flag.
 *
 */

void set_addr_mode(addr_modes_t mode)
{
    if (mode == ABSOLUTE_mode) {
        SETF(rIR, I_ABS);
        SETF(rIR, I_NBAR);
        
        PPR.P = 1;
        
#if 0
        if (switches . degenerate_mode)
          setDegenerate();
#endif 
        sim_debug (DBG_DEBUG, & cpu_dev, "APU: Setting absolute mode.\n");
    } else if (mode == APPEND_mode) {
        if (! TSTF (rIR, I_ABS) && TSTF (rIR, I_NBAR))
          sim_debug (DBG_DEBUG, & cpu_dev, "APU: Keeping append mode.\n");
        else
           sim_debug (DBG_DEBUG, & cpu_dev, "APU: Setting append mode.\n");
        CLRF(rIR, I_ABS);
        
        SETF(rIR, I_NBAR);
        
    } else if (mode == BAR_mode) {
        CLRF(rIR, I_ABS);
        CLRF(rIR, I_NBAR);
        
        sim_debug (DBG_WARN, & cpu_dev, "APU: Setting bar mode.\n");
    } else if (mode == TEMPORARY_ABSOLUTE_mode) {
        PPR.P = 1;
        
#if 0
        if (switches . degenerate_mode)
          setDegenerate();
#endif
        
        sim_debug (DBG_DEBUG, & cpu_dev, "APU: Setting temporary absolute mode.\n");

    } else {
        sim_debug (DBG_ERR, & cpu_dev, "APU: Unable to determine address mode.\n");
        cancel_run(STOP_BUG);
    }
    secret_addressing_mode = mode;
    //processorAddressingMode = mode;
}


//=============================================================================

/*
 ic_hist - Circular queue of instruction history
 Used for display via cpu_show_history()
 */

static int ic_hist_max = 0;
static int ic_hist_ptr;
static int ic_hist_wrapped;
enum hist_enum { h_instruction, h_fault, h_intr } htype;
struct ic_hist_t {
    addr_modes_t addr_mode;
    uint seg;
    uint ic;
    //enum hist_enum { instruction, fault, intr } htype;
    enum hist_enum htype;
    union {
        int intr;
        int fault;
        instr_t instr;
    } detail;
};

typedef struct ic_hist_t ic_hist_t;

static ic_hist_t *ic_hist;

void ic_history_init(void)
{
    ic_hist_wrapped = 0;
    ic_hist_ptr = 0;
    if (ic_hist != NULL)
        free(ic_hist);
    if (ic_hist_max < 60)
        ic_hist_max = 60;
    ic_hist = (ic_hist_t*) malloc(sizeof(*ic_hist) * ic_hist_max);
}

// XXX when multiple cpus are supported, make the cpu  data structure
// an array and merge the unit state info into here; coding convention
// is the name should be 'cpu' (as is 'iom' and 'scu'); but that name
// is taken. It should probably be merged into here, and then this
// should then be renamed.

#define N_CPU_UNITS_MAX 1
static struct
  {
    struct
      {
        flag_t inuse;
        int scu_unit_num; // 
        DEVICE * devp;
        UNIT * unitp;
      } ports [N_CPU_PORTS];

  } cpu_array [N_CPU_UNITS_MAX];

// XXX when multiple cpus are supported, merge this into cpu_reset

void cpu_reset_array (void)
  {
    for (int i = 0; i < N_CPU_UNITS_MAX; i ++)
      for (int p = 0; p < N_CPU_UNITS; p ++)
        cpu_array [i] . ports [p] . inuse = false;
  }

// A scu is trying to attach a cable to us
//  to my port cpu_unit_num, cpu_port_num
//  from it's port scu_unit_num, scu_port_num
//

t_stat cable_to_cpu (int cpu_unit_num, int cpu_port_num, int scu_unit_num, int scu_port_num)
  {
    if (cpu_unit_num < 0 || cpu_unit_num >= cpu_dev . numunits)
      {
        //sim_debug (DBG_ERR, & sys_dev, "cable_to_cpu: cpu_unit_num out of range <%d>\n", cpu_unit_num);
        sim_printf ("cable_to_cpu: cpu_unit_num out of range <%d>\n", cpu_unit_num);
        return SCPE_ARG;
      }

    if (cpu_port_num < 0 || cpu_port_num >= N_CPU_PORTS)
      {
        //sim_debug (DBG_ERR, & sys_dev, "cable_to_cpu: cpu_port_num out of range <%d>\n", cpu_port_num);
        sim_printf ("cable_to_cpu: cpu_port_num out of range <%d>\n", cpu_port_num);
        return SCPE_ARG;
      }

    if (cpu_array [cpu_unit_num] . ports [cpu_port_num] . inuse)
      {
        //sim_debug (DBG_ERR, & sys_dev, "cable_to_cpu: socket in use\n");
        sim_printf ("cable_to_cpu: socket in use\n");
        return SCPE_ARG;
      }

    DEVICE * devp = & scu_dev;
    UNIT * unitp = & scu_unit [scu_unit_num];
     
    cpu_array [cpu_unit_num] . ports [cpu_port_num] . inuse = true;
    cpu_array [cpu_unit_num] . ports [cpu_port_num] . scu_unit_num = scu_unit_num;

    cpu_array [cpu_unit_num] . ports [cpu_port_num] . devp = devp;
    cpu_array [cpu_unit_num] . ports [cpu_port_num] . unitp  = unitp;

    unitp -> u3 = cpu_port_num;
    unitp -> u4 = 0;
    unitp -> u5 = cpu_unit_num;

    return SCPE_OK;
  }

static t_stat cpu_show_config(FILE *st, UNIT *uptr, int val, void *desc)
{
    int unit_num = UNIT_NUM (uptr);
    if (unit_num < 0 || unit_num >= cpu_dev . numunits)
      {
        sim_debug (DBG_ERR, & cpu_dev, "cpu_show_config: Invalid unit number %d\n", unit_num);
        sim_printf ("error: invalid unit number %d\n", unit_num);
        return SCPE_ARG;
      }

    sim_printf ("CPU unit number %d\n", unit_num);

    sim_printf("Fault base:               %03o(8)\n", switches . FLT_BASE);
    sim_printf("CPU number:               %01o(8)\n", switches . cpu_num);
    sim_printf("Data switches:            %012llo(8)\n", switches . data_switches);
    sim_printf("Port enable:              %01o(8)\n", switches . port_enable);
    sim_printf("Port configuration:       %012llo(8)\n", switches . port_config);
    sim_printf("Port interlace:           %02o(8)\n", switches . port_interlace);
    sim_printf("Processor mode:           %s [%o]\n", switches . proc_mode ? "Multics" : "GCOS", switches . proc_mode);
    sim_printf("Processor speed:          %02o(8)\n", switches . proc_speed);
    sim_printf("Invert Absolute:          %01o(8)\n", switches . invert_absolute);
    sim_printf("Bit 29 test code:         %01o(8)\n", switches . b29_test);
    sim_printf("DIS enable:               %01o(8)\n", switches . dis_enable);
    sim_printf("AutoAppend disable:       %01o(8)\n", switches . auto_append_disable);
    sim_printf("LPRPn set high bits only: %01o(8)\n", switches . lprp_highonly);
    sim_printf("Steady clock:             %01o(8)\n", switches . steady_clock);
    sim_printf("Degenerate mode:          %01o(8)\n", switches . degenerate_mode);
    sim_printf("Append after:             %01o(8)\n", switches . append_after);

    return SCPE_OK;
}

//
// set cpu0 config=<blah> [;<blah>]
//
//    blah =
//           faultbase = n
//           num = n
//           data = n
//           portenable = n
//           portconfig = n
//           portinterlace = n
//           mode = n
//           speed = n
//    Hacks:
//           invertabsolute = n
//           b29test = n // deprecated
//           dis_enable = n
//           auto_append_disable = n // still need for 20184, not for t4d
//           lprp_highonly = n // deprecated
//           steadyclock = on|off
//           degenerate_mode = n // deprecated
//           append_after = n

static config_value_list_t multics_fault_base [] =
  {
    { "multics", 2 },
    { NULL }
  };

static config_value_list_t cfg_on_off [] =
  {
    { "off", 0 },
    { "on", 1 },
    { "disable", 0 },
    { "enable", 1 },
    { NULL }
  };

static config_value_list_t cpu_mode [] =
  {
    { "gcos", 0 },
    { "multics", 1 },
    { NULL }
  };

static config_list_t cpu_config_list [] =
  {
    /*  0 */ { "faultbase", 0, 0177, multics_fault_base },
    /*  1 */ { "num", 0, 07, NULL },
    /*  2 */ { "data", 0, 0777777777777, NULL },
    /*  3 */ { "portenable", 0, 017, NULL },
    /*  4 */ { "portconfig", 0, 0777777777777, NULL },
    /*  5 */ { "portinterlace", 0, 017, NULL },
    /*  6 */ { "mode", 0, 01, cpu_mode }, 
    /*  7 */ { "speed", 0, 017, NULL }, // XXX use keywords

    // Hacks

    /*  8 */ { "invertabsolute", 0, 1, cfg_on_off }, 
    /*  9 */ { "b29test", 0, 1, cfg_on_off }, 
    /* 10 */ { "dis_enable", 0, 1, cfg_on_off }, 
    /* 11 */ { "auto_append_disable", 0, 1, cfg_on_off }, 
    /* 12 */ { "lprp_highonly", 0, 1, cfg_on_off }, 
    /* 13 */ { "steady_clock", 0, 1, cfg_on_off },
    /* 14 */ { "degenerate_mode", 0, 1, cfg_on_off },
    /* 15 */ { "append_after", 0, 1, cfg_on_off },
    { NULL }
  };

static t_stat cpu_set_config (UNIT * uptr, int32 value, char * cptr, void * desc)
  {
// XXX Minor bug; this code doesn't check for trailing garbage

    int cpu_unit_num = UNIT_NUM (uptr);
    if (cpu_unit_num < 0 || cpu_unit_num >= cpu_dev . numunits)
      {
        sim_debug (DBG_ERR, & cpu_dev, "cpu_set_config: Invalid unit number %d\n", cpu_unit_num);
        sim_printf ("error: cpu_set_config: invalid unit number %d\n", cpu_unit_num);
        return SCPE_ARG;
      }

    config_state_t cfg_state = { NULL };

    for (;;)
      {
        int64_t v;
        int rc = cfgparse ("cpu_set_config", cptr, cpu_config_list, & cfg_state, & v);
        switch (rc)
          {
            case -2: // error
              cfgparse_done (& cfg_state);
              return SCPE_ARG; 

            case -1: // done
              break;

            case 0: // FAULTBASE
              switches . FLT_BASE = v;
              break;

            case 1: // NUM
              switches . cpu_num = v;
              break;

            case 2: // DATA
              switches . data_switches = v;
              break;

            case 3: // PORTENABLE
              switches . port_enable = v;
              break;

            case 4: // PORTCONFIG
              switches . port_config = v;
              break;

            case 5: // PORTINTERLACE
              switches . port_interlace = v;
              break;

            case 6: // MODE
              switches . proc_mode = v;
              break;

            case 7: // SPEED
              switches . proc_speed = v;
              break;

            case 8: // INVERTABSOLUTE
              switches . invert_absolute = v;
              break;

            case 9: // B29TEST
              switches . b29_test = v;
              break;

            case 10: // DIS_ENABLE
              switches . dis_enable = v;
              break;

            case 11: // AUTO_APPEND_DISABLE
              switches . auto_append_disable = v;
              break;

            case 12: // LPRP_HIGHONLY
              switches . lprp_highonly = v;
              break;

            case 13: // STEADY_CLOCK
              switches . steady_clock = v;
              break;

            case 14: // DEGENERATE_MODE
              switches . degenerate_mode = v;
              break;

            case 15: // APPEND_AFTER
              switches . append_after = v;
              break;

            default:
              sim_debug (DBG_ERR, & cpu_dev, "cpu_set_config: Invalid cfgparse rc <%d>\n", rc);
              sim_printf ("error: cpu_set_config: invalid cfgparse rc <%d>\n", rc);
              cfgparse_done (& cfg_state);
              return SCPE_ARG; 
          } // switch
        if (rc < 0)
          break;
      } // process statements
    cfgparse_done (& cfg_state);
    return SCPE_OK;
  }


