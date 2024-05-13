/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: a10449da-171b-11ee-acc0-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2023 Charles Anthony
 * Copyright (c) 2022-2023 Jeffrey H. Johnson
 * Copyright (c) 2022-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

// It is believed that the maximum segment
// number that Multics uses will be 4095.
// Normal usage should be <= ~512 (0400)?

//#define UC_CACHE_SZ 4096
#define UC_CACHE_SZ 512

// Micro-cache

struct ucache_s {
  bool   valid;
  word15 segno;
  word18 offset;
  word14 bound;
  word1  p;
  word24 address;
  word3  r1;
  bool   paged;
};
typedef struct ucache_s ucache_t;

#define UC_INSTRUCTION_FETCH   0
#define UC_INDIRECT_WORD_FETCH 1
#define UC_OPERAND_READ        2
#define UC_OPERAND_READ_TRA    3
#define UC_OPERAND_READ_CALL6  4
#define UC_NUM                 5

struct uCache_s {
  ucache_t caches [UC_NUM][UC_CACHE_SZ];
#if defined(UCACHE_STATS)
  uint64_t hits   [UC_NUM];
  uint64_t misses [UC_NUM];
  uint64_t skips  [UC_NUM];
  uint64_t call6Skips;
  uint64_t ralrSkips;
  uint64_t segnoSkips;
#endif /* if defined(UCACHE_STATS) */
};

typedef struct uCache_s uCache_t;

void ucInvalidate (void);
void ucCacheSave  (uint ucNum, word15 segno, word18 offset, word14   bound, word1   p, word24   address, word3   r1, bool   paged);
bool ucCacheCheck (uint ucNum, word15 segno, word18 offset, word14 * bound, word1 * p, word24 * address, word3 * r1, bool * paged);
#if defined(UCACHE_STATS)
void ucacheStats (int cpuNo);
#endif /* if defined(UCACHE_STATS) */
