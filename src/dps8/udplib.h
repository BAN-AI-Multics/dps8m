/*
 * udplib.h: IMP/TIP Modem and Host Interface socket routines using UDP
 *
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: MIT
 * scspell-id: 1d277b36-f630-11ec-8beb-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2013 Robert Armstrong <bob@jfcl.com>
 * Copyright (c) 2015-2016 Charles Anthony
 * Copyright (c) 2021-2023 The DPS8M Development Team
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL ROBERT ARMSTRONG BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Robert Armstrong shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Robert Armstrong.
 *
 * ---------------------------------------------------------------------------
 */

#ifdef WITH_ABSI_DEV
# define NOLINK      (-1)
# define PFLG_FINAL 00001
int udp_create  (const char * premote, int32_t * plink);
int udp_release (int32_t link);
int udp_send    (int32_t link, uint16_t * pdata, uint16_t count, uint16_t flags);
int udp_receive (int32_t link, uint16_t * pdata, uint16_t maxbufg);
#endif /* ifdef WITH_ABSI_DEV */
