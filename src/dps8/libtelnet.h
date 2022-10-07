/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: CC-PDDC
 * SPDX-FileCopyrightText: Public domain
 * scspell-id: 8a194784-f62f-11ec-85b5-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * libTELNET - TELNET protocol handling library
 *
 * SUMMARY:
 *
 * libTELNET is a library for handling the TELNET protocol.  It includes
 * routines for parsing incoming data from a remote peer as well as
 * formatting data to send to the remote peer.
 *
 * libTELNET uses a callback-oriented API, allowing application-specific
 * handling of various events.  The callback system is also used for
 * buffering outgoing protocol data, allowing the application to maintain
 * control over the actual socket connection.
 *
 * Features supported include the full TELNET protocol, Q-method option
 * negotiation, and NEW-ENVIRON.
 *
 * ---------------------------------------------------------------------------
 *
 * CONFORMS TO:
 *
 * RFC-854   -  https://www.faqs.org/rfcs/rfc854.html
 * RFC-855   -  https://www.faqs.org/rfcs/rfc855.html
 * RFC-1091  -  https://www.faqs.org/rfcs/rfc1091.html
 * RFC-1143  -  https://www.faqs.org/rfcs/rfc1143.html
 * RFC-1408  -  https://www.faqs.org/rfcs/rfc1408.html
 * RFC-1572  -  https://www.faqs.org/rfcs/rfc1572.html
 *
 * ---------------------------------------------------------------------------
 *
 * LICENSE:
 *
 * The author or authors of this code dedicate any and all copyright
 * interest in this code to the public domain. We make this dedication
 * for the benefit of the public at large and to the detriment of our
 * heirs and successors. We intend this dedication to be an overt act
 * of relinquishment in perpetuity of all present and future rights to
 * this code under copyright law.
 *
 * ---------------------------------------------------------------------------
 *
 * Sean Middleditch <sean@sourcemud.org>
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

#if !defined(LIBTELNET_INCLUDE)
# define LIBTELNET_INCLUDE 1

/* local definitions */
# include "dps8.h"

/* standard C headers necessary for the libTELNET API */
# include <stdarg.h>
# include <stddef.h>

/* printf type checking feature in GCC and some other compilers */
# if defined(__GNUC__) && !defined(USING_DPSPRINTF)
#  define TELNET_GNU_PRINTF(f,a) __attribute__((format(printf, f, a))) /* internal helper */
#  define TELNET_GNU_SENTINEL __attribute__((sentinel)) /* internal helper */
# else
#  define TELNET_GNU_PRINTF(f,a) /* internal helper */
#  define TELNET_GNU_SENTINEL /* internal helper */
# endif

/* Disable environ macro for Visual C++ 2015. */
# undef environ

/* Telnet state tracker object type. */
typedef struct telnet_t telnet_t;

/* Telnet event object type. */
typedef union telnet_event_t telnet_event_t;

/* Telnet option table element type. */
typedef struct telnet_telopt_t telnet_telopt_t;

/* Telnet commands and special values. */
# define TELNET_IAC   255
# define TELNET_DONT  254
# define TELNET_DO    253
# define TELNET_WONT  252
# define TELNET_WILL  251
# define TELNET_SB    250
# define TELNET_GA    249
# define TELNET_EL    248
# define TELNET_EC    247
# define TELNET_AYT   246
# define TELNET_AO    245
# define TELNET_IP    244
# define TELNET_BREAK 243
# define TELNET_DM    242
# define TELNET_NOP   241
# define TELNET_SE    240
# define TELNET_EOR   239
# define TELNET_ABORT 238
# define TELNET_SUSP  237
# define TELNET_EOF   236

