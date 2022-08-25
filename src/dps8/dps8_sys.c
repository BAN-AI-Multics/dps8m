/*
 * vim: filetype=c:tabstop=4:tw=100:expandtab
 * vim: ruler:hlsearch:incsearch:autoindent:wildmenu:wrapscan
 * SPDX-License-Identifier: ICU
 * SPDX-License-Identifier: Multics
 * scspell-id: ff1a12fc-f62e-11ec-aea6-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2021 Charles Anthony
 * Copyright (c) 2016 Michal Tomek
 * Copyright (c) 2021 Jeffrey H. Johnson <trnsz@pobox.com>
 * Copyright (c) 2021-2022 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
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
#ifndef __MINGW64__
# ifndef __MINGW32__
#  ifndef CROSS_MINGW64
#   ifndef CROSS_MINGW32
#    ifndef __OpenBSD__
#     ifndef __HAIKU__
#      include <wordexp.h>
#     endif /* ifndef __HAIKU__ */
#    endif /* ifndef __OpenBSD__ */
#    include <signal.h>
#   endif /* ifndef CROSS_MINGW32 */
#  endif /* ifndef CROSS_MINGW64 */
# endif /* ifndef __MINGW32__ */
#endif /* ifndef __MINGW64__ */
#include <unistd.h>
#include <ctype.h>

#ifdef __APPLE__
# include <pthread.h>
#endif

#include "dps8.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_iom.h"
#include "dps8_console.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_state.h"
#include "dps8_ins.h"
#include "dps8_math.h"
#include "dps8_mt.h"
#include "dps8_socket_dev.h"
#include "dps8_disk.h"
#include "dps8_append.h"
#include "dps8_fnp2.h"
#include "dps8_crdrdr.h"
#include "dps8_crdpun.h"
#include "dps8_prt.h"
#include "dps8_urp.h"
#include "dps8_absi.h"
#include "dps8_utils.h"
#include "shm.h"
#include "utlist.h"
#include "ver.h"
#if defined(THREADZ) || defined(LOCKLESS)
# include "threadz.h"
#endif

#ifdef PANEL68
# include "panelScraper.h"
#endif

#include "segldr.h"

#define DBG_CTR cpu.cycleCnt

#define ASSUME0 0

// Strictly speaking, memory belongs in the SCU.
// We will treat memory as viewed from the CPU and elide the
// SCU configuration that maps memory across multiple SCUs.
// I would guess that multiple SCUs helped relieve memory
// contention across multiple CPUs, but that is a level of
// emulation that will be ignored.

struct system_state_s * system_state;

vol word36 * M = NULL;  // memory

//
// These are part of the scp interface
//

char sim_name[] = "DPS8/M";
int32 sim_emax = 4;  // some EIS can take up to 4-words
static void dps8_init(void);
static void dps8_exit (void);
void (*sim_vm_init) (void) = & dps8_init;  // CustomCmds;
void (*sim_vm_exit) (void) = & dps8_exit;  // CustomCmds;

#ifdef TESTING
static t_addr parse_addr(DEVICE *dptr, const char *cptr, const char **optr);
static void fprint_addr(FILE *stream, DEVICE *dptr, t_addr addr);
#endif // TESTING

int32 luf_flag = 1;

////////////////////////////////////////////////////////////////////////////////
//
// SCP Commands
//

//
// System configuration commands
//

// Script to string cables and set switches

