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
 * Copyright (c) 2021-2022 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
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
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_append.h"
#include "dps8_iefp.h"
#include "dps8_addrmods.h"
#include "dps8_utils.h"

#define DBG_CTR cpu.cycleCnt

// new Read/Write stuff ...

void Read (word18 address, word36 * result, processor_cycle_type cyctyp)
  {
    cpu.TPR.CA = cpu.iefpFinalAddress = address;
    bool isBAR = get_bar_mode ();

    //if (get_went_appending () ||
    if (cpu.cu.XSF || (cyctyp != INSTRUCTION_FETCH && cpu.currentInstruction.b29))
      {
        goto B29;
      }

    switch (get_addr_mode ())
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                set_apu_status (apuStatus_FABS); // XXX maybe...
                cpu.iefpFinalAddress = get_BAR_address (address);
                fauxDoAppendCycle (cyctyp);
#ifdef LOCKLESS
                if (cyctyp == OPERAND_RMW || cyctyp == APU_DATA_RMW)
                  core_read_lock (cpu.iefpFinalAddress, result, __func__);
                else
                  core_read (cpu.iefpFinalAddress, result, __func__);
#else
                core_read (cpu.iefpFinalAddress, result, __func__);
#endif
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Read (Actual) Read:       bar address=%08o  "
                           "readData=%012"PRIo64"\n", address, *result);
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "Read ABS BAR");
                HDBGMRead (cpu.iefpFinalAddress, * result, "Read ABS BAR");
#endif
                return;
              }
            else
              {
                set_apu_status (apuStatus_FABS);
                fauxDoAppendCycle (cyctyp);
#ifdef LOCKLESS
                if (cyctyp == OPERAND_RMW || cyctyp == APU_DATA_RMW)
                  core_read_lock (address, result, __func__);
                else
                  core_read (address, result, __func__);
#else
                core_read (address, result, __func__);
#endif
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Read (Actual) Read:       abs address=%08o  "
                           "readData=%012"PRIo64"\n", address, *result);
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_abs_read, 0, address, "Read ABS");
                HDBGMRead (address, * result, "Read ABS");
#endif
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
                cpu.iefpFinalAddress = do_append_cycle (cyctyp, result, 1);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Read (Actual) Read:  bar iefpFinalAddress=%08o  "
                           "readData=%012"PRIo64"\n",
                           cpu.iefpFinalAddress, * result);
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "Read BAR");
                HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read BAR");
#endif

                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (cyctyp, result, 1);
                // XXX Don't trace Multics idle loop
                if (cpu.PPR.PSR != 061 && cpu.PPR.IC != 0307)
                  {
                    sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                               "Read (Actual) Read:  iefpFinalAddress=%08o  "
                               "readData=%012"PRIo64"\n",
                               cpu.iefpFinalAddress, * result);
#ifdef TESTING
                    HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "Read");
                    HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read");
#endif
                  }
              }
            return;
          }
      }
    return ;//SCPE_UNK;
  }

void Read2 (word18 address, word36 * result, processor_cycle_type cyctyp)
  {
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode ();

    //if (get_went_appending () ||
    if (cpu.cu.XSF || (cyctyp != INSTRUCTION_FETCH && cpu.currentInstruction.b29) ||
        cyctyp == RTCD_OPERAND_FETCH) // ISOLTS-886
           // Another option would be to set_went_appending in ReadRTCDOp
      {
        goto B29;
      }

    switch (get_addr_mode ())
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                set_apu_status (apuStatus_FABS); // XXX maybe...
                cpu.iefpFinalAddress = get_BAR_address (address);

                fauxDoAppendCycle (cyctyp);
                core_read2 (cpu.iefpFinalAddress, result + 0, result + 1,
                            __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 2; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                  "Read2 (Actual) Read:       bar address=%08o"
                                  "  readData=%012"PRIo64"\n",
                                  address + i, result [i]);
                  }
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "Read2 ABBR");
                HDBGMRead (cpu.iefpFinalAddress, * result, "Read2 ABBR evn");
                HDBGMRead (cpu.iefpFinalAddress+1, * (result+1), "Read2 ABBR odd");
#endif
                return;
              }
            else
              {
                set_apu_status (apuStatus_FABS);
                fauxDoAppendCycle (cyctyp);
                core_read2 (address, result + 0, result + 1, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 2; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "Read2 (Actual) Read:       abs address=%08o"
                                 "  readData=%012"PRIo64"\n",
                                 address + i, result [i]);
                  }
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_abs_read, 0, address, "Read2 AB");
                HDBGMRead (cpu.iefpFinalAddress, * result, "Read2 AB evn");
                HDBGMRead (cpu.iefpFinalAddress+1, * (result+1), "Read2 AB odd");
