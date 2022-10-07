/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * SPDX-License-Identifier: Multics
 * scspell-id: 7ec3e12d-f62d-11ec-8431-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2021 Dean Anderson
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
#include <signal.h>
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
#include "utfile.h"

#define DBG_CTR 1

#ifdef TESTING
# undef FREE
# define FREE(p) free(p)
#endif /* ifdef TESTING */

//-- // XXX We use this where we assume there is only one unit
//-- #define ASSUME0 0
//--

#define N_PUN_UNITS 1 // default

static t_stat pun_reset (DEVICE * dptr);
static t_stat pun_show_nunits (FILE *st, UNIT *uptr, int val, const void *desc);
static t_stat pun_set_nunits (UNIT * uptr, int32 value, const char * cptr, void * desc);
static t_stat pun_show_device_name (FILE *st, UNIT *uptr, int val, const void *desc);
static t_stat pun_set_device_name (UNIT * uptr, int32 value, const char * cptr, void * desc);
static t_stat pun_show_path (UNUSED FILE * st, UNIT * uptr, UNUSED int val,
                             UNUSED const void * desc);
static t_stat pun_set_path (UNUSED UNIT * uptr, UNUSED int32 value, const UNUSED char * cptr,
                            UNUSED void * desc);
static t_stat pun_set_config (UNUSED UNIT *  uptr, UNUSED int32 value, const char * cptr,
                              UNUSED void * desc);
static t_stat pun_show_config (UNUSED FILE * st, UNUSED UNIT * uptr, UNUSED int val,
                               UNUSED const void * desc);

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
    { "INFO",   DBG_INFO,   NULL },
    { "ERR",    DBG_ERR,    NULL },
    { "WARN",   DBG_WARN,   NULL },
    { "DEBUG",  DBG_DEBUG,  NULL },
    { "ALL",    DBG_ALL,    NULL }, // don't move as it messes up DBG message
    { NULL,     0,          NULL }
  };

#define UNIT_WATCH UNIT_V_UF

static MTAB pun_mod [] =
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
      pun_set_nunits,                       /* Validation routine */
      pun_show_nunits,                      /* Display routine    */
      "Number of PUN units in the system",  /* Value descriptor   */
      NULL                                  /* Help               */
    },
    {
      MTAB_XTD | MTAB_VUN | \
      MTAB_VALR | MTAB_NC,                  /* Mask               */
      0,                                    /* Match              */
      "NAME",                               /* Print string       */
      "NAME",                               /* Match string       */
      pun_set_device_name,                  /* Validation routine */
      pun_show_device_name,                 /* Display routine    */
      "Set the punch device name",          /* Value descriptor   */
      NULL                                  /* Help               */
    },
    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO | \
      MTAB_VALR | MTAB_NC,                  /* Mask               */
      0,                                    /* Match              */
      "PATH",                               /* Print string       */
      "PATH",                               /* Match string       */
      pun_set_path,                         /* Validation Routine */
      pun_show_path,                        /* Display Routine    */
      "Path to card punch directories",     /* Value Descriptor   */
      NULL                                  /* Help               */
    },
    {
      MTAB_XTD | MTAB_VUN,                  /* Mask               */
      0,                                    /* Match              */
      (char *) "CONFIG",                    /* Print string       */
      (char *) "CONFIG",                    /* Match string       */
      pun_set_config,                       /* Validation routine */
      pun_show_config,                      /* Display routine    */
      NULL,                                 /* Value descriptor   */
      NULL,                                 /* Help               */
    },

    { 0, 0, NULL, NULL, 0, 0, NULL, NULL }
  };

DEVICE pun_dev = {
    "PUN",        /* Name                */
    pun_unit,     /* Units               */
    NULL,         /* Registers           */
    pun_mod,      /* Modifiers           */
    N_PUN_UNITS,  /* #Units              */
    10,           /* Address radix       */
    24,           /* Address width       */
    1,            /* Address increment   */
    8,            /* Data radix          */
    36,           /* Data width          */
    NULL,         /* Examine             */
    NULL,         /* Deposit             */
    pun_reset,    /* Reset               */
    NULL,         /* Boot                */
    NULL,         /* Attach              */
    NULL,         /* Detach              */
    NULL,         /* Context             */
    DEV_DEBUG,    /* Flags               */
    0,            /* Debug control flags */
    pun_dt,       /* Debug flag names    */
    NULL,         /* Memory size change  */
    NULL,         /* Logical name        */
    NULL,         /* Help                */
    NULL,         /* Attach help         */
    NULL,         /* Attach context      */
    NULL,         /* Description         */
    NULL          /* End                 */
};

