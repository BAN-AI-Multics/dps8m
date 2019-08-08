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
//  dps8_crdpun.c
//  dps8
//
//  Created by Harry Reed on 6/16/13.
//  Copyright (c) 2013 Harry Reed. All rights reserved.
//

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#include "dps8.h"
#include "dps8_iom.h"
#include "dps8_crdpun.h"
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
 
/*
 Copyright (c) 2007-2013 Michael Mondy
 
 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at http://example.org/project/LICENSE.
 */


#define N_PUN_UNITS 1 // default

static t_stat pun_reset (DEVICE * dptr);
static t_stat pun_show_nunits (FILE *st, UNIT *uptr, int val, const void *desc);
static t_stat pun_set_nunits (UNIT * uptr, int32 value, const char * cptr, void * desc);
static t_stat pun_show_device_name (FILE *st, UNIT *uptr, int val, const void *desc);
static t_stat pun_set_device_name (UNIT * uptr, int32 value, const char * cptr, void * desc);

#define UNIT_FLAGS ( UNIT_FIX | UNIT_ATTABLE | UNIT_ROABLE | UNIT_DISABLE | \
                     UNIT_IDLE )
UNIT pun_unit [N_PUN_UNITS_MAX] =
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
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL}
  };

#define PUN_UNIT_NUM(uptr) ((uptr) - pun_unit)

static DEBTAB pun_dt [] =
  {
    { "NOTIFY", DBG_NOTIFY, NULL },
    { "INFO", DBG_INFO, NULL },
    { "ERR", DBG_ERR, NULL },
    { "WARN", DBG_WARN, NULL },
    { "DEBUG", DBG_DEBUG, NULL },
    { "ALL", DBG_ALL, NULL }, // don't move as it messes up DBG message
    { NULL, 0, NULL }
  };

#define UNIT_WATCH UNIT_V_UF

static MTAB pun_mod [] =
  {
    { UNIT_WATCH, 1, "WATCH", "WATCH", 0, 0, NULL, NULL },
    { UNIT_WATCH, 0, "NOWATCH", "NOWATCH", 0, 0, NULL, NULL },
    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO | MTAB_VALR, /* mask */
      0,            /* match */
      "NUNITS",     /* print string */
      "NUNITS",         /* match string */
      pun_set_nunits, /* validation routine */
      pun_show_nunits, /* display routine */
      "Number of PUN units in the system", /* value descriptor */
      NULL // Help
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_VALR | MTAB_NC, /* mask */
      0,            /* match */
      "NAME",     /* print string */
      "NAME",         /* match string */
      pun_set_device_name, /* validation routine */
      pun_show_device_name, /* display routine */
      "Select the boot drive", /* value descriptor */
      NULL          // help
    },

    { 0, 0, NULL, NULL, 0, 0, NULL, NULL }
  };


DEVICE pun_dev = {
    "PUN",       /*  name */
    pun_unit,    /* units */
    NULL,         /* registers */
    pun_mod,     /* modifiers */
    N_PUN_UNITS, /* #units */
    10,           /* address radix */
    24,           /* address width */
    1,            /* address increment */
    8,            /* data radix */
    36,           /* data width */
    NULL,         /* examine */
    NULL,         /* deposit */ 
    pun_reset,   /* reset */
    NULL,         /* boot */
    NULL,         /* attach */
    NULL,         /* detach */
    NULL,         /* context */
    DEV_DEBUG,    /* flags */
    0,            /* debug control flags */
    pun_dt,      /* debug flag names */
    NULL,         /* memory size change */
    NULL,         /* logical name */
    NULL,         // help
    NULL,         // attach help
    NULL,         // attach context
    NULL,         // description
    NULL
};

static struct pun_state
  {
    char device_name [MAX_DEV_NAME_LEN];
    int punfile; // fd
    // bool cachedBanner;
    bool sawEOD;
  } pun_state [N_PUN_UNITS_MAX];

