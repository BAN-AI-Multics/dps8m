/*
 * vim: filetype=c:tabstop=2:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: e39fb11a-ffab-11ef-b6d4-80ee73e9b8e7
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

/*
 * get_core_count - returns the number of physical cores (or zero on error).
 *
 * XXX: This works by using libhwloc (dynamically loading it at runtime).
 * Fallbacks might be implemented for common systems using other methods.
 *
 * Fully working on Haiku, Linux, FreeBSD, macOS, Solaris, and illumos.
 *
 * AIX seems to work but is not well tested.  The program MUST be compiled
 * with -D_THREAD_SAFE (e.g. `xlc_r`, `ibm-clang_r`) and -lpthread linked.
 * We check if it returns greater than 1 on AIX, otherwise assume an error.
 *
 * On NetBSD, the program MUST be linked with -lpthread (or else it crashes
 * at startup).  However, it still doesn't seem to actually work, so for
 * NetBSD -- for now -- we'll avoid potential problems and just return 0.
 *
 * Haiku's libhwloc port is deficient so we use the native API instead.
 *
 * Windows support is just returning 0 for now.  Cygwin remains untested.
 *
 * These functions are not intended to be thread-safe or reentrant and
 * should be called only from the main program thread once, preferably close
 * to program start-up.
 */

#include <ctype.h>
#if !defined(__MINGW32__) && !defined(__MINGW64__) && !defined(CROSS_MINGW32) && !defined(CROSS_MINGW64)
# include <dlfcn.h>
#endif
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "dps8_sir.h"

#if defined(__HAIKU__)
# include <OS.h>
#endif

#if !defined(CHAR_BIT)
# define CHAR_BIT 8
#endif

#if !defined(HAS_INCLUDE)
# if defined __has_include
#  define HAS_INCLUDE(inc) __has_include(inc)
# else
#  define HAS_INCLUDE(inc) 0
# endif
#endif

#if !defined(__HAIKU__)
# if HAS_INCLUDE(<elf.h>)
#  include <elf.h>
# endif
# if HAS_INCLUDE(<hwloc.h>)
#  include <hwloc.h>
# endif
#endif

#if !defined(HWLOC_OBJ_CORE)
# define HWLOC_OBJ_CORE 5
#endif

#if defined(EI_CLASS) && defined(EI_NIDENT) && defined(ELFCLASS32) && \
    defined(ELFCLASS64) && defined(ELFMAG) && defined(SELFMAG)
# define USE_ELF_H
#endif

#if defined(FREE)
# undef FREE
#endif /* if defined(FREE) */
#define FREE(p) do  \
  {                 \
    free((p));      \
    (p) = NULL;     \
  } while(0)

bool dps8_topo_used = false;

#if !defined(__HAIKU__)
static inline int
file_exists (const char *path)
{
  struct stat st;
  return 0 == stat(path, &st);
}
#endif

#if !defined(__HAIKU__)
static inline int
is_compatible_architecture (const char *path)
{
# if !defined(USE_ELF_H)
  (void)path;
  return 1;
# else
  FILE *f = fopen(path, "rb");
  if (NULL == f) {
#  if defined(TOPO_TESTING)
    (void)fprintf(stderr, "WARNING: Unable to open '%s'!\n", path);
#  endif
    return 0;
  }

  unsigned char e_ident[EI_NIDENT];
  if (EI_NIDENT != fread(e_ident, 1, EI_NIDENT, f)) {
#  if defined(TOPO_TESTING)
    (void)fprintf(stderr, "WARNING: Bad header in '%s'!\n", path);
#  endif
    fclose(f);
    return 0;
  }
  fclose(f);

  if (0 != memcmp(e_ident, ELFMAG, SELFMAG)) {
#  if defined(TOPO_TESTING)
    (void)fprintf(stderr, "WARNING: Bad header in '%s'!\n", path);
#  endif
    return 0;
  }

  const uint32_t bits = (int)sizeof(void *) * CHAR_BIT;

  if (64 == bits) {
    /* cppcheck-suppress arrayIndexOutOfBounds */
    if (EI_NIDENT >= EI_CLASS && ELFCLASS64 == e_ident[EI_CLASS]) { //-V560
#  if defined(TOPO_TESTING)
      (void)fprintf(stderr, "NOTICE: '%s' is valid 64-bit ELF.\n", path);
#  endif
      return 1;
    }
  } else if (32 == bits) {
    /* cppcheck-suppress arrayIndexOutOfBounds */
    if (EI_NIDENT >= EI_CLASS && ELFCLASS32 == e_ident[EI_CLASS]) { //-V560
#  if defined(TOPO_TESTING)
      (void)fprintf(stderr, "NOTICE: '%s' is valid 32-bit ELF.\n", path);
#  endif
      return 1;
    }
  }
#  if defined(TOPO_TESTING)
  (void)fprintf(stderr, "WARNING: '%s' could not be validated!\n", path);
#  endif
  return 0;
# endif
}
#endif

