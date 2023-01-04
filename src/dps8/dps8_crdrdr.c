/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * SPDX-License-Identifier: Multics
 * scspell-id: 89935fa0-f62d-11ec-beae-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2016 Michal Tomek
 * Copyright (c) 2021 Dean S. Anderson
 * Copyright (c) 2021-2023 The DPS8M Development Team
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
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

#include "dps8.h"
#include "dps8_iom.h"
#include "dps8_crdrdr.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_utils.h"

#define DBG_CTR 1
#define N_RDR_UNITS 1 // default

static t_stat rdr_reset (DEVICE * dptr);
static t_stat rdr_show_nunits (FILE *st, UNIT *uptr, int val, const void *desc);
static t_stat rdr_set_nunits (UNIT * uptr, int32 value, const char * cptr, void * desc);
static t_stat rdr_show_device_name (FILE *st, UNIT *uptr, int val, const void *desc);
static t_stat rdr_set_device_name (UNIT * uptr, int32 value, const char * cptr, void * desc);
static t_stat rdr_show_path (UNUSED FILE * st, UNIT * uptr, UNUSED int val,
                             UNUSED const void * desc);
static t_stat rdr_set_path (UNUSED UNIT * uptr, UNUSED int32 value, const UNUSED char * cptr,
                            UNUSED void * desc);

#define UNIT_FLAGS ( UNIT_FIX | UNIT_ATTABLE | UNIT_ROABLE | UNIT_DISABLE | \
                     UNIT_IDLE )
UNIT rdr_unit [N_RDR_UNITS_MAX] =
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

#define RDR_UNIT_NUM(uptr) ((uptr) - rdr_unit)

static DEBTAB rdr_dt [] =
  {
    { "NOTIFY", DBG_NOTIFY, NULL },
    { "INFO",   DBG_INFO,   NULL },
    { "ERR",    DBG_ERR,    NULL },
    { "WARN",   DBG_WARN,   NULL },
    { "DEBUG",  DBG_DEBUG,  NULL },
    { "ALL",    DBG_ALL,    NULL }, // don't move as it messes up DBG message
    { NULL,     0,          NULL }
  };

#define UNIT_WATCH UNIT_V_UF

static MTAB rdr_mod [] =
  {
#ifndef SPEED
    { UNIT_WATCH, 1, "WATCH",   "WATCH",   0, 0, NULL, NULL },
    { UNIT_WATCH, 0, "NOWATCH", "NOWATCH", 0, 0, NULL, NULL },
#endif
    {
      MTAB_XTD | MTAB_VDV | \
      MTAB_NMO | MTAB_VALR,                 /* Mask               */
      0,                                    /* Match              */
      "NUNITS",                             /* Print string       */
      "NUNITS",                             /* Match string       */
      rdr_set_nunits,                       /* Validation routine */
      rdr_show_nunits,                      /* Display routine    */
      "Number of RDR units in the system",  /* Value descriptor   */
      NULL // Help
    },
    {
      MTAB_XTD | MTAB_VUN | \
      MTAB_VALR | MTAB_NC,                  /* Mask               */
      0,                                    /* Match              */
      "NAME",                               /* Print string       */
      "NAME",                               /* Match string       */
      rdr_set_device_name,                  /* Validation routine */
      rdr_show_device_name,                 /* Display routine    */
      "Select the boot drive",              /* Value descriptor   */
      NULL                                  /* Help               */
    },
    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO | \
      MTAB_VALR | MTAB_NC,                  /* Mask               */
      0,                                    /* Match              */
      "PATH",                               /* Print string       */
      "PATH",                               /* Match string       */
      rdr_set_path,                         /* Validation routine */
      rdr_show_path,                        /* Display routine    */
      "Path to card reader directories",    /* Value descriptor   */
      NULL                                  /* Help               */
    },

    { 0, 0, NULL, NULL, 0, 0, NULL, NULL }
  };

DEVICE rdr_dev = {
    "RDR",        /* Name                */
    rdr_unit,     /* Units               */
    NULL,         /* Registers           */
    rdr_mod,      /* Modifiers           */
    N_RDR_UNITS,  /* #units              */
    10,           /* Address radix       */
    24,           /* Address width       */
    1,            /* Address increment   */
    8,            /* Data radix          */
    36,           /* Data width          */
    NULL,         /* Examine             */
    NULL,         /* Deposit             */
    rdr_reset,    /* Reset               */
    NULL,         /* Boot                */
    NULL,         /* Attach              */
    NULL,         /* Detach              */
    NULL,         /* Context             */
    DEV_DEBUG,    /* Flags               */
    0,            /* Debug control flags */
    rdr_dt,       /* Debug flag names    */
    NULL,         /* Memory size change  */
    NULL,         /* Logical name        */
    NULL,         /* Help                */
    NULL,         /* Attach help         */
    NULL,         /* Attach context      */
    NULL,         /* Description         */
    NULL          /* End                 */
};

