/*
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>

#define CARD_COL_COUNT 80
#define NIBBLES_PER_COL 3
#define NIBBLES_PER_CARD (CARD_COL_COUNT * NIBBLES_PER_COL)
#define BYTES_PER_CARD (NIBBLES_PER_CARD / 2)
#define CHAR_MATRIX_BYTES 5
#define GLYPHS_PER_CARD 22
#define MAX_GLYPH_BUFFER_LEN 1024

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef uint16 word12;
typedef unsigned int uint;

typedef struct
{
    word12 column[CARD_COL_COUNT];
} card_image_t;

enum parse_state
{
    Idle,
    StartingJob,
    PunchGlyphLookup,
    EndOfHeader,
    CacheCard,
    EndOfDeck,
    EndOfJob
};

enum parse_event
{
    NoEvent,
    BannerCard,
    EndOfDeckCard,
    Card,
    Done
};

typedef struct card_cache_node CARD_CACHE_ENTRY;

struct card_cache_node
{
    card_image_t *card;
    CARD_CACHE_ENTRY *next_entry;
};

typedef struct
{
    CARD_CACHE_ENTRY *first_cache_card;
    CARD_CACHE_ENTRY *last_cache_card;
} CARD_CACHE;

enum parse_state current_state = Idle;

char glyph_buffer[MAX_GLYPH_BUFFER_LEN];

CARD_CACHE banner_card_cache;
CARD_CACHE data_card_cache;
CARD_CACHE trailer_card_cache;

// Card image of the banner card
static word12 banner_card[] =
    {
        00005,
        00000,
        00000,
        07700,
        07704,
        07704,
        07704,
        07700,
        00000,
        00000,
        00077,
        00477,
        00477,
        00477,
        00077,
        00000,
        00000,
        07700,
        07704,
        07704,
        07704,
        07700,
        00000,
        00000,
        00077,
        00477,
        00477,
        00477,
        00077,
        00000,
        00000,
        07700,
        07704,
        07704,
        07704,
        07700,
        00000,
        00000,
        00077,
        00477,
        00477,
        00477,
        00077,
        00000,
        00000,
        07700,
        07704,
        07704,
        07704,
        07700,
        00000,
        00000,
        00077,
        00477,
        00477,
        00477,
        00077,
        00000,
        00000,
        07700,
        07704,
        07704,
        07704,
        07700,
        00000,
        00000,
        00077,
        00477,
        00477,
        00477,
        00077,
        00000,
        00000,
        07700,
        07704,
        07704,
        07704,
        07700,
        00000,
        00005,
};

// Card image of the End Of Deck card
static word12 end_of_deck_card[] =
    {
        00005,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        02000,
        02400,
        02400,
        02400,
        03700,
        00000,
        00000,
        03721,
        02112,
        02104,
        02104,
        03737,
        00000,
        00000,
        00021,
        00021,
        00021,
        00021,
        00037,
        00000,
        00000,
        01621,
        02125,
        02125,
        02125,
        03737,
        00000,
        00000,
        03716,
        00221,
        00421,
        01021,
        03737,
        00000,
        00000,
        02100,
        02500,
        02500,
        02500,
        03700,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00000,
        00005,
};

/*
 *   MCC Punch Codes -> ASCI Conversion Table
 * The table entry is the MCC Punch Code and the 
 * index of the entry is the ASCII character code.
 */

