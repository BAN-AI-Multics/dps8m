/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 6315eaa3-f630-11ec-96fc-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2013-2019 Charles Anthony
 * Copyright (c) 2021 Jeffrey H. Johnson
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

//-V::595

#include <uv.h>
#include <ctype.h>
#include <signal.h>
#include "dps8.h"
#include "dps8_sys.h"
#include "dps8_cpu.h"
#include "dps8_utils.h"
#include "libtelnet.h"
#include "uvutil.h"

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

#define USE_REQ_DATA

static void accessTelnetReadCallback (uv_tcp_t * client,
                            ssize_t nread,
                            unsigned char * buf);

//
// alloc_buffer: libuv callback handler to allocate buffers for incoming data.
//

static void alloc_buffer (UNUSED uv_handle_t * handle, size_t suggested_size,
                          uv_buf_t * buf)
  {
/* Suppress Clang Analyzer's possible memory leak warning */
#if !defined(__clang_analyzer__)
    * buf = uv_buf_init ((char *) malloc (suggested_size),
                         (uint) suggested_size);
#endif /* if !defined(__clang_analyzer__) */
  }

static void accessWriteCallback (uv_write_t * req, int status)
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
            sim_warn ("accessWriteCallback status %d (%s)\r\n", -status,
                      xstrerror_l(-status));
          }

        // connection reset by peer
        accessCloseConnection (req->handle);
      }

#if defined(USE_REQ_DATA)
    FREE (req->data);
#else
    unsigned int nbufs = req->nbufs;
    uv_buf_t * bufs = req->bufs;
    for (unsigned int i = 0; i < nbufs; i ++)
      {
        if (bufs && bufs[i].base)
          {
            FREE (bufs[i].base);
            //bufp->base = NULL;
          }
        if (req->bufsml[i].base)
          {
            FREE (req->bufsml[i].base);
          }
      }
#endif /* if defined(USE_REQ_DATA) */

    // the buf structure is copied; do not free.
    //sim_printf ("freeing req %p\r\n", req);
    FREE (req);
  }

//
// accessReadCallback: libuv read complete callback
//
//   Cleanup on read error.
//   Forward data to appropriate handler.

static void accessReadCallback (uv_stream_t* stream,
                             ssize_t nread,
                             const uv_buf_t* buf)
  {
    uv_access * access = (uv_access *) stream->data;
    if (nread < 0)
      {
        //if (nread == UV_EOF)
          {
            accessCloseConnection (stream);
          }
      }
    else if (nread > 0)
      {
        if (access->telnetp)
          {
            telnet_recv (access->telnetp, buf->base, (size_t) nread);
          }
        else
          {
            accessTelnetReadCallback ((uv_tcp_t *) stream,
                                      (ssize_t) nread,
                                      (unsigned char *) buf->base);
          }
      }
    if (buf->base)
        free (buf->base); /* X-LINTED: FREE */
  }

//
// Enable reading on connection
//

static void accessReadStart (uv_tcp_t * client)
  {
    if (! client || uv_is_closing ((uv_handle_t *) client))
      return;
    uv_read_start ((uv_stream_t *) client, alloc_buffer, accessReadCallback);
  }

// Create and start a write request

static void accessStartWriteActual (uv_tcp_t * client, char * data,
                                        ssize_t datalen)
  {
    if (! client || uv_is_closing ((uv_handle_t *) client))
      return;
/* Suppress Clang Analyzer's possible memory leak warning */
#if !defined(__clang_analyzer__)
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
    uv_buf_t buf = uv_buf_init ((char *) malloc ((unsigned long) datalen),
                                                 (uint) datalen);
# if defined(USE_REQ_DATA)
    req->data = buf.base;
# endif /* if defined(USE_REQ_DATA) */
    memcpy (buf.base, data, (unsigned long) datalen);
//sim_printf ("write %u<%s>\r\n", datalen, data);
    int ret = uv_write (req, (uv_stream_t *) client, & buf, 1,
                        accessWriteCallback);
// There seems to be a race condition when Multics signals a disconnect_line;
// We close the socket, but Multics is still writing its goodbye text trailing
// NULs.
// If the socket has been closed, write will return BADF; just ignore it.
    if (ret < 0 && ret != -EBADF)
      sim_printf ("uv_write returns %d\r\n", ret);
#endif /* if !defined(__clang_analyzer__) */
  }

void accessStartWrite (uv_tcp_t * client, char * data, ssize_t datalen)
  {
    if ((client == NULL) || uv_is_closing ((uv_handle_t *) client))
      return;
    uv_access * access = (uv_access *) client->data;
    if (access->telnetp)
      telnet_send (access->telnetp, data, (size_t) datalen);
    else
      accessStartWriteActual (client, data, (ssize_t) datalen);
  }

