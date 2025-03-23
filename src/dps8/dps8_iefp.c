/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * SPDX-License-Identifier: Multics
 * scspell-id: 755f1556-f62e-11ec-8184-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2015 Eric Swenson
 * Copyright (c) 2017 Michal Tomek
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 *
 * This source file may contain code comments that adapt, include, and/or
 * incorporate Multics program code and/or documentation distributed under
 * the Multics License.  In the event of any discrepancy between code
 * comments herein and the original Multics materials, the original Multics
 * materials should be considered authoritative unless otherwise noted.
 * For more details and historical background, see the LICENSE.md file at
 * the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

//-V::536

#include "dps8.h"
#include "dps8_sys.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_append.h"
#include "dps8_iefp.h"
#include "dps8_addrmods.h"
#include "dps8_utils.h"

#define DBG_CTR cpu.cycleCnt

// new Read/Write stuff ...

#if defined(OLDAPP)
void Read (cpu_state_t * cpup, word18 address, word36 * result, processor_cycle_type cyctyp)
  {
    cpu.TPR.CA = cpu.iefpFinalAddress = address;
    bool isBAR = get_bar_mode (cpup);

    //if (get_went_appending () ||
    if (cpu.cu.XSF || (cyctyp != INSTRUCTION_FETCH && cpu.currentInstruction.b29))
      {
        goto B29;
      }

    switch (get_addr_mode (cpup))
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
                cpu.iefpFinalAddress = get_BAR_address (cpup, address);
                fauxDoAppendCycle (cpup, cyctyp);
# if defined(LOCKLESS)
                if (cyctyp == OPERAND_RMW || cyctyp == APU_DATA_RMW)
                  core_read_lock (cpup, cpu.iefpFinalAddress, result, __func__);
                else
                  core_read (cpup, cpu.iefpFinalAddress, result, __func__);
# else
                core_read (cpup, cpu.iefpFinalAddress, result, __func__);
# endif
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Read (Actual) Read:       bar address=%08o  "
                           "readData=%012"PRIo64"\r\n", address, *result);
# if defined(TESTING)
                HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "Read ABS BAR");
                HDBGMRead (cpu.iefpFinalAddress, * result, "Read ABS BAR");
# endif
                return;
              }
            else
              {
                set_apu_status (cpup, apuStatus_FABS);
                fauxDoAppendCycle (cpup, cyctyp);
# if defined(LOCKLESS)
                if (cyctyp == OPERAND_RMW || cyctyp == APU_DATA_RMW)
                  core_read_lock (cpup, address, result, __func__);
                else
                  core_read (cpup, address, result, __func__);
# else
                core_read (cpup, address, result, __func__);
# endif
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Read (Actual) Read:       abs address=%08o  "
                           "readData=%012"PRIo64"\r\n", address, *result);
# if defined(TESTING)
                HDBGIEFP (hdbgIEFP_abs_read, 0, address, "Read ABS");
                HDBGMRead (address, * result, "Read ABS");
# endif
                return;
              }
          }

        case APPEND_mode:
          {
B29:;
            if (isBAR)
              {
                cpu.TPR.CA = get_BAR_address (address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                cpu.iefpFinalAddress = do_append_cycle (cpup, cyctyp, result, 1);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Read (Actual) Read:  bar iefpFinalAddress=%08o  "
                           "readData=%012"PRIo64"\r\n",
                           cpu.iefpFinalAddress, * result);
# if defined(TESTING)
                HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "Read BAR");
                HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read BAR");
# endif

                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (cpup, cyctyp, result, 1);
                // XXX Don't trace Multics idle loop
                if (cpu.PPR.PSR != 061 && cpu.PPR.IC != 0307)
                  {
                    sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                               "Read (Actual) Read:  iefpFinalAddress=%08o  "
                               "readData=%012"PRIo64"\r\n",
                               cpu.iefpFinalAddress, * result);
# if defined(TESTING)
                    HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "Read");
                    HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read");
# endif
                  }
              }
            return;
          }
      }
    return ;//SCPE_UNK;
  }
#endif // OLDAPP

#if !defined(OLDAPP)
void ReadAPUDataRead (cpu_state_t * cpup, word18 address, word36 * result) {
  cpu.TPR.CA = cpu.iefpFinalAddress = address;
  bool isBAR = get_bar_mode (cpup);

  if (cpu.cu.XSF || cpu.currentInstruction.b29)
    goto B29;

  switch (get_addr_mode (cpup)) {
    case ABSOLUTE_mode: {
      if (isBAR) {
        set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
        cpu.iefpFinalAddress = get_BAR_address (cpup, address);
        fauxDoAppendCycle (cpup, APU_DATA_READ);
        core_read (cpup, cpu.iefpFinalAddress, result, __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "ReadAPUDataRead (Actual) Read:       bar address=%08o  readData=%012"PRIo64"\r\n",
                   address, *result);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "ReadAPUDataRead ABS BAR");
        HDBGMRead (cpu.iefpFinalAddress, * result, "ReadAPUDataRead ABS BAR");
# endif
        return;
      } else {
        set_apu_status (cpup, apuStatus_FABS);
        fauxDoAppendCycle (cpup, APU_DATA_READ);
        core_read (cpup, address, result, __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "ReadAPUDataRead (Actual) Read:       abs address=%08o  readData=%012"PRIo64"\r\n",
                   address, *result);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_read, 0, address, "ReadAPUDataRead ABS");
        HDBGMRead (address, * result, "ReadAPUDataRead ABS");
# endif
        return;
      }
    }

    case APPEND_mode: {
B29:;
      if (isBAR) {
        cpu.TPR.CA = get_BAR_address (cpup, address);
        cpu.TPR.TSR = cpu.PPR.PSR;
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.iefpFinalAddress = doAppendCycleAPUDataRead (cpup, result, 1);
        sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                   "ReadAPUDataRead (Actual) Read:  bar iefpFinalAddress=%08o  readData=%012"PRIo64"\r\n",
                   cpu.iefpFinalAddress, * result);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "ReadAPUDataRead BAR");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "ReadAPUDataRead BAR");
# endif
        return;
      } else {
        cpu.iefpFinalAddress = doAppendCycleAPUDataRead (cpup, result, 1);
        // XXX Don't trace Multics idle loop
        if (cpu.PPR.PSR != 061 && cpu.PPR.IC != 0307) {
          sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                     "ReadAPUDataRead (Actual) Read:  iefpFinalAddress=%08o  readData=%012"PRIo64"\r\n",
                     cpu.iefpFinalAddress, * result);
# if defined(TESTING)
          HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "ReadAPUDataRead");
          HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "ReadAPUDataRead");
# endif
        }
        return;
      }
    }
  }
}

void ReadOperandRead (cpu_state_t * cpup, word18 address, word36 * result) {
  cpu.TPR.CA = cpu.iefpFinalAddress = address;
  bool isBAR = get_bar_mode (cpup);

  if (cpu.cu.XSF || cpu.currentInstruction.b29)
    goto B29;

  switch (get_addr_mode (cpup)) {
    case ABSOLUTE_mode: {
      if (isBAR) {
        set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
        cpu.iefpFinalAddress = get_BAR_address (cpup, address);
        fauxDoAppendCycle (cpup, OPERAND_READ);
        core_read (cpup, cpu.iefpFinalAddress, result, __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "readOperandRead (Actual) Read:       bar address=%08o  readData=%012"PRIo64"\r\n",
                   address, *result);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "readOperandRead ABS BAR");
        HDBGMRead (cpu.iefpFinalAddress, * result, "readOperandRead ABS BAR");
# endif
        return;
      } else {
        set_apu_status (cpup, apuStatus_FABS);
        fauxDoAppendCycle (cpup, OPERAND_READ);
        core_read (cpup, address, result, __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "readOperandRead (Actual) Read:       abs address=%08o  readData=%012"PRIo64"\r\n",
                   address, *result);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_read, 0, address, "readOperandRead ABS");
        HDBGMRead (address, * result, "readOperandRead ABS");
# endif
        return;
      }
    }

    case APPEND_mode: {
B29:;
      if (isBAR) {
        cpu.TPR.CA = get_BAR_address (cpup, address);
        cpu.TPR.TSR = cpu.PPR.PSR;
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.iefpFinalAddress = doAppendCycleOperandRead (cpup, result, 1);
        sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                   "readOperandRead (Actual) Read:  bar iefpFinalAddress=%08o  readData=%012"PRIo64"\r\n",
                   cpu.iefpFinalAddress, * result);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "readOperandRead BAR");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "readOperandRead BAR");
