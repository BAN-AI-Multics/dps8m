/*
 * sim_fio.c: simulator file I/O library
 *
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: MIT
 * scspell-id: bc8c09f8-f62a-11ec-9723-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 1993-2008 Robert M. Supnik
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Robert M. Supnik shall not
 * be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Robert M. Supnik.
 *
 * ---------------------------------------------------------------------------
 */

/*
 * This library includes:
 *
 * sim_finit            -       initialize package
 * sim_fopen            -       open file
 * sim_fread            -       endian independent read (formerly fxread)
 * sim_write            -       endian independent write (formerly fxwrite)
 * sim_fseek            -       conditionally extended (>32b) seek (
 * sim_fseeko           -       extended seek (>32b if available)
 * sim_fsize            -       get file size
 * sim_fsize_name       -       get file size of named file
 * sim_fsize_ex         -       get file size as a t_offset
 * sim_fsize_name_ex    -       get file size as a t_offset of named file
 * sim_buf_copy_swapped -       copy data swapping elements along the way
 * sim_buf_swap_data    -       swap data elements inplace in buffer
 */

#include <fcntl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "sim_defs.h"

#include "../decNumber/decContext.h"
#include "../decNumber/decNumberLocal.h"

#if !defined(DECLITEND)
# error Unknown platform endianness
#endif /* if !defined(DECLITEND) */

#if defined(NO_LOCALE)
# define xstrerror_l strerror
#endif

#if defined(FREE)
# undef FREE
#endif /* if defined(FREE) */
#define FREE(p) do  \
  {                 \
    free((p));      \
    (p) = NULL;     \
  } while(0)

t_bool sim_end;                     /* TRUE = little endian, FALSE = big endian */

/*
 * OS-independent, endian independent binary I/O package
 *
 * For consistency, all binary data read and written by the simulator
 * is stored in little endian data order.  That is, in a multi-byte
 * data item, the bytes are written out right to left, low order byte
 * to high order byte.  On a big endian host, data is read and written
 * from high byte to low byte.  Consequently, data written on a little
 * endian system must be byte reversed to be usable on a big endian
 * system, and vice versa.
 *
 * These routines are analogs of the standard C runtime routines
 * fread and fwrite.  If the host is little endian, or the data items
 * are size char, then the calls are passed directly to fread or
 * fwrite.  Otherwise, these routines perform the necessary byte swaps.
 * Sim_fread swaps in place, sim_fwrite uses an intermediate buffer.
 */

int32 sim_finit (void)
{
sim_end = DECLITEND;
return sim_end;
}

void sim_buf_swap_data (void *bptr, size_t size, size_t count)
{
size_t j;
size_t k;
unsigned char by, *sptr, *dptr;

if (sim_end || (count == 0) || (size == sizeof (char)))
    return;
for (j = 0, dptr = sptr = (unsigned char *) bptr;       /* loop on items */
     j < count; j++) {
    for (k = (int32)(size - 1); k >= (((int32) size + 1) / 2); k--) {
        by = *sptr;                                     /* swap end-for-end */
        *sptr++ = *(dptr + k);
        *(dptr + k) = by;
        }
    sptr = dptr = dptr + size;                          /* next item */
    }
}

size_t sim_fread (void *bptr, size_t size, size_t count, FILE *fptr)
{
size_t c;

if ((size == 0) || (count == 0))                        /* check arguments */
    return 0;
c = fread (bptr, size, count, fptr);                    /* read buffer */
if (sim_end || (size == sizeof (char)) || (c == 0))     /* le, byte, or err? */
    return c;                                           /* done */
sim_buf_swap_data (bptr, size, count);
return c;
}

void sim_buf_copy_swapped (void *dbuf, const void *sbuf, size_t size, size_t count)
{
size_t j;
size_t k;
const unsigned char *sptr = (const unsigned char *)sbuf;
unsigned char *dptr = (unsigned char *)dbuf;

if (sim_end || (size == sizeof (char))) {
    memcpy (dptr, sptr, size * count);
    return;
    }
for (j = 0; j < count; j++) {                           /* loop on items */
    /* Unsigned countdown loop. Pre-decrement k before it's used inside the
       loop so that k == 0 in the loop body to process the last item, then
       terminate. Initialize k to size for the same reason: the pre-decrement
       gives us size - 1 in the loop body. */
    for (k = size; k > 0; /* empty */)
        *(dptr + --k) = *sptr++;
    dptr = dptr + size;
    }
}

size_t sim_fwrite (const void *bptr, size_t size, size_t count, FILE *fptr)
{
size_t c, nelem, nbuf, lcnt, total;
int32 i;
const unsigned char *sptr;
unsigned char *sim_flip;

if ((size == 0) || (count == 0))                        /* check arguments */
    return 0;
if (sim_end || (size == sizeof (char)))                 /* le or byte? */
    return fwrite (bptr, size, count, fptr);            /* done */
sim_flip = (unsigned char *)malloc(FLIP_SIZE);
if (!sim_flip)
    return 0;
nelem = FLIP_SIZE / size;                               /* elements in buffer */
nbuf = count / nelem;                                   /* number buffers */
lcnt = count % nelem;                                   /* count in last buf */
if (lcnt) nbuf = nbuf + 1;
else lcnt = nelem;
total = 0;
sptr = (const unsigned char *) bptr;                    /* init input ptr */
for (i = (int32)nbuf; i > 0; i--) {                     /* loop on buffers */
    c = (i == 1)? lcnt: nelem;
    sim_buf_copy_swapped (sim_flip, sptr, size, c);
    sptr = sptr + size * count;
    c = fwrite (sim_flip, size, c, fptr);
    if (c == 0) {
        FREE(sim_flip);
        return total;
        }
    total = total + c;
    }
FREE(sim_flip);
return total;
}

