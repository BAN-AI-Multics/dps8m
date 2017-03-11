/*
 Copyright (c) 2007-2013 Michael Mondy
 Copyright 2012-2016 by Harry Reed
 Copyright 2013-2016 by Charles Anthony
 Copyright 2015 by Eric Swenson

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

/**
 * \file dps8_iefp.c
 * \project dps8
 * \date 01/11/14
 * \copyright Copyright (c) 2014 Harry Reed. All rights reserved.
 */

#include "dps8.h"
#include "dps8_bar.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_cpu.h"
#include "dps8_append.h"
#include "dps8_iefp.h"
#include "dps8_utils.h"
#include "dps8_addrmods.h"
#include "hdbg.h"

// new Read/Write stuff ...

t_stat Read (word18 address, word36 * result, _processor_cycle_type cyctyp)
  {
    //cpu.iefpFinalAddress = address;
    cpu.TPR.CA = cpu.iefpFinalAddress = address;
    bool isBAR = get_bar_mode ();

    if (cpu.cu.TSN_VALID [0] || get_went_appending ())
      {
        //if (isBAR)
          //sim_printf ("went appending fired in BAR mode\n"); 
        goto B29;
      }

    switch (get_addr_mode())
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                setAPUStatus (apuStatus_FABS); // XXX maybe...
                cpu.iefpFinalAddress = getBARaddress (address);
                fauxDoAppendCycle (cyctyp);
                core_read(cpu . iefpFinalAddress, result, __func__);
                sim_debug(DBG_FINAL, &cpu_dev, "Read (Actual) Read:       bar address=%08o  readData=%012"PRIo64"\n", address, *result);
#ifdef HDBG
                hdbgMRead (cpu . iefpFinalAddress, * result);
#endif
                return SCPE_OK;
            } else {
                setAPUStatus (apuStatus_FABS);
                fauxDoAppendCycle (cyctyp);
                core_read(address, result, __func__);
                sim_debug(DBG_FINAL, &cpu_dev, "Read (Actual) Read:       abs address=%08o  readData=%012"PRIo64"\n", address, *result);
#ifdef HDBG
                hdbgMRead (address, * result);
#endif
                return SCPE_OK;
            }
        }

        case APPEND_mode:
        {
B29:;
            if (isBAR)
            {
                //word18 barAddress = getBARaddress (address);
                //cpu.iefpFinalAddress = doAppendCycle(barAddress, cyctyp, result, 1);
                cpu.TPR.CA = getBARaddress (address);
                cpu.iefpFinalAddress = doAppendCycle(cyctyp, result, 1);
                sim_debug (DBG_APPENDING | DBG_FINAL, &cpu_dev, "Read (Actual) Read:  bar iefpFinalAddress=%08o  readData=%012"PRIo64"\n", cpu . iefpFinalAddress, *result);
#ifdef HDBG
                hdbgMRead (cpu . iefpFinalAddress, * result);
#endif

                return SCPE_OK;
            } else {
                //cpu . iefpFinalAddress = doAppendCycle(address, cyctyp, result, 1);
                cpu . iefpFinalAddress = doAppendCycle (cyctyp, result, 1);
                // XXX Don't trace Multics idle loop
                if (cpu . PPR.PSR != 061 && cpu . PPR.IC != 0307)
                  {
                    sim_debug(DBG_APPENDING | DBG_FINAL, &cpu_dev, "Read (Actual) Read:  iefpFinalAddress=%08o  readData=%012"PRIo64"\n", cpu . iefpFinalAddress, *result);
#ifdef HDBG
                    hdbgMRead (cpu . iefpFinalAddress, * result);
#endif
                  }
            }
            return SCPE_OK;
        }
    }
    return SCPE_UNK;
}

