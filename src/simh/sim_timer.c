/*
 * sim_timer.c: simulator timer library
 *
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: MIT
 * scspell-id: de3abd63-f62a-11ec-a888-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 1993-2010 Robert M. Supnik
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Robert M. Supnik shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Robert M. Supnik.
 *
 * ---------------------------------------------------------------------------
 */

/*
 * This library includes the following routines:
 *
 * sim_timer_init -         initialize timing system
 * sim_os_msec  -           return elapsed time in msec
 * sim_os_sleep -           sleep specified number of seconds
 * sim_os_ms_sleep -        sleep specified number of milliseconds
 * sim_idle_ms_sleep -      sleep specified number of milliseconds
 *                          or until awakened by an asynchronous
 *                          event
 * sim_timespec_diff        subtract two timespec values
 * sim_timer_activate_after schedule unit for specific time
 */

#include "sim_defs.h"
#include <ctype.h>
#include <math.h>

#define SIM_INTERNAL_CLK (SIM_NTIMERS+(1<<30))
#define SIM_INTERNAL_UNIT sim_internal_timer_unit

#if defined(MIN)
# undef MIN
#endif /* if defined(MIN) */
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))

#if defined(MAX)
# undef MAX
#endif /* if defined(MAX) */
#define MAX(a,b)  (((a) > (b)) ? (a) : (b))

uint32 sim_idle_ms_sleep (unsigned int msec);

static int32 sim_calb_tmr           = -1;     /* the system calibrated timer */
static int32 sim_calb_tmr_last      = -1;     /* shadow value when at sim> prompt */
static double sim_inst_per_sec_last =  0;     /* shadow value when at sim> prompt */

static uint32 sim_idle_rate_ms                           = 0;
static uint32 sim_os_sleep_min_ms                        = 0;
static uint32 sim_os_sleep_inc_ms                        = 0;
static uint32 sim_os_clock_resoluton_ms                  = 0;
static uint32 sim_os_tick_hz                             = 0;
static uint32 sim_idle_calib_pct                         = 0;
static UNIT *sim_clock_unit[SIM_NTIMERS+1]               = {NULL};
UNIT   * volatile sim_clock_cosched_queue[SIM_NTIMERS+1] = {NULL};
static int32 sim_cosched_interval[SIM_NTIMERS+1];
static t_bool sim_catchup_ticks                          = FALSE;

#define sleep1Samples       10

static uint32 _compute_minimum_sleep (void)
{
uint32 i, tot, tim;

sim_os_set_thread_priority (PRIORITY_ABOVE_NORMAL);
sim_idle_ms_sleep (1);              /* Start sampling on a tick boundary */
for (i = 0, tot = 0; i < sleep1Samples; i++)
    tot += sim_idle_ms_sleep (1);
tim = tot / sleep1Samples;          /* Truncated average */
sim_os_sleep_min_ms = tim;
sim_idle_ms_sleep (1);              /* Start sampling on a tick boundary */
for (i = 0, tot = 0; i < sleep1Samples; i++)
    tot += sim_idle_ms_sleep (sim_os_sleep_min_ms + 1);
tim = tot / sleep1Samples;          /* Truncated average */
sim_os_sleep_inc_ms = tim - sim_os_sleep_min_ms;
sim_os_set_thread_priority (PRIORITY_NORMAL);
return sim_os_sleep_min_ms;
}

uint32 sim_idle_ms_sleep (unsigned int msec)
{
return sim_os_ms_sleep (msec);
}

#if defined(_WIN32)
/* On Windows there are several potentially disjoint threading APIs */
/* in use (base win32 pthreads, libSDL provided threading, and direct */
/* calls to beginthreadex), so go directly to the Win32 threading APIs */
/* to manage thread priority */
t_stat sim_os_set_thread_priority (int below_normal_above)
{
const static int val[3] = {THREAD_PRIORITY_BELOW_NORMAL, THREAD_PRIORITY_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL};

if ((below_normal_above < -1) || (below_normal_above > 1))
    return SCPE_ARG;
SetThreadPriority (GetCurrentThread(), val[1 + below_normal_above]);
return SCPE_OK;
}
#else
/* Native pthreads priority implementation */
t_stat sim_os_set_thread_priority (int below_normal_above)
{
int sched_policy, min_prio, max_prio;
struct sched_param sched_priority;

# if !defined(__gnu_hurd__)
if ((below_normal_above < -1) || (below_normal_above > 1))
    return SCPE_ARG;

pthread_getschedparam (pthread_self(), &sched_policy, &sched_priority);
#  if !defined(__PASE__)
min_prio = sched_get_priority_min(sched_policy);
max_prio = sched_get_priority_max(sched_policy);
#  else
min_prio = 1;
max_prio = 127;
#  endif /* if !defined(__PASE__) */
switch (below_normal_above) {
    case PRIORITY_BELOW_NORMAL:
        sched_priority.sched_priority = min_prio;
        break;
    case PRIORITY_NORMAL:
        sched_priority.sched_priority = (max_prio + min_prio) / 2;
        break;
    case PRIORITY_ABOVE_NORMAL:
        sched_priority.sched_priority = max_prio;
        break;
    }
pthread_setschedparam (pthread_self(), sched_policy, &sched_priority);
# endif /* if !defined(__gnu_hurd__) */
return SCPE_OK;
}
#endif

/* OS-dependent timer and clock routines */

