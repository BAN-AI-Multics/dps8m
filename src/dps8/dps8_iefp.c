/*
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2015 Eric Swenson
 * Copyright (c) 2017 Michal Tomek
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 */

/**
 * \file dps8_iefp.c
 * \project dps8
 * \date 01/11/14
 * \copyright Copyright (c) 2014 Harry Reed. All rights reserved.
 */

#include "dps8.h"
#include "dps8_sys.h"
#include "dps8_scu.h"
#include "dps8_cpu.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_faults.h"
#include "dps8_append.h"
#include "dps8_iefp.h"
#include "dps8_addrmods.h"
#include "dps8_utils.h"

#define DBG_CTR cpu.cycleCnt

// new Read/Write stuff ...

void Read (cpu_state_t *cpu_p, word18 address, word36 * result, processor_cycle_type cyctyp)
  {
    cpu.TPR.CA = cpu.iefpFinalAddress = address;
    bool isBAR = get_bar_mode (cpu_p);

    //if (get_went_appending () ||
    if (cpu.cu.XSF || (cyctyp != INSTRUCTION_FETCH && cpu.currentInstruction.b29))
      {
        goto B29;
      }

    switch (get_addr_mode (cpu_p))
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                set_apu_status (cpu_p, apuStatus_FABS); // XXX maybe...
                cpu.iefpFinalAddress = get_BAR_address (cpu_p, address);
                fauxDoAppendCycle (cpup, cyctyp);
#ifdef LOCKLESS
                if (cyctyp == OPERAND_RMW || cyctyp == APU_DATA_RMW)
                  core_read_lock (cpu_p, cpu.iefpFinalAddress, result, __func__);
                else
                  core_read (cpu_p, cpu.iefpFinalAddress, result, __func__);
#else
                core_read (cpu_p, cpu.iefpFinalAddress, result, __func__);
#endif
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Read (Actual) Read:       bar address=%08o  "
                           "readData=%012"PRIo64"\n", address, *result);
                HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address);
                HDBGMRead (cpu.iefpFinalAddress, * result);
                return;
              }
            else
              {
                set_apu_status (cpu_p, apuStatus_FABS);
                fauxDoAppendCycle (cpup, cyctyp);
#ifdef LOCKLESS
                if (cyctyp == OPERAND_RMW || cyctyp == APU_DATA_RMW)
                  core_read_lock (cpu_p, address, result, __func__);
                else
                  core_read (cpu_p, address, result, __func__);
#else
                core_read (cpu_p, address, result, __func__);
#endif
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Read (Actual) Read:       abs address=%08o  "
                           "readData=%012"PRIo64"\n", address, *result);
                HDBGIEFP (hdbgIEFP_abs_read, 0, address);
                HDBGMRead (address, * result);
                return;
              }
          }

        case APPEND_mode:
          {
B29:;
            if (isBAR)
              {
                cpu.TPR.CA = get_BAR_address (cpu_p, address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, cyctyp, result, 1);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Read (Actual) Read:  bar iefpFinalAddress=%08o  "
                           "readData=%012"PRIo64"\n",
                           cpu.iefpFinalAddress, * result);
                HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address);
                HDBGMRead (cpu.iefpFinalAddress, * result);

                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, cyctyp, result, 1);
                // XXX Don't trace Multics idle loop
                if (cpu.PPR.PSR != 061 && cpu.PPR.IC != 0307)
                  {
                    sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                               "Read (Actual) Read:  iefpFinalAddress=%08o  "
                               "readData=%012"PRIo64"\n",
                               cpu.iefpFinalAddress, * result);
                    HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address);
                    HDBGMRead (cpu.iefpFinalAddress, * result);
                  }
              }
            return;
          }
      }
    return ;//SCPE_UNK;
  }

