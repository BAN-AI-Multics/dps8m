/*
 * vim: filetype=c:tabstop=4:ai:expandtab
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
#ifndef __MINGW64__
# ifndef __MINGW32__
#  ifndef CROSS_MINGW64
#   ifndef CROSS_MINGW32
#    include <signal.h>
#   endif /* ifndef CROSS_MINGW32 */
#  endif /* ifndef CROSS_MINGW64 */
# endif /* ifndef __MINGW32__ */
#endif /* ifndef __MINGW64__ */
#include <time.h>
#include <stdint.h>
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
#include "dps8_mgp.h"
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

#define DBG_CTR cpu.cycleCnt

#define ASSUME0 0

#ifdef TESTING
# undef FREE
# define FREE(p) free(p)
#endif /* ifdef TESTING */

// Strictly speaking, memory belongs in the SCU.
// We will treat memory as viewed from the CPU and elide the
// SCU configuration that maps memory across multiple SCUs.
// I would guess that multiple SCUs helped relieve memory
// contention across multiple CPUs, but that is a level of
// emulation that will be ignored.

struct system_state_s * system_state;

vol word36 * M = NULL;  //-V707   // memory

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
//  033 MGP0           MGP Read controller
//  034 MGP1           MGP Write controller
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
    "CABLE_RIPOUT",

    "SET CPU NUNITS=6",
    "SET IOM NUNITS=2",
    // ; 16 drives plus a placeholder for the controller
    "SET TAPE NUNITS=17",
    "SET MTP NUNITS=1",
    // ; 4 3381 drives; 2 controllers
    // ; 4 d501 drives; 2 controller
    // ; 4 d451 drives; same controller has d501s
    // ; 2 d500 drives; same controller has d501s
    "SET IPC NUNITS=2",
    "SET MSP NUNITS=2",
    "SET DISK NUNITS=26",
    "SET SCU NUNITS=4",
    "SET OPC NUNITS=2",
    "SET FNP NUNITS=8",
    "SET URP NUNITS=10",
    "SET RDR NUNITS=3",
    "SET PUN NUNITS=3",
    "SET PRT NUNITS=4",
# ifdef WITH_ABSI_DEV
#  ifndef __MINGW64__
#   ifndef __MINGW32__
#    ifndef CROSS_MINGW64
#     ifndef CROSS_MINGW32
    "SET ABSI NUNITS=1",
#     endif /* ifndef CROSS_MINGW32 */
#    endif /* ifndef CROSS_MINGW64 */
#   endif /* ifndef __MINGW32__ */
#  endif /* ifndef __MINGW64__ */
# endif /* ifdef WITH_ABSI_DEV */

# ifdef WITH_MGP_DEV
#  ifndef __MINGW64__
#   ifndef __MINGW32__
#    ifndef CROSS_MINGW64
#     ifndef CROSS_MINGW32
    "SET MGP NUNITS=2",
#     endif /* ifndef CROSS_MINGW32 */
#    endif /* ifndef CROSS_MINGW64 */
#   endif /* ifndef __MINGW32__ */
#  endif /* ifndef __MINGW64__ */
# endif /* ifdef WITH_MGP_DEV */

# ifdef WITH_SOCKET_DEV
#  ifndef __MINGW64__
#   ifndef __MINGW32__
#    ifndef CROSS_MINGW64
#     ifndef CROSS_MINGW32
    "SET SKC NUNITS=64",
#     endif /* ifndef CROSS_MINGW32 */
#    endif /* ifndef CROSS_MINGW64 */
#   endif /* ifndef __MINGW32__ */
#  endif /* ifndef __MINGW64__ */
# endif /* ifdef WITH_SOCKET_DEV */

// CPU0

    "SET CPU0 CONFIG=FAULTBASE=Multics",

    "SET CPU0 CONFIG=NUM=0",
    // ; As per GB61-01 Operators Guide, App. A
    // ; switches: 4, 6, 18, 19, 20, 23, 24, 25, 26, 28
    "SET CPU0 CONFIG=DATA=024000717200",
    "SET CPU0 CONFIG=ADDRESS=000000000000",

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

    "SET CPU0 CONFIG=PORT=A",
    "SET CPU0   CONFIG=ASSIGNMENT=0",
    "SET CPU0   CONFIG=INTERLACE=0",
    "SET CPU0   CONFIG=ENABLE=1",
    "SET CPU0   CONFIG=INIT_ENABLE=1",
    "SET CPU0   CONFIG=STORE_SIZE=4M",

    "SET CPU0 CONFIG=PORT=B",
    "SET CPU0   CONFIG=ASSIGNMENT=1",
    "SET CPU0   CONFIG=INTERLACE=0",
    "SET CPU0   CONFIG=ENABLE=1",
    "SET CPU0   CONFIG=INIT_ENABLE=1",
    "SET CPU0   CONFIG=STORE_SIZE=4M",

    "SET CPU0 CONFIG=PORT=C",
    "SET CPU0   CONFIG=ASSIGNMENT=2",
    "SET CPU0   CONFIG=INTERLACE=0",
    "SET CPU0   CONFIG=ENABLE=1",
    "SET CPU0   CONFIG=INIT_ENABLE=1",
    "SET CPU0   CONFIG=STORE_SIZE=4M",

    "SET CPU0 CONFIG=PORT=D",
    "SET CPU0   CONFIG=ASSIGNMENT=3",
    "SET CPU0   CONFIG=INTERLACE=0",
    "SET CPU0   CONFIG=ENABLE=1",
    "SET CPU0   CONFIG=INIT_ENABLE=1",
    "SET CPU0   CONFIG=STORE_SIZE=4M",

    // ; 0 = GCOS 1 = VMS
    "SET CPU0 CONFIG=MODE=Multics",

    "SET CPU0 CONFIG=ENABLE_CACHE=enable",
    "SET CPU0 CONFIG=SDWAM=enable",
    "SET CPU0 CONFIG=PTWAM=enable",

    // ; 0 = 8/70
    "SET CPU0 CONFIG=SPEED=0",

    "SET CPU0 CONFIG=DIS_ENABLE=enable",
    "SET CPU0 CONFIG=STEADY_CLOCK=disable",
    "SET CPU0 CONFIG=HALT_ON_UNIMPLEMENTED=disable",
    "SET CPU0 CONFIG=ENABLE_WAM=disable",
    "SET CPU0 CONFIG=REPORT_FAULTS=disable",
    "SET CPU0 CONFIG=TRO_ENABLE=enable",
    "SET CPU0 CONFIG=Y2K=disable",
    "SET CPU0 CONFIG=DRL_FATAL=disable",
    "SET CPU0 CONFIG=USEMAP=disable",
    "SET CPU0 CONFIG=PROM_INSTALLED=enable",
    "SET CPU0 CONFIG=HEX_MODE_INSTALLED=disable",
    "SET CPU0 CONFIG=CACHE_INSTALLED=enable",
    "SET CPU0 CONFIG=CLOCK_SLAVE_INSTALLED=enable",

// CPU1

    "SET CPU1 CONFIG=FAULTBASE=Multics",

    "SET CPU1 CONFIG=NUM=1",
    // ; As per GB61-01 Operators Guide, App. A
    // ; switches: 4, 6, 18, 19, 20, 23, 24, 25, 26, 28
    "SET CPU1 CONFIG=DATA=024000717200",
    "SET CPU1 CONFIG=ADDRESS=000000000000",

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

    "SET CPU1 CONFIG=PORT=A",
    "SET CPU1   CONFIG=ASSIGNMENT=0",
    "SET CPU1   CONFIG=INTERLACE=0",
    "SET CPU1   CONFIG=ENABLE=1",
    "SET CPU1   CONFIG=INIT_ENABLE=1",
    "SET CPU1   CONFIG=STORE_SIZE=4M",

    "SET CPU1 CONFIG=PORT=B",
    "SET CPU1   CONFIG=ASSIGNMENT=1",
    "SET CPU1   CONFIG=INTERLACE=0",
    "SET CPU1   CONFIG=ENABLE=1",
    "SET CPU1   CONFIG=INIT_ENABLE=1",
    "SET CPU1   CONFIG=STORE_SIZE=4M",

    "SET CPU1 CONFIG=PORT=C",
    "SET CPU1   CONFIG=ASSIGNMENT=2",
    "SET CPU1   CONFIG=INTERLACE=0",
    "SET CPU1   CONFIG=ENABLE=1",
    "SET CPU1   CONFIG=INIT_ENABLE=1",
    "SET CPU1   CONFIG=STORE_SIZE=4M",

    "SET CPU1 CONFIG=PORT=D",
    "SET CPU1   CONFIG=ASSIGNMENT=3",
    "SET CPU1   CONFIG=INTERLACE=0",
    "SET CPU1   CONFIG=ENABLE=1",
    "SET CPU1   CONFIG=INIT_ENABLE=1",
    "SET CPU1   CONFIG=STORE_SIZE=4M",

    // ; 0 = GCOS 1 = VMS
    "SET CPU1 CONFIG=MODE=Multics",

    "SET CPU1 CONFIG=ENABLE_CACHE=enable",
    "SET CPU1 CONFIG=SDWAM=enable",
    "SET CPU1 CONFIG=PTWAM=enable",

    // ; 0 = 8/70
    "SET CPU1 CONFIG=SPEED=0",

    "SET CPU1 CONFIG=DIS_ENABLE=enable",
    "SET CPU1 CONFIG=STEADY_CLOCK=disable",
    "SET CPU1 CONFIG=HALT_ON_UNIMPLEMENTED=disable",
    "SET CPU1 CONFIG=ENABLE_WAM=disable",
    "SET CPU1 CONFIG=REPORT_FAULTS=disable",
    "SET CPU1 CONFIG=TRO_ENABLE=enable",
    "SET CPU1 CONFIG=Y2K=disable",
    "SET CPU1 CONFIG=DRL_FATAL=disable",
    "SET CPU1 CONFIG=USEMAP=disable",
    "SET CPU1 CONFIG=PROM_INSTALLED=enable",
    "SET CPU1 CONFIG=HEX_MODE_INSTALLED=disable",
    "SET CPU1 CONFIG=CACHE_INSTALLED=enable",
    "SET CPU1 CONFIG=CLOCK_SLAVE_INSTALLED=enable",

// CPU2

    "SET CPU2 CONFIG=FAULTBASE=Multics",

    "SET CPU2 CONFIG=NUM=2",
    // ; As per GB61-01 Operators Guide, App. A
    // ; switches: 4, 6, 18, 19, 20, 23, 24, 25, 26, 28
    "SET CPU2 CONFIG=DATA=024000717200",
    "SET CPU2 CONFIG=ADDRESS=000000000000",

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

    "SET CPU2 CONFIG=PORT=A",
    "SET CPU2   CONFIG=ASSIGNMENT=0",
    "SET CPU2   CONFIG=INTERLACE=0",
    "SET CPU2   CONFIG=ENABLE=1",
    "SET CPU2   CONFIG=INIT_ENABLE=1",
    "SET CPU2   CONFIG=STORE_SIZE=4M",

    "SET CPU2 CONFIG=PORT=B",
    "SET CPU2   CONFIG=ASSIGNMENT=1",
    "SET CPU2   CONFIG=INTERLACE=0",
    "SET CPU2   CONFIG=ENABLE=1",
    "SET CPU2   CONFIG=INIT_ENABLE=1",
    "SET CPU2   CONFIG=STORE_SIZE=4M",

    "SET CPU2 CONFIG=PORT=C",
    "SET CPU2   CONFIG=ASSIGNMENT=2",
    "SET CPU2   CONFIG=INTERLACE=0",
    "SET CPU2   CONFIG=ENABLE=1",
    "SET CPU2   CONFIG=INIT_ENABLE=1",
    "SET CPU2   CONFIG=STORE_SIZE=4M",

    "SET CPU2 CONFIG=PORT=D",
    "SET CPU2   CONFIG=ASSIGNMENT=3",
    "SET CPU2   CONFIG=INTERLACE=0",
    "SET CPU2   CONFIG=ENABLE=1",
    "SET CPU2   CONFIG=INIT_ENABLE=1",
    "SET CPU2   CONFIG=STORE_SIZE=4M",

    // ; 0 = GCOS 1 = VMS
    "SET CPU2 CONFIG=MODE=Multics",

    "SET CPU2 CONFIG=ENABLE_CACHE=enable",
    "SET CPU2 CONFIG=SDWAM=enable",
    "SET CPU2 CONFIG=PTWAM=enable",

    // ; 0 = 8/70
    "SET CPU2 CONFIG=SPEED=0",

    "SET CPU2 CONFIG=DIS_ENABLE=enable",
    "SET CPU2 CONFIG=STEADY_CLOCK=disable",
    "SET CPU2 CONFIG=HALT_ON_UNIMPLEMENTED=disable",
    "SET CPU2 CONFIG=ENABLE_WAM=disable",
    "SET CPU2 CONFIG=REPORT_FAULTS=disable",
    "SET CPU2 CONFIG=TRO_ENABLE=enable",
    "SET CPU2 CONFIG=Y2K=disable",
    "SET CPU2 CONFIG=DRL_FATAL=disable",
    "SET CPU2 CONFIG=USEMAP=disable",
    "SET CPU2 CONFIG=PROM_INSTALLED=enable",
    "SET CPU2 CONFIG=HEX_MODE_INSTALLED=disable",
    "SET CPU2 CONFIG=CACHE_INSTALLED=enable",
    "SET CPU2 CONFIG=CLOCK_SLAVE_INSTALLED=enable",