#if defined (_WIN32)

/* Win32 routines */

const t_bool rtc_avail = TRUE;

uint32 sim_os_msec (void)
{
return timeGetTime ();
}

void sim_os_sleep (unsigned int sec)
{
Sleep (sec * 1000);
return;
}

void sim_timer_exit (void)
{
timeEndPeriod (sim_idle_rate_ms);
return;
}

uint32 sim_os_ms_sleep_init (void)
{
TIMECAPS timers;

if (timeGetDevCaps (&timers, sizeof (timers)) != TIMERR_NOERROR)
    return 0;
if (timers.wPeriodMin == 0)
    return 0;
if (timeBeginPeriod (timers.wPeriodMin) != TIMERR_NOERROR)
    return 0;
atexit (sim_timer_exit);
/* return measured actual minimum sleep time */
return _compute_minimum_sleep ();
}

uint32 sim_os_ms_sleep (unsigned int msec)
{
uint32 stime = sim_os_msec();

Sleep (msec);
return sim_os_msec () - stime;
}

#else

/* UNIX routines */

# include <time.h>
# include <sys/time.h>
# include <signal.h>
# include <unistd.h>
# define NANOS_PER_MILLI     1000000
# define MILLIS_PER_SEC      1000

const t_bool rtc_avail = TRUE;

uint32 sim_os_msec (void)
{
struct timeval cur;
struct timezone foo;
int st1ret;
uint32 msec;

st1ret = gettimeofday (&cur, &foo);
  if (st1ret != 0)
    {
      fprintf (stderr, "\rFATAL: gettimeofday failure! Aborting at %s[%s:%d]\r\n",
               __func__, __FILE__, __LINE__);
# if defined(USE_BACKTRACE)
#  if defined(SIGUSR2)
      (void)raise(SIGUSR2);
      /*NOTREACHED*/ /* unreachable */
#  endif /* if defined(SIGUSR2) */
# endif /* if defined(USE_BACKTRACE) */
      abort();
    }
msec = (((uint32) cur.tv_sec) * 1000UL) + (((uint32) cur.tv_usec) / 1000UL);
return msec;
}

void sim_os_sleep (unsigned int sec)
{
sleep (sec);
return;
}

uint32 sim_os_ms_sleep_init (void)
{
return _compute_minimum_sleep ();
}

uint32 sim_os_ms_sleep (unsigned int milliseconds)
{
uint32 stime = sim_os_msec ();
struct timespec treq;

treq.tv_sec  = milliseconds / MILLIS_PER_SEC;
treq.tv_nsec = (milliseconds % MILLIS_PER_SEC) * NANOS_PER_MILLI;
(void) nanosleep (&treq, NULL);
return sim_os_msec () - stime;
}

#endif

/* diff = min - sub */
void
sim_timespec_diff (struct timespec *diff, const struct timespec *min, struct timespec *sub)
{
/* move the minuend value to the difference and operate there. */
*diff = *min;
/* Borrow as needed for the nsec value */
while (sub->tv_nsec > diff->tv_nsec) {
    --diff->tv_sec;
    diff->tv_nsec += 1000000000L;
    }
diff->tv_nsec -= sub->tv_nsec;
diff->tv_sec -= sub->tv_sec;
/* Normalize the result */
while (diff->tv_nsec > 1000000000L) {
    ++diff->tv_sec;
    diff->tv_nsec -= 1000000000L;
    }
}

/* Forward declarations */

static double _timespec_to_double              (struct timespec *time);
static void   _double_to_timespec              (struct timespec *time, double dtime);
static void   _rtcn_configure_calibrated_clock (int32  newtmr);
static void   _sim_coschedule_cancel(UNIT      *uptr);

/* OS independent clock calibration package */

