/*
 * vim: filetype=c:tabstop=4:tw=100:expandtab
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2015 Eric Swenson
 * Copyright (c) 2021-2022 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

//void Read (word18 addr, word36 *dat, processor_cycle_type cyctyp);
void ReadAPUDataRead (word18 addr, word36 *dat);
void ReadOperandRead (word18 addr, word36 *dat);
void ReadOperandRMW (word18 addr, word36 *dat);
void ReadAPUDataRMW (word18 addr, word36 *dat);
void ReadInstructionFetch (word18 addr, word36 *dat);
//void Read2 (word18 addr, word36 *dat, processor_cycle_type cyctyp);
void Read2OperandRead (word18 address, word36 * result);
void Read2IOperandRMW (word18 address, word36 * result);
void Read2InstructionFetch (word18 address, word36 * result);
void Read2RTCDOperandFetch (word18 address, word36 * result);
void Read2IndirectWordFetch (word18 address, word36 * result);
void Write (word18 addr, word36 dat, processor_cycle_type cyctyp);
void WriteAPUDataStore (word18 addr, word36 dat);
void WriteOperandStore (word18 addr, word36 dat);
//void Write2 (word18 address, word36 * data, processor_cycle_type cyctyp);
void Write2OperandStore (word18 address, word36 * data);
void Write1 (word18 address, word36 data, bool isAR);
void Write8 (word18 address, word36 * data, bool isAR);
void Write16 (word18 address, word36 * data);
void Write32 (word18 address, word36 * data);
void Read8 (word18 address, word36 * result, bool isAR);
void Read16 (word18 address, word36 * result);
void WritePage (word18 address, word36 * data, bool isAR);
void ReadPage (word18 address, word36 * result, bool isAR);
void ReadIndirect (void);