// CPU3

    "SET CPU3 CONFIG=FAULTBASE=Multics",

    "SET CPU3 CONFIG=NUM=3",
    // ; As per GB61-01 Operators Guide, App. A
    // ; switches: 4, 6, 18, 19, 20, 23, 24, 25, 26, 28
    "SET CPU3 CONFIG=DATA=024000717200",
    "SET CPU3 CONFIG=ADDRESS=000000000000",

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

    "SET CPU3 CONFIG=PORT=A",
    "SET CPU3   CONFIG=ASSIGNMENT=0",
    "SET CPU3   CONFIG=INTERLACE=0",
    "SET CPU3   CONFIG=ENABLE=1",
    "SET CPU3   CONFIG=INIT_ENABLE=1",
    "SET CPU3   CONFIG=STORE_SIZE=4M",

    "SET CPU3 CONFIG=PORT=B",
    "SET CPU3   CONFIG=ASSIGNMENT=1",
    "SET CPU3   CONFIG=INTERLACE=0",
    "SET CPU3   CONFIG=ENABLE=1",
    "SET CPU3   CONFIG=INIT_ENABLE=1",
    "SET CPU3   CONFIG=STORE_SIZE=4M",

    "SET CPU3 CONFIG=PORT=C",
    "SET CPU3   CONFIG=ASSIGNMENT=2",
    "SET CPU3   CONFIG=INTERLACE=0",
    "SET CPU3   CONFIG=ENABLE=1",
    "SET CPU3   CONFIG=INIT_ENABLE=1",
    "SET CPU3   CONFIG=STORE_SIZE=4M",

    "SET CPU3 CONFIG=PORT=D",
    "SET CPU3   CONFIG=ASSIGNMENT=3",
    "SET CPU3   CONFIG=INTERLACE=0",
    "SET CPU3   CONFIG=ENABLE=1",
    "SET CPU3   CONFIG=INIT_ENABLE=1",
    "SET CPU3   CONFIG=STORE_SIZE=4M",

    // ; 0 = GCOS 1 = VMS
    "SET CPU3 CONFIG=MODE=Multics",

    "SET CPU3 CONFIG=ENABLE_CACHE=enable",
    "SET CPU3 CONFIG=SDWAM=enable",
    "SET CPU3 CONFIG=PTWAM=enable",

    // ; 0 = 8/70
    "SET CPU3 CONFIG=SPEED=0",

    "SET CPU3 CONFIG=DIS_ENABLE=enable",
    "SET CPU3 CONFIG=STEADY_CLOCK=disable",
    "SET CPU3 CONFIG=HALT_ON_UNIMPLEMENTED=disable",
    "SET CPU3 CONFIG=ENABLE_WAM=disable",
    "SET CPU3 CONFIG=REPORT_FAULTS=disable",
    "SET CPU3 CONFIG=TRO_ENABLE=enable",
    "SET CPU3 CONFIG=Y2K=disable",
    "SET CPU3 CONFIG=DRL_FATAL=disable",
    "SET CPU3 CONFIG=USEMAP=disable",
    "SET CPU3 CONFIG=PROM_INSTALLED=enable",
    "SET CPU3 CONFIG=HEX_MODE_INSTALLED=disable",
    "SET CPU3 CONFIG=CACHE_INSTALLED=enable",
    "SET CPU3 CONFIG=CLOCK_SLAVE_INSTALLED=enable",

// CPU4

    "SET CPU4 CONFIG=FAULTBASE=Multics",

    "SET CPU4 CONFIG=NUM=4",
    // ; As per GB61-01 Operators Guide, App. A
    // ; switches: 4, 6, 18, 19, 20, 23, 24, 25, 26, 28
    "SET CPU4 CONFIG=DATA=024000717200",
    "SET CPU4 CONFIG=ADDRESS=000000000000",

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

    "SET CPU4 CONFIG=PORT=A",
    "SET CPU4   CONFIG=ASSIGNMENT=0",
    "SET CPU4   CONFIG=INTERLACE=0",
    "SET CPU4   CONFIG=ENABLE=1",
    "SET CPU4   CONFIG=INIT_ENABLE=1",
    "SET CPU4   CONFIG=STORE_SIZE=4M",

    "SET CPU4 CONFIG=PORT=B",
    "SET CPU4   CONFIG=ASSIGNMENT=1",
    "SET CPU4   CONFIG=INTERLACE=0",
    "SET CPU4   CONFIG=ENABLE=1",
    "SET CPU4   CONFIG=INIT_ENABLE=1",
    "SET CPU4   CONFIG=STORE_SIZE=4M",

    "SET CPU4 CONFIG=PORT=C",
    "SET CPU4   CONFIG=ASSIGNMENT=2",
    "SET CPU4   CONFIG=INTERLACE=0",
    "SET CPU4   CONFIG=ENABLE=1",
    "SET CPU4   CONFIG=INIT_ENABLE=1",
    "SET CPU4   CONFIG=STORE_SIZE=4M",

    "SET CPU4 CONFIG=PORT=D",
    "SET CPU4   CONFIG=ASSIGNMENT=3",
    "SET CPU4   CONFIG=INTERLACE=0",
    "SET CPU4   CONFIG=ENABLE=1",
    "SET CPU4   CONFIG=INIT_ENABLE=1",
    "SET CPU4   CONFIG=STORE_SIZE=4M",

    // ; 0 = GCOS 1 = VMS
    "SET CPU4 CONFIG=MODE=Multics",

    "SET CPU4 CONFIG=ENABLE_CACHE=enable",
    "SET CPU4 CONFIG=SDWAM=enable",
    "SET CPU4 CONFIG=PTWAM=enable",

    // ; 0 = 8/70
    "SET CPU4 CONFIG=SPEED=0",

    "SET CPU4 CONFIG=DIS_ENABLE=enable",
    "SET CPU4 CONFIG=STEADY_CLOCK=disable",
    "SET CPU4 CONFIG=HALT_ON_UNIMPLEMENTED=disable",
    "SET CPU4 CONFIG=ENABLE_WAM=disable",
    "SET CPU4 CONFIG=REPORT_FAULTS=disable",
    "SET CPU4 CONFIG=TRO_ENABLE=enable",
    "SET CPU4 CONFIG=Y2K=disable",
    "SET CPU4 CONFIG=DRL_FATAL=disable",
    "SET CPU4 CONFIG=USEMAP=disable",
    "SET CPU4 CONFIG=PROM_INSTALLED=enable",
    "SET CPU4 CONFIG=HEX_MODE_INSTALLED=disable",
    "SET CPU4 CONFIG=CACHE_INSTALLED=enable",
    "SET CPU4 CONFIG=CLOCK_SLAVE_INSTALLED=enable",

// CPU5

    "SET CPU5 CONFIG=FAULTBASE=Multics",

    "SET CPU5 CONFIG=NUM=5",
    // ; As per GB61-01 Operators Guide, App. A
    // ; switches: 4, 6, 18, 19, 20, 23, 24, 25, 26, 28
    "SET CPU5 CONFIG=DATA=024000717200",
    "SET CPU5 CONFIG=ADDRESS=000000000000",

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

    "SET CPU5 CONFIG=PORT=A",
    "SET CPU5   CONFIG=ASSIGNMENT=0",
    "SET CPU5   CONFIG=INTERLACE=0",
    "SET CPU5   CONFIG=ENABLE=1",
    "SET CPU5   CONFIG=INIT_ENABLE=1",
    "SET CPU5   CONFIG=STORE_SIZE=4M",

    "SET CPU5 CONFIG=PORT=B",
    "SET CPU5   CONFIG=ASSIGNMENT=1",
    "SET CPU5   CONFIG=INTERLACE=0",
    "SET CPU5   CONFIG=ENABLE=1",
    "SET CPU5   CONFIG=INIT_ENABLE=1",
    "SET CPU5   CONFIG=STORE_SIZE=4M",

    "SET CPU5 CONFIG=PORT=C",
    "SET CPU5   CONFIG=ASSIGNMENT=2",
    "SET CPU5   CONFIG=INTERLACE=0",
    "SET CPU5   CONFIG=ENABLE=1",
    "SET CPU5   CONFIG=INIT_ENABLE=1",
    "SET CPU5   CONFIG=STORE_SIZE=4M",

    "SET CPU5 CONFIG=PORT=D",
    "SET CPU5   CONFIG=ASSIGNMENT=3",
    "SET CPU5   CONFIG=INTERLACE=0",
    "SET CPU5   CONFIG=ENABLE=1",
    "SET CPU5   CONFIG=INIT_ENABLE=1",
    "SET CPU5   CONFIG=STORE_SIZE=4M",

    // ; 0 = GCOS 1 = VMS
    "SET CPU5 CONFIG=MODE=Multics",

    "SET CPU5 CONFIG=ENABLE_CACHE=enable",
    "SET CPU5 CONFIG=SDWAM=enable",
    "SET CPU5 CONFIG=PTWAM=enable",

    // ; 0 = 8/70
    "SET CPU5 CONFIG=SPEED=0",

    "SET CPU5 CONFIG=DIS_ENABLE=enable",
    "SET CPU5 CONFIG=STEADY_CLOCK=disable",
    "SET CPU5 CONFIG=HALT_ON_UNIMPLEMENTED=disable",
    "SET CPU5 CONFIG=ENABLE_WAM=disable",
    "SET CPU5 CONFIG=REPORT_FAULTS=disable",
    "SET CPU5 CONFIG=TRO_ENABLE=enable",
    "SET CPU5 CONFIG=Y2K=disable",
    "SET CPU5 CONFIG=DRL_FATAL=disable",
    "SET CPU5 CONFIG=USEMAP=disable",
    "SET CPU5 CONFIG=PROM_INSTALLED=enable",
    "SET CPU5 CONFIG=HEX_MODE_INSTALLED=disable",
    "SET CPU5 CONFIG=CACHE_INSTALLED=enable",
    "SET CPU5 CONFIG=CLOCK_SLAVE_INSTALLED=enable",

