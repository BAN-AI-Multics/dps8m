/*
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 */

extern DEVICE dsk_dev;
extern UNIT dsk_unit [N_DSK_UNITS_MAX];

extern DEVICE ipc_dev;
extern UNIT ipc_unit [N_IPC_UNITS_MAX];

extern DEVICE msp_dev;
extern UNIT msp_unit [N_IPC_UNITS_MAX];

void disk_init(void);
t_stat attachDisk (char * label);
iom_cmd_rc_t dsk_iom_cmd (uint iomUnitIdx, uint chan);