/* Telnet option values. */
# define TELNET_TELOPT_BINARY          0
# define TELNET_TELOPT_ECHO            1
# define TELNET_TELOPT_RCP             2
# define TELNET_TELOPT_SGA             3
# define TELNET_TELOPT_NAMS            4
# define TELNET_TELOPT_STATUS          5
# define TELNET_TELOPT_TM              6
# define TELNET_TELOPT_RCTE            7
# define TELNET_TELOPT_NAOL            8
# define TELNET_TELOPT_NAOP            9
# define TELNET_TELOPT_NAOCRD         10
# define TELNET_TELOPT_NAOHTS         11
# define TELNET_TELOPT_NAOHTD         12
# define TELNET_TELOPT_NAOFFD         13
# define TELNET_TELOPT_NAOVTS         14
# define TELNET_TELOPT_NAOVTD         15
# define TELNET_TELOPT_NAOLFD         16
# define TELNET_TELOPT_XASCII         17
# define TELNET_TELOPT_LOGOUT         18
# define TELNET_TELOPT_BM             19
# define TELNET_TELOPT_DET            20
# define TELNET_TELOPT_SUPDUP         21
# define TELNET_TELOPT_SUPDUPOUTPUT   22
# define TELNET_TELOPT_SNDLOC         23
# define TELNET_TELOPT_TTYPE          24
# define TELNET_TELOPT_EOR            25
# define TELNET_TELOPT_TUID           26
# define TELNET_TELOPT_OUTMRK         27
# define TELNET_TELOPT_TTYLOC         28
# define TELNET_TELOPT_3270REGIME     29
# define TELNET_TELOPT_X3PAD          30
# define TELNET_TELOPT_NAWS           31
# define TELNET_TELOPT_TSPEED         32
# define TELNET_TELOPT_LFLOW          33
# define TELNET_TELOPT_LINEMODE       34
# define TELNET_TELOPT_XDISPLOC       35
# define TELNET_TELOPT_ENVIRON        36
# define TELNET_TELOPT_AUTHENTICATION 37
# define TELNET_TELOPT_ENCRYPT        38
# define TELNET_TELOPT_NEW_ENVIRON    39
# define TELNET_TELOPT_EXOPL         255

/* Protocol codes for TERMINAL-TYPE commands. */
# define TELNET_TTYPE_IS   0
# define TELNET_TTYPE_SEND 1

/* Protocol codes for NEW-ENVIRON/ENVIRON commands. */
# define TELNET_ENVIRON_IS      0
# define TELNET_ENVIRON_SEND    1
# define TELNET_ENVIRON_INFO    2
# define TELNET_ENVIRON_VAR     0
# define TELNET_ENVIRON_VALUE   1
# define TELNET_ENVIRON_ESC     2
# define TELNET_ENVIRON_USERVAR 3

/* Telnet state tracker flags. */

/* Control behavior of telnet state tracker. */
# define TELNET_FLAG_PROXY   (1<<0)
# define TELNET_FLAG_NVT_EOL (1<<1)

/* Internal-only bits in option flags */
# define TELNET_FLAG_TRANSMIT_BINARY (1<<5)
# define TELNET_FLAG_RECEIVE_BINARY  (1<<6)
# define TELNET_PFLAG_DEFLATE        (1<<7)

/*
 * error codes
 */
enum telnet_error_t {
        TELNET_EOK = 0,   /* no error                          */
        TELNET_EBADVAL,   /* invalid parameter, or API misuse  */
        TELNET_ENOMEM,    /* memory allocation failure         */
        TELNET_EOVERFLOW, /* data exceeds buffer size          */
        TELNET_EPROTOCOL, /* invalid sequence of special bytes */
};
typedef enum telnet_error_t telnet_error_t; /* Error code type. */

/*
 * event codes
 */
enum telnet_event_type_t {
        TELNET_EV_DATA = 0,        /* raw text data has been received    */
        TELNET_EV_SEND,            /* data needs to be sent to the peer  */
        TELNET_EV_IAC,             /* generic IAC code received          */
        TELNET_EV_WILL,            /* WILL option negotiation received   */
        TELNET_EV_WONT,            /* WONT option negotiation received   */
        TELNET_EV_DO,              /* DO option negotiation received     */
        TELNET_EV_DONT,            /* DONT option negotiation received   */
        TELNET_EV_SUBNEGOTIATION,  /* sub-negotiation data received      */
        TELNET_EV_TTYPE,           /* TTYPE command has been received    */
        TELNET_EV_ENVIRON,         /* ENVIRON command has been received  */
        TELNET_EV_WARNING,         /* recoverable error has occurred     */
        TELNET_EV_ERROR            /* non-recoverable error has occurred */
};
typedef enum telnet_event_type_t telnet_event_type_t; /* Telnet event type */