#endif
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
                cpu.iefpFinalAddress = do_append_cycle (cyctyp, result, 2);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 2; i ++)
                     sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                "Read2 (Actual) Read:  bar iefpFinalAddress="
                                "%08o  readData=%012"PRIo64"\n",
                                cpu.iefpFinalAddress + i, result [i]);
                  }
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "Read2 BR");
                HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read2 BR evn");
                HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, * (result+1), "Read2 BR odd");
#endif
                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (cyctyp, result, 2);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 2; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                 "Read2 (Actual) Read:  iefpFinalAddress=%08o"
                                 "  readData=%012"PRIo64"\n",
                                 cpu.iefpFinalAddress + i, result [i]);
                  }
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    if (cyctyp == OPERAND_READ)
                      {
                        for (uint i = 0; i < 2; i ++)
                          sim_debug (DBG_FINAL, & cpu_dev,
                                     "Read2 (Actual) Read:  iefpFinalAddress=%08o"
                                     "  readData=%012"PRIo64"\n",
                                     cpu.iefpFinalAddress + i, result [i]);
                      }
                  }
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "Read2");
                HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, * result, "Read2 evn");
                HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, * (result+1), "Read2 odd");
#endif
              }
            return;
          }
      }
    return ;//SCPE_UNK;
  }

void Read8 (word18 address, word36 * result, bool isAR)
  {
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode ();

    if (isAR || cpu.cu.XSF /*get_went_appending ()*/)
      {
        goto B29;
      }

    switch (get_addr_mode ())
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                set_apu_status (apuStatus_FABS); // XXX maybe...
                cpu.iefpFinalAddress = get_BAR_address (address);

                fauxDoAppendCycle (APU_DATA_READ);
                core_readN (cpu.iefpFinalAddress, result, 8, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "Read8 (Actual) Read:       bar address=%08o"
                                 "  readData=%012"PRIo64"\n",
                                 address + i, result [i]);
                  }
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "Read8 ABBR");
                for (uint i = 0; i < 8; i ++)
                  HDBGMRead (cpu.iefpFinalAddress + i, result [i], "Read8 ABBR");
#endif
                return;
              }
            else
              {
                set_apu_status (apuStatus_FABS);
                fauxDoAppendCycle (APU_DATA_READ);
                core_readN (address, result, 8, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "Read8 (Actual) Read:       abs address=%08o"
                                 "  readData=%012"PRIo64"\n",
                                 address + i, result [i]);
                  }
#ifdef TESTING
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
                cpu.TPR.CA = get_BAR_address (address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                cpu.iefpFinalAddress = do_append_cycle (APU_DATA_READ, result,
                                                        8);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                     sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                "Read8 (Actual) Read:  bar iefpFinalAddress="
                                "%08o  readData=%012"PRIo64"\n",
                                cpu.iefpFinalAddress + i, result [i]);
                  }
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "Read8 BAR");
                for (uint i = 0; i < 8; i ++)
                  HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + i, cpu.iefpFinalAddress + i, result[i], "Read8 BAR");
#endif
                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (APU_DATA_READ, result,
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
#ifdef TESTING
                    HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "Read8");
                    for (uint i = 0; i < 8; i ++)
                      HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + i, cpu.iefpFinalAddress + i, result [i], "Read8");
#endif
                  }
              }
            return;
          }
      }
    return ;//SCPE_UNK;
  }

void Read16 (word18 address, word36 * result)
  {
    address &= paragraphMask; // Round to 8 word boundary
    Read8 (address, result, cpu.currentInstruction.b29);
    Read8 (address + 8, result + 8, cpu.currentInstruction.b29);
    return;
  }

