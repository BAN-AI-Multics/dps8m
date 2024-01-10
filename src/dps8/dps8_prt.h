/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: cffcb19a-f62e-11ec-b3a0-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

extern DEVICE prt_dev;
extern UNIT prt_unit [N_PRT_UNITS_MAX];

void prt_init(void);
iom_cmd_rc_t prt_iom_cmd (uint iomUnitIdx, uint chan);
t_stat burst_printer (UNUSED int32 arg, const char * buf);