enum deckFormat { sevenDeck, cardDeck, streamDeck };

// Windows cannot unlink an open file; rework the code to unlink the
// submitted card deck after closing it.
//  -- Add fname tp rdr_state
//  -- Add unlink calls at eof close
static struct rdr_state
  {
    enum rdr_mode
      {
        rdr_no_mode, rdr_rd_bin
      } io_mode;
    int deckfd;
    // Sending a ++END means that the read_cards command has to be reissued
    //enum { deckStart = 0, eof1Sent, uid1Sent, inputSent, eof2Sent, uid2Sent } deckState;
    enum { deckStart = 0, eof1Sent, uid1Sent, inputSent, eof2Sent} deckState;
    enum deckFormat deckFormat;
    bool running;
    char device_name [MAX_DEV_NAME_LEN];
    char fname [PATH_MAX+1];
  } rdr_state [N_RDR_UNITS_MAX];

static char* rdr_name = "rdr";
static char rdr_path_prefix[PATH_MAX+1];

/*
 * rdr_init()
 */

// Once-only initialization

void rdr_init (void)
  {
    memset (rdr_path_prefix, 0, sizeof (rdr_path_prefix));
    memset (rdr_state, 0, sizeof (rdr_state));
    for (uint i = 0; i < N_RDR_UNITS_MAX; i ++)
      rdr_state [i] . deckfd = -1;
  }

static t_stat rdr_reset (UNUSED DEVICE * dptr)
  {
#if 0
    for (uint i = 0; i < dptr -> numunits; i ++)
      {
        // sim_rdr_reset (& rdr_unit [i]);
        // sim_cancel (& rdr_unit [i]);
      }
#endif
    return SCPE_OK;
  }

// General Electric Cards
//
// General Electric used the following collating sequence on their machines,
// including the GE 600 (the machine on which Multics was developed); this is
// largely upward compatible from the IBM 026 commercial character set, and it
// shows strong influence from the IBM 1401 character set while supporting the
// full ASCII character set, with 64 printable characters, as it was understood
// in the 1960's.
//
// GE   &-0123456789ABCDEFGHIJKLMNOPQR/STUVWXYZ[#@:>?+.](<\^$*);'_,%="!
//      ________________________________________________________________
//     /&-0123456789ABCDEFGHIJKLMNOPQR/STUVWXYZ #@:>V .¤(<§ $*);^±,%='"
// 12 / O           OOOOOOOOO                        OOOOOO
// 11|   O                   OOOOOOOOO                     OOOOOO
//  0|    O                           OOOOOOOOO                  OOOOOO
//  1|     O        O        O        O
//  2|      O        O        O        O       O     O     O     O
//  3|       O        O        O        O       O     O     O     O
//  4|        O        O        O        O       O     O     O     O
//  5|         O        O        O        O       O     O     O     O
//  6|          O        O        O        O       O     O     O     O
//  7|           O        O        O        O       O     O     O     O
//  8|            O        O        O        O OOOOOOOOOOOOOOOOOOOOOOOO
//  9|             O        O        O        O
//   |__________________________________________________________________
// In the above, the 0-8-2 punch shown as _ should be printed as an assignment
// arrow, and the 11-8-2 punch shown as ^ should be printed as an up-arrow.
// This conforms to the evolution of of these ASCII symbols from the time GE
// adopted this character set and the present.

