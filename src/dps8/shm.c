/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: db14885d-f62f-11ec-9741-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2014-2016 Charles Anthony
 * Copyright (c) 2021 Jeffrey H. Johnson
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

/* Shared memory functions */

#if ( defined( USE_FLOCK ) && defined( USE_FCNTL ))
# ifndef USE_BFLOCK
#  define USE_BFLOCK
# endif /* ifndef USE_BFLOCK */
#endif /* if ( defined(USE_FLOCK) && defined(USE_FCNTL) ) */

#include <errno.h>
#include <fcntl.h> /* For O_* constants, locks */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <sys/types.h>
#include <unistd.h>

#include "shm.h"

#ifndef TRUE
# define TRUE 1
#endif /* ifndef TRUE */

#ifndef FALSE
# define FALSE 0
#endif /* ifndef FALSE */

extern int sim_randstate;
extern int sim_randompst;
extern int sim_nostate;
extern int sim_iglock;
extern int sim_nolock;

void *
create_shm(char *key, size_t shm_size)
{
  void *p;
  char buf[256];

#ifdef USE_BFLOCK
  char lck[260];
#endif /* ifdef USE_BFLOCK */

  sprintf(buf, "dps8m.%s", key);

#ifdef USE_BFLOCK
  sprintf(lck, ".%s", buf);

  int lck_fd = open(lck, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (lck_fd == -1)
    {
      fprintf(
        stderr,
        "%s(): Failed to open \"%s\" (error %d)\r\n",
        __func__,
        lck,
        errno);
      return NULL;
    }

  if (sim_randstate && !sim_randompst)
    {
      unlink(lck);
    }

#endif /* ifdef USE_BFLOCK */

  int fd = open(buf, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1)
    {
      fprintf(
        stderr,
        "%s(): Failed to open \"%s\" (error %d)\r\n",
        __func__,
        buf,
        errno);
      return NULL;
    }

  if (sim_randstate && !sim_randompst)
    {
      unlink(buf);
    }

#ifdef USE_BFLOCK
# define SPIDLEN 128

# ifndef HOST_NAME_MAX
#  ifdef _POSIX_HOST_NAME_MAX
#   define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#  else
#   define HOST_NAME_MAX 255
#  endif /* ifdef _POSIX_HOST_NAME_MAX */
# endif /* ifndef HOST_NAME_MAX */

  struct flock bflock;
  memset(&bflock, 0, sizeof ( bflock ));
  bflock.l_type = F_WRLCK;
  int brc = 0;
  if (!sim_nolock)
    {
      brc = fcntl(lck_fd, F_SETLK, &bflock);
    }

  int fct = 0;
  char spid[SPIDLEN];
  unsigned long lkpid = 0;
  char sthostname[HOST_NAME_MAX + 1];
  char shostname[HOST_NAME_MAX * 2];
  /* cppcheck-suppress unreadVariable */
  int pch = 0;
  int ypch = 0;
  FILE *lck_fp;
  if (brc < 0)
    {
      fprintf(
        stderr,
        "%s(): Failed to lock \"%s\" (error %d)\r\n",
        __func__,
        lck,
        errno);
      if (!sim_nolock)
        {
          if (fcntl(lck_fd, F_GETLK, &bflock) == 0 && bflock.l_pid > 0)
            {
              lkpid = (unsigned long)bflock.l_pid;
            }
        }

      (void)close(lck_fd);
      lck_fp = fopen(lck, "r");
      fprintf(stderr, "\r\n*** Is another simulator running");
      if (lck_fp != NULL)
        {
          while (( pch = fgetc(lck_fp)) != EOF || fct < SPIDLEN)
            {
              if (!( pch < 32 || pch > 126 ))
                {
                  ypch++;
                  if (ypch == 1)
                    {
                      fprintf(stderr, " as PID ");
                    }

                  fprintf(stderr, "%c", pch);
                }

              fct++;
            }
        }

      if (lkpid != 0 && ypch == 0)
        {
          fprintf(stderr, " as PID %lu", lkpid);
        }

      fprintf(stderr, "? ***\r\n\r\n");
      if (!sim_iglock)
        {
          return NULL;
        }
    }

  if (ftruncate(lck_fd, (off_t)0) == -1)
    {
      fprintf(
        stderr,
        "%s(): Failed to clear \"%s\" (error %d)\r\n",
        __func__,
        lck,
        errno);
      if (!sim_iglock)
        {
          return NULL;
        }
    }

# if !defined ( __APPLE__ ) && !defined ( __HAIKU__ ) && !defined ( __serenity__ )
  if ( !(sim_nostate) )
    (void)fdatasync(lck_fd);
# endif /* if !defined ( __APPLE__ ) && !defined ( __HAIKU__ ) && !defined ( __serenity__) */

  (void)snprintf(spid, SPIDLEN, "%ld ", (long)getpid());

  memset(&sthostname, 0, sizeof ( sthostname ));
  int hrc = gethostname(sthostname, HOST_NAME_MAX + 1);
  if (hrc != 0)
    {
      (void)sprintf(sthostname, "(unknown)");
    }

  (void)sprintf(shostname, "on %s\n", sthostname);
  if (write(lck_fd, spid, strlen(spid)) != strlen(spid))
    {
      fprintf(
        stderr,
        "%s(): Failed to save PID to \"%s\" (error %d)\r\n",
        __func__,
        lck,
        errno);
      if (!sim_iglock)
        {
          return NULL;
        }
    }

  if ( !(sim_nostate) )
    (void)fsync(lck_fd);

  if (write(lck_fd, shostname, strlen(shostname)) != strlen(shostname))
    {
      fprintf(
        stderr,
        "%s(): Failed to save host to \"%s\" (error %d)\r\n",
        __func__,
        lck,
        errno);
      if (!sim_iglock)
        {
          return NULL;
        }
    }

  if ( !(sim_nostate) )
    (void)fsync(lck_fd);
#endif /* ifdef USE_BFLOCK */

#ifdef USE_FLOCK
  int rc = 0;
  if (!sim_nolock)
    {
      rc = flock(fd, LOCK_EX | LOCK_NB);
    }

  if (rc < 0)
    {
      fprintf(
        stderr,
        "%s(): Failed to lock \"%s\" (error %d)\r\n",
        __func__,
        buf,
        errno);
      if (!sim_iglock)
        {
          return NULL;
        }
    }

#elif USE_FCNTL /* ifdef USE_FLOCK */
  struct flock lock;
  memset(&lock, 0, sizeof ( lock ));
  lock.l_type = F_WRLCK;
  int rc = 0;
  if (!sim_nolock)
    {
      rc = fcntl(fd, F_SETLK, &lock);
    }

  if (rc < 0)
    {
      fprintf(
        stderr,
        "%s(): Failed to lock \"%s\" (error %d)\r\n",
        __func__,
        buf,
        errno);
      if (fcntl(fd, F_GETLK, &lock) == FALSE && lock.l_pid > 0)
        {
          fprintf(
            stderr,
            "\r\n*** Is another simulator running as PID %lu? ***\r\n\r\n",
            (unsigned long)lock.l_pid);
        }
      if (!sim_iglock)
        {
          return NULL;
        }
    }
#endif /* elif USE_FCNTL */

  if (ftruncate(fd, (off_t)shm_size) == -1)
    {
      fprintf(
        stderr,
        "%s(): Failed to size \"%s\" (error %d)\r\n",
        __func__,
        buf,
        errno);
      return NULL;
    }

#if !defined ( __APPLE__ ) && !defined ( __HAIKU__ ) && !defined ( __serenity__ )
  if ( !(sim_nostate) )
    (void)fdatasync(fd);
#endif /* if !defined ( __APPLE__ ) && !defined ( __HAIKU__ ) && !defined ( __serenity__ ) */

  p = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (p == MAP_FAILED)
    {
      fprintf(
        stderr,
        "%s(): Failed to memory map \"%s\" (error %d)\r\n",
        __func__,
        buf,
        errno);
      return NULL;
    }

  if ( !(sim_nostate) )
    if (msync(p, shm_size, MS_SYNC) == -1)
      {
        fprintf(
          stderr,
          "%s(): Failed to synchronize \"%s\" (error %d)\r\n",
          __func__,
          buf,
          errno);
        return NULL;
      }

  return p;
}

#ifdef API
void *
open_shm(char *key, size_t shm_size)
{
  void *p;
  char buf[256];

# ifdef USE_BFLOCK
  char lck[260];
# endif /* USE_BFLOCK */

  sprintf(buf, "dps8m.%s", key);

# ifdef USE_BFLOCK
  sprintf(lck, ".%s", buf);

  int lck_fd = open(lck, O_RDWR, 0);
  if (lck_fd == -1)
    {
      fprintf(
        stderr,
        "%s(): Failed to open \"%s\" (error %d)\r\n",
        __func__,
        lck,
        errno);
      return NULL;
    }
# endif /* ifdef USE_BFLOCK */

  int fd = open(buf, O_RDWR, 0);
  if (fd == -1)
    {
      fprintf(
        stderr,
        "%s(); Failed to open \"%s\" (error %d)\r\n",
        __func__,
        buf,
        errno);
      return NULL;
    }

# ifdef USE_BFLOCK
  struct flock bflock;
  memset(&bflock, 0, sizeof ( bflock ));
  bflock.l_type = F_WRLCK;
  int brc = 0;
  if (!sim_nolock)
    {
      brc = fcntl(lck_fd, F_SETLK, &bflock);
    }

  if (brc < 0)
    {
      fprintf(
        stderr,
        "%s(): Failed to lock \"%s\": %d\r\n",
        __func__,
        lck,
        errno);
      if (!sim_iglock)
        {
          return NULL;
        }
    }
# endif /* ifdef USE_BFLOCK */

# ifdef USE_FLOCK
  int rc = 0;
  if (!sim_nolock)
    {
      rc = flock(fd, LOCK_EX | LOCK_NB);
    }

  if (rc < 0)
    {
      fprintf(
        stderr,
        "%s(): Failed to lock \"%s\" (error %d)\r\n",
        __func__,
        buf,
        errno);
      if(!sim_iglock) return NULL;
    }

# elif USE_FCNTL /* ifdef USE_FLOCK */
  struct flock lock;
  memset(&lock, 0, sizeof ( lock ));
  lock.l_type = F_WRLCK;
  int rc = 0;
  if (!sim_nolock)
    {
      rc = fcntl(fd, F_SETLK, &lock);
    }

  if (rc < 0)
    {
      fprintf(
        stderr,
        "%s(): Failed to lock \"%s\" (error %d)\r\n",
        __func__,
        buf,
        errno);
      if (!sim_iglock)
        {
          return NULL;
        }
    }
# endif          /* elif USE_FCNTL */

  p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (p == MAP_FAILED)
    {
# ifdef USE_FCNTL
      lock.l_type = F_UNLCK;
      if (!sim_nolock)
        {
          (void)fcntl(fd, F_SETLK, &lock);
        }

      if ( !(sim_nostate) )
        (void)fsync(fd);
      (void)close(fd);
# endif /* ifdef USE_FCNTL */

# ifdef USE_BFLOCK
      bflock.l_type = F_UNLCK;
      if (!sim_nolock)
        {
          (void)fcntl(lck_fd, F_SETLK, &bflock);
        }

      if ( !(sim_nostate) )
        (void)fsync(lck_fd);
      (void)close(lck_fd);
# endif /* ifdef USE_BFLOCK */

      fprintf(
        stderr,
        "%s(): Failed to memory map \"%s\" (error %d)\r\n",
        __func__,
        buf,
        errno);
      return NULL;
    }

  if ( !(sim_nostate) )
    (void)msync(p, shm_size, MS_SYNC);

  return p;
}
#endif /* ifdef API */
