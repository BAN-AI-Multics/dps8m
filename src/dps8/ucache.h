/*
 * vim: filetype=c:tabstop=4:tw=100:expandtab
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2022 Charles Anthony
 * Copyright (c) 2022 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#define UC_BIG_CACHE_SZ 4096

// Micro cache

struct ucache_s {
  bool valid;
  word15 segno;
  word18 offset;
  word14 bound;
  word1  p;
  word24 address;
  word3  r1;
  bool paged;
};
typedef struct ucache_s ucache_t;

#define UC_INSTRUCTION_FETCH 0
#define UC_OPERAND_READ 1
#define UC_INDIRECT_WORD_FETCH 2
#define UC_NUM 3

struct uCache_s {
  ucache_t caches[UC_NUM];
  ucache_t operandReadBigCache[UC_BIG_CACHE_SZ];
#ifdef UCACHE_STATS
  uint64_t hits[UC_NUM];
  uint64_t misses[UC_NUM];
  uint64_t skips[UC_NUM];
  uint64_t call6Skips;
#endif
};

typedef struct uCache_s uCache_t;

void ucInvalidate (void);
void ucCacheSave (uint ucNum, word15 segno, word18 offset, word14 bound, word1 p, word24 address, word3 r1, bool paged);
bool ucCacheCheck (uint ucNum, word15 segno, word18 offset, word14 * bound, word1 * p, word24 * address, word3 * r1, bool * paged);
