#include <string.h>

#include "dps8.h"
#include "dps8_cpu.h"

void ucInvalidate (void) {
  memset (cpu.uCache.caches, 0, sizeof (cpu.uCache.caches));
}

void ucCacheSave (uint ucNum, word15 segno, word18 offset, word14 bound, word1 p, word24 address, word3 r1, bool paged) {
  if (segno >= UC_CACHE_SZ) {
    return;
  }
  ucache_t * ep;
  ep = & cpu.uCache.caches[ucNum][segno];
  ep->valid = true;
  ep->segno = segno;
  ep->offset = offset;
  ep->bound = bound;
  ep->address = address;
  ep->r1 = r1;
  ep->p = p;
  ep->paged = paged;
#ifdef HDBG
  hdbgNote ("ucache", "save %u %05o:%06o %05o %o %08o %o %o", ucNum, segno, offset, bound, p, address, r1, paged);
#endif
}

bool ucCacheCheck (uint ucNum, word15 segno, word18 offset, word14 * bound, word1 * p, word24 * address, word3 * r1, bool * paged) {
  if (segno >= UC_CACHE_SZ) {
    cpu.uCache.segnoSkips ++;
    return false;
  }
  ucache_t * ep;
  ep = & cpu.uCache.caches[ucNum][segno];
  // Is cache entry valid?
  if (! ep->valid) {
#ifdef HDBG
    hdbgNote ("ucache", "check not valid");
#endif
    goto miss;
  }
#if 0
  // Same segment?
  if (ep->segno != segno) {
# ifdef HDBG
    hdbgNote ("ucache", "segno %o != %o\r\n", ep->segno, segno);
# endif
    goto miss;
  }
#endif
  // Same page?
  if (ep->paged && ((ep->offset & PG18MASK) != (offset & PG18MASK))) {
#ifdef HDBG
    hdbgNote ("ucache", "pgno %o != %o\r\n", (ep->offset & PG18MASK), (offset & PG18MASK));
#endif
    goto miss;
  }
  // In bounds?
  if (((offset >> 4) & 037777) > ep->bound) {
    //sim_printf ("bound %o != %o\r\n", ((offset >> 4) & 037777), ep->bound);
#ifdef HDBG
    hdbgNote ("ucache", "bound %o != %o\r\n", ((offset >> 4) & 037777), ep->bound);
#endif
    goto miss;
  }
#ifdef HDBG
  hdbgNote ("ucache", "hit %u %05o:%06o %05o %o %08o %o %o", ucNum, segno, offset, ep->bound, ep->p, ep->address, ep->r1, ep->paged);
#endif
  * bound = ep->bound;
#if 0
  if (ep->paged) {
    word18 pgoffset = offset & OS18MASK;
    * address = (ep->address & PG24MASK) + pgoffset;
# ifdef HDBG
    hdbgNote ("ucache", "  FAP pgoffset %06o address %08o", pgoffset, * address);
# endif
  } else {
    * address = (ep->address & 077777760) + offset;
# ifdef HDBG
    hdbgNote ("ucache", "  FANP pgoffset %06o address %08o", pgoffset, * address);
# endif
  }
#else
  * address = ep->address;
#endif
  * r1 = ep->r1;
  * p = ep->p;
  * paged = ep->paged;
#ifdef UCACHE_STATS
  cpu.uCache.hits[ucNum] ++;
#endif
  return true;
miss:;
#ifdef UCACHE_STATS
  cpu.uCache.misses[ucNum] ++;
#endif
  return false;
}

#ifdef UCACHE_STATS
void ucacheStats (int cpuNo) {

  sim_msg ("Micro-cache statisitics (hit/miss/skip)\n");
# define pct(a, b) ((b) ? (a) * 100.0 / ((a) + (b)) : 0)
# define args(a, b, c) a, b, c, pct (a, (b + c))
# define stats(n) args (cpus[cpuNo].uCache.hits[n], cpus[cpuNo].uCache.misses[n], cpus[cpuNo].uCache.skips[n])
  sim_msg ("  Instruction fetch %'lu/%'lu/%'lu %4.1f%%\n", stats (UC_INSTRUCTION_FETCH));
  sim_msg ("  Operand read %'lu/%'lu/%'lu %4.1f%%\n", stats (UC_OPERAND_READ));
  sim_msg ("  Indirect word fetch %'lu/%'lu/%'lu %4.1f%%\n", stats (UC_INDIRECT_WORD_FETCH));
  sim_msg ("  RALR skips: %'lu\n", cpus[cpuNo].uCache.ralrSkips);
  sim_msg ("  CALL6 skips: %'lu\n", cpus[cpuNo].uCache.call6Skips);
  sim_msg ("  Segno skips: %'lu\n", cpus[cpuNo].uCache.segnoSkips);
# undef pct
# undef args
# undef stats
}
#endif

