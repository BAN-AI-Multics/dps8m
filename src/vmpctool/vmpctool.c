/*
 * vmpctool - a virtual memory page cache utility
 * 
 * vmpctool is forked from vmtouch; the original version
 * is available from https://hoytech.com/vmtouch/
 *
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2009-2017 Doug Hoyte and contributors.
 * Copyright (c) 2020 Jeffrey H. Johnson <trnsz@pobox.com>
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   3. The name of the author may not be used to endorse or promote
 *      products derived from this software without specific prior written
 *      permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define PROGNAME "vmpctool"
#define PROGDESC "virtual memory page cache utility"
#define VMTOUCH_VERSION "2101.13.3 (2021-12-02)"
#define RESIDENCY_CHART_WIDTH 41
#define CHART_UPDATE_INTERVAL 0.37
#define KiB 1024
#define MAX_CRAWL_DEPTH 1 * KiB + 1
#define MAX_NUMBER_OF_IGNORES 1 * KiB + 1
#define MAX_NUMBER_OF_FILENAME_FILTERS 1 * KiB + 1
#define VBUFSIZ 4 * KiB + 1

#if defined( __linux__ ) || defined( __SVR4__ )
# define _FILE_OFFSET_BITS 64
# define _POSIX_SOURCE 1
# define _XOPEN_SOURCE 600
# define _DEFAULT_SOURCE 1
# define _BSD_SOURCE 1
# define _GNU_SOURCE 1
# if defined ( __linux__ )
#  define voff64_t __off64_t
# elif defined ( __SVR4__ )
#  define __EXTENSIONS__ 1
#  define voff64_t off_t
# endif /* if defined (__linux__) */
#else
# define voff64_t long long int
#endif /* if defined( __linux__ ) || defined( __SVR4__ ) */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <inttypes.h>
#include <libgen.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <search.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#ifndef RLIMIT_NOFILE
# ifdef __RLIMIT_OFILE
#  define RLIMIT_NOFILE __RLIMIT_OFILE
# else /* ifdef __RLIMIT_OFILE */
#  define RLIMIT_NOFILE 7
# endif /* ifdef __RLIMIT_OFILE */
#endif /* ifndef RLIMIT_NOFILE */

#if defined( __linux__ )
# include <linux/limits.h>
# include <sys/ioctl.h>
# include <sys/mount.h>
# include <sys/utsname.h>
#endif /* if defined( __linux__ ) */

#ifndef PATH_MAX
# define PATH_MAX 4 * KiB
#endif /* ifndef PATH_MAX */

#ifndef MAX_FILENAME_LENGTH
# ifdef NAME_MAX
#  define MAX_FILENAME_LENGTH NAME_MAX
# else /* ifdef NAME_MAX */
#  ifdef __linux__
#   define MAX_FILENAME_LENGTH 4 * KiB
#  else /* ifdef __linux__ */
#   define MAX_FILENAME_LENGTH 1 * KiB
#  endif /* ifdef __linux__ */
# endif /* ifdef NAME_MAX */
#endif /* ifndef MAX_FILENAME_LENGTH */

struct dev_and_inode
{
  dev_t dev;
  ino_t ino;
};

static long pagesize = 0;
static int64_t total_pages = 0;
static int64_t total_pages_in_core = 0;
static int64_t total_files = 0;
static int64_t total_dirs = 0;
static size_t offset = 0;
static size_t max_len = 0;
static unsigned int junk_counter = 0;
static int curr_crawl_depth = 0;
static ino_t crawl_inodes[MAX_CRAWL_DEPTH];
static void *seen_inodes = NULL;
static dev_t orig_device = 0;
static int orig_device_inited = 0;
static int o_touch = 0;
static int o_evict = 0;
static int o_quiet = 0;
static int o_verbose = 0;
static int o_lock = 0;
static int o_lockall = 0;
static int o_daemon = 0;
static int o_followsymlinks = 0;
static int o_singlefilesystem = 0;
static int o_ignorehardlinkeduplictes = 0;
static size_t o_max_file_size = SIZE_MAX;
static int o_wait = 0;
static char *o_batch = NULL;
static char *o_pidfile = NULL;
static char *o_output = NULL;
static int o_0_delim = 0;
static char *ignore_list[MAX_NUMBER_OF_IGNORES];
static char *filename_filter_list[MAX_NUMBER_OF_FILENAME_FILTERS];
static int number_of_ignores = 0;
static int number_of_filename_filters = 0;
static int exit_pipe[2];
static int daemon_pid = -1;

static void
send_exit_signal(char code)
{
  if (daemon_pid == 0 && o_wait)
    {
      if (write(exit_pipe[1], &code, 1) < 0)
        {
          (void)fprintf(stderr,
            "%s: write: %s (error %d)",
            __func__, strerror(errno), errno);
        }
    }
}

static void
progversion(void)
{
  (void)printf("\r%s %s - %s\n", PROGNAME, VMTOUCH_VERSION, PROGDESC);
}

static void
usage(char *prog)
{
  progversion();
  (void)printf("\r USAGE: %s [ SWITCHES ... ] < FILES ... >\n", prog);
  (void)printf("\r  Switches:\n");
  (void)printf("\r    -?             \t display this help text and exit\n");
  (void)printf("\r    -t             \t touch pages from files into memory\n");
  (void)printf("\r    -e             \t evict pages from files from memory\n");
  (void)printf("\r    -l             \t lock files pages using mlock(2)\n");
  (void)printf("\r    -L             \t lock files pages using mlockall(2)\n");
  (void)printf("\r    -d             \t daemonize %s process\n", PROGNAME);
  (void)printf("\r    -m <MAXSIZE>   \t process files only up to <MAXSIZE>\n");
  (void)printf("\r    -p <RANGE>     \t process only specified <RANGE>\n");
  (void)printf("\r    -f             \t include symbolic link targets\n");
  (void)printf("\r    -F             \t single filesystem mode\n");
  (void)printf("\r    -h             \t include hard link targets\n");
  (void)printf("\r    -i <PATTERN>   \t ignore files matching <PATTERN>\n");
  (void)printf("\r    -I <PATTERN>   \t only process files matching <PATTERN>\n");
  (void)printf("\r    -b <INFILE>    \t read files to process from <INFILE>\n");
  (void)printf("\r    -0             \t use NULL character as seperator\n");
  (void)printf("\r    -w             \t wait for processes before daemonizing\n");
  (void)printf("\r    -P <PIDFILE>   \t write a <PIDFILE> to disk\n");
  (void)printf("\r    -o kv          \t format output as key/value pairs\n");
  (void)printf("\r    -v             \t verbose; increase output verbosity\n");
  (void)printf("\r    -V             \t display version information and exit\n");
  (void)printf("\r    -q             \t quiet; suppress non-essential output\n");
  exit(1);
}

