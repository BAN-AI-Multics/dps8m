/*
 * Copyright (c) 2013-2019 Charles Anthony
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include "tracker.h"

int brkbrk (int32_t arg, const char *  buf);
void hdbgPrint (void);

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
        fprintf (stderr, "\r\n[%lld]\r\n", cycleCnt);
        fprintf (stderr, "expected: %05o:%06o %012lo\r\n", psegno, pic, popcode);
        fprintf (stderr, "got:      %05o:%06o %012lo\r\n", segno, ic, opcode);
#ifdef HDBG
        hdbgPrint ();
#endif
        brkbrk (0, "");
        exit (1);
      }
  }

void trk_init (bool write)
  {
    if (write)
      {
        fd = open ("track.dat", O_WRONLY | O_CREAT | O_TRUNC, 0660);
      }
    else
      {
        fd = open ("track.dat", O_RDONLY, 0660);
      }
    if (fd == -1)
      {
        perror ("trk_init");
        exit (1);
      }
    writing = write;
    tracking = true;
  }



