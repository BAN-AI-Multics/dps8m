/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: CC-PDDC or MIT-0
 * SPDX-FileCopyrightText: Public domain or The DPS8M Development Team
 * scspell-id: 845a8edb-f62f-11ec-a0d8-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * libTELNET - TELNET protocol handling library
 *
 * Sean Middleditch <sean@sourcemud.org>
 *
 * The author or authors of this code dedicate any and all copyright
 * interest in this code to the public domain. We make this dedication
 * for the benefit of the public at large and to the detriment of our heirs
 * and successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * code under copyright law.
 *
 * ---------------------------------------------------------------------------
 */

/*
 * The person or persons who have associated work with this document
 * (the "Dedicator" or "Certifier") hereby either (a) certifies that,
 * to the best of his knowledge, the work of authorship identified
 * is in the public domain of the country from which the work is
 * published, or (b) hereby dedicates whatever copyright the dedicators
 * holds in the work of authorship identified below (the "Work") to the
 * public domain. A certifier, moreover, dedicates any copyright
 * interest he may have in the associated work, and for these purposes,
 * is described as a "dedicator" below.
 *
 * A certifier has taken reasonable steps to verify the copyright
 * status of this work. Certifier recognizes that his good faith
 * efforts may not shield him from liability if in fact the work
 * certified is not in the public domain.
 *
 * Dedicator makes this dedication for the benefit of the public at
 * large and to the detriment of the Dedicator's heirs and successors.
 * Dedicator intends this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights under
 * copyright law, whether vested or contingent, in the Work. Dedicator
 * understands that such relinquishment of all rights includes the
 * relinquishment of all rights to enforce (by lawsuit or otherwise)
 * those copyrights in the Work.
 *
 * Dedicator recognizes that, once placed in the public domain, the
 * Work may be freely reproduced, distributed, transmitted, used,
 * modified, built upon, or otherwise exploited by anyone for any
 * purpose, commercial or non-commercial, and in any way, including by
 * methods that have not yet been invented or conceived.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#ifdef __GNUC__
# define NO_RETURN   __attribute__ ((noreturn))
# define UNUSED      __attribute__ ((unused))
#else
# define NO_RETURN
# define UNUSED
#endif

/* Win32 compatibility */
#if defined(_WIN32)
# define __func__ __FUNCTION__
# if defined(_MSC_VER)
#  if _MSC_VER <= 1700
#   define va_copy(dest, src) (dest = src)
#  endif
# endif
#endif

#include "libtelnet.h"

#undef FREE
#define FREE(p) do  \
  {                 \
    free((p));      \
    (p) = NULL;     \
  } while(0)

#ifdef TESTING
# undef realloc
# undef FREE
# define FREE(p) free(p)
# define realloc trealloc
#endif /* ifdef TESTING */

/* helper for Q-method option tracking */
#define Q_US(q)    ((q).state & 0x0F)
#define Q_HIM(q)  (((q).state & 0xF0) >> 4)
#define Q_MAKE(us,him) ((us) | ((him) << 4))

/* helper for the negotiation routines */
#define NEGOTIATE_EVENT(telnet,cmd,opt) \
        ev.type = (cmd);                \
        ev.neg.telopt = (opt);          \
        (telnet)->eh((telnet), &ev, (telnet)->ud);

/* telnet state codes */
enum telnet_state_t {
        TELNET_STATE_DATA = 0,
        TELNET_STATE_EOL,
        TELNET_STATE_IAC,
        TELNET_STATE_WILL,
        TELNET_STATE_WONT,
        TELNET_STATE_DO,
        TELNET_STATE_DONT,
        TELNET_STATE_SB,
        TELNET_STATE_SB_DATA,
        TELNET_STATE_SB_DATA_IAC
};
typedef enum telnet_state_t telnet_state_t;

/* telnet state tracker */
struct telnet_t {
        /* user data */
        void *ud;
        /* telopt support table */
        const telnet_telopt_t *telopts;
        /* event handler */
        telnet_event_handler_t eh;
        /* RFC-1143 option negotiation states */
        struct telnet_rfc1143_t *q;
        /* sub-request buffer */
        char *buffer;
        /* current size of the buffer */
        size_t buffer_size;
        /* current buffer write position (also length of buffer data) */
        size_t buffer_pos;
        /* current state */
        enum telnet_state_t state;
        /* option flags */
        unsigned char flags;
        /* current subnegotiation telopt */
        unsigned char sb_telopt;
        /* length of RFC-1143 queue */
        unsigned int q_size;
        /* number of entries in RFC-1143 queue */
        unsigned int q_cnt;
};

/* RFC-1143 option negotiation state */
typedef struct telnet_rfc1143_t {
        unsigned char telopt;
        unsigned char state;
} telnet_rfc1143_t;

/* RFC-1143 state names */
#define Q_NO         0
#define Q_YES        1
#define Q_WANTNO     2
#define Q_WANTYES    3
#define Q_WANTNO_OP  4
#define Q_WANTYES_OP 5

/* telnet NVT EOL sequences */
static const char CRLF[]  = { '\r', '\n' };
static const char CRNUL[] = { '\r', '\0' };

/* buffer sizes */
static const size_t _buffer_sizes[] = { 0, 512, 2048, 8192, 16384, };
static const size_t _buffer_sizes_count = sizeof(_buffer_sizes) /
                                          sizeof(_buffer_sizes[0]);

