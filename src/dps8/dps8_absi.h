/*
 * vim: filetype=c:tabstop=4:tw=100:expandtab
 * vim: ruler:hlsearch:incsearch:autoindent:wildmenu:wrapscan
 * SPDX-License-Identifier: ICU
 * scspell-id: 3bd4abd3-f62d-11ec-a9ab-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2015-2016 Charles Anthony
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

extern       DEVICE absi_dev;
extern       UNIT absi_unit [N_ABSI_UNITS_MAX];

void         absi_init (void);
void         absi_process_event (void);
iom_cmd_rc_t absi_iom_cmd (uint iomUnitIdx, uint chan);