// Sadly, AG91, App. C disagrees
#if 0
static void asciiToH (char * str, uint * hstr)
  {
    char haystack [] = "&-0123456789ABCDEFGHIJKLMNOPQR/STUVWXYZ[#@:>?+.](<\\^$*);'_,%=\"!";
    uint table [] =
      {
        0b100000000000, // &
        0b010000000000, // -
        0b001000000000, // 0
        0b000100000000, // 1
        0b000010000000, // 2
        0b000001000000, // 3
        0b000000100000, // 4
        0b000000010000, // 5
        0b000000001000, // 6
        0b000000000100, // 7
        0b000000000010, // 8
        0b000000000001, // 9

        0b100100000000, // A
        0b100010000000, // B
        0b100001000000, // C
        0b100000100000, // D
        0b100000010000, // E
        0b100000001000, // F
        0b100000000100, // G
        0b100000000010, // H
        0b100000000001, // I

        0b010100000000, // J
        0b010010000000, // K
        0b010001000000, // L
        0b010000100000, // M
        0b010000010000, // N
        0b010000001000, // O
        0b010000000100, // P
        0b010000000010, // Q
        0b010000000001, // R

        0b001100000000, // /
        0b001010000000, // S
        0b001001000000, // T
        0b001000100000, // U
        0b001000010000, // V
        0b001000001000, // W
        0b001000000100, // X
        0b001000000010, // Y
        0b001000000001, // Z

        0b000010000010, // [
        0b000001000010, // #
        0b000000100010, // @
        0b000000010010, // :
        0b000000001010, // >
        0b000000000110, // ?

        0b100010000010, // +
        0b100001000010, // .
        0b100000100010, // ]
        0b100000010010, // (
        0b100000001010, // <
        0b100000000110, // backslash

        0b010010000010, // ^
        0b010001000010, // $
        0b010000100010, // *
        0b010000010010, // )
        0b010000001010, // ;
        0b010000000110, // '

        0b001010000010, // _
        0b001001000010, // ,
        0b001000100010, // %
        0b001000010010, // =
        0b001000001010, // "
        0b001000000110, // !
      };
    for (char * p = str; * p; p ++)
      {
        uint h = 0b000000000110; // ?
        char * q = index (haystack, toupper (* p));
if (q) sim_printf ("found %c at offset %ld\r\n", * p, q - haystack);
        if (q)
          h = table [q - haystack];
        * hstr ++ = h;
      }
 }
#endif

// From card_codes_.alm

static uint16 table [128] =
  {
    05403, 04401, 04201, 04101, 00005, 01023, 01013, 01007,
    02011, 04021, 02021, 04103, 04043, 04023, 04013, 04007,
    06403, 02401, 02201, 02101, 00043, 00023, 00201, 01011,
    02003, 02403, 00007, 01005, 02043, 02023, 02013, 02007,
    00000, 02202, 00006, 00102, 02102, 01042, 04000, 00022,
    04022, 02022, 02042, 04012, 01102, 02000, 04102, 01400,
    01000, 00400, 00200, 00100, 00040, 00020, 00010, 00004,
    00002, 00001, 00202, 02012, 04042, 00012, 01012, 01006,
    00042, 04400, 04200, 04100, 04040, 04020, 04010, 04004,
    04002, 04001, 02400, 02200, 02100, 02040, 02020, 02010,
    02004, 02002, 02001, 01200, 01100, 01040, 01020, 01010,
    01004, 01002, 01001, 05022, 04202, 06022, 02006, 01022,
    00402, 05400, 05200, 05100, 05040, 05020, 05010, 05004,
    05002, 05001, 06400, 06200, 06100, 06040, 06020, 06010,
    06004, 06002, 06001, 03200, 03100, 03040, 03020, 03010,
    03004, 03002, 03001, 05000, 04006, 03000, 03400, 00000
  };

static void asciiToH (char * str, uint * hstr, size_t l)
  {
    char * p = str;
    for (size_t i = 0; i < l; i ++)
    //for (char * p = str; * p; p ++)
      {
        * hstr ++ = table [(* p) & 0177];
        p ++;
      }
  }

#if 0
static char * testDeck [] =
  {
    "++eof",
    "++uid cac",
    "++data test \\Anthony \\Sys\\Eng",
    "++password \\X\\X\\X",
    "++format rmcc trim  addnl",
    "++input",
    "test",
    "++eof",
    "++uid cac",
    "++end",
    NULL
  };

static int testDeckLine = 0;
#endif

#if 0
static const char *bit_rep[16] = {
    [ 0] = "0000", [ 1] = "0001", [ 2] = "0010", [ 3] = "0011",
    [ 4] = "0100", [ 5] = "0101", [ 6] = "0110", [ 7] = "0111",
    [ 8] = "1000", [ 9] = "1001", [10] = "1010", [11] = "1011",
    [12] = "1100", [13] = "1101", [14] = "1110", [15] = "1111",
};
#endif

static int getCardLine (int fd, unsigned char * buffer)
  {
    uint n = 0;
    buffer [n] = 0;
    while (1)
      {
        uint8 ch;
        ssize_t rc = read (fd, & ch, 1);
        if (rc <= 0) // eof or err
          return n == 0;
        if (ch == '\n')
          return 0;
        buffer [n ++] = ch;
        buffer [n] = 0;
        if (n > 79)
          return 0;
      }
    /*NOTREACHED*/ /* unreachable */
    return 0;
  }

