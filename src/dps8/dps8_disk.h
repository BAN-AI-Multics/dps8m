/*
 Copyright (c) 2007-2013 Michael Mondy
 Copyright 2012-2016 by Harry Reed
 Copyright 2013-2016 by Charles Anthony

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

extern DEVICE dsk_dev;
extern UNIT dsk_unit [N_DSK_UNITS_MAX];

//extern DEVICE ipc_dev;
//extern UNIT ipc_unit [N_IPC_UNITS_MAX];

extern DEVICE msp_dev;
extern UNIT msp_unit [N_MSP_UNITS_MAX];

struct dsk_state
  {
    uint typeIdx;
    enum { disk_no_mode, disk_seek512_mode, disk_seek64_mode, disk_seek_mode, disk_read_mode, disk_write_mode, disk_request_status_mode } io_mode;
    uint seekPosition;
    char device_name [MAX_DEV_NAME_LEN];
#ifdef LOCKLESS
    pthread_mutex_t dsk_lock;
#endif
  };

extern struct dsk_state dsk_states [N_DSK_UNITS_MAX];


void disk_init(void);
//t_stat attachDisk (char * label);
t_stat attach_disk (char * label, bool with_protect, char * drive);
int dsk_iom_cmd (uint iomUnitIdx, uint chan);
t_stat load_disk (uint dsk_unit_idx, const char * disk_filename, bool ro);
t_stat print_config_deck (int32 flag, UNUSED const char * cptr);
t_stat sync_config_deck (int32 flag, UNUSED const char * cptr);


