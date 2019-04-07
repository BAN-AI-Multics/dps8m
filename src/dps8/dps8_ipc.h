
/*
 Copyright (c) 2007-2013 Michael Mondy
 Copyright 2012-2016 by Harry Reed
 Copyright 2019 by Charles Anthony

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

extern DEVICE ipcd_dev;
extern UNIT ipcd_unit [N_IPCD_UNITS_MAX];
extern DEVICE ipct_dev;
extern UNIT ipct_unit [N_IPCT_UNITS_MAX];

int ipcd_iom_cmd (uint iomUnitIdx, uint chan);
int ipct_iom_cmd (uint iomUnitIdx, uint chan);