/* error generation function */
static telnet_error_t _error(telnet_t *telnet, unsigned line,
                             const char* func, telnet_error_t err,
                             int fatal, const char *fmt, ...) {
        telnet_event_t ev;
        char buffer[512];
        va_list va;

        /* format informational text */
        va_start(va, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, va);
        va_end(va);

        /* send error event to the user */
        ev.type       = fatal ? TELNET_EV_ERROR : TELNET_EV_WARNING;
        ev.error.file = __FILE__;
        ev.error.func = func;
        ev.error.line = (int) line;
        ev.error.msg  = buffer;
        telnet->eh(telnet, &ev, telnet->ud);

        return err;
}

/* push bytes out */
static void _send(telnet_t *telnet, const char *buffer,
                  size_t size) {
        telnet_event_t ev;

        ev.type        = TELNET_EV_SEND;
        ev.data.buffer = buffer;
        ev.data.size   = size;
        telnet->eh(telnet, &ev, telnet->ud);
}

/* to send bags of unsigned chars */
#define _sendu(t, d, s) _send((t), (const char*)(d), (s))

/*
 * check if we support a particular telopt; if us is non-zero, we
 * check if we (local) supports it, otherwise we check if he (remote)
 * supports it.  return non-zero if supported, zero if not supported.
 */

static __inline__ int _check_telopt(telnet_t *telnet, unsigned char telopt,
                                    int us) {
        int i;

        /* if we have no telopts table, we obviously don't support it */
        if (telnet->telopts == 0)
                return 0;

        /* loop until found or end marker (us and him both 0) */
        for (i = 0; telnet->telopts[i].telopt != -1; ++i) {
                if (telnet->telopts[i].telopt == telopt) {
                        if (us && telnet->telopts[i].us == TELNET_WILL)
                                return 1;
                        else if (!us && telnet->telopts[i].him == TELNET_DO)
                                return 1;
                        else
                                return 0;
                }
        }

        /* not found, so not supported */
        return 0;
}

/* retrieve RFC-1143 option state */
static __inline__ telnet_rfc1143_t _get_rfc1143(telnet_t *telnet,
                                                unsigned char telopt) {
        telnet_rfc1143_t empty;
        unsigned int i;

        /* search for entry */
        for (i = 0; i != telnet->q_cnt; ++i) {
                if (telnet->q[i].telopt == telopt) {
                        return telnet->q[i];
                }
        }

        /* not found, return empty value */
        empty.telopt = telopt;
        empty.state  = 0;
        return empty;
}

/* save RFC-1143 option state */
static __inline__ void _set_rfc1143(telnet_t *telnet, unsigned char telopt,
                                    unsigned char us, unsigned char him) {
        telnet_rfc1143_t *qtmp;
        unsigned int i;

        /* search for entry */
        for (i = 0; i != telnet->q_cnt; ++i) {
                if (telnet->q[i].telopt == telopt) {
                        telnet->q[i].state = (unsigned char) Q_MAKE(us,him);
                        if (telopt != TELNET_TELOPT_BINARY)
                                return;
                        telnet->flags &=
                            (unsigned char)~(TELNET_FLAG_TRANSMIT_BINARY |
                                             TELNET_FLAG_RECEIVE_BINARY);
                        if (us == Q_YES)
                                telnet->flags |= TELNET_FLAG_TRANSMIT_BINARY;
                        if (him == Q_YES)
                                telnet->flags |= TELNET_FLAG_RECEIVE_BINARY;
                        return;
                }
        }

        /*
         * we're going to need to track state for it, so grow the queue
         * by 4 (four) elements and put the telopt into it; bail on allocation
         * error.  we go by four because it seems like a reasonable guess as
         * to the number of enabled options for most simple code, and it
         * allows for an acceptable number of reallocations for complex code.
         */

#define QUANTUM 4
    /* Did we reach the end of the table? */
       if (i >= telnet->q_size) {
               /* Expand the size */
               if ((qtmp = (telnet_rfc1143_t *)realloc(telnet->q,
                       sizeof(telnet_rfc1143_t) * (telnet->q_size + QUANTUM))) == 0) {
                       _error(telnet, __LINE__, __func__, TELNET_ENOMEM, 0,
                                       "realloc() failed: %s", xstrerror_l(errno));
                       return;
               }
               memset(&qtmp[telnet->q_size], 0, sizeof(telnet_rfc1143_t) * QUANTUM);
               telnet->q       = qtmp;
               telnet->q_size += QUANTUM;
        }
       /* Add entry to end of table */
       telnet->q[telnet->q_cnt].telopt = telopt;
       telnet->q[telnet->q_cnt].state  = (unsigned char) Q_MAKE(us, him);
       telnet->q_cnt ++;
}

/* send negotiation bytes */
static __inline__ void _send_negotiate(telnet_t *telnet, unsigned char cmd,
                                       unsigned char telopt) {
        unsigned char bytes[3];
        bytes[0] = TELNET_IAC;
        bytes[1] = cmd;
        bytes[2] = telopt;
        _sendu(telnet, bytes, 3);
}

