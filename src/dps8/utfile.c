/*
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 */

// This file contains file manipulation utilities.

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


#define MAX_MKSTEMPS_TRIES 10000

static char valid_file_name_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

/* 
   This is a replacement for the mkstemps function which is not available
   on all platforms.
*/
int utfile_mkstemps(char *request_pattern, int suffix_length)
  {
    int pattern_length;
    char *mask_pointer;

    char *pattern = strdup(request_pattern);

    pattern_length = strlen(pattern);

    if ((pattern_length - 6) < suffix_length)
      {
        free(pattern);
        return -1;
      }

    int mask_offset = pattern_length - (6 + suffix_length);

    if (strncmp(&pattern[mask_offset], "XXXXXX", 6))
      {
        free(pattern);
        return -1;
      }

    mask_pointer = &pattern[mask_offset];

    int valid_char_count = strlen(valid_file_name_chars);

    for (int count = 0; count < MAX_MKSTEMPS_TRIES; count++)
      {
        for (int mask_index = 0; mask_index < 6; mask_index++)
          {
            mask_pointer[mask_index] = valid_file_name_chars[random() % valid_char_count];
          }

        int fd = open(pattern, O_CREAT|O_RDWR|O_EXCL, 0600);

        if (fd >= 0)
          {
            free(pattern);
            return fd;
          }

        // If the error is not file already exists or is a directory, then we just bail out because something else is wrong
        if ((errno != EEXIST) && (errno != EISDIR))
          {
            break;
          }
      }

    // If we get here, we were unable to create a unique file name despite a lot of tries
    free(pattern);
    return -1;
  }