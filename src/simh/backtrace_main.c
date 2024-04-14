/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: BSD-2-Clause
 * scspell-id: 14a58b2b-3c43-11ed-a845-80ee73e9b8e7
 *
 * -------------------------------------------------------------------------
 *
 * backtrace_main.c
 *
 * libbacktrace (https://github.com/ianlancetaylor/libbacktrace) start-up.
 *
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2021-2023 Jeffrey H. Johnson
 * Copyright (c) 2022-2024 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * -------------------------------------------------------------------------
 */

#if defined(USE_BACKTRACE)
# if defined(_INC_BACKTRACE_FUNC)
#  if !defined(_INC_BACKTRACE_MAIN)
#   define _INC_BACKTRACE_MAIN
#   if !defined(BACKTRACE_SKIP)
#    define BACKTRACE_SKIP 1
#   endif /* if !defined(BACKTRACE_SKIP) */
#   include <signal.h>
#   if defined(SIGSEGV)
  (void)signal(SIGSEGV, backtrace_handler);
#   endif /* if defined(SIGSEGV) */
#   if defined(SIGILL)
  (void)signal(SIGILL, backtrace_handler);
#   endif /* if defined(SIGILL) */
#   if defined(SIGFPE)
  (void)signal(SIGFPE, backtrace_handler);
#   endif /* if defined(SIGFPE) */
#   if defined(SIGBUS)
  (void)signal(SIGBUS, backtrace_handler);
#   endif /* if defined(SIGBUS) */
#   if defined(SIGUNUSED)
  (void)signal(SIGUNUSED, backtrace_handler);
#   endif /* if defined(SIGUNUSED) */
#   if defined(SIGSYS)
  (void)signal(SIGSYS, backtrace_handler);
#   endif /* if defined(SIGSYS) */
#   if defined(SIGUSR2)
  (void)signal(SIGUSR2, backtrace_handler);
#   endif /* if defined(SIGUSR2) */
#   if defined(SIGSTKFLT)
  (void)signal(SIGSTKFLT, backtrace_handler);
#   endif /* if defined(SIGSTKFLT) */
#   if defined(SIGPOWER)
#    if defined(SIG_IGN)
  (void)signal(SIGPOWER, SIG_IGN);
#    endif /* if defined(SIG_IGN) */
#   endif /* if defined(SIGPOWER) */
#   if defined(SIGPWR)
#    if defined(SIG_IGN)
  (void)signal(SIGPWR, SIG_IGN);
#    endif /* if defined(SIG_IGN) */
#   endif /* if defined(SIGPWR) */
state = backtrace_create_state(
  NULL, BACKTRACE_SUPPORTS_THREADS, error_callback, NULL);
#  endif /* if !defined(_INC_BACKTRACE_MAIN) */
# endif /* if defined(_INC_BACKTRACE_FUNC) */
#endif /* if defined(USE_BACKTRACE) */