static int getCardData (int fd, char * buffer)
  {
    memset (buffer, 0, 80);
    ssize_t rc = read (fd, buffer, 80);
    if (rc < 0)
      return 0;
    return (int) rc;
  }

#define rawCardImageBytes (80 * 12 / 8)
static int getRawCardData (int fd, uint8_t * buffer)
  {
    memset (buffer, 0, rawCardImageBytes + 2);
    ssize_t rc = read (fd, buffer, rawCardImageBytes);
    if (rc < 0)
      return 0;
    return (int) rc;
  }

#ifdef TESTING
static bool empty = false;
#endif

static int rdrReadRecord (uint iomUnitIdx, uint chan) {
  iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
  sim_debug (DBG_NOTIFY, & rdr_dev, "Read binary\n");
  uint ctlr_unit_idx  = get_ctlr_idx (iomUnitIdx, chan);
  uint unitIdx        = cables->urp_to_urd[ctlr_unit_idx][p->IDCW_DEV_CODE].unit_idx;

#if 0
  if (rdr_state [unitIdx].deckfd < 0) {
empty:;
    p->stati = 04201; // hopper empty
    //p->stati = 04205; // hopper empty, "last batch" button pressed
    //p->stati = 04200; // offline
    //p->stati = 04240; // data alert
    p->tallyResidue = 0;
# ifdef TESTING
    if (! empty)
      sim_printf ("hopper empty\r\n");
    empty = true;
# endif
    return IOM_CMD_ERROR;
  }
#endif
#ifdef TESTING
  sim_printf ("hopper not empty\r\n");
  empty = false;
#endif
  unsigned char cardImage [80] = "";
  uint8_t rawCardImage [rawCardImageBytes + 2 ];
  size_t l = 0;
  // initialize to quiet compiler
  enum deckFormat thisCard = cardDeck;

  static int jobNo = 0;

  switch (rdr_state [unitIdx].deckState) {

    case deckStart: {
#ifdef TESTING
  sim_printf ("deckStart: sending ++EOF\r\n");
#endif
      strcpy ((char *) cardImage, "++EOF");
      l = strlen ((char *) cardImage);
      thisCard = cardDeck; //-V1048
      rdr_state [unitIdx].deckState = eof1Sent;
      jobNo ++;
    }
    break;

    case eof1Sent: {
#ifdef TESTING
  sim_printf ("eof1Sent: sending ++UID\r\n");
#endif
      sprintf ((char *) cardImage, "++UID %d", jobNo);
      l = strlen ((char *) cardImage);
      thisCard = cardDeck; //-V1048
      rdr_state [unitIdx].deckState = uid1Sent;
    }
    break;

    case uid1Sent: {
#ifdef TESTING
  sim_printf ("uid1Sent: sending data\r\n");
#endif
      int rc = getCardLine (rdr_state [unitIdx].deckfd, cardImage);
      if (rc) {
#ifdef TESTING
  sim_printf ("uid1Sent: getCardLine returned %d\r\n", rc);
#endif
        close (rdr_state [unitIdx].deckfd);
        // Windows can't unlink open files; do it now...
        rc = unlink (rdr_state [unitIdx].fname);
        if (rc)
          perror ("card reader deck unlink\n");
        rdr_state [unitIdx].deckfd = -1;
        rdr_state [unitIdx].deckState = deckStart;
        p->stati = 04201; // hopper empty
        return IOM_CMD_DISCONNECT;
      }
#ifdef TESTING
  sim_printf ("uid1Sent: getCardLine returned <%s>\r\n", cardImage);
#endif
      l = strlen ((char *) cardImage);
      thisCard = cardDeck; //-V1048
      if (strncasecmp ((char *) cardImage, "++input", 7) == 0) {
#ifdef TESTING
  sim_printf ("uid1Sent: switching to inputSent <%s>\r\n", cardImage);
#endif
        rdr_state [unitIdx].deckState = inputSent;
      }
    }
    break;

    // Reading the actual data cards

    case inputSent: {
#ifdef TESTING
  sim_printf ("inputSent: format %d\r\n", rdr_state [unitIdx].deckFormat);
#endif
      switch (rdr_state [unitIdx].deckFormat) {
        case cardDeck: {
#ifdef TESTING
  sim_printf ("inputSent: cardDeck\r\n");
#endif
          int rc = getCardLine (rdr_state [unitIdx].deckfd, cardImage);
          if (rc) {
            strcpy ((char *) cardImage, "++EOF");
            rdr_state [unitIdx].deckState = eof2Sent;
          }
          l = strlen ((char *) cardImage);
        }
        thisCard = cardDeck; //-V1048
        break;

        case streamDeck: {
#ifdef TESTING
  sim_printf ("inputSent: streamDeck\r\n");
#endif
          l = (size_t) getCardData (rdr_state [unitIdx].deckfd, (char *) cardImage);
          if (l) {
            thisCard = streamDeck;
          } else {
            strcpy ((char *) cardImage, "++EOF");
            l = strlen ((char *) cardImage);
            rdr_state [unitIdx].deckState = eof2Sent;
            thisCard = cardDeck; //-V1048
          }
#ifdef TESTING
  sim_printf ("inputSent: getCardLine returned <%s>\r\n", cardImage);
#endif
        }
        break;

        case sevenDeck: {
#ifdef TESTING
  sim_printf ("inputSent: 7Deck\r\n");
#endif
          l = (size_t) getRawCardData (rdr_state [unitIdx].deckfd, rawCardImage);
          if (l) {
            thisCard = sevenDeck;
          } else {
            strcpy ((char *) cardImage, "++EOF");
            l = strlen ((char *) cardImage);
            rdr_state [unitIdx].deckState = eof2Sent;
            thisCard = cardDeck; //-V1048
          }
        }
        break;

      } // switch (deckFormat)
    } // case inputSent
    break;

// Sending a ++END means that the read_cards command has to be reissued
#if 1
    case eof2Sent: {
# ifdef TESTING
  sim_printf ("eof2Sent\r\n");
# endif
      sprintf ((char *) cardImage, "++UID %d", jobNo);
      l = strlen ((char *) cardImage);
      thisCard = cardDeck; //-V1048
      rdr_state [unitIdx].deckState = deckStart;
      close (rdr_state [unitIdx].deckfd);
      // Windows can't unlink open files; do it now...
      int rc = unlink (rdr_state [unitIdx].fname);
      if (rc)
        perror ("card reader deck unlink\n");
      rdr_state [unitIdx].deckfd = -1;
    }
    break;
#else
    case eof2Sent: {
# ifdef TESTING
  sim_printf ("eof2Sent\r\n");
# endif
      sprintf ((char *) cardImage, "++UID %d", jobNo);
      l = strlen ((char *) cardImage);
      thisCard = cardDeck;
      rdr_state [unitIdx].deckState = uid2Sent;
    }
    break;

    case uid2Sent: {
# ifdef TESTING
  sim_printf ("uid2Sent\r\n");
# endif
      sprintf ((char *) cardImage, "++END");
      l = strlen ((char *) cardImage);
      thisCard = cardDeck;
      rdr_state [unitIdx].deckState = deckStart;
      close (rdr_state [unitIdx].deckfd);
      // Windows can't unlink open files; do it now...
      int rc = unlink (rdr_state [unitIdx].fname);
      if (rc)
        perror ("card reader deck unlink\n");
      rdr_state [unitIdx].deckfd = -1;
    }
    break;
#endif
  }

#if 0
    while (l > 0 && cardImage [l - 1] == '\n')
      cardImage [-- l] = 0;
#endif
    //sim_printf ("card <%s>\r\n", cardImage);
#ifdef TESTING
  sim_printf ("\r\n");
  sim_printf ("\r\n");
  for (uint i = 0; i < 80; i ++) {
    if (isprint (cardImage [i]))
      sim_printf ("%c", cardImage [i]);
    else
      sim_printf ("\\%03o", cardImage [i]);
  }
  sim_printf ("\r\n");
  sim_printf ("\r\n");
#endif
  word36 buffer [27];
  switch (thisCard) {

    case sevenDeck: {
      // This will overread rawCardImage by 12 bits, but that's okay
      // because Multics will ignore the last 12 bits.
      for (uint i = 0; i < 27; i ++)
        buffer [i] = extr36 ((uint8 *) rawCardImage, i);
      //sim_printf ("7deck %012"PRIo64" %012"PRIo64" %012"PRIo64" %012"PRIo64"\r\n",
      //             buffer [0], buffer [1], buffer [2], buffer [3]);
    }
    break;

    case streamDeck:
#if 0
          {
            // This will overread cardImage by 12 bits, but that's okay
            // because Multics will ignore the last 12 bits.
            for (uint i = 0; i < 27; i ++)
              buffer [i] = extr36 ((uint8 *) cardImage, i);
          }
          break;

#endif
    case cardDeck: {
      if (l > 80) {
        sim_warn ("Whups. rdr l %lu > 80; truncating.\n", (unsigned long)l);
        l = 80;
        //cardImage [l] = 0;
      }

      uint hbuf [l];
      asciiToH ((char *) cardImage, hbuf, l);

      // 12 bits / char
      uint nbits = (uint) l * 12;
      // 36 bits / word
      uint tally = (nbits + 35) / 36;

      if (tally > 27) { //-V547
        sim_warn ("Impossible rdr tally: %d > 27; truncating.\n", tally);
        tally = 27;
      }

      // Remember that Hollerith for blank is 0, this is really
      // filling the buffer with blanks.
      memset (buffer, 0, sizeof (buffer));
      for (uint col = 0; col < l; col ++) {
        uint wordno  = col / 3;
        uint fieldno = col % 3;
        putbits36_12 (& buffer [wordno], fieldno * 12, (word12) hbuf [col]);
      }
    }
    break;
  }
#if 0
  sim_printf ("\r\n");
  for (uint i = 0; i < 27; i ++) {
    sim_printf ("  %012"PRIo64"     \r\n", buffer [i]);
# define B(n) bit_rep [(buffer [i] >> n) & 0x0f]
    for (int j = 32; j >= 0; j -= 4)
      sim_printf ("%s", B(j));
    sim_printf ("\r\n");
  }
  sim_printf ("\r\n");
#endif
  p->stati = 04000;

  // Card images are 80 columns.
  uint tally = 27;

  iom_indirect_data_service (iomUnitIdx, chan, buffer, & tally, true);
  p->initiate     = false;
  p->stati        = 04000; //-V1048  // ok
  p->tallyResidue = (word12) tally & MASK12;
  p->charPos      = 0;

  return IOM_CMD_PROCEED;
}