static void
fatal(const char *fmt, ...)
{
  va_list ap;
  char buf[VBUFSIZ];

  va_start(ap, fmt);
  (void)vsnprintf(buf, sizeof ( buf ), fmt, ap);
  va_end(ap);

  (void)fprintf(stderr, "\rFATAL: %s:%s\n", PROGNAME, buf);
  send_exit_signal(1);
  exit(1);
}

static void
warning(const char *fmt, ...)
{
  va_list ap;
  char buf[VBUFSIZ];

  va_start(ap, fmt);
  (void)vsnprintf(buf, sizeof ( buf ), fmt, ap);
  va_end(ap);

  if (!o_quiet)
    { (void)fprintf(stderr, "\rWARN: %s:%s\n", PROGNAME, buf); }
}

static void
reopen_all(void)
{
  if   ( freopen("/dev/null", "r",  stdin) == NULL
      || freopen("/dev/null", "w", stdout) == NULL
      || freopen("/dev/null", "w", stderr) == NULL )
    {
      fatal(
        "%s:%d: freopen failed\r\n       -> %s (error %d)",
        __func__, __LINE__, strerror(errno), errno);
      /* NOTREACHED */
      exit(1);
    }
}

static int
wait_for_child(void)
{
  int  exit_read = 0;
  char exit_value = 0;
  int  wait_status;

  /* CONSTANTCONDITION */
  while (true)
    {
      struct timeval tv;
      fd_set rfds;
      FD_ZERO(&rfds);
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      FD_SET(exit_pipe[0], &rfds);
      if (select(exit_pipe[0] + 1, &rfds, NULL, NULL, &tv) < 0)
        {
          fatal(
            "%s:%d: select failed\r\n       -> %s (error %d)",
            __func__, __LINE__, strerror(errno), errno);
          /* NOTREACHED */
          exit(1);
        }

      if (waitpid(daemon_pid, &wait_status, WNOHANG) > 0)
        {
          fatal(
            "%s:%d: daemon failure\r\n       -> daemon exited unexpectedly",
            __func__, __LINE__);
          /* NOTREACHED */
          exit(1);
        }

      if (FD_ISSET(exit_pipe[0], &rfds))
        { break; }
    }
  exit_read = (int)read(exit_pipe[0], &exit_value, 1);
  if (exit_read < 0)
    {
      fatal(
        "%s:%d: read failure\r\n       -> %s (error %d)",
        __func__, __LINE__, strerror(errno), errno);
      /* NOTREACHED */
      exit(1);
    }

  return exit_value;
}

static void
go_daemon(void)
{
  daemon_pid = fork();
  if (daemon_pid == -1)
    {
      fatal(
        "%s:%d: fork failure\r\n       -> %s (error %d)",
        __func__, __LINE__, strerror(errno), errno);
      /* NOTREACHED */
      exit(1);
    }

  if (daemon_pid)
    {
      if (o_wait)
        { exit(wait_for_child()); }
      exit(0);
    }

  if (setsid() == -1)
    {
      fatal(
        "%s:%d: setsid failure\r\n       -> %s (error %d)",
        __func__, __LINE__, strerror(errno), errno);
      /* NOTREACHED */
      exit(1);
    }

  if (!o_wait)
    { reopen_all(); }
}

static char *
pretty_print_size(int64_t inp)
{
  static char output[KiB];

  if (inp < KiB)
    {
      (void)snprintf(output, sizeof ( output ), "%" PRId64 "B", inp);
      return output;
    }

  inp /= KiB;
  if (inp < KiB)
    {
      (void)snprintf(output, sizeof ( output ), "%" PRId64 "KiB", inp);
      return output;
    }

  inp /= KiB;
  if (inp < KiB)
    {
      (void)snprintf(output, sizeof ( output ), "%" PRId64 "MiB", inp);
      return output;
    }

  inp /= KiB;
  if (inp < KiB)
    {
      (void)snprintf(output, sizeof ( output ), "%" PRId64 "GiB", inp);
      return output;
    }

  inp /= KiB;
  if (inp < KiB)
    {
      (void)snprintf(output, sizeof ( output ), "%" PRId64 "TiB", inp);
      return output;
    }

  inp /= KiB;
  if (inp < KiB)
    {
      (void)snprintf(output, sizeof ( output ), "%" PRId64 "PiB", inp);
      return output;
    }

  inp /= KiB;
  (void)snprintf(output, sizeof ( output ), "%" PRId64 "EiB", inp);
  return output;
}