t_stat Read2 (word18 address, word36 * result, _processor_cycle_type cyctyp)
  {
    //cpu.iefpFinalAddress = address;
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode ();

    if (cpu.cu.TSN_VALID [0] || get_went_appending ())
      {
        goto B29;
      }

    switch (get_addr_mode ())
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                setAPUStatus (apuStatus_FABS); // XXX maybe...
                cpu.iefpFinalAddress = getBARaddress (address);
        
                fauxDoAppendCycle (cyctyp);
                core_read2 (cpu.iefpFinalAddress, result + 0, result + 1, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 2; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev, "Read2 (Actual) Read:       bar address=%08o  readData=%012"PRIo64"\n", address+i, result [i]);
                  }
                return SCPE_OK;
              }
            else
              {
                setAPUStatus (apuStatus_FABS);
                fauxDoAppendCycle (cyctyp);
                core_read2 (address, result + 0, result + 1, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 2; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev, "Read2 (Actual) Read:       abs address=%08o  readData=%012"PRIo64"\n", address+i, result[i]);
                  }
                return SCPE_OK;
              }
          }

        case APPEND_mode:
          {
B29:;
            if (isBAR)
              {
                //word18 barAddress = getBARaddress (address);
                //cpu.iefpFinalAddress = doAppendCycle (barAddress, cyctyp, result, 2);
                cpu.TPR.CA = getBARaddress (address);
                cpu.iefpFinalAddress = doAppendCycle (cyctyp, result, 2);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 2; i ++)
                     sim_debug (DBG_APPENDING | DBG_FINAL, &cpu_dev, "Read2 (Actual) Read:  bar iefpFinalAddress=%08o  readData=%012"PRIo64"\n", cpu.iefpFinalAddress + i, result [i]);
                  }
                return SCPE_OK;
              }
            else
              {
                //cpu.iefpFinalAddress = doAppendCycle (address, cyctyp, result, 2);
                cpu.iefpFinalAddress = doAppendCycle (cyctyp, result, 2);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 2; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev, "Read2 (Actual) Read:  iefpFinalAddress=%08o  readData=%012"PRIo64"\n", cpu.iefpFinalAddress + i, result [i]);
                 }
              }
            return SCPE_OK;
          }
      }
    return SCPE_UNK;
  }

t_stat Read8 (word18 address, word36 * result)
  {
    //cpu.iefpFinalAddress = address;
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode ();

    if (cpu.cu.TSN_VALID [0] || get_went_appending ())
      {
        goto B29;
      }

    switch (get_addr_mode ())
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                setAPUStatus (apuStatus_FABS); // XXX maybe...
                cpu.iefpFinalAddress = getBARaddress (address);
        
                fauxDoAppendCycle (APU_DATA_READ);
                core_readN (cpu.iefpFinalAddress, result, 8, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev, "Read8 (Actual) Read:       bar address=%08o  readData=%012"PRIo64"\n", address+i, result [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < 8; i ++)
                  hdbgMRead (cpu.iefpFinalAddress + i, result [i]);
#endif
                return SCPE_OK;
              }
            else
              {
                setAPUStatus (apuStatus_FABS);
                fauxDoAppendCycle (APU_DATA_READ);
                core_readN (address, result, 8, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev, "Read8 (Actual) Read:       abs address=%08o  readData=%012"PRIo64"\n", address+i, result[i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < 8; i ++)
                  hdbgMRead (address + i, result [i]);
#endif
                return SCPE_OK;
              }
          }

        case APPEND_mode:
          {
B29:;
            if (isBAR)
              {
                //word18 barAddress = getBARaddress (address);
                //cpu.iefpFinalAddress = doAppendCycle (barAddress, APU_DATA_READ, result, 8);
                cpu.TPR.CA = getBARaddress (address);
                cpu.iefpFinalAddress = doAppendCycle (APU_DATA_READ, result, 8);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                     sim_debug (DBG_APPENDING | DBG_FINAL, &cpu_dev, "Read8 (Actual) Read:  bar iefpFinalAddress=%08o  readData=%012"PRIo64"\n", cpu.iefpFinalAddress + i, result [i]);
                  }

                return SCPE_OK;
              }
            else
              {
                //cpu.iefpFinalAddress = doAppendCycle (address, APU_DATA_READ, result, 8);
                cpu.iefpFinalAddress = doAppendCycle (APU_DATA_READ, result, 8);
                // XXX Don't trace Multics idle loop
                if (cpu.PPR.PSR != 061 && cpu.PPR.IC != 0307)
                  {
                    if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                      {
                        for (uint i = 0; i < 8; i ++)
                          sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev, "Read8 (Actual) Read:  iefpFinalAddress=%08o  readData=%012"PRIo64"\n", cpu.iefpFinalAddress + i, result [i]);
                      }
#ifdef HDBG
                    for (uint i = 0; i < 8; i ++)
                      hdbgMRead (cpu.iefpFinalAddress + i, result [i]);
#endif
                  }
              }
            return SCPE_OK;
          }
      }
    return SCPE_UNK;
  }