static void submit (enum deckFormat fmt, char * fname, uint16 readerIndex)
  {
    if (readerIndex >= N_RDR_UNITS_MAX) {
      sim_warn("crdrdr: submit called with invalid reader index %ld\n", (long) readerIndex);
      return;
    }

    int deckfd = open (fname, O_RDONLY);
    if (deckfd < 0)
      perror ("card reader deck open\n");
// Windows can't unlink open files; save the file name and unlink on close.
    // int rc = unlink (fname); // this only works on UNIX
#ifdef TESTING
    sim_printf ("submit %s\r\n", fname);
#endif
    strcpy (rdr_state [readerIndex].fname, fname);
    rdr_state [readerIndex].deckfd     = deckfd;
    rdr_state [readerIndex].deckState  = deckStart;
    rdr_state [readerIndex].deckFormat = fmt;
    if (deckfd >= 0)
      rdrCardReady (readerIndex);
  }

static void scanForCards(uint16 readerIndex)
  {
    char rdr_dir [2 * PATH_MAX + 1];

    if (readerIndex >= N_RDR_UNITS_MAX) {
      sim_warn("crdrdr: scanForCards called with invalid reader index %d\n", readerIndex);
      return;
    }

#if !defined(__MINGW64__) || !defined(__MINGW32__)
    sprintf(rdr_dir, "/tmp/%s%c", rdr_name, 'a' + readerIndex);
#else
    sprintf(rdr_dir, "%s/%s%c", getenv("TEMP"), rdr_name, 'a' + readerIndex);
#endif /* if !defined(__MINGW64__) || !defined(__MINGW32__) */

    if (rdr_path_prefix [0])
      {
        sprintf(rdr_dir, "%s%s%c", rdr_path_prefix, rdr_name, 'a' + readerIndex);
      }

    DIR * dp;
    dp = opendir (rdr_dir);
    if (! dp)
      {
        sim_warn ("crdrdr opendir '%s' fail.\n", rdr_dir);
        perror ("opendir");
        return;
      }
    struct dirent * entry;
    struct stat info;
    char fqname [2 * PATH_MAX + 1];
    while ((entry = readdir (dp)))
      {
        strcpy (fqname, rdr_dir);
        strcat (fqname, "/");
        strcat (fqname, entry -> d_name);

        if (stat(fqname, &info) != 0)
          {
            sim_warn("crdrdr: scanForCards stat() error for %s: %s\n", fqname, strerror(errno));
            continue;
          }

        if (S_ISDIR(info.st_mode))
          {
            // Found a directory so skip it
            continue;
          }

        if (rdr_state [readerIndex] . deckfd < 0)
          {
            if (strncmp (entry -> d_name, "cdeck.", 6) == 0)
              {
                submit (cardDeck, fqname, readerIndex);
                break;
              }
            if (strncmp (entry -> d_name, "7deck.", 6) == 0)
              {
                submit (sevenDeck, fqname, readerIndex);
                break;
              }
            if (strncmp (entry -> d_name, "sdeck.", 6) == 0)
              {
                submit (streamDeck, fqname, readerIndex);
                break;
              }
          }
        if (strcmp (entry -> d_name, "discard") == 0)
          {
// Windows can't unlink open files; do it now...
            int rc = unlink (fqname);
            if (rc)
              perror ("crdrdr discard unlink\n");
            if (rdr_state [readerIndex] . deckfd >= 0)
              {
                close (rdr_state [readerIndex] . deckfd);
                rc = unlink (rdr_state [readerIndex] . fname);
                if (rc)
                  perror ("crdrdr deck unlink\n");
                rdr_state [readerIndex] . deckfd = -1;
                rdr_state [readerIndex] . deckState = deckStart;
                break;
             }
          }
      }
    closedir (dp);
  }