#if !defined(__HAIKU__)
static char **
get_candidate_lib_dirs (int32_t *n_dirs)
{
  size_t count = 0;
  char **dirs = NULL;
  const char *ld_path = getenv("LD_LIBRARY_PATH");

  if (ld_path && *ld_path) {
    char *copy = strdup(ld_path);
    if (NULL == copy) {
      /* cppcheck-suppress memleak */
      return NULL;
    }

    char *token = strtok(copy, ":");
    while (token) {
      count++;
      token = strtok(NULL, ":");
    }
    FREE(copy);
  }

  const char *defaults[] = {
      "/lib",
      "/lib64",
      "/usr/lib",
      "/usr/lib64",
      "/usr/lib/64",
      "/usr/local/lib",
      "/opt/lib",
      "/opt/local/lib",
      "/opt/freeware/lib",
      "/boot/system/lib",
      "/opt/hwloc/lib",
      "/usr/local/hwloc",
      "/home/linuxbrew/.linuxbrew/lib",
      "/usr/pkg/lib",
      "/usr/lib/32",
      "/usr/lib/aarch64-linux-gnu",
      "/usr/lib/arm-linux-gnueabihf",
  };

  uint32_t n_defaults = sizeof(defaults) / sizeof(defaults[0]);
  count += n_defaults;

  FILE *fp = fopen("/proc/self/maps", "r");
  char *temp_paths[SIR_MAXPATH];
  uint32_t temp_count = 0;
  if (fp) {
    char line[SIR_MAXPATH];
    while (fgets(line, sizeof(line), fp) && temp_count < SIR_MAXPATH) {
      char *path = strstr(line, "/");
      if (path && strstr(path, ".so")) {
        char *dir_end = strrchr(path, '/');
        if (dir_end) {
          *dir_end = '\0';
          int is_dup = 0;
          for (uint32_t i = 0; i < temp_count; i++) {
            if (0 == strcmp(temp_paths[i], path)) {
              is_dup = 1;
              break;
            }
          }
          if (!is_dup) {
            temp_paths[temp_count++] = strdup(path);
            if (NULL == temp_paths[temp_count - 1]) {
              for (uint32_t i = 0; i < temp_count - 1; i++)
                FREE(temp_paths[i]);
              fclose(fp);
              return NULL;
            }
          }
        }
      }
    }
    fclose(fp);
    count += temp_count;
  }

  dirs = malloc(sizeof(char *) * (count + 1));
  if (NULL == dirs) {
    for (uint32_t i = 0; i < temp_count; i++)
      FREE(temp_paths[i]);
    return NULL;
  }

  int32_t index = 0;

  if (ld_path && *ld_path) {
    char *copy = strdup(ld_path);
    if (NULL == copy) {
      FREE(dirs);
      for (uint32_t i = 0; i < temp_count; i++)
        FREE(temp_paths[i]);
      return NULL;
    }
    char *token = strtok(copy, ":");
    while (token) {
      dirs[index++] = strdup(token);
      if (NULL == dirs[index - 1]) {
        for (int32_t i = 0; i < index - 1; i++)
          FREE(dirs[i]);
        FREE(dirs);
        FREE(copy);
        for (uint32_t i = 0; i < temp_count; i++)
          FREE(temp_paths[i]);
        return NULL;
      }
      token = strtok(NULL, ":");
    }
    FREE(copy);
  }

  for (uint32_t i = 0; i < temp_count; i++) {
    dirs[index++] = temp_paths[i];
  }

  for (uint32_t i = 0; i < n_defaults; i++) {
    dirs[index++] = strdup(defaults[i]);
    if (NULL == dirs[index - 1]) {
      for (int32_t j = 0; j < index - 1; j++)
        FREE(dirs[j]);
      FREE(dirs);
      return NULL;
    }
  }

  dirs[index] = NULL;
  *n_dirs = index;
  return dirs;
}
#endif

