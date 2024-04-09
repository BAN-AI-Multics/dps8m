/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * SPDX-License-Identifier: Multics
 * scspell-id: 6e07fe19-f62d-11ec-86f2-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2023 Charles Anthony
 * Copyright (c) 2017 Michal Tomek
 * Copyright (c) 2021-2023 Jeffrey H. Johnson
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 *
 * This source file may contain code comments that adapt, include, and/or
 * incorporate Multics program code and/or documentation distributed under
 * the Multics License.  In the event of any discrepancy between code
 * comments herein and the original Multics materials, the original Multics
 * materials should be considered authoritative unless otherwise noted.
 * For more details and historical background, see the LICENSE.md file at
 * the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include "dps8.h"
#include "dps8_addrmods.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_append.h"
#include "dps8_ins.h"
#include "dps8_state.h"
#include "dps8_math.h"
#include "dps8_iefp.h"
#include "dps8_console.h"
#include "dps8_fnp2.h"
#include "dps8_socket_dev.h"
#include "dps8_crdrdr.h"
#include "dps8_absi.h"
#include "dps8_mgp.h"
#include "dps8_utils.h"

#ifdef M_SHARED
# include "shm.h"
#endif

#include "dps8_opcodetable.h"
#include "sim_defs.h"

#if defined(THREADZ) || defined(LOCKLESS)
# include "threadz.h"
__thread uint current_running_cpu_idx;
#endif

#if defined(__MACH__) && defined(__APPLE__) && \
  ( defined(__PPC__) || defined(_ARCH_PPC) )
# include <mach/clock.h>
# include <mach/mach.h>
# ifdef MACOSXPPC
#  undef MACOSXPPC
# endif /* ifdef MACOSXPPC */
# define MACOSXPPC 1
#endif /* if defined(__MACH__) && defined(__APPLE__) &&
           ( defined(__PPC__) || defined(_ARCH_PPC) ) */

#include "ver.h"

#define DBG_CTR cpu.cycleCnt

#define ASSUME0 0

#ifdef TESTING
# undef FREE
# define FREE(p) free(p)
#endif /* ifdef TESTING */

// CPU data structures

static UNIT cpu_unit [N_CPU_UNITS_MAX] = {
#ifdef NO_C_ELLIPSIS
  { UDATA (NULL, UNIT_FIX|UNIT_BINK, MEMSIZE), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FIX|UNIT_BINK, MEMSIZE), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FIX|UNIT_BINK, MEMSIZE), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FIX|UNIT_BINK, MEMSIZE), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FIX|UNIT_BINK, MEMSIZE), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FIX|UNIT_BINK, MEMSIZE), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FIX|UNIT_BINK, MEMSIZE), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FIX|UNIT_BINK, MEMSIZE), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL }
#else
  [0 ... N_CPU_UNITS_MAX - 1] = {
    UDATA (NULL, UNIT_FIX|UNIT_BINK, MEMSIZE), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL
  }
#endif
};

#define UNIT_IDX(uptr) ((uptr) - cpu_unit)

// Assume CPU clock ~ 1MIPS. lockup time is 32 ms
#define LOCKUP_KIPS 1000
static uint kips = LOCKUP_KIPS;
static uint64 luf_limits[] =
  {
     2000*LOCKUP_KIPS/1000,
     4000*LOCKUP_KIPS/1000,
     8000*LOCKUP_KIPS/1000,
    16000*LOCKUP_KIPS/1000,
    32000*LOCKUP_KIPS/1000
  };

struct stall_point_s stall_points [N_STALL_POINTS];
bool stall_point_active = false;

#ifdef PANEL68
static void panel_process_event (void);
#endif

static t_stat simh_cpu_reset_and_clear_unit (UNIT * uptr,
                                             UNUSED int32 value,
                                             UNUSED const char * cptr,
                                             UNUSED void * desc);
static char * cycle_str (cycles_e cycle);

static t_stat cpu_show_config (UNUSED FILE * st, UNIT * uptr,
                               UNUSED int val, UNUSED const void * desc)
  {
    long cpu_unit_idx = UNIT_IDX (uptr);
    if (cpu_unit_idx < 0 || cpu_unit_idx >= N_CPU_UNITS_MAX)
      {
        sim_warn ("error: Invalid unit number %ld\n", (long) cpu_unit_idx);
        return SCPE_ARG;
      }

#define PFC_INT8 "%c%c%c%c%c%c%c%c"

#define PBI_8(i)                     \
    ( ((i) & 0x80ll) ? '1' : '0' ),  \
    ( ((i) & 0x40ll) ? '1' : '0' ),  \
    ( ((i) & 0x20ll) ? '1' : '0' ),  \
    ( ((i) & 0x10ll) ? '1' : '0' ),  \
    ( ((i) & 0x08ll) ? '1' : '0' ),  \
    ( ((i) & 0x04ll) ? '1' : '0' ),  \
    ( ((i) & 0x02ll) ? '1' : '0' ),  \
    ( ((i) & 0x01ll) ? '1' : '0' )

#define PFC_INT16 PFC_INT8  PFC_INT8
#define PFC_INT32 PFC_INT16 PFC_INT16
#define PFC_INT64 PFC_INT32 PFC_INT32

#define PBI_16(i) PBI_8((i)  >>  8), PBI_8(i)
#define PBI_32(i) PBI_16((i) >> 16), PBI_16(i)
#define PBI_64(i) PBI_32((i) >> 32), PBI_32(i)

    char dsbin[66], adbin[34];

    sim_msg ("CPU unit number %ld\n", (long) cpu_unit_idx);

    sim_msg ("Fault base:                   %03o(8)\n",
                cpus[cpu_unit_idx].switches.FLT_BASE);
    sim_msg ("CPU number:                   %01o(8)\n",
                cpus[cpu_unit_idx].switches.cpu_num);
    sim_msg ("Data switches:                %012llo(8)\n",
                (unsigned long long)cpus[cpu_unit_idx].switches.data_switches);
    (void)snprintf (dsbin, 65, PFC_INT64,
                    PBI_64((unsigned long long)cpus[cpu_unit_idx].switches.data_switches));
    sim_msg ("                              %36s(2)\n",
                dsbin + strlen(dsbin) - 36);
    sim_msg ("Address switches:             %06o(8)\n",
                cpus[cpu_unit_idx].switches.addr_switches);
    (void)snprintf (adbin, 33, PFC_INT32,
                    PBI_32(cpus[cpu_unit_idx].switches.addr_switches));
    sim_msg ("                              %18s(2)\n",
                adbin + strlen(adbin) - 18);
    for (int i = 0; i < (cpus[cpu_unit_idx].tweaks.l68_mode ? N_L68_CPU_PORTS : N_DPS8M_CPU_PORTS); i ++)
      {
        sim_msg ("Port%c enable:                 %01o(8)\n",
                    'A' + i, cpus[cpu_unit_idx].switches.enable [i]);
        sim_msg ("Port%c init enable:            %01o(8)\n",
                    'A' + i, cpus[cpu_unit_idx].switches.init_enable [i]);
        sim_msg ("Port%c assignment:             %01o(8)\n",
                    'A' + i, cpus[cpu_unit_idx].switches.assignment [i]);
        sim_msg ("Port%c interlace:              %01o(8)\n",
                    'A' + i, cpus[cpu_unit_idx].switches.interlace [i]);
        sim_msg ("Port%c store size:             %01o(8)\n",
                    'A' + i, cpus[cpu_unit_idx].switches.store_size [i]);
      }
    sim_msg ("Processor mode:               %s [%o]\n",
                cpus[cpu_unit_idx].switches.procMode == procModeMultics ? "Multics" : cpus[cpu_unit_idx].switches.procMode == procModeGCOS ? "GCOS" : "???",
                cpus[cpu_unit_idx].switches.procMode);
    sim_msg ("8K Cache:                     %s\n",
                cpus[cpu_unit_idx].switches.enable_cache ? "Enabled" : "Disabled");
    sim_msg ("SDWAM:                        %s\n",
                cpus[cpu_unit_idx].switches.sdwam_enable ? "Enabled" : "Disabled");
    sim_msg ("PTWAM:                        %s\n",
                cpus[cpu_unit_idx].switches.ptwam_enable ? "Enabled" : "Disabled");

    sim_msg ("Processor speed:              %02o(8)\n",
                cpus[cpu_unit_idx].options.proc_speed);
    sim_msg ("DIS enable:                   %01o(8)\n",
                cpus[cpu_unit_idx].tweaks.dis_enable);
    sim_msg ("Steady clock:                 %01o(8)\n",
                scu [0].steady_clock);
    sim_msg ("Halt on unimplemented:        %01o(8)\n",
                cpus[cpu_unit_idx].tweaks.halt_on_unimp);
    sim_msg ("Enable simulated SDWAM/PTWAM: %01o(8)\n",
                cpus[cpu_unit_idx].tweaks.enable_wam);
    sim_msg ("Report faults:                %01o(8)\n",
                cpus[cpu_unit_idx].tweaks.report_faults);
    sim_msg ("TRO faults enabled:           %01o(8)\n",
                cpus[cpu_unit_idx].tweaks.tro_enable);
    sim_msg ("Y2K enabled:                  %01o(8)\n",
                scu [0].y2k);
    sim_msg ("drl fatal enabled:            %01o(8)\n",
                cpus[cpu_unit_idx].tweaks.drl_fatal);
    sim_msg ("useMap:                       %d\n",
                cpus[cpu_unit_idx].tweaks.useMap);
    sim_msg ("PROM installed:               %01o(8)\n",
                cpus[cpu_unit_idx].options.prom_installed);
    sim_msg ("Hex mode installed:           %01o(8)\n",
                cpus[cpu_unit_idx].options.hex_mode_installed);
    sim_msg ("8K cache installed:           %01o(8)\n",
                cpus[cpu_unit_idx].options.cache_installed);
    sim_msg ("Clock slave installed:        %01o(8)\n",
                cpus[cpu_unit_idx].options.clock_slave_installed);
#ifdef AFFINITY
    if (cpus[cpu_unit_idx].set_affinity)
      sim_msg ("CPU affinity:                 %d\n", cpus[cpu_unit_idx].affinity);
    else
      sim_msg ("CPU affinity:                 not set\n");
#endif
    sim_msg ("ISOLTS mode:                  %01o(8)\n", cpus[cpu_unit_idx].tweaks.isolts_mode);
    sim_msg ("NODIS mode:                   %01o(8)\n", cpus[cpu_unit_idx].tweaks.nodis);
    sim_msg ("6180 mode:                    %01o(8) [%s]\n", cpus[cpu_unit_idx].tweaks.l68_mode, cpus[cpu_unit_idx].tweaks.l68_mode ? "6180" : "DPS8/M");
    return SCPE_OK;
  }

//
// set cpu0 config=<blah> [;<blah>]
//
//    blah =
//           faultbase = n
//           num = n
//           data = n
//           portenable = n
//           portconfig = n
//           portinterlace = n
//           mode = n
//           speed = n
//    Hacks:
//           dis_enable = n
//           steadyclock = on|off
//           halt_on_unimplemented = n
//           enable_wam = n
//           report_faults = n
//               n = 0 don't
//               n = 1 report
//               n = 2 report overflow
//           tro_enable = n
//           y2k
//           drl_fatal

static config_value_list_t cfg_multics_fault_base [] =
  {
    { "multics", 2 },
    { NULL,      0 }
  };

static config_value_list_t cfg_on_off [] =
  {
    { "off",     0 },
    { "on",      1 },
    { "disable", 0 },
    { "enable",  1 },
    { NULL,      0 }
  };

static config_value_list_t cfg_l68_mode [] = {
  { "dps8/m", 0 },
  { "dps8m",  0 },
  { "dps8",   0 },
  { "l68",    1 },
  { "l6180",  1 },
  { "6180",   1 },
};

static config_value_list_t cfg_cpu_mode [] =
  {
    { "gcos",    0 },
    { "multics", 1 },
    { NULL,      0 }
  };

static config_value_list_t cfg_port_letter [] =
  {
    { "a",  0 },
    { "b",  1 },
    { "c",  2 },
    { "d",  3 },
    { "e",  4 },
    { "f",  5 },
    { "g",  6 },
    { "h",  7 },
    { NULL, 0 }
  };

static config_value_list_t cfg_interlace [] =
  {
    { "off", 0 },
    { "2",   2 },
    { "4",   4 },
    { NULL,  0 }
  };

#ifdef AFFINITY
static config_value_list_t cfg_affinity [] =
  {
    { "off", -1 },
    { NULL,  0  }
  };
#endif

static config_value_list_t cfg_size_list [] =
  {
#if 0
// For Level-68:
// rsw.incl.pl1
//
//  /* DPS and L68 memory sizes */
//  dcl  dps_mem_size_table (0:7) fixed bin (24) static options (constant) init
//      (32768, 65536, 4194304, 131072, 524288, 1048576, 2097152, 262144);
//
//  Note that the third array element above, is changed incompatibly in MR10.0.
//  In previous releases, this array element was used to decode a port size of
//  98304 (96K). With MR10.0 it is now possible to address 4MW per CPU port, by
//  installing  FCO # PHAF183 and using a group 10 patch plug, on L68 and DPS
//  CPUs.

    { "32",    0 }, //   32768
    { "64",    1 }, //   65536
    { "4096",  2 }, // 4194304
    { "128",   3 }, //  131072
    { "512",   4 }, //  524288
    { "1024",  5 }, // 1048576
    { "2048",  6 }, // 2097152
    { "256",   7 }, //  262144

    { "32K",   0 },
    { "64K",   1 },
    { "4096K", 2 },
    { "128K",  3 },
    { "512K",  4 },
    { "1024K", 5 },
    { "2048K", 6 },
    { "256K",  7 },

    { "1M", 5 },
    { "2M", 6 },
    { "4M", 2 },

// For DPS8/M:
// These values are taken from the dps8_mem_size_table loaded by the boot tape.

    {    "32", 0 },
    {    "64", 1 },
    {   "128", 2 },
    {   "256", 3 },
    {   "512", 4 },
    {  "1024", 5 },
    {  "2048", 6 },
    {  "4096", 7 },

    {   "32K", 0 },
    {   "64K", 1 },
    {  "128K", 2 },
    {  "256K", 3 },
    {  "512K", 4 },
    { "1024K", 5 },
    { "2048K", 6 },
    { "4096K", 7 },

    { "1M", 5 },
    { "2M", 6 },
    { "4M", 7 },
    { NULL, 0 }
#endif
    { "32",     8 },    //   32768
    { "32K",    8 },    //   32768
    { "64",     9 },    //   65536
    { "64K",    9 },    //   65536
    { "128",   10 },    //  131072
    { "128K",  10 },    //  131072
    { "256",   11 },    //  262144
    { "256K",  11 },    //  262144
    { "512",   12 },    //  524288
    { "512K",  12 },    //  524288
    { "1024",  13 },    // 1048576
    { "1024K", 13 },    // 1048576
    { "1M",    13 },
    { "2048",  14 },    // 2097152
    { "2048K", 14 },    // 2097152
    { "2M",    14 },
    { "4096",  15 },    // 4194304
    { "4096K", 15 },    // 4194304
    { "4M",    15 },
    { NULL,    0  }
  };

static config_list_t cpu_config_list [] =
  {
    { "faultbase",             0,  0177,            cfg_multics_fault_base },
    { "num",                   0,  07,              NULL                   },
    { "data",                  0,  0777777777777,   NULL                   },
    { "stopnum",               0,  999999,          NULL                   },
    { "mode",                  0,  01,              cfg_cpu_mode           },
    { "speed",                 0,  017,             NULL                   },  // XXX use keywords
    { "port",                  0,  N_CPU_PORTS - 1, cfg_port_letter        },
    { "assignment",            0,  7,               NULL                   },
    { "interlace",             0,  1,               cfg_interlace          },
    { "enable",                0,  1,               cfg_on_off             },
    { "init_enable",           0,  1,               cfg_on_off             },
    { "store_size",            0,  7,               cfg_size_list          },
    { "enable_cache",          0,  1,               cfg_on_off             },
    { "sdwam",                 0,  1,               cfg_on_off             },
    { "ptwam",                 0,  1,               cfg_on_off             },

    // Hacks
    { "dis_enable",            0,  1,               cfg_on_off             },
    // steady_clock was moved to SCU; keep here for script compatibility
    { "steady_clock",          0,  1,               cfg_on_off             },
    { "halt_on_unimplemented", 0,  1,               cfg_on_off             },
    { "enable_wam",            0,  1,               cfg_on_off             },
    { "report_faults",         0,  1,               cfg_on_off             },
    { "tro_enable",            0,  1,               cfg_on_off             },
    // y2k was moved to SCU; keep here for script compatibility
    { "y2k",                   0,  1,               cfg_on_off             },
    { "drl_fatal",             0,  1,               cfg_on_off             },
    { "useMap",                0,  1,               cfg_on_off             },
    { "address",               0,  0777777,         NULL                   },
    { "prom_installed",        0,  1,               cfg_on_off             },
    { "hex_mode_installed",    0,  1,               cfg_on_off             },
    { "cache_installed",       0,  1,               cfg_on_off             },
    { "clock_slave_installed", 0,  1,               cfg_on_off             },
    { "enable_emcall",         0,  1,               cfg_on_off             },

    // Tuning
#ifdef AFFINITY
    { "affinity",              -1, 32767,           cfg_affinity           },
#endif
    { "isolts_mode",           0,  1,               cfg_on_off             },
    { "nodis",                 0,  1,               cfg_on_off             },
    { "l68_mode",              0,  1,               cfg_l68_mode           },
    { NULL,                    0,  0,               NULL                   }
  };

static t_stat cpu_set_config (UNIT * uptr, UNUSED int32 value,
                              const char * cptr, UNUSED void * desc)
  {
    long cpu_unit_idx = UNIT_IDX (uptr);
    if (cpu_unit_idx < 0 || cpu_unit_idx >= N_CPU_UNITS_MAX)
      {
        sim_warn ("error: cpu_set_config: Invalid unit number %ld\n",
                    (long) cpu_unit_idx);
        return SCPE_ARG;
      }

    static int port_num = 0;

    config_state_t cfg_state = { NULL, NULL };

    for (;;)
      {
        int64_t v;
        int rc = cfg_parse (__func__, cptr, cpu_config_list,
                           & cfg_state, & v);
        if (rc == -1) // done
          {
            break;
          }
        if (rc == -2) // error
          {
            cfg_parse_done (& cfg_state);
            return SCPE_ARG;
          }

        const char * p = cpu_config_list [rc] . name;
        if (strcmp (p, "faultbase") == 0)
          cpus[cpu_unit_idx].switches.FLT_BASE = (uint) v;
        else if (strcmp (p, "num") == 0)
          cpus[cpu_unit_idx].switches.cpu_num = (uint) v;
        else if (strcmp (p, "data") == 0)
          cpus[cpu_unit_idx].switches.data_switches = (word36) v;
        else if (strcmp (p, "stopnum") == 0)
          {
            // set up for check stop
            // convert stopnum to bcd
            int64_t d1 = (v / 1000) % 10;
            int64_t d2 = (v /  100) % 10;
            int64_t d3 = (v /   10) % 10;
            int64_t d4 = (v /    1) % 10;
            word36 d = 0123000000000;
            putbits36_6 (& d,  9, (word4) d1);
            putbits36_6 (& d, 15, (word4) d2);
            putbits36_6 (& d, 21, (word4) d3);
            putbits36_6 (& d, 27, (word4) d4);
            cpus[cpu_unit_idx].switches.data_switches = d;
          }
        else if (strcmp (p, "address") == 0)
          cpus[cpu_unit_idx].switches.addr_switches = (word18) v;
        else if (strcmp (p, "mode") == 0)
          cpus[cpu_unit_idx].switches.procMode = v ? procModeMultics : procModeGCOS;
        else if (strcmp (p, "speed") == 0)
          cpus[cpu_unit_idx].options.proc_speed = (uint) v;
        else if (strcmp (p, "port") == 0) {
          if ((! cpus[cpu_unit_idx].tweaks.l68_mode) && (int) v > 4) {
            cfg_parse_done (& cfg_state);
            return SCPE_ARG;
          }
          port_num = (int) v;
        }
        else if (strcmp (p, "assignment") == 0)
          cpus[cpu_unit_idx].switches.assignment [port_num] = (uint) v;
        else if (strcmp (p, "interlace") == 0)
          cpus[cpu_unit_idx].switches.interlace [port_num] = (uint) v;
        else if (strcmp (p, "enable") == 0)
          cpus[cpu_unit_idx].switches.enable [port_num] = (uint) v;
        else if (strcmp (p, "init_enable") == 0)
          cpus[cpu_unit_idx].switches.init_enable [port_num] = (uint) v;
        else if (strcmp (p, "store_size") == 0) {
          if (v > 7) {
            if (cpus[cpu_unit_idx].tweaks.l68_mode) {
              switch (v) {
                case  8:  v = 0;   break; // 32K
                case  9:  v = 1;   break; // 64K
                case 10:  v = 3;   break; // 128K
                case 11:  v = 7;   break; // 256K
                case 12:  v = 4;   break; // 512K
                case 13:  v = 5;   break; // 1024K
                case 14:  v = 6;   break; // 2048K
                case 15:  v = 2;   break; // 4096K
              }
            } else {
              switch (v) {
                case  8:  v = 0;   break; // 32K
                case  9:  v = 1;   break; // 64K
                case 10:  v = 2;   break; // 128K
                case 11:  v = 3;   break; // 256K
                case 12:  v = 4;   break; // 512K
                case 13:  v = 5;   break; // 1024K
                case 14:  v = 6;   break; // 2048K
                case 15:  v = 7;   break; // 4096K
              }
            }
          }
          cpus[cpu_unit_idx].switches.store_size [port_num] = (uint) v;
        }
        else if (strcmp (p, "enable_cache") == 0)
          cpus[cpu_unit_idx].switches.enable_cache = (uint) v ? true : false;
        else if (strcmp (p, "sdwam") == 0)
          cpus[cpu_unit_idx].switches.sdwam_enable = (uint) v ? true : false;
        else if (strcmp (p, "ptwam") == 0)
          cpus[cpu_unit_idx].switches.ptwam_enable = (uint) v ? true : false;
        else if (strcmp (p, "dis_enable") == 0)
          cpus[cpu_unit_idx].tweaks.dis_enable = (uint) v;
        else if (strcmp (p, "steady_clock") == 0)
          scu [0].steady_clock = (uint) v;
        else if (strcmp (p, "halt_on_unimplemented") == 0)
          cpus[cpu_unit_idx].tweaks.halt_on_unimp = (uint) v;
        else if (strcmp (p, "enable_wam") == 0)
          cpus[cpu_unit_idx].tweaks.enable_wam = (uint) v;
        else if (strcmp (p, "report_faults") == 0)
          cpus[cpu_unit_idx].tweaks.report_faults = (uint) v;
        else if (strcmp (p, "tro_enable") == 0)
          cpus[cpu_unit_idx].tweaks.tro_enable = (uint) v;
        else if (strcmp (p, "y2k") == 0)
          scu [0].y2k = (uint) v;
        else if (strcmp (p, "drl_fatal") == 0)
          cpus[cpu_unit_idx].tweaks.drl_fatal = (uint) v;
        else if (strcmp (p, "useMap") == 0)
          cpus[cpu_unit_idx].tweaks.useMap = v;
        else if (strcmp (p, "prom_installed") == 0)
          cpus[cpu_unit_idx].options.prom_installed = v;
        else if (strcmp (p, "hex_mode_installed") == 0)
          cpus[cpu_unit_idx].options.hex_mode_installed = v;
        else if (strcmp (p, "cache_installed") == 0)
          cpus[cpu_unit_idx].options.cache_installed = v;
        else if (strcmp (p, "clock_slave_installed") == 0)
          cpus[cpu_unit_idx].options.clock_slave_installed = v;
        else if (strcmp (p, "enable_emcall") == 0)
          cpus[cpu_unit_idx].tweaks.enable_emcall = v;
#ifdef AFFINITY
        else if (strcmp (p, "affinity") == 0)
          if (v < 0)
            {
              cpus[cpu_unit_idx].set_affinity = false;
            }
          else
            {
              cpus[cpu_unit_idx].set_affinity = true;
              cpus[cpu_unit_idx].affinity = (uint) v;
            }
#endif
        else if (strcmp (p, "isolts_mode") == 0)
          {
            cpus[cpu_unit_idx].tweaks.isolts_mode = v;
            if (v)
              {
                uint store_sz;
                if (cpus[cpu_unit_idx].tweaks.l68_mode) // L68
                  store_sz = 3;
                else // DPS8M
                  store_sz = 2;
                cpus[cpu_unit_idx].isolts_switches_save     = cpus[cpu_unit_idx].switches;
                cpus[cpu_unit_idx].isolts_switches_saved    = true;

                cpus[cpu_unit_idx].switches.data_switches   = 00000030714000;
                cpus[cpu_unit_idx].switches.addr_switches   = 0100150;
                cpus[cpu_unit_idx].tweaks.useMap            = true;
                cpus[cpu_unit_idx].tweaks.enable_wam        = true;
                cpus[cpu_unit_idx].switches.assignment  [0] = 0;
                cpus[cpu_unit_idx].switches.interlace   [0] = false;
                cpus[cpu_unit_idx].switches.enable      [0] = false;
                cpus[cpu_unit_idx].switches.init_enable [0] = false;
                cpus[cpu_unit_idx].switches.store_size  [0] = store_sz;

                cpus[cpu_unit_idx].switches.assignment  [1] = 0;
                cpus[cpu_unit_idx].switches.interlace   [1] = false;
                cpus[cpu_unit_idx].switches.enable      [1] = true;
                cpus[cpu_unit_idx].switches.init_enable [1] = false;
                cpus[cpu_unit_idx].switches.store_size  [1] = store_sz;

                cpus[cpu_unit_idx].switches.assignment  [2] = 0;
                cpus[cpu_unit_idx].switches.interlace   [2] = false;
                cpus[cpu_unit_idx].switches.enable      [2] = false;
                cpus[cpu_unit_idx].switches.init_enable [2] = false;
                cpus[cpu_unit_idx].switches.store_size  [2] = store_sz;

                cpus[cpu_unit_idx].switches.assignment  [3] = 0;
                cpus[cpu_unit_idx].switches.interlace   [3] = false;
                cpus[cpu_unit_idx].switches.enable      [3] = false;
                cpus[cpu_unit_idx].switches.init_enable [3] = false;
                cpus[cpu_unit_idx].switches.store_size  [3] = store_sz;

                if (cpus[cpu_unit_idx].tweaks.l68_mode) { // L68
                  cpus[cpu_unit_idx].switches.assignment  [4] = 0;
                  cpus[cpu_unit_idx].switches.interlace   [4] = false;
                  cpus[cpu_unit_idx].switches.enable      [4] = false;
                  cpus[cpu_unit_idx].switches.init_enable [4] = false;
                  cpus[cpu_unit_idx].switches.store_size  [4] = 3;

                  cpus[cpu_unit_idx].switches.assignment  [5] = 0;
                  cpus[cpu_unit_idx].switches.interlace   [5] = false;
                  cpus[cpu_unit_idx].switches.enable      [5] = false;
                  cpus[cpu_unit_idx].switches.init_enable [5] = false;
                  cpus[cpu_unit_idx].switches.store_size  [5] = 3;

                  cpus[cpu_unit_idx].switches.assignment  [6] = 0;
                  cpus[cpu_unit_idx].switches.interlace   [6] = false;
                  cpus[cpu_unit_idx].switches.enable      [6] = false;
                  cpus[cpu_unit_idx].switches.init_enable [6] = false;
                  cpus[cpu_unit_idx].switches.store_size  [6] = 3;

                  cpus[cpu_unit_idx].switches.assignment  [7] = 0;
                  cpus[cpu_unit_idx].switches.interlace   [7] = false;
                  cpus[cpu_unit_idx].switches.enable      [7] = false;
                  cpus[cpu_unit_idx].switches.init_enable [7] = false;
                  cpus[cpu_unit_idx].switches.store_size  [7] = 3;
                }
                cpu_reset_unit_idx ((uint) cpu_unit_idx, false);
                simh_cpu_reset_and_clear_unit (cpu_unit + cpu_unit_idx, 0, NULL, NULL);
                cpus[cpu_unit_idx].switches.enable      [1] = true;
              }
            else
              {
                cpus[cpu_unit_idx].switches = cpus[cpu_unit_idx].isolts_switches_save;
                cpus[cpu_unit_idx].isolts_switches_saved    = false;

                cpu_reset_unit_idx ((uint) cpu_unit_idx, false);
                simh_cpu_reset_and_clear_unit (cpu_unit + cpu_unit_idx, 0, NULL, NULL);
              }
          }
        else if (strcmp (p, "nodis") == 0)
          cpus[cpu_unit_idx].tweaks.nodis = v;
        else if (strcmp (p, "l68_mode") == 0)
          cpus[cpu_unit_idx].tweaks.l68_mode= v;
        else
          {
            sim_warn ("error: cpu_set_config: Invalid cfg_parse rc <%ld>\n",
                        (long) rc);
            cfg_parse_done (& cfg_state);
            return SCPE_ARG;
          }
      } // process statements
    cfg_parse_done (& cfg_state);

    return SCPE_OK;
  }

