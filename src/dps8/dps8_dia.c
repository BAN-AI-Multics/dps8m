//#define dumpseg
//#define DDBG(x) x
#define DDBG(x) 
/*
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2017 Charles Anthony
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 */

#include <stdio.h>
#include <ctype.h>

#include "dps8.h"
#include "dps8_sys.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_cpu.h"
#include "dps8_iom.h"
#include "dps8_dia.h"
#include "dps8_cable.h"
#include "dps8_utils.h"
#include "dnpkt.h"
#define DBG_CTR 1

#ifdef THREADZ
# include "threadz.h"
#endif

#define ASSUME0 0 // XXX

// libuv interface

//
// allocBuffer: libuv callback handler to allocate buffers for incomingd data.
//

static void allocBuffer (UNUSED uv_handle_t * handle, size_t suggested_size, uv_buf_t * buf) {
  suggested_size = sizeof (dnPkt);
  char * p = (char *) malloc (suggested_size);
  if (! p) {
     sim_warn ("%s malloc fail\r\n", __func__);
  }
  * buf = uv_buf_init (p, (uint) suggested_size);
}

/* sim_parse_addr       host:port

   Presumption is that the input, if it doesn't contain a ':' character is a port specifier.
   If the host field contains one or more colon characters (i.e. it is an IPv6 address),
   the IPv6 address MUST be enclosed in square bracket characters (i.e. Domain Literal format)

   Inputs:
        cptr    =       pointer to input string
        default_host
                =       optional pointer to default host if none specified
        host_len =      length of host buffer
        default_port
                =       optional pointer to default port if none specified
        port_len =      length of port buffer
        validate_addr = optional name/addr which is checked to be equivalent
                        to the host result of parsing the other input.  This
                        address would usually be returned by sim_accept_conn.
   Outputs:
        host    =       pointer to buffer for IP address (may be NULL), 0 = none
        port    =       pointer to buffer for IP port (may be NULL), 0 = none
        result  =       status (0 on complete success or -1 if
                        parsing can't happen due to bad syntax, a value is
                        out of range, a result can't fit into a result buffer,
                        a service name doesn't exist, or a validation name
                        doesn't match the parsed host)
*/

static int sim_parse_addr (const char *cptr, char *host, size_t host_len, const char *default_host, char *port, size_t port_len, const char *default_port, const char *validate_addr) {
  char gbuf[CBUFSIZE], default_pbuf[CBUFSIZE];
  const char *hostp;
  char *portp;
  char *endc;
  unsigned long portval;

  if ((host != NULL) && (host_len != 0))
    memset (host, 0, host_len);
  if ((port != NULL) && (port_len != 0))
    memset (port, 0, port_len);
  if ((cptr == NULL) || (*cptr == 0)) {
    if (((default_host == NULL) || (*default_host == 0)) || ((default_port == NULL) || (*default_port == 0)))
      return -1;
    if ((host == NULL) || (port == NULL))
      return -1;                                  /* no place */
    if ((strlen(default_host) >= host_len) || (strlen(default_port) >= port_len))
      return -1;                                  /* no room */
    strcpy (host, default_host);
    strcpy (port, default_port);
    return 0;
  }
  memset (default_pbuf, 0, sizeof (default_pbuf));
  if (default_port)
    strncpy (default_pbuf, default_port, sizeof (default_pbuf)-1);
  gbuf[sizeof (gbuf)-1] = '\0';
  strncpy (gbuf, cptr, sizeof (gbuf)-1);
  hostp = gbuf;                                           /* default addr */
  portp = NULL;
  if ((portp = strrchr (gbuf, ':')) &&                    /* x:y? split */
      (NULL == strchr (portp, ']'))) {
    *portp++ = 0;
    if (*portp == '\0')
      portp = default_pbuf;
  } else {                                                  /* No colon in input */
    portp = gbuf;                                       /* Input is the port specifier */
    hostp = (const char *)default_host;                 /* host is defaulted if provided */
  }
  if (portp != NULL) {
    portval = strtoul (portp, &endc, 10);
    if ((*endc == '\0') && ((portval == 0) || (portval > 65535)))
      return -1;                                      /* numeric value too big */
    if (*endc != '\0') {
      struct servent *se = getservbyname (portp, "tcp");

      if (se == NULL)
        return -1;                                  /* invalid service name */
    }
  }
  if (port)                                               /* port wanted? */
    if (portp != NULL) {
      if (strlen (portp) >= port_len)
        return -1;                                  /* no room */
      else
        strcpy (port, portp);
    }
  if (hostp != NULL) {
    if (']' == hostp[strlen (hostp)-1]) {
      if ('[' != hostp[0])
        return -1;                                  /* invalid domain literal */
      /* host may be the const default_host so move to temp buffer before modifying */
      strncpy (gbuf, hostp+1, sizeof (gbuf)-1);         /* remove brackets from domain literal host */
      gbuf[strlen (gbuf)-1] = '\0';
      hostp = gbuf;
    }
  }
  if (host) {                                             /* host wanted? */
    if (hostp != NULL) {
      if (strlen (hostp) >= host_len)
        return -1;                                  /* no room */
      else
        if (('\0' != hostp[0]) || (default_host == NULL))
          strcpy (host, hostp);
        else if (strlen (default_host) >= host_len)
          return -1;                          /* no room */
        else
          strcpy (host, default_host);
    } else {
      if (default_host) {
        if (strlen (default_host) >= host_len)
          return -1;                              /* no room */
        else
          strcpy (host, default_host);
      }
    }
  }
  if (validate_addr) {
    struct addrinfo *ai_host, *ai_validate, *ai, *aiv;
    int status;

    if (hostp == NULL)
      return -1;
    if (getaddrinfo (hostp, NULL, NULL, & ai_host))
      return -1;
    if (getaddrinfo (validate_addr, NULL, NULL, & ai_validate))
      return -1;
    status = -1;
    for (ai = ai_host; ai != NULL; ai = ai->ai_next) {
      for (aiv = ai_validate; aiv != NULL; aiv = aiv->ai_next) {
        if ((ai->ai_addrlen == aiv->ai_addrlen) &&
            (ai->ai_family == aiv->ai_family) &&
            (0 == memcmp (ai->ai_addr, aiv->ai_addr, ai->ai_addrlen))) {
          status = 0;
          break;
        }
      }
    }
    if (status != 0) {
      /* be generous and allow successful validations against variations of localhost addresses */
      if (((0 == strcmp ("127.0.0.1", hostp)) &&
           (0 == strcmp ("::1", validate_addr))) ||
          ((0 == strcmp ("127.0.0.1", validate_addr)) &&
           (0 == strcmp ("::1", hostp))))
        status = 0;
    }
    return status;
  }
  return 0;
}

// From sihm udplib

static int tcp_parse_remote (const char * premote, char * rhost, size_t rhostl, int32_t * rport) {
  // This routine will parse a remote address string in any of these forms -
  //
  //            :w.x.y.z:rrrr
  //            :name.domain.com:rrrr
  //            :rrrr
  //            w.x.y.z:rrrr
  //            name.domain.com:rrrr
  //
  // In all examples, "rrrr" is the remote port number that we use for transmitting.  
  // "w.x.y.z" is a dotted IP for the remote machine
  // and "name.domain.com" is its name (which will be looked up to get the IP).
  // If the host name/IP is omitted then it defaults to "localhost".

  char host [64], port [16];
  if (* premote == '\0')
    return -1;
  * rhost = '\0';

  if (sim_parse_addr (premote, host, sizeof (host), "localhost", port, sizeof (port), NULL, NULL))
    return -1;
  strncpy (rhost, host, rhostl);
  * rport = atoi (port);
  return 0;
}

