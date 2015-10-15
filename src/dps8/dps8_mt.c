//#define IOMDBG1
/**
 * \file dps8_mt.c
 * \project dps8
 * \date 9/17/12
 * \copyright Copyright (c) 2012 Harry Reed. All rights reserved.
*/

#include <stdio.h>
#include "dps8.h"
#include "dps8_mt.h"
#include "dps8_sys.h"
#include "dps8_utils.h"
#include "dps8_cpu.h"
#include "dps8_iom.h"
#include "dps8_cable.h"

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

#include "dps8_mt.h"

static t_stat mt_rewind (UNIT * uptr, int32 value, char * cptr, void * desc);
static t_stat mt_show_nunits (FILE *st, UNIT *uptr, int val, void *desc);
static t_stat mt_set_nunits (UNIT * uptr, int32 value, char * cptr, void * desc);
static t_stat mt_show_boot_drive (FILE *st, UNIT *uptr, int val, void *desc);
static t_stat mt_set_boot_drive (UNIT * uptr, int32 value, char * cptr, void * desc);
static t_stat mt_show_device_name (FILE *st, UNIT *uptr, int val, void *desc);
static t_stat mt_set_device_name (UNIT * uptr, int32 value, char * cptr, void * desc);
static t_stat mt_show_tape_path (FILE *st, UNIT *uptr, int val, void *desc);
static t_stat mt_set_tape_path (UNIT * uptr, int32 value, char * cptr, void * desc);
//static int mt_iom_io (UNIT * unitp, uint chan, uint dev_code, uint * tally, uint * cp, word36 * wordp, word12 * stati);

#define N_MT_UNITS 1 // default

static t_stat mt_svc (UNIT *up);

UNIT mt_unit [N_MT_UNITS_MAX] = {
    // NOTE: other SIMH tape sims don't set UNIT_SEQ
    // CAC: Looking at SIMH source, the only place UNIT_SEQ is used
    // by the "run" command's reset sequence; units that have UNIT_SEQ
    // set will be issued a rewind on reset.
    // Looking at the sim source again... It is used on several of the
    // run commands, including CONTINUE.
    // Turning UNIT_SEQ off.
    // XXX Should we rewind on reset? What is the actual behavior?
// Unit 0 is the controller
    {UDATA (& mt_svc,                /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& mt_svc, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& mt_svc, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& mt_svc, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& mt_svc, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& mt_svc, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& mt_svc, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& mt_svc, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& mt_svc, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& mt_svc, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& mt_svc, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& mt_svc, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& mt_svc, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& mt_svc, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& mt_svc, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& mt_svc, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& mt_svc, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL}
};

static DEBTAB mt_dt [] =
  {
    { "NOTIFY", DBG_NOTIFY },
    { "INFO", DBG_INFO },
    { "ERR", DBG_ERR },
    { "WARN", DBG_WARN },
    { "DEBUG", DBG_DEBUG },
    { "ALL", DBG_ALL }, // don't move as it messes up DBG message
    { NULL, 0 }
  };

#define UNIT_WATCH (1 << MTUF_V_UF)

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
      MTAB_XTD | MTAB_VUN | MTAB_VALR, /* mask */
      0,            /* match */
      "BOOT_DRIVE",     /* print string */
      "BOOT_DRIVE",         /* match string */
      mt_set_boot_drive, /* validation routine */
      mt_show_boot_drive, /* display routine */
      "Select the boot drive", /* value descriptor */
      NULL          // help
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_VALR | MTAB_NC, /* mask */
      0,            /* match */
      "DEVICE_NAME",     /* print string */
      "DEVICE_NAME",         /* match string */
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
    { 0, 0, NULL, NULL, NULL, NULL, NULL, NULL }
  };

#define MT_UNIT_NUM(uptr) ((uptr) - mt_unit)

static t_stat mt_reset (DEVICE * dptr);

DEVICE tape_dev = {
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
};

#define MAX_DEV_NAME_LEN 64

//-- /* unfinished; copied from tape_dev */
static const char * simh_tape_msg (int code); // hack
static const size_t bufsz = 4096 * 9 / 2;
static struct tape_state
  {
    enum { no_mode, read_mode, write_mode, survey_mode } io_mode;
    bool is9;
    uint8 buf [bufsz];
    t_mtrlnt tbc; // Number of bytes read into buffer
    uint words_processed; // Number of Word36 processed from the buffer
// XXX bug: 'sim> set tapeN rewind' doesn't reset rec_num
    int rec_num; // track tape position
    char device_name [MAX_DEV_NAME_LEN];
  } tape_states [N_MT_UNITS_MAX];

// XXX this assumes only one controller, needs to be indexed
static int boot_drive = 1; // Drive number to boot from
#define TAPE_PATH_LEN 4096
static char tape_path [TAPE_PATH_LEN];

#if 0
t_stat rewindDone (UNIT * uptr)
  {
    int32 driveNumber = uptr -> u3;
    send_special_interrupt (cables -> cablesFromIomToTap [driveNumber] . iomUnitIdx,
                            cables -> cablesFromIomToTap [driveNumber] . chan_num,
                            cables -> cablesFromIomToTap [driveNumber] . dev_code,
                            0, 0100 /* rewind complete */);
    return SCPE_OK;
  }
#endif

#if 0
static UNIT rewindDoneUnit =
  { UDATA (& rewindDone, 0, 0), 0, 0, 0, 0, 0, NULL, NULL };
#endif

static int findTapeUnit (int iomUnitIdx, int chan_num, int dev_code)
  {
    for (int i = 0; i < N_MT_UNITS_MAX; i ++)
      {
        if (iomUnitIdx == cables -> cablesFromIomToTap [i] . iomUnitIdx &&
            chan_num     == cables -> cablesFromIomToTap [i] . chan_num     &&
            dev_code     == cables -> cablesFromIomToTap [i] . dev_code)
          return i;
      }
    return -1;
  }

#if 0
UNIT * getTapeUnit (uint driveNumber)
  {
    return mt_unit + driveNumber;
  }

void tape_send_special_interrupt (uint driveNumber)
  {
    send_special_interrupt (cables -> cablesFromIomToTap [driveNumber] . iomUnitIdx,
                            cables -> cablesFromIomToTap [driveNumber] . chan_num);
  }
#endif

void loadTape (uint driveNumber, char * tapeFilename, bool ro)
  {
    t_stat stat = sim_tape_attach (& mt_unit [driveNumber], tapeFilename);
    if (stat != SCPE_OK)
      {
        sim_printf ("loadTape sim_tape_attach returned %d\n", stat);
        return;
      }
    if (ro)
      mt_unit [driveNumber] . flags |= MTUF_WRP;
    else
      mt_unit [driveNumber] . flags &= ~ MTUF_WRP;
    send_special_interrupt (cables -> cablesFromIomToTap [driveNumber] . iomUnitIdx,
                            cables -> cablesFromIomToTap [driveNumber] . chan_num,
                            cables -> cablesFromIomToTap [driveNumber] . dev_code,
                            0, 020 /* tape drive to ready */);
  }

void unloadTape (uint driveNumber)
  {
    if (mt_unit [driveNumber] . flags & UNIT_ATT)
      {
        t_stat stat = sim_tape_detach (& mt_unit [driveNumber]);
        if (stat != SCPE_OK)
          {
            sim_printf ("unloadTape sim_tape_detach returned %d\n", stat);
            return;
          }
      }
    send_special_interrupt (cables -> cablesFromIomToTap [driveNumber] . iomUnitIdx,
                            cables -> cablesFromIomToTap [driveNumber] . chan_num,
                            cables -> cablesFromIomToTap [driveNumber] . dev_code,
                            0, 040 /* unload complere */);
  }

void mt_init(void)
  {
    memset(tape_states, 0, sizeof(tape_states));
    for (int i = 0; i < N_MT_UNITS_MAX; i ++)
      {
        mt_unit [i] . capac = 40000000;
      }
    boot_drive = 1;
  }

static t_stat mt_reset (DEVICE * dptr)
  {
    for (int i = 0; i < (int) dptr -> numunits; i ++)
      {
        sim_tape_reset (& mt_unit [i]);
        sim_cancel (& mt_unit [i]);
      }
    return SCPE_OK;
  }