void ReadPage (word18 address, word36 * result, bool isAR)
  {
    if ((address & PGMK) != 0)
      {
        sim_warn ("ReadPage not on boundary %06o\n", address);
      }
    address &= (word18) ~PGMK; // Round to page boundary
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode ();

    if (isAR || cpu.cu.XSF /*get_went_appending ()*/)
      {
        goto B29;
      }

    switch (get_addr_mode ())
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                set_apu_status (apuStatus_FABS); // XXX maybe...
                cpu.iefpFinalAddress = get_BAR_address (address);

                fauxDoAppendCycle (APU_DATA_READ);
                core_readN (cpu.iefpFinalAddress, result, PGSZ, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "ReadPage (Actual) Read:       bar address="
                                 "%08o  readData=%012"PRIo64"\n",
                                  address + i, result [i]);
                  }
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_abs_bar_read, 0, address, "ReadPage AB");
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGMRead (cpu.iefpFinalAddress + i, result [i], "ReadPage AB");
#endif
                return;
              }
            else
              {
                set_apu_status (apuStatus_FABS);
                fauxDoAppendCycle (APU_DATA_READ);
                core_readN (address, result, PGSZ, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "ReadPage (Actual) Read:       abs address="
                                 "%08o  readData=%012"PRIo64"\n",
                                 address + i, result [i]);
                  }
#ifdef TESTING
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
                cpu.TPR.CA = get_BAR_address (address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                cpu.iefpFinalAddress = do_append_cycle (APU_DATA_READ, result,
                                                        PGSZ);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                     sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                "ReadPage (Actual) Read:  bar iefpFinalAddress="
                                "%08o  readData=%012"PRIo64"\n",
                                cpu.iefpFinalAddress + i, result [i]);
                  }
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_bar_read, cpu.TPR.TSR, address, "ReadPage B");
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + i, cpu.iefpFinalAddress + i, result [i], "ReadPage B");
#endif

                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (APU_DATA_READ, result,
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
#ifdef TESTING
                    HDBGIEFP (hdbgIEFP_read, cpu.TPR.TSR, address, "ReadPage");
                    for (uint i = 0; i < PGSZ; i ++)
                      HDBGAPURead (cpu.TPR.TSR, cpu.TPR.CA + i, cpu.iefpFinalAddress + i, result [i], "ReadPage");
#endif
                  }
              }
            return;
          }
      }
    return ;//SCPE_UNK;
  }

void Write (word18 address, word36 data, processor_cycle_type cyctyp)
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
                if (cyctyp == OPERAND_STORE && cpu.useZone)
                  {
                    core_write_zone (cpu.iefpFinalAddress, data, __func__);
                  }
                else
                  {
                    core_write (cpu.iefpFinalAddress, data, __func__);
                  }
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Write(Actual) Write:      bar address=%08o "
                           "writeData=%012"PRIo64"\n", address, data);
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address, "Write ABBR");
                HDBGMWrite (cpu.iefpFinalAddress, data, "Write ABBR");
#endif
                return;
              }
            else
              {
                set_apu_status (apuStatus_FABS);
                fauxDoAppendCycle (cyctyp);
                if (cyctyp == OPERAND_STORE && cpu.useZone)
                  {
                    core_write_zone (address, data, __func__);
                  }
                else
                  {
                    core_write (address, data, __func__);
                  }
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Write(Actual) Write:      abs address=%08o "
                           "writeData=%012"PRIo64"\n",
                           address, data);
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_abs_write, 0, address, "Write AB");
                HDBGMWrite (address, data, "Write AB");
#endif
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
                cpu.iefpFinalAddress = do_append_cycle (cyctyp, & data, 1);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Write(Actual) Write: bar iefpFinalAddress=%08o "
                           "writeData=%012"PRIo64"\n",
                           cpu.iefpFinalAddress, data);
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address, "Write BR");
                HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data, "Write BR");
#endif
                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (cyctyp, & data, 1);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Write(Actual) Write: iefpFinalAddress=%08o "
                           "writeData=%012"PRIo64"\n",
                           cpu.iefpFinalAddress, data);
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address, "Write");
                HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data, "Write");
#endif
                return;
              }
          }
      }

    return ;//SCPE_UNK;
  }

void Write2 (word18 address, word36 * data, processor_cycle_type cyctyp)
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
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address, "Write2 ABBR");
                HDBGMWrite (cpu.iefpFinalAddress, data [0], "Write2 ABBR evn");
                HDBGMWrite (cpu.iefpFinalAddress+1, data [1], "Write2 ABBR odd");
#endif
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Write2 (Actual) Write:      bar address=%08o "
                           "writeData=%012"PRIo64" %012"PRIo64"\n",
                           address, data [0], data [1]);
              }
            else
              {
                set_apu_status (apuStatus_FABS);
                fauxDoAppendCycle (cyctyp);
                core_write2 (address, data [0], data [1], __func__);
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Write2 (Actual) Write:      abs address=%08o "
                           "writeData=%012"PRIo64" %012"PRIo64"\n",
                           address, data [0], data [1]);
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_abs_write, 0, address, "Write2 AB");
                HDBGMWrite (address, data [0], "Write2 AB evn");
                HDBGMWrite (address+1, data [1], "Write2 AB odd");
