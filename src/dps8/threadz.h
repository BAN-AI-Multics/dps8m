/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 07828852-f630-11ec-a049-80ee73e9b8e7
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

#define tdbg

#define use_spinlock

// Wrapper around pthreads

#if defined(__APPLE__)
# include <mach/mach.h>
# include <mach/mach_time.h>
#endif /* if defined(__APPLE__) */

#include <pthread.h>

#if 0
typedef pthread_t cthread_t;
typedef pthread_mutex_t cthread_mutex_t;
typedef pthread_attr_t cthread_attr_t;
typedef pthread_mutexattr_t cthread_mutexattr_t;
typedef pthread_cond_t cthread_cond_t;
typedef pthread_condattr_t cthread_condattr_t;

static inline int cthread_cond_wait (cthread_cond_t * restrict cond,
                                     cthread_mutex_t * restrict mutex)
  {
    return pthread_cond_wait (cond, mutex);
  }

static inline int cthread_mutex_init (cthread_mutex_t * restrict mutex,
                                      const cthread_mutexattr_t * restrict attr)
  {
    return pthread_mutex_init (mutex, attr);
  }

static inline int cthread_cond_init (cthread_cond_t * restrict cond,
                                     const cthread_condattr_t * restrict attr)
  {
    return pthread_cond_init (cond, attr);
  }

static inline int cthread_mutex_lock (cthread_mutex_t * mutex)
  {
    return pthread_mutex_lock (mutex);
  }

static inline int cthread_mutex_trylock (cthread_mutex_t * mutex)
  {
    return pthread_mutex_trylock (mutex);
  }

static inline int cthread_mutex_unlock (cthread_mutex_t * mutex)
  {
    return pthread_mutex_unlock (mutex);
  }

static inline int cthread_create (cthread_t * thread, const cthread_attr_t * attr,
                          void * (* start_routine) (void *), void * arg)
  {
    return pthread_create (thread, attr, start_routine, arg);
  }

static inline int cthread_cond_signal (cthread_cond_t *cond)
  {
    return pthread_cond_signal (cond);
  }

static inline int cthread_cond_timedwait (pthread_cond_t * restrict cond,
                                          pthread_mutex_t * restrict mutex,
                                          const struct timespec * restrict abstime)
  {
    return pthread_cond_timedwait (cond, mutex, abstime);
  }
#endif

#if !defined(LOCKLESS)
extern pthread_rwlock_t mem_lock;
#endif /* if !defined(LOCKLESS) */

// local lock
void lock_ptr (pthread_mutex_t * lock);
void unlock_ptr (pthread_mutex_t * lock);

// libuv resource lock
void lock_libuv (void);
void unlock_libuv (void);
bool test_libuv_lock (void);

// resource lock
#if !defined(QUIET_UNUSED)
void lock_simh (void);
void unlock_simh (void);
#endif /* if !defined(QUIET_UNUSED) */

// atomic memory lock
#if !defined(LOCKLESS)
bool get_rmw_lock (void);
void lock_rmw (void);
void lock_mem_rd (void);
void lock_mem_wr (void);
void unlock_rmw (void);
void unlock_mem (void);
void unlock_mem_force (void);
#endif /* if !defined(LOCKLESS) */

// scu lock
void lock_scu (void);
void unlock_scu (void);

// synchronous clock lock
// void lockSync (void);
// void unlockSync (void);

// iom lock
void lock_iom (void);
void unlock_iom (void);

// testing lock
#if defined(TESTING)
void lock_tst (void);
void unlock_tst (void);
bool test_tst_lock (void);
#endif /* if defined(TESTING) */

// CPU threads
struct cpuThreadz_t
  {
    pthread_t cpuThread;
    int cpuThreadArg;

    // run/stop switch
    bool run;
    pthread_cond_t runCond;
    pthread_mutex_t runLock;

    // DIS sleep
#if defined (USE_MONOTONIC)
# if !defined(__APPLE__) && defined (CLOCK_MONOTONIC)
    clockid_t sleepClock;
    pthread_condattr_t sleepCondAttr;
# endif /* if !defined(__APPLE__) && defined (CLOCK_MONOTONIC) */
#endif /* if defined (USE_MONOTONIC) */
    pthread_cond_t sleepCond;
    pthread_mutex_t sleepLock;
    volAtomic bool sleeping;

  };
extern struct cpuThreadz_t cpuThreadz [N_CPU_UNITS_MAX];

void createCPUThread (uint cpuNum);
void stopCPUThread(void);
#if defined(THREADZ)
void cpuRunningWait (void);
#endif /* if defined(THREADZ) */
unsigned long sleepCPU (unsigned long usec);
void wakeCPU (uint cpuNum);

#if defined(IO_THREADZ)
// IOM threads

struct iomThreadz_t
  {
    pthread_t iomThread;
    int iomThreadArg;

    volAtomic bool ready;

    // interrupt wait
    bool intr;
    pthread_cond_t intrCond;
    pthread_mutex_t intrLock;

# if defined(tdbg)
    // debugging
    int inCnt;
    int outCnt;
# endif /* if defined(tdbg) */
  };
extern struct iomThreadz_t iomThreadz [N_IOM_UNITS_MAX];

void createIOMThread (uint iomNum);
void iomInterruptWait (void);
void iomInterruptDone (void);
void iomDoneWait (uint iomNum);
void setIOMInterrupt (uint iomNum);
void iomRdyWait (uint iomNum);

// Channel threads

struct chnThreadz_t
  {
    bool started;

    pthread_t chnThread;
    int chnThreadArg;

    // waiting at the gate
    volatile bool ready;

    // connect wait
    bool connect;
    pthread_cond_t connectCond;
    pthread_mutex_t connectLock;

# if defined(tdbg)
    // debugging
    int inCnt;
    int outCnt;
# endif /* if defined(tdbg) */
  };
extern struct chnThreadz_t chnThreadz [N_IOM_UNITS_MAX] [MAX_CHANNELS];

void createChnThread (uint iomNum, uint chnNum, const char * devTypeStr);
void chnConnectWait (void);
void chnConnectDone (void);
void setChnConnect (uint iomNum, uint chnNum);
void chnRdyWait (uint iomNum, uint chnNum);
#endif /* if defined(IO_THREADZ) */

void initThreadz (void);
void setSignals (void);

#if defined(IO_ASYNC_PAYLOAD_CHAN_THREAD)
extern pthread_cond_t iomCond;
extern pthread_mutex_t iom_start_lock;
#endif /* if defined(IO_ASYNC_PAYLOAD_CHAN_THREAD) */
