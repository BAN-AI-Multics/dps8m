/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 35c9cf5c-ebbf-11ed-8d34-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2015-2018 Charles Anthony
 * Copyright (c) 2023 Björn Victor
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

// Defining USE_SOCKET_DEV_APPROACH to 1 means taking "the socket_dev
// approach", meaning the Read operation doesn't really do much, but
// instead the event/poll loop (see mgp_process_event) does the actual
// reading. This doesn't seem to work very well, basically nothing
// gets read.

// Defining USE_SOCKET_DEV_APPROACH to 0 means the Read operation does
// the reading, returning IOM_CMD_DISCONNECT to signal that there is
// something in the buffer. Additionally the event/poll loop
// (mgp_process_event) checks if there is anything to read and in that
// case explicitly does send_terminate_interrupt(). This works a
// little bit better, as some things are actually read, but not
// sufficiently well.

#define USE_SOCKET_DEV_APPROACH 0

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include <sys/types.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/time.h>

#include "dps8.h"
#include "dps8_iom.h"
#include "dps8_mgp.h"
#include "dps8_sys.h"
#include "dps8_cable.h"
#include "dps8_cpu.h"
#include "dps8_faults.h"
#include "dps8_scu.h"
#include "dps8_utils.h"

#if defined(FREE)
# undef FREE
#endif /* if defined(FREE) */
#define FREE(p) do  \
  {                 \
    free((p));      \
    (p) = NULL;     \
  } while(0)

#if defined(WITH_MGP_DEV)

# define DBG_CTR  1

static void mgp_init_dev_state(void);
static void dumppkt(char *hdr, word36 *buf, uint words);
static int  handle_mgp_packet(word36 *buf, uint words);
static int  poll_from_cbridge(word36 *buf, uint words, uint probe_only);
static void mgp_wants_to_read(uint iom_unit_idx, uint chan);

# define MAX_CONNS  64

struct conn_table
{
  int skt;                    /* socket to cbridge                     */
  u_short remote_addr;        /* remote address (from connect/cbridge) */
  char *contact_name;         /* contact name (likewise)               */
  u_short local_id;           /* local id                              */
  u_short multics_proc;
  u_char pkt_last_received;   /* from Multics */
  u_char pkt_last_sent;       /* to Multics   */
};

struct mgp_dev_state
{
  u_char first_frame_received;
  u_char frame_last_received; /* from Multics                             */
  u_char frame_last_sent;     /* to Multics                               */
  u_char send_noop;           /* set to make sure NOOP is sent to Multics */
  short read_index;           /* conn we last read from, for round-robin  */
  u_char want_to_read;        /* flag that Multics wants to read          */
  uint read_unit_idx;
  uint read_unit_chan;
  struct conn_table conns[MAX_CONNS];
} mgp_dev_state;

static struct mgp_state
  {
    char device_name[MAX_DEV_NAME_LEN];
  } mgp_state[N_MGP_UNITS_MAX];

# define N_MGP_UNITS  2 // default

# define UNIT_FLAGS \
        ( UNIT_FIX | UNIT_ATTABLE | UNIT_ROABLE | UNIT_DISABLE | UNIT_IDLE )

UNIT mgp_unit[N_MGP_UNITS_MAX] = {
  {
    UDATA(NULL, UNIT_FLAGS, 0),
    0,   0,   0,   0,   0,
    NULL,  NULL,  NULL,  NULL
  }
};

static inline uint32_t
hash32s(const void *buf, size_t len, uint32_t h)
{
  const unsigned char *p = buf;

  for (size_t i = 0; i < len; i++)
    h = h * 31 + p[i];

  h ^= h >> 17;
  h *= UINT32_C(0xed5ad4bb);
  h ^= h >> 11;
  h *= UINT32_C(0xac4c1b51);
  h ^= h >> 15;
  h *= UINT32_C(0x31848bab);
  h ^= h >> 14;

  return h;
}

# define MGP_UNIT_IDX(uptr)  (( uptr ) - mgp_unit )

static DEBTAB mgp_dt[] = {
     { "NOTIFY", DBG_NOTIFY, NULL },
     { "INFO",   DBG_INFO,   NULL },
     { "ERR",    DBG_ERR,    NULL },
     { "WARN",   DBG_WARN,   NULL },
     { "DEBUG",  DBG_DEBUG,  NULL },
     { "ALL",    DBG_ALL,    NULL }, // Don't move as it messes up DBG message
     { NULL,     0,          NULL }
};

static t_stat
mgp_show_nunits(UNUSED FILE *st, UNUSED UNIT *uptr, UNUSED int val,
                UNUSED const void *desc)
{
  sim_printf("Number of MGP units in system is %d\n", mgp_dev.numunits);

  return SCPE_OK;
}

static t_stat
mgp_set_nunits(UNUSED UNIT *uptr, UNUSED int32 value, const char *cptr,
               UNUSED void *desc)
{
  if (!cptr)
    {
      return SCPE_ARG;
    }

  int n = atoi(cptr);
  if (n < 1 || n > N_MGP_UNITS_MAX)
    {
      return SCPE_ARG;
    }

  mgp_dev.numunits = (uint32)n;

  return SCPE_OK;
}

static t_stat
mgp_show_device_name(UNUSED FILE *st, UNIT *uptr, UNUSED int val,
                     UNUSED const void *desc)
{
  int n = (int)MGP_UNIT_IDX(uptr);

  if (n < 0 || n >= N_MGP_UNITS_MAX)
    {
      return SCPE_ARG;
    }

  if (mgp_state[n].device_name[1] != 0)
    {
      sim_printf("name     : %s", mgp_state[n].device_name);
    }
  else
    {
      sim_printf("name     : MGP%d", n);
    }

  return SCPE_OK;
}

static t_stat
mgp_set_device_name(UNIT *uptr, UNUSED int32 value, const char *cptr,
                    UNUSED void *desc)
{
  int n = (int)MGP_UNIT_IDX(uptr);

  if (n < 0 || n >= N_MGP_UNITS_MAX)
    {
      return SCPE_ARG;
    }

  if (cptr)
    {
      strncpy(mgp_state[n].device_name, cptr, MAX_DEV_NAME_LEN - 1);
      mgp_state[n].device_name[MAX_DEV_NAME_LEN - 1] = 0;
    }
  else
    {
      mgp_state[n].device_name[0] = 0;
    }

  return SCPE_OK;
}

# define UNIT_WATCH  UNIT_V_UF

static MTAB mgp_mod[] = {
# if !defined(SPEED)
  { UNIT_WATCH, 1, "WATCH",   "WATCH",   0, 0, NULL, NULL },
  { UNIT_WATCH, 0, "NOWATCH", "NOWATCH", 0, 0, NULL, NULL },
# endif /* if !defined(SPEED) */
  {
    MTAB_XTD | MTAB_VDV | MTAB_NMO | MTAB_VALR, /* Mask               */
    0,                                          /* Match              */
    "NUNITS",                                   /* Print string       */
    "NUNITS",                                   /* Match string       */
    mgp_set_nunits,                             /* Validation routine */
    mgp_show_nunits,                            /* Display routine    */
    "Number of MGP units in the system",        /* Value descriptor   */
    NULL                                        /* Help               */
  },
  {
    MTAB_XTD | MTAB_VUN | MTAB_VALR | MTAB_NC,  /* Mask               */
    0,                                          /* Match              */
    "NAME",                                     /* Print string       */
    "NAME",                                     /* Match string       */
    mgp_set_device_name,                        /* Validation routine */
    mgp_show_device_name,                       /* Display routine    */
    "Set the device name",                      /* Value descriptor   */
    NULL                                        /* Help               */
  },
  MTAB_eol
};

static t_stat
mgp_reset(UNUSED DEVICE *dptr)
{
  // mgpResetRX (0);
  // mgpResetTX (0);

  return SCPE_OK;
}

static t_stat
mgpAttach(UNIT *uptr, const char *cptr)
{
  if (!cptr)
    {
      return SCPE_ARG;
    }

  // If we're already attached, then detach ...
  if (( uptr->flags & UNIT_ATT ) != 0)
    {
      detach_unit(uptr);
    }

  uptr->flags |= UNIT_ATT;

  return SCPE_OK;
}

// Detach (connect) ...
static t_stat
mgpDetach(UNIT *uptr)
{
  if (( uptr->flags & UNIT_ATT ) == 0)
    {
      return SCPE_OK;
    }

  uptr->flags &= ~(unsigned int)UNIT_ATT;

  return SCPE_OK;
}

