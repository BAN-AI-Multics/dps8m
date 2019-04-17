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

//#define IOMDBG1
/**
 * \file dps8_mt.c
 * \project dps8
 * \date 9/17/12
 * \copyright Copyright (c) 2012 Harry Reed. All rights reserved.
*/

#include <stdio.h>
#include <ctype.h>

#include "dps8.h"
#include "dps8_mt.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_utils.h"

#define DBG_CTR 1

/*
 mt.c -- mag tape
 See manual AN87
 */
/*
 Copyright (c) 2007-2013 Michael Mondy
 
 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at http://example.org/project/LICENSE.
 */

/*
 
 COMMENTS ON "CHAN DATA" AND THE T&D TAPE (test and diagnostic tape)
 
 The IOM status service provides the "residue" from the last PCW or
 IDCW as part of the status.  Bootload_tape_label.alm indicates
 that after a read binary operation, the field is interpreted as the
 device number and that a device number of zero is legal.
 
 The IOM boot channel will store an IDCW with a chan-data field of zero.
 AN70, page 2-1 says that when the tape is read in native mode via an
 IOM or IOCC, the tape drive number in the IDCW will be zero.  It says
 that a non-zero tape drive number in the IDCW indicates that BOS is
 being used to simulate an IOM. (Presumaby written before BCE replaced
 BOS.)
 
 However...
 
 This seems to imply that an MPC could be connected to a single
 channel and support multiple tape drives by using chan-data as a
 device id.  If so, it seems unlikely that chan-data could ever
 represent anything else such as a count of a number of records to
 back space over (which is hinted at as an example in AN87).
 
 Using chan-data as a device-id that is zero from the IOM also
 implies that Multics could only be cold booted from a tape drive with
 device-id zero.  That doesn't seem to mesh with instructions
 elsewhere... And BCE has to (initially) come from the boot tape...

     Comment by CAC:  From MDD-005-02:

       "     bootload_tape_label  is read  in by  one of  two means.   In
        native mode, the  IOM or IMU reads it into  absolute location 30,
        leaving  the PCW,  DCW's, and   other essentials  in locations  0
        through  5.  The IMU  leaves an indication  of its identity  just
        after this block of information.

             In  BOS compatibility mode,  the BOS BOOT  command simulates
        the IOM, leaving the same information.  However, it also leaves a
        config deck and flagbox (although bce has its own flagbox) in the
        usual locations.   This allows Bootload Multics to  return to BOS
        if there is a BOS to return to.  The presence of BOS is indicated
        by the tape drive number being  non-zero in the idcw in the "IOM"
        provided information.   (This is normally zero  until firmware is
        loaded into the bootload tape MPC.) 

 The T&D tape seems to want to see non-zero residue from the very
 first tape read.  That seems to imply that the T&D tape could not
 be booted by the IOM!  Perhaps the T&D tape requires BCE (which
 replaced BOS) ?
 
 TODO
 
 When simulating timing, switch to queuing the activity instead
 of queueing the status return.   That may allow us to remove most
 of our state variables and more easily support save/restore.

 Convert the rest of the routines to have a chan_devinfo argument.
 
 Allow multiple tapes per channel.
 */

#include "sim_tape.h"

static DEBTAB mt_dt [] =
  {
    { "NOTIFY", DBG_NOTIFY, NULL },
    { "INFO", DBG_INFO, NULL },
    { "ERR", DBG_ERR, NULL },
    { "WARN", DBG_WARN, NULL },
    { "DEBUG", DBG_DEBUG, NULL },
    { "ALL", DBG_ALL, NULL }, // don't move as it messes up DBG message
    { NULL, 0, NULL }
  };

//////////////
//////////////
//
// MTP
//

#define MTP_UNIT_IDX(uptr) ((uptr) - mtp_unit)
#define N_MTP_UNITS 1 // default

static struct mtp_state_s
  {
    uint boot_drive;
    char device_name [MAX_DEV_NAME_LEN];
  } mtp_state [N_MTP_UNITS_MAX];

static t_stat mtp_show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr, 
                               UNUSED int val, UNUSED const void * desc)
  {
    sim_printf ("Number of MTP controllers in the system is %d\n",
                mtp_dev.numunits);
    return SCPE_OK;
  }

static t_stat mtp_set_nunits (UNUSED UNIT * uptr, UNUSED int32 value, 
                              const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    int n = atoi (cptr);
    if (n < 0 || n > N_MTP_UNITS_MAX)
      return SCPE_ARG;
    mtp_dev.numunits = (uint32) n;
    return SCPE_OK;
  }


static t_stat mtp_show_boot_drive (UNUSED FILE * st, UNIT * uptr, 
                                   UNUSED int val, UNUSED const void * desc)
  {
    long mtp_unit_idx = MTP_UNIT_IDX (uptr);
    if (mtp_unit_idx < 0 || mtp_unit_idx >= N_MTP_UNITS_MAX)
      {
        sim_printf ("Controller unit number out of range\n");
        return SCPE_ARG;
      }
    sim_printf ("Tape drive dev_code to boot from is %u\n",
                mtp_state[mtp_unit_idx].boot_drive);
    return SCPE_OK;
  }

static t_stat mtp_set_boot_drive (UNIT * uptr, UNUSED int32 value, 
                                 const char * cptr, UNUSED void * desc)
  {
    long mtp_unit_idx = MTP_UNIT_IDX (uptr);
    if (mtp_unit_idx < 0 || mtp_unit_idx >= N_MTP_UNITS_MAX)
      {
        sim_printf ("Controller unit number out of range\n");
        return SCPE_ARG;
      }
    if (! cptr)
      return SCPE_ARG;
    int n = (int) atoi (cptr);
    if (n < 0 || n >= N_DEV_CODES)
      return SCPE_ARG;
    mtp_state[mtp_unit_idx].boot_drive = (uint) n;
    return SCPE_OK;
  }

UNIT mtp_unit [N_MTP_UNITS_MAX] =
   {
      [0 ... N_MTP_UNITS_MAX-1] =
        {
          UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL
        }
  };

static t_stat mtp_show_device_name (UNUSED FILE * st, UNIT * uptr, 
                                    UNUSED int val, UNUSED const void * desc)
  {
    int n = (int) MTP_UNIT_IDX (uptr);
    if (n < 0 || n >= N_MTP_UNITS_MAX)
      return SCPE_ARG;
    sim_printf("Controller device name is %s\n", mtp_state [n].device_name);
    return SCPE_OK;
  }

static t_stat mtp_set_device_name (UNIT * uptr, UNUSED int32 value, 
                                   const char * cptr, UNUSED void * desc)
  {
    int n = (int) MTP_UNIT_IDX (uptr);
    if (n < 0 || n >= N_MTP_UNITS_MAX)
      return SCPE_ARG;
    if (cptr)
      {
        strncpy (mtp_state[n].device_name, cptr, MAX_DEV_NAME_LEN-1);
        mtp_state[n].device_name[MAX_DEV_NAME_LEN-1] = 0;
      }
    else
      mtp_state[n].device_name[0] = 0;
    return SCPE_OK;
  }

static MTAB mtp_mod [] =
  {
    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO | MTAB_VALR, /* mask */
      0,            /* match */
      "NUNITS",     /* print string */
      "NUNITS",         /* match string */
      mtp_set_nunits, /* validation routine */
      mtp_show_nunits, /* display routine */
      "Number of TAPE units in the system", /* value descriptor */
      NULL          // help
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_VALR, /* mask */
      0,            /* match */
      "BOOT_DRIVE",     /* print string */
      "BOOT_DRIVE",         /* match string */
      mtp_set_boot_drive, /* validation routine */
      mtp_show_boot_drive, /* display routine */
      "Select the boot drive", /* value descriptor */
      NULL          // help
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_VALR | MTAB_NC, /* mask */
      0,            /* match */
      "NAME",     /* print string */
      "NAME",         /* match string */
      mtp_set_device_name, /* validation routine */
      mtp_show_device_name, /* display routine */
      "Set the device name", /* value descriptor */
      NULL          // help
    },
    { 0, 0, NULL, NULL, NULL, NULL, NULL, NULL }
  };

static t_stat mtp_reset (UNUSED DEVICE * dptr)
  {
    return SCPE_OK;
  }

DEVICE mtp_dev =
  {
    "MTP",            /* name */
    mtp_unit,         /* units */
    NULL,             /* registers */
    mtp_mod,          /* modifiers */
    N_MTP_UNITS,      /* #units */
    10,               /* address radix */
    31,               /* address width */
    1,                /* address increment */
    8,                /* data radix */
    9,                /* data width */
    NULL,             /* examine routine */
    NULL,             /* deposit routine */
    mtp_reset,        /* reset routine */
    NULL,             /* boot routine */
    NULL,             /* attach routine */
    NULL,             /* detach routine */
    NULL,             /* context */
    DEV_DEBUG,        /* flags */
    0,                /* debug control flags */
    mt_dt,            /* debug flag names */
    NULL,             /* memory size change */
    NULL,             /* logical name */
    NULL,             // attach help
    NULL,             // help
    NULL,             // help context
    NULL,             // device description
    NULL
  };