static int32  rtc_ticks[SIM_NTIMERS+1]                   = { 0 }; /* ticks */
static uint32 rtc_hz[SIM_NTIMERS+1]                      = { 0 }; /* tick rate */
static uint32 rtc_rtime[SIM_NTIMERS+1]                   = { 0 }; /* real time */
static uint32 rtc_vtime[SIM_NTIMERS+1]                   = { 0 }; /* virtual time */
static double rtc_gtime[SIM_NTIMERS+1]                   = { 0 }; /* instruction time */
static uint32 rtc_nxintv[SIM_NTIMERS+1]                  = { 0 }; /* next interval */
static int32  rtc_based[SIM_NTIMERS+1]                   = { 0 }; /* base delay */
static int32  rtc_currd[SIM_NTIMERS+1]                   = { 0 }; /* current delay */
static int32  rtc_initd[SIM_NTIMERS+1]                   = { 0 }; /* initial delay */
static uint32 rtc_elapsed[SIM_NTIMERS+1]                 = { 0 }; /* sec since init */
static uint32 rtc_calibrations[SIM_NTIMERS+1]            = { 0 }; /* calibration count */
static double rtc_clock_skew_max[SIM_NTIMERS+1]          = { 0 }; /* asynchronous max skew */
static double rtc_clock_start_gtime[SIM_NTIMERS+1]       = { 0 }; /* reference instruction time for clock */
static double rtc_clock_tick_size[SIM_NTIMERS+1]         = { 0 }; /* 1/hz */
static uint32 rtc_calib_initializations[SIM_NTIMERS+1]   = { 0 }; /* Initialization Count */
static double rtc_calib_tick_time[SIM_NTIMERS+1]         = { 0 }; /* ticks time */
static double rtc_calib_tick_time_tot[SIM_NTIMERS+1]     = { 0 }; /* ticks time - total*/
static uint32 rtc_calib_ticks_acked[SIM_NTIMERS+1]       = { 0 }; /* ticks Acked */
static uint32 rtc_calib_ticks_acked_tot[SIM_NTIMERS+1]   = { 0 }; /* ticks Acked - total */
static uint32 rtc_clock_ticks[SIM_NTIMERS+1]             = { 0 }; /* ticks delivered since catchup base */
static uint32 rtc_clock_ticks_tot[SIM_NTIMERS+1]         = { 0 }; /* ticks delivered since catchup base - total */
static double rtc_clock_catchup_base_time[SIM_NTIMERS+1] = { 0 }; /* reference time for catchup ticks */
static uint32 rtc_clock_catchup_ticks[SIM_NTIMERS+1]     = { 0 }; /* Record of catchups */
static uint32 rtc_clock_catchup_ticks_tot[SIM_NTIMERS+1] = { 0 }; /* Record of catchups - total */
static t_bool rtc_clock_catchup_pending[SIM_NTIMERS+1]   = { 0 }; /* clock tick catchup pending */
static t_bool rtc_clock_catchup_eligible[SIM_NTIMERS+1]  = { 0 }; /* clock tick catchup eligible */
static uint32 rtc_clock_time_idled[SIM_NTIMERS+1]        = { 0 }; /* total time idled */
static uint32 rtc_clock_calib_skip_idle[SIM_NTIMERS+1]   = { 0 }; /* Calibrations skipped due to idling */
static uint32 rtc_clock_calib_gap2big[SIM_NTIMERS+1]     = { 0 }; /* Calibrations skipped Gap Too Big */
static uint32 rtc_clock_calib_backwards[SIM_NTIMERS+1]   = { 0 }; /* Calibrations skipped Clock Running Backwards */

UNIT sim_timer_units[SIM_NTIMERS+1];                     /* one for each timer and one for an */
                                                         /* internal clock if no clocks are registered */
UNIT sim_internal_timer_unit;                            /* Internal calibration timer */
UNIT sim_throttle_unit;                                  /* one for throttle */

t_stat sim_throt_svc (UNIT *uptr);
t_stat sim_timer_tick_svc (UNIT *uptr);

#define DBG_TRC       0x008                 /* tracing */
#define DBG_CAL       0x010                 /* calibration activities */
#define DBG_TIM       0x020                 /* timer thread activities */
#define DBG_ACK       0x080                 /* interrupt acknowledgement activities */
DEBTAB sim_timer_debug[] = {
  {"TRACE", DBG_TRC, "Trace routine calls"},
  {"IACK",  DBG_ACK, "interrupt acknowledgement activities"},
  {"CALIB", DBG_CAL, "Calibration activities"},
  {"TIME",  DBG_TIM, "Activation and scheduling activities"},
  {0}
};

/* Forward device declarations */
extern DEVICE sim_timer_dev;
extern DEVICE sim_throttle_dev;

void sim_rtcn_init_all (void)
{
int32 tmr;

for (tmr = 0; tmr <= SIM_NTIMERS; tmr++)
    if (rtc_initd[tmr] != 0)
        sim_rtcn_init (rtc_initd[tmr], tmr);
return;
}

int32 sim_rtcn_init (int32 time, int32 tmr)
{
return sim_rtcn_init_unit (NULL, time, tmr);
}

int32 sim_rtcn_init_unit (UNIT *uptr, int32 time, int32 tmr)
{
if (time == 0)
    time = 1;
if (tmr == SIM_INTERNAL_CLK)
    tmr = SIM_NTIMERS;
else {
    if ((tmr < 0) || (tmr >= SIM_NTIMERS))
        return time;
    }
/*
 * If we'd previously succeeded in calibrating a tick value, then use that
 * delay as a better default to setup when we're re-initialized.
 * Re-initializing happens on any boot or after any breakpoint/continue.
 */
if (rtc_currd[tmr])
    time = rtc_currd[tmr];
if (!uptr)
    uptr = sim_clock_unit[tmr];
sim_debug (DBG_CAL, &sim_timer_dev, "_sim_rtcn_init_unit(unit=%s, time=%d, tmr=%d)\n", sim_uname(uptr), time, tmr);
if (uptr) {
    if (!sim_clock_unit[tmr])
        sim_register_clock_unit_tmr (uptr, tmr);
    }
rtc_clock_start_gtime[tmr]        = sim_gtime();
rtc_rtime[tmr]                    = sim_os_msec ();
rtc_vtime[tmr]                    = rtc_rtime[tmr];
rtc_nxintv[tmr]                   = 1000;
rtc_ticks[tmr]                    = 0;
rtc_hz[tmr]                       = 0;
rtc_based[tmr]                    = time;
rtc_currd[tmr]                    = time;
rtc_initd[tmr]                    = time;
rtc_elapsed[tmr]                  = 0;
rtc_calibrations[tmr]             = 0;
rtc_clock_ticks_tot[tmr]         += rtc_clock_ticks[tmr];
rtc_clock_ticks[tmr]              = 0;
rtc_calib_tick_time_tot[tmr]     += rtc_calib_tick_time[tmr];
rtc_calib_tick_time[tmr]          = 0;
rtc_clock_catchup_pending[tmr]    = FALSE;
rtc_clock_catchup_eligible[tmr]   = FALSE;
rtc_clock_catchup_ticks_tot[tmr] += rtc_clock_catchup_ticks[tmr];
rtc_clock_catchup_ticks[tmr]      = 0;
rtc_calib_ticks_acked_tot[tmr]   += rtc_calib_ticks_acked[tmr];
rtc_calib_ticks_acked[tmr]        = 0;
++rtc_calib_initializations[tmr];
_rtcn_configure_calibrated_clock (tmr);
return time;
}