/*
 * environ command information
 */
struct telnet_environ_t {
        unsigned char type; /* either TELNET_ENVIRON_VAR or TELNET_ENVIRON_USERVAR   */
        char *var;          /* name of the variable being set                        */
        char *value;        /* value of variable being set; empty string if no value */
};

/*
 * event information
 */
union telnet_event_t {
        /*
         * Event type
         *
         * The type field determines which event structure fields have been filled in.
         */
        enum telnet_event_type_t type;

        /*
         * Data event: for DATA and SEND events
         */
        struct data_t {
                enum telnet_event_type_t _type; /* alias for type            */
                const char *buffer;             /* byte buffer               */
                size_t size;                    /* number of bytes in buffer */
        } data;

        /*
         * WARNING and ERROR events
         */
        struct error_t {
                enum telnet_event_type_t _type; /* alias for type                */
                const char *file;               /* file the error occurred in     */
                const char *func;               /* function the error occurred in */
                const char *msg;                /* error message string          */
                int line;                       /* line of file error occurred on */
                telnet_error_t errcode;         /* error code                    */
        } error;

        /*
         * command event: for IAC
         */
        struct iac_t {
                enum telnet_event_type_t _type; /* alias for type          */
                unsigned char cmd;              /* telnet command received */
        } iac;

        /*
         * negotiation event: WILL, WONT, DO, DONT
         */
        struct negotiate_t {
                enum telnet_event_type_t _type; /* alias for type          */
                unsigned char telopt;           /* option being negotiated */
        } neg;

        /*
         * subnegotiation event
         */
        struct subnegotiate_t {
                enum telnet_event_type_t _type; /* alias for type              */
                const char *buffer;             /* data of sub-negotiation     */
                size_t size;                    /* number of bytes in buffer   */
                unsigned char telopt;           /* option code for negotiation */
        } sub;

        /*
         * TTYPE event
         */
        struct ttype_t {
                enum telnet_event_type_t _type; /* alias for type                       */
                unsigned char cmd;              /* TELNET_TTYPE_IS or TELNET_TTYPE_SEND */
                const char* name;               /* terminal type name (IS only)         */
        } ttype;

        /*
         * ENVIRON/NEW-ENVIRON event
         */
        struct environ_t {
                enum telnet_event_type_t _type;        /* alias for type               */
                const struct telnet_environ_t *values; /* array of variable values     */
                size_t size;                           /* number of elements in values */
                unsigned char cmd;                     /* SEND, IS, or INFO            */
        } environ;
};

/*
 * event handler
 *
 * This is the type of function that must be passed to
 * telnet_init() when creating a new telnet object.  The
 * function will be invoked once for every event generated
 * by the libTELNET protocol parser.
 *
 * param telnet    The telnet object that generated the event
 * param event     Event structure with details about the event
 * param user_data User-supplied pointer
 */
typedef void (*telnet_event_handler_t)(telnet_t *telnet,
                telnet_event_t *event, void *user_data);

/*
 * telopt support table element; use telopt of -1 for end marker
 */
struct telnet_telopt_t {
        short telopt;      /* one of the TELOPT codes or -1 */
        unsigned char us;  /* TELNET_WILL or TELNET_WONT    */
        unsigned char him; /* TELNET_DO or TELNET_DONT      */
};

/*
 * state tracker -- private data structure
 */
struct telnet_t;

/*
 * Initialize a telnet state tracker.
 *
 * This function initializes a new state tracker, which is used for all
 * other libTELNET functions.  Each connection must have its own
 * telnet state tracker object.
 *
 * param telopts   Table of TELNET options the application supports.
 * param eh        Event handler function called for every event.
 * param flags     0 or TELNET_FLAG_PROXY.
 * param user_data Optional data pointer that will be passed to eh.
 * return Telnet state tracker object.
 */
extern telnet_t* telnet_init(const telnet_telopt_t *telopts,
                telnet_event_handler_t eh, unsigned char flags, void *user_data);

/*
 * Free up any memory allocated by a state tracker.
 *
 * This function must be called when a telnet state tracker is no
 * longer needed (such as after the connection has been closed) to
 * release any memory resources used by the state tracker.
 *
 * param telnet Telnet state tracker object.
 */