/*
 * pun_init()
 *
 */

// Once-only initialization

void pun_init (void)
  {
    memset (pun_state, 0, sizeof (pun_state));
    for (int i = 0; i < N_PUN_UNITS_MAX; i ++)
      pun_state [i] . punfile = -1;
  }

static t_stat pun_reset (UNUSED DEVICE * dptr)
  {
#if 0
    for (uint i = 0; i < dptr -> numunits; i ++)
      {
        // sim_pun_reset (& pun_unit [i]);
        //sim_cancel (& pun_unit [i]);
      }
#endif
    return SCPE_OK;
  }




static void openPunFile (int pun_unit_num, UNUSED word36 * buffer, UNUSED uint tally)
  {
    if (pun_state [pun_unit_num] . punfile != -1)
      return;

// Some brave person needs to parse the binary card image to extract
// the request # and user id out of it.

//    000500000000
//    777777777777
//    777777770000
//    000000000000
//    000000000000
//    000000002000
//    200037002000
//    200000000000
//    270025372500
//    251035000000
//    000021162521
//    252125213716
//    000000003716
//    012101210121
//    371600000000
//    371623212521
//    212137160000
//    000021002537
//    250025103700
//    000000003500
//    260024002400
//    370000000000
//    000000000000
//    000000000000
//    000077777777
//    777777777777
//    000000050000
//  
//     *****                                                                 *****  
//     *****         *****  *****  *****  *   *  *****  *****  *****         *****  
//     *****           *        *      *  *   *  *   *      *  *   *         *****  
//     *****           *    *****   ****  *   *  * * *   ****  *****         *****  
//     *****           *    *          *  *   *  **  *      *   *  *         *****  
//     *****           *    *****  *****  *****  *****  *****  *   *         *****  
//     *****                                                                 *****  
//     *****                 *      ***    ***    ***    *                   *****  
//     *****                 * *   *   *  *   *  *   *   * *                 *****  
//  *  *****                 *     *   *  *   *  *   *   *                   ***** *
//     *****                 *     *   *  *   *  *   *   *                   *****  
//  *  *****                 *      ***    ***    ***    *                   ***** *
//  
//    *****                                                                 *****   
//    *****         *****  *****  *****  *   *  *****  *****  *****         *****   
//    *****         *   *  *      *   *  *   *  *      *        *           *****   
//    *****         *****  ****   * * *  *   *  ****   *****    *           *****   
//    *****         *  *   *      *  **  *   *  *          *    *           *****   
//    *****         *   *  *****  *****  *****  *****  *****    *           *****   
//    *****                                                                 *****   
//    *****                   *    ***    ***    ***      *                 *****   
//    *****                 * *   *   *  *   *  *   *   * *                 *****   
//  * *****                   *   *   *  *   *  *   *     *                 *****  *
//    *****                   *   *   *  *   *  *   *     *                 *****   
//  * *****                   *    ***    ***    ***      *                 *****  *
//  
//    000500000000
//    270025002500
//    250035000000
//    000020001000
//    070010002000
//    000000002700
//    250025002500
//    350000000000
//    000003000300
//    030000000000
//    000020371024
//    072410242037
//    000000003700
//    020304031003
//    370000000000
//    373721022104
//    211037370000
//    000037210421
//    043704213721
//    000000002037
//    201037042010
//    203700000000
//    371602210421
//    102137370000
//    000037372424
//    242424243737
//    000000050000
//  
//                                                                                  
//     *****  *   *  *****         *   *  *   *  *****  *   *  *****  *   *  *****  
//         *   * *       *          * *   *  **  *   *  *   *    *    *  **  *   *  
//     *****    *    *****           *    * * *  *   *  *****    *    * * *  *****  
//     *        *    *       ***     *    **  *  *   *  *   *    *    **  *  *   *  
//     *****    *    *****   ***     *    *   *  *****  *   *    *    *   *  *   *  
//                                                                                  
//                                 *****         *   *  *****  *   *   ****  *****  
//                                 *   *         *  **    *    ** **  *   *  *   *  
//  *                              *****         * * *    *    * * *  *   *  ***** *
//                                 *   *   ***   **  *    *    *   *  *   *  *   *  
//  *                              *   *   ***   *   *  *****  *   *   ****  *   * *
//  
//                                                                                  
//    *****  *   *  *****  *   *  *****  *   *  *   *         *****  *   *  *****   
//    *   *  **  *    *    *   *  *   *  **  *   * *          *       * *   *       
//    *****  * * *    *    *****  *   *  * * *    *           *****    *    *****   
//    *   *  *  **    *    *   *  *   *  *  **    *     ***       *    *        *   
//    *   *  *   *    *    *   *  *****  *   *    *     ***   *****    *    *****   
//                                                                                  
//    *****  ****   *   *  *****  *   *         *****                               
//    *   *  *   *  ** **    *    **  *         *   *                               
//  * *****  *   *  * * *    *    * * *         *****                              *
//    *   *  *   *  *   *    *    *  **   ***   *   *                               
//  * *   *  ****   *   *  *****  *   *   ***   *   *                              *
//  

#if 0
// The first (spooled) write is a banner card; special case it and delay opening until
// the next card

    if (tally == 27 && memcmp (buffer, bannerCard, sizeof (bannerCard)) == 0)
      {
        pun_state [pun_unit_num] . cachedBanner = true;
        return;
      }


    char qno [5], name [LONGEST + 1];
    int rc = parseID (buffer, tally, qno, name);
    char template [129 + LONGEST];
    if (rc == 0)
      sprintf (template, "pun%c.spool.XXXXXX", 'a' + pun_unit_num);
    else
      sprintf (template, "pun%c.spool.%s.%s.XXXXXX", 'a' + pun_unit_num, qno, name);
#else
    char template [129];
    sprintf (template, "pun%c.spool.XXXXXX", 'a' + pun_unit_num);
    pun_state [pun_unit_num] . punfile = mkstemp (template);
#endif
#if 0
    if (pun_state [pun_unit_num] . cachedBanner)
      {
        // 014 013 is slew to 013 (top of odd page?); just do a ff
        //char cache [2] = {014, 013};
        //write (pun_state [pun_unit_num] . punfile, & cache, 2);
        char cache = '\f';
        write (pun_state [pun_unit_num] . punfile, & cache, 1);
        pun_state [pun_unit_num] . cachedBanner = false;
      }
#endif
  }

