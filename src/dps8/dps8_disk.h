/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 26ac6f06-f62e-11ec-ab75-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
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

extern DEVICE dsk_dev;
extern UNIT dsk_unit [N_DSK_UNITS_MAX];

extern DEVICE ipc_dev;
extern UNIT ipc_unit [N_IPC_UNITS_MAX];

extern DEVICE msp_dev;
extern UNIT msp_unit [N_IPC_UNITS_MAX];

struct dsk_state
  {
    uint typeIdx;
    enum
      {
        disk_no_mode, disk_rd_clr_stats, disk_rd_status_reg, disk_rd_config,
        disk_rd, disk_seek_512, disk_wr, disk_seek_64, disk_special_seek,
        disk_rd_ctrl_reg
      } io_mode;
    uint tAndDCapac;
    bool seekValid; // True if seekPosition contains a valid seek address.
    uint seekPosition;
    char device_name [MAX_DEV_NAME_LEN];
#ifdef LOCKLESS
    pthread_mutex_t dsk_lock;
#endif
  };

// Disk types
//
//  D500
//  D451
//  D400
//  D190
//  D181
//  D501
//  3380
//  3381

enum seekSize_t { seek_64, seek_512};
struct diskType_t
  {
    char * typename;
    uint capac;
    uint firstDevNumber;
    bool removable;
    enum seekSize_t seekSize; // false: seek 64  true: seek 512
    uint sectorSizeWords;
    uint dau_type;
  };
extern struct diskType_t diskTypes [];

struct msp_state_s
  {
    char device_name [MAX_DEV_NAME_LEN];
  };
extern struct msp_state_s msp_states [N_MSP_UNITS_MAX];

extern struct dsk_state dsk_states [N_DSK_UNITS_MAX];

void disk_init(void);
t_stat attachDisk (char * label);
iom_cmd_rc_t dsk_iom_cmd (uint iomUnitIdx, uint chan);
t_stat loadDisk (uint dsk_unit_idx, const char * disk_filename, bool ro);
t_stat unloadDisk (uint dsk_unit_idx);
t_stat signal_disk_ready (uint dsk_unit_idx);