/* negotiation handling magic for RFC-1143 */
static void _negotiate(telnet_t *telnet, unsigned char telopt) {
        telnet_event_t ev;
        telnet_rfc1143_t q;

        /* in PROXY mode, just pass it thru and do nothing */
        if (telnet->flags & TELNET_FLAG_PROXY) {
                switch ((int)telnet->state) {
                case TELNET_STATE_WILL:
                        NEGOTIATE_EVENT(telnet, TELNET_EV_WILL, telopt);
                        break;
                case TELNET_STATE_WONT:
                        NEGOTIATE_EVENT(telnet, TELNET_EV_WONT, telopt);
                        break;
                case TELNET_STATE_DO:
                        NEGOTIATE_EVENT(telnet, TELNET_EV_DO, telopt);
                        break;
                case TELNET_STATE_DONT:
                        NEGOTIATE_EVENT(telnet, TELNET_EV_DONT, telopt);
                        break;
                }
                return;
        }

        /* lookup the current state of the option */
        q = _get_rfc1143(telnet, telopt);

        /* start processing... */
        switch ((int)telnet->state) {
        /* request to enable option on remote end or confirm DO */
        case TELNET_STATE_WILL:
                switch (Q_HIM(q)) {
                case Q_NO:
                        if (_check_telopt(telnet, telopt, 0)) {
                                _set_rfc1143(telnet, telopt, Q_US(q), Q_YES);
                                _send_negotiate(telnet, TELNET_DO, telopt);
                                NEGOTIATE_EVENT(telnet, TELNET_EV_WILL, telopt);
                        } else
                                _send_negotiate(telnet, TELNET_DONT, telopt);
                        break;
                case Q_WANTNO:
                        _set_rfc1143(telnet, telopt, Q_US(q), Q_NO);
                        NEGOTIATE_EVENT(telnet, TELNET_EV_WONT, telopt);
                        _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
                                        "DONT answered by WILL");
                        break;
                case Q_WANTNO_OP:
                        _set_rfc1143(telnet, telopt, Q_US(q), Q_YES);
                        _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
                                        "DONT answered by WILL");
                        break;
                case Q_WANTYES:
                        _set_rfc1143(telnet, telopt, Q_US(q), Q_YES);
                        NEGOTIATE_EVENT(telnet, TELNET_EV_WILL, telopt);
                        break;
                case Q_WANTYES_OP:
                        _set_rfc1143(telnet, telopt, Q_US(q), Q_WANTNO);
                        _send_negotiate(telnet, TELNET_DONT, telopt);
                        NEGOTIATE_EVENT(telnet, TELNET_EV_WILL, telopt);
                        break;
                }
                break;

        /* request to disable option on remote end, confirm DONT, reject DO */
        case TELNET_STATE_WONT:
                switch (Q_HIM(q)) {
                case Q_YES:
                        _set_rfc1143(telnet, telopt, Q_US(q), Q_NO);
                        _send_negotiate(telnet, TELNET_DONT, telopt);
                        NEGOTIATE_EVENT(telnet, TELNET_EV_WONT, telopt);
                        break;
                case Q_WANTNO:
                        _set_rfc1143(telnet, telopt, Q_US(q), Q_NO);
                        NEGOTIATE_EVENT(telnet, TELNET_EV_WONT, telopt);
                        break;
                case Q_WANTNO_OP:
                        _set_rfc1143(telnet, telopt, Q_US(q), Q_WANTYES);
                        _send_negotiate(telnet, TELNET_DO, telopt);
                        NEGOTIATE_EVENT(telnet, TELNET_EV_WONT, telopt);
                        break;
                case Q_WANTYES:
                case Q_WANTYES_OP:
                        _set_rfc1143(telnet, telopt, Q_US(q), Q_NO);
                        break;
                }
                break;

        /* request to enable option on local end or confirm WILL */
        case TELNET_STATE_DO:
                switch (Q_US(q)) {
                case Q_NO:
                        if (_check_telopt(telnet, telopt, 1)) {
                                _set_rfc1143(telnet, telopt, Q_YES, Q_HIM(q));
                                _send_negotiate(telnet, TELNET_WILL, telopt);
                                NEGOTIATE_EVENT(telnet, TELNET_EV_DO, telopt);
                        } else
                                _send_negotiate(telnet, TELNET_WONT, telopt);
                        break;
                case Q_WANTNO:
                        _set_rfc1143(telnet, telopt, Q_NO, Q_HIM(q));
                        NEGOTIATE_EVENT(telnet, TELNET_EV_DONT, telopt);
                        _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
                                        "WONT answered by DO");
                        break;
                case Q_WANTNO_OP:
                        _set_rfc1143(telnet, telopt, Q_YES, Q_HIM(q));
                        _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
                                        "WONT answered by DO");
                        break;
                case Q_WANTYES:
                        _set_rfc1143(telnet, telopt, Q_YES, Q_HIM(q));
                        NEGOTIATE_EVENT(telnet, TELNET_EV_DO, telopt);
                        break;
                case Q_WANTYES_OP:
                        _set_rfc1143(telnet, telopt, Q_WANTNO, Q_HIM(q));
                        _send_negotiate(telnet, TELNET_WONT, telopt);
                        NEGOTIATE_EVENT(telnet, TELNET_EV_DO, telopt);
                        break;
                }
                break;

        /* request to disable option on local end, confirm WONT, reject WILL */
        case TELNET_STATE_DONT:
                switch (Q_US(q)) {
                case Q_YES:
                        _set_rfc1143(telnet, telopt, Q_NO, Q_HIM(q));
                        _send_negotiate(telnet, TELNET_WONT, telopt);
                        NEGOTIATE_EVENT(telnet, TELNET_EV_DONT, telopt);
                        break;
                case Q_WANTNO:
                        _set_rfc1143(telnet, telopt, Q_NO, Q_HIM(q));
                        NEGOTIATE_EVENT(telnet, TELNET_EV_DONT, telopt);
                        break;
                case Q_WANTNO_OP:
                        _set_rfc1143(telnet, telopt, Q_WANTYES, Q_HIM(q));
                        _send_negotiate(telnet, TELNET_WILL, telopt);
                        NEGOTIATE_EVENT(telnet, TELNET_EV_DONT, telopt);
                        break;
                case Q_WANTYES:
                case Q_WANTYES_OP:
                        _set_rfc1143(telnet, telopt, Q_NO, Q_HIM(q));
                        break;
                }
                break;
        }
}