# if 0 // Disabled until the port expander code is working
// CPU6

    "SET CPU6 CONFIG=FAULTBASE=Multics",

    "SET CPU6 CONFIG=NUM=6",
    // ; As per GB61-01 Operators Guide, App. A
    // ; switches: 4, 6, 18, 19, 20, 23, 24, 25, 26, 28
    "SET CPU6 CONFIG=DATA=024000717200",
    "SET CPU6 CONFIG=ADDRESS=000000000000",

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

    "SET CPU6 CONFIG=PORT=A",
    "SET CPU6   CONFIG=ASSIGNMENT=0",
    "SET CPU6   CONFIG=INTERLACE=0",
    "SET CPU6   CONFIG=ENABLE=1",
    "SET CPU6   CONFIG=INIT_ENABLE=1",
    "SET CPU6   CONFIG=STORE_SIZE=4M",

    "SET CPU6 CONFIG=PORT=B",
    "SET CPU6   CONFIG=ASSIGNMENT=1",
    "SET CPU6   CONFIG=INTERLACE=0",
    "SET CPU6   CONFIG=ENABLE=1",
    "SET CPU6   CONFIG=INIT_ENABLE=1",
    "SET CPU6   CONFIG=STORE_SIZE=4M",

    "SET CPU6 CONFIG=PORT=C",
    "SET CPU6   CONFIG=ASSIGNMENT=2",
    "SET CPU6   CONFIG=INTERLACE=0",
    "SET CPU6   CONFIG=ENABLE=1",
    "SET CPU6   CONFIG=INIT_ENABLE=1",
    "SET CPU6   CONFIG=STORE_SIZE=4M",

    "SET CPU6 CONFIG=PORT=D",
    "SET CPU6   CONFIG=ASSIGNMENT=3",
    "SET CPU6   CONFIG=INTERLACE=0",
    "SET CPU6   CONFIG=ENABLE=1",
    "SET CPU6   CONFIG=INIT_ENABLE=1",
    "SET CPU6   CONFIG=STORE_SIZE=4M",

    // ; 0 = GCOS 1 = VMS
    "SET CPU6 CONFIG=MODE=Multics",

    "SET CPU6 CONFIG=ENABLE_CACHE=enable",
    "SET CPU6 CONFIG=SDWAM=enable",
    "SET CPU6 CONFIG=PTWAM=enable",

    // ; 0 = 8/70
    "SET CPU6 CONFIG=SPEED=0",

    "SET CPU6 CONFIG=DIS_ENABLE=enable",
    "SET CPU6 CONFIG=STEADY_CLOCK=disable",
    "SET CPU6 CONFIG=HALT_ON_UNIMPLEMENTED=disable",
    "SET CPU6 CONFIG=ENABLE_WAM=disable",
    "SET CPU6 CONFIG=REPORT_FAULTS=disable",
    "SET CPU6 CONFIG=TRO_ENABLE=enable",
    "SET CPU6 CONFIG=Y2K=disable",
    "SET CPU6 CONFIG=DRL_FATAL=disable",
    "SET CPU6 CONFIG=USEMAP=disable",
    "SET CPU6 CONFIG=PROM_INSTALLED=enable",
    "SET CPU6 CONFIG=HEX_MODE_INSTALLED=disable",
    "SET CPU6 CONFIG=CACHE_INSTALLED=enable",
    "SET CPU6 CONFIG=CLOCK_SLAVE_INSTALLED=enable",

# endif

# if 0 // Disabled until the port expander code is working

// CPU7

    "SET CPU7 CONFIG=FAULTBASE=Multics",

    "SET CPU7 CONFIG=NUM=7",
    // ; As per GB61-01 Operators Guide, App. A
    // ; switches: 4, 6, 18, 19, 20, 23, 24, 25, 26, 28
    "SET CPU7 CONFIG=DATA=024000717200",
    "SET CPU7 CONFIG=ADDRESS=000000000000",

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

    "SET CPU7 CONFIG=PORT=A",
    "SET CPU7   CONFIG=ASSIGNMENT=0",
    "SET CPU7   CONFIG=INTERLACE=0",
    "SET CPU7   CONFIG=ENABLE=1",
    "SET CPU7   CONFIG=INIT_ENABLE=1",
    "SET CPU7   CONFIG=STORE_SIZE=4M",

    "SET CPU7 CONFIG=PORT=B",
    "SET CPU7   CONFIG=ASSIGNMENT=1",
    "SET CPU7   CONFIG=INTERLACE=0",
    "SET CPU7   CONFIG=ENABLE=1",
    "SET CPU7   CONFIG=INIT_ENABLE=1",
    "SET CPU7   CONFIG=STORE_SIZE=4M",

    "SET CPU7 CONFIG=PORT=C",
    "SET CPU7   CONFIG=ASSIGNMENT=2",
    "SET CPU7   CONFIG=INTERLACE=0",
    "SET CPU7   CONFIG=ENABLE=1",
    "SET CPU7   CONFIG=INIT_ENABLE=1",
    "SET CPU7   CONFIG=STORE_SIZE=4M",

    "SET CPU7 CONFIG=PORT=D",
    "SET CPU7   CONFIG=ASSIGNMENT=3",
    "SET CPU7   CONFIG=INTERLACE=0",
    "SET CPU7   CONFIG=ENABLE=1",
    "SET CPU7   CONFIG=INIT_ENABLE=1",
    "SET CPU7   CONFIG=STORE_SIZE=4M",

    // ; 0 = GCOS 1 = VMS
    "SET CPU7 CONFIG=MODE=Multics",

    "SET CPU7 CONFIG=ENABLE_CACHE=enable",
    "SET CPU7 CONFIG=SDWAM=enable",
    "SET CPU7 CONFIG=PTWAM=enable",

    // ; 0 = 8/70
    "SET CPU7 CONFIG=SPEED=0",

    "SET CPU7 CONFIG=DIS_ENABLE=enable",
    "SET CPU7 CONFIG=STEADY_CLOCK=disable",
    "SET CPU7 CONFIG=HALT_ON_UNIMPLEMENTED=disable",
    "SET CPU7 CONFIG=ENABLE_WAM=disable",
    "SET CPU7 CONFIG=REPORT_FAULTS=disable",
    "SET CPU7 CONFIG=TRO_ENABLE=enable",
    "SET CPU7 CONFIG=Y2K=disable",
    "SET CPU7 CONFIG=DRL_FATAL=disable",
    "SET CPU7 CONFIG=USEMAP=disable",
    "SET CPU7 CONFIG=PROM_INSTALLED=enable",
    "SET CPU7 CONFIG=HEX_MODE_INSTALLED=disable",
    "SET CPU7 CONFIG=CACHE_INSTALLED=enable",
    "SET CPU7 CONFIG=CLOCK_SLAVE_INSTALLED=enable",

# endif

// IOM0

    "SET IOM0 CONFIG=IOM_BASE=Multics",
    "SET IOM0 CONFIG=MULTIPLEX_BASE=0120",
    "SET IOM0 CONFIG=OS=Multics",
    "SET IOM0 CONFIG=BOOT=tape",
    "SET IOM0 CONFIG=TAPECHAN=012",
    "SET IOM0 CONFIG=CARDCHAN=011",
    "SET IOM0 CONFIG=SCUPORT=0",

    "SET IOM0 CONFIG=PORT=0",
    "SET IOM0   CONFIG=ADDR=0",
    "SET IOM0   CONFIG=INTERLACE=0",
    "SET IOM0   CONFIG=ENABLE=1",
    "SET IOM0   CONFIG=INITENABLE=0",
    "SET IOM0   CONFIG=HALFSIZE=0",
    "SET IOM0   CONFIG=STORE_SIZE=4M",

    "SET IOM0 CONFIG=PORT=1",
    "SET IOM0   CONFIG=ADDR=1",
    "SET IOM0   CONFIG=INTERLACE=0",
    "SET IOM0   CONFIG=ENABLE=1",
    "SET IOM0   CONFIG=INITENABLE=0",
    "SET IOM0   CONFIG=HALFSIZE=0",
    "SET IOM0   CONFIG=STORE_SIZE=4M",

    "SET IOM0 CONFIG=PORT=2",
    "SET IOM0   CONFIG=ADDR=2",
    "SET IOM0   CONFIG=INTERLACE=0",
    "SET IOM0   CONFIG=ENABLE=1",
    "SET IOM0   CONFIG=INITENABLE=0",
    "SET IOM0   CONFIG=HALFSIZE=0",
    "SET IOM0   CONFIG=STORE_SIZE=4M",

    "SET IOM0 CONFIG=PORT=3",
    "SET IOM0   CONFIG=ADDR=3",
    "SET IOM0   CONFIG=INTERLACE=0",
    "SET IOM0   CONFIG=ENABLE=1",
    "SET IOM0   CONFIG=INITENABLE=0",
    "SET IOM0   CONFIG=HALFSIZE=0",
    "SET IOM0   CONFIG=STORE_SIZE=4M",

    "SET IOM0 CONFIG=PORT=4",
    "SET IOM0   CONFIG=ENABLE=0",

    "SET IOM0 CONFIG=PORT=5",
    "SET IOM0   CONFIG=ENABLE=0",

    "SET IOM0 CONFIG=PORT=6",
    "SET IOM0   CONFIG=ENABLE=0",

    "SET IOM0 CONFIG=PORT=7",
    "SET IOM0   CONFIG=ENABLE=0",

// IOM1

    "SET IOM1 CONFIG=IOM_BASE=Multics2",
    "SET IOM1 CONFIG=MULTIPLEX_BASE=0121",
    "SET IOM1 CONFIG=OS=Multics",
    "SET IOM1 CONFIG=BOOT=tape",
    "SET IOM1 CONFIG=TAPECHAN=012",
    "SET IOM1 CONFIG=CARDCHAN=011",
    "SET IOM1 CONFIG=SCUPORT=0",

    "SET IOM1 CONFIG=PORT=0",
    "SET IOM1   CONFIG=ADDR=0",
    "SET IOM1   CONFIG=INTERLACE=0",
    "SET IOM1   CONFIG=ENABLE=1",
    "SET IOM1   CONFIG=INITENABLE=0",
    "SET IOM1   CONFIG=HALFSIZE=0",

    "SET IOM1 CONFIG=PORT=1",
    "SET IOM1   CONFIG=ADDR=1",
    "SET IOM1   CONFIG=INTERLACE=0",
    "SET IOM1   CONFIG=ENABLE=1",
    "SET IOM1   CONFIG=INITENABLE=0",
    "SET IOM1   CONFIG=HALFSIZE=0",

    "SET IOM1 CONFIG=PORT=2",
    "SET IOM1   CONFIG=ENABLE=0",
    "SET IOM1 CONFIG=PORT=3",
    "SET IOM1   CONFIG=ENABLE=0",
    "SET IOM1 CONFIG=PORT=4",
    "SET IOM1   CONFIG=ENABLE=0",
    "SET IOM1 CONFIG=PORT=5",
    "SET IOM1   CONFIG=ENABLE=0",
    "SET IOM1 CONFIG=PORT=6",
    "SET IOM1   CONFIG=ENABLE=0",
    "SET IOM1 CONFIG=PORT=7",
    "SET IOM1   CONFIG=ENABLE=0",

# if 0

// IOM2

    "set IOM2 CONFIG=IOM_BASE=Multics2",
    "set IOM2 CONFIG=MULTIPLEX_BASE=0121",
    "set IOM2 CONFIG=OS=Multics",
    "set IOM2 CONFIG=BOOT=tape",
    "SET IOM2 CONFIG=TAPECHAN=012",
    "SET IOM2 CONFIG=CARDCHAN=011",
    "SET IOM2 CONFIG=SCUPORT=0",

    "SET IOM2 CONFIG=PORT=0",
    "SET IOM2   CONFIG=ADDR=0",
    "SET IOM2   CONFIG=INTERLACE=0",
    "SET IOM2   CONFIG=ENABLE=1",
    "SET IOM2   CONFIG=INITENABLE=0",
    "SET IOM2   CONFIG=HALFSIZE=0",

    "SET IOM2 CONFIG=PORT=1",
    "SET IOM2   CONFIG=ADDR=1",
    "SET IOM2   CONFIG=INTERLACE=0",
    "SET IOM2   CONFIG=ENABLE=1",
    "SET IOM2   CONFIG=INITENABLE=0",
    "SET IOM2   CONFIG=HALFSIZE=0",

    "SET IOM2 CONFIG=PORT=2",
    "SET IOM2   CONFIG=ENABLE=0",
    "SET IOM2 CONFIG=PORT=3",
    "SET IOM2   CONFIG=ENABLE=0",
    "SET IOM2 CONFIG=PORT=4",
    "SET IOM2   CONFIG=ENABLE=0",
    "SET IOM2 CONFIG=PORT=5",
    "SET IOM2   CONFIG=ENABLE=0",
    "SET IOM2 CONFIG=PORT=6",
    "SET IOM2   CONFIG=ENABLE=0",
    "SET IOM2 CONFIG=PORT=7",
    "SET IOM2   CONFIG=ENABLE=0",

