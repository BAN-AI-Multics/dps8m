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

#define CARD_COL_COUNT 80
#define NIBBLES_PER_COL 3
#define NIBBLES_PER_CARD (CARD_COL_COUNT * NIBBLES_PER_COL)
#define BYTES_PER_CARD (NIBBLES_PER_CARD / 2)

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef uint16 word12;
typedef struct
{
    word12 column[CARD_COL_COUNT];
} card_image_t;

static word12 row_bit_masks[] = { 0x800, 0x400, 0x200, 0x100, 0x080, 0x040, 0x020, 0x010, 0x008, 0x004, 0x002, 0x001};

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
        int nibble_offset = (2-(current_nibble % 3)) * 4;
        int col = current_nibble / 3;

        word12 nibble = (current_nibble & 0x1) ? byte_buffer[byte_offset] : (byte_buffer[byte_offset] >> 4)  ;
        nibble &= 0x00000F;
        card->column[col] |= (nibble << nibble_offset);
    }
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

static void print_cards(FILE *in_file)
{
    card_image_t *card; 
    while (card = read_card(in_file))
    {
        print_card(card);
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    printf("****\nPunch File Utility\n****\n");

    FILE* in_file = fopen("/home/deana/multics/kit-test-12.7/run/punches/puna/SYSADMIN.50001.07-31-21.0645.4.1.raw", "r");
    print_cards(in_file);

}
