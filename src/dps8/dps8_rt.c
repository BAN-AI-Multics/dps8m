/*
 * vim: filetype=c:tabstop=2:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: d18b0dfa-fd83-11ef-a2d3-80ee73e9b8e7
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
 * Real-time watchdog and priority helpers
 *
 * Supported: IBM AIX, Android, Linux/glibc, Linux/musl,
 *            macOS, QNX, SerenityOS, FreeBSD, NetBSD,
 *            OpenBSD, Haiku, Solaris, and illumos.
 *
 * Unsupported: Microsoft Windows, Cygwin, GNU/Hurd,
 *              IBM z/OS USS, and PASE for IBM i.
 */

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE
#endif

#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "dps8.h"
#include "dps8_cpu.h"
#include "dps8_priv.h"
#include "dps8_sir.h"

#include "../simh/sim_timer.h"

#if !defined(WATCHDOG_INTERVAL)
# define WATCHDOG_INTERVAL (5000000) /* 5 seconds */
#endif

#if !defined(WATCHDOG_MAX_MISSES)
# define WATCHDOG_MAX_MISSES (6)
#endif

#if !defined(WD_MS_SEC)
# define WD_MS_SEC 1000000
#endif

/* RT scheduler class */
#if !defined(RT_SCHEDULER)
# define RT_SCHEDULER SCHED_RR
#endif

/* SCP xstrerror_l */
#if defined(NO_LOCALE)
# define xstrerror_l strerror
#else
extern const char *xstrerror_l(int errnum);
#endif

/* Globals */
volatile time_t watchdog_timestamp;
volatile bool realtime_ok = false;

struct g_sched_info {
  int policy;
  struct sched_param param;
};
struct g_sched_info global_sched_info;

/* restore_thread_sched: sets thread scheduler and priority as stashed by save_thread_sched */
int
restore_thread_sched(const pthread_t thread_id)
{
  return pthread_setschedparam(thread_id, global_sched_info.policy, &global_sched_info.param);
}

/* watchdog_recover: recovery procedure */
void
watchdog_recover(void)
{
  static unsigned long watchdog_triggered = 0;

  watchdog_triggered++;
  realtime_ok = false;

  (void)sir_alert("RT watchdog activated after %d missed updates (%d seconds)",
                  WATCHDOG_MAX_MISSES, WATCHDOG_MAX_MISSES * (WATCHDOG_INTERVAL / WD_MS_SEC));

  if (watchdog_triggered == 1) {
    (void)sir_notice("Resetting supervisor thread parameters");
    int ret = restore_thread_sched(main_thread_id);
    if (0 != ret) {
      (void)sir_error("Error #%d resetting supervisor thread parameters: %s",
                      ret, xstrerror_l(ret));
    }

#if defined(THREADZ) || defined(LOCKLESS)
    ret = 0;
    for (uint32_t cpuNo = 0; cpuNo < N_CPU_UNITS_MAX; cpuNo++) {
      if (cpus[cpuNo].cycleCnt) {
        (void)sir_notice("Resetting CPU %c thread parameters", cpuNo + 'A');
        ret = restore_thread_sched(cpus[cpuNo].thread_id);
        if (0 != ret) {
          (void)sir_error("Error #%d resetting CPU %c thread parameters: %s",
                          ret, cpuNo + 'A', xstrerror_l(ret));
        }
      }
    }
#endif
  } else {
    (void)sir_warn("RT watchdog count is %lu; not resetting thread parameters!",
                   watchdog_triggered);
  }
}

/* watchdog_writer thread runs at standard or lowered priority */
void
*watchdog_writer(void *arg)
{
  const bool forever = true;
  (void)arg;

  (void)_sir_setthreadname("watchdog_writer");

#if !defined(__QNX__)
  (void)sim_os_set_thread_priority(PRIORITY_BELOW_NORMAL);
#endif

  while (forever) { //-V654
    watchdog_timestamp = time(NULL);
    (void)sim_usleep(WATCHDOG_INTERVAL);
  }

  return NULL;
}

/* watchdog_reader thread runs at maximum real-time priority */
void
*watchdog_reader(void *arg)
{
  const bool forever = true;
  int miss_count = 0;
  (void)arg;

  (void)_sir_setthreadname("rt_watchdog");

  (void)sim_usleep(WATCHDOG_INTERVAL);
  time_t prev_timestamp = watchdog_timestamp;

  while (forever) { //-V654
    (void)sim_usleep(WATCHDOG_INTERVAL);

    time_t current_timestamp = watchdog_timestamp;

    if (current_timestamp == prev_timestamp) {
      miss_count++;
      if (miss_count >= WATCHDOG_MAX_MISSES) {
        watchdog_recover();
      }
    } else {
      miss_count = 0;
    }

    prev_timestamp = current_timestamp;
  }

  return NULL;
}

/*
 * To simplify the implementation, all of the following
 * functions will exit on failure!  Call watchdog_startup
 * early in the start-up process to avoid nasty surprises.
 */

