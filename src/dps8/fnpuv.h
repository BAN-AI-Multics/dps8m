/*
 Copyright 2016 by Charles Anthony

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

struct uvClientData
  {
    bool assoc;
    uint fnpno;
    uint lineno;
    /* telnet_t */ void * telnetp;
    // Work buffet for processLineInput
    char buffer [1024];
    size_t nPos;
  };

typedef struct uvClientData uvClientData;

void fnpuvInit (int telnet_port);
void fnpuv3270Init (int telnet3270_port);
void fnpuv3270Poll (bool start);
void fnpuvProcessEvent (void);
void fnpuvProcess3270Event (void);
void fnpuv_start_write (uv_tcp_t * client, char * data, ssize_t len);
void fnpuv_start_writestr (uv_tcp_t * client, char * data);
void fnpuv_eor (uv_tcp_t * client);
void fnpuv_start_write_actual (uv_tcp_t * client, char * data, ssize_t datalen);
void fnpuv_associated_brk (uv_tcp_t * client);
void fnpuv_unassociated_readcb (uv_tcp_t * client, ssize_t nread, unsigned char * buf);
void fnpuv_associated_readcb (uv_tcp_t * client, ssize_t nread, unsigned char * buf);
void fnpuv_read_start (uv_tcp_t * client);
void fnpuv_read_stop (uv_tcp_t * client);
void fnpuv_dial_out (uint fnpno, uint lineno, word36 d1, word36 d2, word36 d3);
void fnpuv_open_slave (uint fnpno, uint lineno);
void close_connection (uv_stream_t* stream);
#ifdef TUN
void fnpTUNProcessEvent (void);
#endif