t_stat Read16 (word18 address, word36 * result)
  {
    //cpu.iefpFinalAddress = address;
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode ();

    if (cpu.cu.TSN_VALID [0] || get_went_appending ())
      {
        goto B29;
      }

    switch (get_addr_mode ())
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                setAPUStatus (apuStatus_FABS); // XXX maybe...
                cpu.iefpFinalAddress = getBARaddress (address);
        
                fauxDoAppendCycle (APU_DATA_READ);
                core_readN (cpu.iefpFinalAddress, result, 16, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 16; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev, "Read16 (Actual) Read:       bar address=%08o  readData=%012"PRIo64"\n", address+i, result [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < 16; i ++)
                  hdbgMRead (cpu.iefpFinalAddress + i, result [i]);
#endif
                return SCPE_OK;
              }
            else
              {
                setAPUStatus (apuStatus_FABS);
                fauxDoAppendCycle (APU_DATA_READ);
                core_readN (address, result, 16, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 16; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev, "Read16 (Actual) Read:       abs address=%08o  readData=%012"PRIo64"\n", address+i, result[i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < 16; i ++)
                  hdbgMRead (address + i, result [i]);
#endif
                return SCPE_OK;
              }
          }

        case APPEND_mode:
          {
B29:;
            if (isBAR)
              {
                //word18 barAddress = getBARaddress (address);
                //cpu.iefpFinalAddress = doAppendCycle (barAddress, APU_DATA_READ, result, 16);
                cpu.TPR.CA = getBARaddress (address);
                cpu.iefpFinalAddress = doAppendCycle (APU_DATA_READ, result, 16);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 16; i ++)
                     sim_debug (DBG_APPENDING | DBG_FINAL, &cpu_dev, "Read16 (Actual) Read:  bar iefpFinalAddress=%08o  readData=%012"PRIo64"\n", cpu.iefpFinalAddress + i, result [i]);
                  }

                return SCPE_OK;
              }
            else
              {
                //cpu.iefpFinalAddress = doAppendCycle (address, APU_DATA_READ, result, 16);
                cpu.iefpFinalAddress = doAppendCycle (APU_DATA_READ, result, 16);
                // XXX Don't trace Multics idle loop
                if (cpu.PPR.PSR != 061 && cpu.PPR.IC != 0307)
                  {
                    if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                      {
                        for (uint i = 0; i < 16; i ++)
                          sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev, "Read16 (Actual) Read:  iefpFinalAddress=%08o  readData=%012"PRIo64"\n", cpu.iefpFinalAddress + i, result [i]);
                      }
#ifdef HDBG
                    for (uint i = 0; i < 16; i ++)
                      hdbgMRead (cpu.iefpFinalAddress + i, result [i]);
#endif
                  }
              }
            return SCPE_OK;
          }
      }
    return SCPE_UNK;
  }

