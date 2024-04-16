/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 336ce0b0-f62d-11ec-873c-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2015-2018 Charles Anthony
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdint.h>

#include "dps8.h"
#include "dps8_iom.h"
#include "dps8_absi.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_utils.h"

#include "udplib.h"

#undef FREE
#define FREE(p) do  \
  {                 \
    free((p));      \
    (p) = NULL;     \
  } while(0)

#if defined(WITH_ABSI_DEV)
# define DBG_CTR 1

static struct absi_state
  {
    char device_name [MAX_DEV_NAME_LEN];
    int link;
  } absi_state [N_ABSI_UNITS_MAX];

# define N_ABSI_UNITS 1 // default

# define UNIT_FLAGS ( UNIT_FIX | UNIT_ATTABLE | UNIT_ROABLE | UNIT_DISABLE | \
                     UNIT_IDLE )
UNIT absi_unit[N_ABSI_UNITS_MAX] =
  {
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL}
  };

# define ABSI_UNIT_IDX(uptr) ((uptr) - absi_unit)

static DEBTAB absi_dt[] =
  {
    { "NOTIFY", DBG_NOTIFY, NULL },
    { "INFO",   DBG_INFO,   NULL },
    { "ERR",    DBG_ERR,    NULL },
    { "WARN",   DBG_WARN,   NULL },
    { "DEBUG",  DBG_DEBUG,  NULL },
    { "ALL",    DBG_ALL,    NULL }, // Don't move as it messes up DBG message
    { NULL,     0,          NULL }
  };

static t_stat absi_show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr,
                                UNUSED int val, UNUSED const void * desc)
  {
    sim_printf ("Number of ABSI units in system is %d\n", absi_dev.numunits);
    return SCPE_OK;
  }

static t_stat absi_set_nunits (UNUSED UNIT * uptr, UNUSED int32 value,
                               const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    int n = atoi (cptr);
    if (n < 1 || n > N_ABSI_UNITS_MAX)
      return SCPE_ARG;
    absi_dev.numunits = (uint32) n;
    return SCPE_OK;
  }

static t_stat absi_show_device_name (UNUSED FILE * st, UNIT * uptr,
                                    UNUSED int val, UNUSED const void * desc)
  {
    int n = (int) ABSI_UNIT_IDX (uptr);
    if (n < 0 || n >= N_ABSI_UNITS_MAX)
      return SCPE_ARG;
    if (absi_state[n].device_name[1] != 0)
      sim_printf("name     : %s", absi_state[n].device_name);
      else
        sim_printf("name     : ABSI%d", n);
    return SCPE_OK;
  }

static t_stat absi_set_device_name (UNIT * uptr, UNUSED int32 value,
                                   const char * cptr, UNUSED void * desc)
  {
    int n = (int) ABSI_UNIT_IDX (uptr);
    if (n < 0 || n >= N_ABSI_UNITS_MAX)
      return SCPE_ARG;
    if (cptr)
      {
        strncpy (absi_state[n].device_name, cptr, MAX_DEV_NAME_LEN-1);
        absi_state[n].device_name[MAX_DEV_NAME_LEN-1] = 0;
      }
    else
      absi_state[n].device_name[0] = 0;
    return SCPE_OK;
  }

# define UNIT_WATCH UNIT_V_UF

static MTAB absi_mod[] =
  {
# if !defined(SPEED)
    { UNIT_WATCH, 1, "WATCH",   "WATCH",   0, 0, NULL, NULL },
    { UNIT_WATCH, 0, "NOWATCH", "NOWATCH", 0, 0, NULL, NULL },
# endif /* if !defined(SPEED) */
    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO | MTAB_VALR,  /* Mask               */
      0,                                           /* Match              */
      "NUNITS",                                    /* Print string       */
      "NUNITS",                                    /* Match string       */
      absi_set_nunits,                             /* Validation routine */
      absi_show_nunits,                            /* Display routine    */
      "Number of ABSI units in the system",        /* Value descriptor   */
      NULL                                         /* Help               */
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_VALR | MTAB_NC,   /* Mask               */
      0,                                           /* Match              */
      "NAME",                                      /* Print string       */
      "NAME",                                      /* Match string       */
      absi_set_device_name,                        /* Validation routine */
      absi_show_device_name,                       /* Display routine    */
      "Set the device name",                       /* Value descriptor   */
      NULL                                         /* Help               */
    },
    MTAB_eol
  };

