/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: b39ac98e-f62e-11ec-98a8-80ee73e9b8e7
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

#define BUFSZ (4096 * 9 / 2)

struct tape_state
  {
    //enum tape_mode { tape_no_mode, tape_read_mode, tape_write_mode, tape_survey_mode } io_mode;
    enum tape_mode
      {
        tape_no_mode, tape_rd_ctlr, tape_rd_9, tape_rd_bin, tape_initiate_rd_mem,
        tape_wr_9, tape_wr_bin, tape_initiate_wr_mem, tape_MTP_wr, tape_wr_ctrl_regs, tape_survey,
      } io_mode;
    bool is9;
    uint8 buf [BUFSZ];
    t_mtrlnt tbc; // Number of bytes read into buffer
    uint words_processed; // Number of Word36 processed from the buffer
// XXX bug: 'sim> set tapeN rewind' doesn't reset rec_num
    int rec_num; // track tape position
    char device_name [MAX_DEV_NAME_LEN];
    word16 cntlrAddress;
    word16 cntlrTally;
    int tape_length;
  };

extern struct tape_state tape_states [N_MT_UNITS_MAX];

extern UNIT mt_unit [N_MT_UNITS_MAX];
extern DEVICE tape_dev;
extern UNIT mtp_unit [N_MTP_UNITS_MAX];
extern DEVICE mtp_dev;

void mt_init(void);
void mt_exit (void);
int get_mt_numunits (void);
//UNIT * getTapeUnit (uint driveNumber);
//void tape_send_special_interrupt (uint driveNumber);
t_stat loadTape (uint driveNumber, char * tapeFilename, bool ro);
t_stat attachTape (char * label, bool withring, char * drive);
#ifndef QUIET_UNUSED
t_stat detachTape (char * drive);
#endif
iom_cmd_rc_t mt_iom_cmd (uint iomUnitIdx, uint chan);
t_stat unloadTape (uint driveNumber);
t_stat mount_tape (int32 arg, const char * buf);
t_stat signal_tape (uint tap_unit_idx, word8 status0, word8 status1);
