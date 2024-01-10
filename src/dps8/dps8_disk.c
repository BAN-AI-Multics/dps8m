/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * SPDX-License-Identifier: Multics
 * scspell-id: 22079097-f62e-11ec-b987-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2018 Charles Anthony
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

//-V::536

// source/library_dir_dir/system_library_1/source/bound_volume_rldr_ut_.s.archive/rdisk_.pl1
// source/library_dir_dir/system_library_1/source/bound_rcp_.s.archive/rcp_disk_.pl1

#include <stdio.h>

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

#ifdef LOCKLESS
# include "threadz.h"
#endif

#define DBG_CTR 1

//
// A possible disk data packing algorithm
//
// Currently sectors are 512 * word36. Word36 is 64bits; that 56% utilization,
// A computationally efficient packing would improve disk throughput and storage
// efficiency
//
//     word36 in[512]
//     struct dsksec
//       {
//         uint32 low32[512];
//         uint8 high4[512];
//       } out;
//     uint32 * map = (uint32 *) in;
//     uint8 * highmap = (uint8 *) in;
//
//     for (uint i = 0; i < 512; i ++)
//       {
//         out . low32[i] = map[i * 2];
//         out . high4[i] = (uint8) (map[i * 2 + 1]);
//       }
//
// This 36/40 -- 90% utilization; at the cost of the scatter/gather and
// the cost of emulated disk sectors size not a multiple of host disk
// sector size.
//

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

// fs_dev.sect_per_dev:
//           vfd       36/4000000          Bulk
//           vfd       36/814*40*19        MSU0500
//           vfd       36/814*40*19        MSU0450
//           vfd       36/410*40*19        MSU0400
//           vfd       36/410*31*19        DSU190
//           vfd       36/202*18*20        DSU181
//           vfd       36/840*64*20        MSU0501
//           vfd       36/885*255          FIPS 3380
//           vfd       36/1770*255         FIPS 3381

//dcl  last_sect_num (9) fixed bin (24) static options (constant) init /* table of # last sector number for each device */
//     (0, 618639, 616359, 309319, 239722, 71999, 1075199, 225674, 451349);
//dcl  last_physical_sect_num (9) fixed bin (24) static options (constant) init /* table of # of last sector on device (includes T&D cylinders) */
//     (0, 639919, 619399, 312359, 242249, 72359, 1077759, 225674, 451859);

//          last     t&d last
// D500     618639    639919
// D451     616359    619399
// D400     309319    312359
// D190     239722    242249
// D181      71999     72359
// D501    1075199   1077759
// 3380     225674    225674
// 3381     451349    451859

struct diskType_t diskTypes[] = {
  { "3381",  451350,  0, false, seek_512, 512,   0 }, // disk_init assumes 3381 is at index 0
  { "d500",  618640,  1, false, seek_64,   64, 200 },
  { "d451",  616360,  1, true,  seek_64,   64, 154 },
  { "d400",  309320,  1, true,  seek_64,   64,  84 }, // d400 is a d190 with "high-efficiency format (40 sectors/track)"
  { "d190",  239723,  1, true,  seek_64,   64,  84 }, // 190A 84, 190B 137
  { "d181",   72000,  1, true,  seek_64,   64,   0 }, // no idea what the dau idx is
  { "d501", 1075200,  1, false, seek_64,   64, 201 },
  { "3380",  225675,  0, false, seek_512, 512,   0 }, // 338x is never attached to a dau
};
#define N_DISK_TYPES (sizeof (diskTypes) / sizeof (struct diskType_t))

static uint tAndDCapac[N_DISK_TYPES] = { 451860, 639920, 619400, 312360, 242250, 72360, 1077760, 225675 };

#define M3381_SECTORS (1770*255)
//#define M3381_SECTORS 6895616
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
//  Search Identifier Equal [CC HH R]
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
//     These bits indicate a complete track as good, defective, or alternate.
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

#ifdef POLTS_DISK_TESTING
static int nCmds = 0;
#endif

#define UNIT_FLAGS ( UNIT_FIX | UNIT_ATTABLE | UNIT_ROABLE | \
                     UNIT_DISABLE | UNIT_IDLE | DKUF_F_STD )
UNIT dsk_unit[N_DSK_UNITS_MAX] = {
#ifdef NO_C_ELLIPSIS
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL }
#else
  [0 ... N_DSK_UNITS_MAX-1] = {
    UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL
  }
#endif
};

#define DSK_UNIT_IDX(uptr) ((uptr) - dsk_unit)

static DEBTAB disk_dt[] =
  {
    { "TRACE",  DBG_TRACE,  NULL },
    { "NOTIFY", DBG_NOTIFY, NULL },
    { "INFO",   DBG_INFO,   NULL },
    { "ERR",    DBG_ERR,    NULL },
    { "WARN",   DBG_WARN,   NULL },
    { "DEBUG",  DBG_DEBUG,  NULL },
    { "ALL",    DBG_ALL,    NULL }, // Don't move as it messes up DBG message
    { NULL,     0,          NULL }
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
    if (n < 1 || n > N_DSK_UNITS_MAX)
      return SCPE_ARG;
    dsk_dev . numunits = (uint32) n;
    return SCPE_OK;
  }

static t_stat disk_show_type (UNUSED FILE * st, UNUSED UNIT * uptr, UNUSED int val, UNUSED const void * desc)
  {
    int diskUnitIdx = (int) DSK_UNIT_IDX (uptr);
    if (diskUnitIdx < 0 || diskUnitIdx >= N_DSK_UNITS_MAX)
      {
        sim_printf ("error: Invalid unit number %ld\n", (long) diskUnitIdx);
        return SCPE_ARG;
      }

    sim_printf("type     : %s", diskTypes[dsk_states[diskUnitIdx].typeIdx].typename);

    return SCPE_OK;
  }

static t_stat disk_set_type (UNUSED UNIT * uptr, UNUSED int32 value, const char * cptr, UNUSED void * desc)
  {
    int diskUnitIdx = (int) DSK_UNIT_IDX (uptr);
    if (diskUnitIdx < 0 || diskUnitIdx >= N_DSK_UNITS_MAX)
      {
        sim_printf ("error: Invalid unit number %ld\n", (long) diskUnitIdx);
        return SCPE_ARG;
      }

    uint i;
    for (i = 0; i < N_DISK_TYPES; i++)
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
    dsk_unit[diskUnitIdx].capac = (t_addr) diskTypes[i].capac;
    dsk_states[diskUnitIdx].tAndDCapac = tAndDCapac[i];
    return SCPE_OK;
  }

