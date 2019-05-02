/*
 Copyright 2015-2016 by Charles Anthony

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

struct absi_state_s
  {
    char device_name [MAX_DEV_NAME_LEN];
    int link;
  };
extern struct absi_state_s absi_state[N_SKC_UNITS_MAX];

extern DEVICE absi_dev;
extern UNIT absi_unit [N_ABSI_UNITS_MAX];

void absi_init (void);
void absi_process_event (void);
int absi_iom_cmd (uint iomUnitIdx, uint chan);