static word12 mcc_punch_codes[128] =
    {
        05403, // ?   (9-12-0-8-1)  0xb03  2819
        04401, // ?   (9-12-1)  0x901  2305
        04201, // ?   (9-12-2)  0x881  2177
        04101, // ?   (9-12-3)  0x841  2113
        00005, // ?   (9-7)  0x5  5
        01023, // ?   (9-0-8-5)  0x213  531
        01013, // ?   (9-0-8-6)  0x20b  523
        01007, // ?   (9-0-8-7)  0x207  519
        02011, // ?   (9-11-6)  0x409  1033
        04021, // ?   (9-12-5)  0x811  2065
        02021, // ?   (9-11-5)  0x411  1041
        04103, // ?   (9-12-8-3)  0x843  2115
        04043, // ?   (9-12-8-4)  0x823  2083
        04023, // ?   (9-12-8-5)  0x813  2067
        04013, // ?   (9-12-8-6)  0x80b  2059
        04007, // ?   (9-12-8-7)  0x807  2055
        06403, // ?   (12-11-9-8-1)  0xd03  3331
        02401, // ?   (9-11-1)  0x501  1281
        02201, // ?   (9-11-2)  0x481  1153
        02101, // ?   (9-11-3)  0x441  1089
        00043, // ?   (9-8-4)  0x23  35
        00023, // ?   (9-8-5)  0x13  19
        00201, // ?   (9-2)  0x81  129
        01011, // ?   (9-0-6)  0x209  521
        02003, // ?   (9-11-8)  0x403  1027
        02403, // ?   (9-11-8-1)  0x503  1283
        00007, // ?   (9-8-7)  0x7  7
        01005, // ?   (9-0-7)  0x205  517
        02043, // ?   (9-11-8-4)  0x423  1059
        02023, // ?   (9-11-8-5)  0x413  1043
        02013, // ?   (9-11-8-6)  0x40b  1035
        02007, // ?   (9-11-8-7)  0x407  1031
        00000, //     ()  0x0  0
        02202, // !   (11-8-2)  0x482  1154
        00006, // "   (8-7)  0x6  6
        00102, // #   (8-3)  0x42  66
        02102, // $   (11-8-3)  0x442  1090
        01042, // %   (0-8-4)  0x222  546
        04000, // &   (12)  0x800  2048
        00022, // '   (8-5)  0x12  18
        04022, // (   (12-8-5)  0x812  2066
        02022, // )   (11-8-5)  0x412  1042
        02042, // *   (11-8-4)  0x422  1058
        04012, // +   (12-8-6)  0x80a  2058
        01102, // ,   (0-8-3)  0x242  578
        02000, // -   (11)  0x400  1024
        04102, // .   (12-8-3)  0x842  2114
        01400, // /   (0-1)  0x300  768
        01000, // 0   (0)  0x200  512
        00400, // 1   (1)  0x100  256
        00200, // 2   (2)  0x80  128
        00100, // 3   (3)  0x40  64
        00040, // 4   (4)  0x20  32
        00020, // 5   (5)  0x10  16
        00010, // 6   (6)  0x8  8
        00004, // 7   (7)  0x4  4
        00002, // 8   (8)  0x2  2
        00001, // 9   (9)  0x1  1
        00202, // :   (8-2)  0x82  130
        02012, // ;   (11-8-6)  0x40a  1034
        04042, // <   (12-8-4)  0x822  2082
        00012, // =   (8-6)  0xa  10
        01012, // >   (0-8-6)  0x20a  522
        01006, // ?   (0-8-7)  0x206  518
        00042, // @   (8-4)  0x22  34
        04400, // A   (12-1)  0x900  2304
        04200, // B   (12-2)  0x880  2176
        04100, // C   (12-3)  0x840  2112
        04040, // D   (12-4)  0x820  2080
        04020, // E   (12-5)  0x810  2064
        04010, // F   (12-6)  0x808  2056
        04004, // G   (12-7)  0x804  2052
        04002, // H   (12-8)  0x802  2050
        04001, // I   (12-9)  0x801  2049
        02400, // J   (11-1)  0x500  1280
        02200, // K   (11-2)  0x480  1152
        02100, // L   (11-3)  0x440  1088
        02040, // M   (11-4)  0x420  1056
        02020, // N   (11-5)  0x410  1040
        02010, // O   (11-6)  0x408  1032
        02004, // P   (11-7)  0x404  1028
        02002, // Q   (11-8)  0x402  1026
        02001, // R   (11-9)  0x401  1025
        01200, // S   (0-2)  0x280  640
        01100, // T   (0-3)  0x240  576
        01040, // U   (0-4)  0x220  544
        01020, // V   (0-5)  0x210  528
        01010, // W   (0-6)  0x208  520
        01004, // X   (0-7)  0x204  516
        01002, // Y   (0-8)  0x202  514
        01001, // Z   (0-9)  0x201  513
        05022, // [   (12-0-8-5)  0xa12  2578
        04202, // \   (12-8-2)  0x882  2178
        06022, // ]   (12-11-8-5)  0xc12  3090
        02006, // ^   (11-8-7)  0x406  1030
        01022, // _   (0-8-5)  0x212  530
        00402, // `   (8-1)  0x102  258
        05400, // a   (12-0-1)  0xb00  2816
        05200, // b   (12-0-2)  0xa80  2688
        05100, // c   (12-0-3)  0xa40  2624
        05040, // d   (12-0-4)  0xa20  2592
        05020, // e   (12-0-5)  0xa10  2576
        05010, // f   (12-0-6)  0xa08  2568
        05004, // g   (12-0-7)  0xa04  2564
        05002, // h   (12-0-8)  0xa02  2562
        05001, // i   (12-0-9)  0xa01  2561
        06400, // j   (12-11-1)  0xd00  3328
        06200, // k   (12-11-2)  0xc80  3200
        06100, // l   (12-11-3)  0xc40  3136
        06040, // m   (12-11-4)  0xc20  3104
        06020, // n   (12-11-5)  0xc10  3088
        06010, // o   (12-11-6)  0xc08  3080
        06004, // p   (12-11-7)  0xc04  3076
        06002, // q   (12-11-8)  0xc02  3074
        06001, // r   (12-11-9)  0xc01  3073
        03200, // s   (11-0-2)  0x680  1664
        03100, // t   (11-0-3)  0x640  1600
        03040, // u   (11-0-4)  0x620  1568
        03020, // v   (11-0-5)  0x610  1552
        03010, // w   (11-0-6)  0x608  1544
        03004, // x   (11-0-7)  0x604  1540
        03002, // y   (11-0-8)  0x602  1538
        03001, // z   (11-0-9)  0x601  1537
        05000, // {   (12-0)  0xa00  2560
        04006, // |   (12-8-7)  0x806  2054
        03000, // }   (11-0)  0x600  1536
        03400, // ~   (11-0-1)  0x700  1792
        04005, //    (12-7-9)  0x805  2053
};