#ifndef PERF_STRIP
static char * default_base_system_script [] =
  {
    // ;
    // ; Configure test system
    // ;
    // ; CPU, IOM * 2, MPC, TAPE * 16, DISK * 16, SCU * 4, OPC * 2, FNP, URP * 3,
    // ; PRT, RDR, PUN
    // ;
    // ;
    // ; From AN70-1 System Initialization PLM May 84, pg 8-4:
    // ;
    // ; All CPUs and IOMs must share the same layout of port assignments to
    // ; SCUs. Thus, if memory port B of CPU C goes to SCU D, the memory port
    // ; B of all other CPUs and IOMs must go to SCU D. All CPUs and IOMs must
    // ; describe this SCU the same; all must agree in memory sizes. Also, all
    // ; SCUs must agree on port assignments of CPUs and IOMs. This, if port 3
    // ; of SCU C goes to CPU A, the port 3 of all other SCUs must also go to
    // ; CPU A.
    // ;
    // ; Pg. 8-6:
    // ;
    // ; The actual memory size of the memory attached to the SCU attached to
    // ; the processor port in questions is 32K * 2 ** (encoded memory size).
    // ; The port assignment couples with the memory size to determine the base
    // ; address of the SCU connected to the specified CPU port (absolute
    // ; address of the first location in the memory attached to that SCU). The
    // ; base address of the SCU is the (actual memory size) * (port assignment).
    // ;
    // ; Pg. 8-6
    // ;
    // ; [bits 09-11 lower store size]
    // ;
    // ; A DPS-8 SCU may have up to four store units attached to it. If this is
    // ; the case, two stores units form a pair of units. The size of a pair of
    // ; units (or a single unit) is 32K * 2 ** (lower store size) above.
    // ;
    // ;
    // ;
    // ; Looking at bootload_io, it would appear that Multics is happier with
    // ; IOM0 being the bootload IOM, despite suggestions elsewhere that was
    // ; not a requirement.

//
// IOM channel assignments
//
// IOM A
//
//  012 MTP0           tape drives
//  013 IPC0 port 0    FIPS disk controller
//  014 MSP0 port 0    disk controller
//  015 URP0           card reader controller
//  016 URP1           card punch controller
//  017 URP2           printer controller
//  020 FNPD           comm line controller
//  021 FNPA           comm line controller
//  022 FNPB           comm line controller
//  023 FNPC           comm line controller
//  024 FNPE           comm line controller
//  025 FNPF           comm line controller
//  026 FNPG           comm line controller
//  027 FNPH           comm line controller
//  032 ABSI0          IMP controller
//  036 OPC0           operator console
//  040 SKCA
//  041 SKCB
//  042 SKCC
//  043 SKCD
//  044 SKCE
//  045 SKCF
//  046 SKCG
//  047 SKCH
//
// IOM B
//
//  013 IPC0 port 1    FIPS disk controller
//  014 MSP0 port 1    disk controller

    // ; Disconnect everything...
    "cable_ripout",

    "set cpu nunits=6",
    "set iom nunits=2",
    // ; 16 drives plus a placeholder for the controller
    "set tape nunits=17",
    "set mtp nunits=1",
    // ; 4 3381 drives; 2 controllers
    // ; 4 d501 drives; 2 controller
    // ; 4 d451 drives; same controller has d501s
    // ; 2 d500 drives; same controller has d501s
    "set ipc nunits=2",
    "set msp nunits=2",
    "set disk nunits=26",
    "set scu nunits=4",
    "set opc nunits=2",
    "set fnp nunits=8",
    "set urp nunits=10",
    "set rdr nunits=3",
    "set pun nunits=3",
    "set prt nunits=4",
# ifndef __MINGW64__
#  ifndef __MINGW32__
#   ifndef CROSS_MINGW64
#    ifndef CROSS_MINGW32
    "set skc nunits=64",
    "set absi nunits=1",
#    endif /* ifndef CROSS_MINGW32 */
#   endif /* ifndef CROSS_MINGW64 */
#  endif /* ifndef __MINGW32__ */
# endif /* ifndef __MINGW64__ */

// CPU0

    "set cpu0 config=faultbase=Multics",

    "set cpu0 config=num=0",
    // ; As per GB61-01 Operators Guide, App. A
    // ; switches: 4, 6, 18, 19, 20, 23, 24, 25, 26, 28
    "set cpu0 config=data=024000717200",
    "set cpu0 config=address=000000000000",

    // ; enable ports 0 and 1 (scu connections)
    // ; portconfig: ABCD
    // ;   each is 3 bits addr assignment
    // ;           1 bit enabled
    // ;           1 bit sysinit enabled
    // ;           1 bit interlace enabled (interlace?)
    // ;           3 bit memory size
    // ;              0 - 32K
    // ;              1 - 64K
    // ;              2 - 128K
    // ;              3 - 256K
    // ;              4 - 512K
    // ;              5 - 1M
    // ;              6 - 2M
    // ;              7 - 4M

    "set cpu0 config=port=A",
    "set cpu0   config=assignment=0",
    "set cpu0   config=interlace=0",
    "set cpu0   config=enable=1",
    "set cpu0   config=init_enable=1",
    "set cpu0   config=store_size=4M",

    "set cpu0 config=port=B",
    "set cpu0   config=assignment=1",
    "set cpu0   config=interlace=0",
    "set cpu0   config=enable=1",
    "set cpu0   config=init_enable=1",
    "set cpu0   config=store_size=4M",

    "set cpu0 config=port=C",
    "set cpu0   config=assignment=2",
    "set cpu0   config=interlace=0",
    "set cpu0   config=enable=1",
    "set cpu0   config=init_enable=1",
    "set cpu0   config=store_size=4M",

    "set cpu0 config=port=D",
    "set cpu0   config=assignment=3",
    "set cpu0   config=interlace=0",
    "set cpu0   config=enable=1",
    "set cpu0   config=init_enable=1",
    "set cpu0   config=store_size=4M",

    // ; 0 = GCOS 1 = VMS
    "set cpu0 config=mode=Multics",

    "set cpu0 config=enable_cache=enable",
    "set cpu0 config=sdwam=enable",
    "set cpu0 config=ptwam=enable",

    // ; 0 = 8/70
    "set cpu0 config=speed=0",

    "set cpu0 config=dis_enable=enable",
    "set cpu0 config=steady_clock=disable",
    "set cpu0 config=halt_on_unimplemented=disable",
    "set cpu0 config=enable_wam=disable",
    "set cpu0 config=report_faults=disable",
    "set cpu0 config=tro_enable=enable",
    "set cpu0 config=y2k=disable",
    "set cpu0 config=drl_fatal=disable",
    "set cpu0 config=useMap=disable",
    "set cpu0 config=prom_installed=enable",
    "set cpu0 config=hex_mode_installed=disable",
    "set cpu0 config=cache_installed=enable",
    "set cpu0 config=clock_slave_installed=enable",

// CPU1

    "set cpu1 config=faultbase=Multics",

    "set cpu1 config=num=1",
    // ; As per GB61-01 Operators Guide, App. A
    // ; switches: 4, 6, 18, 19, 20, 23, 24, 25, 26, 28
    "set cpu1 config=data=024000717200",
    "set cpu1 config=address=000000000000",

    // ; enable ports 0 and 1 (scu connections)
    // ; portconfig: ABCD
    // ;   each is 3 bits addr assignment
    // ;           1 bit enabled
    // ;           1 bit sysinit enabled
    // ;           1 bit interlace enabled (interlace?)
    // ;           3 bit memory size
    // ;              0 - 32K
    // ;              1 - 64K
    // ;              2 - 128K
    // ;              3 - 256K
    // ;              4 - 512K
    // ;              5 - 1M
    // ;              6 - 2M
    // ;              7 - 4M

    "set cpu1 config=port=A",
    "set cpu1   config=assignment=0",
    "set cpu1   config=interlace=0",
    "set cpu1   config=enable=1",
    "set cpu1   config=init_enable=1",
    "set cpu1   config=store_size=4M",

    "set cpu1 config=port=B",
    "set cpu1   config=assignment=1",
    "set cpu1   config=interlace=0",
    "set cpu1   config=enable=1",
    "set cpu1   config=init_enable=1",
    "set cpu1   config=store_size=4M",

    "set cpu1 config=port=C",
    "set cpu1   config=assignment=2",
    "set cpu1   config=interlace=0",
    "set cpu1   config=enable=1",
    "set cpu1   config=init_enable=1",
    "set cpu1   config=store_size=4M",

    "set cpu1 config=port=D",
    "set cpu1   config=assignment=3",
    "set cpu1   config=interlace=0",
    "set cpu1   config=enable=1",
    "set cpu1   config=init_enable=1",
    "set cpu1   config=store_size=4M",

    // ; 0 = GCOS 1 = VMS
    "set cpu1 config=mode=Multics",

    "set cpu1 config=enable_cache=enable",
    "set cpu1 config=sdwam=enable",
    "set cpu1 config=ptwam=enable",

    // ; 0 = 8/70
    "set cpu1 config=speed=0",

    "set cpu1 config=dis_enable=enable",
    "set cpu1 config=steady_clock=disable",
    "set cpu1 config=halt_on_unimplemented=disable",
    "set cpu1 config=enable_wam=disable",
    "set cpu1 config=report_faults=disable",
    "set cpu1 config=tro_enable=enable",
    "set cpu1 config=y2k=disable",
    "set cpu1 config=drl_fatal=disable",
    "set cpu1 config=useMap=disable",
    "set cpu1 config=prom_installed=enable",
    "set cpu1 config=hex_mode_installed=disable",
    "set cpu1 config=cache_installed=enable",
    "set cpu1 config=clock_slave_installed=enable",

// CPU2

    "set cpu2 config=faultbase=Multics",

    "set cpu2 config=num=2",
    // ; As per GB61-01 Operators Guide, App. A
    // ; switches: 4, 6, 18, 19, 20, 23, 24, 25, 26, 28
    "set cpu2 config=data=024000717200",
    "set cpu2 config=address=000000000000",

    // ; enable ports 0 and 1 (scu connections)
    // ; portconfig: ABCD
    // ;   each is 3 bits addr assignment
    // ;           1 bit enabled
    // ;           1 bit sysinit enabled
    // ;           1 bit interlace enabled (interlace?)
    // ;           3 bit memory size
    // ;              0 - 32K
    // ;              1 - 64K
    // ;              2 - 128K
    // ;              3 - 256K
    // ;              4 - 512K
    // ;              5 - 1M
    // ;              6 - 2M
    // ;              7 - 4M

    "set cpu2 config=port=A",
    "set cpu2   config=assignment=0",
    "set cpu2   config=interlace=0",
    "set cpu2   config=enable=1",
    "set cpu2   config=init_enable=1",
    "set cpu2   config=store_size=4M",

    "set cpu2 config=port=B",
    "set cpu2   config=assignment=1",
    "set cpu2   config=interlace=0",
    "set cpu2   config=enable=1",
    "set cpu2   config=init_enable=1",
    "set cpu2   config=store_size=4M",

    "set cpu2 config=port=C",
    "set cpu2   config=assignment=2",
    "set cpu2   config=interlace=0",
    "set cpu2   config=enable=1",
    "set cpu2   config=init_enable=1",
    "set cpu2   config=store_size=4M",

    "set cpu2 config=port=D",
    "set cpu2   config=assignment=3",
    "set cpu2   config=interlace=0",
    "set cpu2   config=enable=1",
    "set cpu2   config=init_enable=1",
    "set cpu2   config=store_size=4M",

    // ; 0 = GCOS 1 = VMS
    "set cpu2 config=mode=Multics",

    "set cpu2 config=enable_cache=enable",
    "set cpu2 config=sdwam=enable",
    "set cpu2 config=ptwam=enable",

    // ; 0 = 8/70
    "set cpu2 config=speed=0",

    "set cpu2 config=dis_enable=enable",
    "set cpu2 config=steady_clock=disable",
    "set cpu2 config=halt_on_unimplemented=disable",
    "set cpu2 config=enable_wam=disable",
    "set cpu2 config=report_faults=disable",
    "set cpu2 config=tro_enable=enable",
    "set cpu2 config=y2k=disable",
    "set cpu2 config=drl_fatal=disable",
    "set cpu2 config=useMap=disable",
    "set cpu2 config=prom_installed=enable",
    "set cpu2 config=hex_mode_installed=disable",
    "set cpu2 config=cache_installed=enable",
    "set cpu2 config=clock_slave_installed=enable",

// CPU3

    "set cpu3 config=faultbase=Multics",

    "set cpu3 config=num=3",
    // ; As per GB61-01 Operators Guide, App. A
    // ; switches: 4, 6, 18, 19, 20, 23, 24, 25, 26, 28
    "set cpu3 config=data=024000717200",
    "set cpu3 config=address=000000000000",

    // ; enable ports 0 and 1 (scu connections)
    // ; portconfig: ABCD
    // ;   each is 3 bits addr assignment
    // ;           1 bit enabled
    // ;           1 bit sysinit enabled
    // ;           1 bit interlace enabled (interlace?)
    // ;           3 bit memory size
    // ;              0 - 32K
    // ;              1 - 64K
    // ;              2 - 128K
    // ;              3 - 256K
    // ;              4 - 512K
    // ;              5 - 1M
    // ;              6 - 2M
    // ;              7 - 4M

    "set cpu3 config=port=A",
    "set cpu3   config=assignment=0",
    "set cpu3   config=interlace=0",
    "set cpu3   config=enable=1",
    "set cpu3   config=init_enable=1",
    "set cpu3   config=store_size=4M",

    "set cpu3 config=port=B",
    "set cpu3   config=assignment=1",
    "set cpu3   config=interlace=0",
    "set cpu3   config=enable=1",
    "set cpu3   config=init_enable=1",
    "set cpu3   config=store_size=4M",

    "set cpu3 config=port=C",
    "set cpu3   config=assignment=2",
    "set cpu3   config=interlace=0",
    "set cpu3   config=enable=1",
    "set cpu3   config=init_enable=1",
    "set cpu3   config=store_size=4M",

    "set cpu3 config=port=D",
    "set cpu3   config=assignment=3",
    "set cpu3   config=interlace=0",
    "set cpu3   config=enable=1",
    "set cpu3   config=init_enable=1",
    "set cpu3   config=store_size=4M",

    // ; 0 = GCOS 1 = VMS
    "set cpu3 config=mode=Multics",

    "set cpu3 config=enable_cache=enable",
    "set cpu3 config=sdwam=enable",
    "set cpu3 config=ptwam=enable",

    // ; 0 = 8/70
    "set cpu3 config=speed=0",

    "set cpu3 config=dis_enable=enable",
    "set cpu3 config=steady_clock=disable",
    "set cpu3 config=halt_on_unimplemented=disable",
    "set cpu3 config=enable_wam=disable",
    "set cpu3 config=report_faults=disable",
    "set cpu3 config=tro_enable=enable",
    "set cpu3 config=y2k=disable",
    "set cpu3 config=drl_fatal=disable",
    "set cpu3 config=useMap=disable",
    "set cpu3 config=prom_installed=enable",
    "set cpu3 config=hex_mode_installed=disable",
    "set cpu3 config=cache_installed=enable",
    "set cpu3 config=clock_slave_installed=enable",

// CPU4

    "set cpu4 config=faultbase=Multics",

    "set cpu4 config=num=4",
    // ; As per GB61-01 Operators Guide, App. A
    // ; switches: 4, 6, 18, 19, 20, 23, 24, 25, 26, 28
    "set cpu4 config=data=024000717200",
    "set cpu4 config=address=000000000000",

    // ; enable ports 0 and 1 (scu connections)
    // ; portconfig: ABCD
    // ;   each is 3 bits addr assignment
    // ;           1 bit enabled
    // ;           1 bit sysinit enabled
    // ;           1 bit interlace enabled (interlace?)
    // ;           3 bit memory size
    // ;              0 - 32K
    // ;              1 - 64K
    // ;              2 - 128K
    // ;              3 - 256K
    // ;              4 - 512K
    // ;              5 - 1M
    // ;              6 - 2M
    // ;              7 - 4M

    "set cpu4 config=port=A",
    "set cpu4   config=assignment=0",
    "set cpu4   config=interlace=0",
    "set cpu4   config=enable=1",
    "set cpu4   config=init_enable=1",
    "set cpu4   config=store_size=4M",

    "set cpu4 config=port=B",
    "set cpu4   config=assignment=1",
    "set cpu4   config=interlace=0",
    "set cpu4   config=enable=1",
    "set cpu4   config=init_enable=1",
    "set cpu4   config=store_size=4M",

    "set cpu4 config=port=C",
    "set cpu4   config=assignment=2",
    "set cpu4   config=interlace=0",
    "set cpu4   config=enable=1",
    "set cpu4   config=init_enable=1",
    "set cpu4   config=store_size=4M",

    "set cpu4 config=port=D",
    "set cpu4   config=assignment=3",
    "set cpu4   config=interlace=0",
    "set cpu4   config=enable=1",
    "set cpu4   config=init_enable=1",
    "set cpu4   config=store_size=4M",

    // ; 0 = GCOS 1 = VMS
    "set cpu4 config=mode=Multics",

    "set cpu4 config=enable_cache=enable",
    "set cpu4 config=sdwam=enable",
    "set cpu4 config=ptwam=enable",

    // ; 0 = 8/70
    "set cpu4 config=speed=0",

    "set cpu4 config=dis_enable=enable",
    "set cpu4 config=steady_clock=disable",
    "set cpu4 config=halt_on_unimplemented=disable",
    "set cpu4 config=enable_wam=disable",
    "set cpu4 config=report_faults=disable",
    "set cpu4 config=tro_enable=enable",
    "set cpu4 config=y2k=disable",
    "set cpu4 config=drl_fatal=disable",
    "set cpu4 config=useMap=disable",
    "set cpu4 config=prom_installed=enable",
    "set cpu4 config=hex_mode_installed=disable",
    "set cpu4 config=cache_installed=enable",
    "set cpu4 config=clock_slave_installed=enable",

// CPU5

    "set cpu5 config=faultbase=Multics",

    "set cpu5 config=num=5",
    // ; As per GB61-01 Operators Guide, App. A
    // ; switches: 4, 6, 18, 19, 20, 23, 24, 25, 26, 28
    "set cpu5 config=data=024000717200",
    "set cpu5 config=address=000000000000",

    // ; enable ports 0 and 1 (scu connections)
    // ; portconfig: ABCD
    // ;   each is 3 bits addr assignment
    // ;           1 bit enabled
    // ;           1 bit sysinit enabled
    // ;           1 bit interlace enabled (interlace?)
    // ;           3 bit memory size
    // ;              0 - 32K
    // ;              1 - 64K
    // ;              2 - 128K
    // ;              3 - 256K
    // ;              4 - 512K
    // ;              5 - 1M
    // ;              6 - 2M
    // ;              7 - 4M

    "set cpu5 config=port=A",
    "set cpu5   config=assignment=0",
    "set cpu5   config=interlace=0",
    "set cpu5   config=enable=1",
    "set cpu5   config=init_enable=1",
    "set cpu5   config=store_size=4M",

    "set cpu5 config=port=B",
    "set cpu5   config=assignment=1",
    "set cpu5   config=interlace=0",
    "set cpu5   config=enable=1",
    "set cpu5   config=init_enable=1",
    "set cpu5   config=store_size=4M",

    "set cpu5 config=port=C",
    "set cpu5   config=assignment=2",
    "set cpu5   config=interlace=0",
    "set cpu5   config=enable=1",
    "set cpu5   config=init_enable=1",
    "set cpu5   config=store_size=4M",

    "set cpu5 config=port=D",
    "set cpu5   config=assignment=3",
    "set cpu5   config=interlace=0",
    "set cpu5   config=enable=1",
    "set cpu5   config=init_enable=1",
    "set cpu5   config=store_size=4M",

    // ; 0 = GCOS 1 = VMS
    "set cpu5 config=mode=Multics",

    "set cpu5 config=enable_cache=enable",
    "set cpu5 config=sdwam=enable",
    "set cpu5 config=ptwam=enable",

    // ; 0 = 8/70
    "set cpu5 config=speed=0",

    "set cpu5 config=dis_enable=enable",
    "set cpu5 config=steady_clock=disable",
    "set cpu5 config=halt_on_unimplemented=disable",
    "set cpu5 config=enable_wam=disable",
    "set cpu5 config=report_faults=disable",
    "set cpu5 config=tro_enable=enable",
    "set cpu5 config=y2k=disable",
    "set cpu5 config=drl_fatal=disable",
    "set cpu5 config=useMap=disable",
    "set cpu5 config=prom_installed=enable",
    "set cpu5 config=hex_mode_installed=disable",
    "set cpu5 config=cache_installed=enable",
    "set cpu5 config=clock_slave_installed=enable",

# if 0 // Until the port expander code is working
// CPU6

    "set cpu6 config=faultbase=Multics",

    "set cpu6 config=num=6",
    // ; As per GB61-01 Operators Guide, App. A
    // ; switches: 4, 6, 18, 19, 20, 23, 24, 25, 26, 28
    "set cpu6 config=data=024000717200",
    "set cpu6 config=address=000000000000",

    // ; enable ports 0 and 1 (scu connections)
    // ; portconfig: ABCD
    // ;   each is 3 bits addr assignment
    // ;           1 bit enabled
    // ;           1 bit sysinit enabled
    // ;           1 bit interlace enabled (interlace?)
    // ;           3 bit memory size
    // ;              0 - 32K
    // ;              1 - 64K
    // ;              2 - 128K
    // ;              3 - 256K
    // ;              4 - 512K
    // ;              5 - 1M
    // ;              6 - 2M
    // ;              7 - 4M

    "set cpu6 config=port=A",
    "set cpu6   config=assignment=0",
    "set cpu6   config=interlace=0",
    "set cpu6   config=enable=1",
    "set cpu6   config=init_enable=1",
    "set cpu6   config=store_size=4M",

    "set cpu6 config=port=B",
    "set cpu6   config=assignment=1",
    "set cpu6   config=interlace=0",
    "set cpu6   config=enable=1",
    "set cpu6   config=init_enable=1",
    "set cpu6   config=store_size=4M",

    "set cpu6 config=port=C",
    "set cpu6   config=assignment=2",
    "set cpu6   config=interlace=0",
    "set cpu6   config=enable=1",
    "set cpu6   config=init_enable=1",
    "set cpu6   config=store_size=4M",

    "set cpu6 config=port=D",
    "set cpu6   config=assignment=3",
    "set cpu6   config=interlace=0",
    "set cpu6   config=enable=1",
    "set cpu6   config=init_enable=1",
    "set cpu6   config=store_size=4M",

    // ; 0 = GCOS 1 = VMS
    "set cpu6 config=mode=Multics",

    "set cpu6 config=enable_cache=enable",
    "set cpu6 config=sdwam=enable",
    "set cpu6 config=ptwam=enable",

    // ; 0 = 8/70
    "set cpu6 config=speed=0",

    "set cpu6 config=dis_enable=enable",
    "set cpu6 config=steady_clock=disable",
    "set cpu6 config=halt_on_unimplemented=disable",
    "set cpu6 config=enable_wam=disable",
    "set cpu6 config=report_faults=disable",
    "set cpu6 config=tro_enable=enable",
    "set cpu6 config=y2k=disable",
    "set cpu6 config=drl_fatal=disable",
    "set cpu6 config=useMap=disable",
    "set cpu6 config=prom_installed=enable",
    "set cpu6 config=hex_mode_installed=disable",
    "set cpu6 config=cache_installed=enable",
    "set cpu6 config=clock_slave_installed=enable",

# endif

# if 0 // Until the port expander code is working

// CPU7

    "set cpu7 config=faultbase=Multics",

    "set cpu7 config=num=7",
    // ; As per GB61-01 Operators Guide, App. A
    // ; switches: 4, 6, 18, 19, 20, 23, 24, 25, 26, 28
    "set cpu7 config=data=024000717200",
    "set cpu7 config=address=000000000000",

    // ; enable ports 0 and 1 (scu connections)
    // ; portconfig: ABCD
    // ;   each is 3 bits addr assignment
    // ;           1 bit enabled
    // ;           1 bit sysinit enabled
    // ;           1 bit interlace enabled (interlace?)
    // ;           3 bit memory size
    // ;              0 - 32K
    // ;              1 - 64K
    // ;              2 - 128K
    // ;              3 - 256K
    // ;              4 - 512K
    // ;              5 - 1M
    // ;              6 - 2M
    // ;              7 - 4M

    "set cpu7 config=port=A",
    "set cpu7   config=assignment=0",
    "set cpu7   config=interlace=0",
    "set cpu7   config=enable=1",
    "set cpu7   config=init_enable=1",
    "set cpu7   config=store_size=4M",

    "set cpu7 config=port=B",
    "set cpu7   config=assignment=1",
    "set cpu7   config=interlace=0",
    "set cpu7   config=enable=1",
    "set cpu7   config=init_enable=1",
    "set cpu7   config=store_size=4M",

    "set cpu7 config=port=C",
    "set cpu7   config=assignment=2",
    "set cpu7   config=interlace=0",
    "set cpu7   config=enable=1",
    "set cpu7   config=init_enable=1",
    "set cpu7   config=store_size=4M",

    "set cpu7 config=port=D",
    "set cpu7   config=assignment=3",
    "set cpu7   config=interlace=0",
    "set cpu7   config=enable=1",
    "set cpu7   config=init_enable=1",
    "set cpu7   config=store_size=4M",

    // ; 0 = GCOS 1 = VMS
    "set cpu7 config=mode=Multics",

    "set cpu7 config=enable_cache=enable",
    "set cpu7 config=sdwam=enable",
    "set cpu7 config=ptwam=enable",

    // ; 0 = 8/70
    "set cpu7 config=speed=0",

    "set cpu7 config=dis_enable=enable",
    "set cpu7 config=steady_clock=disable",
    "set cpu7 config=halt_on_unimplemented=disable",
    "set cpu7 config=enable_wam=disable",
    "set cpu7 config=report_faults=disable",
    "set cpu7 config=tro_enable=enable",
    "set cpu7 config=y2k=disable",
    "set cpu7 config=drl_fatal=disable",
    "set cpu7 config=useMap=disable",
    "set cpu7 config=prom_installed=enable",
    "set cpu7 config=hex_mode_installed=disable",
    "set cpu7 config=cache_installed=enable",
    "set cpu7 config=clock_slave_installed=enable",

# endif

// IOM0

    "set iom0 config=iom_base=Multics",
    "set iom0 config=multiplex_base=0120",
    "set iom0 config=os=Multics",
    "set iom0 config=boot=tape",
    "set iom0 config=tapechan=012",
    "set iom0 config=cardchan=011",
    "set iom0 config=scuport=0",

    "set iom0 config=port=0",
    "set iom0   config=addr=0",
    "set iom0   config=interlace=0",
    "set iom0   config=enable=1",
    "set iom0   config=initenable=0",
    "set iom0   config=halfsize=0",
    "set iom0   config=store_size=4M",

    "set iom0 config=port=1",
    "set iom0   config=addr=1",
    "set iom0   config=interlace=0",
    "set iom0   config=enable=1",
    "set iom0   config=initenable=0",
    "set iom0   config=halfsize=0",
    "set iom0   config=store_size=4M",

    "set iom0 config=port=2",
    "set iom0   config=addr=2",
    "set iom0   config=interlace=0",
    "set iom0   config=enable=1",
    "set iom0   config=initenable=0",
    "set iom0   config=halfsize=0",
    "set iom0   config=store_size=4M",

    "set iom0 config=port=3",
    "set iom0   config=addr=3",
    "set iom0   config=interlace=0",
    "set iom0   config=enable=1",
    "set iom0   config=initenable=0",
    "set iom0   config=halfsize=0",
    "set iom0   config=store_size=4M",

    "set iom0 config=port=4",
    "set iom0   config=enable=0",

    "set iom0 config=port=5",
    "set iom0   config=enable=0",

    "set iom0 config=port=6",
    "set iom0   config=enable=0",

    "set iom0 config=port=7",
    "set iom0   config=enable=0",

// IOM1

    "set iom1 config=iom_base=Multics2",
    "set iom1 config=multiplex_base=0121",
    "set iom1 config=os=Multics",
    "set iom1 config=boot=tape",
    "set iom1 config=tapechan=012",
    "set iom1 config=cardchan=011",
    "set iom1 config=scuport=0",

    "set iom1 config=port=0",
    "set iom1   config=addr=0",
    "set iom1   config=interlace=0",
    "set iom1   config=enable=1",
    "set iom1   config=initenable=0",
    "set iom1   config=halfsize=0;",

    "set iom1 config=port=1",
    "set iom1   config=addr=1",
    "set iom1   config=interlace=0",
    "set iom1   config=enable=1",
    "set iom1   config=initenable=0",
    "set iom1   config=halfsize=0;",

    "set iom1 config=port=2",
    "set iom1   config=enable=0",
    "set iom1 config=port=3",
    "set iom1   config=enable=0",
    "set iom1 config=port=4",
    "set iom1   config=enable=0",
    "set iom1 config=port=5",
    "set iom1   config=enable=0",
    "set iom1 config=port=6",
    "set iom1   config=enable=0",
    "set iom1 config=port=7",
    "set iom1   config=enable=0",

# if 0

// IOM2

    "set iom2 config=iom_base=Multics2",
    "set iom2 config=multiplex_base=0121",
    "set iom2 config=os=Multics",
    "set iom2 config=boot=tape",
    "set iom2 config=tapechan=012",
    "set iom2 config=cardchan=011",
    "set iom2 config=scuport=0",

    "set iom2 config=port=0",
    "set iom2   config=addr=0",
    "set iom2   config=interlace=0",
    "set iom2   config=enable=1",
    "set iom2   config=initenable=0",
    "set iom2   config=halfsize=0;",

    "set iom2 config=port=1",
    "set iom2   config=addr=1",
    "set iom2   config=interlace=0",
    "set iom2   config=enable=1",
    "set iom2   config=initenable=0",
    "set iom2   config=halfsize=0;",

    "set iom2 config=port=2",
    "set iom2   config=enable=0",
    "set iom2 config=port=3",
    "set iom2   config=enable=0",
    "set iom2 config=port=4",
    "set iom2   config=enable=0",
    "set iom2 config=port=5",
    "set iom2   config=enable=0",
    "set iom2 config=port=6",
    "set iom2   config=enable=0",
    "set iom2 config=port=7",
    "set iom2   config=enable=0",

// IOM3

    "set iom3 config=iom_base=Multics2",
    "set iom3 config=multiplex_base=0121",
    "set iom3 config=os=Multics",
    "set iom3 config=boot=tape",
    "set iom3 config=tapechan=012",
    "set iom3 config=cardchan=011",
    "set iom3 config=scuport=0",

    "set iom3 config=port=0",
    "set iom3   config=addr=0",
    "set iom3   config=interlace=0",
    "set iom3   config=enable=1",
    "set iom3   config=initenable=0",
    "set iom3   config=halfsize=0;",

    "set iom3 config=port=1",
    "set iom3   config=addr=1",
    "set iom3   config=interlace=0",
    "set iom3   config=enable=1",
    "set iom3   config=initenable=0",
    "set iom3   config=halfsize=0;",

    "set iom3 config=port=2",
    "set iom3   config=enable=0",
    "set iom3 config=port=3",
    "set iom3   config=enable=0",
    "set iom3 config=port=4",
    "set iom3   config=enable=0",
    "set iom3 config=port=5",
    "set iom3   config=enable=0",
    "set iom3 config=port=6",
    "set iom3   config=enable=0",
    "set iom3 config=port=7",
    "set iom3   config=enable=0",
# endif

// SCU0

    "set scu0 config=mode=program",
    "set scu0 config=port0=enable",
    "set scu0 config=port1=enable",
    "set scu0 config=port2=enable",
    "set scu0 config=port3=enable",
    "set scu0 config=port4=enable",
    "set scu0 config=port5=enable",
    "set scu0 config=port6=enable",
    "set scu0 config=port7=enable",
    "set scu0 config=maska=7",
    "set scu0 config=maskb=off",
    "set scu0 config=lwrstoresize=7",
    "set scu0 config=cyclic=0040",
    "set scu0 config=nea=0200",
    "set scu0 config=onl=014",
    "set scu0 config=int=0",
    "set scu0 config=lwr=0",

// SCU1

    "set scu1 config=mode=program",
    "set scu1 config=port0=enable",
    "set scu1 config=port1=enable",
    "set scu1 config=port2=enable",
    "set scu1 config=port3=enable",
    "set scu1 config=port4=enable",
    "set scu1 config=port5=enable",
    "set scu1 config=port6=enable",
    "set scu1 config=port7=enable",
    "set scu1 config=maska=off",
    "set scu1 config=maskb=off",
    "set scu1 config=lwrstoresize=7",
    "set scu1 config=cyclic=0040",
    "set scu1 config=nea=0200",
    "set scu1 config=onl=014",
    "set scu1 config=int=0",
    "set scu1 config=lwr=0",

// SCU2

    "set scu2 config=mode=program",
    "set scu2 config=port0=enable",
    "set scu2 config=port1=enable",
    "set scu2 config=port2=enable",
    "set scu2 config=port3=enable",
    "set scu2 config=port4=enable",
    "set scu2 config=port5=enable",
    "set scu2 config=port6=enable",
    "set scu2 config=port7=enable",
    "set scu2 config=maska=off",
    "set scu2 config=maskb=off",
    "set scu2 config=lwrstoresize=7",
    "set scu2 config=cyclic=0040",
    "set scu2 config=nea=0200",
    "set scu2 config=onl=014",
    "set scu2 config=int=0",
    "set scu2 config=lwr=0",

// SCU3

    "set scu3 config=mode=program",
    "set scu3 config=port0=enable",
    "set scu3 config=port1=enable",
    "set scu3 config=port2=enable",
    "set scu3 config=port3=enable",
    "set scu3 config=port4=enable",
    "set scu3 config=port5=enable",
    "set scu3 config=port6=enable",
    "set scu3 config=port7=enable",
    "set scu3 config=maska=off",
    "set scu3 config=maskb=off",
    "set scu3 config=lwrstoresize=7",
    "set scu3 config=cyclic=0040",
    "set scu3 config=nea=0200",
    "set scu3 config=onl=014",
    "set scu3 config=int=0",
    "set scu3 config=lwr=0",

# if 0
// SCU4

    "set scu4 config=mode=program",
    "set scu4 config=port0=enable",
    "set scu4 config=port1=enable",
    "set scu4 config=port2=enable",
    "set scu4 config=port3=enable",
    "set scu4 config=port4=enable",
    "set scu4 config=port5=enable",
    "set scu4 config=port6=enable",
    "set scu4 config=port7=enable",
    "set scu4 config=maska=off",
    "set scu4 config=maskb=off",
    "set scu4 config=lwrstoresize=7",
    "set scu4 config=cyclic=0040",
    "set scu4 config=nea=0200",
    "set scu4 config=onl=014",
    "set scu4 config=int=0",
    "set scu4 config=lwr=0",

// SCU5

    "set scu5 config=mode=program",
    "set scu5 config=port0=enable",
    "set scu5 config=port1=enable",
    "set scu5 config=port2=enable",
    "set scu5 config=port3=enable",
    "set scu5 config=port4=enable",
    "set scu5 config=port5=enable",
    "set scu5 config=port6=enable",
    "set scu5 config=port7=enable",
    "set scu5 config=maska=off",
    "set scu5 config=maskb=off",
    "set scu5 config=lwrstoresize=7",
    "set scu5 config=cyclic=0040",
    "set scu5 config=nea=0200",
    "set scu5 config=onl=014",
    "set scu5 config=int=0",
    "set scu5 config=lwr=0",

// SCU6

    "set scu6 config=mode=program",
    "set scu6 config=port0=enable",
    "set scu6 config=port1=enable",
    "set scu6 config=port2=enable",
    "set scu6 config=port3=enable",
    "set scu6 config=port4=enable",
    "set scu6 config=port5=enable",
    "set scu6 config=port6=enable",
    "set scu6 config=port7=enable",
    "set scu6 config=maska=off",
    "set scu6 config=maskb=off",
    "set scu6 config=lwrstoresize=7",
    "set scu6 config=cyclic=0040",
    "set scu6 config=nea=0200",
    "set scu6 config=onl=014",
    "set scu6 config=int=0",
    "set scu6 config=lwr=0",

// SCU7

    "set scu7 config=mode=program",
    "set scu7 config=port0=enable",
    "set scu7 config=port1=enable",
    "set scu7 config=port2=enable",
    "set scu7 config=port3=enable",
    "set scu7 config=port4=enable",
    "set scu7 config=port5=enable",
    "set scu7 config=port6=enable",
    "set scu7 config=port7=enable",
    "set scu7 config=maska=off",
    "set scu7 config=maskb=off",
    "set scu7 config=lwrstoresize=7",
    "set scu7 config=cyclic=0040",
    "set scu7 config=nea=0200",
    "set scu7 config=onl=014",
    "set scu7 config=int=0",
    "set scu7 config=lwr=0",
# endif

    // ; There are bugs in the FNP code that require sim unit number
    // ; to be the same as the Multics unit number; ie fnp0 == fnpa, etc.
    // ;
    // ; fnp a 3400
    // ; fnp b 3700
    // ; fnp c 4200
    // ; fnp d 4500
    // ; fnp e 5000
    // ; fnp f 5300
    // ; fnp g 5600
    // ; fnp h 6100

    "set fnp0 config=mailbox=03400",
    "set fnp0 ipc_name=fnp-a",
    "set fnp1 config=mailbox=03700",
    "set fnp1 ipc_name=fnp-b",
    "set fnp2 config=mailbox=04200",
    "set fnp2 ipc_name=fnp-c",
    "set fnp3 config=mailbox=04500",
    "set fnp3 ipc_name=fnp-d",
    "set fnp4 config=mailbox=05000",
    "set fnp4 ipc_name=fnp-e",
    "set fnp5 config=mailbox=05300",
    "set fnp5 ipc_name=fnp-f",
    "set fnp6 config=mailbox=05600",
    "set fnp6 ipc_name=fnp-g",
    "set fnp7 config=mailbox=06100",
    "set fnp7 ipc_name=fnp-h",

    //XXX"set mtp0 boot_drive=1",
    // ; Attach tape MPC to IOM 0, chan 012, dev_code 0
    "set mtp0 boot_drive=0",
    "set mtp0 name=MTP0",
    // ; Attach TAPE unit 0 to IOM 0, chan 012, dev_code 1
    "cable IOM0 012 MTP0 0",
    "cable IOM1 012 MTP0 1",
    "cable MTP0 1 TAPE1",
    "set tape1 name=tapa_01",
    "cable MTP0 2 TAPE2",
    "set tape2 name=tapa_02",
    "cable MTP0 3 TAPE3",
    "set tape3 name=tapa_03",
    "cable MTP0 4 TAPE4",
    "set tape4 name=tapa_04",
    "cable MTP0 5 TAPE5",
    "set tape5 name=tapa_05",
    "cable MTP0 6 TAPE6",
    "set tape6 name=tapa_06",
    "cable MTP0 7 TAPE7",
    "set tape7 name=tapa_07",
    "cable MTP0 8 TAPE8",
    "set tape8 name=tapa_08",
    "cable MTP0 9 TAPE9",
    "set tape9 name=tapa_09",
    "cable MTP0 10 TAPE10",
    "set tape10 name=tapa_10",
    "cable MTP0 11 TAPE11",
    "set tape11 name=tapa_11",
    "cable MTP0 12 TAPE12",
    "set tape12 name=tapa_12",
    "cable MTP0 13 TAPE13",
    "set tape13 name=tapa_13",
    "cable MTP0 14 TAPE14",
    "set tape14 name=tapa_14",
    "cable MTP0 15 TAPE15",
    "set tape15 name=tapa_15",
    "cable MTP0 16 TAPE16",
    "set tape16 name=tapa_16",

// 4 3381 disks

    "set ipc0 name=IPC0",
    "cable IOM0 013 IPC0 0",
    "cable IOM1 013 IPC0 1",
    // ; Attach DISK unit 0 to IPC0 dev_code 0",
    "cable IPC0 0 DISK0",
    "set disk0 type=3381",
    "set disk0 name=dska_00",
    // ; Attach DISK unit 1 to IPC0 dev_code 1",
    "cable IPC0 1 DISK1",
    "set disk1 type=3381",
    "set disk1 name=dska_01",
    // ; Attach DISK unit 2 to IPC0 dev_code 2",
    "cable IPC0 2 DISK2",
    "set disk2 type=3381",
    "set disk2 name=dska_02",
    // ; Attach DISK unit 3 to IPC0 dev_code 3",
    "cable IPC0 3 DISK3",
    "set disk3 type=3381",
    "set disk3 name=dska_03",

// 4 d501 disks + 4 d451 disks + 2 d500 disks

    "set msp0 name=MSP0",
    "cable IOM0 014 MSP0 0",
    "cable IOM1 014 MSP0 1",

    // ; Attach DISK unit 4 to MSP0 dev_code 1",
    "cable MSP0 1 DISK4",
    "set disk4 type=d501",
    "set disk4 name=dskb_01",
    // ; Attach DISK unit 5 to MSP0 dev_code 2",
    "cable MSP0 2 DISK5",
    "set disk5 type=d501",
    "set disk5 name=dskb_02",
    // ; Attach DISK unit 6 to MSP0 dev_code 3",
    "cable MSP0 3 DISK6",
    "set disk6 type=d501",
    "set disk6 name=dskb_03",
    // ; Attach DISK unit 7 to MSP0 dev_code 4",
    "cable MSP0 4 DISK7",
    "set disk7 type=d501",
    "set disk7 name=dskb_04",

    // ; Attach DISK unit 8 to MSP0 dev_code 5",
    "cable MSP0 5 DISK8",
    "set disk8 type=d451",
    "set disk8 name=dskb_05",
    // ; Attach DISK unit 9 to MSP0 dev_code 6",
    "cable MSP0 6 DISK9",
    "set disk9 type=d451",
    "set disk9 name=dskb_06",
    // ; Attach DISK unit 10 to MSP0 dev_code 7",
    "cable MSP0 7 DISK10",
    "set disk10 type=d451",
    "set disk10 name=dskb_07",
    // ; Attach DISK unit 11 to MSP0 dev_code 8",
    "cable MSP0 8 DISK11",
    "set disk11 type=d451",
    "set disk11 name=dskb_08",
    // ; Attach DISK unit 12 to MSP0 dev_code 9",
    "cable MSP0 9 DISK12",
    "set disk12 type=d500",
    "set disk12 name=dskb_09",
    // ; Attach DISK unit 13 to MSP0 dev_code 10",
    "cable MSP0 10 DISK13",
    "set disk13 type=d500",
    "set disk13 name=dskb_10",

    // since we define 16 (decimal) 3381s in the default config deck, we need to add another 12
    // ; Attach DISK unit 14 to IPC0 dev_code 4",
    "cable IPC0 4 DISK14",
    "set disk14 type=3381",
    "set disk14 name=dska_04",
    // ; Attach DISK unit 15 to IPC0 dev_code 5",
    "cable IPC0 5 DISK15",
    "set disk15 type=3381",
    "set disk15 name=dska_05",
    // ; Attach DISK unit 16 to IPC0 dev_code 6",
    "cable IPC0 6 DISK16",
    "set disk16 type=3381",
    "set disk16 name=dska_06",
    // ; Attach DISK unit 17 to IPC0 dev_code 7",
    "cable IPC0 7 DISK17",
    "set disk17 type=3381",
    "set disk17 name=dska_07",
    // ; Attach DISK unit 18 to IPC0 dev_code 8",
    "cable IPC0 8 DISK18",
    "set disk18 type=3381",
    "set disk18 name=dska_08",
    // ; Attach DISK unit 19 to IPC0 dev_code 9",
    "cable IPC0 9 DISK19",
    "set disk19 type=3381",
    "set disk19 name=dska_09",
    // ; Attach DISK unit 20 to IPC0 dev_code 10",
    "cable IPC0 10 DISK20",
    "set disk20 type=3381",
    "set disk20 name=dska_10",
    // ; Attach DISK unit 21 to IPC0 dev_code 11",
    "cable IPC0 11 DISK21",
    "set disk21 type=3381",
    "set disk21 name=dska_11",
    // ; Attach DISK unit 22 to IPC0 dev_code 12",
    "cable IPC0 12 DISK22",
    "set disk22 type=3381",
    "set disk22 name=dska_12",
    // ; Attach DISK unit 23 to IPC0 dev_code 13",
    "cable IPC0 13 DISK23",
    "set disk23 type=3381",
    "set disk23 name=dska_13",
    // ; Attach DISK unit 24 to IPC0 dev_code 14",
    "cable IPC0 14 DISK24",
    "set disk24 type=3381",
    "set disk24 name=dska_14",
    // ; Attach DISK unit 25 to IPC0 dev_code 15",
    "cable IPC0 15 DISK25",
    "set disk25 type=3381",
    "set disk25 name=dska_15",

    // ; Attach OPC unit 0 to IOM A, chan 036, dev_code 0
    "cable IOMA 036 opc0",
    "cable IOMA 053 opc1",
    "set opc1 config=model=m6601",
   // No devices for console, so no 'cable OPC0 # CONx'

    // ;;;
    // ;;; FNP
    // ;;;

    // ; Attach FNP unit 3 (d) to IOM A, chan 020, dev_code 0
    "cable IOMA 020 FNPD",
    // ; Attach FNP unit 0 (a) to IOM A, chan 021, dev_code 0
    "cable IOMA 021 FNPA",
    // ; Attach FNP unit 1 (b) to IOM A, chan 022, dev_code 0
    "cable IOMA 022 FNPB",
    // ; Attach FNP unit 2 (c) to IOM A, chan 023, dev_code 0
    "cable IOMA 023 FNPC",
    // ; Attach FNP unit 4 (e) to IOM A, chan 024, dev_code 0
    "cable IOMA 024 FNPE",
    // ; Attach FNP unit 5 (f) to IOM A, chan 025, dev_code 0
    "cable IOMA 025 FNPF",
    // ; Attach FNP unit 6 (g) to IOM A, chan 026, dev_code 0
    "cable IOMA 026 FNPG",
    // ; Attach FNP unit 7 (h) to IOM A, chan 027, dev_code 0
    "cable IOMA 027 FNPH",

    // ;;;
    // ;;; MPC
    // ;;;

    // ; Attach MPC unit 0 to IOM 0, char 015, dev_code 0
    "cable IOM0 015 URP0",
    "set urp0 name=urpa",

    // ; Attach RDR unit 0 to IOM 0, chan 015, dev_code 1
    "cable URP0 1 RDR0",
    "set rdr0 name=rdra",

    // ; Attach MPC unit 1 to IOM 0, char 016, dev_code 0
    "cable IOM0 016 URP1",
    "set urp1 name=urpb",

    // ; Attach PUN unit 0 to IOM 0, chan 016, dev_code 1
    "cable URP1 1 PUN0",
    "set pun0 name=puna",

    // ; Attach MPC unit 2 to IOM 0, char 017, dev_code 0
    "cable IOM0 017 URP2",
    "set urp2 name=urpc",

    // ; Attach PRT unit 0 to IOM 0, chan 017, dev_code 1
    "set prt0 name=prta",
//    "set prt0 model=1600",    // Needs polts fixes
    "cable URP2 1 PRT0",

    // ; Attach MPC unit 3 to IOM 0, char 050, dev_code 0
    "cable ioma 050 urp3",
    "set urp3 name=urpd",

    // ; Attach PRT unit 1 to IOM 0, chan 050, dev_code 1
    "set prt1 name=prtb",
//    "set prt1 model=303",    // Needs polts fixes
    "cable urp3 1 prt1",

    // ; Attach MPC unit 4 to IOM 0, char 051, dev_code 0
    "cable ioma 051 urp4",
    "set urp4 name=urpe",

    // ; Attach PRT unit 2 to IOM 0, chan 051, dev_code 1
    "set prt2 name=prtc",
//    "set prt2 model=300",    // Needs polts fixes
    "cable urp4 1 prt2",

    // ; Attach MPC unit 5 to IOM 0, chan 052, dev_code 0
    "cable ioma 052 urp5",
    "set urp5 name=urpf",

    // ; Attach PRT unit 3 to IOM 0, chan 052, dev_code 1
    "set prt3 name=prtd",
//    "set prt3 model=202",    // Needs polts fixes
    "cable urp5 1 prt3",

    // ; Attach MPC unit 6 to IOM 0, chan 055, dev_code 0
    "cable ioma 055 urp6",
    "set urp6 name=urpg",

    // ; Attach RDR unit 1 to IOM 0, chan 055, dev_code 1
    "set rdr1 name=rdrb",
    "cable urp6 1 rdrb",

    // ; Attach MPC unit 7 to IOM 0, chan 056, dev_code 0
    "cable ioma 056 urp7",
    "set urp7 name=urph",

    // ; Attach RDR unit 2 to IOM 0, chan 056, dev_code 1
    "set rdr2 name=rdrc",
    "cable urp7 1 rdrc",

    // ; Attach MPC unit 8 to IOM 0, chan 057, dev_code 0
    "cable ioma 057 urp8",
    "set urp8 name=urpi",

    // ; Attach PUN unit 1 to IOM 0, chan 057, dev_code 1
    "set pun1 name=punb",
    "cable urp8 1 punb",

    // ; Attach MPC unit 9 to IOM 0, chan 060, dev_code 0
    "cable ioma 060 urp9",
    "set urp9 name=urpj",

    // ; Attach PUN unit 2 to IOM 0, chan 060, dev_code 1
    "set pun2 name=punc",
    "cable urp9 1 punc",

# ifndef __MINGW64__
#  ifndef __MINGW32__
#   ifndef CROSS_MINGW32
#    ifndef CROSS_MINGW64
    "cable IOMA 040 SKCA",
    "cable IOMA 041 SKCB",
    "cable IOMA 042 SKCC",
    "cable IOMA 043 SKCD",
    "cable IOMA 044 SKCE",
    "cable IOMA 045 SKCF",
    "cable IOMA 046 SKCG",
    "cable IOMA 047 SKCH",
#    endif /* ifndef CROSS_MINGW64 */
#   endif /* ifndef CROSS_MINGW32 */
#  endif /* ifndef __MINGW32__ */
# endif /* ifndef __MINGW64__ */

# if 0
    // ; Attach PRT unit 1 to IOM 0, chan 017, dev_code 2
    "set prt1 name=prtb",
    "cable URP2 2 PRT1",

    // ; Attach PRT unit 2 to IOM 0, chan 017, dev_code 3
    "set prt2 name=prtc",
    "cable URP2 3 PRT2",

    // ; Attach PRT unit 3 to IOM 0, chan 017, dev_code 4
    "cable URP2 4 PRT3",
    "set prt3 name=prtd",

    // ; Attach PRT unit 4 to IOM 0, chan 017, dev_code 5
    "cable URP2 5 PRT4",
    "set prt4 name=prte",

    // ; Attach PRT unit 5 to IOM 0, chan 017, dev_code 6
    "cable URP2 6 PRT5",
    "set prt5 name=prtf",

    // ; Attach PRT unit 6 to IOM 0, chan 017, dev_code 7
    "cable URP2 7 PRT6",
    "set prt6 name=prtg",

    // ; Attach PRT unit 7 to IOM 0, chan 017, dev_code 8
    "cable URP2 8 PRT7",
    "set prt7 name=prth",

    // ; Attach PRT unit 8 to IOM 0, chan 017, dev_code 9
    "cable URP2 9 PRT8",
    "set prt8 name=prti",

    // ; Attach PRT unit 9 to IOM 0, chan 017, dev_code 10
    "cable URP2 10 PRT9",
    "set prt9 name=prtj",

    // ; Attach PRT unit 10 to IOM 0, chan 017, dev_code 11
    "cable URP2 11 PRT10",
    "set prt10 name=prtk",

    // ; Attach PRT unit 11 to IOM 0, chan 017, dev_code 12
    "cable URP2 12 PRT11",
    "set prt11 name=prtl",

    // ; Attach PRT unit 12 to IOM 0, chan 017, dev_code 13
    "cable URP2 13 PRT12",
    "set prt12 name=prtm",

    // ; Attach PRT unit 13 to IOM 0, chan 017, dev_code 14
    "cable URP2 14 PRT13",
    "set prt13 name=prtn",

    // ; Attach PRT unit 14 to IOM 0, chan 017, dev_code 15
    "cable URP2 15 PRT14",
    "set prt14 name=prto",

    // ; Attach PRT unit 15 to IOM 0, chan 017, dev_code 16
    "set prt15 name=prtp",

    // ; Attach PRT unit 16 to IOM 0, chan 017, dev_code 17
    "set prt16 name=prtq",
# endif

# ifndef __MINGW64__
#  ifndef __MINGW32__
#   ifndef CROSS_MINGW64
#    ifndef CROSS_MINGW32
    // ; Attach ABSI unit 0 to IOM 0, chan 032, dev_code 0
    "cable IOM0 032 ABSI0",
#    endif /* CROSS_MINGW32 */
#   endif /* CROSS_MINGW64 */
#  endif /* __MINGW32__ */
# endif /* __MINGW64__ */

    // ; Attach IOM unit 0 port A (0) to SCU unit 0, port 0
    "cable SCU0 0 IOM0 0", // SCU0 port 0 IOM0 port 0

    // ; Attach IOM unit 0 port B (1) to SCU unit 1, port 0
    "cable SCU1 0 IOM0 1", // SCU1 port 0 IOM0 port 1

    // ; Attach IOM unit 0 port C (2) to SCU unit 2, port 0
    "cable SCU2 0 IOM0 2", // SCU2 port 0 IOM0 port 2

    // ; Attach IOM unit 0 port D (3) to SCU unit 3, port 0
    "cable SCU3 0 IOM0 3", // SCU3 port 0 IOM0 port 3

    // ; Attach IOM unit 1 port A (0) to SCU unit 0, port 1
    "cable SCU0 1 IOM1 0", // SCU0 port 0 IOM0 port 0

    // ; Attach IOM unit 1 port B (1) to SCU unit 1, port 1
    "cable SCU1 1 IOM1 1", // SCU1 port 0 IOM0 port 1

    // ; Attach IOM unit 1 port C (2) to SCU unit 2, port 1
    "cable SCU2 1 IOM1 2", // SCU2 port 0 IOM0 port 2

    // ; Attach IOM unit 1 port D (3) to SCU unit 3, port 1
    "cable SCU3 1 IOM1 3", // SCU3 port 0 IOM0 port 3

// SCU0 --> CPU0-5

    // ; Attach SCU unit 0 port 7 to CPU unit A (1), port 0
    "cable SCU0 7 CPU0 0",

    // ; Attach SCU unit 0 port 6 to CPU unit B (1), port 0
    "cable SCU0 6 CPU1 0",

    // ; Attach SCU unit 0 port 5 to CPU unit C (2), port 0
    "cable SCU0 5 CPU2 0",

    // ; Attach SCU unit 0 port 4 to CPU unit D (3), port 0
    "cable SCU0 4 CPU3 0",

    // ; Attach SCU unit 0 port 3 to CPU unit E (4), port 0
    "cable SCU0 3 CPU4 0",

    // ; Attach SCU unit 0 port 2 to CPU unit F (5), port 0
    "cable SCU0 2 CPU5 0",

// SCU1 --> CPU0-5

    // ; Attach SCU unit 1 port 7 to CPU unit A (1), port 1
    "cable SCU1 7 CPU0 1",

    // ; Attach SCU unit 1 port 6 to CPU unit B (1), port 1
    "cable SCU1 6 CPU1 1",

    // ; Attach SCU unit 1 port 5 to CPU unit C (2), port 1
    "cable SCU1 5 CPU2 1",

    // ; Attach SCU unit 1 port 4 to CPU unit D (3), port 1
    "cable SCU1 4 CPU3 1",

    // ; Attach SCU unit 1 port 3 to CPU unit E (4), port 0
    "cable SCU1 3 CPU4 1",

    // ; Attach SCU unit 1 port 2 to CPU unit F (5), port 0
    "cable SCU1 2 CPU5 1",

// SCU2 --> CPU0-5

    // ; Attach SCU unit 2 port 7 to CPU unit A (1), port 2
    "cable SCU2 7 CPU0 2",

    // ; Attach SCU unit 2 port 6 to CPU unit B (1), port 2
    "cable SCU2 6 CPU1 2",

    // ; Attach SCU unit 2 port 5 to CPU unit C (2), port 2
    "cable SCU2 5 CPU2 2",

    // ; Attach SCU unit 2 port 4 to CPU unit D (3), port 2
    "cable SCU2 4 CPU3 2",

    // ; Attach SCU unit 2 port 3 to CPU unit E (4), port 0
    "cable SCU2 3 CPU4 2",

    // ; Attach SCU unit 2 port 2 to CPU unit F (5), port 0
    "cable SCU2 2 CPU5 2",

// SCU3 --> CPU0-5

    // ; Attach SCU unit 3 port 7 to CPU unit A (1), port 3
    "cable SCU3 7 CPU0 3",

    // ; Attach SCU unit 3 port 6 to CPU unit B (1), port 3
    "cable SCU3 6 CPU1 3",

    // ; Attach SCU unit 3 port 5 to CPU unit C (2), port 3
    "cable SCU3 5 CPU2 3",

    // ; Attach SCU unit 3 port 4 to CPU unit D (3), port 3
    "cable SCU3 4 CPU3 3",

    // ; Attach SCU unit 3 port 3 to CPU unit E (4), port 0
    "cable SCU3 3 CPU4 3",

    // ; Attach SCU unit 3 port 2 to CPU unit F (5), port 0
    "cable SCU3 2 CPU5 3",

    "set cpu0 reset",
    "set scu0 reset",
    "set scu1 reset",
    "set scu2 reset",
    "set scu3 reset",
    "set iom0 reset",

# if defined(THREADZ) || defined(LOCKLESS)
    "set cpu nunits=6",
# else
#  ifdef ROUND_ROBIN
    "set cpu nunits=6",
#  else
    "set cpu nunits=1",
#  endif
# endif // THREADZ
    // "set sys config=activate_time=8",
    // "set sys config=terminate_time=8",
    "set sys config=connect_time=-1",
  }; // default_base_system_script

