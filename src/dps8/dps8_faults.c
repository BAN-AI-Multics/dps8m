/*
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2017 Michal Tomek
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 */

#include <stdio.h>

#include "dps8.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_append.h"
#include "dps8_ins.h"
#include "dps8_utils.h"
#if defined(THREADZ) || defined(LOCKLESS)
# include "threadz.h"
#endif

#define DBG_CTR cpu.cycleCnt

/*
 FAULT RECOGNITION
 For the discussion following, the term "function" is defined as a major processor functional cycle. Examples are: APPEND CYCLE, CA CYCLE, INSTRUCTION FETCH CYCLE, OPERAND STORE CYCLE, DIVIDE EXECUTION CYCLE. Some of these cycles are discussed in various sections of this manual.
 Faults in groups 1 and 2 cause the processor to abort all functions immediately by entering a FAULT CYCLE.
 Faults in group 3 cause the processor to "close out" current functions without taking any irrevocable action (such as setting PTW.U in an APPEND CYCLE or modifying an indirect word in a CA CYCLE), then to discard any pending functions (such as an APPEND CYCLE needed during a CA CYCLE), and to enter a FAULT CYCLE.
 Faults in group 4 cause the processor to suspend overlapped operation, to complete current and pending functions for the current instruction, and then to enter a FAULT CYCLE.
 Faults in groups 5 or 6 are normally detected during virtual address formation and instruction decode. These faults cause the processor to suspend overlapped operation, to complete the current and pending instructions, and to enter a FAULT CYCLE. If a fault in a higher priority group is generated by the execution of the current or pending instructions, that higher priority fault will take precedence and the group 5 or 6 fault will be lost. If a group 5 or 6 fault is detected during execution of the current instruction (e.g., an access violation, out of segment bounds, fault
 ￼￼
 during certain interruptible EIS instructions), the instruction is considered "complete" upon detection of the fault.
 Faults in group 7 are held and processed (with interrupts) at the completion of the current instruction pair. Group 7 faults are inhibitable by setting bit 28 of the instruction word.
 Faults in groups 3 through 6 must wait for the system controller to acknowledge the last access request before entering the FAULT CYCLE.
 */

/*

                                Table 7-1. List of Faults

 Decimal fault     Octal (1)      Fault   Fault name            Priority    Group
     number      fault address   mnemonic
        0      ;         0     ;      sdf  ;   Shutdown             ;   27     ;     7
        1      ;         2     ;      str  ;   Store                ;   10     ;     4
        2      ;         4     ;      mme  ;   Master mode entry 1  ;   11     ;     5
        3      ;         6     ;      f1   ;   Fault tag 1          ;   17     ;     5
        4      ;        10     ;      tro  ;   Timer runout         ;   26     ;     7
        5      ;        12     ;      cmd  ;   Command              ;   9      ;     4
        6      ;        14     ;      drl  ;   Derail               ;   15     ;     5
        7      ;        16     ;      luf  ;   Lockup               ;   5      ;     4
        8      ;        20     ;      con  ;   Connect              ;   25     ;     7
        9      ;        22     ;      par  ;   Parity               ;   8      ;     4
        10     ;        24     ;      ipr  ;   Illegal procedure    ;   16     ;     5
        11     ;        26     ;      onc  ;   Operation not complete ; 4      ;     2
        12     ;        30     ;      suf  ;   Startup              ;   1      ;     1
        13     ;        32     ;      ofl  ;   Overflow             ;   7      ;     3
        14     ;        34     ;      div  ;   Divide check         ;   6      ;     3
        15     ;        36     ;      exf  ;   Execute              ;   2      ;     1
        16     ;        40     ;      df0  ;   Directed fault 0     ;   20     ;     6
        17     ;        42     ;      df1  ;   Directed fault 1     ;   21     ;     6
        18     ;        44     ;      df2  ;   Directed fault 2     ;   22     ;     6
        19     ;        46     ;      df3  ;   Directed fault 3     ;   23     ;     6
        20     ;        50     ;      acv  ;   Access violation     ;   24     ;     6
        21     ;        52     ;      mme2 ;   Master mode entry 2  ;   12     ;     5
        22     ;        54     ;      mme3 ;   Master mode entry 3  ;   13     ;     5
        23     ;        56     ;      mme4 ;   Master mode entry 4  ;   14     ;     5
        24     ;        60     ;      f2   ;   Fault tag 2          ;   18     ;     5
        25     ;        62     ;      f3   ;   Fault tag 3          ;   19     ;     5
        26     ;        64     ;           ;   Unassigned           ;          ;
        27     ;        66     ;           ;   Unassigned           ;          ;

*/

