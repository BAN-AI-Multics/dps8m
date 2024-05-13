/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: BSD-2-Clause
 * scspell-id: 9722a7c6-3c33-11ed-9dc5-80ee73e9b8e7
 *
 * -------------------------------------------------------------------------
 *
 * backtrace_func.c
 *
 * libbacktrace (https://github.com/ianlancetaylor/libbacktrace) helpers.
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
# if !defined(_INC_BACKTRACE_FUNC)
#  define _INC_BACKTRACE_FUNC
#  if !defined(BACKTRACE_SKIP)
#   define BACKTRACE_SKIP 1
#  endif /* if !defined(BACKTRACE_SKIP) */

#  include <signal.h>

static struct backtrace_state *state = NULL;
static volatile long bt_pid;
static int stopbt, function_count, hidden_function_count,
           unknown_function_count, backtrace_reported = 0;

_Noreturn static void
error_callback(void *data, const char *message, int error_number)
{
  sigset_t block; sigset_t block_n;
  sigfillset(&block); sigfillset(&block_n);
  sigprocmask(SIG_SETMASK, &block, &block_n);
  (void)data; (void)error_number;
  (void)fprintf(stderr, "\r No backtrace: %s\r\n", message);
  (void)fprintf(stderr,
    "\r\n***********************************************************\r\n\r\n");
  abort();
}

static int
full_callback(void *data, uintptr_t pc, const char *pathname,
              int line_number, const char *function)
{
  (void)data; (void)pc;
  function_count++;
  if (pathname != NULL || function != NULL || line_number != 0)
    {
      if (!stopbt)
        {
          (void)fprintf(stderr, "\r %-.3d: %s()\r\n       %s:%d\r\n",
            function_count, function, pathname, line_number);
          backtrace_reported++;
          int n = strcmp(function, BACKTRACE_MAIN);
          if (n == 0)
            {
              stopbt = 1;
            }
        }
      else
        {
          return 0;
        }
    }
  else
    {
      if (function_count > 1)
        {
          if (!stopbt)
            {
              (void)fprintf(stderr, "\r %-.3d: ???\r\n", function_count);
            }
          else
            {
              hidden_function_count++;
            }
          unknown_function_count++;
        }
      else
        {
          function_count--;
        }
    }
  return 0;
}

_Noreturn static void
backtrace_handler(int number)
{
  sigset_t block; sigset_t block_n;
  sigfillset(&block); sigfillset(&block_n);
  sigprocmask(SIG_SETMASK, &block, &block_n);
  (void)fprintf(stderr,
    "\r\n\r\n****** FATAL ERROR ****************************************\r\n");
  if (bt_pid > 1)
    {
#  if defined(SIGUSR2)
      if (number == SIGUSR2)
        {
          (void)fprintf(stderr,
                "\r\n   PID %ld raised SIGUSR2 (signal %d) ... :(\r\n\r\n",
                (long)bt_pid, number);
        }
      else
        {
#  endif /* if defined(SIGUSR2) */
          (void)fprintf(stderr,
                "\r\n   PID %ld caught fatal signal %d ... :(\r\n\r\n",
                (long)bt_pid, number);
#  if defined(SIGUSR2)
        }
#  endif /* if defined(SIGUSR2) */
    }
  else
    {
      (void)fprintf(stderr,
              "\r\n   Caught fatal signal %d ... :(\r\n\r\n",
              number);
    }
  backtrace_full(state, BACKTRACE_SKIP, full_callback, error_callback, NULL);
  if (backtrace_reported)
    {
      if (hidden_function_count > 1)
        {
          (void)fprintf(stderr,
            "\r        (%d earlier callers not shown)\r\n",
            hidden_function_count);
        }
      if (hidden_function_count == 1)
        {
          (void)fprintf(stderr,
            "\r        (%d earlier caller not shown)\r\n",
            hidden_function_count);
        }
    }
#  if defined(SIGUSR2)
  if (number != SIGUSR2)
    {
      (void)fprintf(stderr,
        "\r\n****** BUG REPORTING **************************************\r\n\r\n");
      (void)fprintf(stderr,
        " URL: https://gitlab.com/dps8m/dps8m/-/wikis/Bug-Reporting\r\n");
    }
#  endif /* if defined(SIGUSR2) */
  (void)fprintf(stderr,
    "\r\n***********************************************************\r\n\r\n");
#  if defined(USE_DUMA)
  DUMA_CHECKALL();
#  endif /* if defined(USE_DUMA) */
  abort();
}
# endif /* if !defined(_INC_BACKTRACE_FUNC) */
#endif /* if defined(USE_BACKTRACE) */