static void mtReadRecord (uint iomUnitIdx, uint chan)
  {


// If a tape read IDCW has multiple DDCWs, are additional records read?

    iomChanData_t * p = & iomChanData [iomUnitIdx] [chan];
    struct device * d = & cables -> cablesFromIomToDev [iomUnitIdx] .
                      devices [chan] [p -> IDCW_DEV_CODE];
    uint devUnitIdx = d -> devUnitIdx;
    UNIT * unitp = & mt_unit [devUnitIdx];
    struct tape_state * tape_statep = & tape_states [devUnitIdx];

    enum { dataOK, noTape, tapeMark, tapeEOM } tapeStatus;

    tape_statep -> is9 = p -> IDCW_DEV_CMD == 003;
    sim_debug (DBG_DEBUG, & tape_dev,
               tape_statep -> is9 ?" mt_cmd: Read 9 record\n" : "mt_cmd: Read binary record\n");
    // We read the record into the tape controllers memory;
    // IOM will subsequently retrieve the data via DCWs.
    tape_statep -> tbc = 0;
    if (! (unitp -> flags & UNIT_ATT))
      {
        tapeStatus = noTape;
        goto ddcws;
      }
    int rc = sim_tape_rdrecf (unitp, & tape_statep -> buf [0], & tape_statep -> tbc,
                               bufsz);
    sim_debug (DBG_DEBUG, & tape_dev, "sim_tape_rdrecf returned %d, with tbc %d\n", rc, tape_statep -> tbc);
    if (rc == MTSE_TMK)
       {
         tape_statep -> rec_num ++;
         sim_debug (DBG_NOTIFY, & tape_dev,
                    "%s: EOF: %s\n", __func__, simh_tape_msg (rc));
        p -> stati = 04423; // EOF category EOF file mark
        if (tape_statep -> tbc != 0)
          {
            sim_debug (DBG_ERR, &tape_dev,
                       "%s: Read %d bytes with EOF.\n", 
                       __func__, tape_statep -> tbc);
            return;
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
            sim_debug (DBG_ERR, &tape_dev,
                       "%s: Read %d bytes with EOM.\n", 
                       __func__, tape_statep -> tbc);
            return;
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
        return;
      }
    tape_statep -> rec_num ++;
    tapeStatus = dataOK;

ddcws:;

    tape_statep -> words_processed = 0;
    if (unitp->flags & UNIT_WATCH)
      sim_printf ("Tape %ld reads record %d\n",
                  MT_UNIT_NUM (unitp), tape_statep -> rec_num);
    p -> initiate = true;
    tape_statep -> io_mode = read_mode;


// Process DDCWs

    bool ptro;
    bool send;
    bool uff;
    do
      {
        int rc = iomListService (iomUnitIdx, chan, & ptro, & send, & uff);
        if (rc < 0)
          {
            sim_printf ("mtReadRecord list service failed\n");
            return /* -1 */;
          }
        if (uff)
          {
            sim_printf ("mtReadRecord ignoring uff\n"); // XXX
          }
        if (! send)
          {
            sim_printf ("mtReadRecord nothing to send\n");
            return /* 1 */;
          }
        if (p -> DCW_18_20_CP == 07 || p -> DDCW_22_23_TYPE == 2)
          {
            sim_printf ("mtReadRecord expected DDCW\n");
            return /* -1 */;
          }


        uint tally = p -> DDCW_TALLY;
        uint daddr = p -> DDCW_ADDR;
        if (tally == 0)
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Tally of zero interpreted as 010000(4096)\n",
                       __func__);
            tally = 4096;
          }

        sim_debug (DBG_DEBUG, & tape_dev,
                   "%s: Tally %d (%o)\n", __func__, tally, tally);
        if (p -> PCW_63_PTP)
          {
            word36 buffer [tally];
            uint i;
            for (i = 0; i < tally; i ++)
              {
                int rc;
                if (tape_statep -> is9)
                  rc = extractASCII36FromBuffer (tape_statep -> buf, tape_statep -> tbc, & tape_statep -> words_processed, buffer + i);
                else
                  rc = extractWord36FromBuffer (tape_statep -> buf, tape_statep -> tbc, & tape_statep -> words_processed, buffer + i);
                if (rc)
                  {
                    // BUG: There isn't another word to be read from the tape buffer,
                    // but the IOM wants  another word.
                    // BUG: How did this tape hardware handle an attempt to read more
                    // data than was present?
                    // One answer is in bootload_tape_label.alm which seems to assume
                    // a 4000 all-clear status.
                    // Boot_tape_io.pl1 seems to assume that "short reads" into an
                    // over-large buffer should not yield any error return.
                    // So we'll set the flags to all-ok, but return an out-of-band
                    // non-zero status to make the iom stop.
                    // BUG: See some of the IOM status fields.
                    // BUG: The IOM should be updated to return its DCW tally residue
                    // to the caller.
                    p -> stati = 04000;
                    if (sim_tape_wrp (unitp))
                      p -> stati |= 1;
                    sim_debug (DBG_WARN, & tape_dev,
                               "%s: Read buffer exhausted on channel %d\n",
                               __func__, chan);
                    break;
                  }
              }
            p -> tallyResidue = tally - i;
if (tape_statep -> is9) sim_printf ("words %d %012llo %12llo\n", tape_statep -> words_processed, buffer [0], buffer [1]);
            indirectDataService (iomUnitIdx, chan, daddr, tape_statep -> words_processed, buffer,
                                 idsTypeW36, true, & p -> isOdd);
          }
        else
          {
            p -> tallyResidue = tally;
            while (tally)
              {
                // read
                word36 w;
                int rc;
                if (tape_statep -> is9)
                  rc = extractASCII36FromBuffer (tape_statep -> buf, tape_statep -> tbc, & tape_statep -> words_processed, & w);
                else
                  rc = extractWord36FromBuffer (tape_statep -> buf, tape_statep -> tbc, & tape_statep -> words_processed, & w);
                if (rc)
                  {
                    p -> stati = 04000;
                    if (sim_tape_wrp (unitp))
                      p -> stati |= 1;
                    sim_debug (DBG_WARN, & tape_dev,
                               "%s: Read buffer exhausted on channel %d\n",
                               __func__, chan);
                    break;
                  }
                core_write (daddr, w, __func__);
                p -> isOdd = daddr % 2;
                daddr ++;
                tally --;
                p -> tallyResidue --;
              }
          }
        p -> stati = 04000; // BUG: do we need to detect end-of-record?
        if (sim_tape_wrp (unitp))
          p -> stati |= 1;

        sim_debug (DBG_INFO, & tape_dev,
                   "%s: Read %d bytes from simulated tape; status %04o\n",
                   __func__, (int) tape_statep -> tbc, p -> stati);

if (p -> DDCW_22_23_TYPE != 0)
  sim_printf ("curious... a tape read with more than one DDCW?\n");

      }
    while (p -> DDCW_22_23_TYPE != 0); // while not IOTD
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
    iomChanData_t * p = & iomChanData [iomUnitIdx] [chan];

// The bootload read command does a read on drive 0; the controler
// recgnizes (somehow) a special case for bootload and subs. in
// the boot drive unit set by the controller config. switches
    if (p -> IDCW_DEV_CODE == 0 && p -> IDCW_DEV_CMD == 05)
        p -> IDCW_DEV_CODE = boot_drive;

    struct device * d = & cables -> cablesFromIomToDev [iomUnitIdx] .
                      devices [chan] [p -> IDCW_DEV_CODE];
    uint devUnitIdx = d -> devUnitIdx;
    UNIT * unitp = & mt_unit [devUnitIdx];
    struct tape_state * tape_statep = & tape_states [devUnitIdx];

    tape_statep -> io_mode = no_mode;
    switch (p -> IDCW_DEV_CMD)
      {
        case 0: // CMD 00 Request status
          {
            p -> stati = 04000; // have_status = 1
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            if (sim_tape_eot (unitp))
              p -> stati |= 0340;
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Request status: %04o\n", p -> stati);
          }
          break;

        case 3: // CMD 03 -- Read 9 Record
        case 5: // CMD 05 -- Read Binary Record
          {
            mtReadRecord (iomUnitIdx, chan);
          }
          break;

        case 040:               // CMD 040 -- Reset Status
          {
            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            if (sim_tape_eot (unitp))
              p -> stati |= 0340;
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Reset status is %04o.\n",
                       __func__, p -> stati);
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
            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            if (sim_tape_eot (unitp))
              p -> stati |= 0340;
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Reset device status: %o\n", p -> stati);
          }
          break;

        case 070:              // CMD 070 -- Rewind.
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Rewind\n");
            sim_tape_rewind (unitp);

            tape_statep -> rec_num = 0;
            if (unitp->flags & UNIT_WATCH)
              sim_printf ("Tape %ld rewinds\n", MT_UNIT_NUM (unitp));

            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            if (sim_tape_eot (unitp))
              p -> stati |= 0340;
            //rewindDoneUnit . u3 = mt_unit_num;
            //sim_activate (& rewindDoneUnit, 4000000); // 4M ~= 1 sec
            send_special_interrupt (cables -> cablesFromIomToTap [devUnitIdx] . iomUnitIdx,
                                    cables -> cablesFromIomToTap [devUnitIdx] . chan_num,
                                    cables -> cablesFromIomToTap [devUnitIdx] . dev_code,
                                    0, 0100 /* rewind complete */);

          }
          break;
   
        default:
          {
fail:
            p -> stati = 04501;
            sim_debug (DBG_ERR, & tape_dev,
                       "%s: Unknown command 0%o\n", __func__, p -> IDCW_DEV_CMD);
            p -> chanStatus = chanStatIncorrectDCW;
sim_printf ("%s: Unknown command 0%o\n", __func__, p -> IDCW_DEV_CMD);
            break;
          }
      } // IDCW_DEV_CMD
    return 0;
  }

