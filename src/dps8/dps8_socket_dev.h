/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: f4a96cd7-f62e-11ec-ba95-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2017 Charles Anthony
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#ifdef WITH_SOCKET_DEV
extern UNIT   sk_unit [N_SKC_UNITS_MAX];
extern DEVICE skc_dev;
void          sk_init(void);
void          sk_process_event (void);
iom_cmd_rc_t  skc_iom_cmd (uint iomUnitIdx, uint chan);
#endif /* ifdef WITH_SOCKET_DEV */