static t_stat absi_reset (UNUSED DEVICE * dptr)
  {
    //absiResetRX (0);
    //absiResetTX (0);
    return SCPE_OK;
  }

static t_stat absiAttach (UNIT * uptr, const char * cptr)
  {
    if (! cptr)
      return SCPE_ARG;
    int unitno = (int) (uptr - absi_unit);

    //    ATTACH HIn llll:w.x.y.z:rrrr - connect via UDP to a remote

    t_stat ret;
    char * pfn;
    //uint16 imp = 0; // we only support a single attachment to a single IMP

    // If we're already attached, then detach ...
    if ((uptr->flags & UNIT_ATT) != 0)
      detach_unit (uptr);

    // Make a copy of the "file name" argument.  udp_create() actually modifies
    // the string buffer we give it, so we make a copy now so we'll have
    // something to display in the "SHOW HIn ..." command.
    pfn = (char *) calloc ((CBUFSIZE + 1), sizeof (char));
    if (pfn == NULL)
      return SCPE_MEM;
    strncpy (pfn, cptr, CBUFSIZE);

    // Create the UDP connection.
    ret = udp_create (cptr, & absi_state[unitno].link);
    if (ret != SCPE_OK)
      {
        FREE (pfn);
        return ret;
      }

    uptr->flags |= UNIT_ATT;
    uptr->filename = pfn;
    return SCPE_OK;
  }

// Detach (connect) ...
static t_stat absiDetach (UNIT * uptr)
  {
    int unitno = (int) (uptr - absi_unit);
    t_stat ret;
    if ((uptr->flags & UNIT_ATT) == 0)
      return SCPE_OK;
    if (absi_state[unitno].link == NOLINK)
      return SCPE_OK;

    ret = udp_release (absi_state[unitno].link);
    if (ret != SCPE_OK)
      return ret;
    absi_state[unitno].link = NOLINK;
    uptr->flags &= ~ (unsigned int) UNIT_ATT;
    FREE (uptr->filename);
    return SCPE_OK;
  }

DEVICE absi_dev = {
    "ABSI",       /* Name                */
    absi_unit,    /* Units               */
    NULL,         /* Registers           */
    absi_mod,     /* Modifiers           */
    N_ABSI_UNITS, /* #units              */
    10,           /* Address radix       */
    24,           /* Address width       */
    1,            /* Address increment   */
    8,            /* Data radix          */
    36,           /* Data width          */
    NULL,         /* Examine             */
    NULL,         /* Deposit             */
    absi_reset,   /* Reset               */
    NULL,         /* Boot                */
    absiAttach,   /* Attach              */
    absiDetach,   /* Detach              */
    NULL,         /* Context             */
    DEV_DEBUG,    /* Flags               */
    0,            /* Debug control flags */
    absi_dt,      /* Debug flag names    */
    NULL,         /* Memory size change  */
    NULL,         /* Logical name        */
    NULL,         /* Help                */
    NULL,         /* Attach help         */
    NULL,         /* Attach context      */
    NULL,         /* Description         */
    NULL          /* End                 */
};

/*
 * absi_init()
 */

// Once-only initialization

void absi_init (void)
  {
    (void)memset (absi_state, 0, sizeof (absi_state));
    for (int i = 0; i < N_ABSI_UNITS_MAX; i ++)
      absi_state[i].link = NOLINK;
  }

