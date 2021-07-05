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
 * LICENSE file at the top-level directory of this distribution.
 */

extern DEVICE rdr_dev;
extern UNIT rdr_unit [N_RDR_UNITS_MAX];

void rdr_init(void);
int rdr_iom_cmd (uint iomUnitIdx, uint chan);
void rdrCardReady (int unitNum);
void rdrProcessEvent (void);