static int64_t
parse_size(char *inp)
{
  char *tp;
  int len = (int)strlen(inp);
  char *errstr = "bad input; try '4096b', '4k', '100M', '1.5G', etc.";
  char mult_char;
  int mult = 1;
  double val;

  if (len < 1)
    {
      fatal("%s:%d: %s", __func__, __LINE__, errstr);
      /* NOTREACHED */
      exit(1);
    }

  mult_char = (char)tolower(inp[len - 1]);

  if (isalpha(mult_char))
    {
      switch (mult_char)
        {
        case 'b':
          mult = 1;
          break;

        case 'k':
          mult = KiB;
          break;

        case 'm':
          mult = KiB * KiB;
          break;

        case 'g':
          mult = KiB * KiB * KiB;
          break;

        default:
          fatal("%s:%d: unknown multiplier %c", __func__, __LINE__, mult_char);
          /* NOTREACHED */
          exit(1);
        }
      inp[len - 1] = '\0';
    }

  val = (double)strtod(inp, &tp);

  if ((long)val < 0L || val == HUGE_VAL || *tp != '\0')
    {
      fatal("%s:%d: %s", __func__, __LINE__, errstr);
      /* NOTREACHED */
      exit(1);
    }

  val *= mult;

  if ((long long)val > INT64_MAX)
    {
      fatal("%s:%d: %s", __func__, __LINE__, errstr);
      /* NOTREACHED */
      exit(1);
    }

  return (int64_t)val;
}

static int64_t
bytes2pages(int64_t bytes)
{
  if (pagesize == 0)
    {
      fatal("%s:%d: pagesize cannot be 0", __func__, __LINE__);
      /* NOTREACHED */
      exit(1);
    }

  return ( bytes + pagesize - 1 ) / pagesize;
}

static void
parse_range(char *inp)
{
  char *token;
  size_t upper_range = 0;
  size_t lower_range = 0;

  token = strsep(&inp, "-");

  if (inp == NULL)
    { upper_range = (size_t)parse_size(token); }
  else
    { if (token != NULL)
        { if (*token != '\0') { lower_range = (size_t)parse_size(token); } }
      else
        {
          fatal("%s:%d: token cannot be NULL", __func__, __LINE__);
          /* NOTREACHED */
          exit(1);
        }

      token = strsep(&inp, "-");
      if (*token != '\0')
        { upper_range = (size_t)parse_size(token); }

      token = strsep(&inp, "-");
      if (token != NULL)
        {
          fatal("%s:%d: malformed range; multiple hyphens", __func__, __LINE__);
          /* NOTREACHED */
          exit(1);
        }
    }

  /* offset must be a multiple of pagesize */
  if (pagesize == 0)
    {
      fatal("%s:%d: pagesize cannot be 0", __func__, __LINE__);
      /* NOTREACHED */
      exit(1);
    }

  offset = ( (unsigned long)lower_range / (unsigned long)pagesize )
           * (unsigned long)pagesize;

  if (upper_range)
    {
      if (upper_range <= offset)
        {
          fatal("%s:%d: range limits out of order", __func__, __LINE__);
          /* NOTREACHED */
          exit(1);
        }

      max_len = upper_range - offset;
    }
}

static void
parse_ignore_item(char *inp)
{
  if (inp == NULL)
    { return; }

  if (strlen(inp) > MAX_FILENAME_LENGTH)
    {
      fatal(
        "%s:%d: %d-character pattern exceeds %d-character limit",
        __func__, __LINE__, strlen(inp), MAX_FILENAME_LENGTH);
      /* NOTREACHED */
      return;
    }

  if (number_of_ignores >= MAX_NUMBER_OF_IGNORES)
    {
      fatal(
        "%s:%d: %d patterns specified; exceeds %d pattern limit",
        __func__, __LINE__, number_of_ignores, MAX_NUMBER_OF_IGNORES - 1);
      /* NOTREACHED */
      return;
    }

  ignore_list[number_of_ignores] = strdup(inp);
  number_of_ignores++;
}

static void
parse_filename_filter_item(char *inp)
{
  if (inp == NULL)
    { return; }

  if (strlen(inp) > MAX_FILENAME_LENGTH)
    {
      fatal("%s:%d: oversized pattern provided to -I", __func__, __LINE__);
      /* NOTREACHED */
      return;
    }

  if (number_of_filename_filters >= MAX_NUMBER_OF_FILENAME_FILTERS)
    {
      fatal(
        "%s:%d: too many patterns specified with -I; limit is %d",
        __func__, __LINE__, MAX_NUMBER_OF_FILENAME_FILTERS);
      /* NOTREACHED */
      return;
    }

  filename_filter_list[number_of_filename_filters] = strdup(inp);
  number_of_filename_filters++;
}

static int
aligned_p(void *p)
{ return 0 == ((long)p & ( pagesize - 1 )); }

static int
is_mincore_page_resident(char p)
{ return p & 0x1; }

static void
increment_nofile_rlimit(void)
{
  struct rlimit r;

  if (getrlimit(RLIMIT_NOFILE, &r))
    {
      fatal(
        "%s:%d: increment_nofile_rlimit: getrlimit failed\r\n       -> %s (error %d)",
        __func__, __LINE__, strerror(errno), errno);
      /* NOTREACHED */
      exit(1);
    }

  r.rlim_cur = r.rlim_max + 1; r.rlim_max = r.rlim_max + 1;

  if (setrlimit(RLIMIT_NOFILE, &r))
    {
      if (errno == EPERM)
        {
          if (getuid() == 0 || geteuid() == 0)
            {
              fatal(
                "%s:%d: system file limit reached\r\n       -> %s (error %d)",
                __func__, __LINE__, strerror(errno), errno);
              /* NOTREACHED */
              exit(errno);
            }

          fatal(
            "%s:%d: user file limit reached\r\n       -> %s (error %d)",
            __func__, __LINE__, strerror(errno), errno);
          /* NOTREACHED */
          exit(errno);
        }

      fatal(
        "%s:%d: increment_nofile_rlimit: setrlimit failed\r\n       -> %s (error %d)",
        __func__, __LINE__, strerror(errno), errno);
      /* NOTREACHED */
      exit(errno);
    }
}