static t_stat dsk_show_device_name (UNUSED FILE * st, UNIT * uptr,
                                    UNUSED int val, UNUSED const void * desc)
  {
    int n = (int) DSK_UNIT_IDX (uptr);
    if (n < 0 || n >= N_DSK_UNITS_MAX)
      return SCPE_ARG;
    sim_printf("name     : %s", dsk_states[n].device_name);
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

//
// Looking at rcp_disk.pl1, it appears that the special interrupt for disks
// only causes rcp to poll the device for status; there don't appear to
// be any status specific bits to send here; signal ready is a misnomer;
// it is just signal.

t_stat signal_disk_ready (uint dsk_unit_idx) {

  // Don't signal if the sim is not actually running....
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
      //sim_printf ("%s %d %o\n", disk_filename, ro,  mt_unit[dsk_unit_idx] . flags);
      //sim_printf ("special int %d %o\n", dsk_unit_idx, mt_unit[dsk_unit_idx] . flags);

  uint ctlr_unit_idx = cables->dsk_to_ctlr[dsk_unit_idx].ctlr_unit_idx;
  enum ctlr_type_e ctlr_type = cables->dsk_to_ctlr[dsk_unit_idx].ctlr_type;
  if (ctlr_type != CTLR_T_MSP && ctlr_type != CTLR_T_IPC) {
    // If None, assume that the cabling hasn't happened yet.
    if (ctlr_type != CTLR_T_NONE) {
      sim_warn ("loadDisk lost\n");
      return SCPE_ARG;
    }
    return SCPE_OK;
  }

#if 0
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
            if (cables->ipc_to_iom[ctlr_unit_idx][ctlr_port_num].in_use)
              {
                uint iom_unit_idx = cables->ipc_to_iom[ctlr_unit_idx][ctlr_port_num].iom_unit_idx;
                uint chan_num = cables->ipc_to_iom[ctlr_unit_idx][ctlr_port_num].chan_num;
                uint dev_code = cables->dsk_to_ctlr[dsk_unit_idx].dev_code;

                send_special_interrupt (iom_unit_idx, chan_num, dev_code, 0x40, 01 /* disk pack ready */);
                sent_one = true;
              }
          }
      }
    if (! sent_one)
      {
        sim_printf ("loadDisk can't find controller; dropping interrupt\n");
        return SCPE_ARG;
      }

    return SCPE_OK;
#else
  for (uint ctlr_port_num = 0; ctlr_port_num < MAX_CTLR_PORTS; ctlr_port_num ++) {
    if (ctlr_type == CTLR_T_MSP) {
      if (cables->msp_to_iom[ctlr_unit_idx][ctlr_port_num].in_use) {
         uint iom_unit_idx = cables->msp_to_iom[ctlr_unit_idx][ctlr_port_num].iom_unit_idx;
         uint chan_num = cables->msp_to_iom[ctlr_unit_idx][ctlr_port_num].chan_num;
         uint dev_code = cables->dsk_to_ctlr[dsk_unit_idx].dev_code;

         send_special_interrupt (iom_unit_idx, chan_num, dev_code, 0x40, 01 /* disk pack ready */);
         return SCPE_OK;
       }
     } else {
       if (cables->ipc_to_iom[ctlr_unit_idx][ctlr_port_num].in_use) {
         uint iom_unit_idx = cables->ipc_to_iom[ctlr_unit_idx][ctlr_port_num].iom_unit_idx;
         uint chan_num = cables->ipc_to_iom[ctlr_unit_idx][ctlr_port_num].chan_num;
         uint dev_code = cables->dsk_to_ctlr[dsk_unit_idx].dev_code;

         send_special_interrupt (iom_unit_idx, chan_num, dev_code, 0x40, 01 /* disk pack ready */);
         return SCPE_OK;
       }
     }
   }
   return SCPE_ARG;
#endif
}

static t_stat disk_set_ready (UNIT * uptr, UNUSED int32 value,
                              UNUSED const char * cptr,
                              UNUSED void * desc)
  {
    long disk_unit_idx = DSK_UNIT_IDX (uptr);
    if (disk_unit_idx >= (long) dsk_dev.numunits)
      {
        sim_warn ("%s: error: Invalid unit number %ld\n", __func__, (long) disk_unit_idx);
        return SCPE_ARG;
      }
    return signal_disk_ready ((uint) disk_unit_idx);
  }

t_stat unloadDisk (uint dsk_unit_idx) {
  if (dsk_unit [dsk_unit_idx] . flags & UNIT_ATT) {
    t_stat stat = sim_disk_detach (& dsk_unit [dsk_unit_idx]);
    if (stat != SCPE_OK) {
      sim_warn ("%s: sim_disk_detach returned %ld\n", __func__, (long) stat);
      return SCPE_ARG;
    }
  }
  return signal_disk_ready ((uint) dsk_unit_idx);
}

t_stat loadDisk (uint dsk_unit_idx, const char * disk_filename, bool ro) {
  if (ro)
    dsk_unit[dsk_unit_idx].flags |= MTUF_WRP;
  else
    dsk_unit[dsk_unit_idx].flags &= ~ MTUF_WRP;
  t_stat stat = attach_unit (& dsk_unit [dsk_unit_idx], disk_filename);
  if (stat != SCPE_OK) {
    sim_printf ("%s: sim_disk_attach returned %d\n", __func__, stat);
    return stat;
  }
  return signal_disk_ready ((uint) dsk_unit_idx);
}

#define UNIT_WATCH UNIT_V_UF