#endif
              }
          }
          break;

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
                           "writeData=%012"PRIo64" %012"PRIo64"\n",
                           address, data [0], data [1]);
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address, "Write2 BR");
                HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data[0], "Write2 BR evn");
                HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, data[1], "Write2 BR odd");
#endif
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (cyctyp, data, 2);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Write2 (Actual) Write: iefpFinalAddress=%08o "
                           "writeData=%012"PRIo64" %012"PRIo64"\n",
                           address, data [0], data [1]);
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address, "Write2");
                HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data[0], "Write2 evn");
                HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA + 1, cpu.iefpFinalAddress + 1, data[1], "Write2 odd");
#endif
              }
          }
          break;
      }
    return ;//SCPE_UNK;
  }

void Write1 (word18 address, word36 data, bool isAR)
  {
    cpu.TPR.CA = cpu.iefpFinalAddress = address;
    bool isBAR = get_bar_mode ();
    if (isAR || cpu.cu.XSF /*get_went_appending ()*/)
      goto B29;
    switch (get_addr_mode ())
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
             {
                cpu.iefpFinalAddress = get_BAR_address (address);
                set_apu_status (apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (APU_DATA_STORE);
                core_write (cpu.iefpFinalAddress, data, __func__);
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Write1(Actual) Write:      bar address=%08o "
                           "writeData=%012"PRIo64"\n",
                           address, data);
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address, "Write1 ABBR");
                HDBGMWrite (cpu.iefpFinalAddress, data, "Write1 ABBR");
#endif
                return;
              }
            else
              {
                set_apu_status (apuStatus_FABS);
                fauxDoAppendCycle (APU_DATA_STORE);
                core_write (address, data, __func__);
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Write1(Actual) Write:      abs address=%08o "
                           "writeData=%012"PRIo64"\n",
                           address, data);
#ifdef TESTING
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
                cpu.TPR.CA = get_BAR_address (address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                cpu.iefpFinalAddress = do_append_cycle (APU_DATA_STORE, & data,
                                                        1);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Write8(Actual) Write: bar iefpFinalAddress="
                           "%08o writeData=%012"PRIo64"\n",
                           cpu.iefpFinalAddress, data);
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address, "Write1 BR");
                HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data, "Write1 BR");
#endif
                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (APU_DATA_STORE, & data,
                                                       1);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Write(Actual) Write: iefpFinalAddress=%08o "
                           "writeData=%012"PRIo64"\n",
                           cpu.iefpFinalAddress, data);
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address, "Write1");
                HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA, cpu.iefpFinalAddress, data, "Write1");
#endif
                return;
              }
          }
      }
    return ;//SCPE_UNK;
  }

void Write8 (word18 address, word36 * data, bool isAR)
  {
    address &= paragraphMask; // Round to 8 word boundarryt
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode ();

    if (isAR || cpu.cu.XSF /*get_went_appending ()*/)
      goto B29;

    switch (get_addr_mode ())
     {
        case ABSOLUTE_mode:
          {
            if (isBAR)
             {
                cpu.iefpFinalAddress = get_BAR_address (address);
                set_apu_status (apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (APU_DATA_STORE);
                core_writeN (cpu.iefpFinalAddress, data, 8, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "Write8(Actual) Write:      bar address=%08o "
                                 "writeData=%012"PRIo64"\n",
                                 address + i, data [i]);
                  }
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address, "Write8 ABBR");
                for (uint i = 0; i < 8; i ++)
                  HDBGMWrite (cpu.iefpFinalAddress + i, data [i], "Write8 ABBR");
#endif
                return;
              }
            else
              {
                set_apu_status (apuStatus_FABS);
                fauxDoAppendCycle (APU_DATA_STORE);
                core_writeN (address, data, 8, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "Write8(Actual) Write:      abs address=%08o "
                                 "writeData=%012"PRIo64"\n",
                                  address + i, data [i]);
                  }
#ifdef TESTING
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
                cpu.TPR.CA = get_BAR_address (address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                cpu.iefpFinalAddress = do_append_cycle (APU_DATA_STORE, data,
                                                        8);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                 "Write8(Actual) Write: bar iefpFinalAddress="
                                 "%08o writeData=%012"PRIo64"\n",
                                 cpu.iefpFinalAddress + i, data [i]);
                  }
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address, "Write8 BR");
                for (uint i = 0; i < 8; i ++)
                  HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA + i, cpu.iefpFinalAddress + i, data [i], "Write8 BR");