#ifdef IOM2
static int xmt_cmd (UNIT * unitp, pcw_t * pcwp, bool * disc)
  {
    //if (pcwp -> control == 3) sim_printf ("XXXX marker\n");
    //if (pcwp -> control == 3) sim_printf ("XXXX marker\n");
    //if (pcwp -> control == 3) sim_printf ("XXXX marker\n");
    //if (pcwp -> control == 3) sim_printf ("XXXX marker\n");
    int mt_unit_num = MT_UNIT_NUM (unitp);
    int iomUnitIdx = cables -> cablesFromIomToTap [mt_unit_num] . iomUnitIdx;
    struct tape_state * tape_statep = & tape_states [mt_unit_num];
    * disc = false;

    int chan = pcwp-> chan;

    iomChannelData_ * p = & iomChannelData [iomUnitIdx] [chan];
    if (p -> ptp &&
        pcwp -> dev_cmd != 057 && // survey devices
        pcwp -> dev_cmd != 072 && // rewind/unload
        pcwp -> dev_cmd != 000 && // Request status
        pcwp -> dev_cmd != 060 && // set 800 bpi
        pcwp -> dev_cmd != 061 && // set 556 bpi
        pcwp -> dev_cmd != 064 && // set 200 bpi
        pcwp -> dev_cmd != 070 && // rewind
        pcwp -> dev_cmd != 005 && // read
        pcwp -> dev_cmd != 015 && // write
        pcwp -> dev_cmd != 055 && // write eof (tape mark)
        pcwp -> dev_cmd != 063 && // permit
        pcwp -> dev_cmd != 061 && // Set 556 BPI
        pcwp -> dev_cmd != 043 && // Set 556 BPI
        pcwp -> dev_cmd != 060 && // Set 800 BPI
        pcwp -> dev_cmd != 042 && // Set 800 BPI
        pcwp -> dev_cmd != 065 && // Set 1600 CPI
        pcwp -> dev_cmd != 013 && // Write tape 9
        pcwp -> dev_cmd != 044 && // Forward skip record
        pcwp -> dev_cmd != 047 && // Forward skip file
        pcwp -> dev_cmd != 003 && // Read tape 9

// According to poll_mpc.pl1
// Note: XXX should probably be checking these...
//  idcw.chan_cmd = "40"b3; /* Indicate special controller command */
//  idcw.chan_cmd = "41"b3; /* Indicate special controller command */


// EURC MPC
        pcwp -> dev_cmd != 002 && // read controller main memory (ASCII)
// 031 read statistics
//  idcw.chan_cmd = "41"b3;  /* Indicate special controller command */
// 006 initiate read data transfer
// 032 write main memory (binary)
// 016 initiate write data transfer
// 000 suspend controller
// 020 release controller


// DAU MSP
// 024 read configuration
// 016 read/clear statistics

        pcwp -> dev_cmd != 040)   // reset status
      {
        sim_printf ("PTP in mt; dev_cmd %o\n", pcwp -> dev_cmd);
        //sim_err ("PTP in mt\n");
        goto fail;
      }

    sim_debug (DBG_DEBUG, & tape_dev,
               "mt_cmd cmd %o unit %d\n",
               pcwp -> dev_cmd, mt_unit_num);
    p -> tallyResidue = 0;
    switch (pcwp -> dev_cmd)
      {
        case 0: // CMD 00 Request status
          {
            p -> stati = 04000; // have_status = 1
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            if (sim_tape_eot (unitp))
              p -> stati |= 0340;
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Request status: %04o\n", p -> stati);
          }
          break;

// Read controller main memory is used by the poll_mpc tool to track
// usage. if we just report illegal command, pool_mpc will give up.
// XXX ticket #46
#if 0
        case 02:               // CMD 02 -- Read controller main memory (ASCII)
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Read controller main memory\n");
            p -> stati = 04000; // have_status = 1
            //* need_data = true;

#if 0
sim_printf ("get the idcw\n");
            // Get the IDCW
            dcw_t dcw;
            int rc = iomListService (iomUnitIdx, chan, & dcw, NULL);

            if (rc)
              {
                sim_printf ("list service failed\n");
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                break;
              }
            if (dcw . type != idcw)
              {
                sim_printf ("not idcw? %d\n", dcw . type);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                break;
              }
#endif

#if 1
#ifdef IOMDBG1
sim_printf ("get the ddcw\n");
#endif
            // Get the DDCW
            dcw_t dcw;
            int rc = iomListService (iomUnitIdx, chan, & dcw, NULL);

            if (rc)
              {
                sim_printf ("list service failed\n");
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncomplete;
                break;
              }
            if (dcw . type != ddcw)
              {
                sim_printf ("not ddcw? %d\n", dcw . type);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncorrectDCW;
                break;
              }
#endif

            uint type = dcw.fields.ddcw.type;
            uint tally = dcw.fields.ddcw.tally;
            uint daddr = dcw.fields.ddcw.daddr;
            // uint cp = dcw.fields.ddcw.cp;
            if (pcwp -> mask)
              daddr |= ((pcwp -> ext) & MASK6) << 18;
if (type == 0) sim_printf ("IOTD\n");
if (type == 1) sim_printf ("IOTP\n");
            if (type == 0) // IOTD
              * disc = true;
            else if (type == 1) // IOTP
              * disc = false;
            else
              {
sim_printf ("uncomfortable with this\n");
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncorrectDCW;
                break;
              }


            sim_debug (DBG_DEBUG, & tape_dev,
                       "tally %04o daddr %06o\n", tally, daddr);
sim_printf ("tally %d\n", tally);            
#if 0
            if (tally != 8)
              {
                sim_debug (DBG_DEBUG, & tape_dev,
                           "%s: Expected tally of 8; got %d\n",
                           __func__, tally);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                break;
              }
#endif
#if 0
            word36 buffer [8];
            int cnt = 0;
            for (uint i = 0; i < 8; i ++)
              buffer [i] = 0;
            
            for (uint i = 1; i < N_MT_UNITS_MAX; i ++)
              {
                if (cables -> cablesFromIomToTap [i] . iomUnitIdx != -1)
                  {
                    word18 handler = 0;
// Test hack: unit 0 never operational
                    if (i != 0)
                      handler |= 0100000; // operational
                    //if (find_dev_from_unit (& mt_unit [i]))
                      //sim_printf ("Unit %d has dev\n", i);
                    if (mt_unit [i] . filename)
                      {
                        handler |= 0040000; // ready
                        //sim_printf ("Unit %d ready\n", i);
                      }
                    handler |= (cables -> cablesFromIomToTap [i] . dev_code & 037) << 9; // number
                    handler |= 0000040; // 200 ips
                    handler |= 0000020; // 9 track
                    handler |= 0000007; // 800/1600/6250
                    sim_debug (DBG_DEBUG, & tape_dev,
                               "unit %d handler %06o\n", i, handler);
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
              }
#ifdef IOMDBG1
iomChannelData_ * p = & iomChannelData [iomUnitIdx] [chan];
sim_printf ("chan_mode %d\n", p -> chan_mode);
#endif
            indirectDataService (iomUnitIdx, chan, daddr, 8, buffer,
                                 idsTypeW36, true, & p -> isOdd);
#endif
            p -> stati = 04000;
          }
          break;
#endif
 
        case 3: // CMD 03 -- Read 9 Record
        case 5: // CMD 05 -- Read Binary Record
          {
            tape_statep -> is9 = pcwp -> dev_cmd == 003;
            sim_debug (DBG_DEBUG, & tape_dev,
                       tape_statep -> is9 ?" mt_cmd: Read 9 record\n" : "mt_cmd: Read binary record\n");
            // We read the record into the tape controllers memory;
            // IOM can subsequently retrieve the data via DCWs.
            tape_statep -> tbc = 0;
            int ret;
            if (! (unitp -> flags & UNIT_ATT))
              ret = MTSE_UNATT;
            else
              {
                ret = sim_tape_rdrecf (unitp, tape_statep -> buf, & tape_statep -> tbc,
                                       bufsz);
                sim_debug (DBG_DEBUG, & tape_dev, "sim_tape_rdrecf returned %d, with tbc %d\n", ret, tape_statep -> tbc);
                // XXX put unit number in here...
if (tape_statep -> is9) sim_printf ("tbc %d <%s>\n", tape_statep -> tbc, tape_statep -> buf);
              }
            if (ret != 0)
              {
                * disc = true;
                if (ret == MTSE_TMK)
                  {
                    tape_statep -> rec_num ++;
                    sim_debug (DBG_NOTIFY, & tape_dev,
                                "%s: EOF: %s\n", __func__, simh_tape_msg (ret));
                    p -> stati = 04423; // EOF category EOF file mark
                    if (tape_statep -> tbc != 0)
                      {
                        sim_debug (DBG_ERR, &tape_dev,
                                   "%s: Read %d bytes with EOF.\n", 
                                   __func__, tape_statep -> tbc);
                        break;
                      }
                    break;
                  }
                if (ret == MTSE_EOM)
                  {
                    sim_debug (DBG_NOTIFY, & tape_dev,
                                "%s: EOM: %s\n", __func__, simh_tape_msg (ret));
// If the tape is blank, a read should result in '4302' blank tape on read.
                    if (sim_tape_bot (unitp))
                      p -> stati = 04302; // blank tape on read
                    else
                      p -> stati = 04340; // EOT file mark
                    if (tape_statep -> tbc != 0)
                      {
                        sim_debug (DBG_ERR, &tape_dev,
                                   "%s: Read %d bytes with EOM.\n", 
                                   __func__, tape_statep -> tbc);
                        break;
                      }
                    break;
                  }
                sim_debug (DBG_ERR, & tape_dev,
                           "%s: Cannot read tape: %d - %s\n",
                           __func__, ret, simh_tape_msg(ret));
                sim_debug (DBG_ERR, & tape_dev,
                           "%s: Returning arbitrary error code\n",
                           __func__);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatParityErrPeriph;
                break;
              }
            tape_statep -> rec_num ++;
            tape_statep -> words_processed = 0;
            if (unitp->flags & UNIT_WATCH)
              sim_printf ("Tape %ld reads record %d\n",
                          MT_UNIT_NUM (unitp), tape_statep -> rec_num);

            // Get the DDCW

            dcw_t dcw;
            int rc = iomListService (iomUnitIdx, chan, & dcw, NULL);

            if (rc)
              {
                sim_printf ("list service failed\n");
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncomplete;
                break;
              }
            if (dcw . type != ddcw)
              {
                sim_printf ("not ddcw? %d\n", dcw . type);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncorrectDCW;
                break;
              }

            uint type = dcw.fields.ddcw.type;
            uint tally = dcw.fields.ddcw.tally;
            uint daddr = dcw.fields.ddcw.daddr;
            // uint cp = dcw.fields.ddcw.cp;
            if (pcwp -> mask)
              daddr |= ((pcwp -> ext) & MASK6) << 18;
            if (type == 0) // IOTD
              * disc = true;
            else if (type == 1) // IOTP
              * disc = false;
            else
              {
sim_printf ("uncomfortable with this\n");
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncorrectDCW;
                break;
              }
#if 0
            if (type == 3 && tally != 1)
              {
                sim_debug (DBG_ERR, & tape_dev, "%s: Type is 3, but tally is %d\n",
                           __func__, tally);
              }
#endif
            if (tally == 0)
              {
                sim_debug (DBG_DEBUG, & tape_dev,
                           "%s: Tally of zero interpreted as 010000(4096)\n",
                           __func__);
                tally = 4096;
              }

            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Tally %d (%o)\n", __func__, tally, tally);
            if (p -> ptp)
              {
                word36 buffer [tally];
                uint i;
                for (i = 0; i < tally; i ++)
                  {
                    int rc;
                    if (tape_statep -> is9)
                      rc = extractASCII36FromBuffer (tape_statep -> buf, tape_statep -> tbc, & tape_statep -> words_processed, buffer + i);
                    else
                      rc = extractWord36FromBuffer (tape_statep -> buf, tape_statep -> tbc, & tape_statep -> words_processed, buffer + i);
                    if (rc)
                      {
                        // BUG: There isn't another word to be read from the tape buffer,
                        // but the IOM wants  another word.
                        // BUG: How did this tape hardware handle an attempt to read more
                        // data than was present?
                        // One answer is in bootload_tape_label.alm which seems to assume
                        // a 4000 all-clear status.
                        // Boot_tape_io.pl1 seems to assume that "short reads" into an
                        // over-large buffer should not yield any error return.
                        // So we'll set the flags to all-ok, but return an out-of-band
                        // non-zero status to make the iom stop.
                        // BUG: See some of the IOM status fields.
                        // BUG: The IOM should be updated to return its DCW tally residue
                        // to the caller.
                        p -> stati = 04000;
                        if (sim_tape_wrp (unitp))
                          p -> stati |= 1;
                        sim_debug (DBG_WARN, & tape_dev,
                                   "%s: Read buffer exhausted on channel %d\n",
                                   __func__, chan);
                        break;
                      }
                  }
                p -> tallyResidue = tally - i;
if (tape_statep -> is9) sim_printf ("words %d %012llo %12llo\n", tape_statep -> words_processed, buffer [0], buffer [1]);
                indirectDataService (iomUnitIdx, chan, daddr, tape_statep -> words_processed, buffer,
                                     idsTypeW36, true, & p -> isOdd);
              }
            else
              {
                p -> tallyResidue = tally;
                while (tally)
                  {
                    // read
                    word36 w;
                    int rc;
                    if (tape_statep -> is9)
                      rc = extractASCII36FromBuffer (tape_statep -> buf, tape_statep -> tbc, & tape_statep -> words_processed, & w);
                    else
                      rc = extractWord36FromBuffer (tape_statep -> buf, tape_statep -> tbc, & tape_statep -> words_processed, & w);
                    if (rc)
                      {
                        // BUG: There isn't another word to be read from the tape buffer,
                        // but the IOM wants  another word.
                        // BUG: How did this tape hardware handle an attempt to read more
                        // data than was present?
                        // One answer is in bootload_tape_label.alm which seems to assume
                        // a 4000 all-clear status.
                        // Boot_tape_io.pl1 seems to assume that "short reads" into an
                        // over-large buffer should not yield any error return.
                        // So we'll set the flags to all-ok, but return an out-of-band
                        // non-zero status to make the iom stop.
                        // BUG: See some of the IOM status fields.
                        // BUG: The IOM should be updated to return its DCW tally residue
                        // to the caller.
                        p -> stati = 04000;
                        if (sim_tape_wrp (unitp))
                          p -> stati |= 1;
                        sim_debug (DBG_WARN, & tape_dev,
                                   "%s: Read buffer exhausted on channel %d\n",
                                   __func__, chan);
                        break;
                      }
                    store_abs_word (daddr, w, __func__);
                    p -> isOdd = daddr % 2;
                    daddr ++;
                    tally --;
                    p -> tallyResidue --;
                  }
              }
            p -> stati = 04000; // BUG: do we need to detect end-of-record?
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;

            sim_debug (DBG_INFO, & tape_dev,
                       "%s: Read %d bytes from simulated tape; status %04o\n",
                       __func__, (int) tape_statep -> tbc, p -> stati);
          }
          break;

        case 013: // CMD 013 -- Write tape 9
        case 015: // CMD 015 -- Write Binary Record
          {
            tape_statep -> is9 = pcwp -> dev_cmd == 013;
            sim_debug (DBG_DEBUG, & tape_dev,
                       tape_statep -> is9 ?" mt_cmd: Write 9 record\n" : "mt_cmd: Write binary record\n");

            // Get the DDCW

            dcw_t dcw;
            int rc = iomListService (iomUnitIdx, chan, & dcw, NULL);

            if (rc)
              {
                sim_printf ("list service failed\n");
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncomplete;
                break;
              }
            if (dcw . type != ddcw)
              {
                sim_printf ("not ddcw? %d\n", dcw . type);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncorrectDCW;
                break;
              }

            uint type = dcw.fields.ddcw.type;
            uint tally = dcw.fields.ddcw.tally;
            uint daddr = dcw.fields.ddcw.daddr;
            // uint cp = dcw.fields.ddcw.cp;
            if (pcwp -> mask)
              daddr |= ((pcwp -> ext) & MASK6) << 18;
            if (type == 0) // IOTD
              * disc = true;
            else if (type == 1) // IOTP
              * disc = false;
            else
              {
                sim_printf ("uncomfortable with this\n"); // XXX
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncorrectDCW;
                break;
              }
            if (tally == 0)
              {
                sim_debug (DBG_DEBUG, & tape_dev,
                           "%s: Tally of zero interpreted as 010000(4096)\n",
                           __func__);
                tally = 4096;
              }

            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Tally %d (%o)\n", __func__, tally, tally);

            tape_statep -> words_processed = 0;
            if (tape_statep -> is9)
              tape_statep -> tbc = bufsz * 4;
            else
              tape_statep -> tbc = (bufsz * 9 + 1) / 2;
            if (p -> ptp)
              {
                word36 buffer [tally];
            
                // copy from core into buffer
                indirectDataService (iomUnitIdx, chan, daddr, tally, buffer,
                                     idsTypeW36, false, & p -> isOdd);
                uint i;
                for (i = 0; i < tally; i ++)
                  {
                    int rc;
                    if (tape_statep -> is9)
                      {
                        rc = insertASCII36toBuffer (tape_statep -> buf, 
                                                    tape_statep -> tbc, 
                                                    & tape_statep -> words_processed, 
                                                    buffer [i]);
                      }
                    else
                      {
                        rc = insertWord36toBuffer (tape_statep -> buf, 
                                                   tape_statep -> tbc, 
                                                   & tape_statep -> words_processed, 
                                                   buffer [i]);
                        }
                    if (rc)
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
                p -> tallyResidue = tally - i;
              }
            else
              {
                p -> tallyResidue = tally;
                while (tally)
                  {
                    word36 w;

                    // get data from core
                    fetch_abs_word (daddr, & w, __func__);

                    // repack into buf
                    int rc;
                    if (tape_statep -> is9)
                      {
                        rc = insertASCII36toBuffer ( tape_statep -> buf, tape_statep -> tbc, & tape_statep -> words_processed, w);
                      }
                    else
                      {
                        rc = insertWord36toBuffer ( tape_statep -> buf, tape_statep -> tbc, & tape_statep -> words_processed, w);
                      }
                    if (rc)
                      {
                        // BUG: There isn't another word to be read from the tape buffer,
                        // but the IOM wants  another word.
                        p -> stati = 04000;
                        if (sim_tape_wrp (unitp))
                          p -> stati |= 1;
                        sim_debug (DBG_WARN, & tape_dev,
                                   "%s: Write buffer exhausted on channel %d\n",
                                   __func__, chan);
                        break;
                      }
                    p -> isOdd = daddr % 2;
                    daddr ++;
                    tally --;
                    p -> tallyResidue --;
                  }
              }

            sim_debug (DBG_DEBUG, & tape_dev, 
                       "buf filled with words_processed %d\n",
                        tape_statep -> words_processed);
            // write buf to the tape

            if (tape_statep -> is9)
              tape_statep -> tbc = tape_statep -> words_processed * 4;
            else
              tape_statep -> tbc = (tape_statep -> words_processed * 9 + 1) / 2;
            int ret;
            if (! (unitp -> flags & UNIT_ATT))
              ret = MTSE_UNATT;
            else
              {
                ret = sim_tape_wrrecf (unitp, tape_statep -> buf, tape_statep -> tbc);
if (tape_statep -> is9) sim_printf ("tbc %d <%s>\n", tape_statep -> tbc, tape_statep -> buf);
                sim_debug (DBG_DEBUG, & tape_dev, "sim_tape_wrrecf returned %d, with tbc %d\n", ret, tape_statep -> tbc);
                // XXX put unit number in here...
              }
            if (ret != 0)
              {
                * disc = true;
                if (ret == MTSE_EOM)
                  {
                    sim_debug (DBG_NOTIFY, & tape_dev,
                                "%s: EOM: %s\n", __func__, simh_tape_msg (ret));
                    p -> stati = 04340; // EOT file mark
                    if (tape_statep -> tbc != 0)
                      {
                        sim_debug (DBG_ERR, &tape_dev,
                                   "%s: Wrote %d bytes with EOM.\n", 
                                   __func__, tape_statep -> tbc);
                        break;
                      }
                    break;
                  }
                sim_debug (DBG_ERR, & tape_dev,
                           "%s: Cannot write tape: %d - %s\n",
                           __func__, ret, simh_tape_msg(ret));
                sim_debug (DBG_ERR, & tape_dev,
                           "%s: Returning arbitrary error code\n",
                           __func__);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatParityErrPeriph;
                break;
              }
            tape_statep -> rec_num ++;
            if (unitp->flags & UNIT_WATCH)
              sim_printf ("Tape %ld writes record %d\n",
                          MT_UNIT_NUM (unitp), tape_statep -> rec_num);

            p -> stati = 04000; 
            if (sim_tape_eot (unitp))
              p -> stati |= 0340;

            sim_debug (DBG_INFO, & tape_dev,
                       "%s: Wrote %d bytes to simulated tape; status %04o\n",
                       __func__, (int) tape_statep -> tbc, p -> stati);
          }
          break;

        case 044: // 044 Forward skip  Record
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Forward Skip Record\n");
            // BUG: Do we need to clear the buffer?
            // BUG? We don't check the channel data for a count
            // Get the DDCW

            dcw_t dcw;
            int rc = iomListService (iomUnitIdx, chan, & dcw, NULL);

            if (rc)
              {
                sim_printf ("list service failed\n");
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncomplete;
                break;
              }
            if (dcw . type != ddcw)
              {
                sim_printf ("not ddcw? %d\n", dcw . type);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncorrectDCW;
                break;
              }

            uint type = dcw.fields.ddcw.type;
            uint tally = dcw.fields.ddcw.tally;
            //uint daddr = dcw.fields.ddcw.daddr;
            // uint cp = dcw.fields.ddcw.cp;
            //if (pcwp -> mask)
              //daddr |= ((pcwp -> ext) & MASK6) << 18;
            if (type == 0) // IOTD
              * disc = true;
            else if (type == 1) // IOTP
              * disc = false;
            else
              {
sim_printf ("uncomfortable with this\n");
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncorrectDCW;
                break;
              }

//
// dcl ORDERS_PER_IDCW_TALLY  fixed bin static options (constant) init (64);
//  /* The following orders take a tally */
//  
//  QUEUE_ORDER (2): /* backspace record */
//  QUEUE_ORDER (4): /* forwardspace record */
//     if count >= ORDERS_PER_IDCW_TALLY then
//       idcw.count = "00"b3;   do as many as we can */
//     else idcw.count = bit (bin (count, 6), 6);
//

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
sim_printf ("skipped %d != tally %d\n", skipped, tally);
              }

            tape_statep -> rec_num += skipped;
            if (unitp->flags & UNIT_WATCH)
              sim_printf ("Tape %ld forward skips to record %d\n",
                          MT_UNIT_NUM (unitp), tape_statep -> rec_num);

            p -> tallyResidue = tally - skipped;

            sim_debug (DBG_NOTIFY, & tape_dev, 
                       "mt_iom_cmd: Forward space %d records\n", skipped);

            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            if (sim_tape_eot (unitp))
              p -> stati |= 0340;
          }
          break;

        case 046: // 046 Backspace Record
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Backspace Record\n");
            // BUG: Do we need to clear the buffer?
            // BUG? We don't check the channel data for a count
            // Get the DDCW

            dcw_t dcw;
            int rc = iomListService (iomUnitIdx, chan, & dcw, NULL);

            if (rc)
              {
                sim_printf ("list service failed\n");
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncomplete;
                break;
              }
            if (dcw . type != ddcw)
              {
                sim_printf ("not ddcw? %d\n", dcw . type);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncorrectDCW;
                break;
              }

            uint type = dcw.fields.ddcw.type;
            uint tally = dcw.fields.ddcw.tally;
            //uint daddr = dcw.fields.ddcw.daddr;
            // uint cp = dcw.fields.ddcw.cp;
            //if (pcwp -> mask)
              //daddr |= ((pcwp -> ext) & MASK6) << 18;
            if (type == 0) // IOTD
              * disc = true;
            else if (type == 1) // IOTP
              * disc = false;
            else
              {
sim_printf ("uncomfortable with this\n");
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncorrectDCW;
                break;
              }

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
sim_printf ("skipped %d != tally %d\n", skipped, tally);
              }
            tape_statep -> rec_num -= skipped;
            if (unitp->flags & UNIT_WATCH)
              sim_printf ("Tape %ld skip back to record %d\n",
                          MT_UNIT_NUM (unitp), tape_statep -> rec_num);

            p -> tallyResidue = tally - skipped;

            sim_debug (DBG_NOTIFY, & tape_dev, 
                       "mt_iom_cmd: Backspace %d records\n", skipped);
