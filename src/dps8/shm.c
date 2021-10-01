/*
 * Copyright (c) 2014-2016 Charles Anthony
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 */

// shared memory library

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#ifdef USE_FLOCK
#include <sys/file.h>
#endif

#include "shm.h"

void * create_shm (char * key, size_t size)
  {
    void * p;
    char buf [256];
#ifdef L68
    sprintf (buf, "l68.%s", key);
#else
    sprintf (buf, "dps8m.%s", key);
#endif /* ifdef L68 */
    int fd = open (buf, O_RDWR | O_CREAT, 0600);
    if (fd == -1)
      {
        fprintf (stderr, "create_shm open fail %d\r\n", errno);
        return NULL;
      }

#ifdef USE_FLOCK
    int rc = flock (fd, LOCK_EX | LOCK_NB);
    if (rc < 0) {
      fprintf (stderr, "%s flock fail %d\r\n", __func__, errno);
      return NULL;
    }
#endif

    if (ftruncate (fd, (off_t) size) == -1)
      {
        fprintf (stderr, "create_shm  ftruncate  fail %d\r\n", errno);
        return NULL;
      }

    p = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (p == MAP_FAILED)
      {
        fprintf (stderr, "create_shm mmap  fail %d\r\n", errno);
        return NULL;
      }
    return p;
  }

void * open_shm (char * key, size_t size)
  {
    void * p;
    char buf [256];
#ifdef L68
    sprintf (buf, "l68.%s", key);
#else
    sprintf (buf, "dps8m.%s", key);
#endif /* ifdef L68 */
    int fd = open (buf, O_RDWR, 0600);
    if (fd == -1)
      {
        fprintf (stderr, "open_shm open fail %d\r\n", errno);
        return NULL;
      }

#ifdef USE_FLOCK
    int rc = flock (fd, LOCK_EX | LOCK_NB);
    if (rc < 0) {
      fprintf (stderr, "%s flock fail %d\r\n", __func__, errno);
      return NULL;
    }
#endif

    p = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED)
      {
        close (fd);
        fprintf (stderr, "open_shm mmap  fail %d\r\n", errno);
        return NULL;
      }
    return p;
  }