static double
gettimeofday_as_double(void)
{
  struct timeval tv;

  long xtrc = gettimeofday(&tv, NULL);

  if (xtrc == -1)
    {
      if (errno)
        {
          fatal(
            "%s:%d: gettimeofday failure\r\n       -> %s (error %d)",
            __func__, __LINE__, strerror(errno), errno);
          /* NOTREACHED */
          exit(errno);
        }
      else
        {
          fatal("%s:%d: gettimeofday failure", __func__, __LINE__);
          /* NOTREACHED */
          exit(1);
        }
    }

  return (double)((double)tv.tv_sec + (double)((long)tv.tv_usec / 1000000.0L ));
}

static unsigned long int
intlength(long int value)
{
  unsigned long int l = !value;

  while (value)
    { l++; value /= 10; }
  return l;
}

static void
print_page_residency_chart(FILE *out, char *mincore_array,
                           int64_t pages_in_file)
{
  int64_t pages_in_core = 0;
  int64_t pages_per_char = 0;
  int64_t i, j = 0, curr = 0;
  int64_t stretch_factor = 0;

  if (pages_in_file <= RESIDENCY_CHART_WIDTH)
    { pages_per_char = 1; }
  else
    { pages_per_char = ( pages_in_file / RESIDENCY_CHART_WIDTH ) + 1; }

  stretch_factor = RESIDENCY_CHART_WIDTH - ( pages_in_file / pages_per_char );

  (void)fprintf(out, "\r[");
  for (i = 0; i < (int64_t)pages_in_file; i++)
    {
      if (is_mincore_page_resident(mincore_array[i]))
        { curr++; pages_in_core++; }

      j++;
      if (j == pages_per_char)
        {
          if (curr == pages_per_char)
            { (void)fprintf(out, "="); }
          else if (curr == 0)
            { (void)fprintf(out, " "); }
          else
            { (void)fprintf(out, "-"); }
          j = curr = 0;
        }
    }

  if (j)
    {
      if (curr == j)
        { (void)fprintf(out, "="); }
      else if (curr == 0)
        { (void)fprintf(out, " "); }
      else
        { (void)fprintf(out, "-"); }
    }

  unsigned long subtint = intlength(pages_in_core);
  unsigned long leadint = intlength(pages_in_file);
  long formint
    = (long)(  7 - strlen(pretty_print_size(pagesize * pages_in_file)));
  long leadspc
    = (long)(( 5 - strlen(pretty_print_size(pagesize * pages_in_file)))
             + ( 1 + ( 9 * 2 ) + 1 ) - ( 1 + ( 2 * leadint ) + 1 ));

  if (formint < 0)
    { formint = 0; }

  (void)fprintf(out, "]");

  if (pages_in_file < RESIDENCY_CHART_WIDTH)
    { (void)fprintf(out, " "); }

  if (pages_per_char >= 1)
    { (void)fprintf(out, " "); }

  for (long ik = (long)( leadint - subtint ) + leadspc - formint + stretch_factor;
       ik >= 0; --ik)
         { (void)fprintf(out, " "); }

  (void)fprintf(out, "%" PRId64 "/%" PRId64, pages_in_core, pages_in_file);
  for (long k = formint; k >= 0; --k)
    { (void)fprintf(out, " "); }

  (void)fflush(out); (void)fflush(stderr); (void)fflush(stdout);
  (void)fprintf(out, "~%s  %-3.0Lf%%",
    pretty_print_size(pagesize * pages_in_file),
    (long double)((long double)((long double)pages_in_core + 0.01L )
      / (long double)((long double)pages_in_file + 0.01L ) * 100L ));
  (void)fflush(out); (void)fflush(stderr); (void)fflush(stdout);
}

#ifdef __linux__
static int
can_do_mincore(struct stat *st)
{
  struct utsname utsinfo;

  if (uname(&utsinfo) == 0)
    {
      unsigned long ver[2 * KiB];
      int i = 0;
      char *p = utsinfo.release;
      (void)memset(ver, '\0', sizeof(unsigned long)*2*KiB);
      while (*p)
        {
          if (isdigit(*p))
            { ver[i] = (unsigned long)strtol(p, &p, 10); i++; }
          else
            { p++; }
        }
      if (( ver[0] < 5 ) || ( ver[1] < 2 ))
        { return 1; }
    }

  uid_t uid = getuid();

  return st->st_uid == uid
         || ( st->st_gid == getgid() && ( st->st_mode & S_IWGRP ))
         || ( st->st_mode & S_IWOTH ) || uid == 0;
}
#endif /* ifdef __linux__ */

