/*
 * vim: filetype=c:tabstop=2:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: e86a7296-0066-11f0-aebb-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2025 Jeffrey H. Johnson
 * Copyright (c) 2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

/*
 * Security and privilege stuff
 *
 * Supported: SunOS (Solaris, illumos) only.
 *
 * sunos_obtain_realtime_privileges - On SunOS, tries to obtain privileges
 *     needed for real-time operations.  This requires privileges itself,
 *     so the binary should be made setuid root.  After privileges are
 *     obtained, all unnecessary privileges are dropped.  This should be
 *     called early from main so superuser privileges are given up quickly.
 *
 * Potentially we can extend these functions to support other systems.
 *
 * Linux with Landlock is a possible candidate, although Landlock support
 * is not enabled by default in any of the mainstream Linux distributions.
 *
 * NOTE: Not using libsir logging because this happens before sir_init!
 */

#if defined(__sun) || defined(__illumos__)
# include <errno.h>
# include <priv.h>
# include <stdbool.h>
# include <stdint.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
#endif

/* SCP xstrerror_l */
#if defined(NO_LOCALE)
# define xstrerror_l strerror
#else
extern const char *xstrerror_l(int errnum);
#endif

/* sunos_obtain_realtime_privileges: try to obtain RT privileges */
void
sunos_obtain_realtime_privileges(void)
{
#if defined(__sun) || defined(__illumos__)
  uid_t ruid = getuid();

  priv_set_t *basic = NULL;
  priv_set_t *desired = NULL;

  basic = priv_allocset();
  desired = priv_allocset();

  if (NULL != basic && NULL != desired) {
    priv_basicset(basic);
    priv_copyset(basic, desired);

    const char *wanted_privs[] = {
      PRIV_PROC_LOCK_MEMORY,
      PRIV_PROC_PRIOCNTL
    };

    for (size_t i = 0; i < sizeof(wanted_privs) / sizeof(wanted_privs[0]); i++) {
      if (-1 == priv_addset(desired, wanted_privs[i])) {
# if defined(TESTING)
        (void)fprintf(stderr, "Unable to request %s: %s\r\n",
                      wanted_privs[i], xstrerror_l(errno));
# else
        (void)desired; /*LINT*/
# endif
      }
    }

# if defined(TESTING)
    bool warn_changed = false;
# endif
    if (-1 == setppriv(PRIV_SET, PRIV_PERMITTED, desired)) {
# if defined(TESTING)
      (void)fprintf(stderr, "Unable to change permitted privilege set: %s\r\n",
                    xstrerror_l(errno));
      warn_changed = true;
# else
      (void)desired; /*LINT*/
# endif
    }

    if (-1 == setppriv(PRIV_SET, PRIV_INHERITABLE, desired)) {
# if defined(TESTING)
      if (false == warn_changed) {
        (void)fprintf(stderr, "Unable to change inheritable privilege set: %s\r\n",
                      xstrerror_l(errno));
      }
# else
      (void)desired; /*LINT*/
# endif
    }
  }
# if defined(TESTING)
  else {
    (void)fprintf(stderr, "Failed to allocate memory for privilege sets.\r\n");
  }
# endif

  if (-1 == seteuid(ruid)) {
    (void)fprintf(stderr, "Unable to drop privileges from UID %d: %s\r\n",
                  geteuid(), xstrerror_l(errno));
    exit(EXIT_FAILURE);
  }

  if (NULL != basic && -1 == setppriv(PRIV_SET, PRIV_INHERITABLE, basic)) {
    (void)fprintf(stderr, "Unable to change inheritable privilege set: %s\r\n",
                  xstrerror_l(errno));
    priv_freeset(basic);
    priv_freeset(desired);
    exit(EXIT_FAILURE);
  }

  if (NULL != basic) {
    priv_freeset(basic);
  }

  if (NULL != desired) {
    priv_freeset(desired);
  }
#else
  /* Not SunOS (Solaris, illumos): do nothing */
#endif
}