#if !defined(__HAIKU__)
static inline void
free_candidate_lib_dirs (char **dirs)
{
  if (NULL == dirs)
    return;

  for (uint32_t i = 0; dirs[i] != NULL; i++)
    FREE(dirs[i]);

  FREE(dirs);
}
#endif

/* Find libhwloc */
#if !defined(__HAIKU__)
static char *
find_libhwloc_path (void)
{
  const char *candidates[] = {
      "libhwloc.so.15",
      "libhwloc.so.5",
      "libhwloc.so",
      "libhwloc.15.dylib",
      "libhwloc.5.dylib",
      "libhwloc.dylib",
      NULL
  };

  int32_t n_dirs = 0;
  char *found = NULL;
  char **dirs = get_candidate_lib_dirs(&n_dirs);

  if (NULL == dirs)
    return NULL;

  for (uint32_t i = 0; candidates[i] != NULL; i++) {
    for (uint32_t j = 0; j < n_dirs; j++) {
      char fullpath[SIR_MAXPATH];
      (void)snprintf(fullpath, sizeof(fullpath), "%s/%s", dirs[j], candidates[i]);
      if (file_exists(fullpath) && is_compatible_architecture(fullpath)) {
        found = strdup(fullpath);
        break;
      }
    }
    if (NULL != found)
      break;
  }

  free_candidate_lib_dirs(dirs);
  return found;
}
#endif

/* libhwloc */
typedef void *dl_hwloc_topology_t;
typedef int (*fp_hwloc_topology_init) (dl_hwloc_topology_t *);
typedef int (*fp_hwloc_topology_load) (dl_hwloc_topology_t);
typedef void (*fp_hwloc_topology_destroy) (dl_hwloc_topology_t);
typedef int (*fp_hwloc_get_type_depth) (dl_hwloc_topology_t, int);
typedef unsigned (*fp_hwloc_get_nbobjs_by_depth) (dl_hwloc_topology_t, int);

