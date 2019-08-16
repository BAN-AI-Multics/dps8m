/*
 Copyright (c) 2007-2013 Michael Mondy
 Copyright 2012-2016 by Harry Reed
 Copyright 2013-2018 by Charles Anthony

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

//
//  dps8_disk.c
//  dps8
//
//  Created by Harry Reed on 6/16/13.
//  Copyright (c) 2013 Harry Reed. All rights reserved.
//

// source/library_dir_dir/system_library_1/source/bound_volume_rldr_ut_.s.archive/rdisk_.pl1

// source/library_dir_dir/system_library_1/source/bound_rcp_.s.archive/rcp_disk_.pl1

#include <stdio.h>
#include <ctype.h>
#ifdef IO_LATENCY
#include <unistd.h>
#endif

#include "dps8.h"
#include "dps8_iom.h"
#include "dps8_disk.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "sim_disk.h"
#include "dps8_utils.h"
#include "threadz.h"

#ifdef PROFILER
#include "dps8_state.h"
#endif

#define DBG_CTR 1

/*
 disk.c -- disk drives
 
 Copyright (c) 2007-2013 Michael Mondy
 
 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at http://example.org/project/LICENSE.
 */


//
// A possible disk data packing algoritim
//
// Currently sectors are 512 * word36. Word36 is 64bits; that 56% utilization,
// A computationally effectent packing would improve disk throughput and storage
// efficency
//
//     word36 in [512]
//     struct dsksec
//       {
//         uint32 low32 [512];
//         uint8 high4 [512];
//       } out;
//     uint32 * map = (uint32 *) in;
//     uint8 * highmap = (uint8 *) in;
//
//     for (uint i = 0; i < 512; i ++)
//       {
//         out . low32 [i] = map [i * 2];
//         out . high4 [i] = (uint8) (map [i * 2 + 1]);
//       }
//
// This 36/40 -- 90% utilization; at the cost of the scatter/gather and
// the cost of emulated disk sectors size not a multilple of host disk
// sector size.
//

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
    uint sects_per_cyl; // not canonical, used for seek latency calculations
    useconds_t seek_latency;
    useconds_t xfer_latency;
  };

// dau_type stat_mpc_.pl1
//
//  dcl  ddev_model (0:223) char (6) static options (constant)
//          init ((84) (""), "  190A", (52) (""), "  190B", "   401", "  190B", (14) (""), "   451", (31) (""), "   402",
//          (13) (""), "   500", "   501", "   500", "   501", "   500", "   501", "   500", "   501", (9) (""), "   509",
//          "", "   509", "", "   509", "", "   509");

//  84   0:83   ""
//   1  84      "190A"
//  52  85:135  ""
//   1  137     "190B
//   1  138     "401"
//   1  139     "190B"
//  14  140:153 ""
//   1  154     "451"
//  31  155:185 ""
//   1  186     "402"
//  13  175:199 ""
//   1  200     "500"
//   1  201     "501"
//   1  202     "500"
//   1  203     "501"
//   1  204     "500"
//   1  205     "501"
//   1  206     "500"
//   1  207     "501"
//   9  208:216 ""
//   1  217     "509"
//   1  218     ""
//   1  219     "509"
//   1  220     ""
//   1  221     "509"
//   1  222     ""
//   1  223     "509"

// Latency
//
// http://www.extremetech.com/extreme/142911-ibm-3390-the-worlds-largest-and-most-expensive-hard-drive-teardown
// https://www.ibm.com/ibm/history/exhibits/storage/storage_3390.html
//
// The 3390 disks rotated faster than those in the 3380. Faster disk rotation
// reduced rotational delay, the time required for the correct area of the disk
// surface to move to the point where data could be read or written. In the
// 3390's initial models, the average rotational delay was reduced to 7.1
// milliseconds from 8.3 milliseconds for the 3380 family.
//
// 3390
//  4.2 MB/S data transfer rate
//  12.5 msec average seek time
//  "probably" 2500-3000 rpm.
//
// Sectors are 512 words; 2304 bytes.
//  4.2 MB/s  / 2304 => 1800 secs/sec
//  0.0005 secs/sector
//  .5 msecs/sector
//  500 usecs/sector
//
#define LATENCY_XFER 500
//
// The non-FIPS disks have 64 word sectors
//
#define LATENCY_XFER64 (500 / 8)
//
//  12.5 msecs seek avg
//  3390 had 590 cyls; assume avg means half worst case, so 295 cyls in 12.5 ms
//  12.5 ms / 295 cyls => .04 ms/cyl => 40 usec
//
#define LATENCY_SEEK_CYL 40
//
//  12,000 usecs seek
//
//  7.1 ms rotation delay avg.
//  7,100 usec
//
#define LATENCY_ROT 7100
//



static struct diskType_t diskTypes [] =
  {
    { // disk_init assumes 3381 is at index 0
      "3381", 224790, 0, false, seek_512, 512, 0, 255, LATENCY_SEEK_CYL, LATENCY_XFER
    },
    {
      "d500",  38258, 1, false, seek_64, 64, 200, 40 * 19, LATENCY_SEEK_CYL, LATENCY_XFER64
    },
    {
      "d451",  38258, 1, true, seek_64, 64, 154, 40 * 19, LATENCY_SEEK_CYL, LATENCY_XFER64
    },
    {
      "d400",  19270, 1, true, seek_64, 64, 84, 40 * 19, LATENCY_SEEK_CYL, LATENCY_XFER64 // d400 is a d190 with "high-efficiency format (40 sectors/track)"
    },
    {
      "d190",  14760, 1, true, seek_64, 64, 84, 31 * 19, LATENCY_SEEK_CYL, LATENCY_XFER64 // 190A 84, 190B 137
    },
    {
      "d181",  4444, 1, true, seek_64, 64, 0, 18 * 20, LATENCY_SEEK_CYL, LATENCY_XFER64 // no idea what the dau idx is
    },
    {
      "d501",  67200, 1, false, seek_64, 64, 201, 64 * 20, LATENCY_SEEK_CYL, LATENCY_XFER64
    },
    {
      "3380", 112395, 0, false, seek_512, 512, 0, 255, LATENCY_SEEK_CYL, LATENCY_XFER64 // 338x is never attached to a dau
    },
  };
#define N_DISK_TYPES (sizeof (diskTypes) / sizeof (struct diskType_t))

#define M3381_SECTORS 6895616
// records per subdev: 74930 (127 * 590)
// number of sub-volumes: 3
// records per dev: 3 * 74930 = 224790
// cyl/sv: 590
// cyl: 1770 (3*590)
// rec/cyl 127
// tracks/cyl 15
// sector size: 512
// sectors: 451858
// data: 3367 MB, 3447808 KB, 6895616 sectors,
//  3530555392 bytes, 98070983 records?

#define N_DISK_UNITS 2 // default

struct dsk_state dsk_states [N_DSK_UNITS_MAX];

//-- // extern t_stat disk_svc(UNIT *up);

// ./library_dir_dir/include/fs_dev_types.incl.alm
//
// From IBM GA27-1661-3_IBM_3880_Storage_Control_Description_May80, pg 4.4:
//
//  The Seek command transfers the six-byte seek address from the channel to
//  the storage director....
//
//     Bytes 0-5: 0 0 C C 0 H
//
//       Model       Cmax   Hmax
//       3330-1       410     18
//       3330-11      814     18
//       3340 (35MB)  348     11
//       3340 (70MB)  697     11
//       3344         697     11
//       3350         559     29
//
//  Search Identifier Equal  [CC HH R]
//    

// ./library_dir_dir/system_library_1/source/bound_page_control.s.archive/disk_control.pl1
//
// dcl     devadd             fixed bin (18);              /* record number part of device address */
//
//  /* Compute physical sector address from input info.  Physical sector result
//   accounts for unused sectors per cylinder. */
//
//        if pvte.is_sv then do;                  /* convert the subvolume devadd to the real devadd */
//             record_offset = mod (devadd, pvte.records_per_cyl);
//             devadd = ((devadd - record_offset) * pvte.num_of_svs) + pvte.record_factor + record_offset;
//        end;
//        sector = devadd * sect_per_rec (pvte.device_type);/* raw sector. */
//        cylinder = divide (sector, pvtdi.usable_sect_per_cyl, 12, 0);
//        sector = sector + cylinder * pvtdi.unused_sect_per_cyl;
//        sector = sector + sect_off;                     /* sector offset, if any. */
//

// DB37rs DSS190 Disk Subsystem, pg. 27:
//
//   Seek instruction:
//     Sector count limit, bits 0-11: These bits define the binary sector count.
//     All zeros is a maximum count of 4096.
//   Track indicator, bits 12-13:
//     These bits inidicate a complete track as good, defective, or alternate.
//       00 = primary track - good
//       01 = alternate track - good
//       10 = defective track - alternate track assigned
//       11 = defective track - no alternate track assigned
//
//   Sector address, bits 16-35
//
//      0                                                  35
//      XXXX  XXXX | XXXX XXXX | XXXX XXXX | XXXX XXXX | XXXX 0000 
//        BYTE 0         1           2           3           4
//
//  Seek        011100
//  Read        010101
//  Read ASCII  010011
//  Write       011001
//  Write ASCII 011010
//  Write and compare
//              011011
//  Request status
//              000000
//  reset status
//              100000
//  bootload control store
//              001000
//  itr boot    001001
//

#define UNIT_FLAGS ( UNIT_FIX | UNIT_ATTABLE | UNIT_ROABLE | UNIT_DISABLE | \
                     UNIT_IDLE | DKUF_F_RAW)
UNIT dsk_unit [N_DSK_UNITS_MAX] =
  {
    [0 ... N_DSK_UNITS_MAX-1] =
      {
        UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS),
        0, 0, 0, 0, 0, NULL, NULL, NULL, NULL
      }
  };

#define DSK_UNIT_IDX(uptr) ((uptr) - dsk_unit)

static DEBTAB disk_dt [] =
  {
    { "TRACE", DBG_TRACE, NULL },
    { "NOTIFY", DBG_NOTIFY, NULL },
    { "INFO", DBG_INFO, NULL },
    { "ERR", DBG_ERR, NULL },
    { "WARN", DBG_WARN, NULL },
    { "DEBUG", DBG_DEBUG, NULL },
    { "ALL", DBG_ALL, NULL }, // don't move as it messes up DBG message
    { NULL, 0, NULL }
  };