#endif

                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (APU_DATA_STORE, data,
                                                        8);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                 "Write8(Actual) Write: iefpFinalAddress=%08o "
                                 "writeData=%012"PRIo64"\n",
                                 cpu.iefpFinalAddress + i, data [i]);
                  }
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address, "Write8");
                for (uint i = 0; i < 8; i ++)
                  HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA + i, cpu.iefpFinalAddress + i, data [i], "Write8");
#endif

                return;
              }
          }
      }
    return ;//SCPE_UNK;
  }

void Write16 (word18 address, word36 * data)
  {
    address &= paragraphMask; // Round to 8 word boundary
    Write8 (address, data, cpu.currentInstruction.b29);
    Write8 (address + 8, data + 8, cpu.currentInstruction.b29);
    return;
  }

void Write32 (word18 address, word36 * data)
  {
//#define paragraphMask 077777770
    //address &= paragraphMask; // Round to 8 word boundary
    address &= 077777740; // Round to 32 word boundary
    Write8 (address, data, cpu.currentInstruction.b29);
    Write8 (address + 8, data + 8, cpu.currentInstruction.b29);
    Write8 (address + 16, data + 16, cpu.currentInstruction.b29);
    Write8 (address + 24, data + 24, cpu.currentInstruction.b29);
    return;
  }

void WritePage (word18 address, word36 * data, bool isAR)
  {
    if ((address & PGMK) != 0)
      {
        sim_warn ("WritePage not on boundary %06o\n", address);
      }
    address &= (word18) ~PGMK; // Round to page boundary
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode ();

    if (isAR || cpu.cu.XSF /*get_went_appending ()*/)
      goto B29;

    switch (get_addr_mode ())
     {
        case ABSOLUTE_mode:
          {
            if (isBAR)
             {
                cpu.iefpFinalAddress = get_BAR_address (address);
                set_apu_status (apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (APU_DATA_STORE);
                core_writeN (cpu.iefpFinalAddress, data, PGSZ, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "WritePage(Actual) Write:      bar address="
                                 "%08o writeData=%012"PRIo64"\n",
                                 address + i, data [i]);
                  }
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_abs_bar_write, 0, address, "WritePage ABBR");
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGMWrite (cpu.iefpFinalAddress + i, data [i], "WritePage ABBR");
#endif
                return;
              }
            else
              {
                set_apu_status (apuStatus_FABS);
                fauxDoAppendCycle (APU_DATA_STORE);
                core_writeN (address, data, PGSZ, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev,
                                 "WritePage(Actual) Write:      abs address="
                                 "%08o writeData=%012"PRIo64"\n",
                                 address + i, data [i]);
                  }
#ifdef TESTING
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
                cpu.TPR.CA = get_BAR_address (address);
                cpu.TPR.TSR = cpu.PPR.PSR;
                cpu.TPR.TRR = cpu.PPR.PRR;
                cpu.iefpFinalAddress = do_append_cycle (APU_DATA_STORE, data,
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
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_bar_write, cpu.TPR.TSR, address, "WritePage BR");
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA + i, cpu.iefpFinalAddress + i, data [i], "WritePage BR");
#endif
                return;
              }
            else
              {
                cpu.iefpFinalAddress = do_append_cycle (APU_DATA_STORE, data,
                                                        PGSZ);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                                 "WritePage(Actual) Write: iefpFinalAddress="
                                 "%08o writeData=%012"PRIo64"\n",
                                 cpu.iefpFinalAddress + i, data [i]);
                  }
#ifdef TESTING
                HDBGIEFP (hdbgIEFP_write, cpu.TPR.TSR, address, "WritePage");
                for (uint i = 0; i < PGSZ; i ++)
                  HDBGAPUWrite (cpu.TPR.TSR, cpu.TPR.CA + i, cpu.iefpFinalAddress + i, data [i], "WritePage");
#endif

                return;
              }
          }
      }
    return ;//SCPE_UNK;
  }

void ReadIndirect (void)
  {
    if (cpu.TPR.CA & 1) // is odd?
      {
        Read (cpu.TPR.CA, cpu.itxPair, INDIRECT_WORD_FETCH);
        cpu.itxPair[1] = MASK36; // fill with ones for debugging
      }
    else
      {
        Read2 (cpu.TPR.CA, cpu.itxPair, INDIRECT_WORD_FETCH);
      }
    return;
  }
