/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 0d484872-f630-11ec-b82e-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2013-2019 Charles Anthony
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include "dps8.h"
#include "tracker.h"

int brkbrk (int32_t arg, const char *  buf);
#ifdef TESTING
void hdbgPrint (void);
#endif /* ifdef TESTING */

static int fd;
static bool writing;
static bool tracking = false;

void trk (unsigned long long cycleCnt, uint16_t segno, uint32_t ic, uint64_t opcode)
  {
    if (! tracking)
      return;
    if (writing)
      {
        write (fd, & segno, sizeof (segno));
        write (fd, & ic, sizeof (ic));
        write (fd, & opcode, sizeof (opcode));
        return;
      }
    uint16_t psegno;
    uint32_t pic;
    uint64_t popcode;
    read (fd, & psegno, sizeof (segno));
    read (fd, & pic, sizeof (ic));
    read (fd, & popcode, sizeof (opcode));
    if (segno != psegno ||
        ic != pic ||
        opcode != popcode)
      {
        (void)fprintf (stderr, "\r\n[%llu]\r\n",
                       (unsigned long long int)cycleCnt);
        (void)fprintf (stderr, "expected: %05o:%06o %012llo\r\n", psegno, pic,
                       (unsigned long long int)popcode);
        (void)fprintf (stderr, "got:      %05o:%06o %012llo\r\n", segno, ic,
                       (unsigned long long int)opcode);
#ifdef TESTING
        hdbgPrint ();
#endif /* ifdef TESTING */
        brkbrk (0, "");
        exit (1);
      }
  }

void trk_init (bool write)
  {
    if (write)
      {
        fd = open ("track.dat", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
      }
    else
      {
        fd = open ("track.dat", O_RDONLY, 0);
      }
    if (fd == -1)
      {
        perror ("trk_init");
        exit (1);
      }
    writing = write;
    tracking = true;
  }