/* get_core_count */
uint32_t
get_core_count(void)
{
  static uint32_t num_cores = 0;
#if defined(__HAIKU__)
  const uint32_t maxnodes = 1024;

  dps8_topo_used = true;
  cpu_topology_node_info* topology = malloc(sizeof(cpu_topology_node_info) * maxnodes);
  if (NULL == topology)
    return 0; /* Memory allocation failure */

  uint32_t nodecount = maxnodes;
  status_t status = get_cpu_topology_info(topology, &nodecount);

  if (B_OK == status) {
    for (uint32_t i = 0; i < nodecount; i++) {
      if (B_TOPOLOGY_CORE == topology[i].type)
        num_cores++;
    }
    return num_cores;
  } else {
    return 0;
  }

  FREE(topology);
#elif defined(__MINGW32__) || defined(__MINGW64__) || defined(CROSS_MINGW32) || defined(CROSS_MINGW64)
  dps8_topo_used = true;
  return 0;
#elif defined(__NetBSD__)
  dps8_topo_used = true;
  return 0;
#else
  char *lib_path = find_libhwloc_path();

  if (NULL == lib_path)
    return 0; /* Could not find usable libhwloc */

  void *handle = dlopen(lib_path, RTLD_LAZY);
  FREE(lib_path);
  if (NULL == handle) {
# if defined(TOPO_TESTING)
    char *dl_error = dlerror();
    if (dl_error && *dl_error)
      (void)fprintf(stderr, "ERROR: dlopen: %s\n", dl_error);
# endif
    return 0;
  }
  (void)dlerror();

  dps8_topo_used = true;

  fp_hwloc_topology_init hwloc_topology_init =
    (fp_hwloc_topology_init)dlsym(handle, "hwloc_topology_init");
  if (NULL == hwloc_topology_init) {
# if defined(TOPO_TESTING)
    char *dl_error = dlerror();
    if (dl_error && *dl_error)
      (void)fprintf(stderr, "ERROR: no hwloc_topology_init: %s\n", dl_error);
# endif
    (void)dlclose(handle);
    return 0;
  }

  fp_hwloc_topology_load hwloc_topology_load =
    (fp_hwloc_topology_load)dlsym(handle, "hwloc_topology_load");
  if (NULL == hwloc_topology_load) {
# if defined(TOPO_TESTING)
    char *dl_error = dlerror();
    if (dl_error && *dl_error)
      (void)fprintf(stderr, "ERROR: no hwloc_topology_load: %s\n", dl_error);
# endif
    (void)dlclose(handle);
    return 0;
  }

  fp_hwloc_topology_destroy hwloc_topology_destroy =
    (fp_hwloc_topology_destroy)dlsym(handle, "hwloc_topology_destroy");
  if (NULL == hwloc_topology_destroy) {
# if defined(TOPO_TESTING)
    char *dl_error = dlerror();
    if (dl_error && *dl_error)
      (void)fprintf(stderr, "ERROR: no hwloc_topology_destroy: %s\n", dl_error);
# endif
    (void)dlclose(handle);
    return 0;
  }

  fp_hwloc_get_type_depth hwloc_get_type_depth =
    (fp_hwloc_get_type_depth)dlsym(handle, "hwloc_get_type_depth");
  if (NULL == hwloc_get_type_depth) {
# if defined(TOPO_TESTING)
    char *dl_error = dlerror();
    if (dl_error && *dl_error)
      (void)fprintf(stderr, "ERROR: no hwloc_get_type_depth: %s\n", dl_error);
# endif
    (void)dlclose(handle);
    return 0;
  }

  fp_hwloc_get_nbobjs_by_depth hwloc_get_nbobjs_by_depth =
    (fp_hwloc_get_nbobjs_by_depth)dlsym(handle, "hwloc_get_nbobjs_by_depth");
  if (NULL == hwloc_get_nbobjs_by_depth) {
# if defined(TOPO_TESTING)
    char *dl_error = dlerror();
    if (dl_error && *dl_error)
      (void)fprintf(stderr, "ERROR: no hwloc_get_nbobjs_by_depth: %s\n", dl_error);
# endif
    (void)dlclose(handle);
    return 0;
  }

  dl_hwloc_topology_t topology;
  if (0 != hwloc_topology_init(&topology)) {
# if defined(TOPO_TESTING)
    (void)fprintf(stderr, "ERROR: hwloc_topology_init failure.\n");
# endif
    (void)dlclose(handle);
    return 0;
  }

  if (0 != hwloc_topology_load(topology)) {
# if defined(TOPO_TESTING)
    (void)fprintf(stderr, "ERROR: hwloc_topology_load failure.\n");
    hwloc_topology_destroy(topology);
# endif
    (void)dlclose(handle);
    return 0;
  }

  int32_t core_depth = hwloc_get_type_depth(topology, HWLOC_OBJ_CORE);
  if (-1 == core_depth) {
# if defined(TOPO_TESTING)
    (void)fprintf(stderr, "ERROR: hwloc_get_type_depth failure.\n");
    hwloc_topology_destroy(topology);
# endif
    (void)dlclose(handle);
    return 0;
  }

  num_cores = hwloc_get_nbobjs_by_depth(topology, core_depth);
  hwloc_topology_destroy(topology);
  (void)dlclose(handle);
#endif
#if defined(_AIX)
  if (num_cores < 2)
    return 0;
#endif
  return num_cores;
}