// Execute a line of script

static void do_ini_line (char * text)
  {
    //sim_msg ("<%s?\n", text);
    char gbuf[257];
    const char * cptr = get_glyph (text, gbuf, 0); /* get command glyph */
    CTAB *cmdp;
    if ((cmdp = find_cmd (gbuf)))            /* lookup command */
      {
        t_stat stat = cmdp->action (cmdp->arg, cptr); /* if found, exec */
        if (stat != SCPE_OK)
          sim_warn ("%s: %s\n", sim_error_text (SCPE_UNK), text);
      }
    else
      sim_warn ("%s: %s\n", sim_error_text (SCPE_UNK), text);
  }

// Execute the base system script; this strings the cables
// and sets the switches

static t_stat set_default_base_system (UNUSED int32 arg, UNUSED const char * buf)
  {
# ifdef PERF_STRIP
    cpu_dev.numunits = 1;
# else
    int n_lines = sizeof (default_base_system_script) / sizeof (char *);
    for (int line = 0; line < n_lines; line ++)
      do_ini_line (default_base_system_script [line]);
# endif
    return SCPE_OK;
  }

// Skip records on the boot tape.
// The T&D tape first record is for testing DPS8s, the
// second record (1st record / tape mark / 2nd record)
// is for testing DPS8/Ms.
// XXX assumes the boot tape is on tape unit 0

static t_stat boot_skip (int32 UNUSED arg, UNUSED const char * buf)
  {
    uint32 skipped;
    t_stat rc = sim_tape_sprecsf (& mt_unit[0], 1, & skipped);
    if (rc == SCPE_OK)
      tape_states[0].rec_num ++;
    return rc;
  }

// Simulate pressing the 'EXECUTE FAULT' button. Used as an
// emergency interrupt of Multics if it hangs and becomes
// unresponsive to the operators console.

static t_stat do_execute_fault (UNUSED int32 arg,  UNUSED const char * buf)
  {
    // Assume bootload CPU
    setG7fault (0, FAULT_EXF, fst_zero);
    return SCPE_OK;
  }

// Simulate pressing the 'XED 10000' sequence; this starts BCE
//
//  sim> restart
//  sim> go

static t_stat do_restart (UNUSED int32 arg,  UNUSED const char * buf)
  {
    if (sim_is_running)
      {
        sim_printf ("Don't restart a running system....\r\n");
        return SCPE_ARG;
      }
    int n = 010000;
    if (buf)
      {
        n = (int) strtol (buf, NULL, 0);
      }
    sim_printf ("Restart entry 0%o\n", n);

# if 0
    // Assume bootload CPU
    cpu.cu.IWB = M [n] & MASK36;
    cpu.cu.IRODD = M [n + 1] & MASK36;
    cpu.cu.xde = 1;
    cpu.cu.xdo = 1;
    cpu.isExec = true;
    cpu.isXED = true;
    cpu.cycle = FAULT_EXEC_cycle;
    set_addr_mode (ABSOLUTE_mode);
# endif

    cpu_reset_unit_idx (0, false);
    cpu.restart         = true;
    cpu.restart_address = (uint) n;

    t_stat rc = run_cmd (RU_CONT, "");
    return rc;
  }