#ifndef QUIET_UNUSED
static dps8faults _faultsP[] = { // sorted by priority
//  number  address  mnemonic   name                 Priority    Group
    {   12,     030,    "suf",  "Startup",                  1,       1,     false },
    {   15,     036,    "exf",  "Execute",                  2,       1,     false },
    {   31,     076,    "trb",  "Trouble",                  3,       2,     false },
    {   11,     026,    "onc",  "Operation not complete",       4,           2,     false },
    {   7,      016,    "luf",  "Lockup",                       5,           4,     false },
    {   14,     034,    "div",  "Divide check",                 6,           3,     false },
    {   13,     032,    "ofl",  "Overflow",                     7,           3,     false },
    {   9,      022,    "par",  "Parity",                       8,           4,     false },
    {   5,      012,    "cmd",  "Command",                      9,           4,     false },
    {   1,       2 ,    "str",  "Store",                        10,          4,     false },
    {   2,       4 ,    "mme",  "Master mode entry 1",          11,          5,     false },
    {   21,     052,    "mme2", "Master mode entry 2",          12,          5,     false },
    {   22,     054,    "mme3", "Master mode entry 3",          13,          5,     false },
    {   23,     056,    "mme4", "Master mode entry 4",          14,          5,     false },
    {   6,      014,    "drl",  "Derail",                       15,          5,     false },
    {   10,     024,    "ipr",  "Illegal procedure",            16,          5,     false },
    {   3,       06,    "f1",   "Fault tag 1",                  17,          5,     false },
    {   24,     060,    "f2",   "Fault tag 2",                  18,          5,     false },
    {   25,     062,    "f3",   "Fault tag 3",                  19,          5,     false },
    {   16,     040,    "df0",  "Directed fault 0",             20,          6,     false },
    {   17,     042,    "df1",  "Directed fault 1",             21,          6,     false },
    {   18,     044,    "df2",  "Directed fault 2",             22,          6,     false },
    {   19,     046,    "df3",  "Directed fault 3",             23,          6,     false },
    {   20,     050,    "acv",  "Access violation",             24,          6,     false },
    {   8,      020,    "con",  "Connect",                      25,          7,     false },
    {   4,      010,    "tro",  "Timer runout",                 26,          7,     false },
    {   0,       0 ,    "sdf",  "Shutdown",                     27,          7,     false },
    {   26,     064,    "???",  "Unassigned",               -1,     -1,     false },
    {   27,     066,    "???",  "Unassigned",               -1,     -1,     false },
    {   -1,     -1,     NULL,   NULL,                       -1,     -1,     false }
};
#endif
#ifndef QUIET_UNUSED
static dps8faults _faults[] = {    // sorted by number
    //  number  address  mnemonic   name                 Priority    Group
    {   0,       0 ,    "sdf",  "Shutdown",                     27,          7,     false },
    {   1,       2 ,    "str",  "Store",                        10,          4,     false },
    {   2,       4 ,    "mme",  "Master mode entry 1",          11,          5,     false },
    {   3,       06,    "f1",   "Fault tag 1",                  17,          5,     false },
    {   4,      010,    "tro",  "Timer runout",                 26,          7,     false },
    {   5,      012,    "cmd",  "Command",                      9,           4,     false },
    {   6,      014,    "drl",  "Derail",                       15,          5,     false },
    {   7,      016,    "luf",  "Lockup",                       5,           4,     false },
    {   8,      020,    "con",  "Connect",                      25,          7,     false },
    {   9,      022,    "par",  "Parity",                       8,           4,     false },
    {   10,     024,    "ipr",  "Illegal procedure",            16,          5,     false },
    {   11,     026,    "onc",  "Operation not complete",       4,           2,     false },
    {   12,     030,    "suf",  "Startup",                  1,       1,     false },
    {   13,     032,    "ofl",  "Overflow",                     7,           3,     false },
    {   14,     034,    "div",  "Divide check",                 6,           3,     false },
    {   15,     036,    "exf",  "Execute",                  2,       1,     false },
    {   16,     040,    "df0",  "Directed fault 0",             20,          6,     false },
    {   17,     042,    "df1",  "Directed fault 1",             21,          6,     false },
    {   18,     044,    "df2",  "Directed fault 2",             22,          6,     false },
    {   19,     046,    "df3",  "Directed fault 3",             23,          6,     false },
    {   20,     050,    "acv",  "Access violation",             24,          6,     false },
    {   21,     052,    "mme2", "Master mode entry 2",          12,          5,     false },
    {   22,     054,    "mme3", "Master mode entry 3",          13,          5,     false },
    {   23,     056,    "mme4", "Master mode entry 4",          14,          5,     false },
    {   24,     060,    "f2",   "Fault tag 2",                  18,          5,     false },
    {   25,     062,    "f3",   "Fault tag 3",                  19,          5,     false },
    {   26,     064,    "???",  "Unassigned",               -1,     -1,     false },
    {   27,     066,    "???",  "Unassigned",               -1,     -1,     false },
    {   28,     070,    "???",  "Unassigned",               -1,     -1,     false },
    {   29,     072,    "???",  "Unassigned",               -1,     -1,     false },
    {   30,     074,    "???",  "Unassigned",               -1,     -1,     false },
    {   31,     076,    "trb",  "Trouble",                  3,       2,     false },

    {   -1,     -1,     NULL,   NULL,                       -1,     -1,     false }
};
#endif

char * faultNames [N_FAULTS] =
  {
    "Shutdown",
    "Store",
    "Master mode entry 1",
    "Fault tag 1",
    "Timer runout",
    "Command",
    "Derail",
    "Lockup",
    "Connect",
    "Parity",
    "Illegal procedure",
    "Operation not complete",
    "Startup",
    "Overflow",
    "Divide check",
    "Execute",
    "Directed fault 0",
    "Directed fault 1",
    "Directed fault 2",
    "Directed fault 3",
    "Access violation",
    "Master mode entry 2",
    "Master mode entry 3",
    "Master mode entry 4",
    "Fault tag 2",
    "Fault tag 3",
    "Unassigned 26",
    "Unassigned 27",
    "Unassigned 28",
    "Unassigned 29",
    "Unassigned 30",
    "Trouble"
  };
//bool pending_fault = false;     // true when a fault has been signalled, but not processed


#ifndef QUIET_UNUSED
static bool port_interrupts[8] = {false, false, false, false, false, false, false, false };
#endif