int32 sim_rtcn_calb (uint32 ticksper, int32 tmr)
{
if (tmr == SIM_INTERNAL_CLK)
    tmr = SIM_NTIMERS;
else {
    if ((tmr < 0) || (tmr >= SIM_NTIMERS))
        return 10000;
    }
if (rtc_hz[tmr] != ticksper) {                          /* changing tick rate? */
    rtc_hz[tmr] = ticksper;
    rtc_clock_tick_size[tmr] = 1.0/ticksper;
    _rtcn_configure_calibrated_clock (tmr);
    rtc_currd[tmr] = (int32)(sim_timer_inst_per_sec()/ticksper);
    }
if (sim_clock_unit[tmr] == NULL) {                      /* Not using TIMER units? */
    rtc_clock_ticks[tmr] += 1;
    rtc_calib_tick_time[tmr] += rtc_clock_tick_size[tmr];
    }
if (rtc_clock_catchup_pending[tmr]) {                   /* catchup tick? */
    ++rtc_clock_catchup_ticks[tmr];                     /* accumulating which were catchups */
    rtc_clock_catchup_pending[tmr] = FALSE;
    }
return rtc_currd[tmr];                                  /* return now avoiding counting catchup tick in calibration */
}

/* sim_timer_init - get minimum sleep time available on this host */

t_bool sim_timer_init (void)
{
int tmr;
uint32 clock_start, clock_last, clock_now;

sim_debug (DBG_TRC, &sim_timer_dev, "sim_timer_init()\n");
for (tmr=0; tmr<=SIM_NTIMERS; tmr++) {
    sim_timer_units[tmr].action = &sim_timer_tick_svc;
    sim_timer_units[tmr].flags  = UNIT_DIS | UNIT_IDLE;
    }
SIM_INTERNAL_UNIT.flags = UNIT_DIS | UNIT_IDLE;
sim_register_internal_device (&sim_timer_dev);
sim_register_clock_unit_tmr (&SIM_INTERNAL_UNIT, SIM_INTERNAL_CLK);
sim_idle_rate_ms = sim_os_ms_sleep_init ();             /* get OS timer rate */

clock_last = clock_start = sim_os_msec ();
sim_os_clock_resoluton_ms = 1000;
do {
    uint32 clock_diff;

    clock_now = sim_os_msec ();
    clock_diff = clock_now - clock_last;
    if ((clock_diff > 0) && (clock_diff < sim_os_clock_resoluton_ms))
        sim_os_clock_resoluton_ms = clock_diff;
    clock_last = clock_now;
    } while (clock_now < clock_start + 100);
sim_os_tick_hz = 1000/(sim_os_clock_resoluton_ms * (sim_idle_rate_ms/sim_os_clock_resoluton_ms));
return (sim_idle_rate_ms != 0);
}

