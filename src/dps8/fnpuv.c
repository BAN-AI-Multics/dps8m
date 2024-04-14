/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 2fdee814-f62f-11ec-8f3d-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2016 Charles Anthony
 * Copyright (c) 2016 Michal Tomek
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

// The FNP <--> libuv interface
//
// Every libuv TCP connection has a uv_tcp_t object.
//
// The uv_tcp_t object has a 'user data' field defined as "void * data".
//
// This code stores a pointer to a "struct uvClientData" in 'data'; this
// structure allows this code to determine which HSLA line the connection
// has been made with.
//
// The uvClientData structure contains:
//
//    bool assoc
//    uint fnpno;
//    uint lineno;
//
//       If 'assoc' is true, the TCP connection is associated with a particular
//       HSLA line as specified by 'fnpno' and 'lineno'. A dialup line is
//       associated by the user entering the FNP name and line number in the
//       multiplexor dial in dialog. For slave and dialout lines, 'assoc'
//       is always true as the association is predetermined by the FNP
//       configuration.
//
//    char buffer [1024];
//    size_t nPos;
//
//      Line canonicalization buffer used by the multiplexor dial in dialog.
//
// Each HSLA line is identified by a 'fnpno', 'lineno' pair. 'fnpno' indexes
// into the 'fnpUnitData' data structure and 'lineno' into
// 'fnpUnitData[fnpno].MState.line[]'.
//
// The 'line' structure contains:
//
//   enum service_types service;
//
//     This tracks the Multics' line configuration of 'dialup', 'dialout', and
//     'slave'.
//
//   uv_tcp_t * client;
//
//     If this is non-null, the HSLA has a TCP connection.
//
//   telnet_t * telnetp;
//
//     If this is non-null, Telnet is enabled on the connection.
//
//   uv_tcp_t server;
//
//     This is used by the 'slave' logic and holds the handle of the
//     TCP server port for the slave line.
//
//     The data field of 'server' points to a uvClientData structure; this
//     field is used by the incoming connection handler to distinguish the
//     slave and dialup server ports. (If null, dialup; else slave.)
//
//   uv_connect_t doConnect;
//
//     libuv uses a uv_connect_t object to create outbound connections;
//     the dialout code uses 'doConnect' for this.
//
//   int port;
//
//     The port number that a slave line listens on.
//

// Dialup logic.
//
// The code assumes a single port number for all dialup lines, even for the
// case of multiple FNPs.
//
// 'fnpuvInit()' is called when by the 'fnpserverport' command logic. It
// creates a TCP server listen port, assigning 'on_new_connection()' as
// the connection callback handler.
//
// When a connection is made to the listen port, libuv invokes the
// 'on_new_connection()' callback handler.
//
// The handler creates a 'uv_tcp_t' object 'client' and accepts the
// connection.
//
// It then checks the 'data' field on the listener object to see if this
// is a dialup or slave listener. For the case of dialup, a 'uvClientData'
// object is created and its 'assoc' field is set to false to flag the
// the connection as not yet being associated with an HSLA line; the
// libtelnet data structure is initialized and the initial Telnet negotiations
// are started. 'client->data' is set to the 'uvClientData' object,
// reading is enabled on 'client', and the HSLA line selection dialog
// prompt is sent down the dialup connection.
//
// When input data is ready on the connection (user input), libuv allocates
// a buffer, fills it with the data calls the 'fuv_read_cb()' callback handler.
//
// The handler looks at the 'data' field of the connection to access the
// 'uvClientData' structure; if the structure indicates that Telnet
// processing is to be done, it passes the data to libtelent handler.
// Otherwise, it checks the 'assoc' field; if true the data is passed to the
// FNP incoming data handler; if false, to the multiplexor line selection
// dialog handler. The libtelnet handler, after stripping and processing
// Telnet overhead passes any remaining data on in the same manner.
// The data buffer is then freed, and the callback returns.
//
// Since the FNP incoming data handler is dependent on Multics accepting
// the input data before it can safely dispose of the data, it disables
// reading on the connection, and reenables it when Multics has accepted the
// data.

// Data are written to the TCP connections with 'fnpuv_start_write()'. The
// 'uvClientData' is inspected for telnet usage; if so, the data is passed
// to libtelnet for telnet packaging and is sent on to
// 'fnpuv_start_write_actual'. If telnet is not in play, the data is sent
// directly on to 'fnpuv_start_write_actual()' There, a buffer is allocated
// and the data copied and a 'uv_write_t' request object is created and
// initialized data the buffer address and length, and a write request
// is send to libuv.
//
// When the write is complete, libuv calls the write callback 'fuv_write_cb()',
// which frees the data buffers and destroys the write request object.

// Dialout logic
//
// When the FNP receives a 'dial_out' command, it calls 'fnpuv_dial_out()'.
// The dial string is parsed for IP address, port number and telnet flag.
// An 'uv_tcp_t' object constructed and its address stored in the HSLA
// 'client' field. An 'uvClientData' object is created, associated with
// the dialout line HSLA, and if the Telnet flag is set, the Telnet processor
// is initialized. The TCP connect is initiated and the procedure returns.
//
// When the connection succeeds or times out, the 'on_dialout_connect()' callback
// is called. on_dialout_connect retrieves the 'uvClientData'. If the connection
// succeeded, the connection data field is set to the 'uvClientData'; read is
// enabled on the connection, and the 'accept_new_terminal' flag is set which
// will cause an 'accept_new_terminal' command to be send to Multics.
//

#if defined(TUN)
// TUN interface

// If makes more sense to me to have the emulator on an endpoint, but
// that's not really the way ethernet works; for the nonce, we will
// create a subnet and route to it with a tun device; the emulator
// will listen on the device and do the whole ARP rigamorole.
//
// It's not clear to me what happens if multiple emulators are listening
// on the same tun device, do they each get a copy of the message (i.e.
// are we wired as point-to-point)?
//
// Create device node:
//   mkdir /dev/net (if it doesn't exist already)
//   mknod /dev/net/tun c 10 200
//   chmod 0666 /dev/net/tun
//   modprobe tun
//
// The subnet gateway device is 'dps8m' at address 192.168.8.0
//
//   sudo ip tuntap add mode tun dps8m
//   sudo ip link set dps8m up
//   sudo ip addr add 192.168.8.1/24 dev dps8m
#endif /* if defined(TUN) */

