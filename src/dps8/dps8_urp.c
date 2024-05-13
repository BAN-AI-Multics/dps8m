/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 0aeda9d7-f62f-11ec-a611-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
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

#include "dps8.h"
#include "dps8_iom.h"
#include "dps8_urp.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_utils.h"

#define DBG_CTR 1

//-- // XXX We use this where we assume there is only one unit
//-- #define ASSUME0 0
//--

#define N_PRU_UNITS 1 // default

static struct urpState
  {
    enum urpMode
      {
        urpNoMode, urpSetDiag, urpInitRdData
      } ioMode;
    char deviceName [MAX_DEV_NAME_LEN];
  } urpState [N_URP_UNITS_MAX];

#define UNIT_FLAGS ( UNIT_FIX | UNIT_ATTABLE | UNIT_ROABLE | UNIT_DISABLE | \
                     UNIT_IDLE )
UNIT urp_unit [N_URP_UNITS_MAX] = {
#if defined(NO_C_ELLIPSIS)
  { UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },
  { UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL }
#else
  [0 ... N_URP_UNITS_MAX-1] = {
    UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL
  }
#endif /* if defined(NO_C_ELLIPSIS) */
};

#define URPUNIT_NUM(uptr) ((uptr) - urp_unit)

static DEBTAB urp_dt [] =
  {
    { "NOTIFY", DBG_NOTIFY, NULL },
    {   "INFO",   DBG_INFO, NULL },
    {    "ERR",    DBG_ERR, NULL },
    {   "WARN",   DBG_WARN, NULL },
    {  "DEBUG",  DBG_DEBUG, NULL },
    {  "TRACE",  DBG_TRACE, NULL },
    {    "ALL",    DBG_ALL, NULL }, // Don't move as it messes up DBG message
    {     NULL,          0, NULL }
  };

static t_stat urpShowUnits (UNUSED FILE * st, UNUSED UNIT * uptr, UNUSED int val, UNUSED const void * desc)
  {
    sim_printf("Number of URP units in system is %d\n", urp_dev.numunits);
    return SCPE_OK;
  }

static t_stat urpSetUnits (UNUSED UNIT * uptr, UNUSED int32 value, const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    int n = atoi (cptr);
    if (n < 1 || n > N_URP_UNITS_MAX)
      return SCPE_ARG;
    urp_dev.numunits = (uint32) n;
    return SCPE_OK;
  }

static t_stat urpShowDeviceName (UNUSED FILE * st, UNIT * uptr, UNUSED int val, UNUSED const void * desc)
  {
    int n = (int) URPUNIT_NUM (uptr);
    if (n < 0 || n >= N_URP_UNITS_MAX)
      return SCPE_ARG;
    sim_printf ("name     : %s", urpState[n].deviceName);
    return SCPE_OK;
  }

static t_stat urpSetDeviceName (UNUSED UNIT * uptr, UNUSED int32 value, UNUSED const char * cptr, UNUSED void * desc)
  {
    int n = (int) URPUNIT_NUM (uptr);
    if (n < 0 || n >= N_URP_UNITS_MAX)
      return SCPE_ARG;
    if (cptr)
      {
        strncpy (urpState[n].deviceName, cptr, MAX_DEV_NAME_LEN - 1);
        urpState[n].deviceName[MAX_DEV_NAME_LEN - 1] = 0;
      }
    else
      urpState[n].deviceName [0] = 0;
    return SCPE_OK;
  }

#define UNIT_WATCH UNIT_V_UF

static MTAB urp_mod [] =
  {
#if !defined(SPEED)
    { UNIT_WATCH, 1, "WATCH", "WATCH", 0, 0, NULL, NULL },
    { UNIT_WATCH, 0, "NOWATCH", "NOWATCH", 0, 0, NULL, NULL },
#endif /* if !defined(SPEED) */
    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO | MTAB_VALR, /* Mask               */
      0,                                          /* Match              */
      "NUNITS",                                   /* Print string       */
      "NUNITS",                                   /* Match string       */
      urpSetUnits,                                /* Validation routine */
      urpShowUnits,                               /* Display routine    */
      "Number of URP units in the system",        /* Value descriptor   */
      NULL                                        /* Help               */
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_VALR | MTAB_NC,  /* Mask               */
      0,                                          /* Match              */
      "NAME",                                     /* Print string       */
      "NAME",                                     /* Match string       */
      urpSetDeviceName,                           /* Validation routine */
      urpShowDeviceName,                          /* Display routine    */
      "Set the device name",                      /* Value descriptor   */
      NULL                                        /* Help               */
    },

    { 0, 0, NULL, NULL, 0, 0, NULL, NULL }
  };

static t_stat urpReset (UNUSED DEVICE * dptr)
  {
    return SCPE_OK;
  }

DEVICE urp_dev = {
    "URP",        /* Name                */
    urp_unit,     /* Unit                */
    NULL,         /* Registers           */
    urp_mod,      /* Modifiers           */
    N_PRU_UNITS,  /* Number of units     */
    10,           /* Address radix       */
    24,           /* Address width       */
    1,            /* Address increment   */
    8,            /* Data radix          */
    36,           /* Data width          */
    NULL,         /* Examine             */
    NULL,         /* Deposit             */
    urpReset,     /* Reset               */
    NULL,         /* Boot                */
    NULL,         /* Attach              */
    NULL,         /* Detach              */
    NULL,         /* Context             */
    DEV_DEBUG,    /* Flags               */
    0,            /* Debug control flags */
    urp_dt,       /* Debug flag names    */
    NULL,         /* Memory size change  */
    NULL,         /* Logical name        */
    NULL,         /* Help                */
    NULL,         /* Attach help         */
    NULL,         /* Attach context      */
    NULL,         /* Description         */
    NULL          /* End                 */
};