#if 0
static inline void fnp_core_read (word24 addr, word36 *data, UNUSED const char * ctx)
  {
#ifdef THREADZ
    lock_mem_rd ();
#endif
#ifdef SCUMEM
    iom_core_read (addr, data, ctx);
#else
    * data = M [addr] & DMASK;
#endif
#ifdef THREADZ
    unlock_mem ();
#endif
  }
#endif

#define N_DIA_UNITS 1 // default
#define DIA_UNIT_IDX(uptr) ((uptr) - dia_unit)

static config_list_t dia_config_list [] =
  {
    /*  0 */ { "mailbox", 0, 07777, NULL },
    { NULL, 0, 0, NULL }
  };

static t_stat set_config (UNIT * uptr, UNUSED int value, const char * cptr, UNUSED void * desc)
  {
    uint dia_unit_idx = (uint) DIA_UNIT_IDX (uptr);
    //if (dia_unit_idx >= dia_dev.numunits)
    if (dia_unit_idx >= N_DIA_UNITS_MAX)
      {
        sim_debug (DBG_ERR, & dia_dev, "DIA SET CONFIG: Invalid unit number %d\n", dia_unit_idx);
        sim_printf ("error: DIA SET CONFIG: invalid unit number %d\n", dia_unit_idx);
        return SCPE_ARG;
      }

    struct dia_unit_data * dudp = dia_data.dia_unit_data + dia_unit_idx;

    config_state_t cfg_state = { NULL, NULL };

    for (;;)
      {
        int64_t v;
        int rc = cfg_parse ("DIA SET CONFIG", cptr, dia_config_list, & cfg_state, & v);
        switch (rc)
          {
            case -2: // error
              cfg_parse_done (& cfg_state);
              return SCPE_ARG;

            case -1: // done
              break;

            case 0: // mailbox
              dudp -> mailboxAddress = (uint) v;
              break;

            default:
              sim_printf ("error: DIA SET CONFIG: invalid cfg_parse rc <%d>\n", rc);
              cfg_parse_done (& cfg_state);
              return SCPE_ARG;
          } // switch
        if (rc < 0)
          break;
      } // process statements
    cfg_parse_done (& cfg_state);
    return SCPE_OK;
  }

static t_stat show_config (UNUSED FILE * st, UNIT * uptr, UNUSED int val,
                           UNUSED const void * desc)
  {
    long unit_idx = DIA_UNIT_IDX (uptr);
    if (unit_idx >= (long) N_DIA_UNITS_MAX)
      {
        sim_debug (DBG_ERR, & dia_dev,
                   "DIA SHOW CONFIG: Invalid unit number %ld\n", unit_idx);
        sim_printf ("error: invalid unit number %ld\n", unit_idx);
        return SCPE_ARG;
      }

    sim_printf ("DIA unit number %ld\n", unit_idx);
    struct dia_unit_data * dudp = dia_data.dia_unit_data + unit_idx;

    sim_printf ("DIA Mailbox Address:         %04o(8)\n", dudp -> mailboxAddress);

    return SCPE_OK;
  }


static t_stat show_status (UNUSED FILE * st, UNIT * uptr, UNUSED int val,
                             UNUSED const void * desc)
  {
    long dia_unit_idx = DIA_UNIT_IDX (uptr);
    if (dia_unit_idx >= (long) dia_dev.numunits)
      {
        sim_debug (DBG_ERR, & dia_dev,
                   "DIA SHOW STATUS: Invalid unit number %ld\n", dia_unit_idx);
        sim_printf ("error: invalid unit number %ld\n", dia_unit_idx);
        return SCPE_ARG;
      }

    sim_printf ("DIA unit number %ld\n", dia_unit_idx);
    struct dia_unit_data * dudp = dia_data.dia_unit_data + dia_unit_idx;

    sim_printf ("mailboxAddress:              %04o\n", dudp->mailboxAddress);
    return SCPE_OK;
  }

static t_stat show_nunits (UNUSED FILE * st, UNUSED UNIT * uptr,
                           UNUSED int val, UNUSED const void * desc)
  {
    sim_printf("Number of DIA units in system is %d\n", dia_dev.numunits);
    return SCPE_OK;
  }

static t_stat set_nunits (UNUSED UNIT * uptr, UNUSED int32 value,
                             const char * cptr, UNUSED void * desc)
  {
    if (! cptr)
      return SCPE_ARG;
    int n = atoi (cptr);
    if (n < 1 || n > N_DIA_UNITS_MAX)
      return SCPE_ARG;
    dia_dev.numunits = (uint32) n;
    return SCPE_OK;
  }

static MTAB dia_mod [] =
  {
    {
      MTAB_XTD | MTAB_VUN | MTAB_NMO | MTAB_VALR, /* mask */
      0,            /* match */
      "CONFIG",     /* print string */
      "CONFIG",         /* match string */
      set_config,         /* validation routine */
      show_config, /* display routine */
      NULL,          /* value descriptor */
      NULL   // help string
    },

    {
      MTAB_XTD | MTAB_VUN | MTAB_NMO | MTAB_VALR, /* mask */
      0,            /* match */
      "STATUS",     /* print string */
      "STATUS",         /* match string */
      NULL,         /* validation routine */
      show_status, /* display routine */
      NULL,          /* value descriptor */
      NULL   // help string
    },

    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO | MTAB_VALR, /* mask */
      0,            /* match */
      "NUNITS",     /* print string */
      "NUNITS",         /* match string */
      set_nunits, /* validation routine */
      show_nunits, /* display routine */
      "Number of DIA units in the system", /* value descriptor */
      NULL          // help
    },
    { 0, 0, NULL, NULL, NULL, NULL, NULL, NULL }
  };


