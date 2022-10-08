/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * SPDX-License-Identifier: Multics
 * scspell-id: cb56e6b9-f62e-11ec-8a20-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2020-2021 Dean Anderson
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
#include <ctype.h>
#include <unistd.h>

#include "dps8.h"
#include "dps8_iom.h"
#include "dps8_prt.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_utils.h"
#include "utfile.h"

#define DBG_CTR 1

//-- // XXX We use this where we assume there is only one unit
//-- #define ASSUME0 0
//--

// printer_types.incl.pl1
//
// dcl  models (13) fixed bin /* table of printer model numbers */
//     (202, 300, 301, 302, 303, 304, 401, 402, 901, 1000, 1200, 1201, 1600);
//
// dcl  types (13) fixed bin /* table of corresponding printer types */
//    (  1,   2,   2,   2,   3,   3,   4,   4,   4,    4,    4,    4,    4);
//
// dcl  WRITE (4) bit (6) /* printer write edited commands */
//     ("011000"b, "011000"b, "011100"b, "011100"b);
//
// dcl  WRITE_NE_SLEW (4) bit (6) /* printer write non-edited commands */
//     ("001001"b, "001001"b, "001101"b, "001101"b);
//
// dcl  LOAD_IMAGE (4) bit (6) /* printer load image buffer commands */
//     ("000000"b, "001100"b, "000001"b, "000001"b);
//
// dcl  LOAD_VFC (4) bit (6) /* printer load VFC image commands */
//     ("000000"b, "000000"b, "000000"b, "000101"b);
//
// dcl  READ_STATUS (4) bit (6) /* printer read detailed status command */
//     ("000000"b, "000000"b, "000000"b, "000011"b);

// AN87 only defines commands for
//   PRT203/303, PRU1200/1600
// and
//   PRT202/300

#define N_PRT_UNITS 1 // default

static t_stat prt_reset (DEVICE * dptr);
static t_stat prt_show_nunits (FILE *st, UNIT *uptr, int val, const void *desc);
static t_stat prt_set_nunits (UNIT * uptr, int32 value, const char * cptr, void * desc);
static t_stat prt_show_device_name (FILE *st, UNIT *uptr, int val, const void *desc);
static t_stat prt_set_device_name (UNIT * uptr, int32 value, const char * cptr, void * desc);
static t_stat prt_set_config (UNUSED UNIT *  uptr, UNUSED int32 value,
                              const char * cptr, UNUSED void * desc);
static t_stat prt_show_config (UNUSED FILE * st, UNUSED UNIT * uptr,
                               UNUSED int  val, UNUSED const void * desc);
static t_stat prt_show_path (UNUSED FILE * st, UNIT * uptr,
                                       UNUSED int val, UNUSED const void * desc);
static t_stat prt_set_path (UNUSED UNIT * uptr, UNUSED int32 value,
                                    const UNUSED char * cptr, UNUSED void * desc);
static t_stat prt_set_ready (UNIT * uptr, UNUSED int32 value,
                             UNUSED const char * cptr,
                             UNUSED void * desc);

static t_stat prt_show_device_model (FILE *st, UNIT *uptr, int val, const void *desc);
static t_stat prt_set_device_model (UNIT * uptr, int32 value, const char * cptr, void * desc);

#define UNIT_FLAGS ( UNIT_FIX | UNIT_ATTABLE | UNIT_ROABLE | UNIT_DISABLE | \
                     UNIT_IDLE )
UNIT prt_unit[N_PRT_UNITS_MAX] =
  {
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL}
  };

#define PRT_UNIT_NUM(uptr) ((uptr) - prt_unit)

static DEBTAB prt_dt[] =
  {
    { "NOTIFY", DBG_NOTIFY, NULL },
    { "INFO",   DBG_INFO,   NULL },
    { "ERR",    DBG_ERR,    NULL },
    { "WARN",   DBG_WARN,   NULL },
    { "DEBUG",  DBG_DEBUG,  NULL },
    { "ALL",    DBG_ALL,    NULL }, /* don't move as it messes up DBG message */
    { NULL,     0,          NULL }
  };

#define UNIT_WATCH UNIT_V_UF

static MTAB prt_mod[] =
  {
#ifndef SPEED
    { UNIT_WATCH, 1, "WATCH",   "WATCH",   0, 0, NULL, NULL },
    { UNIT_WATCH, 0, "NOWATCH", "NOWATCH", 0, 0, NULL, NULL },
#endif
    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO | MTAB_VALR, /* mask               */
      0,                                          /* match              */
      "NUNITS",                                   /* print string       */
      "NUNITS",                                   /* match string       */
      prt_set_nunits,                             /* validation routine */
      prt_show_nunits,                            /* display routine    */
      "Number of PRT units in the system",        /* value descriptor   */
      NULL                                        /* help               */
    },
    {
      MTAB_XTD | MTAB_VDV  | MTAB_NMO | \
                 MTAB_VALR | MTAB_NC,             /* mask               */
      0,                                          /* match              */
      "PATH",                                     /* print string       */
      "PATH",                                     /* match string       */
      prt_set_path,                               /* validation routine */
      prt_show_path,                              /* display routine    */
      "Path to write PRT files",                  /* value descriptor   */
      NULL                                        /* help               */
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_VALR | MTAB_NC,  /* mask               */
      0,                                          /* match              */
      "NAME",                                     /* print string       */
      "NAME",                                     /* match string       */
      prt_set_device_name,                        /* validation routine */
      prt_show_device_name,                       /* display routine    */
      "Select the printer name",                  /* value descriptor   */
      NULL                                        /* help               */
    },

    {
      MTAB_XTD | MTAB_VUN | MTAB_VALR | MTAB_NC,  /* mask               */
      0,                                          /* match              */
      "MODEL",                                    /* print string       */
      "MODEL",                                    /* match string       */
      prt_set_device_model,                       /* validation routine */
      prt_show_device_model,                      /* display routine    */
      "Select the printer model",                 /* value descriptor   */
      NULL                                        /* help               */
    },
    {
      MTAB_XTD | MTAB_VUN,                        /* mask               */
      0,                                          /* match              */
      (char *) "CONFIG",                          /* print string       */
      (char *) "CONFIG",                          /* match string       */
      prt_set_config,                             /* validation routine */
      prt_show_config,                            /* display routine    */
      NULL,                                       /* value descriptor   */
      NULL,                                       /* help               */
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_NMO | MTAB_VALR, /* mask               */
      0,                                          /* match              */
      "READY",                                    /* print string       */
      "READY",                                    /* match string       */
      prt_set_ready,                              /* validation routine */
      NULL,                                       /* display routine    */
      NULL,                                       /* value descriptor   */
      NULL                                        /* help               */
    },
    { 0, 0, NULL, NULL, 0, 0, NULL, NULL }
  };

DEVICE prt_dev = {
    "PRT",        /* name                */
    prt_unit,     /* unit                */
    NULL,         /* registers           */
    prt_mod,      /* modifiers           */
    N_PRT_UNITS,  /* number of units     */
    10,           /* address radix       */
    24,           /* address width       */
    1,            /* address increment   */
    8,            /* data radix          */
    36,           /* data width          */
    NULL,         /* examine             */
    NULL,         /* deposit             */
    prt_reset,    /* reset               */
    NULL,         /* boot                */
    NULL,         /* attach              */
    NULL,         /* detach              */
    NULL,         /* context             */
    DEV_DEBUG,    /* flags               */
    0,            /* debug control flags */
    prt_dt,       /* debug flag names    */
    NULL,         /* memory size change  */
    NULL,         /* logical name        */
    NULL,         /* help                */
    NULL,         /* attach help         */
    NULL,         /* attach context      */
    NULL,         /* description         */
    NULL          /* end                 */
};

typedef struct
  {
    enum prt_mode
      {
         prtNoMode, prtPrt, prtLdImgBuf, prtRdStatReg, prtLdVFCImg
      }  ioMode;
    int  prtUnitNum;
    bool isBCD;
    bool isEdited;
    int  slew;
    char device_name[MAX_DEV_NAME_LEN];
    int  prtfile; // fd
    //bool last;
    bool cachedFF;
    bool split;
    int  model;
  } prt_state_t;