static void
vmpc_file(char *path)
{
  int fd = -1;
  void *mem = NULL;
  struct stat sb;
  size_t len_of_file = 0;
  size_t len_of_range = 0;
  int64_t pages_in_range;
  int i;
  int res;
  int open_flags;

retry_open:
  open_flags = O_RDONLY;

#if defined( O_NOATIME )
  open_flags |= O_NOATIME;
#endif /* if defined( O_NOATIME ) */

  fd = open(path, open_flags, 0);

#if defined( O_NOATIME )
  if (fd == -1 && errno == EPERM)
    { open_flags &= ~O_NOATIME; fd = open(path, open_flags, 0); }

#endif /* if defined( O_NOATIME ) */

  if (fd == -1)
    {
      if (errno == ENFILE || errno == EMFILE)
        {
          increment_nofile_rlimit();
          goto retry_open;
        }

      warning(
        "%s:%d: unable to open %s\r\n      -> %s (error %d)",
        __func__, __LINE__, path, strerror(errno), errno);
      goto bail;
    }

  res = fstat(fd, &sb);

  if (res)
    {
      warning(
        "%s:%d: unable to fstat %s\r\n      -> %s (error %d)",
        __func__, __LINE__, path, strerror(errno), errno);
      goto bail;
    }

  if (S_ISBLK(sb.st_mode))
    {
#if defined( __linux__ )
      if (ioctl(fd, BLKGETSIZE64, &len_of_file))
        {
          warning(
            "%s:%d: unable to ioctl %s\r\n      -> %s (error %d)",
            __func__, __LINE__, path, strerror(errno), errno);
          goto bail;
        }

#else  /* if defined( __linux__ ) */
      fatal(
        "%s:%d: discovery of block device size not supported",
        __func__, __LINE__);
      /* NOTREACHED */
      exit(1);
#endif /* if defined( __linux__ ) */
    }
  else
    { len_of_file = (size_t)sb.st_size; }

  if (len_of_file == 0)
    { goto bail; }

  if (len_of_file > o_max_file_size)
    {
      warning("%s:%d: file %s too large", __func__, __LINE__, path);
      goto bail;
    }

  if (max_len > 0 && ( offset + max_len ) < len_of_file)
    { len_of_range = max_len; }
  else if (offset >= len_of_file)
    {
      warning("%s:%d: file %s smaller than offset", __func__, __LINE__, path);
      goto bail;
    }
  else
    { len_of_range = (unsigned long)len_of_file - (unsigned long)offset; }

  mem = mmap(
    NULL,
    (unsigned long)len_of_range,
    PROT_READ,
    MAP_SHARED,
    fd,
    (voff64_t)offset);

  if (mem == MAP_FAILED)
    {
      warning(
        "%s:%d: unable to mmap file %s\r\n      -> %s (error %d)",
        __func__, __LINE__, path, strerror(errno), errno);
      goto bail;
    }

  if (!aligned_p(mem))
    {
      fatal("%s:%d: mmap: %s: not page aligned", __func__, __LINE__, path);
      /* NOTREACHED */
      exit(1);
    }

  pages_in_range = (int64_t)bytes2pages((int64_t)len_of_range);

  total_pages += pages_in_range;

  if (o_evict)
    {
      if (o_verbose)
        { (void)fprintf(stdout, "Evicting %s\n", path); }

#if defined( __linux__ )
# ifdef POSIX_FADV_DONTNEED
      if (posix_fadvise(
            fd,
            (voff64_t)offset,
            (voff64_t)len_of_range,
            POSIX_FADV_DONTNEED))
        {
          warning(
            "%s:%d: unable to posix_fadvise file %s\r\n      -> %s (error %d)",
            __func__, __LINE__, path, strerror(errno), errno);
        }
# endif /* ifdef POSIX_FADV_DONTNEED */

#elif defined( __OpenBSD__ ) || defined( __FreeBSD_kernel__ ) \
   || defined( __FreeBSD__ ) || defined( __sun__ ) || defined( __APPLE__ )
      if (msync(mem, len_of_range, MS_INVALIDATE))
        {
          warning(
            "%s:%d: unable to msync invalidate file %s\r\n      -> %s (error %d)",
            __func__, __LINE__, path, strerror(errno), errno);
        }

#else  /* if defined( __linux__ ) */
      fatal("%s:%d: cache eviction not supported", __func__, __LINE__);
      /* NOTREACHED */
      exit(1);
#endif /* if defined( __linux__ ) */
    }
  else
    {
      double last_chart_print_time = 0.0, temp_time;
      char *mincore_array = malloc((unsigned long)pages_in_range);
      if (mincore_array == NULL)
        {
          fatal(
            "%s:%d: failed to malloc mincore\r\n       -> %s (error %d)",
            __func__, __LINE__, strerror(errno), errno);
          /* NOTREACHED */
          exit(errno);
        }
#if !defined( __OpenBSD__ )
      if (mincore(mem, len_of_range, (void *)mincore_array))
        {
          fatal(
            "%s:%d: mincore failed for %s\r\n       -> %s (error %d)",
            __func__, __LINE__, path, strerror(errno), errno);
          /* NOTREACHED */
          exit(1);
        }
#else /* if !defined( __OpenBSD__) */
      if ( !o_quiet )
        warning(
          "%s:%d: mincore is unavailable; data will be inaccurate!",
          __func__, __LINE__);
#endif /* if !defined( __OpenBSD__) */

      for (i = 0; i < pages_in_range; i++)
        {
          if (is_mincore_page_resident(mincore_array[i]))
            { total_pages_in_core++; }
        }

      if (o_verbose)
        {
          if (strlen(path) <= ( 33 + RESIDENCY_CHART_WIDTH ))
            {
              if (path[0] == '/' && path[1] == '/')
                {
                  char *ctpath = path + 1;
                  (void)fprintf(stdout, "\r%s\n", ctpath);
                }
              else
                { (void)fprintf(stdout, "\r%s\n", path); }
            }
          else
            {
              (void)fprintf(
                stdout,
                "\r... %s\n",
                ( path + ( strlen(path) - ( 33 + RESIDENCY_CHART_WIDTH ))));
            }

#ifdef __linux__
          if (!can_do_mincore(&sb) && ( !o_quiet ) && ( o_verbose > 1 ))
            {
              warning(
                "%s:%d: no mincore permissions; data will be inaccurate!",
                __func__, __LINE__);
            }
#endif /* ifdef __linux__ */

          last_chart_print_time = gettimeofday_as_double();
          print_page_residency_chart(stdout, mincore_array, pages_in_range);
        }

      if (o_touch)
        {
          for (i = 0; i < pages_in_range; i++)
            {
              junk_counter += (unsigned int)((char *)mem )[i * pagesize];
              mincore_array[i] = 1;

              if (o_verbose)
                {
                  temp_time = gettimeofday_as_double();

                  if (temp_time > ( last_chart_print_time + CHART_UPDATE_INTERVAL ))
                    {
                      last_chart_print_time = temp_time;
                      print_page_residency_chart(stdout, mincore_array, pages_in_range);
                    }
                }
            }
        }

      if (o_verbose)
        {
          print_page_residency_chart(stdout, mincore_array, pages_in_range);
          (void)fprintf(stdout, "\n");
        }

      free(mincore_array);
    }

  if (o_lock)
    {
      if (mlock(mem, len_of_range))
        {
          fatal(
            "%s:%d: mlock failed for %s\r\n       -> %s (error %d)",
            __func__, __LINE__, path, strerror(errno), errno);
          /* NOTREACHED */
          exit(errno);
        }
    }

bail:
  if (!o_lock && !o_lockall && mem)
    {
      if (munmap(mem, len_of_range))
        {
          warning(
            "%s:%d: unable to munmap file %s\r\n      -> %s (error %d)",
            __func__, __LINE__, path, strerror(errno), errno);
        }
    }

  if (fd != -1)
    { (void)close(fd); }
}