static t_stat set_sys_polling_interval (UNUSED int32 arg, const char * buf)
  {
    if (! buf)
      return SCPE_ARG;
    int n = atoi (buf);
    if (n < 1 || n > 1000) // 1 millisecond to 1 second
      {
        sim_printf ("POLL %d: must be 1 (1 millisecond) to 1000 (1 second)\r\n", n);
        return SCPE_ARG;
      }
    sim_printf ("Polling set to %d milliseconds\r\n", n);
    sys_opts.sys_poll_interval = (uint) n;
    return SCPE_OK;
  }

static t_stat set_sys_slow_polling_interval (UNUSED int32 arg, const char * buf)
  {
    if (! buf)
      return SCPE_ARG;
    int n = atoi (buf);
    if (n < 1 || n > 1000) // 1 - slow poll every pool; 1000 - slow poll every 1000 polls
      {
        sim_printf ("SLOWPOLL %d: must be 1 (1 slow poll per pol) to 1000 (1 slow poll per 1000 polls)\r\n", n);
        return SCPE_ARG;
      }
    sim_printf ("Slow polling set to %d polls\r\n", n);
    sys_opts.sys_slow_poll_interval = (uint) n;
    return SCPE_OK;
  }

static t_stat set_sys_poll_check_rate (UNUSED int32 arg, const char * buf)
  {
    if (! buf)
      return SCPE_ARG;
    int n = atoi (buf);
    if (n < 1 || n > 1024*1024) // 1 - poll check rate in CPY cycles: 1 - check every cycle; 1024 check every 1024 cycles
      {
        sim_printf ("CHECKPOLL %d: must be 1 (check every cycle) to 1048576 (check every million cycles\r\n", n);
        return SCPE_ARG;
      }
    sim_printf ("Poll check rate set to %d CPU cycles\r\n", n);
    sys_opts.sys_poll_check_rate = (uint) n;
    return SCPE_OK;
  }
#endif /* ifndef PERF_STRIP */

//
// Debugging commands
//

#ifdef TESTING

// Filter settings for our customized sim_debug

// Start debug output at CPU cycle N.
uint64 sim_deb_start      = 0;
// Stop debug output at CPU cycle N.
uint64 sim_deb_stop       = 0;
// Break to scp prompt at CPU cycle N.
uint64 sim_deb_break      = 0;
// Enable CPU sim_debug iff PPR.PSR == N
bool sim_deb_segno_on     = false;
# ifdef NO_C_ELLIPSIS
bool sim_deb_segno[DEBUG_SEGNO_LIMIT];
# else
bool sim_deb_segno[DEBUG_SEGNO_LIMIT] = { [0 ... DEBUG_SEGNO_LIMIT - 1] = false };
# endif
// Enable CPU sim_debug iff PPR.PRR == N
uint64 sim_deb_ringno     = NO_SUCH_RINGNO;
// Suppress CPU sim_debug calls that pass all
// of the filters after N times
uint64 sim_deb_skip_limit = 0;
// Suppress the first N CPU sim_debug calls
// that pass all of the filters
uint64 sim_deb_skip_cnt   = 0;
// Suppress sim_debug until the MME instruction
// has been executed N times
uint64 sim_deb_mme_cntdwn = 0;
// Suppress CPU sim_debug unless CPU number bit set
uint dbgCPUMask           = 0377; // default all 8 on

// Suppress CPU sim_debug unless BAR mode
bool sim_deb_bar          = false;

// Set the various filters

static t_stat dps_debug_mme_cntdwn (UNUSED int32 arg, const char * buf)
  {
    sim_deb_mme_cntdwn = strtoull (buf, NULL, 0);
    sim_msg ("Debug MME countdown set to %"PRId64"\n", sim_deb_mme_cntdwn);
    return SCPE_OK;
  }

static t_stat dps_debug_skip (UNUSED int32 arg, const char * buf)
  {
    sim_deb_skip_cnt   = 0;
    sim_deb_skip_limit = strtoull (buf, NULL, 0);
    sim_msg ("Debug skip set to %"PRId64"\n", sim_deb_skip_limit);
    return SCPE_OK;
  }

static t_stat dps_debug_start (UNUSED int32 arg, const char * buf)
  {
    sim_deb_start = strtoull (buf, NULL, 0);
    sim_msg ("Debug set to start at cycle: %"PRId64"\n", sim_deb_start);
    return SCPE_OK;
  }

static t_stat dps_debug_stop (UNUSED int32 arg, const char * buf)
  {
    sim_deb_stop = strtoull (buf, NULL, 0);
    sim_msg ("Debug set to stop at cycle: %"PRId64"\n", sim_deb_stop);
    return SCPE_OK;
  }

static t_stat dps_debug_break (UNUSED int32 arg, const char * buf)
  {
    sim_deb_break = strtoull (buf, NULL, 0);
    if (buf[0] == '+')
      sim_deb_break += sim_deb_start;
    sim_msg ("Debug set to break at cycle: %"PRId64"\n", sim_deb_break);
    return SCPE_OK;
  }

static t_stat dps_debug_segno (int32 arg, const char * buf)
  {
    if (arg)
      {
        unsigned long segno = strtoul (buf, NULL, 0);
        if (segno >= DEBUG_SEGNO_LIMIT)
          {
            sim_printf ("out of range; 0 to %u %d.\n", DEBUG_SEGNO_LIMIT, DEBUG_SEGNO_LIMIT);
            return SCPE_ARG;
          }
        sim_deb_segno[segno] = true;
        sim_deb_segno_on     = true;
        sim_msg ("Debug set for segno %lo %ld.\n", segno, (long) segno);
      }
    else
      {
        memset (sim_deb_segno, 0, sizeof (sim_deb_segno));
        sim_deb_segno_on = false;
        sim_msg ("Debug set for all segments\n");
      }
    return SCPE_OK;
  }

static t_stat dps_debug_ringno (UNUSED int32 arg, const char * buf)
  {
    sim_deb_ringno = strtoull (buf, NULL, 0);
    sim_msg ("Debug set to ringno %"PRIo64"\n", sim_deb_ringno);
    return SCPE_OK;
  }

static t_stat dps_debug_bar (int32 arg, UNUSED const char * buf)
  {
    sim_deb_bar = arg;
    if (arg)
      sim_msg ("Debug set BAR %"PRIo64"\n", sim_deb_ringno);
    else
      sim_msg ("Debug unset BAR %"PRIo64"\n", sim_deb_ringno);
    return SCPE_OK;
  }

# if 0
t_stat computeAbsAddrN (word24 * abs_addr, int segno, uint offset)
  {
    word24 res;

    if (get_addr_mode () != APPEND_mode)
      {
        sim_warn ("CPU not in append mode\n");
        return SCPE_ARG;
      }

    if (cpu.DSBR.U == 1) // Unpaged
      {
        if (2 * (uint) /*TPR.TSR*/ segno >= 16 * ((uint) cpu.DSBR.BND + 1))
          {
            sim_warn ("DSBR boundary violation.\n");
            return SCPE_ARG;
          }

        // 2. Fetch the target segment SDW from cpu.DSBR.ADDR + 2 * segno.

        word36 SDWe, SDWo;
        core_read ((cpu.DSBR.ADDR + 2U * /*TPR.TSR*/ (uint) segno) & PAMASK,
                   & SDWe, __func__);
        core_read ((cpu.DSBR.ADDR + 2U * /*TPR.TSR*/ (uint) segno  + 1) & PAMASK,
                   & SDWo, __func__);

        // 3. If SDW.DF = 0, then generate directed fault n where n is given in
        // SDW.FC. The value of n used here is the value assigned to define a
        // missing segment fault or, simply, a segment fault.

        // abs_addr doesn't care if the page isn't resident

        // 4. If offset >= 16 * (SDW.BOUND + 1), then generate an access
        // violation, out of segment bounds, fault.

        word14 BOUND = (SDWo >> (35 - 14)) & 037777;
        if (/*TPR.CA*/ offset >= 16 * (BOUND + 1))
          {
            sim_warn ("SDW boundary violation.\n");
            return SCPE_ARG;
          }

        // 5. If the access bits (SDW.R, SDW.E, etc.) of the segment are
        //    incompatible with the reference, generate the appropriate access
        //    violation fault.

        // abs_addr doesn't care

        // 6. Generate 24-bit absolute main memory address SDW.ADDR + offset.

        word24 ADDR = (SDWe >> 12) & 077777760;
        res = (word24) ADDR + (word24) /*TPR.CA*/ offset;
        res &= PAMASK; //24 bit math
        //res <<= 12; // 24:12 format

      }
    else
      {
        //word15 segno = TPR.TSR;
        //word18 offset = TPR.CA;

        // 1. If 2 * segno >= 16 * (cpu.DSBR.BND + 1), then generate an access
        // violation, out of segment bounds, fault.

        if (2 * (uint) segno >= 16 * ((uint) cpu.DSBR.BND + 1))
          {
            sim_warn ("DSBR boundary violation.\n");
            return SCPE_ARG;
          }

        // 2. Form the quantities:
        //       y1 = (2 * segno) modulo 1024
        //       x1 = (2 * segno ­ y1) / 1024

        word24 y1 = (2 * (uint) segno) % 1024;
        word24 x1 = (2 * (uint) segno - y1) / 1024;

        // 3. Fetch the descriptor segment PTW(x1) from DSBR.ADR + x1.

        word36 PTWx1;
        core_read ((cpu.DSBR.ADDR + x1) & PAMASK, & PTWx1, __func__);

        ptw_s PTW1;
        PTW1.ADDR = GETHI(PTWx1);
        PTW1.U = TSTBIT(PTWx1, 9);
        PTW1.M = TSTBIT(PTWx1, 6);
        PTW1.DF = TSTBIT(PTWx1, 2);
        PTW1.FC = PTWx1 & 3;

        // 4. If PTW(x1).DF = 0, then generate directed fault n where n is
        // given in PTW(x1).FC. The value of n used here is the value
        // assigned to define a missing page fault or, simply, a
        // page fault.

        if (!PTW1.DF)
          {
            sim_warn ("!PTW1.DF\n");
            return SCPE_ARG;
          }

        // 5. Fetch the target segment SDW, SDW(segno), from the
        // descriptor segment page at PTW(x1).ADDR + y1.

        word36 SDWeven, SDWodd;
        core_read2(((PTW1.ADDR << 6) + y1) & PAMASK, & SDWeven, & SDWodd,
                    __func__);

        sdw0_s SDW0;
        // even word
        SDW0.ADDR = (SDWeven >> 12) & PAMASK;
        SDW0.R1 = (SDWeven >> 9) & 7;
        SDW0.R2 = (SDWeven >> 6) & 7;
        SDW0.R3 = (SDWeven >> 3) & 7;
        SDW0.DF = TSTBIT(SDWeven, 2);
        SDW0.FC = SDWeven & 3;

        // odd word
        SDW0.BOUND = (SDWodd >> 21) & 037777;
        SDW0.R = TSTBIT(SDWodd, 20);
        SDW0.E = TSTBIT(SDWodd, 19);
        SDW0.W = TSTBIT(SDWodd, 18);
        SDW0.P = TSTBIT(SDWodd, 17);
        SDW0.U = TSTBIT(SDWodd, 16);
        SDW0.G = TSTBIT(SDWodd, 15);
        SDW0.C = TSTBIT(SDWodd, 14);
        SDW0.EB = SDWodd & 037777;

        // 6. If SDW(segno).DF = 0, then generate directed fault n where
        // n is given in SDW(segno).FC.
        // This is a segment fault as discussed earlier in this section.

        if (!SDW0.DF)
          {
            sim_warn ("!SDW0.DF\n");
            return SCPE_ARG;
          }

        // 7. If offset >= 16 * (SDW(segno).BOUND + 1), then generate an
        // access violation, out of segment bounds, fault.

        if (((offset >> 4) & 037777) > SDW0.BOUND)
          {
            sim_warn ("SDW boundary violation\n");
            return SCPE_ARG;
          }

        // 8. If the access bits (SDW(segno).R, SDW(segno).E, etc.) of the
        // segment are incompatible with the reference, generate the
        // appropriate access violation fault.

        // Only the address is wanted, so no check

        if (SDW0.U == 0)
          {
            // Segment is paged
            // 9. Form the quantities:
            //    y2 = offset modulo 1024
            //    x2 = (offset - y2) / 1024

            word24 y2 = offset % 1024;
            word24 x2 = (offset - y2) / 1024;

            // 10. Fetch the target segment PTW(x2) from SDW(segno).ADDR + x2.

            word36 PTWx2;
            core_read ((SDW0.ADDR + x2) & PAMASK, & PTWx2, __func__);

            ptw_s PTW2;
            PTW2.ADDR = GETHI(PTWx2);
            PTW2.U = TSTBIT(PTWx2, 9);
            PTW2.M = TSTBIT(PTWx2, 6);
            PTW2.DF = TSTBIT(PTWx2, 2);
            PTW2.FC = PTWx2 & 3;

            // 11.If PTW(x2).DF = 0, then generate directed fault n where n is
            // given in PTW(x2).FC. This is a page fault as in Step 4 above.

            // abs_addr only wants the address; it doesn't care if the page is
            // resident

            // if (!PTW2.DF)
            //   {
            //     sim_debug (DBG_APPENDING, & cpu_dev, "absa fault !PTW2.DF\n");
            //     // initiate a directed fault
            //     doFault(FAULT_DF0 + PTW2.FC, 0, "ABSA !PTW2.DF");
            //   }

            // 12. Generate the 24-bit absolute main memory address
            // PTW(x2).ADDR + y2.

            res = (((word24) PTW2.ADDR) << 6)  + (word24) y2;
            res &= PAMASK; //24 bit math
            //res <<= 12; // 24:12 format
          }
        else
          {
            // Segment is unpaged
            // SDW0.ADDR is the base address of the segment
            res = (word24) SDW0.ADDR + offset;
            res &= PAMASK; //24 bit math
            res <<= 12; // 24:12 format
          }
      }

    * abs_addr = res;
    return SCPE_OK;
  }
# endif

// Translate seg:offset to absolute address

static t_stat abs_addr_n (int segno, uint offset)
  {
    word24 res;

    //t_stat rc = computeAbsAddrN (& res, segno, offset);
    if (dbgLookupAddress ((word18) segno, offset, & res, NULL))
      return SCPE_ARG;

    sim_msg ("Address is %08o\n", res);
    return SCPE_OK;
  }

// ABS segno:offset
// scp command to translate segno:offset to absolute address

static t_stat abs_addr (UNUSED int32 arg, const char * buf)
  {
    uint segno;
    uint offset;
    if (sscanf (buf, "%o:%o", & segno, & offset) != 2)
      return SCPE_ARG;
    return abs_addr_n ((int) segno, offset);
  }

// LOAD_SYSTEM_BOOK <filename>

// Read a system_book segment, extracting segment names and numbers
// and component names, offsets, and lengths

# define BOOT_SEGMENTS_MAX 1024
# define BOOT_COMPONENTS_MAX 4096
# define BOOK_SEGMENT_NAME_LEN 33

static struct book_segment
  {
    char * segname;
    int  segno;
  } book_segments[BOOT_SEGMENTS_MAX];

static int n_book_segments = 0;

static struct book_component
  {
    char * compname;
    int  book_segment_number;
    uint txt_start, txt_length;
    int  intstat_start, intstat_length, symbol_start, symbol_length;
  } book_components[BOOT_COMPONENTS_MAX];

static int n_book_components = 0;

static int lookup_book_segment (char * name)
  {
    for (int i = 0; i < n_book_segments; i ++)
      if (strcmp (name, book_segments[i].segname) == 0)
        return i;
    return -1;
  }

static int add_book_segment (char * name, int segno)
  {
    int n = lookup_book_segment (name);
    if (n >= 0)
      return n;
    if (n_book_segments >= BOOT_SEGMENTS_MAX)
      return -1;
    book_segments[n_book_segments].segname = strdup (name);
    book_segments[n_book_segments].segno   = segno;
    n = n_book_segments;
    n_book_segments ++;
    return n;
  }

static int add_book_component (int segnum, char * name, uint txt_start,
                               uint txt_length, int intstat_start,
                               int intstat_length, int symbol_start,
                               int symbol_length)
  {
    if (n_book_components >= BOOT_COMPONENTS_MAX)
      return -1;
    book_components[n_book_components].compname            = strdup (name);
    book_components[n_book_components].book_segment_number = segnum;
    book_components[n_book_components].txt_start           = txt_start;
    book_components[n_book_components].txt_length          = txt_length;
    book_components[n_book_components].intstat_start       = intstat_start;
    book_components[n_book_components].intstat_length      = intstat_length;
    book_components[n_book_components].symbol_start        = symbol_start;
    book_components[n_book_components].symbol_length       = symbol_length;
    int n = n_book_components;
    n_book_components ++;
    return n;
  }

// Given a segno:offset, try to translate to
// component name and offset in the component

// Warning: returns ptr to static buffer
static char * lookup_system_book_address (word18 segno, word18 offset,
                                         char * * compname, word18 * compoffset)
  {
    static char buf[129];
    int i;

    for (i = 0; i < n_book_segments; i ++)
      if (book_segments[i].segno == (int) segno)
        break;

    if (i >= n_book_segments)
      return NULL;

    int best = -1;
    uint bestoffset = 0;

    for (int j = 0; j < n_book_components; j ++)
      {
        if (book_components[j].book_segment_number != i)
          continue;
        if (book_components[j].txt_start <= offset &&
            book_components[j].txt_start + book_components[j].txt_length > offset)
          {
            sprintf (buf, "%s:%s+0%0o", book_segments[i].segname,
              book_components[j].compname,
              offset - book_components[j].txt_start);
            if (compname)
              * compname = book_components[j].compname;
            if (compoffset)
              * compoffset = offset - book_components[j].txt_start;
            return buf;
          }
        if (book_components[j].txt_start <= offset &&
            book_components[j].txt_start > bestoffset)
          {
            best = j;
            bestoffset = book_components[j].txt_start;
          }
      }

    if (best != -1)
      {
        // Didn't find a component track bracketed the offset; return the
        // component that was before the offset
        if (compname)
          * compname = book_components[best].compname;
        if (compoffset)
          * compoffset = offset - book_components[best].txt_start;
        sprintf (buf, "%s:%s+0%0o", book_segments[i].segname,
          book_components[best].compname,
          offset - book_components[best].txt_start);
        return buf;
      }

    // Found a segment, but it had no components. Return the segment name
    // as the component name

    if (compname)
      * compname = book_segments[i].segname;
    if (compoffset)
      * compoffset = offset;
    sprintf (buf, "%s:+0%0o", book_segments[i].segname,
             offset);
    return buf;
  }

// Given a segno and offset, find the component name and its
// offset in the segment

char * lookup_address (word18 segno, word18 offset, char * * compname,
                       word18 * compoffset)
  {
    if (compname)
      * compname = NULL;
    if (compoffset)
      * compoffset = 0;

    // Magic numbers!
    // Multics seems to have a copy of hpchs_ (segno 0162) in segment 0322;
    // This little tweak allows source code level tracing for segment 0322,
    // and has no operational significance to the emulator
    // Hmmm. What is happening is that these segments are being loaded into
    // ring 4, and assigned segment #'s; the assigned number will vary
    // depending on the exact sequence of events.
    if (segno == 0322)
      segno = 0162;
    if (segno == 0310)
      segno = 041;
    if (segno == 0314)
      segno = 041;
    if (segno == 0313)
      segno = 040;
    if (segno == 0317)
      segno = 0161;

# if 0
    // Hack to support formline debugging
#  define IOPOS 02006 // interpret_op_ptr_ offset
    if (segno == 0371)
      {
        if (offset < IOPOS)
          {
            if (compname)
              * compname = "find_condition_info_";
            if (compoffset)
              * compoffset = offset;
            static char buf[129];
            sprintf (buf, "bound_debug_util_:find_condition_info_+0%0o",
                  offset - 0);
            return buf;
          }
        else
          {
            if (compname)
              * compname = "interpret_op_ptr_";
            if (compoffset)
              * compoffset = offset - IOPOS;
            static char buf[129];
            sprintf (buf, "bound_debug_util_:interpret_op_ptr_+0%0o",
                  offset - IOPOS);
            return buf;
          }

      }
# endif

    char * ret = lookup_system_book_address (segno, offset, compname, compoffset);
    return ret;
  }

// Given a segment name and component name, return the
// components segment number and offset

// Warning: returns ptr to static buffer
static int lookup_system_book_name (char * segname, char * compname, long * segno,
                                    long * offset)
  {
    int i;
    for (i = 0; i < n_book_segments; i ++)
      if (strcmp (book_segments[i].segname, segname) == 0)
        break;
    if (i >= n_book_segments)
      return -1;

    for (int j = 0; j < n_book_components; j ++)
      {
        if (book_components[j].book_segment_number != i)
          continue;
        if (strcmp (book_components[j].compname, compname) == 0)
          {
            * segno = book_segments[i].segno;
            * offset = (long) book_components[j].txt_start;
            return 0;
          }
      }

   return -1;
 }