# endif

        return;
      } else {
        cpu.iefpFinalAddress = doAppendCycleOperandRead (cpup, result, 1);
        // XXX Don't trace Multics idle loop
        if (cpu.PPR.PSR != 061 && cpu.PPR.IC != 0307) {
          sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                     "readOperandRead (Actual) Read:  iefpFinalAddress=%08o  readData=%012"PRIo64"\r\n",
                     cpu.iefpFinalAddress, * result);
# if defined(TESTING)
          HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "readOperandRead");
          HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "readOperandRead");
# endif
        }
        return;
      }
    }
  }
}

# if defined(LOCKLESS)
void ReadOperandRMW (cpu_state_t * cpup, word18 address, word36 * result) {
  cpu.TPR.CA = cpu.iefpFinalAddress = address;
  bool isBAR = get_bar_mode (cpup);

  if (cpu.cu.XSF || cpu.currentInstruction.b29)
    goto B29;

  switch (get_addr_mode (cpup)) {
    case ABSOLUTE_mode: {
      if (isBAR) {
        set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
        cpu.iefpFinalAddress = get_BAR_address (cpup, address);
        fauxDoAppendCycle (cpup, OPERAND_RMW);
        core_read_lock (cpup, cpu.iefpFinalAddress, result, __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "ReadOperandRMW (Actual) Read:       bar address=%08o  readData=%012"PRIo64"\r\n",
                   address, *result);
#  if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "ReadOperandRMW ABS BAR");
        HDBGMRead (cpu.iefpFinalAddress, * result, "ReadOperandRMW ABS BAR");
#  endif
        return;
      } else {
        set_apu_status (cpup, apuStatus_FABS);
        fauxDoAppendCycle (cpup, OPERAND_RMW);
        core_read_lock (cpup, address, result, __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "ReadOperandRMW (Actual) Read:       abs address=%08o  readData=%012"PRIo64"\r\n",
                   address, *result);
#  if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_read, 0, address, "ReadOperandRMW ABS");
        HDBGMRead (address, * result, "ReadOperandRMW ABS");
#  endif
        return;
      }
    }

    case APPEND_mode: {
B29:;
      if (isBAR) {
        cpu.TPR.CA = get_BAR_address (cpup, address);
        cpu.TPR.TSR = cpu.PPR.PSR;
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.iefpFinalAddress = doAppendCycleOperandRMW (cpup, result, 1);
        sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                   "ReadOperandRMW (Actual) Read:  bar iefpFinalAddress=%08o  readData=%012"PRIo64"\r\n",
                   cpu.iefpFinalAddress, * result);
#  if defined(TESTING)
        HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "ReadOperandRMW BAR");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "ReadOperandRMW BAR");
#  endif
        return;
      } else {
        cpu.iefpFinalAddress = doAppendCycleOperandRMW (cpup, result, 1);
        // XXX Don't trace Multics idle loop
        if (cpu.PPR.PSR != 061 && cpu.PPR.IC != 0307) {
          sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                     "ReadOperandRMW (Actual) Read:  iefpFinalAddress=%08o  readData=%012"PRIo64"\r\n",
                     cpu.iefpFinalAddress, * result);
#  if defined(TESTING)
          HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "ReadOperandRMW");
          HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "ReadOperandRMW");
#  endif
        }
        return;
      }
    }
  }
}

void ReadAPUDataRMW (cpu_state_t * cpup, word18 address, word36 * result) {
  cpu.TPR.CA = cpu.iefpFinalAddress = address;
  bool isBAR = get_bar_mode (cpup);

  if (cpu.cu.XSF || cpu.currentInstruction.b29)
    goto B29;

  switch (get_addr_mode (cpup)) {
    case ABSOLUTE_mode: {
      if (isBAR) {
        set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
        cpu.iefpFinalAddress = get_BAR_address (cpup, address);
        fauxDoAppendCycle (cpup, APU_DATA_RMW);
        core_read_lock (cpup, cpu.iefpFinalAddress, result, __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "ReadAPUDataRMW (Actual) Read:       bar address=%08o  readData=%012"PRIo64"\r\n",
                   address, *result);
#  if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "ReadAPUDataRMW ABS BAR");
        HDBGMRead (cpu.iefpFinalAddress, * result, "ReadAPUDataRMW ABS BAR");
#  endif
        return;
      } else {
        set_apu_status (cpup, apuStatus_FABS);
        fauxDoAppendCycle (cpup, APU_DATA_RMW);
        core_read_lock (cpup, address, result, __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "ReadAPUDataRMW (Actual) Read:       abs address=%08o  readData=%012"PRIo64"\r\n",
                   address, *result);
#  if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_read, 0, address, "ReadAPUDataRMW ABS");
        HDBGMRead (address, * result, "ReadAPUDataRMW ABS");
#  endif
        return;
      }
    }

    case APPEND_mode: {
B29:;
      if (isBAR) {
        cpu.TPR.CA = get_BAR_address (cpup, address);
        cpu.TPR.TSR = cpu.PPR.PSR;
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.iefpFinalAddress = doAppendCycleAPUDataRMW (cpup, result, 1);
        sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                   "ReadAPUDataRMW (Actual) Read:  bar iefpFinalAddress=%08o  readData=%012"PRIo64"\r\n",
                   cpu.iefpFinalAddress, * result);
#  if defined(TESTING)
        HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "ReadAPUDataRMW BAR");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "ReadAPUDataRMW BAR");
#  endif
        return;
      } else {
        cpu.iefpFinalAddress = doAppendCycleAPUDataRMW (cpup, result, 1);
        // XXX Don't trace Multics idle loop
        if (cpu.PPR.PSR != 061 && cpu.PPR.IC != 0307) {
          sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                     "ReadAPUDataRMW (Actual) Read:  iefpFinalAddress=%08o  readData=%012"PRIo64"\r\n",
                     cpu.iefpFinalAddress, * result);
#  if defined(TESTING)
          HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "ReadAPUDataRMW");
          HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "ReadAPUDataRMW");
#  endif
        }
        return;
      }
    }
  }
  return ;//SCPE_UNK;
}
# endif

void ReadInstructionFetch (cpu_state_t * cpup, word18 address, word36 * result) {
  cpu.TPR.CA = cpu.iefpFinalAddress = address;
  bool isBAR = get_bar_mode (cpup);

  if (cpu.cu.XSF)
    goto B29;

  switch (get_addr_mode (cpup)) {
    case ABSOLUTE_mode: {
      if (isBAR) {
        set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
        cpu.iefpFinalAddress = get_BAR_address (cpup, address);
        fauxDoAppendCycle (cpup, INSTRUCTION_FETCH);
        core_read (cpup, cpu.iefpFinalAddress, result, __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "ReadInstructionFetch (Actual) Read:       bar address=%08o  readData=%012"PRIo64"\r\n",
                   address, *result);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "ReadInstructionFetch ABS BAR");
        HDBGMRead (cpu.iefpFinalAddress, * result, "ReadInstructionFetch ABS BAR");
# endif
        return;
      } else {
        set_apu_status (cpup, apuStatus_FABS);
        fauxDoAppendCycle (cpup, INSTRUCTION_FETCH);
        core_read (cpup, address, result, __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "ReadInstructionFetch (Actual) Read:       abs address=%08o  readData=%012"PRIo64"\r\n",
                   address, *result);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_read, 0, address, "ReadInstructionFetch ABS");
        HDBGMRead (address, * result, "ReadInstructionFetch ABS");
# endif
        return;
      }
    }

    case APPEND_mode: {
B29:;
      if (isBAR) {
        cpu.TPR.CA = get_BAR_address (cpup, address);
        cpu.TPR.TSR = cpu.PPR.PSR;
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.iefpFinalAddress = doAppendCycleInstructionFetch (cpup, result, 1);
        sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                   "ReadInstructionFetch (Actual) Read:  bar iefpFinalAddress=%08o  readData=%012"PRIo64"\r\n",
                   cpu.iefpFinalAddress, * result);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "ReadInstructionFetch BAR");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "ReadInstructionFetch BAR");
# endif
        return;
      } else {
        cpu.iefpFinalAddress = doAppendCycleInstructionFetch (cpup, result, 1);
        // XXX Don't trace Multics idle loop
        if (cpu.PPR.PSR != 061 && cpu.PPR.IC != 0307) {
          sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                     "ReadInstructionFetch (Actual) Read:  iefpFinalAddress=%08o  readData=%012"PRIo64"\r\n",
                     cpu.iefpFinalAddress, * result);
# if defined(TESTING)
          HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "ReadInstructionFetch");
          HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "ReadInstructionFetch");
# endif
        }
        return;
      }
    }
  }
}

