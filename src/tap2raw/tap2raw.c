/*
 * vim: filetype=c:tabstop=2:softtabstop=2:shiftwidth=2:ai:expandtab
 * scspell-id: 21a630e0-1afc-11f0-af20-80ee73e9b8e7
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Copyright (c) 2024-2025 Jeffrey H. Johnson
 * Copyright (c) 2024-2025 The DPS8M Development Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
# include <fcntl.h>
# include <io.h>
#endif /* if defined( _WIN32 ) */

#define VERSION_MAJ 1
#define VERSION_MIN 1
#define VERSION_PAT 1

#define MBUFSIZ (1024 * 64)

static uint64_t wcnt, block, reccnt, mark;
static bool isbe;

static void
status (void)
{
  int64_t ft = ftell (stdin);

  if (-1 == ft)
    {
      (void)fprintf (stderr, "Error reading input: %s\r\n",
                     strerror (errno)); /*LINTOK: xstrerror_l*/
      exit (EXIT_FAILURE);
    }

  (void)fprintf (stderr,
                 "Position %-11" PRId64 " Mark %-8" PRIu64 " Block %-6" PRIu64
                 " %5" PRIu64 " record%s\r\n",
                 ft, mark, block, reccnt, reccnt > 1 ? "s" : "");
}

static void
checkbe (void)
{
  int tmp = 1;
  isbe = (*((char *)&tmp) ? false : true);
}

static uint16_t
xhtons (uint16_t x)
{
  if (isbe)
    {
      unsigned char *s = (unsigned char *)&x;
      return (uint16_t)(s[0] << 8 | s[1]);
    }
  else
    {
      return x;
    }
}

int
main (int argc, char *argv[])
{
  uint64_t bread;
  uint8_t bc[4] = { 0 };
  uint8_t buf[MBUFSIZ];
  const bool forever = true;

  if (1 < argc)
    {
      if (0 == strcmp (argv[1], "-h") ||
          0 == strcmp (argv[1], "-H") ||
          0 == strcmp (argv[1], "--help"))
        {
          (void)fprintf (stdout, "Usage:\r\n");
          (void)fprintf (stdout, "  tap2raw < infile.tap > outfile.raw\r\n");
          (void)fprintf (stdout, "  tap2raw [ -v | --version ]\r\n");
          (void)fprintf (stdout, "  tap2raw [ -h | --help ]\r\n");
          return EXIT_SUCCESS;
        }
      else if (0 == strcmp (argv[1], "-v") ||
               0 == strcmp (argv[1], "-V") ||
               0 == strcmp (argv[1], "--version"))
        {
          (void)fprintf (stdout, "tap2raw %d.%d.%d\r\n",
                         VERSION_MAJ, VERSION_MIN, VERSION_PAT);
          return EXIT_SUCCESS;
        } else {
          (void)fprintf (stderr, "Error: Invalid option; try '-h'\r\n");
         return EXIT_FAILURE;
        }
    }

  checkbe ();

#if defined(_WIN32)
  _setmode (_fileno (stdin), O_BINARY);
  _setmode (_fileno (stdout), O_BINARY);
#endif /* if defined( _WIN32 ) */

  block = 1;
  reccnt = mark = 0;
  do
    {
      (void)memset (buf, 0, sizeof (char) * MBUFSIZ);
      bread = fread (bc, sizeof (char), 4, stdin);
      if (0 == bread)
        {
          break;
        }

      wcnt = ((xhtons ((uint32_t)bc[0] | ((uint32_t)bc[1] << 8)) + 1) & ~1UL);
      if (wcnt)
        {
          (void)fread (buf, sizeof (char), wcnt, stdin);
          (void)fwrite (buf, sizeof (char), wcnt, stdout);
          (void)fread (bc, sizeof (char), 4, stdin);
          reccnt++;
        }
      else
        {
          if (reccnt)
            {
              mark++;
              status ();
            }

          block++;
          reccnt = 0;
        }
    }
  while (forever);

  mark++;
  status ();

  return EXIT_SUCCESS;
}