static char * source_search_path = NULL;

// Given a component name and an offset in the component,
// find the listing file of the component and try to
// print the source code line that generated the code at
// component:offset

void list_source (char * compname, word18 offset, uint dflag)
  {
    const int offset_str_len = 10;
    //char offset_str[offset_str_len + 1];
    char offset_str[17];
    sprintf (offset_str, "    %06o", offset);

    char path[(source_search_path ? strlen (source_search_path) : 1) +
               1 + // "/"
               (compname ? strlen (compname) : 1) +
                1 + strlen (".list") + 1];
    char * searchp = source_search_path ? source_search_path : ".";
    // find <search path>/<compname>.list
    while (* searchp)
      {
        size_t pathlen = strcspn (searchp, ":");
        strncpy (path, searchp, pathlen);
        path[pathlen] = '\0';
        if (searchp[pathlen] == ':')
          searchp += pathlen + 1;
        else
          searchp += pathlen;

        if (compname)
          {
            strcat (path, "/");
            strcat (path, compname);
          }
        strcat (path, ".list");
        //sim_msg ("<%s>\n", path);
        FILE * listing = fopen (path, "r");
        if (listing)
          {
            char line[1025];
            if (feof (listing))
              goto fileDone;
            fgets (line, 1024, listing);
            if (strncmp (line, "ASSEMBLY LISTING", 16) == 0)
              {
                // Search ALM listing file
                // sim_msg ("found <%s>\n", path);

                // ALM listing files look like:
                //     000226  4a  4 00010 7421 20  \tstx2]tbootload_0$entry_stack_ptr,id
                while (! feof (listing))
                  {
                    fgets (line, 1024, listing);
                    if (strncmp (line, offset_str, (size_t) offset_str_len) == 0)
                      {
                        if (! dflag)
                          sim_msg ("%s", line);
                        else
                          sim_debug (dflag, & cpu_dev, "%s", line);
                        //break;
                      }
                    if (strcmp (line, "\fLITERALS\n") == 0)
                      break;
                  }
              } // if assembly listing
            else if (strncmp (line, "\tCOMPILATION LISTING", 20) == 0)
              {
                // Search PL/I listing file

                // PL/I files have a line location table
                //     "   LINE    LOC      LINE    LOC ...."

                bool foundTable = false;
                while (! feof (listing))
                  {
                    fgets (line, 1024, listing);
                    if (strncmp (line, "   LINE    LOC", 14) != 0)
                      continue;
                    foundTable = true;
                    // Found the table
                    // Table lines look like
                    //     "     13 000705       275 000713  ...
                    // But some times
                    //     "     10 000156   21   84 000164
                    //     "      8 000214        65 000222    4   84 000225
                    //
                    //     "    349 001442       351 001445       353 001454    1    9 001456    1   11 001461    1   12 001463    1   13 001470
                    //     " 1   18 001477       357 001522       361 001525       363 001544       364 001546       365 001547       366 001553

                    //  I think the numbers refer to include files...
                    //   But of course the format is slightly off...
                    //    table    ".1...18
                    //    listing  ".1....18
                    int best = -1;
                    char bestLines[8] = {0, 0, 0, 0, 0, 0, 0};
                    while (! feof (listing))
                      {
                        int loc[7];
                        char linenos[7][8];
                        memset (linenos, 0, sizeof (linenos));
                        fgets (line, 1024, listing);
                        // sometimes the leading columns are blank...
                        while (strncmp (line,
                                        "                 ", 8 + 6 + 3) == 0)
                          memmove (line, line + 8 + 6 + 3,
                                   strlen (line + 8 + 6 + 3));
                        // deal with the extra numbers...

                        int cnt = sscanf (line,
                          // " %d %o %d %o %d %o %d %o %d %o %d %o %d %o",
                          "%8c%o%*3c"
                          "%8c%o%*3c"
                          "%8c%o%*3c"
                          "%8c%o%*3c"
                          "%8c%o%*3c"
                          "%8c%o%*3c"
                          "%8c%o",
                          (char *) & linenos[0], (uint *) & loc[0],
                          (char *) & linenos[1], (uint *) & loc[1],
                          (char *) & linenos[2], (uint *) & loc[2],
                          (char *) & linenos[3], (uint *) & loc[3],
                          (char *) & linenos[4], (uint *) & loc[4],
                          (char *) & linenos[5], (uint *) & loc[5],
                          (char *) & linenos[6], (uint *) & loc[6]);
                        if (! (cnt == 2 || cnt == 4 || cnt == 6 ||
                               cnt == 8 || cnt == 10 || cnt == 12 ||
                               cnt == 14))
                          break; // end of table
                        int n;
                        for (n = 0; n < cnt / 2; n ++)
                          {
                            if (loc[n] > best && loc[n] <= (int) offset)
                              {
                                best = loc[n];
                                memcpy (bestLines, linenos[n],
                                        sizeof (bestLines));
                              }
                          }
                        if (best == (int) offset)
                          break;
                      }
                    if (best == -1)
                      goto fileDone; // Not found in table

                    //   But of course the format is slightly off...
                    //    table    ".1...18
                    //    listing  ".1....18
                    // bestLines "21   84 "
                    // listing   " 21    84 "
                    char searchPrefix[10];
                    searchPrefix[0] = ' ';
                    searchPrefix[1] = bestLines[0];
                    searchPrefix[2] = bestLines[1];
                    searchPrefix[3] = ' ';
                    searchPrefix[4] = bestLines[2];
                    searchPrefix[5] = bestLines[3];
                    searchPrefix[6] = bestLines[4];
                    searchPrefix[7] = bestLines[5];
                    searchPrefix[8] = bestLines[6];
                    // ignore trailing space; some times it's a tab
                    // searchPrefix[ 9] = bestLines[ 7];
                    searchPrefix[9] = '\0';

                    // Look for the line in the listing
                    rewind (listing);
                    while (! feof (listing))
                      {
                        fgets (line, 1024, listing);
                        if (strncmp (line, "\f\tSOURCE", 8) == 0)
                          goto fileDone; // end of source code listing
                        char prefix[10];
                        strncpy (prefix, line, 9);
                        prefix[9] = '\0';
                        if (strcmp (prefix, searchPrefix) != 0)
                          continue;
                        // Got it
                        if (!dflag)
                          sim_msg ("%s", line);
                        else
                          sim_debug (dflag, & cpu_dev, "%s", line);
                        //break;
                      }
                    goto fileDone;
                  } // if table start
                if (! foundTable)
                  {
                    // Can't find the LINE/LOC table; look for listing
                    rewind (listing);
                    while (! feof (listing))
                      {
                        fgets (line, 1024, listing);
                        if (strncmp (line,
                                     offset_str + 4,
                                     offset_str_len - 4) == 0)
                          {
                            if (! dflag)
                              sim_msg ("%s", line);
                            else
                              sim_debug (dflag, & cpu_dev, "%s", line);
                            //break;
                          }
                        //if (strcmp (line, "\fLITERALS\n") == 0)
                          //break;
                      }
                  } // if ! tableFound
              } // if PL/I listing

fileDone:
            fclose (listing);
          } // if (listing)
      }
  }

// STK

static t_stat stack_trace (UNUSED int32 arg,  UNUSED const char * buf)
  {
    char * msg;

    word15 icSegno = cpu.PPR.PSR;
    word18 icOffset = cpu.PPR.IC;

    sim_msg ("Entry ptr   %05o:%06o\n", icSegno, icOffset);

    char * compname;
    word18 compoffset;
    char * where = lookup_address (icSegno, icOffset,
                                   & compname, & compoffset);
    if (where)
      {
        sim_msg ("%05o:%06o %s\n", icSegno, icOffset, where);
        list_source (compname, compoffset, 0);
      }
    sim_msg ("\n");

    // According to AK92
    //
    //  pr0/ap operator segment pointer
    //  pr6/sp stack frame pointer
    //  pr4/lp linkage section for the executing procedure
    //  pr7/sb stack base

    word15 fpSegno  = cpu.PR[6].SNR;
    word18 fpOffset = cpu.PR[6].WORDNO;

    for (uint frameNo = 1; ; frameNo ++)
      {
        sim_msg ("Frame %d %05o:%06o\n",
                    frameNo, fpSegno, fpOffset);

        word24 fp;
        if (dbgLookupAddress (fpSegno, fpOffset, & fp, & msg))
          {
            sim_msg ("can't lookup fp (%05o:%06o) because %s\n",
                    fpSegno, fpOffset, msg);
            break;
          }

        word15 prevfpSegno  = (word15) ((M[fp + 16] >> 18) & MASK15);
        word18 prevfpOffset = (word18) ((M[fp + 17] >> 18) & MASK18);

        sim_msg ("Previous FP %05o:%06o\n", prevfpSegno, prevfpOffset);

        word15 returnSegno  = (word15) ((M[fp + 20] >> 18) & MASK15);
        word18 returnOffset = (word18) ((M[fp + 21] >> 18) & MASK18);

        sim_msg ("Return ptr  %05o:%06o\n", returnSegno, returnOffset);

        if (returnOffset == 0)
          {
            if (frameNo == 1)
              {
                // try rX[7] as the return address
                sim_msg ("guessing X7 has a return address....\n");
                where = lookup_address (icSegno, cpu.rX[7] - 1,
                                        & compname, & compoffset);
                if (where)
                  {
                    sim_msg ("%05o:%06o %s\n", icSegno, cpu.rX[7] - 1, where);
                    list_source (compname, compoffset, 0);
                  }
              }
          }
        else
          {
            where = lookup_address (returnSegno, returnOffset - 1,
                                    & compname, & compoffset);
            if (where)
              {
                sim_msg ("%05o:%06o %s\n",
                            returnSegno, returnOffset - 1, where);
                list_source (compname, compoffset, 0);
              }
          }

        word15 entrySegno  = (word15) ((M[fp + 22] >> 18) & MASK15);
        word18 entryOffset = (word18) ((M[fp + 23] >> 18) & MASK18);

        sim_msg ("Entry ptr   %05o:%06o\n", entrySegno, entryOffset);

        where = lookup_address (entrySegno, entryOffset,
                                & compname, & compoffset);
        if (where)
          {
            sim_msg ("%05o:%06o %s\n", entrySegno, entryOffset, where);
            list_source (compname, compoffset, 0);
          }

        word15 argSegno  = (word15) ((M[fp + 26] >> 18) & MASK15);
        word18 argOffset = (word18) ((M[fp + 27] >> 18) & MASK18);
        sim_msg ("Arg ptr     %05o:%06o\n", argSegno, argOffset);

        word24 ap;
        if (dbgLookupAddress (argSegno, argOffset, & ap, & msg))
          {
            sim_msg ("can't lookup arg ptr (%05o:%06o) because %s\n",
                    argSegno, argOffset, msg);
            goto skipArgs;
          }

        word16 argCount  = (word16) ((M[ap + 0] >> 19) & MASK17);
        word18 callType  = (word18) (M[ap + 0] & MASK18);
        word16 descCount = (word16) ((M[ap + 1] >> 19) & MASK17);
        sim_msg ("arg_count   %d\n", argCount);
        switch (callType)
          {
            case 0u:
              sim_msg ("call_type Quick internal call\n");
              break;
            case 4u:
              sim_msg ("call_type Inter-segment\n");
              break;
            case 8u:
              sim_msg ("call_type Enviroment pointer\n");
              break;
            default:
              sim_msg ("call_type Unknown (%o)\n", callType);
              goto skipArgs;
              }
        sim_msg ("desc_count  %d\n", descCount);

# if 0
        if (descCount)
          {
            // XXX walk descriptor and arg list together
          }
        else
# endif
          {
            for (uint argno = 0; argno < argCount; argno ++)
              {
                uint argnoos       = ap + 2 + argno * 2;
                word15 argnoSegno  = (word15) ((M[argnoos] >> 18) & MASK15);
                word18 argnoOffset = (word18) ((M[argnoos + 1] >> 18) & MASK18);
                word24 argnop;
                if (dbgLookupAddress (argnoSegno, argnoOffset, & argnop, & msg))
                  {
                    sim_msg ("can't lookup arg%d ptr (%05o:%06o) because %s\n",
                                argno, argSegno, argOffset, msg);
                    continue;
                  }
                word36 argv = M[argnop];
                sim_msg ("arg%d value   %05o:%06o[%08o] "
                            "%012"PRIo64" (%"PRIu64")\n",
                            argno, argSegno, argOffset, argnop, argv, argv);
                sim_msg ("\n");
             }
         }
skipArgs:;

        sim_msg ("End of frame %d\n\n", frameNo);

        if (prevfpSegno == 077777 && prevfpOffset == 1)
          break;
        fpSegno  = prevfpSegno;
        fpOffset = prevfpOffset;
      }
    return SCPE_OK;
  }

static t_stat list_source_at (UNUSED int32 arg, UNUSED const char *  buf)
  {
    // list seg:offset
    uint segno;
    uint offset;
    if (sscanf (buf, "%o:%o", & segno, & offset) != 2)
      return SCPE_ARG;
    char * compname;
    word18 compoffset;
    char * where = lookup_address ((word18) segno, offset,
                                   & compname, & compoffset);
    if (where)
      {
        sim_msg ("%05o:%06o %s\n", segno, offset, where);
        list_source (compname, compoffset, 0);
      }
    return SCPE_OK;
  }

static t_stat load_system_book (UNUSED int32 arg, UNUSED const char * buf)
  {
// Quietly ignore if not debug enabled
# ifndef SPEED
    // Multics 12.5 assigns segment number to collection 3 starting at 0244.
    uint c3 = 0244;

#  define bufSz 257
    char filebuf[bufSz];
    int  current = -1;

    FILE * fp = fopen (buf, "r");
    if (! fp)
      {
        sim_msg ("error opening file %s\n", buf);
        return SCPE_ARG;
      }
    for (;;)
      {
        char * bufp = fgets (filebuf, bufSz, fp);
        if (! bufp)
          break;
        //sim_msg ("<%s\n>", filebuf);
        char name[BOOK_SEGMENT_NAME_LEN];
        uint segno, p0, p1, p2;

        // 32 is BOOK_SEGMENT_NAME_LEN - 1
        int cnt = sscanf (filebuf, "%32s %o  (%o, %o, %o)", name, & segno,
          & p0, & p1, & p2);
        if (filebuf[0] != '\t' && cnt == 5)
          {
            //sim_msg ("A: %s %d\n", name, segno);
            int rc = add_book_segment (name, (int) segno);
            if (rc < 0)
              {
                sim_warn ("error adding segment name\n");
                fclose (fp);
                return SCPE_ARG;
              }
            continue;
          }
        else
          {
            // Check for collection 3 segment
            // 32 is BOOK_SEGMENT_NAME_LEN - 1
            cnt = sscanf (filebuf, "%32s  (%o, %o, %o)", name,
              & p0, & p1, & p2);
            if (filebuf[0] != '\t' && cnt == 4)
              {
                if (strstr (name, "fw.") || strstr (name, ".ec"))
                  continue;
                //sim_msg ("A: %s %d\n", name, segno);
                int rc = add_book_segment (name, (int) (c3 ++));
                if (rc < 0)
                  {
                    sim_warn ("error adding segment name\n");
                    fclose (fp);
                    return SCPE_ARG;
                  }
                continue;
              }
          }
        cnt = sscanf (filebuf, "Bindmap for >ldd>h>e>%32s", name);
        if (cnt != 1)
          cnt = sscanf (filebuf, "Bindmap for >ldd>hard>e>%32s", name);
        if (cnt == 1)
          {
            //sim_msg ("B: %s\n", name);
            //int rc = add_book_segment (name);
            int rc = lookup_book_segment (name);
            if (rc < 0)
              {
                // The collection 3.0 segments do not have segment numbers,
                // and the 1st digit of the 3-tuple is 1, not 0. Ignore
                // them for now.
                current = -1;
                continue;
                //sim_warn ("error adding segment name\n");
                //return SCPE_ARG;
              }
            current = rc;
            continue;
          }

        uint txt_start, txt_length;
        int intstat_start, intstat_length, symbol_start, symbol_length;
        cnt = sscanf (filebuf, "%32s %o %o %o %o %o %o", name, & txt_start,
                      & txt_length, & intstat_start, & intstat_length,
                      & symbol_start, & symbol_length);

        if (cnt == 7)
          {
            //sim_msg ("C: %s\n", name);
            if (current >= 0)
              {
                add_book_component (current, name, txt_start, txt_length,
                                    intstat_start, intstat_length, symbol_start,
                                    symbol_length);
              }
            continue;
          }

        cnt = sscanf (filebuf, "%32s %o  (%o, %o, %o)", name, & segno,
          & p0, & p1, & p2);
        if (filebuf[0] == '\t' && cnt == 5)
          {
            //sim_msg ("D: %s %d\n", name, segno);
            int rc = add_book_segment (name, (int) segno);
            if (rc < 0)
              {
                sim_warn ("error adding segment name\n");
                fclose (fp);
                return SCPE_ARG;
              }
            continue;
          }

      }
    fclose (fp);
#  if 0
    for (int i = 0; i < n_book_segments; i ++)
      {
        sim_msg ("  %-32s %6o\n", book_segments[i].segname,
                    book_segments[i].segno);
        for (int j = 0; j < n_book_components; j ++)
          {
            if (book_components[j].book_segment_number == i)
              {
                fprintf (stderr, "    %-32s %6o %6o %6o %6o %6o %6o\n",
                  book_components[j].compname,
                  book_components[j].txt_start,
                  book_components[j].txt_length,
                  book_components[j].intstat_start,
                  book_components[j].intstat_length,
                  book_components[j].symbol_start,
                  book_components[j].symbol_length);
              }
          }
      }
#  endif
# endif
    return SCPE_OK;
  }

static t_stat add_system_book_entry (UNUSED int32 arg, const char * buf)
  {
    // asbe segname compname seg txt_start txt_len intstat_start intstat_length
    // symbol_start symbol_length
    char segname[BOOK_SEGMENT_NAME_LEN];
    char compname[BOOK_SEGMENT_NAME_LEN];
    uint segno;
    uint txt_start, txt_len;
    uint  intstat_start, intstat_length;
    uint  symbol_start, symbol_length;

    // 32 is BOOK_SEGMENT_NAME_LEN - 1
    if (sscanf (buf, "%32s %32s %o %o %o %o %o %o %o",
                segname, compname, & segno,
                & txt_start, & txt_len, & intstat_start, & intstat_length,
                & symbol_start, & symbol_length) != 9)
      return SCPE_ARG;

    int idx = add_book_segment (segname, (int) segno);
    if (idx < 0)
      return SCPE_ARG;

    if (add_book_component (idx, compname, txt_start, txt_len, (int) intstat_start,
                           (int) intstat_length, (int) symbol_start,
                           (int) symbol_length) < 0)
      return SCPE_ARG;

    return SCPE_OK;
  }

// LSB n:n   given a segment number and offset, return a segment name,
//           component and offset in that component
//     sname:cname+offset
//           given a segment name, component name and offset, return
//           the segment number and offset

static t_stat lookup_system_book (UNUSED int32  arg, const char * buf)
  {
    char w1[strlen (buf)];
    char w2[strlen (buf)];
    char w3[strlen (buf)];
    long segno, offset;

    size_t colon = strcspn (buf, ":");
    if (buf[colon] != ':')
      return SCPE_ARG;

    strncpy (w1, buf, colon);
    w1[colon] = '\0';
    //sim_msg ("w1 <%s>\n", w1);

    size_t plus = strcspn (buf + colon + 1, "+");
    if (buf[colon + 1 + plus] == '+')
      {
        strncpy (w2, buf + colon + 1, plus);
        w2[plus] = '\0';
        strcpy (w3, buf + colon + 1 + plus + 1);
      }
    else
      {
        strcpy (w2, buf + colon + 1);
        strcpy (w3, "");
      }
    //sim_msg ("w1 <%s>\n", w1);
    //sim_msg ("w2 <%s>\n", w2);
    //sim_msg ("w3 <%s>\n", w3);

    char * end1;
    segno = strtol (w1, & end1, 8);
    char * end2;
    offset = strtol (w2, & end2, 8);

    if (* end1 == '\0' && * end2 == '\0' && * w3 == '\0')
      {
        // n:n
        char * ans = lookup_address ((word18) segno, (word18) offset, NULL, NULL);
        sim_warn ("%s\n", ans ? ans : "not found");
      }
    else
      {
        if (* w3)
          {
            char * end3;
            offset = strtol (w3, & end3, 8);
            if (* end3 != '\0')
              return SCPE_ARG;
          }
        else
          offset = 0;
        long comp_offset;
        int rc = lookup_system_book_name (w1, w2, & segno, & comp_offset);
        if (rc)
          {
            sim_warn ("not found\n");
            return SCPE_OK;
          }
        sim_msg ("0%o:0%o\n", (uint) segno, (uint) (comp_offset + offset));
        abs_addr_n  ((int) segno, (uint) (comp_offset + offset));
      }
/*
    if (sscanf (buf, "%o:%o", & segno, & offset) != 2)
      return SCPE_ARG;
    char * ans = lookup_address (segno, offset);
    sim_msg ("%s\n", ans ? ans : "not found");
*/
    return SCPE_OK;
  }

// Assumes unpaged DSBR