//-----------------------------------------------------------------------------
// ***  Constants, unchanging lookup tables, etc

#ifndef QUIET_UNUSED
static int fault2group[32] = {
    // from AL39, page 7-3
    7, 4, 5, 5, 7, 4, 5, 4,
    7, 4, 5, 2, 1, 3, 3, 1,
    6, 6, 6, 6, 6, 5, 5, 5,
    5, 5, 0, 0, 0, 0, 0, 2
};

static int fault2prio[32] = {
    // from AL39, page 7-3
    27, 10, 11, 17, 26,  9, 15,  5,
    25,  8, 16,  4,  1,  7,  6,  2,
    20, 21, 22, 23, 24, 12, 13, 14,
    18, 19,  0,  0,  0,  0,  0,  3
};
#endif

/*
 * fault handler(s).
 */

#ifdef TESTING
// We stash a few things for debugging; they are accessed by emCall.
static word18 fault_ic;
static word15 fault_psr;
static char fault_msg [1024];


void emCallReportFault (void)
  {
           sim_printf ("fault report:\n");
           sim_printf ("  fault number %d (%o)\n", cpu . faultNumber, cpu . faultNumber);
           sim_printf ("  subfault number %"PRIu64" (%"PRIo64")\n", cpu.subFault.bits, cpu.subFault.bits);
           sim_printf ("  faulting address %05o:%06o\n", fault_psr, fault_ic);
           sim_printf ("  msg %s\n", fault_msg);
  }
#endif


void clearFaultCycle (void)
  {
    cpu . bTroubleFaultCycle = false;
  }

/*

Faults in groups 1 and 2 cause the processor to abort all functions immediately
by entering a FAULT CYCLE.

Faults in group 3 cause the processor to "close out" current functions without
taking any irrevocable action (such as setting PTW.U in an APPEND CYCLE or
modifying an indirect word in a CA CYCLE), then to discard any pending
functions (such as an APPEND CYCLE needed during a CA CYCLE), and to enter a
FAULT CYCLE.

Faults in group 4 cause the processor to suspend overlapped operation, to
complete current and pending functions for the current instruction, and then to
enter a FAULT CYCLE.

Faults in groups 5 or 6 are normally detected during virtual address formation
and instruction decode. These faults cause the processor to suspend overlapped
operation, to complete the current and pending instructions, and to enter a
FAULT CYCLE. If a fault in a higher priority group is generated by the
execution of the current or pending instructions, that higher priority fault
will take precedence and the group 5 or 6 fault will be lost. If a group 5 or 6
fault is detected during execution of the current instruction (e.g., an access
violation, out of segment bounds, fault during certain interruptible EIS
instructions), the instruction is considered "complete" upon detection of the
fault.

Faults in group 7 are held and processed (with interrupts) at the completion
of the current instruction pair.

Group 7 faults are inhibitable by setting bit 28 of the instruction word.

Faults in groups 3 through 6 must wait for the system controller to acknowledge
the last access request before entering the FAULT CYCLE.

After much rumination here are my thoughts for fault processing .....

For now, at least, we must remember a few things:

1) We only have 1 cpu so we have few & limited async faults - shutdown, TRO,
etc.
2) We have no overlapping instruction execution
3) Becuase of 2) we have no pending instructions
4) We have no system controller to wait for

Group 1 & 2 faults can be processed immediately and then proceed to next
instruction as long as no transfer prevents us from returing from the XED pair.

Group 3 faults will probably also execute immediately since a G3 fault causes
"the processor to "close out" current functions without taking any irrevocable
action (such as setting PTW.U in an APPEND CYCLE or modifying an indirect word
in a CA CYCLE), then to discard any pending functions (such as an APPEND CYCLE
needed during a CA CYCLE), and to enter a FAULT CYCLE."

Group 4 faults will probably also execute immediately since a G4 fault causes
"the processor to suspend overlapped operation, to complete current and pending
functions for the current instruction, and then to enter a FAULT CYCLE."

Group 5 & 6 faults will probably also execute immediately because "if a group 5
or 6 fault is detected during execution of the current instruction (e.g., an
access violation, out of segment bounds, fault during certain interruptible EIS
instructions), the instruction is considered "complete" upon detection of the
fault." However, remember "If a fault in a higher priority group is generated
by the execution of the current or pending instructions, that higher priority
fault will take precedence and the group 5 or 6 fault will be lost. If a group
5 or 6 fault is detected during execution of the current instruction (e.g., an
access violation, out of segment bounds, fault during certain interruptible EIS
instructions), the instruction is considered "complete" upon detection of the
fault."

For furter justification of immediate execution since "Faults in groups 3
through 6 must wait for the system controller to acknowledge the last access
request before entering the FAULT CYCLE."

Group 7 faults will be processed after next even instruction decode instruction
decode, but before instruction execution. In this way we can actually use
bit-28 tp inhibit interrupts

*/

#ifdef LOOPTRC
# include <time.h>
void elapsedtime (void);
#endif