void Read2 (cpu_state_t *cpu_p, word18 address, word36 * result, processor_cycle_type cyctyp)
  {
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode (cpu_p);

    //if (get_went_appending () ||
    if (cpu.cu.XSF || (cyctyp != INSTRUCTION_FETCH && cpu.currentInstruction.b29) ||
        cyctyp == RTCD_OPERAND_FETCH) // ISOLTS-886
           // Another option would be to set_went_appending in ReadRTCDOp
      {
        goto B29;
      }

    switch (get_addr_mode (cpu_p))
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                set_apu_status (cpu_p, apuStatus_FABS); // XXX maybe...
                cpu.iefpFinalAddress = get_BAR_address (cpu_p, address);

                fauxDoAppendCycle (cpup, cyctyp);
                core_read2 (cpu_p, cpu.iefpFinalAddress, result + 0, result + 1,
                            __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 2; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                  "Read2 (Actual) Read:       bar address=%08o"
                                  "  readData=%012"PRIo64"\n",
                                  address + i, result [i]);
                  }
                HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address);
                HDBGMRead (cpu.iefpFinalAddress, * result);
                HDBGMRead (cpu.iefpFinalAddress+1, * (result+1));
                return;
              }
            else
              {
                set_apu_status (cpu_p, apuStatus_FABS);
                fauxDoAppendCycle (cpup, cyctyp);
                core_read2 (cpu_p, address, result + 0, result + 1, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 2; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "Read2 (Actual) Read:       abs address=%08o"
                                 "  readData=%012"PRIo64"\n",
                                 address + i, result [i]);
                  }
                HDBGIEFP (hdbgIEFP_abs_read, 0, address);
                HDBGMRead (cpu.iefpFinalAddress, * result);
                HDBGMRead (cpu.iefpFinalAddress+1, * (result+1));
                return;
              }
          }

        case APPEND_mode:
          {
B29:;
            if (isBAR)
              {
                cpu.TPR.CA = get_BAR_address (cpu_p, address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, cyctyp, result, 2);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 2; i ++)
                     sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                "Read2 (Actual) Read:  bar iefpFinalAddress="
                                "%08o  readData=%012"PRIo64"\n",
                                cpu.iefpFinalAddress + i, result [i]);
                  }
                HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address);
                HDBGMRead (cpu.iefpFinalAddress, * result);
                HDBGMRead (cpu.iefpFinalAddress+1, * (result+1));
                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, cyctyp, result, 2);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 2; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                 "Read2 (Actual) Read:  iefpFinalAddress=%08o"
                                 "  readData=%012"PRIo64"\n",
                                 cpu.iefpFinalAddress + i, result [i]);
                  }
                HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address);
                HDBGMRead (cpu.iefpFinalAddress, * result);
                HDBGMRead (cpu.iefpFinalAddress+1, * (result+1));
              }
            return;
          }
      }
    return ;//SCPE_UNK;
  }

void Read8 (cpu_state_t *cpu_p, word18 address, word36 * result, bool isAR)
  {
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode (cpu_p);

    if (isAR || cpu.cu.XSF /*get_went_appending ()*/)
      {
        goto B29;
      }

    switch (get_addr_mode (cpu_p))
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                set_apu_status (cpu_p, apuStatus_FABS); // XXX maybe...
                cpu.iefpFinalAddress = get_BAR_address (cpu_p, address);

                fauxDoAppendCycle (cpup, APU_DATA_READ);
                core_readN (cpu_p, cpu.iefpFinalAddress, result, 8, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "Read8 (Actual) Read:       bar address=%08o"
                                 "  readData=%012"PRIo64"\n",
                                 address + i, result [i]);
                  }
#ifdef HDBG
                HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address);
                for (uint i = 0; i < 8; i ++)
                  HDBGMRead (cpu.iefpFinalAddress + i, result [i]);
#endif
                return;
              }
            else
              {
                set_apu_status (cpu_p, apuStatus_FABS);
                fauxDoAppendCycle (cpup, APU_DATA_READ);
                core_readN (cpu_p, address, result, 8, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "Read8 (Actual) Read:       abs address=%08o"
                                 "  readData=%012"PRIo64"\n",
                                 address + i, result [i]);
                  }
#ifdef HDBG
                HDBGIEFP (hdbgIEFP_abs_read, 0, address);
                for (uint i = 0; i < 8; i ++)
                  HDBGMRead (address + i, result [i]);
#endif
                return;
              }
          }

        case APPEND_mode:
          {
B29:;
            if (isBAR)
              {
                cpu.TPR.CA = get_BAR_address (cpu_p, address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, APU_DATA_READ, result,
                                                        8);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                     sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                "Read8 (Actual) Read:  bar iefpFinalAddress="
                                "%08o  readData=%012"PRIo64"\n",
                                cpu.iefpFinalAddress + i, result [i]);
                  }
#ifdef HDBG
                HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address);
                for (uint i = 0; i < 8; i ++)
                  HDBGMRead (cpu.iefpFinalAddress + i, result[i]);
