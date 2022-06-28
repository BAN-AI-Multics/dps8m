/*
 * vim: filetype=c:tabstop=4:tw=100:expandtab
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2022 Charles Anthony
 * Copyright (c) 2022 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

void uc_invalidate (void);
void uc_cache_save (uint uc_num, word15 segno, word18 offset, word14 bound, word1 p, word24 address, word3 r1);
bool uc_cache_check (uint uc_num, word15 segno, word18 offset, word14 * bound, word1 * p, word24 * address, word3 * r1);