// IOM3

    "SET IOM3 CONFIG=IOM_BASE=Multics2",
    "SET IOM3 CONFIG=MULTIPLEX_BASE=0121",
    "SET IOM3 CONFIG=OS=Multics",
    "SET IOM3 CONFIG=BOOT=TAPE",
    "SET IOM3 CONFIG=TAPECHAN=012",
    "SET IOM3 CONFIG=CARDCHAN=011",
    "SET IOM3 CONFIG=SCUPORT=0",

    "SET IOM3 CONFIG=PORT=0",
    "SET IOM3   CONFIG=ADDR=0",
    "SET IOM3   CONFIG=INTERLACE=0",
    "SET IOM3   CONFIG=ENABLE=1",
    "SET IOM3   CONFIG=INITENABLE=0",
    "SET IOM3   CONFIG=HALFSIZE=0",

    "SET IOM3 CONFIG=PORT=1",
    "SET IOM3   CONFIG=ADDR=1",
    "SET IOM3   CONFIG=INTERLACE=0",
    "SET IOM3   CONFIG=ENABLE=1",
    "SET IOM3   CONFIG=INITENABLE=0",
    "SET IOM3   CONFIG=HALFSIZE=0",

    "SET IOM3 CONFIG=PORT=2",
    "SET IOM3   CONFIG=ENABLE=0",
    "SET IOM3 CONFIG=PORT=3",
    "SET IOM3   CONFIG=ENABLE=0",
    "SET IOM3 CONFIG=PORT=4",
    "SET IOM3   CONFIG=ENABLE=0",
    "SET IOM3 CONFIG=PORT=5",
    "SET IOM3   CONFIG=ENABLE=0",
    "SET IOM3 CONFIG=PORT=6",
    "SET IOM3   CONFIG=ENABLE=0",
    "SET IOM3 CONFIG=PORT=7",
    "SET IOM3   CONFIG=ENABLE=0",
# endif

// SCU0

    "SET SCU0 CONFIG=MODE=program",
    "SET SCU0 CONFIG=PORT0=enable",
    "SET SCU0 CONFIG=PORT1=enable",
    "SET SCU0 CONFIG=PORT2=enable",
    "SET SCU0 CONFIG=PORT3=enable",
    "SET SCU0 CONFIG=PORT4=enable",
    "SET SCU0 CONFIG=PORT5=enable",
    "SET SCU0 CONFIG=PORT6=enable",
    "SET SCU0 CONFIG=PORT7=enable",
    "SET SCU0 CONFIG=MASKA=7",
    "SET SCU0 CONFIG=MASKB=off",
    "SET SCU0 CONFIG=LWRSTORESIZE=7",
    "SET SCU0 CONFIG=CYCLIC=0040",
    "SET SCU0 CONFIG=NEA=0200",
    "SET SCU0 CONFIG=ONL=014",
    "SET SCU0 CONFIG=INT=0",
    "SET SCU0 CONFIG=LWR=0",

// SCU1

    "SET SCU1 CONFIG=MODE=program",
    "SET SCU1 CONFIG=PORT0=enable",
    "SET SCU1 CONFIG=PORT1=enable",
    "SET SCU1 CONFIG=PORT2=enable",
    "SET SCU1 CONFIG=PORT3=enable",
    "SET SCU1 CONFIG=PORT4=enable",
    "SET SCU1 CONFIG=PORT5=enable",
    "SET SCU1 CONFIG=PORT6=enable",
    "SET SCU1 CONFIG=PORT7=enable",
    "SET SCU1 CONFIG=MASKA=off",
    "SET SCU1 CONFIG=MASKB=off",
    "SET SCU1 CONFIG=LWRSTORESIZE=7",
    "SET SCU1 CONFIG=CYCLIC=0040",
    "SET SCU1 CONFIG=NEA=0200",
    "SET SCU1 CONFIG=ONL=014",
    "SET SCU1 CONFIG=INT=0",
    "SET SCU1 CONFIG=LWR=0",

// SCU2

    "SET SCU2 CONFIG=MODE=program",
    "SET SCU2 CONFIG=PORT0=enable",
    "SET SCU2 CONFIG=PORT1=enable",
    "SET SCU2 CONFIG=PORT2=enable",
    "SET SCU2 CONFIG=PORT3=enable",
    "SET SCU2 CONFIG=PORT4=enable",
    "SET SCU2 CONFIG=PORT5=enable",
    "SET SCU2 CONFIG=PORT6=enable",
    "SET SCU2 CONFIG=PORT7=enable",
    "SET SCU2 CONFIG=MASKA=off",
    "SET SCU2 CONFIG=MASKB=off",
    "SET SCU2 CONFIG=LWRSTORESIZE=7",
    "SET SCU2 CONFIG=CYCLIC=0040",
    "SET SCU2 CONFIG=NEA=0200",
    "SET SCU2 CONFIG=ONL=014",
    "SET SCU2 CONFIG=INT=0",
    "SET SCU2 CONFIG=LWR=0",

// SCU3

    "SET SCU3 CONFIG=MODE=program",
    "SET SCU3 CONFIG=PORT0=enable",
    "SET SCU3 CONFIG=PORT1=enable",
    "SET SCU3 CONFIG=PORT2=enable",
    "SET SCU3 CONFIG=PORT3=enable",
    "SET SCU3 CONFIG=PORT4=enable",
    "SET SCU3 CONFIG=PORT5=enable",
    "SET SCU3 CONFIG=PORT6=enable",
    "SET SCU3 CONFIG=PORT7=enable",
    "SET SCU3 CONFIG=MASKA=off",
    "SET SCU3 CONFIG=MASKB=off",
    "SET SCU3 CONFIG=LWRSTORESIZE=7",
    "SET SCU3 CONFIG=CYCLIC=0040",
    "SET SCU3 CONFIG=NEA=0200",
    "SET SCU3 CONFIG=ONL=014",
    "SET SCU3 CONFIG=INT=0",
    "SET SCU3 CONFIG=LWR=0",

# if 0
// SCU4

    "SET SCU4 CONFIG=MODE=program",
    "SET SCU4 CONFIG=PORT0=enable",
    "SET SCU4 CONFIG=PORT1=enable",
    "SET SCU4 CONFIG=PORT2=enable",
    "SET SCU4 CONFIG=PORT3=enable",
    "SET SCU4 CONFIG=PORT4=enable",
    "SET SCU4 CONFIG=PORT5=enable",
    "SET SCU4 CONFIG=PORT6=enable",
    "SET SCU4 CONFIG=PORT7=enable",
    "SET SCU4 CONFIG=MASKA=off",
    "SET SCU4 CONFIG=MASKB=off",
    "SET SCU4 CONFIG=LWRSTORESIZE=7",
    "SET SCU4 CONFIG=CYCLIC=0040",
    "SET SCU4 CONFIG=NEA=0200",
    "SET SCU4 CONFIG=ONL=014",
    "SET SCU4 CONFIG=INT=0",
    "SET SCU4 CONFIG=LWR=0",

// SCU5

    "SET SCU5 CONFIG=MODE=program",
    "SET SCU5 CONFIG=PORT0=enable",
    "SET SCU5 CONFIG=PORT1=enable",
    "SET SCU5 CONFIG=PORT2=enable",
    "SET SCU5 CONFIG=PORT3=enable",
    "SET SCU5 CONFIG=PORT4=enable",
    "SET SCU5 CONFIG=PORT5=enable",
    "SET SCU5 CONFIG=PORT6=enable",
    "SET SCU5 CONFIG=PORT7=enable",
    "SET SCU5 CONFIG=MASKA=off",
    "SET SCU5 CONFIG=MASKB=off",
    "SET SCU5 CONFIG=LWRSTORESIZE=7",
    "SET SCU5 CONFIG=CYCLIC=0040",
    "SET SCU5 CONFIG=NEA=0200",
    "SET SCU5 CONFIG=ONL=014",
    "SET SCU5 CONFIG=INT=0",
    "SET SCU5 CONFIG=LWR=0",

// SCU6

    "SET SCU6 CONFIG=MODE=program",
    "SET SCU6 CONFIG=PORT0=enable",
    "SET SCU6 CONFIG=PORT1=enable",
    "SET SCU6 CONFIG=PORT2=enable",
    "SET SCU6 CONFIG=PORT3=enable",
    "SET SCU6 CONFIG=PORT4=enable",
    "SET SCU6 CONFIG=PORT5=enable",
    "SET SCU6 CONFIG=PORT6=enable",
    "SET SCU6 CONFIG=PORT7=enable",
    "SET SCU6 CONFIG=MASKA=off",
    "SET SCU6 CONFIG=MASKB=off",
    "SET SCU6 CONFIG=LWRSTORESIZE=7",
    "SET SCU6 CONFIG=CYCLIC=0040",
    "SET SCU6 CONFIG=NEA=0200",
    "SET SCU6 CONFIG=ONL=014",
    "SET SCU6 CONFIG=INT=0",
    "SET SCU6 CONFIG=LWR=0",

// SCU7

    "SET SCU7 CONFIG=MODE=program",
    "SET SCU7 CONFIG=PORT0=enable",
    "SET SCU7 CONFIG=PORT1=enable",
    "SET SCU7 CONFIG=PORT2=enable",
    "SET SCU7 CONFIG=PORT3=enable",
    "SET SCU7 CONFIG=PORT4=enable",
    "SET SCU7 CONFIG=PORT5=enable",
    "SET SCU7 CONFIG=PORT6=enable",
    "SET SCU7 CONFIG=PORT7=enable",
    "SET SCU7 CONFIG=MASKA=off",
    "SET SCU7 CONFIG=MASKB=off",
    "SET SCU7 CONFIG=LWRSTORESIZE=7",
    "SET SCU7 CONFIG=CYCLIC=0040",
    "SET SCU7 CONFIG=NEA=0200",
    "SET SCU7 CONFIG=ONL=014",
    "SET SCU7 CONFIG=INT=0",
    "SET SCU7 CONFIG=LWR=0",
# endif

    // ; DO NOT CHANGE THE CONFIGURATION OF THE FNP UNITS.
    // ; Restrictions in the FNP code that REQUIRE the simulator
    // ; unit numbers to be the same as the Multics unit numbers!
    // ; i.e. fnp0 == fnpa, etc.
    // ;
    // ; FNP a 3400
    // ; FNP b 3700
    // ; FNP c 4200
    // ; FNP d 4500
    // ; FNP e 5000
    // ; FNP f 5300
    // ; FNP g 5600
    // ; FNP h 6100

    "SET FNP0 CONFIG=MAILBOX=03400",
    "SET FNP0 IPC_NAME=fnp-a",
    "SET FNP1 CONFIG=MAILBOX=03700",
    "SET FNP1 IPC_NAME=fnp-b",
    "SET FNP2 CONFIG=MAILBOX=04200",
    "SET FNP2 IPC_NAME=fnp-c",
    "SET FNP3 CONFIG=MAILBOX=04500",
    "SET FNP3 IPC_NAME=fnp-d",
    "SET FNP4 CONFIG=MAILBOX=05000",
    "SET FNP4 IPC_NAME=fnp-e",
    "SET FNP5 CONFIG=MAILBOX=05300",
    "SET FNP5 IPC_NAME=fnp-f",
    "SET FNP6 CONFIG=MAILBOX=05600",
    "SET FNP6 IPC_NAME=fnp-g",
    "SET FNP7 CONFIG=MAILBOX=06100",
    "SET FNP7 IPC_NAME=fnp-h",

    //XXX"set mtp0 boot_drive=1",
    // ; Attach tape MPC to IOM 0, chan 012, dev_code 0
    "SET MTP0 BOOT_DRIVE=0",
    "SET MTP0 NAME=MTP0",
    // ; Attach TAPE unit 0 to IOM 0, chan 012, dev_code 1
    "CABLE IOM0 012 MTP0 0",
    "CABLE IOM1 012 MTP0 1",
    "CABLE MTP0 1 TAPE1",
    "SET TAPE1 NAME=tapa_01",
    "CABLE MTP0 2 TAPE2",
    "SET TAPE2 NAME=tapa_02",
    "CABLE MTP0 3 TAPE3",
    "SET TAPE3 NAME=tapa_03",
    "CABLE MTP0 4 TAPE4",
    "SET TAPE4 NAME=tapa_04",
    "CABLE MTP0 5 TAPE5",
    "SET TAPE5 NAME=tapa_05",
    "CABLE MTP0 6 TAPE6",
    "SET TAPE6 NAME=tapa_06",
    "CABLE MTP0 7 TAPE7",
    "SET TAPE7 NAME=tapa_07",
    "CABLE MTP0 8 TAPE8",
    "SET TAPE8 NAME=tapa_08",
    "CABLE MTP0 9 TAPE9",
    "SET TAPE9 NAME=tapa_09",
    "CABLE MTP0 10 TAPE10",
    "SET TAPE10 NAME=tapa_10",
    "CABLE MTP0 11 TAPE11",
    "SET TAPE11 NAME=tapa_11",
    "CABLE MTP0 12 TAPE12",
    "SET TAPE12 NAME=tapa_12",
    "CABLE MTP0 13 TAPE13",
    "SET TAPE13 NAME=tapa_13",
    "CABLE MTP0 14 TAPE14",
    "SET TAPE14 NAME=tapa_14",
    "CABLE MTP0 15 TAPE15",
    "SET TAPE15 NAME=tapa_15",
    "CABLE MTP0 16 TAPE16",
    "SET TAPE16 NAME=tapa_16",

