/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: BSD-2-Clause
 * scspell-id: 736ea385-f62a-11ec-9ef9-80ee73e9b8e7
 *
 * -------------------------------------------------------------------------
 *
 * linehistory.h
 *
 * linehistory is forked from linenoise; the original version
 * is available from https://github.com/antirez/linenoise
 *
 * ------------------------------------------------------------------------
 *
 * Copyright (c) 2010-2014 Salvatore Sanfilippo <antirez@gmail.com>
 * Copyright (c) 2010-2013 Pieter Noordhuis <pcnoordhuis@gmail.com>
 * Copyright (c) 2020-2021 Jeffrey H. Johnson <trnsz@pobox.com>
 * Copyright (c) 2021-2024 The DPS8M Development Team
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

#if !defined(_POSIX_C_SOURCE)
# define _POSIX_C_SOURCE 200809L
#endif /* if !defined(_POSIX_C_SOURCE) */

#if _POSIX_C_SOURCE < 200809L
# undef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 200809L
#endif /* if _POSIX_C_SOURCE < 200809L */

#if !defined(__LINENOISE_H)
# define __LINENOISE_H

# if !defined(__MINGW32__) && !defined(CROSS_MINGW32) && !defined(CROSS_MINGW64) && !defined(__MINGW64__) && !defined(_MSC_VER) && !defined(_MSC_BUILD)

#  if !defined(HAVE_LINEHISTORY)
#   define HAVE_LINEHISTORY
#  endif /* if !defined(HAVE_LINEHISTORY) */

#  include <stddef.h>
#  include <signal.h> /* raise */

#  if defined(LH_COMPLETION)
typedef struct linenoiseCompletions
{
  size_t len;
  char   **cvec;
} linenoiseCompletions;

typedef void (linenoiseCompletionCallback) (const char *,
                                            linenoiseCompletions *);
void    linenoiseSetCompletionCallback(linenoiseCompletionCallback *);
void    linenoiseAddCompletion(linenoiseCompletions *, const char *);
#  endif /* if defined(LH_COMPLETION) */

#  if defined(LH_HINTS)
typedef void (linenoiseFreeHintsCallback) (void *);
typedef char *(linenoiseHintsCallback)(const char *, int *color, int *bold);
void    linenoiseSetHintsCallback(linenoiseHintsCallback *);
void    linenoiseSetFreeHintsCallback(linenoiseFreeHintsCallback *);
#  endif /* if defined(LH_HINTS) */

char *linenoise(const char *prompt);
void linenoiseFree(void *ptr);
int  linenoiseHistoryAdd(const char *line);
int  linenoiseHistorySetMaxLen(int len);
int  linenoiseHistorySave(const char *filename);
int  linenoiseHistoryLoad(const char *filename);
void linenoiseClearScreen(void);
void linenoiseSetMultiLine(int ml);
void linenoisePrintKeyCodes(void);
#  if defined(LH_MASKMODE)
void linenoiseMaskModeEnable(void);
void linenoiseMaskModeDisable(void);
#  endif /* if defined(LH_MASKMODE) */

# endif /* if !defined (__MINGW32__) && !defined (__MINGW64__) && !defined (CROSS_MINGW32) && !defined (CROSS_MINGW64) && !defined (_MSC_VER) && !defined (_MSC_BUILD) */

#endif /* if __LINENOISE_H */