void ReadIndirectWordFetch (cpu_state_t * cpup, word18 address, word36 * result) {
  cpu.TPR.CA = cpu.iefpFinalAddress = address;
  bool isBAR = get_bar_mode (cpup);

  if (cpu.cu.XSF || cpu.currentInstruction.b29)
    goto B29;

  switch (get_addr_mode (cpup)) {
    case ABSOLUTE_mode: {
      if (isBAR) {
        set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
        cpu.iefpFinalAddress = get_BAR_address (cpup, address);
        fauxDoAppendCycle (cpup, INDIRECT_WORD_FETCH);
        core_read (cpup, cpu.iefpFinalAddress, result, __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "ReadIndirectWordFetch (Actual) Read:       bar address=%08o  readData=%012"PRIo64"\r\n",
                   address, *result);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "ReadIndirectWordFetch ABS BAR");
        HDBGMRead (cpu.iefpFinalAddress, * result, "ReadIndirectWordFetch ABS BAR");
# endif
        return;
      } else {
        set_apu_status (cpup, apuStatus_FABS);
        fauxDoAppendCycle (cpup, INDIRECT_WORD_FETCH);
        core_read (cpup, address, result, __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "ReadIndirectWordFetch (Actual) Read:       abs address=%08o  readData=%012"PRIo64"\r\n",
                   address, *result);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_read, 0, address, "ReadIndirectWordFetch ABS");
        HDBGMRead (address, * result, "ReadIndirectWordFetch ABS");
# endif
        return;
      }
  }

  case APPEND_mode: {
B29:;
      if (isBAR) {
        cpu.TPR.CA = get_BAR_address (cpup, address);
        cpu.TPR.TSR = cpu.PPR.PSR;
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.iefpFinalAddress = doAppendCycleIndirectWordFetch (cpup, result, 1);
        sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                   "ReadIndirectWordFetch (Actual) Read:  bar iefpFinalAddress=%08o  readData=%012"PRIo64"\r\n",
                   cpu.iefpFinalAddress, * result);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "ReadIndirectWordFetch BAR");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "ReadIndirectWordFetch BAR");
# endif
        return;
      } else {
        cpu.iefpFinalAddress = doAppendCycleIndirectWordFetch (cpup, result, 1);
        // XXX Don't trace Multics idle loop
        if (cpu.PPR.PSR != 061 && cpu.PPR.IC != 0307) {
          sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                     "ReadIndirectWordFetch (Actual) Read:  iefpFinalAddress=%08o  readData=%012"PRIo64"\r\n",
                     cpu.iefpFinalAddress, * result);
# if defined(TESTING)
            HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "ReadIndirectWordFetch");
            HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "ReadIndirectWordFetch");
# endif
          }
          return;
        }
    }
  }
}
#endif // !OLDAPP

#if defined(OLDAPP)
void Read2 (cpu_state_t * cpup, word18 address, word36 * result, processor_cycle_type cyctyp) {
  cpu.TPR.CA = cpu.iefpFinalAddress = address;

  bool isBAR = get_bar_mode (cpup);

  if (cpu.cu.XSF || (cyctyp != INSTRUCTION_FETCH && cpu.currentInstruction.b29) || cyctyp == RTCD_OPERAND_FETCH) // ISOLTS-886
         // Another option would be to set_went_appending in ReadRTCDOp
    goto B29;

  switch (get_addr_mode (cpup)) {
    case ABSOLUTE_mode: {
      if (isBAR) {
        set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
        cpu.iefpFinalAddress = get_BAR_address (cpup, address);

        fauxDoAppendCycle (cpup, cyctyp);
        core_read2 (cpup, cpu.iefpFinalAddress, result + 0, result + 1, __func__);
        if_sim_debug (DBG_FINAL, & cpu_dev) {
            for (uint i = 0; i < 2; i ++)
              sim_debug (DBG_FINAL, & cpu_dev,
                         "Read2 (Actual) Read:       bar address=%08o" "  readData=%012"PRIo64"\r\n",
                         address + i, result [i]);
        }
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "Read2 ABBR");
        HDBGMRead (cpu.iefpFinalAddress, * result, "Read2 ABBR evn");
        HDBGMRead (cpu.iefpFinalAddress+1, * (result+1), "Read2 ABBR odd");
# endif
        return;
      } else {
        set_apu_status (cpup, apuStatus_FABS);
        fauxDoAppendCycle (cpup, cyctyp);
        core_read2 (cpup, address, result + 0, result + 1, __func__);
        if_sim_debug (DBG_FINAL, & cpu_dev) {
          for (uint i = 0; i < 2; i ++)
            sim_debug (DBG_FINAL, & cpu_dev,
                       "Read2 (Actual) Read:       abs address=%08o" "  readData=%012"PRIo64"\r\n",
                       address + i, result [i]);
        }
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_read, 0, address, "Read2 AB");
        HDBGMRead (cpu.iefpFinalAddress, * result, "Read2 AB evn");
        HDBGMRead (cpu.iefpFinalAddress+1, * (result+1), "Read2 AB odd");
# endif
        return;
      }
    }

    case APPEND_mode: {
B29:;
      if (isBAR) {
        cpu.TPR.CA = get_BAR_address (cpup, address);
        cpu.TPR.TSR = cpu.PPR.PSR;
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.iefpFinalAddress = do_append_cycle (cpup, cyctyp, result, 2);
        if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev) {
          for (uint i = 0; i < 2; i ++)
           sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                      "Read2 (Actual) Read:  bar iefpFinalAddress=" "%08o  readData=%012"PRIo64"\r\n",
                      cpu.iefpFinalAddress + i, result [i]);
        }
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "Read2 BR");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read2 BR evn");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, * (result+1), "Read2 BR odd");
# endif
        return;
      } else {
        cpu.iefpFinalAddress = do_append_cycle (cpup, cyctyp, result, 2);
        if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev) {
          for (uint i = 0; i < 2; i ++)
            sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                       "Read2 (Actual) Read:  iefpFinalAddress=%08o" "  readData=%012"PRIo64"\r\n",
                       cpu.iefpFinalAddress + i, result [i]);
        }
        if_sim_debug (DBG_FINAL, & cpu_dev) {
          if (cyctyp == OPERAND_READ) {
            for (uint i = 0; i < 2; i ++)
              sim_debug (DBG_FINAL, & cpu_dev,
                         "Read2 (Actual) Read:  iefpFinalAddress=%08o" "  readData=%012"PRIo64"\r\n",
                         cpu.iefpFinalAddress + i, result [i]);
          }
        }
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "Read2");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read2 evn");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, * (result+1), "Read2 odd");
# endif
      }
      return;
    }
  }
}
#endif // OLDAPP

#if !defined(OLDAPP)
void Read2OperandRead (cpu_state_t * cpup, word18 address, word36 * result) {
  cpu.TPR.CA = cpu.iefpFinalAddress = address;

  bool isBAR = get_bar_mode (cpup);

  if (cpu.cu.XSF || cpu.currentInstruction.b29)
    goto B29;

  switch (get_addr_mode (cpup)) {
    case ABSOLUTE_mode: {
      if (isBAR) {
        set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
        cpu.iefpFinalAddress = get_BAR_address (cpup, address);

        fauxDoAppendCycle (cpup, OPERAND_READ);
        core_read2 (cpup, cpu.iefpFinalAddress, result + 0, result + 1, __func__);
        if_sim_debug (DBG_FINAL, & cpu_dev) {
            for (uint i = 0; i < 2; i ++)
              sim_debug (DBG_FINAL, & cpu_dev,
                         "Read2OperandRead (Actual) Read:       bar address=%08o" "  readData=%012"PRIo64"\r\n",
                         address + i, result [i]);
        }
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "Read2OperandRead ABBR");
        HDBGMRead (cpu.iefpFinalAddress, * result, "Read2OperandRead ABBR evn");
        HDBGMRead (cpu.iefpFinalAddress+1, * (result+1), "Read2OperandRead ABBR odd");
# endif
        return;
      } else {
        set_apu_status (cpup, apuStatus_FABS);
        fauxDoAppendCycle (cpup, OPERAND_READ);
        core_read2 (cpup, address, result + 0, result + 1, __func__);
        if_sim_debug (DBG_FINAL, & cpu_dev) {
          for (uint i = 0; i < 2; i ++)
            sim_debug (DBG_FINAL, & cpu_dev,
                       "Read2OperandRead (Actual) Read:       abs address=%08o" "  readData=%012"PRIo64"\r\n",
                       address + i, result [i]);
        }
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_read, 0, address, "Read2OperandRead AB");
        HDBGMRead (cpu.iefpFinalAddress, * result, "Read2OperandRead AB evn");
        HDBGMRead (cpu.iefpFinalAddress+1, * (result+1), "Read2OperandRead AB odd");
# endif
        return;
      }
    }

    case APPEND_mode: {
B29:;
      if (isBAR) {
        cpu.TPR.CA = get_BAR_address (cpup, address);
        cpu.TPR.TSR = cpu.PPR.PSR;
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.iefpFinalAddress = doAppendCycleOperandRead (cpup, result, 2);
        if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev) {
          for (uint i = 0; i < 2; i ++)
           sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                      "Read2OperandRead (Actual) Read:  bar iefpFinalAddress=" "%08o  readData=%012"PRIo64"\r\n",
                      cpu.iefpFinalAddress + i, result [i]);
        }
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "Read2OperandRead BR");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read2OperandRead BR evn");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, * (result+1), "Read2OperandRead BR odd");
# endif
        return;
      } else {
        cpu.iefpFinalAddress = doAppendCycleOperandRead (cpup, result, 2);
        if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev) {
          for (uint i = 0; i < 2; i ++)
            sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                       "Read2OperandRead (Actual) Read:  iefpFinalAddress=%08o" "  readData=%012"PRIo64"\r\n",
                       cpu.iefpFinalAddress + i, result [i]);
        }
        if_sim_debug (DBG_FINAL, & cpu_dev) {
          for (uint i = 0; i < 2; i ++)
            sim_debug (DBG_FINAL, & cpu_dev,
                       "Read2OperandRead (Actual) Read:  iefpFinalAddress=%08o" "  readData=%012"PRIo64"\r\n",
                       cpu.iefpFinalAddress + i, result [i]);
        }
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "Read2OperandRead");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read2OperandRead evn");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, * (result+1), "Read2OperandRead odd");
# endif
        return;
      }
    }
  }
}