/*
 * process an ENVIRON/NEW-ENVIRON subnegotiation buffer
 *
 * the algorithm and approach used here is kind of a hack,
 * but it reduces the number of memory allocations we have
 * to make.
 *
 * we copy the bytes back into the buffer, starting at the very
 * beginning, which makes it easy to handle the ENVIRON ESC
 * escape mechanism as well as ensure the variable name and
 * value strings are NUL-terminated, all while fitting inside
 * of the original buffer.
 */

static int _environ_telnet(telnet_t *telnet, unsigned char type,
                           char* buffer, size_t size) {
        telnet_event_t ev;
        struct telnet_environ_t *values = 0;
        char *c, *last, *out;
        size_t eindex, count;

        /* if we have no data, just pass it through */
        if (size == 0) {
                return 0;
        }

        /* first byte must be a valid command */
        if ((unsigned)buffer[0] != TELNET_ENVIRON_SEND &&
                        (unsigned)buffer[0] != TELNET_ENVIRON_IS &&
                        (unsigned)buffer[0] != TELNET_ENVIRON_INFO) {
                _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
                                "telopt %ld subneg has invalid command", (long) type);
                return 0;
        }

        /* store ENVIRON command */
        ev.environ.cmd = (unsigned char) buffer[0];

        /* if we have no arguments, send an event with no data end return */
        if (size == 1) {
                /* no list of variables given */
                ev.environ.values = 0;
                ev.environ.size   = 0;

                /* invoke event with our arguments */
                ev.type = TELNET_EV_ENVIRON;
                telnet->eh(telnet, &ev, telnet->ud);

                return 0;
        }

        /* very second byte must be VAR or USERVAR, if present */
        if ((unsigned)buffer[1] != TELNET_ENVIRON_VAR &&
                        (unsigned)buffer[1] != TELNET_ENVIRON_USERVAR) {
                _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
                                "telopt %d subneg missing variable type", type);
                return 0;
        }

        /* ensure last byte is not an escape byte (makes parsing later easier) */
        if ((unsigned)buffer[size - 1] == TELNET_ENVIRON_ESC) {
                _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
                                "telopt %d subneg ends with ESC", type);
                return 0;
        }

        /* count arguments; each valid entry starts with VAR or USERVAR */
        count = 0;
        for (c = buffer + 1; c < buffer + size; ++c) {
                if (*c == TELNET_ENVIRON_VAR || *c == TELNET_ENVIRON_USERVAR) {
                        ++count;
                } else if (*c == TELNET_ENVIRON_ESC) {
                        /* skip the next byte */
                        ++c;
                }
        }

        /* allocate argument array, bail on error */
        if ((values = (struct telnet_environ_t *)calloc(count,
                        sizeof(struct telnet_environ_t))) == 0) {
                _error(telnet, __LINE__, __func__, TELNET_ENOMEM, 0,
                                "calloc() failed: %s", xstrerror_l(errno));
                return 0;
        }

        /* parse argument array strings */
        out = buffer;
        c = buffer + 1;
        for (eindex = 0; eindex != count; ++eindex) {
                /* remember the variable type (will be VAR or USERVAR) */
                values[eindex].type = (unsigned char) (*c++);

                /* scan until we find an end-marker, and buffer up unescaped
                 * bytes into our buffer */
                last = out;
                while (c < buffer + size) {
                        /* stop at the next variable or at the value */
                        if ((unsigned)*c == TELNET_ENVIRON_VAR ||
                                        (unsigned)*c == TELNET_ENVIRON_VALUE ||
                                        (unsigned)*c == TELNET_ENVIRON_USERVAR) {
                                break;
                        }

                        /* buffer next byte (taking into account ESC) */
                        if (*c == TELNET_ENVIRON_ESC) {
                                ++c;
                        }

                        *out++ = *c++;
                }
                *out++ = '\0';

                /* store the variable name we have just received */
                values[eindex].var   = last;
                values[eindex].value = "";

                /* if we got a value, find the next end marker and
                 * store the value; otherwise, store empty string */
                if (c < buffer + size && *c == TELNET_ENVIRON_VALUE) {
                        ++c;
                        last = out;
                        while (c < buffer + size) {
                                /* stop when we find the start of the next variable */
                                if ((unsigned)*c == TELNET_ENVIRON_VAR ||
                                                (unsigned)*c == TELNET_ENVIRON_USERVAR) {
                                        break;
                                }

                                /* buffer next byte (taking into account ESC) */
                                if (*c == TELNET_ENVIRON_ESC) {
                                        ++c;
                                }

                                *out++ = *c++;
                        }
                        *out++ = '\0';

                        /* store the variable value */
                        values[eindex].value = last;
                }
        }

        /* pass values array and count to event */
        ev.environ.values = values;
        ev.environ.size   = count;

        /* invoke event with our arguments */
        ev.type = TELNET_EV_ENVIRON;
        telnet->eh(telnet, &ev, telnet->ud);

        /* clean up */
        FREE(values);
        return 0;
}