#endif

            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            if (sim_tape_eot (unitp))
              p -> stati |= 0340;
          }
          break;

        case 047: // 047 Backspace File
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Backspace File\n");
            // BUG: Do we need to clear the buffer?
            // BUG? We don't check the channel data for a count
            // XXX Why does this command have a DDCW?

            // Get the DDCW

            dcw_t dcw;
            int rc = iomListService (iomUnitIdx, chan, & dcw, NULL);

            if (rc)
              {
                sim_printf ("list service failed\n");
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncomplete;
                break;
              }
            if (dcw . type != ddcw)
              {
                sim_printf ("not ddcw? %d\n", dcw . type);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncorrectDCW;
                break;
              }

            uint type = dcw.fields.ddcw.type;
            uint tally = dcw.fields.ddcw.tally;
            //uint daddr = dcw.fields.ddcw.daddr;
            // uint cp = dcw.fields.ddcw.cp;
            //if (pcwp -> mask)
              //daddr |= ((pcwp -> ext) & MASK6) << 18;
            if (type == 0) // IOTD
              * disc = true;
            else if (type == 1) // IOTP
              * disc = false;
            else
              {
sim_printf ("uncomfortable with this\n");
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncorrectDCW;
                break;
              }

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
sim_printf ("sim_tape_spfilebyrecr returned %d\n", ret);
                 break;
              }
            if (skipped != tally)
              {
sim_printf ("skipped %d != tally %d\n", skipped, tally);
              }

            tape_statep -> rec_num -= recsskipped;
            if (unitp->flags & UNIT_WATCH)
              sim_printf ("Tape %ld backward skips to record %d\n",
                          MT_UNIT_NUM (unitp), tape_statep -> rec_num);

            p -> tallyResidue = tally - skipped;
            sim_debug (DBG_NOTIFY, & tape_dev, 
                       "mt_iom_cmd: Backspace %d records\n", tally);