#endif
                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, APU_DATA_READ, result,
                                                        8);
                // XXX Don't trace Multics idle loop
                if (cpu.PPR.PSR != 061 && cpu.PPR.IC != 0307)
                  {
                    if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                      {
                        for (uint i = 0; i < 8; i ++)
                          sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                     "Read8 (Actual) Read:  iefpFinalAddress="
                                     "%08o  readData=%012"PRIo64"\n",
                                     cpu.iefpFinalAddress + i, result [i]);
                      }
#ifdef HDBG
                    HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address);
                    for (uint i = 0; i < 8; i ++)
                      HDBGMRead (cpu.iefpFinalAddress + i, result [i]);
#endif
                  }
              }
            return;
          }
      }
    return ;//SCPE_UNK;
  }

void Read16 (cpu_state_t *cpu_p, word18 address, word36 * result)
  {
    address &= paragraphMask; // Round to 8 word boundary
    Read8 (cpu_p, address, result, cpu.currentInstruction.b29);
    Read8 (cpu_p, address + 8, result + 8, cpu.currentInstruction.b29);
    return;
  }

void ReadPage (cpu_state_t *cpu_p, word18 address, word36 * result, bool isAR)
  {
    if ((address & PGMK) != 0)
      {
        sim_warn ("ReadPage not on boundary %06o\n", address);
      }
    address &= (word18) ~PGMK; // Round to page boundary
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode (cpu_p);

    if (isAR || cpu.cu.XSF /*get_went_appending ()*/)
      {
        goto B29;
      }

    switch (get_addr_mode (cpu_p))
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                set_apu_status (cpu_p, apuStatus_FABS); // XXX maybe...
                cpu.iefpFinalAddress = get_BAR_address (cpu_p, address);

                fauxDoAppendCycle (cpup, APU_DATA_READ);
                core_readN (cpu_p, cpu.iefpFinalAddress, result, PGSZ, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "ReadPage (Actual) Read:       bar address="
                                 "%08o  readData=%012"PRIo64"\n",
                                  address + i, result [i]);
                  }
#ifdef HDBG
                HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address);
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGMRead (cpu.iefpFinalAddress + i, result [i]);
#endif
                return;
              }
            else
              {
                set_apu_status (cpu_p, apuStatus_FABS);
                fauxDoAppendCycle (cpup, APU_DATA_READ);
                core_readN (cpu_p, address, result, PGSZ, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "ReadPage (Actual) Read:       abs address="
                                 "%08o  readData=%012"PRIo64"\n",
                                 address + i, result [i]);
                  }
#ifdef HDBG
                HDBGIEFP (hdbgIEFP_abs_read, 0, address);
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGMRead (address + i, result [i]);
#endif
                return;
              }
          }

        case APPEND_mode:
          {
B29:;
            if (isBAR)
              {
                cpu.TPR.CA = get_BAR_address (cpu_p, address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, APU_DATA_READ, result,
                                                        PGSZ);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                     sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                "ReadPage (Actual) Read:  bar iefpFinalAddress="
                                "%08o  readData=%012"PRIo64"\n",
                                cpu.iefpFinalAddress + i, result [i]);
                  }
#ifdef HDBG
                HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address);
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGMRead (cpu.iefpFinalAddress + i, result [i]);
#endif

                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, APU_DATA_READ, result,
                                                        PGSZ);
                // XXX Don't trace Multics idle loop
                if (cpu.PPR.PSR != 061 && cpu.PPR.IC != 0307)
                  {
                    if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                      {
                        for (uint i = 0; i < PGSZ; i ++)
                          sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                     "ReadPage (Actual) Read:  iefpFinalAddress"
                                     "=%08o  readData=%012"PRIo64"\n",
                                     cpu.iefpFinalAddress + i, result [i]);
                      }
#ifdef HDBG
                    HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address);
                    for (uint i = 0; i < PGSZ; i ++)
                      HDBGMRead (cpu.iefpFinalAddress + i, result [i]);
#endif
                  }
              }
            return;
          }
      }
    return ;//SCPE_UNK;
  }