/* parse TERMINAL-TYPE command subnegotiation buffers */
static int _ttype_telnet(telnet_t *telnet, const char* buffer, size_t size) {
        telnet_event_t ev;

        /* make sure request is not empty */
        if (size == 0) {
                _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
                                "incomplete TERMINAL-TYPE request");
                return 0;
        }

        /* make sure request has valid command type */
        if (buffer[0] != TELNET_TTYPE_IS &&
                        buffer[0] != TELNET_TTYPE_SEND) {
                _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
                                "TERMINAL-TYPE request has invalid type");
                return 0;
        }

        /* send proper event */
        if (buffer[0] == TELNET_TTYPE_IS) {
                char *name;

                /* allocate space for name */
                if ((name = (char *)malloc(size)) == 0) {
                        _error(telnet, __LINE__, __func__, TELNET_ENOMEM, 0,
                                        "malloc() failed: %s", xstrerror_l(errno));
                        return 0;
                }
                memcpy(name, buffer + 1, size - 1);
                name[size - 1] = '\0';

                ev.type       = TELNET_EV_TTYPE;
                ev.ttype.cmd  = TELNET_TTYPE_IS;
                ev.ttype.name = name;
                telnet->eh(telnet, &ev, telnet->ud);

                /* clean up */
                FREE(name);
        } else {
                ev.type       = TELNET_EV_TTYPE;
                ev.ttype.cmd  = TELNET_TTYPE_SEND;
                ev.ttype.name = 0;
                telnet->eh(telnet, &ev, telnet->ud);
        }

        return 0;
}

/*
 * process a subnegotiation buffer; return non-zero if the current buffer
 * must be aborted and reprocessed.
 */

static int _subnegotiate(telnet_t *telnet) {
        telnet_event_t ev;

        /* standard subnegotiation event */
        ev.type       = TELNET_EV_SUBNEGOTIATION;
        ev.sub.telopt = telnet->sb_telopt;
        ev.sub.buffer = telnet->buffer;
        ev.sub.size   = telnet->buffer_pos;
        telnet->eh(telnet, &ev, telnet->ud);

        switch (telnet->sb_telopt) {

        /* specially handled subnegotiation telopt types */
        case TELNET_TELOPT_TTYPE:
                return _ttype_telnet(telnet, telnet->buffer, telnet->buffer_pos);
        case TELNET_TELOPT_ENVIRON:
        case TELNET_TELOPT_NEW_ENVIRON:
                return _environ_telnet(telnet, telnet->sb_telopt, telnet->buffer,
                                telnet->buffer_pos);
        default:
                return 0;
        }
}

/* initialize a telnet state tracker */
telnet_t *telnet_init(const telnet_telopt_t *telopts,
                      telnet_event_handler_t eh, unsigned char flags, void *user_data) {
        /* allocate structure */
        struct telnet_t *telnet = (telnet_t*)calloc(1, sizeof(telnet_t));
        if (telnet == 0)
                return 0;

        /* initialize data */
        telnet->ud      = user_data;
        telnet->telopts = telopts;
        telnet->eh      = eh;
        telnet->flags   = flags;

        return telnet;
}

/* free up any memory allocated by a state tracker */
void telnet_free(telnet_t *telnet) {
        /* free sub-request buffer */
        if (telnet->buffer != 0) {
                FREE(telnet->buffer);
                telnet->buffer      = 0; //-V1048
                telnet->buffer_size = 0;
                telnet->buffer_pos  = 0;
        }

        /* free RFC-1143 queue */
        if (telnet->q) {
                FREE(telnet->q);
                telnet->q      = NULL;
                telnet->q_size = 0;
                telnet->q_cnt  = 0;
        }

        /* free the telnet structure itself */
        free(telnet); /* X-LINTED: FREE */
}

/* push a byte into the telnet buffer */
static telnet_error_t _buffer_byte(telnet_t *telnet,
                                   unsigned char byte) {
        char *new_buffer;

        /* check if we're out of room */
        if (telnet->buffer_pos == telnet->buffer_size) {
                size_t i;
                /* find the next buffer size */
                for (i = 0; i != _buffer_sizes_count; ++i) {
                        if (_buffer_sizes[i] == telnet->buffer_size) {
                                break;
                        }
                }

                /* overflow -- can't grow any more */
                if (i >= _buffer_sizes_count - 1) {
                        _error(telnet, __LINE__, __func__, TELNET_EOVERFLOW, 0,
                                        "subnegotiation buffer size limit reached");
                        return TELNET_EOVERFLOW;
                }

                /* (re)allocate buffer */
                new_buffer = (char *)realloc(telnet->buffer, _buffer_sizes[i + 1]);
                if (new_buffer == 0) {
                        _error(telnet, __LINE__, __func__, TELNET_ENOMEM, 0,
                                        "realloc() failed");
                        return TELNET_ENOMEM;
                }

                telnet->buffer = new_buffer;
                telnet->buffer_size = _buffer_sizes[i + 1];
        }

        /* push the byte, all set */
        telnet->buffer[telnet->buffer_pos++] = (char) byte;
        return TELNET_EOK;
}

