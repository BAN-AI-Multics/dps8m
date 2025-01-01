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
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#if defined(OLDAPP)
void Read (word18 addr, word36 *dat, processor_cycle_type cyctyp);
void Read2 (word18 addr, word36 *dat, processor_cycle_type cyctyp);
void Write (word18 addr, word36 dat, processor_cycle_type cyctyp);
void Write2 (word18 address, word36 * data, processor_cycle_type cyctyp);
# define ReadAPUDataRead(addr,data) Read (addr, data, APU_DATA_READ)
# define ReadOperandRead(addr,data) Read (addr, data, OPERAND_READ)
# if defined(LOCKLESS)
#  define ReadOperandRMW(addr,data) Read (addr, data, OPERAND_RMW)
#  define ReadAPUDataRMW(addr,data) Read (addr, data, APU_DATA_RMW)
# else
#  define ReadOperandRMW ReadOperandRead
#  define ReadAPUDataRMW ReadAPUDataRead
# endif /* if defined(LOCKLESS) */
# define ReadInstructionFetch(addr,data) Read (addr, data, INSTRUCTION_FETCH)
# define ReadIndirectWordFetch(addr,data) Read2 (addr, data, INDIRECT_WORD_FETCH)
# define Read2OperandRead(addr,data) Read2 (addr, data, OPERAND_READ)
# if defined(LOCKLESS)
#  define Read2OperandRMW(addr,data) Read2 (addr, data, OPERAND_RMW)
# endif /* if defined(LOCKLESS) */
# define Read2InstructionFetch(addr,data) Read2 (addr, data, INSTRUCTION_FETCH)
# define Read2RTCDOperandFetch(addr,data) Read2 (addr, data, RTCD_OPERAND_FETCH)
# define Read2IndirectWordFetch(addr,data) Read2 (addr, data, INDIRECT_WORD_FETCH)
# define WriteAPUDataStore(addr,data) Write (addr, data, APU_DATA_STORE)
# define WriteOperandStore(addr,data) Write (addr, data, OPERAND_STORE)
# define Write2OperandStore(addr,data) Write2 (addr, data, OPERAND_STORE)
#else
void ReadAPUDataRead (cpu_state_t * cpup, word18 addr, word36 *dat);
void ReadOperandRead (cpu_state_t * cpup, word18 addr, word36 *dat);
# if defined(LOCKLESS)
void ReadOperandRMW (cpu_state_t * cpup, word18 addr, word36 *dat);
void ReadAPUDataRMW (cpu_state_t * cpup, word18 addr, word36 *dat);
# else
#  define ReadOperandRMW ReadOperandRead
#  define ReadAPUDataRMW ReadAPUDataRead
# endif /* if defined(LOCKLESS) */
void ReadInstructionFetch (cpu_state_t * cpup, word18 addr, word36 *dat);
void ReadIndirectWordFetch (cpu_state_t * cpup, word18 address, word36 * result);
void Read2OperandRead (cpu_state_t * cpup, word18 address, word36 * result);
# if defined(LOCKLESS)
void Read2OperandRMW (cpu_state_t * cpup, word18 address, word36 * result);
# endif /* if defined(LOCKLESS) */
void Read2InstructionFetch (cpu_state_t * cpup, word18 address, word36 * result);
void Read2RTCDOperandFetch (cpu_state_t * cpup, word18 address, word36 * result);
void Read2IndirectWordFetch (cpu_state_t * cpup, word18 address, word36 * result);
void WriteAPUDataStore (cpu_state_t * cpup, word18 addr, word36 dat);
void WriteOperandStore (cpu_state_t * cpup, word18 addr, word36 dat);
void Write2OperandStore (cpu_state_t * cpup, word18 address, word36 * data);
#endif /* if defined(OLDAPP) */
void Write1 (cpu_state_t * cpup, word18 address, word36 data, bool isAR);
void Write8 (cpu_state_t * cpup, word18 address, word36 * data, bool isAR);
void Write16 (cpu_state_t * cpup, word18 address, word36 * data);
void Write32 (cpu_state_t * cpup, word18 address, word36 * data);
void Read8 (cpu_state_t * cpup, word18 address, word36 * result, bool isAR);
void Read16 (cpu_state_t * cpup, word18 address, word36 * result);
void WritePage (cpu_state_t * cpup, word18 address, word36 * data, bool isAR);
void ReadPage (cpu_state_t * cpup, word18 address, word36 * result, bool isAR);
void ReadIndirect (cpu_state_t * cpup);