DEVICE mgp_dev = {
  "MGP",       /* Name                */
  mgp_unit,    /* Units               */
  NULL,        /* Registers           */
  mgp_mod,     /* Modifiers           */
  N_MGP_UNITS, /* #units              */
  10,          /* Address radix       */
  24,          /* Address width       */
  1,           /* Address increment   */
  8,           /* Data radix          */
  36,          /* Data width          */
  NULL,        /* Examine             */
  NULL,        /* Deposit             */
  mgp_reset,   /* Reset               */
  NULL,        /* Boot                */
  mgpAttach,   /* Attach              */
  mgpDetach,   /* Detach              */
  NULL,        /* Context             */
  DEV_DEBUG,   /* Flags               */
  0,           /* Debug control flags */
  mgp_dt,      /* Debug flag names    */
  NULL,        /* Memory size change  */
  NULL,        /* Logical name        */
  NULL,        /* Help                */
  NULL,        /* Attach help         */
  NULL,        /* Attach context      */
  NULL,        /* Description         */
  NULL         /* End                 */
};

/*
 * mgp_init()
 */

// Once-only initialization

void
mgp_init(void)
{
  (void)memset(mgp_state, 0, sizeof ( mgp_state ));
  // Init the other state too
  mgp_init_dev_state();
}

static iom_cmd_rc_t
get_ddcw(iom_chan_data_t *p, uint iom_unit_idx, uint chan, bool *ptro,
         uint expected_tally, uint *tally)
{
# if defined(TESTING)
  cpu_state_t * cpup = _cpup;
# endif
  bool  send, uff;
  int   rc = iom_list_service(iom_unit_idx, chan, ptro, &send, &uff);

  if (rc < 0)
    {
      p->stati = 05001; // BUG: arbitrary error code; config switch
      sim_warn("%s list service failed\n", __func__);

      return IOM_CMD_ERROR;
    }

  if (uff)
    {
      sim_warn("%s ignoring uff\n", __func__); // XXX
    }

  if (!send)
    {
      sim_warn("%s nothing to send\n", __func__);
      p->stati = 05001; // BUG: arbitrary error code; config switch

      return IOM_CMD_ERROR;
    }

  if (IS_IDCW(p) || IS_TDCW(p))
    {
      sim_warn("%s expected DDCW\n", __func__);
      p->stati = 05001; // BUG: arbitrary error code; config switch

      return IOM_CMD_ERROR;
    }

  *tally = p->DDCW_TALLY;

  if (*tally == 0)
    {
      sim_debug(DBG_DEBUG, &mgp_dev,
                "%s: Tally of zero interpreted as 010000(4096)\n", __func__);
      *tally = 4096;
    }

  sim_debug(DBG_DEBUG, &mgp_dev,
            "%s: Tally %d (%o)\n", __func__, *tally, *tally);

  if (expected_tally && *tally != expected_tally)
    {
      sim_warn("mgp_dev call expected tally of %d; got %d\n",
               expected_tally, *tally);
      p->stati = 05001; // BUG: arbitrary error code; config switch

      return IOM_CMD_ERROR;
    }

  return IOM_CMD_PROCEED;
}

static char *
cmd_name(int code)
{
  // Where can I find documentation for these?
  switch (code)
    {
    case 000:
      return "Request status";

    case 001:
      return "Read";

    case 011:
      return "Write";

    case 020:
      return "Host switch down";

    case 040:
      return "Reset status";

    case 042:
      return "Disable Bus Back";

    case 043:
      return "Enable Bus Back";

    case 060:
      return "Host switch up";

    default:
      return "Unknown";
    }
}

static iom_cmd_rc_t
mgp_cmd(uint iom_unit_idx, uint chan)
{
# if defined(TESTING)
  cpu_state_t * cpup = _cpup;
# endif
  iom_chan_data_t *p = &iom_chan_data[iom_unit_idx][chan];

  sim_debug(DBG_TRACE, &mgp_dev,
            "mgp_cmd CHAN_CMD %o DEV_CODE %o DEV_CMD %o COUNT %o\n",
            p->IDCW_CHAN_CMD, p->IDCW_DEV_CODE, p->IDCW_DEV_CMD, p->IDCW_COUNT);

  // Not IDCW?
  if (IS_NOT_IDCW(p))
    {
      sim_warn("%s: Unexpected IOTx\n", __func__);

      return IOM_CMD_ERROR;
    }

  bool ptro;

  sim_printf("mgp_cmd %#o (%s)\n",
             p->IDCW_DEV_CMD, cmd_name(p->IDCW_DEV_CMD));

  switch (p->IDCW_DEV_CMD)
    {
    case 000: // CMD 00 Request status
    {
      p->stati = 04000;
      sim_printf("mgp request status\n");
    }
    break;

    case 001: // CMD 01 Read
    {
      sim_debug(DBG_DEBUG, &mgp_dev, "%s: mgp_dev_$read\n", __func__);

      const uint    expected_tally = 0;
      uint          tally;
      iom_cmd_rc_t  rc
        = get_ddcw(p, iom_unit_idx, chan, &ptro, expected_tally, &tally);
      if (rc)
        {
          return rc;
        }

      word36  buffer[4096];  /* tally size is max 4096 bytes */
      uint    words_processed;
      iom_indirect_data_service(
        iom_unit_idx, chan, buffer, &words_processed, false);

      sim_printf("mgp_cmd: Read unit %#x chan %#x (%d)\n",
                 iom_unit_idx, chan, chan);

      /*
       * Command pending, don't send terminate interrupt
       */

      rc = IOM_CMD_PENDING;

      mgp_wants_to_read(iom_unit_idx, chan);
# if !USE_SOCKET_DEV_APPROACH
      int v;
      if (( v = poll_from_cbridge(buffer, words_processed, 0)) < 0)
        {
          // nothing to read
          sim_printf("%s: nothing to read\n", __func__);
        }
      else
        {
          // something was read
          if (v > 0)
            {
              sim_printf("%s: read something, rc IOM_CMD_DISCONNECT\n",
                         __func__);
              rc = IOM_CMD_DISCONNECT; /* so send terminate interrupt */
            }

          dumppkt("Read", buffer, words_processed);
        }
# endif /* if !USE_SOCKET_DEV_APPROACH */

      iom_indirect_data_service(
        iom_unit_idx, chan, buffer, &words_processed, true);

# if 0  // @@@@ Errors should be reported somehow
      if (v < 0)
        {
          p->stati  = 05001; // BUG: arbitrary error code; config switch
          rc        = IOM_CMD_PENDING;
        }
      else
# endif /* if !USE_SOCKET_DEV_APPROACH */
      p->stati = 04000; /* bogus status, since we're
                         * not sending terminate interrupt */
      return rc;
    }
    /*NOTREACHED*/ /* unreachable */
    break;

    case 011: // CMD 11 Write
    {
      sim_debug(DBG_DEBUG, &mgp_dev, "%s: mgp_dev_$write\n", __func__);

      const uint    expected_tally = 0;
      uint          tally;
      iom_cmd_rc_t  rc
        = get_ddcw(p, iom_unit_idx, chan, &ptro, expected_tally, &tally);

      word36  buffer[4096];  /* tally size is max 4096 bytes */
      uint    words_processed;
      iom_indirect_data_service(
        iom_unit_idx, chan, buffer, &words_processed, false);

      sim_printf("mgp_cmd: Write unit %#x chan %#x (%d)\n",
                 iom_unit_idx, chan, chan);
      dumppkt("Write", buffer, words_processed);

      int v = handle_mgp_packet(buffer, words_processed);
      sim_printf("%s: handle_mgp_packet returned %d\n", __func__, v);
# if 0
      // @@@@ Errors should be reported somehow
      if (v < 0)
        {
          p->stati  = 05001; // BUG: arbitrary error code; config switch
          rc        = IOM_CMD_ERROR;
        }
      else
        {
# endif /* if 0 */
      rc        = IOM_CMD_DISCONNECT; /* send terminate interrupt */
      p->stati  = 04000;
# if 0
        }
# endif /* if 0 */

      iom_indirect_data_service(
        iom_unit_idx, chan, buffer, &words_processed, true);

      return rc;
    }
    /*NOTREACHED*/ /* unreachable */
    break;

    case 020: // CMD 20 Host switch down
    {
      p->stati = 04000;
      sim_printf("mgp host switch down\n");
    }
    break;

    case 040: // CMD 40 Reset status
    {
      p->stati = 04000;
      // is called repeatedly causing system console to be unusable
      // sim_printf ("mgp reset status\n");
    }
    break;

    case 042: // CMD 42 Disable Bus Back
    {
      p->stati = 04000;
      sim_printf("mgp disable bus back\n");
    }
    break;

    case 043: // CMD 43 Enable Bus Back
    {
      p->stati = 04000;
      sim_printf("mgp enable bus back\n");
    }
    break;

    case 060: // CMD 60 Host switch up
    {
      p->stati = 04000;
      sim_printf("mgp host switch up\n");
    }
    break;

    default:
    {
      if (p->IDCW_DEV_CMD != 051) // ignore bootload console probe
        {
          sim_warn("%s: MGP unrecognized device command  %02o\n",
            __func__, p->IDCW_DEV_CMD);
        }

      p->stati       = 04501; // cmd reject, invalid opcode
      p->chanStatus  = chanStatIncorrectDCW;
    }
      return IOM_CMD_ERROR;
    }

  if (p->IDCW_CHAN_CMD == 0)
    {
      return IOM_CMD_DISCONNECT; // don't do DCW list
    }

  return IOM_CMD_PROCEED;
}