static void _process(telnet_t *telnet, const char *buffer, size_t size) {
        telnet_event_t ev;
        unsigned char byte;
        size_t i, start;
        for (i = start = 0; i != size; ++i) {
                byte = (unsigned char) buffer[i];
                switch (telnet->state) {
                /* regular data */
                case TELNET_STATE_DATA:
                        /* on an IAC byte, pass through all pending bytes and
                         * switch states */
                        if (byte == TELNET_IAC) {
                                if (i != start) {
                                        ev.type        = TELNET_EV_DATA;
                                        ev.data.buffer = buffer + start;
                                        ev.data.size   = i - start;
                                        telnet->eh(telnet, &ev, telnet->ud);
                                }
                                telnet->state = TELNET_STATE_IAC;
                        } else if (byte == '\r' &&
                                            (telnet->flags & TELNET_FLAG_NVT_EOL) &&
                                           !(telnet->flags & TELNET_FLAG_RECEIVE_BINARY)) {
                                if (i != start) {
                                        ev.type        = TELNET_EV_DATA;
                                        ev.data.buffer = buffer + start;
                                        ev.data.size   = i - start;
                                        telnet->eh(telnet, &ev, telnet->ud);
                                }
                                telnet->state = TELNET_STATE_EOL;
                        }
                        break;

                /* NVT EOL to be translated */
                case TELNET_STATE_EOL:
                        if (byte != '\n') {
                                byte           = '\r';
                                ev.type        = TELNET_EV_DATA;
                                ev.data.buffer = (char*)&byte;
                                ev.data.size   = 1;
                                telnet->eh(telnet, &ev, telnet->ud);
                                byte           = (unsigned char) buffer[i];
                        }
                        /* any byte following '\r' other than '\n' or '\0' is invalid, */
                        /* so pass both \r and the byte */
                        start = i;
                        if (byte == '\0')
                                ++start;
                        /* state update */
                        telnet->state = TELNET_STATE_DATA;
                        break;

                /* IAC command */
                case TELNET_STATE_IAC:
                        switch (byte) {
                        /* subnegotiation */
                        case TELNET_SB:
                                telnet->state = TELNET_STATE_SB;
                                break;
                        /* negotiation commands */
                        case TELNET_WILL:
                                telnet->state = TELNET_STATE_WILL;
                                break;
                        case TELNET_WONT:
                                telnet->state = TELNET_STATE_WONT;
                                break;
                        case TELNET_DO:
                                telnet->state = TELNET_STATE_DO;
                                break;
                        case TELNET_DONT:
                                telnet->state = TELNET_STATE_DONT;
                                break;
                        /* IAC escaping */
                        case TELNET_IAC:
                                /* event */
                                ev.type        = TELNET_EV_DATA;
                                ev.data.buffer = (char*)&byte;
                                ev.data.size   = 1;
                                telnet->eh(telnet, &ev, telnet->ud);

                                /* state update */
                                start         = i + 1;
                                telnet->state = TELNET_STATE_DATA;
                                break;
                        /* some other command */
                        default:
                                /* event */
                                ev.type       = TELNET_EV_IAC;
                                ev.iac.cmd    = byte;
                                telnet->eh(telnet, &ev, telnet->ud);

                                /* state update */
                                start         = i + 1;
                                telnet->state = TELNET_STATE_DATA;
                        }
                        break;

                /* negotiation commands */
                case TELNET_STATE_WILL:
                case TELNET_STATE_WONT:
                case TELNET_STATE_DO:
                case TELNET_STATE_DONT:
                        _negotiate(telnet, byte);
                        start         = i + 1;
                        telnet->state = TELNET_STATE_DATA;
                        break;

                /* subnegotiation -- determine subnegotiation telopt */
                case TELNET_STATE_SB:
                        telnet->sb_telopt  = byte;
                        telnet->buffer_pos = 0;
                        telnet->state      = TELNET_STATE_SB_DATA;
                        break;

                /* subnegotiation -- buffer bytes until end request */
                case TELNET_STATE_SB_DATA:
                        /* IAC command in subnegotiation -- either IAC SE or IAC IAC */
                        if (byte == TELNET_IAC) {
                                telnet->state = TELNET_STATE_SB_DATA_IAC;
                        } else if (_buffer_byte(telnet, byte) != TELNET_EOK) {
                                start = i + 1;
                                telnet->state = TELNET_STATE_DATA;
                        }
                        break;

                /* IAC escaping inside a subnegotiation */
                case TELNET_STATE_SB_DATA_IAC:
                        switch (byte) {
                        /* end subnegotiation */
                        case TELNET_SE:
                                /* return to default state */
                                start = i + 1;
                                telnet->state = TELNET_STATE_DATA;

                                /* process subnegotiation */
                                if (_subnegotiate(telnet) != 0) {
                                        telnet_recv(telnet, &buffer[start], size - start);
                                        return;
                                }
                                break;
                        /* escaped IAC byte */
                        case TELNET_IAC:
                                /* push IAC into buffer */
                                if (_buffer_byte(telnet, TELNET_IAC) !=
                                                TELNET_EOK) {
                                        start = i + 1;
                                        telnet->state = TELNET_STATE_DATA;
                                } else {
                                        telnet->state = TELNET_STATE_SB_DATA;
                                }
                                break;
                        /*
                         * Something else -- protocol error.  attempt to process
                         * content in subnegotiation buffer, then evaluate the
                         * given command as an IAC code.
                         */

                        default:
                                _error(telnet, __LINE__, __func__, TELNET_EPROTOCOL, 0,
                                                "unexpected byte after IAC inside SB: %d",
                                                byte);

                                /* enter IAC state */
                                start = i + 1;
                                telnet->state = TELNET_STATE_IAC;

                                /*
                                 * Process subnegotiation; see comment in
                                 * TELNET_STATE_SB_DATA_IAC about invoking telnet_recv()
                                 */

                                if (_subnegotiate(telnet) != 0) {
                                        telnet_recv(telnet, &buffer[start], size - start);
                                        return;
                                } else {

                                        /*
                                         * Recursive call to get the current input byte processed
                                         * as a regular IAC command.  we could use a goto, but
                                         * that would be gross.
                                         */

                                        _process(telnet, (char *)&byte, 1);
                                }
                                break;
                        }
                        break;
                }
        }

        /* pass through any remaining bytes */
        if (telnet->state == TELNET_STATE_DATA && i != start) {
                ev.type        = TELNET_EV_DATA;
                ev.data.buffer = buffer + start;
                ev.data.size   = i - start;
                telnet->eh(telnet, &ev, telnet->ud);
        }
}