// 4 3381 disks

    "SET IPC0 NAME=IPC0",
    "CABLE IOM0 013 IPC0 0",
    "CABLE IOM1 013 IPC0 1",
    // ; Attach DISK unit 0 to IPC0 dev_code 0",
    "CABLE IPC0 0 DISK0",
    "SET DISK0 TYPE=3381",
    "SET DISK0 NAME=dska_00",
    // ; Attach DISK unit 1 to IPC0 dev_code 1",
    "cable IPC0 1 DISK1",
    "set disk1 type=3381",
    "set disk1 name=dska_01",
    // ; Attach DISK unit 2 to IPC0 dev_code 2",
    "CABLE IPC0 2 DISK2",
    "SET DISK2 TYPE=3381",
    "SET DISK2 NAME=dska_02",
    // ; Attach DISK unit 3 to IPC0 dev_code 3",
    "CABLE IPC0 3 DISK3",
    "SET DISK3 TYPE=3381",
    "SET DISK3 NAME=dska_03",

// 4 d501 disks + 4 d451 disks + 2 d500 disks
    "SET MSP0 NAME=MSP0",
    "CABLE IOM0 014 MSP0 0",
    "CABLE IOM1 014 MSP0 1",

    // ; Attach DISK unit 4 to MSP0 dev_code 1",
    "CABLE MSP0 1 DISK4",
    "SET disk4 TYPE=d501",
    "SET disk4 NAME=dskb_01",
    // ; Attach DISK unit 5 to MSP0 dev_code 2",
    "CABLE MSP0 2 DISK5",
    "SET DISK5 TYPE=d501",
    "SET DISK5 NAME=dskb_02",
    // ; Attach DISK unit 6 to MSP0 dev_code 3",
    "CABLE MSP0 3 DISK6",
    "SET DISK6 TYPE=d501",
    "SET DISK6 NAME=dskb_03",
    // ; Attach DISK unit 7 to MSP0 dev_code 4",
    "CABLE MSP0 4 DISK7",
    "SET DISK7 TYPE=d501",
    "SET DISK7 NAME=dskb_04",

    // ; Attach DISK unit 8 to MSP0 dev_code 5",
    "CABLE MSP0 5 DISK8",
    "SET DISK8 TYPE=d451",
    "SET DISK8 NAME=dskb_05",
    // ; Attach DISK unit 9 to MSP0 dev_code 6",
    "CABLE MSP0 6 DISK9",
    "SET DISK9 TYPE=d451",
    "SET DISK9 NAME=dskb_06",
    // ; Attach DISK unit 10 to MSP0 dev_code 7",
    "CABLE MSP0 7 DISK10",
    "SET DISK10 TYPE=d451",
    "SET DISK10 NAME=dskb_07",
    // ; Attach DISK unit 11 to MSP0 dev_code 8",
    "CABLE MSP0 8 DISK11",
    "SET DISK11 TYPE=d451",
    "SET DISK11 NAME=dskb_08",
    // ; Attach DISK unit 12 to MSP0 dev_code 9",
    "CABLE MSP0 9 DISK12",
    "SET DISK12 TYPE=d500",
    "SET DISK12 NAME=dskb_09",
    // ; Attach DISK unit 13 to MSP0 dev_code 10",
    "CABLE MSP0 10 DISK13",
    "SET DISK13 TYPE=d500",
    "SET DISK13 NAME=dskb_10",

    // since we define 16 (decimal) 3381s in the default config deck, we need to add another 12
    // ; Attach DISK unit 14 to IPC0 dev_code 4",
    "CABLE IPC0 4 DISK14",
    "SET DISK14 TYPE=3381",
    "SET DISK14 NAME=dska_04",
    // ; Attach DISK unit 15 to IPC0 dev_code 5",
    "CABLE IPC0 5 DISK15",
    "SET DISK15 TYPE=3381",
    "SET DISK15 NAME=dska_05",
    // ; Attach DISK unit 16 to IPC0 dev_code 6",
    "CABLE IPC0 6 DISK16",
    "SET DISK16 TYPE=3381",
    "SET DISK16 NAME=dska_06",
    // ; Attach DISK unit 17 to IPC0 dev_code 7",
    "CABLE IPC0 7 DISK17",
    "SET DISK17 TYPE=3381",
    "SET DISK17 NAME=dska_07",
    // ; Attach DISK unit 18 to IPC0 dev_code 8",
    "CABLE IPC0 8 DISK18",
    "SET DISK18 TYPE=3381",
    "SET DISK18 NAME=dska_08",
    // ; Attach DISK unit 19 to IPC0 dev_code 9",
    "CABLE IPC0 9 DISK19",
    "SET DISK19 TYPE=3381",
    "SET DISK19 NAME=dska_09",
    // ; Attach DISK unit 20 to IPC0 dev_code 10",
    "CABLE IPC0 10 DISK20",
    "SET DISK20 TYPE=3381",
    "SET DISK20 NAME=dska_10",
    // ; Attach DISK unit 21 to IPC0 dev_code 11",
    "CABLE IPC0 11 DISK21",
    "SET DISK21 TYPE=3381",
    "SET DISK21 NAME=dska_11",
    // ; Attach DISK unit 22 to IPC0 dev_code 12",
    "CABLE IPC0 12 DISK22",
    "SET DISK22 TYPE=3381",
    "SET DISK22 NAME=dska_12",
    // ; Attach DISK unit 23 to IPC0 dev_code 13",
    "CABLE IPC0 13 DISK23",
    "SET DISK23 TYPE=3381",
    "SET DISK23 NAME=dska_13",
    // ; Attach DISK unit 24 to IPC0 dev_code 14",
    "CABLE IPC0 14 DISK24",
    "SET DISK24 TYPE=3381",
    "SET DISK24 NAME=dska_14",
    // ; Attach DISK unit 25 to IPC0 dev_code 15",
    "CABLE IPC0 15 DISK25",
    "SET DISK25 TYPE=3381",
    "SET DISK25 NAME=dska_15",

    // ; Attach OPC unit 0 to IOM A, chan 036, dev_code 0
    "CABLE IOMA 036 OPC0",
    // ; Attach OPC unit 1 to IOM A, chan 053, dev_code 0
    "CABLE IOMA 053 OPC1",
    "SET OPC1 CONFIG=MODEL=m6601",
    // No "devices" for console, so no 'CABLE OPC0 # CONx', etc.

    // ;;;
    // ;;; FNP
    // ;;;

    // ; Attach FNP unit 3 (d) to IOM A, chan 020, dev_code 0
    "CABLE IOMA 020 FNPD",
    // ; Attach FNP unit 0 (a) to IOM A, chan 021, dev_code 0
    "CABLE IOMA 021 FNPA",
    // ; Attach FNP unit 1 (b) to IOM A, chan 022, dev_code 0
    "CABLE IOMA 022 FNPB",
    // ; Attach FNP unit 2 (c) to IOM A, chan 023, dev_code 0
    "CABLE IOMA 023 FNPC",
    // ; Attach FNP unit 4 (e) to IOM A, chan 024, dev_code 0
    "CABLE IOMA 024 FNPE",
    // ; Attach FNP unit 5 (f) to IOM A, chan 025, dev_code 0
    "CABLE IOMA 025 FNPF",
    // ; Attach FNP unit 6 (g) to IOM A, chan 026, dev_code 0
    "CABLE IOMA 026 FNPG",
    // ; Attach FNP unit 7 (h) to IOM A, chan 027, dev_code 0
    "CABLE IOMA 027 FNPH",

    // ;;;
    // ;;; MPC
    // ;;;

    // ; Attach MPC unit 0 to IOM 0, char 015, dev_code 0
    "CABLE IOM0 015 URP0",
    "SET URP0 NAME=urpa",

    // ; Attach RDR unit 0 to IOM 0, chan 015, dev_code 1
    "CABLE URP0 1 RDR0",
    "SET RDR0 NAME=rdra",

    // ; Attach MPC unit 1 to IOM 0, char 016, dev_code 0
    "CABLE IOM0 016 URP1",
    "SET URP1 NAME=urpb",

    // ; Attach PUN unit 0 to IOM 0, chan 016, dev_code 1
    "CABLE URP1 1 PUN0",
    "SET PUN0 NAME=puna",

    // ; Attach MPC unit 2 to IOM 0, char 017, dev_code 0
    "CABLE IOM0 017 URP2",
    "SET URP2 NAME=urpc",

    // ; Attach PRT unit 0 to IOM 0, chan 017, dev_code 1
    "CABLE URP2 1 PRT0",
    "SET PRT0 NAME=prta",
//    "SET PRT0 MODEL=1600",    // Needs polts fixes

    // ; Attach MPC unit 3 to IOM 0, char 050, dev_code 0
    "CABLE IOMA 050 URP3",
    "SET URP3 NAME=urpd",

    // ; Attach PRT unit 1 to IOM 0, chan 050, dev_code 1
    "CABLE URP3 1 PRT1",
    "SET PRT1 NAME=prtb",
//    "SET PRT1 MODEL=303",    // Needs polts fixes

    // ; Attach MPC unit 4 to IOM 0, char 051, dev_code 0
    "CABLE IOMA 051 URP4",
    "SET URP4 NAME=urpe",

    // ; Attach PRT unit 2 to IOM 0, chan 051, dev_code 1
    "CABLE URP4 1 PRT2",
    "SET PRT2 NAME=prtc",
//    "SET PRT2 MODEL=300",    // Needs polts fixes

    // ; Attach MPC unit 5 to IOM 0, chan 052, dev_code 0
    "CABLE IOMA 052 URP5",
    "SET URP5 NAME=urpf",

    // ; Attach PRT unit 3 to IOM 0, chan 052, dev_code 1
    "CABLE URP5 1 PRT3",
    "SET PRT3 NAME=prtd",
//    "SET PRT3 MODEL=202",    // Needs polts fixes

    // ; Attach MPC unit 6 to IOM 0, chan 055, dev_code 0
    "CABLE IOMA 055 URP6",
    "SET URP6 NAME=urpg",

    // ; Attach RDR unit 1 to IOM 0, chan 055, dev_code 1
    "CABLE URP6 1 RDRB",
    "SET RDR1 NAME=rdrb",

    // ; Attach MPC unit 7 to IOM 0, chan 056, dev_code 0
    "CABLE IOMA 056 URP7",
    "SET URP7 NAME=urph",

    // ; Attach RDR unit 2 to IOM 0, chan 056, dev_code 1
    "CABLE URP7 1 RDRC",
    "SET RDR2 NAME=rdrc",

    // ; Attach MPC unit 8 to IOM 0, chan 057, dev_code 0
    "CABLE IOMA 057 URP8",
    "SET URP8 NAME=urpi",

    // ; Attach PUN unit 1 to IOM 0, chan 057, dev_code 1
    "CABLE URP8 1 PUNB",
    "SET PUN1 NAME=punb",

    // ; Attach MPC unit 9 to IOM 0, chan 060, dev_code 0
    "CABLE IOMA 060 URP9",
    "SET URP9 NAME=urpj",

    // ; Attach PUN unit 2 to IOM 0, chan 060, dev_code 1
    "CABLE URP9 1 PUNC",
    "SET PUN2 NAME=punc",