#define ASSUME0 0

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#if defined(TUN)
# include <string.h>
# include <fcntl.h>
# include <errno.h>
# include <sys/ioctl.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <netinet/in.h>
# include <sys/socket.h>
# include <linux/if.h>
# include <linux/if_tun.h>
#endif /* if defined(TUN) */

#include "dps8.h"
#include "dps8_scu.h"
#include "dps8_sys.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_fnp2.h"
#include "dps8_utils.h"
#include "fnpuv.h"
#include "fnptelnet.h"

#if defined(TESTING)
# undef realloc
# undef FREE
# define FREE(p) free(p)
# define realloc trealloc
#endif /* if defined(TESTING) */

#define USE_REQ_DATA

// Making it up...
#define DEFAULT_BACKLOG 1024

#if defined(TUN)
static int tun_alloc (char * dev)
  {
    struct ifreq ifr;
    int fd, err;

    if ((fd = open ("/dev/net/tun", O_RDWR)) < 0)
      //return tun_alloc_old (dev);
      return fd;

    (void)memset (& ifr, 0,  sizeof (ifr));

    /*
     *  Flags: IFF_TUN   - TUN device (no Ethernet headers)
     *        IFF_TAP   - TAP device
     *
     *        IFF_NO_PI - Do not provide packet information
     */

    ifr.ifr_flags = IFF_TUN;
    if (* dev)
      strncpy (ifr.ifr_name, dev, IFNAMSIZ);

    if ((err = ioctl (fd, TUNSETIFF, (void *) & ifr)) < 0)
      {
        close (fd);
        return err;
      }
    strcpy (dev, ifr.ifr_name);
    return fd;
  }
#endif /* if defined(TUN) */

//
// alloc_buffer: libuv callback handler to allocate buffers for incoming data.
//

static void alloc_buffer (UNUSED uv_handle_t * handle, size_t suggested_size,
                          uv_buf_t * buf)
 {
/* Suppress Clang Analyzer's possible memory leak warning */
#if !defined ( __clang_analyzer__ )
    * buf = uv_buf_init ((char *) malloc (suggested_size), (uint) suggested_size);
#endif /* if !defined ( __clang_analyzer__ ) */
  }

void fnpuv_associated_brk (uv_tcp_t * client)
  {
    if (! client || uv_is_closing ((uv_handle_t *) client))
      return;
    if (! client->data)
      {
        sim_warn ("fnpuv_associated_brk bad client data\r\n");
        return;
      }
    uvClientData * p = (uvClientData *) client->data;
    uint fnpno = p -> fnpno;
    uint lineno = p -> lineno;
    struct t_line * linep = & fnpData.fnpUnitData[fnpno].MState.line[lineno];
    // The break key forces an input buffer flush.
    // XXX is this a race condition? Is it possible for processFnpMbx to see
    // the line_break before the accept input?
    linep->accept_input = 1;
    linep->line_break=true;
  }

// read callback for connections that are associated with an HSLA line;
// forward the data to the FNP input handler

void fnpuv_associated_readcb (uv_tcp_t * client,
                           ssize_t nread,
                           unsigned char * buf)
  {
    processLineInput (client, buf, nread);
  }

void fnpuv_3270_readcb (uv_tcp_t * client,
                           ssize_t nread,
                           unsigned char * buf)
  {
    process3270Input (client, buf, nread);
  }

// read callback for connections that are no associated with an HSLA line;
// forward the data to the line selection dialog handler

void fnpuv_unassociated_readcb (uv_tcp_t * client,
                           ssize_t nread,
                           unsigned char * buf)
  {
    //(void)printf ("unassoc. <%*s>\n", (int) nread, buf->base);
    processUserInput (client, buf, nread);
  }

static void fuv_close_cb (uv_handle_t * stream)
  {
    FREE (stream);
  }

// teardown a connection

void close_connection (uv_stream_t* stream)
  {
    if (! stream)
      {
        sim_warn ("close_connection bad client data\r\n");
        return;
      }
    uvClientData * p = (uvClientData *) stream->data;

    // If stream->data, the stream is associated with a Multics line.
    // Tear down that association
    //if (p && fnpData.fnpUnitData[p->fnpno].MState.line[p->lineno].service != service_3270)
    if (p)
      {
        // If assoc is false, then the disconnect happened before the actual
        // association took place
        if (p->assoc)
          {
            struct t_line * linep = & fnpData.fnpUnitData[p->fnpno].MState.line[p->lineno];
            if (linep->service  == service_3270)
             {
               // On the 3270, the station closing does not close the controller
               sim_printf ("\r[FNP emulation: TN3270 %d.%d DISCONNECT]\r\n", ASSUME0, p->stationNo);
             }
            else
              {
                sim_printf ("\r[FNP emulation: DISCONNECT %c.d%03d]\r\n", p->fnpno+'a', p->lineno);
#if defined(DISC_DELAY)
                linep -> line_disconnected = DISC_DELAY;
#else
                linep -> line_disconnected = true;
#endif /* if defined(DISC_DELAY) */
                linep -> listen = false;
                if (linep->inBuffer)
                  FREE (linep->inBuffer);
                linep->inBuffer = NULL;
                linep->inSize   = 0;
                linep->inUsed   = 0;
                linep->nPos     = 0;
              }
            if (linep->line_client)
              {
                // linep->line_client is a copied stream->data; will be freed
                // below
                linep->line_client = NULL;
              }
          }
        else // ! p-assoc
          {
            sim_printf ("\r[FNP emulation: DISCONNECT]\r\n");
          }
        // Clean up allocated data
        if (p->telnetp)
          {
            telnet_free (p->telnetp);
            // telnet_free frees self
            //free (p->telnetp);
            p->telnetp = NULL;
          }
        if (((uvClientData *) stream->data)->ttype)
          FREE (((uvClientData *) stream->data)->ttype);
        FREE (stream->data);
        stream->data = NULL;
      } // if (p)
    if (! uv_is_closing ((uv_handle_t *) stream))
      uv_close ((uv_handle_t *) stream, fuv_close_cb);
  }

//
// fuv_read_cb: libuv read complete callback
//
//   Cleanup on read error.
//   Forward data to appropriate handler.