//////////////
//////////////
//
// tape drive
//


#define MT_UNIT_NUM(uptr) ((uptr) - mt_unit)

struct tape_state tape_states [N_MT_UNITS_MAX];
static const char * simh_tape_msg (int code); // hack
// XXX this assumes only one controller, needs to be indexed
#define TAPE_PATH_LEN 4096
static char tape_path [TAPE_PATH_LEN];


#define N_MT_UNITS 1 // default

UNIT mt_unit [N_MT_UNITS_MAX] =
   {
    // NOTE: other SIMH tape sims don't set UNIT_SEQ
    // CAC: Looking at SIMH source, the only place UNIT_SEQ is used
    // by the "run" command's reset sequence; units that have UNIT_SEQ
    // set will be issued a rewind on reset.
    // Looking at the sim source again... It is used on several of the
    // run commands, including CONTINUE.
    // Turning UNIT_SEQ off.
    // XXX Should we rewind on reset? What is the actual behavior?
// Unit 0 is the controller
      [0 ... N_MT_UNITS_MAX-1] =
        {
          UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE |
                       UNIT_DISABLE | UNIT_IDLE, 0),
          0, 0, 0, 0, 0, NULL, NULL, NULL, NULL
        }
  };

#define UNIT_WATCH (1 << MTUF_V_UF)

static t_stat mt_rewind (UNIT * uptr, UNUSED int32 value, 
                         UNUSED const char * cptr, UNUSED void * desc)
  {
    return sim_tape_rewind (uptr);
  }

static t_stat mt_show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr, 
                              UNUSED int val, UNUSED const void * desc)
  {
    sim_printf("Number of TAPE units in system is %d\n", tape_dev . numunits);
    return SCPE_OK;
  }

static t_stat mt_set_nunits (UNUSED UNIT * uptr, UNUSED int32 value, 
                             const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    int n = atoi (cptr);
    if (n < 1 || n > N_MT_UNITS_MAX)
      return SCPE_ARG;
    tape_dev . numunits = (uint32) n;
    return SCPE_OK;
  }

static t_stat mt_show_device_name (UNUSED FILE * st, UNIT * uptr, 
                                   UNUSED int val, UNUSED const void * desc)
  {
    int n = (int) MT_UNIT_NUM (uptr);
    if (n < 0 || n >= N_MT_UNITS_MAX)
      return SCPE_ARG;
    sim_printf("Tape drive device name is %s\n", tape_states [n] . device_name);
    return SCPE_OK;
  }

static t_stat mt_set_device_name (UNUSED UNIT * uptr, UNUSED int32 value, 
                                  UNUSED const char * cptr, UNUSED void * desc)
  {
    int n = (int) MT_UNIT_NUM (uptr);
    if (n < 0 || n >= N_MT_UNITS_MAX)
      return SCPE_ARG;
    if (cptr)
      {
        strncpy (tape_states [n] . device_name, cptr, MAX_DEV_NAME_LEN - 1);
        tape_states [n] . device_name [MAX_DEV_NAME_LEN - 1] = 0;
      }
    else
      tape_states [n] . device_name [0] = 0;
    return SCPE_OK;
  }

static t_stat mt_show_tape_path (UNUSED FILE * st, UNUSED UNIT * uptr, 
                                 UNUSED int val, UNUSED const void * desc)
  {
    sim_printf("Tape path <%s>\n", tape_path);
    return SCPE_OK;
  }

static t_stat mt_set_tape_path (UNUSED UNIT * uptr, UNUSED int32 value, 
                             const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    if (strlen (cptr) >= TAPE_PATH_LEN - 1)
      {
        sim_printf ("truncating tape path\n");
      }
    strncpy (tape_path, cptr, TAPE_PATH_LEN);
    tape_path [TAPE_PATH_LEN - 1] = 0;
    return SCPE_OK;
  }

static t_stat mt_set_capac (UNUSED UNIT * uptr, UNUSED int32 value, 
                             const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    t_stat rc;
    int i;
    // skip the boot tape drive; Multics doesn't use it, and this
    // allows setting capacity even though the boot tape is attached. 
    for (i = 1; i < N_MT_UNITS_MAX; i ++)
      {
        rc = sim_tape_set_capac (mt_unit + i, value, cptr, desc);
        if (rc != SCPE_OK)
          return rc;
      }
    return SCPE_OK;
  }


static MTAB mt_mod [] =
  {
    { UNIT_WATCH, UNIT_WATCH, "WATCH", "WATCH", NULL, NULL, NULL, NULL },
    { UNIT_WATCH, 0, "NOWATCH", "NOWATCH", NULL, NULL, NULL, NULL },
    {
       MTAB_XTD | MTAB_VUN | MTAB_NC, /* mask */
      0,            /* match */
      NULL,         /* print string */
      "REWIND",     /* match string */
      mt_rewind,    /* validation routine */
      NULL,         /* display routine */
      NULL,         /* value descriptor */
      NULL          // help
    },
    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO | MTAB_VALR, /* mask */
      0,            /* match */
      "NUNITS",     /* print string */
      "NUNITS",         /* match string */
      mt_set_nunits, /* validation routine */
      mt_show_nunits, /* display routine */
      "Number of TAPE units in the system", /* value descriptor */
      NULL          // help
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_VALR | MTAB_NC, /* mask */
      0,            /* match */
      "NAME",     /* print string */
      "NAME",         /* match string */
      mt_set_device_name, /* validation routine */
      mt_show_device_name, /* display routine */
      "Set the device name", /* value descriptor */
      NULL          // help
    },
    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO | MTAB_VALR, /* mask */
      0,            /* match */
      "TAPE_PATH",     /* print string */
      "TAPE_PATH",         /* match string */
      mt_set_tape_path, /* validation routine */
      mt_show_tape_path, /* display routine */
      "Set the path to the directory containing tape images", /* value descriptor */
      NULL          // help
    },
    {
      MTAB_XTD | MTAB_VUN, /* mask */
      0,            /* match */
      "CAPACITY",     /* print string */
      "CAPACITY",         /* match string */
      sim_tape_set_capac, /* validation routine */
      sim_tape_show_capac, /* display routine */
      "Set the device capacity", /* value descriptor */
      NULL          // help
    },
    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO | MTAB_VALR, /* mask */
      0,            /* match */
      "CAPACITY_ALL",     /* print string */
      "CAPACITY_ALL",         /* match string */
      mt_set_capac, /* validation routine */
      NULL, /* display routine */
      "Set the tape capacity of all drives", /* value descriptor */
      NULL          // help
    },
    { 0, 0, NULL, NULL, NULL, NULL, NULL, NULL }
  };

static t_stat mt_reset (DEVICE * dptr)
  {
    for (int i = 0; i < (int) dptr -> numunits; i ++)
      {
        sim_tape_reset (& mt_unit [i]);
        //sim_cancel (& mt_unit [i]);
      }
    return SCPE_OK;
  }


DEVICE tape_dev =
  {
    "TAPE",           /* name */
    mt_unit,          /* units */
    NULL,             /* registers */
    mt_mod,           /* modifiers */
    N_MT_UNITS,       /* #units */
    10,               /* address radix */
    31,               /* address width */
    1,                /* address increment */
    8,                /* data radix */
    9,                /* data width */
    NULL,             /* examine routine */
    NULL,             /* deposit routine */
    mt_reset,         /* reset routine */
    NULL,             /* boot routine */
    &sim_tape_attach, /* attach routine */
    &sim_tape_detach, /* detach routine */
    NULL,             /* context */
    DEV_DEBUG,        /* flags */
    0,                /* debug control flags */
    mt_dt,            /* debug flag names */
    NULL,             /* memory size change */
    NULL,             /* logical name */
    NULL,             // attach help
    NULL,             // help
    NULL,             // help context
    NULL,             // device description
    NULL
  };


void loadTape (uint driveNumber, char * tapeFilename, bool ro)
  {
    if (ro)
      mt_unit [driveNumber] . flags |= MTUF_WRP;
    else
      mt_unit [driveNumber] . flags &= ~ MTUF_WRP;
    t_stat stat = sim_tape_attach (& mt_unit [driveNumber], tapeFilename);
    if (stat != SCPE_OK)
      {
        sim_printf ("%s sim_tape_attach returned %d\n", __func__, stat);
        return;
      }

    if (! sim_is_running)
      return;

    uint ctlr_unit_idx = cables->tape_to_mtp [driveNumber].ctlr_unit_idx;
    // Which port should the controller send the interrupt to? All of them...
    bool sent_one = false;
    for (uint ctlr_port_num = 0; ctlr_port_num < MAX_CTLR_PORTS; ctlr_port_num ++)
      if (cables->mtp_to_iom[ctlr_unit_idx][ctlr_port_num].in_use)
        {
          uint iom_unit_idx = cables->mtp_to_iom[ctlr_unit_idx][ctlr_port_num].iom_unit_idx;
          uint chan_num = cables->mtp_to_iom[ctlr_unit_idx][ctlr_port_num].chan_num;
          uint dev_code = cables->tape_to_mtp[driveNumber].dev_code;

          send_special_interrupt (iom_unit_idx, chan_num, dev_code, 0, 020 /* tape drive to ready */);
          sent_one = true;
        }
    if (! sent_one)
      {
        sim_printf ("loadTape can't find controller; dropping interrupt\n");
        return;
      }
  }