static int
compare_func(const void *p1, const void *p2)
{
  const struct dev_and_inode *kp1 = p1, *kp2 = p2;
  int cmp1;

  cmp1 = ( kp1->ino > kp2->ino ) - ( kp1->ino < kp2->ino );
  if (cmp1 != 0)
    { return cmp1; }

  return ( kp1->dev > kp2->dev ) - ( kp1->dev < kp2->dev );
}

static inline void
add_object(struct stat *st)
{
  struct dev_and_inode *newp = malloc(sizeof ( struct dev_and_inode ));

  if (newp == NULL)
    {
      fatal("%s:%d: malloc: out of memory", __func__, __LINE__);
      /* NOTREACHED */
      exit(1);
    }

  newp->dev = st->st_dev; newp->ino = st->st_ino;
  if (tsearch(newp, &seen_inodes, compare_func) == NULL)
    {
      fatal("%s:%d: tsearch: out of memory", __func__, __LINE__);
      /* NOTREACHED */
      exit(1);
    }
}

static int
is_ignored(const char *path)
{
  char *path_copy;
  int match, i;

  if (!number_of_ignores)
    { return 0; }

  path_copy = strdup(path);
  match = 0;

  char *filename = basename(path_copy);

  for (i = 0; i < number_of_ignores; i++)
    {
      if (fnmatch(ignore_list[i], filename, 0) == 0)
        {
          match = 1;
          break;
        }
    }

  free(path_copy);
  return match;
}

static int
is_filename_filtered(const char *path)
{
  char *path_copy;
  int match, i;

  if (!number_of_filename_filters)
    { return 1; }

  path_copy = strdup(path);
  match = 0;

  char *filename = basename(path_copy);

  for (i = 0; i < number_of_filename_filters; i++)
    {
      if (fnmatch(filename_filter_list[i], filename, 0) == 0)
        {
          match = 1;
          break;
        }
    }

  free(path_copy);
  return match;
}

static inline int
find_object(struct stat *st)
{
  struct dev_and_inode obj;
  void *res;

  obj.dev = st->st_dev;
  obj.ino = st->st_ino;
  res = (void *)tfind(&obj, &seen_inodes, compare_func);
  return res != (void *)NULL;
}

static void
vmpc_rdir(char *path)
{
  struct stat sb;
  DIR *dirp;
  struct dirent *de;
  char *ndpath[PATH_MAX];
  char npath[PATH_MAX];
  int res;
  unsigned long tp_path_len = (unsigned long)strlen(path);
  int i;

  if (path[0] == '/' && path[1] == '/')
    { *ndpath = path + 1; path = *ndpath; }

  if (path[tp_path_len - 1] == '/' && tp_path_len > 1)
    { path[tp_path_len - 1] = '\0'; }

  if (is_ignored(path))
    { return; }

  res = o_followsymlinks ? stat(path, &sb) : lstat(path, &sb);

  if (res)
    {
      warning(
        "%s:%d: unable to stat %s\r\n      -> %s (error %d)",
        __func__, __LINE__, path, strerror(errno), errno);
      return;
    }
  else
    {
      if (S_ISLNK(sb.st_mode))
        {
          warning("%s:%d: not following link %s", __func__, __LINE__, path);
          return;
        }

      if (o_singlefilesystem)
        {
          if (!orig_device_inited)
            {
              orig_device = sb.st_dev;
              orig_device_inited = 1;
            }
          else
            {
              if (sb.st_dev != orig_device)
                {
                  warning(
                    "%s:%d: single filesystem mode: skipping %s",
                    __func__, __LINE__, path);
                  return;
                }
            }
        }

      if (!o_ignorehardlinkeduplictes && sb.st_nlink > 1)
        {
          if (find_object(&sb))
            { return; }
          else
            { add_object(&sb); }
        }

      if (S_ISDIR(sb.st_mode))
        {
          for (i = 0; i < curr_crawl_depth; i++)
            {
              if (crawl_inodes[i] == sb.st_ino)
                {
                  warning(
                    "%s:%d: too many redirections: %s",
                    __func__, __LINE__, path);
                  return;
                }
            }

          if (curr_crawl_depth == MAX_CRAWL_DEPTH)
            {
              fatal(
                "%s:%d: maximum directory depth reached: %s",
                __func__, __LINE__, path);
              /* NOTREACHED */
              exit(1);
            }

          total_dirs++;

          crawl_inodes[curr_crawl_depth] = sb.st_ino;

retry_opendir:

          dirp = opendir(path);

          if (dirp == NULL)
            {
              if (errno == ENFILE || errno == EMFILE)
                {
                  increment_nofile_rlimit();
                  goto retry_opendir;
                }

              warning(
                "%s:%d: unable to opendir %s\r\n      -> %s (error %d)",
                __func__, __LINE__, path, strerror(errno), errno);
              return;
            }

          while (( de = readdir(dirp)) != NULL)
            {
              if (strcmp(de->d_name,  ".") == 0 ||
                  strcmp(de->d_name, "..") == 0)
                    { continue; }

              if ((unsigned long)(snprintf(npath, sizeof (npath),
                  "%s/%s", path, de->d_name)) >= (unsigned long)sizeof(npath))
                {
                  warning("%s:%d: path too long %s", __func__, __LINE__, path);
                  goto bail;
                }

              curr_crawl_depth++; vmpc_rdir(npath); curr_crawl_depth--;
            }

bail:
          if (closedir(dirp))
            {
              warning(
                "%s:%d: unable to closedir %s\r\n      -> %s (error %d)",
                __func__, __LINE__, path, strerror(errno), errno);
              return;
            }
        }
      else if (S_ISLNK(sb.st_mode))
        {
          warning("%s:%d: skipping symlink: %s", __func__, __LINE__, path);
          return;
        }
      else if (S_ISREG(sb.st_mode) || S_ISBLK(sb.st_mode))
        {
          if (is_filename_filtered(path))
            { total_files++; vmpc_file(path); }
        }
      else
        {
          warning("%s:%d: skipping special file: %s", __func__, __LINE__, path);
        }
    }
}

