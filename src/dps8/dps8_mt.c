/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * SPDX-License-Identifier: Multics
 * scspell-id: ae1c781a-f62e-11ec-bd2e-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2018 Charles Anthony
 * Copyright (c) 2021 Dean Anderson
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

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <dirent.h>

#include "dps8.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_utils.h"
#include "dps8_mt.h"

#define DBG_CTR 1

#ifdef TESTING
# undef FREE
# define FREE(p) free(p)
#endif /* ifdef TESTING */

/*
 * mt.c -- mag tape
 * See manual AN87
 */

/*
 *
 *  COMMENTS ON "CHAN DATA" AND THE T&D TAPE (test and diagnostic tape)
 *
 * The IOM status service provides the "residue" from the last PCW or
 * IDCW as part of the status.  Bootload_tape_label.alm indicates
 * that after a read binary operation, the field is interpreted as the
 * device number and that a device number of zero is legal.
 *
 * The IOM boot channel will store an IDCW with a chan-data field of zero.
 * AN70, page 2-1 says that when the tape is read in native mode via an
 * IOM or IOCC, the tape drive number in the IDCW will be zero.  It says
 * that a non-zero tape drive number in the IDCW indicates that BOS is
 * being used to simulate an IOM. (Presumably written before BCE replaced
 * BOS.)
 *
 * However...
 *
 * This seems to imply that an MPC could be connected to a single
 * channel and support multiple tape drives by using chan-data as a
 * device id.  If so, it seems unlikely that chan-data could ever
 * represent anything else such as a count of a number of records to
 * back space over (which is hinted at as an example in AN87).
 *
 * Using chan-data as a device-id that is zero from the IOM also
 * implies that Multics could only be cold booted from a tape drive with
 * device-id zero.  That doesn't seem to mesh with instructions
 * elsewhere... And BCE has to (initially) come from the boot tape...
 *
 *     Comment by CAC:  From MDD-005-02:
 *
 *       "     bootload_tape_label  is read  in by  one of  two means.   In
 *        native mode, the  IOM or IMU reads it into  absolute location 30,
 *        leaving  the PCW,  DCW's, and   other essentials  in locations  0
 *        through  5.  The IMU  leaves an indication  of its identity  just
 *        after this block of information.
 *
 *             In  BOS compatibility mode,  the BOS BOOT  command simulates
 *        the IOM, leaving the same information.  However, it also leaves a
 *        config deck and flagbox (although bce has its own flagbox) in the
 *        usual locations.   This allows Bootload Multics to  return to BOS
 *        if there is a BOS to return to.  The presence of BOS is indicated
 *        by the tape drive number being  non-zero in the idcw in the "IOM"
 *        provided information.   (This is normally zero  until firmware is
 *        loaded into the bootload tape MPC.)
 *
 * The T&D tape seems to want to see non-zero residue from the very
 * first tape read.  That seems to imply that the T&D tape could not
 * be booted by the IOM!  Perhaps the T&D tape requires BCE (which
 * replaced BOS) ?
 *
 *  TODO
 *
 * When simulating timing, switch to queuing the activity instead
 * of queueing the status return.   That may allow us to remove most
 * of our state variables and more easily support save/restore.
 *
 * Convert the rest of the routines to have a chan_devinfo argument.
 *
 * Allow multiple tapes per channel.
 */

#include "sim_tape.h"