t_stat ReadPage (word18 address, word36 * result)
  {
    if ((address & PGMK) != 0)
      {
        sim_err ("ReadPage not on boundary %06o\n", address);
      }
    address &= (word18) ~PGMK; // Round to page boundary
    //cpu.iefpFinalAddress = address;
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    // The following is wrong; we do need get_bar_mode for when the SCU 
    // instruction in the fault pair does writeOperands(); 

    //--- // We don't need get_bar_mode here as this code won't be
    //--- // used when reading fault pairs
    //--- bool isBAR = TST_I_NBAR ? false : true;

    // Correct version
    bool isBAR = get_bar_mode ();

    if (cpu.cu.TSN_VALID [0] || get_went_appending ())
      {
        goto B29;
      }

    switch (get_addr_mode ())
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                setAPUStatus (apuStatus_FABS); // XXX maybe...
                cpu.iefpFinalAddress = getBARaddress (address);
        
                fauxDoAppendCycle (APU_DATA_READ);
                core_readN (cpu.iefpFinalAddress, result, PGSZ, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev, "ReadPage (Actual) Read:       bar address=%08o  readData=%012"PRIo64"\n", address + i, result [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < PGSZ; i ++)
                  hdbgMRead (cpu.iefpFinalAddress + i, result [i]);
#endif
                return SCPE_OK;
              }
            else
              {
                setAPUStatus (apuStatus_FABS);
                fauxDoAppendCycle (APU_DATA_READ);
                core_readN (address, result, PGSZ, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev, "ReadPage (Actual) Read:       abs address=%08o  readData=%012"PRIo64"\n", address+i, result[i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < PGSZ; i ++)
                  hdbgMRead (address + i, result [i]);
#endif
                return SCPE_OK;
              }
          }

        case APPEND_mode:
          {
B29:;
            if (isBAR)
              {
                //word18 barAddress = getBARaddress (address);
                //cpu.iefpFinalAddress = doAppendCycle (barAddress, APU_DATA_READ, result, PGSZ);
                cpu.TPR.CA = getBARaddress (address);
                cpu.iefpFinalAddress = doAppendCycle (APU_DATA_READ, result, PGSZ);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                     sim_debug (DBG_APPENDING | DBG_FINAL, &cpu_dev, "ReadPage (Actual) Read:  bar iefpFinalAddress=%08o  readData=%012"PRIo64"\n", cpu.iefpFinalAddress + i, result [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < PGSZ; i ++)
                  hdbgMRead (cpu.iefpFinalAddress + i, result [i]);
#endif

                return SCPE_OK;
              }
            else
              {
                //cpu.iefpFinalAddress = doAppendCycle (address, APU_DATA_READ, result, PGSZ);
                cpu.iefpFinalAddress = doAppendCycle (APU_DATA_READ, result, PGSZ);
                // XXX Don't trace Multics idle loop
                if (cpu.PPR.PSR != 061 && cpu.PPR.IC != 0307)
                  {
                    if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                      {
                        for (uint i = 0; i < PGSZ; i ++)
                          sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev, "ReadPage (Actual) Read:  iefpFinalAddress=%08o  readData=%012"PRIo64"\n", cpu.iefpFinalAddress + i, result [i]);
                      }
#ifdef HDBG
                    for (uint i = 0; i < PGSZ; i ++)
                      hdbgMRead (cpu.iefpFinalAddress + i, result [i]);
#endif
                  }
              }
            return SCPE_OK;
          }
      }
    return SCPE_UNK;
  }