# ifdef WITH_SOCKET_DEV
#  ifndef __MINGW64__
#   ifndef __MINGW32__
#    ifndef CROSS_MINGW32
#     ifndef CROSS_MINGW64
    "CABLE IOMA 040 SKCA",
    "CABLE IOMA 041 SKCB",
    "CABLE IOMA 042 SKCC",
    "CABLE IOMA 043 SKCD",
    "CABLE IOMA 044 SKCE",
    "CABLE IOMA 045 SKCF",
    "CABLE IOMA 046 SKCG",
    "CABLE IOMA 047 SKCH",
#     endif /* ifndef CROSS_MINGW64 */
#    endif /* ifndef CROSS_MINGW32 */
#   endif /* ifndef __MINGW32__ */
#  endif /* ifndef __MINGW64__ */
# endif /* ifdef WITH_SOCKET_DEV */

# if 0
    // ; Attach PRT unit 1 to IOM 0, chan 017, dev_code 2
    "SET PRT1 NAME=prtb",
    "CABLE URP2 2 PRT1",

    // ; Attach PRT unit 2 to IOM 0, chan 017, dev_code 3
    "SET PRT2 NAME=prtc",
    "CABLE URP2 3 PRT2",

    // ; Attach PRT unit 3 to IOM 0, chan 017, dev_code 4
    "CABLE URP2 4 PRT3",
    "SET PRT3 NAME=prtd",

    // ; Attach PRT unit 4 to IOM 0, chan 017, dev_code 5
    "CABLE URP2 5 PRT4",
    "SET PRT4 NAME=prte",

    // ; Attach PRT unit 5 to IOM 0, chan 017, dev_code 6
    "CABLE URP2 6 PRT5",
    "SET PRT5 NAME=prtf",

    // ; Attach PRT unit 6 to IOM 0, chan 017, dev_code 7
    "CABLE URP2 7 PRT6",
    "SET PRT6 NAME=prtg",

    // ; Attach PRT unit 7 to IOM 0, chan 017, dev_code 8
    "CABLE URP2 8 PRT7",
    "SET PRT7 NAME=prth",

    // ; Attach PRT unit 8 to IOM 0, chan 017, dev_code 9
    "CABLE URP2 9 PRT8",
    "SET PRT8 NAME=prti",

    // ; Attach PRT unit 9 to IOM 0, chan 017, dev_code 10
    "CABLE URP2 10 PRT9",
    "SET PRT9 NAME=prtj",

    // ; Attach PRT unit 10 to IOM 0, chan 017, dev_code 11
    "CABLE URP2 11 PRT10",
    "SET PRT10 NAME=prtk",

    // ; Attach PRT unit 11 to IOM 0, chan 017, dev_code 12
    "CABLE URP2 12 PRT11",
    "SET PRT11 NAME=prtl",

    // ; Attach PRT unit 12 to IOM 0, chan 017, dev_code 13
    "CABLE URP2 13 PRT12",
    "SET PRT12 NAME=prtm",

    // ; Attach PRT unit 13 to IOM 0, chan 017, dev_code 14
    "CABLE URP2 14 PRT13",
    "SET PRT13 NAME=prtn",

    // ; Attach PRT unit 14 to IOM 0, chan 017, dev_code 15
    "CABLE URP2 15 PRT14",
    "SET PRT14 NAME=prto",

    // ; Attach PRT unit 15 to IOM 0, chan 017, dev_code 16
    "SET PRT15 NAME=prtp",

    // ; Attach PRT unit 16 to IOM 0, chan 017, dev_code 17
    "SET PRT16 NAME=prtq",
# endif

# ifdef WITH_ABSI_DEV
#  ifndef __MINGW64__
#   ifndef __MINGW32__
#    ifndef CROSS_MINGW64
#     ifndef CROSS_MINGW32
    // ; Attach ABSI unit 0 to IOM 0, chan 032, dev_code 0
    "CABLE IOM0 032 ABSI0",
#     endif /* CROSS_MINGW32 */
#    endif /* CROSS_MINGW64 */
#   endif /* __MINGW32__ */
#  endif /* __MINGW64__ */
# endif /* ifdef WITH_ABSI_DEV */

# ifdef WITH_MGP_DEV
#  ifndef __MINGW64__
#   ifndef __MINGW32__
#    ifndef CROSS_MINGW64
#     ifndef CROSS_MINGW32
    // ; Attach MGPR unit 0 to IOM 0, chan 033, dev_code 0
    "CABLE IOM0 033 MGP0",
    // ; Attach MGPW unit 1 to IOM 0, chan 034, dev_code 0
    "CABLE IOM0 034 MGP1",
#     endif /* CROSS_MINGW32 */
#    endif /* CROSS_MINGW64 */
#   endif /* __MINGW32__ */
#  endif /* __MINGW64__ */
# endif /* ifdef WITH_MGP_DEV */

    // ; Attach IOM unit 0 port A (0) to SCU unit 0, port 0
    "CABLE SCU0 0 IOM0 0", // SCU0 port 0 IOM0 port 0

    // ; Attach IOM unit 0 port B (1) to SCU unit 1, port 0
    "CABLE SCU1 0 IOM0 1", // SCU1 port 0 IOM0 port 1

    // ; Attach IOM unit 0 port C (2) to SCU unit 2, port 0
    "CABLE SCU2 0 IOM0 2", // SCU2 port 0 IOM0 port 2

    // ; Attach IOM unit 0 port D (3) to SCU unit 3, port 0
    "CABLE SCU3 0 IOM0 3", // SCU3 port 0 IOM0 port 3

    // ; Attach IOM unit 1 port A (0) to SCU unit 0, port 1
    "CABLE SCU0 1 IOM1 0", // SCU0 port 0 IOM0 port 0

    // ; Attach IOM unit 1 port B (1) to SCU unit 1, port 1
    "CABLE SCU1 1 IOM1 1", // SCU1 port 0 IOM0 port 1

    // ; Attach IOM unit 1 port C (2) to SCU unit 2, port 1
    "CABLE SCU2 1 IOM1 2", // SCU2 port 0 IOM0 port 2

    // ; Attach IOM unit 1 port D (3) to SCU unit 3, port 1
    "CABLE SCU3 1 IOM1 3", // SCU3 port 0 IOM0 port 3

// SCU0 --> CPU0-5

    // ; Attach SCU unit 0 port 7 to CPU unit A (1), port 0
    "CABLE SCU0 7 CPU0 0",

    // ; Attach SCU unit 0 port 6 to CPU unit B (1), port 0
    "CABLE SCU0 6 CPU1 0",

    // ; Attach SCU unit 0 port 5 to CPU unit C (2), port 0
    "CABLE SCU0 5 CPU2 0",

    // ; Attach SCU unit 0 port 4 to CPU unit D (3), port 0
    "CABLE SCU0 4 CPU3 0",

    // ; Attach SCU unit 0 port 3 to CPU unit E (4), port 0
    "CABLE SCU0 3 CPU4 0",

    // ; Attach SCU unit 0 port 2 to CPU unit F (5), port 0
    "CABLE SCU0 2 CPU5 0",

// SCU1 --> CPU0-5

    // ; Attach SCU unit 1 port 7 to CPU unit A (1), port 1
    "CABLE SCU1 7 CPU0 1",

    // ; Attach SCU unit 1 port 6 to CPU unit B (1), port 1
    "CABLE SCU1 6 CPU1 1",

    // ; Attach SCU unit 1 port 5 to CPU unit C (2), port 1
    "CABLE SCU1 5 CPU2 1",

    // ; Attach SCU unit 1 port 4 to CPU unit D (3), port 1
    "CABLE SCU1 4 CPU3 1",

    // ; Attach SCU unit 1 port 3 to CPU unit E (4), port 0
    "CABLE SCU1 3 CPU4 1",

    // ; Attach SCU unit 1 port 2 to CPU unit F (5), port 0
    "CABLE SCU1 2 CPU5 1",

// SCU2 --> CPU0-5

    // ; Attach SCU unit 2 port 7 to CPU unit A (1), port 2
    "CABLE SCU2 7 CPU0 2",

    // ; Attach SCU unit 2 port 6 to CPU unit B (1), port 2
    "CABLE SCU2 6 CPU1 2",

    // ; Attach SCU unit 2 port 5 to CPU unit C (2), port 2
    "CABLE SCU2 5 CPU2 2",

    // ; Attach SCU unit 2 port 4 to CPU unit D (3), port 2
    "CABLE SCU2 4 CPU3 2",

    // ; Attach SCU unit 2 port 3 to CPU unit E (4), port 0
    "CABLE SCU2 3 CPU4 2",

    // ; Attach SCU unit 2 port 2 to CPU unit F (5), port 0
    "CABLE SCU2 2 CPU5 2",

// SCU3 --> CPU0-5

    // ; Attach SCU unit 3 port 7 to CPU unit A (1), port 3
    "CABLE SCU3 7 CPU0 3",

    // ; Attach SCU unit 3 port 6 to CPU unit B (1), port 3
    "CABLE SCU3 6 CPU1 3",

    // ; Attach SCU unit 3 port 5 to CPU unit C (2), port 3
    "CABLE SCU3 5 CPU2 3",

    // ; Attach SCU unit 3 port 4 to CPU unit D (3), port 3
    "CABLE SCU3 4 CPU3 3",

    // ; Attach SCU unit 3 port 3 to CPU unit E (4), port 0
    "CABLE SCU3 3 CPU4 3",

    // ; Attach SCU unit 3 port 2 to CPU unit F (5), port 0
    "CABLE SCU3 2 CPU5 3",

    "SET CPU0 RESET",
    "SET SCU0 RESET",
    "SET SCU1 RESET",
    "SET SCU2 RESET",
    "SET SCU3 RESET",
    "SET IOM0 RESET",

# if defined(THREADZ) || defined(LOCKLESS)
    "SET CPU NUNITS=6",
# else
#  ifdef ROUND_ROBIN
    "SET CPU NUNITS=6",
#  else
    "SET CPU NUNITS=1",
#  endif
# endif // THREADZ
    // "SET SYS CONFIG=ACTIVATE_TIME=8",
    // "SET SYS CONFIG=TERMINATE_TIME=8",
    "SET SYS CONFIG=CONNECT_TIME=-1",
  }; // default_base_system_script

void print_default_base_system_script (void)
  {
    int n_lines = sizeof (default_base_system_script) / sizeof (char *);
    sim_printf ("; DEFAULT_BASE_SYSTEM_SCRIPT (%lu lines follow)\n",
            (unsigned long)n_lines);
    for (int line = 0; line < n_lines; line ++)
    sim_printf ("%s\n", default_base_system_script[line]);
  }

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
    int n = 010000; //-V536
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
    sim_msg ("Debug MME countdown set to %llu\n", (long long unsigned int)sim_deb_mme_cntdwn);
    return SCPE_OK;
  }

static t_stat dps_debug_skip (UNUSED int32 arg, const char * buf)
  {
    sim_deb_skip_cnt   = 0;
    sim_deb_skip_limit = strtoull (buf, NULL, 0);
    sim_msg ("Debug skip set to %llu\n", (long long unsigned int)sim_deb_skip_limit);
    return SCPE_OK;
  }

static t_stat dps_debug_start (UNUSED int32 arg, const char * buf)
  {
    sim_deb_start = strtoull (buf, NULL, 0);
    sim_msg ("Debug set to start at cycle: %llu\n", (long long unsigned int)sim_deb_start);
    return SCPE_OK;
  }

static t_stat dps_debug_stop (UNUSED int32 arg, const char * buf)
  {
    sim_deb_stop = strtoull (buf, NULL, 0);
    sim_msg ("Debug set to stop at cycle: %llu\n", (long long unsigned int)sim_deb_stop);
    return SCPE_OK;
  }