static DEBTAB mt_dt [] =
  {
    { "NOTIFY", DBG_NOTIFY, NULL },
    { "INFO",   DBG_INFO,   NULL },
    { "ERR",    DBG_ERR,    NULL },
    { "WARN",   DBG_WARN,   NULL },
    { "DEBUG",  DBG_DEBUG,  NULL },
    { "TRACE",  DBG_TRACE,  NULL },
    { "ALL",    DBG_ALL,    NULL }, // Don't move as it messes up DBG message
    { NULL,     0,          NULL }
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
    sim_printf ("boot     : %u",
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

UNIT mtp_unit [N_MTP_UNITS_MAX] = {
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
  [0 ... N_MTP_UNITS_MAX-1] = {
    UDATA (NULL, 0, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL
  }
#endif
};

static t_stat mtp_show_device_name (UNUSED FILE * st, UNIT * uptr,
                                    UNUSED int val, UNUSED const void * desc)
  {
    int n = (int) MTP_UNIT_IDX (uptr);
    if (n < 0 || n >= N_MTP_UNITS_MAX)
      return SCPE_ARG;
    sim_printf("name     : %s", mtp_state [n].device_name);
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
      MTAB_XTD | MTAB_VDV | \
      MTAB_NMO | MTAB_VALR,                 /* Mask               */
      0,                                    /* Match              */
      "NUNITS",                             /* Print string       */
      "NUNITS",                             /* Match string       */
      mtp_set_nunits,                       /* Validation routine */
      mtp_show_nunits,                      /* Display routine    */
      "Number of TAPE units in the system", /* Value descriptor   */
      NULL                                  /* Help               */
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_VALR,      /* Mask               */
      0,                                    /* Match              */
      "BOOT_DRIVE",                         /* Print string       */
      "BOOT_DRIVE",                         /* Match string       */
      mtp_set_boot_drive,                   /* Validation routine */
      mtp_show_boot_drive,                  /* Display routine    */
      "Select the boot drive",              /* Value descriptor   */
      NULL                                  /* Help               */
    },
    {
      MTAB_XTD | MTAB_VUN | \
      MTAB_VALR | MTAB_NC,                  /* Mask               */
      0,                                    /* Match              */
      "NAME",                               /* Print string       */
      "NAME",                               /* Match string       */
      mtp_set_device_name,                  /* Validation routine */
      mtp_show_device_name,                 /* Display routine    */
      "Set the device name",                /* Value descriptor   */
      NULL                                  /* Help               */
    },
    { 0, 0, NULL, NULL, NULL, NULL, NULL, NULL }
  };

static t_stat mtp_reset (UNUSED DEVICE * dptr)
  {
    return SCPE_OK;
  }

DEVICE mtp_dev =
  {
    "MTP",            /* Name                */
    mtp_unit,         /* Units               */
    NULL,             /* Registers           */
    mtp_mod,          /* Modifiers           */
    N_MTP_UNITS,      /* #units              */
    10,               /* Address radix       */
    31,               /* Address width       */
    1,                /* Address increment   */
    8,                /* Data radix          */
    9,                /* Data width          */
    NULL,             /* Examine routine     */
    NULL,             /* Deposit routine     */
    mtp_reset,        /* Reset routine       */
    NULL,             /* Boot routine        */
    NULL,             /* Attach routine      */
    NULL,             /* Detach routine      */
    NULL,             /* Context             */
    DEV_DEBUG,        /* Flags               */
    0,                /* Debug control flags */
    mt_dt,            /* Debug flag names    */
    NULL,             /* Memory size change  */
    NULL,             /* Logical name        */
    NULL,             /* Attach help         */
    NULL,             /* Help                */
    NULL,             /* Help context        */
    NULL,             /* Device description  */
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
static char tape_path_prefix [PATH_MAX+2];

// Multics RCP limits the volume name in the label to 32 characters
#define LABEL_MAX 32

#define N_MT_UNITS 1 // default

UNIT mt_unit [N_MT_UNITS_MAX] = {
    // CAC: Looking at SCP source, the only place UNIT_SEQ is used
    // by the "run" command's reset sequence; units that have UNIT_SEQ
    // set will be issued a rewind on reset.
    // Looking at the sim source again... It is used on several of the
    // run commands, including CONTINUE.
    // Turning UNIT_SEQ off.
    // XXX Should we rewind on reset? What is the actual behavior?
// Unit 0 is the controller
#ifdef NO_C_ELLIPSIS
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL }
#else
  [0 ... N_MT_UNITS_MAX-1] = {
    UDATA (NULL, UNIT_ATTABLE | /* UNIT_SEQ | */ UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL
  }
#endif
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
    if (tape_states[n].device_name[1] == 0)
      sim_printf("name     : default");
    else
      sim_printf("name     : %s", tape_states[n].device_name);
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
    sim_printf("TAPE DEFAULT_PATH: %s\n", tape_path_prefix);
    return SCPE_OK;
  }

typedef struct path_node PATH_ENTRY;

struct path_node
  {
      size_t prefix_len;
      char label_prefix[LABEL_MAX + 1];
      char dir[PATH_MAX + 1];
      PATH_ENTRY *next_entry;
  };

static PATH_ENTRY *search_list_head = NULL;
static PATH_ENTRY *search_list_tail = NULL;

static t_stat mt_set_tape_path (UNUSED UNIT * uptr, UNUSED int32 value,
                             const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;

    size_t len = strlen(cptr);

    // We check for length - (2 + max label length) to allow for the null, a possible '/' being added and the label file name being added
    if (len >= (sizeof(tape_path_prefix) - (LABEL_MAX + 2)))
      return SCPE_ARG;

    // If any paths have been added, we need to remove them now
    PATH_ENTRY *current_entry = search_list_head;
    while (current_entry != NULL)
      {
        PATH_ENTRY *old_entry = current_entry;
        current_entry = current_entry->next_entry;
        FREE(old_entry);
      }

    search_list_head = NULL;
    search_list_tail = NULL;

    // Verify the new default path exists
    DIR * dp;
    dp = opendir (cptr);
    if (! dp)
      {
        sim_warn ("\rInvalid '%s' ", cptr);
        perror ("opendir");
        sim_warn ("\r\n");
        return SCPE_ARG;
      }

    closedir(dp);

    // Save the new default path
    strncpy (tape_path_prefix, cptr, (sizeof(tape_path_prefix)-1));
    if (len > 0)
      {
        if (tape_path_prefix[len - 1] != '/')
          {
            tape_path_prefix[len++] = '/';
            tape_path_prefix[len] = 0;
          }
      }

    return SCPE_OK;
  }

static t_stat mt_add_tape_search_path(UNUSED UNIT * uptr, UNUSED int32 value,
                             const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;

    if (!tape_path_prefix[0])
      {
        sim_print("ERROR: DEFAULT_PATH must be set before ADD_PATH is used.\n");
        return SCPE_ARG;
      }

    size_t len = strlen(cptr);

    char buffer[len + 2];
    char prefix[len + 2];
    char dir[len + 2];

    strcpy(buffer, cptr);

    // Break up parameter into prefix and directory
    char *token = strtok(buffer, "=");
    if (token == NULL)
      {
        return SCPE_ARG;
      }

    strcpy(prefix, token);

    token = strtok(NULL, "=");
    if (token == NULL)
      {
        sim_print("ERROR: ADD_PATH parameter must be specified as [prefix]=[dir]\n");
        sim_print("   i.e. SET TAPE ADD_PATH=BK=./tapes/backups\n");
        return SCPE_ARG;
      }

    strcpy(dir, token);

    if (strtok(NULL, "=") != NULL)
      {
        return SCPE_ARG;
      }

    size_t prefix_len = strlen(prefix);
    if ((prefix_len > LABEL_MAX) || (prefix_len < 1))
      {
        return SCPE_ARG;
      }

    size_t dir_len = strlen(dir);

    // We check against PATH_MAX - 1 to account for possibly adding a slash at the end of the path
    if (dir_len > (PATH_MAX - 1))
      {
        return SCPE_ARG;
      }

    // Verify the new path to add exists
    DIR * dp;
    dp = opendir (dir);
    if (! dp)
      {
        sim_warn ("\rInvalid '%s' ", dir);
        perror ("opendir");
        sim_warn ("\r\n");
        return SCPE_ARG;
      }

    closedir(dp);

    PATH_ENTRY *new_entry = malloc(sizeof(PATH_ENTRY));
    if (!new_entry)
      {
        fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                 __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# ifdef SIGUSR2
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
# endif /* ifdef SIGUSR2 */
#endif /* if defined(USE_BACKTRACE) */
        abort();
      }
    new_entry->prefix_len = prefix_len;
    strcpy(new_entry->label_prefix, prefix);
    strcpy(new_entry->dir, dir);

    if (new_entry->dir[dir_len - 1] != '/')
      {
        new_entry->dir[dir_len++] = '/';
        new_entry->dir[dir_len] = 0;
      }

    new_entry->next_entry = NULL;
    if (search_list_tail == NULL)
      {
        search_list_head = new_entry;
        search_list_tail = new_entry;
      }
    else
      {
        search_list_tail->next_entry = new_entry;
        search_list_tail = new_entry;
      }

    return SCPE_OK;
  }

static t_stat mt_show_tape_search_paths(UNUSED FILE * st, UNUSED UNIT * uptr,
                                 UNUSED int val, UNUSED const void * desc)
  {
    sim_print("Tape directory search paths:\n");
    sim_printf("%-32s %s\n", "Prefix", "Directory");
    sim_printf("%-32s %s\n", "------", "---------");
    PATH_ENTRY *current_entry = search_list_head;
    while (current_entry != NULL)
      {
          sim_printf("%-32s %s\n", current_entry->label_prefix, current_entry->dir);
          current_entry = current_entry->next_entry;
      }

    sim_printf("%-32s %s\n", "[default]", tape_path_prefix);

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

t_stat signal_tape (uint tap_unit_idx, word8 status0, word8 status1) {

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
      //sim_printf ("%s %d %o\n", disk_filename, ro,  mt_unit [tap_unit_idx] . flags);
      //sim_printf ("special int %d %o\n", tap_unit_idx, mt_unit [tap_unit_idx] . flags);

  if (! sim_is_running)
    return SCPE_OK;

  uint ctlr_unit_idx = cables->tape_to_mtp[tap_unit_idx].ctlr_unit_idx;

// Sending interrupts to all ports causes error messages in syslog; the additional
// interrupts are "unexpected"
#if 0
    // Which port should the controller send the interrupt to? All of them...
    bool sent_one = false;
    for (uint ctlr_port_num = 0; ctlr_port_num < MAX_CTLR_PORTS; ctlr_port_num ++)
      if (cables->mtp_to_iom[ctlr_unit_idx][ctlr_port_num].in_use)
        {
          uint iom_unit_idx = cables->mtp_to_iom[ctlr_unit_idx][ctlr_port_num].iom_unit_idx;
          uint chan_num = cables->mtp_to_iom[ctlr_unit_idx][ctlr_port_num].chan_num;
          uint dev_code = cables->tape_to_mtp[tap_unit_idx].dev_code;

          send_special_interrupt (iom_unit_idx, chan_num, dev_code, 0, 020 /* tape drive to ready */);
          sent_one = true;
        }
    if (! sent_one)
      {
        sim_printf ("loadTape can't find controller; dropping interrupt\n");
        return SCPE_ARG;
      }

    return SCPE_OK;
#else
  for (uint ctlr_port_num = 0; ctlr_port_num < MAX_CTLR_PORTS; ctlr_port_num ++) {
    if (cables->mtp_to_iom[ctlr_unit_idx][ctlr_port_num].in_use) {
      uint iom_unit_idx = cables->mtp_to_iom[ctlr_unit_idx][ctlr_port_num].iom_unit_idx;
      uint chan_num = cables->mtp_to_iom[ctlr_unit_idx][ctlr_port_num].chan_num;
      uint dev_code = cables->tape_to_mtp[tap_unit_idx].dev_code;
      send_special_interrupt (iom_unit_idx, chan_num, dev_code, 0, 020 /* tape drive to ready */);
      return SCPE_OK;
    }
  }
  sim_warn ("loadTape can't find controller; dropping interrupt\n");
  return SCPE_ARG;
#endif
}

static t_stat tape_set_ready (UNIT * uptr, UNUSED int32 value,
                              UNUSED const char * cptr,
                              UNUSED void * desc)
  {
    long tape_unit_idx = MT_UNIT_NUM (uptr);
    if (tape_unit_idx >= N_MT_UNITS_MAX)
      {
        sim_debug (DBG_ERR, & tape_dev,
                   "Tape set ready: Invalid unit number %ld\n", (long) tape_unit_idx);
        sim_printf ("error: Invalid unit number %ld\n", (long) tape_unit_idx);
        return SCPE_ARG;
      }
    return signal_tape ((unsigned int) tape_unit_idx, 0, 020 /* tape drive to ready */);
  }

static MTAB mt_mod [] =
  {
#ifndef SPEED
    { UNIT_WATCH, UNIT_WATCH, "WATCH",   "WATCH",   NULL, NULL, NULL, NULL },
    { UNIT_WATCH, 0,          "NOWATCH", "NOWATCH", NULL, NULL, NULL, NULL },
#endif
    {
       MTAB_XTD | MTAB_VUN | \
       MTAB_NC,                                  /* Mask               */
      0,                                         /* Match              */
      NULL,                                      /* Print string       */
      "REWIND",                                  /* Match string       */
      mt_rewind,                                 /* Validation routine */
      NULL,                                      /* Display routine    */
      NULL,                                      /* Value descriptor   */
      NULL                                       /* Help               */
    },
    {
      MTAB_XTD | MTAB_VDV | \
      MTAB_NMO | MTAB_VALR,                      /* Mask               */
      0,                                         /* Match              */
      "NUNITS",                                  /* Print string       */
      "NUNITS",                                  /* Match string       */
      mt_set_nunits,                             /* Validation routine */
      mt_show_nunits,                            /* Display routine    */
      "Number of TAPE units in the system",      /* Value descriptor   */
      NULL                                       /* Help               */
    },
    {
      MTAB_XTD | MTAB_VUN | \
      MTAB_VALR | MTAB_NC,                       /* Mask               */
      0,                                         /* Match              */
      "NAME",                                    /* Print string       */
      "NAME",                                    /* Match string       */
      mt_set_device_name,                        /* Validation routine */
      mt_show_device_name,                       /* Display routine    */
      "Set the device name",                     /* Value descriptor   */
      NULL                                       /* Help               */
    },
    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO | \
      MTAB_VALR | MTAB_NC,                       /* Mask               */
      0,                                         /* Match              */
      "DEFAULT_PATH",                            /* Print string       */
      "DEFAULT_PATH",                            /* Match string       */
      mt_set_tape_path,                          /* Validation routine */
      mt_show_tape_path,                         /* Display routine    */
      "Set the default path to the directory containing tape images (also clear search paths)",
      NULL
    },
    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO | \
      MTAB_VALR | MTAB_NC,                       /* Mask               */
      0,                                         /* Match              */
      "ADD_PATH",                                /* Print string       */
      "ADD_PATH",                                /* Match string       */
      mt_add_tape_search_path,                   /* Validation routine */
      mt_show_tape_search_paths,                 /* Display routine    */
      "Add a search path for tape directories",  /* Value descriptor   */
      NULL                                       /* Help               */
    },
    {
      MTAB_XTD | MTAB_VUN,                       /* Mask               */
      0,                                         /* Match              */
      "CAPACITY",                                /* Print string       */
      "CAPACITY",                                /* Match string       */
      sim_tape_set_capac,                        /* Validation routine */
      sim_tape_show_capac,                       /* Display routine    */
      "Set the device capacity",                 /* Value descriptor   */
      NULL                                       /* Help               */
    },
    {
      MTAB_XTD | MTAB_VDV | \
      MTAB_NMO | MTAB_VALR,                      /* Mask               */
      0,                                         /* Match              */
      "CAPACITY_ALL",                            /* Print string       */
      "CAPACITY_ALL",                            /* Match string       */
      mt_set_capac,                              /* Validation routine */
      NULL,                                      /* Display routine    */
      "Set the tape capacity of all drives",     /* Value descriptor   */
      NULL                                       /* Help               */
    },
    {
      MTAB_XTD | MTAB_VUN | \
      MTAB_NMO | MTAB_VALR,                      /* Mask               */
      0,                                         /* Match              */
      "READY",                                   /* Print string       */
      "READY",                                   /* Match string       */
      tape_set_ready,                            /* Validation routine */
      NULL,                                      /* Display routine    */
      NULL,                                      /* Value descriptor   */
      NULL                                       /* Help string        */
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
    "TAPE",            /* Name                */
    mt_unit,           /* Units               */
    NULL,              /* Registers           */
    mt_mod,            /* Modifiers           */
    N_MT_UNITS,        /* #Units              */
    10,                /* Address radix       */
    31,                /* Address width       */
    1,                 /* Address increment   */
    8,                 /* Data radix          */
    9,                 /* Data width          */
    NULL,              /* Examine routine     */
    NULL,              /* Deposit routine     */
    mt_reset,          /* Reset routine       */
    NULL,              /* Boot routine        */
    &sim_tape_attach,  /* Attach routine      */
    &sim_tape_detach,  /* Detach routine      */
    NULL,              /* Context             */
    DEV_DEBUG,         /* Flags               */
    0,                 /* Debug control flags */
    mt_dt,             /* Debug flag names    */
    NULL,              /* Memory size change  */
    NULL,              /* Logical name        */
    NULL,              /* Attach help         */
    NULL,              /* Help                */
    NULL,              /* Help context        */
    NULL,              /* Device description  */
    NULL               /* End                 */
  };

static void deterimeFullTapeFileName(char * tapeFileName, char * buffer, int bufferLength)
  {
    // If no path prefixing, just return the given tape file name
    if (!tape_path_prefix[0])
      {
          strncpy(buffer, tapeFileName, (unsigned long)bufferLength);
          buffer[bufferLength - 1] = 0;
          return;
      }

    // Prefixing is in effect so now we need to search the additional path list for a prefix match
    PATH_ENTRY *current_entry = search_list_head;
    while (current_entry != NULL)
      {
        if (strncmp(current_entry->label_prefix, tapeFileName, current_entry->prefix_len) == 0)
          {
              break;
          }

        current_entry = current_entry->next_entry;
      }

    char *selected_path = tape_path_prefix;     // Start with the default path
    if (current_entry != NULL)
      {
        selected_path = current_entry->dir;
      }

    // Verify we won't overrun the output buffer
    if ((size_t) bufferLength < (strlen(selected_path) + strlen(tapeFileName) + 1))
      {
          // Bad news, we are going to overrun the buffer so
          // we just use as much of the tape file name as we can
          strncpy(buffer, tapeFileName, (unsigned long)bufferLength);
          buffer[bufferLength - 1] = 0;
          return;
      }

    // Everything will fit so construct the full tape file name and path
    int buffWrote;
    buffWrote = snprintf(buffer,
                    ((int)((strlen(selected_path)+strlen(tapeFileName))+1)),
                        "%s%s", selected_path, tapeFileName);
    if (buffWrote < 0)
      sim_warn("%s snprintf problem, returned %d\n", __func__, buffWrote);
  }

t_stat loadTape (uint driveNumber, char * tapeFilename, bool ro)
  {
    char full_tape_file_name[PATH_MAX + 1];

    if (ro)
      mt_unit [driveNumber] . flags |= MTUF_WRP;
    else
      mt_unit [driveNumber] . flags &= ~ MTUF_WRP;

    deterimeFullTapeFileName(tapeFilename, full_tape_file_name, (sizeof(full_tape_file_name)-1));

    sim_printf("loadTape Attaching drive %u to file %s\n", driveNumber, full_tape_file_name);
    t_stat stat = sim_tape_attach (& mt_unit [driveNumber], full_tape_file_name);
    if (stat != SCPE_OK)
      {
        sim_printf ("%s sim_tape_attach returned %d\n", __func__, stat);
        return stat;
      }
    return signal_tape (driveNumber, 0, 020 /* tape drive to ready */);
  }

t_stat unloadTape (uint driveNumber)
  {
    if (mt_unit [driveNumber] . flags & UNIT_ATT)
      {
        t_stat stat = sim_tape_detach (& mt_unit [driveNumber]);
        if (stat != SCPE_OK)
          {
            sim_warn ("%s sim_tape_detach returned %d\n", __func__, stat);
            return stat;
          }
      }
    return signal_tape (driveNumber, 0, 040 /* unload complete */);
  }

void mt_init(void)
  {
    memset(tape_states, 0, sizeof(tape_states));
    for (int i = 0; i < N_MT_UNITS_MAX; i ++)
      {
        mt_unit [i] . capac = 40000000;
      }
  }

void mt_exit (void) {
  // If any paths have been added, we need to remove them now
  PATH_ENTRY * current_entry = search_list_head;
  while (current_entry != NULL) {
    PATH_ENTRY * old_entry = current_entry;
    current_entry = current_entry->next_entry;
    FREE (old_entry);
  }
}

static iom_cmd_rc_t mtReadRecord (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {

// If a tape read IDCW has multiple DDCWs, are additional records read?

    iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan];
    UNIT * unitp = & mt_unit[devUnitIdx];
    struct tape_state * tape_statep = & tape_states[devUnitIdx];

    //enum { dataOK, noTape, tapeMark, tapeEOM, tapeError } tapeStatus;
    sim_debug (DBG_DEBUG, & tape_dev, "%s: Read %s record\n", __func__,
               tape_statep->is9 ? "9" : "binary");
    // We read the record into the tape controllers memory; the IOT will move it to core
    tape_statep->tbc = 0;
    if (! (unitp -> flags & UNIT_ATT))
      {
        p->stati = 04104;
        return IOM_CMD_ERROR;
      }
    t_stat rc = sim_tape_rdrecf (unitp, & tape_statep -> buf [0], & tape_statep -> tbc,
                               BUFSZ);
    sim_debug (DBG_DEBUG, & tape_dev, "%s: sim_tape_rdrecf returned %d, with tbc %d\n",
            __func__, rc, tape_statep -> tbc);
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
        //tapeStatus = tapeMark;
        return IOM_CMD_ERROR;
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
        //tapeStatus = tapeEOM;
        return IOM_CMD_PROCEED;
      }
    if (rc != 0)
      {
        sim_debug (DBG_ERR, & tape_dev,
                   "%s: Cannot read tape: %d - %s\n",
                   __func__, rc, simh_tape_msg (rc));
        sim_debug (DBG_ERR, & tape_dev,
                   "%s: Returning arbitrary error code\n",
                   __func__);
        p -> stati      = 05001; // BUG: arbitrary error code; config switch
        p -> chanStatus = chanStatParityErrPeriph;
        return IOM_CMD_ERROR;
      }
    p -> stati = 04000;
    if (sim_tape_wrp (unitp))
      p -> stati |= 1;
    tape_statep -> rec_num ++;

    tape_statep -> words_processed = 0;
    if (unitp->flags & UNIT_WATCH)
      sim_printf ("Tape %ld reads record %d\n",
                  (long) MT_UNIT_NUM (unitp), tape_statep -> rec_num);

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
    int rc2;
    for (i = 0; i < tally; i ++)
      {
        if (tape_statep -> is9)
          rc2 = extractASCII36FromBuffer (tape_statep -> buf,
                  tape_statep -> tbc, & tape_statep -> words_processed, buffer + i);
        else
          rc2 = extractWord36FromBuffer (tape_statep -> buf,
                  tape_statep -> tbc, & tape_statep -> words_processed, buffer + i);
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
// XXX This assumes that the tally was bigger than the record
    if (tape_statep -> is9)
      p -> charPos = tape_statep -> tbc % 4;
    else
      p -> charPos = (tape_statep -> tbc * 8) / 9 % 4;
    return IOM_CMD_PROCEED;
  }

static void mtReadCtrlMainMem (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {
    struct tape_state * tape_statep = & tape_states[devUnitIdx];
    word36 control;
    uint count;
    iom_indirect_data_service (iomUnitIdx, chan, & control, &count, false);
    if (count != 1)
      sim_warn ("%s: count %d not 1\n", __func__, count);
    tape_statep -> cntlrAddress = getbits36_16 (control, 0);
    tape_statep -> cntlrTally   = getbits36_16 (control, 16);
  }

static void mtInitRdMem (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan];

    uint tally = p -> DDCW_TALLY;
    if (tally != 04000)
      {
        sim_warn ("%s: tape controller read memory expected tally of 04000\n", __func__);
        p -> stati      = 04501;
        p -> chanStatus = chanStatIncorrectDCW;
        return;
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
//  8     2 fw_id bit (16),                                    /* Firmware identification */
//  9     2 fw_rev,                                            /* Firmware revision */
//          3 pad1 bit (4),
//          3 lrev (2) bit (4),                                /* Letter revision */
//          3 srev bit (4),                                    /* Sub revision */
// 10     2 as_date,                                           /* Assembly date */
//          3 month bit (8),
//          3 day bit (8),
// 11     2 pad2 (5) bit (16);

// For the 501
//  mpc_data.mpc_err_int_ctr_addr = 253;    /* 00FD */
//  mpc_data.mpc_err_data_reg_addr = 254;   /* 00FE */

    mem [charTableOS + 0]  = 4096;          // mem_sze
    mem [charTableOS + 1]  = 0;             // config_sw

    // Set the addresses to recognizable values
    mem [charTableOS + 2]  = 04000 + 0123;  // trace_tab_p
    mem [charTableOS + 3]  = 0;             // trace_tab_size
    mem [charTableOS + 4]  = 0;             // trace_tab_cur
    mem [charTableOS + 5]  = 04000 + 0234;  // mpc_stat
    mem [charTableOS + 6]  = 04000 + 0345;  // dev_stat
    mem [charTableOS + 7]  = 04000 + 0456;  // rev_l_tab
    mem [charTableOS + 8]  = 012345;        // fw_id

    // Set fw_rev to 0013; I thinks that will xlate as 'A3' (0x01 is A)
    mem [charTableOS + 9]  = 0x0013;        // fw_rev

    mem [charTableOS + 10] = 0x1025;        // as_date Oct 27.

    word36 buf [tally];
    // Make clang analyzer happy
    memset (buf, 0, sizeof (word36) * tally);
    for (uint i = 0; i < tally; i ++)
      {
        putbits36_18 (buf + i,  0, mem [i * 2]);
        putbits36_18 (buf + i, 18, mem [i * 2 + 1]);
      }
    iom_indirect_data_service (iomUnitIdx, chan, buf, & tally, true);
    p -> stati = 04000;
  }

static void mtWRCtrlRegs (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan];
    // Fake the indirect data service
    p->initiate = false;
    return;
  }

static void mtInitWrMem (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan];
    // Fake the indirect data service
    p->initiate = false;
    return;
  }

static void mtMTPWr (uint devUnitIdx, uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan];
    struct tape_state * tape_statep = & tape_states [devUnitIdx];

    word36 control;
    uint count;
    iom_indirect_data_service (iomUnitIdx, chan, & control, &count, false);
    if (count != 1)
      sim_warn ("%s: count %d not 1\n", __func__, count);
    tape_statep -> cntlrAddress = getbits36_16 (control, 0);
    tape_statep -> cntlrTally = getbits36_16 (control, 16);
    p -> stati = 04000;
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

// XXX This assumes that the tally was bigger than the record
    if (tape_statep -> is9)
      p -> charPos = tape_statep -> tbc % 4;
    else
      p -> charPos = (tape_statep -> tbc * 8) / 9 % 4;

    // Write buf to tape

    if (! (unitp -> flags & UNIT_ATT))
      return MTSE_UNATT;

    int ret = sim_tape_wrrecf (unitp, tape_statep -> buf, tape_statep -> tbc);
    sim_debug (DBG_DEBUG, & tape_dev, "%s: sim_tape_wrrecf returned %d, with tbc %d\n",
               __func__, ret, tape_statep -> tbc);

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
                   "%s: unit %d handler %06o\n", __func__, unit_idx, handler);
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
    p -> stati = 04000; //-V1048
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

iom_cmd_rc_t mt_iom_cmd (uint iomUnitIdx, uint chan) {
  iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
#ifdef TESTING
  if_sim_debug (DBG_TRACE, & tape_dev) dumpDCW (p->DCW, 0);
#endif
// The bootload read command does a read on drive 0; the controller
// recognizes (somehow) a special case for bootload and subs. in
// the boot drive unit set by the controller config. switches
// XXX But controller commands are directed to drive 0, so this
// logic is incorrect. If we just set the boot drive to 0, the
// system will just boot from 0, and ignore it thereafter.
// Although, the install process identifies tapa_00 as a device;
// check the survey code to make sure its not incorrectly
// reporting 0 as a valid device.

// Simplifying design decision: tapa_00 is hidden, always has the boot tape.

  uint ctlr_unit_idx = get_ctlr_idx (iomUnitIdx, chan);
  uint dev_code = p->IDCW_DEV_CODE;
#ifdef TESTING
  if_sim_debug (DBG_TRACE, & tape_dev) dumpDCW (p->DCW, 0);
#endif
  if (p->IDCW_DEV_CODE == 0)
    dev_code = mtp_state[ctlr_unit_idx].boot_drive;

  sim_debug (DBG_DEBUG, & tape_dev, "%s: Tape %c%02o_%02o\n", __func__, iomChar (iomUnitIdx), chan, dev_code);

  uint devUnitIdx = cables->mtp_to_tape[ctlr_unit_idx][dev_code].unit_idx;
  UNIT * unitp = & mt_unit [devUnitIdx];
  struct tape_state * tape_statep = & tape_states [devUnitIdx];

  iom_cmd_rc_t rc = IOM_CMD_PROCEED;

  // IDCW?
  if (IS_IDCW (p)) {
    // IDCW

    // According to poll_mpc.pl1
    // Note: XXX should probably be checking these...
    //  idcw.chan_cmd = "40"b3; /* Indicate special controller command */
    //  idcw.chan_cmd = "41"b3; /* Indicate special controller command */

    // The bootload read command does a read on drive 0; the controller
    // recognizes (somehow) a special case for bootload and subs. in
    // the boot drive unit set by the controller config. switches
    // XXX But controller commands are directed to drive 0, so this
    // logic is incorrect. If we just set the boot drive to 0, the
    // system will just boot from 0, and ignore it thereafter.
    // Although, the install process identifies tapa_00 as a device;
    // check the survey code to make sure its not incorrectly
    // reporting 0 as a valid device.

    tape_statep->io_mode = tape_no_mode;
    sim_debug (DBG_DEBUG, & tape_dev, "%s: IDCW_DEV_CMD %oo %d.\n", __func__, p->IDCW_DEV_CMD, p->IDCW_DEV_CMD);
    switch (p->IDCW_DEV_CMD) {

      case 000: { // CMD 00 Request status -- controller status, not tape drive
          if_sim_debug (DBG_TRACE, & tape_dev) {
            sim_printf ("// Tape Request Status\r\n");
          }
          // If special controller command, then command 0 is 'suspend'
          if (p->IDCW_CHAN_CMD == 040) {
            sim_debug (DBG_DEBUG, & tape_dev, "%s: controller suspend\n", __func__);
            send_special_interrupt (iomUnitIdx, chan, p->IDCW_DEV_CODE, 01, 0 /* suspended */);
            p->stati = 04000;
          } else {
            p->stati = 04000;
          }
          sim_debug (DBG_DEBUG, & tape_dev, "%s: Request status: %04o control %0o chan_cmd %02o\n", __func__,
                     p->stati, p->IDCW_CHAN_CTRL, p->IDCW_CHAN_CMD);
        }
        break;

// dcl  1 stat_buf aligned based (workp),             /* The IOI buffer segment */
//        2 idcw1 bit (36),                           /* Will be read controller main memory */
//        2 dcw1 bit (36),                            /* Addr=stat_buf.control, tally=1 */
//        2 idcw2 bit (36),                           /* Will be initiate read data transfer */
//        2 dcw2 bit (36),                            /* Address=stat_buf.mem, tally=rest of segment */

//        2 control,                                  /* Describes where data is in mpc */
//          3 addr bit (16) unal,                     /* Addr in mpc memory */
//          3 tally bit (16) unal,                    /* Count in mpc words */
//          3 fill bit (4) unal,
//        2 stats (0:83) bit (18) unal;               /* EURC statistics in ASCII */

//       2 mem (0:mpc_memory_size - 1) bit (18) unal; /* This is the mpc memory */

//    / * Build read or write (dev stat block) main memory dcw list */
//
//              idcwp = addr (buf.idcw1);             /* First IDCW */
//              buf.idcw1 = "0"b;
//
//              if OP = READ_MPC_MEM
//              then idcw.command = "02"b3;           /* Command is read controller main memory (ASCII) */
//              else idcw.command = "32"b3;           /* Command is write main memory (binary) */
//
//              idcw.code = "111"b;                   /* This makes it an IDCW */
//              idcw.control = "10"b;                 /* Set continue bit */
//              idcw.chan_cmd = "40"b3;               /* Indicate special controller command */
//              dcwp = addr (buf.dcw1);
//              buf.dcw1 = "0"b;
//              dcw.address = rel (addr (buf.control));           /* Get offset to control word */
//              dcw.tally = "000000000001"b;
//              idcwp = addr (buf.idcw2);             /* Second IDCW */
//              buf.idcw2 = "0"b;
//              idcw.code = "111"b;                   /* Code is 111 to make it an idcw */
//              idcw.chan_cmd = "40"b3;               /* Special controller command */
//              dcwp = addr (buf.dcw2);
//              buf.dcw2 = "0"b;
//              dcw.address = rel (addr (buf.mem));   /* Offset to core image */
//              dcw.tally = bit (bin (size (buf) - bin (rel (addr (buf.mem)), 18), 12));
//                                                    /* Rest of seg */
//
//              if OP = READ_MPC_MEM then do;
//                   idcw.command = "06"b3;           /* Command is initiate read data transfer */
//                   buf.addr = "0"b;                 /* Mpc address to start is 0 */
//                   buf.tally = bit (bin (mpc_memory_size, 16), 16);
//                   end;
//
//
// Control word:
//
//

//      case 001: Unassigned

      case 002:               // CMD 02 -- Read controller main memory (ASCII)
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("// Tape Read Controller Main Memory\r\n");
        }
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Read controller main memory\n", __func__);

        tape_statep->io_mode = tape_rd_ctlr;
        p->stati = 04000;
        break;

      case 003: // CMD 03 -- Read 9 Record
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("// Tape Read 9 Record\r\n");
        }
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Read 9 record\n", __func__);
        tape_statep->io_mode = tape_rd_9;
        tape_statep->is9 = true;
        if (! (unitp->flags & UNIT_ATT)) {
          p->stati = 04104;
          return IOM_CMD_ERROR;
        }
        p->stati = 04000;
        break;

//          case 004: Read BCD Record

      case 005: // CMD 05 -- Read Binary Record
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("// Tape Read Binary Record\r\n");
        }
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Read binary record\n", __func__);
        tape_statep->io_mode = tape_rd_bin;
        tape_statep->is9 = false;
        if (! (unitp->flags & UNIT_ATT)) {
          p->stati = 04104;
          return IOM_CMD_ERROR;
        }
        p->stati = 04000;
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
// I interpret that as the 16 bit memory being broken into 8 bit bytes, zero
// extended to 9 bits, and packed 4 to a word.

// From char_mpc_.pl1, assuming MTP501
//
//      mtc500_char init (0000000011100000b),                  /* Mtc500 characteristics table at 00E0 (hex) */

      case 006:               // CMD 06 -- initiate read data transfer
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("// Tape Initiate Read Data Transfer\r\n");
        }
        sim_debug (DBG_DEBUG, & tape_dev, "%s: initiate read data transfer\n", __func__);
        tape_statep->io_mode = tape_initiate_rd_mem;
        p->stati = 04000;
        break;