static void unloadTape (uint driveNumber)
  {
    if (mt_unit [driveNumber] . flags & UNIT_ATT)
      {
        t_stat stat = sim_tape_detach (& mt_unit [driveNumber]);
        if (stat != SCPE_OK)
          {
            sim_warn ("%s sim_tape_detach returned %d\n", __func__, stat);
            return;
          }
      }
    uint ctlr_unit_idx = cables->tape_to_mtp [driveNumber].ctlr_unit_idx;
    // Which port should the controller send the interrupt to?
    // Find one that is connected...
    uint ctlr_port_num;
    for (ctlr_port_num = 0; ctlr_port_num < MAX_CTLR_PORTS; ctlr_port_num ++)
      if (cables->mtp_to_iom[driveNumber][ctlr_port_num].in_use)
        break;
    if (ctlr_port_num >= MAX_CTLR_PORTS)
      {
        sim_printf ("loadTape can't file controller; dropping interrupt\n");
        return;
      }
    uint iom_unit_idx = cables->mtp_to_iom[ctlr_unit_idx][ctlr_port_num].iom_unit_idx;
    uint chan_num = cables->mtp_to_iom[ctlr_unit_idx][ctlr_port_num].chan_num;
    uint dev_code = cables->tape_to_mtp[driveNumber].dev_code;

    send_special_interrupt (iom_unit_idx, chan_num, dev_code, 0, 040 /* unload complere */);
  }

void mt_init(void)
  {
    memset(tape_states, 0, sizeof(tape_states));
    for (int i = 0; i < N_MT_UNITS_MAX; i ++)
      {
        mt_unit [i] . capac = 40000000;
      }
  }