static t_stat disk_show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr, UNUSED int val, UNUSED const void * desc)
  {
    sim_printf("Number of DISK units in system is %d\n", dsk_dev . numunits);
    return SCPE_OK;
  }

static t_stat disk_set_nunits (UNUSED UNIT * uptr, UNUSED int32 value, const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    int n = atoi (cptr);
    if (n < 0 || n > N_DSK_UNITS_MAX)
      return SCPE_ARG;
    dsk_dev . numunits = (uint32) n;
    return SCPE_OK;
  }

#if 0
static t_stat disk_show_type (UNUSED FILE * st, UNUSED UNIT * uptr, UNUSED int val, UNUSED const void * desc)
  {
    int diskUnitIdx = (int) DSK_UNIT_IDX (uptr);
    if (diskUnitIdx < 0 || diskUnitIdx >= N_DSK_UNITS_MAX)
      {
        sim_printf ("error: invalid unit number %d\n", diskUnitIdx);
        return SCPE_ARG;
      }

    sim_printf("type %s\r\n", diskTypes[dsk_states[diskUnitIdx].typeIdx].typename);

    return SCPE_OK;
  }
#endif

static t_stat disk_set_type (UNUSED UNIT * uptr, UNUSED int32 value, const char * cptr, UNUSED void * desc)
  {
    int diskUnitIdx = (int) DSK_UNIT_IDX (uptr);
    if (diskUnitIdx < 0 || diskUnitIdx >= N_DSK_UNITS_MAX)
      {
        sim_printf ("error: invalid unit number %d\n", diskUnitIdx);
        return SCPE_ARG;
      }

    uint i;
    for (i = 0; i < N_DISK_TYPES; i ++)
      {
        if (strcasecmp (cptr, diskTypes[i].typename) == 0)
          break;
      }
    if (i >= N_DISK_TYPES)
     {
       sim_printf ("Disk type %s unrecognized, expected one of "
                   "%s %s %s %s %s %s %s %s\r\n",
                   cptr,
                   diskTypes[0].typename,
                   diskTypes[1].typename,
                   diskTypes[2].typename,
                   diskTypes[3].typename,
                   diskTypes[4].typename,
                   diskTypes[5].typename,
                   diskTypes[6].typename,
                   diskTypes[7].typename);
        return SCPE_ARG;
      }
    dsk_states[diskUnitIdx].typeIdx = i;
    dsk_unit[diskUnitIdx].capac = (t_addr) diskTypes[diskUnitIdx].capac;
    //sim_printf ("disk unit %d set to type %s\r\n",
                //diskUnitIdx, diskTypes[i].typename);
    return SCPE_OK;
  }

static t_stat dsk_show_device_name (UNUSED FILE * st, UNIT * uptr, 
                                    UNUSED int val, UNUSED const void * desc)
  {
    int n = (int) DSK_UNIT_IDX (uptr);
    if (n < 0 || n >= N_DSK_UNITS_MAX)
      return SCPE_ARG;
    sim_printf("Controller device name is %s\n", dsk_states [n].device_name);
    return SCPE_OK;
  }

static t_stat dsk_set_device_name (UNIT * uptr, UNUSED int32 value, 
                                   const char * cptr, UNUSED void * desc)
  {
    int n = (int) DSK_UNIT_IDX (uptr);
    if (n < 0 || n >= N_DSK_UNITS_MAX)
      return SCPE_ARG;
    if (cptr)
      {
        strncpy (dsk_states[n].device_name, cptr, MAX_DEV_NAME_LEN-1);
        dsk_states[n].device_name[MAX_DEV_NAME_LEN-1] = 0;
      }
    else
      dsk_states[n].device_name[0] = 0;
    return SCPE_OK;
  }

static t_stat signal_disk_ready (uint dsk_unit_idx)
  {

// controller ready
//    send_special_interrupt ((uint) cables -> cablesFromIomToDsk [dsk_unit_idx] . iomUnitIdx,
//                            (uint) cables -> cablesFromIomToDsk [dsk_unit_idx] . chan_num,
//                            0,
//                            0x40, 00 /* controller ready */);

    // Don't signal in the sim is actually running....
    if (! sim_is_running)
      return SCPE_OK;
    // if substr (special_status_word, 20, 1) ^= "1"b | substr (special_status_word, 13, 6) ^= "00"b3
    // if substr (special_status_word, 34, 3) ^= "001"b
    // Note the 34,3 spans 34,35,36; therefore the bits are 1..36, not 0..35
    // 20,1 is bit 19
    // 13,6, is bits 12..17
    // status0 is 19..26
    // status1 is 28..35
    // so substr (w, 20, 1) is bit 0 of status0
    //    substr (w, 13, 6) is the low 6 bits of dev_no
    //    substr (w, 34, 3) is the low 3 bits of status 1
        //sim_printf ("%s %d %o\n", disk_filename, ro,  mt_unit [dsk_unit_idx] . flags);
        //sim_printf ("special int %d %o\n", dsk_unit_idx, mt_unit [dsk_unit_idx] . flags);

    uint ctlr_unit_idx = cables->dsk_to_ctlr [dsk_unit_idx].ctlr_unit_idx;
    enum ctlr_type_e ctlr_type = cables->dsk_to_ctlr [dsk_unit_idx].ctlr_type;
    if (ctlr_type != CTLR_T_MSP && ctlr_type != CTLR_T_IPCD)
      {
        // If None, assume that the cabling hasn't happend yey.
        if (ctlr_type != CTLR_T_NONE)
          {
sim_printf ("lost %u\n", ctlr_type);
            sim_warn ("signal_disk_ready lost\n");
            return SCPE_ARG;
          }
        return SCPE_OK;
      }

    // Which port should the controller send the interrupt to? All of them...
    bool sent_one = false;
    for (uint ctlr_port_num = 0; ctlr_port_num < MAX_CTLR_PORTS; ctlr_port_num ++)
      {
        if (ctlr_type == CTLR_T_MSP)
          {
            if (cables->msp_to_iom[ctlr_unit_idx][ctlr_port_num].in_use)
              {
                uint iom_unit_idx = cables->msp_to_iom[ctlr_unit_idx][ctlr_port_num].iom_unit_idx;
                uint chan_num = cables->msp_to_iom[ctlr_unit_idx][ctlr_port_num].chan_num;
                uint dev_code = cables->dsk_to_ctlr[dsk_unit_idx].dev_code;

                send_special_interrupt (iom_unit_idx, chan_num, dev_code, 0x40, 01 /* disk pack ready */);
                sent_one = true;
              }
          }
        else
          {
            if (cables->ipcd_to_iom[ctlr_unit_idx][ctlr_port_num].in_use)
              {
                uint iom_unit_idx = cables->ipcd_to_iom[ctlr_unit_idx][ctlr_port_num].iom_unit_idx;
                uint chan_num = cables->ipcd_to_iom[ctlr_unit_idx][ctlr_port_num].chan_num;
                uint dev_code = cables->dsk_to_ctlr[dsk_unit_idx].dev_code;

                send_special_interrupt (iom_unit_idx, chan_num, dev_code, 0x40, 01 /* disk pack ready */);
                sent_one = true;
              }
          }
      }
    if (! sent_one)
      {
        sim_printf ("signal_disk_ready can't find controller; dropping interrupt\n");
        return SCPE_ARG;
      }

    return SCPE_OK;
  }

static t_stat disk_set_ready (UNIT * uptr, UNUSED int32 value, 
                              UNUSED const char * cptr,
                              UNUSED void * desc)
  {
    long disk_unit_idx = DSK_UNIT_IDX (uptr);
    if (disk_unit_idx >= (long) dsk_dev.numunits)
      {
        sim_debug (DBG_ERR, & dsk_dev, 
                   "Disk set ready: Invalid unit number %ld\n", disk_unit_idx);
        sim_printf ("error: invalid unit number %ld\n", disk_unit_idx);
        return SCPE_ARG;
      }
    return signal_disk_ready ((uint) disk_unit_idx);
  }

t_stat unload_disk (uint dsk_unit_idx)
  {
    if (dsk_unit [dsk_unit_idx] . flags & UNIT_ATT)
      {
        t_stat stat = sim_disk_detach (& dsk_unit [dsk_unit_idx]);
        if (stat != SCPE_OK)
          {
            sim_warn ("%s sim_disk_detach returned %d\n", __func__, stat);
            return SCPE_ARG;
          }
      }
    return signal_disk_ready ((uint) dsk_unit_idx);
  }

t_stat load_disk (uint dsk_unit_idx, const char * disk_filename, bool ro)
  {
    if (ro)
      dsk_unit[dsk_unit_idx].flags |= MTUF_WRP;
    else
      dsk_unit[dsk_unit_idx].flags &= ~ MTUF_WRP;
    //sim_printf ("in loadTape %d %s\n", dsk_unit_idx, disk_filename);
    t_stat stat = attach_unit (& dsk_unit [dsk_unit_idx], disk_filename);
    if (stat != SCPE_OK)
      {
        sim_printf ("load_disk sim_disk_attach returned %d\n", stat);
        return stat;
      }
    return signal_disk_ready ((uint) dsk_unit_idx);
  }

#define UNIT_WATCH UNIT_V_UF

static MTAB disk_mod [] =
  {
    { UNIT_WATCH, 1, "WATCH", "WATCH", 0, 0, NULL, NULL },
    { UNIT_WATCH, 0, "NOWATCH", "NOWATCH", 0, 0, NULL, NULL },
    {
      MTAB_dev_value, /* mask */
      0,            /* match */
      "NUNITS",     /* print string */
      "NUNITS",         /* match string */
      disk_set_nunits, /* validation routine */
      disk_show_nunits, /* display routine */
      "Number of DISK units in the system", /* value descriptor */
      NULL // Help
    },
    {
      MTAB_unit_value_show, // mask 
      0,                    // match
      "TYPE",               // print string
      "TYPE",               // match string
      disk_set_type,        // validation routine
      NULL /*disk_show_type*/,       // display routine
      "disk type",          // value descriptor
      "D500, D451, D400, D190, D181, D501, 3380, 3381" // Help
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_VALR | MTAB_NC, /* mask */
      0,            /* match */
      "NAME",     /* print string */
      "NAME",         /* match string */
      dsk_set_device_name, /* validation routine */
      dsk_show_device_name, /* display routine */
      "Set the device name", /* value descriptor */
      NULL          // help
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_NMO | MTAB_VALR, /* mask */
      0,            /* match */
      "READY",     /* print string */
      "READY",         /* match string */
      disk_set_ready,         /* validation routine */
      NULL, /* display routine */
      NULL,          /* value descriptor */
      NULL   // help string
    },
    MTAB_eol
  };


