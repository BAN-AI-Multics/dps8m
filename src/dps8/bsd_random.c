/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: BSD-3-Clause
 * scspell-id: ecbb6700-f62c-11ec-a713-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 1983-1991 Regents of the University of California
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * All rights reserved.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ---------------------------------------------------------------------------
 */

#if defined( __MINGW64__ ) || defined( __MINGW32__ )

# include <stdio.h>

# include "bsd_random.h"

# define TYPE_0    0
# define BREAK_0   8
# define DEG_0     0
# define SEP_0     0

# define TYPE_1    1
# define BREAK_1   32
# define DEG_1     7
# define SEP_1     3

# define TYPE_2    2
# define BREAK_2   64
# define DEG_2     15
# define SEP_2     1

# define TYPE_3    3
# define BREAK_3   128
# define DEG_3     31
# define SEP_3     3

# define TYPE_4    4
# define BREAK_4   256
# define DEG_4     63
# define SEP_4     1

# define MAX_TYPES 5

static int degrees[MAX_TYPES] = {
  DEG_0, DEG_1, DEG_2, DEG_3, DEG_4
};

static int seps[MAX_TYPES] = {
  SEP_0, SEP_1, SEP_2, SEP_3, SEP_4
};

static long randtbl[DEG_3 + 1] = {
  TYPE_3,     0x9a319039, 0x32d9c024, 0x9b663182, 0x5da1f342, 0xde3b81e0,
  0xdf0a6fb5, 0xf103bc02, 0x48f340fb, 0x7449e56b, 0xbeb1dbb0, 0xab5c5918,
  0x946554fd, 0x8c2e680f, 0xeb3d799f, 0xb11ee0b7, 0x2d436b86, 0xda672e2a,
  0x1588ca88, 0xe369735d, 0x904f35f7, 0xd7158fd6, 0x6fa6f051, 0x616e6b96,
  0xac94efdc, 0x36413f93, 0xc622c298, 0xf5a42ab8, 0x8a88d77b, 0xf5ad9d0e,
  0x8999220b, 0x27fb47b9,
};

static long *fptr     = &randtbl[SEP_3 + 1];
static long *rptr     = &randtbl[1];
static long *state    = &randtbl[1];
static int  rand_type = TYPE_3;
static int  rand_deg  = DEG_3;
static int  rand_sep  = SEP_3;
static long *end_ptr  = &randtbl[DEG_3 + 1];

void
bsd_srandom(unsigned int x)
{
  if (rand_type == TYPE_0)
    {
      state[0] = x;
    }
  else
    {
      register int i;
      state[0] = x;
      for (i = 1; i < rand_deg; i++)
        {
          state[i] = 1103515245 * state[i - 1] + 12345;
        }

      fptr = &state[rand_sep];
      rptr = &state[0];
      for (i = 0; i < 10 * rand_deg; i++)
        {
          (void)bsd_random();
        }
    }
}

char *
bsd_initstate(unsigned int seed, char *arg_state, int n)
{
  register char *ostate = (char *)( &state[-1] );

  if (rand_type == TYPE_0)
    {
      state[-1] = rand_type;
    }
  else
    {
      state[-1] = MAX_TYPES * ( rptr - state ) + rand_type;
    }

  if (n < BREAK_0)
    {
      return 0;
    }

  if (n < BREAK_1)
    {
      rand_type = TYPE_0;
      rand_deg  = DEG_0;
      rand_sep  = SEP_0;
    }
  else if (n < BREAK_2)
    {
      rand_type = TYPE_1;
      rand_deg  = DEG_1;
      rand_sep  = SEP_1;
    }
  else if (n < BREAK_3)
    {
      rand_type = TYPE_2;
      rand_deg  = DEG_2;
      rand_sep  = SEP_2;
    }
  else if (n < BREAK_4)
    {
      rand_type = TYPE_3;
      rand_deg  = DEG_3;
      rand_sep  = SEP_3;
    }
  else
    {
      rand_type = TYPE_4;
      rand_deg  = DEG_4;
      rand_sep  = SEP_4;
    }

  state   = &(((long *)arg_state )[1] );
  end_ptr = &state[rand_deg];
  bsd_srandom(seed);
  if (rand_type == TYPE_0)
    {
      state[-1] = rand_type;
    }
  else
    {
      state[-1] = MAX_TYPES * ( rptr - state ) + rand_type;
    }

  return ostate;
}

char *
bsd_setstate(char *arg_state)
{
  register long *new_state = (long *)arg_state;
  register int type        = (int)( new_state[0] % MAX_TYPES );
  register int rear        = (int)( new_state[0] / MAX_TYPES );
  char *ostate             = (char *)( &state[-1] );

  if (rand_type == TYPE_0)
    {
      state[-1] = rand_type;
    }
  else
    {
      state[-1] = MAX_TYPES * ( rptr - state ) + rand_type;
    }

  switch (type)
    {
    case TYPE_0:
    case TYPE_1:
    case TYPE_2:
    case TYPE_3:
    case TYPE_4:
      rand_type = type;
      rand_deg  = degrees[type];
      rand_sep  = seps[type];
      break;
    }

  state = &new_state[1];
  if (rand_type != TYPE_0)
    {
      rptr = &state[rear];
      fptr = &state[( rear + rand_sep ) % rand_deg];
    }

  end_ptr = &state[rand_deg];
  return ostate;
}

long
bsd_random(void)
{
  long i;

  if (rand_type == TYPE_0)
    {
      i = state[0] = ( state[0] * 1103515245 + 12345 ) & 0x7fffffff;
    }
  else
    {
      *fptr += *rptr;
      i = ( *fptr >> 1 ) & 0x7fffffff;
      if (++fptr >= end_ptr)
        {
          fptr = state;
          ++rptr;
        }
      else if (++rptr >= end_ptr)
        {
          rptr = state;
        }
    }

  return i;
}

#endif /* if defined(__MINGW64__) || defined(__MINGW32__) */
