/*
 Copyright 2013-2016 by Charles Anthony

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

// The MIT Multics archive contain files that end in ".9"; these files have
// undergone a 9 bit to 16 bit conversion. This program undoes that, restoring
// them to a packed 72 bit in 9 bytes format.


#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "bit36.h"

#ifdef __MINGW64__
#define open(x,y,args...) open(x, y|O_BINARY,##args)
#define creat(x,y) open(x, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, y)
#undef isprint
#define isprint(c) (c>=0x20 && c<=0x7f)
#endif


int main (int argc, char * argv [])
  {
    if (argc != 3)
      {
        printf ("extract <in> <out>\n");
        exit (1);
      }

    int fd = open (argv [1], O_RDONLY);
    if (fd < 0)
      {
        printf ("can't open in\n");
        exit (1);
      }

    int fdout = open (argv [2], O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (fdout < 0)
      {
        printf ("can't open out\n");
        exit (1);
      }

    for (;;)
      {
        uint8_t sixteen_bytes [16];
        memset (sixteen_bytes, 0, sizeof (sixteen_bytes));

        ssize_t sz = read (fd, sixteen_bytes, sizeof (sixteen_bytes));
        if (sz == 0)
          break;
        if (sz < 0)
          {
            printf ("error reading in\n");
            exit (1);
          }
#if 0
        if (sz != sizeof (sixteen_bytes))
          {
            printf ("short read; zero padded\n");
          }
#endif

        uint16_t eight_shorts [8];
        for (int i = 0; i < 8; i ++)
          eight_shorts [i] = (((uint16_t) sixteen_bytes [i * 2]) << 8) + (uint16_t) sixteen_bytes [i * 2 + 1];

        // Pack
        uint8_t packed [9]; // 72 bits

        // for (int i = 0; i < 8; i ++)
          // {
            // for (int j = 8; j >= 0; j --)
              // printf ("%d", (eight_shorts [i] >> j) & 1);
            // //printf (" ");
          // }
        // printf ("\n");

        packed [0] =                                    ((eight_shorts [0] >> 1) & 0xFF);
        packed [1] = ((eight_shorts [0] << 7) & 0x80) | ((eight_shorts [1] >> 2) & 0x7F);
        packed [2] = ((eight_shorts [1] << 6) & 0xC0) | ((eight_shorts [2] >> 3) & 0x3F);
        packed [3] = ((eight_shorts [2] << 5) & 0xE0) | ((eight_shorts [3] >> 4) & 0x1F);
        packed [4] = ((eight_shorts [3] << 4) & 0xF0) | ((eight_shorts [4] >> 5) & 0x0F);
        packed [5] = ((eight_shorts [4] << 3) & 0xF8) | ((eight_shorts [5] >> 6) & 0x07);
        packed [6] = ((eight_shorts [5] << 2) & 0xFC) | ((eight_shorts [6] >> 7) & 0x03);
        packed [7] = ((eight_shorts [6] << 1) & 0xFE) | ((eight_shorts [7] >> 8) & 0x01);
        packed [8] = ((eight_shorts [7] << 0) & 0xFF)                                   ;

        // for (int i = 0; i < 9; i ++)
          // for (int j = 7; j >= 0; j --)
            // printf ("%d", (packed [i] >> j) & 1);
        // printf ("\n");
        // printf ("\n");

        sz = write (fdout, packed, sizeof (packed));
        if (sz != sizeof (packed))
          {
            printf ("error writing out\n");
            exit (1);
          }
      }
    close (fdout);
    return 0;
  }