static int mtReadRecord (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {


// If a tape read IDCW has multiple DDCWs, are additional records read?

    iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
    UNIT * unitp = & mt_unit [devUnitIdx];
    struct tape_state * tape_statep = & tape_states [devUnitIdx];

    enum { dataOK, noTape, tapeMark, tapeEOM, tapeError } tapeStatus;
    tape_statep -> is9 = p -> IDCW_DEV_CMD == 003;
    sim_debug (DBG_DEBUG, & tape_dev, "%s: Read %s record\n", __func__,
               tape_statep -> is9 ? "9" : "binary");
    // We read the record into the tape controllers memory;
    // IOM will subsequently retrieve the data via DCWs.
    tape_statep -> tbc = 0;
    if (! (unitp -> flags & UNIT_ATT))
      {
        tapeStatus = noTape;
        goto ddcws;
      }
    int rc = sim_tape_rdrecf (unitp, & tape_statep -> buf [0], & tape_statep -> tbc,
                               BUFSZ);
    sim_debug (DBG_DEBUG, & tape_dev, "sim_tape_rdrecf returned %d, with tbc %d\n", rc, tape_statep -> tbc);
    if (rc == MTSE_TMK)
       {
         tape_statep -> rec_num ++;
         sim_debug (DBG_NOTIFY, & tape_dev,
                    "%s: EOF: %s\n", __func__, simh_tape_msg (rc));
        p -> stati = 04423; // EOF category EOF file mark
        if (tape_statep -> tbc != 0)
          {
            sim_warn ("%s: Read %d bytes with EOF.\n", 
                        __func__, tape_statep -> tbc);
          }
        tape_statep -> tbc = 0;
        tapeStatus = tapeMark;
        goto ddcws;
      }
    if (rc == MTSE_EOM)
      {
        sim_debug (DBG_NOTIFY, & tape_dev,
                    "%s: EOM: %s\n", __func__, simh_tape_msg (rc));
// If the tape is blank, a read should result in '4302' blank tape on read.
        if (sim_tape_bot (unitp))
          p -> stati = 04302; // blank tape on read
        else
          p -> stati = 04340; // EOT file mark
        if (tape_statep -> tbc != 0)
          {
            sim_warn ("%s: Read %d bytes with EOM.\n", 
                        __func__, tape_statep -> tbc);
            //return 0;
          }
        tape_statep -> tbc = 0;
        tapeStatus = tapeEOM;
        goto ddcws;
      }
    if (rc != 0)
      {
        sim_debug (DBG_ERR, & tape_dev,
                   "%s: Cannot read tape: %d - %s\n",
                   __func__, rc, simh_tape_msg (rc));
        sim_debug (DBG_ERR, & tape_dev,
                   "%s: Returning arbitrary error code\n",
                   __func__);
        p -> stati = 05001; // BUG: arbitrary error code; config switch
        p -> chanStatus = chanStatParityErrPeriph;
        //return 0;
        tapeStatus = tapeEOM;
        goto ddcws;
      }
    p -> stati = 04000;
    if (sim_tape_wrp (unitp))
      p -> stati |= 1;
    tape_statep -> rec_num ++;
    tapeStatus = dataOK;
    p -> initiate = false; 

ddcws:;

    tape_statep -> words_processed = 0;
    if (unitp->flags & UNIT_WATCH)
      sim_printf ("Tape %ld reads record %d\n",
                  (long) MT_UNIT_NUM (unitp), tape_statep -> rec_num);
    tape_statep -> io_mode = tape_read_mode;


// Process DDCWs

    bool ptro, send, uff;
    do
      {
        int rc2 = iom_list_service (iomUnitIdx, chan, & ptro, & send, & uff);
        if (rc2 < 0)
          {
            p -> stati = 05001; // BUG: arbitrary error code; config switch
            sim_warn ("%s list service failed\n", __func__);
            return IOM_CMD_ERROR;
          }
        if (uff)
          {
            sim_warn ("%s ignoring uff\n", __func__); // XXX
          }
        if (! send)
          {
            sim_warn ("%s nothing to send\n", __func__);
            p -> stati = 05001; // BUG: arbitrary error code; config switch
            return IOM_CMD_IGNORED;
          }
        if (p -> DCW_18_20_CP == 07 || p -> DDCW_22_23_TYPE == 2)
          {
            sim_warn ("%s expected DDCW\n", __func__);
            p -> stati = 05001; // BUG: arbitrary error code; config switch
            return IOM_CMD_ERROR;
          }


        if (tapeStatus == dataOK)
          {
            uint tally = p -> DDCW_TALLY;
            if (tally == 0)
              {
                sim_debug (DBG_DEBUG, & tape_dev,
                           "%s: Tally of zero interpreted as 010000(4096)\n",
                           __func__);
                tally = 4096;
              }

            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Tally %d (%o)\n", __func__, tally, tally);

            word36 buffer [tally];
            uint i;
            for (i = 0; i < tally; i ++)
              {
                if (tape_statep -> is9)
                  rc2 = extractASCII36FromBuffer (tape_statep -> buf, tape_statep -> tbc, & tape_statep -> words_processed, buffer + i);
                else
                  rc2 = extractWord36FromBuffer (tape_statep -> buf, tape_statep -> tbc, & tape_statep -> words_processed, buffer + i);
                if (rc2)
                  {
                     break;
                  }
              }
#if 0
            if (tape_statep -> is9) {
              sim_printf ("<");
                for (uint i = 0; i < tally * 4; i ++) {
                uint wordno = i / 4;
                uint charno = i % 4;
                uint ch = (buffer [wordno] >> ((3 - charno) * 9)) & 0777;
                if (isprint (ch))
                  sim_printf ("%c", ch);
                else
                  sim_printf ("\\%03o", ch);
              }
                sim_printf (">\n");
            }
#endif
            iom_indirect_data_service (iomUnitIdx, chan, buffer,
                                    & tape_statep -> words_processed, true);
            if (p -> tallyResidue)
              {
                sim_debug (DBG_WARN, & tape_dev,
                           "%s: Read buffer exhausted on channel %d\n",
                               __func__, chan);

              }
// XXX This assumes that the tally was bigger then the record
            if (tape_statep -> is9)
              p -> charPos = tape_statep -> tbc % 4;
            else
              p -> charPos = (tape_statep -> tbc * 8) / 9 % 4;
          }

        //if (p -> DDCW_22_23_TYPE != 0)
          //sim_warn ("curious... a tape read with more than one DDCW?\n");

      }
    while (p -> DDCW_22_23_TYPE != 0); // while not IOTD
    //if (sim_tape_wrp (unitp))
      //p -> stati |= 1;
    return 0;
  }

static int mtWriteRecord (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {

// If a tape write IDCW has multiple DDCWs, are additional records written?

    iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
    UNIT * unitp = & mt_unit [devUnitIdx];
    struct tape_state * tape_statep = & tape_states [devUnitIdx];

    tape_statep -> is9 = p -> IDCW_DEV_CMD == 013;
    sim_debug (DBG_DEBUG, & tape_dev, "%s: Write %s record\n", __func__,
               tape_statep -> is9 ? "9" : "binary");

    p -> isRead = false;

// Get the DDCW

    bool ptro, send, uff;
loop:;
    int rc = iom_list_service (iomUnitIdx, chan, & ptro, & send, & uff);
//sim_printf ("DDCW_22_23_TYPE %u\n", p->DDCW_22_23_TYPE);
    if (rc < 0)
      {
        p -> stati = 05001; // BUG: arbitrary error code; config switch
        sim_warn ("%s list service failed\n", __func__);
        return IOM_CMD_ERROR;
      }
    if (uff)
      {
        sim_warn ("%s ignoring uff\n", __func__); // XXX
      }
    if (! send)
      {
        sim_warn ("%s nothing to send\n", __func__);
        p -> stati = 05001; // BUG: arbitrary error code; config switch
        return IOM_CMD_IGNORED;
      }
    if (p -> DCW_18_20_CP == 07 || p -> DDCW_22_23_TYPE == 2)
      {
        sim_warn ("%s expected DDCW\n", __func__);
        p -> stati = 05001; // BUG: arbitrary error code; config switch
        return IOM_CMD_ERROR;
      }


    uint tally = p -> DDCW_TALLY;
    if (tally == 0)
      {
        sim_debug (DBG_DEBUG, & tape_dev,
                   "%s: Tally of zero interpreted as 010000(4096)\n",
                   __func__);
        tally = 4096;
      }

    sim_debug (DBG_DEBUG, & tape_dev,
               "%s: Tally %d (%o)\n", __func__, tally, tally);

    // Fetch data from core into buffer

    tape_statep -> words_processed = 0;
    word36 buffer [tally];

    iom_indirect_data_service (iomUnitIdx, chan, buffer,
                            & tape_statep -> words_processed, false);

#if 0
            if (tape_statep -> is9) {
              sim_printf ("<");
                for (uint i = 0; i < tally * 4; i ++) {
                uint wordno = i / 4;
                uint charno = i % 4;
                uint ch = (buffer [wordno] >> ((3 - charno) * 9)) & 0777;
                if (isprint (ch))
                  sim_printf ("%c", ch);
                else
                  sim_printf ("\\%03o", ch);
              }
                sim_printf (">\n");
            }
#endif
// XXX char_pos ??

    if (tape_statep -> is9)
      tape_statep -> tbc = tape_statep -> words_processed * 4;
    else
      tape_statep -> tbc = (tape_statep -> words_processed * 9 + 1) / 2;

    // Pack data from buffer into tape format

    tape_statep -> words_processed = 0;
    uint i;
    for (i = 0; i < tally; i ++)
      {
        int rc2;
        if (tape_statep -> is9)
          {
            rc2 = insertASCII36toBuffer (tape_statep -> buf, 
                                        tape_statep -> tbc, 
                                        & tape_statep -> words_processed, 
                                        buffer [i]);
          }
        else
          {
            rc2 = insertWord36toBuffer (tape_statep -> buf, 
                                       tape_statep -> tbc, 
                                       & tape_statep -> words_processed, 
                                       buffer [i]);
            }
        if (rc2)
          {
            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            sim_debug (DBG_WARN, & tape_dev,
                       "%s: Write buffer exhausted on channel %d\n",
                       __func__, chan);
            break;
          }
      }
    p -> tallyResidue = (word12) (tally - i);

// XXX This assumes that the tally was bigger then the record
    if (tape_statep -> is9)
      p -> charPos = tape_statep -> tbc % 4;
    else
      p -> charPos = (tape_statep -> tbc * 8) / 9 % 4;
  
    // Write buf to tape

    if (! (unitp -> flags & UNIT_ATT))
      return MTSE_UNATT;

    int ret = sim_tape_wrrecf (unitp, tape_statep -> buf, tape_statep -> tbc);
    sim_debug (DBG_DEBUG, & tape_dev, "sim_tape_wrrecf returned %d, with tbc %d\n", ret, tape_statep -> tbc);

    if (unitp->io_flush)
      unitp->io_flush (unitp);                              /* flush buffered data */
    // XXX put unit number in here...

    if (ret != 0)
      {
        // Actually only returned on read
        if (ret == MTSE_EOM)
          {
            sim_debug (DBG_NOTIFY, & tape_dev,
                        "%s: EOM: %s\n", __func__, simh_tape_msg (ret));
            p -> stati = 04340; // EOT file mark
            if (tape_statep -> tbc != 0)
              {
                sim_warn ("%s: Wrote %d bytes with EOM.\n",
                           __func__, tape_statep -> tbc);
              }
            return 0;
          }
        sim_warn ("%s: Cannot write tape: %d - %s\n",
                   __func__, ret, simh_tape_msg(ret));
        sim_warn ("%s: Returning arbitrary error code\n",
                   __func__);
        p -> stati = 05001; // BUG: arbitrary error code; config switch
        p -> chanStatus = chanStatParityErrPeriph;
        return IOM_CMD_ERROR;
      }
    tape_statep -> rec_num ++;
    if (unitp->flags & UNIT_WATCH)
      sim_printf ("Tape %ld writes record %d\n",
                  (long) MT_UNIT_NUM (unitp), tape_statep -> rec_num);

    sim_tape_wreom (unitp);
    if (unitp->io_flush)
      unitp->io_flush (unitp);                              /* flush buffered data */

    p -> stati = 04000;
    if (sim_tape_wrp (unitp))
      p -> stati |= 1;
    if (sim_tape_eot (unitp))
      p -> stati |= 04340;

    sim_debug (DBG_INFO, & tape_dev,
               "%s: Wrote %d bytes to simulated tape; status %04o\n",
               __func__, (int) tape_statep -> tbc, p -> stati);

    //if (p -> DDCW_22_23_TYPE != 0)
      //sim_warn ("curious... a tape write with more than one DDCW?\n");
    if (p -> DDCW_22_23_TYPE == 1) // IOTP
      {
        //sim_printf ("saw IOTP, looping\n");
        goto loop;
      }

    //if (sim_tape_wrp (unitp))
      //p -> stati |= 1;
    return 0;
  }

// 0 ok
// -1 problem
static int surveyDevices (uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
// According to rcp_tape_survey_.pl1:
//
//       2 survey_data,
//         3 handler (16) unaligned,
//           4 pad1 bit (1),               400000
//           4 reserved bit (1),           200000
//           4 operational bit (1),        100000
//           4 ready bit (1),               40000
//           4 number uns fixed bin (5),    37000
//           4 pad2 bit (1),                  400
//           4 speed uns fixed bin (3),       240
//           4 nine_track bit (1),             20
//           4 density uns fixed bin (4);      17

    sim_debug (DBG_DEBUG, & tape_dev,
               "%s: Survey devices\n", __func__);
    p -> stati = 04000; // have_status = 1
    // Get the DDCW
    bool ptro, send, uff;
    int rc = iom_list_service (iomUnitIdx, chan, & ptro, & send, & uff);
    if (rc < 0)
      {
        sim_warn ("%s list service failed\n", __func__);
        p -> stati = 05001; // BUG: arbitrary error code; config switch
        p -> chanStatus = chanStatIncomplete;
        return IOM_CMD_ERROR;
      }
    if (uff)
      {
        sim_warn ("%s ignoring uff\n", __func__); // XXX
      }
    if (! send)
      {
        sim_warn ("%s nothing to send\n", __func__);
        p -> stati = 05001; // BUG: arbitrary error code; config switch
        p -> chanStatus = chanStatIncomplete;
        return IOM_CMD_ERROR;
      }
    if (p -> DCW_18_20_CP == 07 || p -> DDCW_22_23_TYPE == 2)
      {
        sim_warn ("%s expected DDCW\n", __func__);
        p -> stati = 05001; // BUG: arbitrary error code; config switch
        p -> chanStatus = chanStatIncorrectDCW;
        return IOM_CMD_ERROR;
      }

    if (p -> DDCW_TALLY != 8)
      {
        sim_debug (DBG_DEBUG, & tape_dev,
                   "%s: Expected tally of 8; got %d\n",
                   __func__, p -> DDCW_TALLY);
        p -> stati = 05001; // BUG: arbitrary error code; config switch
        p -> chanStatus = chanStatIncorrectDCW;
        return IOM_CMD_ERROR;
      }

    uint bufsz = 8;
    word36 buffer [bufsz];
    uint cnt = 0;
    for (uint i = 0; i < bufsz; i ++)
      buffer [i] = 0;
    
    uint ctlr_idx = get_ctlr_idx (iomUnitIdx, chan);
    // Walk the device codes
    for (uint dev_code = 0; dev_code < N_DEV_CODES; dev_code ++)
      {
       if (cnt / 2 >= bufsz)
          break;
        // Which device on the string is connected to that device code
        struct ctlr_to_dev_s * p = & cables->mtp_to_tape[ctlr_idx][dev_code];
        if (! p -> in_use)
          continue;
        uint unit_idx = p->unit_idx;

        word18 handler = 0;
        handler |= 0100000; // operational
        if (mt_unit [unit_idx].filename)
          {
            handler |= 0040000; // ready
          }
        handler |= ((word18) dev_code & 037) << 9; // number
        handler |= 0000040; // 200 ips
        handler |= 0000020; // 9 track
        handler |= 0000007; // 800/1600/6250
        sim_debug (DBG_DEBUG, & tape_dev,
                   "unit %d handler %06o\n", unit_idx, handler);
        if (cnt % 2 == 0)
          {
            buffer [cnt / 2] = ((word36) handler) << 18;
          }
        else
          {
            buffer [cnt / 2] |= handler;
          }
        cnt ++;
      }
    iom_indirect_data_service (iomUnitIdx, chan, buffer, & bufsz, true);
    p -> stati = 04000;
    return 0;
  }

// Tally: According to tape_ioi_io.pl1:
//
// backspace file, forward space file, write EOF, erase:
//   tally is always set to 1
// backspace record, forward space record.
//   tally is set to count; tally of 0 means 64.
// density, write control registers
//   tally is set to one.
// request device status
//   don't know
// data security erase, rewind, rewind/unload, tape load, request status, 
// reset status, request device status, reset device status, set file permit, 
// set file protect, reserve device, release device, read control registers
//   no idcw.

static int mt_cmd (uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];

// According to poll_mpc.pl1
// Note: XXX should probably be checking these...
//  idcw.chan_cmd = "40"b3; /* Indicate special controller command */
//  idcw.chan_cmd = "41"b3; /* Indicate special controller command */

// The bootload read command does a read on drive 0; the controler
// recgnizes (somehow) a special case for bootload and subs. in
// the boot drive unit set by the controller config. switches
// XXX But controller commands are directed to drive 0, so this
// logic is incorrect. If we just set the boot drive to 0, the
// system will just boot from 0, and ignore it thereafter.
// Although, the install process identifies tapa_00 as a device;
// check the survey code to make sure it's not incorrectly
// reporting 0 as a valid device.

// Simplifying design decision: tapa_00 is hidden, always has the boot tape.

    uint ctlr_unit_idx = get_ctlr_idx (iomUnitIdx, chan);

    sim_debug (DBG_DEBUG, & tape_dev, "IDCW_DEV_CODE %d\n", p -> IDCW_DEV_CODE);
    uint dev_code = p -> IDCW_DEV_CODE;
    if (p -> IDCW_DEV_CODE == 0)
      dev_code = mtp_state[ctlr_unit_idx].boot_drive;
    sim_debug (DBG_DEBUG, & tape_dev, "dev_code %d\n", dev_code);

    uint devUnitIdx = cables->mtp_to_tape[ctlr_unit_idx][dev_code].unit_idx;
    UNIT * unitp = & mt_unit [devUnitIdx];
    struct tape_state * tape_statep = & tape_states [devUnitIdx];

    tape_statep -> io_mode = tape_no_mode;
//sim_printf ("mt cmd dev_code %u cmd %u. 0%o\n", dev_code, p -> IDCW_DEV_CMD, p -> IDCW_DEV_CMD);
    sim_debug (DBG_DEBUG, & tape_dev, "IDCW_DEV_CMD %oo %d.\n", p->IDCW_DEV_CMD, p->IDCW_DEV_CMD);
    switch (p -> IDCW_DEV_CMD)
      {
        case 0: // CMD 00 Request status -- controller status, not tape drive
          {
            if (p -> IDCW_CHAN_CMD == 040) // If special controller command, then command 0 is 'suspend'
              {
                sim_debug (DBG_DEBUG, & tape_dev,
                           "controller suspend\n");
                send_special_interrupt (iomUnitIdx, chan, p->IDCW_DEV_CODE, 01, 0 /* suspended */);
                p -> stati = 04000; // have_status = 1
              }
            else
              {
                p -> stati = 04000; // have_status = 1
              }
//sim_printf ("tape req status chan_cmd %o\n", p -> IDCW_CHAN_CMD);
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Request status: %04o\n", __func__, p -> stati);
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Request status control: %o\n", __func__, p -> IDCW_CONTROL);
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Request status channel command: %o\n", __func__, p -> IDCW_CHAN_CMD);
          }
          break;

// dcl  1 stat_buf aligned based (workp),                      /* The IOI buffer segment */
//        2 idcw1 bit (36),                                    /* Will be read controller main memory */
//        2 dcw1 bit (36),                                     /* Addr=stat_buf.control, tally=1 */
//        2 idcw2 bit (36),                                    /* Will be initiate read data transfer */
//        2 dcw2 bit (36),                                     /* Address=stat_buf.mem, tally=rest of segment */


//        2 control,                                           /* Describes where data is in mpc */
//          3 addr bit (16) unal,                              /* Addr in mpc memory */
//          3 tally bit (16) unal,                             /* Count in mpc words */
//          3 fill bit (4) unal,
//        2 stats (0:83) bit (18) unal;                        /* EURC statistics in ASCII */

//       2 mem (0:mpc_memory_size - 1) bit (18) unal;         /* This is the mpc memory */


//    / * Build read or write (dev stat block) main memory dcw list */
//    
//              idcwp = addr (buf.idcw1);                         /* First IDCW */
//              buf.idcw1 = "0"b;
//    
//              if OP = READ_MPC_MEM
//              then idcw.command = "02"b3;                       /* Command is read controller main memory (ASCII) */
//              else idcw.command = "32"b3;                       /* Command is write main memory (binary) */
//    
//              idcw.code = "111"b;                               /* This makes it an IDCW */
//              idcw.control = "10"b;                             /* Set continue bit */
//              idcw.chan_cmd = "40"b3;                           /* Indicate special controller command */
//              dcwp = addr (buf.dcw1);
//              buf.dcw1 = "0"b;
//              dcw.address = rel (addr (buf.control));           /* Get offset to control word */
//              dcw.tally = "000000000001"b;
//              idcwp = addr (buf.idcw2);                         /* Second IDCW */
//              buf.idcw2 = "0"b;
//              idcw.code = "111"b;                               /* Code is 111 to make it an idcw */
//              idcw.chan_cmd = "40"b3;                           /* Special controller command */
//              dcwp = addr (buf.dcw2);
//              buf.dcw2 = "0"b;
//              dcw.address = rel (addr (buf.mem));               /* Offset to core image */
//              dcw.tally = bit (bin (size (buf) - bin (rel (addr (buf.mem)), 18), 12));
//                                                                /* Rest of seg */
//    
//              if OP = READ_MPC_MEM then do;
//                   idcw.command = "06"b3;                       /* Command is initiate read data transfer */
//                   buf.addr = "0"b;                             /* Mpc address to start is 0 */
//                   buf.tally = bit (bin (mpc_memory_size, 16), 16);
//                   end;
//    
//    
// Control word:
//
//  
 
        case 02:               // CMD 02 -- Read controller main memory (ASCII)
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Read controller main memory\n", __func__);

            bool ptro, send, uff;
            int rc = iom_list_service (iomUnitIdx, chan, & ptro, & send, & uff);
            if (rc < 0)
              {
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                sim_warn ("%s list service failed\n", __func__);
                return IOM_CMD_ERROR;
              }
            if (uff)
              {
                sim_warn ("%s ignoring uff\n", __func__); // XXX
              }
            if (! send)
              {
                sim_warn ("%s nothing to send\n", __func__);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                return IOM_CMD_IGNORED;
              }
            if (p -> DCW_18_20_CP == 07 || p -> DDCW_22_23_TYPE == 2)
              {
                sim_warn ("%s expected DDCW\n", __func__);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                return IOM_CMD_ERROR;
              }

//sim_printf ("chan mode %d\n", p -> chanMode);
//sim_printf ("ddcw %012"PRIo64"\n", p -> DCW);
            word36 control;
	    uint count;
            iom_indirect_data_service (iomUnitIdx, chan, & control, &count, false);
//sim_printf ("control %012"PRIo64"\n", control);
//sim_printf ("  addr %012"PRIo64" tally %012"PRIo64"\n", getbits36_16 (control, 0), getbits36_16 (control, 16));
	    if (count != 1)
	      sim_warn ("%s: count %d not 1\n", __func__, count);
            tape_statep -> cntlrAddress = getbits36_16 (control, 0);
            tape_statep -> cntlrTally = getbits36_16 (control, 16);

            p -> stati = 04000;
          }
         break;

        case 3: // CMD 03 -- Read 9 Record
        case 5: // CMD 05 -- Read Binary Record
          {
            int rc = mtReadRecord (devUnitIdx, iomUnitIdx, chan);
            if (rc)
              return IOM_CMD_ERROR;
          }
          break;

// How is the mpc memory sent?
// This is the code from poll_mpc that extracts the memory copy from the buffer and 
// repacks it the way it wants"
//
//     2 mem (0:mpc_memory_size - 1) bit (18) unal;         /* This is the mpc memory */
//
//     dcl  mpc_mem_bin (0:4095) bit (16) unal;                    /* mpc mem converted to binary */
//
//     do i = 0 to mpc_memory_size - 1;
//       substr (mpc_mem_bin (i), 1, 8) = substr (buf.mem (i), 2, 8);
//       substr (mpc_mem_bin (i), 9, 8) = substr (buf.mem (i), 11, 8);
//     end;
//
// I interpet that as the 16 bit memory being broken into 8 bit bytes, zero
// extented to 9 bits, and packed 4 to a word.

// From char_mpc_.pl1, assuming MTP501
//
//      mtc500_char init (0000000011100000b),                  /* Mtc500 characteristics table at 00E0 (hex) */

        case 06:               // CMD 06 -- initiate read data transfer
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: initiate read data transfer\n", __func__);

            bool ptro, send, uff;
            int rc = iom_list_service (iomUnitIdx, chan, & ptro, & send, & uff);
            if (rc < 0)
              {
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                sim_warn ("%s list service failed\n", __func__);
                return IOM_CMD_ERROR;
              }
            if (uff)
              {
                sim_warn ("%s ignoring uff\n", __func__); // XXX
              }
            if (! send)
              {
                sim_warn ("%s nothing to send\n", __func__);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                return IOM_CMD_IGNORED;
              }
            if (p -> DCW_18_20_CP == 07 || p -> DDCW_22_23_TYPE == 2)
              {
                sim_warn ("%s expected DDCW\n", __func__);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                return IOM_CMD_ERROR;
              }
//sim_printf ("ddcw %012"PRIo64"\n", p -> DCW);
//sim_printf (" addr %06o tally %06o\n", p -> DDCW_ADDR, p -> DDCW_TALLY);
            uint tally = p -> DDCW_TALLY;
            if (tally != 04000)
              {
                sim_warn ("tape controller read memory expected tally of 04000\n");
                p -> stati = 04501;
                p -> chanStatus = chanStatIncorrectDCW;
                break;
              }
            uint16 mem [04000 * 2];
            memset (mem, 0, sizeof (mem));

            const uint charTableOS = 0xE0; // Mtc500 characteristics table at 00E0 (hex)
// dcl  1 tape_char based (char_ptr) unaligned,
//  0     2 mem_sze bit (16),                                  /* Read/write memory size */
//  1     2 config_sw bit (16),                                /* Configuration switch settings */
//  2     2 trace_tab_p bit (16),                              /* Trace table begin ptr */
//  3     2 trace_tab_size bit (16),                           /* Trace table size */
//  4     2 trace_tab_cur bit (16),                            /* Trace table current entry ptr */
//  5     2 mpc_stat bit (16),                                 /* Mpc statistics table pointer */
//  6     2 dev_stat bit (16),                                 /* Device statistics table pointer */
//  7     2 rev_l_tab bit (16),                                /* Revision level table? */
//  8     2 fw_id bit (16),                                    /* Firmware identifacation */
//  9     2 fw_rev,                                            /* Firmware revision */
//          3 pad1 bit (4),
//          3 lrev (2) bit (4),                                /* Letter revision */
//          3 srev bit (4),                                    /* Sub revision */
// 10     2 as_date,                                           /* Assembly date */
//          3 month bit (8),
//          3 day bit (8),
// 11     2 pad2 (5) bit (16);

// For the 501
//                    mpc_data.mpc_err_int_ctr_addr = 253;    /* 00FD */
//                    mpc_data.mpc_err_data_reg_addr = 254;   /* 00FE */

            mem [charTableOS + 0] = 4096; // mem_sze
            mem [charTableOS + 1] = 0; // config_sw 

// Set the addresses to recognizable values
            mem [charTableOS + 2] = 04000 + 0123; // trace_tab_p
            mem [charTableOS + 3] = 0; // trace_tab_size
            mem [charTableOS + 4] = 0; // trace_tab_cur
            mem [charTableOS + 5] = 04000 + 0234; // mpc_stat
            mem [charTableOS + 6] = 04000 + 0345; // dev_stat
            mem [charTableOS + 7] = 04000 + 0456; // rev_l_tab
            mem [charTableOS + 8] = 012345; // fw_id

            // Set fw_rev to 0013; I thinks that will xlate as 'A3' (0x01 is A)

            mem [charTableOS + 9] = 0x0013; // fw_rev

            mem [charTableOS + 10] = 0x1025; // as_date Oct 27.

            word36 buf [tally];
            for (uint i = 0; i < tally; i ++)
              {
                putbits36_18 (buf + i,  0, mem [i * 2]);
                putbits36_18 (buf + i, 18, mem [i * 2 + 1]);
              }
            iom_indirect_data_service (iomUnitIdx, chan, buf, & tally, true);
            p -> stati = 04000;
            p -> initiate = false; 
          }
          break;


        case 013: // CMD 013 -- Write tape 9
        case 015: // CMD 015 -- Write Binary Record
          {
            int rc = mtWriteRecord (devUnitIdx, iomUnitIdx, chan);
            if (rc)
              return IOM_CMD_ERROR;
          }
          break;

        case 016:               // CMD 016 -- initiate write data transfer
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: initiate write data transfer\n", __func__);

            bool ptro, send, uff;
            int rc = iom_list_service (iomUnitIdx, chan, & ptro, & send, & uff);
            if (rc < 0)
              {
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                sim_warn ("%s list service failed\n", __func__);
                return IOM_CMD_ERROR;
              }
            if (uff)
              {
                sim_warn ("%s ignoring uff\n", __func__); // XXX
              }
            if (! send)
              {
                sim_warn ("%s nothing to send\n", __func__);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                return IOM_CMD_IGNORED;
              }
            if (p -> DCW_18_20_CP == 07 || p -> DDCW_22_23_TYPE == 2)
              {
                sim_warn ("%s expected DDCW\n", __func__);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                return IOM_CMD_ERROR;
              }
//sim_printf ("ddcw %012"PRIo64"\n", p -> DCW);
//sim_printf (" addr %06o tally %06o\n", p -> DDCW_ADDR, p -> DDCW_TALLY);
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: initiate write data transfer tally %d\n", __func__, p -> DDCW_TALLY);
          }
          break;


        case 020: // CMD 020 -- release controller
          {
            if (p -> IDCW_CHAN_CMD == 040) // If special controller command, then command 020 is 'release'
              {
                sim_debug (DBG_DEBUG, & tape_dev,
                           "controller release\n");
                send_special_interrupt (iomUnitIdx, chan, p->IDCW_DEV_CODE, 02, 0 /* released */);
                p -> stati = 04000; // have_status = 1
              }
            else
              {
                p -> stati = 04501;
                p -> chanStatus = chanStatIncorrectDCW;
                sim_warn ("%s: Unknown command 0%o\n", __func__, p -> IDCW_DEV_CMD);
              }
//sim_printf ("tape req status chan_cmd %o\n", p -> IDCW_CHAN_CMD);
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Relese: %04o\n", __func__, p -> stati);
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Relese control: %o\n", __func__, p -> IDCW_CONTROL);
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Relese channel command: %o\n", __func__, p -> IDCW_CHAN_CMD);
          }
          break;