# if defined(LOCKLESS)
void Read2OperandRMW (cpu_state_t * cpup, word18 address, word36 * result) {
  cpu.TPR.CA = cpu.iefpFinalAddress = address;

  bool isBAR = get_bar_mode (cpup);

  if (cpu.cu.XSF || cpu.currentInstruction.b29)
    goto B29;

  switch (get_addr_mode (cpup)) {
    case ABSOLUTE_mode: {
      if (isBAR) {
        set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
        cpu.iefpFinalAddress = get_BAR_address (cpup, address);

        fauxDoAppendCycle (cpup, OPERAND_RMW);
        core_read2 (cpup, cpu.iefpFinalAddress, result + 0, result + 1, __func__);
        if_sim_debug (DBG_FINAL, & cpu_dev) {
            for (uint i = 0; i < 2; i ++)
              sim_debug (DBG_FINAL, & cpu_dev,
                         "Read2OperandRMW (Actual) Read:       bar address=%08o" "  readData=%012"PRIo64"\r\n",
                         address + i, result [i]);
        }
#  if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "Read2OperandRMW ABBR");
        HDBGMRead (cpu.iefpFinalAddress, * result, "Read2OperandRMW ABBR evn");
        HDBGMRead (cpu.iefpFinalAddress+1, * (result+1), "Read2OperandRMW ABBR odd");
#  endif
        return;
      } else {
        set_apu_status (cpup, apuStatus_FABS);
        fauxDoAppendCycle (cpup, OPERAND_RMW);
        core_read2 (cpup, address, result + 0, result + 1, __func__);
        if_sim_debug (DBG_FINAL, & cpu_dev) {
          for (uint i = 0; i < 2; i ++)
            sim_debug (DBG_FINAL, & cpu_dev,
                       "Read2OperandRMW (Actual) Read:       abs address=%08o" "  readData=%012"PRIo64"\r\n",
                       address + i, result [i]);
        }
#  if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_read, 0, address, "Read2OperandRMW AB");
        HDBGMRead (cpu.iefpFinalAddress, * result, "Read2OperandRMW AB evn");
        HDBGMRead (cpu.iefpFinalAddress+1, * (result+1), "Read2OperandRMW AB odd");
#  endif
        return;
      }
    }

    case APPEND_mode: {
B29:;
      if (isBAR) {
        cpu.TPR.CA = get_BAR_address (cpup, address);
        cpu.TPR.TSR = cpu.PPR.PSR;
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.iefpFinalAddress = doAppendCycleOperandRMW (cpup, result, 2);
        if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev) {
          for (uint i = 0; i < 2; i ++)
           sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                      "Read2OperandRMW (Actual) Read:  bar iefpFinalAddress=" "%08o  readData=%012"PRIo64"\r\n",
                      cpu.iefpFinalAddress + i, result [i]);
        }
#  if defined(TESTING)
        HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "Read2OperandRMW BR");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read2OperandRMW BR evn");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, * (result+1), "Read2OperandRMW BR odd");
#  endif
        return;
      } else {
        cpu.iefpFinalAddress = doAppendCycleOperandRMW (cpup, result, 2);
        if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev) {
          for (uint i = 0; i < 2; i ++)
            sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                       "Read2OperandRMW (Actual) Read:  iefpFinalAddress=%08o" "  readData=%012"PRIo64"\r\n",
                       cpu.iefpFinalAddress + i, result [i]);
        }
#  if defined(TESTING)
        HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "Read2OperandRMW");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read2OperandRMW evn");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, * (result+1), "Read2OperandRMW odd");
#  endif
        return;
      }
    }
  }
}
# endif

void Read2InstructionFetch (cpu_state_t * cpup, word18 address, word36 * result) {
  cpu.TPR.CA = cpu.iefpFinalAddress = address;

  bool isBAR = get_bar_mode (cpup);

  if (cpu.cu.XSF)
    goto B29;

  switch (get_addr_mode (cpup)) {
    case ABSOLUTE_mode: {
      if (isBAR) {
        set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
        cpu.iefpFinalAddress = get_BAR_address (cpup, address);

        fauxDoAppendCycle (cpup, INSTRUCTION_FETCH);
        core_read2 (cpup, cpu.iefpFinalAddress, result + 0, result + 1, __func__);
        if_sim_debug (DBG_FINAL, & cpu_dev) {
            for (uint i = 0; i < 2; i ++)
              sim_debug (DBG_FINAL, & cpu_dev,
                         "Read2InstructionFetch (Actual) Read:       bar address=%08o" "  readData=%012"PRIo64"\r\n",
                         address + i, result [i]);
        }
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "Read2InstructionFetch ABBR");
        HDBGMRead (cpu.iefpFinalAddress, * result, "Read2InstructionFetch ABBR evn");
        HDBGMRead (cpu.iefpFinalAddress+1, * (result+1), "Read2InstructionFetch ABBR odd");
# endif
        return;
      } else {
        set_apu_status (cpup, apuStatus_FABS);
        fauxDoAppendCycle (cpup, INSTRUCTION_FETCH);
        core_read2 (cpup, address, result + 0, result + 1, __func__);
        if_sim_debug (DBG_FINAL, & cpu_dev) {
          for (uint i = 0; i < 2; i ++)
            sim_debug (DBG_FINAL, & cpu_dev,
                       "Read2InstructionFetch (Actual) Read:       abs address=%08o" "  readData=%012"PRIo64"\r\n",
                       address + i, result [i]);
        }
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_read, 0, address, "Read2InstructionFetch AB");
        HDBGMRead (cpu.iefpFinalAddress, * result, "Read2InstructionFetch AB evn");
        HDBGMRead (cpu.iefpFinalAddress+1, * (result+1), "Read2InstructionFetch AB odd");
# endif
        return;
      }
    }

    case APPEND_mode: {
B29:;
      if (isBAR) {
        cpu.TPR.CA = get_BAR_address (cpup, address);
        cpu.TPR.TSR = cpu.PPR.PSR;
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.iefpFinalAddress = doAppendCycleInstructionFetch (cpup, result, 2);
        if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev) {
          for (uint i = 0; i < 2; i ++)
           sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                      "Read2InstructionFetch (Actual) Read:  bar iefpFinalAddress=" "%08o  readData=%012"PRIo64"\r\n",
                      cpu.iefpFinalAddress + i, result [i]);
        }
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "Read2InstructionFetch BR");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read2InstructionFetch BR evn");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, * (result+1), "Read2InstructionFetch BR odd");
# endif
        return;
      } else {
        cpu.iefpFinalAddress = doAppendCycleInstructionFetch (cpup, result, 2);
        if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev) {
          for (uint i = 0; i < 2; i ++)
            sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                       "Read2InstructionFetch (Actual) Read:  iefpFinalAddress=%08o" "  readData=%012"PRIo64"\r\n",
                       cpu.iefpFinalAddress + i, result [i]);
        }
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "Read2InstructionFetch");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read2InstructionFetch evn");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, * (result+1), "Read2InstructionFetch odd");
# endif
        return;
      }
    }
  }
}

