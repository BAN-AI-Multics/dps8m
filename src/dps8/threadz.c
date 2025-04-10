/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: fe96c073-f62f-11ec-bf37-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2013-2019 Charles Anthony
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

//
// Thread Wrappers
//

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include "dps8.h"
#include "dps8_sys.h"
#include "dps8_cpu.h"
#include "dps8_faults.h"
#include "dps8_iom.h"
#include "dps8_utils.h"

#include "threadz.h"
#if defined(__FreeBSD__) || defined(__OpenBSD__)
# include <pthread_np.h>
#endif /* FreeBSD || OpenBSD */

//
// Resource locks
//

// scp library serializer

#if defined(IO_ASYNC_PAYLOAD_CHAN_THREAD)
pthread_cond_t iomCond;
pthread_mutex_t iom_start_lock;
#endif

#if !defined(QUIET_UNUSED)
void lock_simh (void)
  {
    pthread_mutex_lock (& simh_lock);
  }

void unlock_simh (void)
  {
    pthread_mutex_unlock (& simh_lock);
  }
#endif

// libuv library serializer

static pthread_mutex_t libuv_lock;

void lock_libuv (void)
  {
    pthread_mutex_lock (& libuv_lock);
  }

void unlock_libuv (void)
  {
    pthread_mutex_unlock (& libuv_lock);
  }

#if defined(TESTING)
bool test_libuv_lock (void)
  {
    //sim_debug (DBG_TRACE, & cpu_dev, "test_libuv_lock\r\n");
    int rc;
    rc = pthread_mutex_trylock (& libuv_lock);
    if (rc)
      {
         // couldn't lock; presumably already
         return true;
      }
    // lock acquired, it wasn't locked
    rc = pthread_mutex_unlock (& libuv_lock);
    if (rc)
      sim_printf ("test_libuv_lock pthread_mutex_lock libuv_lock %d\r\n", rc);
    return false;
  }
#endif

// Memory serializer

//   addrmods RMW lock
//     Read()
//     Write()
//
//   CPU -- reset locks
//
//   core_read/core_write locks
//
//   read_operand/write operand RMW lock
//     Read()
//     Write()
//
//  IOM

// mem_lock -- memory atomicity lock
// rmw_lock -- big R/M/W cycle lock

#if !defined(LOCKLESS)
pthread_rwlock_t mem_lock = PTHREAD_RWLOCK_INITIALIZER;
static __thread bool have_mem_lock = false;
static __thread bool have_rmw_lock = false;

bool get_rmw_lock (void)
  {
    return have_rmw_lock;
  }

void lock_rmw (void)
  {
    if (have_rmw_lock)
      {
        sim_warn ("%s: Already have RMW lock\r\n", __func__);
        return;
      }
    if (have_mem_lock)
      {
        sim_warn ("%s: Already have memory lock\r\n", __func__);
        return;
      }
    int rc= pthread_rwlock_wrlock (& mem_lock);
    if (rc)
      sim_printf ("%s pthread_rwlock_rdlock mem_lock %d\r\n", __func__, rc);
    have_mem_lock = true;
    have_rmw_lock = true;
  }

void lock_mem_rd (void)
  {
    // If the big RMW lock is on, do nothing.
    if (have_rmw_lock)
      return;

    if (have_mem_lock)
      {
        sim_warn ("%s: Already have memory lock\r\n", __func__);
        return;
      }
    int rc= pthread_rwlock_rdlock (& mem_lock);
    if (rc)
      sim_printf ("%s pthread_rwlock_rdlock mem_lock %d\r\n", __func__, rc);
    have_mem_lock = true;
  }

void lock_mem_wr (void)
  {
    // If the big RMW lock is on, do nothing.
    if (have_rmw_lock)
      return;

    if (have_mem_lock)
      {
        sim_warn ("%s: Already have memory lock\r\n", __func__);
        return;
      }
    int rc= pthread_rwlock_wrlock (& mem_lock);
    if (rc)
      sim_printf ("%s pthread_rwlock_wrlock mem_lock %d\r\n", __func__, rc);
    have_mem_lock = true;
  }