#endif

            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            if (sim_tape_eot (unitp))
              p -> stati |= 0340;
          }
          break;

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
              }
            if (ret != 0)
              {
                * disc = true;
                if (ret == MTSE_EOM)
                  {
                    sim_debug (DBG_NOTIFY, & tape_dev,
                                "%s: EOM: %s\n", __func__, simh_tape_msg (ret));
                    p -> stati = 04340; // EOT file mark
                    sim_debug (DBG_ERR, &tape_dev,
                               "%s: Wrote tape mark with EOM.\n", 
                               __func__);
                    break;
                  }
                sim_debug (DBG_ERR, & tape_dev,
                           "%s: Cannot write tape mark: %d - %s\n",
                           __func__, ret, simh_tape_msg(ret));
                sim_debug (DBG_ERR, & tape_dev,
                           "%s: Returning arbitrary error code\n",
                           __func__);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatParityErrPeriph;
                break;
              }

            tape_statep -> rec_num ++;
            if (unitp->flags & UNIT_WATCH)
              sim_printf ("Tape %ld writes tape mark %d\n",
                          MT_UNIT_NUM (unitp), tape_statep -> rec_num);

            p -> stati = 04000; 
            if (sim_tape_eot (unitp))
              p -> stati |= 0340;

            sim_debug (DBG_INFO, & tape_dev,
                       "%s: Wrote tape mark; status %04o\n",
                       __func__, p -> stati);
          }
          break;

        case 057:               // CMD 057 -- Survey devices
          {
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
                       "mt_cmd: Survey devices\n");
            p -> stati = 04000; // have_status = 1
            //* need_data = true;