void rdrProcessEvent (void)
  {
    if (rdr_path_prefix [0])
      {
        // We support multiple card readers when path prefixing is turned on
        // so we need to check each possible reader to see if it is active
        for (uint16 reader_idx = 0; reader_idx < rdr_dev . numunits; reader_idx++)
          {
            if (rdr_state [reader_idx] . running)
                scanForCards(reader_idx);
          }
      }
    else
      {
        // When path prefixing is off we only support a single card reader
        // (this is for backwards compatibility)
        if (! rdr_state [0] . running)
          return;

        scanForCards(0);
      }
  }

void rdrCardReady (int unitNum)
  {
    uint ctlr_unit_idx = cables->rdr_to_urp [unitNum].ctlr_unit_idx;
    uint ctlr_port_num = 0; // Single port device
    uint iom_unit_idx  = cables->urp_to_iom[ctlr_unit_idx][ctlr_port_num].iom_unit_idx;
    uint chan_num      = cables->urp_to_iom[ctlr_unit_idx][ctlr_port_num].chan_num;
    uint dev_code      = cables->rdr_to_urp[unitNum].dev_code;
    send_special_interrupt (iom_unit_idx, chan_num, dev_code, 0377, 0377 /* card reader to ready */); //-V536
  }

iom_cmd_rc_t rdr_iom_cmd (uint iomUnitIdx, uint chan) {
  iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
  uint dev_code       = p->IDCW_DEV_CODE;

  sim_debug (DBG_TRACE, & rdr_dev, "%s: RDR %c%02o_%02o\n",
          __func__, iomChar (iomUnitIdx), chan, dev_code);

  uint ctlr_unit_idx        = get_ctlr_idx (iomUnitIdx, chan);
  uint unitIdx              = cables->urp_to_urd[ctlr_unit_idx][p->IDCW_DEV_CODE].unit_idx;
  struct rdr_state * statep = & rdr_state[unitIdx];
  statep->running           = true;

  iom_cmd_rc_t rc = IOM_CMD_PROCEED;
  // IDCW?
  if (IS_IDCW (p)) {
    // IDCW
    statep->io_mode = rdr_no_mode;

    switch (p->IDCW_DEV_CMD) {

      case 000: // CMD 00 Request status
        sim_debug (DBG_DEBUG, & rdr_dev, "%s: Request Status\n", __func__);
        p->stati = 04000;
        // This is controller status, not device status
        //if (rdr_state[unitIdx].deckfd < 0)
          //p->stati = 04201; // hopper empty
#ifdef TESTING
sim_printf ("Request status %04o\r\n", p->stati);
#endif
        break;

      case 001: // CMD 01 Read binary
        sim_debug (DBG_DEBUG, & rdr_dev, "%s: Read Binary\n", __func__);
        if (rdr_state [unitIdx].deckfd < 0) {
          p->stati        = 04201; // hopper empty
          p->tallyResidue = 0;
#ifdef TESTING
          if (! empty)
            sim_printf ("hopper empty\r\n");
          empty = true;
#endif
          return IOM_CMD_DISCONNECT;
        }
        statep->io_mode = rdr_rd_bin;
        p->stati        = 04000;
        // This is controller status, not device status
        //if (rdr_state[unitIdx].deckfd < 0)
          //p->stati = 04201; // hopper empty
#ifdef TESTING
sim_printf ("read binary %04o\r\n", p->stati);
#endif
        break;

      case 040: // CMD 40 Reset status
        sim_debug (DBG_DEBUG, & rdr_dev, "%s: Request Status\n", __func__);
        p->stati  = 04000;
        p->isRead = false;
        // This is controller status, not device status
        //if (rdr_state[unitIdx].deckfd < 0)
          //p->stati = 04201; // hopper empty
#ifdef TESTING
sim_printf ("reset status %04o\r\n", p->stati);
#endif
        break;

      default:
#ifdef TESTING
sim_printf ("unknown  %o\r\n", p->IDCW_DEV_CMD);
#endif
        if (p->IDCW_DEV_CMD != 051) // ignore bootload console probe
          sim_warn ("%s: RDR unrecognized device command  %02o\n", __func__, p->IDCW_DEV_CMD);
        p->stati      = 04501; // cmd reject, invalid opcode
        p->chanStatus = chanStatIncorrectDCW;
        return IOM_CMD_ERROR;
    } // switch IDCW_DEV_CMD

    sim_debug (DBG_DEBUG, & rdr_dev, "%s: stati %04o\n", __func__, p->stati);
    return IOM_CMD_PROCEED;
  } // if IDCW

  // Not IDCW; TDCW are captured in IOM, so must be IOTD, IOTP or IOTNP
  switch (statep->io_mode) {

    case rdr_no_mode:
#ifdef TESTING
      sim_printf ("%s: Unexpected IOTx\r\n", __func__);
#endif
      //sim_warn ("%s: Unexpected IOTx\n", __func__);
      //return IOM_CMD_ERROR;
      break;

    case rdr_rd_bin: {
      int rc = rdrReadRecord (iomUnitIdx, chan);
#ifdef TESTING
sim_printf ("rdrReadRecord returned %d\r\n", rc);
#endif
      if (rc)
        return IOM_CMD_DISCONNECT;
    }
    break;

    default:
      sim_warn ("%s: Unrecognized io_mode %d\n", __func__, statep->io_mode);
      return IOM_CMD_ERROR;
  }
  return rc;
}

