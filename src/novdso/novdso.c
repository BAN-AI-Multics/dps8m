/*
 * vim: set filetype=c tabstop=4 autoinput expandtab
 * SPDX-License-Identifier: MIT
 * scspell-id: 4a19aab5-433e-11ed-ae50-80ee73e9b8e7
 *
 * Copyright (c) 2020 Daniel Teunis
 * Copyright (c) 2022 The DPS8M Development Team
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

#include <stdio.h>

#ifdef __linux__
# include <errno.h>
# include <linux/auxvec.h>
# include <signal.h>
# include <stdlib.h>
# include <string.h>
# include <sys/prctl.h>
# include <sys/ptrace.h>
# include <sys/reg.h>
# include <sys/wait.h>
# include <unistd.h>

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
          (void)fprintf(stderr, "\r%s: %s\r\n", myfile, strerror(errno));
        }
    }
  else
    {
      exitStatus = traceProcess(child);
    }

  return (exitStatus);
}

#else  /* ifdef __linux__ */
int
main(void)
{
  (void)fprintf(stderr, "\rError: A Linux system is required.\r\n");

  return (EXIT_FAILURE);
}
#endif /* ifdef __linux__ */