static void fuv_read_cb (uv_stream_t* stream,
                         ssize_t nread,
                         const uv_buf_t* buf)
  {
    if (nread < 0)
      {
        //if (nread == UV_EOF)
          {
            close_connection (stream);
          }
      }
    else if (nread > 0)
      {
        if (! stream)
          {
            sim_warn ("fuv_read_cb bad client data\r\n");
            return;
          }
        uvClientData * p = (uvClientData *) stream->data;
        if (p)
          {
            if (p->telnetp)
              {
                telnet_recv (p->telnetp, buf->base, (size_t) nread);
              }
            else
              {
#if 1
                (* p->read_cb) ((uv_tcp_t *) stream, nread, (unsigned char *) buf->base);

#else
                if (p -> assoc)
                  {
                    fnpuv_associated_readcb ((uv_tcp_t *) stream, nread, (unsigned char *) buf->base);
                  }
                else
                  {
                    fnpuv_unassociated_readcb ((uv_tcp_t *) stream, nread, (unsigned char *) buf->base);
                  }
#endif
              }
          }
      }

    if (buf->base)
        free (buf->base); /* X-LINTED: FREE */
  }

//
// fuv_write_cb: libuv write complete callback
//
//   Cleanup on error
//   Free buffers
//

static void fuv_write_cb (uv_write_t * req, int status)
  {
    if (status < 0)
      {
        if (status == -ECONNRESET || status == -ECANCELED ||
            status == -EPIPE)
          {
            // This occurs when the other end disconnects; not an "error"
          }
        else
          {
            sim_warn ("fuv_write_cb status %d (%s)\n", -status, xstrerror_l(-status));
          }

        // connection reset by peer
        close_connection (req->handle);
      }

#if defined(USE_REQ_DATA)
    //sim_printf ("freeing bufs %p\n", req->data);
    FREE (req->data);
#else
    unsigned int nbufs = req->nbufs;
    uv_buf_t * bufs = req->bufs;
    for (unsigned int i = 0; i < nbufs; i ++)
      {
        if (bufs && bufs[i].base)
          {
            //sim_printf ("freeing bufs%d %p\n", i, bufs[i].base);
            FREE (bufs[i].base);
          }
        if (req->bufsml[i].base)
          {
            //sim_printf ("freeing bufsml%d %p@%p\n", i, req->bufsml[i].base, & req->bufsml[i].base);
            FREE (req->bufsml[i].base);
          }
      }
#endif /* if defined(USE_REQ_DATA) */

    // the buf structure is copied; do not free.
    //sim_printf ("freeing req %p\n", req);
    FREE (req);
  }

//
// fuv_write_3270_cb: libuv write complete callback
//
//   Cleanup on error
//   Free buffers
//

static void fuv_write_3270_cb (uv_write_t * req, int status) {
    uv_tcp_t * client = (uv_tcp_t *) req->handle;
    fuv_write_cb (req, status);
    set_3270_write_complete (client);
  }

// Create and start a write request

static void fnpuv_start_write_3270_actual (UNUSED uv_tcp_t * client, unsigned char * data, ssize_t datalen)
  {
#if defined(TESTING)
      sim_printf ("fnpuv_start_write_3270_actual\r\n");
#endif /* if defined(TESTING) */

    // Find the client from the device selection call

    uint stn_no;
    for (stn_no = 0; stn_no < ADDR_MAP_ENTRIES; stn_no ++)
      if (addr_map [stn_no] == fnpData.ibm3270ctlr[ASSUME0].selDevChar)
        break;
    if (stn_no >= ADDR_MAP_ENTRIES)
      {
        sim_printf ("fnpuv_start_write_3270_actual couldn't find selDevChar %02x\r\n", (unsigned int) fnpData.ibm3270ctlr[ASSUME0].selDevChar);
        return;
      }
    uv_tcp_t * stn_client = fnpData.ibm3270ctlr[ASSUME0].stations[stn_no].client;
    if (! stn_client || uv_is_closing ((uv_handle_t *) stn_client))
      return;

    // Allocate write request
/* Suppress Clang Analyzer's possible memory leak warning */
#if !defined (__clang_analyzer__)
    uv_write_t * req = (uv_write_t *) malloc (sizeof (uv_write_t));
    if (!req)
      {
        (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                       __func__, __FILE__, __LINE__);
# if defined(USE_BACKTRACE)
#  if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
#  endif /* if defined(SIGUSR2) */
# endif /* if defined(USE_BACKTRACE) */
        abort();
      }
    // This makes sure that bufs*.base and bufsml*.base are NULL
    (void)memset (req, 0, sizeof (uv_write_t));
    uv_buf_t buf = uv_buf_init ((char *) malloc ((unsigned long) datalen), (uint) datalen);
//sim_printf ("allocated req %p data %p\n", req, buf.base);
# if defined(USE_REQ_DATA)
    req->data = buf.base;
# endif
//sim_printf ("fnpuv_start_write_actual req %p buf.base %p\n", req, buf.base);
    memcpy (buf.base, data, (unsigned long) datalen);
    int ret = uv_write (req, (uv_stream_t *) stn_client, & buf, 1, fuv_write_3270_cb);
// There seems to be a race condition when Multics signals a disconnect_line;
// We close the socket, but Multics is still writing its goodbye text trailing
// NULs.
// If the socket has been closed, write will return BADF; just ignore it.
    if (ret < 0 && ret != -EBADF)
      sim_printf ("\r[FNP emulation: uv_write returned %d]\r\n", ret);
#endif /* if !defined ( __clang_analyzer__ ) */
  }

void fnpuv_start_write_actual (uv_tcp_t * client, unsigned char * data, ssize_t datalen)
  {
    if (! client || uv_is_closing ((uv_handle_t *) client))
      return;
/* Suppress Clang Analyzer's possible memory leak warning */
#if !defined ( __clang_analyzer__ )
    uv_write_t * req = (uv_write_t *) malloc (sizeof (uv_write_t));
    if (!req)
      {
        (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                       __func__, __FILE__, __LINE__);
# if defined(USE_BACKTRACE)
#  if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
#  endif /* if defined(SIGUSR2) */
# endif /* if defined(USE_BACKTRACE) */
        abort();
      }
    // This makes sure that bufs*.base and bufsml*.base are NULL
    (void)memset (req, 0, sizeof (uv_write_t));
    uv_buf_t buf = uv_buf_init ((char *) malloc ((unsigned long) datalen), (uint) datalen);
//sim_printf ("allocated req %p data %p\n", req, buf.base);
# if defined(USE_REQ_DATA)
    req->data = buf.base;
# endif
//sim_printf ("fnpuv_start_write_actual req %p buf.base %p\n", req, buf.base);
    memcpy (buf.base, data, (unsigned long) datalen);
    int ret = uv_write (req, (uv_stream_t *) client, & buf, 1, fuv_write_cb);
// There seems to be a race condition when Multics signals a disconnect_line;
// We close the socket, but Multics is still writing its goodbye text trailing
// NULs.
// If the socket has been closed, write will return BADF; just ignore it.
    if (ret < 0 && ret != -EBADF)
      sim_printf ("\r[FNP emulation: uv_write returned %d]\r\n", ret);
#endif /* if !defined ( __clang_analyzer__ ) */
  }