//  3 == command pending
//  2 == did command
//  1 == ignored command
//  0 == ok
// -1 == problem

iom_cmd_rc_t
mgp_iom_cmd(uint iom_unit_idx, uint chan)
{
  iom_chan_data_t *p = &iom_chan_data[iom_unit_idx][chan];

  // Is it an IDCW?
  if (IS_IDCW(p))
    {
      return mgp_cmd(iom_unit_idx, chan);
    }

  sim_printf("%s expected IDCW\n", __func__);

  return IOM_CMD_ERROR;
}

void
mgp_process_event(void)
{
# if USE_SOCKET_DEV_APPROACH
  if (mgp_dev_state.want_to_read)
    {
      uint iom_unit_idx = mgp_dev_state.read_unit_idx;
      uint chan         = mgp_dev_state.read_unit_chan;
      // This is normally 128 or 129, needs 4 for header + 488/4 data = 128
      word36 buffer[128];
      uint words_processed = 128;
      (void)memset(buffer, 0, sizeof(buffer));

      int v = poll_from_cbridge(buffer, words_processed, 0);
      // mgp_dev_state.want_to_read = 0;
      if (v <= 0)
        {
          // nothing to read
          // sim_printf("%s: nothing to read\n", __func__);
        }
      else if (v > 0)
        {
          sim_printf("%s: read something (%d) for unit %d chan %d\n", __func__,
                     v, iom_unit_idx, chan);
          dumppkt("Read", buffer, words_processed);
          iom_indirect_data_service(
              iom_unit_idx, chan, buffer, &words_processed, true);
          send_terminate_interrupt(iom_unit_idx, chan);
        }
    }
# else
  int v = poll_from_cbridge(NULL, 0, 1);
  if (v > 0)
    {
      uint iom_unit_idx = mgp_dev_state.read_unit_idx;
      uint chan         = mgp_dev_state.read_unit_chan;
      if (iom_chan_data[iom_unit_idx][chan].in_use != false )
        {
          // And avoid complaints
          sim_printf("%s: poll %d, terminate interrupt unit \"%s\": iom "
                     "unit %#x, chan %#x\n", __func__, v,
                     mgp_state[iom_unit_idx].device_name,
                     iom_unit_idx, chan);
          send_terminate_interrupt(iom_unit_idx, chan);
        }
    }
# endif /* if USE_SOCKET_DEV_APPROACH */
}

# define CBRIDGE_PACKET_SOCKET        "/tmp/chaos_packet"
# define CBRIDGE_PACKET_HEADER_SIZE   4
# define CH_PK_MAX_DATALEN            488

// Chaosnet opcodes
enum
{
  CHOP_RFC = 1,
  CHOP_OPN,
  CHOP_CLS,
  CHOP_FWD,
  CHOP_ANS,
  CHOP_SNS,
  CHOP_STS,
  CHOP_RUT,
  CHOP_LOS,
  CHOP_LSN,
  CHOP_MNT,
  CHOP_EOF,
  CHOP_UNC,
  CHOP_BRD,
  CHOP_ACK  = 0177, // Note: extension for the NCP Packet socket
  CHOP_DAT  = 0200,
  CHOP_DWD  = 0300
};

enum mgp_pktypes
{
  pktype_NOOP = 1,
  pktype_CONNECT,
  pktype_OPEN,
  pktype_CLOSE,
  pktype_LOSE,
  pktype_STATUS,
  pktype_SEND_STATUS,
  pktype_ORDER,
  pktype_DATA = 255
};

static char *pktype_names[] = {
  "NULL", "NOOP",   "CONNECT",     "OPEN",  "CLOSE",
  "LOSE", "STATUS", "SEND_STATUS", "ORDER", NULL
  };

static char *chop_names[] = {
  "NIL", "RFC", "OPN", "CLS", "FWD", "ANS", "SNS", "STS",
  "RUT", "LOS", "LSN", "MNT", "EOF", "UNC", "BRD"
  };

static char *
pktype_name(uint t)
{
  if (( t > 0 ) && ( t <= pktype_ORDER ))
    {
      return pktype_names[t];
    }
  else if (t == pktype_DATA)
    {
      return "DATA";
    }
  else
    {
      return NULL;
    }
}

static char *
chop_name(uint c)
{
  if (( c > 0 ) && ( c <= CHOP_BRD ))
    {
      return chop_names[c];
    }
  else if (c == CHOP_ACK)
    {
      return "ACK";
    }
  else if (c >= 0200)  //-V536  /* Decimal: 128; Octal: 0200 */
    {
      return "DAT";
    }
  else
    {
      return NULL;
    }
}

// Number of words in an MGP packet header
# define MGP_PACKET_HEADER_SIZE  4

struct mgp_packet_header
{
  // The u_char fields are "technically" 9-bit,
  // but they are always 8-bit in reality
  u_char checksum;             /* Checksum low byte in first word          */
  u_char identification;       /* A character that always appears here (#) */
  u_char packet_type;          /* Similar to Chaos operation code          */
  struct
  {                            /* bits about packet                        */
    u_int unusable  : 1;       /* can't transfer this bit to/from PDP-11   */
    u_int nak       : 1;       /* This packet is NAKing some packet        */
    u_int reply_now : 1;       /* This packet requires an immediate reply  */
    u_int padding   : 5;       /* Unused, reserved                         */
    u_int loopback  : 1;       /* Used for detecting errors                */
  } flags;
  u_char frame_number;         /* Per-link flow control                    */
  u_char receipt_number;       /* ditto                                    */
  u_char packet_number;        /* Per-connection flow control              */
  u_char ack_number;           /* ditto                                    */
  // These three are 2 x 9-bit in Multics, but implemented as 2 x 8-bit
  u_short byte_count;          /* Number of bytes in data                  */
  u_short source_process;      /* Who it came from                         */
  u_short destination_process; /* Who it's going to                        */
  u_char chaos_opcode;         /* The opcode in the original packet        */
  u_char reserved;             /* Not yet used.   MBZ                      */
};

int
valid_chaos_host_address(u_short addr)
{
  // both subnet and host part must be non-zero
  return ( addr > 0xff ) && (( addr & 0xff ) != 0 );
}

// Copy Multics packet data (9-bit) to cbridge packet data (8-bit).
// words is max #words in buf, dlen is max #bytes in dest.
static void
copy_packet9_to_cbridge8(word36 *buf, uint words, u_char *dest, int dlen)
{
# if !defined(__clang_analyzer__)
  int j;
  /* Convert from 9-bit to 8-bit */
  for (j = 0; j < words * 4 && j < dlen; j++)
    {
      // Clang Analyzer warning: 1st function call argument is an uninitialized value
      dest[j] = getbits36_9(buf[MGP_PACKET_HEADER_SIZE + j / 4], ( j % 4 ) * 9);
    }
# endif /* if !defined(__clang_analyzer__) */
}

// and the other way around
static void
copy_cbridge8_to_packet9(u_char *src, int slen, word36 *buf, uint words)
{
  int j;
  /* Convert from 8-bit to 9-bit */
  for (j = 0; j < words * 4 && j < slen; j++)
    {
      putbits36_9(&buf[MGP_PACKET_HEADER_SIZE + j / 4], ( j % 4 ) * 9, src[j]);
    }
}

// cf mgp_checksum_.pl1
// Compute checksum on a raw Multics packet
u_char
mgp_checksum_raw(word36 *buf, uint words)
{
  int j, cks = 0;
  // Skip checksum and ident bytes
  for (j = 2; j < words * 4; j++)
    {
      cks += getbits36_9(buf[j / 4], ( j % 4 ) * 9);
    }

  return cks % 256;
}