static t_stat rdr_show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr, UNUSED int val,
                               UNUSED const void * desc)
  {
    sim_printf("Number of RDR units in system is %d\n", rdr_dev . numunits);
    return SCPE_OK;
  }

static t_stat rdr_set_nunits (UNUSED UNIT * uptr, UNUSED int32 value, const char * cptr,
                              UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    int n = atoi (cptr);
    if (n < 1 || n > N_RDR_UNITS_MAX)
      return SCPE_ARG;
    rdr_dev . numunits = (uint32) n;
    return SCPE_OK;
  }

static t_stat rdr_show_device_name (UNUSED FILE * st, UNIT * uptr,
                                    UNUSED int val, UNUSED const void * desc)
  {
    long n = RDR_UNIT_NUM (uptr);
    if (n < 0 || n >= N_RDR_UNITS_MAX)
      return SCPE_ARG;
    sim_printf("name     : %s", rdr_state [n] . device_name);
    return SCPE_OK;
  }

static t_stat rdr_set_device_name (UNUSED UNIT * uptr, UNUSED int32 value,
                                   UNUSED const char * cptr, UNUSED void * desc)
  {
    long n = RDR_UNIT_NUM (uptr);
    if (n < 0 || n >= N_RDR_UNITS_MAX)
      return SCPE_ARG;
    if (cptr)
      {
        strncpy (rdr_state [n] . device_name, cptr, MAX_DEV_NAME_LEN - 1);
        rdr_state [n] . device_name [MAX_DEV_NAME_LEN - 1] = 0;
      }
    else
      rdr_state [n] . device_name [0] = 0;
    return SCPE_OK;
  }

static t_stat rdr_set_path (UNUSED UNIT * uptr, UNUSED int32 value,
                            const UNUSED char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;

    size_t len = strlen(cptr);

    // We check for length - (3 + length of rdr_name) to allow for the
    // null, a possible '/' being added and "rdrx" being added
    if (len >= (sizeof(rdr_path_prefix) - (strlen(rdr_name) + 3)))
      return SCPE_ARG;

    strncpy(rdr_path_prefix, cptr, sizeof(rdr_path_prefix));
    if (len > 0)
      {
        if (rdr_path_prefix[len - 1] != '/')
          {
            if (len == sizeof(rdr_path_prefix) - 1)
              return SCPE_ARG;
            rdr_path_prefix[len++] = '/';
            rdr_path_prefix[len] = 0;
          }
      }
    return SCPE_OK;
  }

static t_stat rdr_show_path (UNUSED FILE * st, UNUSED UNIT * uptr,
                             UNUSED int val, UNUSED const void * desc)
  {
    sim_printf("Path to card reader directories is %s\n", rdr_path_prefix);
    return SCPE_OK;
  }