/* sim_show_timers - show running timer information */
t_stat sim_show_timers (FILE* st, DEVICE *dptr, UNIT* uptr, int32 val, CONST char* desc)
{
int tmr, clocks;
struct timespec now;
time_t time_t_now;
int32 calb_tmr = (sim_calb_tmr == -1) ? sim_calb_tmr_last : sim_calb_tmr;

for (tmr=clocks=0; tmr<=SIM_NTIMERS; ++tmr) {
    if (0 == rtc_initd[tmr])
        continue;

    if (sim_clock_unit[tmr]) {
        ++clocks;
        fprintf (st, "%s clock device is %s%s%s\n",
                 sim_name,
                 (tmr == SIM_NTIMERS) ? "Internal Calibrated Timer(" : "",
                 sim_uname(sim_clock_unit[tmr]),
                 (tmr == SIM_NTIMERS) ? ")" : "");
        }

    fprintf (st, "%s%sTimer %d:\n", "",
             rtc_hz[tmr] ? "Calibrated " : "Uncalibrated ",
             tmr);
    if (rtc_hz[tmr]) {
        fprintf (st, "  Running at:                %lu Hz\n",
                 (unsigned long)rtc_hz[tmr]);
        fprintf (st, "  Tick Size:                 %s\n",
                 sim_fmt_secs (rtc_clock_tick_size[tmr]));
        fprintf (st, "  Ticks in current second:   %lu\n",
                 (unsigned long)rtc_ticks[tmr]);
        }
    fprintf (st, "  Seconds Running:           %lu (%s)\n",
             (unsigned long)rtc_elapsed[tmr],
             sim_fmt_secs ((double)rtc_elapsed[tmr]));
    if (tmr == calb_tmr) {
        fprintf (st, "  Calibration Opportunities: %lu\n",
                 (unsigned long)rtc_calibrations[tmr]);
        if (sim_idle_calib_pct)
            fprintf (st, "  Calib Skip Idle Thresh %%:  %lu\n",
                     (unsigned long)sim_idle_calib_pct);
        if (rtc_clock_calib_skip_idle[tmr])
            fprintf (st, "  Calibs Skip While Idle:    %lu\n",
                     (unsigned long)rtc_clock_calib_skip_idle[tmr]);
        if (rtc_clock_calib_backwards[tmr])
            fprintf (st, "  Calibs Skip Backwards:     %lu\n",
                     (unsigned long)rtc_clock_calib_backwards[tmr]);
        if (rtc_clock_calib_gap2big[tmr])
            fprintf (st, "  Calibs Skip Gap Too Big:   %lu\n",
                     (unsigned long)rtc_clock_calib_gap2big[tmr]);
        }
    if (rtc_gtime[tmr])
        fprintf (st, "  Instruction Time:          %.0f\n",
                 rtc_gtime[tmr]);
    fprintf (st, "  Current Insts Per Tick:    %lu\n",
             (unsigned long)rtc_currd[tmr]);
    fprintf (st, "  Initializations:           %lu\n",
             (unsigned long)rtc_calib_initializations[tmr]);
    fprintf (st, "  Total Ticks:               %lu\n",
             (unsigned long)rtc_clock_ticks_tot[tmr]+(unsigned long)rtc_clock_ticks[tmr]);
    if (rtc_clock_skew_max[tmr] != 0.0)
        fprintf (st, "  Peak Clock Skew:           %s%s\n",
                 sim_fmt_secs (fabs(rtc_clock_skew_max[tmr])),
                 (rtc_clock_skew_max[tmr] < 0) ? " fast" : " slow");
    if (rtc_calib_ticks_acked[tmr])
        fprintf (st, "  Ticks Acked:               %lu\n",
                 (unsigned long)rtc_calib_ticks_acked[tmr]);
    if (rtc_calib_ticks_acked_tot[tmr]+rtc_calib_ticks_acked[tmr] != rtc_calib_ticks_acked[tmr]) //-V584
        fprintf (st, "  Total Ticks Acked:         %lu\n",
                 (unsigned long)rtc_calib_ticks_acked_tot[tmr]+(unsigned long)rtc_calib_ticks_acked[tmr]);
    if (rtc_calib_tick_time[tmr])
        fprintf (st, "  Tick Time:                 %s\n",
                 sim_fmt_secs (rtc_calib_tick_time[tmr]));
    if (rtc_calib_tick_time_tot[tmr]+rtc_calib_tick_time[tmr] != rtc_calib_tick_time[tmr])
        fprintf (st, "  Total Tick Time:           %s\n",
                 sim_fmt_secs (rtc_calib_tick_time_tot[tmr]+rtc_calib_tick_time[tmr]));
    if (rtc_clock_catchup_ticks[tmr])
        fprintf (st, "  Catchup Ticks Sched:       %lu\n",
                 (unsigned long)rtc_clock_catchup_ticks[tmr]);
    if (rtc_clock_catchup_ticks_tot[tmr]+rtc_clock_catchup_ticks[tmr] != rtc_clock_catchup_ticks[tmr]) //-V584
        fprintf (st, "  Total Catchup Ticks Sched: %lu\n",
                 (unsigned long)rtc_clock_catchup_ticks_tot[tmr]+(unsigned long)rtc_clock_catchup_ticks[tmr]);
    clock_gettime (CLOCK_REALTIME, &now);
    time_t_now = (time_t)now.tv_sec;
    fprintf (st, "  Wall Clock Time Now:       %8.8s.%03d\n", 11+ctime(&time_t_now), (int)(now.tv_nsec/1000000));
    if (rtc_clock_catchup_eligible[tmr]) {
        _double_to_timespec (&now, rtc_clock_catchup_base_time[tmr]+rtc_calib_tick_time[tmr]);
        time_t_now = (time_t)now.tv_sec;
        fprintf (st, "  Catchup Tick Time:         %8.8s.%03d\n", 11+ctime(&time_t_now), (int)(now.tv_nsec/1000000));
        _double_to_timespec (&now, rtc_clock_catchup_base_time[tmr]);
        time_t_now = (time_t)now.tv_sec;
        fprintf (st, "  Catchup Base Time:         %8.8s.%03d\n", 11+ctime(&time_t_now), (int)(now.tv_nsec/1000000));
        }
    if (rtc_clock_time_idled[tmr])
        fprintf (st, "  Total Time Idled:          %s\n",   sim_fmt_secs (rtc_clock_time_idled[tmr]/1000.0));
    }
if (clocks == 0)
    fprintf (st, "%s clock device is not specified, co-scheduling is unavailable\n", sim_name);
return SCPE_OK;
}

t_stat sim_show_clock_queues (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
int tmr;

for (tmr=0; tmr<=SIM_NTIMERS; ++tmr) {
    if (sim_clock_unit[tmr] == NULL)
        continue;
    if (sim_clock_cosched_queue[tmr] != QUEUE_LIST_END) {
        int32 accum;

        fprintf (st, "%s clock (%s) co-schedule event queue status\n",
                 sim_name, sim_uname(sim_clock_unit[tmr]));
        accum = 0;
        for (uptr = sim_clock_cosched_queue[tmr]; uptr != QUEUE_LIST_END; uptr = uptr->next) { //-V763
            if ((dptr = find_dev_from_unit (uptr)) != NULL) { //-V763
                fprintf (st, "  %s", sim_dname (dptr));
                if (dptr->numunits > 1)
                    fprintf (st, " unit %d", (int32) (uptr - dptr->units));
                }
            else
                fprintf (st, "  Unknown");
            if (accum > 0)
                fprintf (st, " after %d ticks", accum);
            fprintf (st, "\n");
            accum = accum + uptr->time;
            }
        }
    }
return SCPE_OK;
}