/* Forward Declaration */

t_offset sim_ftell (FILE *st);

/* Get file size */

t_offset sim_fsize_ex (FILE *fp)
{
t_offset pos, sz;

if (fp == NULL)
    return 0;
pos = sim_ftell (fp);
if (sim_fseek (fp, 0, SEEK_END))
  return 0;
sz = sim_ftell (fp);
if (sim_fseeko (fp, pos, SEEK_SET))
  return 0;
return sz;
}

t_offset sim_fsize_name_ex (const char *fname)
{
FILE *fp;
t_offset sz;

if ((fp = sim_fopen (fname, "rb")) == NULL)
    return 0;
sz = sim_fsize_ex (fp);
fclose (fp);
return sz;
}

uint32 sim_fsize_name (const char *fname)
{
return (uint32)(sim_fsize_name_ex (fname));
}

uint32 sim_fsize (FILE *fp)
{
return (uint32)(sim_fsize_ex (fp));
}

/* OS-dependent routines */

/* Optimized file open */

FILE *sim_fopen (const char *file, const char *mode)
{
FILE *fsc = NULL;
#if defined(USE_FCNTL) || defined(USE_FLOCK)
# include <fcntl.h>
# include <sys/stat.h>
# include <sys/types.h>
int writable = 0;
int rc = 0;
if (strstr(mode, "+") != NULL)
  writable = 1;
if (strstr(mode, "w") != NULL)
  writable = 1;
if (strstr(mode, "a") != NULL)
  writable = 1;
#endif /* if defined(USE_FCNTL) || defined(USE_FLOCK) */
fsc = fopen (file, mode);
#if defined(USE_FCNTL)
struct flock lock;
(void)memset (&lock, 0, sizeof(lock));
lock.l_type = F_WRLCK;
if (writable && !sim_nolock) {
  if (fsc != NULL)
    rc = fcntl (fileno(fsc), F_SETLK, &lock);
  if (rc < 0) {
    if (!sim_quiet) {
      sim_printf ("%s(%s): %s [Error %d]",
                  __func__, mode, xstrerror_l(errno), errno);
      if (fcntl(fileno(fsc), F_GETLK, &lock) == 0 && lock.l_pid > 0)
        sim_printf (" (locked by PID %lu)",
                   (unsigned long)lock.l_pid);
      sim_printf ("\r\n");
    }
    if (!sim_iglock) {
      fclose(fsc);
      return NULL;
    }
  }
}
#elif defined(USE_FLOCK) /* if defined(USE_FCNTL) */
if (writable && !sim_nolock) {
  if (fsc != NULL)
    rc = flock (fileno(fsc), LOCK_EX | LOCK_NB);
  if (rc < 0) {
    if (!sim_quiet)
      sim_printf ("%s(%s): %s [Error %d] (locked?)\r\n",
                  __func__, mode, xstrerror_l(errno), errno);
    if (!sim_iglock) return NULL;
  }
}
#endif /* elif defined(USE_FLOCK) */
return fsc;
}

#if defined (_WIN32)
# include <sys/stat.h>

int sim_fseeko (FILE *st, t_offset offset, int whence)
{
fpos_t fileaddr;
struct _stati64 statb;

switch (whence) {
    case SEEK_SET:
        fileaddr = (fpos_t)offset;
        break;

    case SEEK_END:
        if (_fstati64 (_fileno (st), &statb))
            return (-1);
        fileaddr = statb.st_size + offset;
        break;
    case SEEK_CUR:
        if (fgetpos (st, &fileaddr))
            return (-1);
        fileaddr = fileaddr + offset;
        break;

    default:
        errno = EINVAL;
        return (-1);
        }

return fsetpos (st, &fileaddr);
}

t_offset sim_ftell (FILE *st)
{
fpos_t fileaddr;
if (fgetpos (st, &fileaddr))
    return (-1);
return (t_offset)fileaddr;
}

#else

int sim_fseeko (FILE *st, t_offset xpos, int origin)
{
return fseeko (st, (off_t)xpos, origin);
}

t_offset sim_ftell (FILE *st)
{
return (t_offset)(ftello (st));
}

#endif

int sim_fseek (FILE *st, t_addr offset, int whence)
{
return sim_fseeko (st, (t_offset)offset, whence);
}

#if defined(_WIN32)
# include <io.h>
int sim_set_fsize (FILE *fptr, t_addr size)
{
return _chsize(_fileno(fptr), (long)size);
}

int sim_set_fifo_nonblock (FILE *fptr)
{
return -1;
}

#else /* !defined(_WIN32) */
# include <unistd.h>
int sim_set_fsize (FILE *fptr, t_addr size)
{
return ftruncate(fileno(fptr), (off_t)size);
}

# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>

int sim_set_fifo_nonblock (FILE *fptr)
{
struct stat stbuf;

if (!fptr || fstat (fileno(fptr), &stbuf))
    return -1;
# if defined(S_IFIFO) && defined(O_NONBLOCK)
if ((stbuf.st_mode & S_IFIFO)) {
    int flags = fcntl(fileno(fptr), F_GETFL, 0);
    return fcntl(fileno(fptr), F_SETFL, flags | O_NONBLOCK);
    }
# endif
return -1;
}

#endif