static sdw0_s *fetchSDW (word15 segno)
  {
    word36 SDWeven, SDWodd;

    core_read2 ((cpu.DSBR.ADDR + 2u * segno) & PAMASK, & SDWeven, & SDWodd,
                 __func__);

    // even word

    sdw0_s *SDW = & cpu._s;
    memset (SDW, 0, sizeof (cpu._s));

    SDW->ADDR   = (SDWeven >> 12) & 077777777;
    SDW->R1     = (SDWeven >> 9)  & 7;
    SDW->R2     = (SDWeven >> 6)  & 7;
    SDW->R3     = (SDWeven >> 3)  & 7;
    SDW->DF     = TSTBIT (SDWeven,  2);
    SDW->FC     = SDWeven & 3;

    // odd word
    SDW->BOUND  = (SDWodd >> 21) & 037777;
    SDW->R      = TSTBIT (SDWodd,  20);
    SDW->E      = TSTBIT (SDWodd,  19);
    SDW->W      = TSTBIT (SDWodd,  18);
    SDW->P      = TSTBIT (SDWodd,  17);
    SDW->U      = TSTBIT (SDWodd,  16);
    SDW->G      = TSTBIT (SDWodd,  15);
    SDW->C      = TSTBIT (SDWodd,  14);
    SDW->EB     = SDWodd & 037777;

    return SDW;
  }

static t_stat virtAddrN (uint address)
  {
    if (cpu.DSBR.U) {
        for(word15 segno = 0; 2u * segno < 16u * (cpu.DSBR.BND + 1u); segno += 1)
        {
            sdw0_s *s = fetchSDW(segno);
            if (address >= s -> ADDR && address < s -> ADDR + s -> BOUND * 16u)
              sim_msg ("  %06o:%06o\n", segno, address - s -> ADDR);
        }
    } else {
        for(word15 segno = 0;
            2u * segno < 16u * (cpu.DSBR.BND + 1u);
            segno += 512u)
        {
            word24 y1 = (2u * segno) % 1024u;
            word24 x1 = (2u * segno - y1) / 1024u;
            word36 PTWx1;
            core_read ((cpu.DSBR.ADDR + x1) & PAMASK, & PTWx1, __func__);

            ptw_s PTW1;
            PTW1.ADDR = GETHI(PTWx1);
            PTW1.U    = TSTBIT(PTWx1, 9);
            PTW1.M    = TSTBIT(PTWx1, 6);
            PTW1.DF   = TSTBIT(PTWx1, 2);
            PTW1.FC   = PTWx1 & 3;

            if (PTW1.DF == 0)
                continue;
            //sim_msg ("%06o  Addr %06o U %o M %o DF %o FC %o\n",
            //            segno, PTW1.ADDR, PTW1.U, PTW1.M, PTW1.DF, PTW1.FC);
            //sim_msg ("    Target segment page table\n");
            for (word15 tspt = 0; tspt < 512u; tspt ++)
            {
                word36 SDWeven, SDWodd;
                core_read2(((PTW1.ADDR << 6) + tspt * 2u) & PAMASK, & SDWeven,
                           & SDWodd, __func__);
                sdw0_s SDW0;
                // even word
                SDW0.ADDR  = (SDWeven >> 12) & PAMASK;
                SDW0.R1    = (SDWeven >> 9)  & 7u;
                SDW0.R2    = (SDWeven >> 6)  & 7u;
                SDW0.R3    = (SDWeven >> 3)  & 7u;
                SDW0.DF    = TSTBIT(SDWeven,   2);
                SDW0.FC    = SDWeven & 3u;

                // odd word
                SDW0.BOUND = (SDWodd >> 21) & 037777;
                SDW0.R     = TSTBIT(SDWodd,   20);
                SDW0.E     = TSTBIT(SDWodd,   19);
                SDW0.W     = TSTBIT(SDWodd,   18);
                SDW0.P     = TSTBIT(SDWodd,   17);
                SDW0.U     = TSTBIT(SDWodd,   16);
                SDW0.G     = TSTBIT(SDWodd,   15);
                SDW0.C     = TSTBIT(SDWodd,   14);
                SDW0.EB    = SDWodd & 037777;

                if (SDW0.DF == 0)
                    continue;
                //sim_msg ("    %06o Addr %06o %o,%o,%o F%o BOUND %06o "
                //          "%c%c%c%c%c\n",
                //          tspt, SDW0.ADDR, SDW0.R1, SDW0.R2, SDW0.R3, SDW0.F,
                //          SDW0.BOUND, SDW0.R ? 'R' : '.', SDW0.E ? 'E' : '.',
                //          SDW0.W ? 'W' : '.', SDW0.P ? 'P' : '.',
                //          SDW0.U ? 'U' : '.');
                if (SDW0.U == 0)
                {
                    for (word18 offset = 0;
                         offset < 16u * (SDW0.BOUND + 1u);
                         offset += 1024)
                    {
                        word24 y2 = offset % 1024;
                        word24 x2 = (offset - y2) / 1024;

                        // 10. Fetch the target segment PTW(x2) from
                        //     SDW(segno).ADDR + x2.

                        word36 PTWx2;
                        core_read ((SDW0.ADDR + x2) & PAMASK, & PTWx2, __func__);

                        ptw_s PTW2;
                        PTW2.ADDR = GETHI(PTWx2);
                        PTW2.U    = TSTBIT(PTWx2, 9);
                        PTW2.M    = TSTBIT(PTWx2, 6);
                        PTW2.DF   = TSTBIT(PTWx2, 2);
                        PTW2.FC   = PTWx2 & 3;

                        //sim_msg ("        %06o  Addr %06o U %o M %o F %o "
                        //            "FC %o\n",
                        //            offset, PTW2.ADDR, PTW2.U, PTW2.M, PTW2.F,
                        //            PTW2.FC);
                        if (address >= PTW2.ADDR + offset &&
                            address < PTW2.ADDR + offset + 1024)
                          sim_msg ("  %06o:%06o\n", tspt, (address - offset) - PTW2.ADDR);

                      }
                  }
                else
                  {
                    if (address >= SDW0.ADDR &&
                        address < SDW0.ADDR + SDW0.BOUND * 16u)
                      sim_msg ("  %06o:%06o\n", tspt, address - SDW0.ADDR);
                  }
            }
        }
    }

    return SCPE_OK;

  }

// VIRTUAL address

static t_stat virt_address (UNUSED int32 arg, const char * buf)
  {
    uint address;
    if (sscanf (buf, "%o", & address) != 1)
      return SCPE_ARG;
    return virtAddrN (address);
  }

// search path is path:path:path....

static t_stat set_search_path (UNUSED int32 arg, UNUSED const char * buf)
  {
// Quietly ignore if debugging not enabled
# ifndef SPEED
    if (source_search_path)
      free (source_search_path);
    source_search_path = strdup (buf);
# endif
    return SCPE_OK;
  }

// Hook for gdb
//
// The idea is that if you want to set a gdb breakpoint for a particularly
// complex condition, you can add a test for the condition to the emulator
// code and call brkbrk() when the condition is met; by doing a gdb
// 'b brkbrk', gdb will see when the condition is met.
//

t_stat brkbrk (UNUSED int32 arg, UNUSED const char *  buf)
  {
    //list_source (buf, 0);
    return SCPE_OK;
  }

// SBREAK segno:offset

static t_stat sbreak (int32 arg, const char * buf)
  {
    //printf (">> <%s>\n", buf);
    int segno, offset;
    int where;
    int cnt = sscanf (buf, "%o:%o%n", & segno, & offset, & where);
    if (cnt != 2)
      {
        return SCPE_ARG;
      }
    char reformatted[strlen (buf) + 20];
    sprintf (reformatted, "0%04o%06o%s", segno, offset, buf + where);
    //printf (">> <%s>\n", reformatted);
    t_stat rc = brk_cmd (arg, reformatted);
    return rc;
  }

# ifdef DVFDBG
static t_stat dfx1entry (UNUSED int32 arg, UNUSED const char * buf)
  {
// divide_fx1, divide_fx3
    sim_msg ("dfx1entry\n");
    sim_msg ("rA %012"PRIo64" (%llu)\n", rA, rA);
    sim_msg ("rQ %012"PRIo64" (%llu)\n", rQ, rQ);
    // Figure out the caller's text segment, according to pli_operators.
    // sp:tbp -> PR[6].SNR:046
    word24 pa;
    char * msg;
    if (dbgLookupAddress (cpu.PR[6].SNR, 046, & pa, & msg))
      {
        sim_msg ("text segment number lookup failed because %s\n", msg);
      }
    else
      {
        sim_msg ("text segno %012"PRIo64" (%llu)\n", M[pa], M[pa]);
      }
sim_msg ("%05o:%06o\n", cpu.PR[2].SNR, cpu.rX[0]);
//dbgStackTrace ();
    if (dbgLookupAddress (cpu.PR[2].SNR, cpu.rX[0], & pa, & msg))
      {
        sim_msg ("return address lookup failed because %s\n", msg);
      }
    else
      {
        sim_msg ("scale %012"PRIo64" (%llu)\n", M[pa], M[pa]);
      }
    if (dbgLookupAddress (cpu.PR[2].SNR, cpu.PR[2].WORDNO, & pa, & msg))
      {
        sim_msg ("divisor address lookup failed because %s\n", msg);
      }
    else
      {
        sim_msg ("divisor %012"PRIo64" (%llu)\n", M[pa], M[pa]);
      }
    return SCPE_OK;
  }

static t_stat dfx1exit (UNUSED int32 arg, UNUSED const char * buf)
  {
    sim_msg ("dfx1exit\n");
    sim_msg ("rA %012"PRIo64" (%llu)\n", rA, rA);
    sim_msg ("rQ %012"PRIo64" (%llu)\n", rQ, rQ);
    return SCPE_OK;
  }

static t_stat dv2scale (UNUSED int32 arg, UNUSED const char * buf)
  {
    sim_msg ("dv2scale\n");
    sim_msg ("rQ %012"PRIo64" (%llu)\n", rQ, rQ);
    return SCPE_OK;
  }

static t_stat dfx2entry (UNUSED int32 arg, UNUSED const char * buf)
  {
// divide_fx2
    sim_msg ("dfx2entry\n");
    sim_msg ("rA %012"PRIo64" (%llu)\n", rA, rA);
    sim_msg ("rQ %012"PRIo64" (%llu)\n", rQ, rQ);
    // Figure out the caller's text segment, according to pli_operators.
    // sp:tbp -> PR[6].SNR:046
    word24 pa;
    char * msg;
    if (dbgLookupAddress (cpu.PR[6].SNR, 046, & pa, & msg))
      {
        sim_msg ("text segment number lookup failed because %s\n", msg);
      }
    else
      {
        sim_msg ("text segno %012"PRIo64" (%llu)\n", M[pa], M[pa]);
      }
#  if 0
sim_msg ("%05o:%06o\n", cpu.PR[2].SNR, cpu.rX[0]);
//dbgStackTrace ();
    if (dbgLookupAddress (cpu.PR[2].SNR, cpu.rX[0], & pa, & msg))
      {
        sim_msg ("return address lookup failed because %s\n", msg);
      }
    else
      {
        sim_msg ("scale ptr %012"PRIo64" (%llu)\n", M[pa], M[pa]);
        if ((M[pa] & 077) == 043)
          {
            word15 segno = (M[pa] >> 18u) & MASK15;
            word18 offset = (M[pa + 1] >> 18u) & MASK18;
            word24 ipa;
            if (dbgLookupAddress (segno, offset, & ipa, & msg))
              {
                sim_msg ("divisor address lookup failed because %s\n", msg);
              }
            else
              {
                sim_msg ("scale %012"PRIo64" (%llu)\n", M[ipa], M[ipa]);
              }
          }
      }
#  endif
    if (dbgLookupAddress (cpu.PR[2].SNR, cpu.PR[2].WORDNO, & pa, & msg))
      {
        sim_msg ("divisor address lookup failed because %s\n", msg);
      }
    else
      {
        sim_msg ("divisor %012"PRIo64" (%llu)\n", M[pa], M[pa]);
        sim_msg ("divisor %012"PRIo64" (%llu)\n", M[pa + 1], M[pa + 1]);
      }
    return SCPE_OK;
  }

static t_stat mdfx3entry (UNUSED int32 arg, UNUSED const char * buf)
  {
// operator to form mod(fx2,fx1)
// entered with first arg in q, bp pointing at second

// divide_fx1, divide_fx2
    sim_msg ("mdfx3entry\n");
    //sim_msg ("rA %012"PRIo64" (%llu)\n", rA, rA);
    sim_msg ("rQ %012"PRIo64" (%llu)\n", rQ, rQ);
    // Figure out the caller's text segment, according to pli_operators.
    // sp:tbp -> PR[6].SNR:046
    word24 pa;
    char * msg;
    if (dbgLookupAddress (cpu.PR[6].SNR, 046, & pa, & msg))
      {
        sim_msg ("text segment number lookup failed because %s\n", msg);
      }
    else
      {
        sim_msg ("text segno %012"PRIo64" (%llu)\n", M[pa], M[pa]);
      }
//sim_msg ("%05o:%06o\n", cpu.PR[2].SNR, cpu.rX[0]);
//dbgStackTrace ();
#  if 0
    if (dbgLookupAddress (cpu.PR[2].SNR, cpu.rX[0], & pa, & msg))
      {
        sim_msg ("return address lookup failed because %s\n", msg);
      }
    else
      {
        sim_msg ("scale %012"PRIo64" (%llu)\n", M[pa], M[pa]);
      }
#  endif
    if (dbgLookupAddress (cpu.PR[2].SNR, cpu.PR[2].WORDNO, & pa, & msg))
      {
        sim_msg ("divisor address lookup failed because %s\n", msg);
      }
    else
      {
        sim_msg ("divisor %012"PRIo64" (%llu)\n", M[pa], M[pa]);
      }
    return SCPE_OK;
  }

static t_stat smfx1entry (UNUSED int32 arg, UNUSED const char * buf)
  {
// operator to form mod(fx2,fx1)
// entered with first arg in q, bp pointing at second

// divide_fx1, divide_fx2
    sim_msg ("smfx1entry\n");
    //sim_msg ("rA %012"PRIo64" (%llu)\n", rA, rA);
    sim_msg ("rQ %012"PRIo64" (%llu)\n", rQ, rQ);
    // Figure out the caller's text segment, according to pli_operators.
    // sp:tbp -> PR[6].SNR:046
    word24 pa;
    char * msg;
    if (dbgLookupAddress (cpu.PR[6].SNR, 046, & pa, & msg))
      {
        sim_msg ("text segment number lookup failed because %s\n", msg);
      }
    else
      {
        sim_msg ("text segno %012"PRIo64" (%llu)\n", M[pa], M[pa]);
      }
sim_msg ("%05o:%06o\n", cpu.PR[2].SNR, cpu.rX[0]);
//dbgStackTrace ();
    if (dbgLookupAddress (cpu.PR[2].SNR, cpu.rX[0], & pa, & msg))
      {
        sim_msg ("return address lookup failed because %s\n", msg);
      }
    else
      {
        sim_msg ("scale %012"PRIo64" (%llu)\n", M[pa], M[pa]);
      }
    if (dbgLookupAddress (cpu.PR[2].SNR, cpu.PR[2].WORDNO, & pa, & msg))
      {
        sim_msg ("divisor address lookup failed because %s\n", msg);
      }
    else
      {
        sim_msg ("divisor %012"PRIo64" (%llu)\n", M[pa], M[pa]);
      }
    return SCPE_OK;
  }
# endif // DVFDBG

// SEARCHMEMORY value

static t_stat search_memory (UNUSED int32 arg, const char * buf)
  {
    word36 value;
    if (sscanf (buf, "%"PRIo64"", & value) != 1)
      return SCPE_ARG;

    uint i;
    for (i = 0; i < MEMSIZE; i ++)
      if ((M[i] & DMASK) == value)
        sim_msg ("%08o\n", i);
    return SCPE_OK;
  }

static t_stat set_dbg_cpu_mask (int32 UNUSED arg, const char * UNUSED buf)
  {
    uint msk;
    int cnt = sscanf (buf, "%u", & msk);
    if (cnt != 1)
      {
        sim_msg ("Huh?\n");
        return SCPE_ARG;
      }
    sim_msg ("mask set to %u\n", msk);
    dbgCPUMask = msk;
    return SCPE_OK;
  }

#endif // TESTING

//
// Misc. commands
//

#ifdef PANEL68
static t_stat scraper (UNUSED int32 arg, const char * buf)
  {
    if (strcasecmp (buf, "start") == 0)
      return panelScraperStart ();
    if (strcasecmp (buf, "stop") == 0)
      return panelScraperStop ();
    if (strcasecmp (buf, "msg") == 0)
      {
        return panelScraperMsg (NULL);
      }
    if (strncasecmp (buf, "msg ", 4) == 0)
      {
        const char * p = buf + 4;
        while (* p == ' ')
          p ++;
        return panelScraperMsg (p);
      }
    sim_msg ("err: scraper start|stop|msg\n");
    return SCPE_ARG;
  }
#endif

#ifdef YIELD
static t_stat clear_yield (int32 flag, UNUSED const char * cptr)
  {
    return SCPE_OK;
  }

static t_stat yield (int32 flag, UNUSED const char * cptr)
  {
    return SCPE_OK;
  }
#endif

#ifndef PERF_STRIP
static t_stat set_luf (int32 flag, UNUSED const char * cptr)
  {
    luf_flag = flag;
    return SCPE_OK;
  }
#endif /* ifndef PERF_STRIP */

#ifdef DBGEVENT
uint n_dbgevents;
struct dbgevent_t dbgevents[max_dbgevents];
struct timespec dbgevent_t0;

static int dbgevent_compar (const void * a, const void * b)
  {
    struct dbgevent_t * ea = (struct dbgevent_t *) a;
    struct dbgevent_t * eb = (struct dbgevent_t *) b;
    if (ea->segno < eb->segno)
      return -1;
    if (ea->segno > eb->segno)
      return 1;
    if (ea->offset < eb->offset)
      return -1;
    if (ea->offset > eb->offset)
      return 1;
    return 0;
  }

int dbgevent_lookup (word15 segno, word18 offset)
  {
    struct dbgevent_t key = {segno, offset, false};
    struct dbgevent_t * p = (struct dbgevent_t *) bsearch (& key, dbgevents, (size_t) n_dbgevents,
            sizeof (struct dbgevent_t), dbgevent_compar);
    if (! p)
      return -1;
    return (int) (p - dbgevents);
  }

// "dbbevent segno:offset"
//
// arg: 0 set t0 event
//      1 set event
//      2 clear event
//      3 list events
//      4 clear all events

// XXX think about per-thread timing?

static t_stat set_dbgevent (int32 arg, const char * buf)
  {
    if (arg == 0 || arg == 1)
      {
        if (n_dbgevents >= max_dbgevents)
          {
            sim_printf ("too many dbgevents %u/%u\r\n", n_dbgevents, max_dbgevents);
            return SCPE_ARG;
          }
        if (strlen (buf) > dbgevent_tagsize - 1)
          {
            sim_printf ("command too long %lu/%u\r\n", strlen (buf), dbgevent_tagsize -1);
            return SCPE_ARG;
          }

        uint segno;
        uint offset;
        if (sscanf (buf, "%o:%o", & segno, & offset) != 2)
          return SCPE_ARG;
        if (segno > MASK15 || offset > MASK18)
          return SCPE_ARG;
        if (dbgevent_lookup ((word15) segno, (word18) offset) != -1)
          {
            sim_printf ("not adding duplicate 0%o:0%o\r\n", segno, offset);
            return SCPE_ARG;
          }
        dbgevents[n_dbgevents].segno                     = (word15) segno;
        dbgevents[n_dbgevents].offset                    = (word18) offset;
        dbgevents[n_dbgevents].t0                        = arg == 0;
        strncpy (dbgevents[n_dbgevents].tag, buf, dbgevent_tagsize - 1);
        dbgevents[n_dbgevents].tag[dbgevent_tagsize - 1] = 0;
        sim_printf ("%o:%o %u(%d) %s\r\n", dbgevents[n_dbgevents].segno,
            dbgevents[n_dbgevents].offset,
                dbgevents[n_dbgevents].t0, arg, dbgevents[n_dbgevents].tag);
        n_dbgevents ++;
        qsort (dbgevents, n_dbgevents, sizeof (struct dbgevent_t), dbgevent_compar);
      }
    else if (arg == 2)
      {
        uint segno;
        uint offset;
        if (sscanf (buf, "%o:%o", & segno, & offset) != 2)
          return SCPE_ARG;
        int n = dbgevent_lookup ((word15) segno, (word18) offset);
        if (n < 0)
          {
            sim_printf ("0%o:0%o not found\r\n", segno, offset);
            return SCPE_ARG;
          }
        for (int i = n; i < n_dbgevents - 1; i ++)
          dbgevents[i] = dbgevents[i + 1];
        n_dbgevents --;
      }
    else if (arg == 3)
      {
        for (int i = 0; i < n_dbgevents; i ++)
         sim_printf ("    %s %05o:%06o %s\r\n", dbgevents[i].t0 ? "T0" : "  ", dbgevents[i].segno,
                 dbgevents[i].offset,dbgevents[i].tag);
      }
    else if (arg == 4)
      {
        n_dbgevents = 0;
        sim_printf ("dbgevents cleared\r\n");
      }
    else
      {
        sim_printf ("set_dbgevent bad arg %d\r\n", arg);
        return SCPE_ARG;
      }
    return SCPE_OK;
  }
#endif

//  REWIND name
//
//  rewind tapa_05
//