// Compute checksum on a header struct and 8-bit byte body
u_char
mgp_checksum(struct mgp_packet_header *p, u_char *pkt, uint pklen)
{
  uint i, cks = 0;

  // Flags are actually 9 bits!
  cks  = ( p->flags.unusable  << 8 )
       | ( p->flags.nak       << 7 )
       | ( p->flags.reply_now << 6 )
       | ( p->flags.padding   << 1 )
       |   p->flags.loopback;

  cks  +=  p->packet_type
       +   p->frame_number
       +   p->receipt_number
       +   p->packet_number
       +   p->ack_number
       + ( p->byte_count           & 0xff )
       + ( p->byte_count          >> 8    )
       + ( p->source_process       & 0xff )
       + ( p->source_process      >> 8    )
       + ( p->destination_process  & 0xff )
       + ( p->destination_process >> 8    )
       +   p->chaos_opcode;
  for (i = 0; i < pklen; i++)
    {
      cks += pkt[i];
    }

  return cks % 256;
}

static struct mgp_packet_header *
parse_packet_header(word36 *buf, uint words)
{
  if (words * 4 < sizeof ( struct mgp_packet_header ))
    {
      sim_printf("%s: buffer too small (%d words) for mgp packet header\n",
                 __func__, words);

      return NULL;
    }

  struct mgp_packet_header *p = malloc(sizeof ( struct mgp_packet_header ));
  if (p == NULL)
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

  int checksum        = getbits36_9(buf[0],  0);
  int id              = getbits36_9(buf[0],  9);
  int pktype          = getbits36_9(buf[0], 18);
  // int flags        = getbits36_9(buf[0], 27);
  int f_unus          = getbits36_1(buf[0], 27);
  int f_nak           = getbits36_1(buf[0], 28);
  int f_rnow          = getbits36_1(buf[0], 29);
  int f_pad           = getbits36_5(buf[0], 30);
  int f_loop          = getbits36_1(buf[0], 35);

  p->checksum         = checksum;
  p->identification   = id;
  p->packet_type      = pktype;
  p->flags.unusable   = f_unus;
  p->flags.nak        = f_nak;
  p->flags.reply_now  = f_rnow;
  p->flags.padding    = f_pad;
  p->flags.loopback   = f_loop;

  int framenr         = getbits36_9(buf[1],  0);
  int rcpt            = getbits36_9(buf[1],  9);
  int pknr            = getbits36_9(buf[1], 18);
  int acknr           = getbits36_9(buf[1], 27);

  p->frame_number     = framenr;
  p->receipt_number   = rcpt;
  p->packet_number    = pknr;
  p->ack_number       = acknr;

  // Note: 8-bit byte values combine to 16-bit values.
  int bytecount  =     ( getbits36_9(buf[2],  0) & 0xff )
                    | (( getbits36_9(buf[2],  9) & 0xff ) << 8 );
  int srcprc     =     ( getbits36_9(buf[2], 18) & 0xff )
                    | (( getbits36_9(buf[2], 27) & 0xff ) << 8 );

  p->byte_count      = bytecount;
  p->source_process  = srcprc;

  int dstprc     =     ( getbits36_9(buf[3],  0) & 0xff )
                    | (( getbits36_9(buf[3],  9) & 0xff ) << 8 );
  int chopcode   =       getbits36_9(buf[3], 18);
  int mbz        =       getbits36_9(buf[3], 27);

  p->destination_process  = dstprc;
  p->chaos_opcode         = chopcode;
  p->reserved             = mbz;

  return p;
}

static void
unparse_packet_header(struct mgp_packet_header *p, word36 *buf, uint words)
{
  if (words * 4 < sizeof ( struct mgp_packet_header ))
    {
      sim_printf("%s: buffer too small (%d words) for mgp packet header\n",
                 __func__, words);

      return;
    }

  putbits36_9(&buf[0],  0, p->checksum);
  putbits36_9(&buf[0],  9, p->identification);
  putbits36_9(&buf[0], 18, p->packet_type);
  putbits36_1(&buf[0], 27, p->flags.unusable);
  putbits36_1(&buf[0], 28, p->flags.nak);
  putbits36_1(&buf[0], 29, p->flags.reply_now);
  putbits36_5(&buf[0], 30, p->flags.padding);
  putbits36_1(&buf[0], 35, p->flags.loopback);

  putbits36_9(&buf[1],  0, p->frame_number);
  putbits36_9(&buf[1],  9, p->receipt_number);
  putbits36_9(&buf[1], 18, p->packet_number);
  putbits36_9(&buf[1], 27, p->ack_number);

  // Note: these mgp_packet_header values
  // are 16 bits, and put in 9-bit fields

  putbits36_9(&buf[2],  0, p->byte_count      & 0xff);
  putbits36_9(&buf[2],  9, p->byte_count     >> 8   );
  putbits36_9(&buf[2], 18, p->source_process  & 0xff);
  putbits36_9(&buf[2], 27, p->source_process >> 8   );

  putbits36_9(&buf[3],  0, p->destination_process  & 0xff);
  putbits36_9(&buf[3],  9, p->destination_process >> 8   );

  putbits36_9(&buf[3], 18, p->chaos_opcode);
  putbits36_9(&buf[3], 27, p->reserved);
}

static void
dumppkt(char *hdr, word36 *buf, uint words)
{
  int i;
  struct mgp_packet_header *p = parse_packet_header(buf, words);
  if (p == NULL)
    {
      sim_printf("%s: failed to parse packet!\n", __func__);

      return;
    }

  sim_printf("%s packet (%d words)\n", hdr, words);
  sim_printf("cks %#x, id %#x, type %#x (%s), flags %#x (%s%s%s%s)\n"
             "frame %#x, rcpt %#x, pknr %#x, acknr %#x\n"
             "bytecount %d, src %#x, dst %#x, chopcode %#o (%s)\n",
             p->checksum, p->identification, p->packet_type,
             pktype_name(p->packet_type),
             ( p->flags.unusable << 8 ) | ( p->flags.nak << 7 )
             | ( p->flags.reply_now << 6 ) | ( p->flags.padding << 1 )
             | p->flags.loopback, p->flags.unusable ? "unusable " : "",
             p->flags.nak ? "NAK " : "", p->flags.reply_now ? "rNOW " : "",
             p->flags.loopback ? "loop" : "", p->frame_number,
             p->receipt_number, p->packet_number, p->ack_number,
             p->byte_count, p->source_process, p->destination_process,
             p->chaos_opcode, chop_name(p->chaos_opcode));

  if (p->identification != '#')
    {
      sim_printf("[Warning: identification byte is %d instead of %d]\n",
                 p->identification, '#');
    }

  if (p->reserved != 0)
    {
      sim_printf("[Warning: MBZ byte is %d]\n", p->reserved);
    }

  int pklen = 4 + ( p->byte_count / 4 ) \
                + ( p->byte_count % 4 ? 1 : 0 );
  FREE(p);
  for (i = 0; i < pklen; i++)
    {
      int lh  = getbits36_18 (buf[i],  0);
      int rh  = getbits36_18 (buf[i], 18);
      int b0  = getbits36_9  (buf[i],  0);
      int b1  = getbits36_9  (buf[i],  9);
      int b2  = getbits36_9  (buf[i], 18);
      int b3  = getbits36_9  (buf[i], 27);
      if (i < MGP_PACKET_HEADER_SIZE)
        {
          sim_printf(" %d: %06o,,%06o = 0x%02x %02x %02x %02x\n",
                     i, lh, rh, b0, b1, b2, b3);
        }
      else
        {
          /* avoid printing NULs for the console output to work */
          char chars[128], *cp = chars;
          (void)memset(chars, 0, sizeof ( chars ));
          if (b0 && b0 < 0177 && b0 >= 040)
            {
              cp += sprintf(cp, "'%c' ",
                      b0);
            }
          else
            {
              cp += sprintf(cp, "'^%c' ",
                      b0 < 0100 ? b0 + 0100 : b0 - 0100);
            }

          if (b1 && b1 < 0177 && b1 >= 040)
            {
              cp += sprintf(cp, "'%c' ",
                      b1);
            }
          else
            {
              cp += sprintf(cp, "'^%c' ",
                      b1 < 0100 ? b1 + 0100 : b1 - 0100);
            }

          if (b2 && b2 < 0177 && b2 >= 040)
            {
              cp += sprintf(cp, "'%c' ",
                      b2);
            }
          else
            {
              cp += sprintf(cp, "'^%c' ",
                      b2 < 0100 ? b2 + 0100 : b2 - 0100);
            }

          if (b3 && b3 < 0177 && b3 >= 040)
            {
              cp += sprintf(cp, "'%c'",
                      b3);
            }
          else
            {
              cp += sprintf(cp, "'^%c'",
                      b3 < 0100 ? b3 + 0100 : b3 - 0100);
            }

          sim_printf(" %d: %06o,,%06o = 0x%02x %02x %02x %02x = %s\n",
                     i, lh, rh, b0, b1, b2, b3, chars);
        }
    }

  sim_printf("EOP\n"); /* although this helps */
}

