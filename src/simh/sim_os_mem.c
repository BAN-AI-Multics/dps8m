/*
 * vim: filetype=c:tabstop=2:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: f59cc352-0209-11f0-af38-80ee73e9b8e7
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
 * Memory availability, checked in the following order:
 *
 * 1. libuv uv_get_available_memory
 * 2. libuv uv_get_free_memory
 * 3. Linux "available" memory
 * 4. Linux "free+buffer+cache" memory
 * 5. Linux "committable" memory
 * 6. Zero
 */

#include <errno.h>
#include <stdbool.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#define MEM_BUFFER_SIZE 4096
#define UV_VERSION(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

static uint64_t
memory_available_uv(void) {
  uint64_t available_memory_b = 0;

#if UV_VERSION(1, 45, 0) <= UV_VERSION_HEX
  available_memory_b = uv_get_available_memory();
#endif
  if (0 == available_memory_b)
    available_memory_b = uv_get_free_memory();

  return available_memory_b;
}

static int
mem_parse_value(const char *line, const char *label, uint64_t *value)
{
  if (!line || !label || !value)
    return -1;

  size_t label_len = strlen(label);
  if (0 != strncmp(line, label, label_len))
    return -1;

  const char *ptr = line + label_len;
  while (' ' == *ptr || '\t' == *ptr)
    ptr++;

  char *endptr;
  errno = 0;
  *value = strtoull(ptr, &endptr, 10) * 1024;
  if (ERANGE == errno || ULLONG_MAX == *value || ptr == endptr)
    return -1;

  while (' ' == *endptr || '\t' == *endptr)
    endptr++;
  if (0 != strncmp(endptr, "kB", 2) || 3 < strlen(endptr))
    return -1;

  return 0;
}

uint64_t
sim_memory_available(void) {
  FILE *fp = fopen("/proc/meminfo", "r");
  uint64_t uv_usable_memory = memory_available_uv();

  if (!fp)
    return uv_usable_memory;

  char buffer[MEM_BUFFER_SIZE];
  uint64_t commit_limit = 0, mem_free = 0, buffers = 0, cached = 0, mem_available = 0;
  bool found_commit = false, found_free = false, found_buffers = false, found_cached = false, found_available = false;

  while (fgets(buffer, sizeof(buffer), fp)) {
    buffer[sizeof(buffer) - 1] = '\0';

    if (false == found_commit && 0 == mem_parse_value(buffer, "CommitLimit:", &commit_limit))
      found_commit = true;
    else if (false == found_free && 0 == mem_parse_value(buffer, "MemFree:", &mem_free))
      found_free = true;
    else if (false == found_buffers && 0 == mem_parse_value(buffer, "Buffers:", &buffers))
      found_buffers = true;
    else if (false == found_cached && 0 == mem_parse_value(buffer, "Cached:", &cached))
      found_cached = true;
    else if (false == found_available && 0 == mem_parse_value(buffer, "MemAvailable:", &mem_available))
      found_available = true;

    if (found_commit && found_free && found_buffers && found_cached && found_available)
      break;
  }

  (void)fclose(fp);

  uint64_t linux_commit_memory = found_commit ? commit_limit : 0;
  uint64_t linux_freebufcache_memory = (found_free && found_buffers && found_cached) ?
                                       (mem_free + buffers + cached) : 0;
  uint64_t linux_available_memory = found_available ? mem_available : 0;

  if (uv_usable_memory)
    return uv_usable_memory;

  if (linux_available_memory)
    return linux_available_memory;

  if (linux_freebufcache_memory)
    return linux_freebufcache_memory;

  if (linux_commit_memory)
    return linux_commit_memory;

  return 0;
}