#ifndef NEED_128
const _fault_subtype fst_zero = (_fault_subtype) {.bits=0};
const _fault_subtype fst_acv9 = (_fault_subtype) {.fault_acv_subtype=ACV9};
const _fault_subtype fst_acv15 = (_fault_subtype) {.fault_acv_subtype=ACV15};
const _fault_subtype fst_ill_mod = (_fault_subtype) {.fault_ipr_subtype=FR_ILL_MOD};
const _fault_subtype fst_ill_proc = (_fault_subtype) {.fault_ipr_subtype=FR_ILL_PROC};
const _fault_subtype fst_ill_dig = (_fault_subtype) {.fault_ipr_subtype=FR_ILL_DIG};
const _fault_subtype fst_ill_op = (_fault_subtype) {.fault_ipr_subtype=FR_ILL_OP};
const _fault_subtype fst_str_oob = (_fault_subtype) {.fault_str_subtype=flt_str_oob};
const _fault_subtype fst_str_nea = (_fault_subtype) {.fault_str_subtype=flt_str_nea};
const _fault_subtype fst_str_ptr = (_fault_subtype) {.fault_str_subtype=flt_str_ill_ptr};
const _fault_subtype fst_cmd_lprpn = (_fault_subtype) {.fault_cmd_subtype=flt_cmd_lprpn_bits};
const _fault_subtype fst_cmd_ctl = (_fault_subtype) {.fault_cmd_subtype=flt_cmd_not_control};
const _fault_subtype fst_onc_nem = (_fault_subtype) {.fault_onc_subtype=flt_onc_nem};
#endif
// CANFAULT
void doFault (_fault faultNumber, _fault_subtype subFault,
              const char * faultMsg)
  {
#ifdef LOOPTRC
if (faultNumber == FAULT_TRO)
{
 elapsedtime ();
 sim_printf (" TRO PSR:IC %05o:%06o\r\n", cpu.PPR.PSR, cpu.PPR.IC);
}
else if (faultNumber == FAULT_ACV)
{
 elapsedtime ();
 sim_printf (" ACV %012llo PSR:IC %05o:%06o\r\n", subFault.bits, cpu.PPR.PSR, cpu.PPR.IC);
}
#endif
//if (current_running_cpu_idx)
    //sim_printf ("Fault %d(0%0o), sub %ld(0%lo), dfc %c, '%s'\n",
               //faultNumber, faultNumber, subFault, subFault,
               //cpu . bTroubleFaultCycle ? 'Y' : 'N', faultMsg);
//if (current_running_cpu_idx)
    //sim_printf ("xde %d xdo %d\n", cpu.cu.xde, cpu.cu.xdo);
    sim_debug (DBG_FAULT, & cpu_dev,
               "Fault %d(0%0o), sub %"PRIu64"(0%"PRIo64"), dfc %c, '%s'\n",
               faultNumber, faultNumber, subFault.bits, subFault.bits,
               cpu . bTroubleFaultCycle ? 'Y' : 'N', faultMsg);
#ifdef PROFILER
    __atomic_add_fetch (& cpu.faults[faultNumber], 1u, __ATOMIC_ACQUIRE);
#endif
#ifdef TESTING
    HDBGFault (faultNumber, subFault, faultMsg, "");
#endif
#ifndef SPEED
    if_sim_debug (DBG_FAULT, & cpu_dev)
      traceInstruction (DBG_FAULT);
#endif

    PNL (cpu.DACVpDF = faultNumber >=  FAULT_DF0 && faultNumber <= FAULT_ACV;)

#ifdef TESTING
    // some debugging support stuff
    fault_psr = cpu . PPR.PSR;
    fault_ic = cpu . PPR.IC;
    strcpy (fault_msg, faultMsg);
#endif

    //if (faultNumber < 0 || faultNumber > 31)
    if (faultNumber & ~037U)  // quicker?
    {
        sim_printf ("fault(out-of-range): %d %"PRIo64" '%s'\n",
                    faultNumber, subFault.bits, faultMsg ? faultMsg : "?");
        sim_warn ("fault out-of-range\n");
        faultNumber = FAULT_TRB;
    }

    cpu.faultNumber = faultNumber;
    cpu.subFault = subFault;
    cpu.faultCnt [faultNumber] ++;

    // "The occurrence of a fault or interrupt sets the cache-to-register mode bit to OFF." a:AL39/cmr1
    CPTUR (cptUseCMR);
    cpu.CMR.csh_reg = 0;

    // Increment FCT

    word3 FCT = cpu.cu.APUCycleBits & MASK3;
    FCT = (FCT + 1u) & MASK3;
    cpu.cu.APUCycleBits = (word12) ((cpu.cu.APUCycleBits & 07770) | FCT);

    // Set fault register bits

    CPTUR (cptUseFR);
    if (faultNumber == FAULT_IPR)
      {
#if 0
        if (subFault == flt_ipr_ill_op)
          cpu . faultRegister [0] |= FR_ILL_OP;
        else if (subFault == flt_ipr_ill_mod)
          cpu . faultRegister [0] |= FR_ILL_MOD;
        else if (subFault == flt_ipr_ill_dig)
          cpu . faultRegister [0] |= FR_ILL_DIG;
        else /* if (subFault == flt_ipr_ill_proc) */ // and all others
          cpu . faultRegister [0] |= FR_ILL_PROC;
#else
        cpu . faultRegister [0] |= subFault.bits;
#endif
      }
    else if (faultNumber == FAULT_ONC && subFault.fault_onc_subtype == flt_onc_nem)
      {
        cpu . faultRegister [0] |= FR_NEM;
      }
    else if (faultNumber == FAULT_STR)
      {
        if (subFault.fault_str_subtype == flt_str_oob)
          cpu . faultRegister [0] |= FR_OOB;
        //else if (subFault.fault_str_subtype == flt_str_ill_ptr)
          //cpu . faultRegister [0] |= ?;    // XXX
        //else if (subFault.fault_str_subtype == flt_str_nea)
          //cpu . faultRegister [0] |= ?;    // XXX
      }
    else if (faultNumber == FAULT_CON)
      {
        switch (subFault.fault_con_subtype)
          {
            case con_a:
              cpu . faultRegister [0] |= FR_CON_A;
              break;
            case con_b:
              cpu . faultRegister [0] |= FR_CON_B;
              break;
            case con_c:
              cpu . faultRegister [0] |= FR_CON_C;
              break;
            case con_d:
              cpu . faultRegister [0] |= FR_CON_D;
              break;
            default:
              sim_warn ("FAULT_CON can't map port %lo\n", (long unsigned) subFault.fault_con_subtype);
              break;
          }
      }

    // Set cu word1 fault bits

    cpu . cu . IRO_ISN = 0;
    cpu . cu . OEB_IOC = 0;
    cpu . cu . EOFF_IAIM = 0;
    cpu . cu . ORB_ISP = 0;
    cpu . cu . ROFF_IPR = 0;
    cpu . cu . OWB_NEA = 0;
    cpu . cu . WOFF_OOB = 0;
    cpu . cu . NO_GA = 0;
    cpu . cu . OCB = 0;
    cpu . cu . OCALL = 0;
    cpu . cu . BOC = 0;
#ifdef DPS8M
    cpu . cu . PTWAM_ER = 0;
#endif
    cpu . cu . CRT = 0;
    cpu . cu . RALR = 0;
    cpu . cu . SDWAM_ER = 0;
    cpu . cu . OOSB = 0;
    cpu . cu . PARU = 0;
    cpu . cu . PARL = 0;
    cpu . cu . ONC1 = 0;
    cpu . cu . ONC2 = 0;
    cpu . cu . IA = 0;
    cpu . cu . IACHN = 0;
    cpu . cu . CNCHN = (faultNumber == FAULT_CON) ? subFault.fault_con_subtype & MASK3 : 0;

    // Set control unit 'fault occured during instruction fetch' flag
    cpu . cu . FIF = cpu . cycle == FETCH_cycle ? 1 : 0;
    cpu . cu . FI_ADDR = (word5) faultNumber;

    // XXX Under what conditions should this be set?
    // Assume no
    // Reading Multics source, it seems like Multics is setting this bit; I'm going
    // to assume that the h/w also sets it to 0, and the s/w has to explicitly set it on.
    cpu . cu . rfi = 0;

// Try to decide if this a MIF fault (fault during EIS instruction)
// EIS instructions are not used in fault/interrupt pairs, so the
// only time an EIS instruction could be executing is during EXEC_cycle.
// I am also assuming that only multi-word EIS instructions are of interest.
// Testing faultNumber fixes ISOLTS 890-04a
    // fixes 890-04a and 791 / 792
    SC_I_MIF (cpu.cycle == EXEC_cycle &&
              (cpu.currentInstruction.info->ndes > 0 ||
               (faultNumber == FAULT_IPR && (subFault.fault_ipr_subtype & FR_ILL_OP) &&
                cpu.currentInstruction.opcodeX &&
                (cpu.currentInstruction.opcode & 0410) == 0)));
    sim_debug (DBG_TRACEEXT, & cpu_dev, "MIF %o\n", TST_I_MIF);
#if 0
sim_debug (DBG_FAULT, & cpu_dev, "cycle %u ndes %u fn %u v %u\n", cpu.cycle, cpu.currentInstruction.info->ndes, faultNumber, (cpu . cycle == EXEC_cycle && cpu . currentInstruction . info -> ndes > 0) || faultNumber == FAULT_IPR);
    SC_I_MIF (cpu . cycle == EXEC_cycle &&
        cpu . currentInstruction . info -> ndes > 0);
    //SC_I_MIF ((cpu . cycle == EXEC_cycle &&
        //cpu . currentInstruction . info -> ndes > 0) ||
        //faultNumber == FAULT_IPR);
#endif

    if (faultNumber == FAULT_ACV)
      {
        // This is annoyingly inefficent since the subFault value
        // is bitwise the same as the upper half of CU word1;
        // if the upperhalf were not broken out, then this would be
        // cpu . cu . word1_upper_half = subFault.

        if (subFault.fault_acv_subtype & ACV0)
          cpu . cu . IRO_ISN = 1;
        if (subFault.fault_acv_subtype & ACV1)
          cpu . cu . OEB_IOC = 1;
        if (subFault.fault_acv_subtype & ACV2)
          cpu . cu . EOFF_IAIM = 1;
        if (subFault.fault_acv_subtype & ACV3)
          cpu . cu . ORB_ISP = 1;
        if (subFault.fault_acv_subtype & ACV4)
          cpu . cu . ROFF_IPR = 1;
        if (subFault.fault_acv_subtype & ACV5)
          cpu . cu . OWB_NEA = 1;
        if (subFault.fault_acv_subtype & ACV6)
          cpu . cu . WOFF_OOB = 1;
        if (subFault.fault_acv_subtype & ACV7)
          cpu . cu . NO_GA = 1;
        if (subFault.fault_acv_subtype & ACV8)
          cpu . cu . OCB = 1;
        if (subFault.fault_acv_subtype & ACV9)
          cpu . cu . OCALL = 1;
        if (subFault.fault_acv_subtype & ACV10)
          cpu . cu . BOC = 1;
        if (subFault.fault_acv_subtype & ACV11)
          cpu . cu . PTWAM_ER = 1;
        if (subFault.fault_acv_subtype & ACV12)
          cpu . cu . CRT = 1;
        if (subFault.fault_acv_subtype & ACV13)
          cpu . cu . RALR = 1;
        if (subFault.fault_acv_subtype & ACV14)
          cpu . cu . SDWAM_ER = 1;
        if (subFault.fault_acv_subtype & ACV15)
{
          cpu . cu . OOSB = 1;
//if (cpu.PPR.PSR == 0263) {
//  hdbgPrint ();
//exit (1);
}

}
      }
    else if (faultNumber == FAULT_STR)
      {
        if (subFault.fault_str_subtype == flt_str_oob)
          cpu . cu . WOFF_OOB = 1;
        //else if (subFault.fault_str_subtype == flt_str_ill_ptr)
          //cpu . cu . ??? = 1; // XXX
        else if (subFault.fault_str_subtype == flt_str_nea)
          cpu . cu . OWB_NEA = 1;
      }
    else if (faultNumber == FAULT_IPR)
      {
        if (subFault.fault_ipr_subtype & FR_ILL_OP)
          cpu . cu . OEB_IOC = 1;
        if (subFault.fault_ipr_subtype & FR_ILL_MOD)
          cpu . cu . EOFF_IAIM = 1;
        if (subFault.fault_ipr_subtype & FR_ILL_SLV)
          cpu . cu . ORB_ISP = 1;
        if (subFault.fault_ipr_subtype & FR_ILL_DIG)
          cpu . cu . ROFF_IPR = 1;
      }
    else if (faultNumber == FAULT_CMD)
      {
        if (subFault.fault_cmd_subtype == flt_cmd_lprpn_bits)
          cpu . cu . IA = 0;
        else if (subFault.fault_cmd_subtype == flt_cmd_not_control)
          cpu . cu . IA = 010;
      }

