/*
 Copyright 2013-2016 by Charles Anthony

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

// Pack an .oct file into a tape record
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "bit36.h"

#ifdef __MINGW64__
#define open(x,y,args...) open(x, y|O_BINARY,##args)
#define creat(x,y) open(x, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, y)
#endif

int main (int argc, char * argv [])
  {
       FILE *fopen(const char *path, const char *mode);

#ifndef __MINGW64__
    FILE * fin = fopen (argv [1], "r");
#else
    FILE * fin = fopen (argv [1], "rb");
#endif
    if (! fin)
      {
        printf ("can't open input file\n");
        exit (1);
      }

    int fout = open (argv [2], O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (fout < 0)
      {
        printf ("can't open output file\n");
        exit (1);
      }

    char buf [256];
    fgets (buf, 256, fin);
    if (strncmp (buf, "!SIZE", 5))
      {
        printf ("can't find !SIZE\n");
        exit (1);
      }
    // !SIZE is right; it is the last loaded + 1; it doesn't take ORG into accout
    long size = strtol (buf + 6, NULL, 8);
    uint8_t * outbuf = malloc ((size * 9 + 1) / 2);

    int wc = 0;
    while (! feof (fin))
      {
        char * rv = fgets (buf, 256, fin);
        if (! rv)
          continue;
        if (buf [0] < '0' || buf [0] > '7')
          continue;
        word36 w36 = strtoll (buf + 12, NULL, 8);
        put36 (w36, outbuf, wc);
        wc ++;
      }
    write (fout, outbuf, (wc * 9 + 1) / 2);
    return 0;
  }
