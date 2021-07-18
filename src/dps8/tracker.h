/*
 * Copyright (c) 2013-2019 Charles Anthony
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 */

void trk_init (bool write);
void trk (unsigned long long cycleCnt, uint16_t segno, uint32_t ic, uint64_t opcode);