static MTAB disk_mod[] =
  {
#ifndef SPEED
    { UNIT_WATCH, 1, "WATCH",   "WATCH",   0, 0, NULL, NULL },
    { UNIT_WATCH, 0, "NOWATCH", "NOWATCH", 0, 0, NULL, NULL },
#endif
    {
      MTAB_dev_value,                                   /* Mask               */
      0,                                                /* Match              */
      "NUNITS",                                         /* Print string       */
      "NUNITS",                                         /* Match string       */
      disk_set_nunits,                                  /* Validation routine */
      disk_show_nunits,                                 /* Display routine    */
      "Number of DISK units in the system",             /* Value descriptor   */
      NULL                                              /* Help               */
    },
    {
      MTAB_unit_value_show,                             /* Mask               */
      0,                                                /* Match              */
      "TYPE",                                           /* Print string       */
      "TYPE",                                           /* Match string       */
      disk_set_type,                                    /* Validation routine */
      disk_show_type,                                   /* Display routine    */
      "disk type",                                      /* Value descriptor   */
      "D500, D451, D400, D190, D181, D501, 3380, 3381", /* Help               */
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_VALR | MTAB_NC,        /* Mask               */
      0,                                                /* Match              */
      "NAME",                                           /* Print string       */
      "NAME",                                           /* Match string       */
      dsk_set_device_name,                              /* Validation routine */
      dsk_show_device_name,                             /* Display routine    */
      "Set the device name",                            /* Value descriptor   */
      NULL                                              /* Help               */
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_NMO | MTAB_VALR,       /* Mask               */
      0,                                                /* Match              */
      "READY",                                          /* Print string       */
      "READY",                                          /* Match string       */
      disk_set_ready,                                   /* Validation routine */
      NULL,                                             /* Display routine    */
      NULL,                                             /* Value descriptor   */
      NULL                                              /* Help string        */
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
        sim_printf ("error: Invalid unit number %ld\n", (long) diskUnitIdx);
        return SCPE_ARG;
      }

    return loadDisk ((uint) diskUnitIdx, cptr, false);
  }

// No disks known to Multics had more than 2^24 sectors...
DEVICE dsk_dev = {
    "DISK",                /* Name                */
    dsk_unit,              /* Units               */
    NULL,                  /* Registers           */
    disk_mod,              /* Modifiers           */
    N_DISK_UNITS,          /* #Units              */
    10,                    /* Address radix       */
    24,                    /* Address width       */
    1,                     /* Address increment   */
    8,                     /* Data radix          */
    36,                    /* Data width          */
    NULL,                  /* Examine             */
    NULL,                  /* Deposit             */
    disk_reset,            /* Reset               */
    NULL,                  /* Boot                */
    disk_attach,           /* Attach              */
    NULL /*disk_detach*/,  /* Detach              */
    NULL,                  /* Context             */
    DEV_DEBUG,             /* Flags               */
    0,                     /* Debug control flags */
    disk_dt,               /* Debug flag names    */
    NULL,                  /* Memory size change  */
    NULL,                  /* Logical name        */
    NULL,                  /* Help                */
    NULL,                  /* Attach help         */
    NULL,                  /* Attach context      */
    NULL,                  /* Description         */
    NULL                   /* End                 */
};

/*
 * disk_init()
 */

// Once-only initialization