static void accessPutCharForce (uv_access * access, char ch)
  {
    //sim_putchar (ch);
    accessStartWrite (access->client, & ch, 1);
  }

static void accessPutStrForce (uv_access * access, char * str)
  {
    size_t l = strlen (str);
    accessStartWrite (access->client, str, (ssize_t) l);
  }

void accessStartWriteStr (uv_tcp_t * client, char * data)
  {
    accessStartWrite (client, data, (ssize_t) strlen (data));
  }

static void accessLogon (uv_access * access, unsigned char * buf, ssize_t nread)
  {
    for (ssize_t nchar = 0; nchar < nread; nchar ++)
      {
        unsigned char kar = buf[nchar];
        // buffer too full for anything more?
        if ((unsigned long) access->pwPos >= sizeof (access->pwBuffer) - 1)
          {
            // yes. Only allow \n, \r, ^H, ^R
            switch (kar)
              {
                case '\b':  // backspace
                case 127:   // delete
                  {
                    // remove char from line
                    accessPutStrForce (access, "\b \b");
                    access->pwBuffer[access->pwPos] = 0;     // remove char from buffer
                    if (access->pwPos > 0)
                      access->pwPos -= 1;             // back up buffer pointer
                  }
                  break;

                case '\n':
                case '\r':
                  {
                    access->pwBuffer[access->pwPos] = 0;
                    goto check;
                  }

                case 0x12:  // ^R
                  {
                    accessPutStrForce (access, "^R\r\n"); // echo ^R
                    access->connectPrompt (access->client);
                    accessPutStrForce (access, access->pwBuffer);
                  }
                 break;

                default:
                  break;
              } // switch kar
            continue; // process next character in buffer
          } // if buffer full

        if (isprint (kar))   // printable?
          {
            accessPutCharForce (access, '*');
            access->pwBuffer[access->pwPos++] = (char) kar;
            access->pwBuffer[access->pwPos] = 0;
          }
        else
          {
            switch (kar)
              {
                case '\b':  // backspace
                case 127:   // delete
                  {
                    // remove char from line
                    accessPutStrForce (access, "\b \b");
                   // remove char from buffer
                    access->pwBuffer[access->pwPos] = 0;
                    if (access->pwPos > 0)
                      access->pwPos -= 1;   // back up buffer pointer
                  }
                  break;

                case '\n':
                case '\r':
                  {
                    access->pwBuffer[access->pwPos] = 0;
                    goto check;
                  }

                case 0x12:  // ^R
                  {
                    accessPutStrForce (access, "^R\r\n"); // echo ^R
                    access->connectPrompt (access->client);
                    accessPutStrForce (access, access->pwBuffer);
                  }
                  break;

                default:
                  break;
              } // switch kar
          } // not printable
      } // for nchar
    return;

check:;
    char cpy[access->pwPos + 1];
    memcpy (cpy, access->pwBuffer, (unsigned long) access->pwPos);
    cpy[access->pwPos] = 0;
    trim (cpy);
    //sim_printf ("<%s>", cpy);
    access->pwPos = 0;
    accessPutStrForce (access, "\r\n");

    if (strcmp (cpy, access->pw) == 0)
      {
        accessPutStrForce (access, "ok\r\n");
        sim_printf ("\r[OPC emulation: ACCESS GRANTED]\r\n");
        goto associate;
      }
    else
      {
        //accessPutStrForce ("<");
        //accessPutStrForce (access->pwBuffer);
        //accessPutStrForce (">\r\n");
        accessPutStrForce (access, "nope\r\n");
        sim_printf ("\r[OPC emulation: INVALID PASSWORD]\r\n");
        goto reprompt;
      }

reprompt:;
    access->connectPrompt (access->client);
    return;

associate:;
    access->loggedOn = true;
    if (access->connected)
      access->connected (access->client);
  }

static void accessCloseCallback (uv_handle_t * stream)
  {
    FREE (stream);
    //access->client = NULL;
  }

void accessCloseConnection (uv_stream_t* stream)
  {
    uv_access * access = (uv_access *) stream->data;
    sim_printf ("\r[OPC emulation: DISCONNECT]\r\n");
    // Clean up allocated data
    if (access->telnetp)
      {
        telnet_free (access->telnetp);
        access->telnetp = NULL;
      }
    if (! uv_is_closing ((uv_handle_t *) stream))
      uv_close ((uv_handle_t *) stream, accessCloseCallback);
    access->client = NULL;
  }