REG sim_timer_reg[] = {
    { NULL }
    };

/* Clear catchup */

t_stat sim_timer_clr_catchup (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
if (sim_catchup_ticks)
    sim_catchup_ticks = FALSE;
return SCPE_OK;
}

t_stat sim_timer_set_catchup (UNIT *uptr, int32 val, CONST char *cptr, void *desc)
{
if (!sim_catchup_ticks)
    sim_catchup_ticks = TRUE;
return SCPE_OK;
}

t_stat sim_timer_show_catchup (FILE *st, UNIT *uptr, int32 val, CONST void *desc)
{
fprintf (st, "Calibrated Ticks%s", sim_catchup_ticks ? " with Catchup Ticks" : "");
return SCPE_OK;
}

MTAB sim_timer_mod[] = {
  { MTAB_VDV, MTAB_VDV, "CATCHUP", "CATCHUP", \
      &sim_timer_set_catchup, &sim_timer_show_catchup, NULL, "Enables/Displays Clock Tick catchup mode" },
  { MTAB_VDV, 0, NULL, "NOCATCHUP", \
      &sim_timer_clr_catchup, NULL, NULL, "Disables Clock Tick catchup mode" },
  { 0 },
};

static t_stat sim_timer_clock_reset (DEVICE *dptr);

DEVICE sim_timer_dev = {
    "TIMER", sim_timer_units, sim_timer_reg, sim_timer_mod,
    SIM_NTIMERS+1, 0, 0, 0, 0, 0,
    NULL, NULL, &sim_timer_clock_reset, NULL, NULL, NULL,
    NULL, DEV_DEBUG | DEV_NOSAVE, 0, sim_timer_debug};

/* Clock assist activities */
t_stat sim_timer_tick_svc (UNIT *uptr)
{
int tmr = (int)(uptr-sim_timer_units);
t_stat stat;

rtc_clock_ticks[tmr] += 1;
rtc_calib_tick_time[tmr] += rtc_clock_tick_size[tmr];
/*
 * Some devices may depend on executing during the same instruction or
 * immediately after the clock tick event.  To satisfy this, we directly
 * run the clock event here and if it completes successfully, schedule any
 * currently coschedule units to run now.  Ticks should never return a
 * non-success status, while co-schedule activities might, so they are
 * queued to run from sim_process_event
 */
if (sim_clock_unit[tmr]->action == NULL)
    return SCPE_IERR;
stat = sim_clock_unit[tmr]->action (sim_clock_unit[tmr]);
--sim_cosched_interval[tmr];                    /* Countdown ticks */
if (stat == SCPE_OK) {
    if (rtc_clock_catchup_eligible[tmr]) {      /* calibration started? */
        struct timespec now;
        double skew;

        clock_gettime(CLOCK_REALTIME, &now);
        skew = (_timespec_to_double(&now) - (rtc_calib_tick_time[tmr]+rtc_clock_catchup_base_time[tmr]));

        if (fabs(skew) > fabs(rtc_clock_skew_max[tmr]))
            rtc_clock_skew_max[tmr]   = skew;
        }
    while ((sim_clock_cosched_queue[tmr] != QUEUE_LIST_END) &&
           (sim_cosched_interval[tmr] < sim_clock_cosched_queue[tmr]->time)) {
        UNIT *cptr                    = sim_clock_cosched_queue[tmr];
        sim_clock_cosched_queue[tmr]  = cptr->next;
        cptr->next                    = NULL;
        cptr->cancel                  = NULL;
        _sim_activate (cptr, 0);
        }
    if (sim_clock_cosched_queue[tmr] != QUEUE_LIST_END)
        sim_cosched_interval[tmr]     = sim_clock_cosched_queue[tmr]->time;
    else
        sim_cosched_interval[tmr]     = 0;
    }
sim_timer_activate_after (uptr, 1000000/rtc_hz[tmr]);
return stat;
}

#if !defined(__CYGWIN__) && \
  ( defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || \
    defined(CROSS_MINGW32) || defined(CROSS_MINGW64) )
void win32_usleep(__int64 usec)
{
  HANDLE timer;
  LARGE_INTEGER ft;

  ft.QuadPart = -(10*usec);

  timer = CreateWaitableTimer(NULL, TRUE, NULL);
  SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
  WaitForSingleObject(timer, INFINITE);
  CloseHandle(timer);
}
#endif /* if !defined(__CYGWIN__) &&
            ( defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__) ||
             defined(CROSS_MINGW32) || defined(CROSS_MINGW64) ) */

int
sim_usleep(useconds_t tusleep)
{
#if ( !defined(__APPLE__) && !defined(__OpenBSD__) )
# if !defined(__CYGWIN__) && \
  ( defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__) || \
    defined(CROSS_MINGW32) || defined(CROSS_MINGW64) )
  win32_usleep(tusleep);

  return 0;