void unlock_rmw (void)
  {
    if (! have_mem_lock)
      {
        sim_warn ("%s: Don't have memory lock\r\n", __func__);
        return;
      }
    if (! have_rmw_lock)
      {
        sim_warn ("%s: Don't have RMW lock\r\n", __func__);
        return;
      }

    int rc = pthread_rwlock_unlock (& mem_lock);
    if (rc)
      sim_printf ("%s pthread_rwlock_ublock mem_lock %d\r\n", __func__, rc);
    have_mem_lock = false;
    have_rmw_lock = false;
  }

void unlock_mem (void)
  {
    if (have_rmw_lock)
      return;
    if (! have_mem_lock)
      {
        sim_warn ("%s: Don't have memory lock\r\n", __func__);
        return;
      }

    int rc = pthread_rwlock_unlock (& mem_lock);
    if (rc)
      sim_printf ("%s pthread_rwlock_ublock mem_lock %d\r\n", __func__, rc);
    have_mem_lock = false;
  }

void unlock_mem_force (void)
  {
    if (have_mem_lock)
      {
        int rc = pthread_rwlock_unlock (& mem_lock);
        if (rc)
          sim_printf ("%s pthread_rwlock_unlock mem_lock %d\r\n", __func__, rc);
      }
    have_mem_lock = false;
    have_rmw_lock = false;
  }
#endif

// local serializer

void lock_ptr (pthread_mutex_t * lock)
  {
    int rc;
    rc = pthread_mutex_lock (lock);
    if (rc)
      sim_printf ("lock_ptr %d\r\n", rc);
  }

void unlock_ptr (pthread_mutex_t * lock)
  {
    //sim_debug (DBG_TRACE, & cpu_dev, "unlock_scu\r\n");
    int rc;
    rc = pthread_mutex_unlock (lock);
    if (rc)
      sim_printf ("unlock_ptr %d\r\n", rc);
  }

// SCU serializer

static pthread_mutex_t scu_lock;

void lock_scu (void)
  {
    //sim_debug (DBG_TRACE, & cpu_dev, "lock_scu\r\n");
    int rc;
    rc = pthread_mutex_lock (& scu_lock);
    if (rc)
      sim_printf ("lock_scu pthread_spin_lock scu %d\r\n", rc);
  }

void unlock_scu (void)
  {
    //sim_debug (DBG_TRACE, & cpu_dev, "unlock_scu\r\n");
    int rc;
    rc = pthread_mutex_unlock (& scu_lock);
    if (rc)
      sim_printf ("unlock_scu pthread_spin_lock scu %d\r\n", rc);
  }
// synchronous clock serializer

// static pthread_mutex_t syncLock;

// void lockSync (void)
//   {
//     int rc;
//     rc = pthread_mutex_lock (& syncLock);
//     if (rc)
//       sim_printf ("lockSync pthread_spin_lock syncLock %d\r\n", rc);
//   }

// void unlockSync (void)
//   {
//     int rc;
//     rc = pthread_mutex_unlock (& syncLock);
//     if (rc)
//       sim_printf ("unlockSync pthread_spin_lock syncLock %d\r\n", rc);
//   }

// IOM serializer

static pthread_mutex_t iom_lock;

void lock_iom (void)
  {
    int rc;
    rc = pthread_mutex_lock (& iom_lock);
    if (rc)
      sim_printf ("%s pthread_spin_lock iom %d\r\n", __func__, rc);
  }

void unlock_iom (void)
  {
    int rc;
    rc = pthread_mutex_unlock (& iom_lock);
    if (rc)
      sim_printf ("%s pthread_spin_lock iom %d\r\n", __func__, rc);
  }

// Debugging tool

#if defined(TESTING)
static pthread_mutex_t tst_lock = PTHREAD_MUTEX_INITIALIZER;

void lock_tst (void)
  {
    //sim_debug (DBG_TRACE, & cpu_dev, "lock_tst\r\n");
    int rc;
    rc = pthread_mutex_lock (& tst_lock);
    if (rc)
      sim_printf ("lock_tst pthread_mutex_lock tst_lock %d\r\n", rc);
  }

void unlock_tst (void)
  {
    //sim_debug (DBG_TRACE, & cpu_dev, "unlock_tst\r\n");
    int rc;
    rc = pthread_mutex_unlock (& tst_lock);
    if (rc)
      sim_printf ("unlock_tst pthread_mutex_lock tst_lock %d\r\n", rc);
  }

// assertion