/* push a bytes into the state tracker */
void telnet_recv(telnet_t *telnet, const char *buffer,
                 size_t size) {
        _process(telnet, buffer, size);
}

/* send an iac command */
void telnet_iac(telnet_t *telnet, unsigned char cmd) {
        unsigned char bytes[2];
        bytes[0] = TELNET_IAC;
        bytes[1] = cmd;
        _sendu(telnet, bytes, 2);
}

/* send negotiation */
void telnet_negotiate(telnet_t *telnet, unsigned char cmd,
                      unsigned char telopt) {
        telnet_rfc1143_t q;

        /* if we're in proxy mode, just send it now */
        if (telnet->flags & TELNET_FLAG_PROXY) {
                unsigned char bytes[3];
                bytes[0] = TELNET_IAC;
                bytes[1] = cmd;
                bytes[2] = telopt;
                _sendu(telnet, bytes, 3);
                return;
        }

        /* get current option states */
        q = _get_rfc1143(telnet, telopt);

        switch (cmd) {
        /* advertise willingness to support an option */
        case TELNET_WILL:
                switch (Q_US(q)) {
                case Q_NO:
                        _set_rfc1143(telnet, telopt, Q_WANTYES, Q_HIM(q));
                        _send_negotiate(telnet, TELNET_WILL, telopt);
                        break;
                case Q_WANTNO:
                        _set_rfc1143(telnet, telopt, Q_WANTNO_OP, Q_HIM(q));
                        break;
                case Q_WANTYES_OP:
                        _set_rfc1143(telnet, telopt, Q_WANTYES, Q_HIM(q));
                        break;
                }
                break;

        /* force turn-off of locally enabled option */
        case TELNET_WONT:
                switch (Q_US(q)) {
                case Q_YES:
                        _set_rfc1143(telnet, telopt, Q_WANTNO, Q_HIM(q));
                        _send_negotiate(telnet, TELNET_WONT, telopt);
                        break;
                case Q_WANTYES:
                        _set_rfc1143(telnet, telopt, Q_WANTYES_OP, Q_HIM(q));
                        break;
                case Q_WANTNO_OP:
                        _set_rfc1143(telnet, telopt, Q_WANTNO, Q_HIM(q));
                        break;
                }
                break;

        /* ask remote end to enable an option */
        case TELNET_DO:
                switch (Q_HIM(q)) {
                case Q_NO:
                        _set_rfc1143(telnet, telopt, Q_US(q), Q_WANTYES);
                        _send_negotiate(telnet, TELNET_DO, telopt);
                        break;
                case Q_WANTNO:
                        _set_rfc1143(telnet, telopt, Q_US(q), Q_WANTNO_OP);
                        break;
                case Q_WANTYES_OP:
                        _set_rfc1143(telnet, telopt, Q_US(q), Q_WANTYES);
                        break;
                }
                break;

        /* demand remote end disable an option */
        case TELNET_DONT:
                switch (Q_HIM(q)) {
                case Q_YES:
                        _set_rfc1143(telnet, telopt, Q_US(q), Q_WANTNO);
                        _send_negotiate(telnet, TELNET_DONT, telopt);
                        break;
                case Q_WANTYES:
                        _set_rfc1143(telnet, telopt, Q_US(q), Q_WANTYES_OP);
                        break;
                case Q_WANTNO_OP:
                        _set_rfc1143(telnet, telopt, Q_US(q), Q_WANTNO);
                        break;
                }
                break;
        }
}

/* send non-command data (escapes IAC bytes) */
void telnet_send(telnet_t *telnet, const char *buffer,
                 size_t size) {
        size_t i, l;

        for (l = i = 0; i != size; ++i) {
                /* dump prior portion of text, send escaped bytes */
                if (buffer[i] == (char)TELNET_IAC) {
                        /* dump prior text if any */
                        if (i != l) {
                                _send(telnet, buffer + l, i - l);
                        }
                        l = i + 1;

                        /* send escape */
                        telnet_iac(telnet, TELNET_IAC);
                }
        }

        /* send whatever portion of buffer is left */
        if (i != l) {
                _send(telnet, buffer + l, i - l);
        }
}