static config_value_list_t cfg_on_off[] =
  {
    { "off",     0 },
    { "on",      1 },
    { "disable", 0 },
    { "enable",  1 },
    { NULL,      0 }
  };

static config_list_t pun_config_list[] =
  {
   { "logcards", 0, 1, cfg_on_off },
   { NULL,       0, 0, NULL }
  };

#define WORDS_PER_CARD 27
#define MAX_GLYPH_BUFFER_LEN 1024
#define CARD_COL_COUNT 80
#define NIBBLES_PER_COL 3
#define GLYPHS_PER_CARD 22
#define CHAR_MATRIX_BYTES 5

enum parse_state {
    Idle, StartingJob, PunchGlyphLookup, EndOfHeader, CacheCard, EndOfDeck, EndOfJob
};

enum parse_event {
    NoEvent, BannerCard, EndOfDeckCard, Card, Done
};

typedef struct card_cache_node CARD_CACHE_ENTRY;

struct card_cache_node
  {
      word12 tally;
      word36 card_data[WORDS_PER_CARD];
      CARD_CACHE_ENTRY *next_entry;
  };

typedef struct
  {
    char device_name[MAX_DEV_NAME_LEN];
    int  punfile_raw;              // fd of file to get all cards in punch code (w/banner cards)
    bool log_cards;                         // Flag to log card images
    enum parse_state current_state;
    char raw_file_name [PATH_MAX + 1];      // Name associated with punfile_raw
    char glyph_buffer[MAX_GLYPH_BUFFER_LEN];
    CARD_CACHE_ENTRY *first_cached_card;
    CARD_CACHE_ENTRY *last_cached_card;
    enum pun_mode { punNoMode, punWrBin } ioMode;
  } pun_state_t ;

static pun_state_t pun_state[N_PUN_UNITS_MAX];
static char pun_path_prefix[PATH_MAX-63];   // The -63 is to leave room for file name

/*
 * pun_init()
 */

// Once-only initialization

void pun_init (void)
  {
    memset (pun_path_prefix, 0, sizeof (pun_path_prefix));
    memset (pun_state, 0, sizeof (pun_state));
    for (int i = 0; i < N_PUN_UNITS_MAX; i ++)
      {
        pun_state [i] . punfile_raw   = -1;
        pun_state [i] . current_state = Idle;
      }
  }