static prt_state_t prt_state[N_PRT_UNITS_MAX];

static char prt_path[1025];

#define N_MODELS 13

static const char * model_names[N_MODELS] =
  {
    "202", "300", "301",  "302",  "303",  "304",
    "401", "402", "901", "1000", "1200", "1201",
    "1600"
  };

#define MODEL_1600 12

static const int model_type[N_MODELS] =
  { 1, 2, 2, 2, 3, 3,
    4, 4, 4, 4, 4, 4,
    4
  };

#ifdef NO_C_ELLIPSIS
static const uint8 newlines[128] = {
  '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
  '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
  '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
  '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
  '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
  '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
  '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
  '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
  '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
  '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
  '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
  '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
  '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
  '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
  '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
  '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n'
};

static const uint8 spaces[128] = {
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
};
#else
static const uint8 newlines[128] = { [0 ... 127] = '\n' };
static const uint8   spaces[128] = { [0 ... 127] = ' ' };
#endif
static const uint8 formfeed[1] = { '\f' };
static const uint8 crlf[4]     = { '\r', '\n', '\r', '\n' };
static const uint8 cr[2]       = { '\r', '\n' };

/*
 * prt_init()
 *
 */

// Once-only initialization

void prt_init (void)
  {
    memset (prt_path, 0, sizeof (prt_path));
    memset (prt_state, 0, sizeof (prt_state));
    for (int i = 0; i < N_PRT_UNITS_MAX; i ++)
      {
        prt_state[i].prtfile = -1;
        prt_state[i].model = MODEL_1600;
      }
  }

static t_stat prt_reset (UNUSED DEVICE * dptr)
  {
#if 0
    for (uint i = 0; i < dptr -> numunits; i ++)
      {
        // sim_prt_reset (& prt_unit[i]);
        // sim_cancel (& prt_unit[i]);
      }
#endif
    return SCPE_OK;
  }

// Given an array of word36 and a 9bit char offset, return the char

static word9 gc (word36 * b, uint os)
  {
    uint wordno = os / 4;
    uint charno = os % 4;
    return (word9) getbits36_9 (b[wordno], charno * 9);
  }

// Don't know what the longest user id is...
#define LONGEST 128

// looking for space/space/5 digit number/\037/\005/name/\037
// qno will get 5 chars + null;

//  040040061060 060060062037 005101156164 150157156171 056123171163 101144155151 156056141037 145061060060 060062013002
// <  10002\037\005Anthony.SysAdmin.a\037e10002\013\002>
//  01234567   8   9
static int parseID (word36 * b, uint tally, char * qno, char * name)
  {
    if (tally < 3)
      return 0;
    if (gc (b, 0) != 040)
      return 0;
    if (gc (b, 1) != 040)
      return 0;
    uint i;
    for (i = 0; i < 5; i ++)
      {
        word9 ch = gc (b, 2 + i);
        if (ch < '0' || ch > '9')
          return 0;
        qno[i] = (char) ch;
      }
    qno[5] = 0;
    if (gc (b, 7) != 037)
      return 0;
    //if (gc (b, 8) != 005)
      //return 0;
    for (i = 0; i < LONGEST; i ++)
      {
        word9 ch = gc (b, 9 + i);
        if (ch == 037)
          break;
        if (! isprint (ch))
          return 0;
        name[i] = (char) ch;
      }
    name[i] = 0;
    return -1;
  }

// 0 ok
// -1 unable to open print file
// -2 unable to write to print file
// -3 form feed cached, no i/o done.

static int openPrtFile (int prt_unit_num, word36 * buffer, uint tally)
  {
    if (prt_state[prt_unit_num].prtfile != -1)
      return 0;

// The first (spooled) write is a formfeed; special case it and delay opening
//  until the next line

    if (tally == 1 && buffer[0] == 0014013000000llu) //-V536
      {
        prt_state[prt_unit_num].cachedFF = true;
        return -3;
      }

    char qno[6], name[LONGEST + 1];
    int rc = parseID (buffer, tally, qno, name);
    char template[1024 + 129 + LONGEST];
    char unit_designator = 'a' + (char) prt_unit_num;
    char split_prefix[6];
    split_prefix[0] = 0;
    if (prt_state [prt_unit_num] . split) {
      sprintf(split_prefix, "prt%c/", unit_designator);
    }
    if (rc == 0)
      sprintf (template, "%s%sprt%c.spool.XXXXXX.prt", prt_path, split_prefix, unit_designator);
    else
      sprintf (template, "%s%sprt%c.spool.%s.%s.XXXXXX.prt", prt_path, split_prefix, unit_designator, qno, name);

    prt_state[prt_unit_num].prtfile = utfile_mkstemps (template, 4);
    if (prt_state[prt_unit_num].prtfile == -1)
      {
        sim_warn ("Unable to open printer file '%s', errno %d\n", template, errno);
        return -1;
      }
    if (prt_state[prt_unit_num].cachedFF)
      {
        ssize_t n_write = write (prt_state[prt_unit_num].prtfile, formfeed, 1);
        if (n_write != 1)
          {
            return -2;
          }
        prt_state[prt_unit_num].cachedFF = false;
      }
    return 0;
  }

// looking for lines "\037\014%%%%%\037\005"
static int eoj (word36 * buffer, uint tally)
  {
    if (tally < 3)
      return 0;
    if (getbits36_9 (buffer[0], 0) != 037)
      return 0;
    if (getbits36_9 (buffer[0], 9) != 014)
      return 0;
    word9 ch = getbits36_9 (buffer[0], 18);
    if (ch < '0' || ch > '9')
      return 0;
    ch = getbits36_9 (buffer[0], 27);
    if (ch < '0' || ch > '9')
      return 0;
    ch = getbits36_9 (buffer[1], 0);
    if (ch < '0' || ch > '9')
      return 0;
    ch = getbits36_9 (buffer[1], 9);
    if (ch < '0' || ch > '9')
      return 0;
    ch = getbits36_9 (buffer[1], 18);
    if (ch < '0' || ch > '9')
      return 0;
    if (getbits36_9 (buffer[1], 27) != 037)
      return 0;
    if (getbits36_9 (buffer[2], 0) != 005)
      return 0;
    return -1;
  }

// Based on prt_status_table_.alm
//  I think that "substat_entry a,b,c,d"
//    a  character number (6 bit chars; 6/word))
//    b  bit pattern
// so
//   substat_entry       1,000000,,(Normal)
// means "a 0 in char 1" means normal.
//