/* send non-command text (escapes IAC bytes and does NVT translation) */
void telnet_send_text(telnet_t *telnet, const char *buffer,
                      size_t size) {
        size_t i, l;

        for (l = i = 0; i != size; ++i) {
                /* dump prior portion of text, send escaped bytes */
                if (buffer[i] == (char)TELNET_IAC) {
                        /* dump prior text if any */
                        if (i != l) {
                                _send(telnet, buffer + l, i - l);
                        }
                        l = i + 1;

                        /* send escape */
                        telnet_iac(telnet, TELNET_IAC);
                }
                /* special characters if not in BINARY mode */
                else if (!(telnet->flags & TELNET_FLAG_TRANSMIT_BINARY) &&
                                 (buffer[i] == '\r' || buffer[i] == '\n')) {
                        /* dump prior portion of text */
                        if (i != l) {
                                _send(telnet, buffer + l, i - l);
                        }
                        l = i + 1;

                        /* automatic translation of \r -> CRNUL */
                        if (buffer[i] == '\r') {
                                _send(telnet, CRNUL, 2);
                        }
                        /* automatic translation of \n -> CRLF */
                        else {
                                _send(telnet, CRLF, 2);
                        }
                }
        }

        /* send whatever portion of buffer is left */
        if (i != l) {
                _send(telnet, buffer + l, i - l);
        }
}

/* send subnegotiation header */
void telnet_begin_sb(telnet_t *telnet, unsigned char telopt) {
        unsigned char sb[3];
        sb[0] = TELNET_IAC;
        sb[1] = TELNET_SB;
        sb[2] = telopt;
        _sendu(telnet, sb, 3);
}

/* send formatted data with \r and \n translation in addition to IAC IAC */
int telnet_vprintf(telnet_t *telnet, const char *fmt, va_list va) {
        char buffer[1024];
        char *output = buffer;
        int rs, i, l;

        /* format */
        va_list va2;
        va_copy(va2, va);
        rs = vsnprintf(buffer, sizeof(buffer), fmt, va);
        if ((unsigned long) rs >= sizeof(buffer)) {
                output = (char*)malloc((unsigned long) ((unsigned long)rs + 1L));
                if (output == 0) {
                        _error(telnet, __LINE__, __func__, TELNET_ENOMEM, 0,
                                        "malloc() failed: %s", xstrerror_l(errno));
                        va_end(va2);
                        return -1;
                }
                rs = vsnprintf(output, rs + 1, fmt, va2);
        }
        va_end(va2);
        va_end(va);

        /* send */
        for (l = i = 0; i != rs; ++i) {
                /* special characters */
                if (output[i] == (char)TELNET_IAC || output[i] == '\r' ||
                                output[i] == '\n') {
                        /* dump prior portion of text */
                        if (i != l)
                                _send(telnet, output + l, (size_t) (i - l));
                        l = i + 1;

                        /* IAC -> IAC IAC */
                        if (output[i] == (char)TELNET_IAC)
                                telnet_iac(telnet, TELNET_IAC);
                        /* automatic translation of \r -> CRNUL */
                        else if (output[i] == '\r')
                                _send(telnet, CRNUL, 2);
                        /* automatic translation of \n -> CRLF */
                        else if (output[i] == '\n')
                                _send(telnet, CRLF, 2);
                }
        }

        /* send whatever portion of output is left */
        if (i != l) {
                _send(telnet, output + l, (size_t) (i - l));
        }

        /* free allocated memory, if any */
        if (output != buffer) {
                FREE(output);
        }

        return rs;
}

/* see telnet_vprintf */
int telnet_printf(telnet_t *telnet, const char *fmt, ...) {
        va_list va;
        int rs;

        va_start(va, fmt);
        rs = telnet_vprintf(telnet, fmt, va);
        va_end(va);

        return rs;
}

/* send formatted data through telnet_send */
int telnet_raw_vprintf(telnet_t *telnet, const char *fmt, va_list va) {
        char buffer[1024];
        char *output = buffer;
        int rs;

        /* format; allocate more space if necessary */
        va_list va2;
        va_copy(va2, va);
        rs = vsnprintf(buffer, sizeof(buffer), fmt, va);
        if ((unsigned long) rs >= sizeof(buffer)) {
                output = (char*)malloc((unsigned long) rs + 1);
                if (output == 0) {
                        _error(telnet, __LINE__, __func__, TELNET_ENOMEM, 0,
                                        "malloc() failed: %s", xstrerror_l(errno));
                        va_end(va2);
                        return -1;
                }
                rs = vsnprintf(output, (int)((unsigned int) rs + 1), fmt, va2);
        }
        va_end(va2);
        va_end(va);

        /* send out the formatted data */
        telnet_send(telnet, output, (size_t) rs);

        /* release allocated memory, if any */
        if (output != buffer) {
                FREE(output);
        }

        return rs;
}

/* see telnet_raw_vprintf */
int telnet_raw_printf(telnet_t *telnet, const char *fmt, ...) {
        va_list va;
        int rs;

        va_start(va, fmt);
        rs = telnet_raw_vprintf(telnet, fmt, va);
        va_end(va);

        return rs;
}