UNIT dia_unit [N_DIA_UNITS_MAX] = {
    {UDATA (NULL, UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL}
};

static DEBTAB dia_DT [] =
  {
    { "TRACE", DBG_TRACE, NULL },
    { "NOTIFY", DBG_NOTIFY, NULL },
    { "INFO", DBG_INFO, NULL },
    { "ERR", DBG_ERR, NULL },
    { "WARN", DBG_WARN, NULL },
    { "DEBUG", DBG_DEBUG, NULL },
    { "ALL", DBG_ALL, NULL }, // don't move as it messes up DBG message
    { NULL, 0, NULL }
  };


static t_stat reset (UNUSED DEVICE * dptr)
  {
    return SCPE_OK;
  }


static void dnSendCB (uv_write_t * req, int status) {
  if (status)
    sim_printf ("send cb %d\r\n", status);
}

static void dnSend (uint diaUnitIdx, dnPkt * pkt) {
sim_printf ("DEBUG: >%s\r\n", (char *) pkt);
  struct dia_unit_data * dudp = & dia_data.dia_unit_data[diaUnitIdx];
  uv_write_t * req = malloc (sizeof (uv_write_t));
  if (! req) {
    sim_printf ("%s req malloc fail\r\n", __func__);
    return;
  }
  // set base to null
  memset (req, 0, sizeof (uv_write_t));

  void * p = malloc (sizeof (dnPkt));
  if (! p) {
    sim_printf ("%s buf malloc fail\r\n", __func__);
    return;
  }
  uv_buf_t buf = uv_buf_init ((char *) p, (uint) sizeof (dnPkt));

  memcpy (buf.base, pkt, sizeof (dnPkt));
  int rc = uv_write (req, (uv_stream_t *) & dudp->tcpHandle, & buf, 1, dnSendCB);
  if (rc < 0) {
    fprintf (stderr, "%s uv_udp_send failed %d\n", __func__, rc);
  }
}

static void processCmdR (uv_stream_t * req, dnPkt * pkt) {
  diaClientData * cdp = (diaClientData *) req->data;
  if (cdp->magic != DIA_CLIENT_MAGIC)
    sim_printf ("ERROR: %s no magic\r\n", __func__);
  //struct dia_unit_data * dudp = & dia_data.dia_unit_data[cdp->diaUnitIdx];
  word24 addr;
  int n = sscanf (pkt->cmdR.addr, "%o", & addr);
  if (n != 1) {
    sim_printf ("%s unable to extract address\r\n", __func__);
  } else {
    word36 data;
    //iom_chan_data_t * p = & iom_chan_data[cdp->iomUnitIdx][cdp->chan];
//sim_printf ("%s %o %o %o\n", __func__, p->PCW_63_PTP, p->PCW_64_PGE, p->PCW_PAGE_TABLE_PTR);
//sim_printf ("%s tally %o addr %o\n", __func__, p->DDCW_TALLY, p->DDCW_ADDR);
    iom_direct_data_service (cdp->iomUnitIdx, cdp->chan, addr, & data, direct_load);

    dnPkt pkt;
    memset (& pkt, 0, sizeof (pkt));
    pkt.cmd = DN_CMD_DATA;
    sprintf (pkt.cmdD.data, "%08o:%012llo", addr, data);
    dnSend (cdp->diaUnitIdx, & pkt);
  }
}

static void processCmdW (uv_stream_t * req, dnPkt * pkt) {

// IDB: 000033000100 100501017600 000000000001
//for (uint i = 0; i < 513000; i ++) {
  //word36 w = M[i] & MASK36;
  //if (w == 0000033000100 || w == 0100501017600 || w == 0000000000001) {
    //sim_printf ("%08o:%012llo\r\n", i, w);
  //}
//}

  diaClientData * cdp = (diaClientData *) req->data;
  if (cdp->magic != DIA_CLIENT_MAGIC)
    sim_printf ("ERROR: %s no magic\r\n", __func__);
  word24 addr;
  word36 data;
  int n = sscanf (pkt->cmdW.data, "%08o:%012llo", & addr, & data);
  if (n != 2) {
    sim_printf ("%s unable to extract address\r\n", __func__);
  } else {
    iom_direct_data_service (cdp->iomUnitIdx, cdp->chan, addr, & data, direct_store);
sim_printf ("DEBUG: %s store %08o:%012llo\r\n", __func__, addr, data);
  }
}

void processCmdSEC (uv_stream_t * req, dnPkt * pkt) {
  //sim_printf ("DEBUG:disconnect\r\n");
  diaClientData * cdp = (diaClientData *) req->data;
  if (cdp->magic != DIA_CLIENT_MAGIC)
    sim_printf ("ERROR: %s no magic\r\n", __func__);
  uint cell = 3;
  int n = sscanf (pkt->cmdS.cell, "%o", & cell);
  if (n != 1) {
    sim_printf ("%s unable to extract cell\r\n", __func__);
  }
  send_special_interrupt (cdp->iomUnitIdx, cdp->chan, cell, 0, 0);
}

void processCmdDis (uv_stream_t * req, dnPkt * pkt) {
  //sim_printf ("DEBUG:disconnect\r\n");
  diaClientData * cdp = (diaClientData *) req->data;
  if (cdp->magic != DIA_CLIENT_MAGIC)
    sim_printf ("ERROR: %s no magic\r\n", __func__);
  send_terminate_interrupt (cdp->iomUnitIdx, cdp->chan);
}

static void processRecv (uv_stream_t * req, dnPkt * pkt, ssize_t nread) {
  if (nread != sizeof (dnPkt)) {
    sim_printf ("%s incoming packet size %ld, expected %ld\r\n", __func__, nread, sizeof (dnPkt));
for (uint i = 0; i < nread; i ++) {
  unsigned char ch = ((unsigned char *) pkt) [i];
  sim_printf ("%4d %03o ", i, ch);
  if (isprint (ch))
    sim_printf ("'%c'\r\n", ch);
  else
    sim_printf ("'\%03o'\r\n", ch);
}
  }
sim_printf ("DEBUG: <%s\r\n", (char *) pkt);
  if (pkt->cmd == DN_CMD_READ) {
    processCmdR (req, pkt);
  } else if (pkt->cmd == DN_CMD_WRITE) {
    processCmdW (req, pkt);
  } else if (pkt->cmd == DN_CMD_DISCONNECT) {
    processCmdDis (req, pkt);
  } else if (pkt->cmd == DN_CMD_SET_EXEC_CELL) {
    processCmdSEC (req, pkt);
  } else {
    sim_printf ("Ignoring cmd %c %o\r\n", isprint (pkt->cmd) ? pkt->cmd : '.', pkt->cmd);
  }
}

static void onRead (uv_stream_t* req, ssize_t nread, const uv_buf_t* buf) {
  if (nread < 0) {
    fprintf (stderr, "Read error!\n");
    if (! uv_is_closing ((uv_handle_t *) req))
      uv_close ((uv_handle_t *) req, NULL);
    free (buf->base);
    return;
  }
  if (nread > 0) {
    if (! req) {
      sim_printf ("bad req\r\n");
      return;
    } else {
      //sim_printf ("recv %ld %c\r\n", nread, buf->base[0]);
      processRecv (req, (dnPkt *) buf->base, nread);
    }
  } else {
    //sim_printf ("recv empty\r\n");
  }
  if (buf->base)
    free (buf->base);
  return;
}

static void onConnect (uv_connect_t * server, int status) {
  if (status < 0) {
    sim_printf ("connected %d\n", status);
    return;
  }
  diaClientData * cdp = (diaClientData *) server->data;
  if (cdp->magic != DIA_CLIENT_MAGIC)
    sim_printf ("ERROR: %s no magic\r\n", __func__);
  struct dia_unit_data * dudp = & dia_data.dia_unit_data[cdp->diaUnitIdx];
  //uv_tcp_nodelay ((uv_tcp_t *) & dudp->tcpHandle, 1);
  //uv_stream_set_blocking ((uv_stream_t *) & dudp->tcpHandle, 1);
  uv_read_start ((uv_stream_t *) & dudp->tcpHandle, allocBuffer, onRead);
}

static int dia_connect (UNIT * uptr) {
  int diaUnitIdx = (int) (uptr - dia_unit);
  struct dia_unit_data * dudp = & dia_data.dia_unit_data[diaUnitIdx];

// Set up send

  int rc = uv_tcp_init (uv_default_loop (), & dudp->tcpHandle);
  if (rc) {
    sim_printf ("%s unable to init tcpHandle %d\r\n", __func__, rc);
    return -1;
  }

  struct sockaddr_in raddr;
  uv_ip4_addr (dudp->rhost, dudp->rport, & raddr);

  rc = uv_tcp_init (uv_default_loop (), & dudp->tcpHandle);
  if (rc) {
    sim_printf ("%s unable to connect tcpHandle %d\r\n", __func__, rc);
    return -1;
  }

  struct ctlr_to_iom_s * there = & cables->dia_to_iom[diaUnitIdx][ASSUME0];
//sim_printf ("DIAA at %u %u\r\n", there->iom_unit_idx, there->chan_num);
  dudp->clientData.magic = DIA_CLIENT_MAGIC;
  dudp->clientData.iomUnitIdx = there->iom_unit_idx;
  dudp->clientData.chan = there->chan_num;
  dudp->clientData.diaUnitIdx = diaUnitIdx;
  dudp->reqConnect.data = & dudp->clientData;
  dudp->tcpHandle.data = & dudp->clientData;

  rc = uv_tcp_connect (& dudp->reqConnect, & dudp->tcpHandle, (struct sockaddr *) & raddr, onConnect);
  if (rc) {
    sim_printf ("%s unable to connect udpSendHandle %d\r\n", __func__, rc);
    return -1;
  }
  uptr->flags |= UNIT_ATT;
  return 0;
}

static t_stat dia_attach (UNIT * uptr, const char * cptr) {
  if (! cptr)
    return SCPE_ARG;
  int diaUnitIdx = (int) (uptr - dia_unit);
  struct dia_unit_data * dudp = & dia_data.dia_unit_data[diaUnitIdx];


  // ATTACH DNn w.x.y.z:rrrr - connect via TCp to a remote simh host

  // If we're already attached, then detach ...
  if ((uptr->flags & UNIT_ATT) != 0)
    detach_unit (uptr);

  // Make a copy of the "file name" argument.  dn_udp_create() actually modifies
  // the string buffer we give it, so we make a copy now so we'll have
  // something to display in the "SHOW DNn ..." command.
  strncpy (dudp->attachAddress, cptr, ATTACH_ADDRESS_SZ);
  uptr->filename = dudp->attachAddress;

  int rc = tcp_parse_remote (cptr, dudp->rhost, sizeof (dudp->rhost), & dudp->rport);
  if (rc) {
    sim_warn ("%s unable to parse address %d\r\n", __func__, rc);
    return SCPE_ARG;
  }
  // sim_printf ("%d %d:%s:%d\r\n", rc, lport, dudp->rhost, dudp->rport);

  rc = dia_connect (uptr);
  //if (rc == 0) {
    //uptr->flags |= UNIT_ATT;
    //dudp->connected = true;
  //}
  return SCPE_OK;
}

static void close_cb (uv_handle_t * stream) {
sim_printf ("close cb\r\n");
  diaClientData * cdp = (diaClientData *) stream->data;
  if (cdp->magic != DIA_CLIENT_MAGIC)
    sim_printf ("ERROR: %s no magic\r\n", __func__);
  dia_unit[cdp->diaUnitIdx].flags &= ~ (unsigned int) UNIT_ATT;
  free (stream);
}

// Detach (connect) ...
static t_stat dia_detach (UNIT * uptr) {
  int diaUnitIdx = (int) (uptr - dia_unit);
  struct dia_unit_data * dudp = & dia_data.dia_unit_data[diaUnitIdx];
  if ((uptr->flags & UNIT_ATT) == 0)
    return SCPE_OK;
  //if (! dudp->connected)
    //return SCPE_OK;

  if (! uv_is_closing ((uv_handle_t *) & dudp->tcpHandle))
    uv_close ((uv_handle_t *) & dudp->tcpHandle, close_cb);

  //dudp->connected = false;
  uptr->flags &= ~ (unsigned int) UNIT_ATT;
  //free (uptr->filename);
  //uptr->filename = NULL;
  return SCPE_OK;
}

DEVICE dia_dev = {
    "DIA",           /* name */
    dia_unit,        /* units */
    NULL,            /* registers */
    dia_mod,         /* modifiers */
    N_DIA_UNITS,     /* #units */
    10,              /* address radix */
    31,              /* address width */
    1,               /* address increment */
    8,               /* data radix */
    9,               /* data width */
    NULL,            /* examine routine */
    NULL,            /* deposit routine */
    reset,           /* reset routine */
    NULL,            /* boot routine */
    dia_attach,          /* attach routine */
    dia_detach,          /* detach routine */
    NULL,            /* context */
    DEV_DEBUG,       /* flags */
    0,               /* debug control flags */
    dia_DT,          /* debug flag names */
    NULL,            /* memory size change */
    NULL,            /* logical name */
    NULL,            // attach help
    NULL,            // help
    NULL,            // help context
    NULL,            // device description
    NULL
};

t_dia_data dia_data;

struct dn355_submailbox
  {
    word36 word1; // dn355_no; is_hsla; la_no; slot_no
    word36 word2; // cmd_data_len; op_code; io_cmd
    word36 command_data [3];
    word36 word6; // data_addr, word_cnt;
    word36 pad3 [2];
  };

struct fnp_submailbox // 28 words
  {
                                                                 // AN85
    word36 word1; // dn355_no; is_hsla; la_no; slot_no    // 0      word0
    word36 word2; // cmd_data_len; op_code; io_cmd        // 1      word1
    word36 mystery [26];                                         // word2...
  };

struct mailbox
  {
    word36 dia_pcw;
    word36 mailbox_requests;
    word36 term_inpt_mpx_wd;
    word36 last_mbx_req_count;
    word36 num_in_use;
    word36 mbx_used_flags;
    word36 crash_data [2];
    struct dn355_submailbox dn355_sub_mbxes [8];
    struct fnp_submailbox fnp_sub_mbxes [4];
  };

#define MAILBOX_WORDS             (sizeof (struct mailbox) / sizeof (word36))
#define DIA_PCW                 (offsetof (struct mailbox, dia_pcw) / sizeof (word36))
#define TERM_INPT_MPX_WD        (offsetof (struct mailbox, term_inpt_mpx_wd) / sizeof (word36))
#define CRASH_DATA              (offsetof (struct mailbox, crash_data) / sizeof (word36))
#define DN355_SUB_MBXES         (offsetof (struct mailbox, dn355_sub_mbxes) / sizeof (word36))
#define FNP_SUB_MBXES           (offsetof (struct mailbox, fnp_sub_mbxes) / sizeof (word36))


//
// Once-only initialization
//

void dia_init (void) {
//sim_printf ("DEBUG: sizeof (dnPkt) %lu\r\n", sizeof (dnPkt));
  memset(& dia_data, 0, sizeof (dia_data));
  //for (uint diaUnitIdx = 0; diaUnitIdx < N_DIA_UNITS_MAX; diaUnitIdx ++) {
    //struct dia_unit_data * dudp = & dia_data.dia_unit_data[diaUnitIdx];
    //uv_udp_init (uv_default_loop (), & dudp->udpSendHandle);     
  //}
}

#if 0
static inline void fnp_core_write (word24 addr, word36 data, UNUSED const char * ctx)
  {
#ifdef THREADZ
    lock_mem_wr ();
#endif
#ifdef SCUMEM
    iom_core_write (addr, data, ctx);
#else
    M [addr] = data & DMASK;
#endif
#ifdef THREADZ
    unlock_mem ();
#endif
  }
#endif

#if 0
//
// Convert virtual address to physical
//

static uint virtToPhys (uint ptPtr, uint l66Address)
  {
    uint pageTable = ptPtr * 64u;
    uint l66AddressPage = l66Address / 1024u;

    word36 ptw;
    uint iomUnitIdx = (uint) cables->cablesFromIomToFnp [decoded.devUnitIdx].iomUnitIdx;
#ifdef SCUMEM
    iom_core_read (iomUnitIdx, pageTable + l66AddressPage, & ptw, "fnpIOMCmd get ptw");
#else
    //fnp_core_read (pageTable + l66AddressPage, & ptw, "fnpIOMCmd get ptw");
    iom_direct_data_service (iomUnitIdx, chan, pateTable + l66AddressPage, & ptw, direct_load);
#endif
    uint page = getbits36_14 (ptw, 4);
    uint addr = page * 1024u + l66Address % 1024u;
    return addr;
  }
#endif

#if 0
//
// udp packets
//
//   pkt[0] = cmd
//     cmd 1 - bootload
//

static void dnSendCB (uv_udp_send_t * req, int status) {
sim_printf ("send cb %d\r\n", status);
}

static void dnSend (dnPkt * pkt, struct dia_unit_data * dudp) {
  uv_udp_send_t * req = malloc (sizeof (uv_udp_send_t));
  if (! req) {
    sim_warn ("%s req malloc fail\r\n", __func__);
    return;
  }
  uv_buf_t * bufs = malloc (sizeof (uv_buf_t));
  if (! bufs) {
    sim_warn ("%s bufs malloc fail\r\n", __func__);
    return;
  }
  bufs[0].base = (char *) pkt;
  bufs[0].len = sizeof (dnPkt);
  int rc = uv_udp_send (req, & dudp->udpSendHandle, bufs, 1, NULL, dnSendCB);
  if (rc < 0) {
    fprintf (stderr, "%s uv_udp_send failed %d\n", __func__, rc);
  }
}

void processCmdR (uv_udp_t * req, dnPkt * pkt) {
  word24 addr;
  int n = sscanf (pkt->cmdR.addr, "%o", & addr);
  if (n != 1) {
    sim_printf ("%s unable to extract address\r\n", __func__);
  } else {
    sim_printf ("R %08o\r\n", addr);
    if (!cdp || cdp->magic != DIA_CLIENT_MAGIC) {
      sim_printf ("no magic\r\n");
      return;
    }
//sim_printf ("dia @ %u %u\r\n", cdp->iomUnitIdx, cdp->chan);
  word36 data;
  // We can't use data service because the address is externally specified,
  // not part of a DCW list.
  // Actually, we can and must. The transfer is under control
  // of a CIOC, however indirectly and the addressing data
  // in the PCW is part of the transfer
    iom_chan_data_t * p = & iom_chan_data[cdp->iomUnitIdx][cdp->chan];
sim_printf ("%s %o %o %o\n", __func__, p->PCW_63_PTP, p->PCW_64_PGE, p->PCW_PAGE_TABLE_PTR);
sim_printf ("%s tally %o addr %o\n", __func__, p->DDCW_TALLY, p->DDCW_ADDR);
  iom_direct_data_service (cdp->iomUnitIdx, cdp->chan, addr, & data, direct_load);

  //iom_indirect_data_service (cdp->iomUnitIdx, cdp->chan, & data, 1u, false);
  //p->DDCW_TALLY --;
  //p->DDCW_ADDR ++;

  //iom_core_read (p->iomUnitIdx, addr, & data, __func__);

sim_printf ("%08o:%012llo\r\n", addr, data);

  dnPkt pkt;
  memset (& pkt, 0, sizeof (pkt));
  pkt.cmd = DN_CMD_DATA;
  sprintf (pkt.cmdD.data, "%08o:%012llo", addr, data);
  dnSend (& pkt, dudp);

  }
}
#endif
static void cmdXferToL6 (uint iomUnitIdx, uint diaUnitIdx, uint chan, word24 l66Addr) {
  //iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan];
  struct dia_unit_data * dudp = & dia_data.dia_unit_data[diaUnitIdx];
  //struct mailbox vol * mbxp = (struct mailbox vol *) & M[dudp->mailboxAddress];

  dudp->l66Addr = l66Addr;

  dnPkt pkt;
  memset (& pkt, 0, sizeof (pkt));
  pkt.cmd = DN_CMD_XFER_TO_L6;
  sprintf (pkt.cmdT.addr, "%08o", l66Addr);
  dnSend (diaUnitIdx, & pkt);
}

static void cmd_bootload (uint iomUnitIdx, uint diaUnitIdx, uint chan, word24 l66Addr) {
  //iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan];
  struct dia_unit_data * dudp = & dia_data.dia_unit_data[diaUnitIdx];
  //struct mailbox vol * mbxp = (struct mailbox vol *) & M[dudp->mailboxAddress];

  dudp->l66Addr = l66Addr;

  dnPkt pkt;
  memset (& pkt, 0, sizeof (pkt));
  pkt.cmd = DN_CMD_BOOTLOAD;
  dnSend (diaUnitIdx, & pkt);
}

static int interruptL66 (uint iomUnitIdx, uint chan) {
  //iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan];
  uint diaUnitIdx = get_ctlr_idx (iomUnitIdx, chan);
  struct dia_unit_data * dudp = & dia_data.dia_unit_data[diaUnitIdx];
  DDBG (sim_printf ("interruptL66 iomUnitIdx %o chan %o diaUnitIdx %o mbox %08o\r\n", iomUnitIdx, chan, diaUnitIdx, dudp->mailboxAddress));
#ifdef SCUMEM
  word24 offset;
  int scu_unit_num =  query_IOM_SCU_bank_map (iomUnitIdx, dudp->mailboxAddress, & offset);
  int scu_unit_idx = cables->cablesFromScus[iomUnitIdx][scu_unit_num].scu_unit_idx;
  struct mailbox vol * mbxp = (struct mailbox *) & scu[scu_unit_idx].M[dudp->mailboxAddress];
#else
  struct mailbox vol * mbxp = (struct mailbox vol *) & M[dudp->mailboxAddress];
#endif
  word36 dia_pcw = mbxp -> dia_pcw;
  DDBG (sim_printf ("dia_pcw %012llo\r\n", dia_pcw));
// AN85, pg 13-5
// When the CS has control information or output data to send
// to the FNP, it fills in a submailbox as described in Section 4
// and sends an interrupt over the DIA. This interrupt is handled
// by dail as described above; when the submailbox is read, the
// transaction control word is set to "submailbox read" so that when
// the I/O completes and dtrans runs, the mailbox decoder (decmbx)
// is called. the I/O command in the submail box is either WCD (for
// control information) or WTX (for output data). If it is WCD,
// decmbx dispatches according to a table of operation codes and
// setting a flag in the IB and calling itest, the "test-state"
// entry of the interpreter. n a few cases, the operation requires
// further DIA I/O, but usually all that remains to be does is to
// "free" the submailbox by turning on the corresponding bit in the
// mailbox terminate interrupt multiplex word (see Section 4) and
// set the transaction control word accordingly. When the I/O to
// update TIMW terminates, the transaction is complete.
//
// If the I/O command is WTX, the submailbox contains the
// address and length of a 'pseudo-DCW" list containing the
// addresses and tallies of data buffers in tty_buf. In this case,
// dia_man connects to a DCW list to read them into a reserved area
// in dia_man. ...


// interrupt level (in "cell"):
//
// mbxs 0-7 are CS -> FNP
// mbxs 8--11 are FNP -> CS
//
//   0-7 Multics has placed a message for the FNP in mbx 0-7.
//   8-11 Multics has updated mbx 8-11
//   12-15 Multics is done with mbx 8-11  (n - 4).

    word6 cell = getbits36_6 (dia_pcw, 24);
    sim_debug (DBG_TRACE, & dia_dev, "CS interrupt %u\n", cell);
    DDBG (sim_printf ("CS interrupt %u\n", cell));
    if (cell < 8)
      {
        //interruptL66_CS_to_FNP ();
      }
    else if (cell >= 8 && cell <= 11)
      {
        //interruptL66_FNP_to_CS ();
      }
    else if (cell >= 12 && cell <= 15)
      {
        //interruptL66_CS_done ();
      }
    else
      {
        sim_debug (DBG_ERR, & dia_dev, "fnp illegal cell number %d\n", cell);
        sim_printf ("fnp illegal cell number %d\n", cell);
        // doFNPfault (...) // XXX
        return -1;
      }
    return 0;
  }

#ifdef dumpseg 
static word24 xbuild_DDSPTW_address (word18 PCW_PAGE_TABLE_PTR, word8 pageNumber)
  {
//    0      5 6        15  16  17  18                       23
//   ----------------------------------------------------------
//   | Page Table Pointer | 0 | 0 |                           |
//   ----------------------------------------------------------
// or
//                        -------------------------------------
//                        |  Direct chan addr 6-13            |
//                        -------------------------------------
    if (PCW_PAGE_TABLE_PTR & 3)
      sim_warn ("%s: pcw_ptp %#o page_no  %#o\n", __func__, PCW_PAGE_TABLE_PTR, pageNumber);
    word24 addr = (((word24) PCW_PAGE_TABLE_PTR) & MASK18) << 6;
    addr |= pageNumber;
    return addr;
  }

static void xfetch_DDSPTW (uint iom_unit_idx, int chan, word18 addr)
  {
    iom_chan_data_t * p = & iom_chan_data[iom_unit_idx][chan];
    word24 pgte = xbuild_DDSPTW_address (p -> PCW_PAGE_TABLE_PTR,
                                      (addr >> 10) & MASK8);
    //iom_core_read (iom_unit_idx, pgte, (word36 *) & p -> PTW_DCW, __func__);
    core_read (pgte, (word36 *) & p -> PTW_DCW, __func__);
    //if ((p -> PTW_DCW & 0740000777747) != 04llu)
      //sim_warn ("%s: chan %d addr %#o pgte %08o ptw %012"PRIo64"\n",
                //__func__, chan, addr, pgte, p -> PTW_DCW);
  }

void xiom_direct_data_service (uint iom_unit_idx, uint chan, word24 daddr, word36 * data,
                           iom_direct_data_service_op op)
  {
    iom_chan_data_t * p = & iom_chan_data[iom_unit_idx][chan];

    if (p -> masked)
      return;

    if (p -> PCW_63_PTP && p -> PCW_64_PGE)
      {
        xfetch_DDSPTW (iom_unit_idx, (int) chan, daddr);
        daddr = ((uint) getbits36_14 (p -> PTW_DCW, 4) << 10) | (daddr & MASK10);
      }

    if (op == direct_store)
      core_write (daddr, * data, __func__);
    else if (op == direct_load)
      core_read (daddr, data, __func__);
    else if (op == direct_read_clear)
      {
        iom_core_read_lock (iom_unit_idx, daddr, data, __func__);
        iom_core_write_unlock (iom_unit_idx, daddr, 0, __func__);
      }
  }
#endif

#if 0
static iom_cmd_rc_t processMBX (uint iomUnitIdx, uint chan) {

  uint diaUnitIdx = get_ctlr_idx (iomUnitIdx, chan);

  iom_cmd_rc_t ret_cmd = IOM_CMD_DISCONNECT;
  //iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan];
  struct dia_unit_data * dudp = & dia_data.dia_unit_data[diaUnitIdx];

// 60132445 FEP Coupler EPS
// 2.2.1 Control Intercommunication
//
// "In Level 66 momory, at a location known to the coupler and
// to Level 6 software is a mailbox area consisting to an Overhead
// mailbox and 7 Channel mailboxes."

  //bool ok = true;
  //struct mailbox vol * mbxp = (struct mailbox vol *) & M [dudp -> mailboxAddress];

  //if (! dudp->connected) {
    //if (! connectFNP (diaUnitIdx)) {
      //ok = false;
      //goto done;
    //}
  //}

  word36 dia_pcw;
  //dia_pcw = mbxp -> dia_pcw;
  iom_direct_data_service (iomUnitIdx, chan, dudp->mailboxAddress+DIA_PCW, & dia_pcw, direct_load);

  DDBG (sim_printf ("mbx %08o:%012"PRIo64"\n", dudp -> mailboxAddress, dia_pcw));

// Mailbox word 0:
//
//   0-17 A
//     18 I
//  19-20 MBZ
//  21-22 RFU
//     23 0
//  24-26 B
//  27-29 D Channel #
//  30-35 C Command
//
//                          A6-A23    A0-A2     A3-A5
// Operation          C         A        B        D
// Interrupt L6      071       ---      Int.     Level
// Bootload L6       072    L66 Addr  L66 Addr  L66 Addr
//                           A6-A23    A0-A2     A3-A5
// Interrupt L66     073      ---      ---     Intr Cell
// Data Xfer to L66  075    L66 Addr  L66 Addr  L66 Addr
//                           A6-A23    A0-A2     A3-A5
// Data Xfer to L6   076    L66 Addr  L66 Addr  L66 Addr
//                           A6-A23    A0-A2     A3-A5

//
// fnp_util.pl1:
//    075 tandd read
//    076 tandd write

// mbx word 1: mailbox_requests fixed bin
//          2: term_inpt_mpx_wd bit (36) aligned
//          3: last_mbx_req_count fixed bin
//          4: num_in_use fixed bin
//          5: mbx_used_flags
//                used (0:7) bit (1) unaligned
//                pad2 bit (28) unaligned
//          6,7: crash_data
//                fault_code fixed bin (18) unal unsigned
//                ic fixed bin (18) unal unsigned
//                iom_fault_status fixed bin (18) unal unsigned
//                fault_word fixed bin (18) unal unsigned
//
//    crash_data according to dn355_boot_interrupt.pl1:
//
//   dcl  1 fnp_boot_status aligned based (stat_ptr),            /* structure of bootload status */
//          2 real_status bit (1) unaligned,                     /* must be "1"b in valid status */
//          2 pad1 bit (2) unaligned,
//          2 major_status bit (3) unaligned,
//          2 pad2 bit (3) unaligned,
//          2 substatus fixed bin (8) unal,                      /* code set by 355, only interesting if major_status is 4 */
//          2 channel_no fixed bin (17) unaligned;               /* channel no. of LSLA in case of config error */
//    only 34 bits???
// major_status:
//  dcl  BOOTLOAD_OK fixed bin int static options (constant) init (0);
//  dcl  CHECKSUM_ERROR fixed bin int static options (constant) init (1);
//  dcl  READ_ERROR fixed bin int static options (constant) init (2);
//  dcl  GICB_ERROR fixed bin int static options (constant) init (3);
//  dcl  INIT_ERROR fixed bin int static options (constant) init (4);
//  dcl  UNWIRE_STATUS fixed bin int static options (constant) init (5);
//  dcl  MAX_STATUS fixed bin int static options (constant) init (5);


// 3.5.1 Commands Issued by Central System
//
// In the issuing of an order by the Central System to the Coupler, the
// sequence occurs:
//
// 1. The L66 program creates a LPW and Pcw for the Central System Connect
// channel. It also generates and stores a control word containing a command
// int he L66 maillbox. A Connect is then issued to the L66 IOM.
//
// 2. The Connect Channel accesses the PCW to get the channel number of
// the Direct Channel that the coupler is attached to. the direct Channel
// sends a signelto the Coupler that a Connect has been issued.
//
// 3. The Coupler now reads the content of the L66 mailbox, obtaining the
// control word. If the control word is legel, the Coupler will write a
// word of all zeros into the mailbox.
//

// 4.1.1.2 Transfer Control Word.
// The transfer control word, which is pointed to by the
// mailbox word in l66 memory on Op Codes 72, 7, 76 contains
// a starting address which applies to L6 memory an a Tally
// of the number of 36 bit words to be transfered. The l66
// memory locations to/from which the transfers occur are
// those immediately follwoing the location where this word
// was obtained.
//
//    00-02  001
//    03-17 L6 Address
//       18 P
//    19-23 MBZ
//    24-25 Tally
//
//     if P = 0 the l6 address:
//        00-07 00000000
//        08-22 L6 address (bits 3-17)
//           23 0
//     if P = 1
//        00-14 L6 address (bits 3-17)
//        15-23 0
//

    //uint chanNum = getbits36_6 (dia_pcw, 24);
  uint command = getbits36_6 (dia_pcw, 30);
  //word36 bootloadStatus = 0;

  if (command == 000) { // reset
    sim_debug (DBG_TRACE, & dia_dev, "%s: chan %d reset command\n", __func__, chan);
    send_general_interrupt (iomUnitIdx, chan, imwTerminatePic);
  } else if (command == 072) { // bootload


    // 60132445 pg 49
    // Extract L66 address from dia_pcw

// According to 60132445:
    //word24 A = (word24) getbits36_18 (dia_pcw,  0);
    //word24 B = (word24) getbits36_3  (dia_pcw, 24);
    //word24 D = (word24) getbits36_3  (dia_pcw, 29);
    //word24 l66Addr = (B << (24 - 3)) | (D << (24 - 3 - 3)) | A;
// According to fnp_util.pl1:
//     dcl  1 a_dia_pcw aligned based (mbxp),    /* better declaration than the one used when MCS is running */
//            2 address fixed bin (18) unsigned unaligned,
    word24 l66Addr = (word24) getbits36_18 (dia_pcw,  0);
    DDBG (sim_printf ("l66Addr %08o\r\n", l66Addr));

    //word36 boot_icw;
    //iom_direct_data_service (iomUnitIdx, chan, l66Addr, & boot_icw, direct_load);
    //word12 tally = getbits36_12 (boot_icw, 24);
    //DDBG (sim_printf ("boot_icw %012llo\n", boot_icw));
    //DDBG (sim_printf ("tally %o %d.\n", tally, tally));

    //// Calculate start of core image
    //word24 image_off = (tally + 64) & 077777700;
    //DDBG (sim_printf ("image_off %o %d.\n", image_off, image_off));

#ifdef dumpseg
#include <unistd.h>
    { int fd = open ("boot_segment.dat", O_RDWR | O_CREAT | O_TRUNC, 0660);
      if (fd < 0) {
        perror ("open boot_segment.dat");
    exit (1);
      } else {
        for (uint i = 0; i < 00060000; i ++) {
          word36 word0;
          xiom_direct_data_service (iomUnitIdx, chan, i/* + 000010000 */, & word0, direct_load);
          ssize_t n = write (fd, & word0, sizeof (word0));
          if (n != sizeof (word0)) {
            sim_printf ("i %u n %ld\r\n", i, n);
            perror ("write boot_segment.dat");
    exit (1);
          }
        }
      }
      close (fd);
      exit (1);
    }
#endif

#if 0
    //DDBG (
    for (uint i = 0; i < 4096; i ++) {
      if (i % 4 == 0) sim_printf ("%06o", i);
      word36 word0;
      iom_direct_data_service (iomUnitIdx, chan, l66Addr + i, & word0, direct_load);
      sim_printf (" %012llo", word0);
      if (i % 4 == 3) sim_printf ("\r\n");
      };
    //)
#endif

    // AN85: gicb routine:
    // "The connect to the bootload PCW causes the DIA to read the 
    // gicb rountine inth the FNP under control of the boot ICW which is 
    // the first word of the boot segment."

    // The ICW contains 100000 000517. 517 is the length of the gicb code.

    // pg 50 4.1.1.2 Transfer control word
    // "The transfer control word, which is pointed to by the
    // mailbox word in L66 memory on Op Codes 72, 75, 76 contains
    // a starting address which applies to L6 memory and a Tally
    // of the number of 36 bit words to be transferred. The L66
    // memory locations to/from which the transfers occur are
    // those immediately following the location where this word
    // was obtained.
    //
    // 0-2: 001
    // 3-17: L6 Address
    // 18: P
    // 19-23: MBZ
    // 24-36: Tally
    //
    // The L6 Address field is interpreted as an effective L6 byte
    // address as follows:
    //
    // If P = 0
    //
    // 0-7: 00000000
    // 8-22: L6 Address field (bits 3-17)
    // 23: 0
    //
    // If P = 1
    //
    // 0-14: L6 Address field (bits 3-17)
    // 15-22: 0000000
    // 23: 0

    cmd_bootload (iomUnitIdx, diaUnitIdx, chan, l66Addr);

// My understanding is that the FNP shouldn't clear the MBX PCW until
// the bootload is complete; but the timeout in fnp_util$connect_to_dia_paged
// is short:
//   do i = 1 to 100000 while (unspec (a_dia_pcw) = old_pcw);
// I am going ahead and acking, but it is unclear to me what the mechanism,
// if any, is for FNP signaling completed successful bootload.

    // Don't acknowledge the boot yet.
    //dudp -> fnpIsRunning = true;
    return IOM_CMD_PENDING;
    //ret_cmd = IOM_CMD_PENDING;
  } else if (command == 071) { // interrupt L6
    //ok = interruptL66 (iomUnitIdx, chan) == 0;
    interruptL66 (iomUnitIdx, chan);
    return IOM_CMD_PENDING;
  } else if (command == 076) { // data xfer from L66 to L6
    word24 l66Addr = (word24) getbits36_18 (dia_pcw,  0);
    DDBG (sim_printf ("l66Addr %08o\r\n", l66Addr));
    cmdXferToL6 (iomUnitIdx, diaUnitIdx, chan, l66Addr);
    return IOM_CMD_PENDING;
    //return IOM_CMD_DISCONNECT;
  } else if (command == 075) { // data xfer from L6 to L66
    // Build the L66 address from the PCW
    //   0-17 A
    //  24-26 B
    //  27-29 D Channel #
    // Operation          C         A        B        D
    // Data Xfer to L66  075    L66 Addr  L66 Addr  L66 Addr
    //                           A6-A23    A0-A2     A3-A5
    // These don't seem to be right; M[L66Add] is always 0.
    //word24 A = (word24) getbits36_18 (dia_pcw,  0);
    //word24 B = (word24) getbits36_3  (dia_pcw, 24);
    //word24 D = (word24) getbits36_3  (dia_pcw, 29);
    //word24 L66Addr = (B << (24 - 3)) | (D << (24 - 3 - 3)) | A;


    // According to fnp_util:
    //  dcl  1 a_dia_pcw aligned based (mbxp),                      /* better declaration than the one used when MCS is running */
    //         2 address fixed bin (18) unsigned unaligned,
    //         2 error bit (1) unaligned,
    //         2 pad1 bit (3) unaligned,
    //         2 parity bit (1) unaligned,
    //         2 pad2 bit (1) unaligned,
    //         2 pad3 bit (3) unaligned,                            /* if we used address extension this would be important */
    //         2 interrupt_level fixed bin (3) unsigned unaligned,
    //         2 command bit (6) unaligned;
    //
    //   a_dia_pcw.address = address;
    //


    //word24 L66Addr = (word24) getbits36_18 (dia_pcw, 0);
    //sim_printf ("L66 xfer\n");
    //sim_printf ("PCW  %012"PRIo64"\n", dia_pcw);
    //sim_printf ("L66Addr %08o\n", L66Addr);
    //sim_printf ("M[] %012"PRIo64"\n", M[L66Addr]);

    // 'dump_mpx d'
    //L66 xfer
    //PCW  022002000075
    //L66Addr 00022002
    //M[] 000000401775
    //L66 xfer
    //PCW  022002000075
    //L66Addr 00022002
    //M[] 003772401775
    //L66 xfer
    //PCW  022002000075
    //L66Addr 00022002
    //M[] 007764401775
    //
    // The contents of M seem much more reasonable, bit still don't match
    // fnp_util$setup_dump_ctl_word. The left octet should be '1', not '0';
    // bit 18 should be 0 not 1. But the offsets and tallies match exactly.
    // Huh... Looking at 'dump_6670_control' control instead, it matches
    // correctly. Apparently fnp_util thinks the FNP is a 6670, not a 335.
    // I can't decipher the call path, so I don't know why; but looking at
    // multiplexer_types.incl.pl1, I would guess that by MR12.x, all FNPs
    // were 6670s.
    //
    // So:
    //
    //   dcl  1 dump_6670_control aligned based (data_ptr),
    //         /* word used to supply DIA address and tally for fdump */
    //          2 fnp_address fixed bin (18) unsigned unaligned,
    //          2 unpaged bit (1) unaligned,
    //          2 mbz bit (5) unaligned,
    //          2 tally fixed bin (12) unsigned unaligned;

    // Since the data is marked 'paged', and I don't understand the
    // the paging mechanism or parameters, I'm going to punt here and
    // not actually transfer any data.
    sim_debug (DBG_TRACE, & dia_dev, "FNP data xfer??\n");
  } else {
    sim_warn ("bogus fnp command %d (%o)\n", command, command);
    //ok = false;
  }

//done:
#if 0
  if (ok) {
    //if_sim_debug (DBG_TRACE, & fnp_dev) dmpmbx (dudp->mailboxAddress);
    //fnp_core_write (dudp -> mailboxAddress, 0, "dia_iom_cmd clear dia_pcw");
    dia_pcw = 0;
    iom_direct_data_service (iomUnitIdx, chan, dudp->mailboxAddress+DIA_PCW, & dia_pcw, direct_store);
    putbits36_1 (& bootloadStatus, 0, 1); // real_status = 1
    putbits36_3 (& bootloadStatus, 3, 0); // major_status = BOOTLOAD_OK;
    putbits36_8 (& bootloadStatus, 9, 0); // substatus = BOOTLOAD_OK;
    putbits36_17 (& bootloadStatus, 17, 0); // channel_no = 0;
    //fnp_core_write (dudp -> mailboxAddress + 6, bootloadStatus, "dia_iom_cmd set bootload status");
    iom_direct_data_service (iomUnitIdx, chan, dudp->mailboxAddress + CRASH_DATA, & bootloadStatus, direct_store);
  } else {
    //dmpmbx (dudp->mailboxAddress);
    sim_printf ("%s not ok\r\n", __func__);
    // 60132445 says the h/w inverts the bit; by software convention, it is initially 0.
    // 3 error bit (1) unaligned, /* set to "1"b if error on connect */
    putbits36_1 (& dia_pcw, 18, 1); // set bit 18
    //fnp_core_write (dudp -> mailboxAddress, dia_pcw, "dia_iom_cmd set error bit");
    iom_direct_data_service (iomUnitIdx, chan, dudp->mailboxAddress+DIA_PCW, & dia_pcw, direct_store);
  }
#endif
  return ret_cmd;
}
#endif

static iom_cmd_rc_t dia_cmd (uint iomUnitIdx, uint chan)
  {
    uint diaUnitIdx = get_ctlr_idx (iomUnitIdx, chan);
    iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan];
    UNIT * uptr = dia_unit + diaUnitIdx;
    if ((uptr->flags & UNIT_ATT) == 0) {
      int rc = dia_connect (uptr);
      if (rc) {
        sim_printf ("Can't connect to FNP\r\n");
        return IOM_CMD_ERROR;
      }
    }
    p -> stati = 0;
    DDBG (sim_printf ("fnp cmd %d\n", p -> IDCW_DEV_CMD));
    switch (p -> IDCW_DEV_CMD)
      {
        case 000: // CMD 00 Request status
          {
            p -> stati = 04000;
            sim_debug (DBG_NOTIFY, & dia_dev, "Request status\n");
            DDBG (sim_printf ("Request status\r\n"));
            // Send a connect message to the coupler
            dnPkt pkt;
            memset (& pkt, 0, sizeof (pkt));
            pkt.cmd = DN_CMD_CONNECT;
            dnSend (diaUnitIdx, & pkt);
          }
          break;

        default:
          {
            p -> stati = 04501;
            p -> chanStatus = chanStatIncorrectDCW;
            if (p->IDCW_DEV_CMD != 051) // ignore bootload console probe
              sim_warn ("dia daze %o\n", p->IDCW_DEV_CMD);
          }
          return IOM_CMD_ERROR;
      }

    // iom_cmd_rc_t cmd = processMBX (iomUnitIdx, chan);

    return IOM_CMD_DISCONNECT; // did command, don't want more
    //return IOM_CMD_PENDING; // did command, don't want more
    // return cmd; // did command, don't want more
  }