// 026 read control registers

// 032: MTP write main memory (binary) (poll_mpc.pl1)

        case 032: // CMD 032 -- MTP write main memory (binary)
                  //    (poll_mpc.pl1); used to clear device stats.
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Write controller main memory\n", __func__);

            bool ptro, send, uff;
            int rc = iom_list_service (iomUnitIdx, chan, & ptro, & send, & uff);
            if (rc < 0)
              {
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                sim_warn ("%s list service failed\n", __func__);
                return IOM_CMD_ERROR;
              }
            if (uff)
              {
                sim_warn ("%s ignoring uff\n", __func__); // XXX
              }
            if (! send)
              {
                sim_warn ("%s nothing to send\n", __func__);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                return IOM_CMD_IGNORED;
              }
            if (p -> DCW_18_20_CP == 07 || p -> DDCW_22_23_TYPE == 2)
              {
                sim_warn ("%s expected DDCW\n", __func__);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                return IOM_CMD_ERROR;
              }

//sim_printf ("chan mode %d\n", p -> chanMode);
//sim_printf ("ddcw %012"PRIo64"\n", p -> DCW);
            word36 control;
	    uint count;
            iom_indirect_data_service (iomUnitIdx, chan, & control, &count, false);
//sim_printf ("control %012"PRIo64"\n", control);
//sim_printf ("  addr %012"PRIo64" tally %012"PRIo64"\n", getbits36_16 (control, 0), getbits36_16 (control, 16));
            if (count != 1)
              sim_warn ("%s: count %d not 1\n", __func__, count);
            tape_statep -> cntlrAddress = getbits36_16 (control, 0);
            tape_statep -> cntlrTally = getbits36_16 (control, 16);
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Write controller main memory address %o\n", __func__,
                       tape_statep -> cntlrAddress);
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Write controller main memory tally %d\n", __func__,
                       tape_statep -> cntlrTally);

            p -> stati = 04000;
          }

        case 040:               // CMD 040 -- Reset Status
          {
            p -> stati = 04000;
            p -> initiate = false;
            p -> isRead = false;
            if (dev_code)
              {
                if (sim_tape_wrp (unitp))
                  p -> stati |= 1;
                if (sim_tape_bot (unitp))
                  p -> stati |= 2;
                //if (sim_tape_eom (unitp))
                  //p -> stati |= 0340;
              }
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Reset status is %04o.\n",
                       __func__, p -> stati);
          }
          break;

        case 041:              // CMD 041 -- Set 6250 cpi.
          {
            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            //if (sim_tape_eom (unitp))
              //p -> stati |= 0340;
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Set 800 bpi\n", __func__);
          }
          break;

        case 044: // 044 -- Forward skip Record
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Forward Skip Record\n");
            uint tally = p -> IDCW_COUNT;
            if (tally == 0)
              {
                sim_debug (DBG_DEBUG, & tape_dev,
                           "%s: Tally of zero interpreted as 64\n",
                           __func__);
                tally = 64;
              }

            sim_debug (DBG_DEBUG, & tape_dev, 
                       "mt_iom_cmd: Forward skip record tally %d\n", tally);