#if 0
sim_printf ("get the idcw\n");
            // Get the IDCW
            dcw_t dcw;
            int rc = iomListService (iomUnitIdx, chan, & dcw, NULL);

            if (rc)
              {
                sim_printf ("list service failed\n");
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                break;
              }
            if (dcw . type != idcw)
              {
                sim_printf ("not idcw? %d\n", dcw . type);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                break;
              }
#endif

#if 1
#ifdef IOMDBG1
sim_printf ("get the ddcw\n");
#endif
            // Get the DDCW
            dcw_t dcw;
            int rc = iomListService (iomUnitIdx, chan, & dcw, NULL);

            if (rc)
              {
                sim_printf ("list service failed\n");
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncomplete;
                break;
              }
            if (dcw . type != ddcw)
              {
                sim_printf ("not ddcw? %d\n", dcw . type);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncorrectDCW;
                break;
              }
#endif

#if 1
            uint type = dcw.fields.ddcw.type;
            uint tally = dcw.fields.ddcw.tally;
            uint daddr = dcw.fields.ddcw.daddr;
            // uint cp = dcw.fields.ddcw.cp;
            if (pcwp -> mask)
              daddr |= ((pcwp -> ext) & MASK6) << 18;
            if (type == 0) // IOTD
              * disc = true;
            else if (type == 1) // IOTP
              * disc = false;
            else
              {
sim_printf ("uncomfortable with this\n");
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                p -> chanStatus = chanStatIncorrectDCW;
                break;
              }


            sim_debug (DBG_DEBUG, & tape_dev,
                       "tally %04o daddr %06o\n", tally, daddr);
            
            if (tally != 8)
              {
                sim_debug (DBG_DEBUG, & tape_dev,
                           "%s: Expected tally of 8; got %d\n",
                           __func__, tally);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                break;
              }
#endif
#if 1
            word36 buffer [8];
            int cnt = 0;
            for (uint i = 0; i < 8; i ++)
              buffer [i] = 0;
            
            for (uint i = 1; i < N_MT_UNITS_MAX; i ++)
              {
                if (cables -> cablesFromIomToTap [i] . iomUnitIdx != -1)
                  {
                    word18 handler = 0;
// Test hack: unit 0 never operational
                    if (i != 0)
                      handler |= 0100000; // operational
                    //if (find_dev_from_unit (& mt_unit [i]))
                      //sim_printf ("Unit %d has dev\n", i);
                    if (mt_unit [i] . filename)
                      {
                        handler |= 0040000; // ready
                        //sim_printf ("Unit %d ready\n", i);
                      }
                    handler |= (cables -> cablesFromIomToTap [i] . dev_code & 037) << 9; // number
                    handler |= 0000040; // 200 ips
                    handler |= 0000020; // 9 track
                    handler |= 0000007; // 800/1600/6250
                    sim_debug (DBG_DEBUG, & tape_dev,
                               "unit %d handler %06o\n", i, handler);
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
              }
#ifdef IOMDBG1
iomChannelData_ * p = & iomChannelData [iomUnitIdx] [chan];
sim_printf ("chan_mode %d\n", p -> chan_mode);
#endif
            indirectDataService (iomUnitIdx, chan, daddr, 8, buffer,
                                 idsTypeW36, true, & p -> isOdd);
#endif
            p -> stati = 04000;
          }
          break;
 
        case 060:              // CMD 060 -- Set 800 bpi.
          {
            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            if (sim_tape_eot (unitp))
              p -> stati |= 0340;
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Set 800 bpi\n");
          }
          break;

        case 061:              // CMD 061 -- Set 556 bpi.
          {
            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            if (sim_tape_eot (unitp))
              p -> stati |= 0340;
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Set 556 bpi\n");
          }
          break;

        case 063:              // CMD 063 -- Set File Permit.
          {
            sim_debug (DBG_ERR, & tape_dev, "Set file permit?\n");
            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
          }
          break;

        case 064:              // CMD 064 -- Set 200 bpi.
          {
            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            if (sim_tape_eot (unitp))
              p -> stati |= 0340;
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Set 200 bpi\n");
          }
          break;

        case 065:              // CMD 064 -- Set 1600 CPI
          {
            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            if (sim_tape_eot (unitp))
              p -> stati |= 0340;
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Set 1600 CPI\n");
          }
          break;

        case 070:              // CMD 070 -- Rewind.
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Rewind\n");
            sim_tape_rewind (unitp);

            tape_statep -> rec_num = 0;
            if (unitp->flags & UNIT_WATCH)
              sim_printf ("Tape %ld rewinds\n", MT_UNIT_NUM (unitp));

            p -> stati = 04000;
            if (sim_tape_wrp (unitp))
              p -> stati |= 1;
            if (sim_tape_bot (unitp))
              p -> stati |= 2;
            if (sim_tape_eot (unitp))
              p -> stati |= 0340;
            //rewindDoneUnit . u3 = mt_unit_num;
            //sim_activate (& rewindDoneUnit, 4000000); // 4M ~= 1 sec
    send_special_interrupt (cables -> cablesFromIomToTap [mt_unit_num] . iomUnitIdx,
                            cables -> cablesFromIomToTap [mt_unit_num] . chan_num,
                            cables -> cablesFromIomToTap [mt_unit_num] . dev_code,
                            0, 0100 /* rewind complete */);

          }
          break;
   
        case 072:              // CMD 072 -- Rewind/Unload.
          {
            if (unitp->flags & UNIT_WATCH)
              sim_printf ("Tape %ld unloads\n",
                          MT_UNIT_NUM (unitp));
            sim_debug (DBG_DEBUG, & tape_dev,
                       "mt_cmd: Rewind/unload\n");
sim_printf ("unloading\n");
            sim_tape_detach (unitp);
            //tape_statep -> rec_num = 0;
            p -> stati = 04000;
    send_special_interrupt (cables -> cablesFromIomToTap [mt_unit_num] . iomUnitIdx,
                            cables -> cablesFromIomToTap [mt_unit_num] . chan_num,
                            cables -> cablesFromIomToTap [mt_unit_num] . dev_code,
                            0, 0040 /* unload complete */);
          }
          break;
   
        default:
          {
fail:
            p -> stati = 04501;
            sim_debug (DBG_ERR, & tape_dev,
                       "%s: Unknown command 0%o\n", __func__, pcwp -> dev_cmd);
            p -> chanStatus = chanStatIncorrectDCW;
            break;
          }
      }

    sim_debug (DBG_DEBUG, & tape_dev, "stati %04o\n", p -> stati);
    // status_service (iomUnitIdx, chan, pcwp -> control == 3);

    //if (pcwp -> control & 1) // marker bit set
    if (p -> PCW_CONTROL == 3) // marker bit set
      {
        sim_printf ("XXX marker intr\n");
        sim_printf ("XXX marker intr\n");
        sim_printf ("XXX marker intr\n");
        sim_printf ("XXX marker intr\n");
        send_marker_interrupt (iomUnitIdx, chan);
      }
    return 0;
  }
