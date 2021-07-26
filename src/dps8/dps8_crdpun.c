/*
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2021 Dean Anderson
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
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
 * Copyright (c) 2007-2013 Michael Mondy
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 */

#define N_PUN_UNITS 1 // default

static t_stat pun_reset (DEVICE * dptr);
static t_stat pun_show_nunits (FILE *st, UNIT *uptr, int val, const void *desc);
static t_stat pun_set_nunits (UNIT * uptr, int32 value, const char * cptr, void * desc);
static t_stat pun_show_device_name (FILE *st, UNIT *uptr, int val, const void *desc);
static t_stat pun_set_device_name (UNIT * uptr, int32 value, const char * cptr, void * desc);
static t_stat pun_show_path (UNUSED FILE * st, UNIT * uptr, UNUSED int val, UNUSED const void * desc);
static t_stat pun_set_path (UNUSED UNIT * uptr, UNUSED int32 value, const UNUSED char * cptr, UNUSED void * desc);
static t_stat pun_set_config (UNUSED UNIT *  uptr, UNUSED int32 value, const char * cptr, UNUSED void * desc);
static t_stat pun_show_config (UNUSED FILE * st, UNUSED UNIT * uptr, UNUSED int  val, UNUSED const void * desc);

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
      "Set the punch device name", /* value descriptor */
      NULL          // help
    },
    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO | MTAB_VALR | MTAB_NC, /* mask */
      0,            /* match */
      "PATH",     /* print string */
      "PATH",         /* match string */
      pun_set_path, /* validation routine */
      pun_show_path, /* display routine */
      "Path to card punch directories", /* value descriptor */
      NULL // Help
    },
    {
      MTAB_XTD | MTAB_VUN, /* mask */
      0,            /* match */
      (char *) "CONFIG",     /* print string */
      (char *) "CONFIG",         /* match string */
      pun_set_config,         /* validation routine */
      pun_show_config, /* display routine */
      NULL,          /* value descriptor */
      NULL,            /* help */
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
    pun_reset,    /* reset */
    NULL,         /* boot */
    NULL,         /* attach */
    NULL,         /* detach */
    NULL,         /* context */
    DEV_DEBUG,    /* flags */
    0,            /* debug control flags */
    pun_dt,       /* debug flag names */
    NULL,         /* memory size change */
    NULL,         /* logical name */
    NULL,         // help
    NULL,         // attach help
    NULL,         // attach context
    NULL,         // description
    NULL
};

static config_value_list_t cfg_on_off[] =
  {
    { "off", 0 },
    { "on", 1 },
    { "disable", 0 },
    { "enable", 1 },
    { NULL, 0 }
  };

static config_list_t pun_config_list[] =
  {
   { "logcards", 0, 1, cfg_on_off },
   { NULL, 0, 0, NULL }
  };

#define WORDS_PER_CARD 27
#define MAX_OCR_BUFFER_LEN 1024
#define CARD_COL_COUNT 80
#define NIBBLES_PER_COL 3
#define OCR_CHARS_PER_CARD 22

enum parse_state {
    Idle, StartingJob, OcrCard, EndOfHeader, WritingDeck, EndOfDeck, EndOfJob
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
    char device_name [MAX_DEV_NAME_LEN];
    int punfile_raw; // fd
    bool log_cards; // Flag to log card images
    bool sawEOD;
    enum parse_state current_state;
    char ocr_buffer[MAX_OCR_BUFFER_LEN];
    CARD_CACHE_ENTRY *first_cached_card;
    CARD_CACHE_ENTRY *last_cached_card;
  } pun_state_t ;
  
static pun_state_t pun_state[N_PUN_UNITS_MAX];
static char pun_name[] = "pun";
static char pun_file_name_template[] = "spool.XXXXXX";
static char pun_path_prefix[PATH_MAX+1];

/*
 * pun_init()
 *
 */

// Once-only initialization