bool test_tst_lock (void)
  {
    //sim_debug (DBG_TRACE, & cpu_dev, "test_tst_lock\r\n");
    int rc;
    rc = pthread_mutex_trylock (& tst_lock);
    if (rc)
      {
         // couldn't lock; presumably already
         return true;
      }
    // lock acquired, it wasn't locked
    rc = pthread_mutex_unlock (& tst_lock);
    if (rc)
      sim_printf ("test_tst_lock pthread_mutex_lock tst_lock %d\r\n", rc);
    return false;
  }
#endif /* if defined(TESTING) */

////////////////////////////////////////////////////////////////////////////////
//
// CPU threads
//
////////////////////////////////////////////////////////////////////////////////

//
// main thread
//   createCPUThread
//
// CPU thread
//   cpuRunningWait
//   sleepCPU
//
// SCU thread
//   wakeCPU
//
// cpuThread:
//
//    while (1)
//      {
//        cpuRunningWait
//        compute...
//        dis:  sleepCPU
//      }

struct cpuThreadz_t cpuThreadz [N_CPU_UNITS_MAX];

// Create a thread with Mach CPU policy

#if defined(__APPLE__)
int pthread_create_with_cpu_policy(
        pthread_t *restrict thread,
        int policy_group,
        const pthread_attr_t *restrict attr,
        void *(*start_routine)(void *), void *restrict arg)
{
# if defined(TESTING)
  sim_msg ("\rAffinity policy group %d requested for thread.\r\n", policy_group);
# endif /* if defined(TESTING) */
  thread_affinity_policy_data_t policy_data = { policy_group };
  int rv = pthread_create_suspended_np(thread, attr, start_routine, arg);
  mach_port_t mach_thread = pthread_mach_thread_np(*thread);
  if (rv != 0)
    {
      return rv;
    }
  thread_policy_set(
          mach_thread,
          THREAD_AFFINITY_POLICY,
          (thread_policy_t)&policy_data,
          THREAD_AFFINITY_POLICY_COUNT);
  thread_resume(mach_thread);
  return 0;
}
#endif /* if defined(__APPLE__) */

// Create CPU thread