#if 0
static int prt_read_status_register (uint dev_unit_idx, uint iom_unit_idx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data[iom_unit_idx][chan];
    //UNIT * unitp = & prt_unit[dev_unit_idx];

// Process DDCW

    bool ptro;
    bool send;
    bool uff;
    int rc = iom_list_service (iom_unit_idx, chan, & ptro, & send, & uff);
    if (rc < 0)
      {
        sim_printf ("%s readStatusRegister list service failed\n", __func__);
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
    if (p -> DCW_18_20_CP == 07 || p -> DDCW_22_23_TYPE == 2)
      {
        sim_printf ("%s expected DDCW\n", __func__);
        return -1;
      }

    uint tally = p -> DDCW_TALLY;

    if (tally != 4)
      {
        sim_debug (DBG_ERR, &prt_dev,
                   "%s: expected tally of 4, is %d\n",
                   __func__, tally);
      }
    if (tally == 0)
      {
        sim_debug (DBG_DEBUG, & prt_dev,
                   "%s: Tally of zero interpreted as 010000(4096)\n",
                   __func__);
        tally = 4096;
      }

// XXX need status register data format
// system_library_tools/source/bound_io_tools_.s.archive/analyze_detail_stat_.pl1  anal_fips_disk_().

# ifdef TESTING
    sim_warn ("Need status register data format; tally %d\n", tally);
# endif
# if 1
    word36 buffer[tally];
    memset (buffer, 0, sizeof (buffer));
    // word 1 char 1   0: normal
    // word 1 char 2   0: device not busy
    // word 1 char 3   0: no device attention bit set
    // word 1 char 4   0: no device data alert
    // word 1 char 5   0: unused
    // word 1 char 6   0: no command reject
    //buffer[0] = 0;
    // word 2 char 1 (7) 0: unused
    // word 2 char 2 (9) 0: unused
    // word 2 char 3 (10) 0: unused
    // word 2 char 4 (11) 0: no MPC attention
    // word 2 char 5 (12) 0: no MPC data alert
    // word 2 char 6 (13) 0: unused
    //buffer[2] = 0;
    // word 3 char 1 (14) 0: no MPC command reject
    // word 3 char 2 (15) 0: unused
    // word 3 char 3 (16) 0: unused
    // word 3 char 4 (17) 0: unused
    // word 3 char 5 (18) 0: unused
    // word 3 char 6 (19) 0: unused
    //buffer[3] = 0;
    uint wordsProcessed = tally;
    iom_indirect_data_service (iom_unit_idx, chan, buffer,
                            & wordsProcessed, true);
    p -> initiate = false;
# else
    for (uint i = 0; i < tally; i ++)
      //M[daddr + i] = 0;
      core_write (daddr + i, 0, "Disk status register");

    //M[daddr] = SIGN36;
    core_write (daddr, SIGN36, "Disk status register");
# endif
    p -> charPos = 0;
    p -> stati = 04000;
    return 0;
  }
#endif

// 0 OK
// -1 Can't open print file
// -2 Can't write to print file

static int print_buf (int prt_unit_num, bool isBCD, bool is_edited, int slew, word36 * buffer, uint tally)
  {
// derived from pr2_conv_$lower_case_table
//
//        0    1    2    3    4    5    6    7
// 000   '0'  '1'  '2'  '3'  '4'  '5'  '6'  '7'
// 010   '8'  '9'  '{'  '#'  '?'  ':'  '>'
// 020   ' '  'a'  'b'  'c'  'd'  'e'  'f'  'g'
// 030   'h'  'i'  '|'  '.'  '}'  '('  '<'  '`'
// 040   '^'  'j'  'k'  'l'  'm'  'm'  'o'  'p'
// 050   'q'  'r'  '_'  '$'  '*'  ')'  ';'  '\''
// 060   '+'  '/'  's'  't'  'u'  'v'  'w'  'x'
// 070   'y'  'z'  '~'  ','  '!'  '='  '"'

    static char * bcd_lc =
      "01234567"
      "89{#?;>?"
      " abcdefg"
      "hi|.}(<\\"
      "^jklmnop"
      "qr_$*);'"
      "+/stuvwx"
      "yz~,!=\"!"; // '!' is actually the escape character, caught above

// derived from pr2_conv_$upper_case_table
//
//        0    1    2    3    4    5    6    7
// 000   '0'  '1'  '2'  '3'  '4'  '5'  '6'  '7'
// 010   '8'  '9'  '['  '#'  '@'  ':'  '>'
// 020   ' '  'A'  'B'  'C'  'D'  'E'  'F'  'G'
// 030   'H'  'I'  '&'  '.'  ']'  '('  '<'  '`'
// 040   '^'  'J'  'K'  'L'  'M'  'N'  'O'  'P'
// 050   'Q'  'R'  '-'  '$'  '*'  ')'  ';'  '\''
// 060   '+'  '/'  'S'  'T'  'U'  'V'  'W'  'X'
// 070   'Y'  'Z'  '\\' ','  '%'  '='  '"'
    static char * bcd_uc =
      "01234567"
      "89[#@;>?"  // '?' is actually in the lower case table; pr2_conv_ never generates 017 in upper case mode
      " ABCDEFG"
      "HI&.](<\\"
      "^JKLMNOP"
      "QR-$*);'"
      "+/STUVWX"
      "YZ_,%=\"!"; // '!' is actually the escape character, caught above

// Used for nonedited; has question mark.
    static char * bcd =
      "01234567"
      "89[#@;> "  // 'POLTS says that 17 is question mark for nonedited
      " ABCDEFG"
      "HI&.](<\\"
      "^JKLMNOP"
      "QR-$*);'"
      "+/STUVWX"
      "YZ_,%=\"!";

    if (prt_state[prt_unit_num].prtfile == -1)
      {
        int rc = openPrtFile (prt_unit_num, buffer, tally);
        if (rc < 0) // Can't open or can't write to print file; or ff cached
          {
            return rc == -3 ? 0 : rc;
          }
      }

#if 0
sim_printf ("%s %s %d %u\n", isBCD ? "BCD" : "ASCII", is_edited ? "edited" : "nonedited", slew, tally);
for (uint i = 0; i < tally; i ++)
  {
    sim_printf ("%012llo \"", buffer[i]);
    for (uint j = 0; j < 4; j ++)
      {
        uint8_t ch = (uint8_t) ((buffer[i] >> ((3 - j) * 9)) & 0177);
        sim_printf ("%c", isprint (ch) ? ch : '?');
      }
    sim_printf ("\" '");
    for (uint j = 0; j < 6; j ++)
      {
        static char * bcd =
          "01234567"
          "89[#@;>?"
          " ABCDEFG"
          "HI&.](<\\"
          "^JKLMNOP"
          "QR-$*);'"
          "+/STUVWX"
          "YZ_,%=\"!";
        uint8_t ch = (uint8_t) bcd[(buffer[i] >> ((5 - j) * 6)) & 0077];
        sim_printf ("%c", isprint (ch) ? ch : '?');
      }
    sim_printf ("'\n");
   }
#endif

    if (slew == -1)
      {
        ssize_t n_write = write (prt_state[prt_unit_num].prtfile, formfeed, 1);
        if (n_write != 1)
          {
            return -2;
          }
      }
    else if (slew)
      {
        for (int i = 0; i < slew; i ++)
          {
            ssize_t n_write = write (prt_state[prt_unit_num].prtfile, crlf, 2);
            if (n_write != 2)
              {
                return -2;
              }
          }
      }
// Not needed; always slew back to column 0 when done.
//    else
//      {
//        write (prt_state[prt_unit_num].prtfile, cr, 1);
//      }

    if (tally)
      {
            if (isBCD)
          {
            uint nchars = tally * 6;
#define get_BCD_char(i) ((uint8_t) ((buffer[i / 6] >> ((5 - i % 6) * 6)) & 077))

            if (! is_edited)
              { // Easy case
                uint8 bytes[nchars];
                for (uint i = 0; i < nchars; i ++)
                  {
                    bytes[i] = (uint8_t) bcd_uc [get_BCD_char (i)];
                  }
                ssize_t n_write = write (prt_state[prt_unit_num].prtfile, bytes, nchars);
                if (n_write != nchars)
                  {
                    return -2;
                  }
              }
            else // edited BCD
              {
                //bool BCD_case = false; // false is upper case
                // POLTS implies 3 sets
                // 0 - initial set, upper case, no question mark.
                // 1  - first change: lower case, question mark.
                // 2  - second change: upper case, question mark.
                int BCD_cset = 0;
                char * table[3] = { bcd, bcd_lc, bcd_uc };

                for (uint i = 0; i < nchars; i ++)
                  {
                    uint8_t ch = get_BCD_char (i);
// Looking at pr2_conv_.alm, it looks like the esc char is 77
//  77 n  if n is
//      0 - 017, slew n lines  (0 is just CR)
//      020 generate slew to top of page,
//      021 generate slew to top of inside page,
//      022 generate slew to top of outside page,
//      041 to 057, generate (n-040) *8 spaces

                    if (ch == 077)
                      {
                        i ++;
                        uint8_t n = get_BCD_char (i);

                        if (n == 077) // pr2_conv_ sez ESC ESC is case shift
                          {
                            //BCD_case = ! BCD_case;
                            switch (BCD_cset)
                              {
                                case 0: BCD_cset = 1; break; // default to lower
                                case 1: BCD_cset = 2; break; // lower to upper
                                case 2: BCD_cset = 1; break; // upper to lower
                              }
                          }
                        else if (n >= 041 && n <= 057)
                          {
                            int nchars = (n - 040) * 8;
                            ssize_t n_write = write (prt_state[prt_unit_num].prtfile, spaces, (size_t)nchars);
                            if (n_write != nchars)
                              {
                                return -2;
                              }
                          }
                        else if (n >= 020 && n <= 022)
                          {
                            // XXX not distinguishing between top of page, inside page, outside page
                            ssize_t n_write = write (prt_state[prt_unit_num].prtfile, formfeed, 1);
                            if (n_write != 1)
                              {
                                return -2;
                              }
                          }
                        else if (n == 0) // slew 0 lines is just CR
                          {
                            ssize_t n_write = write (prt_state[prt_unit_num].prtfile, cr, 1);
                            if (n_write != 1)
                              {
                                return -2;
                              }
                          }
                        else if (n <= 017)
                          {
                            ssize_t n_write = write (prt_state[prt_unit_num].prtfile, newlines, n);
                            if (n_write != n)
                              {
                                return -2;
                              }
                          }
#ifdef TESTING
                        else
                          {
                            sim_warn ("Printer BCD edited ESC %u. %o ignored\n", n, n);
                          }
#endif
                      }
                    else // not escape
                      {
                        ssize_t n_write = write (prt_state[prt_unit_num].prtfile, table[BCD_cset] + ch, 1);
                        if (n_write != 1)
                          {
                            return -2;
                          }
                      }
                  } // for i to nchars
              } // edited BCD
          } // BCD
        else // ASCII
          {
            uint nchars = tally * 4;
#define get_ASCII_char(i) ((uint8_t) ((buffer[i / 4] >> ((3 - i % 4) * 9)) & 0377))

            if (! is_edited)
              { // Easy case
                uint8 bytes[nchars];
                uint nbytes = 0;
                for (uint i = 0; i < nchars; i ++)
                  {
                    uint8_t ch = get_ASCII_char (i);
                    if (isprint (ch))
                      bytes[nbytes ++] = ch;
                  }
                ssize_t n_write = write (prt_state[prt_unit_num].prtfile, bytes, nbytes);
                if (n_write != nbytes)
                  {
                    return -2;
                  }
              }
            else // edited ASCII
              {
                uint col = 0;
                for (uint i = 0; i < tally * 4; i ++)
                  {
                    uint8_t ch = get_ASCII_char (i);
                    if (ch == 037) // insert n spaces
                      {
                        i ++;
                        uint8_t n = get_ASCII_char (i);
                        ssize_t n_write = write (prt_state[prt_unit_num].prtfile, spaces, n);
                        if (n_write != n)
                          {
                            return -2;
                          }
                        col += n;
                      }
                    else if (ch == 013) // insert n new lines
                      {
                        i ++;
                        uint8_t n = get_ASCII_char (i);
                        if (n)
                           {
                            ssize_t n_write = write (prt_state[prt_unit_num].prtfile, newlines, n);
                            if (n_write != n)
                              {
                                return -2;
                              }
                          }
                        else // 0 lines; just slew to beginning of line
                          {
                            ssize_t n_write = write (prt_state[prt_unit_num].prtfile, cr, 1);
                            if (n_write != 1)
                              {
                                return -2;
                              }
                          }
                        col = 0;
                      }
                    else if (ch == 014) // slew page
                      {
                        ssize_t n_write = write (prt_state[prt_unit_num].prtfile, formfeed, 1);
                        if (n_write != 1)
                          {
                            return -2;
                          }
                        col = 0;
                      }
                    else if (ch == 011) // horizontal tab
                      {
                        i ++;
                        uint8_t n = get_ASCII_char (i);
                        if (col < n)
                          {
                            ssize_t n_write = write (prt_state[prt_unit_num].prtfile, spaces, n - col);
                            if (n_write != n - col)
                              {
                                return -2;
                              }
                            col += n;
                          }
                      }
                    else if (isprint (ch))
                      {
                        ssize_t n_write = write (prt_state[prt_unit_num].prtfile, & ch, 1);
                        if (n_write != 1)
                          {
                            return -2;
                          }
                        col ++;
                      }
                  } // for
              } // edited ASCII
          } // ASCII
      } // tally

// Slew back to beginning of line
    ssize_t n_write = write (prt_state[prt_unit_num].prtfile, cr, 1);
    if (n_write != 1)
      {
        return -2;
      }

    if ((! isBCD) && eoj (buffer, tally))
      {
        close (prt_state[prt_unit_num].prtfile);
        prt_state[prt_unit_num].prtfile = -1;
      }
    return 0;
  }

static int loadImageBuffer (uint iom_unit_idx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data[iom_unit_idx][chan];
    // We don't actually have a print chain, so just pretend we loaded the image data
    p->stati = 04000; //-V536
    return 0;
  }

static int readStatusRegister (uint iom_unit_idx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data[iom_unit_idx][chan];

    uint tally = p -> DDCW_TALLY;

    if (tally != 4)
      {
        sim_warn ("%s: expected tally of 4, is %d\n", __func__, tally);
      }
    if (tally == 0)
      {
        tally = 4096;
      }

// system_library_tools/source/bound_io_tools_.s.archive/analyze_detail_stat_.pl1  anal_fips_disk_().

    word36 buffer[tally];
    memset (buffer, 0, sizeof (buffer));
    // word 1 char 1   0: normal
    // word 1 char 2   0: device not busy
    // word 1 char 3   0: no device attention bit set
    // word 1 char 4   0: no device data alert
    // word 1 char 5   0: unused
    // word 1 char 6   0: no command reject
    //buffer[0] = 0;
    // word 2 char 1 (7) 0: unused
    // word 2 char 2 (9) 0: unused
    // word 2 char 3 (10) 0: unused
    // word 2 char 4 (11) 0: no MPC attention
    // word 2 char 5 (12) 0: no MPC data alert
    // word 2 char 6 (13) 0: unused
    //buffer[2] = 0;
    // word 3 char 1 (14) 0: no MPC command reject
    // word 3 char 2 (15) 0: unused
    // word 3 char 3 (16) 0: unused
    // word 3 char 4 (17) 0: unused
    // word 3 char 5 (18) 0: unused
    // word 3 char 6 (19) 0: unused
    //buffer[3] = 0;
    uint wordsProcessed = tally;
    iom_indirect_data_service (iom_unit_idx, chan, buffer,
                            & wordsProcessed, true);
    p->charPos = 0;
    p->stati = 04000; //-V536
    return 0;
  }

static int loadVFCImage (uint iom_unit_idx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data[iom_unit_idx][chan];
    // We don't actually have a VFC, so just pretend we loaded the image data
    p->stati = 04000; //-V536
    return 0;
  }

static iom_cmd_rc_t print_cmd (uint iom_unit_idx, uint chan, int prt_unit_num, bool isBCD, bool is_edited, int slew)
  {
    iom_chan_data_t * p = & iom_chan_data[iom_unit_idx][chan];
    p->isRead = false;

#if 0
// The EURC MPC printer controller sets the number of DCWs in the IDCW and
// ignores the IOTD bits in the DDCWs.

    uint ddcwCnt = p->IDCW_COUNT;
    // Process DDCWs

    bool ptro, send, uff;
    for (uint ddcwIdx = 0; ddcwIdx < ddcwCnt; ddcwIdx ++)
      {
        int rc2 = iom_list_service (iom_unit_idx, chan, & ptro, & send, & uff);
        if (rc2 < 0)
          {
            p -> stati = 05001; // BUG: arbitrary error code; config switch
            sim_printf ("%s list service failed\n", __func__);
            return IOM_CMD_ERROR;
          }
        if (uff)
          {
            sim_printf ("%s ignoring uff\n", __func__); // XXX
          }
        if (! send)
          {
            sim_printf ("%s nothing to send\n", __func__);
            p -> stati = 05001; // BUG: arbitrary error code; config switch
            return IOM_CMD_ERROR;
          }
        if (IS_IDCW (p) || IS_TDCW (p))
          {
            sim_printf ("%s expected DDCW\n", __func__);
            p -> stati = 05001; // BUG: arbitrary error code; config switch
            return IOM_CMD_ERROR;
          }
#endif
        uint tally = p -> DDCW_TALLY;
        sim_debug (DBG_DEBUG, & prt_dev,
                   "%s: Tally %d (%o)\n", __func__, tally, tally);

        if (tally == 0)
          tally = 4096;

        // Copy from core to buffer
        word36 buffer[tally];
        uint wordsProcessed = 0;
        iom_indirect_data_service (iom_unit_idx, chan, buffer,
                                & wordsProcessed, false);
        p -> initiate = false;

#if 0
for (uint i = 0; i < tally; i ++)
   sim_printf (" %012"PRIo64"", buffer[i]);
sim_printf ("\r\n");
#endif

#if 0
      sim_printf ("<");
        for (uint i = 0; i < tally * 4; i ++) {
        uint wordno = i / 4;
        uint charno = i % 4;
        uint ch = (buffer[wordno] >> ((3 - charno) * 9)) & 0777;
        if (isprint (ch))
          sim_printf ("%c", ch);
        else
          sim_printf ("\\%03o", ch);
      }
        sim_printf (">\n");
#endif
        int rc = print_buf (prt_unit_num, isBCD, is_edited, slew, buffer, tally);
        if (rc == -1) // Can't open print file
          {
            p->stati = 04201; // Out of paper
            return IOM_CMD_ERROR;
          }
        if (rc == -2) // Can't write to print file
          {
            p->stati = 04210; // Check alert
            return IOM_CMD_ERROR;
          }

#if 0
        if (eoj (buffer, tally))
          {
            close (prt_state[prt_unit_num].prtfile);
            prt_state[prt_unit_num].prtfile = -1;
          }
#endif
#if 0
    } // for (ddcwIdx)
#endif
    p -> tallyResidue = 0;
    p -> stati = 04000;
    //return IOM_CMD_PROCEED;
    return IOM_CMD_RESIDUE;
  }

iom_cmd_rc_t prt_cmd_202 (uint iomUnitIdx, uint chan) {
  iom_chan_data_t * p  = & iom_chan_data[iomUnitIdx][chan];
  uint ctlr_unit_idx   = get_ctlr_idx (iomUnitIdx, chan);
  uint devUnitIdx      = cables->urp_to_urd[ctlr_unit_idx][p->IDCW_DEV_CODE].unit_idx;
  UNIT * unitp         = & prt_unit[devUnitIdx];
  int prt_unit_num     = (int) PRT_UNIT_NUM (unitp);
  prt_state_t * statep = & prt_state[devUnitIdx];

  // IDCW?
  if (IS_IDCW (p)) {
    // IDCW
    statep->ioMode = prtNoMode;

    switch (p->IDCW_DEV_CMD) {
      case 000: // CMD 00 Request status
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Request Status\n", __func__);
        p->stati = 04000;
        break;

      case 010: // CMD 010 -- print nonedited BCD, slew one line
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Nonedited BCD, Slew One Line\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = true;
        statep->isEdited   = false;
        statep->slew       = 1;
        p->stati           = 04000;
        break;

      case 030: // CMD 030 -- print edited BCD, slew zero lines
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Edited BCD, Slew Zero Lines\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = true;
        statep->isEdited   = true;
        statep->slew       = 0;
        p->stati           = 04000;
        break;

      case 040: // CMD 40 Reset status
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Reset Status\n", __func__);
        p->stati  = 04000;
        p->isRead = false;
        break;

      default:
        p->stati      = 04501; // cmd reject, invalid opcode
        p->chanStatus = chanStatIncorrectDCW;
        if (p->IDCW_DEV_CMD != 051) // ignore bootload console probe
          sim_warn ("%s: PRT unrecognized device command %02o\n", __func__, p->IDCW_DEV_CMD);
        return IOM_CMD_ERROR;
    } // switch IDCW_DEV_CMND

    sim_debug (DBG_DEBUG, & prt_dev, "%s: stati %04o\n", __func__, p->stati);
    return IOM_CMD_PROCEED;
  } // if IDCW

  // Not IDCW; TDCW are captured in IOM, so must be IOTD, IOTP or IOTNP
  switch (statep->ioMode) {
    case prtNoMode:
      //sim_printf ("%s: Unexpected IOTx\n", __func__);
      //sim_warn ("%s: Unexpected IOTx\n", __func__);
      //return IOM_CMD_ERROR;
      break;

    case prtPrt: {
        iom_cmd_rc_t rc = print_cmd (iomUnitIdx, chan, statep->prtUnitNum, statep->isBCD,
                statep->isEdited, statep->slew);
        if (rc) //-V547
          return rc;
      }
      break;

    default:
      sim_warn ("%s: Unrecognized ioMode %d\n", __func__, statep->ioMode);
      return IOM_CMD_ERROR;
  }

  return IOM_CMD_PROCEED;
}

iom_cmd_rc_t prt_cmd_300 (uint iomUnitIdx, uint chan) {
  iom_chan_data_t * p  = & iom_chan_data[iomUnitIdx][chan];
  uint ctlr_unit_idx   = get_ctlr_idx (iomUnitIdx, chan);
  uint devUnitIdx      = cables->urp_to_urd[ctlr_unit_idx][p->IDCW_DEV_CODE].unit_idx;
  UNIT * unitp         = & prt_unit[devUnitIdx];
  int prt_unit_num     = (int) PRT_UNIT_NUM (unitp);
  prt_state_t * statep = & prt_state[devUnitIdx];

  // IDCW?
  if (IS_IDCW (p)) {
    // IDCW
    statep->ioMode = prtNoMode;

    switch (p->IDCW_DEV_CMD) {

      case 000: // CMD 00 Request status
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Request Status\n", __func__);
        p->stati = 04000;
        break;

      case 011: // CMD 011 -- print nonedited ASCII, slew one line
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Nonedited ASCII, Slew One Line\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = false;
        statep->isEdited   = false;
        statep->slew       = 1;
        p->stati           = 04000;
        break;

      case 014: // CMD 014 -- Load Image Buffer
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Load Image Buffer\n", __func__);
        statep->ioMode = prtLdImgBuf;
        p->stati       = 04000;
        break;

      case 030: // CMD 030 -- print edited ASCII, slew zero lines
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Edited ASCII, Slew Zero Lines\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = false;
        statep->isEdited   = true;
        statep->slew       = 0;
        p->stati           = 04000;
        break;

      case 040: // CMD 40 Reset status
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Reset Status\n", __func__);
        p->stati  = 04000;
        p->isRead = false;
        break;

      default:
        p->stati      = 04501; // cmd reject, invalid opcode
        p->chanStatus = chanStatIncorrectDCW;
        if (p->IDCW_DEV_CMD != 051) // ignore bootload console probe
          sim_warn ("%s: PRT unrecognized device command %02o\n", __func__, p->IDCW_DEV_CMD);
        return IOM_CMD_ERROR;
    } // switch IDCW_DEV_CMND

    sim_debug (DBG_DEBUG, & prt_dev, "%s: stati %04o\n", __func__, p->stati);
    return IOM_CMD_PROCEED;
  } // if IDCW

  // Not IDCW; TDCW are captured in IOM, so must be IOTD, IOTP or IOTNP
  switch (statep->ioMode) {
    case prtNoMode:
      //sim_printf ("%s: Unexpected IOTx\n", __func__);
      //sim_warn ("%s: Unexpected IOTx\n", __func__);
      //return IOM_CMD_ERROR;
      break;

    case prtPrt: {
        iom_cmd_rc_t rc = print_cmd (iomUnitIdx, chan, statep->prtUnitNum, statep->isBCD,
                statep->isEdited, statep->slew);
        if (rc) //-V547
          return rc;
      }
      break;

    case prtLdImgBuf: {
        int rc = loadImageBuffer (iomUnitIdx, chan);
        if (rc) //-V547
          return IOM_CMD_ERROR;
      }
      break;

    default:
      sim_warn ("%s: Unrecognized ioMode %d\n", __func__, statep->ioMode);
      return IOM_CMD_ERROR;
  }
  return IOM_CMD_PROCEED;
}

iom_cmd_rc_t prt_cmd_300a (uint iomUnitIdx, uint chan) {
  iom_chan_data_t * p  = & iom_chan_data[iomUnitIdx][chan];
  uint ctlr_unit_idx   = get_ctlr_idx (iomUnitIdx, chan);
  uint devUnitIdx      = cables->urp_to_urd[ctlr_unit_idx][p->IDCW_DEV_CODE].unit_idx;
  UNIT * unitp         = & prt_unit[devUnitIdx];
  int prt_unit_num     = (int) PRT_UNIT_NUM (unitp);
  prt_state_t * statep = & prt_state[devUnitIdx];

  // IDCW?
  if (IS_IDCW (p)) {
    // IDCW
    statep->ioMode = prtNoMode;

    switch (p->IDCW_DEV_CMD) {
      case 000: // CMD 00 Request status
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Request Status\n", __func__);
        p->stati = 04000;
        break;

      case 001: // CMD 001 -- Load Image Buffer
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Load Image Buffer\n", __func__);
        statep->ioMode = prtLdImgBuf;
        p->stati       = 04000;
        break;

      case 015: // CMD 015 -- print nonedited ASCII, slew one line
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Nonedited ASCII, Slew One Line\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = false;
        statep->isEdited   = false;
        statep->slew       = 1;
        p->stati           = 04000;
        break;

      case 034: // CMD 034 -- print edited ASCII, slew zero lines
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Edited ASCII, Slew Zero Lines\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = false;
        statep->isEdited   = true;
        statep->slew       = 0;
        p->stati           = 04000;
        break;

      case 040: // CMD 40 Reset status
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Reset Status\n", __func__);
        p->stati  = 04000;
        p->isRead = false;
        break;

      default:
        p->stati      = 04501; // cmd reject, invalid opcode
        p->chanStatus = chanStatIncorrectDCW;
        if (p->IDCW_DEV_CMD != 051) // ignore bootload console probe
          sim_warn ("%s: PRT unrecognized device command %02o\n", __func__, p->IDCW_DEV_CMD);
        return IOM_CMD_ERROR;
    } // switch IDCW_DEV_CMND

    sim_debug (DBG_DEBUG, & prt_dev, "%s: stati %04o\n", __func__, p->stati);
    return IOM_CMD_PROCEED;
  } // if IDCW

  // Not IDCW; TDCW are captured in IOM, so must be IOTD, IOTP or IOTNP
  switch (statep->ioMode) {
    case prtNoMode:
      //sim_printf ("%s: Unexpected IOTx\n", __func__);
      //sim_warn ("%s: Unexpected IOTx\n", __func__);
      //return IOM_CMD_ERROR;
      break;

    case prtPrt: {
        iom_cmd_rc_t rc = print_cmd (iomUnitIdx, chan, statep->prtUnitNum, statep->isBCD,
                statep->isEdited, statep->slew);
        if (rc) //-V547
          return rc;
      }
      break;

    case prtLdImgBuf: {
        int rc = loadImageBuffer (iomUnitIdx, chan);
        if (rc) //-V547
          return IOM_CMD_ERROR;
      }
      break;

    default:
      sim_warn ("%s: Unrecognized ioMode %d\n", __func__, statep->ioMode);
      return IOM_CMD_ERROR;
  }
  return IOM_CMD_PROCEED;
}

iom_cmd_rc_t prt_cmd_400 (uint iomUnitIdx, uint chan) {
  iom_chan_data_t * p  = & iom_chan_data[iomUnitIdx][chan];
  uint ctlr_unit_idx   = get_ctlr_idx (iomUnitIdx, chan);
  uint devUnitIdx      = cables->urp_to_urd[ctlr_unit_idx][p->IDCW_DEV_CODE].unit_idx;
  UNIT * unitp         = & prt_unit[devUnitIdx];
  int prt_unit_num     = (int) PRT_UNIT_NUM (unitp);
  prt_state_t * statep = & prt_state[devUnitIdx];

  // IDCW?
  if (IS_IDCW (p)) {
    // IDCW
    statep->ioMode = prtNoMode;

    switch (p->IDCW_DEV_CMD) {

      case 000: // CMD 00 Request status
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Request Status\n", __func__);
        p->stati = 04000;
        break;

      case 001: // CMD 001 -- Load Image Buffer
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Load Image Buffer\n", __func__);
        statep->ioMode = prtLdImgBuf;
        p->stati       = 04000;
        break;

      case 003: // CMD 003 -- Read Status
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Load Image Buffer\n", __func__);
        statep->ioMode = prtRdStatReg;
        p->stati       = 04000;
        break;

      // load_vfc: entry (pip, pcip, iop, rcode);
      //
      // dcl 1 vfc_image aligned,                    /* print VFC image */
      //    (2 lpi fixed bin (8),                    /* lines per inch */
      //     2 image_length fixed bin (8),           /* number of lines represented by image */
      //     2 toip,                                 /* top of inside page info */
      //       3 line fixed bin (8),                 /* line number */
      //       3 pattern bit (9),                    /* VFC pattern */
      //     2 boip,                                 /* bottom of inside page info */
      //       3 line fixed bin (8),                 /* line number */
      //       3 pattern bit (9),                    /* VFC pattern */
      //     2 toop,                                 /* top of outside page info */
      //       3 line fixed bin (8),                 /* line number */
      //       3 pattern bit (9),                    /* VFC pattern */
      //     2 boop,                                 /* bottom of outside page info */
      //       3 line fixed bin (8),                 /* line number */
      //       3 pattern bit (9),                    /* VFC pattern */
      //     2 pad bit (18)) unal;                   /* fill out last word */
      //
      // dcl (toip_pattern init ("113"b3),           /* top of inside page pattern */
      //      toop_pattern init ("111"b3),           /* top of outside page pattern */
      //      bop_pattern init ("060"b3))            /* bottom of page pattern */
      //      bit (9) static options (constant);

      case 005: // CMD 005 -- Load VFC image
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Load VFC Image\n", __func__);
        statep->ioMode = prtLdVFCImg;
        p->stati       = 04000;
        break;

      case 010: // CMD 010 -- print nonedited BCD, slew zero lines
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Nonedited BCD, Slew Zero Lines\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = true;
        statep->isEdited   = false;
        statep->slew       = 0;
        p->stati           = 04000;
        break;

      case 011: // CMD 011 -- print nonedited BCD, slew one line
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Nonedited BCD, Slew One Line\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = true;
        statep->isEdited   = false;
        statep->slew       = 1;
        p->stati           = 04000;
        break;

      case 012: // CMD 012 -- print nonedited BCD, slew two lines
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Nonedited BCD, Slew Two Lines\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = true;
        statep->isEdited   = false;
        statep->slew       = 2;
        p->stati           = 04000;
        break;

      case 013: // CMD 013 -- print nonedited BCD, slew top of page
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Nonedited BCD, Slew Top Of Page\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = true;
        statep->isEdited   = false;
        statep->slew       = -1;
        p->stati           = 04000;
        break;

      case 014: // CMD 014 -- print nonedited ASCII, slew zero lines
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Nonedited ASCII, Slew Zero Lines\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = false;
        statep->isEdited   = false;
        statep->slew       = 0;
        p->stati           = 04000;
        break;

      case 015: // CMD 015 -- print nonedited ASCII, slew one line
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Nonedited ASCII, Slew One Line\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = false;
        statep->isEdited   = false;
        statep->slew       = 1;
        p->stati           = 04000;
        break;

      case 016: // CMD 016 -- print nonedited ASCII, slew two lines
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Nonedited ASCII, Slew Two Lines\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = false;
        statep->isEdited   = false;
        statep->slew       = 2;
        p->stati           = 04000;
        break;

      case 017: // CMD 017 -- print nonedited ASCII, slew top of page
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Nonedited ASCII, Slew Top Of Page\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = false;
        statep->isEdited   = false;
        statep->slew       = -1;
        p->stati           = 04000;
        break;

      case 030: // CMD 030 -- print edited BCD, slew zero lines
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Edited BCD, Slew Zero Lines\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = true;
        statep->isEdited   = true;
        statep->slew       = 0;
        p->stati           = 04000;
        break;

      case 031: // CMD 031 -- print edited BCD, slew one line
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Edited BCD, Slew One Line\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = true;
        statep->isEdited   = true;
        statep->slew       = 1;
        p->stati           = 04000;
        break;

      case 032: // CMD 032 -- print edited BCD, slew two lines
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Edited BCD, Slew Two Lines\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = true;
        statep->isEdited   = true;
        statep->slew       = 2;
        p->stati           = 04000;
        break;

      case 033: // CMD 033 -- print edited BCD, slew top of page
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Edited BCD, Slew Top Of Page\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = true;
        statep->isEdited   = true;
        statep->slew       = -1;
        p->stati           = 04000;
        break;

      case 034: // CMD 034 -- print edited ASCII, slew zero lines
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Edited ASCII, Slew Zero Lines\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = false;
        statep->isEdited   = true;
        statep->slew       = 0;
        p->stati           = 04000;
        break;

      case 035: // CMD 035 -- print edited ASCII, slew one line
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Edited ASCII, Slew One Line\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = false;
        statep->isEdited   = true;
        statep->slew       = 1;
        p->stati           = 04000;
        break;

      case 036: // CMD 036 -- print edited ASCII, slew two lines
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Edited ASCII, Slew Two Lines\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = false;
        statep->isEdited   = true;
        statep->slew       = 2;
        p->stati           = 04000;
        break;

      case 037: // CMD 037 -- print edited ASCII, slew top of page
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Print Edited ASCII, Slew Top Of Page\n", __func__);
        statep->ioMode     = prtPrt;
        statep->prtUnitNum = prt_unit_num;
        statep->isBCD      = false;
        statep->isEdited   = true;
        statep->slew       = -1;
        p->stati           = 04000;
        break;

      case 040: // CMD 40 Reset status
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Reset Status\n", __func__);
        p->stati  = 04000;
        p->isRead = false;
        break;

      case 061:  { // CMD 61 Slew one line
          sim_debug (DBG_DEBUG, & prt_dev, "%s: Slew One Line\n", __func__);
          int rc = print_buf (prt_unit_num, false, false, 1, NULL, 0);
          if (rc == -1) { // Can't open print file
            p->stati = 04201; // Out of paper
            return IOM_CMD_ERROR;
          }
          if (rc == -2) { // Can't write to print file
            p->stati = 04210; // Check alert
            return IOM_CMD_ERROR;
          }
          p->stati  = 04000;
          p->isRead = false;
        }
        break;

      case 062: { // CMD 62 Slew two lines
          sim_debug (DBG_DEBUG, & prt_dev, "%s: Slew Two Lines\n", __func__);
          int rc = print_buf (prt_unit_num, false, false, 2, NULL, 0);
          if (rc == -1) { // Can't open print file
            p->stati = 04201; // Out of paper
            return IOM_CMD_ERROR;
          }
          if (rc == -2) { // Can't write to print file
            p->stati = 04210; // Check alert
            return IOM_CMD_ERROR;
          }
          p->stati  = 04000;
          p->isRead = false;
        }
        break;

      case 063: { // CMD 63 Slew to top of page
          sim_debug (DBG_DEBUG, & prt_dev, "%s: Slew To Top Of Page\n", __func__);
          int rc = print_buf (prt_unit_num, false, false, -1, NULL, 0);
          if (rc == -1) { // Can't open print file
            p->stati = 04201; // Out of paper
            return IOM_CMD_ERROR;
          }
          if (rc == -2) { // Can't write to print file
            p->stati = 04210; // Check alert
            return IOM_CMD_ERROR;
          }
          p->stati  = 04000;
          p->isRead = false;
        }
        break;

      case 066: // CMD 66 Reserve device
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Reserve Device\n", __func__);
        p->stati  = 04000;
        p->isRead = false;
        break;

      case 067: // CMD 67 Release device
        sim_debug (DBG_DEBUG, & prt_dev, "%s: Release Device\n", __func__);
        p->stati  = 04000;
        p->isRead = false;
        break;

      default:
        p->stati      = 04501; // cmd reject, invalid opcode
        p->chanStatus = chanStatIncorrectDCW;
        if (p->IDCW_DEV_CMD != 051) // ignore bootload console probe
        sim_warn ("%s: PRT unrecognized device command %02o\n", __func__, p->IDCW_DEV_CMD);
        return IOM_CMD_ERROR;
    } // switch IDCW_DEV_CMND

    sim_debug (DBG_DEBUG, & prt_dev, "%s: stati %04o\n", __func__, p->stati);
    return IOM_CMD_PROCEED;
  } // if IDCW

  // Not IDCW; TDCW are captured in IOM, so must be IOTD, IOTP or IOTNP
  switch (statep->ioMode) {
    case prtNoMode:
      //sim_printf ("%s: Unexpected IOTx\n", __func__);
      //sim_warn ("%s: Unexpected IOTx\n", __func__);
      //return IOM_CMD_ERROR;
      break;

    case prtPrt: {
        iom_cmd_rc_t rc = print_cmd (iomUnitIdx, chan, statep->prtUnitNum, statep->isBCD,
                statep->isEdited, statep->slew);
        if (rc) //-V547
          return rc;
      }
      break;

    case prtLdImgBuf: {
        int rc = loadImageBuffer (iomUnitIdx, chan);
        if (rc) //-V547
          return IOM_CMD_ERROR;
      }
      break;

    case prtRdStatReg: {
        int rc = readStatusRegister (iomUnitIdx, chan);
        if (rc) //-V547
          return IOM_CMD_ERROR;
      }
      break;

    case prtLdVFCImg: {
        int rc = loadVFCImage (iomUnitIdx, chan);
        if (rc) //-V547
          return IOM_CMD_ERROR;
      }
      break;

    default:
      sim_warn ("%s: Unrecognized ioMode %d\n", __func__, statep->ioMode);
      return IOM_CMD_ERROR;
  }
  return IOM_CMD_PROCEED;
}

iom_cmd_rc_t prt_iom_cmd (uint iomUnitIdx, uint chan) {
  iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan];
  uint ctlr_unit_idx  = get_ctlr_idx (iomUnitIdx, chan);
  uint devUnitIdx     = cables->urp_to_urd[ctlr_unit_idx][p->IDCW_DEV_CODE].unit_idx;
  UNIT * unitp        = & prt_unit[devUnitIdx];
  int prt_unit_num    = (int) PRT_UNIT_NUM (unitp);

  switch (model_type [prt_state[prt_unit_num].model]) {
    case 1: // 202
      return prt_cmd_202 (iomUnitIdx, chan);

    case 2: // 300, 301, 302
      return prt_cmd_300 (iomUnitIdx, chan);

    case 3: // 303, 304
      return prt_cmd_300a (iomUnitIdx, chan);
      // switch type 3 cmd

    case 4: // 401, 402, 901, 1000, 1200, 1201, 1600
      return prt_cmd_400 (iomUnitIdx, chan);
  }
  p->stati = 04502; //-V536  // invalid device code
  return IOM_CMD_DISCONNECT;
}

