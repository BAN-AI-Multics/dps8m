// 

#define tdbg

// Wrapper around pthreads

#include <pthread.h>

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





__thread extern uint thisCPUnum;
__thread extern uint thisIOMnum;
__thread extern uint thisChnNum;

// libuv resource lock

void lock_libuv (void);
void unlock_libuv (void);

// simh resource lock

void lock_simh (void);
void unlock_simh (void);

// CPU threads

struct cpuThreadz_t
  {
    cthread_t cpuThread;
    int cpuThreadArg;

    // run/stop switch
    bool run;
    cthread_cond_t runCond;
    cthread_mutex_t runLock;

    // DIS sleep
    bool sleep;
    cthread_cond_t sleepCond;
    cthread_mutex_t sleepLock;

  };
extern struct cpuThreadz_t cpuThreadz [N_CPU_UNITS_MAX];

void createCPUThread (uint cpuNum);
void setCPURun (uint cpuNum, bool run);
void cpuRunningWait (void);
void sleepCPU (unsigned long nsec);

// IOM threads

struct iomThreadz_t
  {
    cthread_t iomThread;
    int iomThreadArg;

    // interrupt wait
    bool intr;
    cthread_cond_t intrCond;
    cthread_mutex_t intrLock;

#ifdef tdbg
    // debugging
    int inCnt;
    int outCnt;
#endif
  };
extern struct iomThreadz_t iomThreadz [N_IOM_UNITS_MAX];

void createIOMThread (uint iomNum);
void iomInterruptWait (void);
void iomInterruptDone (void);
void setIOMInterrupt (uint iomNum);

// Channel threads

struct chnThreadz_t
  {
    bool started;

    cthread_t chnThread;
    int chnThreadArg;

    // connect wait
    bool connect;
    cthread_cond_t connectCond;
    cthread_mutex_t connectLock;

#ifdef tdbg
    // debugging
    int inCnt;
    int outCnt;
#endif
  };
extern struct chnThreadz_t chnThreadz [N_IOM_UNITS_MAX] [MAX_CHANNELS];

void createChnThread (uint iomNum, uint chnNum);
void chnConnectWait (void);
void chnConnectDone (void);
void setChnConnect (uint iomNum, uint chnNum);
void initThreadz (void);
