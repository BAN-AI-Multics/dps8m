/* sim_timer.h: simulator timer library headers

   Copyright (c) 1993-2008 Robert M Supnik
   Copyright (c) 2021 The DPS8M Development Team

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Robert M Supnik shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Robert M Supnik.
*/

#ifndef SIM_TIMER_H_
# define SIM_TIMER_H_   0

/* Pick up a struct timespec definition if it is available */
# include <time.h>
# if defined(__struct_timespec_defined)
#  define _TIMESPEC_DEFINED
# endif
# include <sys/types.h>
# include <pthread.h>

# if defined (__APPLE__)
#  define HAVE_STRUCT_TIMESPEC 1   /* OSX defined the structure but doesn't tell us */
# endif

# ifdef NEED_CLOCK_GETTIME
int clock_gettime(int clock_id, struct timespec *tp);
# endif /* ifdef NEED_CLOCK_GETTIME */

# define SIM_NTIMERS     8                           /* # timers */
# define SIM_TMAX        500                         /* max timer makeup */

# define SIM_INITIAL_IPS 500000                      /* uncalibrated assumption */
                                                    /* about instructions per second */

# define SIM_IDLE_CAL    10                          /* ms to calibrate */
# define SIM_IDLE_STMIN  2                           /* min sec for stability */
# define SIM_IDLE_STDFLT 20                          /* dft sec for stability */
# define SIM_IDLE_STMAX  600                         /* max sec for stability */

# define SIM_THROT_WINIT     1000                    /* cycles to skip */
# define SIM_THROT_WST       10000                   /* initial wait */
# define SIM_THROT_WMUL      4                       /* multiplier */
# define SIM_THROT_WMIN      100                     /* min wait */
# define SIM_THROT_DRIFT_PCT 5                       /* drift percentage for recalibrate */
# define SIM_THROT_MSMIN     10                      /* min for measurement */
# define SIM_THROT_NONE      0                       /* throttle parameters */
# define SIM_THROT_MCYC      1                       /* MegaCycles Per Sec */
# define SIM_THROT_KCYC      2                       /* KiloCycles Per Sec */
# define SIM_THROT_PCT       3                       /* Max Percent of host CPU */
# define SIM_THROT_SPC       4                       /* Specific periodic Delay */

# define TIMER_DBG_IDLE  0x001                       /* Debug Flag for Idle Debugging */
# define TIMER_DBG_QUEUE 0x002                       /* Debug Flag for Asynch Queue Debugging */
# define TIMER_DBG_MUX   0x004                       /* Debug Flag for Asynch Queue Debugging */

t_bool sim_timer_init (void);
void sim_timespec_diff (struct timespec *diff, const struct timespec *min, struct timespec *sub);
int32 sim_rtcn_init (int32 time, int32 tmr);
int32 sim_rtcn_init_unit (UNIT *uptr, int32 time, int32 tmr);
int sim_usleep(useconds_t tusleep);
void sim_rtcn_init_all (void);
int32 sim_rtcn_calb (int32 ticksper, int32 tmr);
t_stat sim_show_timers (FILE* st, DEVICE *dptr, UNIT* uptr, int32 val, CONST char* desc);
t_stat sim_show_clock_queues (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
void sim_throt_sched (void);
void sim_throt_cancel (void);
uint32 sim_os_msec (void);
void sim_os_sleep (unsigned int sec);
uint32 sim_os_ms_sleep (unsigned int msec);
uint32 sim_os_ms_sleep_init (void);
void sim_start_timer_services (void);
void sim_stop_timer_services (void);
t_stat sim_timer_change_asynch (void);
t_stat sim_timer_activate (UNIT *uptr, int32 interval);
t_stat sim_timer_activate_after (UNIT *uptr, uint32 usec_delay);
int32 sim_timer_activate_time (UNIT *uptr);
t_stat sim_register_clock_unit_tmr (UNIT *uptr, int32 tmr);
double sim_timer_inst_per_sec (void);
# define PRIORITY_BELOW_NORMAL  -1
# define PRIORITY_NORMAL         0
# define PRIORITY_ABOVE_NORMAL   1
t_stat sim_os_set_thread_priority (int below_normal_above);

extern t_bool sim_idle_enab;                        /* idle enabled flag */
extern volatile t_bool sim_idle_wait;               /* idle waiting flag */
extern t_bool sim_asynch_timer;
extern DEVICE sim_timer_dev;
extern UNIT * volatile sim_clock_cosched_queue[SIM_NTIMERS+1];
extern const t_bool rtc_avail;

#endif
