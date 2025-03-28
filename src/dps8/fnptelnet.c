/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 24f735f0-f62f-11ec-b8af-80ee73e9b8e7
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

#include <stdio.h>
#include <signal.h>

#include "dps8.h"
#include "dps8_sys.h"
#include "dps8_iom.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_fnp2.h"
#include "dps8_utils.h"
#include "fnpuv.h"
#include "fnptelnet.h"

static const telnet_telopt_t my_telopts[] = {
    { TELNET_TELOPT_SGA,       TELNET_WILL, TELNET_DO   },
    { TELNET_TELOPT_ECHO,      TELNET_WILL, TELNET_DONT },
  //{ TELNET_TELOPT_TTYPE,     TELNET_WONT, TELNET_DONT },
    { TELNET_TELOPT_BINARY,    TELNET_WILL, TELNET_DO   },
  //{ TELNET_TELOPT_NAWS,      TELNET_WONT, TELNET_DONT },
    { -1, 0, 0 }
  };

static const telnet_telopt_t my_3270telopts[] = {
    { TELNET_TELOPT_TTYPE,     TELNET_WILL, TELNET_DO },
    { TELNET_TELOPT_BINARY,    TELNET_WILL, TELNET_DO },
    { TELNET_TELOPT_EOR,       TELNET_WILL, TELNET_DO },
    { -1, 0, 0 }
  };

static void evHandler (UNUSED telnet_t *telnet, telnet_event_t *event, void *user_data)
  {
    uv_tcp_t * client = (uv_tcp_t *) user_data;
    switch (event->type)
      {
        case TELNET_EV_DATA:
          {
            if (! client || ! client->data)
              {
                sim_warn ("evHandler TELNET_EV_DATA bad client data\r\n");
                return;
              }
            uvClientData * p = (uvClientData *) client->data;
            (* p->read_cb) (client, (ssize_t) event->data.size, (unsigned char *)event->data.buffer);
          }
          break;

        case TELNET_EV_SEND:
          {
            //sim_printf ("evHandler: send %zu <%s>\r\n", event->data.size, event->data.buffer);
            //fnpuv_start_write_actual (client, (char *) event->data.buffer, (ssize_t) event->data.size);
            if (! client || ! client->data)
              {
                sim_warn ("evHandler TELNET_EV_SEND bad client data\r\n");
                return;
              }
            uvClientData * p = client->data;
            (* p->write_actual_cb) (client, (unsigned char *) event->data.buffer, (ssize_t) event->data.size);
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
            else if (event->neg.telopt == TELNET_TELOPT_EOR)
              {
                //sim_printf ("EOR rcvd\r\n");
                //fnpuv_recv_eor (client);
                // DO EOR
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
            if (event->neg.telopt == TELNET_TELOPT_BINARY)
              {
                // WILL BINARY
              }
            else if (event->neg.telopt == TELNET_TELOPT_TTYPE)
              {
                // WILL TTYPE
              }
            else if (event->neg.telopt == TELNET_TELOPT_EOR)
              {
                // WILL EOR
              }
            else
              {
                if (event->neg.telopt != 3)
                  sim_printf ("evHandler WILL %d\r\n", event->neg.telopt);
              }
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
            if (event->iac.cmd == TELNET_BREAK ||
                event->iac.cmd == TELNET_IP)
              {
                if (! client || ! client->data)
                  {
                    sim_warn ("evHandler TELNET_EV_IAC bad client data\r\n");
                    return;
                  }
                uvClientData * p = (uvClientData *) client->data;
                if (p -> assoc)
                  {
                    fnpuv_associated_brk (client);
                  }
                else
                  sim_warn ("libtelnet dropping unassociated BRK\r\n");
              }
            else if (event->iac.cmd == TELNET_EOR)
              {
                fnpuv_recv_eor (client);
              }
            else
              if ((!sim_quiet) || (event->iac.cmd != 241))
                sim_warn ("libtelnet unhandled IAC event %d\r\n", event->iac.cmd);
          }
          break;

        case TELNET_EV_TTYPE:
          {
            if (! client || ! client->data)
              {
                sim_warn ("evHandler TELNET_EV_IAC bad client data\r\n");
                return;
              }
            uvClientData * p = (uvClientData *) client->data;
            p->ttype = strdup (event->ttype.name);
            if (!p->ttype)
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
          }
          break;

        case TELNET_EV_SUBNEGOTIATION:
          {
            /* no subnegotiation */
          }
          break;

        default:
          sim_printf ("evHandler: unhandled event %d\r\n", event->type);
          break;
      }

  }

void * ltnConnect (uv_tcp_t * client)
  {
    void * p = (void *) telnet_init (my_telopts, evHandler, 0, client);
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
    const telnet_telopt_t * q = my_telopts;
    while (q->telopt != -1)
      {
        telnet_negotiate (p, q->us, (unsigned char) q->telopt);
        q ++;
      }
    return p;
  }

void * ltnConnect3270 (uv_tcp_t * client)
  {
    void * p = (void *) telnet_init (my_3270telopts, evHandler, 0, client);
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

    // This behavior is copied from Hercules.
    telnet_negotiate (p, TELNET_DO, (unsigned char) TELNET_TELOPT_TTYPE);
    telnet_begin_sb  (p, TELNET_TELOPT_TTYPE);
    const char ttype [1] = { 1 };
    telnet_send (p, ttype, 1);
    telnet_finish_sb (p);
    telnet_negotiate (p, TELNET_WILL, (unsigned char) TELNET_TELOPT_BINARY);
    telnet_negotiate (p, TELNET_DO, (unsigned char) TELNET_TELOPT_BINARY);
    telnet_negotiate (p, TELNET_WILL, (unsigned char) TELNET_TELOPT_EOR);
    telnet_negotiate (p, TELNET_DO, (unsigned char) TELNET_TELOPT_EOR);

    return p;
  }

void ltnEOR (telnet_t * tclient)
  {
    telnet_iac (tclient, TELNET_EOR);
  }

void ltnDialout (UNUSED telnet_t * tclient)
  {
    // dialout telnet: We are a teletype. What settings should we be doing?
    //telnet_negotiate (tclient, TELNET_WILL, TELNET_TELOPT_SGA);
  }

void fnpTelnetInit (void)
  {
#if 0
    for (int fnpno = 0; fnpno < N_FNP_UNITS_MAX; fnpno ++)
      {
        for (int lineno = 0; lineno < MAX_LINES; lineno ++)
          {
            fnpUnitData[fnpno].MState.line[lineno].telnetp = telnet_init (my_telopts, evHandler, 0, NULL);
            if (! fnpUnitData[fnpno].MState.line[lineno].telnetp)
              {
                sim_warn ("telnet_init failed\r\n");
              }
          }
      }
#endif
  }

void fnp3270Init (void)
  {
  }
