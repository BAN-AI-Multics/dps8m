/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 0538414e-f62f-11ec-8915-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2022 Charles Anthony
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#if !defined(_DPS8_SYS_H)
# define _DPS8_SYS_H

# include <uv.h>
# include "uvutil.h"

// System-wide info and options not tied to a specific CPU, IOM, or SCU
typedef struct
  {
    // Delay times are in cycles; negative for immediate
    struct
      {
        volAtomic int connect;    // Delay between CIOC instr & connect channel operation
        //volAtomic int chan_activate;  // Time for a list service to send a DCW
        //volAtomic int boot_time; // delay between CPU start and IOM starting boot process
        //volAtomic int terminate_time; // delay between CPU start and IOM starting boot process
      } iom_times;
    // struct {
        // volAtomic int read;
        // volAtomic int xfer;
    // } mt_times;
    // bool volAtomic warn_uninit; // Warn when reading uninitialized memory

    volAtomic bool no_color;
    volAtomic uint sys_poll_interval; // Polling interval in milliseconds
    volAtomic uint sys_slow_poll_interval; // Polling interval in polling intervals
    volAtomic uint sys_poll_check_rate; // Check for pooling interval rate in CPU cycles
    bool nosync;          // If true, disable clock sync.
} sysinfo_t;

# if defined(THREADZ) || defined(LOCKLESS)
extern volAtomic bool syncClockMode;
extern volAtomic uint syncClockModeMasterIdx; // The CPU sync master
# endif

# if defined(THREADZ) || defined(LOCKLESS)
#  define DO_WORK                          \
  do {                                     \
    cpu.rTRticks ++;                       \
    if (cpu.syncClockModeCache)            \
      cpu.workAllocation --;               \
  } while (0)
# else
#  define DO_WORK                          \
  do {                                     \
    cpu.rTRticks ++;                       \
  } while (0)
# endif

# if defined(TR_WORK_EXEC)
#  define DO_WORK_EXEC DO_WORK
# else
#  define DO_WORK_EXEC
# endif

# if defined(TR_WORK_MEM)
#  define DO_WORK_MEM DO_WORK
# else
#  define DO_WORK_MEM
# endif

# if defined(DBGEVENT)
#  define max_dbgevents 128u
#  define dbgevent_tagsize 128
struct dbgevent_t
  {
    word15 segno;
    word18 offset;
    bool t0;
    char tag[dbgevent_tagsize];
  };

extern uint n_dbgevents;
extern struct dbgevent_t dbgevents[max_dbgevents];
extern struct timespec dbgevent_t0;
int dbgevent_lookup (word15 segno, word18 offset);
# endif

extern volAtomic word36 * M;  //-V707
extern sysinfo_t sys_opts;
extern uint64 sim_deb_start;
extern uint64 sim_deb_stop;
extern uint64 sim_deb_break;
# define DEBUG_SEGNO_LIMIT 1024
extern bool sim_deb_segno_on;
extern bool sim_deb_segno[DEBUG_SEGNO_LIMIT];
# define NO_SUCH_RINGNO ((uint64) -1ll)
extern uint64 sim_deb_ringno;
extern uint64 sim_deb_skip_limit;
extern uint64 sim_deb_mme_cntdwn;
extern uint64 sim_deb_skip_cnt;
extern bool sim_deb_bar;
extern DEVICE *sim_devices[];
extern uint dbgCPUMask;
extern bool breakEnable;

char * lookup_address (word18 segno, word18 offset, char * * compname, word18 * compoffset);
void list_source (char * compname, word18 offset, uint dflag);
//t_stat computeAbsAddrN (word24 * absAddr, int segno, uint offset);

t_stat brkbrk (int32 arg, const char * buf);
extern int32 luf_flag;

#endif