static void accessProcessInput (uv_access * access, unsigned char * buf,
                                 ssize_t nread)
  {
    if (access->inBuffer)
      {
        //sim_warn ("inBuffer overrun\r\n");
        unsigned char * new =
          realloc (access->inBuffer,
                   (unsigned long) (access->inSize + nread));
        if (! new)
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
        memcpy (new + access->inSize, buf, (unsigned long) nread);
        access->inSize += nread;
        access->inBuffer = new;
      }
    else
      {
        access->inBuffer = malloc ((unsigned long) nread);
        if (! access->inBuffer)
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
        memcpy (access->inBuffer, buf, (unsigned long) nread);
        access->inSize = (uint) nread;
        access->inUsed = 0;
      }

    // Prevent further reading until this buffer is consumed
    //fnpuv_read_stop (client);
    //if (! client || uv_is_closing ((uv_handle_t *) client))
      //return;
    //uv_read_stop ((uv_stream_t *) client);
  }

static void accessTelnetReadCallback (uv_tcp_t * client,
                            ssize_t nread,
                            unsigned char * buf)
  {
    uv_access * access = (uv_access *) client->data;
    if (access->loggedOn)
      accessProcessInput (access, buf, nread);
    else
      accessLogon (access, buf, nread);
  }

static void evHandler (UNUSED telnet_t *telnet, telnet_event_t *event,
                       void *user_data)
  {
    uv_tcp_t * client = (uv_tcp_t *) user_data;

    switch (event->type)
      {
        case TELNET_EV_DATA:
          {
            accessTelnetReadCallback (client, (ssize_t) event->data.size,
                            (unsigned char *)event->data.buffer);
          }
          break;

        case TELNET_EV_SEND:
          {
            accessStartWriteActual (client, (char *) event->data.buffer,
                                        (ssize_t) event->data.size);
          }
          break;

        case TELNET_EV_DO:
          {
            if (event->neg.telopt == TELNET_TELOPT_BINARY)
              {
                // DO Binary
              }
            else if (event->neg.telopt == TELNET_TELOPT_SGA)
              {
                // DO Suppress Go Ahead
              }
            else if (event->neg.telopt == TELNET_TELOPT_ECHO)
              {
                // DO Suppress Echo
              }
            else
              {
                sim_printf ("evHandler DO %d\r\n", event->neg.telopt);
              }
          }
          break;

        case TELNET_EV_DONT:
          {
            sim_printf ("evHandler DONT %d\r\n", event->neg.telopt);
          }
          break;

        case TELNET_EV_WILL:
          {
            if (event->neg.telopt != 3)
              sim_printf ("evHandler WILL %d\r\n", event->neg.telopt);
          }
          break;

        case TELNET_EV_WONT:
          {
            sim_printf ("evHandler WONT %d\r\n", event->neg.telopt);
          }
          break;

        case TELNET_EV_ERROR:
          {
            sim_warn ("libtelnet evHandler error <%s>\r\n", event->error.msg);
          }
          break;

        case TELNET_EV_IAC:
          {
            if (event->iac.cmd == 243 || // BRK
                event->iac.cmd == 244)   // IP
              {
                sim_warn ("libtelnet dropping unassociated BRK/IP\r\n");
              }
            else
              if ((!sim_quiet) || (event->iac.cmd != 241))
                sim_warn ("libtelnet unhandled IAC event %d\r\n", event->iac.cmd);
          }
          break;

        default:
          sim_printf ("evHandler: unhandled event %d\r\n", event->type);
          break;
      }

  }

static const telnet_telopt_t my_telopts[] =
  {
    { TELNET_TELOPT_SGA,       TELNET_WILL, TELNET_DO   },
    { TELNET_TELOPT_ECHO,      TELNET_WILL, TELNET_DONT },
  //{ TELNET_TELOPT_TTYPE,     TELNET_WONT, TELNET_DONT },
 //   { TELNET_TELOPT_BINARY,    TELNET_WILL, TELNET_DO   },
    { TELNET_TELOPT_BINARY,    TELNET_WONT, TELNET_DONT   },
  //{ TELNET_TELOPT_NAWS,      TELNET_WONT, TELNET_DONT },
    { -1, 0, 0 }
  };

static void * accessTelnetConnect (uv_tcp_t * client)
  {
    void * p = (void *) telnet_init (my_telopts, evHandler, 0, client);
    if (!p)
      {
        (void)fprintf(stderr, "\rtelnet_init failed at %s[%s:%d]\r\n",
                      __func__, __FILE__, __LINE__);
        return NULL;
      }
    const telnet_telopt_t * q = my_telopts;
    while (q->telopt != -1)
      {
        telnet_negotiate (p, q->us, (unsigned char) q->telopt);
        q ++;
      }
    return p;
  }

//
// Connection callback handler
//

