/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 70f11728-171b-11ee-ac22-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2022 Charles Anthony
 * Copyright (c) 2022-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#include <string.h>

#include "dps8.h"
#include "dps8_cpu.h"

void ucInvalidate (void) {
  (void)memset (cpu.uCache.caches, 0, sizeof (cpu.uCache.caches));
}

void ucCacheSave (uint ucNum, word15 segno, word18 offset, word14 bound, word1 p, word24 address, word3 r1, bool paged) {
  if (segno >= UC_CACHE_SZ) {
    return;
  }
  ucache_t * ep;
  ep          = & cpu.uCache.caches[ucNum][segno];
  ep->valid   = true;
  ep->segno   = segno;
  ep->offset  = offset;
  ep->bound   = bound;
  ep->address = address;
  ep->r1      = r1;
  ep->p       = p;
  ep->paged   = paged;
#if defined(HDBG)
  hdbgNote ("ucache", "save %u %05o:%06o %05o %o %08o %o %o", ucNum, segno, offset, bound, p, address, r1, paged);
#endif
}

bool ucCacheCheck (uint ucNum, word15 segno, word18 offset, word14 * bound, word1 * p, word24 * address, word3 * r1, bool * paged) {
  if (segno >= UC_CACHE_SZ) {
#if defined(UCACHE_STATS)
    cpu.uCache.segnoSkips ++;
#endif
    return false;
  }
  ucache_t * ep;
  ep = & cpu.uCache.caches[ucNum][segno];
  // Is cache entry valid?
  if (! ep->valid) {
#if defined(HDBG)
    hdbgNote ("ucache", "check not valid");
#endif
    goto miss;
  }
#if 0
  // Same segment?
  if (ep->segno != segno) {
# if defined(HDBG)
    hdbgNote ("ucache", "segno %o != %o\r\n", ep->segno, segno);
# endif
    goto miss;
  }
#endif
  // Same page?
  if (ep->paged && ((ep->offset & PG18MASK) != (offset & PG18MASK))) {
#if defined(HDBG)
    hdbgNote ("ucache", "pgno %o != %o\r\n", (ep->offset & PG18MASK), (offset & PG18MASK));
#endif
    goto miss;
  }
  // In bounds?
  if (((offset >> 4) & 037777) > ep->bound) {
    //sim_printf ("bound %o != %o\r\n", ((offset >> 4) & 037777), ep->bound);
#if defined(HDBG)
    hdbgNote ("ucache", "bound %o != %o\r\n", ((offset >> 4) & 037777), ep->bound);
#endif
    goto miss;
  }
#if defined(HDBG)
  hdbgNote ("ucache", "hit %u %05o:%06o %05o %o %08o %o %o", ucNum, segno, offset, ep->bound, ep->p, ep->address, ep->r1, ep->paged);
#endif
  * bound = ep->bound;
#if 0
  if (ep->paged) {
    word18 pgoffset = offset & OS18MASK;
    * address = (ep->address & PG24MASK) + pgoffset;
# if defined(HDBG)
    hdbgNote ("ucache", "  FAP pgoffset %06o address %08o", pgoffset, * address);
# endif
  } else {
    * address = (ep->address & 077777760) + offset;
# if defined(HDBG)
    hdbgNote ("ucache", "  FANP pgoffset %06o address %08o", pgoffset, * address);
# endif
  }
#else
  * address = ep->address;
#endif
  * r1      = ep->r1;
  * p       = ep->p;
  * paged   = ep->paged;
#if defined(UCACHE_STATS)
  cpu.uCache.hits[ucNum] ++;
#endif
  return true;
miss:;
#if defined(UCACHE_STATS)
  cpu.uCache.misses[ucNum] ++;
#endif
  return false;
}

#if defined(UCACHE_STATS)
void ucacheStats (int cpuNo) {

  (void)fflush(stdout);
  (void)fflush(stderr);
  sim_msg ("\r\n|   CPU %c Micro-cache Statistics  |", 'A' + cpuNo);
  sim_msg ("\r\n+---------------------------------+\r\n");
# define pct(a, b) ((b) ? (a) * 100.0 / ((a) + (b)) : 0)
# define args(a, b, c) a, b, c, pct (a, (b + c))
# define stats(n) args ( (long long unsigned)cpus[cpuNo].uCache.hits  [n], \
                         (long long unsigned)cpus[cpuNo].uCache.misses[n], \
                         (long long unsigned)cpus[cpuNo].uCache.skips [n] )
  (void)fflush(stdout);
  (void)fflush(stderr);
