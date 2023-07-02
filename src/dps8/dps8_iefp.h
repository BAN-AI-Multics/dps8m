/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 79aba365-f62e-11ec-b567-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2015 Eric Swenson
 * Copyright (c) 2021-2023 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#ifdef OLDAPP
void Read (word18 addr, word36 *dat, processor_cycle_type cyctyp);
void Read2 (word18 addr, word36 *dat, processor_cycle_type cyctyp);
void Write (word18 addr, word36 dat, processor_cycle_type cyctyp);
void Write2 (word18 address, word36 * data, processor_cycle_type cyctyp);
# define ReadAPUDataRead(addr,data) Read (addr, data, APU_DATA_READ)
# define ReadOperandRead(addr,data) Read (addr, data, OPERAND_READ)
# ifdef LOCKLESS
#  define ReadOperandRMW(addr,data) Read (addr, data, OPERAND_RMW)
#  define ReadAPUDataRMW(addr,data) Read (addr, data, APU_DATA_RMW)
# else
#  define ReadOperandRMW ReadOperandRead
#  define ReadAPUDataRMW ReadAPUDataRead
# endif
# define ReadInstructionFetch(addr,data) Read (addr, data, INSTRUCTION_FETCH)
# define ReadIndirectWordFetch(addr,data) Read2 (addr, data, INDIRECT_WORD_FETCH)
# define Read2OperandRead(addr,data) Read2 (addr, data, OPERAND_READ)
# ifdef LOCKLESS
#  define Read2OperandRMW(addr,data) Read2 (addr, data, OPERAND_RMW)
# endif
# define Read2InstructionFetch(addr,data) Read2 (addr, data, INSTRUCTION_FETCH)
# define Read2RTCDOperandFetch(addr,data) Read2 (addr, data, RTCD_OPERAND_FETCH)
# define Read2IndirectWordFetch(addr,data) Read2 (addr, data, INDIRECT_WORD_FETCH)
# define WriteAPUDataStore(addr,data) Write (addr, data, APU_DATA_STORE)
# define WriteOperandStore(addr,data) Write (addr, data, OPERAND_STORE)
# define Write2OperandStore(addr,data) Write2 (addr, data, OPERAND_STORE)
#else // !OLDAPP
void ReadAPUDataRead (word18 addr, word36 *dat);
void ReadOperandRead (word18 addr, word36 *dat);
# ifdef LOCKLESS
void ReadOperandRMW (word18 addr, word36 *dat);
void ReadAPUDataRMW (word18 addr, word36 *dat);
# else
#  define ReadOperandRMW ReadOperandRead
#  define ReadAPUDataRMW ReadAPUDataRead
# endif
void ReadInstructionFetch (word18 addr, word36 *dat);
void ReadIndirectWordFetch (word18 address, word36 * result);
void Read2OperandRead (word18 address, word36 * result);
# ifdef LOCKLESS
void Read2OperandRMW (word18 address, word36 * result);
# endif
void Read2InstructionFetch (word18 address, word36 * result);
void Read2RTCDOperandFetch (word18 address, word36 * result);
void Read2IndirectWordFetch (word18 address, word36 * result);
void WriteAPUDataStore (word18 addr, word36 dat);
void WriteOperandStore (word18 addr, word36 dat);
void Write2OperandStore (word18 address, word36 * data);
#endif // !OLDAPP
void Write1 (word18 address, word36 data, bool isAR);
void Write8 (word18 address, word36 * data, bool isAR);
void Write16 (word18 address, word36 * data);
void Write32 (word18 address, word36 * data);
void Read8 (word18 address, word36 * result, bool isAR);
void Read16 (word18 address, word36 * result);
void WritePage (word18 address, word36 * data, bool isAR);
void ReadPage (word18 address, word36 * result, bool isAR);
void ReadIndirect (void);