static t_stat disk_reset (UNUSED DEVICE * dptr)
  {
    return SCPE_OK;
  }

static t_stat disk_attach (UNIT *uptr, CONST char *cptr)
  {
    int diskUnitIdx = (int) DSK_UNIT_IDX (uptr);
    if (diskUnitIdx < 0 || diskUnitIdx >= N_DSK_UNITS_MAX)
      {
        sim_printf ("error: invalid unit number %d\n", diskUnitIdx);
        return SCPE_ARG;
      }

    return load_disk ((uint) diskUnitIdx, cptr, false);
  }

// No disks known to multics had more than 2^24 sectors...
DEVICE dsk_dev = {
    "DISK",       /*  name */
    dsk_unit,    /* units */
    NULL,         /* registers */
    disk_mod,     /* modifiers */
    N_DISK_UNITS, /* #units */
    10,           /* address radix */
    24,           /* address width */
    1,            /* address increment */
    8,            /* data radix */
    36,           /* data width */
    NULL,         /* examine */
    NULL,         /* deposit */ 
    disk_reset,   /* reset */
    NULL,         /* boot */
    disk_attach,  /* attach */
    NULL /*disk_detach*/,  /* detach */
    NULL,         /* context */
    DEV_DEBUG,    /* flags */
    0,            /* debug control flags */
    disk_dt,      /* debug flag names */
    NULL,         /* memory size change */
    NULL,         /* logical name */
    NULL,         // help
    NULL,         // attach help
    NULL,         // attach context
    NULL,         // description
   NULL
};

/*
 * disk_init()
 *
 */

// Once-only initialization

