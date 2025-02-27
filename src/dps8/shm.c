/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: db14885d-f62f-11ec-9741-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2014-2016 Charles Anthony
 * Copyright (c) 2021 Jeffrey H. Johnson
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

/* Shared memory functions */

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

#include "../simh/sim_defs.h"
#include "shm.h"

#if defined(NO_LOCALE)
# define xstrerror_l strerror
#endif

#if defined(USE_FLOCK) && defined(USE_FCNTL)
# if !defined(USE_BFLOCK)
#  define USE_BFLOCK
# endif /* if !defined(USE_BFLOCK) */
#endif /* if defined(USE_FLOCK) && defined(USE_FCNTL) */

#if !defined(TRUE)
# define TRUE 1
#endif /* if !defined(TRUE) */

#if !defined(FALSE)
# define FALSE 0
#endif /* if !defined(FALSE) */

extern int sim_randstate;
extern int sim_randompst;
extern int sim_nostate;
extern int sim_iglock;
extern int sim_nolock;

#if !defined(NO_LOCALE)
const char *xstrerror_l(int errnum);
#endif

void *
create_shm(char *key, size_t shm_size)
{
  void *p;
  char buf[256];

#if defined(USE_BFLOCK)
  char lck[260];
#endif /* if defined(USE_BFLOCK) */

  (void)sprintf (buf, "dps8m.%s", key);

#if defined(USE_BFLOCK)
  (void)sprintf (lck, ".%s", buf);

  int lck_fd = open(lck, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (lck_fd == -1)
    {
      (void)fprintf(stderr, "%s(): Failed to open \"%s\": %s (Error %d)\r\n",
                    __func__, lck, xstrerror_l(errno), errno);
      return NULL;
    }

  if (sim_randstate && !sim_randompst)
    {
      unlink(lck);
    }

#endif /* if defined(USE_BFLOCK) */

  int fd = open(buf, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1)
    {
      (void)fprintf(stderr, "%s(): Failed to open \"%s\": %s (Error %d)\r\n",
                    __func__, buf, xstrerror_l(errno), errno);
#if defined(USE_BFLOCK)
      (void)close(lck_fd);
#endif /* if defined(USE_BFLOCK) */
      return NULL;
    }

  if (sim_randstate && !sim_randompst)
    {
      unlink(buf);
    }

#if defined(USE_BFLOCK)
# define SPIDLEN 128

# if !defined(HOST_NAME_MAX)
#  if defined(_POSIX_HOST_NAME_MAX)
#   define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#  else
#   define HOST_NAME_MAX 255
#  endif /* if defined(_POSIX_HOST_NAME_MAX) */
# endif /* if !defined(HOST_NAME_MAX) */

  struct flock bflock;
  (void)memset(&bflock, 0, sizeof ( bflock ));
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
  int lck_ftr = 0;
  FILE *lck_fp;
  if (brc < 0)
    {
      (void)fprintf(stderr, "%s(): Failed to lock \"%s\": %s (Error %d)\r\n",
                    __func__, lck, xstrerror_l(errno), errno);
      if (!sim_nolock)
        {
          if (fcntl(lck_fd, F_GETLK, &bflock) == 0 && bflock.l_pid > 0)
            {
              lkpid = (unsigned long)bflock.l_pid;
            }
        }

# if !defined(__clang_analyzer__)
      (void)close(lck_fd);
      lck_ftr = 1;
      lck_fp = fopen(lck, "r");
      (void)fprintf(stderr, "\r\n*** Is another simulator running");
      if (lck_fp != NULL)
        {
          while (( pch = fgetc(lck_fp)) != EOF || fct < SPIDLEN)
            {
              if (!( pch < 32 || pch > 126 ))
                {
                  ypch++;
                  if (ypch == 1)
                    {
                      (void)fprintf(stderr, " as PID ");
                    }

                  (void)fprintf(stderr, "%c", pch);
                }

              fct++;
            }
          (void)fclose(lck_fp);
        }
# endif

      if (lkpid != 0 && ypch == 0)
        {
          (void)fprintf(stderr, " as PID %lu", lkpid);
        }

      (void)fprintf(stderr, "? ***\r\n\r\n");
      if (!sim_iglock)
        {
          (void)close(fd);
          return NULL;
        }
    }

  if (!lck_ftr && (ftruncate(lck_fd, (off_t)0) == -1))
    {
      (void)fprintf(stderr, "%s(): Failed to clear \"%s\": %s (Error %d)\r\n",
                    __func__, lck, xstrerror_l(errno), errno);
      if (!sim_iglock)
        {
          return NULL;
        }
    }

  (void)snprintf(spid, SPIDLEN, "%ld ", (long)getpid());

  (void)memset(&sthostname, 0, sizeof ( sthostname ));
  int hrc = gethostname(sthostname, HOST_NAME_MAX + 1);
  if (hrc != 0)
    {
      (void)sprintf (sthostname, "(unknown)");
    }

  (void)sprintf (shostname, "on %s\n", sthostname);
  if (!lck_ftr && (write(lck_fd, spid, strlen(spid)) != strlen(spid)))
    {
      (void)fprintf(stderr, "%s(): Failed to save PID to \"%s\": %s (Error %d)\r\n",
                    __func__, lck, xstrerror_l(errno), errno);
      if (!sim_iglock)
        {
          (void)close(fd);
          (void)close(lck_fd);
          return NULL;
        }
    }

  if (!lck_ftr && !(sim_nostate))
    (void)fsync(lck_fd);

  if (!lck_ftr && (write(lck_fd, shostname, strlen(shostname)) != strlen(shostname)))
    {
      (void)fprintf(stderr, "%s(): Failed to save host to \"%s\": %s (Error %d)\r\n",
                    __func__, lck, xstrerror_l(errno), errno);
      if (!sim_iglock)
        {
          (void)close(fd);
          return NULL;
        }
    }

  if (!lck_ftr && !(sim_nostate))
    (void)fsync(lck_fd);
#endif /* if defined(USE_BFLOCK) */

#if defined(USE_FLOCK)
  int rc = 0;
  if (!sim_nolock)
    {
      rc = flock(fd, LOCK_EX | LOCK_NB);
    }

  if (rc < 0)
    {
      (void)fprintf(stderr, "%s(): Failed to lock \"%s\": %s (Error %d)\r\n",
                    __func__, buf, xstrerror_l(errno), errno);
      if (!sim_iglock)
        {
          return NULL;
        }
    }

#elif USE_FCNTL /* if defined(USE_FLOCK) */
  struct flock lock;
  (void)memset(&lock, 0, sizeof ( lock ));
  lock.l_type = F_WRLCK;
  int rc = 0;
  if (!sim_nolock)
    {
      rc = fcntl(fd, F_SETLK, &lock);
    }

  if (rc < 0)
    {
      (void)fprintf(stderr, "%s(): Failed to lock \"%s\": %s (Error %d)\r\n",
                    __func__, buf, xstrerror_l(errno), errno);
      if (fcntl(fd, F_GETLK, &lock) == FALSE && lock.l_pid > 0)
        {
          (void)fprintf(stderr,
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
      (void)fprintf(stderr, "%s(): Failed to initialize \"%s\": %s (Error %d)\r\n",
                    __func__, buf, xstrerror_l(errno), errno);
      return NULL;
    }

  p = mmap(NULL, shm_size, PROT_READ | PROT_WRITE,
#if defined(MAP_POPULATE)
          MAP_POPULATE |
#endif
#if defined(MAP_NOSYNC)
           MAP_NOSYNC |
#endif
           MAP_SHARED, fd, 0);
  if (p == MAP_FAILED)
    {
      (void)fprintf(stderr, "%s(): Failed to memory map \"%s\": %s (Error %d)\r\n",
                    __func__, buf, xstrerror_l(errno), errno);
      return NULL;
    }

  return p;
}

#if defined(API)
void *
open_shm(char *key, size_t shm_size)
{
  void *p;
  char buf[256];

# if defined(USE_BFLOCK)
  char lck[260];
# endif /* if defined(USE_BFLOCK) */

  (void)sprintf (buf, "dps8m.%s", key);

# if defined(USE_BFLOCK)
  (void)sprintf (lck, ".%s", buf);

  int lck_fd = open(lck, O_RDWR, 0);
  if (lck_fd == -1)
    {
      (void)fprintf(stderr, "%s(): Failed to open \"%s\": %s (Error %d)\r\n",
                    __func__, lck, xstrerror_l(errno), errno);
      return NULL;
    }
# endif /* if defined(USE_BFLOCK) */

  int fd = open(buf, O_RDWR, 0);
  if (fd == -1)
    {
      (void)fprintf(stderr, "%s(); Failed to open \"%s\": %s (Error %d)\r\n",
                    __func__, buf, xstrerror_l(errno), errno);
      return NULL;
    }

# if defined(USE_BFLOCK)
  struct flock bflock;
  (void)memset(&bflock, 0, sizeof ( bflock ));
  bflock.l_type = F_WRLCK;
  int brc = 0;
  if (!sim_nolock)
    {
      brc = fcntl(lck_fd, F_SETLK, &bflock);
    }

  if (brc < 0)
    {
      (void)fprintf(stderr, "%s(): Failed to lock \"%s\": %s (Error %d)\r\n",
                    __func__, lck, xstrerror_l(errno), errno);
      if (!sim_iglock)
        {
          return NULL;
        }
    }
# endif /* if defined(USE_BFLOCK) */

# if defined(USE_FLOCK)
  int rc = 0;
  if (!sim_nolock)
    {
      rc = flock(fd, LOCK_EX | LOCK_NB);
    }

  if (rc < 0)
    {
      (void)fprintf(stderr, "%s(): Failed to lock \"%s\": %s (Error %d)\r\n",
                    __func__, buf, xstrerror_l(errno), errno);
      if(!sim_iglock) return NULL;
    }

# elif defined(USE_FCNTL)
  struct flock lock;
  (void)memset(&lock, 0, sizeof ( lock ));
  lock.l_type = F_WRLCK;
  int rc = 0;
  if (!sim_nolock)
    {
      rc = fcntl(fd, F_SETLK, &lock);
    }

  if (rc < 0)
    {
      (void)fprintf(stderr, "%s(): Failed to lock \"%s\": %s (Error %d)\r\n",
                    __func__, buf, xstrerror_l(errno), errno);
      if (!sim_iglock)
        {
          return NULL;
        }
    }
# endif

  p = mmap(NULL, size, PROT_READ | PROT_WRITE,
# if defined(MAP_POPULATE)
          MAP_POPULATE |
# endif
# if defined(MAP_NOSYNC)
           MAP_NOSYNC |
# endif
           MAP_SHARED, fd, 0);
  if (p == MAP_FAILED)
    {
# if defined(USE_FCNTL)
      lock.l_type = F_UNLCK;
      if (!sim_nolock)
        {
          (void)fcntl(fd, F_SETLK, &lock);
        }

      if ( !(sim_nostate) )
        (void)fsync(fd);
      (void)close(fd);
# endif /* if defined(USE_FCNTL) */

# if defined(USE_BFLOCK)
      bflock.l_type = F_UNLCK;
      if (!sim_nolock)
        {
          (void)fcntl(lck_fd, F_SETLK, &bflock);
        }

      if ( !(sim_nostate) )
        (void)fsync(lck_fd);
      (void)close(lck_fd);
# endif /* if defined(USE_BFLOCK) */

      (void)fprintf(stderr, "%s(): Failed to memory map \"%s\": %s (Error %d)\r\n",
                    __func__, buf, xstrerror_l(errno), errno);
      return NULL;
    }

  return p;
}
#endif /* if defined(API) */
