/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 2b7c10aa-f62e-11ec-8477-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#if defined(DUMA)
# if !defined(_POSIX_C_SOURCE)
#  define _POSIX_C_SOURCE 1
# endif /* if !defined(_POSIX_C_SOURCE) */
# undef USE_DUMA
# define USE_DUMA 1
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <duma.h>
#endif /* if defined(DUMA) */