/*
 * dia_iom_cmd()
 *
 */

// 1 ignored command
// 0 ok
// -1 problem

iom_cmd_rc_t dia_iom_cmd (uint iomUnitIdx, uint chan)
  {
    DDBG (sim_printf ("dia_iom_cmd %u %u\r\n", iomUnitIdx, chan));
    iom_chan_data_t * p = & iom_chan_data[iomUnitIdx][chan];
// Is it an IDCW?

    if (IS_IDCW (p))
      {
        return dia_cmd (iomUnitIdx, chan);
      }
    // else // DDCW/TDCW
    sim_printf ("%s expected IDCW %012llo\n", __func__, p->DCW);
    return -1;
  }

#if 0
static void load_stored_boot (void)
  {
    // pg 45:

    //  3.8 BOOTLOAD OF L6 BY L66
    //...
    // The receipt of the Input Stored Boot order causes the coupler,
    // if the Stored Boot but is ONE, to input data into L6 memory as
    // specified by the L66 Bootload order. On completion of this,
    // the Stored Boot bit is cleared.
    //
    // ... the PROM program issues the Input Stored Boot IOLD order
    // to the coupler..
    //
    // ... the L66 Bootload command specifies the L6 memory locations into
    // which the load from L66 is to occur and the extent of the lod;
    // location (0100)16 in L6 would always be the first location to be
    // executed by L6 after the load from L66 assuming that the L66
    // bootload is independent of the mechanization used in L66


    sim_printf ("got load_stored_boot\n");
  }
