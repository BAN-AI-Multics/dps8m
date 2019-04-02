//
/*
 Copyright (c) 2007-2013 Michael Mondy
 Copyright 2012-2016 by Harry Reed
 Copyright 2019 by Charles Anthony

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

#include "dps8.h"
#include "dps8_ipc.h"
#include "dps8_iom.h"


#define IPCD_UNIT_IDX(uptr) ((uptr) - ipcd_unit)
#define N_IPCD_UNITS 1 // default
#define IPCT_UNIT_IDX(uptr) ((uptr) - ipcd_unit)
#define N_IPCT_UNITS 1 // default

struct ipc_state
  {
    char device_name [MAX_DEV_NAME_LEN];
  };

static struct ipc_state ipcd_states [N_IPCD_UNITS_MAX];
static struct ipc_state ipct_states [N_IPCT_UNITS_MAX];

UNIT ipcd_unit [N_IPCD_UNITS_MAX] =
  {
    [0 ... N_IPCD_UNITS_MAX-1] =
      {
        UDATA (NULL, 0, 0),
        0, 0, 0, 0, 0, NULL, NULL, NULL, NULL
      }
  };

static t_stat ipcd_show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr,
                               UNUSED int val, UNUSED const void * desc)
  {
    sim_printf("Number of IPCD units in system is %d\n", ipcd_dev.numunits);
    return SCPE_OK;
  }

static t_stat ipcd_set_nunits (UNUSED UNIT * uptr, UNUSED int32 value,
                              const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    int n = atoi (cptr);
    if (n < 0 || n > N_DSK_UNITS_MAX)
      return SCPE_ARG;
    ipcd_dev.numunits = (uint32) n;
    return SCPE_OK;
  }

static t_stat ipcd_show_device_name (UNUSED FILE * st, UNIT * uptr, 
                                    UNUSED int val, UNUSED const void * desc)
  {
    int n = (int) IPCD_UNIT_IDX (uptr);
    if (n < 0 || n >= N_IPCD_UNITS_MAX)
      return SCPE_ARG;
    sim_printf("Controller device name is %s\n", ipcd_states [n].device_name);
    return SCPE_OK;
  }

static t_stat ipcd_set_device_name (UNIT * uptr, UNUSED int32 value, 
                                   const char * cptr, UNUSED void * desc)
  {
    int n = (int) IPCD_UNIT_IDX (uptr);
    if (n < 0 || n >= N_IPCD_UNITS_MAX)
      return SCPE_ARG;
    if (cptr)
      {
        strncpy (ipcd_states[n].device_name, cptr, MAX_DEV_NAME_LEN-1);
        ipcd_states[n].device_name[MAX_DEV_NAME_LEN-1] = 0;
      }
    else
      ipcd_states[n].device_name[0] = 0;
    return SCPE_OK;
  }

static MTAB ipcd_mod [] =
  {
    {
      MTAB_dev_value, /* mask */
      0,            /* match */
      "NUNITS",     /* print string */
      "NUNITS",         /* match string */
      ipcd_set_nunits, /* validation routine */
      ipcd_show_nunits, /* display routine */
      "Number of DISK units in the system", /* value descriptor */
      NULL // Help
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_VALR | MTAB_NC, /* mask */
      0,            /* match */
      "NAME",     /* print string */
      "NAME",         /* match string */
      ipcd_set_device_name, /* validation routine */
      ipcd_show_device_name, /* display routine */
      "Set the device name", /* value descriptor */
      NULL          // help
    },
    MTAB_eol
  };

DEVICE ipcd_dev =
   {
    "IPCD",       /*  name */
    ipcd_unit,    /* units */
    NULL,         /* registers */
    ipcd_mod,     /* modifiers */
    N_IPCD_UNITS, /* #units */
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

// Route the cmd to the device
int ipcd_iom_cmd (uint iom_unit_idx, uint chan)
  {
#if 0
    // Retrive the device code
    iom_chan_data_t * p = & iom_chan_data [iom_unit_idx] [chan];
    // Is it an IDCW?
    if (p->DCW_18_20_CP != 7)
      {
        sim_printf ("%s expected IDCW\n", __func__);
        return IOM_CMD_ERROR;
      }
    uint dev_code = p->IDCW_DEV_CODE;
    // What kind of device is this channel attached to?
    enum ctlr_type_e ctlr_type = cables->iom_to_ctlr[iom_unit_idx][chan].ctlr_type;
    iom_cmt_t * cmd;
    if (ctlr_type == CTLR_T_IPCD)
      {
        iom_cmd_t * cmd =  cables->ctlr_to_dsk[unit_idx][dev_code].iom_cmd;
      }
    else if (ctlr_type == CTLR_T_IPCT)
      {
        iom_cmd_t * cmd =  cables->ctlr_to_tap[unit_idx][dev_code].iom_cmd;
      }
    uint unit_idx = cables->iom_to_ctlr[iomUnitIdx][chan].ctlr_unit_idx;
#endif
    return IOM_CMD_ERROR;
  }

UNIT ipct_unit [N_IPCT_UNITS_MAX] =
  {
    [0 ... N_IPCT_UNITS_MAX-1] =
      {
        UDATA (NULL, 0, 0),
        0, 0, 0, 0, 0, NULL, NULL, NULL, NULL
      }
  };

static t_stat ipct_show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr,
                               UNUSED int val, UNUSED const void * desc)
  {
    sim_printf("Number of IPCT units in system is %d\n", ipct_dev.numunits);
    return SCPE_OK;
  }