//
// Write data to a connection, doing Telnet if needed.
//

void fnpuv_start_write (uv_tcp_t * client, unsigned char * data, ssize_t datalen)
  {
    if (! client || uv_is_closing ((uv_handle_t *) client))
      return;
    uvClientData * p = (uvClientData *) client->data;
    if (! p) //-V547
      return;
    if (!p->telnetp)
      {
        sim_warn ("telnetp NULL; dropping fnpuv_start_write()\n");
        return;
      }
    telnet_send (p->telnetp, (char *) data, (size_t) datalen);
  }

void fnpuv_start_3270_write (uv_tcp_t * client, unsigned char * data, ssize_t datalen)
  {
    if (! client || uv_is_closing ((uv_handle_t *) client) || ! client->data)
      return;
    uvClientData * p = (uvClientData *) client->data;
    if (! p) //-V547
      return;
#if defined(TESTING)
sim_printf ("fnpuv_start_3270_write\r\n");
#endif

    // Strip BSC protocol:
    //    STX <text> ETX
    //    EOT

    if (datalen == 1 && data [0] == 0x37) // EOT
      {
#if defined(TESTING)
sim_printf ("fnpuv: detected EOT\r\n");
#endif /* if defined(TESTING) */
        fnpuv_send_eor (client);
        return;
      }

    unsigned char * actual_data_start = data;
    unsigned long actual_datalen = (unsigned long) datalen;
    //bool send_eor = false;
    if (data [datalen - 1] == 0x03) // ETX
      {
        actual_datalen --;
        //send_eor = true;
      }
    if (data [0] == 0x02) // STX
      {
        actual_data_start ++;
        actual_datalen --;
        if (data [1] == 0x27) // ESC
          {
            actual_data_start ++;
            actual_datalen --;
          }
      }

    telnet_send (p->telnetp, (char *) actual_data_start, (size_t) actual_datalen);
    //if (send_eor)
      //fnpuv_send_eor (client);
  }

// C-string wrapper for fnpuv_start_write

void fnpuv_start_writestr (uv_tcp_t * client, unsigned char * data)
  {
    //fnpuv_start_write (client, data, (ssize_t) strlen (data));
    if (! client || uv_is_closing ((uv_handle_t *) client))
      return;
    if (! client->data)
      {
        sim_warn ("fnpuv_start_writestr bad client data\r\n");
        return;
      }
    uvClientData * p = client->data;
    (* p->write_cb) (client, data, (ssize_t) strlen ((char *) data));
  }

void fnpuv_send_eor (uv_tcp_t * client)
  {
    if (! client || uv_is_closing ((uv_handle_t *) client))
      return;
    if (! client->data)
      {
        sim_warn ("fnpuv_send_eor bad client data\r\n");
        return;
      }
    uvClientData * p = (uvClientData *) client->data;
    ltnEOR (p->telnetp);
    //unsigned char EOR [] = { TELNET_IAC, TELNET_EOR };
    //fnpuv_start_write_special (client, (char *) EOR, sizeof (EOR));
  }

void fnpuv_recv_eor (uv_tcp_t * client)
  {
    fnpRecvEOR (client);
  }

//
// Enable reading on connection
//

void fnpuv_read_start (uv_tcp_t * client)
  {
    if (! client || uv_is_closing ((uv_handle_t *) client))
      return;
    uv_read_start ((uv_stream_t *) client, alloc_buffer, fuv_read_cb);
  }

//
// Disable reading on connection
//

void fnpuv_read_stop (uv_tcp_t * client)
  {
    if (! client || uv_is_closing ((uv_handle_t *) client))
      return;
    uv_read_stop ((uv_stream_t *) client);
  }

//
// Connection callback handler for dialup connections
//