static t_stat prt_show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr, UNUSED int val,
                               UNUSED const void * desc)
  {
    sim_printf("Number of PRT units in system is %d\n", prt_dev.numunits);
    return SCPE_OK;
  }

static t_stat prt_set_nunits (UNUSED UNIT * uptr, UNUSED int32 value, const char * cptr,
                              UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    int n = atoi (cptr);
    if (n < 1 || n > N_PRT_UNITS_MAX)
      return SCPE_ARG;
    prt_dev.numunits = (uint) n;
    return SCPE_OK;
  }

static t_stat prt_show_device_name (UNUSED FILE * st, UNIT * uptr,
                                    UNUSED int val, UNUSED const void * desc)
  {
    int n = (int) PRT_UNIT_NUM (uptr);
    if (n < 0 || n >= N_PRT_UNITS_MAX)
      return SCPE_ARG;
    sim_printf("name     : %s", prt_state[n].device_name);
    return SCPE_OK;
  }

static t_stat prt_set_device_model (UNUSED UNIT * uptr, UNUSED int32 value,
                                    const UNUSED char * cptr, UNUSED void * desc)
  {
    int n = (int) PRT_UNIT_NUM (uptr);
    if (n < 0 || n >= N_PRT_UNITS_MAX)
      return SCPE_ARG;
    if (cptr)
      {
        for (int i = 0; i < N_MODELS; i ++)
           {
             if (strcmp (cptr, model_names[i]) == 0)
               {
                 prt_state[n].model = i;
                 return SCPE_OK;
               }
            }
        sim_printf ("Model '%s' not known (202 300 301 302 303 304 401 402 901 1000 1200 1201 1600)\n", cptr);
        return SCPE_ARG;
      }
    sim_printf ("Specify model from 202 300 301 302 303 304 401 402 901 1000 1200 1201 1600\n");
    return SCPE_ARG;
  }