# if defined(WIN_STDIO)
  sim_msg ("\r|  Instruction Fetch:             |\r\n|    Hits        %15llu  |\r\n|    Misses      %15llu  |\r\n|    Skipped     %15llu  |\r\n|    Effectiveness   %10.2f%%  |\r\n", stats (UC_INSTRUCTION_FETCH));
  (void)fflush(stdout);
  (void)fflush(stderr);
  sim_msg ("\r+---------------------------------+\r\n");
  sim_msg ("\r|  Operand Read:                  |\r\n|    Hits        %15llu  |\r\n|    Misses      %15llu  |\r\n|    Skipped     %15llu  |\r\n|    Effectiveness   %10.2f%%  |\r\n", stats (UC_OPERAND_READ));
  (void)fflush(stdout);
  (void)fflush(stderr);
#  if defined(IDWF_CACHE)
  sim_msg ("\r+---------------------------------+\r\n");
  sim_msg ("\r|  Indirect Word Fetch:           |\r\n|    Hits        %15llu  |\r\n|    Misses      %15llu  |\r\n|    Skipped     %15llu  |\r\n|    Effectiveness   %10.2f%%  |\r\n", stats (UC_INDIRECT_WORD_FETCH));
  (void)fflush(stdout);
  (void)fflush(stderr);
#  endif
  sim_msg ("\r+---------------------------------+\r\n");
  sim_msg ("\r|  Cache Bypasses:                |\r\n");
  sim_msg ("\r|    RALR        %15llu  |\r\n", (long long unsigned)cpus[cpuNo].uCache.ralrSkips);
  sim_msg ("\r|    CALL6       %15llu  |\r\n", (long long unsigned)cpus[cpuNo].uCache.call6Skips);
  sim_msg ("\r|    Segno       %15llu  |\r\n", (long long unsigned)cpus[cpuNo].uCache.segnoSkips);
  (void)fflush(stdout);
  (void)fflush(stderr);
# else
  sim_msg ("\r|  Instruction Fetch:             |\r\n|    Hits        %'15llu  |\r\n|    Misses      %'15llu  |\r\n|    Skipped     %'15llu  |\r\n|    Effectiveness   %'10.2f%%  |\r\n", stats (UC_INSTRUCTION_FETCH));
  (void)fflush(stdout);
  (void)fflush(stderr);
  sim_msg ("\r+---------------------------------+\r\n");
  sim_msg ("\r|  Operand Read:                  |\r\n|    Hits        %'15llu  |\r\n|    Misses      %'15llu  |\r\n|    Skipped     %'15llu  |\r\n|    Effectiveness   %'10.2f%%  |\r\n", stats (UC_OPERAND_READ));
  (void)fflush(stdout);
  (void)fflush(stderr);
#  if defined(IDWF_CACHE)
  sim_msg ("\r+---------------------------------+\r\n");
  sim_msg ("\r|  Indirect Word Fetch:           |\r\n|    Hits        %'15llu  |\r\n|    Misses      %'15llu  |\r\n|    Skipped     %'15llu  |\r\n|    Effectiveness   %'10.2f%%  |\r\n", stats (UC_INDIRECT_WORD_FETCH));
  (void)fflush(stdout);
  (void)fflush(stderr);
#  endif
  sim_msg ("\r+---------------------------------+\r\n");
  sim_msg ("\r|  Cache Bypasses:                |\r\n");
  sim_msg ("\r|    RALR        %'15llu  |\r\n", (long long unsigned)cpus[cpuNo].uCache.ralrSkips);
  sim_msg ("\r|    CALL6       %'15llu  |\r\n", (long long unsigned)cpus[cpuNo].uCache.call6Skips);
  sim_msg ("\r|    Segno       %'15llu  |\r\n", (long long unsigned)cpus[cpuNo].uCache.segnoSkips);
  (void)fflush(stdout);
  (void)fflush(stderr);
# endif
  sim_msg ("\r+---------------------------------+\r\n");
  (void)fflush(stdout);
  (void)fflush(stderr);
# undef pct
# undef args
# undef stats
}
#endif
