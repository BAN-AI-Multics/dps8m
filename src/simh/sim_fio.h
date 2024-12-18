/*
 * sim_fio.h: simulator file I/O library headers
 *
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: MIT
 * scspell-id: c2fd7c81-f62a-11ec-b01e-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 1993-2008 Robert M. Supnik
 * Copyright (c) 2021-2024 The DPS8M Development Team
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

#if !defined(SIM_FIO_H_)
# define SIM_FIO_H_     0

# define FLIP_SIZE        (1 << 16)               /* flip buf size */
# define fxread(a,b,c,d)  sim_fread (a, b, c, d)
# define fxwrite(a,b,c,d) sim_fwrite (a, b, c, d)

int32 sim_finit (void);
typedef t_int64 t_offset;
FILE *sim_fopen (const char *file, const char *mode);
int sim_fseek (FILE *st, t_addr offset, int whence);
int sim_fseeko (FILE *st, t_offset offset, int whence);
int sim_set_fsize (FILE *fptr, t_addr size);
int sim_set_fifo_nonblock (FILE *fptr);
size_t sim_fread (void *bptr, size_t size, size_t count, FILE *fptr);
size_t sim_fwrite (const void *bptr, size_t size, size_t count, FILE *fptr);
uint32 sim_fsize (FILE *fptr);
uint32 sim_fsize_name (const char *fname);
t_offset sim_ftell (FILE *st);
t_offset sim_fsize_ex (FILE *fptr);
t_offset sim_fsize_name_ex (const char *fname);
void sim_buf_swap_data (void *bptr, size_t size, size_t count);
void sim_buf_copy_swapped (void *dptr, const void *bptr, size_t size, size_t count);
extern t_bool sim_end; /* TRUE = little endian, FALSE = big endian */

#endif
