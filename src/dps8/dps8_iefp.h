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

void Read (word18 addr, word36 *dat, processor_cycle_type cyctyp);
void Read2 (word18 addr, word36 *dat, processor_cycle_type cyctyp);
void Write (word18 addr, word36 dat, processor_cycle_type cyctyp);
void Write2 (word18 address, word36 * data, processor_cycle_type cyctyp);
void Write1 (word18 address, word36 data, bool isAR);
void Write8 (word18 address, word36 * data, bool isAR);
void Write16 (word18 address, word36 * data);
void Write32 (word18 address, word36 * data);
void Read8 (word18 address, word36 * result, bool isAR);
void Read16 (word18 address, word36 * result);
void WritePage (word18 address, word36 * data, bool isAR);
void ReadPage (word18 address, word36 * result, bool isAR);
void ReadIndirect (void);