//                       *****  *   *  ****          *****  *****                 
//                       *      **  *  *   *         *   *  *                     
//                       ****   * * *  *   *         *   *  ****                  
//                       *      *  **  *   *         *   *  *                     
//                       *****  *   *  ****          *****  *                     
//                                                                                
//                              ****   *****  *****  *   *                        
//                              *   *  *      *      *  *                         
//*                             *   *  ****   *      ***                         *
//                              *   *  *      *      *  *                         
//*                             ****   *****  *****  *   *                       *

static word36 eodCard [27] =
  {
    000500000000llu,
    000000000000llu,
    000000000000llu,
    000000000000llu,
    000000000000llu,
    000000002000llu,
    240024002400llu,
    370000000000llu,
    372121122104llu,
    210437370000llu,
    000000210021llu,
    002100210037llu,
    000000001621llu,
    212521252125llu,
    373700000000llu,
    371602210421llu,
    102137370000llu,
    000021002500llu,
    250025003700llu,
    000000000000llu,
    000000000000llu,
    000000000000llu,
    000000000000llu,
    000000000000llu,
    000000000000llu,
    000000000000llu,
    000000050000llu
  };

static word36 bannerCard [27] =
  {
    0000500000000llu,
    0770077047704llu,
    0770477000000llu,
    0000000770477llu,
    0047704770077llu,
    0000000007700llu,
    0770477047704llu,
    0770000000000llu,
    0007704770477llu,
    0047700770000llu,
    0000077007704llu,
    0770477047700llu,
    0000000000077llu,
    0047704770477llu,
    0007700000000llu,
    0770077047704llu,
    0770477000000llu,
    0000000770477llu,
    0047704770077llu,
    0000000007700llu,
    0770477047704llu,
    0770000000000llu,
    0007704770477llu,
    0047700770000llu,
    0000077007704llu,
    0770477047700llu,
    0000000050000llu
  };