// Pipe for sending conn index which needs to have a STATUS packet sent for
static int status_conns[2];

static void
mgp_init_dev_state(void)
{
  (void)memset(&mgp_dev_state, 0, sizeof ( mgp_dev_state ));
  // Start reading at the lowest index
  mgp_dev_state.read_index  = -1;

  status_conns[0] = status_conns[1] = 0;
  if (pipe(status_conns) < 0)
    {
      sim_printf("%s: error from pipe(): %s (%d)\n",
                 __func__, xstrerror_l(errno), errno);
    }

  // Init randomness
  uint32_t h = 0;  /* initial hash value */
# if __STDC_VERSION__ < 201112L
  /* LINTED E_OLD_STYLE_FUNC_DECL */
  void *(*mallocptr)() = malloc;
  h = hash32s(&mallocptr, sizeof(mallocptr), h);
# endif /* if __STDC_VERSION__ < 201112L */
  void *small = malloc(1);
  h = hash32s(&small, sizeof(small), h);
  FREE(small);
  void *big = malloc(1UL << 20);
  h = hash32s(&big, sizeof(big), h);
  FREE(big);
  void *ptr = &ptr;
  h = hash32s(&ptr, sizeof(ptr), h);
  time_t t = time(0);
  h = hash32s(&t, sizeof(t), h);
# if !defined(_AIX)
  for (int i = 0; i < 1000; i++)
    {
      unsigned long counter = 0;
      clock_t start = clock();
      while (clock() == start)
        {
          counter++;
        }
      h = hash32s(&start, sizeof(start), h);
      h = hash32s(&counter, sizeof(counter), h);
    }
# endif /* if !defined(_AIX) */
  int mypid = (int)getpid();
  h = hash32s(&mypid, sizeof(mypid), h);
  char rnd[4];
  FILE *f = fopen("/dev/urandom", "rb");
  if (f)
    {
      if (fread(rnd, sizeof(rnd), 1, f))
        {
          h = hash32s(rnd, sizeof(rnd), h);
        }
      fclose(f);
    }
  srandom(h);
}

static void
mgp_wants_to_read(uint iom_unit_idx, uint chan)
{
  mgp_dev_state.read_unit_idx  = iom_unit_idx;
  mgp_dev_state.read_unit_chan = chan;
  mgp_dev_state.want_to_read   = 1;
}

// Find the first conn which has a zero skt
static int
find_free_conn(void)
{
  int i;
  for (i = 0; i < MAX_CONNS; i++)
    {
      if (mgp_dev_state.conns[i].skt == 0)
        {
          return i;
        }
    }

  return -1;
}

// Find the conn for <remote,local> id.
// local may be 0 which matches all local ids.
static int
find_conn_for(int remote, int local)
{
  int i;
  for (i = 0; i < MAX_CONNS; i++)
    {
      if ( ( mgp_dev_state.conns[i].multics_proc != 0 )
        && ( mgp_dev_state.conns[i].multics_proc == remote )
        && (( local == 0 ) || ( mgp_dev_state.conns[i].local_id == local )) )
        {
          return i;
        }
    }

  return -1;
}

// Return a malloc'd pkt with cbridge header filled in.
static u_char *
make_cbridge_pkt(int len, int opcode)
{
  u_char *pkt = malloc(len + CBRIDGE_PACKET_HEADER_SIZE); /* space for cbridge header */
  if (pkt == NULL)
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
  (void)memset(pkt, 0, len + CBRIDGE_PACKET_HEADER_SIZE);

  pkt[0]  = opcode;
  pkt[2]  = len & 0xff;
  pkt[3]  = len >> 8;

  return pkt;
}

static u_char *
make_rfc_pkt(int *len, char *host, char *contact, char *args)
{
  /* cbridge header + strings with spaces + NUL */
  *len = strlen(host) + 1 + strlen(contact) + \
         ( args == NULL ? 0 : 1 + strlen(args) ) + 1;

  u_char *pkt = make_cbridge_pkt(*len, CHOP_RFC);
  (void)sprintf((char *)&pkt[CBRIDGE_PACKET_HEADER_SIZE],
                "%s %s%s%s", host, contact,
                args == NULL || *args == '\0' ? "" : " ",
                args == NULL || *args == '\0' ? "" : args);

  return pkt;
}

