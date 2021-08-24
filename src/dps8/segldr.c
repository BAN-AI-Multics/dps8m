/*
 * Copyright (c) 2021 Charles Anthony
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dps8.h"
#include "dps8_sys.h"
#include "dps8_cpu.h"
#include "dps8_scu.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_state.h"
#include "dps8_utils.h"

#include "segldr.h"
/* 
 * Segment loader memory layout
 * 
 * 0000   Intterrupt and fault vectors
 *
 * 2000   Bootstrap code entry
 * 
 * 4000   Page tables
 * 
 * Paged segments
 *
 *     4000 descriptor segment page table
 *        4000 PTW for segments 0-511. 0-777
 *     6000 descriptor segment page for segments 0-777
 *     7000 segment page tables
 *    10000  Segment storage
 * 
 */ 


#define ADDR_BOOT   02000
#define ADDR_DSPT   04000
#define ADDR_DSP    06000
#define ADDR_PGS    07000
#define ADDR_SEGS 0100000

#define MAX_SEG_NO 0377

/*
 *  sl init       initialize segment loader
 *  sl bload <segment number> <filename> 
 *                copy the binary file into the next avialable storage
 *                and create an SDW for the file
 *  sl bload boot <filename> 
 *                copy the binary file into memory starting at address 0
 *  al stack <segno> <n_pages>
 */

static word24 nextSegAddr = ADDR_SEGS;
static word24 nextPageAddr = ADDR_PGS;

static void initPageTables (void)
  {
        for (uint addr = ADDR_DSPT; addr < ADDR_SEGS; addr ++)
          M[addr] = 0;
        // Place the PTW for the first 512 segments in the DSPT
        uint x1 = 0;
        word36 * ptwp = (word36 *) M + x1 + ADDR_DSPT;
        putbits36_18 (ptwp,  0, ADDR_DSP >> 6);  // points to the Descriptor Segment Page
        putbits36_1  (ptwp, 26,        0);  // unused
        putbits36_1  (ptwp, 29,        0);  // unmodified
        putbits36_1  (ptwp, 29,        0);  // unmodified
        putbits36_1  (ptwp, 33,        1);  // page is in memory
        putbits36_2  (ptwp, 34,        0);  // fault code
  }

static void addSDW (word24 addr, long segnum, long length)
  {
    // Round length up to page boundary
    //long lengthp = (length + 01777) & 077776000;

    // Number of pages
    long npages = length / 1024;

    // Add SDW, allocate space
    word14 bound = ((length + 15) >> 4) + 1;

    // Add PTW to DSPT
    word24 y1 = (2u * segnum) % 1024u;

    // Allocate target segment page table
    word24 pgTblAddr = nextPageAddr;
    nextPageAddr += 1024;

    // Build Descriptor Segment Page SDW
    // word36 * sdwp = (word36 *) M + ADDR_DSP + y1;
    word24 sdw0 = ADDR_DSP + y1 + 0;
    word24 sdw1 = ADDR_DSP + y1 + 1;
    //sim_printf ("segnum %lo length %lu bound %u sdw0 %o sdw1 %o ADDR %06o\n", (unsigned long) segnum, length, bound, sdw0, sdw1, pgTblAddr);
    putbits36_24 (& M[sdw0],  0, pgTblAddr); // ADDR
// I can't get segldr_boot to cross to ring 4
//    putbits36_3  (& M[sdw0], 24, 4);         // R1
//    putbits36_3  (& M[sdw0], 27, 4);         // R2
//    putbits36_3  (& M[sdw0], 30, 4);         // R3
    putbits36_3  (& M[sdw0], 24, 0);         // R1
    putbits36_3  (& M[sdw0], 27, 0);         // R2
    putbits36_3  (& M[sdw0], 30, 0);         // R3
    putbits36_1  (& M[sdw0], 33, 1);         // F
    putbits36_2  (& M[sdw0], 34, 0);         // FC
    putbits36_1  (& M[sdw1], 0, 0);      // 0
    putbits36_14 (& M[sdw1],  1, bound); // BOUND
    putbits36_1  (& M[sdw1], 15, 1);     // R
    putbits36_1  (& M[sdw1], 16, 1);     // E
    putbits36_1  (& M[sdw1], 17, 1);     // W
    putbits36_1  (& M[sdw1], 18, 0);     // P
    putbits36_1  (& M[sdw1], 19, 0);     // U
    putbits36_1  (& M[sdw1], 20, 1);     // G
    putbits36_1  (& M[sdw1], 21, 1);     // C
    putbits36_14 (& M[sdw1], 21, 0);     // EB

    // Fill out PTWs on Segment page table

    for (word24 pg = 0; pg <= npages; pg ++)
      {
        //word36 * ptwp = (word36 *) M + pgTblAddr + pg;
        word24 ptw = pgTblAddr + pg;
        word18 pgAddr = (addr + pg * 1024) >> 6;
        putbits36_18 (& M[ptw],  0,    pgAddr); // points to the Segment Page
        putbits36_1  (& M[ptw], 26,         0);  // unused
        putbits36_1  (& M[ptw], 29,         0);  // unmodified
        putbits36_1  (& M[ptw], 29,         0);  // unmodified
        putbits36_1  (& M[ptw], 33,         1);  // page is in memory
        putbits36_2  (& M[ptw], 34,         0);  // fault code
        //sim_printf ("   ptw pg %u at %o addr %o\n", pg, pgTblAddr + pg, pgAddr);
      }
  }