void pun_init (void)
  {
    memset (pun_path_prefix, 0, sizeof (pun_path_prefix));
    memset (pun_state, 0, sizeof (pun_state));
    for (int i = 0; i < N_PUN_UNITS_MAX; i ++)
      {
        pun_state [i] . punfile_raw = -1;
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

static uint8 ocr_char_word_offset [11] =
  {
      24, 22, 19, 17, 15, 12, 10, 8, 5, 3, 1
  };

static uint8 ocr_nibble_offset [11] =
  {
       1,  2,  0,  1,  2,  0,  1, 2, 0, 1, 2
  };
  
static uint8 get_lace_char(word36* buffer, uint char_pos)
  {
    if (char_pos >= OCR_CHARS_PER_CARD)
      {
        sim_warn("*** Error: Attempt to read punch OCR character out of range (%u)\n", char_pos);
        return 0;
      }

    bool top = char_pos < 11;                                       // Top or bottom line of characters
    uint char_offset = (char_pos < 11) ? char_pos : char_pos - 11;  // Character number in the line
    uint word_offset = ocr_char_word_offset[char_offset];           // Starting word in the buffer
    uint nibble_offset = ocr_nibble_offset[char_offset];            // Starting nibble in the word
    word12 col_buffer[5];                                           // The extracted 5 columns for the character

    // Extract the five 12-bit words from the main buffer that make up the character
    // Note that in this process we reverse the character image so it reads normally
    // (characters are punched in reverse)
    for (uint col_offset = 0; col_offset < 5; col_offset++)
      {
        col_buffer[4 - col_offset] = (buffer[word_offset] >> (nibble_offset * 12)) & 0xFFF;
        sim_printf(">> col_offset = %u, word_offset = %u, nibble_offset = %u, buffer[word_offset] = %012"PRIo64"\n",
            col_offset, word_offset, nibble_offset, buffer[word_offset]);
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
    uint8 char_matrix[5];

    sim_print("\nCol Matrix => Char Matrix\n");
    for (uint col_offset = 0; col_offset < 5; col_offset++)
      {
        char_matrix[col_offset] = (col_buffer[col_offset] >> (top ? 6 : 0)) & 0x1F;
        sim_printf(" %05o  =>  %03o\n", col_buffer[col_offset], char_matrix[col_offset]);
      }

    sim_print("\r\n");
    for (uint row = 0; row < 5; row++)
      {
        for (uint col = 0; col < 5; col++)
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

    return 0;
  }

static void ocr_card(word36* buffer)
  {
    for (uint c = 0; c < 22; c++)
      {
        get_lace_char(buffer, c);
      }
  }  

static void create_punch_file(pun_state_t * state)
  {
    char template [PATH_MAX+1];

    if (state -> punfile_raw != -1)
      {
          sim_warn("*** Error: Punch file already open when attempting to create new file, closing old file!\n");
          close(state -> punfile_raw);
          state -> punfile_raw = -1;
      }

    if (pun_path_prefix [0])
      {
        sprintf (template, "%s%s/%s", pun_path_prefix, state -> device_name, pun_file_name_template);
      }
    else
      {
        sprintf (template, "%s.%s", 'a' + state -> device_name, pun_file_name_template);
      }

    state -> punfile_raw = mkstemp (template);
  }  


static void write_punch_file (int fd, word36* in_buffer, int word_count)
  {
      if (word_count != WORDS_PER_CARD)
        {
          sim_warn ("Unable to interpret punch buffer due to wrong length, not writing output!\n");
          return;
        }

      uint8 out_buffer[120];
      memset(&out_buffer, 0, sizeof(out_buffer));

      for (int nibble_index = 0; nibble_index < (CARD_COL_COUNT * NIBBLES_PER_COL); nibble_index++)
        {
          int byte_offset = nibble_index / 2;
          int word36_offset = nibble_index / 9;
          int nibble_offset = nibble_index % 9;
          uint8 nibble = (in_buffer[word36_offset] >> ((8 - nibble_offset) * 4)) & 0xF;

          if (nibble_index & 0x1) 
            {
              // Low nibble of byte
              out_buffer[byte_offset] |= nibble;
            }
          else
            {
              // High nibble of byte
              out_buffer[byte_offset] |= (nibble << 4);
            }

        }

      if (write(fd, out_buffer, sizeof(out_buffer)) != sizeof(out_buffer)) {
        sim_warn ("Failed to write to card punch file!\n");
      }
  }  

static void log_card(word12 tally, word36 * buffer)
  {
    sim_printf ("tally %d\n", tally);

    for (uint i = 0; i < tally; i ++)
      {
        sim_printf ("  %012"PRIo64"\n", buffer [i]);
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
            uint wordno = col / 3;
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
            uint wordno = (uint) col / 3;
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
          sim_print("[No Event]");
          break;
        case BannerCard:
          sim_print("[Banner Card]");
          break;
        case EndOfDeckCard:
          sim_print("[End Of Deck Card]");
          break;
        case Card:
          sim_print("[Card]");
          break;
        case Done:
          sim_print("[Done]");
          break;
        default:
          sim_printf("[unknown event %d]", event);
          break;
      }
  }

static void print_state(enum parse_state state)
  {
    switch (state)
      {
        case Idle:
          sim_print("[Idle]");
          break;
        case StartingJob:
          sim_print("[Starting Job]");
          break;
        case OcrCard:
          sim_print("[Ocr Card]");
          break;
        case EndOfHeader:
          sim_print("[End Of Header]");
          break;
        case WritingDeck:
          sim_print("[Writing Deck]");
          break;
        case EndOfDeck:
          sim_print("[End Of Deck]");
          break;
        case EndOfJob:
          sim_print("[End Of Job]");
          break;
        default:
          sim_printf("[unknown state %d]", state);
          break;
      }
  }

static void print_transition(enum parse_state old_state, enum parse_event event, enum parse_state new_state)
  {
      sim_print(">>> Punch Transition: ");
      print_event(event);
      sim_print(" = ");
      print_state(old_state);
      sim_print(" -> ");
      print_state(new_state);
      sim_print("\r\n");
  }

static void clear_card_cache(pun_state_t * state)
  {
    CARD_CACHE_ENTRY *current_entry = state -> first_cached_card;
    while (current_entry != NULL)
      {
        CARD_CACHE_ENTRY *old_entry = current_entry;
        current_entry = current_entry->next_entry;
        free(old_entry);
      }

    state -> first_cached_card = NULL;
    state -> last_cached_card = NULL;

  }

static void dump_card_cache(pun_state_t * state)
  {
    sim_printf("****\nCard Cache Dump\n****\n");

    int card = 0;

    CARD_CACHE_ENTRY *current_entry = state -> first_cached_card;
    while (current_entry != NULL)
      {
        card++;
        sim_printf("\n----Card %d----\n", card);
        for (int i = 0; i < WORDS_PER_CARD; i ++)
          {
            sim_printf ("  %012"PRIo64"\n", current_entry -> card_data[i]);
          }
        sim_printf ("\r\n");
        current_entry = current_entry->next_entry;
      }
  }  

static void save_card_in_cache(pun_state_t * state, word12 tally, word36 * card_buffer)
  {
    CARD_CACHE_ENTRY *new_entry = malloc(sizeof(CARD_CACHE_ENTRY));

    new_entry -> tally = tally;
    memcpy(&new_entry -> card_data, card_buffer, sizeof(word36) * tally);
    new_entry -> next_entry = NULL;

    if (state -> first_cached_card == NULL)
      {
        state -> first_cached_card = new_entry;
        state -> last_cached_card = new_entry;
      }
    else
      {
        state -> last_cached_card -> next_entry = new_entry;
        state -> last_cached_card = new_entry;
      }
  }

static enum parse_event do_state_idle(enum parse_event event, pun_state_t * state)
  {
    print_transition(state -> current_state, event, Idle);
    state -> current_state = Idle;

    return NoEvent;
  }

static enum parse_event do_state_starting_job(enum parse_event event, pun_state_t * state, word12 tally, word36 * card_buffer)
  {
    print_transition(state -> current_state, event, StartingJob);
    state -> current_state = StartingJob;

    clear_card_cache(state);                            // Clear card cache
    state -> ocr_buffer[0] = 0;                         // Clear OCR Buffer
    save_card_in_cache(state, tally, card_buffer);      // Save card in cache

    return NoEvent;
  }

static enum parse_event do_state_ocr_card(enum parse_event event, pun_state_t * state, word12 tally, word36 * card_buffer)
  {
    print_transition(state -> current_state, event, OcrCard);
    state -> current_state = OcrCard;

    ocr_card(card_buffer);

    save_card_in_cache(state, tally, card_buffer);      // Save card in cache

    return NoEvent;
  }

static enum parse_event do_state_end_of_header(enum parse_event event, pun_state_t * state, word12 tally, word36 * card_buffer)
  {
    print_transition(state -> current_state, event, EndOfHeader);
    state -> current_state = EndOfHeader;

    save_card_in_cache(state, tally, card_buffer);      // Save card in cache

    //TODO: Parse OCR Buffer into file name

    create_punch_file(state);                           // Create spool file

    // Write cached cards to spool file
    CARD_CACHE_ENTRY *current_entry = state -> first_cached_card;
    while (current_entry != NULL)
      {
        write_punch_file (state -> punfile_raw, current_entry -> card_data, WORDS_PER_CARD);
        current_entry = current_entry->next_entry;
      }

    dump_card_cache(state);

    clear_card_cache(state);                            // Clear card cache

    return NoEvent;
  }

static enum parse_event do_state_writing_deck(enum parse_event event, pun_state_t * state, word12 tally, word36 * card_buffer)
  {
    print_transition(state -> current_state, event, WritingDeck);
    state -> current_state = WritingDeck;

    write_punch_file (state -> punfile_raw, card_buffer, tally);    // Write card to spool file

    return NoEvent;
  }

static enum parse_event do_state_end_of_deck(enum parse_event event, pun_state_t * state, word12 tally, word36 * card_buffer)
  {
    print_transition(state -> current_state, event, EndOfDeck);
    state -> current_state = EndOfDeck;

    write_punch_file (state -> punfile_raw, card_buffer, tally);    // Write card to spool file

    return NoEvent;
  }

static enum parse_event do_state_end_of_job(enum parse_event event, pun_state_t * state, word12 tally, word36 * card_buffer)
  {
    print_transition(state -> current_state, event, EndOfJob);
    state -> current_state = EndOfJob;

    write_punch_file (state -> punfile_raw, card_buffer, tally);    // Write card to spool file

    // Close spool file
    close (state -> punfile_raw);
    state -> punfile_raw = -1;

    return Done;
  }


static void unexpected_event(enum parse_event event, pun_state_t * state)
  {
    sim_print("*** Punch: Unexpected event ");
    print_event(event);

    sim_print(" in state ");
    print_state(state -> current_state);

    sim_print("***\n");
  }  

static void parse_card(pun_state_t * state, word12 tally, word36 * card_buffer)
  {
    enum parse_event event = Card;

    if (tally == WORDS_PER_CARD && memcmp (card_buffer, eodCard, sizeof (eodCard)) == 0)
      {
        sim_warn("*** Found End Of Deck Card ***\n");
        event = EndOfDeckCard;
      }

    if (tally == WORDS_PER_CARD && memcmp (card_buffer, bannerCard, sizeof (bannerCard)) == 0)
      {
        sim_warn("*** Found Banner Card ***\n");
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

                  case OcrCard:
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

                  case OcrCard:
                    event = do_state_end_of_deck(current_event, state, tally, card_buffer);
                    break;

                  case EndOfHeader:
                    event = do_state_end_of_deck(current_event, state, tally, card_buffer);
                    break;

                  case WritingDeck:
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
                    event = do_state_ocr_card(current_event, state, tally, card_buffer);
                    break;

                  case OcrCard:
                    event = do_state_ocr_card(current_event, state, tally, card_buffer);
                    break;

                  case EndOfHeader:
                    event = do_state_writing_deck(current_event, state, tally, card_buffer);
                    break;

                  case WritingDeck:
                    event = do_state_writing_deck(current_event, state, tally, card_buffer);
                    break;

                  case EndOfDeck:
                    event = do_state_writing_deck(current_event, state, tally, card_buffer);
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

            if (p -> DDCW_TALLY != WORDS_PER_CARD)
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
            p -> initiate = false;

            if (pun_state [pun_unit_num] . log_cards)
              {
                log_card(p -> DDCW_TALLY, buffer);
              }

            parse_card( &pun_state [pun_unit_num], p -> DDCW_TALLY, buffer);

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

static t_stat pun_set_path (UNUSED UNIT * uptr, UNUSED int32 value,
                                    const UNUSED char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;

    size_t len = strlen(cptr);

    // We check for legnth - (3 + length of pun_name + 1) to allow for the null, a possible '/' being added, "punx" and the file name being added
    if (len >= (sizeof(pun_path_prefix) - (strlen(pun_name) + strlen(pun_file_name_template) + 4)))
      return SCPE_ARG;

    strncpy(pun_path_prefix, cptr, sizeof(pun_path_prefix));
    if (len > 0)
      {
        if (pun_path_prefix[len - 1] != '/')
          {
            if (len == sizeof(pun_path_prefix) - 1)
              return SCPE_ARG;
            pun_path_prefix[len++] = '/';
            pun_path_prefix[len] = 0;
          }
      }
    return SCPE_OK;
  }

static t_stat pun_show_path (UNUSED FILE * st, UNIT * uptr,
                                       UNUSED int val, UNUSED const void * desc)
  {
    sim_printf("Path to card punch directories is %s\n", pun_path_prefix);
    return SCPE_OK;
  }

static t_stat pun_set_config (UNUSED UNIT *  uptr, UNUSED int32 value,
                              const char * cptr, UNUSED void * desc)
  {
    int devUnitIdx = (int) PUN_UNIT_NUM (uptr);
    pun_state_t * psp = pun_state + devUnitIdx;
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

        sim_warn ("error: pun_set_config: invalid cfg_parse rc <%d>\n",
                  rc);
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
    sim_msg ("logcards:  %d\n", psp->log_cards);
    return SCPE_OK;
  }