#endif

static int mt_io_cmd (uint iomUnitIdx, uint chan)
  {
    iomChanData_t * p = & iomChanData [iomUnitIdx] [chan];
    struct device * d = & cables -> cablesFromIomToDev [iomUnitIdx] .
                      devices [chan] [p -> IDCW_DEV_CODE];
    uint devUnitIdx = d -> devUnitIdx;
    UNIT * unitp = & mt_unit [devUnitIdx];
    struct tape_state * tape_statep = & tape_states [devUnitIdx];
sim_printf ("mt_io_cmd mode %d\n", tape_statep -> io_mode);

    if (tape_statep -> io_mode == no_mode)
      {
        // no prior read or write command
        p -> stati = 05302; // MPC Device Data Alert Inconsistent command
        sim_debug (DBG_ERR, & tape_dev, "%s: Bad channel %d\n", __func__, chan);
        return 1;
      }

    if (tape_statep -> io_mode == read_mode)
      {
sim_printf ("read_mode\n");
        uint tally = p -> DDCW_TALLY;
        uint daddr = p -> DDCW_ADDR;
        if (tally == 0)
          {
            sim_debug (DBG_DEBUG, & tape_dev,
                       "%s: Tally of zero interpreted as 010000(4096)\n",
                       __func__);
            tally = 4096;
          }

        sim_debug (DBG_DEBUG, & tape_dev,
                   "%s: Tally %d (%o)\n", __func__, tally, tally);
        if (p -> PCW_63_PTP)
          {
sim_printf ("read PCW_63_PTP\n");
            word36 buffer [tally];
            uint i;
            for (i = 0; i < tally; i ++)
              {
                int rc;
                if (tape_statep -> is9)
                  rc = extractASCII36FromBuffer (tape_statep -> buf, tape_statep -> tbc, & tape_statep -> words_processed, buffer + i);
                else
                  rc = extractWord36FromBuffer (tape_statep -> buf, tape_statep -> tbc, & tape_statep -> words_processed, buffer + i);
                if (rc)
                  {
                    // BUG: There isn't another word to be read from the tape buffer,
                    // but the IOM wants  another word.
                    // BUG: How did this tape hardware handle an attempt to read more
                    // data than was present?
                    // One answer is in bootload_tape_label.alm which seems to assume
                    // a 4000 all-clear status.
                    // Boot_tape_io.pl1 seems to assume that "short reads" into an
                    // over-large buffer should not yield any error return.
                    // So we'll set the flags to all-ok, but return an out-of-band
                    // non-zero status to make the iom stop.
                    // BUG: See some of the IOM status fields.
                    // BUG: The IOM should be updated to return its DCW tally residue
                    // to the caller.
                    p -> stati = 04000;
                    if (sim_tape_wrp (unitp))
                      p -> stati |= 1;
                    sim_debug (DBG_WARN, & tape_dev,
                               "%s: Read buffer exhausted on channel %d\n",
                               __func__, chan);
                    break;
                  }
              }
            p -> tallyResidue = tally - i;
if (tape_statep -> is9) sim_printf ("words %d %012llo %12llo\n", tape_statep -> words_processed, buffer [0], buffer [1]);
            indirectDataService (iomUnitIdx, chan, daddr, tape_statep -> words_processed, buffer,
                                 idsTypeW36, true, & p -> isOdd);
          }
        else
          {
sim_printf ("read !PCW_63_PTP\n");
            p -> tallyResidue = tally;
sim_printf ("read tally %u tbc %u\n", tally, tape_statep -> tbc);
{for (int i = 0; i < 32; i ++) sim_printf (" %03o", tape_statep -> buf [i]); sim_printf ("\n");}
            while (tally)
              {
                // read
                word36 w;
                int rc;
                if (tape_statep -> is9)
                  rc = extractASCII36FromBuffer (tape_statep -> buf, tape_statep -> tbc, & tape_statep -> words_processed, & w);
                else
                  rc = extractWord36FromBuffer (tape_statep -> buf, tape_statep -> tbc, & tape_statep -> words_processed, & w);
                if (rc)
                  {
                    p -> stati = 04000;
                    if (sim_tape_wrp (unitp))
                      p -> stati |= 1;
                    sim_debug (DBG_WARN, & tape_dev,
                               "%s: Read buffer exhausted on channel %d\n",
                               __func__, chan);
                    break;
                  }
sim_printf ("  %08o %012llo  %d\n", daddr, w, tape_statep -> words_processed);
                core_write (daddr, w, __func__);
                p -> isOdd = daddr % 2;
                daddr ++;
                tally --;
                p -> tallyResidue --;
              }
          }
        p -> stati = 04000; // BUG: do we need to detect end-of-record?
        if (sim_tape_wrp (unitp))
          p -> stati |= 1;

        sim_debug (DBG_INFO, & tape_dev,
                   "%s: Read %d bytes from simulated tape; status %04o\n",
                   __func__, (int) tape_statep -> tbc, p -> stati);
        return 0;
      }

sim_printf ("mt_io_cmd fail\n");
    // no prior read or write command
    p -> stati = 05302; // MPC Device Data Alert Inconsistent command
    sim_debug (DBG_ERR, & tape_dev, "%s: Bad channel %d\n", __func__, chan);
    return -1;
  }
#if 0
  {
    //sim_debug (DBG_DEBUG, & tape_dev, "%s\n", __func__);
    int mt_unit_num = MT_UNIT_NUM (unitp);
    //int iomUnitIdx = cables -> cablesFromIomToTap [mt_unit_num] . iomUnitIdx;
//--     
//--     int dev_unit_num;
//--     DEVICE* devp = get_iom_channel_dev (iomUnitIdx, chan, dev_code, & dev_unit_num);
//--     if (devp == NULL || devp->units == NULL) {
//--         *majorp = 05;
//--         *subp = 2;
//--         sim_debug (DBG_ERR, &tape_dev, "mt_iom_io: Internal error, no device and/or unit for channel 0%o\n", chan);
//--         return 1;
//--     }
//--     UNIT * unitp = & devp -> units [dev_unit_num];
//--     // BUG: no dev_code
//--     
    struct tape_state * tape_statep = & tape_states [mt_unit_num];
    
    if (tape_statep -> io_mode == no_mode)
      {
        // no prior read or write command
        * stati = 05302; // MPC Device Data Alert Inconsistent command
        sim_debug (DBG_ERR, & tape_dev, "%s: Bad channel %d\n", __func__, chan);
        return 1;
      }
    else if (tape_statep -> io_mode == read_mode)
      {
        while (* tally)
          {
            // read
            if (extractWord36FromBuffer (tape_statep -> buf, tape_statep -> tbc, & tape_statep -> words_processed, wordp) != 0)
              {
                // BUG: There isn't another word to be read from the tape buffer,
                // but the IOM wants  another word.
                // BUG: How did this tape hardware handle an attempt to read more
                // data than was present?
                // One answer is in bootload_tape_label.alm which seems to assume
                // a 4000 all-clear status.
                // Boot_tape_io.pl1 seems to assume that "short reads" into an
                // over-large buffer should not yield any error return.
                // So we'll set the flags to all-ok, but return an out-of-band
                // non-zero status to make the iom stop.
                // BUG: See some of the IOM status fields.
                // BUG: The IOM should be updated to return its DCW tally residue
                // to the caller.
                * stati = 04000;
                if (sim_tape_wrp (unitp))
                  * stati |= 1;
                sim_debug (DBG_WARN, & tape_dev,
                           "%s: Read buffer exhausted on channel %d\n",
                           __func__, chan);
                return 1;
              }
            wordp ++;
            (* tally) --;
          }
        * stati = 04000; // BUG: do we need to detect end-of-record?
        if (sim_tape_wrp (unitp))
          * stati |= 1;
        return 0;
      }
    else if (tape_statep -> io_mode == survey_mode)
      {
        //        2 survey_data,
        //          3 handler (16) unaligned,
        //            4 pad1 bit (1),               // 0
        //            4 reserved bit (1),           // 1
        //            4 operational bit (1),        // 2
        //            4 ready bit (1),              // 3
        //            4 number uns fixed bin (5),   // 4-8
        //            4 pad2 bit (1),               // 9
        //            4 speed uns fixed bin (3),    // 10-12
        //            4 nine_track bit (1),         // 13
        //            4 density uns fixed bin (4);  // 14-17
        
        return 0;
      }
    else
      {
        // write
        sim_debug (DBG_ERR, & tape_dev, "%s: Write I/O Unimplemented\n",
                   __func__);
        * stati = 04340; // Reflective end of tape mark found while trying to write
        return 1;
      }
    
//--     /*notreached*/
//--     *majorp = 0;
//--     *subp = 0;
//--     sim_debug (DBG_ERR, &tape_dev, "mt_iom_io: Internal error.\n");
//--     cancel_run(STOP_BUG);
//    return 1;
  }