static t_stat dps_debug_break (UNUSED int32 arg, const char * buf)
  {
    sim_deb_break = strtoull (buf, NULL, 0);
    if (buf[0] == '+')
      sim_deb_break += sim_deb_start;
    sim_msg ("Debug set to break at cycle: %llu\n", (long long unsigned int)sim_deb_break);
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
        sim_msg ("Debug set for segno %llo %llu.\n",
                (long long unsigned int)segno, (long long unsigned int) segno);
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
    sim_msg ("Debug set to ringno %llo\n", (long long unsigned int)sim_deb_ringno);
    return SCPE_OK;
  }

static t_stat dps_debug_bar (int32 arg, UNUSED const char * buf)
  {
    sim_deb_bar = arg;
    if (arg)
      sim_msg ("Debug set BAR %llo\n", (long long unsigned int)sim_deb_ringno);
    else
      sim_msg ("Debug unset BAR %llo\n", (long long unsigned int)sim_deb_ringno);
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
    if (!book_segments[n_book_segments].segname)
      {
        fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                 __func__, __FILE__, __LINE__);
# if defined(USE_BACKTRACE)
#  ifdef SIGUSR2
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
#  endif /* ifdef SIGUSR2 */
# endif /* if defined(USE_BACKTRACE) */
        abort();
      }
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
    if (!book_components[n_book_components].compname)
      {
        fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                 __func__, __FILE__, __LINE__);
# if defined(USE_BACKTRACE)
#  ifdef SIGUSR2
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
#  endif /* ifdef SIGUSR2 */
# endif /* if defined(USE_BACKTRACE) */
        abort();
      }
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
                        if (! (cnt == 2 || cnt == 4  || cnt == 6  ||
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
                            "%012llo (%llu)\n",
                            argno, argSegno, argOffset, argnop,
                            (unsigned long long int)argv, (unsigned long long int)argv);
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
        (void)strcpy (w3, buf + colon + 1 + plus + 1);
      }
    else
      {
        (void)strcpy (w2, buf + colon + 1);
        (void)strcpy (w3, "");
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
      FREE (source_search_path);
    source_search_path = strdup (buf);
    if (!source_search_path)
      {
        fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                 __func__, __FILE__, __LINE__);
#  if defined(USE_BACKTRACE)
#   ifdef SIGUSR2
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
#   endif /* ifdef SIGUSR2 */
#  endif /* if defined(USE_BACKTRACE) */
        abort();
      }
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
    if (sscanf (buf, "%llo", & value) != 1)
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
      return signal_tape (i, 0, 020 /* tape drive to ready */); //-V536
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

    {"DEFAULT_BASE_SYSTEM", set_default_base_system,  0, "Set configuration to defaults\n",             NULL, NULL},

    {"CABLE",               sys_cable,                0, "String a cable.\n",                           NULL, NULL},
    {"UNCABLE",             sys_cable,                1, "Unstring a cable.\n",                         NULL, NULL},
    {"CABLE_RIPOUT",        sys_cable_ripout,         0, "Remove all cables from the configuration.\n", NULL, NULL},
    {"CABLE_SHOW",          sys_cable_show,           0, "Show the current cabling configuration.\n",   NULL, NULL},

    {"FNPSERVERPORT",       set_fnp_server_port,      0, "Set the FNP dialin TELNET port number\n",     NULL, NULL},
    {"FNPSERVERADDRESS",    set_fnp_server_address,   0, "Set the FNP dialin server binding address\n", NULL, NULL},
    {"FNPSERVER3270PORT",   set_fnp_3270_server_port, 0, "Set the FNP TN3270 dialin port number\n",     NULL, NULL},

//
// System control
//

    {"SKIPBOOT",  boot_skip,                     0, "Skip forward on boot tape\n",                                    NULL, NULL},
    {"FNPSTART",  fnp_start,                     0, "Directs the simulator to immediately start listening for FNP connections.\n",
                                                                                                                      NULL, NULL},
    {"MOUNT",     mount_tape,                    0, "Mount tape image and signal Multics\n",                          NULL, NULL},
    {"LOAD",      load_media,                    1, "Mount disk or tape image and signal Multics\n",                  NULL, NULL},
    {"UNLOAD",    load_media,                    0, "Unmount disk or tape image and signal Multics\n",                NULL, NULL},
    {"READY",     ready_media,                   0, "Signal Multics that media is ready\n",                           NULL, NULL},
    {"REWIND",    rewind_media,                  0, "Rewind tape\n",                                                  NULL, NULL},
    {"XF",        do_execute_fault,              0, "Execute fault: Press the execute fault button\n",                NULL, NULL},
    {"RESTART",   do_restart,                    0, "Execute fault: Press the execute fault button\n",                NULL, NULL},
    {"POLL",      set_sys_polling_interval,      0, "Set polling interval (in milliseconds)\n",                       NULL, NULL},
    {"SLOWPOLL",  set_sys_slow_polling_interval, 0, "Set slow polling interval (in polling intervals).\n",            NULL, NULL},
    {"CHECKPOLL", set_sys_poll_check_rate,       0, "Set polling check rate (in polling intervals).\n",               NULL, NULL},
    {"BURST",     burst_printer,                 0, "Burst process output from printer.\n",                           NULL, NULL},

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

    {"AUTOINPUT",     add_opc_autoinput,   0, "Set primary console auto-input\n",     NULL, NULL},
    {"AI",            add_opc_autoinput,   0, "Set primary console auto-input\n",     NULL, NULL},
    {"AUTOINPUT2",    add_opc_autoinput,   1, "Set secondary console auto-input\n",   NULL, NULL},
    {"AI2",           add_opc_autoinput,   1, "Set secondary console auto-input\n",   NULL, NULL},
    {"CLRAUTOINPUT",  clear_opc_autoinput, 0, "Clear primary console auto-input\n",   NULL, NULL},
    {"CLRAUTOINPUT2", clear_opc_autoinput, 1, "Clear secondary console auto-input\n", NULL, NULL},

//
// Tuning
//

# if YIELD
    {"CLEAR_YIELD",   clear_yield,         1, "Clear yield data points\n",            NULL, NULL},
    {"YIELD",         yield,               1, "Define yield data point\n",            NULL, NULL},
# endif

//
// Hacks
//

    {"LUF",           set_luf,             1, "Enable normal LUF handling\n",         NULL, NULL},
    {"NOLUF",         set_luf,             0, "Disable normal LUF handling\n",        NULL, NULL},

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
    { "commit_id",              SYM_STATE_OFFSET,  SYM_STRING,    offsetof (struct system_state_s, commit_id)   },
    { "M[]",                    SYM_STATE_OFFSET,  SYM_ARRAY,     offsetof (struct system_state_s, M)           },
    { "sizeof(*M)",             SYM_STRUCT_SZ,     SYM_SZ,        sizeof (word36) },

    { "cpus[]",                 SYM_STATE_OFFSET,  SYM_ARRAY,     offsetof (struct system_state_s, cpus)        },
    { "sizeof(*cpus)",          SYM_STRUCT_SZ,     SYM_SZ,        sizeof (cpu_state_t) },

    { "cpus[].PPR",             SYM_STRUCT_OFFSET, SYM_PTR,       offsetof (cpu_state_t,           PPR)         },
    { "cpus[].PPR.PRR",         SYM_STRUCT_OFFSET, SYM_UINT8_3,   offsetof (struct ppr_s,          PRR)         },
    { "cpus[].PPR.PSR",         SYM_STRUCT_OFFSET, SYM_UINT16_15, offsetof (struct ppr_s,          PSR)         },
    { "cpus[].PPR.P",           SYM_STRUCT_OFFSET, SYM_UINT8_1,   offsetof (struct ppr_s,          P)           },
    { "cpus[].PPR.IC",          SYM_STRUCT_OFFSET, SYM_UINT32_18, offsetof (struct ppr_s,          IC)          },

    { "cpus[].cu",              SYM_STRUCT_OFFSET, SYM_PTR,       offsetof (cpu_state_t,           cu)          },
    { "cpus[].cu.IWB",          SYM_STRUCT_OFFSET, SYM_UINT64_36, offsetof (ctl_unit_data_t,       IWB)         },
    { "cpus[].cu.IR",           SYM_STRUCT_OFFSET, SYM_UINT32_18, offsetof (ctl_unit_data_t,       IR)          },

    { "cpus[].rA",              SYM_STRUCT_OFFSET, SYM_UINT64_36, offsetof (cpu_state_t,           rA)          },

    { "cpus[].rQ",              SYM_STRUCT_OFFSET, SYM_UINT64_36, offsetof (cpu_state_t,           rQ)          },

    { "cpus[].rE",              SYM_STRUCT_OFFSET, SYM_UINT64_36, offsetof (cpu_state_t,           rE)          },

    { "cpus[].rX[]",            SYM_STRUCT_OFFSET, SYM_ARRAY,     offsetof (cpu_state_t,           rX)          },
    { "sizeof(*rX)",            SYM_STRUCT_SZ,     SYM_SZ,        sizeof (word18) },
    { "cpus[].rX",              SYM_STRUCT_OFFSET, SYM_UINT32_18, 0 },

    { "cpus[].rTR",             SYM_STRUCT_OFFSET, SYM_UINT32_27, offsetof (cpu_state_t,           rTR)         },

    { "cpus[].rRALR",           SYM_STRUCT_OFFSET, SYM_UINT8_3,   offsetof (cpu_state_t,           rRALR)       },

    { "cpus[].PAR[]",           SYM_STRUCT_OFFSET, SYM_ARRAY,     offsetof (cpu_state_t,           PAR)         },
    { "sizeof(*PAR)",           SYM_STRUCT_SZ,     SYM_SZ,        sizeof (struct par_s) },

    { "cpus[].PAR[].SNR",       SYM_STRUCT_OFFSET, SYM_UINT16_15, offsetof (struct par_s,          SNR)         },
    { "cpus[].PAR[].RNR",       SYM_STRUCT_OFFSET, SYM_UINT8_3,   offsetof (struct par_s,          RNR)         },
    { "cpus[].PAR[].PR_BITNO",  SYM_STRUCT_OFFSET, SYM_UINT8_6,   offsetof (struct par_s,          PR_BITNO)    },
    { "cpus[].PAR[].WORDNO",    SYM_STRUCT_OFFSET, SYM_UINT32_18, offsetof (struct par_s,          WORDNO)      },

    { "cpus[].BAR",             SYM_STRUCT_OFFSET, SYM_PTR,       offsetof (cpu_state_t,           BAR)         },
    { "cpus[].BAR.BASE",        SYM_STRUCT_OFFSET, SYM_UINT16_9,  offsetof (struct bar_s,          BASE)        },
    { "cpus[].BAR.BOUND",       SYM_STRUCT_OFFSET, SYM_UINT16_9,  offsetof (struct bar_s,          BOUND)       },

    { "cpus[].TPR",             SYM_STRUCT_OFFSET, SYM_PTR,       offsetof (cpu_state_t,           TPR)         },
    { "cpus[].TPR.TRR",         SYM_STRUCT_OFFSET, SYM_UINT8_3,   offsetof (struct tpr_s,          TRR)         },
    { "cpus[].TPR.TSR",         SYM_STRUCT_OFFSET, SYM_UINT16_15, offsetof (struct tpr_s,          TSR)         },
    { "cpus[].TPR.TBR",         SYM_STRUCT_OFFSET, SYM_UINT8_6,   offsetof (struct tpr_s,          TBR)         },
    { "cpus[].TPR.CA",          SYM_STRUCT_OFFSET, SYM_UINT32_18, offsetof (struct tpr_s,          CA)          },

    { "cpus[].DSBR",            SYM_STRUCT_OFFSET, SYM_PTR,       offsetof (cpu_state_t,           DSBR)        },
    { "cpus[].DSBR.ADDR",       SYM_STRUCT_OFFSET, SYM_UINT32_24, offsetof (struct dsbr_s,         ADDR)        },
    { "cpus[].DSBR.BND",        SYM_STRUCT_OFFSET, SYM_UINT16_14, offsetof (struct dsbr_s,         BND)         },
    { "cpus[].DSBR.U",          SYM_STRUCT_OFFSET, SYM_UINT8_1,   offsetof (struct dsbr_s,         U)           },
    { "cpus[].DSBR.STACK",      SYM_STRUCT_OFFSET, SYM_UINT16_12, offsetof (struct dsbr_s,         STACK)       },

    { "cpus[].faultNumber",     SYM_STRUCT_OFFSET, SYM_UINT32,    offsetof (cpu_state_t,           faultNumber) },