// sim_tape_sprecsf incorrectly stops on tape marks; 
#if 0
            uint32 skipped;
            t_stat ret = sim_tape_sprecsf (unitp, tally, & skipped);
#else
            uint32 skipped = 0;
            t_stat ret = MTSE_OK;
            while (skipped < tally)
              {
                ret = sim_tape_sprecf (unitp, & tape_statep -> tbc);
                if (ret != MTSE_OK && ret != MTSE_TMK)
                  break;
                skipped = skipped + 1;
              }
#endif
            if (ret != MTSE_OK && ret != MTSE_TMK && ret != MTSE_EOM)
              {
                 break;
              }
            if (skipped != tally)
              {
                sim_warn ("skipped %d != tally %d\n", skipped, tally);
              }

            tape_statep -> rec_num += (int) skipped;
            if (unitp->flags & UNIT_WATCH)
              sim_printf ("Tape %ld forward skips to record %d\n",
                          (long) MT_UNIT_NUM (unitp), tape_statep -> rec_num);

            p -> tallyResidue = (word12) (tally - skipped);

            sim_debug (DBG_NOTIFY, & tape_dev, 
                       "mt_iom_cmd: Forward space %d records\n", skipped);

            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            //if (sim_tape_eom (unitp))
              //p -> stati |= 0340;
          }
          break;

        case 045: // CMD 045 -- Forward Skip File
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Forward Skip File\n");
            uint tally = 1;

            if (tally != 1)
              {
                sim_debug (DBG_DEBUG, & tape_dev,
                           "%s: Forward space file: setting tally %d to 1\n",
                           __func__, tally);
                tally = 1;
              }

            sim_debug (DBG_DEBUG, & tape_dev, 
                       "mt_iom_cmd: Forward space file tally %d\n", tally);

            uint32 skipped, recsskipped;
            t_stat ret = sim_tape_spfilebyrecf (unitp, tally, & skipped, & recsskipped, false);
            if (ret != MTSE_OK && ret != MTSE_TMK && ret != MTSE_LEOT)
              {
                sim_warn ("sim_tape_spfilebyrecf returned %d\n", ret);
                 break;
              }
            if (skipped != tally)
              {
                sim_warn ("skipped %d != tally %d\n", skipped, tally);
              }

            tape_statep -> rec_num += (int) recsskipped;
            if (unitp->flags & UNIT_WATCH)
              sim_printf ("Tape %ld forward skips to record %d\n",
                          (long) MT_UNIT_NUM (unitp), tape_statep -> rec_num);

            p -> tallyResidue = (word12) (tally - skipped);
            sim_debug (DBG_NOTIFY, & tape_dev, 
                       "mt_iom_cmd: Forward space %d files\n", tally);

            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            //if (sim_tape_eom (unitp))
              //p -> stati |= 0340;
          }
          break;

        case 046: // CMD 046 -- Backspace Record
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Backspace Record\n");

            uint tally = p -> IDCW_COUNT;

            if (tally == 0)
              {
                sim_debug (DBG_DEBUG, & tape_dev,
                           "%s: Tally of zero interpreted as 64\n",
                           __func__);
                tally = 64;
              }

            sim_debug (DBG_DEBUG, & tape_dev, 
                       "mt_iom_cmd: Backspace record tally %d\n", tally);

