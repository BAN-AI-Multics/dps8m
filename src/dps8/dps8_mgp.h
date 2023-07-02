/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 4346714b-ebbf-11ed-9a36-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2015-2016 Charles Anthony
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

#ifdef WITH_MGP_DEV
extern       DEVICE  mgp_dev;
extern       UNIT    mgp_unit[N_MGP_UNITS_MAX];
void         mgp_init(void);
void         mgp_process_event(void);
iom_cmd_rc_t mgp_iom_cmd(uint iomUnitIdx, uint chan);
#endif /* ifdef WITH_MGP_DEV */