static word12 row_bit_masks[] = {0x800, 0x400, 0x200, 0x100, 0x080, 0x040, 0x020, 0x010, 0x008, 0x004, 0x002, 0x001};

static int mcc_to_ascii(word12 punch_code)
{
    for (uint i = 0; i < 128; i++)
    {
        if (mcc_punch_codes[i] == punch_code)
        {
            return i;
        }
    }

    return -1;
}

static void convert_mcc_to_ascii(word12 *buffer, char *ascii_string)
{
    for (uint i = 0; i < CARD_COL_COUNT; i++)
    {
        int c = mcc_to_ascii(buffer[i]);
        printf("+++ Punch Code %04o = '%c'\n", buffer[i], c);
        if (c == -1)
        {
            c = ' ';
        }
        ascii_string[i] = c;
    }
    ascii_string[CARD_COL_COUNT] = 0;
}

/*
 *                  Glyph Pattern Lookup
 * This is the parsing of the "lace" cards and extracting the ASCII characters
 * have been punched into the cards (as glphys) so the operator knows how to 
 * deliver the deck.
 */

#define NUM_GLYPH_CHAR_PATTERNS 45

static uint8 glyph_char_patterns[NUM_GLYPH_CHAR_PATTERNS][CHAR_MATRIX_BYTES] =
    {
        // Asterisk
        {037, 037, 037, 037, 037},
        // Space
        {000, 000, 000, 000, 000},
        // Period
        {000, 003, 003, 003, 000},
        // >
        {021, 000, 012, 000, 004},
        // A
        {037, 024, 024, 024, 037},
        // B
        {037, 025, 025, 025, 012},
        // C
        {037, 021, 021, 021, 021},
        // D
        {037, 021, 021, 021, 016},
        // E
        {037, 025, 025, 025, 021},
        // F
        {037, 024, 024, 024, 020},
        // G
        {037, 021, 021, 025, 027},
        // H
        {037, 004, 004, 004, 037},
        // I
        {021, 021, 037, 021, 021},
        // J
        {003, 001, 001, 001, 037},
        // K
        {037, 004, 004, 012, 021},
        // L
        {037, 001, 001, 001, 001},
        // M
        {037, 010, 004, 010, 037},
        // N
        {037, 010, 004, 002, 037},
        // O
        {037, 021, 021, 021, 037},
        // P
        {037, 024, 024, 024, 034},
        // Q
        {037, 021, 025, 023, 037},
        // R
        {037, 024, 024, 026, 035},
        // S
        {035, 025, 025, 025, 027},
        // T
        {020, 020, 037, 020, 020},
        // U
        {037, 001, 001, 001, 037},
        // V
        {030, 006, 001, 006, 030},
        // W
        {037, 002, 004, 002, 037},
        // X
        {021, 012, 004, 012, 021},
        // Y
        {020, 010, 007, 010, 020},
        // Z
        {021, 027, 025, 035, 021},
        // 0
        {016, 021, 021, 021, 016},
        // 1
        {000, 010, 000, 037, 000},
        // 2
        {023, 025, 025, 025, 035},
        // 3
        {021, 025, 025, 025, 037},
        // 4
        {034, 004, 004, 004, 037},
        // 5
        {035, 025, 025, 025, 022},
        // 6
        {037, 005, 005, 005, 007},
        // 7
        {020, 021, 022, 024, 030},
        // 8
        {012, 025, 025, 025, 012},
        // 9
        {034, 024, 024, 024, 037},
        // Underscore
        {001, 001, 001, 001, 001},
        // Hyphen
        {000, 004, 004, 004, 000},
        // (
        {000, 004, 012, 021, 000},
        // )
        {000, 021, 012, 004, 000},
        // /
        {001, 002, 004, 010, 020}};