#ifdef L68
    // History registers
    // IHRRS; AL39 pg 47
    // History register lock control. If this bit is set ON, set STROBE ¢
    // (bit 30, key k) OFF, locking the history registers for all faults
    // including the floating faults.
    CPTUR (cptUseMR);
    if (cpu.MR.emr && cpu.MR.ihrrs)
      {
        cpu.MR.ihr = 0;
      }
#endif
#ifdef DPS8M
    // History registers
    // IHRRS; AL39 pg 49
    // Additional resetting of bit 30. If bit 31 = 1, the following faults also
    // reset bit 30:
    //   Lock Up
    //   Parity
    //   Command
    //   Store
    //   Illegal Procedure
    //   Shutdown
    if (cpu.MR.emr && cpu.MR.ihrrs)
      {
        if (faultNumber == FAULT_LUF ||
            faultNumber == FAULT_PAR ||
            faultNumber == FAULT_CMD ||
            faultNumber == FAULT_STR ||
            faultNumber == FAULT_IPR ||
            faultNumber == FAULT_SDF)
          {
            cpu.MR.ihr = 0;
          }
      }
    // Enable History Registers.  This bit will be reset by ... an Op Not
    // Complete fault. It may be reset by other faults (see bit 31).
    if (faultNumber == FAULT_ONC)
      {
        cpu.MR.ihr = 0;
      }