void disk_init (void)
  {
    // Sets diskTypeIdx to 0: 3381
    memset (dsk_states, 0, sizeof (dsk_states));
#ifdef LOCKLESS
# if defined ( __FreeBSD__ )
        pthread_mutexattr_t scu_attr;
        pthread_mutexattr_init (& scu_attr);
        pthread_mutexattr_settype (& scu_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
# endif
    for (uint i = 0; i < N_DSK_UNITS_MAX; i ++)
      {
# if defined ( __FreeBSD__ )
        pthread_mutex_init (& dsk_states[i].dsk_lock, & scu_attr);
# else
        pthread_mutex_init (& dsk_states[i].dsk_lock, NULL);
# endif
      }
#endif
  }

static iom_cmd_rc_t diskSeek64 (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p            = & iom_chan_data[iomUnitIdx][chan];
    struct dsk_state * disk_statep = & dsk_states[devUnitIdx];

    uint typeIdx                     = disk_statep->typeIdx;
    if (diskTypes[typeIdx].seekSize != seek_64)
      sim_warn ("%s: disk%u sent a SEEK_64 but is 512 sized\n", __func__, typeIdx);

    uint tally = p->DDCW_TALLY;

    // Seek specific processing

    if (tally != 1)
      {
        sim_printf ("disk seek dazed by tally %d != 1\n", tally);
        p->stati = 04510; // Cmd reject, invalid inst. seq.
        return IOM_CMD_ERROR;
      }

    word36 seekData;
    uint count;
    iom_indirect_data_service (iomUnitIdx, chan, & seekData, &count, false);
    // POLTS claims that seek data doesn't count as an I/O xfer
    p->initiate = true;
    if (count != 1)
      sim_warn ("%s: count %d not 1\n", __func__, count);

#ifdef POLTS_DISK_TESTING
    if_sim_debug (DBG_TRACE, & dsk_dev) { sim_printf ("// Seek address %012"PRIo64"\n", seekData); }
#endif

// disk_control.pl1:
//   quentry.sector = bit (sector, 21);  /* Save the disk device address. */
// suggests seeks are 21 bits. The largest sector number is D501 1077759; 4040777 in base 8;
// 21 bits.

    seekData &= MASK21;
    if (seekData >= diskTypes[typeIdx].capac)
      {
        disk_statep->seekValid = false;
        p->stati               = 04304; // Invalid seek address
        return IOM_CMD_ERROR;
      }
    disk_statep->seekValid    = true;
    disk_statep->seekPosition = (uint) seekData;
    p->stati = 04000; // Channel ready
    return IOM_CMD_PROCEED;
  }

static int diskSeek512 (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p            = & iom_chan_data[iomUnitIdx][chan];
    struct dsk_state * disk_statep = & dsk_states[devUnitIdx];
    uint typeIdx                   = disk_statep->typeIdx;
    if (diskTypes[typeIdx].seekSize != seek_512)
      sim_warn ("%s: disk%u sent a SEEK_512 but is 64 sized\n", __func__, typeIdx);

    uint tally = p->DDCW_TALLY;

    // Seek specific processing

    if (tally != 1)
      {
        sim_printf ("disk seek dazed by tally %d != 1\n", tally);
        //p->stati = 04510; // Cmd reject, invalid inst. seq.
        //p->chanStatus = chanStatIncorrectDCW;
        //return -1;
        tally = 1;
      }

    word36 seekData;
    uint count;
    iom_indirect_data_service (iomUnitIdx, chan, & seekData, &count, false);
    // POLTS claims that seek data doesn't count as an I/O xfer
    p->initiate = true;
    if (count != 1)
      sim_warn ("%s: count %d not 1\n", __func__, count);

    seekData &= MASK21;
    if (seekData >= diskTypes[typeIdx].capac)
      {
        disk_statep->seekValid = false;
        p->stati = 04304; // Invalid seek address
        return -1;
      }
    disk_statep->seekValid    = true;
    disk_statep->seekPosition = (uint)seekData;
    p->stati                  = 04000; // Channel ready
    return 0;
  }

static iom_cmd_rc_t diskSeekSpecial (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p            = & iom_chan_data[iomUnitIdx][chan];
    struct dsk_state * disk_statep = & dsk_states[devUnitIdx];

    uint typeIdx = disk_statep->typeIdx;
    //if (diskTypes[typeIdx].seekSize != seek_64)
      //sim_warn ("%s: disk%u sent a SEEK_64 but is 512 sized\n", __func__, typeIdx);

    uint tally = p->DDCW_TALLY;

    // Seek specific processing

    if (tally != 1)
      {
        sim_printf ("disk seek dazed by tally %d != 1\n", tally);
        //p->stati = 04510; // Cmd reject, invalid inst. seq.
        //p->chanStatus = chanStatIncorrectDCW;
        //return IOM_CMD_ERROR;
        tally = 1;
      }

    word36 seekData;
    uint count;
    iom_indirect_data_service (iomUnitIdx, chan, & seekData, &count, false);
    // POLTS claims that seek data doesn't count as an I/O xfer
    p->initiate  = true;
    if (count   != 1)
      sim_warn ("%s: count %d not 1\n", __func__, count);

#ifdef POLTS_DISK_TESTING
    if_sim_debug (DBG_TRACE, & dsk_dev)
      {
        sim_printf ("// Seek address %012"PRIo64"\n", seekData);
      }
#endif

    seekData &= MASK21;
    if (seekData >= dsk_states[typeIdx].tAndDCapac)
      {
        p->stati               = 04304; // Invalid seek address
        disk_statep->seekValid = false;
        //p->chanStatus = chanStatIncomplete;
        return IOM_CMD_ERROR;
      }
    disk_statep->seekValid    = true;
    disk_statep->seekPosition = (uint) seekData;
    p->stati                  = 04000; // Channel ready
    return IOM_CMD_PROCEED;
  }

static int diskRead (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p            = & iom_chan_data[iomUnitIdx][chan];
    UNIT * unitp                   = & dsk_unit[devUnitIdx];
    struct dsk_state * disk_statep = & dsk_states[devUnitIdx];
    uint typeIdx                   = disk_statep->typeIdx;
    uint sectorSizeWords           = diskTypes[typeIdx].sectorSizeWords;
    uint sectorSizeBytes           = ((36 * sectorSizeWords) / 8);

    if (! unitp->fileref) {
      p->stati = 04240; // device offline
#ifdef POLTS_TESTING
if (chan == 014)
    if_sim_debug (DBG_TRACE, & dsk_dev) sim_printf ("// diskRead device offline\r\n");
#endif
      return -1;
    }
    if (! disk_statep->seekValid) {
      p->stati               = 04510; // Invalid instruction sequence
      disk_statep->seekValid = false;
#ifdef POLTS_TESTING
if (chan == 014)
    if_sim_debug (DBG_TRACE, & dsk_dev) sim_printf ("// diskRead seek invalid\r\n");
#endif
      return IOM_CMD_ERROR;
    }
    uint tally = p->DDCW_TALLY;
    if (tally == 0)
      {
        tally = 4096;
      }

    int rc = fseek (unitp->fileref,
                (long) ((long)disk_statep->seekPosition * (long)sectorSizeBytes),
                SEEK_SET);
    if (rc)
      {
        sim_warn ("%s: fseek (read) returned %d, errno %d\n", __func__, rc, errno);
        p->stati = 04202; // attn, seek incomplete
#ifdef POLTS_TESTING
if (chan == 014)
    if_sim_debug (DBG_TRACE, & dsk_dev) sim_printf ("// diskRead seek incomplete\r\n");
#endif
        return -1;
      }

    // Convert from word36 format to packed72 format

    // round tally up to sector boundary

    // this math assumes tally is even.

    uint tallySectors = (tally + sectorSizeWords - 1) /
                         sectorSizeWords;
    uint tallyWords   = tallySectors * sectorSizeWords;
    //uint tallyBytes = tallySectors * sectorSizeBytes;
    uint p72ByteCnt   = (tallyWords * 36) / 8;
    uint8 diskBuffer[p72ByteCnt];
    memset (diskBuffer, 0, sizeof (diskBuffer));

    fflush (unitp->fileref);
    rc = (int) fread (diskBuffer, sectorSizeBytes,
                tallySectors,
                unitp->fileref);

    if (rc == 0) // EOF or error
      {
        if (ferror (unitp->fileref))
          {
            p->stati = 04202; // attn, seek incomplete
            //p->chanStatus = chanStatIncorrectDCW;
#ifdef POLTS_TESTING
if (chan == 014)
    if_sim_debug (DBG_TRACE, & dsk_dev) sim_printf ("// diskRead seek incomplete2\r\n");
#endif
            return -1;
          }
        // We ignore short reads-- we assume that they are reads
        // past the write highwater mark, and return zero data,
        // just as if the disk had been formatted with zeros.
      }
    disk_statep->seekPosition += tallySectors;

    uint wordsProcessed = 0;
    word36 buffer[tally];
    for (uint i = 0; i < tally; i ++)
      {
        word36 w;
        extractWord36FromBuffer (diskBuffer, p72ByteCnt, & wordsProcessed,
                                 & w);
        buffer[i] = w;
      }
    iom_indirect_data_service (iomUnitIdx, chan, buffer,
                            & wordsProcessed, true);
    p->stati = 04000;
#ifdef POLTS_TESTING
if (chan == 014)
    if_sim_debug (DBG_TRACE, & dsk_dev) sim_printf ("// diskRead ok\r\n");
#endif
    return 0;
  }

static int diskWrite (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p            = & iom_chan_data[iomUnitIdx][chan];
    UNIT * unitp                   = & dsk_unit[devUnitIdx];
    struct dsk_state * disk_statep = & dsk_states[devUnitIdx];
    uint typeIdx                   = disk_statep->typeIdx;
    uint sectorSizeWords           = diskTypes[typeIdx].sectorSizeWords;
    uint sectorSizeBytes           = ((36 * sectorSizeWords) / 8);

    if (! unitp->fileref) {
      p->stati = 04240; // device offline
      return -1;
    }

    if (! disk_statep->seekValid) {
      p->stati               = 04510; // Invalid instruction sequence
      disk_statep->seekValid = false;
      return IOM_CMD_ERROR;
    }

    uint tally = p->DDCW_TALLY;

    if (tally == 0)
      {
        tally = 4096;
      }

    int rc = fseek (unitp->fileref,
                (long) ((long)disk_statep->seekPosition * (long)sectorSizeBytes),
                SEEK_SET);
    if (rc)
      {
        sim_printf ("fseek (read) returned %d, errno %d\n", rc, errno);
        p->stati = 04202; // attn, seek incomplete
        return -1;
      }

    // Convert from word36 format to packed72 format

    // round tally up to sector boundary

    // this math assumes tally is even.

    uint tallySectors = (tally + sectorSizeWords - 1) /
                         sectorSizeWords;
    uint tallyWords   = tallySectors * sectorSizeWords;
    uint p72ByteCnt   = (tallyWords * 36) / 8;
    uint8 diskBuffer[p72ByteCnt];
    memset (diskBuffer, 0, sizeof (diskBuffer));
    uint wordsProcessed = 0;

    word36 buffer[tally];
    iom_indirect_data_service (iomUnitIdx, chan, buffer,
                            & wordsProcessed, false);
    wordsProcessed = 0;
    for (uint i = 0; i < tally; i ++)
      {
        insertWord36toBuffer (diskBuffer, p72ByteCnt, & wordsProcessed,
                              buffer[i]);
      }

    rc = (int) fwrite (diskBuffer, sectorSizeBytes,
                 tallySectors,
                 unitp->fileref);
    fflush (unitp->fileref);

    if (rc != (int) tallySectors)
      {
        sim_printf ("fwrite returned %d, errno %d\n", rc, errno);
        p->stati      = 04202; // attn, seek incomplete
        p->chanStatus = chanStatIncorrectDCW;
        return -1;
      }

    disk_statep->seekPosition += tallySectors;

    p->stati = 04000;
    return 0;
  }

static int readStatusRegister (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan];
    UNIT * unitp        = & dsk_unit[devUnitIdx];

    uint tally = p->DDCW_TALLY;

    if (tally != 4 && tally != 2)
      {
        sim_warn ("%s: RSR expected tally of 2 or 4, is %d\n",
                  __func__, tally);
      }
    if (tally == 0)
      {
        tally = 4096;
      }

// XXX need status register data format
// system_library_tools/source/bound_io_tools_.s.archive/analyze_detail_stat_.pl1  anal_fips_disk_().

#ifdef TESTING
    sim_warn ("Need status register data format\n");
#endif
    word36 buffer[tally];
    memset (buffer, 0, sizeof (buffer));
#ifdef POLTS_DISK_TESTING
buffer[0] = nCmds;
#endif
    uint wordsProcessed = tally;
    iom_indirect_data_service (iomUnitIdx, chan, buffer,
                            & wordsProcessed, true);
    p->charPos = 0;
    p->stati   = 04000;
    if (! unitp->fileref)
      p->stati = 04240; // device offline
    return 0;
  }