//      case 007: Reread Binary Record

//      case 010: Control Store Overlay

//      case 011: Main Memory Overlay

//      case 012: Unassigned

      case 013: // CMD 013 -- Write tape 9
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("// Tape Write 9 Record\r\n");
        }
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Write 9 record\n", __func__);
        tape_statep->io_mode = tape_wr_9;
        if (! (unitp->flags & UNIT_ATT)) {
          p->stati = 04104;
          return IOM_CMD_ERROR;
        }
        p->stati = 04000;
        break;

//      case 014: Write BCD Record

      case 015: // CMD 015 -- Write Binary Record
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("// Tape Write Binary Record\r\n");
        }
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Write binary record\n", __func__);
        tape_statep->io_mode = tape_wr_bin;
        if (! (unitp->flags & UNIT_ATT)) {
          p->stati = 04104;
          return IOM_CMD_ERROR;
        }
        p->stati = 04000;
        break;

      case 016:               // CMD 016 -- Write Control Registers
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("// Tape Write Control Registers\r\n");
        }
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Write Control Registers\n", __func__);
        tape_statep->io_mode = tape_wr_ctrl_regs;
        p->stati = 04000;
        break;

//      case 012: Unassigned

      case 020:  { // CMD 020 -- release controller
          if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape Release Controller\r\n"); }
          if (p->IDCW_CHAN_CMD == 040) { // If special controller command, then command 020 is 'release'
            sim_debug (DBG_DEBUG, & tape_dev, "%s: Release controller\n", __func__);
            p->stati = 04000; // have_status = 1
            sim_debug (DBG_DEBUG, & tape_dev, "%s: Release status: %04o control %0o chan_cmd %02o\n",
                       __func__, p->stati, p->IDCW_CHAN_CTRL, p->IDCW_CHAN_CMD);
            send_special_interrupt (iomUnitIdx, chan, p->IDCW_DEV_CODE, 02, 0 /* released */);
          } else {
            p->stati = 04501;
            p->chanStatus = chanStatIncorrectDCW;
            sim_warn ("%s: Unknown command %02o\n", __func__, p->IDCW_DEV_CMD);
          }
        }
        break;