t_stat Write(word18 address, word36 data, _processor_cycle_type cyctyp)
{
    //word24 finalAddress;
    //cpu . iefpFinalAddress = address;
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    // The following is wrong; we do need get_bar_mode for when the SCU 
    // instruction in the fault pair does writeOperands(); 

    //--- // We don't need get_bar_mode here as this code won't be
    //--- // used when reading fault pairs
    //--- bool isBAR = TST_I_NBAR ? false : true;

    // Correct version
    bool isBAR = get_bar_mode ();

    if (cpu.cu.TSN_VALID [0] || get_went_appending ())
        goto B29;
    
    
    switch (get_addr_mode())
    {
        case ABSOLUTE_mode:
        {
            if (isBAR)
            {
                cpu . iefpFinalAddress = getBARaddress(address);
                setAPUStatus (apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (cyctyp);
                core_write(cpu . iefpFinalAddress, data, __func__);
                sim_debug(DBG_FINAL, &cpu_dev, "Write(Actual) Write:      bar address=%08o writeData=%012"PRIo64"\n", address, data);
#ifdef HDBG
                hdbgMWrite (cpu . iefpFinalAddress, data);
#endif
                return SCPE_OK;
            } else {
                setAPUStatus (apuStatus_FABS);
                fauxDoAppendCycle (cyctyp);
                core_write(address, data, __func__);
                sim_debug(DBG_FINAL, &cpu_dev, "Write(Actual) Write:      abs address=%08o writeData=%012"PRIo64"\n", address, data);
#ifdef HDBG
                hdbgMWrite (address, data);
#endif
                return SCPE_OK;
            }
        }

        case APPEND_mode:
        {
B29:
            if (isBAR)
            {
                //word18 barAddress = getBARaddress (address);
                //cpu . iefpFinalAddress = doAppendCycle(barAddress, cyctyp, & data, 1);
                cpu.TPR.CA = getBARaddress (address);
                cpu . iefpFinalAddress = doAppendCycle(cyctyp, & data, 1);
                sim_debug(DBG_APPENDING | DBG_FINAL, &cpu_dev, "Write(Actual) Write: bar iefpFinalAddress=%08o writeData=%012"PRIo64"\n", cpu . iefpFinalAddress, data);
                return SCPE_OK;
            } else {
                //cpu . iefpFinalAddress = doAppendCycle(address, cyctyp, & data, 1);
                cpu . iefpFinalAddress = doAppendCycle(cyctyp, & data, 1);
                sim_debug(DBG_APPENDING | DBG_FINAL, &cpu_dev, "Write(Actual) Write: iefpFinalAddress=%08o writeData=%012"PRIo64"\n", cpu . iefpFinalAddress, data);
                return SCPE_OK;
            }
        }
    }
    
    return SCPE_UNK;
}


t_stat Write2 (word18 address, word36 * data, _processor_cycle_type cyctyp)
  {
    //cpu . iefpFinalAddress = address;
    cpu.TPR.CA = cpu.iefpFinalAddress = address;
    bool isBAR = get_bar_mode ();

    if (cpu.cu.TSN_VALID [0] || get_went_appending ())
      goto B29;
    
    switch (get_addr_mode ())
      {
        case ABSOLUTE_mode:
          {
            if (isBAR)
              {
                cpu.iefpFinalAddress = getBARaddress (address);
                setAPUStatus (apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (cyctyp);
                core_write2 (cpu.iefpFinalAddress, data [0], data [1],
                              __func__);
                sim_debug (DBG_FINAL, & cpu_dev,
                           "Write2 (Actual) Write:      bar address=%08o "
                           "writeData=%012"PRIo64" %012"PRIo64"\n",
                           address, data [0], data [1]);
              }
            else
              {
                setAPUStatus (apuStatus_FABS);
                fauxDoAppendCycle (cyctyp);
                core_write2 (address, data [0], data [1], __func__);
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
                //word18 barAddress = getBARaddress (address);
                //cpu.iefpFinalAddress = doAppendCycle (barAddress, cyctyp, data, 2);
                cpu.TPR.CA = getBARaddress (address);
                cpu.iefpFinalAddress = doAppendCycle (cyctyp, data, 2);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Write2 (Actual) Write: bar iefpFinalAddress=%08o "
                           "writeData=%012"PRIo64" %012"PRIo64"\n", 
                           address, data [0], data [1]);
              }
            else
              {
                //cpu.iefpFinalAddress = doAppendCycle (address, cyctyp, data, 2);
                cpu.iefpFinalAddress = doAppendCycle (cyctyp, data, 2);
                sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev,
                           "Write2 (Actual) Write: iefpFinalAddress=%08o "
                           "writeData=%012"PRIo64" %012"PRIo64"\n", 
                           address, data [0], data [1]);
              }
          }
          break;
      }
    return SCPE_UNK;
  }