void Write (cpu_state_t *cpu_p, word18 address, word36 data, processor_cycle_type cyctyp)
 {
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode (cpu_p);

    if (cpu.cu.XSF /*get_went_appending ()*/ || (cyctyp != INSTRUCTION_FETCH && cpu.currentInstruction.b29))
        goto B29;


    switch (get_addr_mode (cpu_p))
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                cpu.iefpFinalAddress = get_BAR_address (cpu_p, address);
                set_apu_status (cpu_p, apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (cpup, cyctyp);
                if (cyctyp == OPERAND_STORE && cpu.useZone)
                  {
                    core_write_zone (cpu_p, cpu.iefpFinalAddress, data, __func__);
                  }
                else
                  {
                    core_write (cpu_p, cpu.iefpFinalAddress, data, __func__);
                  }
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Write(Actual) Write:      bar address=%08o "
                           "writeData=%012"PRIo64"\n", address, data);
                HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address);
                HDBGMWrite (cpu.iefpFinalAddress, data);
                return;
              }
            else
              {
                set_apu_status (cpu_p, apuStatus_FABS);
                fauxDoAppendCycle (cpup, cyctyp);
                if (cyctyp == OPERAND_STORE && cpu.useZone)
                  {
                    core_write_zone (cpu_p, address, data, __func__);
                  }
                else
                  {
                    core_write (cpu_p, address, data, __func__);
                  }
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Write(Actual) Write:      abs address=%08o "
                           "writeData=%012"PRIo64"\n",
                           address, data);
                HDBGIEFP (hdbgIEFP_abs_write, 0, address);
                HDBGMWrite (address, data);
                return;
              }
          }

        case APPEND_mode:
          {
B29:
            if (isBAR)
              {
                cpu.TPR.CA = get_BAR_address (cpu_p, address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, cyctyp, & data, 1);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Write(Actual) Write: bar iefpFinalAddress=%08o "
                           "writeData=%012"PRIo64"\n",
                           cpu.iefpFinalAddress, data);
                HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address);
                HDBGMWrite (cpu.iefpFinalAddress, data);
                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, cyctyp, & data, 1);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Write(Actual) Write: iefpFinalAddress=%08o "
                           "writeData=%012"PRIo64"\n",
                           cpu.iefpFinalAddress, data);
                HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address);
                HDBGMWrite (cpu.iefpFinalAddress, data);
                return;
              }
          }
      }

    return ;//SCPE_UNK;
  }


void Write2 (cpu_state_t *cpu_p, word18 address, word36 * data, processor_cycle_type cyctyp)
  {
    cpu.TPR.CA = cpu.iefpFinalAddress = address;
    bool isBAR = get_bar_mode (cpu_p);

    if (cpu.cu.XSF /*get_went_appending ()*/ || (cyctyp != INSTRUCTION_FETCH && cpu.currentInstruction.b29))
      goto B29;

    switch (get_addr_mode (cpu_p))
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                cpu.iefpFinalAddress = get_BAR_address (cpu_p, address);
                set_apu_status (cpu_p, apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (cpup, cyctyp);
                core_write2 (cpu_p, cpu.iefpFinalAddress, data [0], data [1],
                              __func__);
                HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address);
                HDBGMWrite (cpu.iefpFinalAddress, data [0]);
                HDBGMWrite (cpu.iefpFinalAddress+1, data [1]);
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Write2 (Actual) Write:      bar address=%08o "
                           "writeData=%012"PRIo64" %012"PRIo64"\n",
                           address, data [0], data [1]);
              }
            else
              {
                set_apu_status (cpu_p, apuStatus_FABS);
                fauxDoAppendCycle (cpup, cyctyp);
                core_write2 (cpu_p, address, data [0], data [1], __func__);
                HDBGIEFP (hdbgIEFP_abs_write, 0, address);
                HDBGMWrite (address, data [0]);
                HDBGMWrite (address+1, data [1]);
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Write2 (Actual) Write:      abs address=%08o "
                           "writeData=%012"PRIo64" %012"PRIo64"\n",
                           address, data [0], data [1]);
              }
          }
          break;

        case APPEND_mode:
          {
B29:
            if (isBAR)
              {
                cpu.TPR.CA = get_BAR_address (cpu_p, address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, cyctyp, data, 2);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Write2 (Actual) Write: bar iefpFinalAddress=%08o "
                           "writeData=%012"PRIo64" %012"PRIo64"\n",
                           address, data [0], data [1]);
                HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address);
                HDBGMWrite (cpu.iefpFinalAddress, data[0]);
                HDBGMWrite (cpu.iefpFinalAddress+1, data[1]);
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, cyctyp, data, 2);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Write2 (Actual) Write: iefpFinalAddress=%08o "
                           "writeData=%012"PRIo64" %012"PRIo64"\n",
                           address, data [0], data [1]);
                HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address);
                HDBGMWrite (cpu.iefpFinalAddress, data[0]);
                HDBGMWrite (cpu.iefpFinalAddress+1, data[1]);
              }
          }
          break;
      }
    return ;//SCPE_UNK;
  }