static iom_cmd_rc_t absi_cmd (uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = &iom_chan_data[iomUnitIdx][chan];
// sim_printf ("absi_cmd CHAN_CMD %o DEV_CODE %o DEV_CMD %o COUNT %o\n",
//p->IDCW_CHAN_CMD, p->IDCW_DEV_CODE, p->IDCW_DEV_CMD, p->IDCW_COUNT);
    sim_debug (DBG_TRACE, & absi_dev,
               "absi_cmd CHAN_CMD %o DEV_CODE %o DEV_CMD %o COUNT %o\n",
               p->IDCW_CHAN_CMD, p->IDCW_DEV_CODE, p->IDCW_DEV_CMD,
               p->IDCW_COUNT);

    // Not IDCW?
    if (IS_NOT_IDCW (p))
      {
        sim_warn ("%s: Unexpected IOTx\n", __func__);
        return IOM_CMD_ERROR;
      }

    switch (p->IDCW_DEV_CMD)
      {
        case 000: // CMD 00 Request status
          {
            p->stati = 04000;
sim_printf ("absi request status\n");
          }
          break;

        case 001: // CMD 01 Read
          {
            p->stati = 04000;
sim_printf ("absi read\n");
          }
          break;

        case 011: // CMD 11 Write
          {
            p->stati = 04000;
sim_printf ("absi write\n");
          }
          break;

        case 020: // CMD 20 Host switch down
          {
            p->stati = 04000;
sim_printf ("absi host switch down\n");
          }
          break;

        case 040: // CMD 40 Reset status
          {
            p->stati = 04000;
sim_printf ("absi reset status\n");
          }
          break;

        case 060: // CMD 60 Host switch up
          {
            p->stati = 04000;
sim_printf ("absi host switch up\n");
          }
          break;

        default:
          {
            if (p->IDCW_DEV_CMD != 051) // ignore bootload console probe
              sim_warn ("%s: ABSI unrecognized device command  %02o\n", __func__, p->IDCW_DEV_CMD);
            p->stati = 04501; // cmd reject, invalid opcode
            p->chanStatus = chanStatIncorrectDCW;
          }
          return IOM_CMD_ERROR;
      }

    if (p->IDCW_CHAN_CMD == 0)
      return IOM_CMD_DISCONNECT; // Don't do DCW list
    return IOM_CMD_PROCEED;
  }

//  1 == ignored command
//  0 == ok
// -1 == problem
iom_cmd_rc_t absi_iom_cmd (uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan];

    // Is it an IDCW?
    if (IS_IDCW (p))
      {
        return absi_cmd (iomUnitIdx, chan);
      }
    sim_printf ("%s expected IDCW\n", __func__);
    return IOM_CMD_ERROR;
  }

void absi_process_event (void)
  {
# define psz 17000
    uint16_t pkt[psz];
    for (uint32 unit = 0; unit < absi_dev.numunits; unit ++)
      {
        if (absi_state[unit].link == NOLINK)
          continue;
        //int sz = udp_receive ((int) unit, pkt, psz);
        int sz = udp_receive (absi_state[unit].link, pkt, psz);
        if (sz < 0)
          {
            (void)fprintf (stderr, "udp_receive failed\n");
          }
        else if (sz == 0)
          {
            //(void)fprintf (stderr, "udp_receive 0\n");
          }
        else
          {
            for (int i = 0; i < sz; i ++)
              {
                (void)fprintf (stderr, "  %06o  %04x  ", pkt[i], pkt[i]);
                for (int b = 0; b < 16; b ++)
                  (void)fprintf (stderr, "%c", pkt[i] & (1 << (16 - b)) ? '1' : '0');
                (void)fprintf (stderr, "\n");
              }
            // Send a NOP reply
            //int16_t reply[2] = 0x0040
            int rc = udp_send (absi_state[unit].link, pkt, (uint16_t) sz,
                               PFLG_FINAL);
            if (rc < 0)
              {
                (void)fprintf (stderr, "udp_send failed\n");
              }
          }
      }
  }
#endif /* if defined(WITH_ABSI_DEV) */