void disk_init (void)
  {
    // Sets diskTypeIdx to 0: 3381
    memset (dsk_states, 0, sizeof (dsk_states));
#ifdef LOCKLESS
    for (uint i = 0; i < N_DSK_UNITS_MAX; i ++)
      {
#ifdef __FreeBSD__
        pthread_mutexattr_t scu_attr;
        pthread_mutexattr_init (& scu_attr);
        pthread_mutexattr_settype (& scu_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    
        pthread_mutex_init (& dsk_states[i].dsk_lock, & scu_attr);
#else                            
        pthread_mutex_init (& dsk_states[i].dsk_lock, NULL);
#endif 
      }
#endif 
  }

static int diskSeek64 (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
    struct dsk_state * disk_statep = & dsk_states [devUnitIdx];
    sim_debug (DBG_NOTIFY, & dsk_dev, "Seek64 %d\n", devUnitIdx);
    disk_statep -> io_mode = disk_seek64_mode;

    uint typeIdx = disk_statep->typeIdx;
    if (diskTypes[typeIdx].seekSize != seek_64)
      sim_warn ("disk%u sent a SEEK_64 but is 512 sized\n", typeIdx);

// Process DDCW

    bool send, uff;
    int rc = iom_list_service (iomUnitIdx, chan, & send, & uff);
    if (rc < 0)
      {
        sim_printf ("diskSeek54 list service failed\n");
        return -1;
      }
    if (uff)
      {
        sim_printf ("diskSeek54 ignoring uff\n"); // XXX
      }
    if (! send)
      {
        sim_printf ("diskSeek54 nothing to send\n");
        return 1;
      }
    if (p -> DCW_18_20_CP == 07 || p -> DDCW_22_23_TYPE == 2)
      {
        sim_printf ("diskSeek54 expected DDCW\n");
        return -1;
      }


    uint tally = p -> DDCW_TALLY;
    sim_debug (DBG_DEBUG, & dsk_dev,
               "%s: Tally %d (%o)\n", __func__, tally, tally);

    // Seek specific processing

    if (tally != 1)
      {
        sim_printf ("disk seek dazed by tally %d != 1\n", tally);
        p -> stati = 04510; // Cmd reject, invalid inst. seq.
        p -> chanStatus = chanStatIncorrectDCW;
        return -1;
      }

    word36 seekData;
    uint count;
    iom_indirect_data_service (iomUnitIdx, chan, & seekData, &count, false);
    if (count != 1)
      sim_warn ("%s: count %d not 1\n", __func__, count);
    seekData &= MASK21;

//sim_printf ("seekData %012"PRIo64"\n", seekData);
// Observations about the seek/write stream
// the stream is seek512 followed by a write 1024.
// the seek data is:  000300nnnnnn
// lets assume the 3 is a copy of the seek cmd # as a data integrity check.
// highest observed n during vol. inoit. 272657(8) 95663(10)
//

#if defined(LOCKLESS) && defined(IO_LATENCY)
        uint delta = (uint) abs ((int) disk_statep->seekPosition - (int) seekData) /
                      diskTypes[typeIdx].sects_per_cyl;
        usleep (diskTypes[typeIdx].seek_latency * delta);
#endif
// disk_control.pl1: 
//   quentry.sector = bit (sector, 21);  /* Save the disk device address. */
// suggests seeks are 21 bits.
//  
    disk_statep -> seekPosition = (uint) seekData;
//sim_printf ("seek seekPosition %d\n", disk_statep -> seekPosition);
    p -> stati = 00000; // Channel ready
#ifdef PROFILER
    __atomic_add_fetch (& system_state->profiler.disk_seeks, 1u, __ATOMIC_ACQUIRE);
#endif
    return 0;
  }

static int diskSeek512 (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
    struct dsk_state * disk_statep = & dsk_states [devUnitIdx];
    sim_debug (DBG_NOTIFY, & dsk_dev, "Seek512 %d\n", devUnitIdx);
//sim_printf ("disk seek512 [%"PRId64"]\n", cpu.cycleCnt);
    disk_statep -> io_mode = disk_seek512_mode;

    uint typeIdx = disk_statep->typeIdx;
    if (diskTypes[typeIdx].seekSize != seek_512)
      sim_warn ("disk%u sent a SEEK_512 but is 64 sized\n", typeIdx);

// Process DDCW

    bool send, uff;
    int rc = iom_list_service (iomUnitIdx, chan, & send, & uff);
    if (rc < 0)
      {
        sim_printf ("diskSeek512 list service failed\n");
        return -1;
      }
    if (uff)
      {
        sim_printf ("diskSeek512 ignoring uff\n"); // XXX
      }
    if (! send)
      {
        sim_printf ("diskSeek512 nothing to send\n");
        return 1;
      }
    if (p -> DCW_18_20_CP == 07 || p -> DDCW_22_23_TYPE == 2)
      {
        sim_printf ("diskSeek512 expected DDCW\n");
        return -1;
      }


    uint tally = p -> DDCW_TALLY;
    sim_debug (DBG_DEBUG, & dsk_dev,
               "%s: Tally %d (%o)\n", __func__, tally, tally);

    // Seek specific processing

    if (tally != 1)
      {
        sim_printf ("disk seek dazed by tally %d != 1\n", tally);
        p -> stati = 04510; // Cmd reject, invalid inst. seq.
        p -> chanStatus = chanStatIncorrectDCW;
        return -1;
      }

    word36 seekData;
    uint count;
    iom_indirect_data_service (iomUnitIdx, chan, & seekData, &count, false);
    if (count != 1)
      sim_warn ("%s: count %d not 1\n", __func__, count);
    seekData &= MASK21;

//sim_printf ("seekData %012"PRIo64"\n", seekData);
// Observations about the seek/write stream
// the stream is seek512 followed by a write 1024.
// the seek data is:  000300nnnnnn
// lets assume the 3 is a copy of the seek cmd # as a data integrity check.
// highest observed n during vol. inoit. 272657(8) 95663(10)
//

#if defined(LOCKLESS) && defined(IO_LATENCY)
        uint delta = (uint) abs ((int) disk_statep->seekPosition - (int) seekData) /
                      diskTypes[typeIdx].sects_per_cyl;
        usleep (diskTypes[typeIdx].seek_latency * delta);
#endif
// disk_control.pl1: 
//   quentry.sector = bit (sector, 21);  /* Save the disk device address. */
// suggests seeks are 21 bits.
//  
    disk_statep -> seekPosition = (uint) seekData;
//sim_printf ("seek seekPosition %d\n", disk_statep -> seekPosition);
    p -> stati = 00000; // Channel ready
#ifdef PROFILER
    __atomic_add_fetch (& system_state->profiler.disk_seeks, 1u, __ATOMIC_ACQUIRE);
#endif
    return 0;
  }

static int diskRead (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
    UNIT * unitp = & dsk_unit [devUnitIdx];
    struct dsk_state * disk_statep = & dsk_states [devUnitIdx];
    uint typeIdx = disk_statep->typeIdx;
    uint sectorSizeWords = diskTypes[typeIdx].sectorSizeWords;
    uint sectorSizeBytes = ((36 * sectorSizeWords) / 8);
    sim_debug (DBG_NOTIFY, & dsk_dev, "Read %d\n", devUnitIdx);
    disk_statep -> io_mode = disk_read_mode;

// Process DDCWs

    bool send, uff;
    do
      {
        int rc = iom_list_service (iomUnitIdx, chan, & send, & uff);
        if (rc < 0)
          {
            sim_printf ("diskRead list service failed\n");
            return -1;
          }
        if (uff)
          {
            sim_printf ("diskRead ignoring uff\n"); // XXX
          }
        if (! send)
          {
            sim_printf ("diskRead nothing to send\n");
            return 1;
          }
        if (p -> DCW_18_20_CP == 07 || p -> DDCW_22_23_TYPE == 2)
          {
            sim_printf ("diskRead expected DDCW\n");
            return -1;
          }

        uint tally = p -> DDCW_TALLY;
        if (tally == 0)
          {
            sim_debug (DBG_DEBUG, & dsk_dev,
                       "%s: Tally of zero interpreted as 010000(4096)\n",
                       __func__);
            tally = 4096;
          }

        sim_debug (DBG_DEBUG, & dsk_dev,
                   "%s: Tally %d (%o)\n", __func__, tally, tally);

        rc = fseek (unitp -> fileref, 
                    (long) (disk_statep -> seekPosition * sectorSizeBytes),
                    SEEK_SET);
        if (rc)
          {
            sim_printf ("fseek (read) returned %d, errno %d\n", rc, errno);
            p -> stati = 04202; // attn, seek incomplete
            return -1;
          }

        // Convert from word36 format to packed72 format

        // round tally up to sector boundary
    
        // this math assumes tally is even.
   
        uint tallySectors = (tally + sectorSizeWords - 1) / 
                             sectorSizeWords;
        uint tallyWords = tallySectors * sectorSizeWords;
        //uint tallyBytes = tallySectors * sectorSizeBytes;
        uint p72ByteCnt = (tallyWords * 36) / 8;
        uint8 diskBuffer [p72ByteCnt];
        memset (diskBuffer, 0, sizeof (diskBuffer));
        sim_debug (DBG_TRACE, & dsk_dev, "Disk read  %3d %8d %3d\n",
                   devUnitIdx, disk_statep -> seekPosition, tallySectors);

        fflush (unitp->fileref);
#if defined(LOCKLESS) && defined(IO_LATENCY)
        usleep (diskTypes[typeIdx].xfer_latency * tallySectors + LATENCY_ROT);
#endif
        rc = (int) fread (diskBuffer, sectorSizeBytes,
                    tallySectors,
                    unitp -> fileref);
 
// The rc code is wrong; it is using read() semantics, for fread().
#if 1
        if (rc == 0) // EOF or error
          {
            if (ferror (unitp->fileref))
              {
                p -> stati = 04202; // attn, seek incomplete
                p -> chanStatus = chanStatIncorrectDCW;
                return -1;
              }
            // We ignore short reads-- we assume that they are reads
            // past the write highwater mark, and return zero data,
            // just as if the disk had been formatted with zeros.
          }
#else
        if (rc == 0) // eof; reading a sector beyond the high water mark.
          {
            // okay; buffer was zero, so just pretend that a zero filled
            // sector was read (ala demand page zero)
          }
        else if (rc != (int) tallySectors)
          {
            sim_printf ("read returned %d, errno %d\n", rc, errno);
            sim_printf ("tally %u\n", tally);
            sim_printf ("tallySectors %u\n", tallySectors);
            sim_printf ("tallyWords %u\n", tallyWords);
            sim_printf ("p72ByteCnt %u\n", p72ByteCnt);
            sim_printf ("fseek to %ld\n",  (long) (disk_statep -> seekPosition * sectorSizeBytes));
            sim_printf ("devUnitIdx %u\n", devUnitIdx);
            sim_printf ("iomUnitIdx %u\n", iomUnitIdx);
            sim_printf ("chan %u\n", chan);
            sim_printf ("typeIdx %u\n", typeIdx);
            sim_printf ("sectorSizeWords %u\n", sectorSizeWords);
            sim_printf ("sectorSizeBytes %u\n", sectorSizeBytes);
            sim_printf ("fileref %p\n", unitp->fileref);
            sim_printf ("knowns:\n");
            for (uint i = 0; i < N_DSK_UNITS_MAX; i ++)
              {
                sim_printf ("  %u typeIdx %u seekPosition %u filename <%s> fileref %p\n",
                  i,
                  dsk_states[i].typeIdx,
                  dsk_states[i].seekPosition,
                  dsk_unit[i].filename,
                  dsk_unit[i].fileref);
              }
            p -> stati = 04202; // attn, seek incomplete
            p -> chanStatus = chanStatIncorrectDCW;
            return -1;
          }
#endif
//sim_printf ("tallySectors %u\n", tallySectors);
//sim_printf ("p72ByteCnt %u\n", p72ByteCnt);
//for (uint i = 0; i < p72ByteCnt; i += 9)
//{ word36 w1 = extr (& diskBuffer [i / 9], 0, 36);
  //word36 w2 = extr (& diskBuffer [i / 9], 36, 36);
  //sim_printf ("%5d %012"PRIo64" %012"PRIo64"\n", i * 2 / 9, w1, w2);
//}
//sim_printf ("read seekPosition %d\n", disk_statep -> seekPosition);
//sim_printf ("diskBuffer 0...\n");
//for (uint i = 0; i < 9; i ++) sim_printf (" %03o", diskBuffer [i]);
//sim_printf ("\n");
        disk_statep -> seekPosition += tallySectors;

        uint wordsProcessed = 0;
        word36 buffer [tally];
        for (uint i = 0; i < tally; i ++)
          {
            word36 w;
            extractWord36FromBuffer (diskBuffer, p72ByteCnt, & wordsProcessed,
                                     & w);
            buffer [i] = w;
          }
        iom_indirect_data_service (iomUnitIdx, chan, buffer,
                                & wordsProcessed, true);
#ifdef PROFILER
        __atomic_add_fetch (& system_state->profiler.disk_reads, 1u, __ATOMIC_ACQUIRE);
        __atomic_add_fetch (& system_state->profiler.disk_read, (unsigned long) tally, __ATOMIC_ACQUIRE);
#endif
      } while (p -> DDCW_22_23_TYPE != 0); // not IOTD
    p -> stati = 04000;
    p -> initiate = false;
    return 0;
  }

static int diskWrite (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
    UNIT * unitp = & dsk_unit [devUnitIdx];
    struct dsk_state * disk_statep = & dsk_states [devUnitIdx];
    uint typeIdx = disk_statep->typeIdx;
    uint sectorSizeWords = diskTypes[typeIdx].sectorSizeWords;
    uint sectorSizeBytes = ((36 * sectorSizeWords) / 8);

    sim_debug (DBG_NOTIFY, & dsk_dev, "Write %d\n", devUnitIdx);
    disk_statep -> io_mode = disk_read_mode;

// Process DDCWs

    bool send, uff;
    do
      {
        int rc = iom_list_service (iomUnitIdx, chan, & send, & uff);
        if (rc < 0)
          {
            sim_printf ("diskWrite list service failed\n");
            return -1;
          }
        if (uff)
          {
            sim_printf ("diskWrite ignoring uff\n"); // XXX
          }
        if (! send)
          {
            sim_printf ("diskWrite nothing to send\n");
            return 1;
          }
        if (p -> DCW_18_20_CP == 07 || p -> DDCW_22_23_TYPE == 2)
          {
            sim_printf ("diskWrite expected DDCW\n");
            return -1;
          }


        uint tally = p -> DDCW_TALLY;

        if (tally == 0)
          {
            sim_debug (DBG_DEBUG, & dsk_dev,
                       "%s: Tally of zero interpreted as 010000(4096)\n",
                       __func__);
            tally = 4096;
          }

        sim_debug (DBG_DEBUG, & dsk_dev,
                   "%s: Tally %d (%o)\n", __func__, tally, tally);

        rc = fseek (unitp -> fileref, 
                    (long) (disk_statep -> seekPosition * sectorSizeBytes),
                    SEEK_SET);
        if (rc)
          {
            sim_printf ("fseek (read) returned %d, errno %d\n", rc, errno);
            p -> stati = 04202; // attn, seek incomplete
            return -1;
          }

        // Convert from word36 format to packed72 format

        // round tally up to sector boundary
    
        // this math assumes tally is even.
   
        uint tallySectors = (tally + sectorSizeWords - 1) / 
                             sectorSizeWords;
        uint tallyWords = tallySectors * sectorSizeWords;
        //uint tallyBytes = tallySectors * sectorSizeBytes;
        uint p72ByteCnt = (tallyWords * 36) / 8;
        uint8 diskBuffer [p72ByteCnt];
        memset (diskBuffer, 0, sizeof (diskBuffer));
        uint wordsProcessed = 0;
#if 0
        for (uint i = 0; i < tally; i ++)
          {
            word36 w;
            core_read (daddr + i, & w, "Disk write");
            insertWord36toBuffer (diskBuffer, p72ByteCnt, & wordsProcessed,
                                  w);
          }
#else
        word36 buffer [tally];
        iom_indirect_data_service (iomUnitIdx, chan, buffer,
                                & wordsProcessed, false);
// XXX is this losing information?
        wordsProcessed = 0;
        for (uint i = 0; i < tally; i ++)
          {
            insertWord36toBuffer (diskBuffer, p72ByteCnt, & wordsProcessed,
                                  buffer [i]);
          }
#endif

#if defined(LOCKLESS) && defined(IO_LATENCY)
        usleep (diskTypes[typeIdx].xfer_latency * tallySectors + LATENCY_ROT);
#endif
        sim_debug (DBG_TRACE, & dsk_dev, "Disk write %3d %8d %3d\n",
                   devUnitIdx, disk_statep -> seekPosition, tallySectors);
        rc = (int) fwrite (diskBuffer, sectorSizeBytes,
                     tallySectors,
                     unitp -> fileref);
        fflush (unitp->fileref);

//sim_printf ("Disk write %8d %3d %08o\n",
//disk_statep -> seekPosition, tallySectors, daddr);
              
        if (rc != (int) tallySectors)
          {
            sim_printf ("fwrite returned %d, errno %d\n", rc, errno);
            p -> stati = 04202; // attn, seek incomplete
            p -> chanStatus = chanStatIncorrectDCW;
            return -1;
          }

        disk_statep -> seekPosition += tallySectors;
#ifdef PROFILER
        __atomic_add_fetch (& system_state->profiler.disk_writes, 1u, __ATOMIC_ACQUIRE);
        __atomic_add_fetch (& system_state->profiler.disk_written, (unsigned long) tally, __ATOMIC_ACQUIRE);
#endif
 
      } while (p -> DDCW_22_23_TYPE != 0); // not IOTD
    p -> stati = 04000;
    p -> initiate = false;
    return 0;
  }

static int readStatusRegister (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
    UNIT * unitp = & dsk_unit [devUnitIdx];
    struct dsk_state * disk_statep = & dsk_states [devUnitIdx];

    sim_debug (DBG_NOTIFY, & dsk_dev, "Read %d\n", devUnitIdx);
    disk_statep -> io_mode = disk_read_mode;

// Process DDCW

    bool send;
    bool uff;
    int rc = iom_list_service (iomUnitIdx, chan, & send, & uff);
    if (rc < 0)
      {
        sim_printf ("readStatusRegister list service failed\n");
        return -1;
      }
    if (uff)
      {
        sim_printf ("readStatusRegister ignoring uff\n"); // XXX
      }
    if (! send)
      {
        sim_printf ("readStatusRegister nothing to send\n");
        return 1;
      }
    if (p -> DCW_18_20_CP == 07 || p -> DDCW_22_23_TYPE == 2)
      {
        sim_printf ("readStatusRegister expected DDCW\n");
        return -1;
      }


    uint tally = p -> DDCW_TALLY;

    if (tally != 4)
      {
        sim_debug (DBG_ERR, &iom_dev, 
                   "%s: RSR expected tally of 4, is %d\n",
                   __func__, tally);
      }
    if (tally == 0)
      {
        sim_debug (DBG_DEBUG, & dsk_dev,
                   "%s: Tally of zero interpreted as 010000(4096)\n",
                   __func__);
        tally = 4096;
      }

// XXX need status register data format 
// system_library_tools/source/bound_io_tools_.s.archive/analyze_detail_stat_.pl1  anal_fips_disk_().

#ifdef TESTING
    sim_warn ("Need status register data format\n");
#endif
#if 1
    word36 buffer [tally];
    memset (buffer, 0, sizeof (buffer));
    buffer [0] = SIGN36;
    uint wordsProcessed = 0;
    iom_indirect_data_service (iomUnitIdx, chan, buffer,
                            & wordsProcessed, true);
#else
    for (uint i = 0; i < tally; i ++)
      //M [daddr + i] = 0;
      core_write (daddr + i, 0, "Disk status register");

    //M [daddr] = SIGN36;
    core_write (daddr, SIGN36, "Disk status register");
#endif
    p -> charPos = 0;
    p -> stati = 04000;
    if (! unitp -> fileref)
      p -> stati = 04240; // device offline
    p -> initiate = false;
    return 0;
  }

static int read_configuration (uint dev_unit_idx, uint iom_unit_idx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data[iom_unit_idx][chan];
    UNIT * unitp = & dsk_unit[dev_unit_idx];
    struct dsk_state * disk_statep = & dsk_states[dev_unit_idx];

    sim_debug (DBG_NOTIFY, & dsk_dev, "Read %d\n", dev_unit_idx);
    disk_statep->io_mode = disk_read_mode;

// Process DDCW

    bool send;
    bool uff;
    int rc = iom_list_service (iom_unit_idx, chan, & send, & uff);
    if (rc < 0)
      {
        sim_printf ("%s list service failed\n", __func__);
        return -1;
      }
    if (uff)
      {
        sim_printf ("%s ignoring uff\n", __func__); // XXX
      }
    if (! send)
      {
        sim_printf ("%s nothing to send\n", __func__);
        return 1;
      }
    if (p->DCW_18_20_CP == 07 || p->DDCW_22_23_TYPE == 2)
      {
        sim_printf ("%s expected DDCW\n", __func__);
        return -1;
      }


    uint tally = p -> DDCW_TALLY;

// poll_mpc.pl1
//
// dcl  1 dau_buf aligned based (workp),   /* The IOI buffer segment */
//        2 cf_idcw bit (36),              /* Read Configuration (24o) */
//        2 cf_dcw bit (36),               /* Addr=dau_buf.data(0), tally=65 */
//        2 st_idcw bit (36),              /* Read/Clear Statistics (16o) */
//        2 st_dcw bit (36),         /* Address=dau_buf.data(130), tally=315 */
//        2 data (0:759) bit (18) unal;    /* Config & statistics area */

#if 0
    if (tally != 4)
      {
        sim_debug (DBG_ERR, &i om_dev, 
                   "%s: RSR expected tally of 4, is %d\n",
                   __func__, tally);
      }
    if (tally == 0)
      {
        sim_debug (DBG_DEBUG, & dsk_dev,
                   "%s: Tally of zero interpreted as 010000(4096)\n",
                   __func__);
        tally = 4096;
      }

// XXX need status register data format 
// system_library_tools/source/bound_io_tools_.s.archive/analyze_detail_stat_.pl1  anal_fips_disk_().

#ifdef TESTING
    sim_warn ("Need status register data format\n");
#endif
#endif

// poll_mpc.pl1
//
// dcl  1 dau_buf aligned based (workp),   /* The IOI buffer segment */
//        2 cf_idcw bit (36),              /* Read Configuration (24o) */
//        2 cf_dcw bit (36),               /* Addr=dau_buf.data(0), tally=65 */
//        2 st_idcw bit (36),              /* Read/Clear Statistics (16o) */
//        2 st_dcw bit (36),         /* Address=dau_buf.data(130), tally=315 */
//        2 data (0:759) bit (18) unal;    /* Config & statistics area */

//  dau_buf.data:
//    0                                35
//   +----------------+------------------+
//   |    data(0)     |   data(1)        |
//   +----------------+------------------+
//   |    data(2)     |   data(3)        |
//   +----------------+------------------+

// dcl  dau_data (0:759) bit (16) unal;                        /* DAU config and stats */

//
//          do i = 0 to 759;
//            substr (dau_data (i), 1, 8) = substr (dau_buf.data (i), 2, 8);
//            substr (dau_data (i), 9, 8) = substr (dau_buf.data (i), 11, 8);
//          end;
//
//   substr (dau_buf.data (i), 2, 8)
//
//    0 1 2 3 4 5 6 7 8     ....       17
//   +-----------------------------------+
//   | |X|X|X|X|X|X|X|X| |Y|Y|Y|Y|Y|Y|Y|Y|
//   +-----------------------------------+
//
//   substr (dau_data (i), 1, 8)
//
//    0 1 2 3 4 5 6 7 8   ....     15
//   +-------------------------------+
//   |X|X|X|X|X|X|X|X|Y|Y|Y|Y|Y|Y|Y|Y|
//   +-------------------------------+

// dcl  1 dau_char based (image_ptr) unaligned,   /* Config data */
//       2 type bit (8),                          /* = 12 HEX */
//       2 hw_rev bit (8) unal,                   /* DAU rev */
//       2 fw_maj_rev bit (8) unal,               /* firmware rev letter */ 
//       2 fw_sub_rev bit (8) unal;               /* firmware rev number */
//       2 dev (64),                              /* seq'ed by dev# */
//                       /* all 4 bytes zero, if device NEVER configured */
//         3 type fixed bin (8) uns unal,         /* device type */
//         3 number fixed bin (8) uns unal,       /* device number, =FF if not configured */
//         3 summary_status bit (8) unal,         /* device SS reg */
//         3 port_number fixed bin (8) uns unal;  /* device DAU port */

// We know that we are an MSP and not an IPC as this command is only issued
// to MSPs.

    uint ctlr_unit_idx = get_ctlr_idx (iom_unit_idx, chan);
    struct ctlr_to_dev_s * dev_p;
    if (cables->iom_to_ctlr[iom_unit_idx][chan].ctlr_type == CTLR_T_IPCD)
      dev_p = cables->ipcd_to_dsk[ctlr_unit_idx];
    else  
      dev_p = cables->msp_to_dsk[ctlr_unit_idx];

// XXX Temp
    word36 buffer [tally];
    memset (buffer, 0, sizeof (buffer));
    putbits36_9 (& buffer [0],  0, 0x12);
    putbits36_9 (& buffer [0],  9, 1);  // h/w revision
    putbits36_9 (& buffer [0], 18, '1');  // fw maj revision
    putbits36_9 (& buffer [0], 27, 'a');  // fw sub revision

    for (word9 dev_num = 0; dev_num < N_DEV_CODES; dev_num ++)
      {
         if (! dev_p[dev_num].in_use)
           continue;
         uint dsk_unit_idx = dev_p[dev_num].unit_idx;
         word9 dau_type = (word9) diskTypes[dsk_unit_idx].dau_type;
         putbits36_9 (& buffer[1+dev_num], 0, dau_type); // dev.type
         putbits36_9 (& buffer[1+dev_num], 9, dev_num); // dev.number
         putbits36_9 (& buffer[1+dev_num], 18, 0); // dev.summary_status // XXX
         putbits36_9 (& buffer[1+dev_num], 27, dev_num); // dev.port_number 
     }

    uint wordsProcessed = tally;
    iom_indirect_data_service (iom_unit_idx, chan, buffer,
                            & wordsProcessed, true);
#if 0
#if 1
    word36 buffer [tally];
    memset (buffer, 0, sizeof (buffer));
    buffer [0] = SIGN36;
    uint wordsProcessed = 0;
    iom_indirect_data_service (iom_unit_idx, chan, buffer,
                            & wordsProcessed, true);
#else
    for (uint i = 0; i < tally; i ++)
      //M [daddr + i] = 0;
      core_write (daddr + i, 0, "Disk status register");

    //M [daddr] = SIGN36;
    core_write (daddr, SIGN36, "Disk status register");
#endif
#endif
    p -> charPos = 0;
    p -> stati = 04000;
    if (! unitp -> fileref)
      p -> stati = 04240; // device offline
    p -> initiate = false;
    return 0;
  }

static int read_and_clear_statistics (uint dev_unit_idx, uint iom_unit_idx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data[iom_unit_idx][chan];
    UNIT * unitp = & dsk_unit[dev_unit_idx];
    struct dsk_state * disk_statep = & dsk_states[dev_unit_idx];

    sim_debug (DBG_NOTIFY, & dsk_dev, "Read %d\n", dev_unit_idx);
    disk_statep->io_mode = disk_read_mode;

// Process DDCW

    bool send;
    bool uff;
    int rc = iom_list_service (iom_unit_idx, chan, & send, & uff);
    if (rc < 0)
      {
        sim_printf ("%s list service failed\n", __func__);
        return -1;
      }
    if (uff)
      {
        sim_printf ("%s ignoring uff\n", __func__); // XXX
      }
    if (! send)
      {
        sim_printf ("%s nothing to send\n", __func__);
        return 1;
      }
    if (p->DCW_18_20_CP == 07 || p->DDCW_22_23_TYPE == 2)
      {
        sim_printf ("%s expected DDCW\n", __func__);
        return -1;
      }


#if 0
    uint tally = p -> DDCW_TALLY;

    if (tally != 4)
      {
        sim_debug (DBG_ERR, &i om_dev, 
                   "%s: RSR expected tally of 4, is %d\n",
                   __func__, tally);
      }
    if (tally == 0)
      {
        sim_debug (DBG_DEBUG, & dsk_dev,
                   "%s: Tally of zero interpreted as 010000(4096)\n",
                   __func__);
        tally = 4096;
      }
#endif

#if 0
#ifdef TESTING
    sim_warn ("Need status register data format\n");
#endif
#if 1
    word36 buffer [tally];
    memset (buffer, 0, sizeof (buffer));
    buffer [0] = SIGN36;
    uint wordsProcessed = 0;
    iom_indirect_data_service (iom_unit_idx, chan, buffer,
                            & wordsProcessed, true);
#else
    for (uint i = 0; i < tally; i ++)
      //M [daddr + i] = 0;
      core_write (daddr + i, 0, "Disk status register");

    //M [daddr] = SIGN36;
    core_write (daddr, SIGN36, "Disk status register");
#endif
#endif
    p -> charPos = 0;
    p -> stati = 04000;
    if (! unitp -> fileref)
      p -> stati = 04240; // device offline
    p -> initiate = false;
    return 0;
  }

// source/library_dir_dir/system_library_1/source/bound_rcp_.s.archive/rcp_disk_.pl1
//
//  dcl set_standby_command    bit (6) internal static init ("72"b3);
//  dcl request_status_command bit (6) internal static init ("00"b3);
//  dcl read_command           bit (6) internal static init ("25"b3);
//  dcl reset_status_command   bit (6) internal static init ("40"b3);

static int disk_cmd (uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
    uint ctlr_unit_idx = get_ctlr_idx (iomUnitIdx, chan);
    uint devUnitIdx;
    if (cables->iom_to_ctlr[iomUnitIdx][chan].ctlr_type == CTLR_T_IPCD)
       devUnitIdx = cables->ipcd_to_dsk[ctlr_unit_idx][p->IDCW_DEV_CODE].unit_idx;
    else if (cables->iom_to_ctlr[iomUnitIdx][chan].ctlr_type == CTLR_T_MSP)
       devUnitIdx = cables->msp_to_dsk[ctlr_unit_idx][p->IDCW_DEV_CODE].unit_idx;
    else
      {
        sim_warn ("disk_cmd lost\n");
        return IOM_CMD_ERROR;
      }

    UNIT * unitp = & dsk_unit [devUnitIdx];
    struct dsk_state * disk_statep = & dsk_states [devUnitIdx];

#ifdef LOCKLESS
    lock_ptr (& dsk_states->dsk_lock);
#endif

    disk_statep -> io_mode = disk_no_mode;
    p -> stati = 0;

    int rc = IOM_CMD_OK;

    switch (p -> IDCW_DEV_CMD)
      {
        case 000: // CMD 00 Request status
          {
            p -> stati = 04000;
            if (! unitp -> fileref)
              p -> stati = 04240; // device offline

            disk_statep -> io_mode = disk_no_mode;
            sim_debug (DBG_NOTIFY, & dsk_dev, "Request status %d\n", devUnitIdx);
          }
          break;

        case 016: // CMD 16 Read and Clear Statistics -- Model 800
          {
// XXX missing. see poll_mpc.pl1, poll_mpc_data.incl.pl1
            int rc1 = read_and_clear_statistics (devUnitIdx, iomUnitIdx, chan);
            if (rc1)
              {
                rc = IOM_CMD_ERROR;
                break;
              }
            //p -> stati = 04000;
            break;
          }
        case 022: // CMD 22 Read Status Resgister
          {
            int rc1 = readStatusRegister (devUnitIdx, iomUnitIdx, chan);
            if (rc1)
              {
                rc = IOM_CMD_ERROR;
                break;
              }
          }
          break;

        case 024: // CMD 24 Read configuration -- Model 800
          {
// XXX missing. see poll_mpc.pl1, poll_mpc_data.incl.pl1
            int rc1 = read_configuration (devUnitIdx, iomUnitIdx, chan);
            if (rc1)
              {
                rc = IOM_CMD_ERROR;
                break;
              }
            //p -> stati = 04000;
            break;
          }

        case 025: // CMD 25 READ
          {
            // XXX is it correct to not process the DDCWs?
            if (! unitp -> fileref)
              {
                p -> stati = 04240; // device offline
                break;
              }
            int rc1 = diskRead (devUnitIdx, iomUnitIdx, chan);
            if (rc1)
              {
                rc = IOM_CMD_ERROR;
                break;
              }
          }
          break;

        case 030: // CMD 30 SEEK_512
          {
            // XXX is it correct to not process the DDCWs?
            if (! unitp -> fileref)
              {
                p -> stati = 04240; // device offline
                break;
              }
            int rc1 = diskSeek512 (devUnitIdx, iomUnitIdx, chan);
            if (rc1)
              {
                rc = IOM_CMD_ERROR;
                break;
              }
          }
          break;

        case 031: // CMD 31 WRITE
        case 033: // CMD 31 WRITE AND COMPARE
          {
            // XXX is it correct to not process the DDCWs?
            if (! unitp -> fileref)
              {
                p -> stati = 04240; // device offline
                break;
              }
            p -> isRead = false;
            int rc1 = diskWrite (devUnitIdx, iomUnitIdx, chan);
            if (rc1)
              {
                rc = IOM_CMD_ERROR;
                break;
              }

            sim_debug (DBG_NOTIFY, & dsk_dev, "Write %d\n", devUnitIdx);
            disk_statep -> io_mode = disk_write_mode;
//sim_printf ("disk write [%"PRId64"]\n", cpu.cycleCnt);
            p -> stati = 04000;
          }
//exit(1);
          break;

        case 034: // CMD 34 SEEK_64
          {
            // XXX is it correct to not process the DDCWs?
            if (! unitp -> fileref)
              {
                p -> stati = 04240; // device offline
                break;
              }
            int rc1 = diskSeek64 (devUnitIdx, iomUnitIdx, chan);
            if (rc1)
              {
                rc = IOM_CMD_ERROR;
                break;
              }
          }
          break;

        case 040: // CMD 40 Reset status
          {
            p -> stati = 04000;
            p -> initiate = false;
            p -> isRead = false;
            //if (! unitp -> fileref)
              //p -> stati = 04240; // device offline
            disk_statep -> io_mode = disk_no_mode;
            sim_debug (DBG_NOTIFY, & dsk_dev, "Reset status %d\n", devUnitIdx);
          }
          break;

        case 042: // CMD 42 RESTORE
          {
            sim_debug (DBG_NOTIFY, & dsk_dev, "Restore %d\n", devUnitIdx);
            p -> stati = 04000;
            if (! unitp -> fileref)
              p -> stati = 04240; // device offline
          }
          break;

        case 072: // CMD 72 SET STANDBY
          {
            sim_debug (DBG_NOTIFY, & dsk_dev, "Standby %d\n", devUnitIdx);
            p -> stati = 04000;
            if (! unitp -> fileref)
              p -> stati = 04240; // device offline
          }
          break;

        default:
          {
            p -> stati = 04501;
            p -> chanStatus = chanStatIncorrectDCW;
            if (p->IDCW_DEV_CMD != 051) // ignore bootload console probe
              sim_warn ("disk daze %o\n", p->IDCW_DEV_CMD);
            sim_debug (DBG_ERR, & dsk_dev,
                       "%s: Unknown command 0%o\n", __func__, p -> IDCW_DEV_CMD);
            rc = IOM_CMD_ERROR;
          }
          break;
      }

#ifdef LOCKLESS
    unlock_ptr (& dsk_states->dsk_lock);
#endif

    return rc;
  }

int dsk_iom_cmd (uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
    int rc = IOM_CMD_OK;

// Is it an IDCW?

    if (p -> DCW_18_20_CP == 7)
      {

        rc = disk_cmd (iomUnitIdx, chan);
      }
    else // DDCW/TDCW
      {
        sim_printf ("dsk_iom_cmd expected IDCW\n");
        return IOM_CMD_ERROR;
      }
    return rc;
  }

//////////
//////////
//////////
///
/// MSP
///

#define MSP_UNIT_IDX(uptr) ((uptr) - msp_unit)
#define N_MSP_UNITS 1 // default

static struct msp_state
  {
    char device_name [MAX_DEV_NAME_LEN];
  } msp_states [N_MSP_UNITS_MAX];

UNIT msp_unit [N_MSP_UNITS_MAX] =
  {
    [0 ... N_MSP_UNITS_MAX-1] =
      {
        UDATA (NULL, 0, 0),
        0, 0, 0, 0, 0, NULL, NULL, NULL, NULL
      }
  };

static t_stat msp_show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr,
                               UNUSED int val, UNUSED const void * desc)
  {
    sim_printf("Number of MSP units in system is %d\n", msp_dev.numunits);
    return SCPE_OK;
  }