static int diskRdCtrlReg (uint dev_unit_idx, uint iom_unit_idx, uint chan) {
  iom_chan_data_t * p = & iom_chan_data[iom_unit_idx][chan];
  UNIT * unitp        = & dsk_unit[dev_unit_idx];
  p->charPos          = 0;
  p->stati            = 04000;
  if (! unitp->fileref)
    p->stati = 04240; // device offline
  return 0;
}

static int read_configuration (uint dev_unit_idx, uint iom_unit_idx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data[iom_unit_idx][chan];
    UNIT * unitp        = & dsk_unit[dev_unit_idx];

    uint tally = p->DDCW_TALLY;

// poll_mpc.pl1
//
// dcl  1 dau_buf aligned based (workp),   /* The IOI buffer segment */
//        2 cf_idcw bit (36),              /* Read Configuration (24o) */
//        2 cf_dcw bit (36),               /* Addr=dau_buf.data(0), tally=65 */
//        2 st_idcw bit (36),              /* Read/Clear Statistics (16o) */
//        2 st_dcw bit (36),               /* Address=dau_buf.data(130), tally=315 */
//        2 data (0:759) bit (18) unal;    /* Config & statistics area */

// XXX need status register data format
// system_library_tools/source/bound_io_tools_.s.archive/analyze_detail_stat_.pl1  anal_fips_disk_().

// poll_mpc.pl1
//
// dcl  1 dau_buf aligned based (workp),   /* The IOI buffer segment */
//        2 cf_idcw bit (36),              /* Read Configuration (24o) */
//        2 cf_dcw bit (36),               /* Addr=dau_buf.data(0), tally=65 */
//        2 st_idcw bit (36),              /* Read/Clear Statistics (16o) */
//        2 st_dcw bit (36),               /* Address=dau_buf.data(130), tally=315 */
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
    if (cables->iom_to_ctlr[iom_unit_idx][chan].ctlr_type == CTLR_T_IPC)
      dev_p = cables->ipc_to_dsk[ctlr_unit_idx];
    else
      dev_p = cables->msp_to_dsk[ctlr_unit_idx];

// XXX Temp
    word36 buffer[tally];
    memset (buffer, 0, sizeof (buffer));
    putbits36_9 (& buffer[0],  0, 0x12);
    putbits36_9 (& buffer[0],  9, 1);    // h/w revision
    putbits36_9 (& buffer[0], 18, '1');  // fw maj revision
    putbits36_9 (& buffer[0], 27, 'a');  // fw sub revision

    for (word9 dev_num = 0; dev_num < N_DEV_CODES; dev_num ++)
      {
         if (! dev_p[dev_num].in_use)
           continue;
         uint dsk_unit_idx = dev_p[dev_num].unit_idx;
         // word9 dau_type    = (word9) diskTypes[dsk_unit_idx].dau_type;
         // ubsan/asan
         word9 dau_type    = (word9) diskTypes[dsk_states[dsk_unit_idx].typeIdx].dau_type;
         putbits36_9 (& buffer[1+dev_num],  0, dau_type); // dev.type
         putbits36_9 (& buffer[1+dev_num],  9, dev_num);  // dev.number
         putbits36_9 (& buffer[1+dev_num], 18, 0);        // dev.summary_status // XXX
         putbits36_9 (& buffer[1+dev_num], 27, dev_num);  // dev.port_number
     }

    uint wordsProcessed = tally;
    iom_indirect_data_service (iom_unit_idx, chan, buffer,
                            & wordsProcessed, true);
    p->charPos = 0;
    p->stati   = 04000;
    if (! unitp->fileref)
      p->stati = 04240; // device offline
    return 0;
  }