#if 0
static void addSDW (word24 addr, long segnum, long length)
  {
    // Add SDW, allocate space
    word14 bound = (length + 15) >> 10;
bound=037777;

    // Build SDW
    word36 * sdwp = (word36 *) M + segnum * 2 + ADDR_PG_TABLE;
    putbits36_24 (sdwp,  0, addr);                    // ADDR
// I can't get segldr_boot to cross to ring 4
//    putbits36_3  (sdwp, 24, 4);                       // R1
//    putbits36_3  (sdwp, 27, 4);                       // R2
//    putbits36_3  (sdwp, 30, 4);                       // R3
    putbits36_3  (sdwp, 24, 0);                       // R1
    putbits36_3  (sdwp, 27, 0);                       // R2
    putbits36_3  (sdwp, 30, 0);                       // R3
    putbits36_1  (sdwp, 33, 1);                       // F
    putbits36_2  (sdwp, 34, 0);                       // FC
    putbits36_1  (sdwp + 1, 0, 0);                    // 0
    putbits36_14 (sdwp + 1,  1, bound);               // BOUND
    putbits36_1  (sdwp + 1, 15, 1);                   // R
    putbits36_1  (sdwp + 1, 16, 1);                   // E
    putbits36_1  (sdwp + 1, 17, 1);                   // W
    putbits36_1  (sdwp + 1, 18, 0);                   // P
    putbits36_1  (sdwp + 1, 19, 1);                   // U
    putbits36_1  (sdwp + 1, 20, 1);                   // G
    putbits36_1  (sdwp + 1, 21, 1);                   // C
    putbits36_14 (sdwp + 1, 21, 0);                   // EB
  }
#endif

static t_stat stack (char * p2, char * p3)
  {

    // Segment number
    char * endptr;
    long segnum = strtol (p2, & endptr, 8);
    if (* endptr)
      return SCPE_ARG;
    if (segnum < 0 || segnum > MAX_SEG_NO)
      {
        sim_printf ("Segment number is limited to 0 to 0377\n");
        return SCPE_ARG;
      }

    long len = strtol (p3, & endptr, 8);
    if (* endptr)
      return SCPE_ARG;
    if (len < 1 || len > 255)
      {
        sim_printf ("Segment length is limited to 1 to 0377\n");
        return SCPE_ARG;
      }

    long length = len * 1024;

    // Add SDW
    addSDW (nextSegAddr, segnum, length);

    sim_printf ("Placed stack (%lo) at %o length %lo allocated %lo\n", (unsigned long) segnum, nextSegAddr, (unsigned long) len, length);
    // Mark the pages as used
    nextSegAddr += length;

    return SCPE_OK;
  }

static t_stat bload (char * p2, char * p3)
  {

    // Segment number
    long segnum;
    if (strcasecmp ("boot", p2) == 0)
      {
        segnum = -1; // Indicate boot load
      }
    else
      {
        char * endptr;
        segnum = strtol (p2, & endptr, 8);
        if (* endptr)
          return SCPE_ARG;
        if (segnum < 0 || segnum > MAX_SEG_NO)
          {
            sim_printf ("Segment number is limited to 0 to 0377\n");
            return SCPE_ARG;
          }
      }

    // Segment image file
    int deckfd = open (p3, O_RDONLY);
    if (deckfd < 0)
      {
        sim_printf ("Unable to open '%s'\n", p3);
        return SCPE_ARG;
      }


    // Copy segment into memory
    word24 addr = nextSegAddr;
    word24 startAddr;
    if (segnum < 0)
      addr = 0;
    else
      addr = nextSegAddr;

    startAddr = addr;

    for (;;)
      {
        ssize_t sz;
// 72 bits at a time; 2 dps8m words == 9 bytes
        uint8_t bytes [9];
        memset (bytes, 0, 9);
        sz = read (deckfd, bytes, 9);
        if (sz == 0)
          break;
        //if (sz != n) short read?
        word36 even = extr36 (bytes, 0);
        word36 odd = extr36 (bytes, 1);
        //sim_printf ("%08o  %012"PRIo64"   %012"PRIo64"\n", addr, even, odd);
        M[addr ++] = even;
        M[addr ++] = odd;
      }
    word24 length = addr - startAddr;

    int lengthp;
    if (segnum >= 0)
      {
        // Add SDW
        addSDW (startAddr, segnum, length);

        // Round lentgth up to page (1024) boundary
        lengthp = (length + 01777) & 077776000;

        // Mark the pages as used
        nextSegAddr += lengthp;
      }
    else
      {
        lengthp = 04000;
        addSDW (0, 0, lengthp);
      }

    sim_printf ("Loaded %s (%lo) at %o length %o allocated %o\n", p3, segnum < 0 ? 0 : (unsigned long) segnum, startAddr, length, lengthp);
    close (deckfd);
    return SCPE_OK;
  }

t_stat segment_loader (int32 arg, const char * buf)
  {
    size_t bufl = strlen (buf) + 1;
    char p1 [bufl], p2 [bufl], p3 [bufl];
    int nParams = sscanf (buf, "%s %s %s", p1, p2, p3);
    if (nParams <= 0)
      goto err;
    if (strncasecmp ("init", p1, strlen(p1)) == 0)
      {
        if (nParams != 1)
          goto err;
        nextSegAddr = ADDR_SEGS;
        initPageTables ();
      }
    else if (strcasecmp ("bload", p1) == 0)
      {
        if (nParams != 3)
          goto err;
        return bload (p2, p3);
      }
    else if (strcasecmp ("stack", p1) == 0)
      {
        if (nParams != 3)
          goto err;
        return stack (p2, p3);
      }
    else
      goto err;
    return SCPE_OK;
  err:
    sim_msg ("Usage:\n"
             "   sl init    initialize\n"
             "   sl bload <segno> <filename>\n");
    return SCPE_ARG;
  }
