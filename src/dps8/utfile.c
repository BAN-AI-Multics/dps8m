/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 485f7d3b-f630-11ec-953b-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2021-2023 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
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

#define MAX_MKSTEMPS_TRIES 10000

#if defined(__MINGW64__) || defined(__MINGW32__)
# include "bsd_random.h"
# define random  bsd_random
# define srandom bsd_srandom
#endif /* if defined(__MINGW64__) || defined(__MINGW32__) */

#if defined(__MACH__) && defined(__APPLE__) && \
  ( defined(__PPC__) || defined(_ARCH_PPC) )
# include <mach/clock.h>
# include <mach/mach.h>
# ifdef MACOSXPPC
#  undef MACOSXPPC
# endif /* ifdef MACOSXPPC */
# define MACOSXPPC 1
#endif /* if defined(__MACH__) && defined(__APPLE__) &&
           ( defined(__PPC__) || defined(_ARCH_PPC) ) */

#undef FREE
#ifdef TESTING
# define FREE(p) free(p)
#else
# define FREE(p) do  \
  {                  \
    free((p));       \
    (p) = NULL;      \
  } while(0)
#endif /* ifdef TESTING */

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

#ifdef MACOSXPPC
  (void)st1;
#endif /* ifdef MACOSXPPC */
  char *pattern = strdup(request_pattern);
  if (!pattern)
    {
      fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
               __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# ifdef SIGUSR2
      (void)raise(SIGUSR2);
      /*NOTREACHED*/ /* unreachable */
# endif /* ifdef SIGUSR2 */
#endif /* if defined(USE_BACKTRACE) */
      abort();
    }

  pattern_length = (long) strlen(pattern);

#ifdef MACOSXPPC
# undef USE_MONOTONIC
#endif /* ifdef MACOSXPPC */
#ifdef USE_MONOTONIC
  st1ret = clock_gettime(CLOCK_MONOTONIC, &st1);
#else
# ifdef MACOSXPPC
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  st1.tv_sec = mts.tv_sec;
  st1.tv_nsec = mts.tv_nsec;
  st1ret = 0;
# else
  st1ret = clock_gettime(CLOCK_REALTIME, &st1);
# endif /* ifdef MACOSXPPC */
#endif /* ifdef USE_MONOTONIC */
  if (st1ret != 0)
    {
      fprintf (stderr, "\rFATAL: clock_gettime failure! Aborting at %s[%s:%d]\r\n",
               __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# ifdef SIGUSR2
      (void)raise(SIGUSR2);
      /*NOTREACHED*/ /* unreachable */
# endif /* ifdef SIGUSR2 */
#endif /* if defined(USE_BACKTRACE) */
      abort();
    }

  uint32_t h = 0;  /* initial hash value */
#if __STDC_VERSION__ < 201112L
  /* LINTED E_OLD_STYLE_FUNC_DECL */
  void *(*mallocptr)() = malloc;
  h = hash32s(&mallocptr, sizeof(mallocptr), h);
#endif /* if __STDC_VERSION__ < 201112L */
  void *small = malloc(1);
  h = hash32s(&small, sizeof(small), h);
  FREE(small);
  void *big = malloc(1UL << 20);
  h = hash32s(&big, sizeof(big), h);
  FREE(big);
  void *ptr = &ptr;
  h = hash32s(&ptr, sizeof(ptr), h);
  time_t t = time(0);
  h = hash32s(&t, sizeof(t), h);
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
  int mypid = (int)getpid();
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