void Read2RTCDOperandFetch (cpu_state_t * cpup, word18 address, word36 * result) {
  cpu.TPR.CA = cpu.iefpFinalAddress = address;

  bool isBAR = get_bar_mode (cpup);

  if (isBAR) {
    cpu.TPR.CA = get_BAR_address (cpup, address);
    cpu.TPR.TSR = cpu.PPR.PSR;
    cpu.TPR.TRR = cpu.PPR.PRR;
    cpu.iefpFinalAddress = doAppendCycleRTCDOperandFetch (cpup, result, 2);
    if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev) {
      for (uint i = 0; i < 2; i ++)
       sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                  "Read2 (Actual) Read:  bar iefpFinalAddress=" "%08o  readData=%012"PRIo64"\r\n",
                  cpu.iefpFinalAddress + i, result [i]);
    }
# if defined(TESTING)
    HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "Read2 BR");
    HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read2 BR evn");
    HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, * (result+1), "Read2 BR odd");
# endif
    return;
  } else {
    cpu.iefpFinalAddress = doAppendCycleRTCDOperandFetch (cpup, result, 2);
    if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev) {
      for (uint i = 0; i < 2; i ++)
        sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                   "Read2 (Actual) Read:  iefpFinalAddress=%08o" "  readData=%012"PRIo64"\r\n",
                   cpu.iefpFinalAddress + i, result [i]);
    }
# if defined(TESTING)
    HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "Read2");
    HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read2 evn");
    HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, * (result+1), "Read2 odd");
# endif
    return;
  }
}

void Read2IndirectWordFetch (cpu_state_t * cpup, word18 address, word36 * result) {
  cpu.TPR.CA = cpu.iefpFinalAddress = address;

  bool isBAR = get_bar_mode (cpup);

  if (cpu.cu.XSF || cpu.currentInstruction.b29)
    goto B29;

  switch (get_addr_mode (cpup)) {
    case ABSOLUTE_mode: {
      if (isBAR) {
        set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
        cpu.iefpFinalAddress = get_BAR_address (cpup, address);

        fauxDoAppendCycle (cpup, INDIRECT_WORD_FETCH);
        core_read2 (cpup, cpu.iefpFinalAddress, result + 0, result + 1, __func__);
        if_sim_debug (DBG_FINAL, & cpu_dev) {
            for (uint i = 0; i < 2; i ++)
              sim_debug (DBG_FINAL, & cpu_dev,
                         "Read2IndirectWordFetch (Actual) Read:       bar address=%08o" "  readData=%012"PRIo64"\r\n",
                         address + i, result [i]);
        }
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "Read2IndirectWordFetch ABBR");
        HDBGMRead (cpu.iefpFinalAddress, * result, "Read2IndirectWordFetch ABBR evn");
        HDBGMRead (cpu.iefpFinalAddress+1, * (result+1), "Read2IndirectWordFetch ABBR odd");
# endif
        return;
      } else {
        set_apu_status (cpup, apuStatus_FABS);
        fauxDoAppendCycle (cpup, INDIRECT_WORD_FETCH);
        core_read2 (cpup, address, result + 0, result + 1, __func__);
        if_sim_debug (DBG_FINAL, & cpu_dev) {
          for (uint i = 0; i < 2; i ++)
            sim_debug (DBG_FINAL, & cpu_dev,
                       "Read2IndirectWordFetch (Actual) Read:       abs address=%08o" "  readData=%012"PRIo64"\r\n",
                       address + i, result [i]);
        }
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_read, 0, address, "Read2IndirectWordFetch AB");
        HDBGMRead (cpu.iefpFinalAddress, * result, "Read2IndirectWordFetch AB evn");
        HDBGMRead (cpu.iefpFinalAddress+1, * (result+1), "Read2IndirectWordFetch AB odd");
# endif
        return;
      }
    }

    case APPEND_mode: {
B29:;
      if (isBAR) {
        cpu.TPR.CA = get_BAR_address (cpup, address);
        cpu.TPR.TSR = cpu.PPR.PSR;
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.iefpFinalAddress = doAppendCycleIndirectWordFetch (cpup, result, 2);
        if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev) {
          for (uint i = 0; i < 2; i ++)
           sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                      "Read2IndirectWordFetch (Actual) Read:  bar iefpFinalAddress=" "%08o  readData=%012"PRIo64"\r\n",
                      cpu.iefpFinalAddress + i, result [i]);
        }
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "Read2IndirectWordFetch BR");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read2IndirectWordFetch BR evn");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, * (result+1), "Read2IndirectWordFetch BR odd");
# endif
        return;
      } else {
        cpu.iefpFinalAddress = doAppendCycleIndirectWordFetch (cpup, result, 2);
        if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev) {
          for (uint i = 0; i < 2; i ++)
            sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                       "Read2IndirectWordFetch (Actual) Read:  iefpFinalAddress=%08o" "  readData=%012"PRIo64"\r\n",
                       cpu.iefpFinalAddress + i, result [i]);
        }
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "Read2IndirectWordFetch");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read2IndirectWordFetch evn");
        HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, * (result+1), "Read2IndirectWordFetch odd");
        return;
# endif
      }
    }
  }
}
#endif // !OLDAPP

void Read8 (cpu_state_t * cpup, word18 address, word36 * result, bool isAR)
  {
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode (cpup);

    if (isAR || cpu.cu.XSF /*get_went_appending ()*/)
      {
        goto B29;
      }

    switch (get_addr_mode (cpup))
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
                cpu.iefpFinalAddress = get_BAR_address (cpup, address);

                fauxDoAppendCycle (cpup, APU_DATA_READ);
                core_readN (cpup, cpu.iefpFinalAddress, result, 8, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "Read8 (Actual) Read:       bar address=%08o"
                                 "  readData=%012"PRIo64"\r\n",
                                 address + i, result [i]);
                  }
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "Read8 ABBR");
                for (uint i = 0; i < 8; i ++)
                  HDBGMRead (cpu.iefpFinalAddress + i, result [i], "Read8 ABBR");
#endif
                return;
              }
            else
              {
                set_apu_status (cpup, apuStatus_FABS);
                fauxDoAppendCycle (cpup, APU_DATA_READ);
                core_readN (cpup, address, result, 8, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "Read8 (Actual) Read:       abs address=%08o"
                                 "  readData=%012"PRIo64"\r\n",
                                 address + i, result [i]);
                  }
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_abs_read, 0, address, "Read8 ABS");
                for (uint i = 0; i < 8; i ++)
                  HDBGMRead (address + i, result [i], "Read8 ABS");
#endif
                return;
              }
          }

        case APPEND_mode:
          {
B29:;
            if (isBAR)
              {
                cpu.TPR.CA = get_BAR_address (cpup, address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                //cpu.iefpFinalAddress = do_append_cycle (APU_DATA_READ, result, 8);
                cpu.iefpFinalAddress = doAppendCycleAPUDataRead (cpup, result, 8);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                     sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                "Read8 (Actual) Read:  bar iefpFinalAddress="
                                "%08o  readData=%012"PRIo64"\r\n",
                                cpu.iefpFinalAddress + i, result [i]);
                  }
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "Read8 BAR");
                for (uint i = 0; i < 8; i ++)
                  HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + i, cpu.iefpFinalAddress + i, result[i], "Read8 BAR");
#endif
                return;
              }
            else
              {
                //cpu.iefpFinalAddress = do_append_cycle (APU_DATA_READ, result, 8);
                cpu.iefpFinalAddress = doAppendCycleAPUDataRead (cpup, result, 8);
                // XXX Don't trace Multics idle loop
                if (cpu.PPR.PSR != 061 && cpu.PPR.IC != 0307)
                  {
                    if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                      {
                        for (uint i = 0; i < 8; i ++)
                          sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                     "Read8 (Actual) Read:  iefpFinalAddress="
                                     "%08o  readData=%012"PRIo64"\r\n",
                                     cpu.iefpFinalAddress + i, result [i]);
                      }
#if defined(TESTING)
                    HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "Read8");
                    for (uint i = 0; i < 8; i ++)
                      HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + i, cpu.iefpFinalAddress + i, result [i], "Read8");
#endif
                  }
                return;
              }
          }
      }
  }

void Read16 (cpu_state_t * cpup, word18 address, word36 * result)
  {
    address &= paragraphMask; // Round to 8 word boundary
    Read8 (cpup, address, result, cpu.currentInstruction.b29);
    Read8 (cpup, address + 8, result + 8, cpu.currentInstruction.b29);
    return;
  }

