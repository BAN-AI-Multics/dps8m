/*
 * vim: set filetype=c:tabstop=4:autoinput:expandtab
 * SPDX-License-Identifier: MIT
 * scspell-id: 4a19aab5-433e-11ed-ae50-80ee73e9b8e7
 *
 * Copyright (c) 2020 Daniel Teunis
 * Copyright (c) 2022-2024 The DPS8M Development Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------------
 *
 * novdso is a wrapper that forces the wrapped process to not use the vDSO
 * library by overwriting the `AT_SYSINFO_EHDR` tag in its auxiliary vector.
 * If the `NOVDSO_PAUSE` environment variable is set, the wrapped process is
 * automatically paused to wait for tracer attachment after process startup.
 * In order to continue the paused process, a SIGCONT signal should be sent.
 *
 * ----------------------------------------------------------------------------
 */

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#undef HAS_INCLUDE
#if defined __has_include
# define HAS_INCLUDE(inc) __has_include(inc)
#else
# define HAS_INCLUDE(inc) 0
#endif /* if defined __has_include */

#if defined(__linux__) && \
  ( defined(__x86_64__) || defined(__x86_64) || \
    defined(__amd64__)  || defined(__amd64) ) && \
  HAS_INCLUDE(<linux/auxvec.h>)

# include <linux/auxvec.h>
# include <signal.h>
# include <string.h>
# include <sys/prctl.h>
# include <sys/ptrace.h>
# include <sys/reg.h>
# include <sys/wait.h>
# include <locale.h>

static void
removeVDSO(int pid)
{
  size_t pos;
  int zeroCount;
  long val;

  pos = (size_t)ptrace(PTRACE_PEEKUSER, pid, sizeof ( long ) * RSP, NULL);
  zeroCount = 0;

  while (zeroCount < 2)
    {
      val = ptrace(PTRACE_PEEKDATA, pid, pos += 8, NULL);
      if (val == 0)
        {
          zeroCount++;
        }
    }
  val = ptrace(PTRACE_PEEKDATA, pid, pos += 8, NULL);

  while (1)
    {
      if (val == AT_NULL)
        {
          break;
        }

      if (val == AT_SYSINFO_EHDR)
        {
          (void)ptrace(PTRACE_POKEDATA, pid, pos, AT_IGNORE);
          break;
        }

      val = ptrace(PTRACE_PEEKDATA, pid, pos += 16, NULL);
    }
}

static int
traceProcess(int pid)
{
  int status;
  int exitStatus = 0;

  (void)waitpid(pid, &status, 0);
  (void)ptrace(
    PTRACE_SETOPTIONS,
    pid,
    0,
    PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEEXEC);
  (void)ptrace(PTRACE_SYSCALL, pid, NULL, NULL);

  while (1)
    {
      (void)waitpid(pid, &status, 0);
      if (WIFEXITED(status))
        {
          break;
        }

      if (status >> 8 == ( SIGTRAP | ( PTRACE_EVENT_EXEC << 8 )))
        {
          removeVDSO(pid);
          if (getenv("NOVDSO_PAUSE"))
            {
              (void)kill(pid, SIGSTOP);
              (void)ptrace(PTRACE_DETACH, pid, NULL, NULL);
              (void)fprintf(
                stderr,
                "\r--- PID %llu paused waiting for tracing attachment ---\n",
                (long long unsigned int)pid);
              while (waitpid(pid, &status, 0) > 0)
                {
                  (void)0;
                }
              break;
            }
        }

      (void)ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
    }
  exitStatus = WEXITSTATUS(status);
  return (exitStatus);
}

# define XSTR_EMAXLEN 32767

static const char
*xstrerror_l(int errnum)
{
  int saved = errno;
  const char *ret = NULL;
  static /* __thread */ char buf[XSTR_EMAXLEN];

# if defined(__APPLE__) || defined(_AIX) || \
      defined(__MINGW32__) || defined(__MINGW64__) || \
        defined(CROSS_MINGW32) || defined(CROSS_MINGW64)
#  if defined(__MINGW32__) || defined(__MINGW64__) || \
        defined(CROSS_MINGW32) || defined(CROSS_MINGW64)
  if (strerror_s(buf, sizeof(buf), errnum) == 0) ret = buf; /*LINTOK: xstrerror_l*/
#  else
  if (strerror_r(errnum, buf, sizeof(buf)) == 0) ret = buf; /*LINTOK: xstrerror_l*/
#  endif
# else
#  if defined(__NetBSD__)
  locale_t loc = LC_GLOBAL_LOCALE;
#  else
  locale_t loc = uselocale((locale_t)0);
#  endif
  locale_t copy = loc;
  if (copy == LC_GLOBAL_LOCALE)
    copy = duplocale(copy);

  if (copy != (locale_t)0)
    {
      ret = strerror_l(errnum, copy); /*LINTOK: xstrerror_l*/
      if (loc == LC_GLOBAL_LOCALE)
        {
          freelocale(copy);
        }
    }
# endif

  if (!ret)
    {
      (void)snprintf(buf, sizeof(buf), "Unknown error %d", errnum);
      ret = buf;
    }

  errno = saved;
  return ret;
}

int
main(int argc, char *argv[])
{
  char *myfile;
  char **myargv;
  int exitStatus = 0;
  pid_t child;

  if (argc < 2)
    {
      (void)fprintf(
        stderr,
        "\rUsage: env [NOVDSO_PAUSE=] novdso <command> [arguments ...]\r\n");
      return (EXIT_FAILURE);
    }

  myfile = argv[1];
  myargv = &argv[1];
  child  = fork();

  if (child == 0)
    {
      (void)prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY);
      (void)ptrace(PTRACE_TRACEME, 0, NULL, NULL);
      (void)kill(getpid(), SIGSTOP);
      if (execvp(myfile, myargv))
        {
          (void)fprintf(stderr, "\r%s: %s (%d)\r\n", myfile, xstrerror_l(errno), errno);
        }
    }
  else
    {
      exitStatus = traceProcess(child);
    }

  return (exitStatus);
}

#else  /* if defined(__linux__) */
int
main(void)
{
  (void)fprintf(stderr, "\rError: System not supported.\r\n");

  return (EXIT_FAILURE);
}
#endif /* if defined(__linux__) &&
           ( defined(__x86_64__) || defined(__x86_64) ||
             defined(__amd64__)  || defined(__amd64) ) &&
           HAS_INCLUDE(<linux/auxvec.h> */