/* save_thread_sched: stash current scheduler and priority */
void
save_thread_sched(const pthread_t thread_id)
{
  int ret = pthread_getschedparam(thread_id, &global_sched_info.policy, &global_sched_info.param);
  if (0 != ret) {
    (void)sir_emerg("FATAL: Failed to save current scheduler parameters!");
    (void)sir_emerg("Error #%d - %s", ret, xstrerror_l(ret));
    exit(EXIT_FAILURE);
    }
}

/* realtime_max_priority: get maximum realtime priority */
int
realtime_max_priority(void)
{
  static int max_priority;

#if defined(_AIX) && defined(__PASE__)
  max_priority = 127;
#else
  max_priority = sched_get_priority_max(RT_SCHEDULER);
  if (-1 == max_priority) {
    (void)sir_emerg("FATAL: Failed to query maximum priority level!");
    (void)sir_emerg("Error #%d - %s", errno, xstrerror_l(errno));
    exit(EXIT_FAILURE);
  }
#endif

  return max_priority;
}

/* set_realtime_priority: set thread_id to realtime priority level */
void
set_realtime_priority(const pthread_t thread_id, const int priority)
{
  struct sched_param param;
  param.sched_priority = priority;

  int ret = pthread_setschedparam(thread_id, RT_SCHEDULER, &param);
  if (0 != ret) {
    (void)sir_emerg("FATAL: Failed to set real-time watchdog priority!");
    (void)sir_emerg("Error #%d - %s", ret, xstrerror_l(ret));
    exit(EXIT_FAILURE);
  }
}

/* check_realtime_priority and check_not_realtime_priority implementation */
static void
check_realtime_priority_impl(const pthread_t thread_id, const int priority, const bool verify)
{
  int policy;
  struct sched_param param_check;
  int ret = pthread_getschedparam(thread_id, &policy, &param_check);
  if (0 == ret) {
    if (verify) {
      if (RT_SCHEDULER != policy) {
        (void)sir_emerg("FATAL: Failed to validate real-time policy (%d != %d)!",
                        RT_SCHEDULER, policy);
        exit(EXIT_FAILURE);
      }
      if (priority != param_check.sched_priority) {
        (void)sir_emerg("FATAL: Failed to validate real-time priority (%d != %d)!",
                        priority, param_check.sched_priority);
        exit(EXIT_FAILURE);
      }
    } else {
      if (priority == param_check.sched_priority) {
        (void)sir_emerg("FATAL: Failed to validate real-time priority (%d)!",
                        param_check.sched_priority);
        exit(EXIT_FAILURE);
      }
    }
  } else {
    (void)sir_emerg("FATAL: Failed to query real-time parameters!");
    (void)sir_emerg("Error #%d - %s", ret, xstrerror_l(ret));
    exit(EXIT_FAILURE);
  }
}

/* check_realtime_priority: verify thread_id set to realtime priority level */
void
check_realtime_priority(const pthread_t thread_id, const int priority)
{
  check_realtime_priority_impl(thread_id, priority, true);
}

/* check_not_realtime_priority: verify thread_id not set to realtime priority level */
void
check_not_realtime_priority(const pthread_t thread_id, const int priority)
{
  check_realtime_priority_impl(thread_id, priority, false);
}

/* watchdog_startup: start the watchdog threads */
void
watchdog_startup(void)
{
  const int max_priority = realtime_max_priority();
  int ret;
  watchdog_timestamp = time(NULL);

  /* watchdog_reader: create reader/receiver thread */
  pthread_t watchdog_reader_id;
  ret = pthread_create(&watchdog_reader_id, NULL, watchdog_reader, NULL);
  if (0 != ret) {
    (void)sir_emerg("FATAL: Failed to start real-time watchdog (watchdog_reader)!");
    (void)sir_emerg("Error #%d - %s", ret, xstrerror_l(ret));
    exit(EXIT_FAILURE);
  }

  /* watchdog_reader: set real-time priority */
  set_realtime_priority(watchdog_reader_id, max_priority);

  /* watchdog_reader: verify watchdog_reader is both real-time and max_priority */
  check_realtime_priority(watchdog_reader_id, max_priority);

#if !defined(TESTING_WATCHDOG)
  /* watchdog_writer: create writer/sender thread */
  pthread_t watchdog_writer_id;
  ret = pthread_create(&watchdog_writer_id, NULL, watchdog_writer, NULL);
  if (0 != ret) {
    (void)sir_emerg("FATAL: Failed to start watchdog (watchdog_writer)!");
    (void)sir_emerg("Error #%d - %s", ret, xstrerror_l(ret));
    exit(EXIT_FAILURE);
  }

  /* watchdog_writer: verify watchdog_writer is NOT max_priority */
  check_not_realtime_priority(watchdog_writer_id, max_priority);
#endif

  realtime_ok = true;
}