void ReadPage (cpu_state_t * cpup, word18 address, word36 * result, bool isAR)
  {
    if ((address & PGMK) != 0)
      {
        sim_warn ("ReadPage not on boundary %06o\r\n", address);
      }
    address &= (word18) ~PGMK; // Round to page boundary
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode (cpup);

    if (isAR || cpu.cu.XSF /*get_went_appending ()*/)
      {
        goto B29;
      }

    switch (get_addr_mode (cpup))
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
                cpu.iefpFinalAddress = get_BAR_address (cpup, address);

                fauxDoAppendCycle (cpup, APU_DATA_READ);
                core_readN (cpup, cpu.iefpFinalAddress, result, PGSZ, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "ReadPage (Actual) Read:       bar address="
                                 "%08o  readData=%012"PRIo64"\r\n",
                                  address + i, result [i]);
                  }
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "ReadPage AB");
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGMRead (cpu.iefpFinalAddress + i, result [i], "ReadPage AB");
#endif
                return;
              }
            else
              {
                set_apu_status (cpup, apuStatus_FABS);
                fauxDoAppendCycle (cpup, APU_DATA_READ);
                core_readN (cpup, address, result, PGSZ, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "ReadPage (Actual) Read:       abs address="
                                 "%08o  readData=%012"PRIo64"\r\n",
                                 address + i, result [i]);
                  }
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_abs_read, 0, address, "ReadPage ABS");
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGMRead (address + i, result [i], "ReadPage ABS");
#endif
                return;
              }
          }

        case APPEND_mode:
          {
B29:;
            if (isBAR)
              {
                cpu.TPR.CA = get_BAR_address (cpup, address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                //cpu.iefpFinalAddress = do_append_cycle (APU_DATA_READ, result, PGSZ);
                cpu.iefpFinalAddress = doAppendCycleAPUDataRead (cpup, result, PGSZ);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                     sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                "ReadPage (Actual) Read:  bar iefpFinalAddress="
                                "%08o  readData=%012"PRIo64"\r\n",
                                cpu.iefpFinalAddress + i, result [i]);
                  }
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "ReadPage B");
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + i, cpu.iefpFinalAddress + i, result [i], "ReadPage B");
#endif

                return;
              }
            else
              {
                //cpu.iefpFinalAddress = do_append_cycle (APU_DATA_READ, result, PGSZ);
                cpu.iefpFinalAddress = doAppendCycleAPUDataRead (cpup, result, PGSZ);
                // XXX Don't trace Multics idle loop
                if (cpu.PPR.PSR != 061 && cpu.PPR.IC != 0307)
                  {
                    if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                      {
                        for (uint i = 0; i < PGSZ; i ++)
                          sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                     "ReadPage (Actual) Read:  iefpFinalAddress"
                                     "=%08o  readData=%012"PRIo64"\r\n",
                                     cpu.iefpFinalAddress + i, result [i]);
                      }
#if defined(TESTING)
                    HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "ReadPage");
                    for (uint i = 0; i < PGSZ; i ++)
                      HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + i, cpu.iefpFinalAddress + i, result [i], "ReadPage");
#endif
                  }
                return;
              }
          }
      }
  }

#if defined(OLDAPP)
void Write (cpu_state_t * cpup, word18 address, word36 data, processor_cycle_type cyctyp) {
  cpu.TPR.CA = cpu.iefpFinalAddress = address;

  bool isBAR = get_bar_mode ();

  if (cpu.cu.XSF || (cyctyp != INSTRUCTION_FETCH && cpu.currentInstruction.b29))
    goto B29;

  switch (get_addr_mode ()) {
    case ABSOLUTE_mode: {
      if (isBAR) {
        cpu.iefpFinalAddress = get_BAR_address (address);
        set_apu_status (apuStatus_FABS); // XXX maybe...
        fauxDoAppendCycle (cyctyp);
        if (cyctyp == OPERAND_STORE && cpu.useZone)
          core_write_zone (cpu.iefpFinalAddress, data, __func__);
        else
          core_write (cpu.iefpFinalAddress, data, __func__);
        sim_debug (DBG_FINAL, & cpu_dev, "Write(Actual) Write:      bar address=%08o writeData=%012"PRIo64"\r\n", address, data);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address, "Write ABBR");
        HDBGMWrite (cpu.iefpFinalAddress, data, "Write ABBR");
# endif
        return;
      } else {
        set_apu_status (apuStatus_FABS);
        fauxDoAppendCycle (cyctyp);
        if (cyctyp == OPERAND_STORE && cpu.useZone)
          core_write_zone (address, data, __func__);
        else
          core_write (address, data, __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "Write(Actual) Write:      abs address=%08o writeData=%012"PRIo64"\r\n",
                   address, data);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_write, 0, address, "Write AB");
        HDBGMWrite (address, data, "Write AB");
# endif
        return;
      }
    }

    case APPEND_mode: {
B29:
      if (isBAR) {
        cpu.TPR.CA = get_BAR_address (cpup, address);
        cpu.TPR.TSR = cpu.PPR.PSR;
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.iefpFinalAddress = do_append_cycle (cpup, cyctyp, & data, 1);
        sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                   "Write(Actual) Write: bar iefpFinalAddress=%08o writeData=%012"PRIo64"\r\n",
                   cpu.iefpFinalAddress, data);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address, "Write BR");
        HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data, "Write BR");
# endif
        return;
      } else {
        cpu.iefpFinalAddress = do_append_cycle (cpup, cyctyp, & data, 1);
        sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                   "Write(Actual) Write: iefpFinalAddress=%08o " "writeData=%012"PRIo64"\r\n",
                   cpu.iefpFinalAddress, data);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address, "Write");
        HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data, "Write");
# endif
        return;
      }
    }
  }
}
#endif // OLDAPP

#if !defined(OLDAPP)
void WriteAPUDataStore (cpu_state_t * cpup, word18 address, word36 data) {
  cpu.TPR.CA = cpu.iefpFinalAddress = address;

  bool isBAR = get_bar_mode (cpup);

  if (cpu.cu.XSF || cpu.currentInstruction.b29)
    goto B29;

  switch (get_addr_mode (cpup)) {
    case ABSOLUTE_mode: {
      if (isBAR) {
        cpu.iefpFinalAddress = get_BAR_address (cpup, address);
        set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
        fauxDoAppendCycle (cpup, APU_DATA_STORE);
        core_write (cpup, cpu.iefpFinalAddress, data, __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "WriteAPUDataStore(Actual) Write:      bar address=%08o writeData=%012"PRIo64"\r\n",
                   address, data);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address, "WriteAPUDataStore ABBR");
        HDBGMWrite (cpu.iefpFinalAddress, data, "WriteAPUDataStore ABBR");
# endif
        return;
      } else {
        set_apu_status (cpup, apuStatus_FABS);
        fauxDoAppendCycle (cpup, APU_DATA_STORE);
        core_write (cpup, address, data, __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "WriteAPUDataStore(Actual) Write:      abs address=%08o writeData=%012"PRIo64"\r\n",
                   address, data);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_write, 0, address, "WriteAPUDataStore AB");
        HDBGMWrite (address, data, "WriteAPUDataStore AB");
# endif
        return;
      }
    }

    case APPEND_mode: {
B29:
      if (isBAR) {
        cpu.TPR.CA = get_BAR_address (cpup, address);
        cpu.TPR.TSR = cpu.PPR.PSR;
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.iefpFinalAddress = doAppendCycleAPUDataStore (cpup, & data, 1);
        sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                   "WriteAPUDataStore(Actual) Write: bar iefpFinalAddress=%08o writeData=%012"PRIo64"\r\n",
                   cpu.iefpFinalAddress, data);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address, "WriteAPUDataStore BR");
        HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data, "WriteAPUDataStore BR");
# endif
        return;
      } else {
        cpu.iefpFinalAddress = doAppendCycleAPUDataStore (cpup, & data, 1);
        sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                   "WriteAPUDataStore(Actual) Write: iefpFinalAddress=%08o " "writeData=%012"PRIo64"\r\n",
                   cpu.iefpFinalAddress, data);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address, "WriteAPUDataStore");
        HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data, "WriteAPUDataStore");
# endif
        return;
      }
    }
  }
}

