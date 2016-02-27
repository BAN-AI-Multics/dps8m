//
//  dps8_crdrdr.c
//  dps8
//
//  Created by Harry Reed on 6/16/13.
//  Copyright (c) 2013 Harry Reed. All rights reserved.
//

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
#include "dps8_utils.h"
#include "dps8_faults.h"
#include "dps8_cpu.h"
#include "dps8_cable.h"

/*
 Copyright (c) 2007-2013 Michael Mondy
 
 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at http://example.org/project/LICENSE.
 */


#define N_CRDRDR_UNITS 1 // default

static t_stat crdrdr_reset (DEVICE * dptr);
static t_stat crdrdr_show_nunits (FILE *st, UNIT *uptr, int val, void *desc);
static t_stat crdrdr_set_nunits (UNIT * uptr, int32 value, char * cptr, void * desc);
static t_stat crdrdr_show_device_name (FILE *st, UNIT *uptr, int val, void *desc);
static t_stat crdrdr_set_device_name (UNIT * uptr, int32 value, char * cptr, void * desc);

#define UNIT_FLAGS ( UNIT_FIX | UNIT_ATTABLE | UNIT_ROABLE | UNIT_DISABLE | \
                     UNIT_IDLE )
UNIT crdrdr_unit [N_CRDRDR_UNITS_MAX] =
  {
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (NULL, UNIT_FLAGS, 0), 0, 0, 0, 0, 0, NULL, NULL}
  };

#define CRDRDR_UNIT_NUM(uptr) ((uptr) - crdrdr_unit)

static DEBTAB crdrdr_dt [] =
  {
    { "NOTIFY", DBG_NOTIFY },
    { "INFO", DBG_INFO },
    { "ERR", DBG_ERR },
    { "WARN", DBG_WARN },
    { "DEBUG", DBG_DEBUG },
    { "ALL", DBG_ALL }, // don't move as it messes up DBG message
    { NULL, 0 }
  };

#define UNIT_WATCH UNIT_V_UF

static MTAB crdrdr_mod [] =
  {
    { UNIT_WATCH, 1, "WATCH", "WATCH", 0, 0, NULL, NULL },
    { UNIT_WATCH, 0, "NOWATCH", "NOWATCH", 0, 0, NULL, NULL },
    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO | MTAB_VALR, /* mask */
      0,            /* match */
      "NUNITS",     /* print string */
      "NUNITS",         /* match string */
      crdrdr_set_nunits, /* validation routine */
      crdrdr_show_nunits, /* display routine */
      "Number of CRDRDR units in the system", /* value descriptor */
      NULL // Help
    },
    {
      MTAB_XTD | MTAB_VUN | MTAB_VALR | MTAB_NC, /* mask */
      0,            /* match */
      "DEVICE_NAME",     /* print string */
      "DEVICE_NAME",         /* match string */
      crdrdr_set_device_name, /* validation routine */
      crdrdr_show_device_name, /* display routine */
      "Select the boot drive", /* value descriptor */
      NULL          // help
    },

    { 0, 0, NULL, NULL, 0, 0, NULL, NULL }
  };


// No crdrdrs known to multics had more than 2^24 sectors...
DEVICE crdrdr_dev = {
    "CRDRDR",       /*  name */
    crdrdr_unit,    /* units */
    NULL,         /* registers */
    crdrdr_mod,     /* modifiers */
    N_CRDRDR_UNITS, /* #units */
    10,           /* address radix */
    24,           /* address width */
    1,            /* address increment */
    8,            /* data radix */
    36,           /* data width */
    NULL,         /* examine */
    NULL,         /* deposit */ 
    crdrdr_reset,   /* reset */
    NULL,         /* boot */
    NULL,         /* attach */
    NULL,         /* detach */
    NULL,         /* context */
    DEV_DEBUG,    /* flags */
    0,            /* debug control flags */
    crdrdr_dt,      /* debug flag names */
    NULL,         /* memory size change */
    NULL,         /* logical name */
    NULL,         // help
    NULL,         // attach help
    NULL,         // attach context
    NULL          // description
};

#define MAX_DEV_NAME_LEN 64



enum deckFormat { sevenDeck, cardDeck, streamDeck };

