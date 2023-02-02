/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 6ae968db-f630-11ec-991c-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2018 Charles Anthony
 * Copyright (c) 2015 Eric Swenson
 * Copyright (c) 2021-2023 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#ifndef UVUTIL_H
# define UVUTIL_H

struct uv_access_s
  {
    uv_loop_t * loop;
    int port;
    char * address;
# define PW_SIZE 128
    char pw[PW_SIZE + 1];
    char pwBuffer[PW_SIZE + 1];
    int pwPos;

    void (* connectPrompt) (uv_tcp_t * client);
    void (* connected) (uv_tcp_t * client);
    bool open;
    uv_tcp_t server;
    uv_tcp_t * client;
    bool useTelnet;
    void * telnetp;
    bool loggedOn;
    unsigned char * inBuffer;
    uint inSize;
    uint inUsed;
  };

typedef struct uv_access_s uv_access;
void accessStartWriteStr (uv_tcp_t * client, char * data);
void uv_open_access (uv_access * access);
# ifndef QUIET_UNUSED
void accessPutStr (uv_access * access, char * str);
void accessPutChar (uv_access * access,  char ch);
# endif
int accessGetChar (uv_access * access);
void accessStartWrite (uv_tcp_t * client, char * data, ssize_t datalen);
void accessCloseConnection (uv_stream_t* stream);
#endif