#endif

    // If already in a FAULT CYCLE then signal trouble fault

    if (cpu.cycle == FAULT_EXEC_cycle)
      {
        sim_debug (DBG_CYCLE, & cpu_dev, "Changing fault number to Trouble fault\n");

        cpu.faultNumber = FAULT_TRB;
        cpu.cu.FI_ADDR = FAULT_TRB;
        cpu.subFault.bits = 0; // XXX ???
        // XXX Does the CU or FR need fixing? ticket #36
        if (cpu . bTroubleFaultCycle)
          {
#if !defined(THREADZ) && !defined(LOCKLESS)
# ifndef PANEL
#  ifndef ROUND_ROBIN
            if ((! sample_interrupts ()) &&
                (sim_qcount () == 0))  // XXX If clk_svc is implemented it will
                                     // break this logic
              {
                sim_printf ("Fault cascade @0%06o with no interrupts pending and no events in queue\n", cpu . PPR.IC);
                sim_printf("\nCycles = %"PRId64"\n", cpu.cycleCnt);
                sim_printf("\nInstructions = %"PRId64"\n", cpu.instrCnt);
                //stop_reason = STOP_FLT_CASCADE;
                longjmp (cpu.jmpMain, JMP_STOP);
              }
#  endif
# endif
#endif
          }
        else
          {
//--            f = &_faults[FAULT_TRB];
            cpu . bTroubleFaultCycle = true;
          }
      }
    else
      {
        cpu . bTroubleFaultCycle = false;
      }

    // If doInstruction faults, the instruction cycle counter doesn't get
    // bumped.
    if (cpu . cycle == EXEC_cycle)
      cpu.instrCnt ++;

    cpu . cycle = FAULT_cycle;
    sim_debug (DBG_CYCLE, & cpu_dev, "Setting cycle to FAULT_cycle\n");
    longjmp (cpu.jmpMain, JMP_REENTRY);
}