#ifdef CWO
void Write1 (cpu_state_t *cpu_p, word18 address, word36 data, bool isAR)
  {
    cpu.TPR.CA = cpu.iefpFinalAddress = address;
    bool isBAR = get_bar_mode (cpu_p);
    if (isAR || cpu.cu.XSF /*get_went_appending ()*/)
      goto B29;
    switch (get_addr_mode (cpu_p))
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
             {
                cpu.iefpFinalAddress = get_BAR_address (cpu_p, address);
                set_apu_status (cpu_p, apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (cpup, APU_DATA_STORE);
                core_write (cpu_p, cpu.iefpFinalAddress, data, __func__);
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Write1(Actual) Write:      bar address=%08o "
                           "writeData=%012"PRIo64"\n",
                           address, data);
                HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address);
                HDBGMWrite (cpu.iefpFinalAddress, data);
                return;
              }
            else
              {
                set_apu_status (cpu_p, apuStatus_FABS);
                fauxDoAppendCycle (cpup, APU_DATA_STORE);
                core_write (cpu_p, address, data, __func__);
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Write1(Actual) Write:      abs address=%08o "
                           "writeData=%012"PRIo64"\n",
                           address, data);
                HDBGIEFP (hdbgIEFP_abs_write, 0, address);
                HDBGMWrite (address, data);
                return;
              }
          }

        case APPEND_mode:
          {
B29:
            if (isBAR)
              {
                cpu.TPR.CA = get_BAR_address (cpu_p, address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, APU_DATA_STORE, & data,
                                                        1);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Write8(Actual) Write: bar iefpFinalAddress="
                           "%08o writeData=%012"PRIo64"\n",
                           cpu.iefpFinalAddress, data);
                HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address);
                HDBGMWrite (cpu.iefpFinalAddress, data);
                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, APU_DATA_STORE, & data,
                                                       1);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Write(Actual) Write: iefpFinalAddress=%08o "
                           "writeData=%012"PRIo64"\n",
                           cpu.iefpFinalAddress, data);
                HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address);
                HDBGMWrite (cpu.iefpFinalAddress, data);
                return;
              }
          }
      }
    return ;//SCPE_UNK;
  }
#endif

void Write8 (cpu_state_t *cpu_p, word18 address, word36 * data, bool isAR)
  {
    address &= paragraphMask; // Round to 8 word boundarryt
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode (cpu_p);

    if (isAR || cpu.cu.XSF /*get_went_appending ()*/)
      goto B29;


    switch (get_addr_mode (cpu_p))
     {
        case ABSOLUTE_mode:
          {
            if (isBAR)
             {
                cpu.iefpFinalAddress = get_BAR_address (cpu_p, address);
                set_apu_status (cpu_p, apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (cpup, APU_DATA_STORE);
                core_writeN (cpu_p, cpu.iefpFinalAddress, data, 8, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "Write8(Actual) Write:      bar address=%08o "
                                 "writeData=%012"PRIo64"\n",
                                 address + i, data [i]);
                  }
#ifdef HDBG
                HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address);
                for (uint i = 0; i < 8; i ++)
                  HDBGMWrite (cpu.iefpFinalAddress + i, data [i]);
#endif
                return;
              }
            else
              {
                set_apu_status (cpu_p, apuStatus_FABS);
                fauxDoAppendCycle (cpup, APU_DATA_STORE);
                core_writeN (cpu_p, address, data, 8, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "Write8(Actual) Write:      abs address=%08o "
                                 "writeData=%012"PRIo64"\n",
                                  address + i, data [i]);
                  }
#ifdef HDBG
                HDBGIEFP (hdbgIEFP_abs_write, 0, address);
                for (uint i = 0; i < 8; i ++)
                  HDBGMWrite (address + i, data [i]);
#endif
                return;
              }
          }

        case APPEND_mode:
          {
B29:
            if (isBAR)
              {
                cpu.TPR.CA = get_BAR_address (cpu_p, address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, APU_DATA_STORE, data,
                                                        8);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                 "Write8(Actual) Write: bar iefpFinalAddress="
                                 "%08o writeData=%012"PRIo64"\n",
                                 cpu.iefpFinalAddress + i, data [i]);
                  }
#ifdef HDBG
                HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address);
                for (uint i = 0; i < 8; i ++)
                  HDBGMWrite (cpu.iefpFinalAddress + i, data [i]);
#endif

                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, APU_DATA_STORE, data,
                                                        8);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                 "Write8(Actual) Write: iefpFinalAddress=%08o "
                                 "writeData=%012"PRIo64"\n",
                                 cpu.iefpFinalAddress + i, data [i]);
                  }
