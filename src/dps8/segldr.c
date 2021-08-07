/*
 * Copyright (c) 2021 Charles Anthony
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 */

#include "dps8.h"
#include "segldr.h"

/* 
 * Segment loader memory layout
 * 
 * 2000   Bootstrap code
 * 
 * 3000   Page tables
 * 
 * Unpaged segemnts
 * 
 *     3000  seg. 0 SDW
 *     3002  seg. 1 SDW
 *     3003  seg. 2 SDW
 *     ...
 *     3600  seg. 300 SDW
 *     ...
 *     3776  seg. 377 SDW
 * 
 * 4000  Segment storage
 * 
 */ 

#define ADDR_BOOT 02000
#define ADDR_PG_TABLE 03000
#define ADDR_SEGS 04000

/*
 *  sl init       initialize segment loader
 *  sl bload <segment number> <filename> 
 *                copy the binary file into the next avialable storage
 *                and create an SDW for the file
 */

t_stat segment_loader (int32 arg, const char * buf)
  {
    sim_printf ("segld <%s>\n", buf);
    size_t bufl = strlen (buf) + 1;
    char p1 [bufl], p2 [bufl], p3 [bufl];
    int nParams = sscanf (buf, "%s %s %s", p1, p2, p3);
    if (nParams <= 0)
      goto err;
    if (strncasecmp ("init", p1, strlen(p1)) == 0)
      {
        if (nParams != 1)
          goto err;
        sim_printf ("init\n");
      }
    else if (strncasecmp ("bload", p1, strlen(p1)) == 0)
      {
        if (nParams != 3)
          goto err;
        sim_printf ("bload\n");
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