static struct crdrdr_state
  {
    char device_name [MAX_DEV_NAME_LEN];
    //FILE * deckfd;
    int deckfd;
    bool running;
    enum { deckStart = 0, eof1Sent, uid1Sent, inputSent, eof2Sent } deckState;
    enum deckFormat deckFormat;
  } crdrdr_state [N_CRDRDR_UNITS_MAX];


static int findCrdrdrUnit (int iomUnitIdx, int chan_num, int dev_code)
  {
    for (int i = 0; i < N_CRDRDR_UNITS_MAX; i ++)
      {
        if (iomUnitIdx == cables -> cablesFromIomToCrdRdr [i] . iomUnitIdx &&
            chan_num     == cables -> cablesFromIomToCrdRdr [i] . chan_num     &&
            dev_code     == cables -> cablesFromIomToCrdRdr [i] . dev_code)
          return i;
      }
    return -1;
  }

/*
 * crdrdr_init()
 *
 */

#if 0
static void usr2signal (UNUSED int signum)
  {
sim_printf ("crd rdr signal caught\n");
    crdrdrCardReady (0);
  }
#endif

// Once-only initialization

void crdrdr_init (void)
  {
    memset (crdrdr_state, 0, sizeof (crdrdr_state));
    for (uint i = 0; i < N_CRDRDR_UNITS_MAX; i ++)
      crdrdr_state [i] . deckfd = -1;
#if 0
    signal (SIGUSR2, usr2signal);
#endif
  }

static t_stat crdrdr_reset (DEVICE * dptr)
  {
    for (uint i = 0; i < dptr -> numunits; i ++)
      {
        // sim_crdrdr_reset (& crdrdr_unit [i]);
        sim_cancel (& crdrdr_unit [i]);
      }
    return SCPE_OK;
  }

// http://homepage.cs.uiowa.edu/~jones/cards/codes.html
// General Electric
// 
// General Electric used the following collating sequence on their machines,
// including the GE 600 (the machine on which Multics was developed); this is
// largely upward compatable from the IBM 026 commercial character set, and it
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
if (q) sim_printf ("found %c at offset %ld\n", * p, q - haystack);
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

static int getCardLine (int fd, char * buffer)
  {
    uint n = 0;
    buffer [n] = 0;
    while (1)
      {
        uint8 ch;
        int rc = read (fd, & ch, 1);
        if (rc <= 0) // eof or err
          return n == 0;
        if (ch == '\n')
          return 0;
        buffer [n ++] = ch;
        buffer [n] = 0;
        if (n >= 79)
         return 0;
     }
  }

static int getCardData (int fd, char * buffer)
  {
    memset (buffer, 0, 80);
    int rc = read (fd, buffer, 80);
    if (rc < 0)
      return 0;
    return rc;
  }

#define rawCardImageBytes (80 * 12 / 8)
static int getRawCardData (int fd, uint8_t * buffer)
  {
    memset (buffer, 0, rawCardImageBytes + 2);
    int rc = read (fd, buffer, rawCardImageBytes);
    if (rc < 0)
      return 0;
    return rc;
  }