//      case 021: Unassigned

//      case 022: Unassigned

//      case 023: Unassigned

//      case 024: Read EBCDIC Record

//      case 025: Read ASCII/EBCDIC Record

//      case 026: Read Control Registers

//      case 027: Read ASCII Record

//      case 030: Unassigned

//      case 031: Diagnostic Mode Control

// 032: MTP write main memory (binary) (poll_mpc.pl1)

      case 032: // CMD 032 -- MTP write main memory (binary)
                //    (poll_mpc.pl1); used to clear device stats.
        if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape Write Main Memory\r\n"); }
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Write controller main memory\n", __func__);
        tape_statep->io_mode = tape_MTP_wr;
        p->stati = 04000;
        break;

//      case 033: Unassigned

//      case 034: Write EBCDIC Record

//      case 035: Write ASCII/EBCDIC Record

//      case 036: Unassigned

//      case 037: Write ASCII Record

      case 040:               // CMD 040 -- Reset Status
        if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape Reset Status\r\n"); }
        sim_debug (DBG_DEBUG, & tape_dev, "%s Reset status\n", __func__);
        p->stati = 04000;
        p->isRead = false;
        // T&D probing
        if (dev_code == 077) {
          p->stati = 04502; // invalid device code
          return IOM_CMD_DISCONNECT;
        }
        if (sim_tape_wrp (unitp))
          p->stati |= 1;
        if (sim_tape_bot (unitp))
          p->stati |= 2;
        //if (sim_tape_eom (unitp))
          //p->stati |= 0340;
        break;

      case 041:              // CMD 041 -- Set 6250 cpi.
        if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape Set 6259 CPI\r\n"); }
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Set 6250 cpi\n", __func__);
        p->stati = 04000;
        if (sim_tape_wrp (unitp))
          p->stati |= 1;
        if (sim_tape_bot (unitp))
          p->stati |= 2;
        //if (sim_tape_eom (unitp))
          //p->stati |= 0340;
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Set 800 bpi\n", __func__);
        break;

      case 042:              // CMD 042 -- Set 800 bpi.
      case 060:              // CMD 060 -- Set 800 bpi.
        if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape Set 800 BPI\r\n"); }
        p->stati = 04000;
        if (sim_tape_wrp (unitp))
          p->stati |= 1;
        if (sim_tape_bot (unitp))
          p->stati |= 2;
        //if (sim_tape_eom (unitp))
          //p->stati |= 0340;
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Set 800 bpi\n", __func__);
        break;

      case 043:              // CMD 043 -- Set 556 bpi.
      case 061:              // CMD 061 -- Set 556 bpi.
        if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape Set 556 BPI\r\n"); }
        p->stati = 04000;
        if (sim_tape_wrp (unitp))
          p->stati |= 1;
        if (sim_tape_bot (unitp))
          p->stati |= 2;
        //if (sim_tape_eom (unitp))
          //p->stati |= 0340;
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Set 556 bpi\n", __func__);
        break;

      case 044: { // 044 -- Forward Skip One Record
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("// Tape Forward Skip One Record\r\n");
          sim_printf ("//    pos before %d\r\n", unitp->pos);
        }