static t_stat prt_show_device_model (UNUSED FILE * st, UNIT * uptr,
                                     UNUSED int val, UNUSED const void * desc)
  {
    int n = (int) PRT_UNIT_NUM (uptr);
    if (n < 0 || n >= N_PRT_UNITS_MAX)
      return SCPE_ARG;
    sim_printf("model    : %s", model_names[prt_state[n].model]);
    return SCPE_OK;
  }

static t_stat prt_set_device_name (UNUSED UNIT * uptr, UNUSED int32 value,
                                   const UNUSED char * cptr, UNUSED void * desc)
  {
    int n = (int) PRT_UNIT_NUM (uptr);
    if (n < 0 || n >= N_PRT_UNITS_MAX)
      return SCPE_ARG;
    if (cptr)
      {
        strncpy (prt_state[n].device_name, cptr, MAX_DEV_NAME_LEN - 1);
        prt_state[n].device_name[MAX_DEV_NAME_LEN - 1] = 0;
      }
    else
      prt_state[n].device_name[0] = 0;
    return SCPE_OK;
  }

static t_stat prt_show_path (UNUSED FILE * st, UNUSED UNUSED UNIT * uptr,
                             UNUSED int val, UNUSED const void * desc)
  {
    sim_printf("Path to PRT files is %s\n", prt_path);
    return SCPE_OK;
  }