static int eoj (uint pun_unit_num, word36 * buffer, uint tally)
  {
    if (tally == 27 && memcmp (buffer, eodCard, sizeof (eodCard)) == 0)
      {
        pun_state [pun_unit_num] . sawEOD = true;
        return 0;
      }
    if (pun_state [pun_unit_num] . sawEOD &&
       tally == 27 && memcmp (buffer, bannerCard, sizeof (bannerCard)) == 0)
      return 1;
    return 0;
  }

static int pun_cmd (uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
    uint ctlr_unit_idx = get_ctlr_idx (iomUnitIdx, chan);
    uint devUnitIdx = cables->urp_to_urd[ctlr_unit_idx][p->IDCW_DEV_CODE].unit_idx;
    UNIT * unitp = & pun_unit [devUnitIdx];
    long pun_unit_num = PUN_UNIT_NUM (unitp);

    switch (p -> IDCW_DEV_CMD)
      {

        case 011: // CMD 011 Punch binary
          {
            p -> isRead = false;
            // Get the DDCW

            bool ptro, send, uff;

            int rc = iom_list_service (iomUnitIdx, chan, & ptro, & send, & uff);
            if (rc < 0)
              {
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                sim_printf ("%s list service failed\n", __func__);
                break;
              }
            if (uff)
              {
                sim_printf ("%s ignoring uff\n", __func__); // XXX
              }
            if (! send)
              {
                sim_printf ("%s nothing to send\n", __func__);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                break;
              }
            if (p -> DCW_18_20_CP == 07 || p -> DDCW_22_23_TYPE == 2)
              {
                sim_printf ("%s expected DDCW\n", __func__);
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                break;
              }

            if (p -> DDCW_TALLY != 27)
              {
                sim_warn ("%s expected tally of 27\n", __func__);
                p -> chanStatus = chanStatIncorrectDCW;
                p -> stati = 05001; // BUG: arbitrary error code; config switch
                break;
              }


//dcl 1 raw aligned,    /* raw column binary card image */
//    2 col (1:80) bit (12) unal,                             /* 80 columns */
//    2 pad bit (12) unal;           

            // Copy from core to buffer
            word36 buffer [p -> DDCW_TALLY];
            uint wordsProcessed = 0;
            iom_indirect_data_service (iomUnitIdx, chan, buffer,
                                    & wordsProcessed, false);

#if 0
sim_printf ("tally %d\n", p-> DDCW_TALLY);
for (uint i = 0; i < p -> DDCW_TALLY; i ++)
  sim_printf ("  %012"PRIo64"\n", buffer [i]);
sim_printf ("\n");

for (uint row = 0; row < 12; row ++)
  {
    for (uint col = 0; col < 80; col ++)
      {
        // 3 cols/word
        uint wordno = col / 3;
        uint fieldno = col % 3;
        word1 bit = getbits36_1 (buffer [wordno], fieldno * 12 + row); 
        if (bit)
          sim_printf ("*");
        else
          sim_printf (" ");
      }
    sim_printf ("\n");
  }
sim_printf ("\n");

for (uint row = 0; row < 12; row ++)
  {
    //for (uint col = 0; col < 80; col ++)
    for (int col = 79; col >= 0; col --)
      {
        // 3 cols/word
        uint wordno = (uint) col / 3;
        uint fieldno = (uint) col % 3;
        word1 bit = getbits36_1 (buffer [wordno], fieldno * 12 + row); 
        if (bit)
          sim_printf ("*");
        else
          sim_printf (" ");
      }
    sim_printf ("\n");
  }
sim_printf ("\n");
#endif

             if (pun_state [pun_unit_num] . punfile == -1)
               openPunFile ((int) pun_unit_num, buffer, p -> DDCW_TALLY);

            write (pun_state [pun_unit_num] . punfile, buffer, sizeof (buffer));

            if (eoj ((uint) pun_unit_num, buffer, p -> DDCW_TALLY))
              {
                //sim_printf ("pun end of job\n");
                close (pun_state [pun_unit_num] . punfile);
                pun_state [pun_unit_num] . punfile = -1;
              }
            p -> stati = 04000; 
          }
          break;


        case 031: // CMD 031 Set Diagnostic Mode (load_mpc.pl1)
          {
            p -> stati = 04000;
            sim_debug (DBG_NOTIFY, & pun_dev, "Set Diagnostic Mode %ld\n", pun_unit_num);
          }
          break;

        case 040: // CMD 40 Reset status
          {
            p -> stati = 04000;
            p -> initiate = false;
            p -> isRead = false;
            sim_debug (DBG_NOTIFY, & pun_dev, "Reset status %ld\n", pun_unit_num);
          }
          break;

        default:
          {
            if (p->IDCW_DEV_CMD != 051) // ignore bootload console probe
              sim_warn ("pun daze %o\n", p -> IDCW_DEV_CMD);
            p -> stati = 04501; // cmd reject, invalid opcode
            p -> chanStatus = chanStatIncorrectDCW;
          }
          return IOM_CMD_ERROR;
        }   

    if (p -> IDCW_CONTROL == 3) // marker bit set
      {
        send_marker_interrupt (iomUnitIdx, (int) chan);
      }
    return IOM_CMD_OK;
  }