void WriteOperandStore (cpu_state_t * cpup, word18 address, word36 data) {
  cpu.TPR.CA = cpu.iefpFinalAddress = address;

  bool isBAR = get_bar_mode (cpup);

  if (cpu.cu.XSF || cpu.currentInstruction.b29)
    goto B29;

  switch (get_addr_mode (cpup)) {
    case ABSOLUTE_mode: {
      if (isBAR) {
        cpu.iefpFinalAddress = get_BAR_address (cpup, address);
        set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
        fauxDoAppendCycle (cpup, OPERAND_STORE);
        if (cpu.useZone)
          core_write_zone (cpup, cpu.iefpFinalAddress, data, __func__);
        else
          core_write (cpup, cpu.iefpFinalAddress, data, __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "WriteOperandStore(Actual) Write:      bar address=%08o writeData=%012"PRIo64"\r\n",
                   address, data);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address, "WriteOperandStore ABBR");
        HDBGMWrite (cpu.iefpFinalAddress, data, "WriteOperandStore ABBR");
# endif
        return;
      } else {
        set_apu_status (cpup, apuStatus_FABS);
        fauxDoAppendCycle (cpup, OPERAND_STORE);
        if (cpu.useZone)
          core_write_zone (cpup, address, data, __func__);
        else
          core_write (cpup, address, data, __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "WriteOperandStore(Actual) Write:      abs address=%08o writeData=%012"PRIo64"\r\n",
                   address, data);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_write, 0, address, "WriteOperandStore AB");
        HDBGMWrite (address, data, "WriteOperandStore AB");
# endif
        return;
      }
    }

    case APPEND_mode: {
B29:
      if (isBAR) {
        cpu.TPR.CA = get_BAR_address (cpup, address);
        cpu.TPR.TSR = cpu.PPR.PSR;
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.iefpFinalAddress = doAppendCycleOperandStore (cpup, & data, 1);
        sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                   "WriteOperandStore(Actual) Write: bar iefpFinalAddress=%08o writeData=%012"PRIo64"\r\n",
                   cpu.iefpFinalAddress, data);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address, "WriteOperandStore BR");
        HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data, "WriteOperandStore BR");
# endif
        return;
      } else {
        cpu.iefpFinalAddress = doAppendCycleOperandStore (cpup, & data, 1);
        sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                   "WriteOperandStore(Actual) Write: iefpFinalAddress=%08o " "writeData=%012"PRIo64"\r\n",
                   cpu.iefpFinalAddress, data);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address, "WriteOperandStore");
        HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data, "WriteOperandStore");
# endif
        return;
      }
    }
  }
}
#endif // !OLDAPP

#if defined(OLDAPP)
void Write2 (cpu_state_t * cpup, word18 address, word36 * data, processor_cycle_type cyctyp)
  {
    cpu.TPR.CA = cpu.iefpFinalAddress = address;
    bool isBAR = get_bar_mode ();

    if (cpu.cu.XSF /*get_went_appending ()*/ || (cyctyp != INSTRUCTION_FETCH && cpu.currentInstruction.b29))
      goto B29;

    switch (get_addr_mode ())
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                cpu.iefpFinalAddress = get_BAR_address (address);
                set_apu_status (apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (cyctyp);
                core_write2 (cpu.iefpFinalAddress, data [0], data [1],
                              __func__);
# if defined(TESTING)
                HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address, "Write2 ABBR");
                HDBGMWrite (cpu.iefpFinalAddress, data [0], "Write2 ABBR evn");
                HDBGMWrite (cpu.iefpFinalAddress+1, data [1], "Write2 ABBR odd");
# endif
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Write2 (Actual) Write:      bar address=%08o "
                           "writeData=%012"PRIo64" %012"PRIo64"\r\n",
                           address, data [0], data [1]);
                return;
              }
            else
              {
                set_apu_status (apuStatus_FABS);
                fauxDoAppendCycle (cyctyp);
                core_write2 (address, data [0], data [1], __func__);
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Write2 (Actual) Write:      abs address=%08o "
                           "writeData=%012"PRIo64" %012"PRIo64"\r\n",
                           address, data [0], data [1]);
# if defined(TESTING)
                HDBGIEFP (hdbgIEFP_abs_write, 0, address, "Write2 AB");
                HDBGMWrite (address, data [0], "Write2 AB evn");
                HDBGMWrite (address+1, data [1], "Write2 AB odd");
# endif
                return;
              }
          }

        case APPEND_mode:
          {
B29:
            if (isBAR)
              {
                cpu.TPR.CA = get_BAR_address (address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                cpu.iefpFinalAddress = do_append_cycle (cyctyp, data, 2);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Write2 (Actual) Write: bar iefpFinalAddress=%08o "
                           "writeData=%012"PRIo64" %012"PRIo64"\r\n",
                           address, data [0], data [1]);
# if defined(TESTING)
                HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address, "Write2 BR");
                HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data[0], "Write2 BR evn");
                HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, data[1], "Write2 BR odd");
# endif
                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (cyctyp, data, 2);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Write2 (Actual) Write: iefpFinalAddress=%08o "
                           "writeData=%012"PRIo64" %012"PRIo64"\r\n",
                           address, data [0], data [1]);
# if defined(TESTING)
                HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address, "Write2");
                HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data[0], "Write2 evn");
                HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, data[1], "Write2 odd");
# endif
                return;
              }
          }
      }
  }
#endif // OLDAPP

#if !defined(OLDAPP)
void Write2OperandStore (cpu_state_t * cpup, word18 address, word36 * data) {
  cpu.TPR.CA = cpu.iefpFinalAddress = address;
  bool isBAR = get_bar_mode (cpup);

  if (cpu.cu.XSF || cpu.currentInstruction.b29)
    goto B29;

  switch (get_addr_mode (cpup)) {
    case ABSOLUTE_mode: {
      if (isBAR) {
        cpu.iefpFinalAddress = get_BAR_address (cpup, address);
        set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
        fauxDoAppendCycle (cpup, OPERAND_STORE);
        core_write2 (cpup, cpu.iefpFinalAddress, data [0], data [1], __func__);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address, "Write2OperandStore ABBR");
        HDBGMWrite (cpu.iefpFinalAddress, data [0], "Write2OperandStore ABBR evn");
        HDBGMWrite (cpu.iefpFinalAddress+1, data [1], "Write2OperandStore ABBR odd");
# endif
        sim_debug (DBG_FINAL, & cpu_dev,
                   "Write2OperandStore (Actual) Write:      bar address=%08o " "writeData=%012"PRIo64" %012"PRIo64"\r\n",
                   address, data [0], data [1]);
        return;
      } else {
        set_apu_status (cpup, apuStatus_FABS);
        fauxDoAppendCycle (cpup, OPERAND_STORE);
        core_write2 (cpup, address, data [0], data [1], __func__);
        sim_debug (DBG_FINAL, & cpu_dev,
                   "Write2OperandStore (Actual) Write:      abs address=%08o " "writeData=%012"PRIo64" %012"PRIo64"\r\n",
                   address, data [0], data [1]);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_abs_write, 0, address, "Write2OperandStore AB");
        HDBGMWrite (address, data [0], "Write2OperandStore AB evn");
        HDBGMWrite (address+1, data [1], "Write2OperandStore AB odd");
# endif
        return;
      }
    }

    case APPEND_mode: {
B29:
      if (isBAR) {
        cpu.TPR.CA = get_BAR_address (cpup, address);
        cpu.TPR.TSR = cpu.PPR.PSR;
        cpu.TPR.TRR = cpu.PPR.PRR;
        cpu.iefpFinalAddress = doAppendCycleOperandStore (cpup, data, 2);
        sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                   "Write2OperandStore (Actual) Write: bar iefpFinalAddress=%08o " "writeData=%012"PRIo64" %012"PRIo64"\r\n",
                   address, data [0], data [1]);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address, "Write2OperandStore BR");
        HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data[0], "Write2OperandStore BR evn");
        HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, data[1], "Write2OperandStore BR odd");
# endif
        return;
      } else {
        cpu.iefpFinalAddress = doAppendCycleOperandStore (cpup, data, 2);
        sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                   "Write2OperandStore (Actual) Write: iefpFinalAddress=%08o " "writeData=%012"PRIo64" %012"PRIo64"\r\n",
                   address, data [0], data [1]);
# if defined(TESTING)
        HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address, "Write2OperandStore");
        HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data[0], "Write2OperandStore evn");
        HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, data[1], "Write2OperandStore odd");
# endif
        return;
      }
    }
  }
}
#endif // !OLDAPP

void Write1 (cpu_state_t * cpup, word18 address, word36 data, bool isAR)
  {
    cpu.TPR.CA = cpu.iefpFinalAddress = address;
    bool isBAR = get_bar_mode (cpup);
    if (isAR || cpu.cu.XSF /*get_went_appending ()*/)
      goto B29;
    switch (get_addr_mode (cpup))
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
             {
                cpu.iefpFinalAddress = get_BAR_address (cpup, address);
                set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (cpup, APU_DATA_STORE);
                core_write (cpup, cpu.iefpFinalAddress, data, __func__);
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Write1(Actual) Write:      bar address=%08o "
                           "writeData=%012"PRIo64"\r\n",
                           address, data);
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address, "Write1 ABBR");
                HDBGMWrite (cpu.iefpFinalAddress, data, "Write1 ABBR");