static int crdrdrReadRecord (uint iomUnitIdx, uint chan)
  {
    iomChanData_t * p = & iomChanData [iomUnitIdx] [chan];
    sim_debug (DBG_NOTIFY, & crdrdr_dev, "Read binary\n");
    uint unitIdx = findCrdrdrUnit (iomUnitIdx, chan, p -> IDCW_DEV_CODE);

    if (crdrdr_state [unitIdx] . deckfd < 0)
       {
empty:;
          p -> stati = 04201; // hopper empty
          //p -> stati = 04205; // hopper empty, "last batch" button pressed
          //p -> stati = 04200; // offline
          //p -> stati = 04240; // data alert
          p -> initiate = false;
          p -> tallyResidue = 0;
sim_printf ("hopper empty\n");
          return -1;
       }

    char cardImage [80] = "";
    uint8_t rawCardImage [rawCardImageBytes + 2 ];
    size_t l = 0;
    enum deckFormat thisCard;

    static int jobNo = 0;

    switch (crdrdr_state [unitIdx] . deckState)
      {
        case deckStart:
          {
            strcpy (cardImage, "++EOF");
            l = strlen (cardImage);
            thisCard = cardDeck;
            crdrdr_state [unitIdx] . deckState = eof1Sent;
            jobNo ++;
          }
          break;

        case eof1Sent:
          {
            sprintf (cardImage, "++UID %d", jobNo);
            l = strlen (cardImage);
            thisCard = cardDeck;
            crdrdr_state [unitIdx] . deckState = uid1Sent;
          }
          break;

        case uid1Sent:
          {
            int rc = getCardLine (crdrdr_state [unitIdx] . deckfd, cardImage);
            if (rc)
              {
                close (crdrdr_state [unitIdx] . deckfd);
                crdrdr_state [unitIdx] . deckfd = -1;
                crdrdr_state [unitIdx] . deckState = deckStart;
                goto empty;
              }
            l = strlen (cardImage);
            thisCard = cardDeck;
            if (strncasecmp (cardImage, "++input", 7) == 0)
              crdrdr_state [unitIdx] . deckState = inputSent;
          }
          break;

        // Reading the actual data cards

        case inputSent:
          {
            switch (crdrdr_state [unitIdx] . deckFormat)
              {
                case cardDeck:
                  {
                    int rc = getCardLine (crdrdr_state [unitIdx] . deckfd, cardImage);
                    if (rc)
                      {
                        strcpy (cardImage, "++EOF");
                        crdrdr_state [unitIdx] . deckState = eof2Sent;
                      }
                    l = strlen (cardImage);
                  }
                  thisCard = cardDeck;
                  break;
            
              case streamDeck:
                {
                  l = getCardData (crdrdr_state [unitIdx] . deckfd, cardImage);
                  if (l)
                    {
                      thisCard = streamDeck;
                    }
                  else
                    {
                      strcpy (cardImage, "++EOF");
                      l = strlen (cardImage);
                      crdrdr_state [unitIdx] . deckState = eof2Sent;
                      thisCard = cardDeck;
                    }
                }
                break;

              case sevenDeck:
                {
                  l = getRawCardData (crdrdr_state [unitIdx] . deckfd, rawCardImage);
                  if (l)
                    {
                      thisCard = sevenDeck;
                    }
                  else
                    {
                      strcpy (cardImage, "++EOF");
                      l = strlen (cardImage);
                      crdrdr_state [unitIdx] . deckState = eof2Sent;
                      thisCard = cardDeck;
                    }
                }
                break;

              } // switch (deckFormat)
          } // case inputSent
          break;

        case eof2Sent:
          {
            sprintf (cardImage, "++UID %d", jobNo);
            l = strlen (cardImage);
            thisCard = cardDeck;
            crdrdr_state [unitIdx] . deckState = deckStart;
            close (crdrdr_state [unitIdx] . deckfd);
            crdrdr_state [unitIdx] . deckfd = -1;
          }
          break;
      }

           
#if 0
    while (l > 0 && cardImage [l - 1] == '\n')
      cardImage [-- l] = 0;
#endif
    //sim_printf ("card <%s>\n", cardImage);
#if 0
sim_printf ("\n");
sim_printf ("\n");
for (uint i = 0; i < 80; i ++)
  {
    if (isprint (cardImage [i]))
      sim_printf ("%c", cardImage [i]);
    else
      sim_printf ("\\%03o", cardImage [i]);
  }
sim_printf ("\n");
sim_printf ("\n");
#endif
    word36 buffer [27];
    switch (thisCard)
      {
        case sevenDeck:
          {
            // This will overead rawCardImage by 12 bits, but that's okay
            // because Multics will ignore the last 12 bits.
            for (uint i = 0; i < 27; i ++)
              buffer [i] = extr36 ((uint8 *) rawCardImage, i);
//sim_printf ("7deck %012llo %012llo %012llo %012llo\n", buffer [0], buffer [1], buffer [2], buffer [3]);
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
        case cardDeck:
          {
            if (l > 80)
              {
                sim_warn ("Whups. crdrdr l %d > 80; truncating.\n", l);
                l = 80;
                //cardImage [l] = 0;
              }


            uint hbuf [l];
            asciiToH (cardImage, hbuf, l);

            // 12 bits / char
            uint nbits = l * 12;
            // 36 bits / word
            uint tally = (nbits + 35) / 36;

            if (tally > 27)
              {
                sim_warn ("Whups. crdrdr tally %d > 27; truncating.\n", tally);
                tally = 27;
              }

            // Remember that Hollerith for blank is 0, this is really
            // filling the buffer with blanks.
            memset (buffer, 0, sizeof (buffer));
            for (uint col = 0; col < l; col ++)
              {
                uint wordno = col / 3;
                uint fieldno = col % 3;
                putbits36 (& buffer [wordno], fieldno * 12, 12, hbuf [col]);
              }
          }
          break;
      }
#if 0
sim_printf ("\n");
for (uint i = 0; i < 27; i ++)
  {
    sim_printf ("  %012llo     \n", buffer [i]);
#define B(n) bit_rep [(buffer [i] >> n) & 0x0f]
    for (int j = 32; j >= 0; j -= 4)
      sim_printf ("%s", B(j));
    sim_printf ("\n");
  }
sim_printf ("\n");
#endif
    p -> stati = 04000;
    p -> initiate = false;

    // Card images are 80 columns.
    uint tally = 27;

// Process DDCW

    bool ptro, send, uff;

    int rc = iomListService (iomUnitIdx, chan, & ptro, & send, & uff);
    if (rc < 0)
      {
        p -> stati = 05001; // BUG: arbitrary error code; config switch
        sim_printf ("%s list service failed\n", __func__);
        return -1;
      }
    if (uff)
      {
        sim_printf ("%s ignoring uff\n", __func__); // XXX
      }
    if (! send)
      {
        sim_printf ("%s nothing to send\n", __func__);
        p -> stati = 05001; // BUG: arbitrary error code; config switch
        return 1;
      }
    if (p -> DCW_18_20_CP == 07 || p -> DDCW_22_23_TYPE == 2)
      {
        sim_printf ("%s expected DDCW\n", __func__);
        p -> stati = 05001; // BUG: arbitrary error code; config switch
        return -1;
      }

    iomIndirectDataService (iomUnitIdx, chan, buffer,
                            & tally, true);
    p -> stati = 04000; // ok
    p -> initiate = false;
    p -> tallyResidue = tally;
    p -> charPos = 0;

    if (p -> DDCW_22_23_TYPE != 0)
      sim_warn ("curious... a card read with more than one DDCW?\n");

    return 0;
  }

static int crdrdr_cmd (uint iomUnitIdx, uint chan)
  {
    iomChanData_t * p = & iomChanData [iomUnitIdx] [chan];
    uint unitIdx = findCrdrdrUnit (iomUnitIdx, chan, p -> IDCW_DEV_CODE);
    crdrdr_state [unitIdx] . running = true;

    sim_debug (DBG_TRACE, & crdrdr_dev, "IDCW_DEV_CMD %o\n", p -> IDCW_DEV_CMD);
    switch (p -> IDCW_DEV_CMD)
      {
        case 000: // CMD 00 Request status
          {
            p -> stati = 04000;
            sim_debug (DBG_NOTIFY, & crdrdr_dev, "Request status\n");
          }
          break;

        case 001: // CMD 01 Read binary
          {
            int rc = crdrdrReadRecord (iomUnitIdx, chan);
            if (rc)
              return rc;
          }
          break;

        case 040: // CMD 40 Reset status
          {
            p -> stati = 04000;
            sim_debug (DBG_NOTIFY, & crdrdr_dev, "Reset status\n");
          }
          break;

        default:
          {
            sim_warn ("crdrdr daze %o\n", p -> IDCW_DEV_CMD);
            p -> stati = 04501; // cmd reject, invalid opcode
            p -> chanStatus = chanStatIncorrectDCW;
          }
          break;
      }
    return 0;
  }

static void submit (enum deckFormat fmt, char * fname)
  {
    //FILE * deckfd = fopen (fname, "r");
    int deckfd = open (fname, O_RDONLY);
    if (deckfd < 0)
      perror ("crdrdr deck open\n");
    int rc = unlink (fname);
    if (rc)
      perror ("crdrdr deck unlink\n");
    sim_printf ("submit %s\n", fname);
    crdrdr_state [0 /* ASSUME0 */] . deckfd = deckfd;
    crdrdr_state [0 /* ASSUME0 */] . deckState = deckStart;
    crdrdr_state [0 /* ASSUME0 */] . deckFormat = fmt;
    if (deckfd >= 0)
      crdrdrCardReady (0 /*ASSUME0*/);
  }

void rdrProcessEvent ()
  {
    char * qdir = "/tmp/rdra";
    if (! crdrdr_state [0 /* ASSUME0 */] . running)
      return;
#if 0
    if (crdrdr_state [0 /* ASSUME0 */] . deckfd >= 0)
      return;
#endif
    DIR * dp;
    dp = opendir (qdir);
    if (! dp)
      {
        sim_warn ("crdrdr opendir '%s' fail.\n", qdir);
        perror ("opendir");
        return;
      }
    struct dirent * entry;
    while ((entry = readdir (dp)))
      {
        //printf ("%s\n", entry -> d_name);
        char fqname [strlen (entry -> d_name) + strlen (qdir) + 64];
        strcpy (fqname, qdir);
        strcat (fqname, "/");
        strcat (fqname, entry -> d_name);
        if (crdrdr_state [0 /* ASSUME0 */] . deckfd < 0)
          {
            if (strncmp (entry -> d_name, "cdeck.", 6) == 0)
              {
                submit (cardDeck, fqname);
                break;
              }
            if (strncmp (entry -> d_name, "7deck.", 6) == 0)
              {
                submit (sevenDeck, fqname);
                break;
              }
            if (strncmp (entry -> d_name, "sdeck.", 6) == 0)
              {
                submit (streamDeck, fqname);
                break;
              }
          }
        if (strcmp (entry -> d_name, "discard") == 0)
          {
            int rc = unlink (fqname);
            if (rc)
              perror ("crdrdr discark unlink\n");
            if (crdrdr_state [0 /* ASSUME0 */] . deckfd >= 0)
              {
                close (crdrdr_state [0 /* ASSUME0 */] . deckfd);
                crdrdr_state [0 /* ASSUME0 */] . deckfd = -1;
                crdrdr_state [0 /* ASSUME0 */] . deckState = deckStart;
                break;
             }
          }
      }
    closedir (dp);
  }


void crdrdrCardReady (int unitNum)
  {
    send_special_interrupt (cables -> cablesFromIomToCrdRdr [unitNum] . iomUnitIdx,
                            cables -> cablesFromIomToCrdRdr [unitNum] . chan_num,
                            cables -> cablesFromIomToCrdRdr [unitNum] . dev_code,
                            0377, 0377 /* tape drive to ready */);
  }

int crdrdr_iom_cmd (uint iomUnitIdx, uint chan)
  {
    iomChanData_t * p = & iomChanData [iomUnitIdx] [chan];

    // Is it an IDCW?
    if (p -> DCW_18_20_CP == 7)
      {
        return crdrdr_cmd (iomUnitIdx, chan);
      }
    sim_printf ("%s expected IDCW\n", __func__);
    return -1;
  }

static t_stat crdrdr_show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr, UNUSED int val, UNUSED void * desc)
  {
    sim_printf("Number of CRDRDR units in system is %d\n", crdrdr_dev . numunits);
    return SCPE_OK;
  }

