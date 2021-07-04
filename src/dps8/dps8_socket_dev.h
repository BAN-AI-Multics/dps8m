/*
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2017 Charles Anthony
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE file at the top-level directory of this distribution.
 */

extern UNIT sk_unit [N_SKC_UNITS_MAX];
extern DEVICE skc_dev;
void sk_init(void);
int skc_iom_cmd (uint iomUnitIdx, uint chan);
void sk_process_event (void);