void createCPUThread (uint cpuNum)
  {
    int rc;
    struct cpuThreadz_t * p = & cpuThreadz[cpuNum];
    if (p->run)
      return;
    cpu_reset_unit_idx (cpuNum, false);
    p->cpuThreadArg = (int) cpuNum;
    // initialize run/stop switch

#if defined(__FreeBSD__) || defined(__OpenBSD__) || (defined(__linux__) && defined(__GLIBC__))
    pthread_mutexattr_t sleep_attr;
    pthread_mutexattr_init(&sleep_attr);
# if !defined(__OpenBSD__)
    pthread_mutexattr_settype(&sleep_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
# endif
    rc = pthread_mutex_init (& p->sleepLock, &sleep_attr);
#else
    rc = pthread_mutex_init (& p->sleepLock, NULL);
#endif /* FreeBSD || OpenBSD || Linux+GLIBC */
    if (rc)
      sim_printf ("createCPUThread pthread_mutex_init sleepLock %d\r\n", rc);

    rc = pthread_mutex_init (& p->runLock, NULL);
    if (rc)
      sim_printf ("createCPUThread pthread_mutex_init runLock %d\r\n", rc);

    rc = pthread_cond_init (& p->runCond, NULL);
    if (rc)
      sim_printf ("createCPUThread pthread_cond_init runCond %d\r\n", rc);

    p->run = true;
    p->sleeping = false;

    // initialize DIS sleep
#if defined(USE_MONOTONIC)
# if defined(__APPLE__) || !defined(CLOCK_MONOTONIC)
    p->sleepClock = CLOCK_REALTIME;
    rc = pthread_cond_init (& p->sleepCond, NULL);
# else
    rc = pthread_condattr_init (& p->sleepCondAttr);
    if (rc)
      sim_printf ("createCPUThread pthread_condattr_init sleepCond %d\r\n", rc);

    rc = pthread_condattr_setclock (& p->sleepCondAttr, CLOCK_MONOTONIC);
    if (rc) {
      //sim_printf ("createCPUThread pthread_condattr_setclock  sleepCond %d\r\n", rc);
      p->sleepClock = CLOCK_REALTIME;
    } else {
      p->sleepClock = CLOCK_MONOTONIC;
    }
# endif // ! APPLE
    rc = pthread_cond_init (& p->sleepCond, & p->sleepCondAttr);
#else
    rc = pthread_cond_init (& p->sleepCond, NULL);
#endif
    if (rc)
      sim_printf ("createCPUThread pthread_cond_init sleepCond %d\r\n", rc);

#if defined(__APPLE__)
    rc = pthread_create_with_cpu_policy(
            & p->cpuThread,
            cpuNum,
            NULL,
            cpu_thread_main,
            & p->cpuThreadArg);
#else
    rc = pthread_create(
            & p->cpuThread,
            NULL,
            cpu_thread_main,
            & p->cpuThreadArg);
#endif /* if defined(__APPLE__) */
    if (rc)
      sim_printf ("createCPUThread pthread_create %d\r\n", rc);

#if defined(AFFINITY)
    if (cpus[cpuNum].set_affinity)
      {
        cpu_set_t cpuset;
        CPU_ZERO (& cpuset);
        CPU_SET (cpus[cpuNum].affinity, & cpuset);
        int s = pthread_setaffinity_np (p->cpuThread, sizeof (cpu_set_t), & cpuset);
        if (s)
          sim_printf ("pthread_setaffinity_np %u on CPU %u returned %d\r\n",
                      cpus[cpuNum].affinity, cpuNum, s);
      }
#endif /* if defined(AFFINITY) */
  }

void stopCPUThread(void)
  {
    struct cpuThreadz_t * p = & cpuThreadz[current_running_cpu_idx];
    p->run = false;
    pthread_exit(NULL);
  }

// Called by CPU thread to block on run/sleep

#if defined(THREADZ)
void cpuRunningWait (void)
  {
    int rc;
    struct cpuThreadz_t * p = & cpuThreadz[current_running_cpu_idx];
    if (p->run)
      return;
    rc = pthread_mutex_lock (& p->runLock);
    if (rc)
      sim_printf ("cpuRunningWait pthread_mutex_lock %d\r\n", rc);
    while (! p->run)
      {
        rc = pthread_cond_wait (& p->runCond, & p->runLock);
        if (rc)
          sim_printf ("cpuRunningWait pthread_cond_wait %d\r\n", rc);
     }
    rc = pthread_mutex_unlock (& p->runLock);
    if (rc)
      sim_printf ("cpuRunningWait pthread_mutex_unlock %d\r\n", rc);
  }
#endif

// Called by CPU thread to sleep until time up or signaled
// Return time left
unsigned long  sleepCPU (unsigned long usec) {
  int rc;
  struct cpuThreadz_t * p = & cpuThreadz[current_running_cpu_idx];
  struct timespec startTime, absTime;

#if defined(USE_MONOTONIC)
  clock_gettime (p->sleepClock, & startTime);
#else
  clock_gettime (CLOCK_REALTIME, & startTime);
#endif /* if defined(USE_MONOTONIC) */
  absTime = startTime;
  int64_t nsec = ((int64_t) usec) * 1000L + (int64_t)startTime.tv_nsec;
  absTime.tv_nsec = nsec % 1000000000L;
  absTime.tv_sec += nsec / 1000000000L;

  rc = pthread_mutex_lock (& p->sleepLock);
  if (rc)
    sim_printf ("sleepCPU pthread_mutex_lock sleepLock %d\r\n", rc);

  p->sleeping = true;
  rc = pthread_cond_timedwait (& p->sleepCond, & p->sleepLock, & absTime);
  p->sleeping = false; //-V519

  int rc2 = pthread_mutex_unlock (& p->sleepLock);
  if (rc2)
    sim_printf ("sleepCPU pthread_mutex_unlock sleepLock %d\r\n", rc2);

  if (rc == ETIMEDOUT) {
    return 0;
  }

  if (rc) {
    cpu_state_t * cpup = _cpup;
    sim_printf ("sleepCPU pthread_cond_timedwait rc %ld  usec %ld TR %lu CPU %lu\r\n",
                (long) rc, (long) usec, (unsigned long) cpu.rTR,
                (unsigned long) current_running_cpu_idx);
  }

  struct timespec newTime;
  struct timespec delta;
#if defined(USE_MONOTONIC)
  clock_gettime (p->sleepClock, & newTime);
#else
  clock_gettime (CLOCK_REALTIME, & newTime);
#endif /* if defined(USE_MONOTONIC) */
  timespec_diff (& absTime, & newTime, & delta);

  if (delta.tv_nsec < 0)
    return 0; // safety
  return (unsigned long) delta.tv_nsec / 1000L;
}

// Called to wake sleeping CPU; such as interrupt during DIS

void wakeCPU (uint cpuNum)
  {
    int rc;
    struct cpuThreadz_t * p = & cpuThreadz[cpuNum];

    rc = pthread_mutex_lock (& p->sleepLock);
    if (rc)
      sim_printf ("sleepCPU pthread_mutex_lock sleepLock %d\r\n", rc);

    if (p->sleeping) {
      rc = pthread_cond_signal (& p->sleepCond);
      if (rc)
        sim_printf ("wakeCPU pthread_cond_signal %d\r\n", rc);
    }

    int rc2 = pthread_mutex_unlock (& p->sleepLock);
    if (rc2)
      sim_printf ("sleepCPU pthread_mutex_unlock sleepLock %d\r\n", rc2);
  }

#if defined(IO_THREADZ)
////////////////////////////////////////////////////////////////////////////////
//
// IOM threads
//
////////////////////////////////////////////////////////////////////////////////

// main thread
//   createIOMThread    create thread
//   iomRdyWait         wait for IOM started
//   setIOMInterrupt    signal IOM to start
//   iomDoneWait        wait for IOM done
// IOM thread
//   iomInterruptWait   IOM thread wait for work
//   iomInterruptDone   IOM thread signal done working
//
//   IOM thread
//     while (1)
//       {
//         iomInterruptWake
//         work...
//         iomInterruptDone
//      }

struct iomThreadz_t iomThreadz [N_IOM_UNITS_MAX];

// Create IOM thread

void createIOMThread (uint iomNum)
  {
    int rc;
    struct iomThreadz_t * p = & iomThreadz[iomNum];
# if defined(tdbg)
    p->inCnt = 0;
    p->outCnt = 0;
# endif
    p->iomThreadArg = (int) iomNum;

    p->ready = false;
    // initialize interrupt wait
    p->intr = false;
    rc = pthread_mutex_init (& p->intrLock, NULL);
    if (rc)
      sim_printf ("createIOMThread pthread_mutex_init intrLock %d\r\n", rc);
    rc = pthread_cond_init (& p->intrCond, NULL);
    if (rc)
      sim_printf ("createIOMThread pthread_cond_init intrCond %d\r\n", rc);

# if defined(__APPLE__)
    rc = pthread_create_with_cpu_policy(
            & p->iomThread,
            iomNum,
            NULL,
            iom_thread_main,
            & p->iomThreadArg);
# else
    rc = pthread_create(
            & p->iomThread,
            NULL,
            iom_thread_main,
            & p->iomThreadArg);
# endif /* if defined(__APPLE__) */
    if (rc)
      sim_printf ("createIOMThread pthread_create %d\r\n", rc);
  }

// Called by IOM thread to block until CIOC call

void iomInterruptWait (void)
  {
    int rc;
    struct iomThreadz_t * p = & iomThreadz[this_iom_idx];
    rc = pthread_mutex_lock (& p->intrLock);
    if (rc)
      sim_printf ("iomInterruptWait pthread_mutex_lock %d\r\n", rc);
    p -> ready = true;
    while (! p->intr)
      {
        rc = pthread_cond_wait (& p->intrCond, & p->intrLock);
        if (rc)
          sim_printf ("iomInterruptWait pthread_cond_wait %d\r\n", rc);
      }
# if defined(tdbg)
    p->outCnt++;
    if (p->inCnt != p->outCnt)
      sim_printf ("iom thread %d in %d out %d\r\n", this_iom_idx,
                  p->inCnt, p->outCnt);
# endif
  }

// Called by IOM thread to signal CIOC complete

void iomInterruptDone (void)
  {
    int rc;
    struct iomThreadz_t * p = & iomThreadz[this_iom_idx];
    p->intr = false;
    rc = pthread_cond_signal (& p->intrCond);
    if (rc)
      sim_printf ("iomInterruptDone pthread_cond_signal %d\r\n", rc);
    rc = pthread_mutex_unlock (& p->intrLock);
    if (rc)
      sim_printf ("iomInterruptDone pthread_mutex_unlock %d\r\n", rc);
  }

// Called by CPU thread to wait for iomInterruptDone

void iomDoneWait (uint iomNum)
  {
    int rc;
    struct iomThreadz_t * p = & iomThreadz[iomNum];
    rc = pthread_mutex_lock (& p->intrLock);
    if (rc)
      sim_printf ("iomDoneWait pthread_mutex_lock %d\r\n", rc);
    while (p->intr)
      {
        rc = pthread_cond_wait (& p->intrCond, & p->intrLock);
        if (rc)
          sim_printf ("iomDoneWait pthread_cond_wait %d\r\n", rc);
      }
    rc = pthread_mutex_unlock (& p->intrLock);
    if (rc)
      sim_printf ("iomDoneWait pthread_mutex_unlock %d\r\n", rc);
  }

// Signal CIOC to IOM thread

void setIOMInterrupt (uint iomNum)
  {
    int rc;
    struct iomThreadz_t * p = & iomThreadz[iomNum];
    rc = pthread_mutex_lock (& p->intrLock);
    if (rc)
      sim_printf ("setIOMInterrupt pthread_mutex_lock %d\r\n", rc);
    while (p->intr)
      {
        rc = pthread_cond_wait(&p->intrCond, &p->intrLock);
        if (rc)
          sim_printf ("setIOMInterrupt pthread_cond_wait intrLock %d\r\n", rc);
      }
# if defined(tdbg)
    p->inCnt++;
# endif
    p->intr = true;
    rc = pthread_cond_signal (& p->intrCond);
    if (rc)
      sim_printf ("setIOMInterrupt pthread_cond_signal %d\r\n", rc);
    rc = pthread_mutex_unlock (& p->intrLock);
    if (rc)
      sim_printf ("setIOMInterrupt pthread_mutex_unlock %d\r\n", rc);
  }

// Wait for IOM thread to initialize

void iomRdyWait (uint iomNum)
  {
    struct iomThreadz_t * p = & iomThreadz[iomNum];
    while (! p -> ready)
      sim_usleep (10000);
   }

////////////////////////////////////////////////////////////////////////////////
//
// Channel threads
//
////////////////////////////////////////////////////////////////////////////////

// main thread
//   createChnThread    create thread
// IOM thread
//   chnRdyWait         wait for channel started
//   setChnConnect      signal channel to start
// Channel thread
//   chnConnectWait     Channel thread wait for work
//   chnConnectDone     Channel thread signal done working
//
//   IOM thread
//     while (1)
//       {
//         iomInterruptWake
//         work...
//         iomInterruptDone
//      }
struct chnThreadz_t chnThreadz [N_IOM_UNITS_MAX] [MAX_CHANNELS];

// Create channel thread

void createChnThread (uint iomNum, uint chnNum, const char * devTypeStr)
  {
    int rc;
    struct chnThreadz_t * p = & chnThreadz[iomNum][chnNum];
    p->chnThreadArg = (int) (chnNum + iomNum * MAX_CHANNELS);

# if defined(tdbg)
    p->inCnt = 0;
    p->outCnt = 0;
# endif
    p->ready = false;
    // initialize interrupt wait
    p->connect = false;
    rc = pthread_mutex_init (& p->connectLock, NULL);
    if (rc)
      sim_printf ("createChnThread pthread_mutex_init connectLock %d\r\n", rc);
    rc = pthread_cond_init (& p->connectCond, NULL);
    if (rc)
      sim_printf ("createChnThread pthread_cond_init connectCond %d\r\n", rc);

# if defined(__APPLE__)
    rc = pthread_create_with_cpu_policy(
            & p->chnThread,
            iomNum
            NULL,
            chan_thread_main,
            & p->chnThreadArg);
# else
    rc = pthread_create(
            & p->chnThread,
            NULL,
            chan_thread_main,
            & p->chnThreadArg);
# endif /* if defined(__APPLE__) */
    if (rc)
      sim_printf ("createChnThread pthread_create %d\r\n", rc);
  }

// Called by channel thread to block until I/O command presented

void chnConnectWait (void)
  {
    int rc;
    struct chnThreadz_t * p = & chnThreadz[this_iom_idx][this_chan_num];

    rc = pthread_mutex_lock (& p->connectLock);
    if (rc)
      sim_printf ("chnConnectWait pthread_mutex_lock %d\r\n", rc);
    p -> ready = true;
    while (! p->connect)
      {
        rc = pthread_cond_wait (& p->connectCond, & p->connectLock);
        if (rc)
          sim_printf ("chnConnectWait pthread_cond_wait %d\r\n", rc);
      }
# if defined(tdbg)
    p->outCnt++;
    if (p->inCnt != p->outCnt)
      sim_printf ("chn thread %d in %d out %d\r\n", this_chan_num,
                  p->inCnt, p->outCnt);
# endif
  }

// Called by channel thread to signal I/O complete

void chnConnectDone (void)
  {
    int rc;
    struct chnThreadz_t * p = & chnThreadz[this_iom_idx][this_chan_num];
    p->connect = false;
    rc = pthread_cond_signal (& p->connectCond);
    if (rc)
      sim_printf ("chnInterruptDone pthread_cond_signal %d\r\n", rc);
    rc = pthread_mutex_unlock (& p->connectLock);
    if (rc)
      sim_printf ("chnConnectDone pthread_mutex_unlock %d\r\n", rc);
  }

// Signal I/O presented to channel thread

void setChnConnect (uint iomNum, uint chnNum)
  {
    int rc;
    struct chnThreadz_t * p = & chnThreadz[iomNum][chnNum];
    rc = pthread_mutex_lock (& p->connectLock);
    if (rc)
      sim_printf ("setChnConnect pthread_mutex_lock %d\r\n", rc);
    while (p->connect)
      {
        rc = pthread_cond_wait(&p->connectCond, &p->connectLock);
        if (rc)
          sim_printf ("setChnInterrupt pthread_cond_wait connectLock %d\r\n", rc);
      }
# if defined(tdbg)
    p->inCnt++;
# endif
    p->connect = true;
    rc = pthread_cond_signal (& p->connectCond);
    if (rc)
      sim_printf ("setChnConnect pthread_cond_signal %d\r\n", rc);
    rc = pthread_mutex_unlock (& p->connectLock);
    if (rc)
      sim_printf ("setChnConnect pthread_mutex_unlock %d\r\n", rc);
  }

// Block until channel thread ready

void chnRdyWait (uint iomNum, uint chnNum)
  {
    struct chnThreadz_t * p = & chnThreadz[iomNum][chnNum];
    while (! p -> ready)
      sim_usleep (10000);
   }
#endif

void initThreadz (void)
  {
#if defined(IO_THREADZ)
    // chnThreadz is sparse; make sure 'started' is false
    (void)memset (chnThreadz, 0, sizeof (chnThreadz));
#endif

#if !defined(LOCKLESS)
    //pthread_rwlock_init (& mem_lock, PTHREAD_PROCESS_PRIVATE);
    have_mem_lock = false;
    have_rmw_lock = false;
#endif
#if defined(__FreeBSD__) || defined(__OpenBSD__) || (defined(__linux__) && defined(__GLIBC__))
    pthread_mutexattr_t scu_attr;
    pthread_mutexattr_init(&scu_attr);
# if !defined(__OpenBSD__)
    pthread_mutexattr_settype(&scu_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
# endif
    pthread_mutex_init (& scu_lock, &scu_attr);
#else
    pthread_mutex_init (& scu_lock, NULL);
#endif /* FreeBSD || OpenBSD || Linux+GLIBC */
    pthread_mutexattr_t iom_attr;
    pthread_mutexattr_init(& iom_attr);
    pthread_mutexattr_settype(& iom_attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init (& iom_lock, & iom_attr);

    pthread_mutexattr_t libuv_attr;
    pthread_mutexattr_init(& libuv_attr);
    pthread_mutexattr_settype(& libuv_attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init (& libuv_lock, & libuv_attr);

#if defined(IO_ASYNC_PAYLOAD_CHAN_THREAD)
    pthread_cond_init (& iomCond, NULL);
    pthread_mutex_init (& iom_start_lock, NULL);
#endif
  }

// Set up per-thread signal handlers

void int_handler (int signal);

void setSignals (void)
  {
#if !defined(__MINGW64__) && !defined(__MINGW32__)
    struct sigaction act;
    (void)memset (& act, 0, sizeof (act));
    act.sa_handler = int_handler;
    act.sa_flags = 0;
    sigaction (SIGINT, & act, NULL);
    sigaction (SIGTERM, & act, NULL);
#endif /* if !defined(__MINGW64__) && !defined(__MINGW32__) */
  }

#if 0
// Force cache coherency

static pthread_mutex_t fenceLock = PTHREAD_MUTEX_INITIALIZER;
void fence (void)
  {
    pthread_mutex_lock (& fenceLock);
    pthread_mutex_unlock (& fenceLock);
  }
#endif