static t_stat msp_set_nunits (UNUSED UNIT * uptr, UNUSED int32 value,
                              const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    int n = atoi (cptr);
    if (n < 0 || n > N_DSK_UNITS_MAX)
      return SCPE_ARG;
    msp_dev.numunits = (uint32) n;
    return SCPE_OK;
  }

static t_stat msp_show_device_name (UNUSED FILE * st, UNIT * uptr, 
                                    UNUSED int val, UNUSED const void * desc)
  {
    int n = (int) MSP_UNIT_IDX (uptr);
    if (n < 0 || n >= N_MSP_UNITS_MAX)
      return SCPE_ARG;
    sim_printf("Controller device name is %s\n", msp_states [n].device_name);
    return SCPE_OK;
  }

static t_stat msp_set_device_name (UNIT * uptr, UNUSED int32 value, 
                                   const char * cptr, UNUSED void * desc)
  {
    int n = (int) MSP_UNIT_IDX (uptr);
    if (n < 0 || n >= N_MSP_UNITS_MAX)
      return SCPE_ARG;
    if (cptr)
      {
        strncpy (msp_states[n].device_name, cptr, MAX_DEV_NAME_LEN-1);
        msp_states[n].device_name[MAX_DEV_NAME_LEN-1] = 0;
      }
    else
      msp_states[n].device_name[0] = 0;
    return SCPE_OK;
  }