#ifdef TESTING
        hdbgNote ("tape", "Tape Forward Skip One Record pos before %d", unitp->pos);
#endif
        if (! (unitp->flags & UNIT_ATT)) {
          p->stati = 04104;
          return IOM_CMD_ERROR;
        }
        uint tally = p->IDCW_COUNT;
        if (tally == 0) {
          sim_debug (DBG_DEBUG, & tape_dev, "%s: Tally of zero interpreted as 64\n", __func__);
          tally = 64;
        }
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Forward skip record %d\n", __func__, tally);

// sim_tape_sprecsf incorrectly stops on tape marks;
#if 0
                uint32 skipped;
                t_stat ret = sim_tape_sprecsf (unitp, tally, & skipped);
#else
        uint32 skipped = 0;
        t_stat ret = MTSE_OK;
        while (skipped < tally) {
          ret = sim_tape_sprecf (unitp, & tape_statep->tbc);
          if (ret != MTSE_OK && ret != MTSE_TMK)
            break;
          skipped = skipped + 1;
        }
#endif
        if (ret != MTSE_OK && ret != MTSE_TMK && ret != MTSE_EOM) {
          break;
        }
        if (skipped != tally) {
          sim_warn ("skipped %d != tally %d\n", skipped, tally);
        }

        tape_statep->rec_num += (int) skipped;
        if (unitp->flags & UNIT_WATCH)
          sim_printf ("Tape %ld forward skips to record %d\n", (long) MT_UNIT_NUM (unitp), tape_statep->rec_num);

        p->tallyResidue = (word12) (tally - skipped);

        p->stati = 04000;
        if (sim_tape_wrp (unitp))
          p->stati |= 1;
        if (sim_tape_bot (unitp))
          p->stati |= 2;
        //if (sim_tape_eom (unitp))
          //p->stati |= 0340;
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("//    pos after %d\r\n", unitp->pos);
        }