# else
#  if !defined(__PASE__)
  struct timespec rqt;
  rqt.tv_sec  = tusleep / 1000000L;
  rqt.tv_nsec = (tusleep % 1000000L) * 1000L;

  return clock_nanosleep(CLOCK_MONOTONIC, 0, &rqt, NULL);
#  else
  return usleep(tusleep);
#  endif /* if !defined(__PASE__) */
# endif /* if !defined(__CYGWIN__) &&
            ( defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__) ||
             defined(CROSS_MINGW32) || defined(CROSS_MINGW64) ) */
#else
# if defined(__APPLE__)
  struct timespec rqt;
  rqt.tv_sec  = tusleep / 1000000L;
  rqt.tv_nsec = (tusleep % 1000000L) * 1000L;
  return nanosleep(&rqt, NULL);
# else
  return usleep(tusleep);
# endif /* if defined(__APPLE__) */
#endif /* if ( !defined(__APPLE__) && !defined(__OpenBSD__) ) */
}

static double _timespec_to_double (struct timespec *time)
{
return ((double)time->tv_sec)+(double)(time->tv_nsec)/1000000000.0;
}

static void _double_to_timespec (struct timespec *time, double dtime)
{
time->tv_sec  = (time_t)floor(dtime);
time->tv_nsec = (long)((dtime-floor(dtime))*1000000000.0);
}

#define CLK_TPS 10
#define CLK_INIT (SIM_INITIAL_IPS/CLK_TPS)
static int32 sim_int_clk_tps;

static t_stat sim_timer_clock_tick_svc (UNIT *uptr)
{
sim_rtcn_calb (sim_int_clk_tps, SIM_INTERNAL_CLK);
sim_activate_after (uptr, 1000000/sim_int_clk_tps);     /* reactivate unit */
return SCPE_OK;
}

static void _rtcn_configure_calibrated_clock (int32 newtmr)
{
int32 tmr;

/* Look for a timer running slower than the host system clock */
sim_int_clk_tps = MIN(CLK_TPS, sim_os_tick_hz);
for (tmr=0; tmr<SIM_NTIMERS; tmr++) {
    if ((rtc_hz[tmr]) &&
        (rtc_hz[tmr] <= (uint32)sim_os_tick_hz))
        break;
    }
if (tmr == SIM_NTIMERS) {                   /* None found? */
    if ((tmr != newtmr) && (!sim_is_active (&SIM_INTERNAL_UNIT))) {
        /* Start the internal timer */
        sim_calb_tmr = SIM_NTIMERS;
        sim_debug (DBG_CAL, &sim_timer_dev,
                   "_rtcn_configure_calibrated_clock() - Starting Internal Calibrated Timer at %dHz\n",
                   sim_int_clk_tps);
        SIM_INTERNAL_UNIT.action = &sim_timer_clock_tick_svc;
        SIM_INTERNAL_UNIT.flags = UNIT_DIS | UNIT_IDLE;
        sim_activate_abs (&SIM_INTERNAL_UNIT, 0);
        sim_rtcn_init_unit (&SIM_INTERNAL_UNIT, (CLK_INIT*CLK_TPS)/sim_int_clk_tps, SIM_INTERNAL_CLK);
        }
    return;
    }
if ((tmr == newtmr) &&
    (sim_calb_tmr == newtmr))               /* already set? */
    return;
if (sim_calb_tmr == SIM_NTIMERS) {      /* was old the internal timer? */
    sim_debug (DBG_CAL, &sim_timer_dev,
               "_rtcn_configure_calibrated_clock() - Stopping Internal Calibrated Timer, New Timer = %d (%dHz)\n",
               tmr, rtc_hz[tmr]);
    rtc_initd[SIM_NTIMERS] = 0;
    rtc_hz[SIM_NTIMERS] = 0;
    sim_cancel (&SIM_INTERNAL_UNIT);
    /* Migrate any coscheduled devices to the standard queue and they will requeue themselves */
    while (sim_clock_cosched_queue[SIM_NTIMERS] != QUEUE_LIST_END) {
        UNIT *uptr = sim_clock_cosched_queue[SIM_NTIMERS];
        _sim_coschedule_cancel (uptr);
        _sim_activate (uptr, 1);
        }
    }
else {
    sim_debug (DBG_CAL, &sim_timer_dev,
               "_rtcn_configure_calibrated_clock() - Changing Calibrated Timer from %d (%dHz) to %d (%dHz)\n",
               sim_calb_tmr, rtc_hz[sim_calb_tmr], tmr, rtc_hz[tmr]);
    sim_calb_tmr = tmr;
    }
sim_calb_tmr = tmr;
}

static t_stat sim_timer_clock_reset (DEVICE *dptr)
{
sim_debug (DBG_TRC, &sim_timer_dev, "sim_timer_clock_reset()\n");
_rtcn_configure_calibrated_clock (sim_calb_tmr);
if (sim_switches & SWMASK ('P')) {
    sim_cancel (&SIM_INTERNAL_UNIT);
    sim_calb_tmr = -1;
    }
return SCPE_OK;
}

void sim_start_timer_services (void)
{
sim_debug (DBG_TRC, &sim_timer_dev, "sim_start_timer_services()\n");
_rtcn_configure_calibrated_clock (sim_calb_tmr);
}