static t_stat crdrdr_set_nunits (UNUSED UNIT * uptr, UNUSED int32 value, char * cptr, UNUSED void * desc)
  {
    int n = atoi (cptr);
    if (n < 1 || n > N_CRDRDR_UNITS_MAX)
      return SCPE_ARG;
    crdrdr_dev . numunits = n;
    return SCPE_OK;
  }

static t_stat crdrdr_show_device_name (UNUSED FILE * st, UNIT * uptr,
                                       UNUSED int val, UNUSED void * desc)
  {
    int n = CRDRDR_UNIT_NUM (uptr);
    if (n < 0 || n >= N_CRDRDR_UNITS_MAX)
      return SCPE_ARG;
    sim_printf("Card reader device name is %s\n", crdrdr_state [n] . device_name);
    return SCPE_OK;
  }

static t_stat crdrdr_set_device_name (UNUSED UNIT * uptr, UNUSED int32 value,
                                    UNUSED char * cptr, UNUSED void * desc)
  {
    int n = CRDRDR_UNIT_NUM (uptr);
    if (n < 0 || n >= N_CRDRDR_UNITS_MAX)
      return SCPE_ARG;
    if (cptr)
      {
        strncpy (crdrdr_state [n] . device_name, cptr, MAX_DEV_NAME_LEN - 1);
        crdrdr_state [n] . device_name [MAX_DEV_NAME_LEN - 1] = 0;
      }
    else
      crdrdr_state [n] . device_name [0] = 0;
    return SCPE_OK;
  }