static t_stat cpu_show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr,
                               UNUSED int val, UNUSED const void * desc)
  {
    sim_msg ("Number of CPUs in system is %d\n", cpu_dev.numunits);
    return SCPE_OK;
  }

static t_stat cpu_set_nunits (UNUSED UNIT * uptr, UNUSED int32 value,
                              const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    int n = atoi (cptr);
    if (n < 1 || n > N_CPU_UNITS_MAX)
      return SCPE_ARG;
    cpu_dev.numunits = (uint32) n;
    return SCPE_OK;
  }

static t_stat cpu_show_kips (UNUSED FILE * st, UNUSED UNIT * uptr,
                             UNUSED int val, UNUSED const void * desc)
  {
    sim_msg ("CPU KIPS %u\n", kips);
    return SCPE_OK;
  }

static t_stat cpu_set_kips (UNUSED UNIT * uptr, UNUSED int32 value,
                            const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    int n = atoi (cptr);
    if (n < 1 || n > 4000000)
      return SCPE_ARG;
    kips = (uint) n;
    luf_limits[0] =  2000*kips/1000;
    luf_limits[1] =  4000*kips/1000;
    luf_limits[2] =  8000*kips/1000;
    luf_limits[3] = 16000*kips/1000;
    luf_limits[4] = 32000*kips/1000;
    return SCPE_OK;
  }