static void
vmpc_batch_crawl(const char *path)
{
  FILE *f;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  int delim = o_0_delim ? '\0' : '\n';

  if (!strcmp(path, "-"))
    { f = stdin; }
  else
    {
      f = fopen(path, "r");
      if (!f)
        {
          warning(
            "%s:%d: unable to open %s\r\n      -> %s (error %d)",
            __func__, __LINE__, path, strerror(errno), errno);
          return;
        }
    }

  while (( read = getdelim(&line, &len, delim, f)) != -1)
    { line[read - 1] = '\0'; vmpc_rdir(line); }

  free(line);
  (void)fclose(f);
}

static void
remove_pidfile(void)
{
  int res = 0;

  res = unlink(o_pidfile);
  if (res < 0 && errno != ENOENT)
    {
      warning(
        "%s:%d: unable to remove pidfile %s\r\n      -> %s (error %d)",
        __func__, __LINE__, o_pidfile, strerror(errno), errno);
    }
}

static void
write_pidfile(void)
{
  FILE *f = NULL;
  long int wrote = -1;

  f = fopen(o_pidfile, "w");
  if (!f)
    {
      warning(
        "%s:%d: unable to open pidfile %s\r\n      -> %s (error %d)",
        __func__, __LINE__, o_pidfile, strerror(errno), errno);
      return;
    }

  wrote = fprintf(f, "%ld\n", (long)getpid());

  (void)fclose(f);

  if (wrote < 1)
    {
      warning(
        "%s:%d: unable to write pidfile %s\r\n      -> %s (error %d)",
        __func__, __LINE__, o_pidfile, strerror(errno), errno);
      remove_pidfile();
    }
}

static void
signal_handler_clear_pidfile(int signal_num)
{
  (void)signal_num;
  remove_pidfile();
}

static void
register_signals_for_pidfile(void)
{
  struct sigaction sa = { 0 };

  sa.sa_handler = signal_handler_clear_pidfile;
  if (sigaction(SIGINT, &sa, NULL) < 0 || sigaction(SIGTERM, &sa, NULL) < 0
      || sigaction(SIGQUIT, &sa, NULL) < 0)
    {
      warning(
        "%s:%d: unable to register signal handler\r\n      -> %s (error %d)",
        __func__, __LINE__, strerror(errno), errno);
    }
}