#ifdef TESTING
        hdbgNote ("tape", "Tape Forward Skip One Record pos after %d", unitp->pos);
#endif
      }
      break;

    case 045: { // CMD 045 -- Forward Skip One File
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("// Tape Forward Skip One File\r\n");
          sim_printf ("//    pos before %d\r\n", unitp->pos);
        }
#ifdef TESTING
        hdbgNote ("tape", "Tape Forward Skip One File pos before %d", unitp->pos);
#endif
        sim_debug (DBG_DEBUG, & tape_dev, "%s:: Forward Skip File\n", __func__);
        if (! (unitp->flags & UNIT_ATT)) {
          p->stati = 04104;
          return IOM_CMD_ERROR;
        }
        uint tally = 1;

        uint32 skipped, recsskipped;
        t_stat ret = sim_tape_spfilebyrecf (unitp, tally, & skipped, & recsskipped, false);
        if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// sim_tape_spfilebyrecf ret %d skipped %d recsskipped %d\r\n", ret, skipped, recsskipped); }
        if (ret != MTSE_OK && ret != MTSE_TMK && ret != MTSE_LEOT) {
          sim_warn ("sim_tape_spfilebyrecf returned %d\n", ret);
          break;
        }
        if (skipped != tally) {
          sim_warn ("skipped %d != tally %d\n", skipped, tally);
        }

        tape_statep->rec_num += (int) recsskipped;
        if (unitp->flags & UNIT_WATCH)
          sim_printf ("Tape %ld forward skips to record %d\n", (long) MT_UNIT_NUM (unitp), tape_statep->rec_num);

        p->tallyResidue = (word12) (tally - skipped);
        sim_debug (DBG_NOTIFY, & tape_dev, "%s: Forward space %d files\n", __func__, tally);

        p->stati = 04000;
        if (sim_tape_wrp (unitp))
          p->stati |= 1;
        if (sim_tape_bot (unitp))
          p->stati |= 2;
        //if (sim_tape_eom (unitp))
          //p->stati |= 0340;
        }
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("//    pos after %d\r\n", unitp->pos);
        }
#ifdef TESTING
        hdbgNote ("tape", "Tape Forward Skip One File pos after %d", unitp->pos);
#endif
        break;

      case 046: { // CMD 046 -- Backspace One Record
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("// Tape Backspace One Record\r\n");
          sim_printf ("//    pos before %d\r\n", unitp->pos);
        }
#ifdef TESTING
        hdbgNote ("tape", "Tape Backspace One Record pos before %d", unitp->pos);