extern void telnet_free(telnet_t *telnet);

/*
 * Push a byte buffer into the state tracker.
 *
 * Passes one or more bytes to the telnet state tracker for
 * protocol parsing.  The byte buffer is most often going to be
 * the buffer that recv() was called for while handling the
 * connection.
 *
 * param telnet Telnet state tracker object.
 * param buffer Pointer to byte buffer.
 * param size   Number of bytes pointed to by buffer.
 */
extern void telnet_recv(telnet_t *telnet, const char *buffer,
                size_t size);

/*
 * Send a telnet command.
 *
 * param telnet Telnet state tracker object.
 * param cmd    Command to send.
 */
extern void telnet_iac(telnet_t *telnet, unsigned char cmd);

/*
 * Send negotiation command.
 *
 * Internally, libTELNET uses RFC1143 option negotiation rules.
 * The negotiation commands sent with this function may be ignored
 * if they are determined to be redundant.
 *
 * param telnet Telnet state tracker object.
 * param cmd    TELNET_WILL, TELNET_WONT, TELNET_DO, or TELNET_DONT.
 * param opt    One of the TELNET_TELOPT_* values.
 */
extern void telnet_negotiate(telnet_t *telnet, unsigned char cmd,
                unsigned char opt);

/*
 * Send non-command data (escapes IAC bytes).
 *
 * param telnet Telnet state tracker object.
 * param buffer Buffer of bytes to send.
 * param size   Number of bytes to send.
 */
extern void telnet_send(telnet_t *telnet,
                const char *buffer, size_t size);

/*
 * Send non-command text (escapes IAC bytes and translates
 * \\r -> CR-NUL and \\n -> CR-LF unless in BINARY mode.
 *
 * param telnet Telnet state tracker object.
 * param buffer Buffer of bytes to send.
 * param size   Number of bytes to send.
 */
extern void telnet_send_text(telnet_t *telnet,
                const char *buffer, size_t size);

/*
 * Begin a sub-negotiation command.
 *
 * Sends IAC SB followed by the telopt code.  All following data sent
 * will be part of the sub-negotiation, until telnet_finish_sb() is
 * called.
 *
 * param telnet Telnet state tracker object.
 * param telopt One of the TELNET_TELOPT_* values.
 */
extern void telnet_begin_sb(telnet_t *telnet,
                unsigned char telopt);

/*
 * Finish a sub-negotiation command.
 *
 * This must be called after a call to telnet_begin_sb() to finish a
 * sub-negotiation command.
 *
 * param telnet Telnet state tracker object.
 */
# define telnet_finish_sb(telnet) telnet_iac((telnet), TELNET_SE)

/*
 * Send formatted data.
 *
 * This function is a wrapper around telnet_send().  It allows using
 * printf-style formatting.
 *
 * Additionally, this function will translate \\r to the CR NUL construct and
 * \\n with CR LF, as well as automatically escaping IAC bytes like
 * telnet_send().
 *
 * param telnet Telnet state tracker object.
 * param fmt    Format string.
 * return Number of bytes sent.
 */
extern int telnet_printf(telnet_t *telnet, const char *fmt, ...)
                TELNET_GNU_PRINTF(2, 3);

/*
 * Send formatted data.
 *
 * See telnet_printf().
 */
extern int telnet_vprintf(telnet_t *telnet, const char *fmt, va_list va);

/*
 * Send formatted data (no newline escaping).
 *
 * This behaves identically to telnet_printf(), except that the \\r and \\n
 * characters are not translated.  The IAC byte is still escaped as normal
 * with telnet_send().
 *
 * param telnet Telnet state tracker object.
 * param fmt    Format string.
 * return Number of bytes sent.
 */
extern int telnet_raw_printf(telnet_t *telnet, const char *fmt, ...)
                TELNET_GNU_PRINTF(2, 3);

/*
 * Send formatted data (no newline escaping).
 *
 * See telnet_raw_printf().
 */
extern int telnet_raw_vprintf(telnet_t *telnet, const char *fmt, va_list va);

#endif /* !defined(LIBTELNET_INCLUDE) */