static int read_and_clear_statistics (uint dev_unit_idx, uint iom_unit_idx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data[iom_unit_idx][chan];
    UNIT * unitp        = & dsk_unit[dev_unit_idx];

    sim_debug (DBG_NOTIFY, & dsk_dev, "Read %d\n", dev_unit_idx);

    p->charPos = 0;
    p->stati   = 04000;
    if (! unitp->fileref)
      p->stati = 04240; // device offline
    return 0;
  }

// source/library_dir_dir/system_library_1/source/bound_rcp_.s.archive/rcp_disk_.pl1
//
//  dcl set_standby_command    bit (6) internal static init ("72"b3);
//  dcl request_status_command bit (6) internal static init ("00"b3);
//  dcl read_command           bit (6) internal static init ("25"b3);
//  dcl reset_status_command   bit (6) internal static init ("40"b3);

iom_cmd_rc_t dsk_iom_cmd (uint iomUnitIdx, uint chan) {
#ifdef POLTS_DISK_TESTING
nCmds ++;
#endif
  iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan];
  uint ctlr_unit_idx  = get_ctlr_idx (iomUnitIdx, chan);
  uint devUnitIdx;

#ifdef POLTS_DISK_TESTING
if (chan == 014)   {if_sim_debug (DBG_TRACE, & dsk_dev) { dumpDCW (p->DCW, 0); }}
#endif
  if (cables->iom_to_ctlr[iomUnitIdx][chan].ctlr_type == CTLR_T_IPC)
    devUnitIdx = cables->ipc_to_dsk[ctlr_unit_idx][p->IDCW_DEV_CODE].unit_idx;
  else if (cables->iom_to_ctlr[iomUnitIdx][chan].ctlr_type == CTLR_T_MSP)
    devUnitIdx = cables->msp_to_dsk[ctlr_unit_idx][p->IDCW_DEV_CODE].unit_idx;
  else {
    sim_warn ("%s: Can't find controller (%d)\n", __func__, cables->iom_to_ctlr[iomUnitIdx][chan].ctlr_type);
    return IOM_CMD_ERROR;
  }

  UNIT * unitp = & dsk_unit[devUnitIdx];
  struct dsk_state * disk_statep = & dsk_states[devUnitIdx];

  iom_cmd_rc_t rc = IOM_CMD_PROCEED;

#ifdef LOCKLESS
  lock_ptr (& dsk_states->dsk_lock); //-V619