static char glyph_chars[NUM_GLYPH_CHAR_PATTERNS] =
    {
        '*', ' ', '.', '>', 'A', 'B', 'C', 'D', 'E', 'F',
        'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        '_', '-', '(', ')', '/'};

static uint8 glyph_char_word_offset[11] =
    {
        24, 22, 19, 17, 15, 12, 10, 8, 5, 3, 1};

static uint8 glyph_nibble_offset[11] =
    {
        1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2};

static uint8 glyph_starting_column[11] =
    {
        73, 66, 59, 52, 45, 38, 31, 24, 17, 10, 3};

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

static void print_card(card_image_t *card)
{
    printf("Card:\n");
    for (int row = 0; row < 12; row++)
    {
        for (int col = 0; col < CARD_COL_COUNT; col++)
        {
            printf(card->column[col] & row_bit_masks[row] ? "*" : " ");
        }
        printf("\n");
    }
}

static void log_char_matrix_pattern(uint8 *char_matrix)
{
    printf("\nChar Matrix\n");
    for (uint col_offset = 0; col_offset < CHAR_MATRIX_BYTES; col_offset++)
    {
        printf(" %03o\n", char_matrix[col_offset]);
    }

    printf("\r\n");
    for (uint row = 0; row < 5; row++)
    {
        for (uint col = 0; col < CHAR_MATRIX_BYTES; col++)
        {
            if ((char_matrix[col] >> (4 - row)) & 0x1)
            {
                printf("*");
            }
            else
            {
                printf(" ");
            }
        }
        printf("\r\n");
    }
    printf("\r\n");
}

static char search_glyph_patterns(uint8 *matrix)
{
    for (int pattern = 0; pattern < NUM_GLYPH_CHAR_PATTERNS; pattern++)
    {
        if (memcmp(matrix, &glyph_char_patterns[pattern], CHAR_MATRIX_BYTES) == 0)
        {
            return glyph_chars[pattern];
        }
    }

    fprintf(stderr, "*** Warning: Punch found unknown block character pattern\n");
    log_char_matrix_pattern(matrix);

    return ' ';
}

static char get_lace_char(word12 *buffer, uint char_pos)
{
    if (char_pos >= GLYPHS_PER_CARD)
    {
        fprintf(stderr, "*** Error: Attempt to read punch block character out of range (%u)\n", char_pos);
        exit(4);
    }

    bool top = char_pos < 11;                                      // Top or bottom line of characters
    uint char_offset = (char_pos < 11) ? char_pos : char_pos - 11; // Character number in the line
    word12 col_buffer[5];                                          // The extracted 5 columns for the character

    // Extract the five 12-bit words from the main buffer that make up the character
    // Note that in this process we reverse the character image so it reads normally
    // (characters are punched in reverse)
    for (uint col_offset = 0; col_offset < 5; col_offset++)
    {
        col_buffer[4 - col_offset] = buffer[glyph_starting_column[char_offset]+col_offset];
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

static void scan_card_for_glyphs(card_image_t *card)
{
    for (uint c_pos = 0; c_pos < 22; c_pos++)
    {
        char c = get_lace_char(card->column, c_pos);
        uint8 current_length = strlen(glyph_buffer);
        if (current_length < (sizeof(glyph_buffer) - 1))
        {
            glyph_buffer[current_length++] = c;
            glyph_buffer[current_length] = 0;
        }
    }
}

static card_image_t *allocate_card()
{
    card_image_t *card = malloc(sizeof(card_image_t));
    if (card == NULL)
    {
        fprintf(stderr, "*** Error: Failed to allocate card image!\n");
        exit(1);
    }
    return card;
}

static card_image_t *read_card(FILE *in_file)
{
    uint8 byte_buffer[BYTES_PER_CARD];

    int bytes_read = fread(&byte_buffer, 1, BYTES_PER_CARD, in_file);

    if (bytes_read == 0)
    {
        if (feof(in_file))
        {
            // We've hit the end of file so just bail out
            return NULL;
        }
        else if (ferror(in_file))
        {
            // Something bad happened, report the error and exit
            perror("Reading card file\n");
            exit(2);
        }
        else
        {
            fprintf(stderr, "*** Error: fread returned zero but failed to set the error or eof flags!\n");
            exit(2);
        }
    }

    // Make sure we read a full card
    if (bytes_read != BYTES_PER_CARD)
    {
        fprintf(stderr, "*** Error: failed to read a full card (only read %d of %d bytes)\n", bytes_read, BYTES_PER_CARD);
        exit(3);
    }

    // We have a full card so allocate a card and convert the byte buffer into the card image
    card_image_t *card = allocate_card();

    for (int current_nibble = 0; current_nibble < NIBBLES_PER_CARD; current_nibble++)
    {
        int byte_offset = current_nibble / 2;
        int nibble_offset = (2 - (current_nibble % 3)) * 4;
        int col = current_nibble / 3;

        word12 nibble = (current_nibble & 0x1) ? byte_buffer[byte_offset] : (byte_buffer[byte_offset] >> 4);
        nibble &= 0x00000F;
        card->column[col] |= (nibble << nibble_offset);
    }

    print_card(card);

    return card;
}

static void print_cards(FILE *in_file)
{
    card_image_t *card;
    while (card = read_card(in_file))
    {
        print_card(card);
        printf("\n");
    }
}

static void save_card_in_cache(CARD_CACHE *card_cache, card_image_t *card)
{
    CARD_CACHE_ENTRY *new_entry = malloc(sizeof(CARD_CACHE_ENTRY));

    new_entry->card = card;
    new_entry->next_entry = NULL;

    if (card_cache->first_cache_card == NULL)
    {
        card_cache->first_cache_card = new_entry;
        card_cache->last_cache_card = new_entry;
    }
    else
    {
        card_cache->last_cache_card->next_entry = new_entry;
        card_cache->last_cache_card = new_entry;
    }
}

static void print_event(enum parse_event event)
{
    switch (event)
    {
    case NoEvent:
        printf("[No Event]");
        break;
    case BannerCard:
        printf("[Banner Card]");
        break;
    case EndOfDeckCard:
        printf("[End Of Deck Card]");
        break;
    case Card:
        printf("[Card]");
        break;
    case Done:
        printf("[Done]");
        break;
    default:
        printf("[unknown event %d]", event);
        break;
    }
}

static void print_state(enum parse_state state)
{
    switch (state)
    {
    case Idle:
        printf("[Idle]");
        break;
    case StartingJob:
        printf("[Starting Job]");
        break;
    case PunchGlyphLookup:
        printf("[Punch Glyph Lookup]");
        break;
    case EndOfHeader:
        printf("[End Of Header]");
        break;
    case CacheCard:
        printf("[Cache Card]");
        break;
    case EndOfDeck:
        printf("[End Of Deck]");
        break;
    case EndOfJob:
        printf("[End Of Job]");
        break;
    default:
        printf("[unknown state %d]", state);
        break;
    }
}

static void print_transition(enum parse_state old_state, enum parse_event event, enum parse_state new_state)
{
    printf(">>> Punch Transition: ");
    print_event(event);
    printf(" = ");
    print_state(old_state);
    printf(" -> ");
    print_state(new_state);
    printf("\r\n");
}

static enum parse_event do_state_idle(enum parse_event event)
{
    print_transition(current_state, event, Idle);
    current_state = Idle;

    return NoEvent;
}

static enum parse_event do_state_starting_job(enum parse_event event, card_image_t *card)
{
    print_transition(current_state, event, StartingJob);
    current_state = StartingJob;

    glyph_buffer[0] = 0;                          // Clear Glyph Buffer
    save_card_in_cache(&banner_card_cache, card); // Save card in cache

    return NoEvent;
}

static enum parse_event do_state_scan_card_for_glyphs(enum parse_event event, card_image_t *card)
{
    print_transition(current_state, event, PunchGlyphLookup);
    current_state = PunchGlyphLookup;

    scan_card_for_glyphs(card);

    save_card_in_cache(&banner_card_cache, card); // Save card in cache

    return NoEvent;
}

static enum parse_event do_state_end_of_header(enum parse_event event, card_image_t *card)
{
    print_transition(current_state, event, EndOfHeader);
    current_state = EndOfHeader;

    save_card_in_cache(&banner_card_cache, card); // Save card in cache

    printf("\n++++ Glyph Buffer ++++\n'%s'\n", glyph_buffer);
#if 0
    char punch_file_name[PATH_MAX + 1];
    if (strlen(glyph_buffer) < 86)
    {
        sim_warn("*** Punch: glyph buffer too short, unable to parse file name '%s'\n", glyph_buffer);
        punch_file_name[0] = 0;
    }
    else
    {
        sprintf(punch_file_name, "%7.7s%7.7s.%5.5s.%2.2s-%2.2s-%2.2s.%6.6s.%2.2s",
                &state->glyph_buffer[68],
                &state->glyph_buffer[79],
                &state->glyph_buffer[14],
                &state->glyph_buffer[46],
                &state->glyph_buffer[49],
                &state->glyph_buffer[52],
                &state->glyph_buffer[57],
                &state->glyph_buffer[35]);
        remove_spaces(punch_file_name);
    }

    strncpy(state->raw_file_name, punch_file_name, sizeof(state->raw_file_name));

    create_punch_files(state); // Create spool file

    // Write cached cards to spool file
    CARD_CACHE_ENTRY *current_entry = state->first_cached_card;
    while (current_entry != NULL)
    {
        write_punch_files(state, current_entry->card, WORDS_PER_CARD, true);
        current_entry = current_entry->next_entry;
    }

    //dump_card_cache(state);

    clear_card_cache(state); // Clear card cache
#endif

    return NoEvent;
}

static enum parse_event do_state_cache_card(enum parse_event event, card_image_t *card)
{
    print_transition(current_state, event, CacheCard);
    current_state = CacheCard;

    save_card_in_cache(&data_card_cache, card); // Save card in cache

    return NoEvent;
}

static enum parse_event do_state_end_of_deck(enum parse_event event, card_image_t *card)
{
    print_transition(current_state, event, EndOfDeck);
    current_state = EndOfDeck;

    save_card_in_cache(&trailer_card_cache, card); // Save card in cache

    return NoEvent;
}

static enum parse_event do_state_end_of_job(enum parse_event event, card_image_t *card)
{
    print_transition(current_state, event, EndOfJob);
    current_state = EndOfJob;

#if 0
    // Write cached cards to spool file
    CARD_CACHE_ENTRY *current_entry = first_cached_card;
    while (current_entry != NULL)
    {
        write_punch_files(state, current_entry->card, WORDS_PER_CARD, (current_entry->next_entry == NULL));
        current_entry = current_entry->next_entry;
    }

    //dump_card_cache(state);

    clear_card_cache(state); // Clear card cache

    write_punch_files(state, card, tally, true); // Write card to spool file

    // Close punch files
    if (state->punfile_raw >= 0)
    {
        close(state->punfile_raw);
        state->punfile_raw = -1;
    }

    if (state->punfile_punch >= 0)
    {
        close(state->punfile_punch);
        state->punfile_punch = -1;
    }

    if (state->punfile_mcc >= 0)
    {
        close(state->punfile_mcc);
        state->punfile_mcc = -1;
    }
#endif
    return Done;
}

static void unexpected_event(enum parse_event event)
{
    printf("*** Unexpected event ");
    print_event(event);

    printf(" in state ");
    print_state(current_state);

    printf("***\n");
}

static void parse_card(card_image_t *card)
{
    enum parse_event event = Card;

    if (memcmp(card, end_of_deck_card, sizeof(end_of_deck_card)) == 0)
    {
        fprintf(stderr, "*** Found End Of Deck Card ***\n");
        event = EndOfDeckCard;
    }

    if (memcmp(card, banner_card, sizeof(banner_card)) == 0)
    {
        fprintf(stderr, "*** Found Banner Card ***\n");
        event = BannerCard;
    }

    while (event != NoEvent)
    {
        enum parse_event current_event = event;
        event = NoEvent;

        switch (current_event)
        {
        case BannerCard:
            switch (current_state)
            {
            case Idle:
                event = do_state_starting_job(current_event, card);
                break;

            case PunchGlyphLookup:
                event = do_state_end_of_header(current_event, card);
                break;

            case EndOfDeck:
                event = do_state_end_of_job(current_event, card);
                break;

            default:
                unexpected_event(current_event);
                break;
            }
            break;

        case EndOfDeckCard:
            switch (current_state)
            {
            case StartingJob:
                event = do_state_end_of_deck(current_event, card);
                break;

            case PunchGlyphLookup:
                event = do_state_end_of_deck(current_event, card);
                break;

            case EndOfHeader:
                event = do_state_end_of_deck(current_event, card);
                break;

            case CacheCard:
                event = do_state_end_of_deck(current_event, card);
                break;

            case EndOfDeck:
                event = do_state_end_of_deck(current_event, card);
                break;

            default:
                unexpected_event(current_event);
                break;
            }
            break;

        case Card:
            switch (current_state)
            {
            case StartingJob:
                event = do_state_scan_card_for_glyphs(current_event, card);
                break;

            case PunchGlyphLookup:
                event = do_state_scan_card_for_glyphs(current_event, card);
                break;

            case EndOfHeader:
                event = do_state_cache_card(current_event, card);
                break;

            case CacheCard:
                event = do_state_cache_card(current_event, card);
                break;

            case EndOfDeck:
                event = do_state_cache_card(current_event, card);
                break;

            default:
                unexpected_event(current_event);
                break;
            }
            break;

        case Done:
            switch (current_state)
            {
            case EndOfJob:
                event = do_state_idle(current_event);
                break;

            default:
                unexpected_event(current_event);
                break;
            }
            break;

        default:
            fprintf(stderr, "*** Error: Punch received unknown event!\n");
            break;
        }
    }
}

static void parse_cards(FILE *in_file)
{
    card_image_t *card;
    while (card = read_card(in_file))
    {
        parse_card(card);
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    printf("****\nPunch File Utility\n****\n");

    FILE *in_file = fopen("/home/deana/multics/kit-test-12.7/run/punches/puna/SYSADMIN.50001.07-31-21.0645.4.1.raw", "r");
    parse_cards(in_file);
}