#endif

#if 0
#define psz 17000
static uint8_t pkt[psz];

// warning: returns ptr to static buffer
static int poll_coupler (uint diaUnitIdx, uint8_t * * pktp)
  {
#ifdef PUNT
  struct dia_unit_data * dudp = & dia_data.dia_unit_data[diaUnitIdx];
    int sz = dn_udp_receive (dudp->udpSendHandle, pkt, psz);
    if (sz < 0)
      {
        sim_printf ("dn_udp_receive failed: %d\n", sz);
        sz = 0;
      }
    * pktp = pkt;
    return sz;
#else
    return 0;
#endif
  }
#endif

void dia_unit_process_events (uint unit_num)
  {
    //DDBG (sim_printf ("punt 5\r\n"));
#if 0 // cac
// XXX rememeber
// XXX        //dudp -> fnpIsRunning = true;
// XXX when bootload complete!

    uint8_t * pktp;
    int sz = poll_coupler (unit_num, & pktp);
//sim_printf ("poll_coupler return %d\n", sz);
    if (! sz)
      {
        return;
      }

   uint8_t cmd = pktp [0];
   switch (cmd)
     {
       // IO Load Stored Boot
       case dn_cmd_ISB_IOLD:
         {
           load_stored_boot ();
           break;
         }

       default:
         {
           sim_printf ("%s got unhandled cmd %u\n", __func__, cmd);
           break;
         }
     }
#endif
  }

// Called @ 100Hz to process FNP background events

void diaProcessEvents (void) {
  //uv_run (dia_data.loop, UV_RUN_NOWAIT);
  for (uint unit_num = 0; unit_num < N_DIA_UNITS_MAX; unit_num ++) {
   //if (dia_data.dia_unit_data[unit_num].connected)
     dia_unit_process_events (unit_num);
  }
}