static MTAB msp_mod [] =
  {
    {
      MTAB_dev_value, /* mask */
      0,            /* match */
      "NUNITS",     /* print string */
      "NUNITS",         /* match string */
      msp_set_nunits, /* validation routine */
      msp_show_nunits, /* display routine */
      "Number of DISK units in the system", /* value descriptor */
      NULL // Help
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_VALR | MTAB_NC, /* mask */
      0,            /* match */
      "NAME",     /* print string */
      "NAME",         /* match string */
      msp_set_device_name, /* validation routine */
      msp_show_device_name, /* display routine */
      "Set the device name", /* value descriptor */
      NULL          // help
    },
    MTAB_eol
  };

DEVICE msp_dev =
   {
    "MSP",       /*  name */
    msp_unit,    /* units */
    NULL,         /* registers */
    msp_mod,     /* modifiers */
    N_MSP_UNITS, /* #units */
    10,           /* address radix */
    24,           /* address width */
    1,            /* address increment */
    8,            /* data radix */
    36,           /* data width */
    NULL,         /* examine */
    NULL,         /* deposit */ 
    NULL,   /* reset */
    NULL,         /* boot */
    NULL,  /* attach */
    NULL /*disk_detach*/,  /* detach */
    NULL,         /* context */
    0,    /* flags */
    0,            /* debug control flags */
    NULL,      /* debug flag names */
    NULL,         /* memory size change */
    NULL,         /* logical name */
    NULL,         // help
    NULL,         // attach help
    NULL,         // attach context
    NULL,         // description
    NULL
  };