/*
 * urp_init()
 */

// Once-only initialization

void urp_init (void)
  {
    (void)memset (urpState, 0, sizeof (urpState));
  }

static iom_cmd_rc_t urpCmd (uint iomUnitIdx, uint chan) {
  iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
#if defined(TESTING)
  if_sim_debug (DBG_TRACE, & urp_dev) dumpDCW (p->DCW, 0);
#endif /* if defined(TESTING) */
 uint ctlrUnitIdx = get_ctlr_idx (iomUnitIdx, chan);
 uint devUnitIdx  = cables->urp_to_urd[ctlrUnitIdx][p->IDCW_DEV_CODE].unit_idx;
 //UNIT * unitp = & urp_unit [devUnitIdx];
 //int urp_unit_num = (int) URPUNIT_NUM (unitp);
 struct urpState * statep = & urpState[devUnitIdx];

 // IDCW?
 if (IS_IDCW (p)) {
    // IDCW
    statep->ioMode = urpNoMode;

    switch (p->IDCW_DEV_CMD) {
      case 000: // CMD 00 Request status
        if_sim_debug (DBG_TRACE, & urp_dev) { sim_printf ("// URP Request Status\r\n"); }
        sim_debug (DBG_DEBUG, & urp_dev, "%s: Request Status\n", __func__);
        p->stati = 04000;
        break;

      case 006: // CMD 005 Initiate read data xfer (load_mpc.pl1)
        if_sim_debug (DBG_TRACE, & urp_dev) { sim_printf ("// URP Initiate Read Data Xfer\r\n"); }
        sim_debug (DBG_DEBUG, & urp_dev, "%s: Initiate Read Data Xfer\n", __func__);
        statep->ioMode = urpInitRdData;
        p->stati = 04000;
        break;

// 011 punch binary
// 031 set diagnostic mode

      case 031: // CMD 031 Set Diagnostic Mode (load_mpc.pl1)
        if_sim_debug (DBG_TRACE, & urp_dev) { sim_printf ("// URP Set Diagnostic Mode\r\n"); }
        sim_debug (DBG_DEBUG, & urp_dev, "%s: Set Diagnostic Mode\n", __func__);
        statep->ioMode = urpSetDiag;
        p->stati = 04000;
        break;

      case 040: // CMD 40 Reset status
        if_sim_debug (DBG_TRACE, & urp_dev) { sim_printf ("// URP Reset Status\r\n"); }
        sim_debug (DBG_DEBUG, & urp_dev, "%s: Reset Status\n", __func__);
        p->stati = 04000;
        p->isRead = false;
        break;

      default:
        if_sim_debug (DBG_TRACE, & urp_dev) { sim_printf ("// URP unknown command %o\r\n", p->IDCW_DEV_CMD); }
        p->stati = 04501; // cmd reject, invalid opcode
        p->chanStatus = chanStatIncorrectDCW;
        if (p->IDCW_DEV_CMD != 051) // ignore bootload console probe
          sim_warn ("%s: URP unrecognized device command  %02o\n", __func__, p->IDCW_DEV_CMD);
        return IOM_CMD_ERROR;
    } // switch IDCW_DEV_CMD

    sim_debug (DBG_DEBUG, & urp_dev, "%s: stati %04o\n", __func__, p->stati);
    return IOM_CMD_PROCEED;
  } // if IDCW

  // Not IDCW; TDCW are captured in IOM, so must be IOTD, IOTP or IOTNP
  switch (statep->ioMode) {
    case urpNoMode:
      if_sim_debug (DBG_TRACE, & urp_dev) { sim_printf ("// URP IOT no mode\r\n"); }
      //sim_printf ("%s: Unexpected IOTx\n", __func__);
      //sim_warn ("%s: Unexpected IOTx\n", __func__);
      //return IOM_CMD_ERROR;
      break;

    case urpSetDiag:
      if_sim_debug (DBG_TRACE, & urp_dev) { sim_printf ("// URP IOT Set Diag\r\n"); }
      // We don't use the DDCW, so just pretend we do. BUG
      p->stati = 04000;
      break;

    case urpInitRdData:
      if_sim_debug (DBG_TRACE, & urp_dev) { sim_printf ("// URP IOT Init Rd Data\r\n"); }
      // We don't use the DDCW, so just pretend we do. BUG
      p->stati = 04000;
      break;

     default:
      if_sim_debug (DBG_TRACE, & urp_dev) { sim_printf ("// URP IOT unknown %d\r\n", statep->ioMode); }
      sim_warn ("%s: Unrecognized ioMode %d\n", __func__, statep->ioMode);
      return IOM_CMD_ERROR;
  }
  return IOM_CMD_PROCEED;
}

iom_cmd_rc_t urp_iom_cmd (uint iomUnitIdx, uint chan) {
  iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
  uint devCode = p->IDCW_DEV_CODE;
  if (devCode == 0)
    return urpCmd (iomUnitIdx, chan);
  uint urpUnitIdx = cables->iom_to_ctlr[iomUnitIdx][chan].ctlr_unit_idx;
  iom_cmd_t * cmd = cables->urp_to_urd[urpUnitIdx][devCode].iom_cmd;
  if (! cmd) {
    //sim_warn ("URP can't find device handler\n");
    //return IOM_CMD_ERROR;
    p->stati = 04502; //-V536  // invalid device code
    return IOM_CMD_DISCONNECT;
  }
  return cmd (iomUnitIdx, chan);
}