#endif

  // IDCW?
  if (p->DCW_18_20_CP == 7) {
    // IDCW
    disk_statep->io_mode = disk_no_mode;
    switch (p->IDCW_DEV_CMD) {
      case 000: // CMD 00 Request status
#ifdef POLTS_DISK_TESTING
//if (chan == 014)
#endif
#ifndef POLTS_DISK_TESTING
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk Request Status\r\n");
        }
#endif
        //disk_statep->io_mode = disk_no_mode;
        p->stati = 04000;
        if (! unitp->fileref)
          p->stati = 04240; // device offline
        break;

      case 016: // CMD 16 Read and Clear Statistics -- Model 800
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk Read And Clear Statistics\r\n");
        }
        disk_statep->io_mode = disk_rd_clr_stats;
        p->recordResidue --;
        p->stati             = 04000;
        if (! unitp->fileref)
          p->stati = 04240; // device offline
        break;

      case 022: // CMD 22 Read Status Register
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk Read Status Register\r\n");
        }
        disk_statep->io_mode = disk_rd_status_reg;
        p->recordResidue --;
        p->stati             = 04000;
        if (! unitp->fileref)
          p->stati = 04240; // device offline
        break;

      case 024: // CMD 24 Read configuration -- Model 800
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk Read Configuration\r\n");
        }
        disk_statep->io_mode = disk_rd_config;
        p->recordResidue --;
        p->stati             = 04000;
        if (! unitp->fileref)
          p->stati = 04240; // device offline
        break;

      case 025: // CMD 25 READ
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk Read\r\n");
        }
        disk_statep->io_mode = disk_rd;
        p->recordResidue --;
        p->stati             = 04000;
        if (! unitp->fileref)
          p->stati = 04240; // device offline
        break;

      case 026: // CMD 26 READ CONTROL REGISTER
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk Read Control Register\r\n");
        }
        disk_statep->io_mode = disk_rd_ctrl_reg;
        p->recordResidue --;
        p->stati             = 04000;
        if (! unitp->fileref)
          p->stati = 04240; // device offline
        break;

      case 030: // CMD 30 SEEK_512
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk Seek 512\r\n");
        }
        disk_statep->io_mode = disk_seek_512;
        p->recordResidue --;
        p->stati             = 04000;
        if (! unitp->fileref)
          p->stati = 04240; // device offline
        break;

      case 031: // CMD 31 WRITE
      case 033: // CMD 31 WRITE AND COMPARE
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk Write\r\n");
        }
        disk_statep->io_mode = disk_wr;
        p->recordResidue --;
        p->stati             = 04000;
        if (! unitp->fileref)
          p->stati = 04240; // device offline
        break;

      case 034: // CMD 34 SEEK_64
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk Seek 64\r\n");
        }
        disk_statep->io_mode = disk_seek_64;
        p->recordResidue --;
        p->stati             = 04000;
        if (! unitp->fileref)
          p->stati = 04240; // device offline
        break;

      case 036: // CMD 36 SPECIAL SEEK (T&D) // Make it work like SEEK_64 and
                                             // hope for the best
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk Special Seek\r\n");
        }
        disk_statep->io_mode = disk_special_seek;
        p->recordResidue --;
        p->stati             = 04000;
        if (! unitp->fileref)
          p->stati = 04240; // device offline
        break;

      case 040: // CMD 40 Reset status
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk Reset Status\r\n");
        }
        p->stati    = 04000;
        // XXX POLTS wants this; I don't understand why.
        p->recordResidue --;
        p->initiate = false; // According to POLTS
        p->isRead   = false;
        // T&D probing
        if (p->IDCW_DEV_CODE == 077) {
          p->stati = 04502; // invalid device code
#ifdef LOCKLESS
          unlock_ptr (& dsk_states->dsk_lock); //-V619
#endif
          return IOM_CMD_DISCONNECT;
        }
        if (! unitp->fileref)
          p->stati = 04240; // device offline
        break;

      case 042: // CMD 42 RESTORE
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk Restore\r\n");
        }
        p->stati    = 04000;
        // XXX POLTS wants this; I don't understand why.
        p->recordResidue --;
        p->initiate = false; // According to POLTS
        if (! unitp->fileref)
          p->stati = 04240; // device offline

        disk_statep->seekValid = false;

        break;

      case 072: // CMD 72 SET STANDBY
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk Set Standby\r\n");
        }
        p->stati = 04000;
        if (! unitp->fileref)
          p->stati = 04240; // device offline
        break;

      default:
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk unknown command %o\r\n", p->IDCW_DEV_CMD);
        }
        p->stati      = 04501;
        p->chanStatus = chanStatIncorrectDCW;
        if (p->IDCW_DEV_CMD != 051) // ignore bootload console probe
          sim_warn ("%s: Unrecognized device command %02o\n", __func__, p->IDCW_DEV_CMD);
        rc =  IOM_CMD_ERROR;
        goto done;
    }
    goto done;
  } // IDCW

  // Not IDCW; TDCW are captured in IOM, so must be IOTD, IOTP or IOTNP
  switch (disk_statep->io_mode) {

    case disk_no_mode:
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
      if_sim_debug (DBG_TRACE, & dsk_dev) {
        sim_printf ("// Disk IOT No Mode\r\n");
      }
// It appears that in some cases Multics builds a dcw list from a generic
// "single IDCW plus optional DDCW" template. That template sets the IDCW
// channel command to "record", regardless of whether or not the
// instruction needs an DDCW. In particular, disk_ctl pings the disk by
// sending a "Reset status" with "record" and an apparently uninitialized
// IOTD. The payload channel will send the IOTD because of the "record";
// the Reset Status command left IO mode at "no mode" so we don't know what
// to do with it. Since this appears benign, we will assume that the
// original H/W ignored, and so shall we.

      //sim_warn ("%s: Unexpected IOTx\n", __func__);
      //rc = IOM_CMD_ERROR;
      goto done;

    case disk_rd_clr_stats: {
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk IOT Rd And Clr Stats\r\n");
        }
        int rc1 = read_and_clear_statistics (devUnitIdx, iomUnitIdx, chan);
        if (rc1) {
          rc = IOM_CMD_ERROR;
          goto done;
        }
      }
      break;

    case disk_rd_status_reg: {
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk IOT Rd Status Reg\r\n");
        }
        int rc1 = readStatusRegister (devUnitIdx, iomUnitIdx, chan);
        if (rc1) {
          rc = IOM_CMD_ERROR;
          goto done;
        }
      }
      break;

    case disk_rd_config: {
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk IOT Rd Config\r\n");
        }
        // XXX missing. see poll_mpc.pl1, poll_mpc_data.incl.pl1
        int rc1 = read_configuration (devUnitIdx, iomUnitIdx, chan);
        if (rc1) {
          rc = IOM_CMD_ERROR;
          goto done;
        }
      }
      break;

    case disk_rd: {
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk IOT Read\r\n");
        }
        int rc1 = diskRead (devUnitIdx, iomUnitIdx, chan);
        if (rc1) {
          rc = IOM_CMD_ERROR;
          goto done;
        }
      }
      break;

    case disk_rd_ctrl_reg: {
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk IOT Read Control Register\r\n");
        }
        int rc1 = diskRdCtrlReg (devUnitIdx, iomUnitIdx, chan);
        if (rc1) {
          rc = IOM_CMD_ERROR;
          goto done;
        }
      }
      break;

    case disk_seek_512: {
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk IOT Seek 512\r\n");
        }
        int rc1 = diskSeek512 (devUnitIdx, iomUnitIdx, chan);
        if (rc1) {
          rc = IOM_CMD_ERROR;
          goto done;
        }
      }
      break;

    case disk_wr: {
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk IOT Write\r\n");
        }
        int rc1 = diskWrite (devUnitIdx, iomUnitIdx, chan);
        if (rc1) {
          rc = IOM_CMD_ERROR;
          goto done;
        }
      }
      break;

    case disk_seek_64: {
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk IOT Seek 64\r\n");
        }
        int rc1 = diskSeek64 (devUnitIdx, iomUnitIdx, chan);
        if (rc1) {
          rc = IOM_CMD_ERROR;
          goto done;
        }
      }
      break;

    case disk_special_seek: { // Make it work like SEEK_64 and hope for the best
#ifdef POLTS_DISK_TESTING
if (chan == 014)
#endif
        if_sim_debug (DBG_TRACE, & dsk_dev) {
          sim_printf ("// Disk IOT special seek\r\n");
        }
        iom_cmd_rc_t rc1 = diskSeekSpecial (devUnitIdx, iomUnitIdx, chan);
        if (rc1) {
          rc = IOM_CMD_ERROR;
          goto done;
        }
      }
      break;
  }

done:
#ifdef LOCKLESS
  unlock_ptr (& dsk_states->dsk_lock); //-V619
#endif
  return rc;
}

//////////
//////////
//////////
///
/// IPC
///

#define IPC_UNIT_IDX(uptr) ((uptr) - ipc_unit)
#define N_IPC_UNITS 1 // default

static struct ipc_state
  {
    char device_name[MAX_DEV_NAME_LEN];
  } ipc_states[N_IPC_UNITS_MAX];

UNIT ipc_unit[N_IPC_UNITS_MAX] = {
#ifdef NO_C_ELLIPSIS
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL }
#else
  [0 ... N_IPC_UNITS_MAX-1] = {
    UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL
  }