#ifdef L68
void do_FFV_fault (uint fault_number, const char * fault_msg)
  {
    sim_debug (DBG_FAULT, & cpu_dev,
               "Floating fault %d '%s'\n",
               fault_number, fault_msg);
# ifndef SPEED
    if_sim_debug (DBG_FAULT, & cpu_dev)
      traceInstruction (DBG_FAULT);
# endif

    if (fault_number < 1 || fault_number > 3)
      {
        sim_printf ("floating fault(out-of-range): %d '%s'\n",
                    fault_number, fault_msg ? fault_msg : "?");
        sim_warn ("fault out-of-range\n");
      }

    cpu.FFV_fault_number = fault_number;
    cpu.faultNumber = fault_number;

    // "The occurrence of a fault or interrupt sets the cache-to-register mode bit to OFF." a:AL39/cmr1
    CPTUR (cptUseCMR);
    cpu.CMR.csh_reg = 0;

    // Increment FCT

    word3 FCT = cpu.cu.APUCycleBits & MASK3;
    FCT = (FCT + 1) & MASK3;
    cpu.cu.APUCycleBits = (word12) ((cpu.cu.APUCycleBits & 07770) | FCT);

    // Set fault register bits
    CPTUR (cptUseFR);
    cpu.faultRegister [0] = 0;

    // Set cu word1 fault bits

    cpu.cu.IRO_ISN = 0;
    cpu.cu.OEB_IOC = 0;
    cpu.cu.EOFF_IAIM = 0;
    cpu.cu.ORB_ISP = 0;
    cpu.cu.ROFF_IPR = 0;
    cpu.cu.OWB_NEA = 0;
    cpu.cu.WOFF_OOB = 0;
    cpu.cu.NO_GA = 0;
    cpu.cu.OCB = 0;
    cpu.cu.OCALL = 0;
    cpu.cu.BOC = 0;
# ifdef DPS8M
    cpu.cu.PTWAM_ER = 0;
# endif
    cpu.cu.CRT = 0;
    cpu.cu.RALR = 0;
    cpu.cu.SDWAM_ER = 0;
    cpu.cu.OOSB = 0;
    cpu.cu.PARU = 0;
    cpu.cu.PARL = 0;
    cpu.cu.ONC1 = 0;
    cpu.cu.ONC2 = 0;
    cpu.cu.IA = 0;
    cpu.cu.IACHN = 0;
    cpu.cu.CNCHN = 0;

    // Set control unit 'fault occured during instruction fetch' flag
    cpu.cu.FIF = 0;
    cpu.cu.FI_ADDR = (word5) fault_number & MASK5;

    // XXX Under what conditions should this be set?
    // Assume no
    // Reading Multics source, it seems like Multics is setting this bit; I'm going
    // to assume that the h/w also sets it to 0, and the s/w has to explicitly set it on.
    cpu.cu.rfi = 0;

// Try to decide if this a MIF fault (fault during EIS instruction)
// EIS instructions are not used in fault/interrupt pairs, so the
// only time an EIS instruction could be executing is during EXEC_cycle.
// I am also assuming that only multi-word EIS instructions are of interest.
# if 1
    SC_I_MIF (cpu.cycle == EXEC_cycle &&
        cpu.currentInstruction.info->ndes > 0);
    sim_debug (DBG_TRACEEXT, & cpu_dev, "MIF %o\n", TST_I_MIF);
# endif

    // History registers
    // IHRRS; AL39 pg 47
    // History register lock control. If this bit is set ON, set STROBE ¢
    // (bit 30, key k) OFF, locking the history registers for all faults
    // including the floating faults.
    CPTUR (cptUseMR);
    if (cpu.MR.emr && cpu.MR.ihrrs)
      {
        cpu.MR.ihr = 0;
      }

    if (cpu.cycle == FAULT_EXEC_cycle)
      {
        cpu.faultNumber = FAULT_TRB;
        cpu.cu.FI_ADDR = FAULT_TRB;
        cpu.subFault.bits = 0; // XXX ???
        // XXX Does the CU or FR need fixing? ticket #36
        if (cpu.bTroubleFaultCycle)
          {
# if !defined(THREADZ) && !defined(LOCKLESS)
#  ifndef PANEL
#   ifndef ROUND_ROBIN
            if ((! sample_interrupts ()) &&
                (sim_qcount () == 0))  // XXX If clk_svc is implemented it will
                                     // break this logic
              {
                sim_printf ("Fault cascade @0%06o with no interrupts pending and no events in queue\n", cpu.PPR.IC);
                sim_printf("\nCycles = %"PRId64"\n", cpu.cycleCnt);
                sim_printf("\nInstructions = %"PRId64"\n", cpu.instrCnt);
                //stop_reason = STOP_FLT_CASCADE;
                longjmp (cpu.jmpMain, JMP_STOP);
              }
#   endif
#  endif
# endif
          }
        else
          {
            cpu.bTroubleFaultCycle = true;
          }
        cpu.cycle = FAULT_cycle;
        sim_debug (DBG_CYCLE, & cpu_dev, "Setting cycle to FAULT_cycle\n");
        longjmp (cpu.jmpMain, JMP_REENTRY);
      }
    cpu.bTroubleFaultCycle = false;

    // If doInstruction faults, the instruction cycle counter doesn't get
    // bumped.
    if (cpu . cycle == EXEC_cycle)
      cpu.instrCnt ++;

    cpu.is_FFV = true;
    cpu.cycle = FAULT_cycle;
    longjmp (cpu.jmpMain, JMP_REENTRY);
}
#endif

void dlyDoFault (_fault faultNumber, _fault_subtype subFault,
                const char * faultMsg)
  {
    cpu.dlyFlt = true;
    cpu.dlyFltNum = faultNumber;
    cpu.dlySubFltNum = subFault;
    cpu.dlyCtx = faultMsg;
  }

//
// return true if group 7 faults are pending ...
//