static t_stat pun_reset (UNUSED DEVICE * dptr)
  {
    return SCPE_OK;
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

static word36 eodCard [WORDS_PER_CARD] =
  {
    0000500000000llu,
    0000000000000llu,
    0000000000000llu,
    0000000000000llu,
    0000000000000llu,
    0000000002000llu,
    0240024002400llu,
    0370000000000llu,
    0372121122104llu,
    0210437370000llu,
    0000000210021llu,
    0002100210037llu,
    0000000001621llu,
    0212521252125llu,
    0373700000000llu,
    0371602210421llu,
    0102137370000llu,
    0000021002500llu,
    0250025003700llu,
    0000000000000llu,
    0000000000000llu,
    0000000000000llu,
    0000000000000llu,
    0000000000000llu,
    0000000000000llu,
    0000000000000llu,
    0000000050000llu
  };

//    *****         *****         *****         *****         *****         *****
//    *****         *****         *****         *****         *****         *****
//    *****         *****         *****         *****         *****         *****
//    *****   ***   *****   ***   *****   ***   *****   ***   *****   ***   *****
//    *****         *****         *****         *****         *****         *****
//    *****         *****         *****         *****         *****         *****
//           *****         *****         *****         *****         *****
//           *****         *****         *****         *****         *****
//           *****         *****         *****         *****         *****
// *   ***   *****   ***   *****   ***   *****   ***   *****   ***   *****   ***  *
//           *****         *****         *****         *****         *****
// *         *****         *****         *****         *****         *****        *

static word36 bannerCard [WORDS_PER_CARD] =
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

/*
 *                  Glyph Pattern Lookup
 * This is the parsing of the "lace" cards and extracting the ASCII characters
 * have been punched into the cards (as glyphs) so the operator knows how to
 * deliver the deck.
 */

#define NUM_GLYPH_CHAR_PATTERNS 45

static uint8 glyph_char_patterns [NUM_GLYPH_CHAR_PATTERNS][CHAR_MATRIX_BYTES] =
  {
      // Asterisk
      { 037, 037, 037, 037, 037 },
      // Space
      { 000, 000, 000, 000, 000 },
      // Period
      { 000, 003, 003, 003, 000 },
      // >
      { 021, 000, 012, 000, 004 },
      // A
      { 037, 024, 024, 024, 037 },
      // B
      { 037, 025, 025, 025, 012 },
      // C
      { 037, 021, 021, 021, 021 },
      // D
      { 037, 021, 021, 021, 016 },
      // E
      { 037, 025, 025, 025, 021 },
      // F
      { 037, 024, 024, 024, 020 },
      // G
      { 037, 021, 021, 025, 027 },
      // H
      { 037, 004, 004, 004, 037 },
      // I
      { 021, 021, 037, 021, 021 },
      // J
      { 003, 001, 001, 001, 037 },
      // K
      { 037, 004, 004, 012, 021 },
      // L
      { 037, 001, 001, 001, 001 },
      // M
      { 037, 010, 004, 010, 037 },
      // N
      { 037, 010, 004, 002, 037 },
      // O
      { 037, 021, 021, 021, 037 },
      // P
      { 037, 024, 024, 024, 034 },
      // Q
      { 037, 021, 025, 023, 037 },
      // R
      { 037, 024, 024, 026, 035 },
      // S
      { 035, 025, 025, 025, 027 },
      // T
      { 020, 020, 037, 020, 020 },
      // U
      { 037, 001, 001, 001, 037 },
      // V
      { 030, 006, 001, 006, 030 },
      // W
      { 037, 002, 004, 002, 037 },
      // X
      { 021, 012, 004, 012, 021 },
      // Y
      { 020, 010, 007, 010, 020 },
      // Z
      { 021, 027, 025, 035, 021 },
      // 0
      { 016, 021, 021, 021, 016 },
      // 1
      { 000, 010, 000, 037, 000 },
      // 2
      { 023, 025, 025, 025, 035 },
      // 3
      { 021, 025, 025, 025, 037 },
      // 4
      { 034, 004, 004, 004, 037 },
      // 5
      { 035, 025, 025, 025, 022 },
      // 6
      { 037, 005, 005, 005, 007 },
      // 7
      { 020, 021, 022, 024, 030 },
      // 8
      { 012, 025, 025, 025, 012 },
      // 9
      { 034, 024, 024, 024, 037 },
      // Underscore
      { 001, 001, 001, 001, 001 },
      // Hyphen
      { 000, 004, 004, 004, 000 },
      // (
      { 000, 004, 012, 021, 000 },
      // )
      { 000, 021, 012, 004, 000 },
      // /
      { 001, 002, 004, 010, 020 }
  };

static char glyph_chars [NUM_GLYPH_CHAR_PATTERNS] =
  {
      '*', ' ', '.', '>', 'A', 'B', 'C', 'D', 'E', 'F',
      'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
      'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
      '_', '-', '(', ')', '/'
  };

static uint8 glyph_char_word_offset [11] =
  {
      24, 22, 19, 17, 15, 12, 10, 8, 5, 3, 1
  };

static uint8 glyph_nibble_offset [11] =
  {
       1,  2,  0,  1,  2,  0,  1, 2, 0, 1, 2
  };

static void remove_spaces(char *str)
{
  int src = 0;
  int dest = 0;
  while (str[src])
    {
      if (str[src] != ' ')
        {
          str[dest++] = str[src];
        }
          src++;
        }
  str[dest] = 0;
}

static void log_char_matrix_pattern(uint8* char_matrix)
  {
    sim_print("\nChar Matrix\n");
    for (uint col_offset = 0; col_offset < CHAR_MATRIX_BYTES; col_offset++)
      {
        sim_printf(" %03o\n", char_matrix[col_offset]);
      }

    sim_print("\r\n");
    for (uint row = 0; row < 5; row++)
      {
        for (uint col = 0; col < CHAR_MATRIX_BYTES; col++)
          {
            if ((char_matrix[col] >> (4 - row)) & 0x1)
              {
                sim_print("*");
              }
            else
              {
                sim_print(" ");
              }
          }
          sim_print("\r\n");
      }
    sim_print("\r\n");

  }

static char search_glyph_patterns(uint8* matrix)
  {
    for (int pattern = 0; pattern < NUM_GLYPH_CHAR_PATTERNS; pattern++)
      {
        if (memcmp(matrix, &glyph_char_patterns[pattern], CHAR_MATRIX_BYTES) == 0)
          {
            return glyph_chars[pattern];
          }
      }

      sim_warn("*** Warning: Punch found unknown block character pattern\n");
      log_char_matrix_pattern(matrix);

      return ' ';
  }

static char get_lace_char(word36* buffer, uint char_pos)
  {
    if (char_pos >= GLYPHS_PER_CARD)
      {
        sim_warn("*** Error: Attempt to read punch block character out of range (%u)\n", char_pos);
        return 0;
      }

    bool top           = char_pos < 11;                              // Top or bottom line of chars
    uint char_offset   = (char_pos < 11) ? char_pos : char_pos - 11; // Character num in the line
    uint word_offset   = glyph_char_word_offset[char_offset];        // Starting word in the buffer
    uint nibble_offset = glyph_nibble_offset[char_offset];           // Starting nibble in the word
    word12 col_buffer[5];                                            // Extracted 5 cols for char

    // Extract the five 12-bit words from the main buffer that make up the character
    // Note that in this process we reverse the character image so it reads normally
    // (characters are punched in reverse)
    for (uint col_offset = 0; col_offset < 5; col_offset++)
      {
        col_buffer[4 - col_offset] = (buffer[word_offset] >> (nibble_offset * 12)) & 0xFFF;
        if (nibble_offset == 0)
          {
            nibble_offset = 2;
            word_offset++;
          }
        else
          {
            nibble_offset--;
          }
      }

    // Now shift the characters into the 5x5 matrix buffer
    uint8 char_matrix[CHAR_MATRIX_BYTES];

    for (uint col_offset = 0; col_offset < CHAR_MATRIX_BYTES; col_offset++)
      {
        char_matrix[col_offset] = (col_buffer[col_offset] >> (top ? 6 : 0)) & 0x1F;
      }

    char c = search_glyph_patterns(char_matrix);

    return c;
  }

static void scan_card_for_glyphs(pun_state_t * state, word36* buffer)
  {
    for (uint c_pos = 0; c_pos < 22; c_pos++)
      {
        char c = get_lace_char(buffer, c_pos);
        uint current_length = (uint)strlen(state -> glyph_buffer);
        if (current_length < (sizeof(state -> glyph_buffer) - 1))
          {
            state -> glyph_buffer[current_length++] = c;
            state -> glyph_buffer[current_length]   = 0;
          }
      }
  }

static void create_punch_file(pun_state_t * state)
  {
    char template [4 * PATH_MAX+1];

    if (state -> punfile_raw != -1)
      {
          sim_warn \
              ("*** Error: Punch file already open when attempting to create new file, closing old file!\n");
          close(state -> punfile_raw);
          state -> punfile_raw = -1;
      }

    if (pun_path_prefix [0])
      {
        sprintf (template, "%s%s/%s.spool.%s.XXXXXX.pun",
                 pun_path_prefix, state -> device_name,
                 state -> device_name,
                 state -> raw_file_name);
      }
    else
      {
        sprintf (template, "%s.spool.%s.XXXXXX.pun",
                 state -> device_name,
                 state -> raw_file_name);
      }

    state -> punfile_raw = utfile_mkstemps(template, 4);
    if (state -> punfile_raw < 0)
      {
        perror("creating punch '.pun' file\n");
      }

  }

static void write_punch_files (pun_state_t * state, word36* in_buffer, int word_count,
                               bool banner_card)
  {
      if (word_count != WORDS_PER_CARD)
        {
          sim_warn ("Unable to interpret punch buffer due to wrong length, not writing output!\n");
          return;
        }

      uint8 byte_buffer[120];
      memset(&byte_buffer, 0, sizeof(byte_buffer));

      word12 word12_buffer[80];
      memset(&word12_buffer, 0, sizeof(word12_buffer));

      for (int nibble_index = 0; nibble_index < (CARD_COL_COUNT * NIBBLES_PER_COL); nibble_index++)
        {
          int byte_offset   = nibble_index / 2;
          int word36_offset = nibble_index / 9;
          int nibble_offset = nibble_index % 9;
          uint8 nibble = (in_buffer[word36_offset] >> ((8 - nibble_offset) * 4)) & 0xF;

          if (nibble_index & 0x1)
            {
              // Low nibble of byte
              byte_buffer[byte_offset] |= nibble;
            }
          else
            {
              // High nibble of byte
              byte_buffer[byte_offset] |= (nibble << 4);
            }

          int word12_offset        =      nibble_index / 3;
          int word12_nibble_offset = 2 - (nibble_index % 3);

          word12_buffer[word12_offset] |= nibble << (word12_nibble_offset * 4);
        }

      if (state->log_cards)
      {
        sim_printf("word12_buffer:\n");
        for (uint i = 0; i < 80; i++)
          {
            sim_printf("  %04o\n", word12_buffer[i]);
          }
        sim_printf("\r\n");
      }

      if (state -> punfile_raw >= 0)
        {
          if (write(state -> punfile_raw, byte_buffer, sizeof(byte_buffer)) \
                  != sizeof(byte_buffer)) {
            sim_warn ("Failed to write to .raw card punch file!\n");
            perror("Writing .raw punch file\n");
          }
        }

  }

static void log_card(word12 tally, word36 * buffer)
  {
    sim_printf ("tally %d\n", tally);

    for (uint i = 0; i < tally; i ++)
      {
        sim_printf ("  %012llo\n", (unsigned long long)buffer [i]);
      }
    sim_printf ("\r\n");

    if (tally != WORDS_PER_CARD)
      {
        sim_warn("Unable to log punch card, tally is not 27 (%d)\n", tally);
        return;
      }

    for (uint row = 0; row < 12; row ++)
      {
        for (uint col = 0; col < 80; col ++)
          {
            // 3 cols/word
            uint wordno  = col / 3;
            uint fieldno = col % 3;
            word1 bit = getbits36_1 (buffer [wordno], fieldno * 12 + row);
            if (bit)
                sim_printf ("*");
            else
                sim_printf (" ");
          }
        sim_printf ("\r\n");
      }
    sim_printf ("\r\n");

    for (uint row = 0; row < 12; row ++)
      {
        //for (uint col = 0; col < 80; col ++)
        for (int col = 79; col >= 0; col --)
          {
            // 3 cols/word
            uint wordno  = (uint) col / 3;
            uint fieldno = (uint) col % 3;
            word1 bit = getbits36_1 (buffer [wordno], fieldno * 12 + row);
            if (bit)
                sim_printf ("*");
            else
                sim_printf (" ");
          }
        sim_printf ("\r\n");
      }
    sim_printf ("\r\n");
  }

static void print_event(enum parse_event event)
  {
    switch (event)
      {
        case NoEvent:
          sim_warn("[No Event]");
          break;
        case BannerCard:
          sim_warn("[Banner Card]");
          break;
        case EndOfDeckCard:
          sim_warn("[End Of Deck Card]");
          break;
        case Card:
          sim_warn("[Card]");
          break;
        case Done:
          sim_warn("[Done]");
          break;
        default:
          sim_warn("[unknown event %d]", event);
          break;
      }
  }

static void print_state(enum parse_state state)
  {
    switch (state)
      {
        case Idle:
          sim_warn("[Idle]");
          break;
        case StartingJob:
          sim_warn("[Starting Job]");
          break;
        case PunchGlyphLookup:
          sim_warn("[Punch Glyph Lookup]");
          break;
        case EndOfHeader:
          sim_warn("[End Of Header]");
          break;
        case CacheCard:
          sim_warn("[Cache Card]");
          break;
        case EndOfDeck:
          sim_warn("[End Of Deck]");
          break;
        case EndOfJob:
          sim_warn("[End Of Job]");
          break;
        default:
          sim_warn("[unknown state %d]", state);
          break;
      }
  }

static void print_transition(enum parse_state old_state, enum parse_event event,
                             enum parse_state new_state)
  {
      sim_warn(">>> Punch Transition: ");
      print_event(event);
      sim_warn(" = ");
      print_state(old_state);
      sim_warn(" -> ");
      print_state(new_state);
      sim_warn("\r\n");
  }

static void clear_card_cache(pun_state_t * state)
  {
    CARD_CACHE_ENTRY *current_entry = state -> first_cached_card;
    while (current_entry != NULL)
      {
        CARD_CACHE_ENTRY *old_entry = current_entry;
        current_entry               = current_entry->next_entry;
        FREE(old_entry);
      }

    state -> first_cached_card = NULL;
    state -> last_cached_card  = NULL;
  }

static void save_card_in_cache(pun_state_t * state, word12 tally, word36 * card_buffer)
  {
    CARD_CACHE_ENTRY *new_entry = malloc(sizeof(CARD_CACHE_ENTRY));
    if (!new_entry)
      {
        fprintf(stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# ifdef SIGUSR2
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
# endif /* ifdef SIGUSR2 */
#endif /* if defined(USE_BACKTRACE) */
        abort();
      }

    new_entry -> tally = tally;
    memcpy(&new_entry -> card_data, card_buffer, sizeof(word36) * tally);
    new_entry -> next_entry = NULL;

    if (state -> first_cached_card == NULL)
      {
        state -> first_cached_card = new_entry;
        state -> last_cached_card  = new_entry;
      }
    else
      {
        state -> last_cached_card -> next_entry = new_entry;
        state -> last_cached_card               = new_entry;
      }
  }

static void transition_state(enum parse_event event, pun_state_t * state,
                             enum parse_state new_state)
  {
    if (state -> log_cards)
      {
        print_transition(state -> current_state, event, new_state);
      }

    state -> current_state = new_state;
  }

static enum parse_event do_state_idle(enum parse_event event, pun_state_t * state)
  {
    transition_state(event, state, Idle);

    // No Action

    return NoEvent;
  }

static enum parse_event do_state_starting_job(enum parse_event event,
                                              pun_state_t * state, word12 tally,
                                              word36 * card_buffer)
  {
    transition_state(event, state, StartingJob);

    clear_card_cache(state);                            // Clear card cache
    state -> glyph_buffer[0] = 0;                       // Clear Glyph Buffer
    save_card_in_cache(state, tally, card_buffer);      // Save card in cache

    return NoEvent;
  }

static enum parse_event do_state_scan_card_for_glyphs(enum parse_event event,
                                                      pun_state_t * state,
                                                      word12 tally,
                                                      word36 * card_buffer)
  {
    transition_state(event, state, PunchGlyphLookup);

    scan_card_for_glyphs(state, card_buffer);

    save_card_in_cache(state, tally, card_buffer);    // Save card in cache

    return NoEvent;
  }

static enum parse_event do_state_end_of_header(enum parse_event event,
                                               pun_state_t * state,
                                               word12 tally,
                                               word36 * card_buffer)
  {
    transition_state(event, state, EndOfHeader);

    save_card_in_cache(state, tally, card_buffer);      // Save card in cache

    if (state -> log_cards)
      {
        sim_printf("\n++++ Glyph Buffer ++++\n'%s'\n", state -> glyph_buffer);
      }

    char punch_file_name[PATH_MAX+1];
    if (strlen(state -> glyph_buffer) < 86)
      {
        sim_warn \
            ("*** Punch: glyph buffer too short, unable to parse file name '%s'\n", state -> glyph_buffer);
        punch_file_name[0] = 0;
      }
    else
      {
        sprintf(punch_file_name, "%5.5s.%22.22s",
            &state -> glyph_buffer[14],
            &state -> glyph_buffer[88]
        );
        remove_spaces(punch_file_name);
      }

    strncpy(state -> raw_file_name, punch_file_name, sizeof(state -> raw_file_name));

    create_punch_file(state);                           // Create spool file

    // Write cached cards to spool file
    CARD_CACHE_ENTRY *current_entry = state -> first_cached_card;
    while (current_entry != NULL)
      {
        write_punch_files (state, current_entry -> card_data, WORDS_PER_CARD, true);
        current_entry = current_entry->next_entry;
      }

    clear_card_cache(state);                            // Clear card cache

    return NoEvent;
  }

static enum parse_event do_state_cache_card(enum parse_event event,
                                            pun_state_t * state,
                                            word12 tally,
                                            word36 * card_buffer)
  {
    transition_state(event, state, CacheCard);

    save_card_in_cache(state, tally, card_buffer);      // Save card in cache

    return NoEvent;
  }

static enum parse_event do_state_end_of_deck(enum parse_event event,
                                             pun_state_t * state,
                                             word12 tally,
                                             word36 * card_buffer)
  {
    transition_state(event, state, EndOfDeck);

    save_card_in_cache(state, tally, card_buffer);      // Save card in cache

    return NoEvent;
  }

static enum parse_event do_state_end_of_job(enum parse_event event,
                                            pun_state_t * state,
                                            word12 tally,
                                            word36 * card_buffer)
  {
    transition_state(event, state, EndOfJob);

    // Write cached cards to spool file
    CARD_CACHE_ENTRY *current_entry = state -> first_cached_card;
    while (current_entry != NULL)
      {
        write_punch_files (state, current_entry -> card_data, WORDS_PER_CARD,
                (current_entry -> next_entry == NULL));
        current_entry = current_entry->next_entry;
      }

    clear_card_cache(state);                                // Clear card cache

    write_punch_files (state, card_buffer, tally, true);    // Write card to spool file

    // Close punch files
    if (state -> punfile_raw >= 0)
      {
        close (state -> punfile_raw);
        state -> punfile_raw = -1;
      }

    return Done;
  }

static void unexpected_event(enum parse_event event, pun_state_t * state)
  {
    sim_warn("*** Punch: Unexpected event ");
    print_event(event);

    sim_warn(" in state ");
    print_state(state -> current_state);

    sim_warn("***\n");
  }

static void parse_card(pun_state_t * state, word12 tally, word36 * card_buffer)
  {
    enum parse_event event = Card;

    if (tally == WORDS_PER_CARD && memcmp (card_buffer, eodCard, sizeof (eodCard)) == 0)
      {
        event = EndOfDeckCard;
      }

    if (tally == WORDS_PER_CARD && memcmp (card_buffer, bannerCard, sizeof (bannerCard)) == 0)
      {
        event = BannerCard;
      }

    while (event != NoEvent)
      {
        enum parse_event current_event = event;
        event = NoEvent;

        switch (current_event)
          {
            case BannerCard:
              switch (state -> current_state)
                {
                  case Idle:
                    event = do_state_starting_job(current_event, state, tally, card_buffer);
                    break;

                  case PunchGlyphLookup:
                    event = do_state_end_of_header(current_event, state, tally, card_buffer);
                    break;

                  case EndOfDeck:
                    event = do_state_end_of_job(current_event, state, tally, card_buffer);
                    break;

                  default:
                    unexpected_event(current_event, state);
                    break;
                }
              break;

            case EndOfDeckCard:
              switch (state -> current_state)
                {
                  case StartingJob:
                    event = do_state_end_of_deck(current_event, state, tally, card_buffer);
                    break;

                  case PunchGlyphLookup:
                    event = do_state_end_of_deck(current_event, state, tally, card_buffer);
                    break;

                  case EndOfHeader:
                    event = do_state_end_of_deck(current_event, state, tally, card_buffer);
                    break;

                  case CacheCard:
                    event = do_state_end_of_deck(current_event, state, tally, card_buffer);
                    break;

                  case EndOfDeck:
                    event = do_state_end_of_deck(current_event, state, tally, card_buffer);
                    break;

                  default:
                    unexpected_event(current_event, state);
                    break;
                }
              break;

            case Card:
              switch (state -> current_state)
                {
                  case StartingJob:
                    event = do_state_scan_card_for_glyphs(current_event, state, tally, card_buffer); //-V1037
                    break;

                  case PunchGlyphLookup:
                    event = do_state_scan_card_for_glyphs(current_event, state, tally, card_buffer); //-V1037
                    break;

                  case EndOfHeader:
                    event = do_state_cache_card(current_event, state, tally, card_buffer); //-V1037
                    break;

                  case CacheCard:
                    event = do_state_cache_card(current_event, state, tally, card_buffer); //-V1037
                    break;

                  case EndOfDeck:
                    event = do_state_cache_card(current_event, state, tally, card_buffer); //-V1037
                    break;

                  default:
                    unexpected_event(current_event, state);
                    break;
                }
              break;

            case Done:
              switch (state -> current_state)
                {
                  case EndOfJob:
                    event = do_state_idle(current_event, state);
                    break;

                  default:
                    unexpected_event(current_event, state);
                    break;
                }
              break;

            default:
              sim_warn("*** Error: Punch received unknown event!\n");
              break;
          }
      }

  }

static int punWriteRecord (uint iomUnitIdx, uint chan)
  {
    iom_chan_data_t * p = & iom_chan_data [iomUnitIdx] [chan];
    uint dev_code       = p->IDCW_DEV_CODE;
    uint ctlr_unit_idx  = get_ctlr_idx (iomUnitIdx, chan);
    uint devUnitIdx     = cables->urp_to_urd[ctlr_unit_idx][dev_code].unit_idx;
    UNIT * unitp        = & pun_unit [devUnitIdx];
    long pun_unit_num   = PUN_UNIT_NUM (unitp);

    p -> isRead = false;
    if (p -> DDCW_TALLY != WORDS_PER_CARD)
      {
        sim_warn ("%s expected tally of 27\n", __func__);
        p -> chanStatus = chanStatIncorrectDCW;
        p -> stati = 05001; // BUG: arbitrary error code; config switch
        return -1;
      }

//dcl 1 raw aligned,    /* raw column binary card image */
//    2 col (1:80) bit (12) unal,                             /* 80 columns */
//    2 pad bit (12) unal;

    // Copy from core to buffer
    word36 buffer [p -> DDCW_TALLY];
    uint wordsProcessed = 0;
    iom_indirect_data_service (iomUnitIdx, chan, buffer, & wordsProcessed, false);
    p->initiate         = false;

    if (pun_state [pun_unit_num] . log_cards)
      {
        log_card(p -> DDCW_TALLY, buffer);
      }

    parse_card( &pun_state [pun_unit_num], p -> DDCW_TALLY, buffer);

    p -> stati = 04000;
    return 0;
  }

iom_cmd_rc_t pun_iom_cmd (uint iomUnitIdx, uint chan) {
  iom_cmd_rc_t rc     = IOM_CMD_PROCEED;
  iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan];
  uint dev_code       = p->IDCW_DEV_CODE;

  sim_debug (DBG_TRACE, & pun_dev, "%s: PUN %c%02o_%02o\n",
          __func__, iomChar (iomUnitIdx), chan, dev_code);

  uint ctlr_unit_idx   = get_ctlr_idx (iomUnitIdx, chan);
  uint devUnitIdx      = cables->urp_to_urd[ctlr_unit_idx][dev_code].unit_idx;
  pun_state_t * statep = & pun_state[devUnitIdx];

  // IDCW?
  if (IS_IDCW (p)) {
    // IDCW
    statep->ioMode = punNoMode;
    switch (p->IDCW_DEV_CMD) {

      case 011: // CMD 011 Punch binary
        sim_debug (DBG_DEBUG, & pun_dev, "%s: Punch Binary\n", __func__);
        statep->ioMode = punWrBin;
        p->stati       = 04000;
        break;

      case 031: // CMD 031 Set Diagnostic Mode (load_mpc.pl1)
        sim_debug (DBG_DEBUG, & pun_dev, "%s: Set Diagnostic Mode\n", __func__);
        p->stati = 04000;
        break;

      case 040: // CMD 40 Reset status
        sim_debug (DBG_DEBUG, & pun_dev, "%s: Reset Status\n", __func__);
        p->stati  = 04000;
        p->isRead = false;
        break;

      default:
        if (p->IDCW_DEV_CMD != 051) // ignore bootload console probe
          sim_warn ("%s: PUN unrecognized device command  %02o\n", __func__, p->IDCW_DEV_CMD);
        p->stati      = 04501; // cmd reject, invalid opcode
        p->chanStatus = chanStatIncorrectDCW;
        return IOM_CMD_ERROR;
    } // switch IDCW_DEV_CMD
    sim_debug (DBG_DEBUG, & pun_dev, "%s: stati %04o\n", __func__, p->stati);
    return IOM_CMD_PROCEED;
  } // IDCW

  // Not IDCW; TDCW are captured in IOM, so must be IOTD, IOTP or IOTNP
  switch (statep->ioMode) {

    case punNoMode:
      //sim_printf ("%s: Unexpected IOTx\n", __func__);
      //sim_warn ("%s: Unexpected IOTx\n", __func__);
      //return IOM_CMD_ERROR;
      break;

    case punWrBin: {
        int rc = punWriteRecord (iomUnitIdx, chan);
        if (rc)
          return IOM_CMD_ERROR;
      }
      break;

    default:
      sim_warn ("%s: Unrecognized ioMode %d\n", __func__, statep->ioMode);
      return IOM_CMD_ERROR;
  }
  return rc;
}

static t_stat pun_show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr, UNUSED int val,
                               UNUSED const void * desc)
  {
    sim_printf("Number of PUN units in system is %d\n", pun_dev . numunits);
    return SCPE_OK;
  }