t_stat rewind_media (int32 arg, const char * buf) {
  char name[strlen (buf)];

  int rc = sscanf (buf, "%s", name);
  if (rc != 1)
    return SCPE_ARG;

  for (uint i = 0; i < N_MT_UNITS_MAX; i ++) {
    if (strcmp (tape_states[i].device_name, name) == 0) {
      UNIT * unitp = & mt_unit [i];
      return sim_tape_rewind (unitp);
    }
  }

  sim_printf ("Can't find name '%s'\r\n", name);
  sim_printf ("REWIND device_name\r\n");
  return SCPE_ARG;
}

// [UN]LOAD  name  image_name ro|rw
//
//  load tapea_05  data.tap ro
//
//  load diskb_01  data.dsk rw
//

t_stat load_media (int32 arg, const char * buf)
  {
    // arg 1: load
    //     0: unload

    char  name[strlen (buf)];
    char fname[strlen (buf)];
    char  perm[strlen (buf)];
    bool ro = false;
    if (arg)
      {
        int rc = sscanf (buf, "%s %s %s", name, fname, perm);
        if (rc != 3)
          return SCPE_ARG;
        if (strcasecmp (perm, "rw") == 0)
          ro = false;
        else if (strcasecmp (perm, "ro") == 0)
          ro = true;
        else
          {
             sim_print ("'%s' not 'ro' or 'rw'\r\n", perm);
             goto usage;
          }
      }
    else
      {
        int rc = sscanf (buf, "%s", name);
        if (rc != 1)
          return SCPE_ARG;
      }

    for (uint i = 0; i < N_DSK_UNITS_MAX; i ++)
      if (strcmp (dsk_states[i].device_name, name) == 0)
        {
          if (arg)
            return loadDisk (i, fname, ro);
          return unloadDisk (i);
        }

    for (uint i = 0; i < N_MT_UNITS_MAX; i ++)
      if (strcmp (tape_states[i].device_name, name) == 0)
        {
          if (arg)
            return loadTape (i, fname, ro);
          return unloadTape (i);
        }

    sim_printf ("Can't find name '%s'\r\n", name);
usage:
    sim_printf ("[UN]LOAD device_name image_name ro|rw\r\n");
    return SCPE_ARG;
  }

t_stat ready_media (int32 arg, const char * buf) {
  char name[strlen (buf)];
  int rc = sscanf (buf, "%s", name);
  if (rc != 1)
    return SCPE_ARG;

  for (uint i = 0; i < N_DSK_UNITS_MAX; i ++) {
    if (strcmp (dsk_states[i].device_name, name) == 0) {
      return signal_disk_ready (i);
    }
  }

  for (uint i = 0; i < N_MT_UNITS_MAX; i ++) {
    if (strcmp (tape_states[i].device_name, name) == 0) {
      return signal_tape (i, 0, 020 /* tape drive to ready */);
    }
  }

  sim_printf ("Can't find name '%s'\r\n", name);
  sim_printf ("[UN]LOAD device_name image_name ro|rw\r\n");
  return SCPE_ARG;
}

////////////////////////////////////////////////////////////////////////////////
//
// s*mh Command table
//

#ifdef TESTING
# include "tracker.h"

static t_stat trkw (UNUSED int32 arg, const char * buf)
  {
    trk_init (true);
    return SCPE_OK;
  }

static t_stat trkr (UNUSED int32 arg, const char * buf)
  {
    trk_init (false);
    return SCPE_OK;
  }
#endif

#ifndef PERF_STRIP
static CTAB dps8_cmds[] =
  {

//
// System configuration
//

    {"DEFAULT_BASE_SYSTEM", set_default_base_system,  0, "Set configuration to defaults\n",             NULL, NULL },

    {"CABLE",               sys_cable,                0, "String a cable\n",                            NULL, NULL },
    {"UNCABLE",             sys_cable,                1, "Unstring a cable\n",                          NULL, NULL },
    {"CABLE_RIPOUT",        sys_cable_ripout,         0, "Unstring all cables\n",                       NULL, NULL },
    {"CABLE_SHOW",          sys_cable_show,           0, "Show cables\n",                               NULL, NULL },

    {"FNPSERVERPORT",       set_fnp_server_port,      0, "Set the FNP dialin TELNET port number\n",     NULL, NULL },
    {"FNPSERVERADDRESS",    set_fnp_server_address,   0, "Set the FNP dialin server binding address\n", NULL, NULL },
    {"FNPSERVER3270PORT",   set_fnp_3270_server_port, 0, "Set the FNP TN3270 dialin port number\n",     NULL, NULL },

//
// System control
//

    {"SKIPBOOT",  boot_skip,                     0, "Skip forward on boot tape\n",                        NULL, NULL },
    {"FNPSTART",  fnp_start,                     0, "Force an immediate FNP initialization\n",            NULL, NULL },
    {"MOUNT",     mount_tape,                    0, "Mount tape image and signal Multics\n",              NULL, NULL },
    {"LOAD",      load_media,                    1, "Mount disk or tape image and signal Multics\n",      NULL, NULL },
    {"UNLOAD",    load_media,                    0, "Unmount disk or tape image and signal Multics\n",    NULL, NULL },
    {"READY",     ready_media,                   0, "Signal Multics that media is ready\n",               NULL, NULL },
    {"REWIND",    rewind_media,                  0, "Rewind tape\n",                                      NULL, NULL },
    {"XF",        do_execute_fault,              0, "Execute fault: Press the execute fault button\n",    NULL, NULL },
    {"RESTART",   do_restart,                    0, "Execute fault: Press the execute fault button\n",    NULL, NULL },
    {"POLL",      set_sys_polling_interval,      0, "Set polling interval (in milliseconds)\n",           NULL, NULL },
    {"SLOWPOLL",  set_sys_slow_polling_interval, 0, "Set slow polling interval (in polling intervals)\n", NULL, NULL },
    {"CHECKPOLL", set_sys_poll_check_rate,       0, "Set polling check rate (in polling intervals)\n",    NULL, NULL },
    {"BURST",     burst_printer,                 0, "Burst process output from printer\n",                NULL, NULL },

//
// Debugging
//

# ifdef TESTING
    {"TRKW",               trkw,                  0, "Start tracking to track.dat\n",                            NULL, NULL},
    {"TRKR",               trkr,                  0, "Start comparing with track.dat\n",                         NULL, NULL},
    {"DBGMMECNTDWN",       dps_debug_mme_cntdwn,  0, "Enable debug after n MMEs\n",                              NULL, NULL},
    {"DBGSKIP",            dps_debug_skip,        0, "Skip first n TRACE debugs\n",                              NULL, NULL},
    {"DBGSTART",           dps_debug_start,       0, "Limit debugging to N > Cycle count\n",                     NULL, NULL},
    {"DBGSTOP",            dps_debug_stop,        0, "Limit debugging to N < Cycle count\n",                     NULL, NULL},
    {"DBGBREAK",           dps_debug_break,       0, "Break when N >= Cycle count\n",                            NULL, NULL},
    {"DBGSEGNO",           dps_debug_segno,       1, "Limit debugging to PSR == segno\n",                        NULL, NULL},
    {"NODBGSEGNO",         dps_debug_segno,       0, "Reset to debugging all segments\n",                        NULL, NULL},
    {"DBGRINGNO",          dps_debug_ringno,      0, "Limit debugging to PRR == ringno\n",                       NULL, NULL},
    {"DBGBAR",             dps_debug_bar,         1, "Limit debugging to BAR mode\n",                            NULL, NULL},
    {"NODBGBAR",           dps_debug_bar,         0, "Limit debugging to BAR mode\n",                            NULL, NULL},
    {"HDBG",               hdbg_size,             0, "Set history debugger buffer size\n",                       NULL, NULL},
    {"HDSEG",              hdbgSegmentNumber,     0, "Set history debugger segment number\n",                    NULL, NULL},
    {"HDBL",               hdbgBlacklist,         0, "Set history debugger blacklist\n",                         NULL, NULL},
    {"PHDBG",              hdbg_print,            0, "Display history size\n",                                   NULL, NULL},
    {"HDBG_CPU_MASK",      hdbg_cpu_mask,         0, "Set which CPUs to track (by mask)\n",                      NULL, NULL},
    {"ABSOLUTE",           abs_addr,              0, "Compute the absolute address of segno:offset\n",           NULL, NULL},
    {"STK",                stack_trace,           0, "Print a stack trace\n",                                    NULL, NULL},
    {"LIST",               list_source_at,        0, "List source for address / segno:offset\n",                 NULL, NULL},
    {"LD_SYSTEM_BOOK",     load_system_book,      0, "Load a Multics system book for symbolic debugging\n",      NULL, NULL},
    {"ASBE",               add_system_book_entry, 0, "Add an entry to the system book\n",                        NULL, NULL},
    {"LOOKUP_SYSTEM_BOOK", lookup_system_book,    0, "Lookup an address or symbol in the Multics system book\n", NULL, NULL},
    {"LSB",                lookup_system_book,    0, "Lookup an address or symbol in the Multics system book\n", NULL, NULL},
    {"VIRTUAL",            virt_address,          0, "Compute the virtual address(es) of segno:offset\n",        NULL, NULL},
    {"SPATH",              set_search_path,       0, "Set source code search path\n",                            NULL, NULL},
    {"TEST",               brkbrk,                0, "GDB test hook\n",                                          NULL, NULL},
#  ifdef DBGEVENT
    {"DBG0EVENT",          set_dbgevent,          0, "Set t0 debug event\n",                                     NULL, NULL},
    {"DBGEVENT",           set_dbgevent,          1, "Set debug event\n",                                        NULL, NULL},
    {"DBGNOEVENT",         set_dbgevent,          2, "Clear debug event\n",                                      NULL, NULL},
    {"DBGLISTEVENTS",      set_dbgevent,          3, "List debug events\n",                                      NULL, NULL},
    {"DBGCLEAREVENTS",     set_dbgevent,          4, "Clear debug events\n",                                     NULL, NULL},
#  endif

// copied from scp.c
#  define SSH_ST 0        /* set */
#  define SSH_SH 1        /* show */
#  define SSH_CL 2        /* clear */
    {"SBREAK",       sbreak,           SSH_ST, "Set a breakpoint with segno:offset syntax\n", NULL, NULL},
    {"NOSBREAK",     sbreak,           SSH_CL, "Unset an SBREAK\n",                           NULL, NULL},
#  ifdef DVFDBG
    // dvf debugging
    {"DFX1ENTRY",    dfx1entry,        0,      "\n",                                          NULL, NULL},
    {"DFX2ENTRY",    dfx2entry,        0,      "\n",                                          NULL, NULL},
    {"DFX1EXIT",     dfx1exit,         0,      "\n",                                          NULL, NULL},
    {"DV2SCALE",     dv2scale,         0,      "\n",                                          NULL, NULL},
    {"MDFX3ENTRY",   mdfx3entry,       0,      "\n",                                          NULL, NULL},
    {"SMFX1ENTRY",   smfx1entry,       0,      "\n",                                          NULL, NULL},
#  endif
    // doesn't work
    //{"DUMPKST",             dumpKST,                  0, "dumpkst: dump the Known Segment Table\n", NULL},
#  ifndef SPEED
    {"WATCH",        set_mem_watch,    1,      "Watch memory location\n",                     NULL, NULL},
    {"NOWATCH",      set_mem_watch,    0,      "Unwatch memory location\n",                   NULL, NULL},
#  endif
    {"SEARCHMEMORY", search_memory,    0,      "Search memory for value\n",                   NULL, NULL},
    {"DBGCPUMASK",   set_dbg_cpu_mask, 0,      "Set per-CPU debug enable mask\n",             NULL, NULL},
# endif // TESTING

    {"SEGLDR",       segment_loader,   0,      "Segment Loader\n",                            NULL, NULL},

//
// Statistics
//

# ifdef MATRIX
    {"DISPLAYMATRIX", display_the_matrix,  0, "Display instruction usage counts\n", NULL, NULL},
# endif

//
// Console scripting
//

    {"AUTOINPUT",     add_opc_autoinput,   0, "Set console auto-input\n",           NULL, NULL},
    {"AI",            add_opc_autoinput,   0, "Set console auto-input\n",           NULL, NULL},
    {"AUTOINPUT2",    add_opc_autoinput,   1, "Set CPU-B console auto-input\n",     NULL, NULL},
    {"AI2",           add_opc_autoinput,   1, "Set console CPU-B auto-input\n",     NULL, NULL},
    {"CLRAUTOINPUT",  clear_opc_autoinput, 0, "Clear console auto-input\n",         NULL, NULL},
    {"CLRAUTOINPUT2", clear_opc_autoinput, 1, "Clear CPU-B console auto-input\n",   NULL, NULL},

//
// Tuning
//

# if YIELD
    {"CLEAR_YIELD",   clear_yield,         1, "Clear yield data points\n",          NULL, NULL},
    {"YIELD",         yield,               1, "Define yield data point\n",          NULL, NULL},
# endif

//
// Hacks
//

    {"LUF",           set_luf,             1, "Enable normal LUF handling\n",       NULL, NULL},
    {"NOLUF",         set_luf,             0, "Disable normal LUF handling\n",      NULL, NULL},

//
// Misc.
//

# ifdef PANEL68
    {"SCRAPER",       scraper,             0, "Control panel scraper\n", NULL, NULL},
# endif
    { NULL,           NULL,                0, NULL,                      NULL, NULL}
  }; // dps8_cmds

# ifndef __MINGW64__
#  ifndef __MINGW32__
#   ifndef CROSS_MINGW64
#    ifndef CROSS_MINGW32
#     ifndef PERF_STRIP
static void usr1_signal_handler (UNUSED int sig)
  {
    sim_msg ("USR1 signal caught; pressing the EXF button\n");
    // Assume the bootload CPU
    setG7fault (ASSUME0, FAULT_EXF, fst_zero);
    return;
  }
#     endif /* ifndef PERF_STRIP */
#    endif /* ifndef CROSS_MINGW32 */
#   endif /* ifndef CROSS_MINGW64 */
#  endif /* ifndef __MINGW32__ */
# endif /* ifndef __MINGW64__ */

static struct symbol_s symbols [] = {
    { "commit_id",              SYM_STATE_OFFSET,  SYM_STRING,    offsetof (struct system_state_s, commit_id) },
    { "M[]",                    SYM_STATE_OFFSET,  SYM_ARRAY,     offsetof (struct system_state_s, M) },
    { "sizeof(*M)",             SYM_STRUCT_SZ,     SYM_SZ,        sizeof (word36) },

    { "cpus[]",                 SYM_STATE_OFFSET,  SYM_ARRAY,     offsetof (struct system_state_s, cpus) },
    { "sizeof(*cpus)",          SYM_STRUCT_SZ,     SYM_SZ,        sizeof (cpu_state_t) },

    { "cpus[].PPR",             SYM_STRUCT_OFFSET, SYM_PTR,       offsetof (cpu_state_t,           PPR) },
    { "cpus[].PPR.PRR",         SYM_STRUCT_OFFSET, SYM_UINT8_3,   offsetof (struct ppr_s,          PRR) },
    { "cpus[].PPR.PSR",         SYM_STRUCT_OFFSET, SYM_UINT16_15, offsetof (struct ppr_s,          PSR) },
    { "cpus[].PPR.P",           SYM_STRUCT_OFFSET, SYM_UINT8_1,   offsetof (struct ppr_s,          P) },
    { "cpus[].PPR.IC",          SYM_STRUCT_OFFSET, SYM_UINT32_18, offsetof (struct ppr_s,          IC) },

    { "cpus[].cu",              SYM_STRUCT_OFFSET, SYM_PTR,       offsetof (cpu_state_t,           cu) },
    { "cpus[].cu.IWB",          SYM_STRUCT_OFFSET, SYM_UINT64_36, offsetof (ctl_unit_data_t,       IWB) },
    { "cpus[].cu.IR",           SYM_STRUCT_OFFSET, SYM_UINT32_18, offsetof (ctl_unit_data_t,       IR) },

    { "cpus[].rA",              SYM_STRUCT_OFFSET, SYM_UINT64_36, offsetof (cpu_state_t,           rA) },

    { "cpus[].rQ",              SYM_STRUCT_OFFSET, SYM_UINT64_36, offsetof (cpu_state_t,           rQ) },

    { "cpus[].rE",              SYM_STRUCT_OFFSET, SYM_UINT64_36, offsetof (cpu_state_t,           rE) },

    { "cpus[].rX[]",            SYM_STRUCT_OFFSET, SYM_ARRAY,     offsetof (cpu_state_t,           rX) },
    { "sizeof(*rX)",            SYM_STRUCT_SZ,     SYM_SZ,        sizeof (word18) },
    { "cpus[].rX",              SYM_STRUCT_OFFSET, SYM_UINT32_18, 0 },

    { "cpus[].rTR",             SYM_STRUCT_OFFSET, SYM_UINT32_27, offsetof (cpu_state_t,           rTR) },

    { "cpus[].rRALR",           SYM_STRUCT_OFFSET, SYM_UINT8_3,   offsetof (cpu_state_t,           rRALR) },

    { "cpus[].PAR[]",           SYM_STRUCT_OFFSET, SYM_ARRAY,     offsetof (cpu_state_t,           PAR) },
    { "sizeof(*PAR)",           SYM_STRUCT_SZ,     SYM_SZ,        sizeof (struct par_s) },

    { "cpus[].PAR[].SNR",       SYM_STRUCT_OFFSET, SYM_UINT16_15, offsetof (struct par_s,          SNR) },
    { "cpus[].PAR[].RNR",       SYM_STRUCT_OFFSET, SYM_UINT8_3,   offsetof (struct par_s,          RNR) },
    { "cpus[].PAR[].PR_BITNO",  SYM_STRUCT_OFFSET, SYM_UINT8_6,   offsetof (struct par_s,          PR_BITNO) },
    { "cpus[].PAR[].WORDNO",    SYM_STRUCT_OFFSET, SYM_UINT32_18, offsetof (struct par_s,          WORDNO) },

    { "cpus[].BAR",             SYM_STRUCT_OFFSET, SYM_PTR,       offsetof (cpu_state_t,           BAR) },
    { "cpus[].BAR.BASE",        SYM_STRUCT_OFFSET, SYM_UINT16_9,  offsetof (struct bar_s,          BASE) },
    { "cpus[].BAR.BOUND",       SYM_STRUCT_OFFSET, SYM_UINT16_9,  offsetof (struct bar_s,          BOUND) },

    { "cpus[].TPR",             SYM_STRUCT_OFFSET, SYM_PTR,       offsetof (cpu_state_t,           TPR) },
    { "cpus[].TPR.TRR",         SYM_STRUCT_OFFSET, SYM_UINT8_3,   offsetof (struct tpr_s,          TRR) },
    { "cpus[].TPR.TSR",         SYM_STRUCT_OFFSET, SYM_UINT16_15, offsetof (struct tpr_s,          TSR) },
    { "cpus[].TPR.TBR",         SYM_STRUCT_OFFSET, SYM_UINT8_6,   offsetof (struct tpr_s,          TBR) },
    { "cpus[].TPR.CA",          SYM_STRUCT_OFFSET, SYM_UINT32_18, offsetof (struct tpr_s,          CA) },

    { "cpus[].DSBR",            SYM_STRUCT_OFFSET, SYM_PTR,       offsetof (cpu_state_t,           DSBR) },
    { "cpus[].DSBR.ADDR",       SYM_STRUCT_OFFSET, SYM_UINT32_24, offsetof (struct dsbr_s,         ADDR) },
    { "cpus[].DSBR.BND",        SYM_STRUCT_OFFSET, SYM_UINT16_14, offsetof (struct dsbr_s,         BND) },
    { "cpus[].DSBR.U",          SYM_STRUCT_OFFSET, SYM_UINT8_1,   offsetof (struct dsbr_s,         U) },
    { "cpus[].DSBR.STACK",      SYM_STRUCT_OFFSET, SYM_UINT16_12, offsetof (struct dsbr_s,         STACK) },

    { "cpus[].faultNumber",     SYM_STRUCT_OFFSET, SYM_UINT32,    offsetof (cpu_state_t,           faultNumber) },
# define SYMTAB_ENUM32(e) { #e,  SYM_ENUM,          SYM_UINT32,    e }
    SYMTAB_ENUM32 (FAULT_SDF),
    SYMTAB_ENUM32 (FAULT_STR),
    SYMTAB_ENUM32 (FAULT_MME),
    SYMTAB_ENUM32 (FAULT_F1),
    SYMTAB_ENUM32 (FAULT_TRO),
    SYMTAB_ENUM32 (FAULT_CMD),
    SYMTAB_ENUM32 (FAULT_DRL),
    SYMTAB_ENUM32 (FAULT_LUF),
    SYMTAB_ENUM32 (FAULT_CON),
    SYMTAB_ENUM32 (FAULT_PAR),
    SYMTAB_ENUM32 (FAULT_IPR),
    SYMTAB_ENUM32 (FAULT_ONC),
    SYMTAB_ENUM32 (FAULT_SUF),
    SYMTAB_ENUM32 (FAULT_OFL),
    SYMTAB_ENUM32 (FAULT_DIV),
    SYMTAB_ENUM32 (FAULT_EXF),
    SYMTAB_ENUM32 (FAULT_DF0),
    SYMTAB_ENUM32 (FAULT_DF1),
    SYMTAB_ENUM32 (FAULT_DF2),
    SYMTAB_ENUM32 (FAULT_DF3),
    SYMTAB_ENUM32 (FAULT_ACV),
    SYMTAB_ENUM32 (FAULT_MME2),
    SYMTAB_ENUM32 (FAULT_MME3),
    SYMTAB_ENUM32 (FAULT_MME4),
    SYMTAB_ENUM32 (FAULT_F2),
    SYMTAB_ENUM32 (FAULT_F3),
    SYMTAB_ENUM32 (FAULT_UN1),
    SYMTAB_ENUM32 (FAULT_UN2),
    SYMTAB_ENUM32 (FAULT_UN3),
    SYMTAB_ENUM32 (FAULT_UN4),
    SYMTAB_ENUM32 (FAULT_UN5),
    SYMTAB_ENUM32 (FAULT_TRB),