int
main(int argc, char **argv)
{
  int ch, i;
  char *prog = argv[0];
  struct timeval start_time;
  struct timeval end_time;

  setlocale(LC_NUMERIC, "");
  
  if (pipe(exit_pipe))
    {
      fatal(
        "%s:%d: pipe failure\r\n       -> %s (error %d)",
        __func__, __LINE__, strerror(errno), errno);
      /* NOTREACHED */
      exit(errno);
    }

  pagesize = sysconf(_SC_PAGESIZE);
  if (pagesize < 1)
    {
      fatal("%s:%d: unable to determine pagesize", __func__, __LINE__);
      /* NOTREACHED */
      exit(1);
    }

  while (( ch = getopt(argc, argv, "?tevVqlLdfFh0i:I:p:b:m:P:wo:")) != -1)
    {
      switch (ch)
        {
        case '?':
          usage(prog);
          break;

        case 't':
          o_touch = 1;
          break;

        case 'e':
          o_evict = 1;
          break;

        case 'q':
          o_quiet = 1;
          break;

        case 'v':
          o_verbose++;
          break;

        case 'V':
          progversion();
          exit(0);
          /* NOTREACHED */
          break;

        case 'l':
          o_lock = 1;
          o_touch = 1;
          break;

        case 'L':
          o_lockall = 1;
          o_touch = 1;
          break;

        case 'd':
          o_daemon = 1;
          break;

        case 'f':
          o_followsymlinks = 1;
          break;

        case 'F':
          o_singlefilesystem = 1;
          break;

        case 'h':
          o_ignorehardlinkeduplictes = 1;
          break;

        case 'p':
          parse_range(optarg);
          break;

        case 'i':
          parse_ignore_item(optarg);
          break;

        case 'I':
          parse_filename_filter_item(optarg);
          break;

        case 'm':
        {
          int64_t val = parse_size(optarg);
          o_max_file_size = (size_t)val;
          if (val != (int64_t)o_max_file_size)
            {
              fatal("%s:%d: value for -m exceeds limit", __func__, __LINE__);
              /* NOTREACHED */
              exit(1);
            }

          break;
        }

        case 'w':
          o_wait = 1;
          break;

        case 'b':
          o_batch = optarg;
          break;

        case '0':
          o_0_delim = 1;
          break;

        case 'P':
          o_pidfile = optarg;
          break;

        case 'o':
          o_output = optarg;
          break;
        }
    }

  argc -= optind; argv += optind;

  if (o_touch)
    {
      if (o_evict)
        {
          fatal(
            "%s:%d: invalid option combination: -t and -e",
            __func__, __LINE__);
          /* NOTREACHED */
          exit(1);
        }
    }

  if (o_evict)
    {
      if (o_lock)
        {
          fatal(
            "%s:%d: invalid option combination: -e and -l",
            __func__, __LINE__);
          /* NOTREACHED */
          exit(1);
        }
    }

  if (o_lock && o_lockall)
    {
      fatal(
        "%s:%d: invalid option combination: -l and -L",
        __func__, __LINE__);
      /* NOTREACHED */
      exit(1);
    }

  if (o_daemon)
    {
      if (!( o_lock || o_lockall ))
        {
          fatal(
            "%s:%d: invalid optionncombination: missing -l or -L",
            __func__, __LINE__);
          /* NOTREACHED */
          exit(1);
        }

      if (!o_wait)
        { o_quiet = 1; o_verbose = 0; }
    }

  if (o_wait && !o_daemon)
    {
      fatal(
        "%s:%d: invalid option combination: missing -d",
        __func__, __LINE__);
      /* NOTREACHED */
      exit(1);
    }

  if (o_quiet && o_verbose)
    {
      fatal(
        "%s:%d: invalid option combination: -q and -v",
        __func__, __LINE__);
      /* NOTREACHED */
      exit(1);
    }

  if (o_pidfile && ( !o_lock && !o_lockall ))
    {
      fatal(
        "%s:%d: invslid option combination: missing -l or -L",
        __func__, __LINE__);
      /* NOTREACHED */
      exit(1);
    }

  if (!argc && !o_batch)
    {
      (void)fprintf(
        stderr,
        "Incorrect number of arguments; try %s '-?' for help.\n",
        prog);
      exit(1);
    }

  if (o_daemon)
    { go_daemon(); }

  long gtrc = gettimeofday(&start_time, NULL);

  if (gtrc == -1)
    {
      if (errno)
        {
          fatal(
            "%s:%d: gettimeofday failure\r\n       -> %s (error %d)",
            __func__, __LINE__,
            strerror(errno),
            errno);
          /* NOTREACHED */
          exit(errno);
        }
      else
        {
          fatal("%s:%d: gettimeofday failure", __func__, __LINE__);
          /* NOTREACHED */
          exit(1);
        }
    }

  if (o_batch)
    { vmpc_batch_crawl(o_batch); }

  for (i = 0; i < argc; i++)
    { vmpc_rdir(argv[i]); }

  long etrc = gettimeofday(&end_time, NULL);

  if (etrc == -1)
    {
      if (errno)
        {
          fatal(
            "%s:%d: gettimeofday failure\r\n       -> %s (error %d)",
            __func__, __LINE__,
            strerror(errno),
            errno);
          /* NOTREACHED */
          exit(errno);
        }
      else
        {
          fatal("%s:%d: gettimeofday failure", __func__, __LINE__);
          /* NOTREACHED */
          exit(1);
        }
    }

  int64_t total_pages_in_core_size = total_pages_in_core * pagesize;
  int64_t total_pages_size = total_pages * pagesize;
  double total_pages_in_core_perc
    = 100.0 * (double)total_pages_in_core / (double)total_pages;
  double elapsed
    = (double)( end_time.tv_sec - start_time.tv_sec )
      + (double)( end_time.tv_usec - start_time.tv_usec ) / 1000000.0;

  if (o_lock || o_lockall)
    {
      if (o_lockall)
        {
          if (mlockall(MCL_CURRENT))
            {
              fatal(
                "%s:%d: unable to mlockall\r\n       -> %s (error %d)",
                __func__, __LINE__,
                strerror(errno),
                errno);
              /* NOTREACHED */
              exit(errno);
            }
        }

      if (o_pidfile)
        { register_signals_for_pidfile(); write_pidfile(); }

      if (!o_quiet)
        {
          (void)printf(
            "LOCKED %'" PRId64 " pages (%s)\n",
            total_pages,
            pretty_print_size(total_pages_size));
        }

      if (o_wait)
        { reopen_all(); }

      send_exit_signal(0);
      (void)select(0, NULL, NULL, NULL, NULL);
      exit(0);
    }

  if (!o_quiet)
    {
      if (o_output == NULL)
        {
          if (o_verbose)
            { (void)printf("\n"); }

          (void)printf("           Files: %'" PRId64 "\n", total_files);
          (void)printf("     Directories: %'" PRId64 "\n", total_dirs);
          if (o_touch)
            {
              (void)printf(
                "   Touched Pages: %'" PRId64 " (%s)\n",
                total_pages,
                pretty_print_size(total_pages_size));
            }
          else if (o_evict)
            {
              (void)printf(
                "   Evicted Pages: %'" PRId64 " (%s)\n",
                total_pages,
                pretty_print_size(total_pages_size));
            }
          else
            {
              (void)printf(
                "  Resident Pages: %'" PRId64 "/%'" PRId64 "  ",
                total_pages_in_core,
                total_pages);
              (void)printf("%s/", pretty_print_size(total_pages_in_core_size));
              (void)printf("%s  ", pretty_print_size(total_pages_size));
              if (total_pages)
                {
                  (void)printf("%.3g%%", total_pages_in_core_perc);
                }

              (void)printf("\n");
            }

          (void)elapsed;
          if (elapsed <= 0)
            {
              (void)printf("         Elapsed: -- seconds\n");
            }

          if (elapsed < 0.001)
            {
              (void)printf(
                "         Elapsed: %.5g microseconds\n",
                elapsed * 1000000);
            }
          else if (elapsed < 0.01)
            {
              (void)printf("         Elapsed: %.5g milliseconds\n", elapsed * 1000);
            }
          else
            {
              (void)printf("         Elapsed: %.5g seconds\n", elapsed);
            }
        }
      else if (strncmp(o_output, "kv", 2) == 0)
        {
          char *desc = o_touch ? "Touched" : o_evict ? "Evicted" : "Resident";
          (void)printf(
            "Files=%" PRId64 " Directories=%" PRId64
            " %sPages=%" PRId64 " TotalPages=%" PRId64
            " %sSize=%" PRId64 " TotalSize=%" PRId64
            " %sPercent=%.3g Elapsed=%.5g\n",
            total_files, total_dirs, desc,
            total_pages_in_core, total_pages, desc,
            total_pages_in_core_size, total_pages_size, desc,
            total_pages_in_core_perc, elapsed);
        }
    }

  return 0;
}