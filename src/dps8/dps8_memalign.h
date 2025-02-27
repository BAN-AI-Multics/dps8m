/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: f720964c-f3ed-11ef-b468-922037bebb33
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#if !defined(_DPS8_MEMALIGN_H)
# define _DPS8_MEMALIGN_H

# include <stdlib.h>
# include <stdatomic.h>
# include "dps8_sir.h"

# undef ALLOC_SIZE
# if HAS_ATTRIBUTE(alloc_size)
#  define ALLOC_SIZE(size) __attribute__((alloc_size(size)))
# endif
# if !defined(ALLOC_SIZE)
#  define ALLOC_SIZE(size)
# endif

# undef ASSUMED_ALIGN
# if HAS_ATTRIBUTE(assumed_align)
#  define ASSUMED_ALIGN(size) __attribute__((assumed_align(size)))
# endif
# if !defined(ASSUMED_ALIGN)
#  define ASSUMED_ALIGN(size)
# endif

# undef ATTR_MALLOC
# if HAS_ATTRIBUTE(malloc)
#  define ATTR_MALLOC __attribute__((malloc))
# endif
# if !defined(ATTR_MALLOC)
#  define ATTR_MALLOC
# endif

# if (!defined(_WIN32) && !defined(__CYGWIN__)) && !defined(__MINGW64__) && !defined(__MINGW32__) && \
     !defined(CROSS_MINGW64) && !defined(CROSS_MINGW32) && !defined(DUMA)
#  include <unistd.h>

#  if defined(_SC_PAGESIZE)
#   define DPS8_MEMALIGN_PAGESIZE _SC_PAGESIZE
#  elif defined(_SC_PAGE_SIZE)
#   define DPS8_MEMALIGN_PAGESIZE _SC_PAGE_SIZE
#  else
#   error No _SC_PAGESIZE or _SC_PAGE_SIZE defined
#  endif

ATTR_MALLOC ASSUMED_ALIGN(DPS8_MEMALIGN_PAGESIZE) ALLOC_SIZE(1)
static inline void *
aligned_malloc(size_t size)
{
    static atomic_size_t page_size = 0;
    size_t current_page_size = atomic_load(&page_size);

    if (current_page_size == 0) {
        size_t temp = (size_t)sysconf(DPS8_MEMALIGN_PAGESIZE);
        size_t expected = 0;

        if (temp < 1)
            temp = 4096;

        if (atomic_compare_exchange_strong(&page_size, &expected, temp)) {
            current_page_size = temp;
        } else {
            current_page_size = atomic_load(&page_size);
        }
    }

    void *ptr = NULL;
    if (posix_memalign(&ptr, current_page_size, size) != 0)
        return NULL;

    return ptr;
}

# else

ATTR_MALLOC ALLOC_SIZE(1)
static inline void *
aligned_malloc(size_t size)
{
    return malloc(size);
}

# endif
#endif
