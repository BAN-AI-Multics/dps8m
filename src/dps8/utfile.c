/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 485f7d3b-f630-11ec-953b-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2021-2022 The DPS8M Development Team
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
#include <unistd.h>

#define MAX_MKSTEMPS_TRIES 10000

#if defined(__MINGW64__) || defined(__MINGW32__)
# include "bsd_random.h"
# define random  bsd_random
# define srandom bsd_srandom
#endif /* if defined(__MINGW64__) || defined(__MINGW32__) */

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

#ifdef USE_MONOTONIC
  st1ret = clock_gettime(CLOCK_MONOTONIC, &st1);
#else
  st1ret = clock_gettime(CLOCK_REALTIME, &st1);
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

  srandom((unsigned int)(getpid() ^ (long)((1LL + (long long)st1.tv_nsec) * (1LL + (long long)st1.tv_sec))));

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
   * a unique file name despite many of tries.
   */

  FREE(pattern);
  return ( -1 );
}