#endif
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Backspace Record\n", __func__);
        if (! (unitp->flags & UNIT_ATT)) {
          p->stati = 04104;
          return IOM_CMD_ERROR;
        }

        uint tally = p->IDCW_COUNT;
        if (tally == 0) {
          sim_debug (DBG_DEBUG, & tape_dev, "%s: Tally of zero interpreted as 64\n", __func__);
          tally = 64;
        }

        sim_debug (DBG_DEBUG, & tape_dev, "%s: Backspace record tally %d\n", __func__, tally);

        // sim_tape_sprecsr stumbles on tape marks; do our own version...
#if 0
        uint32 skipped;
        t_stat ret = sim_tape_sprecsr (unitp, tally, & skipped);
        if (ret != MTSE_OK && ret != MTSE_TMK) {
          sim_printf ("sim_tape_sprecsr returned %d\n", ret);
          break;
        }
#else
        uint32 skipped = 0;
        while (skipped < tally) {
          t_stat ret = sim_tape_sprecr (unitp, & tape_statep->tbc);
          if (ret != MTSE_OK && ret != MTSE_TMK)
            break;
          skipped ++;
        }
#endif
        if (skipped != tally) {
          sim_warn ("skipped %d != tally %d\n", skipped, tally);
        }
        tape_statep->rec_num -= (int) skipped;
        if (unitp->flags & UNIT_WATCH)
          sim_printf ("Tape %ld skip back to record %d\n", (long) MT_UNIT_NUM (unitp), tape_statep->rec_num);

        p->tallyResidue = (word12) (tally - skipped);

        sim_debug (DBG_NOTIFY, & tape_dev, "%s: Backspace %d records\n", __func__, skipped);

        p->stati = 04000;
        if (sim_tape_wrp (unitp))
          p->stati |= 1;
        if (sim_tape_bot (unitp))
          p->stati |= 2;
        //if (sim_tape_eom (unitp))
          //p->stati |= 0340;
        }
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("//    pos after %d\r\n", unitp->pos);
        }
#ifdef TESTING
        hdbgNote ("tape", "Tape Backspace One Record pos after %d", unitp->pos);
#endif
        break;

      case 047: { // CMD 047 -- Backspace One File
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("// Tape Backspace One File\r\n");
          sim_printf ("//    pos before %d\r\n", unitp->pos);
        }
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Backspace File\n", __func__);
#ifdef TESTING
        hdbgNote ("tape", "Tape Backspace One File pos before %d", unitp->pos);
#endif
        if (! (unitp->flags & UNIT_ATT)) {
          p->stati = 04104;
          return IOM_CMD_ERROR;
        }
        uint tally = 1;
#if 0
        if (tally != 1) {
          sim_debug (DBG_DEBUG, & tape_dev, "%s: Back space file: setting tally %d to 1\n", __func__, tally);
          tally = 1;
        }

        sim_debug (DBG_DEBUG, & tape_dev, "%s: Backspace file tally %d\n", __func__, tally);

        int nbs = 0;

        while (tally) {
          t_stat ret = sim_tape_sprecr (unitp, & tape_statep->tbc);
          //sim_printf ("ret %d\n", ret);
          if (ret != MTSE_OK && ret != MTSE_TMK)
            break;
          if (tape_statep->rec_num > 0)
            tape_statep->rec_num --;
          nbs ++;
        }
#else
        uint32 skipped, recsskipped;
        t_stat ret = sim_tape_spfilebyrecr (unitp, tally, & skipped, & recsskipped);
        if (ret != MTSE_OK && ret != MTSE_TMK && ret != MTSE_BOT) {
          sim_warn ("sim_tape_spfilebyrecr returned %d\n", ret);
          break;
        }
        if (skipped != tally) {
          sim_warn ("skipped %d != tally %d\n", skipped, tally);
        }

        tape_statep->rec_num -= (int) recsskipped;
        if (unitp->flags & UNIT_WATCH)
          sim_printf ("Tape %ld backward skips to record %d\n", (long) MT_UNIT_NUM (unitp), tape_statep->rec_num);

        p->tallyResidue = (word12) (tally - skipped);
        sim_debug (DBG_NOTIFY, & tape_dev, "%s: Backspace %d records\n", __func__, tally);
#endif

        p->stati = 04000;
        if (sim_tape_wrp (unitp))
          p->stati |= 1;
        if (sim_tape_bot (unitp))
          p->stati |= 2;
        //if (sim_tape_eom (unitp))
          //p->stati |= 0340;
        }
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("//    pos after %d\r\n", unitp->pos);
        }
#ifdef TESTING
        hdbgNote ("tape", "Tape Backspace One File pos after %d", unitp->pos);
#endif
        break;

      case 050: {              // CMD 050 -- Request device status
          if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape Request Device Status\r\n"); }
          p->stati = 04000;
          if (sim_tape_wrp (unitp))
            p->stati |= 1;
          if (sim_tape_bot (unitp))
            p->stati |= 2;
          //if (sim_tape_eom (unitp))
            //p->stati |= 0340;
          sim_debug (DBG_DEBUG, & tape_dev, "%s: Request device status: %o\n", __func__, p->stati);
        }
        break;

// Okay, this is convoluted. Multics locates the console by sending CMD 051
// to devices in the PCW, with the understanding that only the console
// device will "respond", whatever that means.
// But, bootload_tape_label checks for controller firmware loaded
// ("intelligence") by sending a 051 in a IDCW.
// Since it's difficult here to test for PCW/IDCW, assume that the PCW case
// has been filtered out at a higher level
      case 051: {              // CMD 051 -- Reset device status
          if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape Reset Device Status\r\n"); }
          if (p->isPCW) {
            p->stati = 04501; // cmd reject, invalid opcode
            p->chanStatus = chanStatIncorrectDCW;
            return IOM_CMD_ERROR;
          }
          p->stati = 04000;
          if (sim_tape_wrp (unitp))
            p->stati |= 1;
          if (sim_tape_bot (unitp))
            p->stati |= 2;
          //if (sim_tape_eom (unitp))
            //p->stati |= 0340;
          sim_debug (DBG_DEBUG, & tape_dev, "%s: Reset device status: %o\n", __func__, p->stati);
        }
        break;

//      case 052: Unassigned

//      case 053: Unassigned

//      case 054: Erase

      case 055: // CMD 055 -- Write EOF (tape mark);
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("// Tape Write EOF\r\n");
          sim_printf ("//    pos before %d\r\n", unitp->pos);
        }
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Write tape mark\n", __func__);
#ifdef TESTING
        hdbgNote ("tape", "Tape Write EOF pos before %d", unitp->pos);
#endif

        if (! (unitp->flags & UNIT_ATT)) {
          p->stati = 04104;
          return IOM_CMD_ERROR;
        }
        int ret;
        ret = sim_tape_wrtmk (unitp);
        sim_debug (DBG_DEBUG, & tape_dev, "%s: returned %d\n", __func__, ret);
        if (unitp->io_flush)
          unitp->io_flush (unitp);                              /* flush buffered data */
        if (ret != 0) {
          if (ret == MTSE_EOM) {
            sim_debug (DBG_NOTIFY, & tape_dev, "%s: EOM: %s\n", __func__, simh_tape_msg (ret));
            p->stati = 04340; // EOT file mark
            sim_warn ("%s: Wrote tape mark with EOM.\n", __func__);
            break;
          }
          sim_warn ("%s: Cannot write tape mark: %d - %s\n", __func__, ret, simh_tape_msg(ret));
          sim_warn ("%s: Returning arbitrary error code\n", __func__);
          p->stati = 05001; // BUG: arbitrary error code; config switch
          p->chanStatus = chanStatParityErrPeriph;
          break;
        }

        sim_tape_wreom (unitp);
        if (unitp->io_flush)
          unitp->io_flush (unitp);                              /* flush buffered data */

        tape_statep->rec_num ++;
        if (unitp->flags & UNIT_WATCH)
          sim_printf ("Tape %ld writes tape mark %ld\n", (long) MT_UNIT_NUM (unitp), (long) tape_statep->rec_num);

        p->stati = 04000;
        if (sim_tape_eot (unitp))
          p->stati = 04340;

        sim_debug (DBG_INFO, & tape_dev, "%s: Wrote tape mark; status %04o\n", __func__, p->stati);
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("//    pos after %d\r\n", unitp->pos);
        }
#ifdef TESTING
        hdbgNote ("tape", "Tape Write EOF pos after %d", unitp->pos);
