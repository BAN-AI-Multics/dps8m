/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 34c5cd04-f62f-11ec-b27c-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2016 Charles Anthony
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

typedef void (* uv_read_cb_t)  (uv_tcp_t * client, ssize_t nread, unsigned char * buf);
typedef void (* uv_write_cb_t) (uv_tcp_t * client, unsigned char * data, ssize_t datalen);
struct uvClientData_s
  {
    bool assoc;
    uint fnpno;
    uint lineno;
    /* telnet_t */ void * telnetp;
    uv_read_cb_t  read_cb;
    uv_write_cb_t write_cb;
    uv_write_cb_t write_actual_cb;
    // Work buffer for processLineInput
    char buffer [1024];
    size_t nPos;
    // 3270
    char * ttype;
    uint stationNo;
  };

typedef struct uvClientData_s uvClientData;

int fnpuvInit (int telnet_port, char * telnet_address);
int fnpuv3270Init (int telnet3270_port);
void fnpuv3270Poll (bool start);
void fnpuvProcessEvent (void);
void fnpuv_start_write (uv_tcp_t * client, unsigned char * data, ssize_t len);
void fnpuv_start_writestr (uv_tcp_t * client, unsigned char * data);
void fnpuv_send_eor (uv_tcp_t * client);
void fnpuv_recv_eor (uv_tcp_t * client);
void fnpuv_start_write_actual (uv_tcp_t * client, unsigned char * data, ssize_t datalen);
void fnpuv_associated_brk (uv_tcp_t * client);
void fnpuv_unassociated_readcb (uv_tcp_t * client, ssize_t nread, unsigned char * buf);
void fnpuv_associated_readcb (uv_tcp_t * client, ssize_t nread, unsigned char * buf);
void fnpuv_read_start (uv_tcp_t * client);
void fnpuv_read_stop (uv_tcp_t * client);
void fnpuv_dial_out (uint fnpno, uint lineno, word36 d1, word36 d2, word36 d3);
void fnpuv_open_slave (uint fnpno, uint lineno);
void close_connection (uv_stream_t* stream);
#if defined(TUN)
void fnpTUNProcessEvent (void);
#endif /* if defined(TUN) */
void fnpuv_3270_readcb (uv_tcp_t * client,
                           ssize_t nread,
                           unsigned char * buf);
void fnpuv_start_3270_write (uv_tcp_t * client, unsigned char * data, ssize_t datalen);
void reset_line (struct t_line * linep);