#if 0
            int nbs = 0;

            while (tally)
              {
                t_stat ret = sim_tape_sprecr (unitp, & tape_statep -> tbc);
//sim_printf ("ret %d\n", ret);
                if (ret != MTSE_OK && ret != MTSE_TMK)
                  break;
                if (tape_statep -> rec_num > 0)
                  tape_statep -> rec_num --;
                nbs ++;
              }
#else
// sim_tape_sprecsr sumbles on tape marks; do our own version...
#if 0
            uint32 skipped;
            t_stat ret = sim_tape_sprecsr (unitp, tally, & skipped);
            if (ret != MTSE_OK && ret != MTSE_TMK)
              {
sim_printf ("sim_tape_sprecsr returned %d\n", ret);
                 break;
              }
#else
            uint32 skipped = 0;
            while (skipped < tally)
              {
                t_stat ret = sim_tape_sprecr (unitp, & tape_statep -> tbc);
                if (ret != MTSE_OK && ret != MTSE_TMK)
                  break;
                skipped ++;
              }
#endif
            if (skipped != tally)
              {
                sim_warn ("skipped %d != tally %d\n", skipped, tally);
              }
            tape_statep -> rec_num -= (int) skipped;
            if (unitp->flags & UNIT_WATCH)
              sim_printf ("Tape %ld skip back to record %d\n",
                          (long) MT_UNIT_NUM (unitp), tape_statep -> rec_num);

            p -> tallyResidue = (word12) (tally - skipped);

            sim_debug (DBG_NOTIFY, & tape_dev, 
                       "mt_iom_cmd: Backspace %d records\n", skipped);
#endif

            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            //if (sim_tape_eom (unitp))
              //p -> stati |= 0340;
          }
          break;

        case 047: // CMD 047 -- Backspace File
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Backspace File\n");
            uint tally = 1;

            if (tally != 1)
              {
                sim_debug (DBG_DEBUG, & tape_dev,
                           "%s: Back space file: setting tally %d to 1\n",
                           __func__, tally);
                tally = 1;
              }

            sim_debug (DBG_DEBUG, & tape_dev, 
                       "mt_iom_cmd: Backspace file tally %d\n", tally);

#if 0
            int nbs = 0;

            while (tally)
              {
                t_stat ret = sim_tape_sprecr (unitp, & tape_statep -> tbc);
//sim_printf ("ret %d\n", ret);
                if (ret != MTSE_OK && ret != MTSE_TMK)
                  break;
                if (tape_statep -> rec_num > 0)
                  tape_statep -> rec_num --;
                nbs ++;
              }
#else
            uint32 skipped, recsskipped;
            t_stat ret = sim_tape_spfilebyrecr (unitp, tally, & skipped, & recsskipped);
            if (ret != MTSE_OK && ret != MTSE_TMK && ret != MTSE_BOT)
              {
                sim_warn ("sim_tape_spfilebyrecr returned %d\n", ret);
                 break;
              }
            if (skipped != tally)
              {
                sim_warn ("skipped %d != tally %d\n", skipped, tally);
              }

            tape_statep -> rec_num -= (int) recsskipped;
            if (unitp->flags & UNIT_WATCH)
              sim_printf ("Tape %ld backward skips to record %d\n",
                          (long) MT_UNIT_NUM (unitp), tape_statep -> rec_num);

            p -> tallyResidue = (word12) (tally - skipped);
            sim_debug (DBG_NOTIFY, & tape_dev, 
                       "mt_iom_cmd: Backspace %d records\n", tally);
#endif

            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            //if (sim_tape_eom (unitp))
              //p -> stati |= 0340;
          }
          break;

        case 050:               // CMD 050 -- Request device status
          {
            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            //if (sim_tape_eom (unitp))
              //p -> stati |= 0340;
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Request device status: %o\n", __func__, p -> stati);
          }
          break;

// Okay, this is convoluted. Multics locates the console by sending CMD 051
// to devices in the PCW, with the understanding that only the console 
// device will "respond", whatever that means.
// But, bootload_tape_label checks for controller firmware loaded
// ("intellegence") by sending a 051 in a IDCW.
// Since it's diffcult here to test for PCW/IDCW, assume that the PCW case
// has been filtered out at a higher level
        case 051:               // CMD 051 -- Reset device status
          {
            if (p->isPCW)
              {
                p->stati = 04501; // cmd reject, invalid opcode
                p->chanStatus = chanStatIncorrectDCW;
                return IOM_CMD_ERROR;
              }
             p->stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            //if (sim_tape_eom (unitp))
              //p -> stati |= 0340;
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Reset device status: %o\n", __func__, p -> stati);
          }
          break;