static t_stat ipct_set_nunits (UNUSED UNIT * uptr, UNUSED int32 value,
                              const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    int n = atoi (cptr);
    if (n < 0 || n > N_DSK_UNITS_MAX)
      return SCPE_ARG;
    ipct_dev.numunits = (uint32) n;
    return SCPE_OK;
  }

static t_stat ipct_show_device_name (UNUSED FILE * st, UNIT * uptr, 
                                    UNUSED int val, UNUSED const void * desc)
  {
    int n = (int) IPCT_UNIT_IDX (uptr);
    if (n < 0 || n >= N_IPCT_UNITS_MAX)
      return SCPE_ARG;
    sim_printf("Controller device name is %s\n", ipct_states [n].device_name);
    return SCPE_OK;
  }

static t_stat ipct_set_device_name (UNIT * uptr, UNUSED int32 value, 
                                   const char * cptr, UNUSED void * desc)
  {
    int n = (int) IPCT_UNIT_IDX (uptr);
    if (n < 0 || n >= N_IPCT_UNITS_MAX)
      return SCPE_ARG;
    if (cptr)
      {
        strncpy (ipct_states[n].device_name, cptr, MAX_DEV_NAME_LEN-1);
        ipct_states[n].device_name[MAX_DEV_NAME_LEN-1] = 0;
      }
    else
      ipct_states[n].device_name[0] = 0;
    return SCPE_OK;
  }

static MTAB ipct_mod [] =
  {
    {
      MTAB_dev_value, /* mask */
      0,            /* match */
      "NUNITS",     /* print string */
      "NUNITS",         /* match string */
      ipct_set_nunits, /* validation routine */
      ipct_show_nunits, /* display routine */
      "Number of DISK units in the system", /* value descriptor */
      NULL // Help
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_VALR | MTAB_NC, /* mask */
      0,            /* match */
      "NAME",     /* print string */
      "NAME",         /* match string */
      ipct_set_device_name, /* validation routine */
      ipct_show_device_name, /* display routine */
      "Set the device name", /* value descriptor */
      NULL          // help
    },
    MTAB_eol
  };

DEVICE ipct_dev =
   {
    "IPCT",       /*  name */
    ipct_unit,    /* units */
    NULL,         /* registers */
    ipct_mod,     /* modifiers */
    N_IPCT_UNITS, /* #units */
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

// Route the cmd to the device
int ipct_iom_cmd (uint iom_unit_idx, uint chan)
  {
#if 0
    // Retrive the device code
    iom_chan_data_t * p = & iom_chan_data [iom_unit_idx] [chan];
    // Is it an IDCW?
    if (p->DCW_18_20_CP != 7)
      {
        sim_printf ("%s expected IDCW\n", __func__);
        return IOM_CMD_ERROR;
      }
    uint dev_code = p->IDCW_DEV_CODE;
    // What kind of device is this channel attached to?
    enum ctlr_type_e ctlr_type = cables->iom_to_ctlr[iom_unit_idx][chan].ctlr_type;
    iom_cmt_t * cmd;
    if (ctlr_type == CTLR_T_IPCT_DISK)
      {
        iom_cmd_t * cmd =  cables->ctlr_to_dsk[unit_idx][dev_code].iom_cmd;
      }
    else if (ctlr_type == CTLR_T_IPCT_TAPE)
      {
        iom_cmd_t * cmd =  cables->ctlr_to_tap[unit_idx][dev_code].iom_cmd;
      }
    uint unit_idx = cables->iom_to_ctlr[iomUnitIdx][chan].ctlr_unit_idx;
#endif
    return IOM_CMD_ERROR;
  }

