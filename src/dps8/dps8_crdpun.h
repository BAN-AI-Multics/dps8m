/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 84a7d169-f62d-11ec-8fa8-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2013-2016 Charles Anthony
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

extern DEVICE pun_dev;
extern UNIT pun_unit [N_PUN_UNITS_MAX];
void pun_init(void);
iom_cmd_rc_t pun_iom_cmd (uint iomUnitIdx, uint chan);