t_stat attach_disk (char * label, bool with_protect, char * drive)
  {
    int i;
    for (i = 0; i < N_DSK_UNITS_MAX; i ++)
      {
        if (strcmp (drive, dsk_states[i].device_name) == 0)
          break;
      }
    if (i >= N_DSK_UNITS_MAX)
      {
        sim_printf ("can't find device named %s\n", drive);
        return SCPE_ARG;
      }
    sim_printf ("attach_disk selected unit %d\n", i);
    load_disk ((uint) i, label, with_protect);
    return SCPE_OK;
  }



#define SECTOR_SZ_IN_W36 512
#define SECTOR_SZ_IN_BYTES ((36 * SECTOR_SZ_IN_W36) / 8)
#define RECORD_SZ_IN_BYTES (2 * SECTOR_SZ_IN_BYTES)

typedef uint8_t record[RECORD_SZ_IN_BYTES];

static uint sect_per_cyl; 
static uint sect_per_rec; 
static uint number_of_sv;

static FILE * setup_disk (uint disk_unit_idx)
  {
    UNIT * unitp = & dsk_unit [disk_unit_idx];
    FILE * fileref = unitp->fileref;
    sect_per_cyl = diskTypes[disk_unit_idx].sects_per_cyl;
    if (diskTypes[disk_unit_idx].sectorSizeWords == 64)
      sect_per_rec = 16;
    else
      sect_per_rec = 2;
    if (disk_unit_idx == 0)
      number_of_sv = 3;
    else
      number_of_sv = 1;
    return fileref;
  }

static uint r2s (uint rec, uint sv)
  {
    uint usable = (sect_per_cyl / sect_per_rec) * sect_per_rec;
    uint unusable = sect_per_cyl - usable;
    uint sect = rec * sect_per_rec;
    sect = sect + (sect / usable) * unusable;

    uint sect_offset = sect % sect_per_cyl;
    sect = (sect - sect_offset) * number_of_sv + sv * sect_per_cyl +
            sect_offset;
    return sect;
  }

static void read_record (FILE * fileref, uint rec, record * data)
  {
    uint sect = r2s (rec, 0);

    off_t n = fseek (fileref, sect * SECTOR_SZ_IN_BYTES, SEEK_SET);
    if (n == (off_t) -1)
      {
        sim_warn ("seek off of end of data\n");
        return;
      }
    size_t r = fread (data, sizeof (record), 1, fileref);
    if (r != 1)
      {
        sim_warn ("fread() returned wrong size\n");
        return;
      }
  }

static char * str (word36 w, char buf[5])
  {
    //static char buf[5];
    buf[0] = (char) ((w >> 27) & 0377);
    buf[1] = (char) ((w >> 18) & 0377);
    buf[2] = (char) ((w >>  9) & 0377);
    buf[3] = (char) ((w >>  0) & 0377);
    buf[4] = 0;
    for (int i = 0; i < 4; i ++)
      if (! isprint (buf[i]))
        buf[i] = '?';
    return buf;
  }

static word36 unstr (char * buf)
  {
    return ((word36) buf[0]) << 27 |
           ((word36) buf[1]) << 18 |
           ((word36) buf[2]) <<  9 |
           ((word36) buf[3]) <<  0;
  }

#if 0
static int fixed_bin (word36 w)
  {
/*
    int ret = (int) (w & MASK17);
    if (w & SIGN18)
      ret = - ret;
*/
    int ret = (int) (w & MASK18);
    if (w & SIGN18)
      {
        ret = (int) (((~(uint) ret) + 1u) & MASK18);
      }
    return ret;
  }
#endif

// Return values;
//   -1 disk_unit_idx not attached
//   n 
static int find_config_deck_partion (FILE * fileref, word36 * frecp, word36 * nrecp)
  {
    if (! fileref)
      return -1;

    record r0;
    memset (& r0, 0, sizeof (record));
    read_record (fileref, 0, & r0);

    word36 w;

#define label_part_os (8*64)
#define label_perm_os (5*64)


    w = extr36 (r0, label_part_os + 3);
    uint nparts = (uint) w;
    if (nparts > 47)
      nparts = 47;
    uint partno;
    word36 frec, nrec;
    for (partno = 0; partno < nparts; partno ++)
      {
        uint pos = partno * 4 + 4;
        w = extr36 (r0, label_part_os + pos + 0);
        frec = extr36 (r0, label_part_os + pos + 1);
        nrec = extr36 (r0, label_part_os + pos + 2);
        if (w == 0143157156146) // "conf"
          break;
      }
    if (partno >= nparts)
      return -1;
    * frecp = frec;
    * nrecp = nrec;
    return (int) partno;
  }

// dcl (CONFIG_DECIMAL_TYPE      init ("11"b),
//      CONFIG_OCTAL_TYPE        init ("00"b),
//      CONFIG_SINGLE_CHAR_TYPE  init ("01"b),
//      CONFIG_STRING_TYPE       init ("10"b)) bit (2) aligned static options (constant);
#define CONFIG_DECIMAL_TYPE      3
#define CONFIG_OCTAL_TYPE        0
#define CONFIG_SINGLE_CHAR_TYPE  1
#define CONFIG_STRING_TYPE       2

// each card is 16 words; 1024 words/record; 64 cards/record
#define wrds_crd 16
#define crds_rec 64

static char * format_field (char * buf, word36 w, uint field_type)
  {
    if (field_type == CONFIG_OCTAL_TYPE)
      sprintf (buf, "%llo", w);
    else if (field_type == CONFIG_SINGLE_CHAR_TYPE)
      sprintf (buf, "%c", (char) (w + 'a' - 1));
    else if (field_type == CONFIG_STRING_TYPE)
      str (w, buf);
    else if (field_type == CONFIG_DECIMAL_TYPE)
      sprintf (buf, "%lld.", w);
    else
      sprintf (buf, "%llo field_type %u?", w, field_type);
    return buf;
  }

static void print_fields (uint8_t * r, uint card_os)
  {
    char word[5];
    word36 w = extr36 (r, card_os + 0);
    word36 type_word = extr36 (r, card_os + 15);
    uint n_fields = (uint) (type_word & 15);
    sim_printf ("%s", str (w, word));
    for (uint option = 0; option < n_fields; option ++)
      {
        uint field_type =  (uint) (type_word >> (34 -(option * 2))) & 3u;
        char ff[256];
        word36 w = extr36 (r, card_os + 1 + option);
        format_field (ff, w, field_type);
        sim_printf (" %s", ff);
      }
    sim_printf ("\n");
  }

static void dump_deck (uint8_t * deck, uint n_cards)
  {
    for (uint cardno = 0; cardno < n_cards; cardno ++)
      {
        char word[5];
        uint card_os = cardno * wrds_crd;
        word36 w = extr36 ((uint8_t *) deck, card_os + 0);
        if (w == 0777777777777lu)
          continue;
        str (w, word);
        print_fields ((uint8_t *) deck, card_os);
      }
  }

t_stat print_config_deck (int32 flag, UNUSED const char * cptr)
  {
    if (! cptr)
      return SCPE_ARG;
    if (! strlen (cptr))
      return SCPE_ARG;

    char * end;
    long n = strtol (cptr, & end, 0);
    if (* end != 0)
      return SCPE_ARG;

    if (n < 0 || n > N_DSK_UNITS_MAX)
      return SCPE_ARG;

    uint disk_unit_idx = (uint) n;
    UNIT * unitp = & dsk_unit [disk_unit_idx];
    FILE * fileref = unitp->fileref;
    sect_per_cyl = diskTypes[disk_unit_idx].sects_per_cyl;
    if (diskTypes[disk_unit_idx].sectorSizeWords == 64)
      sect_per_rec = 16;
    else
      sect_per_rec = 2;
    if (disk_unit_idx == 0)
      number_of_sv = 3;
    else
      number_of_sv = 1;
    word36 conf_frec, conf_nrec;
    int partno = find_config_deck_partion (fileref, & conf_frec, & conf_nrec);
    if (partno < 0)
      {
        sim_warn ("Unable to find conf partition\n");
        return SCPE_ARG;
      }
    for (uint nrec = 0; nrec < conf_nrec; nrec ++)
      {
        record r;
        read_record (fileref, (uint) conf_frec + nrec, & r);
#if 0
        for (uint i = 0; i < 1024; i ++)
          {
            char buf[5];
            word36 w = extr36 (r, i);
            printf ("%012llo %s\n", w, str (w, buf));
          }
#endif

        for (uint cardno = 0; cardno < crds_rec; cardno ++)
          {
            char word[5];
            uint card_os = cardno * wrds_crd;
            word36 w = extr36 (r, card_os + 0);
            if (w == 0777777777777lu)
              continue;
            str (w, word);
            print_fields (r, card_os);
sim_printf ("field_type %012llo\n", extr36 ((uint8_t *) r, card_os + 15));
          }
      }

    return SCPE_OK;
  }

typedef word36 card_t [wrds_crd];