#endif
};

static t_stat ipc_show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr,
                               UNUSED int val, UNUSED const void * desc)
  {
    sim_printf("Number of IPC units in system is %d\n", ipc_dev.numunits);
    return SCPE_OK;
  }

static t_stat ipc_set_nunits (UNUSED UNIT * uptr, UNUSED int32 value,
                              const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    int n = atoi (cptr);
    if (n < 0 || n > N_DSK_UNITS_MAX)
      return SCPE_ARG;
    ipc_dev.numunits = (uint32) n;
    return SCPE_OK;
  }

static t_stat ipc_show_device_name (UNUSED FILE * st, UNIT * uptr,
                                    UNUSED int val, UNUSED const void * desc)
  {
    int n = (int) IPC_UNIT_IDX (uptr);
    if (n < 0 || n >= N_IPC_UNITS_MAX)
      return SCPE_ARG;
    if (ipc_states[n].device_name[0] != 0)
      sim_printf("name     : %s", ipc_states[n].device_name);
    else
      sim_printf("name     : default");
    return SCPE_OK;
  }

static t_stat ipc_set_device_name (UNIT * uptr, UNUSED int32 value,
                                   const char * cptr, UNUSED void * desc)
  {
    int n = (int) IPC_UNIT_IDX (uptr);
    if (n < 0 || n >= N_IPC_UNITS_MAX)
      return SCPE_ARG;
    if (cptr)
      {
        strncpy (ipc_states[n].device_name, cptr, MAX_DEV_NAME_LEN-1);
        ipc_states[n].device_name[MAX_DEV_NAME_LEN-1] = 0;
      }
    else
      ipc_states[n].device_name[0] = 0;
    return SCPE_OK;
  }

static MTAB ipc_mod[] =
  {
    {
      MTAB_dev_value,                            /* Mask               */
      0,                                         /* Match              */
      "NUNITS",                                  /* Print string       */
      "NUNITS",                                  /* Match string       */
      ipc_set_nunits,                            /* Validation routine */
      ipc_show_nunits,                           /* Display routine    */
      "Number of DISK units in the system",      /* Value descriptor   */
      NULL                                       /* Help               */
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_VALR | MTAB_NC, /* Mask               */
      0,                                         /* Match              */
      "NAME",                                    /* Print string       */
      "NAME",                                    /* Match string       */
      ipc_set_device_name,                       /* Validation routine */
      ipc_show_device_name,                      /* Display routine    */
      "Set the device name",                     /* Value descriptor   */
      NULL                                       /* Help               */
    },
    MTAB_eol
  };

DEVICE ipc_dev =
   {
    "IPC",                 /* Name                */
    ipc_unit,              /* Units               */
    NULL,                  /* Registers           */
    ipc_mod,               /* Modifiers           */
    N_IPC_UNITS,           /* #Units              */
    10,                    /* Address radix       */
    24,                    /* Address width       */
    1,                     /* Address increment   */
    8,                     /* Data radix          */
    36,                    /* Data width          */
    NULL,                  /* Examine             */
    NULL,                  /* Deposit             */
    NULL,                  /* Reset               */
    NULL,                  /* Boot                */
    NULL,                  /* Attach              */
    NULL /*disk_detach*/,  /* Detach              */
    NULL,                  /* Context             */
    0,                     /* Flags               */
    0,                     /* Debug control flags */
    NULL,                  /* Debug flag names    */
    NULL,                  /* Memory size change  */
    NULL,                  /* Logical name        */
    NULL,                  /* Help                */
    NULL,                  /* Attach help         */
    NULL,                  /* Attach context      */
    NULL,                  /* Description         */
    NULL                   /* End                 */
  };

//////////
//////////
//////////
///
/// MSP
///

#define MSP_UNIT_IDX(uptr) ((uptr) - msp_unit)
#define N_MSP_UNITS 1 // default

struct msp_state_s msp_states [N_MSP_UNITS_MAX];

UNIT msp_unit[N_MSP_UNITS_MAX] =
  {
#ifdef NO_C_ELLIPSIS
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL }
#else
  [0 ... N_MSP_UNITS_MAX-1] = {
    UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL
  }
#endif
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
    if (msp_states[n].device_name[0] != 0)
      sim_printf("name     : %s", msp_states[n].device_name);
    else
      sim_printf("name     : default");
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

static MTAB msp_mod[] =
  {
    {
      MTAB_dev_value,                        /* Mask               */
      0,                                     /* Match              */
      "NUNITS",                              /* Print string       */
      "NUNITS",                              /* Match string       */
      msp_set_nunits,                        /* Validation routine */
      msp_show_nunits,                       /* Display routine    */
      "Number of DISK units in the system",  /* Value descriptor   */
      NULL                                   /* Help               */
    },
    {
      MTAB_XTD | MTAB_VUN | \
      MTAB_VALR | MTAB_NC,                   /* Mask               */
      0,                                     /* Match              */
      "NAME",                                /* Print string       */
      "NAME",                                /* Match string       */
      msp_set_device_name,                   /* Validation routine */
      msp_show_device_name,                  /* Display routine    */
      "Set the device name",                 /* Value descriptor   */
      NULL                                   /* Help               */
    },
    MTAB_eol
  };

DEVICE msp_dev =
   {
    "MSP",                 /* Name                */
    msp_unit,              /* Units               */
    NULL,                  /* Registers           */
    msp_mod,               /* Modifiers           */
    N_MSP_UNITS,           /* #Units              */
    10,                    /* Address radix       */
    24,                    /* Address width       */
    1,                     /* Address increment   */
    8,                     /* Data radix          */
    36,                    /* Data width          */
    NULL,                  /* Examine             */
    NULL,                  /* Deposit             */
    NULL,                  /* Reset               */
    NULL,                  /* Boot                */
    NULL,                  /* Attach              */
    NULL /*disk_detach*/,  /* Detach              */
    NULL,                  /* Context             */
    0,                     /* Flags               */
    0,                     /* Debug control flags */
    NULL,                  /* Debug flag names    */
    NULL,                  /* Memory size change  */
    NULL,                  /* Logical name        */
    NULL,                  /* Help                */
    NULL,                  /* Attach help         */
    NULL,                  /* Attach context      */
    NULL,                  /* Description         */
    NULL                   /* End                 */
  };