// 054 erase

        case 055: // CMD 055 -- Write EOF (tape mark);
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Write tape mark\n");

            int ret;
            if (! (unitp -> flags & UNIT_ATT))
              ret = MTSE_UNATT;
            else
              {
                ret = sim_tape_wrtmk (unitp);
                sim_debug (DBG_DEBUG, & tape_dev, 
                           "sim_tape_wrtmk returned %d\n", ret);
                if (unitp->io_flush)
                  unitp->io_flush (unitp);                              /* flush buffered data */
              }
            if (ret != 0)
              {
                if (ret == MTSE_EOM)
                  {
                    sim_debug (DBG_NOTIFY, & tape_dev,
                                "%s: EOM: %s\n", __func__, simh_tape_msg (ret));
                    p -> stati = 04340; // EOT file mark
                    sim_warn ("%s: Wrote tape mark with EOM.\n", 
                               __func__);
                    break;
                  }
                sim_warn ("%s: Cannot write tape mark: %d - %s\n",
                           __func__, ret, simh_tape_msg(ret));
                sim_warn ("%s: Returning arbitrary error code\n",
                           __func__);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatParityErrPeriph;
                break;
              }

            sim_tape_wreom (unitp);
            if (unitp->io_flush)
              unitp->io_flush (unitp);                              /* flush buffered data */

            tape_statep -> rec_num ++;
            if (unitp->flags & UNIT_WATCH)
              sim_printf ("Tape %ld writes tape mark %d\n",
                          (long) MT_UNIT_NUM (unitp), tape_statep -> rec_num);

            p -> stati = 04000; 
            if (sim_tape_eot (unitp))
              p -> stati = 04340;

            sim_debug (DBG_INFO, & tape_dev,
                       "%s: Wrote tape mark; status %04o\n",
                       __func__, p -> stati);
          }
          break;

        case 057:               // CMD 057 -- Survey devices
          {
            surveyDevices (iomUnitIdx, chan);
          }
          break;

        case 060:              // CMD 060 -- Set 800 bpi.
          {
            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            //if (sim_tape_eom (unitp))
              //p -> stati |= 0340;
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Set 800 bpi\n", __func__);
          }
          break;

        case 061:              // CMD 061 -- Set 556 bpi.
          {
            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            //if (sim_tape_eom (unitp))
              //p -> stati |= 0340;
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Set 556 bpi\n", __func__);
          }
          break;

// 062 reserve device

        case 063:              // CMD 063 -- Set File Permit.
          {
            sim_debug (DBG_WARN, & tape_dev, "Set file permit?\n");
            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
          }
          break;

// 064 Set file protect

        case 064:              // CMD 064 -- Set 200 bpi.
          {
            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            //if (sim_tape_eom (unitp))
              //p -> stati |= 0340;
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Set 200 bpi\n", __func__);
          }
          break;

        case 065:              // CMD 064 -- Set 1600 CPI
          {
            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            //if (sim_tape_eom (unitp))
              //p -> stati |= 0340;
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Set 1600 CPI\n", __func__);
          }
          break;

// 066 release device

        case 070:              // CMD 070 -- Rewind.
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Rewind\n", __func__);
            sim_tape_rewind (unitp);

            tape_statep -> rec_num = 0;
            if (unitp->flags & UNIT_WATCH)
              sim_printf ("Tape %ld rewinds\n", (long) MT_UNIT_NUM (unitp));

            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            //if (sim_tape_eom (unitp))
              //p -> stati |= 0340;
            //rewindDoneUnit . u3 = mt_unit_num;
            //sim_activate (& rewindDoneUnit, 4000000); // 4M ~= 1 sec
            send_special_interrupt (iomUnitIdx, chan, dev_code, 0, 0100 /* rewind complete */);
          }
          break;
   
        case 072:              // CMD 072 -- Rewind/Unload.
          {
            if (unitp->flags & UNIT_WATCH)
              sim_printf ("Tape %ld unloads\n",
                          (long) MT_UNIT_NUM (unitp));
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Rewind/unload\n", __func__);
            sim_tape_detach (unitp);
            //tape_statep -> rec_num = 0;
            p -> stati = 04000;
            send_special_interrupt (iomUnitIdx, chan, dev_code, 0, 0040 /* unload complete */);
          }
          break;

// 073 data security erase
// 075  tape load
// 077 set density

        default:
          {
            p -> stati = 04501;
            p -> chanStatus = chanStatIncorrectDCW;
            if (p->IDCW_DEV_CMD != 051) // ignore bootload console probe
              sim_warn ("mt daze %o\n", p->IDCW_DEV_CMD);
          }
          return IOM_CMD_ERROR;

      } // IDCW_DEV_CMD

    sim_debug (DBG_DEBUG, & tape_dev, "stati %04o\n", p -> stati);

    if (p -> IDCW_CONTROL == 3) // marker bit set
      {
        send_marker_interrupt (iomUnitIdx, (int) chan);
      }
    return IOM_CMD_OK;
  }

// 031 read statistics
//  idcw.chan_cmd = "41"b3;  /* Indicate special controller command */
// 006 initiate read data transfer
// 032 write main memory (binary)
// 016 initiate write data transfer
// 000 suspend controller
// 020 release controller



/*
 * mt_iom_cmd()
 *
 */

// 1 ignored command
// 0 ok
// -1 problem
int mt_iom_cmd (uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];

// Is it an IDCW?

    if (p -> DCW_18_20_CP != 7)
      {
        sim_warn ("%s expected IDCW\n", __func__);
        return IOM_CMD_ERROR;
      }
    return mt_cmd (iomUnitIdx, chan);
  }


static const char *simh_tape_msg(int code)
{
    // WARNING: Only selected SIMH tape routines return private tape codes
    // WARNING: returns static buf
    // BUG: Is using a string constant equivalent to using a static buffer?
    // static char msg[80];
    if (code == MTSE_OK)
        return "OK";
    else if (code == MTSE_UNATT)
        return "Unit not attached to a file";
    else if (code == MTSE_FMT)
        return "Unit specifies an unsupported tape file format";
    else if (code == MTSE_IOERR)
        return "Host OS I/O error";
    else if (code == MTSE_INVRL)
        return "Invalid record length (exceeds maximum allowed)";
    else if (code == MTSE_RECE)
        return "Record header contains error flag";
    else if (code == MTSE_TMK)
        return "Tape mark encountered";
    else if (code == MTSE_BOT)
        return "BOT encountered during reverse operation";
    else if (code == MTSE_EOM)
        return "End of Medium encountered";
    else if (code == MTSE_WRP)
        return "Write protected unit during write operation";
    else
        return "Unknown SIMH tape error";
  }

t_stat attachTape (char * label, bool withring, char * drive)
  {
    //sim_printf ("%s %s %s\n", label, withring ? "rw" : "ro", drive);
    int i;
    for (i = 0; i < N_MT_UNITS_MAX; i ++)
      {
        if (strcmp (drive, tape_states [i] . device_name) == 0)
          break;
      }
    if (i >= N_MT_UNITS_MAX)
      {
        sim_printf ("can't find device named %s\n", drive);
        return SCPE_ARG;
      }
    sim_printf ("attachTape selected unit %d\n", i);
    loadTape ((uint) i, label, ! withring);
    return SCPE_OK;
  }

// mount <image.tap> ring|noring <drive>

t_stat mount_tape (UNUSED int32 arg, const char * buf)
  {
    size_t bufl = strlen (buf) + 1;
    char fname [bufl];
    char ring [bufl];
    char drive [bufl];
    int nParams = sscanf (buf, "%s %s %s", fname, ring, drive);
    if (nParams != 3)
      goto usage;
    size_t ringl = strlen (ring);
    bool withring;
    if (strncasecmp ("noring", ring, ringl) == 0)
      withring = false;
    else if (strncasecmp ("ring", ring, ringl) == 0)
      withring = true;
    else
      goto usage;
    return attachTape (fname, withring, drive);

usage:
     sim_printf ("mount <image.tap> ring|noring <drive>\n");
     return SCPE_ARG;
  }

t_stat detachTape (char * drive)
  {
    //sim_printf ("%s %s %s\n", label, withring ? "rw" : "ro", drive);
    int i;
    for (i = 0; i < N_MT_UNITS_MAX; i ++)
      {
        if (strcmp (drive, tape_states [i] . device_name) == 0)
          break;
      }
    if (i >= N_MT_UNITS_MAX)
      {
        sim_printf ("can't find device named %s\n", drive);
        return SCPE_ARG;
      }
    unloadTape ((uint) i);
    return SCPE_OK;
  }

