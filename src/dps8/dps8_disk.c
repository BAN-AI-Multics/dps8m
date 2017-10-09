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

//
//  dps8_disk.c
//  dps8
//
//  Created by Harry Reed on 6/16/13.
//  Copyright (c) 2013 Harry Reed. All rights reserved.
//

// source/library_dir_dir/system_library_1/source/bound_volume_rldr_ut_.s.archive/rdisk_.pl1

#include <stdio.h>

#include "dps8.h"
#include "dps8_iom.h"
#include "dps8_disk.h"
#include "dps8_sys.h"
#include "dps8_utils.h"
#include "dps8_scu.h"
#include "dps8_cpu.h"
#include "dps8_faults.h"
#include "dps8_cable.h"
#include "sim_disk.h"

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


// assuming 512 word36  sectors; and seekPosition is seek512
#define SECTOR_SZ_IN_W36 512
#define SECTOR_SZ_IN_BYTES ((36 * SECTOR_SZ_IN_W36) / 8)

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
static t_stat disk_reset (DEVICE * dptr);
static t_stat disk_show_nunits (FILE *st, UNIT *uptr, int val, const void *desc);
static t_stat disk_set_nunits (UNIT * uptr, int32 value, const char * cptr, void * desc);

#define UNIT_FLAGS ( UNIT_FIX | UNIT_ATTABLE | UNIT_ROABLE | UNIT_DISABLE | \
                     UNIT_IDLE | DKUF_F_RAW)
UNIT disk_unit [N_DISK_UNITS_MAX] =
  {
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (/* & disk_svc */ NULL, UNIT_FLAGS, M3381_SECTORS), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL}
  };

#define DISK_UNIT_NUM(uptr) ((uptr) - disk_unit)

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

#define UNIT_WATCH UNIT_V_UF

static MTAB disk_mod [] =
  {
    { UNIT_WATCH, 1, "WATCH", "WATCH", 0, 0, NULL, NULL },
    { UNIT_WATCH, 0, "NOWATCH", "NOWATCH", 0, 0, NULL, NULL },
    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO | MTAB_VALR, /* mask */
      0,            /* match */
      "NUNITS",     /* print string */
      "NUNITS",         /* match string */
      disk_set_nunits, /* validation routine */
      disk_show_nunits, /* display routine */
      "Number of DISK units in the system", /* value descriptor */
      NULL // Help
    },
    { 0, 0, NULL, NULL, 0, 0, NULL, NULL }
  };


