/*
 * sim_hints.c: configuration hints
 *
 * vim: filetype=c:tabstop=2:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 1784dba8-00a2-11f0-b624-80ee73e9b8e7
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

#include "sim_defs.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32)
# include <sys/resource.h>
#endif
#include <time.h>
#include <unistd.h>
#if defined(__APPLE__)
# include <sys/types.h>
# include <sys/sysctl.h>
#endif

#include "linehistory.h"

#include "../dps8/dps8.h"
#include "../dps8/dps8_cpu.h"
#include "../dps8/dps8_topo.h"

/*
 * We may run in early startup (before logging has been initialized),
 * so only use non-logging libsir calls from functions in this file.
 */

#include "../dps8/dps8_sir.h"

#if !defined(HAS_INCLUDE)
# if defined __has_include
#  define HAS_INCLUDE(inc) __has_include(inc)
# else
#  define HAS_INCLUDE(inc) 0
# endif
#endif

#if defined(__linux__)
# if HAS_INCLUDE(<linux/capability.h>)
#  include <linux/capability.h>
# endif
# if !defined(CAP_SYS_NICE)
#  define CAP_SYS_NICE 23
# endif
# if !defined(CAP_IPC_LOCK)
#  define CAP_IPC_LOCK 14
# endif
#endif

#if !defined(CHAR_BIT)
# define CHAR_BIT 8
#endif

