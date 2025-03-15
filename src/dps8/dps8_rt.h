/*
 * vim: filetype=c:tabstop=2:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 4b8c831e-fd8d-11ef-94b1-80ee73e9b8e7
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

#if !defined(INCLUDED_DPS8_RT_H)
# define INCLUDED_DPS8_RT_H

# include <pthread.h>
# include <stdbool.h>

extern volatile time_t watchdog_timestamp;
extern volatile bool realtime_ok;

int restore_thread_sched(const pthread_t thread_id);
void save_thread_sched(const pthread_t thread_id);
void watchdog_recover(void);
int realtime_max_priority(void);
void set_realtime_priority(const pthread_t thread_id, const int priority);
void check_realtime_priority(const pthread_t thread_id, const int priority);
void check_not_realtime_priority(const pthread_t thread_id, const int priority);
void watchdog_startup(void);

#endif