static t_stat prt_set_path (UNUSED UNIT * uptr, UNUSED int32 value,
                            const UNUSED char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;

    size_t len = strlen(cptr);

    if (len >= sizeof(prt_path))
      return SCPE_ARG;
    strncpy(prt_path, cptr, sizeof(prt_path));
    if (len > 0)
      {
        if (prt_path[len - 1] != '/')
          {
            if (len == sizeof(prt_path) - 1)
              return SCPE_ARG;
            prt_path[len++] = '/';
            prt_path[len] = 0;
          }
      }
    return SCPE_OK;
  }

t_stat burst_printer (UNUSED int32 arg, const char * buf)
  {
    for (int i = 0; i < N_PRT_UNITS_MAX; i ++)
      {
        if (strcmp (buf, prt_state[i].device_name) == 0)
          {
            if (prt_state[i].prtfile != -1)
              {
                close (prt_state[i].prtfile);
                prt_state[i].prtfile = -1;
                return SCPE_OK;
              }
            sim_printf ("burst sees nothing to burst\n");
            return SCPE_OK;
          }
      }
    sim_printf ("burst can't find printer named '%s'\n", buf);
    return SCPE_ARG;
  }

static t_stat signal_prt_ready (uint prt_unit_idx) {
  // Don't signal if the sim is not running....
  if (! sim_is_running)
    return SCPE_OK;
  uint ctlr_unit_idx = cables->prt_to_urp[prt_unit_idx].ctlr_unit_idx;
#if 0
    // Which port should the controller send the interrupt to? All of them...
    bool sent_one = false;
    for (uint ctlr_port_num = 0; ctlr_port_num < MAX_CTLR_PORTS; ctlr_port_num ++)
      {
        struct ctlr_to_iom_s * urp_to_iom = & cables->urp_to_iom[ctlr_unit_idx][ctlr_port_num];
        if (urp_to_iom->in_use)
          {
            uint iom_unit_idx = urp_to_iom->iom_unit_idx;
            uint chan_num = urp_to_iom->chan_num;
            uint dev_code = cables->prt_to_urp[prt_unit_idx].dev_code;

            send_special_interrupt (iom_unit_idx, chan_num, dev_code, 0x40, 01 /* disk pack ready */);
            sent_one = true;
          }
      }
    if (! sent_one)
      {
        sim_printf ("signal_prt_ready can't find controller; dropping interrupt\n");
        return SCPE_ARG;
      }
    return SCPE_OK;
#else
  // Which port should the controller send the interrupt to? All of them...
  for (uint ctlr_port_num = 0; ctlr_port_num < MAX_CTLR_PORTS; ctlr_port_num ++) {
    struct ctlr_to_iom_s * urp_to_iom = & cables->urp_to_iom[ctlr_unit_idx][ctlr_port_num];
    if (urp_to_iom->in_use) {
      uint iom_unit_idx = urp_to_iom->iom_unit_idx;
      uint chan_num     = urp_to_iom->chan_num;
      uint dev_code     = cables->prt_to_urp[prt_unit_idx].dev_code;

      send_special_interrupt (iom_unit_idx, chan_num, dev_code, 0x40, 01 /* disk pack ready */);
      return SCPE_OK;
    }
  }
  return SCPE_ARG;
#endif
}

