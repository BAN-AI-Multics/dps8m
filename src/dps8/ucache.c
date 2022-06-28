#include <string.h>

#include "dps8.h"
#include "dps8_cpu.h"
#include "ucache.h"

void uc_invalidate (void) {
  memset (cpu.uc_caches, 0, sizeof (cpu.uc_caches));
}

void uc_cache_save (uint uc_num, word15 segno, word18 offset, word14 bound, word1 p, word24 address, word3 r1) {
  cpu.uc_caches[uc_num].valid = true;
  cpu.uc_caches[uc_num].segno = segno;
  cpu.uc_caches[uc_num].offset = offset;
  cpu.uc_caches[uc_num].bound = bound;
  cpu.uc_caches[uc_num].address = address;
  cpu.uc_caches[uc_num].r1 = r1;
  cpu.uc_caches[uc_num].p = p;
}

bool uc_cache_check (uint uc_num, word15 segno, word18 offset, word14 * bound, word1 * p, word24 * address, word3 * r1) {
  if (! cpu.uc_caches[uc_num].valid) {
    //sim_printf ("!valid\r\n");
    return false;
  }
  if (cpu.uc_caches[uc_num].segno != segno) {
    //sim_printf ("segno %o != %o\r\n", cpu.uc_caches[uc_num].segno, segno);
    return false;
  }
  if ((cpu.uc_caches[uc_num].offset & PG18MASK) != (offset & PG18MASK)) {
    //sim_printf ("pgno %o != %o\r\n", (cpu.uc_caches[uc_num].offset & PG18MASK), (offset & PG18MASK));
    return false;
  }
  if (((offset >> 4) & 037777) > cpu.uc_caches[uc_num].bound) {
    //sim_printf ("bound %o != %o\r\n", ((offset >> 4) & 037777), cpu.uc_caches[uc_num].bound);
    return false;
  }
  word18 pgoffset = offset & OS18MASK;
  * bound = cpu.uc_caches[uc_num].bound;
  * address = (cpu.uc_caches[uc_num].address & PG24MASK) + pgoffset;
  * r1 = cpu.uc_caches[uc_num].r1;
  * p = cpu.uc_caches[uc_num].p;
  return true;
}