static void write_card (card_t card, uint8_t * deck, uint card_no)
  {
     uint card_os = card_no * wrds_crd;
     for (uint i = 0; i < wrds_crd; i ++)
       put36 (card[i], deck, card_os + i);
  }

static void append_card (card_t card, uint8_t * deck, uint n_cards)
  {
    for (uint card_no = 0; card_no < n_cards; card_no ++)
      {
        uint card_os = card_no * wrds_crd;
        word36 w = extr36 ((uint8_t *) deck, card_os + 0);
        if (w == 0777777777777lu)
          {
             write_card (card, deck, card_no);
             return;
          }
      }
    sim_warn ("append_card couldn't\n");
  }

static void set_field_type (card_t card, uint no, uint val)
  {
    word36 mask = ~ (((word36) 3) << (34 - no * 2));
    word36 bits = ((word36) val) << (34 - no * 2);
    card[15] = (card[15] & mask) | bits;
  }

static void fill_cpu_card (uint8_t * new_deck, uint n_cards, uint cpu_unit_idx, uint port)
  {
    card_t card;
    memset (card, 0, sizeof (card));
    card[15] = 6;
    card[0] = unstr ("cpu ");
    card[1] = cpu_unit_idx + 1u;
    set_field_type (card, 0, CONFIG_SINGLE_CHAR_TYPE);
    card[2] = port;
    set_field_type (card, 1, CONFIG_OCTAL_TYPE);
    if (cpu_unit_idx == 0)
      card[3] = unstr ("on  ");
    else
      card[3] = unstr ("off ");
    set_field_type (card, 2, CONFIG_STRING_TYPE);
#ifdef L68
    card[4] = unstr ("l68 ");
#endif
#ifdef DPS8M
    card[4] = unstr ("dps8");
#endif
    set_field_type (card, 3, CONFIG_STRING_TYPE);
    card[5] = 80; // model
    set_field_type (card, 4, CONFIG_DECIMAL_TYPE);
    card[6] = 32; // cache size
    set_field_type (card, 5, CONFIG_DECIMAL_TYPE);

    append_card (card, new_deck, n_cards);        
//sim_printf ("field_type %012llo\n", card[15]);
//sim_printf ("append cpu\n");
//dump_deck (new_deck, n_cards);
  }

static void fill_iom_card (uint8_t * new_deck, uint n_cards, uint iom_unit_idx, uint port)
  {
    card_t card;
    memset (card, 0, sizeof (card));
    card[15] = 4;
    card[0] = unstr ("iom ");
    card[1] = iom_unit_idx + 1u;
    set_field_type (card, 0, CONFIG_SINGLE_CHAR_TYPE);
    card[2] = port;
    set_field_type (card, 1, CONFIG_OCTAL_TYPE);
#ifdef L68
    card[3] = unstr ("iom ");
#endif
#ifdef DPS8M
    card[3] = unstr ("imu ");
#endif
    set_field_type (card, 2, CONFIG_STRING_TYPE);
    card[4] = unstr ("on  ");
    set_field_type (card, 3, CONFIG_STRING_TYPE);

    append_card (card, new_deck, n_cards);        
  }

static void fill_ipc_card (uint8_t * new_deck, uint n_cards, uint iom_unit_idx, uint chan, uint n_chans)
  {
    card_t card;
    memset (card, 0, sizeof (card));
    card[15] = 4;
    card[0] = unstr ("ipc  ");
    card[1] = unstr ("fips");
    set_field_type (card, 0, CONFIG_STRING_TYPE);
    card[2] = iom_unit_idx + 1u;
    set_field_type (card, 1, CONFIG_SINGLE_CHAR_TYPE);
    card[3] = chan;
    set_field_type (card, 2, CONFIG_OCTAL_TYPE);
    card[4] = n_chans;
    set_field_type (card, 3, CONFIG_OCTAL_TYPE);

    append_card (card, new_deck, n_cards);        
  }

#if 0
static void fill_mpc_card (uint8_t * new_deck, uint n_cards, unit model, uint iom_unit_idx, uint chan, uint n_chans)
  {
    card_t card;
    memset (card, 0, sizeof (card));
    card[15] = 4;
    card[0] = unstr ("ipc  ");
    card[1] = unstr ("fips");
    set_field_type (card, 0, CONFIG_STRING_TYPE);
    card[2] = iom_unit_idx + 1u;
    set_field_type (card, 1, CONFIG_SINGLE_CHAR_TYPE);
    card[3] = chan;
    set_field_type (card, 2, CONFIG_OCTAL_TYPE);
    card[4] = n_chans;
    set_field_type (card, 3, CONFIG_OCTAL_TYPE);

    append_card (card, new_deck, n_cards);        
  }
#endif

#if 0
static void fill_prph_card (uint8_t * new_deck, uint n_cards, uint iom_unit_idx, uint chan, uint n_chans)
  {
    card_t card;
    memset (card, 0, sizeof (card));
    card[15] = 4;
    card[0] = unstr ("ipc  ");
    card[1] = unstr ("fips");
    set_field_type (card, 0, CONFIG_STRING_TYPE);
    card[2] = iom_unit_idx + 1u;
    set_field_type (card, 1, CONFIG_SINGLE_CHAR_TYPE);
    card[3] = chan;
    set_field_type (card, 2, CONFIG_OCTAL_TYPE);
    card[4] = n_chans;
    set_field_type (card, 3, CONFIG_OCTAL_TYPE);

    append_card (card, new_deck, n_cards);        
  }
#endif

static void fill_new_deck (uint8_t * new_deck, uint n_cards)
  {
    card_t card;
    
    // Initialize new_deck to empty
    card [0] = 0777777777777lu;
    for (uint i = 0; i < n_cards; i ++)
      write_card (card, new_deck, i);

    // List the CPUs
    // Assuming SCU 0 is the bootload SCU, walk its list of ports XXX
    // to find connected CPUs.
    // Ignore expander ports XXX
    for (uint port = 0; port < N_SCU_PORTS; port ++)
      {
        struct scu_to_cpu_s * p = &cables->scu_to_cpu[0][N_SCU_PORTS - port - 1][0];
        if (p->in_use)
          {
            fill_cpu_card (new_deck, n_cards, p->cpu_unit_idx, port);
          }
       }
    // List the IOMs
    // Assuming SCU 0 is the bootload SCU, walk its list of ports XXX
    // to find connected IOMs.

    bool ioms_used [N_IOM_UNITS_MAX]; 
    memset (ioms_used, 0, sizeof (ioms_used));

    for (uint port = 0; port < N_SCU_PORTS; port ++)
      {
        struct scu_to_iom_s * p = &cables->scu_to_iom[0][port];
        if (p->in_use)
          {
            ioms_used[p->iom_unit_idx] = true;
            fill_iom_card (new_deck, n_cards, p->iom_unit_idx, port);
           }
       }

     // Controllers

     for (uint iom_unit_idx = 0; iom_unit_idx < N_IOM_UNITS_MAX; iom_unit_idx ++)
       {
         if (! ioms_used[iom_unit_idx])
           continue;

//     struct iom_to_ctlr_s iom_to_ctlr [N_IOM_UNITS_MAX] [MAX_CHANNELS];
         for (uint chan = 0; chan < MAX_CHANNELS; chan ++)
           {
             struct iom_to_ctlr_s * p = & cables->iom_to_ctlr[iom_unit_idx][chan];
             if (! p->in_use)
               continue;

             enum ctlr_type_e ctlr_type = p->ctlr_type;
             UNIT * board = p->board;
             // Count channels
             uint n_chan = 1;
             for (uint c2 = chan + 1; c2 < MAX_CHANNELS; c2 ++)
               {
                 struct iom_to_ctlr_s * p2 = & cables->iom_to_ctlr[iom_unit_idx][c2];
                 if (! p2->in_use)
                   break;
                 if (p2->board != board)
                   break;
                 n_chan ++;
               }
             switch (ctlr_type)
               {
                 case CTLR_T_NONE:
                   break;

                 case CTLR_T_MTP:
                   //fill_prph_card (new_deck, n_cards, iom_unit_idx, chan, n_chan);
                   break;

                 case CTLR_T_MSP:
                   break;

                 case CTLR_T_IPCD:
                 case CTLR_T_IPCT:
                   fill_ipc_card (new_deck, n_cards, iom_unit_idx, chan, n_chan);
                   break;

                 case CTLR_T_OPC:
                   break;
                 case CTLR_T_URP:
                   break;
                 case CTLR_T_FNP:
                   break;
                 case CTLR_T_ABSI:
                   break;
                 case CTLR_T_SKC:
                   break;
               }
             chan += n_chan - 1;
           }
       }
   
  }

t_stat sync_config_deck (int32 flag, UNUSED const char * cptr)
  {
    if (! cptr)
      return SCPE_ARG;
    if (! strlen (cptr))
      return SCPE_ARG;

    char * end;
    long n = strtol (cptr, & end, 0);
    if (* end != 0)
      return SCPE_ARG;

    if (n < 0 || n > N_DSK_UNITS_MAX)
      return SCPE_ARG;

    uint disk_unit_idx = (uint) n;
    FILE * fileref = setup_disk (disk_unit_idx);
    if (! fileref)
      {
         sim_warn ("Unable to access disk image\n");
         return SCPE_ARG;
      }

    word36 conf_frec, conf_nrec;
    int partno = find_config_deck_partion (fileref, & conf_frec, & conf_nrec);
    if (partno < 0)
      {
        sim_warn ("Unable to find conf partition\n");
        return SCPE_ARG;
      }

    uint n_cards = (uint) conf_nrec * crds_rec;

    uint8_t new_deck [conf_nrec * sizeof (record)];

    fill_new_deck (new_deck, n_cards);

    record deck [conf_nrec];

    for (uint nrec = 0; nrec < conf_nrec; nrec ++)
      {
        read_record (fileref, (uint) conf_frec + nrec, deck + nrec);
      }

    for (uint cardno = 0; cardno < n_cards; cardno ++)
      {
        char word[5];
        uint card_os = cardno * wrds_crd;
        word36 w = extr36 ((uint8_t *) new_deck, card_os + 0);
        if (w == 0777777777777lu)
          continue;
        str (w, word);
        print_fields ((uint8_t *) new_deck, card_os);
      }

    return SCPE_OK;
  }

