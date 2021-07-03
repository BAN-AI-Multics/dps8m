/*
 * Copyright (c) 2015-2016 Charles Anthony
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU  
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE file at the top-level directory of this distribution.
 */

extern DEVICE absi_dev;
extern UNIT absi_unit [N_ABSI_UNITS_MAX];

void absi_init (void);
void absi_process_event (void);
int absi_iom_cmd (uint iomUnitIdx, uint chan);