// Open a packet socket to cbridge (done for each conn)
int
cbridge_open_socket(void)
{
  int slen, sock;
  struct sockaddr_un server;

  if (( sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
      // Check for "out of sockets" or similar?
      sim_printf("%s: socket(AF_UNIX) error: %s (%d)",
                 __func__, xstrerror_l(errno), errno); // @@@@ handle error better?

      return sock;
    }

  server.sun_family  = AF_UNIX;
  (void)sprintf(server.sun_path, "%s", CBRIDGE_PACKET_SOCKET);
  slen = strlen(server.sun_path) + 1 + sizeof ( server.sun_family );

  if (connect(sock, (struct sockaddr *)&server, slen) < 0)
    {
      sim_printf("%s: connect(%s) error: %s (%d)", __func__,
                 server.sun_path, xstrerror_l(errno), errno);
      // @@@@ let the device go down, instead
      close(sock);
      sock = 0;
    }

  return sock;
}

static void
close_conn(int i)
{
  if (i < 0)
    {
      sim_printf("%s: closing conn %d which is invalid!\n", __func__, i);
      return;
    }
  sim_printf("%s: closing conn %d <%#x,%#x>, remote %#o, contact \"%s\"\n",
             __func__, i,
             mgp_dev_state.conns[i].multics_proc,
             mgp_dev_state.conns[i].local_id,
             mgp_dev_state.conns[i].remote_addr,
             mgp_dev_state.conns[i].contact_name);

  if (mgp_dev_state.conns[i].skt > 0)
    {
      close(mgp_dev_state.conns[i].skt);
    }

  mgp_dev_state.conns[i].multics_proc  = 0;
  mgp_dev_state.conns[i].local_id      = 0;
  if (mgp_dev_state.conns[i].contact_name != NULL)
    {
      FREE(mgp_dev_state.conns[i].contact_name);
    }

  mgp_dev_state.conns[i].contact_name  = NULL;
  mgp_dev_state.conns[i].remote_addr   = 0;
}

static int
cbridge_send_packet(int i, u_char *pkt, int len)
{
  int skt  = mgp_dev_state.conns[i].skt;
  int x    = write(skt, pkt, len);
  if (x < 0)
    {
      // @@@@ handle error, pass it on to Multics
      if (( errno == EBADF      )  \
       || ( errno == ECONNRESET )  \
       || ( errno == EPIPE      ))
        {
          sim_printf("%s: socket seems to have closed: %s\n",
                     __func__, xstrerror_l(errno));
          close_conn(i);
        }
      else
        {
          sim_warn("%s: socket write error: %s (%d)\n",
                   __func__, xstrerror_l(errno), errno);
        }
    }
  else if (x != len)
    {
      sim_printf("%s: wrote %d bytes (expected %d)\n", __func__, x, len);
    }

  FREE(pkt);
  return x;
}

// Handle packets from Multics, send them to cbridge
static int
handle_packet(int opcode, struct mgp_packet_header *p, word36 *buf,
              uint words)
{
  int i = find_conn_for(p->source_process, p->destination_process);
  if (i < 0)
    {
      sim_warn("%s: can't find conn for %#x,%#x\n",
               __func__, p->source_process, p->destination_process);
      return -1;
    }

  u_char *pkt = make_cbridge_pkt(p->byte_count, opcode);
  if (p->byte_count > 0)
    {
      copy_packet9_to_cbridge8(
        buf, words, pkt + CBRIDGE_PACKET_HEADER_SIZE, p->byte_count);
    }

  return cbridge_send_packet(
                 i, pkt, CBRIDGE_PACKET_HEADER_SIZE + p->byte_count);
}

static int
handle_close(struct mgp_packet_header *p, word36 *buf, uint words)
{
  // Close the connection in a controlled way:
  // first send and EOF with payload "wait",
  // which makes cbridge send the EOF and wait (some time) for its
  // acknowledgement, and then send an ACK packet here. When the ACK arrives,
  // send the CLS. (No need to save a close message somewhere, since Multics
  // doesn't include any data.)
  word36 eof[1];
  copy_cbridge8_to_packet9((u_char *)"wait", 4, buf, 1);

  return handle_packet(CHOP_EOF, p, eof, 1);
}

static int
handle_lose(int conni, struct mgp_packet_header *p, word36 *buf, uint words)
{
  // send LOS, close socket, clear out our conn
  int v = handle_packet(CHOP_LOS, p, buf, words);
  // after sending a LOS, the connection is gone
  close_conn(conni);

  return v;
}

static int
handle_connect(struct mgp_packet_header *p, word36 *buf, uint words)
{
  char connect_string[256], *net, *host, *contact, *args;
  char *i;
  copy_packet9_to_cbridge8(buf,
    words, (u_char *)connect_string, sizeof ( connect_string ));
  sim_printf("%s: connect string is \"%s\"\n", __func__, connect_string);
  // Parse the connect string, something like "CHAOS 12234 NAME /W BV"
  net  = connect_string;
  i    = index(net, ' ');
  if (i == NULL)
    {
      sim_printf("%s: bad connect string: first space not found\n", __func__);

      return -1;
    }

  *i    = '\0';
  host  = i + 1;
  i     = index(host, ' ');
  if (i == NULL)
    {
      sim_printf("%s: bad connect string: second space not found\n", __func__);

      return -1;
    }

  *i       = '\0';
  contact  = i + 1;
  i        = index(contact, ' ');
  if (i == NULL)
    {
      sim_printf("%s: third space not found, no contact args\n", __func__);
      args = NULL;
    }
  else
    {
      *i    = '\0';
      args  = i + 1;
    }

  sim_printf("%s: parsed connect string: net \"%s\", host \"%s\", contact "
             "\"%s\", args \"%s\"\n", __func__, net, host, contact, args);
  if (strcasecmp(net, "CHAOS") != 0)
    {
      sim_printf("%s: not CHAOS net, ignoring\n", __func__);

      return -1;
    }

  // Now find a free slot in mgp_dev_state.conns,
  int cindex               = find_free_conn();
  if (cindex < 0)
    {
      sim_printf("%s: no free conns available!\n", __func__);
      return -1;
    }
  struct conn_table *conn  = &mgp_dev_state.conns[cindex];
  // save the contact name
  if (conn->contact_name) /* Free any old string */
    {
      FREE(conn->contact_name);
    }

  conn->contact_name = strdup(contact);
  // parse the host address and save it
  u_short raddr;
  if (( sscanf(host, "%ho", &raddr) != 1 ) || !valid_chaos_host_address(raddr))
    {
      sim_printf("%s: bad remote address %s\n", __func__, host);

      return -1;
    }
  else
    {
      conn->remote_addr = raddr;
    }

  // make a local id, fill in multics id,
  conn->local_id      = random() % ( 1 << 16 ); /* srandom() called in mgp_init_dev_state */
  conn->multics_proc  = p->source_process;
  // open a socket,
  int cbskt = cbridge_open_socket();
  if (cbskt < 0)
    {
      sim_printf("%s: unable to get a socket\n", __func__);

      return cbskt;
    }

  conn->skt = cbskt;
  // construct a cbridge packet,
  int cblen;
  u_char *cbpkt = make_rfc_pkt(&cblen, host, contact, args);

  // send the packet on it
  int v = cbridge_send_packet(cindex, cbpkt,
              CBRIDGE_PACKET_HEADER_SIZE + cblen);  /* incl header pls */

  if (v < 0)
    {
      // failed sending, close it again
      // @@@@ and pass on error to Multics
      close_conn(cindex);

      return -1;
    }
  else
    {
      int i = cindex;
      sim_printf(
           "%s: opened conn %d <%#x,%#x>, remote %#o, contact \"%s\"\n",
           __func__, i, mgp_dev_state.conns[i].multics_proc,
           mgp_dev_state.conns[i].local_id, mgp_dev_state.conns[i].remote_addr,
           mgp_dev_state.conns[i].contact_name);

      return cindex;
    }
}

// Given pkt read from Multics, act on it
static int
handle_mgp_packet(word36 *buf, uint words)
{
  struct mgp_packet_header *p  = parse_packet_header(buf, words);
  int rval                     = 0;

  if (   ( p->checksum       == 0 ) \
      && ( p->packet_type    == 0 ) \
      && ( p->frame_number   == 0 ) \
      && ( p->receipt_number == 0 ) )
    {
      FREE(p);
      // Not a real packet, ignore
      return 0;
    }

  if (mgp_dev_state.first_frame_received
      && ( p->frame_number != ( mgp_dev_state.frame_last_received + 1 )))
    {
      sim_printf("%s: unordered frame %#x read, expected %#x\n", __func__,
                 p->frame_number, mgp_dev_state.frame_last_received + 1);
      // send NAK?
    }
  else
    {
      mgp_dev_state.first_frame_received = 1;
    }

  int i = find_conn_for(p->source_process, p->destination_process);
  sim_printf(
          "%s: packet %#x (ack %#x) for conn %d <%#x,%#x>, pktype %d (%s)\n",
          __func__, p->packet_number, p->ack_number, i,
          i < 0 ? 0 : mgp_dev_state.conns[i].multics_proc,
          i < 0 ? 0 : mgp_dev_state.conns[i].local_id,
          p->packet_type, pktype_name(p->packet_type));

  int pktype = p->packet_type;
  switch (pktype)
    {
    case pktype_NOOP:
      // NOOP seems to be nonspecific to conn, just conveying frame/receipt
      // (handled above/below)
      break;

    case pktype_CONNECT:
      rval = handle_connect(p, buf, words);
      break;

    case pktype_OPEN:
      // make it ANS or OPN, depending on opcode
      rval = handle_packet(
        p->chaos_opcode ? p->chaos_opcode : CHOP_OPN, p, buf, words);
      break;

    case pktype_CLOSE:
      rval = handle_close(p, buf, words);
      break;

    case pktype_LOSE:
      rval = handle_lose(i, p, buf, words);
      break;

    case pktype_DATA:
      // just convert to DAT and send (but watch out for EOF, check opcode)
      rval = handle_packet(
        p->chaos_opcode ? p->chaos_opcode : CHOP_DAT, p, buf, words);
      break;

    case pktype_SEND_STATUS:
      if (status_conns[1] > 0)
        {
          char b[2] = { i, 0 };
          sim_printf(
              "%s: asking for STATUS to be sent for conn %d on status_conns\n",
              __func__, i);
          if (write(status_conns[1], b, 1) < 0)
            {
              sim_printf(
                  "%s: write() on status_conns failed: %s (%d)\n",
                  __func__, xstrerror_l(errno), errno);
              status_conns[1] = status_conns[0] = 0;
            }
        }

      break;

    case pktype_STATUS:
      sim_printf("%s: STATUS for conn %d: frame,rcpt = <%#x,%#x>, pkt,ack = "
            "<%#x,%#x>\n", __func__, i, p->frame_number, p->receipt_number,
            p->packet_number, p->ack_number);
      break;

    default:
      sim_printf("%s: can't handle pkt type %#o (%s) yet\n",
                 __func__, pktype, pktype_name(pktype));
      rval = -1;
    }

  // Set a mark to make a NOOP being available for reading next.
    if (p->flags.reply_now)
      {
        sim_printf("%s: reply_NOW set, setting flag for sending NOOP\n",
                   __func__);
        mgp_dev_state.send_noop = 1;
# if 0  // Not sure about this yet
        // Also write this index on the status pipe, so a STATUS can be sent
        if (status_conns[1] > 0)
          {
            if (( i < 0 ) && ( pktype == pktype_CONNECT ) && ( rval >= 0 ))
              {
                // a new conn which just opened
                i = rval;
              }

            if (i >= 0)
              {
                // But only do this if there is actually a conn
                char b[2] = { i, 0 };
                sim_printf(
                    "%s: asking for STATUS to be sent for conn %d on status_conns\n",
                    __func__, i);
                if (write(status_conns[1], b, 1) < 0)
                  {
                    sim_printf("%s: write() on status_conns failed: %s (%d)\n",
                               __func__, xstrerror_l(errno), errno);
                    status_conns[1] = status_conns[0] = 0;
                  }
              }
          }
# endif /* if 0 */
      }

  // Count the frame
  mgp_dev_state.frame_last_received = p->frame_number;
  sim_printf("%s: afterwards, frame last sent %#x, last received %#x\n",
             __func__, mgp_dev_state.frame_last_sent,
             mgp_dev_state.frame_last_received);
  FREE(p);

  return rval;
}

// Handle packets from cbridge, give them to Multics.

// Mapping from Chaos opcode to MGP packet type
u_char opcode_to_pktype[] = {
  0,              /* None                    */
  pktype_CONNECT, /* RFC                     */
  pktype_OPEN,    /* OPN                     */
  pktype_CLOSE,   /* CLS                     */
  0,              /* FWD (not handled)       */
  pktype_OPEN,    /* ANS                     */
  0,              /* SNS (never appears)     */
  pktype_STATUS,  /* STS (internal use only) */
  0,              /* RUT (never appears)     */
  pktype_LOSE,    /* LOS                     */
  0,              /* LSN (never appears)     */
  0,              /* MNT (never appears)     */
  pktype_DATA,    /* EOF                     */
  0,              /* UNC (not handled)       */
  pktype_CONNECT  /* BRD                     */
};

static void
make_mgp_header(struct mgp_packet_header *p, u_char opcode, u_char *pkt,
                uint pklen, int i)
{
  (void)memset(p, 0, sizeof ( struct mgp_packet_header ));
  p->identification = '#';
  if (( opcode > 0 ) && ( opcode <= CHOP_BRD ))
    {
      p->packet_type = opcode_to_pktype[opcode];
    }
  else if (opcode >= CHOP_DAT)
    {
      p->packet_type = pktype_DATA;
    }
  else if (i == -1) /* special case */
    {
      p->packet_type = pktype_NOOP;
    }

  /* no flags? */
  p->flags.reply_now  = 1;
  p->frame_number     = ++mgp_dev_state.frame_last_sent;
  p->receipt_number   = mgp_dev_state.frame_last_received;
  if (i >= 0)
    {
      // Never mind about these if it is a NOOP we're making
      p->packet_number        = ++mgp_dev_state.conns[i].pkt_last_sent;
      p->ack_number           =   mgp_dev_state.conns[i].pkt_last_received;
      p->source_process       =   mgp_dev_state.conns[i].local_id;
      p->destination_process  =   mgp_dev_state.conns[i].multics_proc;
    }

  p->chaos_opcode  = opcode;
  p->byte_count    = pklen;
  sim_printf(
        "%s: made %s (%d) f,r=<%#x,%#x> p,a=<%#x,%#x> opc %s (%d) bc %d\n",
        __func__, pktype_name(p->packet_type), p->packet_type, p->frame_number,
        p->receipt_number, p->packet_number, p->ack_number,
        chop_name(p->chaos_opcode), p->chaos_opcode, p->byte_count);
}

static int
make_mgp_packet(u_char opcode, u_char *pkt, uint pklen, word36 *buf,
                uint words, uint conni)
{
  struct mgp_packet_header hdr;

  // Make an mgp header
  make_mgp_header(&hdr, opcode, pkt, pklen, conni);

  // fill in the checksum (which is computed on the 8-bit bytes!)
  hdr.checksum = mgp_checksum(&hdr, pkt, pklen);

  // fill in the header in the buffer
  unparse_packet_header(&hdr, buf, words);

  // copy the data part, converting from 8-bit to 9-bit
  copy_cbridge8_to_packet9(pkt, pklen, buf, words);
  sim_printf("%s: conn %d <%#x,%#x> made %s pkt %#x (ack %#x), frame %#x "
      "(rcpt %#x)\n", __func__, conni, mgp_dev_state.conns[conni].local_id,
      mgp_dev_state.conns[conni].multics_proc, pktype_name(hdr.packet_type),
      hdr.packet_number, hdr.ack_number, hdr.frame_number, hdr.receipt_number);

  return 0;
}

static int
make_status_packet(int conni, word36 *buf, uint words)
{
  struct mgp_packet_header hdr;

  // Make an mgp header - reuse CHOP_STS which doesn't appear otherwise
  make_mgp_header(&hdr, CHOP_STS, NULL, 0, conni);

  // fill in the checksum (which is computed on the 8-bit bytes!)
  hdr.checksum = mgp_checksum(&hdr, NULL, 0);

  // fill in the header in the buffer
  unparse_packet_header(&hdr, buf, words);

  // (no data to copy)
  sim_printf("%s: conn %d <%#x,%#x> made %s pkt %#x (ack %#x), frame %#x "
      "(rcpt %#x)\n", __func__, conni, mgp_dev_state.conns[conni].local_id,
      mgp_dev_state.conns[conni].multics_proc, pktype_name(hdr.packet_type),
      hdr.packet_number, hdr.ack_number, hdr.frame_number, hdr.receipt_number);

  return 0;
}

static int
make_noop_packet(word36 *buf, uint words)
{
  struct mgp_packet_header hdr;

  // Make an mgp header
  make_mgp_header(&hdr, 0, NULL, 0, -1);
  hdr.packet_number  = 1;

  // fill in the checksum
  hdr.checksum       = mgp_checksum(&hdr, NULL, 0);

  // fill in the header in the buffer
  unparse_packet_header(&hdr, buf, words);

  // (no data to copy)
  sim_printf("%s: made NOOP pkt %#x (ack %#x), frame %#x (rcpt %#x)\n",
             __func__, hdr.packet_number, hdr.ack_number, hdr.frame_number,
             hdr.receipt_number);

  return 0;
}

static int
make_open_packet(u_char opcode, u_char *pkt, uint pklen, word36 *buf,
                 uint words, uint conni)
{
  if (opcode == CHOP_OPN)
    {
      // Skip content of OPN packets, which is
      // the ascii host name (or octal address)
      return make_mgp_packet(opcode, pkt, 0, buf, words, conni);
    }
  else if (opcode == CHOP_ANS)
    {
      // First two bytes is the source (useful e.g. if we sent a broadcast)
      u_short src = pkt[0] | ( pkt[1] << 8 );
      if (src != mgp_dev_state.conns[conni].remote_addr)
        {
          sim_printf("%s: got ANS from %#o but had remote addr %#o\n",
                     __func__, src, mgp_dev_state.conns[conni].remote_addr);
                     mgp_dev_state.conns[conni].remote_addr = src;
        }

      // Skip the two source bytes in the mgp packet
      return make_mgp_packet(opcode, pkt + 2, pklen - 2, buf, words, conni);
    }
  else
    {
      sim_warn("%s: BUG: opcode is not ANS or OPN: %s (%#o)\n",
               __func__, chop_name(opcode), opcode);

      return -1;
    }
}

static int
make_connect_packet(u_char *pkt, uint pklen, word36 *buf, uint words,
                    uint conni)
{
  // An RFC cbridge pkt is almost like a mgp connect pkt
  char *i, *rhost, *args = NULL, connect[128];
  rhost = (char *)pkt;
  // Zap any line ending
  if (( i = index(rhost, '\r')) != NULL)
    {
      *i = '\0';
    }
  else if (( i = index(rhost, '\n')) != NULL)
    {
      *i = '\0';
    }

  // Find any contact args
  if (( i = index(rhost, ' ')) != NULL)
    {
      args  = i + 1;
      *i    = '\0';
    }

  // @@@@ beware: I think Multics wants the name of the remote host,
  // @@@@ not the address.
  // @@@@ But it should handle the address, I think/hope?
  // @@@@ Make sure contact_name is set up also when listening
  // @@@@ - this is future work.

  // Prefix with the "net", add contact name, and args
  if ( ( mgp_dev_state.conns[conni].contact_name == NULL )
   || ( *mgp_dev_state.conns[conni].contact_name == '\0' ))
    {
      sim_printf("%s: no contact name known for conn %d\n", __func__, conni);

      return -1;
    }

  (void)sprintf(connect, "CHAOS %s %s%s%s",
                rhost, mgp_dev_state.conns[conni].contact_name,
                args == NULL ? "" : " ", args == NULL ? "" : args);

  return make_mgp_packet(CHOP_RFC,
    (u_char *)connect, strlen(connect), buf, words, conni);
}

static int
receive_cbridge_opcode(u_char copcode, u_char *cbuf, uint clen, word36 *buf,
                       uint words, int conni)
{
  sim_printf("%s: got opcode %#o (%s)\n",
             __func__, copcode, chop_name(copcode));
  switch (copcode)
    {
    case CHOP_FWD:
    // These should never be seen:
    case CHOP_RUT:
    case CHOP_MNT:
    case CHOP_UNC:
    case CHOP_STS:
    case CHOP_SNS:
    // Do nothing.
      return -1;

    case CHOP_RFC: /* this can originally be a BRD, too */
      // format is similar, but not same
      return make_connect_packet(cbuf, clen, buf, words, conni);

    case CHOP_OPN:
    case CHOP_ANS:
      return make_open_packet(copcode, cbuf, clen, buf, words, conni);

    case CHOP_ACK:
    {
      // This is the ACK of an EOF[wait] (see handle_close),
      // so now it is time to send a CLS
      u_char *clspkt = make_cbridge_pkt(0, CHOP_CLS);
      cbridge_send_packet(conni, clspkt, 0);
      // After sending a CLS, the connection can be discarded (cf MIT AIM 628)
      close_conn(conni);
    }
    break;

    case CHOP_CLS:
    case CHOP_LOS:
    {
      // Pass it on to Multics
      int v = make_mgp_packet(copcode, cbuf, clen, buf, words, conni);
      // and then discard the connection
      close_conn(conni);

      return v;
    }

    default:
      // Most pkts have the same content
      return make_mgp_packet(copcode, cbuf, clen, buf, words, conni);
    }

  return 1;
}

// Read result into buf, return the number of bytes/words put there or -1.
static int
poll_from_cbridge(word36 *buf, uint words, uint probe_only)
{
  fd_set rfd;
  int maxfd, numfds, i, sval, cnt, rval = -1;
  int foundone = 0, tryagain = 0;
  u_char cbuf[CH_PK_MAX_DATALEN + CBRIDGE_PACKET_HEADER_SIZE]; /* Fit data + cbridge header */

  maxfd   = 0;
  numfds  = 0;
  // Add the status pipe to the fd set
  FD_ZERO(&rfd);
  if (status_conns[0] != 0)
    {
      FD_SET(status_conns[0], &rfd);
      maxfd = status_conns[0];
      numfds++;
    }

  // Add all existing conns
  for (i = 0; i < MAX_CONNS; i++)
    {
      if (mgp_dev_state.conns[i].skt != 0)
        {
          FD_SET(mgp_dev_state.conns[i].skt, &rfd);
          numfds++;
          if (mgp_dev_state.conns[i].skt > maxfd)
            {
              maxfd = mgp_dev_state.conns[i].skt;
            }
        }
    }

  if (maxfd > 0)
    {
      struct timeval timeout;
      do
        {
          tryagain         = 0;
          // set a very short timeout (e.g. 0.01ms)
          timeout.tv_sec   = 0;
          timeout.tv_usec  = 10;
          sval             = select(maxfd + 1, &rfd, NULL, NULL, &timeout);
          if (probe_only)
            {
              if ((sval < 0) && (errno == EINTR))
                {
                  return 0; /* Timeout, nothing to read */
                }
              else
                {
                  return sval;
                }
            }

          if (sval < 0)
            {
              if (errno == EINTR)
                {
                  // timed out
                }
              else
                {
                  sim_printf(
                       "%s: select() error, maxfd %d, numfds %d: %s (%d)\n",
                       __func__, maxfd, numfds, xstrerror_l(errno), errno);
                }

              return -1;
            }
          else if (sval > 0)
            {
              int statusconn = -1;

              // First see if a STATUS should be sent
              if (( status_conns[0] != 0 ) && FD_ISSET(status_conns[0], &rfd))
                {
                  char b[2];
                  sim_printf(
                       "%s: about to read a byte from status_conns[0] = %d\n",
                       __func__, status_conns[0]);
                  int s = read(status_conns[0], b, 1);
                  if (s < 0)
                    {
                      sim_warn("%s: read on status_conns failed: %s (%d)\n",
                               __func__, xstrerror_l(errno), errno);
                      status_conns[0] = status_conns[1] = 0;
                    }
                  else if (s == 0)
                    {
                      sim_printf("%s: read on status_conns returned 0\n",
                                 __func__);
                      status_conns[0] = status_conns[1] = 0;
                    }
                  else
                    {
                      sim_printf(
                         "%s: read %d from status_conns, make STATUS packet\n",
                         __func__, b[0]);
                      // make a STATUS packet
                      statusconn = b[0];
                    }
                }

              // Start at the one after the previously read index, to make it
              // round-robin-like
              for (i = mgp_dev_state.read_index + 1; i < MAX_CONNS; i++)
                {
                  if (FD_ISSET(mgp_dev_state.conns[i].skt, &rfd))
                    {
                      mgp_dev_state.read_index = i;
                      // read the header first, then that many bytes
                      if (( cnt = read(mgp_dev_state.conns[i].skt,
                              cbuf, CBRIDGE_PACKET_HEADER_SIZE)) < 0)
                        {
                          // @@@@ handle error, socket closed, pass on to Multics
                          sim_printf(
                              "%s: read() header error for conn %d: %s (%d)\n",
                              __func__, i, xstrerror_l(errno), errno);
                          FD_CLR(mgp_dev_state.conns[i].skt, &rfd);
                          numfds--;
                          close_conn(i);
                          foundone = 0;
                          continue;
                        }
                      else if (cnt == 0)
                        {
                          sim_printf(
                              "%s: read() header zero length conn %d, assuming closed\n",
                              __func__, i);
                          FD_CLR(mgp_dev_state.conns[i].skt, &rfd);
                          numfds--;
                          close_conn(i);
                          foundone = 0;
                          continue;
                        }
                      else if (cnt != CBRIDGE_PACKET_HEADER_SIZE)
                        {
                          sim_printf(
                                "%s: read() header length %d for conn %d\n",
                                __func__, cnt, i);
                          foundone = 0;
                          continue;
                        }

                      // Parse the cbridge packet header
                      int copcode  = cbuf[0];
                      int mbz      = cbuf[1];
                      int clen     = cbuf[2] | ( cbuf[3] << 8 );
                      sim_printf(
                        "%s: read cbridge pkt: opcode %#o (%s), mbz %d, len %d\n",
                        __func__, copcode, chop_name(copcode), mbz, clen);
                      if (( mbz != 0 ) || (( copcode > CHOP_BRD )  \
                                  && ( copcode < CHOP_ACK ))       \
                                  || ( clen > CH_PK_MAX_DATALEN ))
                        {
                          sim_printf(
                            "%s: cbridge header bad: opcode %#o (%s), mbz %d, len %d\n",
                            __func__, copcode, chop_name(copcode), mbz, clen);
                          FD_CLR(mgp_dev_state.conns[i].skt, &rfd);
                          numfds--;
                          close_conn(i);
                          foundone  = 0;
                          rval      = -1;
                          break;
                        }
                      else if (( cnt = read(mgp_dev_state.conns[i].skt, cbuf, clen)) < 0)
                        {
                          // @@@@ handle error, socket closed, pass on to Multics
                          sim_printf(
                               "%s: read() body error for conn %d: %s (%d)\n",
                               __func__, i, xstrerror_l(errno), errno);
                          FD_CLR(mgp_dev_state.conns[i].skt, &rfd);
                          numfds--;
                          close_conn(i);
                          foundone = 0;
                          continue;
                        }
                      else if (cnt != clen)
                        {
                          sim_printf(
                            "%s: read() body read %d (expected %d) for conn %d\n",
                            __func__, cnt, clen, i);
                          foundone = 0;
                          continue;
                        }
                      else
                        {
                          foundone = 1;
                          if (i == statusconn) /* No need for STATUS if we're
                                                * making some other pkt */
                            {
                              statusconn = -1;
                            }

                          // Handle based on chaos opcode, and return
                          rval = receive_cbridge_opcode(
                            copcode, cbuf, clen, buf, words, i);
                          break;
                        }
                    }
                }

              if (statusconn >= 0)
                {
                  sim_printf("%s: making STATUS packet for conn %d\n",
                             __func__, statusconn);
                  make_status_packet(statusconn, buf, words);
                  foundone  = 1;
                  rval      = 1;
                }
            }
          else
            {
              // sim_printf("%s: select() returned 0 (maxfd %d)\n",
              //   __func__, maxfd);
              rval = -1;
            }

          // If none found, start again at the lowest index
          if (!foundone && ( mgp_dev_state.read_index > -1 ))
            {
              sim_printf(
                  "%s: nothing to read at indices over %d, retrying select\n",
                  __func__, mgp_dev_state.read_index);
              mgp_dev_state.read_index  = -1;
              tryagain                  =  1;
            }
        }
      while (tryagain);
    }

  // If we didn't already send something, make a NOOP
  if (mgp_dev_state.send_noop)
    {
      sim_printf("%s: asked to send a NOOP - current frame %#x receipt %#x\n",
                 __func__, mgp_dev_state.frame_last_sent,
                 mgp_dev_state.frame_last_received);
      mgp_dev_state.send_noop = 0;
      if (!foundone)
        { // Only do it if we didn't already send something
          make_noop_packet(buf, words);
          rval = 1;
        }
      else
        {
          sim_printf("%s: already made a packet, skipping NOOP\n", __func__);
        }
    }

  return rval;
}

#endif /* if defined(WITH_MGP_DEV) */