static t_stat pun_set_nunits (UNUSED UNIT * uptr, UNUSED int32 value, const char * cptr,
                              UNUSED void * desc)
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
    sim_printf("name     : %s", pun_state [n] . device_name);
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

static t_stat pun_set_path (UNUSED UNIT * uptr, UNUSED int32 value,
                            const UNUSED char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;

    size_t len = strlen(cptr);

    // Verify that we don't exceed the maximum prefix
    // size ( -2 for the null terminator and a possible '/')
    if (len >= (sizeof(pun_path_prefix) - 2))
      return SCPE_ARG;

    strncpy(pun_path_prefix, cptr, sizeof(pun_path_prefix) - 1);
    if (len > 0)
      {
        if (pun_path_prefix[len - 1] != '/')
          {
            pun_path_prefix[len++] = '/';
            pun_path_prefix[len] = 0;
          }
      }
    return SCPE_OK;
  }

static t_stat pun_show_path (UNUSED FILE * st, UNUSED UNIT * uptr,
                                       UNUSED int val, UNUSED const void * desc)
  {
    sim_printf("Path to card punch directories is %s\n", pun_path_prefix);
    return SCPE_OK;
  }

static t_stat pun_set_config (UNUSED UNIT *  uptr, UNUSED int32 value,
                              const char * cptr, UNUSED void * desc)
  {
    int devUnitIdx           = (int) PUN_UNIT_NUM (uptr);
    pun_state_t * psp        = pun_state + devUnitIdx;
    config_state_t cfg_state = { NULL, NULL };

    for (;;)
      {
        int64_t v;
        int rc = cfg_parse (__func__, cptr, pun_config_list, & cfg_state, & v);
        if (rc == -1) // done
          break;

        if (rc == -2) // error
          {
            cfg_parse_done (& cfg_state);
            return SCPE_ARG;
          }
        const char * p = pun_config_list[rc].name;

        if (strcmp (p, "logcards") == 0)
          {
            psp->log_cards = v != 0;
            continue;
          }

        sim_warn ("error: pun_set_config: Invalid cfg_parse rc <%ld>\n",
                  (long) rc);
        cfg_parse_done (& cfg_state);
        return SCPE_ARG;
      } // process statements
    cfg_parse_done (& cfg_state);
    return SCPE_OK;
  }

static t_stat pun_show_config (UNUSED FILE * st, UNUSED UNIT * uptr,
                               UNUSED int  val, UNUSED const void * desc)
  {
    int devUnitIdx = (int) PUN_UNIT_NUM (uptr);
    pun_state_t * psp = pun_state + devUnitIdx;
    sim_msg ("logcards : %d", psp->log_cards);
    return SCPE_OK;
  }
