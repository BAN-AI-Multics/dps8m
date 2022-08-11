/*
 * vim: filetype=c:tabstop=4:tw=100:expandtab
 * vim: ruler:hlsearch:incsearch:autoindent:wildmenu:wrapscan
 * SPDX-License-Identifier: ICU
 * scspell-id: 128c0e3d-f630-11ec-9579-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2013-2019 Charles Anthony
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

void trk_init (bool write);
void trk (unsigned long long cycleCnt, uint16_t segno, uint32_t ic, uint64_t opcode);