unsigned int hint_count = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline void
sim_hrline(void)
{
  sim_printf("\r\n------------------------------------------------------------------------------\r\n");
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(__linux__)
static int
has_linux_capability(const pid_t pid, const int capability)
{
  char filename[SIR_MAXPATH];
  snprintf(filename, sizeof(filename), "/proc/%d/status", pid);

  FILE *file = fopen(filename, "r");
  if (!file)
    return -1;

  char line[1024];
  uint64_t cap_eff = 0;

  while (fgets(line, sizeof(line), file))
    if (1 == sscanf(line, "CapEff: %llx", (long long unsigned int*)&cap_eff))
      break;

  fclose(file);

  return 0 != (cap_eff & (1ULL << capability));
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int
check_cpu_frequencies (void)
{
#if !defined(__linux__)
  return 0;
#else
  struct dirent *entry;
  char path[SIR_MAXPATH];
  FILE *file;
  int min_freq = 0, max_freq = 0;
  int mismatch = 0;

  DIR *dir = opendir ("/sys/devices/system/cpu");

  if (!dir)
    return 0;

  while ((entry = readdir (dir)) != NULL) {
    if (DT_DIR == entry->d_type && 0 == strncmp (entry->d_name, "cpu", 3)
        && isdigit (entry->d_name[3])) {
      snprintf (path, sizeof (path), "/sys/devices/system/cpu/%s/cpufreq/cpuinfo_min_freq",
                entry->d_name);
      file = fopen (path, "r");
      if (file) {
        fscanf (file, "%d", &min_freq);
        fclose (file);
      }
      snprintf (path, sizeof (path), "/sys/devices/system/cpu/%s/cpufreq/cpuinfo_max_freq",
                entry->d_name);
      file = fopen (path, "r");
      if (file) {
        fscanf (file, "%d", &max_freq);
        fclose (file);
      }
      if (min_freq != max_freq) {
        mismatch = 1;
      }
    }
  }
  closedir (dir);

  if (mismatch) {
    return 1;
  } else {
    return 0;
  }
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define RPI_TEXT "Raspberry Pi"

static int
is_raspberry_pi(void)
{
  FILE *file;
  char line[1024];
  int is_pi = 0;

  file = fopen("/proc/device-tree/model", "r");
  if (file) {
    if (fgets(line, sizeof(line), file) && strstr(line, RPI_TEXT))
      is_pi = 1;
    fclose(file);
    if (is_pi)
      return 1;
  }

  file = fopen("/proc/cpuinfo", "r");
  if (!file)
    return 0;

  while (fgets(line, sizeof(line), file)) {
    if (strncmp(line, "Model", 5) == 0 && strstr(line, RPI_TEXT)) {
      is_pi = 1;
      break;
    }
  }
  fclose(file);

  return is_pi;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static uint32_t
check_pi_issues(void)
{
  uint32_t a_issues = 0;

  FILE *fp = popen("vcgencmd get_throttled 2> /dev/null", "r");
  if (!fp)
    return a_issues;

  char buffer[1024];
  if (NULL == fgets(buffer, sizeof(buffer), fp)) {
    pclose(fp);
    return a_issues;
  }
  pclose(fp);

  char *p = strstr(buffer, "0x");
  if (!p)
    return a_issues;

  uint32_t throttle = (uint32_t)strtoul(p, NULL, 16);
  uint32_t c_issues = throttle & 0xF;
  uint32_t p_issues = (throttle >> 16) & 0xF;
  a_issues = c_issues | p_issues;

  return a_issues;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static uint32_t
check_scaling_governors (void)
{
#if !defined(__linux__)
  return 0;
#else
  struct dirent *entry;
  char path[SIR_MAXPATH];
  FILE *file;
  char governor[256];
  static uint32_t bad_govs;

  DIR *dir = opendir ("/sys/devices/system/cpu");

  if (!dir)
    return 0;

  bad_govs = 0;
  while ((entry = readdir (dir)) != NULL) {
    if (DT_DIR == entry->d_type && 0 == strncmp (entry->d_name, "cpu", 3)
        && isdigit (entry->d_name[3])) {
      snprintf (path, sizeof (path), "/sys/devices/system/cpu/%s/cpufreq/scaling_governor",
                entry->d_name);
      file = fopen (path, "r");
      if (file) {
        if (fgets (governor, sizeof (governor), file)) {
          governor[strcspn (governor, "\n")] = '\0';
          if ( 0 == strcmp (governor, "powersave" )
            || 0 == strcmp (governor, "conservative") ) {
            bad_govs++;
          }
        }
        fclose (file);
      }
    }
  }
  closedir (dir);

  return bad_govs;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

const char *
atm_cwords (int count)
{
  static const char *words[] = { "Zero", "One", "Two", "Three", "Four", "Five", "Six" };
  static char buffer[20];

  if (count >= 0 && count <= 6) {
    return words[count];
  } else {
    snprintf (buffer, sizeof (buffer), "%d", count);
    return buffer;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(__APPLE__)
static int
processIsTranslated(void)
{
  int ret = 0;
  size_t size = sizeof(ret);
  if (sysctlbyname("sysctl.proc_translated", &ret, &size, NULL, 0) == -1) {
    if (errno == ENOENT)
      return 0;
    return -1;
  }
  return ret;
}
#endif /* if defined(_APPLE_) */

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

t_stat
show_hints (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
  /* NOTE: We override the use of flag to enable early startup mode */

  hint_count = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* HINT: Check if time_t is less than 64-bit and recommend mitigations. */

  if (64 > (sizeof (time_t) * CHAR_BIT)) {
    if (!flag) {
      sim_hrline ();
      sim_printf ("\r\n* Hint #%u - LESS THAN 64-BIT TIME REPRESENTATION DETECTED\r\n", ++hint_count);
      sim_printf ("\r\n");
      sim_printf ("  The simulator has detected timekeeping using a %ld-bit representation.\r\n",
                  (long)(sizeof (time_t) * CHAR_BIT));
    } else {
      ++hint_count;
    }
  }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* HINT: Check if running as root and recommend not doing that. */

#if !defined(__HAIKU__) && !defined(_WIN32)
  if (0 == geteuid ()) {
    if (!flag) {
      sim_hrline ();
      sim_printf ("\r\n* Hint #%u - SIMULATOR RUNNING AS ROOT OR SUPERUSER DETECTED\r\n", ++hint_count);
      sim_printf ("\r\n");
      sim_printf ("  You are running the simulator as `root` (or equivalent superuser).\r\n");
      sim_printf ("\r\n");
      sim_printf ("  We highly recommend running the simulator as a non-privileged user.\r\n");
    } else {
      ++hint_count;
    }
  }
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* HINT: Check for TESTING build and warn about it. */

#if defined(TESTING)
  if (!flag) {
    sim_hrline ();
    sim_printf ("\r\n* Hint #%u - TESTING BUILD DETECTED\r\n", ++hint_count);
    sim_printf ("\r\n");
    sim_printf ("  You are running a TESTING build.\r\n");
    sim_printf ("\r\n");
    sim_printf ("  TESTING builds intended strictly for development purposes.\r\n");
    sim_printf ("\r\n");
    sim_printf ("  Reliability, stability, and performance will be adversely impacted.\r\n");
  } else {
    ++hint_count;
  }
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* HINT: Check if we are running with unsupported or experimental build options enabled. */

#if defined(WITH_ABSI_DEV) || defined(WITH_MGP_DEV)
  if (!flag) {
    sim_hrline ();
    sim_printf ("\r\n* Hint #%u - UNSUPPORTED OR EXPERIMENTAL BUILD OPTIONS DETECTED\r\n", ++hint_count);
    sim_printf ("\r\n");
    sim_printf ("  You are running a build with unsupported or experimental options enabled:\r\n");
    sim_printf ("\r\n");
# if defined(WITH_ABSI_DEV)
    sim_printf ("  * WITH_ABSI_DEV is enabled.\r\n");
# endif
# if defined(WITH_MGP_DEV)
    sim_printf ("  * WITH_MGP_DEV is enabled.\r\n");
# endif
    sim_printf ("\r\n");
    sim_printf ("  Reliability, stability, and performance will be adversely impacted.\r\n");
  } else {
    ++hint_count;
  }
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* HINT: Check if we are running on Cygwin, and recommend using a native build instead. */

#if defined(__CYGWIN__)
  if (!flag) {
    sim_hrline ();
    sim_printf ("\r\n* Hint #%u - CYGWIN WINDOWS BUILD DETECTED\r\n", ++hint_count);
    sim_printf ("\r\n");
    sim_printf ("  You are using a Cygwin Windows build.\r\n");
    sim_printf ("\r\n");
    sim_printf ("  The simulator supports native Windows builds (using the MinGW toolchain).\r\n");
    sim_printf ("  Unless you have a specific reason for using a Cygwin build, we recommend\r\n");
    sim_printf ("  using a native build instead, which has better performance for most users.\r\n");
  } else {
    ++hint_count;
  }
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* HINT: Check that we are not using a power-saving CPU governor configuration. */

  if (check_scaling_governors ()) {
    if (!flag) {
      unsigned long long reduced = (unsigned long long)check_scaling_governors();
      unsigned long long total = (unsigned long long)nprocs;
      sim_hrline ();
      sim_printf ("\r\n* Hint #%u - POWER-SAVING CPU GOVERNOR CONFIGURATION DETECTED\r\n", ++hint_count);
      sim_printf ("\r\n");
      if (total > reduced)
        sim_printf("  You have %llu (out of %llu) processors using a power-saving governor.\r\n",
                   reduced, total);
      else
        sim_printf("  You have %llu logical processors configured using a power-saving governor.\r\n",
                   reduced);
      sim_printf ("\r\n");
      sim_printf ("  To enable the `performance` governor, run the following shell command:\r\n");
      sim_printf ("\r\n");
      sim_printf ("    echo performance | \\\r\n");
      sim_printf ("      sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor\r\n");
    } else {
      ++hint_count;
    }
  }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* HINT: Check if we are running a threaded simulator on a uniprocessor system. */

#if defined(LOCKLESS)
  if (nprocs < 2 && ncores < 2) {
    if (!flag) {
      sim_hrline ();
      sim_printf ("\r\n* Hint #%u - UNIPROCESSOR HOST SYSTEM DETECTED\r\n", ++hint_count);
      sim_printf ("\r\n");
      sim_printf ("  You seem to be running the simulator on a uniprocessor host system.\r\n");
      sim_printf ("\r\n");
      sim_printf ("  The simulator supports a single-threaded build option (`NO_LOCKLESS`)\r\n");
      sim_printf ("  which may perform better on uniprocessor systems such as this one.\r\n");
    } else {
      ++hint_count;
    }
  }
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* HINT: Check for deficient TERM values. */

#if defined(HAVE_LINEHISTORY) && !defined(_WIN32)
  const char *term = getenv ("TERM");
  if (NULL == term || 0 == strcmp (term, "dumb") || 0 == strcmp (term, "") || 0 == strcmp (term, "cons25")) {
    if (!flag) {
      sim_hrline ();
      sim_printf ("\r\n* Hint #%u - TERM ENVIRONMENT VARIABLE SET TO UNSUPPORTED VALUE\r\n", ++hint_count);
      sim_printf ("\r\n");
      sim_printf ("  The TERM environment variable is unset (or is set to an unsupported value).\r\n");
      sim_printf ("\r\n");
      sim_printf ("  This disables the line-editing and history recall functionality of the\r\n");
      sim_printf ("  simulator shell.  This is usually an unintentional misconfiguration.\r\n");
    } else {
      ++hint_count;
    }
  }
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* HINT: Check that core dumps have not been limited or disabled. */

#if !defined(_WIN32)
  struct rlimit core_limit;
  if (getrlimit(RLIMIT_CORE, &core_limit) == 0) {
    if (core_limit.rlim_cur == 0) {
      if (!flag) {
        sim_hrline();
        sim_printf("\r\n* Hint #%u - CORE DUMPS ARE DISABLED\r\n", ++hint_count);
        sim_printf("\r\n");
        sim_printf("  Core dumps are disabled.  If the simulator crashes, it will not be able\r\n");
        sim_printf("  to produce a core file, which can help determine the cause of the crash.\r\n");
        sim_printf("\r\n");
        sim_printf("  Try setting `ulimit -c unlimited` (or sometimes `ulimit -Hc unlimited`)\r\n");
        sim_printf("  in your shell to remove this restriction.\r\n");
      } else {
        ++hint_count;
      }
    }
    else if (core_limit.rlim_cur != RLIM_INFINITY) {
      if (!flag) {
        sim_hrline();
        sim_printf("\r\n* Hint #%u - CORE DUMPS ARE RESTRICTED\r\n", ++hint_count);
        sim_printf("\r\n");
        sim_printf("  Core dumps are restricted.  If the simulator crashes, it may not be able\r\n");
        sim_printf("  to produce a core file, which can help determine the cause of the crash.\r\n");
        sim_printf("\r\n");
        sim_printf("  Your configuration is currently restricting core dumps to %llu KB.\r\n",
                   (unsigned long long)core_limit.rlim_cur / 1024);
        sim_printf("\r\n");
        sim_printf("  Try setting `ulimit -c unlimited` (or sometimes `ulimit -Hc unlimited`)\r\n");
        sim_printf("  in your shell to remove this restriction.\r\n");
      } else {
        ++hint_count;
      }
    }
  }
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* HINT: Check if running on a suboptimally configured Raspberry Pi board. */

  if (is_raspberry_pi ()) {
    uint32_t a_issues;
    if (check_cpu_frequencies()) {
      if (!flag) {
        sim_hrline ();
        sim_printf ("\r\n* Hint #%u - RASPBERRY PI FIXED FREQUENCY CONFIGURATION\r\n", ++hint_count);
        sim_printf ("\r\n");
        sim_printf ("  The simulator has detected it is running on a Raspberry Pi single-board\r\n");
        sim_printf ("  computer, which is not configured for fixed frequency operation.\r\n");
        sim_printf ("  Using a fixed-frequency configuration allows for more deterministic\r\n");
        sim_printf ("  response times and enhances performance and simulation fidelity.\r\n");
        sim_printf ("\r\n");
        sim_printf ("  Use `cpupower frequency-info` (from the `linux-cpupower` package) to view\r\n");
        sim_printf ("  the current configuration, or `sudo cpupower frequency-set -f` to change\r\n");
        sim_printf ("  it, for example, `sudo cpupower frequency-set -f 1200Mhz`.  Refer to the\r\n");
        sim_printf ("  documentation for your specific Raspberry Pi model for proper values.\r\n");
      } else {
        ++hint_count;
      }
    }
    a_issues = check_pi_issues ();
    if ( a_issues & (1 << 0) || a_issues & (1 << 1)
      || a_issues & (1 << 2) || a_issues & (1 << 3) ) {
      if (!flag) {
        sim_hrline ();
        sim_printf ("\r\n* Hint #%u - RASPBERRY PI ADVERSE HARDWARE EVENTS RECORDED\r\n", ++hint_count);
        sim_printf ("\r\n");
        sim_printf ("  Your Raspberry Pi has recorded the following adverse hardware event(s):\r\n");
        sim_printf ("\r\n");
        if (a_issues & (1 << 0)) {
          sim_printf("    * Under-voltage events have occurred since boot.\r\n");
        }
        if (a_issues & (1 << 1)) {
          sim_printf("    * CPU frequency capping events have occurred since boot.\r\n");
        }
        if (a_issues & (1 << 2)) {
          sim_printf("    * Thermal throttling events have occurred since boot.\r\n");
        }
        if (a_issues & (1 << 3)) {
          sim_printf("    * Soft temperature limits have been reached or exceeded since boot.\r\n");
        }
      } else {
        ++hint_count;
      }
    }
  }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* HINT: Check if topology information was available, recommend installing libhwloc if not. */

#if !defined(_WIN32)
  if (1 < nprocs && false == dps8_topo_used) {
    if (!flag) {
      sim_hrline ();
      sim_printf ("\r\n* Hint #%u - UNABLE TO DETERMINE SYSTEM TOPOLOGY USING LIBHWLOC\r\n", ++hint_count);
      sim_printf ("\r\n");
      sim_printf ("  No overall topology information could be determined for this system.\r\n");
      sim_printf ("  The simulator is aware this system has %llu logical processors available,\r\n",
                  (unsigned long long)nprocs);
      sim_printf ("  but it was not able to determine the number of actual physical cores.\r\n");
      sim_printf ("\r\n");
      sim_printf ("  The simulator uses topology information to optimize performance and\r\n");
      sim_printf ("  to warn about potentially dangerous or suboptimal configurations.\r\n");
      sim_printf ("  We use libhwloc (https://www-lb.open-mpi.org/projects/hwloc/) to query\r\n");
      sim_printf ("  this information in an architecture and operating system agnostic way.\r\n");
# if defined(__APPLE__)
      sim_printf ("\r\n");
      sim_printf ("  On macOS, you can install the libhwloc library using the Homebrew\r\n");
      sim_printf ("  package manager (https://brew.sh/) by running `brew install hwloc`.\r\n");
# elif defined(__FreeBSD__)
      sim_printf ("\r\n");
      sim_printf ("  On FreeBSD, you can install the hwloc from packages (`pkg install hwloc`)\r\n");
      sim_printf ("  or build from ports (`cd /usr/ports/devel/hwloc/ && make install clean`).\r\n");
# elif defined(__linux__)
      sim_printf ("\r\n");
      sim_printf ("  Linux distributions usually provide this library as the `hwloc` package.\r\n");
# endif
      sim_printf ("\r\n");
      sim_printf ("  No recompilation of the simulator is necessary for it to use libhwloc.\r\n");
    } else {
      ++hint_count;
    }
  }
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* HINT: On Linux, check if we have real-time is allowed via capabilities. */

#if defined(__linux__) && defined(CAP_SYS_NICE)
  const pid_t pid = _sir_getpid();
  const int cap_sys_nice = CAP_SYS_NICE;

  if (nprocs > 1 && !has_linux_capability(pid, cap_sys_nice)) {
    if (!flag) {
      const char * setcap_filename = _sir_getappfilename();
      sim_hrline ();
      sim_printf ("\r\n* Hint #%u - LINUX REAL-TIME SCHEDULING CAPABILITY IS NOT ENABLED\r\n", ++hint_count);
      sim_printf ("\r\n");
      sim_printf ("  Linux real-time scheduling capability is not enabled.\r\n");
      sim_printf ("  Real-time support allows for accurate and deterministic response times\r\n");
      sim_printf ("  and improved thread scheduling behavior, enhancing simulation fidelity.\r\n");
      if (nprocs > 2) {
        if ((unsigned long long)ncores > 1 && (unsigned long long)ncores != (unsigned long long)nprocs) {
          sim_printf("  Since this system seems to have %llu logical (and %llu physical) processors,\r\n",
                     (unsigned long long)nprocs, (unsigned long long)ncores);
        } else {
          sim_printf("  Since this system seems to have at least %llu logical processors available,\r\n",
                   (unsigned long long)nprocs);
        }
        sim_printf ("  the real-time mode (requested by the `-p` argument) may be considered.\r\n");
      }
      sim_printf ("\r\n");
      sim_printf ("  To enable the real-time capability, run the following shell command:\r\n");
      sim_printf ("\r\n");
      sim_printf ("    sudo setcap 'cap_sys_nice,cap_ipc_lock+ep' %s\r\n", setcap_filename ? setcap_filename : "dps8");
    } else {
      ++hint_count;
    }
  }
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* HINT: Check if mlock failed to work and tell the user how to fix it. */

  if (true == mlock_failure) {
    if (!flag) {
      sim_hrline ();
      sim_printf ("\r\n* Hint #%u - UNABLE TO LOCK SIMULATOR MEMORY WITH MLOCK()\r\n", ++hint_count);
      sim_printf ("\r\n");
      sim_printf ("  The simulator attempted, but failed, to use memory locking with mlock().\r\n");
      sim_printf ("  Memory locking prevents the simulated memory from being swapped out to\r\n");
      sim_printf ("  disk.  This avoids page faults and enables more deterministic performance.\r\n");
      sim_printf ("  Memory locking also enhances response times for real-time mode operation.\r\n");
#if defined(__linux__) && defined(CAP_IPC_LOCK)
      const pid_t pid = _sir_getpid();
      const int cap_ipc_lock = CAP_IPC_LOCK;
      if (!has_linux_capability(pid, cap_ipc_lock)) {
        const char * setcap_filename = _sir_getappfilename();
        sim_printf ("\r\n");
        sim_printf ("  You can enable real-time and memory locking by running this command:\r\n");
        sim_printf ("\r\n");
        sim_printf ("    sudo setcap 'cap_sys_nice,cap_ipc_lock+ep' %s\r\n",
                    setcap_filename ? setcap_filename : "dps8");
      }
#elif defined(__sun) || defined(__illumos__)
      const char * setcap_filename = _sir_getappfilename();
      sim_printf ("\r\n");
      sim_printf ("  You can allow memory locking by running the following shell commands:\r\n");
      sim_printf ("\r\n");
      sim_printf ("    sudo chown root:root %s\r\n", setcap_filename ? setcap_filename : "dps8");
      sim_printf ("    sudo chmod u+s %s\r\n", setcap_filename ? setcap_filename : "dps8");
      sim_printf ("\r\n");
      sim_printf ("  This is safe - we drop all unnecessary privileges immediately at start-up.\r\n");
#else
      sim_printf("\r\n");
      sim_printf("  You can check `ulimit -l` in your shell to verify locking is unrestricted.\r\n");
#endif
    } else {
      ++hint_count;
    }
  }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* HINT: Check if atomic types are not lock-free and warn the user about it. */

#if !defined(__SUNPRO_C) && !defined(__SUNPRO_CC)
# define CHECK_LOCK_FREE(var, type_name, counter, names, index) \
   do {                                                         \
     if (!atomic_is_lock_free (&var)) {                         \
       names[index] = type_name;                                \
       index++;                                                 \
       counter++;                                               \
     }                                                          \
   } while (0)

  atomic_uint_fast32_t af32_t = 0;
  atomic_size_t asize_t       = 0;
  atomic_bool abool           = 0;
  atomic_int aint             = 0;
  atomic_long along           = 0;
  int *naptr;
  atomic_uintptr_t aptr;

  atomic_init (&aptr, (uintptr_t)&naptr);
  (void)naptr;

  const char *non_lock_free_names[6];
  int non_lock_free_count = 0;
  int index = 0;

  CHECK_LOCK_FREE (af32_t, "atomic_uint_fast32_t",
                   non_lock_free_count, non_lock_free_names, index);
  CHECK_LOCK_FREE (asize_t, "atomic_size_t",
                   non_lock_free_count, non_lock_free_names, index);
  CHECK_LOCK_FREE (abool, "atomic_bool",
                   non_lock_free_count, non_lock_free_names, index);
  CHECK_LOCK_FREE (aint, "atomic_int",
                   non_lock_free_count, non_lock_free_names, index);
  CHECK_LOCK_FREE (along, "atomic_long",
                   non_lock_free_count, non_lock_free_names, index);
  CHECK_LOCK_FREE (aptr, "atomic_uintptr_t",
                   non_lock_free_count, non_lock_free_names, index);

  if (non_lock_free_count > 0) {
    if (!flag) {
      const char *count_str = atm_cwords (non_lock_free_count);
      sim_hrline ();
      sim_printf ("\r\n* Hint #%u - ATOMIC TYPES DO NOT SUPPORT LOCK-FREE OPERATION\r\n", ++hint_count);
      sim_printf ("\r\n  %s atomic type%s %s not lock-free on this system:\r\n\r\n",
                  count_str, (non_lock_free_count == 1) ? "" : "s",
                  (non_lock_free_count == 1) ? "is" : "are");
      for (unsigned int i = 0; i < non_lock_free_count; i++)
        sim_printf ("    * %s\r\n", non_lock_free_names[i]);
      sim_printf ("\r\n");
      sim_printf ("  Atomic operations are lock-free only if the CPU provides the appropriate\r\n");
      sim_printf ("  instructions and your compiler knows how to take advantage of them.\r\n");
      sim_printf ("\r\n");
      sim_printf ("  You could try upgrading the compiler, but this is usually a hardware\r\n");
      sim_printf ("  limitation.  When the CPU does not support atomic operations, they rely\r\n");
      sim_printf ("  on locking, which adversely impacts the performance of threaded programs.\r\n");
      if (1 == nprocs) {
        sim_printf ("\r\n");
        sim_printf ("  Since it seems this system only has a single processor, you should try\r\n");
        sim_printf ("  building the `NO_LOCKLESS` non-threaded simulator for better performance.\r\n");
      }
    } else {
      ++hint_count;
    }
  }
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(__APPLE__)
  int isRosetta = processIsTranslated();
  if (1 == isRosetta) {
    if (!flag) {
      sim_hrline ();
      sim_printf ("\r\n* Hint #%u - APPLE ROSETTA BINARY TRANSLATION DETECTED\r\n", ++hint_count);
      sim_printf ("\r\n  Rosetta is an x86_64 to ARM64 (Apple Silicon) binary translation process.\r\n");
      sim_printf ("\r\n");
      sim_printf ("  This means that the simulator binary you are executing was not compiled\r\n");
      sim_printf ("  for the native architecture of your system.  Although fully functional\r\n");
      sim_printf ("  performance will be significantly reduced.  It is highly recommended to\r\n");
      sim_printf ("  use a native Apple Silicon (ARM64) build of the simulator.\r\n");
    } else {
      ++hint_count;
    }
  }
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

  if (hint_count) {
    if (!flag) {
      sim_hrline ();
      if (getenv("DPS8M_NO_HINTS") == NULL) {
        sim_printf ("\r\n");
        sim_printf ("To disable hint notifications, set the environment variable `DPS8M_NO_HINTS=1`.\r\n");
      }
      sim_printf ("\r\n");
    } else {
      sim_printf("There %s %d %s available; use \"%s\" to view %s.\r\n\r\n",
                 (hint_count == 1) ? "is" : "are", hint_count, (hint_count == 1) ? "hint" : "hints",
                 (hint_count == 1) ? "SHOW HINT" : "SHOW HINTS", (hint_count == 1) ? "it" : "them");
    }
  } else {
    if (!flag) {
      sim_printf ("No configuration hints currently available.\r\n");
    }
  }

  return 0;
}