// 1 ignored command
// 0 ok
// -1 problem
int pun_iom_cmd (uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
// Is it an IDCW?

    if (p -> DCW_18_20_CP == 7)
      {
        pun_cmd (iomUnitIdx, chan);
      }
    else // DDCW/TDCW
      {
        sim_printf ("%s expected IDCW\n", __func__);
        return IOM_CMD_ERROR;
      }
    return IOM_CMD_OK;
  }

static t_stat pun_show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr, UNUSED int val, UNUSED const void * desc)
  {
    sim_printf("Number of PUN units in system is %d\n", pun_dev . numunits);
    return SCPE_OK;
  }

static t_stat pun_set_nunits (UNUSED UNIT * uptr, UNUSED int32 value, const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    int n = atoi (cptr);
    if (n < 1 || n > N_PUN_UNITS_MAX)
      return SCPE_ARG;
    pun_dev . numunits = (uint32) n;
    return SCPE_OK;
  }

static t_stat pun_show_device_name (UNUSED FILE * st, UNIT * uptr,
                                       UNUSED int val, UNUSED const void * desc)
  {
    long n = PUN_UNIT_NUM (uptr);
    if (n < 0 || n >= N_PUN_UNITS_MAX)
      return SCPE_ARG;
    sim_printf("Card punch device name is %s\n", pun_state [n] . device_name);
    return SCPE_OK;
  }

static t_stat pun_set_device_name (UNUSED UNIT * uptr, UNUSED int32 value,
                                    UNUSED const char * cptr, UNUSED void * desc)
  {
    long n = PUN_UNIT_NUM (uptr);
    if (n < 0 || n >= N_PUN_UNITS_MAX)
      return SCPE_ARG;
    if (cptr)
      {
        strncpy (pun_state [n] . device_name, cptr, MAX_DEV_NAME_LEN - 1);
        pun_state [n] . device_name [MAX_DEV_NAME_LEN - 1] = 0;
      }
    else
      pun_state [n] . device_name [0] = 0;
    return SCPE_OK;
  }