static void on_new_connection (uv_stream_t * server, int status)
  {
    if (status < 0)
      {
        sim_printf ("\r[FNP emulation: new connection error: %s]\r\n", uv_strerror(status));
        // error!
        return;
      }

    uv_tcp_t * client = (uv_tcp_t *) malloc (sizeof (uv_tcp_t));
    if (!client)
      {
        (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                       __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
        abort();
      }
    uv_tcp_init (fnpData.loop, client);
    if (uv_accept (server, (uv_stream_t *) client) != 0)
      {
        uv_close ((uv_handle_t *) client, fuv_close_cb);
        return;
      }

    // if server->data is non-null, this is a slave server; else a dialup
    // server
    if (server->data)
      {
        uvClientData * p = (uvClientData *) server->data;
        struct t_line * linep = & fnpData.fnpUnitData[p->fnpno].MState.line[p->lineno];
#if 1
        // Slave servers only handle a single connection at a time
        if (linep->line_client)
          {
            uv_close ((uv_handle_t *) client, fuv_close_cb);
# if defined(TESTING)
sim_printf ("\r[FNP emulation: dropping 2nd slave]\r\n");
# endif
            return;
          }
#endif
        linep->line_client = client;
      }

    struct sockaddr name;
    int namelen = sizeof (name);
    uv_tcp_nodelay (client,1);
    int ret = uv_tcp_getpeername (client, & name, & namelen);
    if (ret < 0)
      {
        sim_printf ("\r[FNP emulation: CONNECT; addr err %d]\r\n", ret);
      }
    else
      {
        struct sockaddr_in * p = (struct sockaddr_in *) & name;
        sim_printf ("\r[FNP emulation: CONNECT %s]\r\n", inet_ntoa (p -> sin_addr));
      }

    uvClientData * p = (uvClientData *) malloc (sizeof (uvClientData));
    if (! p)
      {
         (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                        __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
        abort();
      }
    client->data       = p;
    p->assoc           = false;
    p->nPos            = 0;
    p->ttype           = NULL;
    p->write_actual_cb = fnpuv_start_write_actual;
    // dialup connections are routed through libtelent
    if (! server->data)
      {
        p->read_cb  = fnpuv_unassociated_readcb;
        p->write_cb = fnpuv_start_write;
        p->telnetp  = ltnConnect (client);

        if (! p->telnetp)
          {
             sim_warn ("ltnConnect failed\n");
             return;
          }
      }
    else
      {
        p->read_cb       = fnpuv_associated_readcb;
        p->write_cb      = fnpuv_start_write_actual;
        p->telnetp       = NULL;
        uvClientData * q = (uvClientData *) server->data;
        p->fnpno         = q->fnpno;
        p->lineno        = q->lineno;
        p->assoc         = true;
      }
    fnpuv_read_start (client);
    if (! server->data)
      fnpConnectPrompt (client);
    else
      {
        uvClientData * p = (uvClientData *) server->data;
        struct t_line * linep = & fnpData.fnpUnitData[p->fnpno].MState.line[p->lineno];
        if (linep->lineType == 0) /* LINE_NONE */
          linep->lineType = 1; /* LINE_ASCII */
        linep->accept_new_terminal = true;
        reset_line (linep);
      }
  }

//
// Setup the dialup listener
//

int fnpuvInit (int telnet_port, char * telnet_address)
  {
    // Ignore multiple calls; this means that once the listen port is
    // opened, it can't be changed. Fixing this requires non-trivial
    // changes.
    if (fnpData.du_server_inited)
      {
        sim_printf ("\r[FNP emulation: FNP already initialized]\r\n");
        return 1;
      }

    if (! fnpData.loop)
      fnpData.loop = uv_default_loop ();

    // Initialize the server socket
    fnpData.loop = uv_default_loop ();
    uv_tcp_init (fnpData.loop, & fnpData.du_server);

// XXX to do clean shutdown
// uv_loop_close (fnpData.loop);

    // Flag the this server as being the dialup server
    fnpData.du_server.data = NULL;

    // Bind and listen
    struct sockaddr_in addr;
    uv_ip4_addr (telnet_address, telnet_port, & addr);
    uv_tcp_bind (& fnpData.du_server, (const struct sockaddr *) & addr, 0);
    int r = uv_listen ((uv_stream_t *) & fnpData.du_server, DEFAULT_BACKLOG,
                       on_new_connection);
    if (r)
     {
        sim_printf ("\r[FNP emulation: listen error: %s:%ld: %s]\r\n", telnet_address, (long) telnet_port, uv_strerror(r));
        return 1;
     }
    fnpData.du_server_inited = true;
    sim_printf ("\r[FNP emulation: TELNET server listening on %s:%ld]\r\n", telnet_address, (long) telnet_port);
    return 0;
  }

// Make a single pass through the libuv event queue.

void fnpuvProcessEvent (void)
  {
    // UV_RUN_NOWAIT: Poll for i/o once but don’t block if there are no
    // pending callbacks. Returns zero if done (no active handles or
    // requests left), or non-zero if more callbacks are expected (meaning
    // you should run the event loop again sometime in the future).

    // Note that uv_run returns non-zero if that are any active_handles
    // (e.g. TCP connection listener open); that means a non-zero
    // return does not mean i/o is pending.
    if (! fnpData.loop)
      return;
    /* int ret = */ uv_run (fnpData.loop, UV_RUN_NOWAIT);
  }

//
// dialout line connection callback
//

static void on_dialout_connect (uv_connect_t * server, int status)
  {
    sim_printf ("\r[FNP emulation: dialout connect]\r\n");
    uvClientData * p = (uvClientData *) server->handle->data;
    // If data is NULL, assume that the line has already been torn down.
    if (! p)
      {
         sim_printf ("\r[FNP emulation: NOTE: on_dialout_connect called with data == NULL]\r\n");
         return;
      }
    struct t_line * linep = & fnpData.fnpUnitData[p->fnpno].MState.line[p->lineno];
    if (status < 0)
      {
        sim_printf ("\r[FNP emulation: dialout connection error: %s]\r\n", uv_strerror(status));
        //sim_printf ("%p\n", p);
        //sim_printf ("%d.%d\n", p->fnpno, p->lineno);
        linep->acu_dial_failure = true;
        return;
      }

    uv_read_start ((uv_stream_t *) linep->line_client, alloc_buffer, fuv_read_cb);
    linep->listen = true;
    if (linep->lineType == 0) /* LINE_NONE */
      linep->lineType = 1; /* LINE_ASCII */
    linep->accept_new_terminal = true;
    linep->was_CR              = false;
    linep->line_client->data   = p;
    if (p->telnetp)
      {
        ltnDialout (p->telnetp);
      }
  }

//
// Perform a dialout
//

void fnpuv_dial_out (uint fnpno, uint lineno, word36 d1, word36 d2, word36 d3)
  {
    if (! fnpData.loop)
      return;
    sim_printf ("\r[FNP emulation: received dial_out %c.h%03d %012llu %012llu %012llu]\r\n", fnpno+'a', lineno,
            (unsigned long long)d1, (unsigned long long)d2, (unsigned long long)d3);
    struct t_line * linep = & fnpData.fnpUnitData[fnpno].MState.line[lineno];
    uint d01   = (d1 >> 30) & 017;
    uint d02   = (d1 >> 24) & 017;
    uint d03   = (d1 >> 18) & 017;
    uint d04   = (d1 >> 12) & 017;
    uint d05   = (d1 >>  6) & 017;
    uint d06   = (d1 >>  0) & 017;
    uint d07   = (d2 >> 30) & 017;
    uint d08   = (d2 >> 24) & 017;
    uint d09   = (d2 >> 18) & 017;
    uint d10   = (d2 >> 12) & 017;
    uint d11   = (d2 >>  6) & 017;
    uint d12   = (d2 >>  0) & 017;
    uint flags = (d3 >> 30) & 017;
    uint p1    = (d3 >> 24) & 017;
    uint p2    = (d3 >> 18) & 017;
    uint p3    = (d3 >> 12) & 017;
    uint p4    = (d3 >>  6) & 017;
    uint p5    = (d3 >>  0) & 017;

    uint oct1  = d01 * 100 + d02 * 10 + d03;
    uint oct2  = d04 * 100 + d05 * 10 + d06;
    uint oct3  = d07 * 100 + d08 * 10 + d09;
    uint oct4  = d10 * 100 + d11 * 10 + d12;

    uint port  = ((((p1 * 10) + p2) * 10 + p3) * 10 + p4) * 10 + p5;

// Flags
//   1    Use Telnet
#if defined(TUN)
//   2    Use TUN device (in which case 'port' is the netmask
//   4

    linep->is_tun = false;
    if (flags & 2)
      {
        if (linep->tun_fd <= 0)
          {
            char a_name [IFNAMSIZ] = "dps8m";
            linep->tun_fd = tun_alloc (a_name);
            if (linep->tun_fd < 0)
              {
                sim_printf ("\r[FNP emulation: dialout TUN tun_alloc returned %d errno %d]\r\n", linep->tun_fd, errno);
                return;
             }
            int flags = fcntl (linep->tun_fd, F_GETFL, 0);
            if (flags < 0)
              {
                sim_printf ("\r[FNP emulation: dialout TUN F_GETFL returned < 0]\r\n");
                return;
              }
            flags |= O_NONBLOCK;
            int ret = fcntl (linep->tun_fd, F_SETFL, flags);
            if (ret)
              {
                sim_printf ("\r[FNP emulation: dialout TUN F_SETFL returned %d]\r\n", ret);
                return;
              }
          }
          linep->is_tun = true;
          return;
        }
#endif /* if defined(TUN) */

    // firewall

    // Default is accept
    bool accept = true;
    uint32_t ip_addr = (uint32_t) ((oct1 << 24) | (oct2 << 16) | (oct3 << 8) | oct4);
    uint this_line = encodeline (fnpno, lineno);
    for (int i = 0; i < n_fw_entries; i ++)
      {
        struct fw_entry_s * p = fw_entries + i;
        if (this_line < p->line_0 || this_line > p->line_1)
          continue;
        if ((ip_addr & p->cidr_mask) != (p->ipaddr & p->cidr_mask))
          continue;
        accept = p->accept;
        break;
      }

    if (! accept)
      {
        sim_printf ("Dialout %c.d%03d denied\r\n", fnpno + 'a', lineno);
        linep->acu_dial_failure = true;
        return;
      }

    char ipaddr [256];
    (void)sprintf (ipaddr, "%d.%d.%d.%d", oct1, oct2, oct3, oct4);
    sim_printf ("calling %s:%d\n", ipaddr,port);

    struct sockaddr_in dest;
    uv_ip4_addr(ipaddr, (int) port, &dest);

    linep->line_client = (uv_tcp_t *) malloc (sizeof (uv_tcp_t));
    if (!linep->line_client)
      {
        (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                       __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
        abort();
      }
    uv_tcp_init (fnpData.loop, linep->line_client);

    uvClientData * p = (uvClientData *) malloc (sizeof (uvClientData));
    if (! p)
      {
        (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                       __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
        abort();
      }
    p->assoc                 = true;
    p->read_cb               = fnpuv_associated_readcb;
    p->nPos                  = 0;
    p->ttype                 = NULL;
    p->fnpno                 = fnpno;
    p->lineno                = lineno;
    linep->line_client->data = p;

    if (flags & 1)
      {
        p->write_cb        = fnpuv_start_write;
        p->write_actual_cb = fnpuv_start_write_actual;
        p->telnetp         = ltnConnect (linep->line_client);
        if (! p->telnetp)
          {
              sim_warn ("ltnConnect failed\n");
          }
      }
    else
      {
        p->write_cb        = fnpuv_start_write_actual;
        p->write_actual_cb = fnpuv_start_write_actual;
        p->telnetp         = NULL; // Mark this line as 'not a telnet connection'
      }

    uv_tcp_connect (& linep->doConnect, linep->line_client, (const struct sockaddr *) & dest, on_dialout_connect);
  }

#if 0
static void on_slave_connect (uv_stream_t * server, int status)
  {
    sim_printf ("slave connect\n");
    uvClientData * p = (uvClientData *) server->data;
    struct t_line * linep = & fnpData.fnpUnitData[p->fnpno].MState.line[p->lineno];
    if (status < 0)
      {
        sim_printf ("Slave connection error %s\n", uv_strerror(status));
        //linep->acu_dial_failure = true;
        return;
      }

    uv_read_start ((uv_stream_t *) & linep->line_client, alloc_buffer, do_readcb);
    linep->accept_new_terminal = true;
  }
#endif

//
// Start a slave line connection listener.
//

void fnpuv_open_slave (uint fnpno, uint lineno)
  {
    if (! fnpData.loop)
      return;
    sim_printf ("\r[FNP emulation: fnpuv_open_slave %d.%d]\r\n", fnpno, lineno);
    struct t_line * linep = & fnpData.fnpUnitData[fnpno].MState.line[lineno];

    // Do we already have a listening port (ie not first time)?
    if (linep->server.data)
      return;

    uv_tcp_init (fnpData.loop, & linep->server);

    // Mark this server has being a slave server
    // XXX This does not get freed during a normal shutdown, as Multics
    // XXX doesn't tell idle slave lines anything. The emulator shutdown
    // XXX needs to call an FNP cleanup routine that frees this.

    uvClientData * p = (uvClientData *) malloc (sizeof (uvClientData));
    if (! p)
      {
        (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                       __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
        abort();
      }
    p->assoc           = false;
    p->read_cb         = fnpuv_associated_readcb;
    p->write_cb        = fnpuv_start_write_actual;
    p->write_actual_cb = fnpuv_start_write_actual;
    p->nPos            = 0;
    p->ttype           = NULL;
    p->fnpno           = fnpno;
    p->lineno          = lineno;
    linep->server.data = p;
    linep->line_client = NULL;

    struct sockaddr_in addr;
    uv_ip4_addr (fnpData.telnet_address, linep->port, & addr);
    uv_tcp_bind (& linep->server, (const struct sockaddr *) & addr, 0);
    int r = uv_listen ((uv_stream_t *) & linep->server, DEFAULT_BACKLOG,
                       on_new_connection);
    if (r)
     {
        sim_printf ("\r[FNP emulation: listen error: %s:%ld: %s]\r\n", fnpData.telnet_address, (long) linep->port, uv_strerror(r));
     }
    sim_printf ("\r[FNP emulation: listening on %s:%ld]\r\n", fnpData.telnet_address, (long) linep->port);
  }

#if defined(TUN)
static void processPacketInput (int fnpno, int lineno, unsigned char * buf, ssize_t nread)
  {
    //uvClientData * p = (uvClientData *) client->data;
    //uint fnpno       = p -> fnpno;
    //uint lineno      = p -> lineno;
    if (fnpno >= N_FNP_UNITS_MAX || lineno >= MAX_LINES)
      {
        sim_printf ("\r[FNP emulation: processPacketInput bogus client data]\r\n");
        return;
      }
//sim_printf ("assoc. %d.%d nread %ld <%*s>\n", fnpno, lineno, (long) nread, buf);
//{for (int i = 0; i < nread; i ++) sim_printf (" %03o", buf[i]);
 //sim_printf ("\n");
//}

    struct t_line * linep = & fnpData.fnpUnitData[fnpno].MState.line[lineno];
    if (linep->inBuffer)
      {
        unsigned char * new = realloc (linep->inBuffer, (unsigned long) (linep->inSize + nread));
        if (! new)
          {
            (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                           __func__, __FILE__, __LINE__)
# if defined(USE_BACKTRACE)
#  if defined(SIGUSR2)
            (void)raise(SIGUSR2);
            /*NOTREACHED*/ /* unreachable */
#  endif /* if defined(SIGUSR2) */
# endif /* if defined(USE_BACKTRACE) */
            abort();
          }
        memcpy (new + linep->inSize, buf, (unsigned long) nread);
        linep->inSize  += nread;
        linep->inBuffer = new;
      }
    else
      {
        linep->inBuffer = malloc ((unsigned long) nread);
        if (! linep->inBuffer)
          {
            (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                           __func__, __FILE__, __LINE__);
# if defined(USE_BACKTRACE)
#  if defined(SIGUSR2)
            (void)raise(SIGUSR2);
            /*NOTREACHED*/ /* unreachable */
#  endif /* if defined(SIGUSR2) */
# endif /* if defined(USE_BACKTRACE) */
            abort();
          }
        memcpy (linep->inBuffer, buf, (unsigned long) nread);
        linep->inSize = (uint) nread;
        linep->inUsed = 0;
      }

  }

static void fnoTUNProcessLine (int fnpno, int lineno, struct t_line * linep)
  {
/* Note that "buffer" should be at least the MTU size of the interface, eg 1500 bytes */
    unsigned char buffer [1500 + 16];
    ssize_t nread = read (linep->tun_fd, buffer, sizeof (buffer));
    if (nread < 0)
      {
        //perror ("Reading from interface");
        //close (linep->tun_fd);
        //exit (1);
        if (errno == EAGAIN)
          return;
        sim_printf ("%ld %ld\n", (long) nread, (long) errno);
        return;
      }

// To make debugging easier, return data as a hex string rather than binary.
// When the stack interface is debugged, switch to binary.
    unsigned char xbufr [2 * (1500 + 16)];
    unsigned char bin2hex [16] = "0123456789ABCDEF";

    for (uint i = 0; i > nread; i ++)
      {
        xbufr [i * 2 + 0] = bin2hex [(buffer [i] >> 4) & 0xf];
        xbufr [i * 2 + 1] = bin2hex [(buffer [i] >> 0) & 0xf];
      }
     xbufr [nread * 2] = 0;
     processPacketInput (fnpno, lineno, xbufr, nread * 2 + 1);

// Debugging
// 4 bytes of metadata
# define ip 4
    /* Do whatever with the data */
    sim_printf("Read %ld bytes\n", (long) nread);
    sim_printf ("%02x %02x %02x %02x %02x %02x %02x %02x\n",
      buffer [0],  buffer [1],  buffer [2],  buffer [3],
      buffer [4],  buffer [5],  buffer [6],  buffer [7]);
    sim_printf ("%02x %02x %02x %02x %02x %02x %02x %02x\n",
      buffer [8],  buffer [9],  buffer [10], buffer [11],
      buffer [12], buffer [13], buffer [14], buffer [15]);
    uint version         =         (buffer [ip +  0] >> 4) & 0xf;
    uint ihl             =         (buffer [ip +  0]) & 0xf;
    uint payload_offset  =                  ip + ihl * 4;
    uint tos             =          buffer [ip +  1];
    uint tl              = ((uint) (buffer [ip +  2]) << 8) +
                                    buffer [ip +  3];
    uint id              = ((uint) (buffer [ip +  4]) << 8) +
                                    buffer [ip +  5];
    uint df              =         (buffer [ip +  6] & 0x40) ? 1 : 0;
    uint mf              =         (buffer [ip +  6] & 0x20) ? 1 : 0;
    uint fragment_offset = ((uint) (buffer [ip +  6] & 0x1f) << 8) +
                                    buffer [ip +  7];
    uint ttl             =          buffer [ip +  8];
    uint protocol        =          buffer [ip +  9];
    uint header_checksum = (((uint) buffer [ip + 10]) << 8) +
                                    buffer [ip + 11];
    uint source_address  = (((uint) buffer [ip + 12]) << 24) +
                           (((uint) buffer [ip + 13]) << 16) +
                           (((uint) buffer [ip + 14]) << 8) +
                                    buffer [ip + 15];
    uint dest_address    = (((uint) buffer [ip + 16]) << 24) +
                           (((uint) buffer [ip + 17]) << 16) +
                           (((uint) buffer [ip + 18]) << 8) +
                                    buffer [ip + 19];
    if (protocol == 1)
      {
        uint type = buffer [payload_offset + 0];
        if (type == 0x08)
          {
            sim_printf ("ICMP Echo Request %d.%d.%d.%d %d.%d.%d.%d\n",
              buffer [ip + 12], buffer [ip + 13], buffer [ip + 14], buffer [ip + 15],
              buffer [ip + 16], buffer [ip + 17], buffer [ip + 18], buffer [ip + 19]);
          }
        else
          {
            sim_printf ("ICMP 0x%02x\n", type);
            sim_printf ("%02x %02x %02x %02x %02x %02x %02x %02x\n",
              buffer [payload_offset + 0], buffer [payload_offset + 1], buffer [payload_offset + 2], buffer [payload_offset + 3],
              buffer [payload_offset + 4], buffer [payload_offset + 5], buffer [payload_offset + 6], buffer [payload_offset + 7]);
          }
      }
    if (protocol == 0x11)
      {
        sim_printf ("UDP\n");

      }
    else
      {
        sim_printf ("protocol %02x\n", protocol);
      }
  }

void fnpTUNProcessEvent (void)
  {
    unint32 numunits = fnp_dev.numunits;
    for (int fnpno = 0; fnpno < numnumts; fnpno ++)
      {
        for (int lineno = 0; lineno < MAX_LINES; lineno ++)
          {
            struct t_line * linep = & fnpData.fnpUnitData[fnpno].MState.line[lineno];
            if (linep->is_tun)
              fnoTUNProcessLine (fnpno, lineno, linep);
          }
      }
  }
#endif

void fnpuv3270Poll (bool start)
  {
    // Called at 100Hz; to 1 second poll
    fnpData.du3270_poll = start ? 100 : 0;
  }

//
// Connection callback handler for dialup connections
//

static void on_new_3270_connection (uv_stream_t * server, int status)
  {
    if (status < 0)
      {
        sim_printf ("\r[FNP emulation: TN3270 new connection error: %s]\r\n", uv_strerror(status));
        // error!
        return;
      }

    uv_tcp_t * client = (uv_tcp_t *) malloc (sizeof (uv_tcp_t));
    if (!client)
      {
        (void)fprintf(stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                      __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
        abort();
      }

    uv_tcp_init (fnpData.loop, client);
    if (uv_accept (server, (uv_stream_t *) client) != 0)
      {
        uv_close ((uv_handle_t *) client, fuv_close_cb);
        return;
      }

    // Search for an available station
    uint stn_no;
    for (stn_no = 0; stn_no < IBM3270_STATIONS_MAX; stn_no ++)
      {
        if (fnpData.ibm3270ctlr[ASSUME0].stations[stn_no].client == NULL)
          break;
      }
    if (stn_no >= IBM3270_STATIONS_MAX)
      {
        // No stations available
        uv_close ((uv_handle_t *) client, fuv_close_cb);
        return;
      }

    uint fnpno  = fnpData.ibm3270ctlr[ASSUME0].fnpno;
    uint lineno = fnpData.ibm3270ctlr[ASSUME0].lineno;
    // Set the line client to NULL; the actual clients are in 'stations'
    fnpData.fnpUnitData[fnpno].MState.line[lineno].line_client = NULL;

    fnpData.ibm3270ctlr[ASSUME0].stations[stn_no].client = client;

    // Set up selection so the telnet negotiation can find the station.
    fnpData.ibm3270ctlr[ASSUME0].selDevChar = addr_map[stn_no];

    struct sockaddr name;
    int namelen = sizeof (name);
    int ret = uv_tcp_getpeername (client, & name, & namelen);
    if (ret < 0)
      {
        sim_printf ("\r[FNP emulation: CONNECT; addr err %d]\r\n", ret);
      }
    else
      {
        struct sockaddr_in * p = (struct sockaddr_in *) & name;
        sim_printf ("\r[FNP emulation: CONNECT %s]\r\n", inet_ntoa (p -> sin_addr));
      }

    uvClientData * p = (uvClientData *) malloc (sizeof (uvClientData));
    if (! p)
      {
         (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                        __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
         (void)raise(SIGUSR2);
         /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
         abort();
      }
    client->data       = p;
    p->assoc           = false;
    p->fnpno           = fnpno;
    p->lineno          = lineno;
    p->nPos            = 0;
    p->ttype           = NULL;
    p->read_cb         = fnpuv_3270_readcb;
    p->write_cb        = fnpuv_start_3270_write;
    p->write_actual_cb = fnpuv_start_write_3270_actual;
    p->stationNo       = stn_no;
    p->telnetp         = ltnConnect3270 (client);

    if (! p->telnetp)
      {
        sim_warn ("ltnConnect3270 failed\n");
        return;
      }
    fnpuv_read_start (client);
    fnp3270ConnectPrompt (client);
       // uvClientData * p               = (uvClientData *) server->data;
       // struct t_line * linep          = & fnpData.fnpUnitData[p->fnpno].MState.line[p->lineno];
       // linep->accept_new_terminal     = true;
       // linep->was_CR                  = false;
       // //linep->listen                = false;
       // linep->inputBufferSize         = 0;
       // linep->ctrlStrIdx              = 0;
       // linep->breakAll                = false;
       // linep->handleQuit              = false;
       // linep->echoPlex                = false;
       // linep->crecho                  = false;
       // linep->lfecho                  = false;
       // linep->tabecho                 = false;
       // linep->replay                  = false;
       // linep->polite                  = false;
       // linep->prefixnl                = false;
       // linep->eight_bit_out           = false;
       // linep->eight_bit_in            = false;
       // linep->odd_parity              = false;
       // linep->output_flow_control     = false;
       // linep->input_flow_control      = false;
       // linep->block_xfer_in_frame_sz  = 0;
       // linep->block_xfer_out_frame_sz = 0;
       // (void)memset (linep->delay_table, 0, sizeof (linep->delay_table));
       // linep->inputSuspendLen         = 0;
       // (void)memset (linep->inputSuspendStr, 0, sizeof (linep->inputSuspendStr));
       // linep->inputResumeLen          = 0;
       // (void)memset (linep->inputResumeStr, 0, sizeof (linep->inputResumeStr));
       // linep->outputSuspendLen        = 0;
       // (void)memset (linep->outputSuspendStr, 0, sizeof (linep->outputSuspendStr));
       // linep->outputResumeLen         = 0;
       // (void)memset (linep->outputResumeStr, 0, sizeof (linep->outputResumeStr));
       // linep->frame_begin             = 0;
       // linep->frame_end               = 0;
       // (void)memset (linep->echnego, 0, sizeof (linep->echnego));
       // linep->line_break              = false;
  }

int fnpuv3270Init (int telnet3270_port)
  {
    // Ignore multiple calls; this means that once the listen port is
    // opened, it can't be changed. Fixing this requires non-trivial
    // changes.
    if (fnpData.du3270_server_inited)
      {
        sim_printf ("\r[FNP emulation: FNP already initialized]\r\n");
        return 1;
      }
    if (! fnpData.loop)
      fnpData.loop = uv_default_loop ();
    // Initialize the server socket
    uv_tcp_init (fnpData.loop, & fnpData.du3270_server);

    // Flag the this server as being a 3270
    fnpData.du3270_server.data = NULL;

    // Bind and listen
    struct sockaddr_in addr;
    uv_ip4_addr (fnpData.telnet_address, telnet3270_port, & addr);
    uv_tcp_bind (& fnpData.du3270_server, (const struct sockaddr *) & addr, 0);
    int r = uv_listen ((uv_stream_t *) & fnpData.du3270_server, DEFAULT_BACKLOG,
                   on_new_3270_connection);
    if (r)
     {
        sim_printf ("\r[FNP emulation: listen error: %s:%ld: %s]\r\n", fnpData.telnet_address, (long) telnet3270_port, uv_strerror(r));
        return 1;
     }
    fnpData.du3270_server_inited = true;
    sim_printf ("\r[FNP emulation: TN3270 server listening on %s:%ld]\r\n", fnpData.telnet_address, (long) telnet3270_port);
    fnpuv3270Poll (false);
    return 0;
  }