static void onNewAccess (uv_stream_t * server, int status)
  {
    if (status < 0)
      {
        (void)fprintf (stderr, "\r[OPC emulation: new connection error %s]\r\n", uv_strerror(status));
        // error!
        return;
      }

    uv_access * access = (uv_access *) server->data;

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
    uv_tcp_init (access->loop, client);
    client->data = server->data;
    if (uv_accept (server, (uv_stream_t *) client) == 0)
      {
        // Only a single connection at a time
        if (access->client)
          {
            sim_printf ("\r\n[OPC emulation: BUMPED]\r\n\r\n");
            accessPutStrForce (access, "\r[OPC emulation: BUMPED]\r\n");
            access->loggedOn = false;
            sim_printf ("\rMultics has disconnected you\r\n");
            accessCloseConnection ((uv_stream_t *) access->client);
          }
        access->client = client;
        uv_tcp_nodelay (client, 1);
        struct sockaddr name;
        int namelen = sizeof (name);
        int ret = uv_tcp_getpeername (access->client, & name, & namelen);
        if (ret < 0)
          {
            sim_printf ("\r[OPC emulation: CONNECT; addr err %d]\r\n", ret);
          }
        else
          {
            struct sockaddr_in * p = (struct sockaddr_in *) & name;
            sim_printf ("\r[OPC emulation: CONNECT %s]\r\n", inet_ntoa (p -> sin_addr));
          }

        if (access->useTelnet)
          {
            access->telnetp = accessTelnetConnect (access->client);
            if (!access->telnetp)
              {
                 sim_warn ("ltnConnect failed\r\n");
                 return;
              }
          }
        else
          {
            access->telnetp = NULL;
          }
        access->loggedOn =
          ! strlen (access->pw);
        if (access->loggedOn)
          access->connected (access->client);
        else
          access->connectPrompt (access->client);
        accessReadStart (access->client);
      }
    else
      {
        uv_close ((uv_handle_t *) client, accessCloseCallback);
      }
  }

void uv_open_access (uv_access * access)
  {
    if (access->open == true)
      {
        sim_printf ("\r[OPC emulation: OPC already initialized]\r\n");
      }

    if (! access->port)
      {
        //sim_printf ("access port disabled\r\n");
        return;
      }

    if (! access->loop)
      access->loop = uv_default_loop ();

    // Do we already have a listening port (ie not first time)?
    if (access->open)
      return;

    uv_tcp_init (access->loop, & access->server);
    access->server.data = (void *) access;
    struct sockaddr_in addr;
    uv_ip4_addr (access->address, access->port, & addr);
    uv_tcp_bind (& access->server, (const struct sockaddr *) & addr, 0);
#define DEFAULT_BACKLOG 1024
    int r = uv_listen ((uv_stream_t *) & access->server,
                       DEFAULT_BACKLOG,
                       onNewAccess);
    if (r)
     {
        (void)fprintf (stderr, "\r[OPC emulation: listen error: %s:%ld: %s]\r\n",
                       access->address, (long) access->port, uv_strerror(r));
      }
    access->open = true;
    if (access->address != NULL)
      sim_printf ("\r[OPC emulation: TELNET server listening on %s:%ld]\r\n",
                  access->address, (long) access->port);
    else
      sim_printf ("\r[OPC emulation: TELNET server listening on 0.0.0.0:%ld]\r\n",
                  (long) access->port);
  }

#if !defined(QUIET_UNUSED)
void accessPutChar (uv_access * access, char ch)
  {
    //sim_putchar (ch);
    if (access->loggedOn)
      accessStartWrite (access->client, & ch, 1);
  }
#endif /* if !defined(QUIET_UNUSED) */

int accessGetChar (uv_access * access)
  {
    // The connection could have closed when we were not looking
    if (! access->client)
      {
        if (access->inBuffer)
          FREE (access->inBuffer);
        access->inBuffer = NULL;
        access->inSize = 0;
        access->inUsed = 0;
        return SCPE_OK;
      }

    if (access->inBuffer && access->inUsed < access->inSize)
       {
         unsigned char c = access->inBuffer[access->inUsed ++];
         if (access->inUsed >= access->inSize)
           {
             FREE (access->inBuffer);
             access->inBuffer = NULL;
             access->inSize = 0;
             access->inUsed = 0;
             // The connection could have been closed when we weren't looking
             //if (access->client)
               //fnpuv_read_start (access->client);
           }
         return (int) c + SCPE_KFLAG;
       }
    return SCPE_OK;

  }

#if !defined(QUIET_UNUSED)
void accessPutStr (uv_access * access, char * str)
  {
    size_t l = strlen (str);
    //for (size_t i = 0; i < l; i ++)
      //sim_putchar (str[i]);
    if (access->loggedOn)
      accessStartWrite (access->client, str, (ssize_t) l);
  }
#endif /* if !defined(QUIET_UNUSED) */