// Note: The DIS code assumes that the only G7 fault is TRO. Adding any
// other G7 faults will potentailly require changing the DIS code.

bool bG7Pending (void)
  {
#ifdef DPS8M
    return cpu.g7Faults != 0;
#endif
#ifdef L68
    return cpu.g7Faults != 0 || cpu.FFV_faults != 0;
#endif
  }

bool bG7PendingNoTRO (void)
  {
#ifdef DPS8M
    return (cpu.g7Faults & (~ (1u << FAULT_TRO))) != 0;
#endif
#ifdef L68
    return (cpu.g7Faults & (~ (1u << FAULT_TRO))) != 0 || cpu.FFV_faults != 0;
#endif
  }

void setG7fault (uint cpuNo, _fault faultNo, _fault_subtype subFault)
  {
    sim_debug (DBG_FAULT, & cpu_dev, "setG7fault CPU %d fault %d (%o) sub %"PRId64" %"PRIo64"\n",
               cpuNo, faultNo, faultNo, subFault.bits, subFault.bits);
    cpus[cpuNo].g7FaultsPreset |= (1u << faultNo);
    //cpu.g7SubFaultsPreset [faultNo] = subFault;
    cpus[cpuNo].g7SubFaults [faultNo] = subFault;
#if defined(THREADZ) || defined(LOCKLESS)
    wakeCPU(cpuNo);
#endif
  }

#ifdef L68
void set_FFV_fault (uint f_fault_no)
  {
    sim_debug (DBG_FAULT, & cpu_dev, "set_FFV_fault CPU f_fault_no %u\n",
               f_fault_no);
    // Map fault number (2/4/6) to bit mask  01/02/04
    cpu.FFV_faults_preset |= 1u << ((f_fault_no / 2) - 1);
  }
#endif

void clearTROFault (void)
  {
    cpu . g7Faults &= ~(1u << FAULT_TRO);
  }

void doG7Fault (bool allowTR)
  {
    // sim_printf ("doG7fault %08o [%"PRId64"]\n", cpu . g7Faults, cpu.cycleCnt);
    // if (cpu . g7Faults)
      // {
        // sim_debug (DBG_FAULT, & cpu_dev, "doG7Fault %08o\n", cpu . g7Faults);
      // }
    // According AL39,  Table 7-1. List of Faults, priority of connect is 25
    // and priority of Timer runout is 26, lower number means higher priority
#if defined(THREADZ) || defined(LOCKLESS)
    lock_scu ();
#endif
     if (cpu.g7Faults & (1u << FAULT_CON))
       {
         cpu.g7Faults &= ~(1u << FAULT_CON);

#if defined(THREADZ) || defined(LOCKLESS)
         unlock_scu ();
#endif
         doFault (FAULT_CON, cpu.g7SubFaults [FAULT_CON], "Connect");
       }

     if (allowTR && (cpu.g7Faults & (1u << FAULT_TRO)))
       {
         cpu . g7Faults &= ~(1u << FAULT_TRO);

         //sim_printf("timer runout %12o\n",cpu.PPR.IC);
#if defined(THREADZ) || defined(LOCKLESS)
         unlock_scu ();
#endif
         doFault (FAULT_TRO, fst_zero, "Timer runout");
       }

     // Strictly speaking EXF isn't a G7 fault, put if we treat is as one,
     // we are allowing the current instruction to complete, simplifying
     // implementation
     if (cpu . g7Faults & (1u << FAULT_EXF))
       {
         cpu . g7Faults &= ~(1u << FAULT_EXF);

#if defined(THREADZ) || defined(LOCKLESS)
         unlock_scu ();
#endif
         doFault (FAULT_EXF, fst_zero, "Execute fault");
       }

#ifdef L68
     if (cpu.FFV_faults & 1u)  // FFV + 2 OC TRAP
       {
         cpu.FFV_faults &= ~1u;
# if defined(THREADZ) || defined(LOCKLESS)
         unlock_scu ();
# endif
         do_FFV_fault (1, "OC TRAP");
       }
     if (cpu.FFV_faults & 2u)  // FFV + 4 CU HISTORY OVERFLOW TRAP
       {
         cpu.FFV_faults &= ~2u;
# if defined(THREADZ) || defined(LOCKLESS)
         unlock_scu ();
# endif
         do_FFV_fault (2, "CU HIST OVF TRAP");
       }
     if (cpu.FFV_faults & 4u)  // FFV + 6 ADR TRAP
       {
         cpu.FFV_faults &= ~4u;
# if defined(THREADZ) || defined(LOCKLESS)
         unlock_scu ();
# endif
         do_FFV_fault (3, "ADR TRAP");
       }
#endif
#if defined(THREADZ) || defined(LOCKLESS)
     unlock_scu ();
#endif
     doFault (FAULT_TRB, (_fault_subtype) {.bits=cpu.g7Faults}, "Dazed and confused in doG7Fault");
  }

void advanceG7Faults (void)
  {
#if defined(THREADZ) || defined(LOCKLESS)
    lock_scu ();
#endif
    cpu.g7Faults |= cpu.g7FaultsPreset;
    cpu.g7FaultsPreset = 0;
    //memcpy (cpu.g7SubFaults, cpu.g7SubFaultsPreset, sizeof (cpu.g7SubFaults));
#ifdef L68
    cpu.FFV_faults |= cpu.FFV_faults_preset;
    cpu.FFV_faults_preset = 0;
#endif
#if defined(THREADZ) || defined(LOCKLESS)
    unlock_scu ();
#endif
  }