#endif
                return;
              }
            else
              {
                set_apu_status (cpup, apuStatus_FABS);
                fauxDoAppendCycle (cpup, APU_DATA_STORE);
                core_write (cpup, address, data, __func__);
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Write1(Actual) Write:      abs address=%08o "
                           "writeData=%012"PRIo64"\r\n",
                           address, data);
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_abs_write, 0, address, "Write1 AB");
                HDBGMWrite (address, data, "Write1 AB");
#endif
                return;
              }
          }

        case APPEND_mode:
          {
B29:
            if (isBAR)
              {
                cpu.TPR.CA = get_BAR_address (cpup, address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                //cpu.iefpFinalAddress = do_append_cycle (APU_DATA_STORE, & data, 1);
                cpu.iefpFinalAddress = doAppendCycleAPUDataStore (cpup, & data, 1);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Write8(Actual) Write: bar iefpFinalAddress="
                           "%08o writeData=%012"PRIo64"\r\n",
                           cpu.iefpFinalAddress, data);
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address, "Write1 BR");
                HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data, "Write1 BR");
#endif
                return;
              }
            else
              {
                //cpu.iefpFinalAddress = do_append_cycle (APU_DATA_STORE, & data, 1);
                cpu.iefpFinalAddress = doAppendCycleAPUDataStore (cpup, & data, 1);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Write(Actual) Write: iefpFinalAddress=%08o "
                           "writeData=%012"PRIo64"\r\n",
                           cpu.iefpFinalAddress, data);
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address, "Write1");
                HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data, "Write1");
#endif
                return;
              }
          }
      }
  }

void Write8 (cpu_state_t * cpup, word18 address, word36 * data, bool isAR)
  {
    address &= paragraphMask; // Round to 8 word boundarryt
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode (cpup);

    if (isAR || cpu.cu.XSF /*get_went_appending ()*/)
      goto B29;

    switch (get_addr_mode (cpup))
     {
        case ABSOLUTE_mode:
          {
            if (isBAR)
             {
                cpu.iefpFinalAddress = get_BAR_address (cpup, address);
                set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (cpup, APU_DATA_STORE);
                core_writeN (cpup, cpu.iefpFinalAddress, data, 8, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "Write8(Actual) Write:      bar address=%08o "
                                 "writeData=%012"PRIo64"\r\n",
                                 address + i, data [i]);
                  }
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address, "Write8 ABBR");
                for (uint i = 0; i < 8; i ++)
                  HDBGMWrite (cpu.iefpFinalAddress + i, data [i], "Write8 ABBR");
#endif
                return;
              }
            else
              {
                set_apu_status (cpup, apuStatus_FABS);
                fauxDoAppendCycle (cpup, APU_DATA_STORE);
                core_writeN (cpup, address, data, 8, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "Write8(Actual) Write:      abs address=%08o "
                                 "writeData=%012"PRIo64"\r\n",
                                  address + i, data [i]);
                  }
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_abs_write, 0, address, "Write8 AB");
                for (uint i = 0; i < 8; i ++)
                  HDBGMWrite (address + i, data [i], "Write8 AB");
#endif
                return;
              }
          }

        case APPEND_mode:
          {
B29:
            if (isBAR)
              {
                cpu.TPR.CA = get_BAR_address (cpup, address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                //cpu.iefpFinalAddress = do_append_cycle (APU_DATA_STORE, data, 8);
                cpu.iefpFinalAddress = doAppendCycleAPUDataStore (cpup, data, 8);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                 "Write8(Actual) Write: bar iefpFinalAddress="
                                 "%08o writeData=%012"PRIo64"\r\n",
                                 cpu.iefpFinalAddress + i, data [i]);
                  }
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address, "Write8 BR");
                for (uint i = 0; i < 8; i ++)
                  HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA + i, cpu.iefpFinalAddress + i, data [i], "Write8 BR");
#endif

                return;
              }
            else
              {
                //cpu.iefpFinalAddress = do_append_cycle (APU_DATA_STORE, data, 8);
                cpu.iefpFinalAddress = doAppendCycleAPUDataStore (cpup, data, 8);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                 "Write8(Actual) Write: iefpFinalAddress=%08o "
                                 "writeData=%012"PRIo64"\r\n",
                                 cpu.iefpFinalAddress + i, data [i]);
                  }
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address, "Write8");
                for (uint i = 0; i < 8; i ++)
                  HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA + i, cpu.iefpFinalAddress + i, data [i], "Write8");
#endif

                return;
              }
          }
      }
  }

void Write16 (cpu_state_t * cpup, word18 address, word36 * data)
  {
    address &= paragraphMask; // Round to 8 word boundary
    Write8 (cpup, address, data, cpu.currentInstruction.b29);
    Write8 (cpup, address + 8, data + 8, cpu.currentInstruction.b29);
  }

void Write32 (cpu_state_t * cpup, word18 address, word36 * data)
  {
//#define paragraphMask 077777770
    //address &= paragraphMask; // Round to 8 word boundary
    address &= 077777740; // Round to 32 word boundary
    Write8 (cpup, address, data, cpu.currentInstruction.b29);
    Write8 (cpup, address + 8, data + 8, cpu.currentInstruction.b29);
    Write8 (cpup, address + 16, data + 16, cpu.currentInstruction.b29);
    Write8 (cpup, address + 24, data + 24, cpu.currentInstruction.b29);
  }

void WritePage (cpu_state_t * cpup, word18 address, word36 * data, bool isAR)
  {
    if ((address & PGMK) != 0)
      {
        sim_warn ("WritePage not on boundary %06o\r\n", address);
      }
    address &= (word18) ~PGMK; // Round to page boundary
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode (cpup);

    if (isAR || cpu.cu.XSF /*get_went_appending ()*/)
      goto B29;

    switch (get_addr_mode (cpup))
     {
        case ABSOLUTE_mode:
          {
            if (isBAR)
             {
                cpu.iefpFinalAddress = get_BAR_address (cpup, address);
                set_apu_status (cpup, apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (cpup, APU_DATA_STORE);
                core_writeN (cpup, cpu.iefpFinalAddress, data, PGSZ, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "WritePage(Actual) Write:      bar address="
                                 "%08o writeData=%012"PRIo64"\r\n",
                                 address + i, data [i]);
                  }
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address, "WritePage ABBR");
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGMWrite (cpu.iefpFinalAddress + i, data [i], "WritePage ABBR");
#endif
                return;
              }
            else
              {
                set_apu_status (cpup, apuStatus_FABS);
                fauxDoAppendCycle (cpup, APU_DATA_STORE);
                core_writeN (cpup, address, data, PGSZ, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "WritePage(Actual) Write:      abs address="
                                 "%08o writeData=%012"PRIo64"\r\n",
                                 address + i, data [i]);
                  }
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_abs_write, 0, address, "WritePage AB");
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGMWrite (address + i, data [i], "WritePage AB");
#endif
                return;
              }
          }

        case APPEND_mode:
          {
B29:
            if (isBAR)
              {
                cpu.TPR.CA = get_BAR_address (cpup, address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                //cpu.iefpFinalAddress = do_append_cycle (APU_DATA_STORE, data, PGSZ);
                cpu.iefpFinalAddress = doAppendCycleAPUDataStore (cpup, data, PGSZ);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                 "WritePage(Actual) Write: bar "
                                 "iefpFinalAddress=%08o writeData=%012"PRIo64
                                 "\r\n",
                                 cpu.iefpFinalAddress + i, data [i]);
                  }
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address, "WritePage BR");
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA + i, cpu.iefpFinalAddress + i, data [i], "WritePage BR");
#endif
                return;
              }
            else
              {
                //cpu.iefpFinalAddress = do_append_cycle (APU_DATA_STORE, data, PGSZ);
                cpu.iefpFinalAddress = doAppendCycleAPUDataStore (cpup, data, PGSZ);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                 "WritePage(Actual) Write: iefpFinalAddress="
                                 "%08o writeData=%012"PRIo64"\r\n",
                                 cpu.iefpFinalAddress + i, data [i]);
                  }
#if defined(TESTING)
                HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address, "WritePage");
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA + i, cpu.iefpFinalAddress + i, data [i], "WritePage");
#endif

                return;
              }
          }
      }
  }

void ReadIndirect (cpu_state_t * cpup)
  {
    if (cpu.TPR.CA & 1) // is odd?
      {
        ReadIndirectWordFetch (cpup, cpu.TPR.CA, cpu.itxPair);
        cpu.itxPair[1] = MASK36; // fill with ones for debugging
      }
    else
      {
        Read2IndirectWordFetch (cpup, cpu.TPR.CA, cpu.itxPair);
      }
  }
