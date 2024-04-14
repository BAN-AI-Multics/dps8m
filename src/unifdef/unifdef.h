/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: BSD-2-Clause
 * scspell-id: 8ce0c4f6-f631-11ec-a3f0-80ee73e9b8e7
 *
 * -------------------------------------------------------------------------
 *
 * Copyright (c) 2012-2013 Tony Finch <dot@dotat.at>
 * Copyright (c) 2021 Jeffrey H. Johnson
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * -------------------------------------------------------------------------
 */

#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* portability stubs */

#define fbinmode(fp) ( fp )

#define replace(old, new) rename(old, new)

static FILE *
mktempmode(char *tmp, int mode)
{
  int fd = mkstemp(tmp);
  (void)mode;

  if (fd < 0)
  {
    return ( NULL );
  }

#if defined(_WIN32) && !defined(__MINGW64__) && !defined(__MINGW32__)
  fchmod(fd, mode & ( S_IRWXU | S_IRWXG | S_IRWXO ));
#endif /* if defined(_WIN32) && !defined(__MINGW64__) && !defined(__MINGW32__) */
  return ( fdopen(fd, "wb"));
}