# define SYMTAB_ENUM32(e) { #e, SYM_ENUM,          SYM_UINT32,    e }
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
  memcpy (system_state->symbolTable.symbols, symbols, sizeof (symbols)); //-V1086
}
#endif /* ifndef PERF_STRIP */

static inline uint32_t
hash32s(const void *buf, size_t len, uint32_t h)
{
  const unsigned char *p = buf;

  for (size_t i = 0; i < len; i++)
    h = h * 31 + p[i];

  h ^= h >> 17;
  h *= UINT32_C(0xed5ad4bb);
  h ^= h >> 11;
  h *= UINT32_C(0xac4c1b51);
  h ^= h >> 15;
  h *= UINT32_C(0x31848bab);
  h ^= h >> 14;

  return h;
}

// Once-only initialization; invoked via SCP

static void dps8_init (void) {
  int st1ret;
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

/* TESTING */
# ifdef TESTING
    sim_msg ("\n Options: ");
#  ifndef HAVE_DPSOPT
#   define HAVE_DPSOPT 1
#  endif
    sim_msg ("TESTING");
# endif /* ifdef TESTING */

/* ISOLTS */
# ifdef ISOLTS
#  ifdef HAVE_DPSOPT
    sim_msg (", ");
#  else
    sim_msg ("\n Options: ");
#  endif
#  ifndef HAVE_DPSOPT
#   define HAVE_DPSOPT 1
#  endif
    sim_msg ("ISOLTS");
# endif /* ifdef ISOLTS */

/* NEED_128 */
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
# endif /* ifdef NEED_128 */

/* NO_UCACHE */
# ifdef NO_UCACHE
#  ifdef HAVE_DPSOPT
    sim_msg (", ");
#  else
    sim_msg ("\n Options: ");
#  endif
#  ifndef HAVE_DPSOPT
#   define HAVE_DPSOPT 1
#  endif
    sim_msg ("NO_UCACHE");
# endif /* ifdef NO_UCACHE */

/* WAM */
# ifdef WAM
#  ifdef HAVE_DPSOPT
    sim_msg (", ");
#  else
    sim_msg ("\n Options: ");
#  endif
#  ifndef HAVE_DPSOPT
#   define HAVE_DPSOPT 1
#  endif
    sim_msg ("WAM");
# endif /* ifdef WAM */

/* ROUND_ROBIN */
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
# endif /* ifdef ROUND_ROBIN */

/* NO_LOCKLESS */
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
# endif /* ifndef NO_LOCKLESS */

/* ABSI */  /* XXX: Change to NO_ABSI once code is non-experimental */
# ifdef WITH_ABSI_DEV
#  ifdef HAVE_DPSOPT
    sim_msg (", ");
#  else
    sim_msg ("\n Options: ");
#  endif
#  ifndef HAVE_DPSOPT
#   define HAVE_DPSOPT 1
#  endif
    sim_msg ("ABSI");
# endif /* ifdef WITH_ABSI_DEV */

/* SOCKET */  /* XXX: Change to NO_SOCKET once code is non-experimental */
# ifdef WITH_SOCKET_DEV
#  ifdef HAVE_DPSOPT
    sim_msg (", ");
#  else
    sim_msg ("\n Options: ");
#  endif
#  ifndef HAVE_DPSOPT
#   define HAVE_DPSOPT 1
#  endif
    sim_msg ("SOCKET");
# endif /* ifdef WITH_SOCKET_DEV */

/* CHAOSNET */  /* XXX: Change to NO_CHAOSNET once code is non-experimental */
# ifdef WITH_MGP_DEV
#  ifdef HAVE_DPSOPT
    sim_msg (", ");
#  else
    sim_msg ("\n Options: ");
#  endif
#  ifndef HAVE_DPSOPT
#   define HAVE_DPSOPT 1
#  endif
    sim_msg ("CHAOSNET");
#  if USE_SOCKET_DEV_APPROACH
    sim_msg ("*");
#  endif /* if USE_SOCKET_DEV_APPROACH */
# endif /* ifdef WITH_MGP_DEV */

/* DUMA */
# ifdef USE_DUMA
#  ifdef HAVE_DPSOPT
    sim_msg (", ");
#  else
    sim_msg ("\n Options: ");
#  endif
#  ifndef HAVE_DPSOPT
#   define HAVE_DPSOPT 1
#  endif
    sim_msg ("DUMA");
# endif /* ifdef USE_DUMA */

/* BACKTRACE */
# ifdef USE_BACKTRACE
#  ifdef HAVE_DPSOPT
    sim_msg (", ");
#  else
    sim_msg ("\n Options: ");
#  endif
#  ifndef HAVE_DPSOPT
#   define HAVE_DPSOPT 1
#  endif
    sim_msg ("BACKTRACE");
# endif /* ifdef USE_BACKTRACE */

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
# endif /* ifdef TESTING */

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
  struct timespec ts;

  char   rssuffix[24];
  memset(rssuffix, 0, 24);

  char   statenme[32];
  memset(statenme, 0, 32);

#ifdef MACOSXPPC
  (void)ts;
# undef USE_MONOTONIC
#endif /* ifdef MACOSXPPC */

#ifdef USE_MONOTONIC
  st1ret = clock_gettime(CLOCK_MONOTONIC, &ts);
#else
# ifdef MACOSXPPC
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts.tv_sec = mts.tv_sec;
  ts.tv_nsec = mts.tv_nsec;
# else
  st1ret = clock_gettime(CLOCK_REALTIME, &ts);
# endif /* ifdef MACOSXPPC */
#endif /* ifdef USE_MONOTONIC */
#ifdef MACOSXPPC
  st1ret = 0;
#endif /* ifdef MACOSXPPC */
  if (st1ret != 0)
    {
      fprintf (stderr, "\rFATAL: clock_gettime failure! Aborting at %s[%s:%d]\r\n",
               __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# ifdef SIGUSR2
      (void)raise(SIGUSR2);
      /*NOTREACHED*/ /* unreachable */
# endif /* ifdef SIGUSR2 */
#endif /* if defined(USE_BACKTRACE) */
      abort();
    }

  uint32_t h = 0;  /* initial hash value */
#if __STDC_VERSION__ < 201112L
  /* LINTED E_OLD_STYLE_FUNC_DECL */
  void *(*mallocptr)() = malloc;
  h = hash32s(&mallocptr, sizeof(mallocptr), h);
#endif /* if __STDC_VERSION__ < 201112L */
  void *small = malloc(1);
  h = hash32s(&small, sizeof(small), h);
  FREE(small);
  void *big = malloc(1UL << 20);
  h = hash32s(&big, sizeof(big), h);
  FREE(big);
  void *ptr = &ptr;
  h = hash32s(&ptr, sizeof(ptr), h);
  time_t t = time(0);
  h = hash32s(&t, sizeof(t), h);
#if !defined(_AIX)
  for (int i = 0; i < 1000; i++)
    {
      unsigned long counter = 0;
      clock_t start = clock();
      while (clock() == start)
        {
          counter++;
        }
      h = hash32s(&start, sizeof(start), h);
      h = hash32s(&counter, sizeof(counter), h);
    }
#endif /* if !defined(_AIX) */
  int mypid = (int)getpid();
  h = hash32s(&mypid, sizeof(mypid), h);
  char rnd[4];
  FILE *f = fopen("/dev/urandom", "rb");
  if (f)
    {
      if (fread(rnd, sizeof(rnd), 1, f))
        {
          h = hash32s(rnd, sizeof(rnd), h);
        }
      fclose(f);
    }
  srandom(h); /* seed rng */

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
    sim_warn ("\rFATAL: %s: aborting at %s[%s:%d]\r\n",
              strerror (svErrno),
              __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# ifdef SIGUSR2
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* ifdef SIGUSR2 */
#endif /* if defined(USE_BACKTRACE) */
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
      memset(system_state, 0, sizeof(*system_state));
      sim_warn ("NOTE: State hash mismatch; system state reset.\r\n");
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
# ifdef WITH_SOCKET_DEV
#  ifndef __MINGW64__
#   ifndef __MINGW32__
#    ifndef CROSS_MINGW64
#     ifndef CROSS_MINGW32
  sk_init ();
#     endif /* ifndef CROSS_MINGW32 */
#    endif /* ifndef CROSS_MINGW64 */
#   endif /* ifndef __MINGW64__ */
#  endif /* ifndef __MINGW32__ */
# endif /* ifdef WITH_SOCKET_DEV */
  fnpInit ();
  console_init (); // must come after fnpInit due to libuv initialization
 /* mpc_init (); */
  scu_init ();
  cpu_init ();
  rdr_init ();
  pun_init ();
  prt_init ();
  urp_init ();

# ifdef WITH_ABSI_DEV
#  ifndef __MINGW64__
#   ifndef __MINGW32__
#    ifndef CROSS_MINGW64
#     ifndef CROSS_MINGW32
  absi_init ();
#     endif /* CROSS_MINGW32 */
#    endif /* CROSS_MINGW64 */
#   endif /* ifndef __MINGW32__ */
#  endif /* ifndef __MINGW64__ */
# endif /* ifdef WITH_ABSI_DEV */

# ifdef WITH_MGP_DEV
#  ifndef __MINGW64__
#   ifndef __MINGW32__
#    ifndef CROSS_MINGW64
#     ifndef CROSS_MINGW32
  mgp_init ();
#     endif /* CROSS_MINGW32 */
#    endif /* CROSS_MINGW64 */
#   endif /* ifndef __MINGW32__ */
#  endif /* ifndef __MINGW64__ */
# endif /* ifdef WITH_MGP_DEV */

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
    { "pr0",   0 }, ///< pr0 - 7
    { "pr1",   1 },
    { "pr2",   2 },
    { "pr3",   3 },
    { "pr4",   4 },
    { "pr5",   5 },
    { "pr6",   6 },
    { "pr7",   7 },

    { "pr[0]", 0 }, ///< pr0 - 7
    { "pr[1]", 1 },
    { "pr[2]", 2 },
    { "pr[3]", 3 },
    { "pr[4]", 4 },
    { "pr[5]", 5 },
    { "pr[6]", 6 },
    { "pr[7]", 7 },

    // See: https://multicians.org/pg/mvm.html
    { "ap",    0 },
    { "ab",    1 },
    { "bp",    2 },
    { "bb",    3 },
    { "lp",    4 },
    { "lb",    5 },
    { "sp",    6 },
    { "sb",    7 },
    {    0,    0 }
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
        (void)strcpy(addspec, cptr);

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
                fprintf(ofile, " %012llo", (unsigned long long int)val[n + 1]);

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
#ifdef WITH_SOCKET_DEV
# ifndef __MINGW64__
#  ifndef __MINGW32__
#   ifndef CROSS_MINGW32
#    ifndef CROSS_MINGW64
    & skc_dev,
#    endif /* ifndef CROSS_MINGW64 */
#   endif /* ifndef CROSS_MINGW32 */
#  endif /* ifndef __MINGW32__ */
# endif /* ifndef __MINGW64__ */
#endif /* ifdef WITH_SOCKET_DEV */
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

#ifdef WITH_ABSI_DEV
# ifndef __MINGW64__
#  ifndef __MINGW32__
#   ifndef CROSS_MINGW32
#    ifndef CROSS_MINGW64
    & absi_dev,
#    endif /* ifndef CROSS_MINGW64 */
#   endif /* ifndef CROSS_MINGW32 */
#  endif /* ifndef __MINGW32__ */
# endif /* ifndef __MINGW64__ */
#endif /* ifdef WITH_ABSI_DEV */

#ifdef WITH_MGP_DEV
# ifndef __MINGW64__
#  ifndef __MINGW32__
#   ifndef CROSS_MINGW32
#    ifndef CROSS_MINGW64
    & mgp_dev,
#    endif /* ifndef CROSS_MINGW64 */
#   endif /* ifndef CROSS_MINGW32 */
#  endif /* ifndef __MINGW32__ */
# endif /* ifndef __MINGW64__ */
#endif /* ifdef WITH_MGP_DEV */

    NULL
  };

#ifdef PERF_STRIP
void dps8_init_strip (void)
  {
    dps8_init ();
  }
#endif