// No disks known to multics had more than 2^24 sectors...
DEVICE disk_dev = {
    "DISK",       /*  name */
    disk_unit,    /* units */
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
    NULL,         /* attach */
    NULL,         /* detach */
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

static struct disk_state
  {
    enum { no_mode, seek512_mode, seek_mode, read_mode, write_mode, request_status_mode } io_mode;
    uint seekPosition;
  } disk_states [N_DISK_UNITS_MAX];

#if 0
static int findDiskUnit (int iomUnitIdx, int chan_num, int dev_code)
  {
    for (int i = 0; i < N_DISK_UNITS_MAX; i ++)
      {
        if (iomUnitIdx == cables -> cablesFromIomToDsk [i] . iomUnitIdx &&
            chan_num     == cables -> cablesFromIomToDsk [i] . chan_num     &&
            dev_code     == cables -> cablesFromIomToDsk [i] . dev_code)
          return i;
      }
    return -1;
  }
#endif

/*
 * disk_init()
 *
 */

// Once-only initialization

void disk_init (void)
  {
    memset (disk_states, 0, sizeof (disk_states));
  }

static t_stat disk_reset (UNUSED DEVICE * dptr)
  {
    //for (uint i = 0; i < dptr -> numunits; i ++)
      //{
        //// sim_disk_reset (& disk_unit [i]);
        //sim_cancel (& disk_unit [i]);
      //}
    return SCPE_OK;
  }

static int diskSeek512 (uint iomUnitIdx, uint chan)
  {
    iomChanData_t * p = & iomChanData [iomUnitIdx] [chan];
    struct device * d = & cables -> cablesFromIomToDev [iomUnitIdx] .
                      devices [chan] [p -> IDCW_DEV_CODE];
    uint devUnitIdx = d -> devUnitIdx;
    struct disk_state * disk_statep = & disk_states [devUnitIdx];
    sim_debug (DBG_NOTIFY, & disk_dev, "Seek512 %d\n", devUnitIdx);
//sim_printf ("disk seek512 [%"PRId64"]\n", sim_timell ());
    disk_statep -> io_mode = seek512_mode;

// Process DDCW

    bool ptro, send, uff;
    int rc = iomListService (iomUnitIdx, chan, & ptro, & send, & uff);
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
    sim_debug (DBG_DEBUG, & disk_dev,
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
    iomDirectDataService (iomUnitIdx, chan, & seekData, false);

//sim_printf ("seekData %012"PRIo64"\n", seekData);
// Observations about the seek/write stream
// the stream is seek512 followed by a write 1024.
// the seek data is:  000300nnnnnn
// lets assume the 3 is a copy of the seek cmd # as a data integrity check.
// highest observed n during vol. inoit. 272657(8) 95663(10)
//

// disk_control.pl1: 
//   quentry.sector = bit (sector, 21);  /* Save the disk device address. */
// suggests seeks are 21 bits.
//  
    disk_statep -> seekPosition = seekData & MASK21;
//sim_printf ("seek seekPosition %d\n", disk_statep -> seekPosition);
    p -> stati = 00000; // Channel ready
    return 0;
  }

static int diskRead (uint iomUnitIdx, uint chan)
  {
    iomChanData_t * p = & iomChanData [iomUnitIdx] [chan];
    struct device * d = & cables -> cablesFromIomToDev [iomUnitIdx] .
                      devices [chan] [p -> IDCW_DEV_CODE];
    uint devUnitIdx = d -> devUnitIdx;
    UNIT * unitp = & disk_unit [devUnitIdx];
    struct disk_state * disk_statep = & disk_states [devUnitIdx];

    sim_debug (DBG_NOTIFY, & disk_dev, "Read %d\n", devUnitIdx);
    disk_statep -> io_mode = read_mode;

// Process DDCWs

    bool ptro, send, uff;
    do
      {
        int rc = iomListService (iomUnitIdx, chan, & ptro, & send, & uff);
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
            sim_debug (DBG_DEBUG, & disk_dev,
                       "%s: Tally of zero interpreted as 010000(4096)\n",
                       __func__);
            tally = 4096;
          }

        sim_debug (DBG_DEBUG, & disk_dev,
                   "%s: Tally %d (%o)\n", __func__, tally, tally);

        rc = fseek (unitp -> fileref, 
                    (long) (disk_statep -> seekPosition * SECTOR_SZ_IN_BYTES),
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
   
        uint tallySectors = (tally + SECTOR_SZ_IN_W36 - 1) / 
                             SECTOR_SZ_IN_W36;
        uint tallyWords = tallySectors * SECTOR_SZ_IN_W36;
        //uint tallyBytes = tallySectors * SECTOR_SZ_IN_BYTES;
        uint p72ByteCnt = (tallyWords * 36) / 8;
        uint8 diskBuffer [p72ByteCnt];
        memset (diskBuffer, 0, sizeof (diskBuffer));
        sim_debug (DBG_TRACE, & disk_dev, "Disk read  %3d %8d %3d\n",
                   devUnitIdx, disk_statep -> seekPosition, tallySectors);

        rc = (int) fread (diskBuffer, SECTOR_SZ_IN_BYTES,
                    tallySectors,
                    unitp -> fileref);

        if (rc == 0) // eof; reading a sector beyond the high water mark.
          {
            // okay; buffer was zero, so just pretend that a zero filled
            // sector was read (ala demand page zero)
          }
        else if (rc != (int) tallySectors)
          {
            sim_printf ("read returned %d, errno %d\n", rc, errno);
            p -> stati = 04202; // attn, seek incomplete
            p -> chanStatus = chanStatIncorrectDCW;
            return -1;
          }
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
        iomIndirectDataService (iomUnitIdx, chan, buffer,
                                & wordsProcessed, true);
        p -> charPos = tally % 4;
      } while (p -> DDCW_22_23_TYPE != 0); // not IOTD
    p -> stati = 04000;
    p -> initiate = false;
    return 0;
  }

static int diskWrite (uint iomUnitIdx, uint chan)
  {
    iomChanData_t * p = & iomChanData [iomUnitIdx] [chan];
    struct device * d = & cables -> cablesFromIomToDev [iomUnitIdx] .
                      devices [chan] [p -> IDCW_DEV_CODE];
    uint devUnitIdx = d -> devUnitIdx;
    UNIT * unitp = & disk_unit [devUnitIdx];
    struct disk_state * disk_statep = & disk_states [devUnitIdx];

    sim_debug (DBG_NOTIFY, & disk_dev, "Write %d\n", devUnitIdx);
    disk_statep -> io_mode = read_mode;

// Process DDCWs

    bool ptro, send, uff;
    do
      {
        int rc = iomListService (iomUnitIdx, chan, & ptro, & send, & uff);
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
            sim_debug (DBG_DEBUG, & disk_dev,
                       "%s: Tally of zero interpreted as 010000(4096)\n",
                       __func__);
            tally = 4096;
          }

        sim_debug (DBG_DEBUG, & disk_dev,
                   "%s: Tally %d (%o)\n", __func__, tally, tally);

        rc = fseek (unitp -> fileref, 
                    (long) (disk_statep -> seekPosition * SECTOR_SZ_IN_BYTES),
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
   
        uint tallySectors = (tally + SECTOR_SZ_IN_W36 - 1) / 
                             SECTOR_SZ_IN_W36;
        uint tallyWords = tallySectors * SECTOR_SZ_IN_W36;
        //uint tallyBytes = tallySectors * SECTOR_SZ_IN_BYTES;
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
        iomIndirectDataService (iomUnitIdx, chan, buffer,
                                & wordsProcessed, false);
// XXX is this losing information?
        wordsProcessed = 0;
        for (uint i = 0; i < tally; i ++)
          {
            insertWord36toBuffer (diskBuffer, p72ByteCnt, & wordsProcessed,
                                  buffer [i]);
          }
#endif

        sim_debug (DBG_TRACE, & disk_dev, "Disk write %3d %8d %3d\n",
                   devUnitIdx, disk_statep -> seekPosition, tallySectors);
        rc = (int) fwrite (diskBuffer, SECTOR_SZ_IN_BYTES,
                     tallySectors,
                     unitp -> fileref);
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
 
      } while (p -> DDCW_22_23_TYPE != 0); // not IOTD
    p -> stati = 04000;
    p -> initiate = false;
    return 0;
  }

static int readStatusRegister (uint iomUnitIdx, uint chan)
  {
    iomChanData_t * p = & iomChanData [iomUnitIdx] [chan];
    struct device * d = & cables -> cablesFromIomToDev [iomUnitIdx] .
                      devices [chan] [p -> IDCW_DEV_CODE];
    uint devUnitIdx = d -> devUnitIdx;
    UNIT * unitp = & disk_unit [devUnitIdx];
    struct disk_state * disk_statep = & disk_states [devUnitIdx];

    sim_debug (DBG_NOTIFY, & disk_dev, "Read %d\n", devUnitIdx);
    disk_statep -> io_mode = read_mode;

// Process DDCW

    bool ptro;
    bool send;
    bool uff;
    int rc = iomListService (iomUnitIdx, chan, & ptro, & send, & uff);
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
        sim_debug (DBG_DEBUG, & disk_dev,
                   "%s: Tally of zero interpreted as 010000(4096)\n",
                   __func__);
        tally = 4096;
      }

// XXX need status register data format 
// system_library_tools/source/bound_io_tools_.s.archive/analyze_detail_stat_.pl1  anal_fips_disk_().

    sim_warn ("Need status register data format\n");
#if 1
    word36 buffer [tally];
    memset (buffer, 0, sizeof (buffer));
    buffer [0] = SIGN36;
    uint wordsProcessed = 0;
    iomIndirectDataService (iomUnitIdx, chan, buffer,
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

static int disk_cmd (uint iomUnitIdx, uint chan)
  {
    iomChanData_t * p = & iomChanData [iomUnitIdx] [chan];
    struct device * d = & cables -> cablesFromIomToDev [iomUnitIdx] .
                      devices [chan] [p -> IDCW_DEV_CODE];
    uint devUnitIdx = d -> devUnitIdx;
    UNIT * unitp = & disk_unit [devUnitIdx];
    struct disk_state * disk_statep = & disk_states [devUnitIdx];

    disk_statep -> io_mode = no_mode;
    p -> stati = 0;

    switch (p -> IDCW_DEV_CMD)
      {
        case 000: // CMD 00 Request status
          {
            p -> stati = 04000;
            if (! unitp -> fileref)
              p -> stati = 04240; // device offline

            disk_statep -> io_mode = no_mode;
            sim_debug (DBG_NOTIFY, & disk_dev, "Request status %d\n", devUnitIdx);
          }
          break;

        case 022: // CMD 22 Read Status Resgister
          {
            int rc = readStatusRegister (iomUnitIdx, chan);
            if (rc)
              return -1;
          }
          break;

        case 025: // CMD 25 READ
          {
            // XXX is it correct to not process the DDCWs?
            if (! unitp -> fileref)
              {
                p -> stati = 04240; // device offline
                break;
              }
            int rc = diskRead (iomUnitIdx, chan);
            if (rc)
              return -1;
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
            int rc = diskSeek512 (iomUnitIdx, chan);
            if (rc)
              return -1;
          }
          break;

        case 031: // CMD 31 WRITE
          {
            // XXX is it correct to not process the DDCWs?
            if (! unitp -> fileref)
              {
                p -> stati = 04240; // device offline
                break;
              }
            p -> isRead = false;
            int rc = diskWrite (iomUnitIdx, chan);
            if (rc)
              return -1;

            sim_debug (DBG_NOTIFY, & disk_dev, "Write %d\n", devUnitIdx);
            disk_statep -> io_mode = write_mode;
//sim_printf ("disk write [%"PRId64"]\n", sim_timell ());
            p -> stati = 04000;
          }
//exit(1);
          break;

#if 0
        case 034: // CMD 34 SEEK
          {
//sim_printf ("disk seek [%"PRId64"]\n", sim_timell ());
            sim_debug (DBG_NOTIFY, & disk_dev, "Seek %d\n", devUnitIdx);
            disk_statep -> io_mode = seek_mode;
          }
          break;
#endif

        case 040: // CMD 40 Reset status
          {
            p -> stati = 04000;
            if (! unitp -> fileref)
              p -> stati = 04240; // device offline
            disk_statep -> io_mode = no_mode;
            sim_debug (DBG_NOTIFY, & disk_dev, "Reset status %d\n", devUnitIdx);
          }
          break;

        case 042: // CMD 42 RESTORE
          {
            sim_debug (DBG_NOTIFY, & disk_dev, "Restore %d\n", devUnitIdx);
            p -> stati = 04000;
            if (! unitp -> fileref)
              p -> stati = 04240; // device offline
          }
          break;


        default:
          {
            p -> stati = 04501;
            sim_debug (DBG_ERR, & disk_dev,
                       "%s: Unknown command 0%o\n", __func__, p -> IDCW_DEV_CMD);
            p -> chanStatus = chanStatIncorrectDCW;
sim_printf ("%s: Unknown command 0%o\n", __func__, p -> IDCW_DEV_CMD);
            break;
          }
      }
    return 0;
  }

#ifdef IOM2
static int disk_ddcw (UNIT * unitp, dcw_t * ddcwp)
  {
    int devUnitIdx = DISK_UNIT_NUM (unitp);
    int iomUnitIdx = cables -> cablesFromIomToDsk [devUnitIdx] . iomUnitIdx;

    struct disk_state * disk_statep = & disk_states [devUnitIdx];
    iomChannelData_ * p = & iomChannelData [iomUnitIdx] [disk_statep -> chan];
    switch (disk_statep -> io_mode)
      {
        case seek_mode:
          {
            sim_printf ("disk seek not here yet\n");
            p -> stati = 04510; // Cmd reject, invalid inst. seq.
            p -> chanStatus = chanStatIncorrectDCW;
          }
          break;

      }
    return 0;
  }
#endif

int disk_iom_cmd (uint iomUnitIdx, uint chan)
  {
    iomChanData_t * p = & iomChanData [iomUnitIdx] [chan];
// Is it an IDCW?

    if (p -> DCW_18_20_CP == 7)
      {

        // Ignore a CMD 051 in the PCW
        if (p -> IDCW_DEV_CMD == 051)
          return 1;

        disk_cmd (iomUnitIdx, chan);
      }
    else // DDCW/TDCW
      {
        sim_printf ("disk_iom_cmd expected IDCW\n");
        return -1;
      }
    return 0;
#ifdef IOM2
            if (devUnitIdx < 0)
              {
// 04502 : COMMAND REJECTED, invalid device code
                iomChannelData_ * p = & iomChannelData [iomUnitIdx] [pcwp -> chan];
                p -> stati = 04502; 
                p -> dev_code = dcw . fields . instr. dev_code;
                p -> chanStatus = chanStatInvalidInstrPCW;
                //status_service (iomUnitIdx, pcwp -> chan, false);
                break;
              }
#endif
  }

static t_stat disk_show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr, UNUSED int val, UNUSED const void * desc)
  {
    sim_printf("Number of DISK units in system is %d\n", disk_dev . numunits);
    return SCPE_OK;
  }

static t_stat disk_set_nunits (UNUSED UNIT * uptr, UNUSED int32 value, const char * cptr, UNUSED void * desc)
  {
    int n = atoi (cptr);
    if (n < 1 || n > N_DISK_UNITS_MAX)
      return SCPE_ARG;
    disk_dev . numunits = (uint32) n;
    return SCPE_OK;
  }

void loadDisk (uint driveNumber, char * diskFilename)
  {
    //sim_printf ("in loadTape %d %s\n", driveNumber, tapeFilename);
    t_stat stat = attach_unit (& disk_unit [driveNumber], diskFilename);
    if (stat != SCPE_OK)
      {
        sim_printf ("loadDisk sim_disk_attach returned %d\n", stat);
        return;
      }

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
    //sim_printf ("%s %d %o\n", tapeFilename, ro,  mt_unit [driveNumber] . flags);
    //sim_printf ("special int %d %o\n", driveNumber, mt_unit [driveNumber] . flags);
    send_special_interrupt ((uint) cables -> cablesFromIomToDsk [driveNumber] . iomUnitIdx,
                            (uint) cables -> cablesFromIomToDsk [driveNumber] . chan_num,
                            (uint) cables -> cablesFromIomToDsk [driveNumber] . dev_code,
                            0x40, 01 /* disk pack ready */);
  }

t_stat attachDisk (char * label)
  {
    //sim_printf ("%s %s %s\n", label, withring ? "rw" : "ro", drive);
    uint i;
    for (i = 1; i < N_DISK_UNITS_MAX; i ++)
      {
sim_printf ("%d fileref %p filename %s\n", i, (void *) disk_unit [i] . fileref, disk_unit [i] . filename);
        if (disk_unit [i] . fileref == NULL)
          break;
      }
    if (i >= N_DISK_UNITS_MAX)
      {
        sim_printf ("can't find available disk drive\n");
        return SCPE_ARG;
      }
    loadDisk (i, label);
    return SCPE_OK;
  }