static t_stat cpu_show_stall (UNUSED FILE * st, UNUSED UNIT * uptr,
                             UNUSED int val, UNUSED const void * desc)
  {
    if (! stall_point_active)
      {
        sim_printf ("No stall points\n");
        return SCPE_OK;
      }

    sim_printf ("Stall points\n");
    for (int i = 0; i < N_STALL_POINTS; i ++)
      if (stall_points[i].segno || stall_points[i].offset)
        {
#ifdef WIN_STDIO
          sim_printf ("%2ld %05o:%06o %10lu\n",
#else
          sim_printf ("%2ld %05o:%06o %'10lu\n",
#endif
                 (long)i, stall_points[i].segno, stall_points[i].offset,
                 (unsigned long)stall_points[i].time);
        }
    return SCPE_OK;
  }

// set cpu stall=n=s:o=t
//   n stall point number
//   s segment number (octal)
//   o offset (octal)
//   t time in microseconds (decimal)

static t_stat cpu_set_stall (UNUSED UNIT * uptr, UNUSED int32 value,
                             const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;

    long n, s, o, t;

    char * end;
    n = strtol (cptr, & end, 0);
    if (* end != '=')
      return SCPE_ARG;
    if (n < 0 || n >= N_STALL_POINTS)
      return SCPE_ARG;

    s = strtol (end + 1, & end, 8);
    if (* end != ':')
      return SCPE_ARG;
    if (s < 0 || s > MASK15)
      return SCPE_ARG;

    o = strtol (end + 1, & end, 8);
    if (* end != '=')
      return SCPE_ARG;
    if (o < 0 || o > MASK18)
      return SCPE_ARG;

    t = strtol (end + 1, & end, 0);
    if (* end != 0)
      return SCPE_ARG;
    if (t < 0 || t > 30000000)
      return SCPE_ARG;

    stall_points[n].segno  = (word15) s;
    stall_points[n].offset = (word18) o;
    stall_points[n].time   = (unsigned int) t;
    stall_point_active     = false;

    for (int i = 0; i < N_STALL_POINTS; i ++)
      if (stall_points[n].segno && stall_points[n].offset)
        stall_point_active = true;

    return SCPE_OK;
  }

static t_stat setCPUConfigL68 (UNIT * uptr, UNUSED int32 value, UNUSED const char * cptr, UNUSED void * desc) {
  long cpuUnitIdx = UNIT_IDX (uptr);
  if (cpuUnitIdx < 0 || cpuUnitIdx >= N_CPU_UNITS_MAX)
    return SCPE_ARG;
  cpu_state_t * cpun = cpus + cpuUnitIdx;

  cpun->tweaks.l68_mode = 1;
  cpun->options.hex_mode_installed = 0;
  for (uint port_num = 0; port_num < N_DPS8M_CPU_PORTS; port_num ++) {
    cpun->switches.assignment[port_num] = port_num;
    cpun->switches.interlace[port_num] = 0;
    cpun->switches.store_size[port_num] = 2;
    cpun->switches.enable[port_num] = 1;
    cpun->switches.init_enable[port_num] = 1;
  }
  for (uint port_num = N_DPS8M_CPU_PORTS; port_num < N_L68_CPU_PORTS; port_num ++) {
    cpun->switches.assignment[port_num] = 0;
    cpun->switches.interlace[port_num] = 0;
    cpun->switches.store_size[port_num] = 0;
    cpun->switches.enable[port_num] = 0;
    cpun->switches.init_enable[port_num] = 0;
  }
  return SCPE_OK;
}

static t_stat setCPUConfigDPS8M (UNIT * uptr, UNUSED int32 value, UNUSED const char * cptr, UNUSED void * desc) {
  long cpuUnitIdx = UNIT_IDX (uptr);
  if (cpuUnitIdx < 0 || cpuUnitIdx >= N_CPU_UNITS_MAX)
    return SCPE_ARG;
  cpu_state_t * cpun = cpus + cpuUnitIdx;

  cpun->tweaks.l68_mode = 0;
  cpun->options.hex_mode_installed = 0;
  for (uint port_num = 0; port_num < N_DPS8M_CPU_PORTS; port_num ++) {
    cpun->switches.assignment[port_num] = port_num;
    cpun->switches.interlace[port_num] = 0;
    cpun->switches.store_size[port_num] = 7;
    cpun->switches.enable[port_num] = 1;
    cpun->switches.init_enable[port_num] = 1;
  }
  for (uint port_num = N_DPS8M_CPU_PORTS; port_num < N_L68_CPU_PORTS; port_num ++) {
    cpun->switches.assignment[port_num] = 0;
    cpun->switches.interlace[port_num] = 0;
    cpun->switches.store_size[port_num] = 0;
    cpun->switches.enable[port_num] = 0;
    cpun->switches.init_enable[port_num] = 0;
  }
  return SCPE_OK;
}

static char * cycle_str (cycles_e cycle)
  {
    switch (cycle)
      {
        //case ABORT_cycle:
          //return "ABORT_cycle";
        case FAULT_cycle:
          return "FAULT_cycle";
        case EXEC_cycle:
          return "EXEC_cycle";
        case FAULT_EXEC_cycle:
          return "FAULT_EXEC_cycle";
        case INTERRUPT_cycle:
          return "INTERRUPT_cycle";
        case INTERRUPT_EXEC_cycle:
          return "INTERRUPT_EXEC_cycle";
        case FETCH_cycle:
          return "FETCH_cycle";
        case PSEUDO_FETCH_cycle:
          return "PSEUDO_FETCH_cycle";
        case SYNC_FAULT_RTN_cycle:
          return "SYNC_FAULT_RTN_cycle";
        default:
          return "unknown cycle";
      }
  }

static void set_cpu_cycle (cycles_e cycle)
  {
    sim_debug (DBG_CYCLE, & cpu_dev, "Setting cycle to %s\n",
               cycle_str (cycle));
    cpu.cycle = cycle;
  }

// DPS8M Memory of 36 bit words is implemented as an array of 64 bit words.
// Put state information into the unused high order bits.
#define MEM_UNINITIALIZED (1LLU<<62)

uint set_cpu_idx (UNUSED uint cpu_idx)
  {
    uint prev = current_running_cpu_idx;
#if defined(THREADZ) || defined(LOCKLESS)
    current_running_cpu_idx = cpu_idx;
#endif
#ifdef ROUND_ROBIN
    current_running_cpu_idx = cpu_idx;
#endif
    cpup = & cpus [current_running_cpu_idx];
    return prev;
  }

void cpu_reset_unit_idx (UNUSED uint cpun, bool clear_mem)
  {
    uint save = set_cpu_idx (cpun);
    if (clear_mem)
      {
        for (uint i = 0; i < MEMSIZE; i ++)
          {
            // Clear lock bits and data field; set uninitialized
#ifdef LOCKLESS
            M[i] = (M[i] & ~(MASK36 | MEM_LOCKED)) | MEM_UNINITIALIZED;
#else
            M[i] = (M[i] & ~(MASK36)) | MEM_UNINITIALIZED;
#endif
          }
      }
    cpu.rA = 0;
    cpu.rQ = 0;

    cpu.PPR.IC   = 0;
    cpu.PPR.PRR  = 0;
    cpu.PPR.PSR  = 0;
    cpu.PPR.P    = 1;
    cpu.RSDWH_R1 = 0;
    cpu.rTR      = MASK27;

    if (cpu.tweaks.isolts_mode)
      {
        cpu.shadowTR = 0;
        cpu.rTRlsb   = 0;
      }
    cpu.rTRticks = 0;

    set_addr_mode (ABSOLUTE_mode);
    SET_I_NBAR;

    cpu.CMR.luf  = 3;    // default of 16 mS
    cpu.cu.SD_ON = cpu.switches.sdwam_enable ? 1 : 0;
    cpu.cu.PT_ON = cpu.switches.ptwam_enable ? 1 : 0;

    if (cpu.tweaks.nodis) {
      set_cpu_cycle (FETCH_cycle);
    } else {
      set_cpu_cycle (EXEC_cycle);
      cpu.cu.IWB = 0000000616000; //-V536  // Stuff DIS instruction in instruction buffer
    }
#ifdef PERF_STRIP
    set_cpu_cycle (FETCH_cycle);
#endif
    cpu.wasXfer        = false;
    cpu.wasInhibited   = false;

    cpu.interrupt_flag = false;
    cpu.g7_flag        = false;

    cpu.faultRegister [0] = 0;
    cpu.faultRegister [1] = 0;

#ifdef RAPRx
    cpu.apu.lastCycle = UNKNOWN_CYCLE;
#endif

    (void)memset (& cpu.PPR, 0, sizeof (struct ppr_s));

    setup_scbank_map ();

    tidy_cu ();
    set_cpu_idx (save);
  }

static t_stat simh_cpu_reset_and_clear_unit (UNIT * uptr,
                                             UNUSED int32 value,
                                             UNUSED const char * cptr,
                                             UNUSED void * desc)
  {
    long cpu_unit_idx = UNIT_IDX (uptr);
    cpu_state_t * cpun = cpus + cpu_unit_idx;
    if (cpun->tweaks.isolts_mode)
      {
        // Currently isolts_mode requires useMap, so this is redundant
        if (cpun->tweaks.useMap)
          {
            for (uint pgnum = 0; pgnum < N_SCBANKS; pgnum ++)
              {
                int base = cpun->sc_addr_map [pgnum];
                if (base < 0)
                  continue;
                for (uint addr = 0; addr < SCBANK_SZ; addr ++)
                  M [addr + (uint) base] = MEM_UNINITIALIZED;
              }
          }
      }
    // Crashes console?
    cpu_reset_unit_idx ((uint) cpu_unit_idx, false);
    return SCPE_OK;
  }

static t_stat simh_cpu_reset_unit (UNIT * uptr,
                                   UNUSED int32 value,
                                   UNUSED const char * cptr,
                                   UNUSED void * desc)
  {
    long cpu_unit_idx = UNIT_IDX (uptr);
    cpu_reset_unit_idx ((uint) cpu_unit_idx, false); // no clear memory
    return SCPE_OK;
  }

#ifndef PERF_STRIP
static uv_loop_t * ev_poll_loop;
static uv_timer_t ev_poll_handle;
#endif /* ifndef PERF_STRIP */

static MTAB cpu_mod[] =
  {
    {
      MTAB_unit_value,                /* Mask               */
      0,                              /* Match              */
      "CONFIG",                       /* Print string       */
      "CONFIG",                       /* Match string       */
      cpu_set_config,                 /* Validation routine */
      cpu_show_config,                /* Display routine    */
      NULL,                           /* Value descriptor   */
      NULL                            /* Help               */
    },

// RESET (INITIALIZE) -- reset CPU

    {
      MTAB_unit_value,                /* Mask               */
      0,                              /* Match              */
      "RESET",                        /* Print string       */
      "RESET",                        /* Match string       */
      simh_cpu_reset_unit,            /* Validation routine */
      NULL,                           /* Display routine    */
      NULL,                           /* Value descriptor   */
      NULL                            /* Help               */
    },

    {
      MTAB_unit_value,                /* Mask               */
      0,                              /* Match              */
      "INITIALIZE",                   /* Print string       */
      "INITIALIZE",                   /* Match string       */
      simh_cpu_reset_unit,            /* Validation routine */
      NULL,                           /* Display routine    */
      NULL,                           /* Value descriptor   */
      NULL                            /* Help               */
    },

// INITAILIZEANDCLEAR (IAC) -- reset CPU, clear Memory

    {
      MTAB_unit_value,                /* Mask               */
      0,                              /* Match              */
      "INITIALIZEANDCLEAR",           /* Print string       */
      "INITIALIZEANDCLEAR",           /* Match string       */
      simh_cpu_reset_and_clear_unit,  /* Validation routine */
      NULL,                           /* Display routine    */
      NULL,                           /* Value descriptor   */
      NULL                            /* Help               */
    },

    {
      MTAB_unit_value,                /* Mask               */
      0,                              /* Match              */
      "IAC",                          /* Print string       */
      "IAC",                          /* Match string       */
      simh_cpu_reset_and_clear_unit,  /* Validation routine */
      NULL,                           /* Display routine    */
      NULL,                           /* Value descriptor   */
      NULL                            /* Help               */
    },

    {
      MTAB_dev_value,                 /* Mask               */
      0,                              /* Match              */
      "NUNITS",                       /* Print string       */
      "NUNITS",                       /* Match string       */
      cpu_set_nunits,                 /* Validation routine */
      cpu_show_nunits,                /* Display routine    */
      NULL,                           /* Value descriptor   */
      NULL                            /* Help               */
    },

    {
      MTAB_dev_value,                 /* Mask               */
      0,                              /* Match              */
      "KIPS",                         /* Print string       */
      "KIPS",                         /* Match string       */
      cpu_set_kips,                   /* Validation routine */
      cpu_show_kips,                  /* Display routine    */
      NULL,                           /* Value descriptor   */
      NULL                            /* Help               */
    },

    {
      MTAB_dev_value,                 /* Mask               */
      0,                              /* Match              */
      "STALL",                        /* Print string       */
      "STALL",                        /* Match string       */
      cpu_set_stall,                  /* Validation routine */
      cpu_show_stall,                 /* Display routine    */
      NULL,                           /* Value descriptor   */
      NULL                            /* Help               */
    },

    {
      MTAB_unit_value,                /* Mask               */
      0,                              /* Match              */
      "DPS8M",                        /* Print string       */
      "DPS8M",                        /* Match string       */
      setCPUConfigDPS8M,              /* Validation routine */
      NULL,                           /* Display routine    */
      NULL,                           /* Value descriptor   */
      NULL                            /* Help               */
    },

    {
      MTAB_unit_value,                /* Mask               */
      0,                              /* Match              */
      "L68",                          /* Print string       */
      "L68",                          /* Match string       */
      setCPUConfigL68,                /* Validation routine */
      NULL,                           /* Display routine    */
      NULL,                           /* Value descriptor   */
      NULL                            /* Help               */
    },

    { 0, 0, NULL, NULL, NULL, NULL, NULL, NULL }
  };

static DEBTAB cpu_dt[] =
  {
    { "TRACE",       DBG_TRACE,       NULL },
    { "TRACEEXT",    DBG_TRACEEXT,    NULL },
    { "MESSAGES",    DBG_MSG,         NULL },

    { "REGDUMPAQI",  DBG_REGDUMPAQI,  NULL },
    { "REGDUMPIDX",  DBG_REGDUMPIDX,  NULL },
    { "REGDUMPPR",   DBG_REGDUMPPR,   NULL },
    { "REGDUMPPPR",  DBG_REGDUMPPPR,  NULL },
    { "REGDUMPDSBR", DBG_REGDUMPDSBR, NULL },
    { "REGDUMPFLT",  DBG_REGDUMPFLT,  NULL }, // Don't move as it messes up DBG message
    { "REGDUMP",     DBG_REGDUMP,     NULL },

    { "ADDRMOD",     DBG_ADDRMOD,     NULL },
    { "APPENDING",   DBG_APPENDING,   NULL },

    { "NOTIFY",      DBG_NOTIFY,      NULL },
    { "INFO",        DBG_INFO,        NULL },
    { "ERR",         DBG_ERR,         NULL },
    { "WARN",        DBG_WARN,        NULL },
    { "DEBUG",       DBG_DEBUG,       NULL }, // Don't move as it messes up DBG message
    { "ALL",         DBG_ALL,         NULL },

    { "FAULT",       DBG_FAULT,       NULL },
    { "INTR",        DBG_INTR,        NULL },
    { "CORE",        DBG_CORE,        NULL },
    { "CYCLE",       DBG_CYCLE,       NULL },
    { "CAC",         DBG_CAC,         NULL },
    { "FINAL",       DBG_FINAL,       NULL },
    { "AVC",         DBG_AVC,         NULL },
    { NULL,          0,               NULL }
  };

// This is part of the scp interface
const char *sim_stop_messages[] =
  {
    "Unknown error",           // SCPE_OK
    "Simulation stop",         // STOP_STOP
    "Breakpoint",              // STOP_BKPT
  };

/* End of scp interface */

/* Processor configuration switches
 *
 * From AM81-04 Multics System Maintenance Procedures
 *
 * "A Level 68 IOM system may contain a maximum of 7 CPUs, 4 IOMs, 8 SCUs,
 * and 16MW of memory ...
 * [CAC]: ... but AN87 says Multics only supports two IOMs
 *
 * ASSIGNMENT: 3 toggle switches determine the base address of the SCU
 * connected to the port. The base address (in KW) is the product of this
 * number and the value defined by the STORE SIZE patch plug for the port.
 *
 * ADDRESS RANGE: toggle FULL/HALF. Determines the size of the SCU as full or
 * half of the STORE SIZE patch.
 *
 * PORT ENABLE: (4? toggles)
 *
 * INITIALIZE ENABLE: (4? toggles) These switches enable the receipt of an
 * initialize signal from the SCU connected to the ports. This signal is used
 * during the first part of bootload to set all CPUs to a known (idle) state.
 * The switch for each port connected to an SCU should be ON, otherwise off.
 *
 * INTERLACE: ... All INTERLACE switches should be OFF for Multics operation.
 *
 */

#ifndef SPEED
static bool watch_bits [MEMSIZE];
#endif

char * str_SDW0 (char * buf, sdw0_s * SDW)
  {
    (void)sprintf (buf, "ADDR=%06o R1=%o R2=%o R3=%o F=%o FC=%o BOUND=%o R=%o "
                        "E=%o W=%o P=%o U=%o G=%o C=%o EB=%o",
                   SDW->ADDR, SDW->R1,    SDW->R2, SDW->R3, SDW->DF,
                   SDW->FC,   SDW->BOUND, SDW->R,  SDW->E,  SDW->W,
                   SDW->P,    SDW->U,     SDW->G,  SDW->C,  SDW->EB);
    return buf;
  }

static t_stat cpu_boot (UNUSED int32 cpu_unit_idx, UNUSED DEVICE * dptr)
  {
    sim_warn ("Try 'BOOT IOMn'\n");
    return SCPE_ARG;
  }

// The original h/w had one to four (DPS8/M) or eight (Level 68) SCUs;
// each SCU held memory.
// Memory accesses were sent to the SCU that held the region of memory
// being addressed.
//
// eg, SCU 0 has 1 MW of memory and SCU 1 has 2 MW
// Address
//      0M +------------------+
//         |                  |  SCU 0
//      1M +------------------+
//         |                  |  SCU 1
//         |                  |
//      3M +------------------+
//
// So SCU 0 has the first MW of addresses, and SCU1 has the second and third
// MWs.
//
// The simulator has a single 16MW array of memory. This code walks the SCUs
// allocates memory regions out of that array to the SCUs based on their
// individual configurations. The 16MW is divided into 4 zones, one for each
// SCU. (SCU0 uses the first 4MW, SCU1 the second 4MW, etc.
//
#define ZONE_SZ (MEM_SIZE_MAX / 4)
//
// The minimum SCU memory size increment is 64KW, which I will refer to as
// a 'bank'. To map a CPU address to the simulated array, the CPU address is
// divided into a bank number and an offset into that bank
//
//    bank_num    = addr / SCBANK_SZ
//    bank_offset = addr % SCBANK_SZ
//
// sc_addr_map[] maps bank numbers to offset in the simulated memory array
//
//    real_addr = sc_addr_map[bank_num] + bank_offset
//

void setup_scbank_map (void)
  {
    // Initialize to unmapped
    for (uint pg = 0; pg < N_SCBANKS; pg ++)
      {
        cpu.sc_addr_map [pg] = -1;
        cpu.sc_scu_map  [pg] = -1;
      }
    for (uint u = 0; u < N_SCU_UNITS_MAX; u ++)
      cpu.sc_num_banks[u] = 0;

    // For each port
    for (int port_num = 0; port_num < (cpu.tweaks.l68_mode ? N_L68_CPU_PORTS : N_DPS8M_CPU_PORTS); port_num ++)
      {
        // Ignore disabled ports
        if (! cpu.switches.enable [port_num])
          continue;

        // Ignore disconnected ports
        // This will happen during early initialization,
        // before any cables are run.
        if (! cables->cpu_to_scu[current_running_cpu_idx][port_num].in_use)
          {
            continue;
          }

        // Calculate the amount of memory in the SCU in words
        uint store_size = cpu.switches.store_size [port_num];
        uint dps8m_store_table [8] =
          { 32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304 };
// ISOLTS sez:
// for DPS88:
//   3. set store size switches to 2222.
// for L68:
//   3. remove the right free-edge connector on the 645pq wwb at slot ab28.
//
// During ISOLTS initialization, it requires that the memory switch be set to
// '3' for all eight ports; this corresponds to '2' for the DPS8M (131072)
// Then:
// isolts: a "lda 65536" (64k) failed to produce a store fault
//
// So it seems that the memory size is expected to be 64K, not 128K as per
// the switches; presumably step 3 causes this. Fake it by tweaking store table:
//
        uint l68_store_table [8] =
          { 32768, 65536, 4194304, 131072, 524288, 1048576, 2097152, 262144 };
        uint l68_isolts_store_table [8] =
          { 32768, 65536, 4194304, 65536, 524288, 1048576, 2097152, 262144 };

        uint sz_wds =
          cpu.tweaks.l68_mode ?
            cpu.tweaks.isolts_mode ?
              l68_isolts_store_table [store_size] :
                l68_store_table [store_size] :
          dps8m_store_table [store_size];

        // Calculate the base address that will be assigned to the SCU
        uint base_addr_wds = sz_wds * cpu.switches.assignment[port_num];

        // Now convert to SCBANK_SZ (number of banks)
        uint num_banks             = sz_wds / SCBANK_SZ;
        cpu.sc_num_banks[port_num] = num_banks;
        uint base_addr_bks         = base_addr_wds / SCBANK_SZ;

        // For each page handled by the SCU
        for (uint pg = 0; pg < num_banks; pg ++)
          {
            // What is the address of this bank?
            uint addr_bks = base_addr_bks + pg;
            // Past the end of memory?
            if (addr_bks < N_SCBANKS)
              {
                // Has this address been already assigned?
                if (cpu.sc_addr_map [addr_bks] != -1)
                  {
                    sim_warn ("scbank overlap addr_bks %d (%o) old port %d "
                                "newport %d\n",
                                addr_bks, addr_bks, cpu.sc_addr_map [addr_bks], port_num);
                  }
                else
                  {
                    // Assign it
                    cpu.sc_addr_map[addr_bks] = (int)((int)port_num * (int)ZONE_SZ + (int)pg * (int)SCBANK_SZ);
                    cpu.sc_scu_map[addr_bks]  = port_num;
                  }
              }
            else
              {
                sim_warn ("addr_bks too big port %d addr_bks %d (%o), "
                            "limit %d (%o)\n",
                            port_num, addr_bks, addr_bks, N_SCBANKS, N_SCBANKS);
              }
          }

      } // for port_num

    //for (uint pg = 0; pg < N_SCBANKS; pg ++)
     //sim_printf ("pg %o map: %08o\n", pg, cpu.sc_addr_map[pg]);
  } // sc_bank_map

int lookup_cpu_mem_map (word24 addr)
  {
    uint scpg = addr / SCBANK_SZ;
    if (scpg < N_SCBANKS)
      {
        return cpu.sc_scu_map[scpg];
      }
    return -1;
  }

//
// serial.txt format
//
//      sn:  number[,number]
//
//  Additional numbers will be for multi-cpu systems.
//  XXX: Other fields to be added.

#ifndef PERF_STRIP
static void get_serial_number (void)
  {
      bool havesn = false;
      FILE * fp = fopen ("./serial.txt", "r");
    while (fp && ! feof (fp))
      {
        char buffer [81] = "";
        char * checksn = fgets (buffer, sizeof (buffer), fp);
        (void)checksn;
        uint cpun, sn;
        if (sscanf (buffer, "sn: %u", & cpu.switches.serno) == 1)
          {
                        if (!sim_quiet)
                          {
                             sim_msg ("%s CPU serial number: %u\n", sim_name, cpu.switches.serno);
                          }
                        havesn = true;
          }
        else if (sscanf (buffer, "sn%u: %u", & cpun, & sn) == 2)
          {
            if (cpun < N_CPU_UNITS_MAX)
              {
                cpus[cpun].switches.serno = sn;
                                if (!sim_quiet)
                                  {
                                     sim_msg ("%s CPU %u serial number: %u\n",
                                     sim_name, cpun, cpus[cpun].switches.serno);
                                  }
                havesn = true;
              }
          }
      }
    if (!havesn)
      {
                if (!sim_quiet)
                  {
                     sim_msg ("\r\nPlease register your system at "
                              "https://ringzero.wikidot.com/wiki:register\n");
                     sim_msg ("or create the file 'serial.txt' containing the line "
                              "'sn: 0'.\r\n\r\n");
                  }
      }
    if (fp)
      fclose (fp);
  }
#endif /* ifndef PERF_STRIP */

#ifdef MACOSXPPC
# undef STATS
#endif /* ifdef MACOSXPPC */

#ifdef STATS
static void do_stats (void)
  {
    static struct timespec stats_time;
    static bool first = true;
    if (first)
      {
        first = false;
        clock_gettime (CLOCK_BOOTTIME, & stats_time);
        sim_msg ("stats started\r\n");
      }
    else
      {
        struct timespec now, delta;
        clock_gettime (CLOCK_BOOTTIME, & now);
        timespec_diff (& stats_time, & now, & delta);
        stats_time = now;
        sim_msg ("stats %6ld.%02ld\r\n", delta.tv_sec,
                    delta.tv_nsec / 10000000);

        sim_msg ("Instruction counts\r\n");
        for (uint i = 0; i < 8; i ++)
          {
# ifdef WIN_STDIO
            sim_msg (" %9lld\r\n", (long long int) cpus[i].instrCnt);
# else
            sim_msg (" %'9lld\r\n", (long long int) cpus[i].instrCnt);
# endif /* ifdef WIN_STDIO */
            cpus[i].instrCnt = 0;
          }
        sim_msg ("\r\n");
      }
  }
#endif

// The 100Hz timer has expired; poll I/O

#ifndef PERF_STRIP
static void ev_poll_cb (UNUSED uv_timer_t * handle)
  {
    // Call the one hertz stuff every 100 loops
    static uint oneHz = 0;
    if (oneHz ++ >= sys_opts.sys_slow_poll_interval) // ~ 1Hz
      {
        oneHz = 0;
        rdrProcessEvent ();
# ifdef STATS
        do_stats ();
# endif
        cpu.instrCntT0 = cpu.instrCntT1;
        cpu.instrCntT1 = cpu.instrCnt;
      }
    fnpProcessEvent ();
# ifdef WITH_SOCKET_DEV
#  ifndef __MINGW64__
#   ifndef __MINGW32__
#    ifndef CROSS_MINGW32
#     ifndef CROSS_MINGW64
    sk_process_event ();
#     endif /* ifndef CROSS_MINGW64 */
#    endif /* ifndef CROSS_MINGW32 */
#   endif /* ifndef __MINGW32__ */
#  endif /* ifndef __MINGW64__ */
# endif /* ifdef WITH_SOCKET_DEV */
    consoleProcess ();
# ifdef IO_ASYNC_PAYLOAD_CHAN
    iomProcess ();
# endif
# ifdef WITH_ABSI_DEV
#  ifndef __MINGW32__
#   ifndef __MINGW64__
#    ifndef CROSS_MINGW32
#     ifndef CROSS_MINGW64
    absi_process_event ();
#     endif /* ifndef CROSS_MINGW64 */
#    endif /* ifndef CROSS_MINGW32 */
#   endif /* ifndef __MINGW64__ */
#  endif /* ifndef __MINGW32__ */
# endif /* ifdef WITH_ABSI_DEV */
# ifdef WITH_MGP_DEV
#  ifndef __MINGW32__
#   ifndef __MINGW64__
#    ifndef CROSS_MINGW32
#     ifndef CROSS_MINGW64
    mgp_process_event ();
#     endif /* ifndef CROSS_MINGW64 */
#    endif /* ifndef CROSS_MINGW32 */
#   endif /* ifndef __MINGW64__ */
#  endif /* ifndef __MINGW32__ */
# endif /* ifdef WITH_MGP_DEV */
    PNL (panel_process_event ());
  }
#endif /* ifndef PERF_STRIP */

// called once initialization

void cpu_init (void)
  {

// !!!! Do not use 'cpu' in this routine; usage of 'cpus' violates 'restrict'
// !!!! attribute

    M = system_state->M;
#ifdef M_SHARED
    cpus = system_state->cpus;
#endif

#ifndef SPEED
    (void)memset (& watch_bits, 0, sizeof (watch_bits));
#endif

    set_cpu_idx (0);

    (void)memset (cpus, 0, sizeof (cpu_state_t) * N_CPU_UNITS_MAX);
    cpus [0].switches.FLT_BASE = 2; // Some of the UnitTests assume this

#ifndef PERF_STRIP
    get_serial_number ();

    ev_poll_loop = uv_default_loop ();
    uv_timer_init (ev_poll_loop, & ev_poll_handle);
    // 10 ms == 100Hz
    uv_timer_start (& ev_poll_handle, ev_poll_cb, sys_opts.sys_poll_interval, sys_opts.sys_poll_interval);
#endif
    // TODO: reset *all* other structures to zero

    cpu.instrCnt = 0;
    cpu.cycleCnt = 0;
    for (int i = 0; i < N_FAULTS; i ++)
      cpu.faultCnt [i] = 0;

#ifdef MATRIX
    initializeTheMatrix ();
#endif
  }

static void cpu_reset (void)
  {
    for (uint i = 0; i < N_CPU_UNITS_MAX; i ++)
      {
        cpu_reset_unit_idx (i, true);
      }

    set_cpu_idx (0);

    sim_debug (DBG_INFO, & cpu_dev, "CPU reset: Running\n");

  }

static t_stat sim_cpu_reset (UNUSED DEVICE *dptr)
  {
    //(void)memset (M, -1, MEMSIZE * sizeof (word36));

    // Fill DPS8M memory with zeros, plus a flag only visible to the emulator
    // marking the memory as uninitialized.

    cpu_reset ();
    return SCPE_OK;
  }

/* Memory examine */
//  t_stat examine_routine (t_val *eval_array, t_addr addr, UNIT *uptr, int32
//  switches)
//  Copy  sim_emax consecutive addresses for unit uptr, starting
//  at addr, into eval_array. The switch variable has bit<n> set if the n'th
//  letter was specified as a switch to the examine command.
// Not true...

static t_stat cpu_ex (t_value *vptr, t_addr addr, UNUSED UNIT * uptr,
                      UNUSED int32 sw)
  {
    if (addr>= MEMSIZE)
        return SCPE_NXM;
    if (vptr != NULL)
      {
        *vptr = M[addr] & DMASK;
      }
    return SCPE_OK;
  }

/* Memory deposit */

static t_stat cpu_dep (t_value val, t_addr addr, UNUSED UNIT * uptr,
                       UNUSED int32 sw)
  {
    if (addr >= MEMSIZE) return SCPE_NXM;
    M[addr] = val & DMASK;
    return SCPE_OK;
  }

/*
 * register stuff ...
 */

#ifdef M_SHARED
// scp has to have a statically allocated IC to refer to.
static word18 dummy_IC;
#endif

static REG cpu_reg[] =
  {
    // IC must be the first; see sim_PC.
#ifdef M_SHARED
    { ORDATA (IC, dummy_IC,       VASIZE), 0, 0, 0 },
#else
    { ORDATA (IC, cpus[0].PPR.IC, VASIZE), 0, 0, 0 },
#endif
    { NULL, NULL, 0, 0, 0, 0,  NULL, NULL, 0, 0, 0 }
  };

/*
 * scp interface
 */

REG *sim_PC = & cpu_reg[0];

/* CPU device descriptor */

DEVICE cpu_dev =
  {
    "CPU",          // name
    cpu_unit,       // units
    cpu_reg,        // registers
    cpu_mod,        // modifiers
    N_CPU_UNITS,    // #units
    8,              // address radix
    PASIZE,         // address width
    1,              // addr increment
    8,              // data radix
    36,             // data width
    & cpu_ex,       // examine routine
    & cpu_dep,      // deposit routine
    & sim_cpu_reset,// reset routine
    & cpu_boot,     // boot routine
    NULL,           // attach routine
    NULL,           // detach routine
    NULL,           // context
    DEV_DEBUG,      // device flags
    0,              // debug control flags
    cpu_dt,         // debug flag names
    NULL,           // memory size change
    NULL,           // logical name
    NULL,           // help
    NULL,           // attach help
    NULL,           // help context
    NULL,           // description
    NULL
  };

void printPtid(pthread_t pt)
{
  unsigned char *ptc = (unsigned char*)(void*)(&pt);
  sim_msg ("\r  Thread ID: 0x");
  for (size_t i=0; i < sizeof( pt ); i++)
    {
      sim_msg ("%02x", (unsigned)(ptc[i]));
    }
  sim_msg ("\r\n");
#ifdef __APPLE__
  sim_msg ("\r   Mach TID: 0x%x\r\n",
           pthread_mach_thread_np( pt ));
#endif /* ifdef __APPLE__ */
}

#ifdef M_SHARED
cpu_state_t * cpus = NULL;
#else
cpu_state_t cpus [N_CPU_UNITS_MAX];
#endif
#if defined(THREADZ) || defined(LOCKLESS)
__thread cpu_state_t * restrict cpup;
#else
cpu_state_t * restrict cpup;
#endif
#ifdef ROUND_ROBIN
uint current_running_cpu_idx;
#endif

// Scan the SCUs; it one has an interrupt present, return the fault pair
// address for the highest numbered interrupt on that SCU. If no interrupts
// are found, return 1.

// Called with SCU lock set

static uint get_highest_intr (void)
  {
    uint fp = 1;
    for (uint scu_unit_idx = 0; scu_unit_idx < N_SCU_UNITS_MAX; scu_unit_idx ++)
      {
        if (cpu.events.XIP [scu_unit_idx])
          {
            fp = scu_get_highest_intr (scu_unit_idx); // CALLED WITH SCU LOCK
            if (fp != 1)
              break;
          }
      }
    return fp;
  }

bool sample_interrupts (void)
  {
    cpu.lufCounter = 0;
    for (uint scu_unit_idx = 0; scu_unit_idx < N_SCU_UNITS_MAX; scu_unit_idx ++)
      {
        if (cpu.events.XIP [scu_unit_idx])
          {
            return true;
          }
      }
    return false;
  }

t_stat simh_hooks (void)
  {
    int reason = 0;

    if (breakEnable && stop_cpu)
      return STOP_STOP;

    if (cpu.tweaks.isolts_mode == 0)
      {
        // check clock queue
        if (sim_interval <= 0)
          {
            reason = sim_process_event ();
            if ((! breakEnable) && reason == SCPE_STOP)
              reason = SCPE_OK;
            if (reason)
              return reason;
          }
      }

    sim_interval --;

#if !defined(THREADZ) && !defined(LOCKLESS)
// This is needed for BCE_TRAP in install scripts
    // sim_brk_test expects a 32 bit address; PPR.IC into the low 18, and
    // PPR.PSR into the high 12
    if (sim_brk_summ &&
        sim_brk_test ((cpu.PPR.IC & 0777777) |
                      ((((t_addr) cpu.PPR.PSR) & 037777) << 18),
                      SWMASK ('E')))  /* breakpoint? */
      return STOP_BKPT; /* stop simulation */
# ifndef SPEED
    if (sim_deb_break && cpu.cycleCnt >= sim_deb_break)
      return STOP_BKPT; /* stop simulation */
# endif
#endif

    return reason;
  }

#ifdef PANEL68
static void panel_process_event (void)
  {
    // INITIALIZE pressed; treat at as a BOOT.
    if (cpu.panelInitialize && cpu.DATA_panel_s_trig_sw == 0)
      {
         // Wait for release
         while (cpu.panelInitialize)
           ;
         if (cpu.DATA_panel_init_sw)
           cpu_reset_unit_idx (ASSUME0, true); // INITIALIZE & CLEAR
         else
           cpu_reset_unit_idx (ASSUME0, false); // INITIALIZE
         // XXX Until a boot switch is wired up
         do_boot ();
      }
    // EXECUTE pressed; EXECUTE PB set, EXECUTE FAULT set
    if (cpu.DATA_panel_s_trig_sw == 0 &&
        cpu.DATA_panel_execute_sw &&  // EXECUTE button
        cpu.DATA_panel_scope_sw &&    // 'EXECUTE PB/SCOPE REPEAT' set to PB
        cpu.DATA_panel_exec_sw == 0)  // 'EXECUTE SWITCH/EXECUTE FAULT'
                                      //  set to FAULT
      {
        // Wait for release
        while (cpu.DATA_panel_execute_sw)
          ;

        if (cpu.DATA_panel_exec_sw) // EXECUTE SWITCH
          {
            cpu_reset_unit_idx (ASSUME0, false);
            cpu.cu.IWB = cpu.switches.data_switches;
            set_cpu_cycle (EXEC_cycle);
          }
         else // EXECUTE FAULT
          {
            setG7fault (current_running_cpu_idx, FAULT_EXF, fst_zero);
          }
      }
  }
#endif

#if defined(THREADZ) || defined(LOCKLESS)
bool bce_dis_called = false;

// The hypervisor CPU for the threadz model
t_stat sim_instr (void)
  {
    t_stat reason = 0;

# if 0
    static bool inited = false;
    if (! inited)
      {
        inited = true;

#  ifdef IO_THREADZ
// Create channel threads

        for (uint iom_unit_idx = 0; iom_unit_idx < N_IOM_UNITS_MAX; iom_unit_idx ++)
          {
            for (uint chan_num = 0; chan_num < MAX_CHANNELS; chan_num ++)
              {
                if (get_ctlr_in_use (iom_unit_idx, chan_num))
                  {
                    enum ctlr_type_e ctlr_type =
                      cables->iom_to_ctlr[iom_unit_idx][chan_num].ctlr_type;
                    createChnThread (iom_unit_idx, chan_num,
                                     ctlr_type_strs [ctlr_type]);
                    chnRdyWait (iom_unit_idx, chan_num);
                  }
              }
          }

// Create IOM threads

        for (uint iom_unit_idx = 0;
             iom_unit_idx < N_IOM_UNITS_MAX;
             iom_unit_idx ++)
          {
            createIOMThread (iom_unit_idx);
            iomRdyWait (iom_unit_idx);
          }
#  endif

// Create CPU threads

        //for (uint cpu_idx = 0; cpu_idx < N_CPU_UNITS_MAX; cpu_idx ++)
        for (uint cpu_idx = 0; cpu_idx < cpu_dev.numunits; cpu_idx ++)
          {
            createCPUThread (cpu_idx);
          }
      }
# endif
    if (cpuThreadz[0].run == false)
          createCPUThread (0);
    do
      {
        // Process deferred events and breakpoints
        reason = simh_hooks ();
        if (reason)
          {
            break;
          }

# if 0
// Check for CPU 0 stopped
        if (! cpuThreadz[0].run)
          {
            sim_msg ("CPU 0 stopped\n");
            return STOP_STOP;
          }
# endif
# if 0

// Check for all CPUs stopped

// This doesn't work for multiple CPU Multics; only one processor does the
// BCE dis; the other processors are doing the pxss 'dis 0777' dance;

        uint n_running = 0;
        for (uint i = 0; i < cpu_dev.numunits; i ++)
          {
            struct cpuThreadz_t * p = & cpuThreadz[i];
            if (p->run)
              n_running ++;
          }
        if (! n_running)
          return STOP_STOP;
# endif

        if (bce_dis_called) {
          //return STOP_STOP;
          reason = STOP_STOP;
          break;
        }

# ifndef PERF_STRIP
// Loop runs at 1000 Hz

#  ifdef LOCKLESS
        lock_iom();
#  endif
        lock_libuv ();
        uv_run (ev_poll_loop, UV_RUN_NOWAIT);
        unlock_libuv ();
#  ifdef LOCKLESS
        unlock_iom();
#  endif
        PNL (panel_process_event ());

        int con_unit_idx = check_attn_key ();
        if (con_unit_idx != -1)
          console_attn_idx (con_unit_idx);
# endif

# ifdef IO_ASYNC_PAYLOAD_CHAN_THREAD
        struct timespec next_time;
#  ifdef MACOSXPPC
        clock_serv_t cclock;
        mach_timespec_t mts;
        host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
        clock_get_time(cclock, &mts);
        mach_port_deallocate(mach_task_self(), cclock);
        next_time.tv_sec = mts.tv_sec;
        next_time.tv_nsec = mts.tv_nsec;
#  else
        clock_gettime (CLOCK_REALTIME, & next_time);
#  endif /* ifdef MACOSXPPC */
        next_time.tv_nsec += 1000l * 1000l;
        if (next_time.tv_nsec >= 1000l * 1000l *1000l)
          {
            next_time.tv_nsec -= 1000l * 1000l *1000l;
            next_time.tv_sec  += (time_t) 1;
          }
        struct timespec new_time;
        do
          {
            pthread_mutex_lock (& iom_start_lock);
            pthread_cond_timedwait (& iomCond,
                                    & iom_start_lock,
                                    & next_time);
            pthread_mutex_unlock (& iom_start_lock);
            lock_iom();
            lock_libuv ();

            iomProcess ();

            unlock_libuv ();
            unlock_iom ();

#  ifdef MACOSXPPC
            clock_serv_t cclock;
            mach_timespec_t mts;
            host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
            clock_get_time(cclock, &mts);
            mach_port_deallocate(mach_task_self(), cclock);
            new_time.tv_sec = mts.tv_sec;
            new_time.tv_nsec = mts.tv_nsec;
#  else
            clock_gettime (CLOCK_REALTIME, & new_time);
#  endif /* ifdef MACOSXPPC */
          }
        while ((next_time.tv_sec == new_time.tv_sec) ? (next_time.tv_nsec > new_time.tv_nsec) : \
                                                       (next_time.tv_sec  > new_time.tv_sec));
# else
//#  ifndef MACOSXPPC /* XXX(jhj) */
        sim_usleep (1000); // 1000 us == 1 ms == 1/1000 sec.
//#  endif /* ifndef MACOSXPPC */
# endif
      }
    while (reason == 0); //-V654

    for (uint cpuNo = 0; cpuNo < N_CPU_UNITS_MAX; cpuNo ++) {
      cpuStats (cpuNo);
    }

# ifdef TESTING
    HDBGPrint ();
# endif
    return reason;
  }
#endif

#if !defined(THREADZ) && !defined(LOCKLESS)
static uint fast_queue_subsample = 0;
#endif

//
// Okay, let's treat this as a state machine
//
//  INTERRUPT_cycle
//     clear interrupt, load interrupt pair into instruction buffer
//     set INTERRUPT_EXEC_cycle
//  INTERRUPT_EXEC_cycle
//     execute instruction in instruction buffer
//     if (! transfer) set INTERUPT_EXEC2_cycle
//     else set FETCH_cycle
//  INTERRUPT_EXEC2_cycle
//     execute odd instruction in instruction buffer
//     set INTERUPT_EXEC2_cycle
//
//  FAULT_cycle
//     fetch fault pair into instruction buffer
//     set FAULT_EXEC_cycle
//  FAULT_EXEC_cycle
//     execute instructions in instruction buffer
//     if (! transfer) set FAULT_EXE2_cycle
//     else set FETCH_cycle
//  FAULT_EXEC2_cycle
//     execute odd instruction in instruction buffer
//     set FETCH_cycle
//
//  FETCH_cycle
//     fetch instruction into instruction buffer
//     set EXEC_cycle
//
//  EXEC_cycle
//     execute instruction in instruction buffer
//     if (repeat conditions) keep cycling
//     if (pair) set EXEC2_cycle
//     else set FETCH_cycle
//  EXEC2_cycle
//     execute odd instruction in instruction buffer
//
//  XEC_cycle
//     load instruction into instruction buffer
//     set EXEC_cycle
//
//  XED_cycle
//     load instruction pair into instruction buffer
//     set EXEC_cycle
//
// other extant cycles:
//  ABORT_cycle

#if defined(THREADZ) || defined(LOCKLESS)
void * cpu_thread_main (void * arg)
  {
    int myid = * (int *) arg;
    set_cpu_idx ((uint) myid);
    unsigned char umyid = (unsigned char)toupper('a' + (int)myid);

    sim_msg ("\rCPU %c thread created.\r\n", (unsigned int)umyid);
# ifdef TESTING
    printPtid(pthread_self());
# endif /* ifdef TESTING */

    sim_os_set_thread_priority (PRIORITY_ABOVE_NORMAL);
    setSignals ();
    threadz_sim_instr ();
    return NULL;
  }
#endif // THREADZ

static void do_LUF_fault (void)
  {
    CPT (cpt1U, 16); // LUF
    cpu.lufCounter  = 0;
    cpu.lufOccurred = false;
// This is a hack to fix ISOLTS 776. ISOLTS checks that the TR has
// decremented by the LUF timeout value. To implement this, we set
// the TR to the expected value.

// LUF  time
//  0    2ms
//  1    4ms
//  2    8ms
//  3   16ms
// units
// you have: 2ms
// units
// You have: 512000Hz
// You want: 1/2ms
//    * 1024
//    / 0.0009765625
//
//  TR = 1024 << LUF
    if (cpu.tweaks.isolts_mode)
      cpu.shadowTR = (word27) cpu.TR0 - (1024u << (is_priv_mode () ? 4 : cpu.CMR.luf));

// The logic fails for test 785:
// set slave mode, LUF time 16ms.
// loop for 15.9 ms.
// set master mode.
// loop for 15.9 ms. The LUF should be noticed, and lufOccurred set.
// return to slave mode. The LUF should fire, with the timer register
// being set for 31.1 ms.

// XXX: Without accurate cycle timing or simply fudging the results,
// I don't see how to fix this one.

    doFault (FAULT_LUF, fst_zero, "instruction cycle lockup");
  }

#if !defined(THREADZ) && !defined(LOCKLESS)
# define threadz_sim_instr sim_instr
#endif

/*
 * addr_modes_e get_addr_mode()
 *
 * Report what mode the CPU is in.
 * This is determined by examining a couple of IR flags.
 *
 * TODO: get_addr_mode() probably belongs in the CPU source file.
 *
 */

static void set_temporary_absolute_mode (void)
  {
    CPT (cpt1L, 20); // set temp. abs. mode
    cpu.secret_addressing_mode = true;
    cpu.cu.XSF = false;
sim_debug (DBG_TRACEEXT, & cpu_dev, "set_temporary_absolute_mode bit 29 sets XSF to 0\n");
    //cpu.went_appending = false;
  }

static bool clear_temporary_absolute_mode (void)
  {
    CPT (cpt1L, 21); // clear temp. abs. mode
    cpu.secret_addressing_mode = false;
    return cpu.cu.XSF;
    //return cpu.went_appending;
  }

t_stat threadz_sim_instr (void)
  {
  //cpu.have_tst_lock = false;

#ifndef SCHED_NEVER_YIELD
    unsigned long long lockYieldAll     = 0;
#endif
    unsigned long long lockWaitMaxAll   = 0;
    unsigned long long lockWaitAll      = 0;
    unsigned long long lockImmediateAll = 0;
    unsigned long long lockCntAll       = 0;
    unsigned long long instrCntAll      = 0;
    unsigned long long cycleCntAll      = 0;

    t_stat reason = 0;

#if !defined(THREADZ) && !defined(LOCKLESS)
    set_cpu_idx (0);
# ifdef M_SHARED
// scp needs to have the IC statically allocated, so a placeholder was
// created.

    // Copy the placeholder so the IC can be set
    cpus [0].PPR.IC = dummy_IC;
# endif

# ifdef ROUND_ROBIN
    cpu.isRunning = true;
    set_cpu_idx (cpu_dev.numunits - 1);

setCPU:;
    uint current = current_running_cpu_idx;
    uint c;
    for (c = 0; c < cpu_dev.numunits; c ++)
      {
        set_cpu_idx (c);
        if (cpu.isRunning)
          break;
      }
    if (c == cpu_dev.numunits)
      {
        sim_msg ("All CPUs stopped\n");
        goto leave;
      }
    set_cpu_idx ((current + 1) % cpu_dev.numunits);
    if (! cpu . isRunning)
      goto setCPU;
# endif
#endif

    // This allows long jumping to the top of the state machine
    int val = setjmp (cpu.jmpMain);

    switch (val)
      {
        case JMP_ENTRY:
        case JMP_REENTRY:
            reason = 0;
            break;
        case JMP_SYNC_FAULT_RETURN:
            set_cpu_cycle (SYNC_FAULT_RTN_cycle);
            break;
        case JMP_STOP:
            reason = STOP_STOP;
            goto leave;
        case JMP_REFETCH:

            // Not necessarily so, but the only times
            // this path is taken is after an RCU returning
            // from an interrupt, which could only happen if
            // was xfer was false; or in a DIS cycle, in
            // which case we want it false so interrupts
            // can happen.
            cpu.wasXfer = false;

            set_cpu_cycle (FETCH_cycle);
            break;
        case JMP_RESTART:
            set_cpu_cycle (EXEC_cycle);
            break;
        default:
          sim_warn ("longjmp value of %d unhandled\n", val);
            goto leave;
      }

    // Main instruction fetch/decode loop

    DCDstruct * ci = & cpu.currentInstruction;

    if (cpu.restart)
      {
        set_cpu_cycle (FAULT_cycle);
      }

    do
      {
        reason = 0;

#if !defined(THREADZ) && !defined(LOCKLESS)
        // Process deferred events and breakpoints
        reason = simh_hooks ();
        if (reason)
          {
            break;
          }

// The event poll is consuming 40% of the CPU according to pprof.
// We only want to process at 100Hz; yet we are testing at ~1MHz.
// If we only test every 1000 cycles, we shouldn't miss by more then
// 10%...

        //if ((! cpu.wasInhibited) && fast_queue_subsample ++ > 1024) // ~ 1KHz
        //static uint fastQueueSubsample = 0;
        if (fast_queue_subsample ++ > sys_opts.sys_poll_check_rate) // ~ 1KHz
          {
            fast_queue_subsample = 0;
# ifdef CONSOLE_FIX
#  if defined(THREADZ) || defined(LOCKLESS)
            lock_libuv ();
#  endif
# endif
            uv_run (ev_poll_loop, UV_RUN_NOWAIT);
# ifdef CONSOLE_FIX
#  if defined(THREADZ) || defined(LOCKLESS)
            unlock_libuv ();
#  endif
# endif
            PNL (panel_process_event ());
          }
#endif // ! THREADZ

        cpu.cycleCnt ++;

#ifdef THREADZ
        // If we faulted somewhere with the memory lock set, clear it.
        unlock_mem_force ();

        // wait on run/switch
        cpuRunningWait ();
#endif // THREADZ
#ifdef LOCKLESS
        core_unlock_all ();
#endif

#ifdef LOCKLESS
        core_unlock_all();
#endif // LOCKLESS

        int con_unit_idx = check_attn_key ();
        if (con_unit_idx != -1)
          console_attn_idx (con_unit_idx);

#if !defined(THREADZ) && !defined(LOCKLESS)
        if (cpu.tweaks.isolts_mode)
          {
            if (cpu.cycle != FETCH_cycle)
              {
                // Sync. the TR with the emulator clock.
                cpu.rTRlsb ++;
                if (cpu.rTRlsb >= 4)
                  {
                    cpu.rTRlsb   = 0;
                    cpu.shadowTR = (cpu.shadowTR - 1) & MASK27;
                    if (cpu.shadowTR == 0) // passing through 0...
                      {
                        if (cpu.tweaks.tro_enable)
                          setG7fault (current_running_cpu_idx, FAULT_TRO, fst_zero);
                      }
                  }
              }
          }
#endif

// Check for TR underflow. The TR is stored in a uint32_t, but is 27 bits wide.
// The TR update code decrements the TR; if it passes through 0, the high bits
// will be set.

// If we assume a 1 MIPS reference platform, the TR would be decremented every
// two instructions (1/2 MHz)

#if 0
        cpu.rTR     -= cpu.rTRticks * 100;
        //cpu.rTR   -= cpu.rTRticks * 50;
        cpu.rTRticks = 0;
#else
# define TR_RATE 2

        //cpu.rTR      -= cpu.rTRticks / TR_RATE;
        // ubsan
        cpu.rTR = (word27) (((word27s) cpu.rTR) - (word27s) (cpu.rTRticks / TR_RATE));
        cpu.rTRticks %= TR_RATE;

#endif

        if (cpu.rTR & ~MASK27)
          {
            cpu.rTR &= MASK27;
            if (cpu.tweaks.tro_enable) {
              setG7fault (current_running_cpu_idx, FAULT_TRO, fst_zero);
            }
          }

        sim_debug (DBG_CYCLE, & cpu_dev, "Cycle is %s\n",
                   cycle_str (cpu.cycle));

        switch (cpu.cycle)
          {
            case INTERRUPT_cycle:
              {
                CPT (cpt1U, 0); // Interrupt cycle
                // In the INTERRUPT CYCLE, the processor safe-stores
                // the Control Unit Data (see Section 3) into
                // program-invisible holding registers in preparation
                // for a Store Control Unit (scu) instruction, enters
                // temporary absolute mode, and forces the current
                // ring of execution C(PPR.PRR) to
                // 0. It then issues an XEC system controller command
                // to the system controller on the highest priority
                // port for which there is a bit set in the interrupt
                // present register.

                uint intr_pair_addr = get_highest_intr ();
#ifdef TESTING
                HDBGIntr (intr_pair_addr, "");
#endif
                cpu.cu.FI_ADDR = (word5) (intr_pair_addr / 2);
                cu_safe_store ();
                // XXX the whole interrupt cycle should be rewritten as an xed
                // instruction pushed to IWB and executed

                CPT (cpt1U, 1); // safe store complete
                // Temporary absolute mode
                set_temporary_absolute_mode ();

                // Set to ring 0
                cpu.PPR.PRR = 0;
                cpu.TPR.TRR = 0;

                sim_debug (DBG_INTR, & cpu_dev, "intr_pair_addr %u flag %d\n",
                           intr_pair_addr, cpu.interrupt_flag);
#ifndef SPEED
                if_sim_debug (DBG_INTR, & cpu_dev)
                    traceInstruction (DBG_INTR);
#endif
                // Check that an interrupt is actually pending
                if (cpu.interrupt_flag)
                  {
                    CPT (cpt1U, 2); // interrupt pending
                    // clear interrupt, load interrupt pair into instruction
                    // buffer; set INTERRUPT_EXEC_cycle.

                    // In the h/w this is done later, but doing it now allows
                    // us to avoid clean up for no interrupt pending.

                    if (intr_pair_addr != 1) // no interrupts
                      {

                        CPT (cpt1U, 3); // interrupt identified

                        // get interrupt pair
                        core_read2 (intr_pair_addr,
                                    & cpu.cu.IWB, & cpu.cu.IRODD, __func__);
#ifdef TESTING
                        HDBGMRead (intr_pair_addr, cpu.cu.IWB, "intr even");
                        HDBGMRead (intr_pair_addr + 1, cpu.cu.IRODD, "intr odd");
#endif
                        cpu.cu.xde = 1;
                        cpu.cu.xdo = 1;
                        cpu.isExec = true;
                        cpu.isXED  = true;

                        CPT (cpt1U, 4); // interrupt pair fetched
                        cpu.interrupt_flag = false;
                        set_cpu_cycle (INTERRUPT_EXEC_cycle);
                        break;
                      } // int_pair != 1
                  } // interrupt_flag

                // If we get here, there was no interrupt

                CPT (cpt1U, 5); // interrupt pair spurious
                cpu.interrupt_flag = false;
                clear_temporary_absolute_mode ();
                // Restores addressing mode
                cu_safe_restore ();
                // We can only get here if wasXfer was
                // false, so we can assume it still is.
                cpu.wasXfer = false;
// The only place cycle is set to INTERRUPT_cycle in FETCH_cycle; therefore
// we can safely assume that is the state that should be restored.
                set_cpu_cycle (FETCH_cycle);
              }
              break;

            case FETCH_cycle:
#ifdef PANEL68
                (void)memset (cpu.cpt, 0, sizeof (cpu.cpt));
#endif
                CPT (cpt1U, 13); // fetch cycle

                PNL (L68_ (cpu.INS_FETCH = false;))

// "If the interrupt inhibit bit is not set in the current instruction
// word at the point of the next sequential instruction pair virtual
// address formation, the processor samples the [group 7 and interrupts]."

// Since XEx/RPx may overwrite IWB, we must remember
// the inhibit bits (cpu.wasInhibited).

// If the instruction pair virtual address being formed is the result of a
// transfer of control condition or if the current instruction is
// Execute (xec), Execute Double (xed), Repeat (rpt), Repeat Double (rpd),
// or Repeat Link (rpl), the group 7 faults and interrupt present lines are
// not sampled.

// Group 7 Faults
//
// Shutdown
//
// An external power shutdown condition has been detected. DC POWER shutdown
// will occur in approximately one millisecond.
//
// Timer Runout
//
// The timer register has decremented to or through the value zero. If the
// processor is in privileged mode or absolute mode, recognition of this fault
// is delayed until a return to normal mode or BAR mode. Counting in the timer
// register continues.
//
// Connect
//
// A connect signal ($CON strobe) has been received from a system controller.
// This event is to be distinguished from a Connect Input/Output Channel (cioc)
// instruction encountered in the program sequence.

                // check BAR bound and raise store fault if above
                // pft 04d 10070, ISOLTS-776 06ad
                if (get_bar_mode ())
                    get_BAR_address (cpu.PPR.IC);

                // Don't check timer runout if privileged
                // ISOLTS-776 04bcf, 785 02c
                // (but do if in a DIS instruction with bit28 clear)
                bool tmp_priv_mode = is_priv_mode ();
                bool is_dis        = cpu.currentInstruction.opcode  == 0616 &&
                                     cpu.currentInstruction.opcodeX == 0;
                bool noCheckTR     = tmp_priv_mode &&
                                     !(is_dis && GET_I (cpu.cu.IWB) == 0);

                if (is_dis)
                  {
                    // take interrupts and g7 faults as long as
                    // last instruction is DIS (??)
                    cpu.interrupt_flag = sample_interrupts ();
                    cpu.g7_flag =
                              noCheckTR ? bG7PendingNoTRO () : bG7Pending ();
                  }
                else if (! (cpu.cu.xde | cpu.cu.xdo |
                       cpu.cu.rpt | cpu.cu.rd | cpu.cu.rl))
                  {
                    if ((!cpu.wasInhibited) &&
                        (cpu.PPR.IC & 1) == 0 &&
                        (! cpu.wasXfer))
                      {
                        CPT (cpt1U, 14); // sampling interrupts
                        cpu.interrupt_flag = sample_interrupts ();
                        cpu.g7_flag =
                          noCheckTR ? bG7PendingNoTRO () : bG7Pending ();
                      }
                    cpu.wasInhibited = false;
                  }
                else
                  {
                    // XEx at an odd location disables interrupt sampling
                    // also for the next instruction pair. ISOLTS-785 02g,
                    // 776 04g
                    // Set the inhibit flag
                    // (I assume RPx behaves in the same way)
                    if ((cpu.PPR.IC & 1) == 1)
                      {
                        cpu.wasInhibited = true;
                      }
                  }

// Multics executes a CPU connect instruction (which should eventually cause a
// connect fault) while interrupts are inhibited and an IOM interrupt is
// pending. Multics then executes a DIS instruction (Delay Until Interrupt
// Set). This should cause the processor to "sleep" until an interrupt is
// signaled. The DIS instruction sees that an interrupt is pending, sets
// cpu.interrupt_flag to signal that the CPU to service the interrupt and
// resumes the CPU.
//
// The CPU state machine sets up to fetch the next instruction. If checks to
// see if this instruction should check for interrupts or faults according to
// the complex rules (interrupts inhibited, even address, not RPT or XEC,
// etc.); it this case, the test fails as the next instruction is at an odd
// address. If the test had passed, the cpu.interrupt_flag would be set or
// cleared depending on the pending interrupt state data, AND the cpu.g7_flag
// would be set or cleared depending on the faults pending data (in this case,
// the connect fault).
//
// Because the flags were not updated, after the test, cpu.interrupt_flag is
// set (since the DIS instruction set it) and cpu.g7_flag is not set.
//
// Next, the CPU sees the that cpu.interrupt flag is set, and starts the
// interrupt cycle despite the fact that a higher priority g7 fault is pending.

// To fix this, check (or recheck) g7 if an interrupt is going to be faulted.
// Either DIS set interrupt_flag and FETCH_cycle didn't so g7 needs to be
// checked, or FETCH_cycle did check it when it set interrupt_flag in which
// case it is being rechecked here. It is [locally] idempotent and light
// weight, so this should be okay.

// not necessary any more because of is_dis logic
#if 0
                if (cpu.interrupt_flag)
                  cpu.g7_flag = noCheckTR ? bG7PendingNoTRO () : bG7Pending ();
#endif
                if (cpu.g7_flag)
                  {
                      cpu.g7_flag        = false;
                      cpu.interrupt_flag = false;
                      sim_debug (DBG_CYCLE, & cpu_dev,
                                 "call doG7Fault (%d)\n", !noCheckTR);
                      doG7Fault (!noCheckTR);
                  }
                if (cpu.interrupt_flag)
                  {
// This is the only place cycle is set to INTERRUPT_cycle; therefore
// return from interrupt can safely assume the it should set the cycle
// to FETCH_cycle.
                    CPT (cpt1U, 15); // interrupt
                    set_cpu_cycle (INTERRUPT_cycle);
                    break;
                  }

// "While in absolute mode or privileged mode the lockup fault is signalled at
// the end of the time limit set in the lockup timer but is not recognized
// until the 32 millisecond limit. If the processor returns to normal mode or
// BAR mode after the fault has been signalled but before the 32 millisecond
// limit, the fault is recognized before any instruction in the new mode is
// executed."

          /*FALLTHRU*/ /* fall through */
          case PSEUDO_FETCH_cycle:

            tmp_priv_mode = is_priv_mode ();
            if (! (luf_flag && tmp_priv_mode))
              cpu.lufCounter ++;
#if 1
            if (cpu.lufCounter > luf_limits[cpu.CMR.luf])
              {
                if (tmp_priv_mode)
                  {
                    // In priv. mode the LUF is noted but not executed
                    cpu.lufOccurred = true;
                  }
                else
                  {
                    do_LUF_fault ();
                  }
              } // lufCounter > luf_limit

            // After 32ms, the LUF fires regardless of priv.
            if (cpu.lufCounter > luf_limits[4])
              {
                do_LUF_fault ();
              }

            // If the LUF occurred in priv. mode and we left priv. mode,
            // fault.
            if (! tmp_priv_mode && cpu.lufOccurred)
              {
                do_LUF_fault ();
              }
#else
            if ((tmp_priv_mode && cpu.lufCounter > luf_limits[4]) ||
                (! tmp_priv_mode &&
                 cpu.lufCounter > luf_limits[cpu.CMR.luf]))
              {
                CPT (cpt1U, 16); // LUF
                cpu.lufCounter = 0;

// This is a hack to fix ISOLTS 776. ISOLTS checks that the TR has
// decremented by the LUF timeout value. To implement this, we set
// the TR to the expected value.

// LUF  time
//  0    2ms
//  1    4ms
//  2    8ms
//  3   16ms
// units
// you have: 2ms
// units
// You have: 512000Hz
// You want: 1/2ms
//    * 1024
//    / 0.0009765625
//
//  TR = 1024 << LUF
               if (cpu.tweaks.isolts_mode)
                 cpu.shadowTR = (word27) cpu.TR0 - (1024u << (is_priv_mode () ? 4 : cpu.CMR.luf));

                doFault (FAULT_LUF, fst_zero, "instruction cycle lockup");
              }
#endif
            if (cpu.cycle == PSEUDO_FETCH_cycle)
              {
                cpu.apu.lastCycle    = INSTRUCTION_FETCH;
                cpu.cu.XSF           = 0;
                cpu.cu.TSN_VALID [0] = 0;
                cpu.TPR.TSR          = cpu.PPR.PSR;
                cpu.TPR.TRR          = cpu.PPR.PRR;
                cpu.wasInhibited     = false;
              }
            else
              {
                CPT (cpt1U, 20); // not XEC or RPx
                cpu.isExec               = false;
                cpu.isXED                = false;
                // fetch next instruction into current instruction struct
                //clr_went_appending (); // XXX not sure this is the right
                                         //  place
                cpu.cu.XSF               = 0;
sim_debug (DBG_TRACEEXT, & cpu_dev, "fetchCycle bit 29 sets XSF to 0\n");
                cpu.cu.TSN_VALID [0]     = 0;
                cpu.TPR.TSR              = cpu.PPR.PSR;
                cpu.TPR.TRR              = cpu.PPR.PRR;
                PNL (cpu.prepare_state   = ps_PIA);
                PNL (L68_ (cpu.INS_FETCH = true;))
                fetchInstruction (cpu.PPR.IC);
              }

            CPT (cpt1U, 21); // go to exec cycle
            advanceG7Faults ();
            set_cpu_cycle (EXEC_cycle);
            break;

          case EXEC_cycle:
          case FAULT_EXEC_cycle:
          case INTERRUPT_EXEC_cycle:
            {
              CPT (cpt1U, 22); // exec cycle

#ifdef LOCKLESS
                if (stall_point_active)
                  {
                    for (int i = 0; i < N_STALL_POINTS; i ++)
                      if (stall_points[i].segno  && stall_points[i].segno  == cpu.PPR.PSR &&
                          stall_points[i].offset && stall_points[i].offset == cpu.PPR.IC)
                        {
# ifdef CTRACE
                          (void)fprintf (stderr, "%10lu %s stall %d\n", seqno (), cpunstr[current_running_cpu_idx], i);
# endif
                          //sim_printf ("stall %2d %05o:%06o\n", i, stall_points[i].segno, stall_points[i].offset);
                          sim_usleep(stall_points[i].time);
                          break;
                        }
                  }
#endif

              // The only time we are going to execute out of IRODD is
              // during RPD, at which time interrupts are automatically
              // inhibited; so the following can ignore RPD harmlessly
              if (GET_I (cpu.cu.IWB))
                cpu.wasInhibited = true;

              t_stat ret = executeInstruction ();
#ifdef TR_WORK_EXEC
              cpu.rTRticks ++;
#endif
              CPT (cpt1U, 23); // execution complete

              if (cpu.tweaks.l68_mode)
                add_l68_CU_history ();
              else
                add_dps8m_CU_history ();

              if (ret > 0)
                {
                   reason = ret;
                   break;
                }

              if (ret == CONT_XEC)
                {
                  CPT (cpt1U, 27); // XEx instruction
                  cpu.wasXfer = false;
                  cpu.isExec  = true;
                  if (cpu.cu.xdo)
                    cpu.isXED = true;

                  cpu.cu.XSF           = 0;
                  cpu.cu.TSN_VALID [0] = 0;
                  cpu.TPR.TSR          = cpu.PPR.PSR;
                  cpu.TPR.TRR          = cpu.PPR.PRR;
                  break;
                }

              if (ret == CONT_TRA || ret == CONT_RET)
                {
                  CPT (cpt1U, 24); // transfer instruction
                  cpu.cu.xde  = cpu.cu.xdo = 0;
                  cpu.isExec  = false;
                  cpu.isXED   = false;
                  // even for CONT_RET else isolts 886 fails
                  cpu.wasXfer = true;

                  if (cpu.cycle != EXEC_cycle) // fault or interrupt
                    {

                      clearFaultCycle ();

// BAR mode:  [NBAR] is set ON (taking the processor
// out of BAR mode) by the execution of any transfer instruction
// other than tss during a fault or interrupt trap.

                      if (! (cpu.currentInstruction.opcode == 0715 &&
                         cpu.currentInstruction.opcodeX == 0))
                        {
                          CPT (cpt1U, 9); // nbar set
                          SET_I_NBAR;
                        }

                      if (!clear_temporary_absolute_mode ())
                        {
                          // didn't go appending
                          sim_debug (DBG_TRACEEXT, & cpu_dev,
                                     "setting ABS mode\n");
                          CPT (cpt1U, 10); // temporary absolute mode
                          set_addr_mode (ABSOLUTE_mode);
                        }
                      else
                        {
                          // went appending
                          sim_debug (DBG_TRACEEXT, & cpu_dev,
                                     "not setting ABS mode\n");
                        }

                    } // fault or interrupt

                  //if (TST_I_ABS && get_went_appending ())
                  if (TST_I_ABS && cpu.cu.XSF)
                    {
                      set_addr_mode (APPEND_mode);
                    }

                  if (ret == CONT_TRA)
                    {
                      // PSEUDO_FETCH_cycle does not check interrupts/g7faults
                      cpu.wasXfer = false;
                      set_cpu_cycle (PSEUDO_FETCH_cycle);
                    }
                  else
                    set_cpu_cycle (FETCH_cycle);
                  break;   // Don't bump PPR.IC, instruction already did it
                }

              if (ret == CONT_DIS)
                {
                  CPT (cpt1U, 25); // DIS instruction

// If we get here, we have encountered a DIS instruction in EXEC_cycle.
//
// We need to idle the CPU until one of the following conditions:
//
//  An external interrupt occurs.
//  The Timer Register underflows.
//  The emulator polled devices need polling.
//
// The external interrupt will only be posted to the CPU engine if the
// device poll posts an interrupt. This means that we do not need to
// detect the interrupts here; if we wake up and poll the devices, the
// interrupt will be detected by the DIS instruction when it is re-executed.
//
// The Timer Register is a fast, high-precision timer but Multics uses it
// in only two ways: detecting I/O lockup during early boot, and process
// quantum scheduling (1/4 second for Multics).
//
// Neither of these require high resolution or high accuracy.
//
// The goal of the polling code is sample at about 100Hz; updating the timer
// register at that rate should suffice.
//
//    sleep for 1/100 of a second
//    update the polling state to trigger a poll
//    update the timer register by 1/100 of a second
//    force the scp queues to process
//    continue processing
//

// The usleep logic is not smart enough w.r.t. ROUND_ROBIN/ISOLTS.
// The sleep should only happen if all running processors are in
// DIS mode.
#ifndef ROUND_ROBIN
                  // 1/100 is .01 secs.
                  // *1000 is 10  milliseconds
                  // *1000 is 10000 microseconds
                  // in uSec;
# if defined(THREADZ) || defined(LOCKLESS)

// XXX If interrupt inhibit set, then sleep forever instead of TRO
                  // rTR is 512KHz; sleepCPU is in 1Mhz
                  //   rTR * 1,000,000 / 512,000
                  //   rTR * 1000 / 512
                  //   rTR * 500 / 256
                  //   rTR * 250 / 128
                  //   rTR * 125 / 64

#  ifdef NO_TIMEWAIT
                  //sim_usleep (sys_opts.sys_poll_interval * 1000 /*10000*/ );
                  struct timespec req, rem;
                  uint ms        = sys_opts.sys_poll_interval;
                  long int nsec  = (long int) ms * 1000L * 1000L;
                  req.tv_nsec    = nsec;
                  req.tv_sec    += req.tv_nsec / 1000000000L;
                  req.tv_nsec   %= 1000000000L;
                  int rc         = nanosleep (& req, & rem); // XXX Does this work on Windows ???
                  // Awakened early?
                  if (rc == -1)
                    {
                       ms = (uint) (rem.tv_nsec / 1000 + req.tv_sec * 1000);
                    }
                  word27 ticks = ms * 512;
                  if (cpu.rTR <= ticks)
                    {
                      if (cpu.tweaks.tro_enable) {
                        setG7fault (current_running_cpu_idx, FAULT_TRO, fst_zero);
                      }
                      cpu.rTR = (cpu.rTR - ticks) & MASK27;
                    }
                  else
                    cpu.rTR = (cpu.rTR - ticks) & MASK27;

                  if (cpu.rTR == 0)
                    cpu.rTR = MASK27;
#  else // !NO_TIMEWAIT
                  // unsigned long left = cpu.rTR * 125u / 64u;
                  // ubsan
                  unsigned long left = (unsigned long) ((uint64) (cpu.rTR) * 125u / 64u);
                  unsigned long nowLeft = left;
                  lock_scu ();
                  if (!sample_interrupts ())
                    {
                      nowLeft = sleepCPU (left);
                    }
                  unlock_scu ();
                  if (nowLeft)
                    {
                      // sleepCPU uses a clock that is not guaranteed to be monotonic, and occasionally returns nowLeft > left.
                      // Don't run rTR backwards if that happens
                      if (nowLeft <= left) {
                        cpu.rTR = (word27) (left * 64 / 125);
                      }
                    }
                  else
                    {
                      // We slept until timer runout
                      if (cpu.tweaks.tro_enable)
                        {
                          lock_scu ();
                          setG7fault (current_running_cpu_idx, FAULT_TRO, fst_zero);
                          unlock_scu ();
                        }
                      cpu.rTR = MASK27;
                    }
#  endif // !NO_TIMEWAIT
                  cpu.rTRticks = 0;
                  break;
# else // ! (THREADZ || LOCKLESS)
                  //sim_sleep (10000);
                  sim_usleep (sys_opts.sys_poll_interval * 1000/*10000*/);
                  // Trigger I/O polling
#  ifdef CONSOLE_FIX
#   if defined(THREADZ) || defined(LOCKLESS)
                  lock_libuv ();
#   endif
#  endif
                  uv_run (ev_poll_loop, UV_RUN_NOWAIT);
#  ifdef CONSOLE_FIX
#   if defined(THREADZ) || defined(LOCKLESS)
                  unlock_libuv ();
#   endif
#  endif
                  fast_queue_subsample = 0;

                  sim_interval = 0;
                  // Timer register runs at 512 KHz
                  // 512000 is 1 second
                  // 512000/100 -> 5120  is .01 second

                  cpu.rTRticks = 0;
                  // Would we have underflowed while sleeping?
                  //if ((cpu.rTR & ~ MASK27) || cpu.rTR <= 5120)
                  //if (cpu.rTR <= 5120)

                  // Timer register runs at 512 KHz
                  // 512Khz / 512 is millisecods
                  if (cpu.rTR <= sys_opts.sys_poll_interval * 512)
                    {
                      if (cpu.tweaks.tro_enable) {
                        setG7fault (current_running_cpu_idx, FAULT_TRO,
                                    fst_zero);
                      }
                      cpu.rTR = (cpu.rTR - sys_opts.sys_poll_interval * 512) & MASK27;
                    }
                  else
                    cpu.rTR = (cpu.rTR - sys_opts.sys_poll_interval * 512) & MASK27;
                  if (cpu.rTR == 0)
                    cpu.rTR = MASK27;
# endif // ! (THREADZ || LOCKLESS)
#endif // ! ROUND_ROBIN
                  /*NOTREACHED*/ /* unreachable */
                  break;
                }

              cpu.wasXfer = false;

              if (ret < 0)
                {
                  sim_warn ("executeInstruction returned %d?\n", ret);
                  break;
                }

              if ((! cpu.cu.repeat_first) &&
                  (cpu.cu.rpt ||
                   (cpu.cu.rd && (cpu.PPR.IC & 1)) ||
                   cpu.cu.rl))
                {
                  CPT (cpt1U, 26); // RPx instruction
                  if (cpu.cu.rd)
                    -- cpu.PPR.IC;
                  cpu.wasXfer = false;
                  set_cpu_cycle (FETCH_cycle);
                  break;
                }

              // If we just did the odd word of a fault pair
              if (cpu.cycle == FAULT_EXEC_cycle &&
                  !cpu.cu.xde && cpu.cu.xdo)
                {
                  clear_temporary_absolute_mode ();
                  cu_safe_restore ();
                  CPT (cpt1U, 12); // cu restored
                  clearFaultCycle ();
                  // cu_safe_restore calls decode_instruction ()
                  // we can determine the instruction length.
                  // decode_instruction() restores ci->info->ndes
                  cpu.wasXfer  = false;
                  cpu.isExec   = false;
                  cpu.isXED    = false;

                  cpu.PPR.IC  += ci->info->ndes;
                  cpu.PPR.IC ++;

                  set_cpu_cycle (FETCH_cycle);
                  break;
                }

              // If we just did the odd word of a interrupt pair
              if (cpu.cycle == INTERRUPT_EXEC_cycle &&
                  !cpu.cu.xde && cpu.cu.xdo)
                {
                  clear_temporary_absolute_mode ();
                  cu_safe_restore ();
                  // cpu.cu.xdo = 0;
// The only place cycle is set to INTERRUPT_cycle in FETCH_cycle; therefore
// we can safely assume that is the state that should be restored.
                  CPT (cpt1U, 12); // cu restored
                  cpu.wasXfer = false;
                  cpu.isExec  = false;
                  cpu.isXED   = false;

                  set_cpu_cycle (FETCH_cycle);
                  break;
                }

              // Even word of fault or interrupt pair or xed
              if (cpu.cu.xde && cpu.cu.xdo)
                {
                  // Get the odd
                  cpu.cu.IWB           = cpu.cu.IRODD;
                  cpu.cu.xde           = 0;
                  cpu.isExec           = true;
                  cpu.isXED            = true;
                  cpu.cu.XSF           = 0;
                  cpu.cu.TSN_VALID [0] = 0;
                  cpu.TPR.TSR          = cpu.PPR.PSR;
                  cpu.TPR.TRR          = cpu.PPR.PRR;
                  break; // go do the odd word
                }

              if (cpu.cu.xde || cpu.cu.xdo)  // we are in an XEC/XED
                {
                  cpu.cu.xde        = cpu.cu.xdo = 0;
                  cpu.isExec        = false;
                  cpu.isXED         = false;
                  CPT (cpt1U, 27);           // XEx instruction
                  cpu.wasXfer       = false;
                  cpu.PPR.IC ++;
                  if (ci->info->ndes > 0)
                    cpu.PPR.IC     += ci->info->ndes;
                  cpu.wasInhibited  = true;
                  set_cpu_cycle (FETCH_cycle);
                  break;
                }

              //ASSURE (cpu.cycle == EXEC_cycle);
              if (cpu.cycle != EXEC_cycle)
                sim_warn ("expected EXEC_cycle (%d)\n", cpu.cycle);

              cpu.cu.xde = cpu.cu.xdo = 0;
              cpu.isExec = false;
              cpu.isXED  = false;

              // use prefetched instruction from cpu.cu.IRODD
              // we must have finished an instruction at an even location
              // skip multiword EIS instructions
              // skip repeat instructions for now
              // skip dis - we may need to take interrupts/g7faults
              // skip if (last instruction) wrote to current instruction range
              //  the hardware really does this and isolts tests it
              //  Multics Differences Manual DPS8 70/M
              //  should take segment number into account?
              if ((cpu.PPR.IC & 1) == 0 &&
                  ci->info->ndes == 0 &&
                  !cpu.cu.repeat_first && !cpu.cu.rpt && !cpu.cu.rd && !cpu.cu.rl &&
                  !(cpu.currentInstruction.opcode == 0616 && cpu.currentInstruction.opcodeX == 0) &&
                  (cpu.PPR.IC & ~3u) != (cpu.last_write  & ~3u))
                {
                  cpu.PPR.IC ++;
                  cpu.wasXfer = false;
                  cpu.cu.IWB  = cpu.cu.IRODD;
                  set_cpu_cycle (PSEUDO_FETCH_cycle);
                  break;
                }

              cpu.PPR.IC ++;
              if (ci->info->ndes > 0)
                cpu.PPR.IC += ci->info->ndes;

              CPT (cpt1U, 28); // enter fetch cycle
              cpu.wasXfer = false;
              set_cpu_cycle (FETCH_cycle);
            }
            break;

          case SYNC_FAULT_RTN_cycle:
            {
              CPT (cpt1U, 29); // sync. fault return
              // cu_safe_restore should have restored CU.IWB, so
              // we can determine the instruction length.
              // decode_instruction() restores ci->info->ndes

              cpu.PPR.IC += ci->info->ndes;
              cpu.PPR.IC ++;
              cpu.wasXfer = false;
              set_cpu_cycle (FETCH_cycle);
            }
            break;

          case FAULT_cycle:
            {
              CPT (cpt1U, 30); // fault cycle
              // In the FAULT CYCLE, the processor safe-stores the Control
              // Unit Data (see Section 3) into program-invisible holding
              // registers in preparation for a Store Control Unit ( scu)
              // instruction, then enters temporary absolute mode, forces the
              // current ring of execution C(PPR.PRR) to 0, and generates a
              // computed address for the fault trap pair by concatenating
              // the setting of the FAULT BASE switches on the processor
              // configuration panel with twice the fault number (see Table
              // 7-1).  This computed address and the operation code for the
              // Execute Double (xed) instruction are forced into the
              // instruction register and executed as an instruction. Note
              // that the execution of the instruction is not done in a
              // normal EXECUTE CYCLE but in the FAULT CYCLE with the
              // processor in temporary absolute mode.

              // F(A)NP should never be stored when faulting.
              // ISOLTS-865 01a,870 02d
              // Unconditional reset of APU status to FABS breaks boot.
              // Checking for F(A)NP here is equivalent to checking that the
              // last append cycle has made it as far as H/I without a fault.
              // Also reset it on TRB fault. ISOLTS-870 05a
              if ((cpu.cu.APUCycleBits & 060) || cpu.secret_addressing_mode)
                  set_apu_status (apuStatus_FABS);

              // XXX the whole fault cycle should be rewritten as an xed
              // instruction pushed to IWB and executed

              // AL39: TRB fault doesn't safestore CUD - the original fault
              // CUD should be stored

              // ISOLTS-870 05a: CUD[5] and IWB are safe stored, possibly
              //  due to CU overlap

              // keep IRODD untouched if TRB occurred in an even location
              if (cpu.faultNumber != FAULT_TRB || cpu.cu.xde == 0)
                {
                  cu_safe_store ();
                }
              else
                {
                  word36 tmpIRODD = cpu.scu_data[7];
                  cu_safe_store ();
                  cpu.scu_data[7] = tmpIRODD;
                }
              CPT (cpt1U, 31); // safe store complete

              // Temporary absolute mode
              set_temporary_absolute_mode ();

              // Set to ring 0
              cpu.PPR.PRR = 0;
              cpu.TPR.TRR = 0;

              // (12-bits of which the top-most 7-bits are used)
              uint fltAddress = (cpu.switches.FLT_BASE << 5) & 07740;
              L68_ (
                if (cpu.is_FFV)
                  {
                    cpu.is_FFV = false;
                    CPTUR (cptUseMR);
                    // The high 15 bits
                    fltAddress = (cpu.MR.FFV & MASK15) << 3;
                  }
              )

              // absolute address of fault YPair
              word24 addr = fltAddress + 2 * cpu.faultNumber;

              if (cpu.restart)
                {
                  cpu.restart = false;
                  addr = cpu.restart_address;
                }

              core_read2 (addr, & cpu.cu.IWB, & cpu.cu.IRODD, __func__);
#ifdef TESTING
              HDBGMRead (addr, cpu.cu.IWB, "fault even");
              HDBGMRead (addr + 1, cpu.cu.IRODD, "fault odd");
#endif
              cpu.cu.xde = 1;
              cpu.cu.xdo = 1;
              cpu.isExec = true;
              cpu.isXED  = true;

              CPT (cpt1U, 33); // set fault exec cycle
              set_cpu_cycle (FAULT_EXEC_cycle);

              break;
            }

          }  // switch (cpu.cycle)
      }
#ifdef ROUND_ROBIN
    while (0);
    if (reason == 0)
      goto setCPU;
#else
    while (reason == 0);
#endif

leave:
#ifdef TESTING
    HDBGPrint ();
#endif

    for (unsigned short n = 0; n < N_CPU_UNITS_MAX; n++)
      {
#ifndef SCHED_NEVER_YIELD
        lockYieldAll     = lockYieldAll     + (unsigned long long)cpus[n].lockYield;
#endif
        lockWaitMaxAll   = lockWaitMaxAll   + (unsigned long long)cpus[n].lockWaitMax;
        lockWaitAll      = lockWaitAll      + (unsigned long long)cpus[n].lockWait;
        lockImmediateAll = lockImmediateAll + (unsigned long long)cpus[n].lockImmediate;
        lockCntAll       = lockCntAll       + (unsigned long long)cpus[n].lockCnt;
        instrCntAll      = instrCntAll      + (unsigned long long)cpus[n].instrCnt;
        cycleCntAll      = cycleCntAll      + (unsigned long long)cpus[n].cycleCnt;
      }

    (void)fflush(stderr);
    (void)fflush(stdout);
#if 1
# ifndef PERF_STRIP
    if (cycleCntAll > (unsigned long long)cpu.cycleCnt)
      {
# endif
        sim_msg ("\r\n");
        sim_msg ("\r+---------------------------------+\r\n");
        sim_msg ("\r|     Aggregate CPU Statistics    |\r\n");
        sim_msg ("\r+---------------------------------+\r\n");
        (void)fflush(stderr);
        (void)fflush(stdout);
# ifdef WIN_STDIO
        sim_msg ("\r|  cycles        %15llu  |\r\n", cycleCntAll);
        sim_msg ("\r|  instructions  %15llu  |\r\n", instrCntAll);
        (void)fflush(stderr);
        (void)fflush(stdout);
        sim_msg ("\r+---------------------------------+\r\n");
        sim_msg ("\r|  lockCnt       %15llu  |\r\n", lockCntAll);
        sim_msg ("\r|  lockImmediate %15llu  |\r\n", lockImmediateAll);
        (void)fflush(stderr);
        (void)fflush(stdout);
        sim_msg ("\r+---------------------------------+\r\n");
        sim_msg ("\r|  lockWait      %15llu  |\r\n", lockWaitAll);
        sim_msg ("\r|  lockWaitMax   %15llu  |\r\n", lockWaitMaxAll);
        (void)fflush(stderr);
        (void)fflush(stdout);
#  ifndef SCHED_NEVER_YIELD
        sim_msg ("\r|  lockYield     %15llu  |\r\n", lockYieldAll);
#  else
        sim_msg ("\r|  lockYield                ----  |\r\n");
#  endif /* ifndef SCHED_NEVER_YIELD */
        sim_msg ("\r+---------------------------------+\r\n");
        (void)fflush(stderr);
        (void)fflush(stdout);
# else
        sim_msg ("\r|  cycles        %'15llu  |\r\n", cycleCntAll);
        sim_msg ("\r|  instructions  %'15llu  |\r\n", instrCntAll);
        (void)fflush(stderr);
        (void)fflush(stdout);
        sim_msg ("\r+---------------------------------+\r\n");
        sim_msg ("\r|  lockCnt       %'15llu  |\r\n", lockCntAll);
        sim_msg ("\r|  lockImmediate %'15llu  |\r\n", lockImmediateAll);
        (void)fflush(stderr);
        (void)fflush(stdout);
        sim_msg ("\r+---------------------------------+\r\n");
        sim_msg ("\r|  lockWait      %'15llu  |\r\n", lockWaitAll);
        sim_msg ("\r|  lockWaitMax   %'15llu  |\r\n", lockWaitMaxAll);
        (void)fflush(stderr);
        (void)fflush(stdout);
#  ifndef SCHED_NEVER_YIELD
        sim_msg ("\r|  lockYield     %'15llu  |\r\n", lockYieldAll);
#  else
        sim_msg ("\r|  lockYield                ----  |\r\n");
#  endif /* ifndef SCHED_NEVER_YIELD */
        sim_msg ("\r+---------------------------------+\r\n");
        (void)fflush(stderr);
        (void)fflush(stdout);
# endif /* ifdef WIN_STDIO */
# ifndef PERF_STRIP
      }
# else
    sim_msg("\r\n");
# endif
#endif

#if 0
    for (int i = 0; i < N_FAULTS; i ++)
      {
        if (cpu.faultCnt [i])
          sim_msg  ("%s faults = %llu\r\n",
                      faultNames [i], (unsigned long long)cpu.faultCnt [i]);
      }
#endif

#if defined(THREADZ) || defined(LOCKLESS)
    stopCPUThread();
#endif

#ifdef M_SHARED
// scp needs to have the IC statically allocated, so a placeholder
// was created. Update the placeholder so the IC can be seen via scp
// and restarting sim_instr won't lose the place.

    set_cpu_idx (0);
    dummy_IC = cpu.PPR.IC;
#endif

    return reason;
  }

/*
 * cd@libertyhaven.com - sez ...
 *  If the instruction addresses a block of four words, the target of the
 * instruction is supposed to be an address that is aligned on a four-word
 * boundary (0 mod 4). If not, the processor will grab the four-word block
 * containing that address that begins on a four-word boundary, even if it
 * has to go back 1 to 3 words. Analogous explanation for 8, 16, and 32 cases.
 *
 * olin@olinsibert.com - sez ...
 *  It means that the appropriate low bits of the address are forced to zero.
 * So it's the previous words, not the succeeding words, that are used to
 * satisfy the request. -- Olin
 */

int operand_size (void)
  {
    DCDstruct * i = & cpu.currentInstruction;
    if (i->info->flags & (READ_OPERAND | STORE_OPERAND))
        return 1;
    else if (i->info->flags & (READ_YPAIR | STORE_YPAIR))
        return 2;
    else if (i->info->flags & (READ_YBLOCK8 | STORE_YBLOCK8))
        return 8;
    else if (i->info->flags & (READ_YBLOCK16 | STORE_YBLOCK16))
        return 16;
    else if (i->info->flags & (READ_YBLOCK32 | STORE_YBLOCK32))
        return 32;
    return 0;
  }

// read instruction operands

void readOperandRead (word18 addr) {
  CPT (cpt1L, 6); // read_operand

#ifdef THREADZ
  DCDstruct * i = & cpu.currentInstruction;
  if (RMWOP (i)) // ldac, ldqc, stac, stacq, snzc
    lock_rmw ();
#endif

  switch (operand_size ()) {
    case 1:
      CPT (cpt1L, 7); // word
      ReadOperandRead (addr, & cpu.CY);
      break;
    case 2:
      CPT (cpt1L, 8); // double word
      addr &= 0777776;   // make even
      Read2OperandRead (addr, cpu.Ypair);
      break;
    case 8:
      CPT (cpt1L, 9); // oct word
      addr &= 0777770;   // make on 8-word boundary
      Read8 (addr, cpu.Yblock8, cpu.currentInstruction.b29);
      break;
    case 16:
      CPT (cpt1L, 10); // 16 words
      addr &= 0777770;   // make on 8-word boundary
      Read16 (addr, cpu.Yblock16);
      break;
    case 32:
      CPT (cpt1L, 11); // 32 words
      addr &= 0777740;   // make on 32-word boundary
      for (uint j = 0 ; j < 32 ; j += 1)
        ReadOperandRead (addr + j, cpu.Yblock32 + j);
      break;
  }
}

void readOperandRMW (word18 addr) {
  CPT (cpt1L, 6); // read_operand
  switch (operand_size ()) {
    case 1:
      CPT (cpt1L, 7); // word
      ReadOperandRMW (addr, & cpu.CY);
      break;
    case 2:
      CPT (cpt1L, 8); // double word
      addr &= 0777776;   // make even
      Read2OperandRead (addr, cpu.Ypair);
      break;
    case 8:
      CPT (cpt1L, 9); // oct word
      addr &= 0777770;   // make on 8-word boundary
      Read8 (addr, cpu.Yblock8, cpu.currentInstruction.b29);
      break;
    case 16:
      CPT (cpt1L, 10); // 16 words
      addr &= 0777770;   // make on 8-word boundary
      Read16 (addr, cpu.Yblock16);
      break;
    case 32:
      CPT (cpt1L, 11); // 32 words
      addr &= 0777740;   // make on 32-word boundary
      for (uint j = 0 ; j < 32 ; j += 1)
        ReadOperandRMW (addr + j, cpu.Yblock32 + j);
      break;
  }
}

// write instruction operands

t_stat write_operand (word18 addr, UNUSED processor_cycle_type cyctyp)
  {
    switch (operand_size ())
      {
        case 1:
            CPT (cpt1L, 12); // word
            WriteOperandStore (addr, cpu.CY);
            break;
        case 2:
            CPT (cpt1L, 13); // double word
            addr &= 0777776;   // make even
            Write2OperandStore (addr + 0, cpu.Ypair);
            break;
        case 8:
            CPT (cpt1L, 14); // 8 words
            addr &= 0777770;   // make on 8-word boundary
            Write8 (addr, cpu.Yblock8, cpu.currentInstruction.b29);
            break;
        case 16:
            CPT (cpt1L, 15); // 16 words
            addr &= 0777770;   // make on 8-word boundary
            Write16 (addr, cpu.Yblock16);
            break;
        case 32:
            CPT (cpt1L, 16); // 32 words
            addr &= 0777740;   // make on 32-word boundary
            //for (uint j = 0 ; j < 32 ; j += 1)
                //Write (addr + j, cpu.Yblock32[j], OPERAND_STORE);
            Write32 (addr, cpu.Yblock32);
            break;
      }

#ifdef THREADZ
    if (cyctyp == OPERAND_STORE)
      {
        DCDstruct * i = & cpu.currentInstruction;
        if (RMWOP (i))
          unlock_mem ();
      }
#endif
    return SCPE_OK;

  }

#ifndef SPEED
t_stat set_mem_watch (int32 arg, const char * buf)
  {
    if (strlen (buf) == 0)
      {
        if (arg)
          {
            sim_warn ("no argument to watch?\n");
            return SCPE_ARG;
          }
        sim_msg ("Clearing all watch points\n");
        (void)memset (& watch_bits, 0, sizeof (watch_bits));
        return SCPE_OK;
      }
    char * end;
    long int n = strtol (buf, & end, 0);
    if (* end || n < 0 || n >= MEMSIZE)
      {
        sim_warn ("Invalid argument to watch? %ld\n", (long) n);
        return SCPE_ARG;
      }
    watch_bits [n] = arg != 0;
    return SCPE_OK;
  }
#endif

/*!
 * "Raw" core interface ....
 */

#ifndef SPEED
static void nem_check (word24 addr, const char * context)
  {
    if (lookup_cpu_mem_map (addr) < 0)
      {
        doFault (FAULT_STR, fst_str_nea,  context);
      }
  }
#endif

// static uint get_scu_unit_idx (word24 addr, word24 * offset)
//   {
//     int cpu_port_num = lookup_cpu_mem_map (addr, offset);
//     if (cpu_port_num < 0) // Can't happen, we passed nem_check above
//       {
//         sim_warn ("cpu_port_num < 0");
//         doFault (FAULT_STR, fst_str_nea,  __func__);
//       }
//     return cables->cpu_to_scu [current_running_cpu_idx][cpu_port_num].scu_unit_idx;
//   }

#if !defined(SPEED) || !defined(INLINE_CORE)
int core_read (word24 addr, word36 *data, const char * ctx)
  {
    PNL (cpu.portBusy = true;)
    SC_MAP_ADDR (addr, addr);
# ifndef LOCKLESS
    if (M[addr] & MEM_UNINITIALIZED)
      {
        sim_debug (DBG_WARN, & cpu_dev,
                   "Uninitialized memory accessed at address %08o; "
                   "IC is 0%06o:0%06o (%s(\n",
                   addr, cpu.PPR.PSR, cpu.PPR.IC, ctx);
      }
# endif
# ifndef SPEED
    if (watch_bits [addr])
      {
        sim_msg ("WATCH [%llu] %05o:%06o read   %08o %012llo (%s)\n",
                    (long long unsigned int)cpu.cycleCnt, cpu.PPR.PSR, cpu.PPR.IC, addr,
                    (long long unsigned int)M [addr], ctx);
        traceInstruction (0);
      }
# endif
# ifdef LOCKLESS
#  ifndef SUNLINT
    word36 v;
    LOAD_ACQ_CORE_WORD(v, addr);
    *data = v & DMASK;
#  endif /* ifndef SUNLINT */
# else
    *data = M[addr] & DMASK;
# endif

# ifdef TR_WORK_MEM
    cpu.rTRticks ++;
# endif
    sim_debug (DBG_CORE, & cpu_dev,
               "core_read  %08o %012"PRIo64" (%s)\n",
                addr, * data, ctx);
    PNL (trackport (addr, * data));
    return 0;
  }
#endif

#ifdef LOCKLESS
int core_read_lock (word24 addr, word36 *data, UNUSED const char * ctx)
{
    SC_MAP_ADDR (addr, addr);
    LOCK_CORE_WORD(addr);
    if (cpu.locked_addr != 0) {
      sim_warn ("core_read_lock: locked %08o locked_addr %08o %c %05o:%06o\n",
                addr, cpu.locked_addr, current_running_cpu_idx + 'A',
                cpu.PPR.PSR, cpu.PPR.IC);
      core_unlock_all ();
    }
    cpu.locked_addr = addr;
# ifndef SUNLINT
    word36 v;
    LOAD_ACQ_CORE_WORD(v, addr);
    * data = v & DMASK;
# endif /* ifndef SUNLINT */
    return 0;
}
#endif

#if !defined(SPEED) || !defined(INLINE_CORE)
int core_write (word24 addr, word36 data, const char * ctx)
  {
    PNL (cpu.portBusy = true;)
    SC_MAP_ADDR (addr, addr);
    if (cpu.tweaks.isolts_mode)
      {
        if (cpu.MR.sdpap)
          {
            sim_warn ("failing to implement sdpap\n");
            cpu.MR.sdpap = 0;
          }
        if (cpu.MR.separ)
          {
            sim_warn ("failing to implement separ\n");
                cpu.MR.separ = 0;
          }
      }
# ifdef LOCKLESS
    LOCK_CORE_WORD(addr);
#  ifndef SUNLINT
    STORE_REL_CORE_WORD(addr, data);
#  endif /* ifndef SUNLINT */
# else
    M[addr] = data & DMASK;
# endif
# ifndef SPEED
    if (watch_bits [addr])
      {
        sim_msg ("WATCH [%llu] %05o:%06o write  %08llo %012llo (%s)\n",
                 (long long unsigned int)cpu.cycleCnt, cpu.PPR.PSR, cpu.PPR.IC,
                 (long long unsigned int)addr, (unsigned long long int)M [addr], ctx);
        traceInstruction (0);
      }
# endif
# ifdef TR_WORK_MEM
    cpu.rTRticks ++;
# endif
    sim_debug (DBG_CORE, & cpu_dev,
               "core_write %08o %012"PRIo64" (%s)\n",
                addr, data, ctx);
    PNL (trackport (addr, data));
    return 0;
  }
#endif

#ifdef LOCKLESS
int core_write_unlock (word24 addr, word36 data, UNUSED const char * ctx)
{
    SC_MAP_ADDR (addr, addr);
    if (cpu.locked_addr != addr)
      {
        sim_warn ("core_write_unlock: locked %08o locked_addr %08o %c %05o:%06o\n",
                  addr,        cpu.locked_addr, current_running_cpu_idx + 'A',
                  cpu.PPR.PSR, cpu.PPR.IC);
       core_unlock_all ();
      }

# ifndef SUNLINT
    STORE_REL_CORE_WORD(addr, data);
# endif /* ifndef SUNLINT */
    cpu.locked_addr = 0;
    return 0;
}

int core_unlock_all (void)
{
  if (cpu.locked_addr != 0) {
      sim_warn ("core_unlock_all: locked %08o %c %05o:%06o\n",
                cpu.locked_addr, current_running_cpu_idx + 'A',
                cpu.PPR.PSR,     cpu.PPR.IC);
# ifndef SUNLINT
      STORE_REL_CORE_WORD(cpu.locked_addr, M[cpu.locked_addr]);
# endif /* ifndef SUNLINT */
      cpu.locked_addr = 0;
  }
  return 0;
}
#endif

#if !defined(SPEED) || !defined(INLINE_CORE)
int core_write_zone (word24 addr, word36 data, const char * ctx)
  {
    PNL (cpu.portBusy = true;)
    if (cpu.tweaks.isolts_mode)
      {
        if (cpu.MR.sdpap)
          {
            sim_warn ("failing to implement sdpap\n");
            cpu.MR.sdpap = 0;
          }
        if (cpu.MR.separ)
          {
            sim_warn ("failing to implement separ\n");
            cpu.MR.separ = 0;
          }
      }
    word24 mapAddr = 0;
    SC_MAP_ADDR (addr, mapAddr);
# ifdef LOCKLESS
    word36 v;
    core_read_lock(addr,  &v, ctx);
    v = (v & ~cpu.zone) | (data & cpu.zone);
    core_write_unlock(addr, v, ctx);
# else
    M[mapAddr] = (M[mapAddr] & ~cpu.zone) | (data & cpu.zone);
# endif
    cpu.useZone = false; // Safety
# ifndef SPEED
    if (watch_bits [mapAddr])
      {
        sim_msg ("WATCH [%llu] %05o:%06o writez %08llo %012llo (%s)\n",
                (unsigned long long int)cpu.cycleCnt, cpu.PPR.PSR, cpu.PPR.IC,
                (unsigned long long int)mapAddr, (unsigned long long int)M [mapAddr], ctx);
        traceInstruction (0);
      }
# endif
# ifdef TR_WORK_MEM
    cpu.rTRticks ++;
# endif
    sim_debug (DBG_CORE, & cpu_dev,
               "core_write_zone %08o %012"PRIo64" (%s)\n",
                mapAddr, data, ctx);
    PNL (trackport (mapAddr, data));
    return 0;
  }
#endif

#if !defined(SPEED) || !defined(INLINE_CORE)
int core_read2 (word24 addr, word36 *even, word36 *odd, const char * ctx)
  {
    PNL (cpu.portBusy = true;)
# if defined(LOCKLESS)
    /*LINTED E_FUNC_VAR_UNUSED*/ /* Appease SUNLINT */
    word36 v;
# endif
    if (addr & 1)
      {
        sim_debug (DBG_MSG, & cpu_dev,
                   "warning: subtracting 1 from pair at %o in "
                   "core_read2 (%s)\n", addr, ctx);
        addr &= (word24)~1; /* make it an even address */
      }
    SC_MAP_ADDR (addr, addr);
# ifndef LOCKLESS
    if (M[addr] & MEM_UNINITIALIZED)
      {
        sim_debug (DBG_WARN, & cpu_dev,
                   "Uninitialized memory accessed at address %08o; "
                   "IC is 0%06o:0%06o (%s)\n",
                   addr, cpu.PPR.PSR, cpu.PPR.IC, ctx);
      }
# endif
# ifndef SPEED
    if (watch_bits [addr])
      {
        sim_msg ("WATCH [%llu] %05o:%06o read2  %08llo %012llo (%s)\n",
                 (unsigned long long int)cpu.cycleCnt, cpu.PPR.PSR, cpu.PPR.IC,
                 (unsigned long long int)addr, (unsigned long long int)M [addr], ctx);
        traceInstruction (0);
      }
# endif
# ifdef LOCKLESS
#  ifndef SUNLINT
    LOAD_ACQ_CORE_WORD(v, addr);
    if (v & MEM_LOCKED)
      sim_warn ("core_read2: even locked %08o locked_addr %08o %c %05o:%06o\n",
                addr,        cpu.locked_addr, current_running_cpu_idx + 'A',
                cpu.PPR.PSR, cpu.PPR.IC);
    *even = v & DMASK;
    addr++;
#  endif /* ifndef SUNLINT */
# else
    *even = M[addr++] & DMASK;
# endif
    sim_debug (DBG_CORE, & cpu_dev,
               "core_read2 %08o %012"PRIo64" (%s)\n",
                addr - 1, * even, ctx);

    // if the even address is OK, the odd will be
    //nem_check (addr,  "core_read2 nem");
# ifndef LOCKLESS
    if (M[addr] & MEM_UNINITIALIZED)
      {
        sim_debug (DBG_WARN, & cpu_dev,
                   "Uninitialized memory accessed at address %08o; "
                   "IC is 0%06o:0%06o (%s)\n",
                    addr, cpu.PPR.PSR, cpu.PPR.IC, ctx);
      }
# endif
# ifndef SPEED
    if (watch_bits [addr])
      {
        sim_msg ("WATCH [%llu] %05o:%06o read2  %08llo %012llo (%s)\n",
                 (unsigned long long int)cpu.cycleCnt, cpu.PPR.PSR, cpu.PPR.IC,
                 (unsigned long long int)addr, (unsigned long long int)M [addr], ctx);
        traceInstruction (0);
      }
# endif
# ifdef LOCKLESS
#  ifndef SUNLINT
    LOAD_ACQ_CORE_WORD(v, addr);
    if (v & MEM_LOCKED)
      sim_warn ("core_read2: odd locked %08o locked_addr %08o %c %05o:%06o\n",
                addr,        cpu.locked_addr, current_running_cpu_idx + 'A',
                cpu.PPR.PSR, cpu.PPR.IC);
    *odd = v & DMASK;
#  endif /* ifndef SUNLINT */
# else
    *odd = M[addr] & DMASK;
# endif
    sim_debug (DBG_CORE, & cpu_dev,
               "core_read2 %08o %012"PRIo64" (%s)\n",
                addr, * odd, ctx);
# ifdef TR_WORK_MEM
    cpu.rTRticks ++;
# endif
    PNL (trackport (addr - 1, * even));
    return 0;
  }
#endif

#if !defined(SPEED) || !defined(INLINE_CORE)
int core_write2 (word24 addr, word36 even, word36 odd, const char * ctx) {
  PNL (cpu.portBusy = true;)
  if (addr & 1) {
    sim_debug (DBG_MSG, & cpu_dev,
               "warning: subtracting 1 from pair at %o in core_write2 " "(%s)\n",
               addr, ctx);
    addr &= (word24)~1; /* make it even a dress, or iron a skirt ;) */
  }
  SC_MAP_ADDR (addr, addr);
  if (cpu.tweaks.isolts_mode) {
    if (cpu.MR.sdpap) {
      sim_warn ("failing to implement sdpap\n");
      cpu.MR.sdpap = 0;
    }
    if (cpu.MR.separ) {
      sim_warn ("failing to implement separ\n");
      cpu.MR.separ = 0;
    }
  }

# ifndef SPEED
  if (watch_bits [addr]) {
    sim_msg ("WATCH [%llu] %05o:%06o write2 %08llo %012llo (%s)\n",
             (unsigned long long int)cpu.cycleCnt, cpu.PPR.PSR, cpu.PPR.IC,
             (unsigned long long int)addr, (unsigned long long int)even, ctx);
    traceInstruction (0);
  }
# endif
# ifdef LOCKLESS
  LOCK_CORE_WORD(addr);
#  ifndef SUNLINT
  STORE_REL_CORE_WORD(addr, even);
#  endif /* ifndef SUNLINT */
  addr++;
# else
  M[addr++] = even & DMASK;
# endif
  sim_debug (DBG_CORE, & cpu_dev, "core_write2 %08o %012llo (%s)\n", addr - 1,
          (long long unsigned int)even, ctx);

  // If the even address is OK, the odd will be
  //mem_check (addr,  "core_write2 nem");

# ifndef SPEED
  if (watch_bits [addr]) {
    sim_msg ("WATCH [%llu] %05o:%06o write2 %08llo %012llo (%s)\n",
             (long long unsigned int)cpu.cycleCnt, cpu.PPR.PSR, cpu.PPR.IC,
             (long long unsigned int)addr, (long long unsigned int)odd, ctx);
    traceInstruction (0);
  }
# endif
# ifdef LOCKLESS
  LOCK_CORE_WORD(addr);
#  ifndef SUNLINT
  STORE_REL_CORE_WORD(addr, odd);
#  endif /* ifndef SUNLINT */
# else
  M[addr] = odd & DMASK;
# endif
# ifdef TR_WORK_MEM
  cpu.rTRticks ++;
# endif
  PNL (trackport (addr - 1, even));
  sim_debug (DBG_CORE, & cpu_dev, "core_write2 %08o %012"PRIo64" (%s)\n", addr, odd, ctx);
  return 0;
}
#endif

/*
 * Instruction fetcher ...
 * Fetch + decode instruction at 18-bit address 'addr'
 */

/*
 * Instruction decoder .....
 */

void decode_instruction (word36 inst, DCDstruct * p)
  {
    CPT (cpt1L, 17); // instruction decoder
    (void)memset (p, 0, sizeof (DCDstruct));

    p->opcode   = GET_OP (inst);   // get opcode
    p->opcodeX  = GET_OPX(inst);   // opcode extension
    p->opcode10 = p->opcode | (p->opcodeX ? 01000 : 0); //-V536
    p->address  = GET_ADDR (inst); // address field from instruction
    p->b29      = GET_A (inst);    // "A" the indirect via pointer register flag
    p->i        = GET_I (inst);    // "I" inhibit interrupt flag
    p->tag      = GET_TAG (inst);  // instruction tag

    p->info     = get_iwb_info  (p);     // get info for IWB instruction

    if (p->info->flags & IGN_B29)
        p->b29 = 0;   // make certain 'a' bit is valid always

    if (p->info->ndes > 0)
      {
        p->b29 = 0;
        p->tag = 0;
        if (p->info->ndes > 1)
          {
            (void)memset (& cpu.currentEISinstruction, 0,
                          sizeof (cpu.currentEISinstruction));
          }
      }
  }

// MM stuff ...

//
// is_priv_mode()
//
// Report whether or or not the CPU is in privileged mode.
// True if in absolute mode or if priv bit is on in segment TPR.TSR
// The processor executes instructions in privileged mode when forming
// addresses in absolute mode or when forming addresses in append mode and the
// segment descriptor word (SDW) for the segment in execution specifies a
// privileged procedure and the execution ring is equal to zero.
//
// PPR.P A flag controlling execution of privileged instructions.
//
// Its value is 1 (permitting execution of privileged instructions) if PPR.PRR
// is 0 and the privileged bit in the segment descriptor word (SDW.P) for the
// procedure is 1; otherwise, its value is 0.
//

int is_priv_mode (void)
  {

// Back when it was ABS/APP/BAR, this test was right; now that
// it is ABS/APP,BAR/NBAR, check bar mode.
// Fixes ISOLTS 890 05a.
    if (get_bar_mode ())
      return 0;

// PPR.P is only relevant if we're in APPEND mode. ABSOLUTE mode ignores it.
    if (get_addr_mode () == ABSOLUTE_mode)
      return 1;
    else if (cpu.PPR.P)
      return 1;

    return 0;
  }

/*
 * get_bar_mode: During fault processing, we do not want to fetch and execute
 * the fault vector instructions in BAR mode. We leverage the
 * secret_addressing_mode flag that is set in set_TEMPORARY_ABSOLUTE_MODE to
 * direct us to ignore the I_NBAR indicator register.
 */

bool get_bar_mode (void)
  {
    return ! (cpu.secret_addressing_mode || TST_I_NBAR);
  }

addr_modes_e get_addr_mode (void)
  {
    if (cpu.secret_addressing_mode)
        return ABSOLUTE_mode; // This is not the mode you are looking for

    // went_appending does not alter privileged state (only enables appending)
    // the went_appending check is only required by ABSA, AFAICT
    // pft 02b 013255, ISOLTS-860
    //if (cpu.went_appending)
    //    return APPEND_mode;

    if (TST_I_ABS)
      {
          return ABSOLUTE_mode;
      }
    else
      {
          return APPEND_mode;
      }
  }

/*
 * set_addr_mode()
 *
 * Put the CPU into the specified addressing mode.   This involves
 * setting a couple of IR flags and the PPR priv flag.
 */

void set_addr_mode (addr_modes_e mode)
  {
//    cpu.cu.XSF = false;
//sim_debug (DBG_TRACEEXT, & cpu_dev, "set_addr_mode bit 29 sets XSF to 0\n");
    //cpu.went_appending = false;
// Temporary hack to fix fault/intr pair address mode state tracking
//   1. secret_addressing_mode is only set in fault/intr pair processing.
//   2. Assume that the only set_addr_mode that will occur is the b29 special
//   case or ITx.
    //if (secret_addressing_mode && mode == APPEND_mode)
      //set_went_appending ();

    cpu.secret_addressing_mode = false;
    if (mode == ABSOLUTE_mode)
      {
        CPT (cpt1L, 22); // set abs mode
        sim_debug (DBG_DEBUG, & cpu_dev, "APU: Setting absolute mode.\n");

        SET_I_ABS;
        cpu.PPR.P = 1;
      }
    else if (mode == APPEND_mode)
      {
        CPT (cpt1L, 23); // set append mode
        if (! TST_I_ABS && TST_I_NBAR)
          sim_debug (DBG_DEBUG, & cpu_dev, "APU: Keeping append mode.\n");
        else
          sim_debug (DBG_DEBUG, & cpu_dev, "APU: Setting append mode.\n");

        CLR_I_ABS;
      }
    else
      {
        sim_debug (DBG_ERR, & cpu_dev,
                  "APU: Unable to determine address mode.\n");
        sim_warn ("APU: Unable to determine address mode. Can't happen!\n");
      }
  }

/*
 * stuff to handle BAR mode ...
 */

/*
 * The Base Address Register provides automatic hardware Address relocation and
 * Address range limitation when the processor is in BAR mode.
 *
 * BAR.BASE: Contains the 9 high-order bits of an 18-bit address relocation
 * constant. The low-order bits are generated as zeros.
 *
 * BAR.BOUND: Contains the 9 high-order bits of the unrelocated address limit.
 * The low- order bits are generated as zeros. An attempt to access main memory
 * beyond this limit causes a store fault, out of bounds. A value of 0 is truly
 * 0, indicating a null memory range.
 *
 * In BAR mode, the base address register (BAR) is used. The BAR contains an
 * address bound and a base address. All computed addresses are relocated by
 * adding the base address. The relocated address is combined with the
 * procedure pointer register to form the virtual memory address. A program is
 * kept within certain limits by subtracting the unrelocated computed address
 * from the address bound. If the result is zero or negative, the relocated
 * address is out of range, and a store fault occurs.
 */

// CANFAULT
word18 get_BAR_address (word18 addr)
  {
    if (cpu . BAR.BOUND == 0)
        // store fault, out of bounds.
        doFault (FAULT_STR, fst_str_oob, "BAR store fault; out of bounds");

    // A program is kept within certain limits by subtracting the
    // unrelocated computed address from the address bound. If the result
    // is zero or negative, the relocated address is out of range, and a
    // store fault occurs.
    //
    // BAR.BOUND - CA <= 0
    // BAR.BOUND <= CA
    // CA >= BAR.BOUND
    //
    if (addr >= (((word18) cpu . BAR.BOUND) << 9))
        // store fault, out of bounds.
        doFault (FAULT_STR, fst_str_oob, "BAR store fault; out of bounds");

    word18 barAddr = (addr + (((word18) cpu . BAR.BASE) << 9)) & 0777777;
    return barAddr;
  }

//=============================================================================

static void add_history (uint hset, word36 w0, word36 w1)
  {
    //if (cpu.MR.emr)
      {
        cpu.history [hset] [cpu.history_cyclic[hset]] [0] = w0;
        cpu.history [hset] [cpu.history_cyclic[hset]] [1] = w1;
        cpu.history_cyclic[hset] = (cpu.history_cyclic[hset] + 1) % N_MODEL_HIST_SIZE;
      }
  }

void add_history_force (uint hset, word36 w0, word36 w1)
  {
    cpu.history [hset] [cpu.history_cyclic[hset]] [0] = w0;
    cpu.history [hset] [cpu.history_cyclic[hset]] [1] = w1;
    cpu.history_cyclic[hset] = (cpu.history_cyclic[hset] + 1) % N_MODEL_HIST_SIZE;
  }

void add_dps8m_CU_history (void)
  {
    if (cpu.skip_cu_hist)
      return;
    if (! cpu.MR_cache.emr)
      return;
    if (! cpu.MR_cache.ihr)
      return;
    if (cpu.MR_cache.hrxfr && ! cpu.wasXfer)
      return;

    word36 flags   = 0; // XXX fill out
    word5 proccmd  = 0; // XXX fill out
    word7 flags2   = 0; // XXX fill out
    word36 w0      = 0, w1 = 0;
    w0            |= flags & 0777777000000;
    w0            |= IWB_IRODD & MASK18;
    w1            |= (cpu.iefpFinalAddress & MASK24) << 12;
    w1            |= (proccmd & MASK5) << 7;
    w1            |= flags2 & 0176;
    add_history (CU_HIST_REG, w0, w1);
  }

#ifndef QUIET_UNUSED
void add_dps8m_DU_OU_history (word36 flags, word18 ICT, word9 RS_REG, word9 flags2)
  {
    word36 w0  = flags, w1 = 0;
    w1        |= (ICT & MASK18) << 18;
    w1        |= (RS_REG & MASK9) << 9;
    w1        |= flags2 & MASK9;
    add_history (DPS8M_DU_OU_HIST_REG, w0, w1);
  }

void add_dps8m_APU_history (word15 ESN, word21 flags, word24 RMA, word3 RTRR, word9 flags2)
  {
    word36 w0  = 0, w1 = 0;
    w0        |= (ESN & MASK15) << 21;
    w0        |= flags & MASK21;
    w1        |= (RMA & MASK24) << 12;
    w1        |= (RTRR & MASK3) << 9;
    w1        |= flags2 & MASK9;
    add_history (cpu.tweaks.l68_mode ? L68_APU_HIST_REG : DPS8M_APU_HIST_REG, w0, w1);
  }

void add_dps8m_EAPU_history (word18 ZCA, word18 opcode)
  {
    word36 w0  = 0;
    w0        |= (ZCA & MASK18) << 18;
    w0        |= opcode & MASK18;
    add_history (DPS8M_EAPU_HIST_REG, w0, 0);
    //cpu.eapu_hist[cpu.eapu_cyclic].ZCA = ZCA;
    //cpu.eapu_hist[cpu.eapu_cyclic].opcode = opcode;
    //cpu.history_cyclic[DPS8M_EAPU_HIST_REG] =
      //(cpu.history_cyclic[DPS8M_EAPU_HIST_REG] + 1) % N_DPS8M_HIST_SIZE;
  }
#endif

// According to ISOLTS
//
//   0 PIA
//   1 POA
//   2 RIW
//   3 SIW
//   4 POT
//   5 PON
//   6 RAW
//   7 SAW
//   8 TRGO
//   9 XDE
//  10 XDO
//  11 IC
//  12 RPTS
//  13 WI
//  14 AR F/E
//  15 XIP
//  16 FLT
//  17 COMPL. ADD BASE
//  18:23 OPCODE/TAG
//  24:29 ADDREG
//  30:34 COMMAND A/B/C/D/E
//  35:38 PORT A/B/C/D
//  39 FB XEC
//  40 INS FETCH
//  41 CU STORE
//  42 OU STORE
//  43 CU LOAD
//  44 OU LOAD
//  45 RB DIRECT
//  46 -PC BUSY
//  47 PORT BUSY

void add_l68_CU_history (void)
  {
    CPT (cpt1L, 24); // add cu hist
// XXX strobe on opcode match
    if (cpu.skip_cu_hist)
      return;
    if (! cpu.MR_cache.emr)
      return;
    if (! cpu.MR_cache.ihr)
      return;

    word36 w0 = 0, w1 = 0;

    // 0 PIA
    // 1 POA
    // 2 RIW
    // 3 SIW
    // 4 POT
    // 5 PON
    // 6 RAW
    // 7 SAW
    PNL (putbits36_8 (& w0, 0, cpu.prepare_state);)
    // 8 TRG
    putbits36_1  (& w0, 8, cpu.wasXfer);
    // 9 XDE
    putbits36_1  (& w0, 9, cpu.cu.xde);
    // 10 XDO
    putbits36_1  (& w0, 10, cpu.cu.xdo);
    // 11 IC
    putbits36_1  (& w0, 11, USE_IRODD?1:0);
    // 12 RPT
    putbits36_1  (& w0, 12, cpu.cu.rpt);
    // 13 WI Wait for instruction fetch XXX Not tracked
    // 14 ARF "AR F/E" Address register Full/Empty Address has valid data
    PNL (putbits36_1 (& w0, 14, cpu.AR_F_E);)
    // 15 !XA/Z "-XIP NOT prepare interrupt address"
    putbits36_1  (& w0, 15, cpu.cycle != INTERRUPT_cycle?1:0);
    // 16 !FA/Z Not tracked. (cu.-FL?)
    putbits36_1  (& w0, 16, cpu.cycle != FAULT_cycle?1:0);
    // 17 M/S  (master/slave, cu.-BASE?, NOT BAR MODE)
    putbits36_1  (& w0, 17, TSTF (cpu.cu.IR, I_NBAR)?1:0);
    // 18:35 IWR (lower half of IWB)
    putbits36_18 (& w0, 18, (word18) (IWB_IRODD & MASK18));

    // 36:53 CA
    putbits36_18 (& w1, 0, cpu.TPR.CA);
    // 54:58 CMD system controller command XXX
    // 59:62 SEL port select (XXX ignoring "only valid if port A-D is selected")
    PNL (putbits36_1 (& w1, 59-36, (cpu.portSelect == 0)?1:0);)
    PNL (putbits36_1 (& w1, 60-36, (cpu.portSelect == 1)?1:0);)
    PNL (putbits36_1 (& w1, 61-36, (cpu.portSelect == 2)?1:0);)
    PNL (putbits36_1 (& w1, 62-36, (cpu.portSelect == 3)?1:0);)
    // 63 XEC-INT An interrupt is present
    putbits36_1 (& w1, 63-36, cpu.interrupt_flag?1:0);
    // 64 INS-FETCH Perform an instruction fetch
    PNL (putbits36_1 (& w1, 64-36, cpu.INS_FETCH?1:0);)
    // 65 CU-STORE Control unit store cycle XXX
    // 66 OU-STORE Operations unit store cycle XXX
    // 67 CU-LOAD Control unit load cycle XXX
    // 68 OU-LOAD Operations unit load cycle XXX
    // 69 DIRECT Direct cycle XXX
    // 70 -PC-BUSY Port control logic not busy XXX
    // 71 BUSY Port interface busy XXX

    add_history (CU_HIST_REG, w0, w1);

    // Check for overflow
    CPTUR (cptUseMR);
    if (cpu.MR.hrhlt && cpu.history_cyclic[CU_HIST_REG] == 0)
      {
        //cpu.history_cyclic[CU_HIST_REG] = 15;
        if (cpu.MR.ihrrs)
          {
            cpu.MR.ihr = 0;
          }
        set_FFV_fault (4);
        return;
      }
  }

// du history register inputs(actual names)
// bit 00= fpol-cx;010       bit 36= fdud-dg;112
// bit 01= fpop-cx;010       bit 37= fgdlda-dc;010
// bit 02= need-desc-bd;000  bit 38= fgdldb-dc;010
// bit 03= sel-adr-bd;000    bit 39= fgdldc-dc;010
// bit 04= dlen=direct-bd;000bit 40= fnld1-dp;110
// bit 05= dfrst-bd;021      bit 41= fgldp1-dc;110
// bit 06= fexr-bd;010       bit 42= fnld2-dp;110
// bit 07= dlast-frst-bd;010 bit 43= fgldp2-dc;110
// bit 08= ddu-ldea-bd;000   bit 44= fanld1-dp;110
// bit 09= ddu-stea-bd;000   bit 45= fanld2-dp;110
// bit 10= dredo-bd;030      bit 46= fldwrt1-dp;110
// bit 11= dlvl<wd-sz-bg;000 bit 47= fldwrt2-dp;110
// bit 12= exh-bg;000        bit 48= data-avldu-cm;000
// bit 13= dend-seg-bd;111   bit 49= fwrt1-dp;110
// bit 14= dend-bd;000       bit 50= fgstr-dc;110
// bit 15= du=rd+wrt-bd;010  bit 51= fanstr-dp;110
// bit 16= ptra00-bd;000     bit 52= fstr-op-av-dg;010
// bit 17= ptra01-bd;000     bit 53= fend-seg-dg;010
// bit 18= fa/i1-bd;110      bit 54= flen<128-dg;010
// bit 19= fa/i2-bd;110      bit 55= fgch-dp;110
// bit 20= fa/i3-bd;110      bit 56= fanpk-dp;110
// bit 21= wrd-bd;000        bit 57= fexmop-dl;110
// bit 22= nine-bd;000       bit 58= fblnk-dp;100
// bit 23= six-bd;000        bit 59= unused
// bit 24= four-bd;000       bit 60= dgbd-dc;100
// bit 25= bit-bd;000        bit 61= dgdb-dc;100
// bit 26= unused            bit 62= dgsp-dc;100
// bit 27= unused            bit 63= ffltg-dc;110
// bit 28= unused            bit 64= frnd-dg;120
// bit 29= unused            bit 65= dadd-gate-dc;100
// bit 30= fsampl-bd;111     bit 66= dmp+dv-gate-db;100
// bit 31= dfrst-ct-bd;010   bit 67= dxpn-gate-dg;100
// bit 32= adj-lenint-cx;000 bit 68= unused
// bit 33= fintrptd-cx;010   bit 69= unused
// bit 34= finhib-stc1-cx;010bit 70= unused
// bit 35= unused            bit 71= unused

void add_l68_DU_history (void)
  {
    CPT (cpt1L, 25); // add du hist
    PNL (add_history (L68_DU_HIST_REG, cpu.du.cycle1, cpu.du.cycle2);)
  }

void add_l68_OU_history (void)
  {
    CPT (cpt1L, 26); // add ou hist
    word36 w0 = 0, w1 = 0;

    // 0-16 RP
    //   0-8 OP CODE
    PNL (putbits36_9 (& w0,  0,       cpu.ou.RS);)

    //   9 CHAR
    putbits36_1 (& w0,       9,       cpu.ou.characterOperandSize ? 1 : 0);

    //   10-12 TAG 1/2/3
    putbits36_3 (& w0,       10,      cpu.ou.characterOperandOffset);

    //   13 CRFLAG
    putbits36_1 (& w0,       13,      cpu.ou.crflag);

    //   14 DRFLAG
    putbits36_1 (& w0,       14,      cpu.ou.directOperandFlag ? 1 : 0);

    //   15-16 EAC
    putbits36_2 (& w0,       15,      cpu.ou.eac);

    // 17 0
    // 18-26 RS REG
    PNL (putbits36_9 (& w0,  18,      cpu.ou.RS);)

    // 27 RB1 FULL
    putbits36_1 (& w0,       27,      cpu.ou.RB1_FULL);

    // 28 RP FULL
    putbits36_1 (& w0,       28,      cpu.ou.RP_FULL);

    // 29 RS FULL
    putbits36_1 (& w0,       29,      cpu.ou.RS_FULL);

    // 30-35 GIN/GOS/GD1/GD2/GOE/GOA
    putbits36_6 (& w0,       30,      (word6) (cpu.ou.cycle >> 3));

    // 36-38 GOM/GON/GOF
    putbits36_3 (& w1,       36-36,   (word3) cpu.ou.cycle);

    // 39 STR OP
    putbits36_1 (& w1,       39-36,   cpu.ou.STR_OP);

    // 40 -DA-AV XXX

    // 41-50 stuvwyyzAB -A-REG -Q-REG -X0-REG .. -X7-REG
    PNL (putbits36_10 (& w1, 41-36,
         (word10) ~opcodes10 [cpu.ou.RS].reg_use);)

    // 51-53 0

    // 54-71 ICT TRACKER
    putbits36_18 (& w1,      54 - 36, cpu.PPR.IC);

    add_history (L68_OU_HIST_REG, w0, w1);
  }

// According to ISOLTS
//  0:2 OPCODE RP
//  3 9 BIT CHAR
//  4:6 TAG 3/4/5
//  7 CR FLAG
//  8 DIR FLAG
//  9 RP15
// 10 RP16
// 11 SPARE
// 12:14 OPCODE RS
// 15 RB1 FULL
// 16 RP FULL
// 17 RS FULL
// 18 GIN
// 19 GOS
// 20 GD1
// 21 GD2
// 22 GOE
// 23 GOA
// 24 GOM
// 25 GON
// 26 GOF
// 27 STORE OP
// 28 DA NOT
// 29:38 COMPLEMENTED REGISTER IN USE FLAG A/Q/0/1/2/3/4/5/6/7
// 39 ?
// 40 ?
// 41 ?
// 42:47 ICT TRACT

// XXX add_APU_history

//  0:5 SEGMENT NUMBER
//  6 SNR/ESN
//  7 TSR/ESN
//  8 FSDPTW
//  9 FPTW2
// 10 MPTW
// 11 FANP
// 12 FAP
// 13 AMSDW
// 14:15 AMSDW #
// 16 AMPTW
// 17:18 AMPW #
// 19 ACV/DF
// 20:27 ABSOLUTE MEMORY ADDRESS
// 28 TRR #
// 29 FLT HLD

void add_l68_APU_history (enum APUH_e op)
  {
    CPT (cpt1L, 28); // add apu hist
    word36 w0 = 0, w1 = 0;

    w0 = op; // set 17-24 FDSPTW/.../FAP bits

    // 0-14 ESN
    putbits36_15 (& w0,      0,  cpu.TPR.TSR);
    // 15-16 BSY
    PNL (putbits36_1 (& w0,  15, (cpu.apu.state & apu_ESN_SNR) ? 1 : 0);)
    PNL (putbits36_1 (& w0,  16, (cpu.apu.state & apu_ESN_TSR) ? 1 : 0);)
    // 25 SDWAMM
    putbits36_1 (& w0,       25, cpu.cu.SDWAMM);
    // 26-29 SDWAMR
    putbits36_4 (& w0,       26, (word4) cpu.SDWAMR);
    // 30 PTWAMM
    putbits36_1 (& w0,       30, cpu.cu.PTWAMM);
    // 31-34 PTWAMR
    putbits36_4 (& w0,       31, (word4) cpu.PTWAMR);
    // 35 FLT
    PNL (putbits36_1 (& w0,  35, (cpu.apu.state & apu_FLT) ? 1 : 0);)

    // 36-59 ADD
    PNL (putbits36_24 (& w1, 0,  cpu.APUMemAddr);)
    // 60-62 TRR
    putbits36_3 (& w1,       24, cpu.TPR.TRR);
    // 66 XXX Multiple match error in SDWAM
    // 70 Segment is encachable
    putbits36_1 (& w1,       34, cpu.SDW0.C);
    // 71 XXX Multiple match error in PTWAM

    add_history (L68_APU_HIST_REG, w0, w1);
  }

#if defined(THREADZ) || defined(LOCKLESS)
//static pthread_mutex_t debug_lock = PTHREAD_MUTEX_INITIALIZER;

static const char * get_dbg_verb (uint32 dbits, DEVICE * dptr)
  {
    static const char * debtab_none    = "DEBTAB_ISNULL";
    static const char * debtab_nomatch = "DEBTAB_NOMATCH";
    const char * some_match            = NULL;
    int32 offset                       = 0;

    if (dptr->debflags == 0)
      return debtab_none;

    dbits &= dptr->dctrl;     /* Look for just the bits that matched */

    /* Find matching words for bitmask */
    while ((offset < 32) && dptr->debflags[offset].name)
      {
        if (dptr->debflags[offset].mask == dbits)   /* All Bits Match */
          return dptr->debflags[offset].name;
        if (dptr->debflags[offset].mask & dbits)
          some_match = dptr->debflags[offset].name;
        offset ++;
      }
    return some_match ? some_match : debtab_nomatch;
  }

void dps8_sim_debug (uint32 dbits, DEVICE * dptr, unsigned long long cnt, const char* fmt, ...)
  {
    //pthread_mutex_lock (& debug_lock);
    if (sim_deb && dptr && (dptr->dctrl & dbits))
      {
        const char * debug_type = get_dbg_verb (dbits, dptr);
        char stackbuf[STACKBUFSIZE];
        int32 bufsize           = sizeof (stackbuf);
        char * buf              = stackbuf;
        va_list arglist;
        int32 i, j, len;
        struct timespec t;
# ifdef MACOSXPPC
        clock_serv_t cclock;
        mach_timespec_t mts;
        host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
        clock_get_time(cclock, &mts);
        mach_port_deallocate(mach_task_self(), cclock);
        t.tv_sec = mts.tv_sec;
        t.tv_nsec = mts.tv_nsec;
# else
        clock_gettime(CLOCK_REALTIME, &t);
# endif /* ifdef MACOSXPPC */

        buf [bufsize-1] = '\0';

        while (1)
          {                 /* format passed string, args */
            va_start (arglist, fmt);
# if defined(NO_vsnprintf)
            len = vsprintf  (buf, fmt, arglist);
# else                                                   /* !defined(NO_vsnprintf) */
            len = vsnprintf (buf, (int)((unsigned long)(bufsize)-1), fmt, arglist);
# endif                                                  /* NO_vsnprintf */
            va_end (arglist);

/* If the formatted result didn't fit into the buffer, then grow the buffer and try again */

            if ((len < 0) || (len >= bufsize-1))
              {
                if (buf != stackbuf)
                  FREE (buf);
                bufsize = bufsize * 2;
                if (bufsize < len + 2)
                  bufsize = len + 2;
                buf = (char *) malloc ((unsigned long) bufsize);
                if (buf == NULL)                            /* out of memory */
                  return;
                buf[bufsize-1] = '\0';
                continue;
              }
            break;
          }

/* Output the formatted data expanding newlines where they exist */

        for (i = j = 0; i < len; ++i)
          {
            if ('\n' == buf[i])
              {
                if (i >= j)
                  {
                    if ((i != j) || (i == 0))
                      {
                          (void)fprintf (sim_deb, "%lld.%06ld: DBG(%lld) %o: %s %s %.*s\r\n",
                                         (long long)t.tv_sec, t.tv_nsec/1000, cnt,
                                         current_running_cpu_idx, dptr->name, debug_type, i-j, &buf[j]);
                      }
                  }
                j = i + 1;
              }
          }

        /* Set unterminated flag for next time */
        if (buf != stackbuf)
          FREE (buf);
      }
    //pthread_mutex_unlock (& debug_lock);
  }
#endif

void setupPROM (uint cpuNo, unsigned char * PROM) {

// 58009997-040 MULTICS Differences Manual DPS 8-70M Aug83
//
// THESE OFFSETS ARE IN OCTAL
//
//  0-13 CPU Model Number
// 13-25 CPU Serial Number
// 26-33 Date-Ship code (YYMMDD)
// 34-40 CPU ID Field (reference RSW 2)
//  Byte 40: Bits 03 (Bits 32-35 of RSW 2 Field
//           Bit 4=1 Hex Option included
//           Bit 5=1 RSCR (Clock) is Slave Mode included
//           Bits 6-7 Reserved for later use.
//       50: Operating System Use
// 51-1777(8) To be defined.
// NOTE: There is the possibility of disagreement between the
//       ID bits of RSW 2 and the ID bits of PROM locations
//       35-40. This condition could result when alterable
//       configuration condition is contained in the PROM.
//       The user is advised to ignore the PROM fields which
//       contain the processor fault vector base (GCOS III)
//       and the processor number and rely on the RSW 2 bits
//       for this purpose. Bits 14-16 of the RSW 2 should be
//       ignored and the bits representing this information in
//       the PROM should be treated as valid.

// "0-13" disagrees with Multics source (start_pl1); it interprets
// it as "0-12"; most likely a typo in 58009997-040.

// CAC notes: I interpret the fields as
//  0-12 CPU Model Number                                          //  0-10  11 chars
// 13-25 CPU Serial Number // 13 chars                             // 11-21  11 chars
// 26-33 Date-Ship code (YYMMDD) // 8 chars (enough for YYYYMMDD). // 22-27   6 chars
// 34-40 CPU ID Field (reference RSW 2)                            // 28-32   5 chars
//  Byte 40: Bits 03 (Bits 32-35 of RSW 2 Field                    //    32
//           Bit 4=1 Hex Option included
//           Bit 5=1 RSCR (Clock) is Slave Mode included
//           Bits 6-7 Reserved for later use.
//       50: Operating System Use                                  //    40

  word36 rsw2 = 0;

  // The PROM copy of RSW 2 contains a canonical RSW 2 rather than the actual RSW 2.
  //   The port interlace is set to 0
  //   The fault base is set to 2 (Multics)
  //   Processor mode is set to 0 (Multics)

  //  0 -   3   4   Port interlace = 0000
  putbits36_4 (& rsw2,  0,   0);
  //  4 -   5   2   CPU type  01 = DPS8
  putbits36_2 (& rsw2,  4,  001);
  //  6 - 12    7   Fault Base  = 2
  putbits36_7 (& rsw2,  6,   2);
  // 13 - 13    1   PROM Present = 1
  putbits36_1 (& rsw2,  13,  1);
  // 14 - 18    5   Pad 00000
  putbits36_5 (& rsw2,  14,  0);
  // 19 - 19    1   CPU  1 = DPS8
  putbits36_1 (& rsw2,  19,  1);
  // 20 - 20    1   8K Cache  1 = Present
  putbits36_1 (& rsw2,  20,  cpus[cpuNo].options.cache_installed ? 1 : 0);
  // 21 - 22    2   Pad
  putbits36_2 (& rsw2,  21,  0);
  // 23 - 23    1   Always 1 for Multics CPU
  putbits36_1 (& rsw2,  23,  1);
  // 24 - 24    1   Proc Mode Bit
  putbits36_1 (& rsw2,  24,  0);
  // 25 - 28    4   Pad
  putbits36_4 (& rsw2,  25,  0);
  // 29 - 32    4   CPU speed options
  putbits36_4 (& rsw2,  29,  cpus[cpuNo].options.proc_speed & 017LL);
  // 33 - 35    3   CPU number
  putbits36_3 (& rsw2,  33,  cpus[cpuNo].switches.cpu_num & 07LL);

  word4 rsw2Ext = 0;
  if (cpus[cpuNo].options.hex_mode_installed)
    rsw2Ext |= 010;  // bit 4
  if (cpus[cpuNo].options.clock_slave_installed)
    rsw2Ext |= 004;  // bit 5
  // bits 6,7 reserved for future use

  char serial[12];
  (void)sprintf (serial, "%-11u", cpus[cpuNo].switches.serno);

#ifdef VER_H_PROM_SHIP
  char * ship = VER_H_PROM_SHIP;
#else
  char * ship = "200101";
#endif /* VER_H_PROM_SHIP */

#ifndef VER_H_PROM_MAJOR_VER
# define VER_H_PROM_MAJOR_VER "999"
#endif /* VER_H_PROM_MAJOR_VER */

#ifndef VER_H_PROM_MINOR_VER
# define VER_H_PROM_MINOR_VER "999"
#endif /* VER_H_PROM_MINOR_VER */

#ifndef VER_H_PROM_PATCH_VER
# define VER_H_PROM_PATCH_VER "999"
#endif /* VER_H_PROM_PATCH_VER */

#ifndef VER_H_PROM_OTHER_VER
# define VER_H_PROM_OTHER_VER "999"
#endif /* VER_H_PROM_OTHER_VER */

#ifndef VER_H_GIT_RELT
# define VER_H_GIT_RELT "X"
#endif /* VER_H_GIT_RELT */

#ifndef VER_H_PROM_VER_TEXT
# define VER_H_PROM_VER_TEXT "Unknown                      "
#endif /* VER_H_PROM_VER_TEXT */

#ifdef BUILD_PROM_OSA_TEXT
# define BURN_PROM_OSA_TEXT BUILD_PROM_OSA_TEXT
#else
# ifndef VER_H_PROM_OSA_TEXT
#  define BURN_PROM_OSA_TEXT "Unknown Build Op Sys"
# else
#  define BURN_PROM_OSA_TEXT VER_H_PROM_OSA_TEXT
# endif /* VER_H_PROM_OSA_TEXT */
#endif /* BUILD_PROM_OSA_TEXT */

#ifdef BUILD_PROM_OSV_TEXT
# define BURN_PROM_OSV_TEXT BUILD_PROM_OSV_TEXT
#else
# ifndef VER_H_PROM_OSV_TEXT
#  define BURN_PROM_OSV_TEXT "Unknown Build Arch. "
# else
#  define BURN_PROM_OSV_TEXT VER_H_PROM_OSV_TEXT
# endif /* VER_H_PROM_OSV_TEXT */
#endif /* BUILD_PROM_OSV_TEXT */

#ifdef BUILD_PROM_TSA_TEXT
# define BURN_PROM_TSA_TEXT BUILD_PROM_TSA_TEXT
#else
# if defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__) || defined(__x86_64__) || defined(__AMD64)
#  define VER_H_PROM_TSA_TEXT "Intel x86_64 (AMD64)"
# elif defined(_M_IX86) || defined(__i386) || defined(__i486) || defined(__i586) || defined(__i686) || defined(__ix86)
#  define VER_H_PROM_TSA_TEXT "Intel ix86 (32-bit) "
# elif defined(_M_ARM64) || defined(__aarch64__) || defined(__arm64__)
#  define VER_H_PROM_TSA_TEXT "AArch64/ARM64/64-bit"
# elif defined(_M_ARM) || defined(__arm__)
#  define VER_H_PROM_TSA_TEXT "AArch32/ARM32/32-bit"
# elif defined(__ia64__) || defined(_M_IA64) || defined(__itanium__)
#  define VER_H_PROM_TSA_TEXT "Intel Itanium (IA64)"
# elif defined(__ppc64__) || defined(__PPC64__) || defined(__ppc64le__) || defined(__PPC64LE__) || defined(__powerpc64__) || defined(__POWERPC64__) || defined(_M_PPC64) || defined(__PPC64) || defined(_ARCH_PPC64)
#  define VER_H_PROM_TSA_TEXT "Power ISA (64-bit)  "
# elif defined(__ppc__) || defined(__PPC__) || defined(__powerpc__) || defined(__POWERPC__) || defined(_M_PPC) || defined(__PPC) || defined(__ppc32__) || defined(__PPC32__) || defined(__powerpc32__) || defined(__POWERPC32__) || defined(_M_PPC32) || defined(__PPC32)
#  define VER_H_PROM_TSA_TEXT "PowerPC ISA (32-bit)"
# elif defined(__s390x__)
#  define VER_H_PROM_TSA_TEXT "IBM z/Architecture  "
# elif defined(__s390__)
#  define VER_H_PROM_TSA_TEXT "IBM ESA System/390  "
# elif defined(__J2__) || defined(__J2P__) || defined(__j2__) || defined(__j2p__)
#  define VER_H_PROM_TSA_TEXT "J-Core J2 Open CPU  "
# elif defined(__SH4__) || defined(__sh4__) || defined(__SH4) || defined(__sh4)
#  define VER_H_PROM_TSA_TEXT "Hitachi/Renesas SH-4"
# elif defined(__SH2__) || defined(__sh2__) || defined(__SH2) || defined(__sh2)
#  define VER_H_PROM_TSA_TEXT "Hitachi/Renesas SH-2"
# elif defined(__alpha__)
#  define VER_H_PROM_TSA_TEXT "Alpha AXP           "
# elif defined(__hppa__) || defined(__HPPA__) || defined(__PARISC__) || defined(__parisc__)
#  define VER_H_PROM_TSA_TEXT "HP PA-RISC          "
# elif defined(__ICE9__) || defined(__ice9__) || defined(__ICE9) || defined(__ice9)
#  define VER_H_PROM_TSA_TEXT "SiCortex ICE-9      "
# elif defined(mips64) || defined(__mips64__) || defined(MIPS64) || defined(_MIPS64_) || defined(__mips64)
#  define VER_H_PROM_TSA_TEXT "MIPS64              "
# elif defined(mips) || defined(__mips__) || defined(MIPS) || defined(_MIPS_) || defined(__mips)
#  define VER_H_PROM_TSA_TEXT "MIPS                "
# elif defined(__OpenRISC__) || defined(__OPENRISC__) || defined(__openrisc__) || defined(__OR1K__) || defined(__JOR1K__) || defined(__OPENRISC1K__) || defined(__OPENRISC1200__)
#  define VER_H_PROM_TSA_TEXT "OpenRISC            "
# elif defined(__sparc64) || defined(__SPARC64) || defined(__SPARC64__) || defined(__sparc64__)
#  define VER_H_PROM_TSA_TEXT "SPARC64             "
# elif defined(__sparc) || defined(__SPARC) || defined(__SPARC__) || defined(__sparc__)
#  define VER_H_PROM_TSA_TEXT "SPARC               "
# elif defined(__riscv) || defined(__riscv__)
#  define VER_H_PROM_TSA_TEXT "RISC-V              "
# elif defined(__myriad2__)
#  define VER_H_PROM_TSA_TEXT "Myriad2             "
# elif defined(__loongarch64) || defined(__loongarch__)
#  define VER_H_PROM_TSA_TEXT "LoongArch           "
# elif defined(_m68851) || defined(__m68k__) || defined(__m68000__) || defined(__M68K)
#  define VER_H_PROM_TSA_TEXT "Motorola m68k       "
# elif defined(__m88k__) || defined(__m88000__) || defined(__M88K)
#  define VER_H_PROM_TSA_TEXT "Motorola m88k       "
# elif defined(__VAX__) || defined(__vax__)
#  define VER_H_PROM_TSA_TEXT "VAX                 "
# elif defined(__NIOS2__) || defined(__nios2__)
#  define VER_H_PROM_TSA_TEXT "Altera Nios II      "
# elif defined(__MICROBLAZE__) || defined(__microblaze__)
#  define VER_H_PROM_TSA_TEXT "Xilinx MicroBlaze   "
# endif
# ifndef VER_H_PROM_TSA_TEXT
#  define BURN_PROM_TSA_TEXT "Unknown Target Arch."
# else
#  define BURN_PROM_TSA_TEXT VER_H_PROM_TSA_TEXT
# endif /* VER_H_PROM_TSA_TEXT */
#endif /* BUILD_PROM_TSA_TEXT */

#ifdef BUILD_PROM_TSV_TEXT
# define BURN_PROM_TSV_TEXT BUILD_PROM_TSV_TEXT
#else
# if (defined(__WIN__) || defined(_WIN32) || defined(IS_WINDOWS) || defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(CROSS_MINGW32) || defined(CROSS_MINGW64)) && !defined(__CYGWIN__)
#  define VER_H_PROM_TSV_TEXT "Microsoft Windows   "
# elif defined(__CYGWIN__)
#  define VER_H_PROM_TSV_TEXT "Windows/Cygwin      "
# elif (defined(__sunos) || defined( __sun ) || defined(__sun__)) && (defined(SYSV) || defined( __SVR4 ) || defined(__SVR4__) || defined(__svr4__))
#  if defined(__illumos__)
#   define VER_H_PROM_TSV_TEXT "illumos             "
#  else
#   define VER_H_PROM_TSV_TEXT "Solaris             "
#  endif
# elif defined(__APPLE__) && defined(__MACH__)
#  define VER_H_PROM_TSV_TEXT "Apple macOS         "
# elif defined(__GNU__) && !defined(__linux__)
#  define VER_H_PROM_TSV_TEXT "GNU/Hurd            "
# elif defined(__ANDROID__) && defined(__ANDROID_API__)
#  if defined(__linux__)
#   define VER_H_PROM_TSV_TEXT "Android/Linux       "
#  else
#   define VER_H_PROM_TSV_TEXT "Android             "
#  endif
# elif defined(__lynxOS__) || defined(__LYNXOS__) || defined(LynxOS) || defined(LYNXOS)
#  define VER_H_PROM_TSV_TEXT "LynxOS              "
# elif defined(__HELENOS__)
#  define VER_H_PROM_TSV_TEXT "HelenOS             "
# elif defined(__linux__)
#  if defined(__BIONIC__)
#   define VER_H_PROM_TSV_TEXT "Linux/Bionic-libc   "
#  elif defined(__UCLIBC__) || defined(UCLIBC)
#   define VER_H_PROM_TSV_TEXT "Linux/uClibc        "
#  elif defined(__NEWLIB__)
#   define VER_H_PROM_TSV_TEXT "Linux/Newlib        "
#  elif defined(__dietlibc__)
#   define VER_H_PROM_TSV_TEXT "Linux/Diet-libc     "
#  elif defined(__GLIBC__)
#   define VER_H_PROM_TSV_TEXT "GNU/Linux           "
#  else
#   define VER_H_PROM_TSV_TEXT "Linux               "
#  endif
# elif defined(__HAIKU__)
#  define VER_H_PROM_TSV_TEXT "Haiku               "
# elif defined(__serenity__)
#  define VER_H_PROM_TSV_TEXT "SerenityOS          "
# elif defined(__FreeBSD__)
#  define VER_H_PROM_TSV_TEXT "FreeBSD             "
# elif defined(__NetBSD__)
#  define VER_H_PROM_TSV_TEXT "NetBSD              "
# elif defined(__OpenBSD__)
#  define VER_H_PROM_TSV_TEXT "OpenBSD             "
# elif defined(__DragonFly__)
#  define VER_H_PROM_TSV_TEXT "DragonFly BSD       "
# elif defined(_AIX)
#  if !defined(__PASE__)
#   define VER_H_PROM_TSV_TEXT "IBM AIX             "
#  else
#   define VER_H_PROM_TSV_TEXT "IBM OS/400 (PASE)   "
#  endif
# elif defined(__VXWORKS__) || defined(__VXWORKS) || defined(__vxworks) || defined(__vxworks__) || defined(_VxWorks)
#  if !defined(__RTP__)
#   define VER_H_PROM_TSV_TEXT "VxWorks             "
#  else
#   define VER_H_PROM_TSV_TEXT "VxWorks RTP         "
#  endif
# elif defined(__rtems__)
#  if defined(__FreeBSD_version)
#   define VER_H_PROM_TSV_TEXT "RTEMS/LibBSD        "
#  else
#   define VER_H_PROM_TSV_TEXT "RTEMS               "
#  endif
# elif defined(__ZEPHYR__)
#  define VER_H_PROM_TSV_TEXT "Zephyr              "
# elif defined(ti_sysbios_BIOS___VERS) || defined(ti_sysbios_BIOS__top__)
#  define VER_H_PROM_TSV_TEXT "TI-RTOS (SYS/BIOS)  "
# elif defined(__OSV__) // -V1040
#  define VER_H_PROM_TSV_TEXT "OSv                 "
# elif defined(MINIX) || defined(MINIX3) || defined(MINIX315) || defined(__minix__) || defined(__minix3__) || defined(__minix315__)
#  define VER_H_PROM_TSV_TEXT "Minix               "
# elif defined(__QNX__)
#  if defined(__QNXNTO__)
#   define VER_H_PROM_TSV_TEXT "QNX Neutrino        "
#  else
#   define VER_H_PROM_TSV_TEXT "QNX                 "
#  endif
# endif
# ifndef VER_H_PROM_TSV_TEXT
#  define BURN_PROM_TSV_TEXT "Unknown Target OpSys"
# else
#  define BURN_PROM_TSV_TEXT VER_H_PROM_TSV_TEXT
# endif /* VER_H_PROM_TSV_TEXT */
#endif /* BUILD_PROM_TSV_TEXT */

#ifndef VER_H_GIT_DATE_SHORT
# define VER_H_GIT_DATE_SHORT "2021-01-01"
#endif /* ifndef VER_H_GIT_DATE_SHORT */

#ifndef BURN_PROM_BUILD_NUM
# define BURN_PROM_BUILD_NUM "        "
#endif /* ifndef BURN_PROM_BUILD_NUM */

#define BURN(offset, length, string) memcpy ((char *) PROM + (offset), string, length)
#define BURN1(offset, byte) PROM[offset] = (char) (byte)

  (void)memset (PROM, 255, 1024);

  //   Offset Length  Data
  BURN  ( 00,  11,  "DPS 8/SIM M");                //    0-10  CPU model ("XXXXXXXXXXX")       //-V1086
  BURN  (013,  11,  serial);                       //   11-21  CPU serial ("DDDDDDDDDDD")      //-V1086
  BURN  (026,   6,  ship);                         //   22-27  CPU ship date ("YYMMDD")        //-V1086
  BURN1 (034,       getbits36_8 (rsw2,  0));       //   34     RSW 2 bits  0- 7                //-V1086
  BURN1 (035,       getbits36_8 (rsw2,  8));       //   35     RSW 2 bits  8-15                //-V1086
  BURN1 (036,       getbits36_8 (rsw2, 16));       //   36     RSW 2 bits 16-23                //-V1086
  BURN1 (037,       getbits36_8 (rsw2, 24));       //   37     RSW 2 bits 24-31                //-V1086
  BURN1 (040,     ((getbits36_4 (rsw2, 32) << 4) \
                               | rsw2Ext));        //   40     RSW 2 bits 32-35, options bits  //-V1086

  /* Begin extended PROM data */
  BURN  ( 60,   1,  "2");                          //   60     PROM Layout Version Number      //-V1086
  BURN  ( 70,  10,  VER_H_GIT_DATE_SHORT);         //   70     Release Git Commit Date         //-V1086
  BURN  ( 80,   3,  VER_H_PROM_MAJOR_VER);         //   80     Major Release Number            //-V1086
  BURN  ( 83,   3,  VER_H_PROM_MINOR_VER);         //   83     Minor Release Number            //-V1086
  BURN  ( 86,   3,  VER_H_PROM_PATCH_VER);         //   86     Patch Release Number            //-V1086
  BURN  ( 89,   3,  VER_H_PROM_OTHER_VER);         //   89     Iteration Release Number        //-V1086
  BURN  ( 92,   8,  BURN_PROM_BUILD_NUM);          //   92     Reserved for Build Number       //-V1086
  BURN  (100,   1,  VER_H_GIT_RELT);               //  100     Release Type                    //-V1086
  BURN  (101,  29,  VER_H_PROM_VER_TEXT);          //  101     Release Text                    //-V1086
  BURN  (130,  20,  BURN_PROM_OSA_TEXT);           //  130     Build System Architecture       //-V1086
  BURN  (150,  20,  BURN_PROM_OSV_TEXT);           //  150     Build System Operating System   //-V1086
  BURN  (170,  20,  BURN_PROM_TSA_TEXT);           //  170     Target System Architecture      //-V1086
  BURN  (190,  20,  BURN_PROM_TSV_TEXT);           //  190     Target System Architecture      //-V1086
}

void cpuStats (uint cpuNo) {
  if (! cpus[cpuNo].cycleCnt)
    return;

  (void)fflush(stderr);
  (void)fflush(stdout);
  sim_msg ("\r\n");
  (void)fflush(stdout);
  (void)fflush(stderr);
  sim_msg ("\r+---------------------------------+\r\n");
  sim_msg ("\r|         CPU %c Statistics        |\r\n", 'A' + cpuNo);
  sim_msg ("\r+---------------------------------+\r\n");
  (void)fflush(stdout);
  (void)fflush(stderr);
#ifdef WIN_STDIO
  sim_msg ("\r|  cycles        %15llu  |\r\n", (unsigned long long)cpus[cpuNo].cycleCnt);
  sim_msg ("\r|  instructions  %15llu  |\r\n", (unsigned long long)cpus[cpuNo].instrCnt);
  (void)fflush(stdout);
  (void)fflush(stderr);
  sim_msg ("\r+---------------------------------+\r\n");
  sim_msg ("\r|  lockCnt       %15llu  |\r\n", (unsigned long long)cpus[cpuNo].lockCnt);
  sim_msg ("\r|  lockImmediate %15llu  |\r\n", (unsigned long long)cpus[cpuNo].lockImmediate);
  (void)fflush(stdout);
  (void)fflush(stderr);
  sim_msg ("\r+---------------------------------+\r\n");
  sim_msg ("\r|  lockWait      %15llu  |\r\n", (unsigned long long)cpus[cpuNo].lockWait);
  sim_msg ("\r|  lockWaitMax   %15llu  |\r\n", (unsigned long long)cpus[cpuNo].lockWaitMax);
  (void)fflush(stdout);
  (void)fflush(stderr);
# ifndef SCHED_NEVER_YIELD
  sim_msg ("\r|  lockYield     %15llu  |\r\n", (unsigned long long)cpus[cpuNo].lockYield);
  (void)fflush(stdout);
  (void)fflush(stderr);
# else
  sim_msg ("\r|  lockYield                ----  |\r\n");
  (void)fflush(stdout);
  (void)fflush(stderr);
# endif /* ifndef SCHED_NEVER_YIELD */
  sim_msg ("\r+---------------------------------+");
  (void)fflush(stdout);
  (void)fflush(stderr);
# ifndef UCACHE
#  ifndef UCACHE_STATS
  sim_msg ("\r\n");
#  endif
# endif
  (void)fflush(stdout);
  (void)fflush(stderr);
#else
  sim_msg ("\r|  cycles        %'15llu  |\r\n", (unsigned long long)cpus[cpuNo].cycleCnt);
  sim_msg ("\r|  instructions  %'15llu  |\r\n", (unsigned long long)cpus[cpuNo].instrCnt);
  (void)fflush(stdout);
  (void)fflush(stderr);
  sim_msg ("\r+---------------------------------+\r\n");
  sim_msg ("\r|  lockCnt       %'15llu  |\r\n", (unsigned long long)cpus[cpuNo].lockCnt);
  sim_msg ("\r|  lockImmediate %'15llu  |\r\n", (unsigned long long)cpus[cpuNo].lockImmediate);
  (void)fflush(stdout);
  (void)fflush(stderr);
  sim_msg ("\r+---------------------------------+\r\n");
  sim_msg ("\r|  lockWait      %'15llu  |\r\n", (unsigned long long)cpus[cpuNo].lockWait);
  sim_msg ("\r|  lockWaitMax   %'15llu  |\r\n", (unsigned long long)cpus[cpuNo].lockWaitMax);
  (void)fflush(stdout);
  (void)fflush(stderr);
# ifndef SCHED_NEVER_YIELD
  sim_msg ("\r|  lockYield     %'15llu  |\r\n", (unsigned long long)cpus[cpuNo].lockYield);
  (void)fflush(stdout);
  (void)fflush(stderr);
# else
  sim_msg ("\r|  lockYield                ----  |\r\n");
  (void)fflush(stdout);
  (void)fflush(stderr);
# endif /* ifndef SCHED_NEVER_YIELD */
  sim_msg ("\r+---------------------------------+");
  (void)fflush(stdout);
  (void)fflush(stderr);
# ifndef UCACHE
#  ifndef UCACHE_STATS
  sim_msg ("\r\n");
#  endif
# endif
  (void)fflush(stderr);
  (void)fflush(stdout);
#endif

#ifdef UCACHE_STATS
  ucacheStats (cpuNo);
#endif

#if 0
  for (int i = 0; i < N_FAULTS; i ++) {
    if (cpus[cpuNo].faultCnt [i])
      sim_msg  ("%s faults = %llu\n", faultNames [i], (unsigned long long)cpus[cpuNo].faultCnt [i]);
  }
#endif
}
