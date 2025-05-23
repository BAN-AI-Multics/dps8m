/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 485f7d3b-f630-11ec-953b-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__sunos) && defined(SYSV)
# include <sys/param.h>
#endif /* if defined(__sunos) && defined(SYSV) */
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>
#include "dps8_sir.h"

#define MAX_MKSTEMPS_TRIES 10000

#if defined(__MINGW64__) || defined(__MINGW32__)
# include "bsd_random.h"
# define random  bsd_random
# define srandom bsd_srandom
#endif /* if defined(__MINGW64__) || defined(__MINGW32__) */

#if defined(FREE)
# undef FREE
#endif /* if defined(FREE) */
#define FREE(p) do  \
  {                 \
    free((p));      \
    (p) = NULL;     \
  } while(0)

static char valid_file_name_chars[]
  = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static inline uint32_t
hash32s(const void *buf, size_t len, uint32_t h)
{
  const unsigned char *p = buf;

  for (size_t i = 0; i < len; i++)
    h = h * 31 + p[i];

  h ^= h >> 17;
  h *= UINT32_C(0xed5ad4bb);
  h ^= h >> 11;
  h *= UINT32_C(0xac4c1b51);
  h ^= h >> 15;
  h *= UINT32_C(0x31848bab);
  h ^= h >> 14;

  return h;
}

 /*
  * This is a minimal portable replacement for the mkstemps
  * function, which is not available on all platforms.
  */

int
utfile_mkstemps(char *request_pattern, int suffix_length)
{
  long pattern_length;
  int st1ret;
  char *mask_pointer;
  struct timespec st1;

  char *pattern = strdup(request_pattern);
  if (!pattern)
    {
      (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                     __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
      (void)raise(SIGUSR2);
      /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
      abort();
    }

  pattern_length = (long) strlen(pattern);

  st1ret = clock_gettime(SIR_WALLCLOCK, &st1);
  if (st1ret != 0)
    {
      (void)fprintf (stderr, "\rFATAL: clock_gettime failure! Aborting at %s[%s:%d]\r\n",
                     __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
      (void)raise(SIGUSR2);
      /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
      abort();
    }

  uint32_t h = 0;  /* initial hash value */
#if __STDC_VERSION__ < 201112L
  /* LINTED E_OLD_STYLE_FUNC_DECL */
  void *(*mallocptr)() = malloc;
  h = hash32s(&mallocptr, sizeof(mallocptr), h);
#endif /* if __STDC_VERSION__ < 201112L */
  void *ptr = &ptr;
  h = hash32s(&ptr, sizeof(ptr), h);
  time_t t = time(0);
  h = hash32s(&t, sizeof(t), h);
#if !defined(_AIX) && !defined(__managarm__)
  for (int i = 0; i < 1000; i++)
    {
      unsigned long counter = 0;
      clock_t start = clock();
      while (clock() == start)
        {
          counter++;
        }
      h = hash32s(&start, sizeof(start), h);
      h = hash32s(&counter, sizeof(counter), h);
    }
#endif /* if !defined(_AIX) */
  int mypid = (int)_sir_getpid();
  h = hash32s(&mypid, sizeof(mypid), h);
  char rnd[4];
  FILE *f = fopen("/dev/urandom", "rb");
  if (f)
    {
      if (fread(rnd, sizeof(rnd), 1, f))
        {
          h = hash32s(rnd, sizeof(rnd), h);
        }
      fclose(f);
    }
  srandom(h);

  if (( (long) pattern_length - 6 ) < (long) suffix_length)
  {
    FREE(pattern);
    return ( -1 );
  }

  long mask_offset = (long) pattern_length - ( 6 + (long) suffix_length );

  if (strncmp(&pattern[mask_offset], "XXXXXX", 6))
  {
    FREE(pattern);
    return ( -1 );
  }

  mask_pointer = &pattern[mask_offset];

  long valid_char_count = (long) strlen(valid_file_name_chars);

  if (valid_char_count < 1)
    {
      FREE(pattern);
      return ( -1 );
    }

  for (int count = 0; count < MAX_MKSTEMPS_TRIES; count++)
  {
    for (int mask_index = 0; mask_index < 6; mask_index++)
    {
      mask_pointer[mask_index]
        = valid_file_name_chars[random() % valid_char_count];
    }

    int fd = open(pattern, O_CREAT | O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);

    if (fd >= 0)
    {
      FREE(pattern);
      return ( fd );
    }

    /*
     * If the error is not "file already exists",
     * or is a directory, then we just bail out.
     */

    if (( errno != EEXIST ) && ( errno != EISDIR ))
    {
      break;
    }
  }

  /*
   * If we get here, we were unable to create
   * a unique file name, despite many tries.
   */

  FREE(pattern);
  return ( -1 );
}