#ifdef HDBG
                HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address);
                for (uint i = 0; i < 8; i ++)
                  HDBGMWrite (cpu.iefpFinalAddress + i, data [i]);
#endif

                return;
              }
          }
      }
    return ;//SCPE_UNK;
  }

void Write16 (cpu_state_t *cpu_p, word18 address, word36 * data)
  {
    address &= paragraphMask; // Round to 8 word boundary
    Write8 (cpu_p, address, data, cpu.currentInstruction.b29);
    Write8 (cpu_p, address + 8, data + 8, cpu.currentInstruction.b29);
    return;
  }

void Write32 (cpu_state_t *cpu_p, word18 address, word36 * data)
  {
//#define paragraphMask 077777770
    //address &= paragraphMask; // Round to 8 word boundary
    address &= 077777740; // Round to 32 word boundary
    Write8 (cpu_p, address, data, cpu.currentInstruction.b29);
    Write8 (cpu_p, address + 8, data + 8, cpu.currentInstruction.b29);
    Write8 (cpu_p, address + 16, data + 16, cpu.currentInstruction.b29);
    Write8 (cpu_p, address + 24, data + 24, cpu.currentInstruction.b29);
    return;
  }

void WritePage (cpu_state_t *cpu_p, word18 address, word36 * data, bool isAR)
  {
    if ((address & PGMK) != 0)
      {
        sim_warn ("WritePage not on boundary %06o\n", address);
      }
    address &= (word18) ~PGMK; // Round to page boundary
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode (cpu_p);

    if (isAR || cpu.cu.XSF /*get_went_appending ()*/)
      goto B29;


    switch (get_addr_mode (cpu_p))
     {
        case ABSOLUTE_mode:
          {
            if (isBAR)
             {
                cpu.iefpFinalAddress = get_BAR_address (cpu_p, address);
                set_apu_status (cpu_p, apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (cpup, APU_DATA_STORE);
                core_writeN (cpu_p, cpu.iefpFinalAddress, data, PGSZ, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "WritePage(Actual) Write:      bar address="
                                 "%08o writeData=%012"PRIo64"\n",
                                 address + i, data [i]);
                  }
#ifdef HDBG
                HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address);
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGMWrite (cpu.iefpFinalAddress + i, data [i]);
#endif
                return;
              }
            else
              {
                set_apu_status (cpu_p, apuStatus_FABS);
                fauxDoAppendCycle (cpup, APU_DATA_STORE);
                core_writeN (cpu_p, address, data, PGSZ, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "WritePage(Actual) Write:      abs address="
                                 "%08o writeData=%012"PRIo64"\n",
                                 address + i, data [i]);
                  }
#ifdef HDBG
                HDBGIEFP (hdbgIEFP_abs_write, 0, address);
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGMWrite (address + i, data [i]);
#endif
                return;
              }
          }

        case APPEND_mode:
          {
B29:
            if (isBAR)
              {
                cpu.TPR.CA = get_BAR_address (cpu_p, address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, APU_DATA_STORE, data,
                                                        PGSZ);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                 "WritePage(Actual) Write: bar "
                                 "iefpFinalAddress=%08o writeData=%012"PRIo64
                                 "\n",
                                 cpu.iefpFinalAddress + i, data [i]);
                  }
#ifdef HDBG
                HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address);
                for (uint i = 0; i < 8; i ++)
                  HDBGMWrite (cpu.iefpFinalAddress + i, data [i]);
#endif
                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (cpu_p, APU_DATA_STORE, data,
                                                        PGSZ);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                 "WritePage(Actual) Write: iefpFinalAddress="
                                 "%08o writeData=%012"PRIo64"\n",
                                 cpu.iefpFinalAddress + i, data [i]);
                  }
#ifdef HDBG
                HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address);
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGMWrite (cpu.iefpFinalAddress + i, data [i]);
#endif

                return;
              }
          }
      }
    return ;//SCPE_UNK;
  }


void ReadIndirect (cpu_state_t *cpu_p)
  {
    if (cpu.TPR.CA & 1) // is odd?
      {
        Read (cpu_p, cpu.TPR.CA, cpu.itxPair, INDIRECT_WORD_FETCH);
        cpu.itxPair[1] = MASK36; // fill with ones for debugging
      }
    else
      {
        Read2 (cpu_p, cpu.TPR.CA, cpu.itxPair, INDIRECT_WORD_FETCH);
      }
    return;
  }