void sim_stop_timer_services (void)
{
int tmr;

sim_debug (DBG_TRC, &sim_timer_dev, "sim_stop_timer_services()\n");

for (tmr=0; tmr<=SIM_NTIMERS; tmr++) {
    int32 accum;

    if (sim_clock_unit[tmr]) {
        /* Stop clock assist unit and make sure the clock unit has a tick queued */
        sim_cancel (&sim_timer_units[tmr]);
        if (rtc_hz[tmr])
            sim_activate (sim_clock_unit[tmr], rtc_currd[tmr]);
        /* Move coscheduled units to the standard event queue */
        accum = 1;
        while (sim_clock_cosched_queue[tmr] != QUEUE_LIST_END) {
            UNIT *cptr = sim_clock_cosched_queue[tmr];

            sim_clock_cosched_queue[tmr] = cptr->next;
            cptr->next                   = NULL;
            cptr->cancel                 = NULL;

            accum += cptr->time;
            _sim_activate (cptr, accum*rtc_currd[tmr]);
            }
        }
    }
sim_cancel (&SIM_INTERNAL_UNIT);                    /* Make sure Internal Timer is stopped */
sim_calb_tmr_last     = sim_calb_tmr;                   /* Save calibrated timer value for display */
sim_inst_per_sec_last = sim_timer_inst_per_sec ();  /* Save execution rate for display */
sim_calb_tmr          = -1;
}

/* Instruction Execution rate. */

double sim_timer_inst_per_sec (void)
{
double inst_per_sec = SIM_INITIAL_IPS;

if (sim_calb_tmr == -1)
    return inst_per_sec;
inst_per_sec = ((double)rtc_currd[sim_calb_tmr])*rtc_hz[sim_calb_tmr];
if (0 == inst_per_sec)
    inst_per_sec = ((double)rtc_currd[sim_calb_tmr])*sim_int_clk_tps;
return inst_per_sec;
}

t_stat sim_timer_activate (UNIT *uptr, int32 interval)
{
return sim_timer_activate_after (uptr, (uint32)((interval * 1000000.0) / sim_timer_inst_per_sec ()));
}

t_stat sim_timer_activate_after (UNIT *uptr, uint32 usec_delay)
{
int inst_delay, tmr;
double inst_delay_d, inst_per_sec;

/* If this is a clock unit, we need to schedule the related timer unit instead */
for (tmr=0; tmr<=SIM_NTIMERS; tmr++)
    if (sim_clock_unit[tmr] == uptr) {
        uptr = &sim_timer_units[tmr];
        break;
        }
if (sim_is_active (uptr))                               /* already active? */
    return SCPE_OK;
inst_per_sec = sim_timer_inst_per_sec ();
inst_delay_d = ((inst_per_sec*usec_delay)/1000000.0);
/* Bound delay to avoid overflow.  */
/* Long delays are usually canceled before they expire */
if (inst_delay_d > (double)0x7fffffff)
    inst_delay_d = (double)0x7fffffff;
inst_delay = (int32)inst_delay_d;
if ((inst_delay == 0) && (usec_delay != 0))
    inst_delay = 1;     /* Minimum non-zero delay is 1 instruction */
sim_debug (DBG_TIM, &sim_timer_dev, "sim_timer_activate_after() - queue addition %s at %d (%d usecs)\n",
           sim_uname(uptr), inst_delay, usec_delay);
return _sim_activate (uptr, inst_delay);                /* queue it now */
}

t_stat sim_register_clock_unit_tmr (UNIT *uptr, int32 tmr)
{
if (tmr == SIM_INTERNAL_CLK)
    tmr = SIM_NTIMERS;
else {
    if ((tmr < 0) || (tmr >= SIM_NTIMERS))
        return SCPE_IERR;
    }
if (NULL == uptr) {                         /* deregistering? */
    while (sim_clock_cosched_queue[tmr] != QUEUE_LIST_END) {
        UNIT *uptr = sim_clock_cosched_queue[tmr];

        _sim_coschedule_cancel (uptr);
        _sim_activate (uptr, 1);
        }
    sim_clock_unit[tmr] = NULL;
    return SCPE_OK;
    }
if (NULL == sim_clock_unit[tmr])
    sim_clock_cosched_queue[tmr] = QUEUE_LIST_END;
sim_clock_unit[tmr] = uptr;
uptr->dynflags |= UNIT_TMR_UNIT;
sim_timer_units[tmr].flags = UNIT_DIS | (sim_clock_unit[tmr] ? UNIT_IDLE : 0); //-V547
return SCPE_OK;
}

/* Cancel a unit on the coschedule queue */
static void _sim_coschedule_cancel (UNIT *uptr)
{
if (uptr->next) {                           /* On a queue? */
    int tmr;

    for (tmr=0; tmr<SIM_NTIMERS; tmr++) {
        if (uptr == sim_clock_cosched_queue[tmr]) {
            sim_clock_cosched_queue[tmr] = uptr->next;
            uptr->next = NULL;
            }
        else {
            UNIT *cptr;
            for (cptr = sim_clock_cosched_queue[tmr];
                (cptr != QUEUE_LIST_END);
                cptr = cptr->next)
                if (cptr->next == (uptr)) {
                    cptr->next = (uptr)->next;
                    uptr->next = NULL;
                    break;
                    }
            }
        if (uptr->next == NULL) {           /* found? */
            uptr->cancel = NULL;
            sim_debug (SIM_DBG_EVENT, &sim_timer_dev, "Canceled Clock Coscheduled Event for %s\n", sim_uname(uptr));
            return;
            }
        }
    }
}