#endif

/*
 * mt_iom_cmd()
 *
 */

// 1 ignored command
// 0 ok
// -1 problem
int mt_iom_cmd (uint iomUnitIdx, uint chan)
  {
    iomChanData_t * p = & iomChanData [iomUnitIdx] [chan];
// Is it an IDCW?

    if (p -> DCW_18_20_CP == 7)
      {
        mt_cmd (iomUnitIdx, chan);
      }
    else // DDCW/TDCW
      {
        sim_printf ("mt_iom_cmd expected IDCW\n");
        return -1;
      }
    return 0;
#ifdef IOM2
    int mt_unit_num = MT_UNIT_NUM (unitp);
    if (mt_unit_num == 0)
      {
        mt_unit_num = boot_drive;
        unitp = mt_unit + mt_unit_num;
        //sim_printf ("pcw boot unit set to %d\n", mt_unit_num);
      }
    int iomUnitIdx = cables -> cablesFromIomToTap [mt_unit_num] . iomUnitIdx;

    // First, execute the command in the PCW, and then walk the 
    // payload channel mbx looking for IDCWs.

    //uint chanloc = mbx_loc (iomUnitIdx, pcwp -> chan);
    //lpw_t lpw;
    //fetch_and_parse_lpw (& lpw, chanloc, false);

    bool disc;
//sim_printf ("1 st call to mt_cmd\n");
    mt_cmd (unitp, pcwp, & disc);

    // ctrl of the pcw is observed to be 0 even when there are idcws in the
    // list so ignore that and force it to 2.
    //uint ctrl = pcwp -> control;
    uint ctrl = 2;

    int ptro = 0;
//#define PTRO
#ifdef PTRO
    while ((! disc) /* && ctrl == 2 */ && ! ptro)
#else
    while ((! disc) && ctrl == 2)
#endif
      {
        dcw_t dcw;
        int rc = iomListService (iomUnitIdx, pcwp -> chan, & dcw, & ptro);
        if (rc)
          {
            break;
          }
        if (dcw . type != idcw)
          {
// 04501 : COMMAND REJECTED, invalid command
            iomChannelData_ * p = & iomChannelData [iomUnitIdx] [pcwp -> chan];
            p -> stati = 04501; 
            p -> dev_code = dcw . fields . instr. dev_code; 
            p -> chanStatus = chanStatInvalidInstrPCW;
            // status_service (iomUnitIdx, pcwp -> chan, false);
            break;
          }


// The dcw does not necessarily have the same dev_code as the pcw....

        mt_unit_num = findTapeUnit (iomUnitIdx, pcwp -> chan, dcw . fields . instr. dev_code);
        if (mt_unit_num == 0)
          {
            mt_unit_num = boot_drive;
            //sim_printf ("dcw boot unit set to %d\n", mt_unit_num);
          }
        if (mt_unit_num < 0)
          {
// 04502 : COMMAND REJECTED, invalid device code
            iomChannelData_ * p = & iomChannelData [iomUnitIdx] [pcwp -> chan];
            p -> stati = 04502; 
            p -> dev_code = dcw . fields . instr. dev_code; 
            p -> chanStatus = chanStatInvalidInstrPCW;
            // status_service (iomUnitIdx, pcwp -> chan, false);
            break;
          }
        unitp = & mt_unit [mt_unit_num];
//sim_printf ("next call to mt_cmd\n");
        mt_cmd (unitp, & dcw . fields . instr, & disc);
        ctrl = dcw . fields . instr . control;
//sim_printf ("disc %d ctrl %d\n", disc, ctrl);
      }
    //status_service (iomUnitIdx, pcwp -> chan, pcwp -> control == 3);

    send_terminate_interrupt (iomUnitIdx, pcwp -> chan);

    return 1;
#endif
  }

static t_stat mt_svc (UNIT * unitp)
  {
#ifdef IOM2
    int mtUnitNum = MT_UNIT_NUM (unitp);
    int iomUnitIdx = cables -> cablesFromIomToTap [mtUnitNum] . iomUnitIdx;
    int chanNum = cables -> cablesFromIomToTap [mtUnitNum] . chan_num;
    pcw_t * pcwp = & iomChannelData [iomUnitIdx] [chanNum] . pcw;
    xxmt_iom_cmd (unitp, pcwp);
#endif
    return SCPE_OK;
  }
    

//-- 
//-- static t_stat mt_svc(UNIT *up)
//-- {
//--     sim_debug (DBG_DEBUG, &tape_dev, "mt_svc: Calling channel service.\n");
//--     return channel_svc(up);
//-- }

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

static t_stat mt_rewind (UNIT * uptr, UNUSED int32 value, 
                         UNUSED char * cptr, UNUSED void * desc)
  {
    return sim_tape_rewind (uptr);
  }

static t_stat mt_show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr, 
                              UNUSED int val, UNUSED void * desc)
  {
    sim_printf("Number of TAPE units in system is %d\n", tape_dev . numunits);
    return SCPE_OK;
  }

static t_stat mt_set_nunits (UNUSED UNIT * uptr, UNUSED int32 value, 
                             char * cptr, UNUSED void * desc)
  {
    int n = atoi (cptr);
    if (n < 1 || n > N_MT_UNITS_MAX)
      return SCPE_ARG;
    tape_dev . numunits = (uint32) n;
    return SCPE_OK;
  }

static t_stat mt_show_boot_drive (UNUSED FILE * st, UNUSED UNIT * uptr, 
                              UNUSED int val, UNUSED void * desc)
  {
    sim_printf("Tape drive to boot from is %d\n", boot_drive);
    return SCPE_OK;
  }

static t_stat mt_set_boot_drive (UNUSED UNIT * uptr, UNUSED int32 value, 
                             UNUSED char * cptr, UNUSED void * desc)
  {
    int n = MT_UNIT_NUM (uptr);
    if (n < 0 || n >= N_MT_UNITS_MAX)
      return SCPE_ARG;
    boot_drive = (uint32) n;
    return SCPE_OK;
  }

static t_stat mt_show_device_name (UNUSED FILE * st, UNIT * uptr, 
                                   UNUSED int val, UNUSED void * desc)
  {
    int n = MT_UNIT_NUM (uptr);
    if (n < 0 || n >= N_MT_UNITS_MAX)
      return SCPE_ARG;
    sim_printf("Tape drive device name is %s\n", tape_states [n] . device_name);
    return SCPE_OK;
  }

static t_stat mt_set_device_name (UNUSED UNIT * uptr, UNUSED int32 value, 
                                  UNUSED char * cptr, UNUSED void * desc)
  {
    int n = MT_UNIT_NUM (uptr);
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
                                 UNUSED int val, UNUSED void * desc)
  {
    sim_printf("Tape path <%s>\n", tape_path);
    return SCPE_OK;
  }

static t_stat mt_set_tape_path (UNUSED UNIT * uptr, UNUSED int32 value, 
                             char * cptr, UNUSED void * desc)
  {
    if (strlen (cptr) >= TAPE_PATH_LEN - 1)
      {
        sim_printf ("truncating tape path\n");
      }
    strncpy (tape_path, cptr, TAPE_PATH_LEN);
    tape_path [TAPE_PATH_LEN - 1] = 0;
    return SCPE_OK;
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
    loadTape (i, label, ! withring);
    return SCPE_OK;
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
    unloadTape (i);
    return SCPE_OK;
  }