#endif
        break;

//      case 056: Unassigned

      case 057:               // CMD 057 -- Survey devices
        if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape Survey Devices\r\n"); }
        sim_debug (DBG_DEBUG, & tape_dev, "%s: survey_devices\n", __func__);
        tape_statep->io_mode = tape_survey;
        p->stati = 04000;
        break;

//      case 060: Set 800 bpi; see case 042:

//      case 061: Set 556 bpi; see case 043:

//      case 062: Set File Protect

      case 063:              // CMD 063 -- Set File Permit.
        if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape Set File Permit\r\n"); }
        sim_debug (DBG_WARN, & tape_dev, "%s: Set file permit\n", __func__);
        p->stati = 04000;
        if (sim_tape_wrp (unitp))
          p->stati |= 1;
        if (sim_tape_bot (unitp))
          p->stati |= 2;
        break;

      case 064:              // CMD 064 -- Set 200 bpi.
        if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape Set 200 BPI\r\n"); }
        p->stati = 04000;
        if (sim_tape_wrp (unitp))
          p->stati |= 1;
        if (sim_tape_bot (unitp))
          p->stati |= 2;
        //if (sim_tape_eom (unitp))
          //p->stati |= 0340;
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Set 200 bpi\n", __func__);
        break;

      case 065:              // CMD 064 -- Set 1600 CPI
        if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape Set 1600 CPI\r\n"); }
        p->stati = 04000;
        if (sim_tape_wrp (unitp))
          p->stati |= 1;
        if (sim_tape_bot (unitp))
          p->stati |= 2;
        //if (sim_tape_eom (unitp))
          //p->stati |= 0340;
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Set 1600 CPI\n", __func__);
        break;

//      case 066: reserve device

//      case 067: release device

      case 070:              // CMD 070 -- Rewind.
        if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape Rewind\r\n"); }
        sim_debug (DBG_DEBUG, & tape_dev, "%s: Rewind\n", __func__);
        if (! (unitp->flags & UNIT_ATT)) {
          p->stati = 04104;
          return IOM_CMD_ERROR;
        }
        sim_tape_rewind (unitp);

        tape_statep->rec_num = 0;
        if (unitp->flags & UNIT_WATCH)
          sim_printf ("Tape %ld rewinds\n", (long) MT_UNIT_NUM (unitp));

        p->stati = 04000;
        if (sim_tape_wrp (unitp))
          p->stati |= 1;
        if (sim_tape_bot (unitp))
          p->stati |= 2;
        //if (sim_tape_eom (unitp))
          //p->stati |= 0340;
        send_special_interrupt (iomUnitIdx, chan, dev_code, 0, 0100 /* rewind complete */);
        break;

//      case 071: Unassigned

      case 072:              // CMD 072 -- Rewind/Unload.
        if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape Rewind/Unload\r\n"); }
        if ((unitp->flags & UNIT_ATT)) {
          if (unitp->flags & UNIT_WATCH)
            sim_printf ("Tape %ld unloads\n", (long) MT_UNIT_NUM (unitp));
          sim_debug (DBG_DEBUG, & tape_dev, "%s: Rewind/unload\n", __func__);
          sim_tape_detach (unitp);
        }
        p->stati = 04000;
        // Sending this confuses RCP. When requesting a tape mount, it
        // first sends an unload command, and then waits for the operator
        // to mount the tape and press ready. This special interrupt
        // is being seen as an operator created event and causes RCP
        // to request the operator to re-ready the drive.
        //send_special_interrupt (iomUnitIdx, chan, dev_code, 0, 0040 /* unload complete */);
        break;

//      case 073: data security erase

//      case 074: Unassigned

//      case 075:  tape load

//      case 076: Unassigned

//      case 077: set density

      default:
        if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape Unknown %02o\r\n", p->IDCW_DEV_CMD); }
        p->stati = 04501;
        p->chanStatus = chanStatIncorrectDCW;
        sim_warn ("%s: tape unrecognized device command  %02o\n", __func__, p->IDCW_DEV_CMD);
        return IOM_CMD_ERROR;

    } // switch IDCW_DEV_CMD

    sim_debug (DBG_DEBUG, & tape_dev, "%s: stati %04o\n", __func__, p->stati);
    return rc;
  } // if IDCW

  // Not IDCW; TDCW are captured in IOM, so must be IOTD, IOTP or IOTNP
  switch (tape_statep->io_mode) {
    case tape_no_mode:
// It appears that in some cases Multics builds a dcw list from a generic "single IDCW plus optional DDCW" template.
// That template sets the IDCW channel command to "record", regardless of whether or not the instruction
// needs an DDCW. In particular, disk_ctl pings the disk by sending a "Reset status" with "record" and a apparently
// uninitialized IOTD. The payload channel will send the IOTD because of the "record"; the Reset Status command left
// IO mode at "no mode" so we don't know what to do with it. Since this appears benign, we will assume that the
// original H/W ignored, and so shall we.
      //sim_warn ("%s: Unexpected IOTx\n", __func__);
      //return IOM_CMD_ERROR;
      break;

    case tape_rd_9:
    case tape_rd_bin: {
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("// Tape IOT Read\r\n");
          sim_printf ("//    pos before %d\r\n", unitp->pos);
        }
#ifdef TESTING
        hdbgNote ("tape", "Tape IOT Read pos before %d", unitp->pos);
#endif
        int rc = mtReadRecord (devUnitIdx, iomUnitIdx, chan);
        if (rc)
          return IOM_CMD_ERROR;
        }
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("//    pos after %d\r\n", unitp->pos);
        }
#ifdef TESTING
        hdbgNote ("tape", "Tape IOT Read pos after %d", unitp->pos);
#endif
        break;

    case tape_wr_9:
    case tape_wr_bin: {
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("// Tape IOT Write\r\n");
          sim_printf ("//    pos before %d\r\n", unitp->pos);
        }
#ifdef TESTING
        hdbgNote ("tape", "Tape IOT write pos before %d", unitp->pos);
#endif
        int rc = mtWriteRecord (devUnitIdx, iomUnitIdx, chan);
        if_sim_debug (DBG_TRACE, & tape_dev) {
          sim_printf ("//    pos after %d\r\n", unitp->pos);
        }
#ifdef TESTING
        hdbgNote ("tape", "Tape IOT write pos after %d", unitp->pos);
#endif
        if (rc)
          return IOM_CMD_ERROR;
      }
      break;

    case tape_rd_ctlr:
      if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape IOT Read Memory\r\n"); }
      mtReadCtrlMainMem (devUnitIdx, iomUnitIdx, chan);
      break;

    case tape_initiate_rd_mem:
      if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape IOT Write Memory\r\n"); }
      mtInitRdMem (devUnitIdx, iomUnitIdx, chan);
      break;

    case tape_initiate_wr_mem:
      if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape IOT Initiate Write\r\n"); }
      mtInitWrMem (devUnitIdx, iomUnitIdx, chan);
      break;

    case tape_MTP_wr:
      if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape IOT MTP Write\r\n"); }
      mtMTPWr (devUnitIdx, iomUnitIdx, chan);
      break;

    case tape_wr_ctrl_regs:
      if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape IOT Wr Ctrl Regs\r\n"); }
      mtWRCtrlRegs (devUnitIdx, iomUnitIdx, chan);
      break;

    case tape_survey:
      if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape IOT Survey\r\n"); }
      surveyDevices (iomUnitIdx, chan);
      break;

    default:
      if_sim_debug (DBG_TRACE, & tape_dev) { sim_printf ("// Tape IOT unknown %d\r\n", tape_statep->io_mode); }
      sim_warn ("%s: Unrecognized io_mode %d\n", __func__, tape_statep->io_mode);
      return IOM_CMD_ERROR;
  }

  return rc;
} // mt_iom_cmd

// 031 read statistics
//  idcw.chan_cmd = "41"b3;  /* Indicate special controller command */
// 006 initiate read data transfer
// 032 write main memory (binary)
// 016 initiate write data transfer
// 000 suspend controller
// 020 release controller

static const char *simh_tape_msg(int code)
{
    // WARNING: Only selected tape routines return private tape codes
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
        return "Unknown tape error";
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

#ifndef QUIET_UNUSED
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
#endif