    { "",                       SYM_EMPTY,         SYM_UNDEF,     0 },
  };

static void systabInit (void) {
  strncpy (system_state->symbolTable.symtabHdr, SYMTAB_HDR, sizeof (system_state->symbolTable.symtabHdr));
  system_state->symbolTable.symtabVer = SYMTAB_VER;
  memcpy (system_state->symbolTable.symbols, symbols, sizeof (symbols));
}
#endif /* ifndef PERF_STRIP */

// Once-only initialization; invoked via SCP

static void dps8_init (void) {
  fflush(stderr); fflush(stdout);
#ifndef PERF_STRIP
  if (!sim_quiet) {
# if defined(GENERATED_MAKE_VER_H) && defined(VER_H_GIT_VERSION)
#  if defined(VER_H_GIT_PATCH_INT) && defined(VER_H_GIT_PATCH)
#   if VER_H_GIT_PATCH_INT < 1
    sim_msg ("%s simulator %s (%ld-bit)",
             sim_name, VER_H_GIT_VERSION,
             (long)(CHAR_BIT*sizeof(void *)));
#   else
    sim_msg ("%s simulator %s+%s (%ld-bit)",
             sim_name, VER_H_GIT_VERSION, VER_H_GIT_PATCH,
             (long)(CHAR_BIT*sizeof(void *)));
#   endif
#  else
    sim_msg ("%s simulator %s (%ld-bit)",
             sim_name, VER_H_GIT_VERSION,
             (long)(CHAR_BIT*sizeof(void *)));
#  endif
# endif
# if !defined(VER_H_GIT_VERSION) || !defined(GENERATED_MAKE_VER_H)
    sim_msg ("%s simulator (%ld-bit)",
             sim_name, (long)(CHAR_BIT*sizeof(void *)));
# endif
# ifdef TESTING
    sim_msg ("\n Options: ");
#  ifndef HAVE_DPSOPT
#   define HAVE_DPSOPT 1
#  endif
    sim_msg ("TESTING");
# endif
# ifdef NEED_128
#  ifdef HAVE_DPSOPT
    sim_msg (", ");
#  else
    sim_msg ("\n Options: ");
#  endif
#  ifndef HAVE_DPSOPT
#   define HAVE_DPSOPT 1
#  endif
    sim_msg ("NEED_128");
# endif
# ifdef ROUND_ROBIN
#  ifdef HAVE_DPSOPT
    sim_msg (", ");
#  else
    sim_msg ("\n Options: ");
#  endif
#  ifndef HAVE_DPSOPT
#   define HAVE_DPSOPT 1
#  endif
    sim_msg ("ROUND_ROBIN");
# endif
# ifndef LOCKLESS
#  ifdef HAVE_DPSOPT
    sim_msg (", ");
#  else
    sim_msg ("\n Options: ");
#  endif
#  ifndef HAVE_DPSOPT
#   define HAVE_DPSOPT 1
#  endif
    sim_msg ("NO_LOCKLESS");
# endif
# if defined(GENERATED_MAKE_VER_H) && defined(VER_H_GIT_HASH)
    sim_msg ("\n  Commit: %s", VER_H_GIT_HASH);
# endif
    sim_msg ("\r\n\r\n");
    fflush(stderr); fflush(stdout);
  }

  // special dps8 initialization stuff that can't be done in reset, etc. ...

# ifdef TESTING
  // These are part of the scp interface
  sim_vm_parse_addr  = parse_addr;
  sim_vm_fprint_addr = fprint_addr;
# endif // TESTING

  sim_vm_cmd = dps8_cmds;

  // This is needed to make sbreak work
  sim_brk_types = sim_brk_dflt = SWMASK ('E');

# ifndef __MINGW64__
#  ifndef __MINGW32__
#   ifndef CROSS_MINGW32
#    ifndef CROSS_MINGW64
  // Wire the XF button to signal USR1
  signal (SIGUSR1, usr1_signal_handler);
  // On line 4,739 of the libuv man page, it recommends this.
  signal(SIGPIPE, SIG_IGN);
#    endif /* ifndef CROSS_MINGW64 */
#   endif /* ifndef CROSS_MINGW32 */
#  endif /* ifndef __MINGW32__ */
# endif /* ifndef __MINGW64__ */

#endif // ! PERF_STRIP

#if defined(__MINGW64__) || defined(__MINGW32__)
# include "bsd_random.h"
# define random  bsd_random
# define srandom bsd_srandom
#endif /* if defined(__MINGW64__) || defined(__MINGW32__) */

  char   rcap = 0;
  char   rnum = 0;
  char   rssuffix[20];
  char   statenme[30];
  struct timespec ts;

  memset(statenme, 0, sizeof(&statenme));
  memset(rssuffix, 0, sizeof(&rssuffix));
  (void)clock_gettime(CLOCK_REALTIME, &ts);
  srandom((unsigned int)(getpid() ^ (ts.tv_sec * ts.tv_nsec)));

  for (int i = 1; i < 21; ++i) {
    rcap = (int)random() % 2;
    rnum = (int)random() % 3;
    if (!rnum) rnum = (((int)random() % 10) + 48);
    if (!rcap) rcap = 33;
    if (rnum >= 48) rssuffix[i-1]=rnum;
    else rssuffix[i-1]=(char)(((long)random() % 26) + 64) + rcap;
  }

#if defined(__MINGW64__)   || \
    defined(__MINGW32__)   || \
    defined(CROSS_MINGW32) || \
    defined(CROSS_MINGW64)
  system_state = malloc (sizeof (struct system_state_s));
#else
  if (sim_randstate)
    sprintf(statenme, "%s.state", rssuffix);
  else
    sprintf(statenme, "state");
  if (!sim_nostate)
    system_state = (struct system_state_s *)
      create_shm (statenme, sizeof (struct system_state_s));
  else
    system_state = malloc (sizeof (struct system_state_s));
#endif

  if (!system_state) {
    int svErrno = errno;
    fflush(stderr); fflush(stdout);
    sim_warn ("FATAL: %s, aborting %s()\r\n",
              strerror (svErrno), __func__);
    exit (svErrno);
  }

#ifndef PERF_STRIP

# ifndef VER_H_GIT_HASH
#  define VER_H_GIT_HASH "0000000000000000000000000000000000000000"
# endif

    fflush(stdout); fflush(stderr);
  if (strlen (system_state->commit_id) == 0) {
    if (!sim_quiet && sim_randstate && sim_randompst)
      sim_printf ("Initialized new system state file \"dps8m.%s\"\r\n",
                  statenme);
  } else {
    if (strcmp (system_state->commit_id, VER_H_GIT_HASH) != 0) {
      sim_warn ("WARNING: System state hash mismatch; \"%s\" may be corrupt!\r\n",
                statenme);
    }
  }
  fflush(stderr); fflush(stdout);

  strncpy (system_state->stateHdr, STATE_HDR, sizeof (system_state->stateHdr));
  system_state->stateVer = STATE_VER;
  strncpy (system_state->commit_id, VER_H_GIT_HASH, sizeof (system_state->commit_id));

  systabInit ();

  // sets connect to 0
  memset (& sys_opts, 0, sizeof (sys_opts));
  // sys_poll_interval 10 ms (100 Hz)
  sys_opts.sys_poll_interval      = 10;
  // sys_slow_poll_interval 100 polls (1 Hz)
  sys_opts.sys_slow_poll_interval = 100;
  // sys_poll_check_rate in CPU cycles
  sys_opts.sys_poll_check_rate    = 1024;
#endif // ! PERF_STRIP

#ifdef PERF_STRIP
  cpu_init ();
#else
  sysCableInit ();
  iom_init ();
  disk_init ();
  mt_init ();
# ifndef __MINGW64__
#  ifndef __MINGW32__
#   ifndef CROSS_MINGW64
#    ifndef CROSS_MINGW32
  sk_init ();
#    endif /* ifndef CROSS_MINGW32 */
#   endif /* ifndef CROSS_MINGW64 */
#  endif /* ifndef __MINGW64__ */
# endif /* ifndef __MINGW32__ */
  fnpInit ();
  console_init (); // must come after fnpInit due to libuv initialization
 /* mpc_init (); */
  scu_init ();
  cpu_init ();
  rdr_init ();
  pun_init ();
  prt_init ();
  urp_init ();
# ifndef __MINGW64__
#  ifndef __MINGW32__
#   ifndef CROSS_MINGW64
#    ifndef CROSS_MINGW32
  absi_init ();
#    endif /* CROSS_MINGW32 */
#   endif /* CROSS_MINGW64 */
#  endif /* ifndef __MINGW32__ */
# endif /* ifndef __MINGW64__ */
  set_default_base_system (0, NULL);
# ifdef PANEL68
  panelScraperInit ();
# endif /* ifdef PANEL68 */
#endif
#if defined(THREADZ) || defined(LOCKLESS)
  initThreadz ();
#endif /* if defined(THREADZ) || defined(LOCKLESS) */
}

// Once-only shutdown; invoked via SCP

static void dps8_exit (void) {
  console_exit ();
  mt_exit ();
  fnpExit ();
}

#ifdef TESTING
static struct pr_table
  {
    char  * alias;    // pr alias
    int   n;          // number alias represents ....
  } _prtab[] =
  {
    {"pr0",   0}, ///< pr0 - 7
    {"pr1",   1},
    {"pr2",   2},
    {"pr3",   3},
    {"pr4",   4},
    {"pr5",   5},
    {"pr6",   6},
    {"pr7",   7},

    {"pr[0]", 0}, ///< pr0 - 7
    {"pr[1]", 1},
    {"pr[2]", 2},
    {"pr[3]", 3},
    {"pr[4]", 4},
    {"pr[5]", 5},
    {"pr[6]", 6},
    {"pr[7]", 7},

    // See: https://multicians.org/pg/mvm.html
    {"ap",    0},
    {"ab",    1},
    {"bp",    2},
    {"bb",    3},
    {"lp",    4},
    {"lb",    5},
    {"sp",    6},
    {"sb",    7},
    {0,       0}
  };

static int getAddress(int segno, int offset)
{
    // XXX Do we need to 1st check SDWAM for segment entry?

    // get address of in-core segment descriptor word from DSBR
    sdw0_s *s = fetchSDW ((word15) segno);

    return (s->ADDR + (word18) offset) & 0xffffff; // keep to 24-bits
}

static t_addr parse_addr (UNUSED DEVICE * dptr, const char *cptr,
                          const char **optr)
  {
    // a segment reference?
    if (strchr(cptr, '|'))
    {
        static char addspec[256];
        strcpy(addspec, cptr);

        *strchr(addspec, '|') = ' ';

        char seg[256], off[256];
        int params = sscanf(addspec, "%s %s", seg, off);
        if (params != 2)
        {
            sim_warn("parse_addr(): illegal number of parameters\n");
            *optr = cptr;   // signal error
            return 0;
        }

        // determine if segment is numeric or symbolic...
        char *endp;
        word18 PRoffset = 0;   // offset from PR[n] register (if any)
        int segno = (int)strtoll(seg, &endp, 8);
        if (endp == seg)
        {
            // not numeric...
            // 1st, see if it's a PR or alias thereof
            struct pr_table *prt = _prtab;
            while (prt->alias)
            {
                if (strcasecmp(seg, prt->alias) == 0)
                {
                    segno = cpu.PR[prt->n].SNR;
                    PRoffset = cpu.PR[prt->n].WORDNO;
                    break;
                }

                prt += 1;
            }

            if (!prt->alias)    // not a PR or alias
            {
              return 0;
            }
        }

        // determine if offset is numeric or symbolic entry point/segdef...
        uint offset = (uint)strtoll(off, &endp, 8);
        if (endp == off)
        {
            // not numeric...
            return 0;
        }

        // if we get here then seg contains a segment# and offset.
        // So, fetch the actual address given the segment & offset ...
        // ... and return this absolute, 24-bit address

        word24 abs_addr = (word24) getAddress(segno, (int) (offset + PRoffset));

        // TODO: only by luck does this work FixMe
        *optr = endp;   //cptr + strlen(cptr);

        return abs_addr;
    }
    else
    {
        // a PR or alias thereof
        int segno = 0;
        word24 offset = 0;
        struct pr_table *prt = _prtab;
        while (prt->alias)
        {
            if (strncasecmp(cptr, prt->alias, strlen(prt->alias)) == 0)
            {
                segno  = cpu.PR[prt->n].SNR;
                offset = cpu.PR[prt->n].WORDNO;
                break;
            }

            prt += 1;
        }
        if (prt->alias)    // a PR or alias
        {
            word24 abs_addr = (word24) getAddress(segno, (int) offset);
            *optr = cptr + strlen(prt->alias);

            return abs_addr;
        }
    }

    // No, determine absolute address given by cptr
    return (t_addr)strtol(cptr, (char **) optr, 8);
}
#endif // TESTING

#ifdef TESTING
static void fprint_addr (FILE * stream, UNUSED DEVICE *  dptr, t_addr simh_addr)
{
    fprintf(stream, "%06o", simh_addr);
}
#endif // TESTING

// This is part of the scp interface
// Based on the switch variable, symbolically output to stream ofile the data in
//  array val at the specified addr in unit uptr.
// "fprint_sym" – Based on the switch variable, symbolically output to
// stream ofile the data in array val at the specified addr in unit uptr.

t_stat fprint_sym (UNUSED FILE * ofile, UNUSED t_addr addr,
                   UNUSED t_value *val, UNUSED UNIT *uptr, int32 UNUSED sw)
{
#ifdef TESTING
// XXX Bug: assumes single cpu
// XXX CAC: This seems rather bogus; deciding the output format based on the
// address of the UNIT? Would it be better to use sim_unit.u3 (or some such
// as a word width?

    if (!((uint) sw & SWMASK ('M')))
        return SCPE_ARG;

    if (uptr == &cpu_dev.units[0])
    {
        word36 word1 = *val;
        char buf[256];
        // get base syntax
        char *d = disassemble(buf, word1);

        fprintf(ofile, "%s", d);

        // decode instruction
        DCDstruct ci;
        DCDstruct * p = & ci;
        decode_instruction (word1, p);

        // MW EIS?
        if (p->info->ndes > 1)
        {
            // Yup, just output word values (for now)
            // XXX Need to complete MW EIS support in disassemble()

            for(uint n = 0 ; n < p->info->ndes; n += 1)
                fprintf(ofile, " %012"PRIo64"", val[n + 1]);

            return (t_stat) -p->info->ndes;
        }

        return SCPE_OK;

        //fprintf(ofile, "%012"PRIo64"", *val);
        //return SCPE_OK;
    }
#endif
    return SCPE_ARG;
}

// This is part of the scp interface
//  – Based on the switch variable, parse character string cptr for a
//  symbolic value val at the specified addr in unit uptr.

t_stat parse_sym (UNUSED const char * cptr, UNUSED t_addr addr,
                  UNUSED UNIT * uptr, UNUSED t_value * val, UNUSED int32 sswitch)
  {
    return SCPE_ARG;
  }

// from MM

sysinfo_t sys_opts;

static t_stat sys_show_config (UNUSED FILE * st, UNUSED UNIT * uptr,
                               UNUSED int  val, UNUSED const void * desc)
  {
    sim_msg ("IOM connect time:         %d\n",
                sys_opts.iom_times.connect);
    return SCPE_OK;
}

static config_value_list_t cfg_timing_list[] =
  {
    { "disable", -1 },
    {  NULL,      0 }
  };

bool breakEnable = false;

static t_stat sys_set_break (UNUSED UNIT *  uptr, int32 value,
                             UNUSED const char * cptr, UNUSED void * desc)
  {
    breakEnable = !! value;
    return SCPE_OK;
  }

static t_stat sys_show_break (UNUSED FILE * st, UNUSED UNIT * uptr,
                              UNUSED int  val, UNUSED const void * desc)
  {
    sim_msg ("BREAK %s\r\n", breakEnable ? "ON" : "OFF" );
    return SCPE_OK;
  }

static config_value_list_t cfg_on_off [] =
  {
    { "off",     0 },
    { "on",      1 },
    { "disable", 0 },
    { "enable",  1 },
    {  NULL,     0 }
  };

static config_list_t sys_config_list[] =
  {
    { "connect_time", -1,  100000, cfg_timing_list },
    { "color",         0,  1,      cfg_on_off      },
    { NULL,            0,  0,      NULL            }
 };

static t_stat sys_set_config (UNUSED UNIT *  uptr, UNUSED int32 value,
                              const char * cptr, UNUSED void * desc)
  {
    config_state_t cfg_state = { NULL, NULL };

    for (;;)
      {
        int64_t v;
        int rc = cfg_parse ("sys_set_config", cptr, sys_config_list, & cfg_state,
                           & v);
        if (rc == -1) // done
          {
            break;
          }
        if (rc == -2) // error
          {
            cfg_parse_done (& cfg_state);
            return SCPE_ARG;
          }

        const char * p = sys_config_list[rc].name;
        if (strcmp (p, "connect_time") == 0)
          sys_opts.iom_times.connect = (int) v;
        else if (strcmp (p, "color") == 0)
          sys_opts.no_color = ! v;
        else
          {
            sim_msg ("error: sys_set_config: Invalid cfg_parse rc <%ld>\n", (long) rc);
            cfg_parse_done (& cfg_state);
            return SCPE_ARG;
          }
      } // process statements
    cfg_parse_done (& cfg_state);
    return SCPE_OK;
  }

static MTAB sys_mod[] =
  {
    {
      MTAB_dev_value,           /* Mask               */
      0,                        /* Match              */
      (char *) "CONFIG",        /* Print string       */
      (char *) "CONFIG",        /* Match string       */
      sys_set_config,           /* Validation routine */
      sys_show_config,          /* Display routine    */
      NULL,                     /* Value descriptor   */
      NULL,                     /* Help               */
    },
    {
      MTAB_dev_novalue,         /* Mask               */
      1,                        /* Match              */
      (char *) "BREAK",         /* Print string       */
      (char *) "BREAK",         /* Match string       */
      sys_set_break,            /* Validation routine */
      sys_show_break,           /* Display routine    */
      NULL,                     /* Value descriptor   */
      NULL,                     /* Help               */
    },
    {
      MTAB_dev_novalue,         /* Mask               */
      0,                        /* Match              */
      (char *) "NOBREAK",       /* Print string       */
      (char *) "NOBREAK",       /* Match string       */
      sys_set_break,            /* Validation routine */
      sys_show_break,           /* Display routine    */
      NULL,                     /* Value descriptor   */
      NULL,                     /* Help               */
    },
    MTAB_eol
  };

static t_stat sys_reset (UNUSED DEVICE  * dptr)
  {
    return SCPE_OK;
  }

static DEVICE sys_dev = {
    "SYS",       /* Name                */
    NULL,        /* Units               */
    NULL,        /* Registers           */
    sys_mod,     /* Modifiers           */
    0,           /* #Units              */
    8,           /* Address radix       */
    PASIZE,      /* Address width       */
    1,           /* Address increment   */
    8,           /* Data radix          */
    36,          /* Data width          */
    NULL,        /* Examine routine     */
    NULL,        /* Deposit routine     */
    & sys_reset, /* Reset routine       */
    NULL,        /* Boot routine        */
    NULL,        /* Attach routine      */
    NULL,        /* Detach routine      */
    NULL,        /* Context             */
    0,           /* Flags               */
    0,           /* Debug control flags */
    0,           /* Debug flag names    */
    NULL,        /* Memory size change  */
    NULL,        /* Logical name        */
    NULL,        /* Help                */
    NULL,        /* Attach_help         */
    NULL,        /* Help_ctx            */
    NULL,        /* Description         */
    NULL         /* End                 */
};

// This is part of the scp interface
DEVICE * sim_devices[] =
  {
    & cpu_dev, // dev[0] is special to the scp interface; it is the 'default device'
    & iom_dev,
    & tape_dev,
#ifndef __MINGW64__
# ifndef __MINGW32__
#  ifndef CROSS_MINGW32
#   ifndef CROSS_MINGW64
    & skc_dev,
#   endif /* ifndef CROSS_MINGW64 */
#  endif /* ifndef CROSS_MINGW32 */
# endif /* ifndef __MINGW32__ */
#endif /* ifndef __MINGW64__ */
    & mtp_dev,
    & fnp_dev,
    & dsk_dev,
    & ipc_dev,
    & msp_dev,
    & scu_dev,
 /* & mpc_dev, */
    & opc_dev,
    & sys_dev,
    & urp_dev,
    & rdr_dev,
    & pun_dev,
    & prt_dev,
#ifndef __MINGW64__
# ifndef __MINGW32__
#  ifndef CROSS_MINGW32
#   ifndef CROSS_MINGW64
    & absi_dev,
#   endif /* ifndef CROSS_MINGW64 */
#  endif /* ifndef CROSS_MINGW32 */
# endif /* ifndef __MINGW32__ */
#endif /* ifndef __MINGW64__ */
    NULL
  };

#ifdef PERF_STRIP
void dps8_init_strip (void)
  {
    dps8_init ();
  }
#endif