static t_stat prt_set_ready (UNIT * uptr, UNUSED int32 value,
                             UNUSED const char * cptr,
                             UNUSED void * desc)
  {
    int n = (int) PRT_UNIT_NUM (uptr);
    if (n < 0 || n >= N_PRT_UNITS_MAX)
      {
        sim_debug (DBG_ERR, & prt_dev,
                   "Printer set ready: Invalid unit number %ld\n", (long) n);
        sim_printf ("error: Invalid unit number %ld\n", (long) n);
        return SCPE_ARG;
      }
    return signal_prt_ready ((uint) n);
  }

static config_value_list_t cfg_on_off[] =
  {
    { "off",     0 },
    { "on",      1 },
    { "disable", 0 },
    { "enable",  1 },
    { NULL,      0 }
  };

static config_list_t prt_config_list[] =
  {
   { "split", 0, 1, cfg_on_off },
   { NULL,    0, 0, NULL }
  };

static t_stat prt_set_config (UNUSED UNIT *  uptr, UNUSED int32 value,
                              const char * cptr, UNUSED void * desc)
  {
    int devUnitIdx    = (int) PRT_UNIT_NUM (uptr);
    prt_state_t * psp = prt_state + devUnitIdx;
    // XXX Minor bug; this code doesn't check for trailing garbage
    config_state_t cfg_state = { NULL, NULL };

    for (;;)
      {
        int64_t v;
        int rc = cfg_parse (__func__, cptr, prt_config_list,
                           & cfg_state, & v);
        if (rc == -1) // done
          break;

        if (rc == -2) // error
          {
            cfg_parse_done (& cfg_state);
            return SCPE_ARG;
          }
        const char * p = prt_config_list[rc].name;

        if (strcmp (p, "split") == 0)
          {
            psp->split = v != 0;
            continue;
          }

        sim_warn ("error: prt_set_config: Invalid cfg_parse rc <%ld>\n",
                  (long) rc);
        cfg_parse_done (& cfg_state);
        return SCPE_ARG;
      } // process statements
    cfg_parse_done (& cfg_state);
    return SCPE_OK;
  }

static t_stat prt_show_config (UNUSED FILE * st, UNUSED UNIT * uptr,
                               UNUSED int  val, UNUSED const void * desc)
  {
    int devUnitIdx    = (int) PRT_UNIT_NUM (uptr);
    prt_state_t * psp = prt_state + devUnitIdx;
    sim_msg ("split    : %d", psp->split);
    return SCPE_OK;
  }