t_stat Write8 (word18 address, word36 * data)
  {
    address &= paragraphMask; // Round to 8 word boundarryt
    //cpu.iefpFinalAddress = address;
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode ();

    if (cpu.cu.TSN_VALID [0] || get_went_appending ())
      goto B29;
    
    
    switch (get_addr_mode ())
     {
        case ABSOLUTE_mode:
          {
            if (isBAR)
             {
                cpu.iefpFinalAddress = getBARaddress (address);
                setAPUStatus (apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (APU_DATA_STORE);
                core_writeN (cpu.iefpFinalAddress, data, 8, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev, "Write8(Actual) Write:      bar address=%08o writeData=%012"PRIo64"\n", address + i, data [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < 8; i ++)
                  hdbgMWrite (cpu.iefpFinalAddress + i, data [i]);
#endif
                return SCPE_OK;
              }
            else
              {
                setAPUStatus (apuStatus_FABS);
                fauxDoAppendCycle (APU_DATA_STORE);
                core_writeN (address, data, 8, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev, "Write(Actual) Write:      abs address=%08o writeData=%012"PRIo64"\n", address + i, data [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < 8; i ++)
                  hdbgMWrite (address + i, data [i]);
#endif
                return SCPE_OK;
              }
          }

        case APPEND_mode:
          {
B29:
            if (isBAR)
              {
                //word18 barAddress = getBARaddress (address);
                //cpu.iefpFinalAddress = doAppendCycle (barAddress, APU_DATA_STORE, data, 8);
                cpu.TPR.CA = getBARaddress (address);
                cpu.iefpFinalAddress = doAppendCycle (APU_DATA_STORE, data, 8);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev, "Write8(Actual) Write: bar iefpFinalAddress=%08o writeData=%012"PRIo64"\n", cpu.iefpFinalAddress + i, data [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < 8; i ++)
                  hdbgMWrite (cpu.iefpFinalAddress + i, data [i]);
#endif
        
                return SCPE_OK;
              }
            else
              {
                //cpu.iefpFinalAddress = doAppendCycle (address, APU_DATA_STORE, data, 8);
                cpu.iefpFinalAddress = doAppendCycle (APU_DATA_STORE, data, 8);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 8; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev, "Write(Actual) Write: iefpFinalAddress=%08o writeData=%012"PRIo64"\n", cpu.iefpFinalAddress + i, data [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < 8; i ++)
                  hdbgMWrite (cpu.iefpFinalAddress + i, data [i]);
#endif
        
                return SCPE_OK;
              }
          }
      }
    return SCPE_UNK;
  }

t_stat Write16 (word18 address, word36 * data)
  {
    address &= paragraphMask; // Round to 8 word boundary
    //cpu.iefpFinalAddress = address;
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode ();

    if (cpu.cu.TSN_VALID [0] || get_went_appending ())
      goto B29;
    
    
    switch (get_addr_mode ())
     {
        case ABSOLUTE_mode:
          {
            if (isBAR)
             {
                cpu.iefpFinalAddress = getBARaddress (address);
                setAPUStatus (apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (APU_DATA_STORE);
                core_writeN (cpu.iefpFinalAddress, data, 16, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 16; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev, "Write16(Actual) Write:      bar address=%08o writeData=%012"PRIo64"\n", address + i, data [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < 16; i ++)
                  hdbgMWrite (cpu.iefpFinalAddress + i, data [i]);
#endif
                return SCPE_OK;
              }
            else
              {
                setAPUStatus (apuStatus_FABS);
                fauxDoAppendCycle (APU_DATA_STORE);
                core_writeN (address, data, 16, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 16; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev, "Write(Actual) Write:      abs address=%08o writeData=%012"PRIo64"\n", address + i, data [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < 16; i ++)
                  hdbgMWrite (address + i, data [i]);
#endif
                return SCPE_OK;
              }
          }

        case APPEND_mode:
          {
B29:
            if (isBAR)
              {
                //word18 barAddress = getBARaddress (address);
                //cpu.iefpFinalAddress = doAppendCycle (barAddress, APU_DATA_STORE, data, 16);
                cpu.TPR.CA = getBARaddress (address);
                cpu.iefpFinalAddress = doAppendCycle (APU_DATA_STORE, data, 16);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 16; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev, "Write16(Actual) Write: bar iefpFinalAddress=%08o writeData=%012"PRIo64"\n", cpu.iefpFinalAddress + i, data [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < 16; i ++)
                  hdbgMWrite (cpu.iefpFinalAddress + i, data [i]);
#endif
        
                return SCPE_OK;
              }
            else
              {
                //cpu.iefpFinalAddress = doAppendCycle (address, APU_DATA_STORE, data, 16);
                cpu.iefpFinalAddress = doAppendCycle (APU_DATA_STORE, data, 16);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 16; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev, "Write16(Actual) Write: iefpFinalAddress=%08o writeData=%012"PRIo64"\n", cpu.iefpFinalAddress + i, data [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < 16; i ++)
                  hdbgMWrite (cpu.iefpFinalAddress + i, data [i]);
#endif
        
                return SCPE_OK;
              }
          }
      }
    return SCPE_UNK;
  }

t_stat Write32 (word18 address, word36 * data)
  {
    address &= paragraphMask; // Round to 8 word boundary
    //cpu.iefpFinalAddress = address;
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode ();

    if (cpu.cu.TSN_VALID [0] || get_went_appending ())
      goto B29;
    
    
    switch (get_addr_mode ())
     {
        case ABSOLUTE_mode:
          {
            if (isBAR)
             {
                cpu.iefpFinalAddress = getBARaddress (address);
                setAPUStatus (apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (APU_DATA_STORE);
                core_writeN (cpu.iefpFinalAddress, data, 32, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 32; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev, "Write32(Actual) Write:      bar address=%08o writeData=%012"PRIo64"\n", address + i, data [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < 32; i ++)
                  hdbgMWrite (cpu.iefpFinalAddress + i, data [i]);
#endif
                return SCPE_OK;
              }
            else
              {
                setAPUStatus (apuStatus_FABS);
                fauxDoAppendCycle (APU_DATA_STORE);
                core_writeN (address, data, 32, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 32; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev, "Write(Actual) Write:      abs address=%08o writeData=%012"PRIo64"\n", address + i, data [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < 32; i ++)
                  hdbgMWrite (address + i, data [i]);
#endif
                return SCPE_OK;
              }
          }

        case APPEND_mode:
          {
B29:
            if (isBAR)
              {
                //word18 barAddress = getBARaddress (address);
                //cpu.iefpFinalAddress = doAppendCycle (barAddress, APU_DATA_STORE, data, 32);
                cpu.TPR.CA = getBARaddress (address);
                cpu.iefpFinalAddress = doAppendCycle (APU_DATA_STORE, data, 32);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 32; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev, "Write32(Actual) Write: bar iefpFinalAddress=%08o writeData=%012"PRIo64"\n", cpu.iefpFinalAddress + i, data [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < 32; i ++)
                  hdbgMWrite (cpu.iefpFinalAddress + i, data [i]);
#endif
        
                return SCPE_OK;
              }
            else
              {
                //cpu.iefpFinalAddress = doAppendCycle (address, APU_DATA_STORE, data, 32);
                cpu.iefpFinalAddress = doAppendCycle (APU_DATA_STORE, data, 32);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < 32; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev, "Write32(Actual) Write: iefpFinalAddress=%08o writeData=%012"PRIo64"\n", cpu.iefpFinalAddress + i, data [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < 32; i ++)
                  hdbgMWrite (cpu.iefpFinalAddress + i, data [i]);
#endif
        
                return SCPE_OK;
              }
          }
      }
    return SCPE_UNK;
  }

t_stat WritePage (word18 address, word36 * data)
  {
    if ((address & PGMK) != 0)
      {
        sim_err ("WritePage not on boundary %06o\n", address);
      }
    address &= (word18) ~PGMK; // Round to page boundary
    //cpu.iefpFinalAddress = address;
    cpu.TPR.CA = cpu.iefpFinalAddress = address;

    bool isBAR = get_bar_mode ();

    if (cpu.cu.TSN_VALID [0] || get_went_appending ())
      goto B29;
    
    
    switch (get_addr_mode ())
     {
        case ABSOLUTE_mode:
          {
            if (isBAR)
             {
                cpu.iefpFinalAddress = getBARaddress (address);
                setAPUStatus (apuStatus_FABS); // XXX maybe...
                fauxDoAppendCycle (APU_DATA_STORE);
                core_writeN (cpu.iefpFinalAddress, data, PGSZ, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev, "WritePage(Actual) Write:      bar address=%08o writeData=%012"PRIo64"\n", address + i, data [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < PGSZ; i ++)
                  hdbgMWrite (cpu.iefpFinalAddress + i, data [i]);
#endif
                return SCPE_OK;
              }
            else
              {
                setAPUStatus (apuStatus_FABS);
                fauxDoAppendCycle (APU_DATA_STORE);
                core_writeN (address, data, PGSZ, __func__);
                if_sim_debug (DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_FINAL, & cpu_dev, "WritePage(Actual) Write:      abs address=%08o writeData=%012"PRIo64"\n", address + i, data [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < PGSZ; i ++)
                  hdbgMWrite (address + i, data [i]);
#endif
                return SCPE_OK;
              }
          }

        case APPEND_mode:
          {
B29:
            if (isBAR)
              {
                //word18 barAddress = getBARaddress (address);
                //cpu.iefpFinalAddress = doAppendCycle (barAddress, APU_DATA_STORE, data, PGSZ);
                cpu.TPR.CA = getBARaddress (address);
                cpu.iefpFinalAddress = doAppendCycle (APU_DATA_STORE, data, PGSZ);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev, "WritePage(Actual) Write: bar iefpFinalAddress=%08o writeData=%012"PRIo64"\n", cpu.iefpFinalAddress + i, data [i]);
                  }
                return SCPE_OK;
              }
            else
              {
                //cpu.iefpFinalAddress = doAppendCycle (address, APU_DATA_STORE, data, PGSZ);
                cpu.iefpFinalAddress = doAppendCycle (APU_DATA_STORE, data, PGSZ);
                if_sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev)
                  {
                    for (uint i = 0; i < PGSZ; i ++)
                      sim_debug (DBG_APPENDING | DBG_FINAL, & cpu_dev, "WritePage(Actual) Write: iefpFinalAddress=%08o writeData=%012"PRIo64"\n", cpu.iefpFinalAddress + i, data [i]);
                  }
#ifdef HDBG
                for (uint i = 0; i < PGSZ; i ++)
                  hdbgMWrite (cpu.iefpFinalAddress + i, data [i]);
#endif
        
                return SCPE_OK;
              }
          }
      }
    return SCPE_UNK;
  }


t_stat ReadIndirect (void)
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
    return SCPE_OK;
  }

void ReadTraOp (void)
  {
    doComputedAddressFormation ();
    Read (cpu.TPR.CA, &cpu.CY, OPERAND_READ);
    if (! (get_addr_mode () == APPEND_mode || cpu.cu.TSN_VALID [0] || get_went_appending ()))
      {
        if (cpu.currentInstruction.info->flags & TSPN_INS)
          {
            word3 n;
            if (cpu.currentInstruction.opcode <= 0273)
              n = (cpu.currentInstruction.opcode & 3);
            else
              n = (cpu.currentInstruction.opcode & 3) + 4;

            // C(PPR.PRR) -> C(PRn .RNR)
            // C(PPR.PSR) -> C(PRn .SNR)
            // C(PPR.IC) -> C(PRn .WORDNO)
            // 000000 -> C(PRn .BITNO)
            cpu.PR[n].RNR = cpu.PPR.PRR;
// According the AL39, the PSR is 'undefined' in absolute mode.
// ISOLTS thinks means don't change the operand
            if (get_addr_mode () == APPEND_mode)
              cpu.PR[n].SNR = cpu.PPR.PSR;
            cpu.PR[n].WORDNO = (cpu.PPR.IC + 1) & MASK18;
            SET_PR_BITNO (n, 0);
          }
        cpu.PPR.IC = cpu.TPR.CA;
        cpu.PPR.PSR = 0;
      }
    sim_debug (DBG_TRACE, & cpu_dev, "ReadTraOp %05o:%06o\n", cpu.PPR.PSR, cpu.PPR.IC);
  }

void ReadRTCDOp (void)
  {
    doComputedAddressFormation ();
    Read2 (cpu.TPR.CA, cpu.Ypair, RTCD_OPERAND_FETCH);
  }

