/*
 * sim_console.c: simulator console I/O library
 *
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: MIT
 * scspell-id: a2e214e2-f62a-11ec-89cf-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 1993-2014 Robert M. Supnik
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Robert M. Supnik shall not
 * be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Robert M. Supnik.
 *
 * ---------------------------------------------------------------------------
 */

//-V::701

/*
 * This module implements the following routines to support terminal and
 * Remote Console I/O:
 *
 * sim_poll_kbd                 poll for keyboard input
 * sim_putchar                  output character to console
 * sim_set_console              set console parameters
 * sim_show_console             show console parameters
 * sim_set_remote_console       set remote console parameters
 * sim_show_remote_console      show remote console parameters
 * sim_set_cons_buff            set console buffered
 * sim_set_cons_unbuff          set console unbuffered
 * sim_set_cons_log             set console log
 * sim_set_cons_nolog           set console nolog
 * sim_show_cons_buff           show console buffered
 * sim_show_cons_log            show console log
 * sim_cons_get_send            get console send structure address
 * sim_cons_get_expect          get console expect structure address
 * sim_show_cons_send_input     show pending input data
 * sim_show_cons_expect         show expect rules and state
 * sim_ttinit                   called once to get initial terminal state
 * sim_ttrun                    called to put terminal into run state
 * sim_ttcmd                    called to return terminal to command state
 * sim_ttclose                  called once before the simulator exits
 * sim_ttisatty                 called to determine if running interactively
 * sim_os_poll_kbd              poll for keyboard input
 * sim_os_putchar               output character to console
 *
 * The first group is OS-independent; the second group is OS-dependent.
 *
 * The following routines are exposed but deprecated:
 *
 * sim_set_telnet               set console to Telnet port
 * sim_set_notelnet             close console Telnet port
 * sim_show_telnet              show console status
 */

#include "sim_defs.h"
#include "sim_tmxr.h"
#include "sim_timer.h"
#include <ctype.h>
#include <signal.h>
#include <math.h>

#define DBG_CTR 0

#include "../dps8/dps8.h"
#include "../dps8/dps8_sir.h"

#if defined(__HAIKU__)
# define nice(n) ({})
#endif /* if defined(__HAIKU__) */

#if defined(TESTING)
# include "../dps8/dps8_cpu.h"
#endif /* if defined(TESTING) */

#define FREE(p) do  \
  {                 \
    free((p));      \
    (p) = NULL;     \
  } while(0)

/* Forward Declarations of Platform specific routines */

static t_stat sim_os_poll_kbd (void);
static t_stat sim_os_putchar (int32 out);
static t_stat sim_os_ttinit (void);
static t_stat sim_os_ttrun (void);
static t_stat sim_os_ttcmd (void);
static t_stat sim_os_ttclose (void);
static t_bool sim_os_ttisatty (void);

static t_stat sim_set_rem_telnet (int32 flag, CONST char *cptr);
static t_stat sim_set_rem_connections (int32 flag, CONST char *cptr);
static t_stat sim_set_rem_timeout (int32 flag, CONST char *cptr);

/* Deprecated CONSOLE HALT, CONSOLE RESPONSE and CONSOLE DELAY support */
static t_stat sim_set_halt (int32 flag, CONST char *cptr);
static t_stat sim_set_response (int32 flag, CONST char *cptr);
static t_stat sim_set_delay (int32 flag, CONST char *cptr);

#define KMAP_WRU        0
#define KMAP_BRK        1
#define KMAP_DEL        2
#define KMAP_MASK       0377
#define KMAP_NZ         0400

int32 sim_int_char = 005;                               /* interrupt character */
int32 sim_brk_char = 000;                               /* break character */
int32 sim_tt_pchar = 0x00002780;
#if defined (_WIN32)
int32 sim_del_char = '\b';                              /* delete character */
#else
int32 sim_del_char = 0177;
#endif /* if defined (_WIN32) */

static t_stat sim_con_poll_svc (UNIT *uptr);                /* console connection poll routine */
static t_stat sim_con_reset (DEVICE *dptr);                 /* console reset routine */
UNIT sim_con_unit = { UDATA (&sim_con_poll_svc, 0, 0)  };   /* console connection unit */

/* debugging bitmaps */
#define DBG_TRC  TMXR_DBG_TRC                           /* trace routine calls */
#define DBG_XMT  TMXR_DBG_XMT                           /* display Transmitted Data */
#define DBG_RCV  TMXR_DBG_RCV                           /* display Received Data */
#define DBG_RET  TMXR_DBG_RET                           /* display Returned Received Data */
#define DBG_ASY  TMXR_DBG_ASY                           /* asynchronous thread activity */
#define DBG_EXP  0x00000001                             /* Expect match activity */
#define DBG_SND  0x00000002                             /* Send (Inject) data activity */

static DEBTAB sim_con_debug[] = {
  {"TRC", DBG_TRC},
  {"XMT", DBG_XMT},
  {"RCV", DBG_RCV},
  {"RET", DBG_RET},
  {"ASY", DBG_ASY},
  {"EXP", DBG_EXP},
  {"SND", DBG_SND},
  {0}
};

static REG sim_con_reg[] = {
    { ORDATAD (WRU,   sim_int_char, 8,  "interrupt character") },
    { ORDATAD (BRK,   sim_brk_char, 8,  "break character") },
    { ORDATAD (DEL,   sim_del_char, 8,  "delete character ") },
    { ORDATAD (PCHAR, sim_tt_pchar, 32, "printable character mask") },
  { 0 },
};

static MTAB sim_con_mod[] = {
  { 0 },
};

DEVICE sim_con_telnet = {
    "CON-TEL", &sim_con_unit, sim_con_reg,   sim_con_mod,
    1,         0,             0,             0,           0,    0,
    NULL,      NULL,          sim_con_reset, NULL,        NULL, NULL,
    NULL,      DEV_DEBUG,     0,             sim_con_debug};
TMLN sim_con_ldsc = { 0 };                                             /* line descr */
TMXR sim_con_tmxr = { 1, 0, 0, &sim_con_ldsc, NULL, &sim_con_telnet }; /* line mux   */

SEND sim_con_send = {SEND_DEFAULT_DELAY, &sim_con_telnet, DBG_SND};
EXPECT sim_con_expect = {&sim_con_telnet, DBG_EXP};

static t_bool sim_con_console_port = TRUE;

/* Unit service for console connection polling */

static t_stat sim_con_poll_svc (UNIT *uptr)
{
if ((sim_con_tmxr.master  == 0) &&                      /* not Telnet and not WRU polling? */
    (sim_con_ldsc.serport == 0) &&
    /* cppcheck-suppress knownConditionTrueFalse */
    (sim_con_console_port))
    return SCPE_OK;                                     /* done */
if (tmxr_poll_conn (&sim_con_tmxr) >= 0)                /* poll connect */
    sim_con_ldsc.rcve = 1;                              /* rcv enabled */
sim_activate_after(uptr, 1000000);                      /* check again in 1 second */
/* cppcheck-suppress knownConditionTrueFalse */
if (!sim_con_console_port)                              /* WRU poll needed */
    sim_poll_kbd();                                     /* sets global stop_cpu when WRU received */
if (sim_con_ldsc.conn)
    tmxr_send_buffered_data (&sim_con_ldsc);            /* try to flush any buffered data */
return SCPE_OK;
}

static t_stat sim_con_reset (DEVICE *dptr)
{
return sim_con_poll_svc (&dptr->units[0]);              /* establish polling as needed */
}

/* Set/show data structures */

static CTAB set_con_tab[] = {
    { "WRU",         &sim_set_kmap,             KMAP_WRU | KMAP_NZ },
    { "BRK",         &sim_set_kmap,             KMAP_BRK },
    { "DEL",         &sim_set_kmap,             KMAP_DEL |KMAP_NZ },
    { "PCHAR",       &sim_set_pchar,            0 },
    { "SPEED",       &sim_set_cons_speed,       0 },
    { "TELNET",      &sim_set_telnet,           0 },
    { "NOTELNET",    &sim_set_notelnet,         0 },
    { "LOG",         &sim_set_logon,            0 },
    { "NOLOG",       &sim_set_logoff,           0 },
    { "DEBUG",       &sim_set_debon,            0 },
    { "NODEBUG",     &sim_set_deboff,           0 },
#define CMD_WANTSTR     0100000
    { "HALT",        &sim_set_halt,             1 | CMD_WANTSTR },
    { "NOHALT",      &sim_set_halt,             0 },
    { "DELAY",       &sim_set_delay,            0 },
    { "RESPONSE",    &sim_set_response,         1 | CMD_WANTSTR },
    { "NORESPONSE",  &sim_set_response,         0 },
    { NULL,          NULL,                      0 }
    };

static CTAB set_rem_con_tab[] = {
    { "CONNECTIONS", &sim_set_rem_connections,  0 },
    { "TELNET",      &sim_set_rem_telnet,       1 },
    { "NOTELNET",    &sim_set_rem_telnet,       0 },
    { "TIMEOUT",     &sim_set_rem_timeout,      0 },
    { NULL,          NULL,                      0 }
    };

static SHTAB show_con_tab[] = {
    { "WRU",         &sim_show_kmap,            KMAP_WRU },
    { "BRK",         &sim_show_kmap,            KMAP_BRK },
    { "DEL",         &sim_show_kmap,            KMAP_DEL },
    { "SPEED",       &sim_show_cons_speed,      0 },
    { "LOG",         &sim_show_cons_log,        0 },
    { "TELNET",      &sim_show_telnet,          0 },
    { "BUFFERED",    &sim_show_cons_buff,       0 },
    { "EXPECT",      &sim_show_cons_expect,     0 },
    { "HALT",        &sim_show_cons_expect,     0 },
    { "INPUT",       &sim_show_cons_send_input, 0 },
    { "RESPONSE",    &sim_show_cons_send_input, 0 },
    { "DELAY",       &sim_show_cons_expect,     0 },
    { NULL,          NULL,                      0 }
    };

static CTAB set_con_telnet_tab[] = {
    { "LOG",         &sim_set_cons_log,         0 },
    { "NOLOG",       &sim_set_cons_nolog,       0 },
    { "BUFFERED",    &sim_set_cons_buff,        0 },
    { "NOBUFFERED",  &sim_set_cons_unbuff,      0 },
    { "UNBUFFERED",  &sim_set_cons_unbuff,      0 },
    { NULL,          NULL,                      0 }
    };

static int32 *cons_kmap[] = {
    &sim_int_char,
    &sim_brk_char,
    &sim_del_char
    };

/* Console I/O package.

   The console terminal can be attached to the controlling window
   or to a Telnet connection.  If attached to a Telnet connection,
   the console is described by internal terminal multiplexor
   sim_con_tmxr and internal terminal line description sim_con_ldsc.
*/

/* SET CONSOLE command */

t_stat sim_set_console (int32 flag, CONST char *cptr)
{
char *cvptr, gbuf[CBUFSIZE];
CTAB *ctptr;
t_stat r;

if ((cptr == NULL) || (*cptr == 0))
    return SCPE_2FARG;
while (*cptr != 0) {                                    /* do all mods */
    cptr = get_glyph_nc (cptr, gbuf, ',');              /* get modifier */
    if ((cvptr = strchr (gbuf, '=')))                   /* = value? */
        *cvptr++ = 0;
    (void)get_glyph (gbuf, gbuf, 0);                    /* modifier to UC */
    if ((ctptr = find_ctab (set_con_tab, gbuf))) {      /* match? */
        r = ctptr->action (ctptr->arg, cvptr);          /* do the rest */
        if (r != SCPE_OK)
            return r;
        }
    else return SCPE_NOPARAM;
    }
return SCPE_OK;
}

/* SHOW CONSOLE command */

t_stat sim_show_console (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
char gbuf[CBUFSIZE];
SHTAB *shptr;
int32 i;

if (*cptr == 0) {                                       /* show all */
    for (i = 0; show_con_tab[i].name; i++)
        show_con_tab[i].action (st, dptr, uptr, show_con_tab[i].arg, cptr);
    return SCPE_OK;
    }
while (*cptr != 0) {
    cptr = get_glyph (cptr, gbuf, ',');                 /* get modifier */
    if ((shptr = find_shtab (show_con_tab, gbuf)))
         shptr->action (st, dptr, uptr, shptr->arg, NULL);
    else return SCPE_NOPARAM;
    }
return SCPE_OK;
}

t_stat sim_rem_con_poll_svc (UNIT *uptr);               /* remote console connection poll routine */
t_stat sim_rem_con_data_svc (UNIT *uptr);               /* remote console connection data routine */
t_stat sim_rem_con_reset (DEVICE *dptr);                /* remote console reset routine */
UNIT sim_rem_con_unit[2] = {
    { UDATA (&sim_rem_con_poll_svc, UNIT_IDLE, 0)  },   /* remote console connection polling unit */
    { UDATA (&sim_rem_con_data_svc, UNIT_IDLE, 0)  }};  /* console data handling unit */

DEBTAB sim_rem_con_debug[] = {
  {"TRC", DBG_TRC},
  {"XMT", DBG_XMT},
  {"RCV", DBG_RCV},
  {0}
};

MTAB sim_rem_con_mod[] = {
  { 0 },
};

DEVICE sim_remote_console = {
    "REM-CON", sim_rem_con_unit, NULL, sim_rem_con_mod,
    2, 0, 0, 0, 0, 0,
    NULL, NULL, sim_rem_con_reset, NULL, NULL, NULL,
    NULL, DEV_DEBUG | DEV_NOSAVE, 0, sim_rem_con_debug};
#define MAX_REMOTE_SESSIONS 40  /* Arbitrary Session Limit */
static int32 *sim_rem_buf_size             = NULL;
static int32 *sim_rem_buf_ptr              = NULL;
static char **sim_rem_buf                  = NULL;
static t_bool *sim_rem_single_mode         = NULL;  /* per line command mode (single command or must continue) */
static TMXR sim_rem_con_tmxr               = { 0, 0, 0, NULL, NULL, &sim_remote_console };/* remote console line mux */
static uint32 sim_rem_read_timeout         = 30;    /* seconds before automatic continue */
static uint32 *sim_rem_read_timeouts       = NULL;  /* per line read timeout (default from sim_rem_read_timeout) */
static int32 sim_rem_active_number         = -1;    /* -1 - not active, >= 0 is index of active console */
int32 sim_rem_cmd_active_line              = -1;    /* step in progress on line # */
static CTAB *sim_rem_active_command        = NULL;  /* active command */
static char *sim_rem_command_buf;                   /* active command buffer */
static t_bool sim_log_temp                 = FALSE; /* temporary log file active */
static char sim_rem_con_temp_name[PATH_MAX+1];
static t_bool sim_rem_master_mode          = FALSE; /* Master Mode Enabled Flag */
static t_bool sim_rem_master_was_enabled   = FALSE; /* Master was Enabled */
static t_bool sim_rem_master_was_connected = FALSE; /* Master Mode has been connected */
static t_offset sim_rem_cmd_log_start      = 0;     /* Log File saved position */

/* SET REMOTE CONSOLE command */

t_stat sim_set_remote_console (int32 flag, CONST char *cptr)
{
char *cvptr, gbuf[CBUFSIZE];
CTAB *ctptr;
t_stat r;

if ((cptr == NULL) || (*cptr == 0))
    return SCPE_2FARG;
while (*cptr != 0) {                                    /* do all mods */
    cptr = get_glyph_nc (cptr, gbuf, ',');              /* get modifier */
    if ((cvptr = strchr (gbuf, '=')))                   /* = value? */
        *cvptr++ = 0;
    (void)get_glyph (gbuf, gbuf, 0);                    /* modifier to UC */
    if ((ctptr = find_ctab (set_rem_con_tab, gbuf))) {  /* match? */
        r = ctptr->action (ctptr->arg, cvptr);          /* do the rest */
        if (r != SCPE_OK)
            return r;
        }
    else return SCPE_NOPARAM;
    }
return SCPE_OK;
}

/* SHOW REMOTE CONSOLE command */

t_stat sim_show_remote_console (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
int32 i, connections;
TMLN *lp;

if (*cptr != 0)
    return SCPE_NOPARAM;
if (sim_rem_active_number >= 0) {
    if (sim_rem_master_mode && (sim_rem_active_number == 0))
        fprintf (st, "Running from Master Mode Remote Console Connection\r\n");
    else
        fprintf (st, "Running from Remote Console Connection %lu\r\n",
                 (unsigned long)sim_rem_active_number);
    }
if (sim_rem_con_tmxr.lines > 1)
    fprintf (st, "Remote Console Input Connections from %lu sources are supported concurrently\r\n",
             (unsigned long)sim_rem_con_tmxr.lines);
if (sim_rem_read_timeout)
    fprintf (st, "Remote Console Input automatically continues after %lu seconds\r\n",
             (unsigned long)sim_rem_read_timeout);
if (!sim_rem_con_tmxr.master)
    fprintf (st, "Remote Console Command input is disabled\r\n");
else
    fprintf (st, "Remote Console Command Input listening on TCP port: %s\r\n",
             sim_rem_con_unit[0].filename);
for (i=connections=0; i<sim_rem_con_tmxr.lines; i++) {
    lp = &sim_rem_con_tmxr.ldsc[i];
    if (!lp->conn)
        continue;
    ++connections;
    if (connections == 1)
        fprintf (st, "Remote Console Connections:\r\n");
    tmxr_fconns (st, lp, i);
    if (sim_rem_read_timeouts[i] != sim_rem_read_timeout) {
        if (sim_rem_read_timeouts[i])
            fprintf (st, "Remote Console Input on connection %lu automatically continues after %lu seconds\r\n",
                     (unsigned long)i, (unsigned long)sim_rem_read_timeouts[i]);
        else
            fprintf (st, "Remote Console Input on connection %lu does not continue automatically\r\n",
                     (unsigned long)i);
        }
    }
return SCPE_OK;
}

/* Unit service for remote console connection polling */

t_stat sim_rem_con_poll_svc (UNIT *uptr)
{
int32 c;

c = tmxr_poll_conn (&sim_rem_con_tmxr);
if (c >= 0) {                                           /* poll connect */
    TMLN *lp = &sim_rem_con_tmxr.ldsc[c];
    char wru_name[8];

    sim_activate_after(uptr + 1, 1000000);                /* start data poll after 1 second */
    lp->rcve                 = 1;                       /* rcv enabled */
    sim_rem_buf_ptr[c]       = 0;                       /* start with empty command buffer */
    sim_rem_single_mode[c]   = TRUE;                    /* start in single command mode */
    sim_rem_read_timeouts[c] = sim_rem_read_timeout;    /* Start with default timeout */
    if (isprint(sim_int_char & 0xFF))
        (void)sprintf(wru_name, "'%c'", sim_int_char & 0xFF);
    else
        if (sim_int_char <= 26)
            (void)sprintf(wru_name, "^%c", '@' + (sim_int_char & 0xFF));
        else
            (void)sprintf(wru_name, "'\\%03o'", sim_int_char & 0xFF);
    tmxr_linemsgf (lp, "%s Remote Console\r\n"
                       "Enter single commands or to enter multiple command mode enter the %s character\r"
                       "%s",
                       sim_name, wru_name,
                       ((sim_rem_master_mode && (c == 0)) ? "" : "\r\nSimulator Running..."));
    if (sim_rem_master_mode && (c == 0))                /* Master Mode session? */
        sim_rem_single_mode[c] = FALSE;                 /*  start in multi-command mode */
    tmxr_send_buffered_data (lp);                       /* flush buffered data */
    }
sim_activate_after(uptr, 1000000);                      /* check again in 1 second */
if (sim_con_ldsc.conn)
    tmxr_send_buffered_data (&sim_con_ldsc);            /* try to flush any buffered data */
return SCPE_OK;
}

static t_stat x_continue_cmd (int32 flag, CONST char *cptr)
{
return SCPE_IERR;           /* This routine should never be called */
}

static t_stat x_step_cmd (int32 flag, CONST char *cptr)
{
return SCPE_IERR;           /* This routine should never be called */
}

static t_stat x_run_cmd (int32 flag, CONST char *cptr)
{
return SCPE_IERR;           /* This routine should never be called */
}

static t_stat x_help_cmd (int32 flag, CONST char *cptr);

static CTAB allowed_remote_cmds[] = {
    { "EXAMINE",  &exdep_cmd,      EX_E },
    { "DEPOSIT",  &exdep_cmd,      EX_D },
    { "EVALUATE", &eval_cmd,       0 },
    { "ATTACH",   &attach_cmd,     0 },
    { "DETACH",   &detach_cmd,     0 },
#if 0 /* Needs updating for dps8 */
    { "ASSIGN",   &assign_cmd,     0 },
    { "DEASSIGN", &deassign_cmd,   0 },
#endif /* Needs updating for dps8 */
    { "CONTINUE", &x_continue_cmd, 0 },
    { "STEP",     &x_step_cmd,     0 },
    { "ECHO",     &echo_cmd,       0 },
    { "SET",      &set_cmd,        0 },
    { "SHOW",     &show_cmd,       0 },
    { "HELP",     &x_help_cmd,     0 },
    { NULL,       NULL }
    };

static CTAB allowed_master_remote_cmds[] = {
    { "EXAMINE",  &exdep_cmd,      EX_E },
    { "DEPOSIT",  &exdep_cmd,      EX_D },
    { "EVALUATE", &eval_cmd,       0 },
    { "ATTACH",   &attach_cmd,     0 },
    { "DETACH",   &detach_cmd,     0 },
#if 0 /* Needs updating for dps8 */
    { "ASSIGN",   &assign_cmd,     0 },
    { "DEASSIGN", &deassign_cmd,   0 },
#endif /* Needs updating for dps8 */
    { "CONTINUE", &x_continue_cmd, 0 },
    { "STEP",     &x_step_cmd,     0 },
    { "ECHO",     &echo_cmd,       0 },
    { "SET",      &set_cmd,        0 },
    { "SHOW",     &show_cmd,       0 },
    { "HELP",     &x_help_cmd,     0 },
    { "EXIT",     &exit_cmd,       0 },
    { "QUIT",     &exit_cmd,       0 },
    { "RUN",      &x_run_cmd,      RU_RUN },
    { "GO",       &x_run_cmd,      RU_GO },
    { "BOOT",     &x_run_cmd,      RU_BOOT },
    { "BREAK",    &brk_cmd,        SSH_ST },
    { "NOBREAK",  &brk_cmd,        SSH_CL },
    { NULL,       NULL }
    };

static CTAB allowed_single_remote_cmds[] = {
    { "ATTACH",   &attach_cmd,     0 },
    { "DETACH",   &detach_cmd,     0 },
    { "EXAMINE",  &exdep_cmd,      EX_E },
    { "EVALUATE", &eval_cmd,       0 },
    { "ECHO",     &echo_cmd,       0 },
    { "SHOW",     &show_cmd,       0 },
    { "HELP",     &x_help_cmd,     0 },
    { NULL,       NULL }
    };

static t_stat x_help_cmd (int32 flag, CONST char *cptr)
{
CTAB *cmdp, *cmdph;

if (*cptr) {
    int32 saved_switches = sim_switches;
    t_stat r;

    sim_switches |= SWMASK ('F');
    r = help_cmd (flag, cptr);
    sim_switches = saved_switches;
    return r;
    }
sim_printf ("Help is available for the following Remote Console commands:\r\n");
for (cmdp=allowed_remote_cmds; cmdp->name != NULL; ++cmdp) {
    cmdph = find_cmd (cmdp->name);
    if (cmdph && cmdph->help)
        sim_printf ("    %s\r\n", cmdp->name);
    }
sim_printf ("Enter \"HELP cmd\" for detailed help on a command\r\n");
return SCPE_OK;
}

static t_stat _sim_rem_message (const char *cmd, t_stat stat)
{
CTAB *cmdp = NULL;
t_stat stat_nomessage = stat & SCPE_NOMESSAGE;  /* extract possible message suppression flag */

cmdp = find_cmd (cmd);
stat = SCPE_BARE_STATUS(stat);              /* remove possible flag */
if (!stat_nomessage) {
    if (cmdp && (cmdp->message))                /* special message handler? */
        cmdp->message (NULL, stat);             /* let it deal with display */
    else {
        if (stat >= SCPE_BASE)                  /* error? */
            sim_printf ("%s\r\n", sim_error_text (stat));
        }
    }
return stat;
}

static void _sim_rem_log_out (TMLN *lp)
{
char cbuf[4*CBUFSIZE];

if (sim_log) {
    size_t unwritten;

    (void)fflush (sim_log);
    (void)sim_fseeko (sim_log, sim_rem_cmd_log_start, SEEK_SET);
    cbuf[sizeof(cbuf)-1] = '\0';
    while (fgets (cbuf, sizeof(cbuf)-1, sim_log))
        tmxr_linemsgf (lp, "%s", cbuf);
    if (!tmxr_input_pending_ln (lp)) {
        do {
            unwritten = tmxr_send_buffered_data (lp);
            if (unwritten == lp->txbsz)
                sim_os_ms_sleep (100);
            } while (unwritten == lp->txbsz);
        }
    }
}

void sim_remote_process_command (void)
{
char cbuf[4*CBUFSIZE], gbuf[CBUFSIZE], *argv[1] = {NULL};
CONST char *cptr;
int32 saved_switches = sim_switches;
t_stat stat;

strcpy (cbuf, sim_rem_command_buf);
while (isspace((unsigned char)cbuf[0]))
    memmove (cbuf, cbuf+1, strlen(cbuf+1)+1);   /* skip leading whitespace */
sim_sub_args (cbuf, sizeof(cbuf), argv);
cptr = cbuf;
cptr = get_glyph (cptr, gbuf, 0);               /* get command glyph */
sim_rem_active_command = find_cmd (gbuf);       /* find command */

sim_ttcmd ();                                   /* restore console */
stat = sim_rem_active_command->action (sim_rem_active_command->arg, cptr);/* execute command */
if (stat != SCPE_OK)
    stat = _sim_rem_message (gbuf, stat);       /* display results */
sim_last_cmd_stat = SCPE_BARE_STATUS(stat);
sim_ttrun ();                                   /* set console mode */
sim_cancel (&sim_rem_con_unit[1]);              /* force immediate activation of sim_rem_con_data_svc */
sim_activate (&sim_rem_con_unit[1], -1);
sim_switches = saved_switches;                  /* restore original switches */
}

/* Unit service for remote console data polling */

t_stat sim_rem_con_data_svc (UNIT *uptr)
{
int32 i, j, c = 0;
t_stat stat = SCPE_OK;
int32 steps = 0;
t_bool was_active_command = (sim_rem_cmd_active_line != -1);
t_bool got_command;
t_bool close_session = FALSE;
TMLN *lp;
char cbuf[4*CBUFSIZE], gbuf[CBUFSIZE], *argv[1] = {NULL};
CONST char *cptr;
CTAB *cmdp = NULL;
CTAB *basecmdp = NULL;
uint32 read_start_time = 0;
#if defined(TESTING)
cpu_state_t * cpup = _cpup;
#endif

tmxr_poll_rx (&sim_rem_con_tmxr);                      /* poll input */
for (i=(was_active_command ? sim_rem_cmd_active_line : 0);
     (i < sim_rem_con_tmxr.lines);
     i++) {
    t_bool master_session = (sim_rem_master_mode && (i == 0));

    lp = &sim_rem_con_tmxr.ldsc[i];
    if (!lp->conn)
        continue;
    if (master_session && !sim_rem_master_was_connected) {
        tmxr_linemsgf (lp, "\r\nMaster Mode Session\r\n");
        tmxr_send_buffered_data (lp);                   /* flush any buffered data */
        }
    sim_rem_master_was_connected |= master_session;     /* Remember if master ever connected */
    stat = SCPE_OK;
    if ((was_active_command) ||
        (master_session && !sim_rem_single_mode[i])) {
        if (was_active_command) {
            sim_rem_cmd_active_line = -1;               /* Done with active command */
            if (!sim_rem_active_command) {              /* STEP command? */
                stat = SCPE_STEP;
                _sim_rem_message ("STEP", stat);        /* produce a STEP complete message */
                }
            _sim_rem_log_out (lp);
            sim_rem_active_command = NULL;              /* Restart loop to process available input */
            was_active_command = FALSE;
            i = -1;
            continue;
            }
        else {
            sim_is_running = 0;
            sim_stop_timer_services ();
            for (j=0; j < sim_rem_con_tmxr.lines; j++) {
                TMLN *lpj = &sim_rem_con_tmxr.ldsc[j];
                if ((i == j) || (!lpj->conn))
                    continue;
                tmxr_linemsgf (lpj, "\r\nRemote Master Console(%s) Entering Commands\r\n", lp->ipad);
                tmxr_send_buffered_data (lpj);         /* flush any buffered data */
                }
            lp = &sim_rem_con_tmxr.ldsc[i];
            }
        }
    else {
        c = tmxr_getc_ln (lp);
        if (!(TMXR_VALID & c))
            continue;
        c = c & ~TMXR_VALID;
        if (sim_rem_single_mode[i]) {
            if (c == sim_int_char) { /* ^E (the interrupt character) must start continue mode console interaction */
                sim_rem_single_mode[i] = FALSE;         /* enter multi command mode */
                sim_is_running = 0;
                sim_stop_timer_services ();
                stat = SCPE_STOP;
                _sim_rem_message ("RUN", stat);
                _sim_rem_log_out (lp);
                for (j=0; j < sim_rem_con_tmxr.lines; j++) {
                    TMLN *lpj = &sim_rem_con_tmxr.ldsc[j];
                    if ((i == j) || (!lpj->conn))
                        continue;
                    tmxr_linemsgf (lpj, "\r\nRemote Console %lu(%s) Entering Commands\r\n",
                                   (unsigned long)i, lp->ipad);
                    tmxr_send_buffered_data (lpj);      /* flush any buffered data */
                    }
                lp = &sim_rem_con_tmxr.ldsc[i];
                if (!master_session)
                    tmxr_linemsg (lp, "\r\nSimulator paused.\r\n");
                if (!master_session && sim_rem_read_timeouts[i]) {
                    tmxr_linemsgf (lp, "Simulation will resume automatically if input is not received in %lu seconds\r\n",
                                   (unsigned long)sim_rem_read_timeouts[i]);
                    tmxr_linemsgf (lp, "\r\n");
                    tmxr_send_buffered_data (lp);       /* flush any buffered data */
                    }
                }
            else {
                if ((sim_rem_buf_ptr[i] == 0) &&        /* At beginning of input line */
                    ((c == '\n') ||                     /* Ignore bare LF between commands (Microsoft Telnet bug) */
                     (c == '\r')))                      /* Ignore empty commands */
                    continue;
                if ((c == '\004') || (c == '\032')) {   /* EOF character (^D or ^Z) ? */
                    tmxr_linemsgf (lp, "\r\nGoodbye\r\n");
                    tmxr_send_buffered_data (lp);       /* flush any buffered data */
                    tmxr_reset_ln (lp);
                    continue;
                    }
                if (sim_rem_buf_ptr[i] == 0) {
                    /* we just picked up the first character on a command line */
                    if (!master_session)
                        tmxr_linemsgf (lp, "\r\n%s", sim_prompt);
                    else
                        tmxr_linemsgf (lp, "\r\n%s", sim_is_running ? "SIM> " : "sim> ");
                    sim_debug (DBG_XMT, &sim_remote_console, "Prompt Written: %s\r\n", sim_is_running ? "SIM> " : "sim> ");
                    if (!tmxr_input_pending_ln (lp))
                        tmxr_send_buffered_data (lp);   /* flush any buffered data */
                    }
                }
            }
        }
    got_command = FALSE;
    while (1) {
        if (stat == SCPE_EXIT)
            return stat|SCPE_NOMESSAGE;
        if (!sim_rem_single_mode[i]) {
            read_start_time = sim_os_msec();
            if (master_session)
                tmxr_linemsg (lp, "sim> ");
            else
                tmxr_linemsg (lp, sim_prompt);
            tmxr_send_buffered_data (lp);               /* flush any buffered data */
            }
        do {
            if (!sim_rem_single_mode[i]) {
                c = tmxr_getc_ln (lp);
                if (!(TMXR_VALID & c)) {
                    tmxr_send_buffered_data (lp);       /* flush any buffered data */
                    if (!master_session &&
                        sim_rem_read_timeouts[i] &&
                        ((sim_os_msec() - read_start_time)/1000 >= sim_rem_read_timeouts[i])) {
                        while (sim_rem_buf_ptr[i] > 0) {/* Erase current input line */
                            tmxr_linemsg (lp, "\b \b");
                            --sim_rem_buf_ptr[i];
                            }
                        if (sim_rem_buf_ptr[i]+80 >= sim_rem_buf_size[i]) {
                            sim_rem_buf_size[i] += 1024;
                            sim_rem_buf[i] = (char *)realloc (sim_rem_buf[i], sim_rem_buf_size[i]);
                            }
                        strcpy (sim_rem_buf[i], "CONTINUE         ! Automatic continue due to timeout");
                        tmxr_linemsgf (lp, "%s\r\n", sim_rem_buf[i]);
                        got_command = TRUE;
                        break;
                        }
                    sim_os_ms_sleep (50);
                    tmxr_poll_rx (&sim_rem_con_tmxr);   /* poll input */
                    if (!lp->conn) {                    /* if connection lost? */
                        sim_rem_single_mode[i] = TRUE;  /* No longer multi-command more */
                        break;                          /* done waiting */
                        }
                    continue;
                    }
                read_start_time = sim_os_msec();
                c = c & ~TMXR_VALID;
                }
            switch (c) {
                case 0:     /* no data */
                    break;
                case '\b':  /* Backspace */
                case 127:   /* Rubout */
                    if (sim_rem_buf_ptr[i] > 0) {
                        tmxr_linemsg (lp, "\b \b");
                        --sim_rem_buf_ptr[i];
                        }
                    break;
                case 27:   /* escape */
                case 21:   /* ^U */
                    while (sim_rem_buf_ptr[i] > 0) {
                        tmxr_linemsg (lp, "\b \b");
                        --sim_rem_buf_ptr[i];
                        }
                    break;
                case '\n':
                    if (sim_rem_buf_ptr[i] == 0)
                        break;
                /*FALLTHRU*/ /* fall through */ /* fallthrough */
                case '\r':
                    tmxr_linemsg (lp, "\r\n");
                    if (sim_rem_buf_ptr[i]+1 >= sim_rem_buf_size[i]) {
                        sim_rem_buf_size[i] += 1024;
                        sim_rem_buf[i] = (char *)realloc (sim_rem_buf[i], sim_rem_buf_size[i]);
                        }
                    sim_rem_buf[i][sim_rem_buf_ptr[i]++] = '\0';
                    sim_debug (DBG_RCV, &sim_remote_console,
                               "Got Command (%lu bytes still in buffer): %s\r\n",
                               (unsigned long)tmxr_input_pending_ln (lp), sim_rem_buf[i]);
                    got_command = TRUE;
                    break;
                case '\004': /* EOF (^D) */
                case '\032': /* EOF (^Z) */
                    while (sim_rem_buf_ptr[i] > 0) {    /* Erase current input line */
                        tmxr_linemsg (lp, "\b \b");
                        --sim_rem_buf_ptr[i];
                        }
                    if (!sim_rem_single_mode[i]) {
                        if (sim_rem_buf_ptr[i]+80 >= sim_rem_buf_size[i]) {
                            sim_rem_buf_size[i] += 1024;
                            sim_rem_buf[i] = (char *)realloc (sim_rem_buf[i], sim_rem_buf_size[i]);
                            }
                        strcpy (sim_rem_buf[i], "CONTINUE         ! Automatic continue before close");
                        tmxr_linemsgf (lp, "%s\r\n", sim_rem_buf[i]);
                        got_command = TRUE;
                        }
                    close_session = TRUE;
                    break;
                default:
                    tmxr_putc_ln (lp, c);
                    if (sim_rem_buf_ptr[i]+2 >= sim_rem_buf_size[i]) {
                        sim_rem_buf_size[i] += 1024;
                        sim_rem_buf[i] = (char *)realloc (sim_rem_buf[i], sim_rem_buf_size[i]);
                        }
                    sim_rem_buf[i][sim_rem_buf_ptr[i]++] = (char)c;
                    sim_rem_buf[i][sim_rem_buf_ptr[i]] = '\0';
                    if (((size_t)sim_rem_buf_ptr[i]) >= sizeof(cbuf))
                        got_command = TRUE;             /* command too long */
                    break;
                }
            c = 0;
            if ((!got_command) && (sim_rem_single_mode[i]) && (tmxr_input_pending_ln (lp))) {
                c = tmxr_getc_ln (lp);
                c = c & ~TMXR_VALID;
                }
            } while ((!got_command) && ((!sim_rem_single_mode[i]) || c));
        if (!tmxr_input_pending_ln (lp))
            tmxr_send_buffered_data (lp);               /* flush any buffered data */
        if ((sim_rem_single_mode[i]) && !got_command) {
            break;
            }
        sim_printf ("Remote Console Command from %s> %s\r\n", lp->ipad, sim_rem_buf[i]);
        got_command = FALSE;
        if (strlen(sim_rem_buf[i]) >= sizeof(cbuf)) {
            sim_printf ("\r\nLine too long. Ignored.  Continuing Simulator execution\r\n");
            tmxr_linemsgf (lp, "\r\nLine too long. Ignored.  Continuing Simulator execution\r\n");
            tmxr_send_buffered_data (lp);               /* try to flush any buffered data */
            break;
            }
        strcpy (cbuf, sim_rem_buf[i]);
        sim_rem_buf_ptr[i] = 0;
        sim_rem_buf[i][sim_rem_buf_ptr[i]] = '\0';
        while (isspace((unsigned char)cbuf[0]))
            memmove (cbuf, cbuf+1, strlen(cbuf+1)+1);   /* skip leading whitespace */
        if (cbuf[0] == '\0') {
            if (sim_rem_single_mode[i]) {
                sim_rem_single_mode[i] = FALSE;
                break;
                }
            else
                continue;
            }
        strcpy (sim_rem_command_buf, cbuf);
        sim_sub_args (cbuf, sizeof(cbuf), argv);
        cptr = cbuf;
        cptr = get_glyph (cptr, gbuf, 0);               /* get command glyph */
        sim_switches = 0;                               /* init switches */
        sim_rem_active_number = i;
        if (!sim_log) {                                 /* Not currently logging? */
            int32 save_quiet = sim_quiet;

            sim_quiet = 1;
            (void)sprintf (sim_rem_con_temp_name, "sim_remote_console_%d.temporary_log", (int)_sir_getpid());
            sim_set_logon (0, sim_rem_con_temp_name);
            sim_quiet = save_quiet;
            sim_log_temp = TRUE;
            }
        sim_rem_cmd_log_start = sim_ftell (sim_log);
        basecmdp = find_cmd (gbuf);                     /* validate basic command */
        if (basecmdp == NULL) {
            if ((gbuf[0] == ';') || (gbuf[0] == '#')) { /* ignore comment */
                sim_rem_cmd_active_line = i;
                was_active_command = TRUE;
                sim_rem_active_command = &allowed_single_remote_cmds[0];/* Dummy */
                i = i - 1;
                break;
                }
            else
                stat = SCPE_UNK;
            }
        else {
            if ((cmdp = find_ctab (
                            sim_rem_single_mode[i] ? allowed_single_remote_cmds : \
                              (master_session ? allowed_master_remote_cmds : \
                               allowed_remote_cmds), gbuf))) { /* lookup command */
                if (cmdp->action == &x_continue_cmd)
                    stat = SCPE_OK;
                else {
                    if (cmdp->action == &exit_cmd)
                        return SCPE_EXIT;
                    if (cmdp->action == &x_step_cmd) {
                        steps = 1;                      /* default of 1 instruction */
                        stat = SCPE_OK;
                        if (*cptr != 0) {               /* argument? */
                             cptr = get_glyph (cptr, gbuf, 0);/* get next glyph */
                             if (*cptr != 0)            /* should be end */
                                 stat = SCPE_2MARG;
                             else {
                                 steps = (int32) get_uint (gbuf, 10, INT_MAX, &stat);
                                 if ((stat != SCPE_OK) || (steps <= 0)) /* error? */
                                     stat = SCPE_ARG;
                                 }
                             }
                        if (stat != SCPE_OK)
                            cmdp = NULL;
                        }
                    else {
                        if (cmdp->action == &x_run_cmd) {
                            sim_switches |= SIM_SW_HIDE;/* Request Setup only */
                            stat = basecmdp->action (cmdp->arg, cptr);
                            sim_switches &= ~SIM_SW_HIDE;/* Done with Setup only mode */
                            if (stat == SCPE_OK) {
                                /* switch to CONTINUE after x_run_cmd() did RUN setup */
                                cmdp = find_ctab (allowed_master_remote_cmds, "CONTINUE");
                                }
                            }
                        else
                            stat = SCPE_REMOTE;         /* force processing outside of sim_instr() */
                        }
                    }
                }
            else
                stat = SCPE_INVREM;
            }
        sim_rem_active_number = -1;
        if ((stat != SCPE_OK) && (stat != SCPE_REMOTE))
            stat = _sim_rem_message (gbuf, stat);
        _sim_rem_log_out (lp);
        if (master_session && !sim_rem_master_mode) {
            sim_rem_single_mode[i] = TRUE;
            return SCPE_STOP;
            }
        if (cmdp && (cmdp->action == &x_continue_cmd)) {
            sim_rem_cmd_active_line = -1;               /* Not active_command */
            if (sim_log_temp &&                         /* If we setup a temporary log, clean it now  */
                (!sim_rem_master_mode)) {
                int32 save_quiet = sim_quiet;

                sim_quiet = 1;
                sim_set_logoff (0, NULL);
                sim_quiet = save_quiet;
                remove (sim_rem_con_temp_name);
                sim_log_temp = FALSE;
                }
            else {
                (void)fflush (sim_log);
                sim_rem_cmd_log_start = sim_ftell (sim_log);
                }
            if (!sim_rem_single_mode[i]) {
                tmxr_linemsg (lp, "Simulator Running...");
                tmxr_send_buffered_data (lp);
                for (j=0; j < sim_rem_con_tmxr.lines; j++) {
                    TMLN *lpj = &sim_rem_con_tmxr.ldsc[j];
                    if ((i == j) || (!lpj->conn))
                        continue;
                    tmxr_linemsg (lpj, "Simulator Running...");
                    tmxr_send_buffered_data (lpj);
                    }
                sim_is_running = 1;
                sim_start_timer_services ();
                }
            /* cppcheck-suppress knownConditionTrueFalse */
            if (cmdp && (cmdp->action == &x_continue_cmd)) //-V560
                sim_rem_single_mode[i] = TRUE;
            else {
                if (!sim_rem_single_mode[i]) {
                    if (master_session)
                        tmxr_linemsgf (lp, "%s", "sim> ");
                    else
                        tmxr_linemsgf (lp, "%s", sim_prompt);
                    tmxr_send_buffered_data (lp);
                    }
                }
            break;
            }
        if ((cmdp && (cmdp->action == &x_step_cmd)) ||
            (stat == SCPE_REMOTE)) {
            sim_rem_cmd_active_line = i;
            break;
            }
        }
    if (close_session) {
        tmxr_linemsgf (lp, "\r\nGoodbye\r\n");
        tmxr_send_buffered_data (lp);                   /* flush any buffered data */
        tmxr_reset_ln (lp);
        sim_rem_single_mode[i] = FALSE;
        }
    }
if (sim_rem_master_was_connected &&                     /* Master mode ever connected? */
    !sim_rem_con_tmxr.ldsc[0].sock)                     /* Master Connection lost? */
    return SCPE_EXIT;                                   /* simulator has been 'unplugged' */
if (sim_rem_cmd_active_line != -1) {
    if (steps)
        sim_activate(uptr, steps);                      /* check again after 'steps' instructions */
    else
        return SCPE_REMOTE;                             /* force sim_instr() to exit to process command */
    }
else
    sim_activate_after(uptr, 100000);                   /* check again in 100 milliseconds */
if (sim_rem_master_was_enabled && !sim_rem_master_mode) {/* Transitioning out of master mode? */
    lp = &sim_rem_con_tmxr.ldsc[0];
    tmxr_linemsgf (lp, "Non Master Mode Session...");   /* report transition */
    tmxr_send_buffered_data (lp);                       /* flush any buffered data */
    return SCPE_STOP|SCPE_NOMESSAGE;                    /* Unwind to the normal input path */
    }
else
    return SCPE_OK;                                     /* keep going */
}

t_stat sim_rem_con_reset (DEVICE *dptr)
{
if (sim_rem_con_tmxr.lines) {
    int32 i;

    for (i=0; i<sim_rem_con_tmxr.lines; i++)
        if (sim_rem_con_tmxr.ldsc[i].conn)
            break;
    if (i != sim_rem_con_tmxr.lines)
        sim_activate_after (&dptr->units[1], 100000);   /* continue polling for open sessions */
    return sim_rem_con_poll_svc (&dptr->units[0]);      /* establish polling as needed */
    }
return SCPE_OK;
}

static t_stat sim_set_rem_telnet (int32 flag, CONST char *cptr)
{
t_stat r;

if (flag) {
    r = sim_parse_addr (cptr, NULL, 0, NULL, NULL, 0, NULL, NULL);
    if (r == SCPE_OK) {
        if (sim_rem_con_tmxr.master)                    /* already open? */
            sim_set_rem_telnet (0, NULL);               /* close first */
        if (sim_rem_con_tmxr.lines == 0)                /* Ir no connection limit set */
            sim_set_rem_connections (0, "1");           /* use 1 */
        sim_rem_con_tmxr.buffered = 1400;               /* Use big enough buffers */
        sim_register_internal_device (&sim_remote_console);
        r = tmxr_attach (&sim_rem_con_tmxr, &sim_rem_con_unit[0], cptr);/* open master socket */
        if (r == SCPE_OK)
            sim_activate_after(&sim_rem_con_unit[0], 1000000); /* check for connection in 1 second */
        return r;
        }
    return SCPE_NOPARAM;
    }
else {
    if (sim_rem_con_tmxr.master) {
        int32 i;

        tmxr_detach (&sim_rem_con_tmxr, &sim_rem_con_unit[0]);
        for (i=0; i<sim_rem_con_tmxr.lines; i++) {
            FREE (sim_rem_buf[i]);
            sim_rem_buf[i] = NULL;
            sim_rem_buf_size[i] = 0;
            sim_rem_buf_ptr[i] = 0;
            sim_rem_single_mode[i] = TRUE;
            }
        }
    }
return SCPE_OK;
}

static t_stat sim_set_rem_connections (int32 flag, CONST char *cptr)
{
int32 lines;
t_stat r;
int32 i;

if (cptr == NULL)
    return SCPE_ARG;
lines = (int32) get_uint (cptr, 10, MAX_REMOTE_SESSIONS, &r);
if (r != SCPE_OK)
    return r;
if (sim_rem_con_tmxr.master)
    return SCPE_ARG;
for (i=0; i<sim_rem_con_tmxr.lines; i++)
    FREE (sim_rem_buf[i]);
sim_rem_con_tmxr.lines = lines;
sim_rem_con_tmxr.ldsc = (TMLN *)realloc (sim_rem_con_tmxr.ldsc, sizeof(*sim_rem_con_tmxr.ldsc)*lines);
if (!sim_rem_con_tmxr.ldsc)
  {
    fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
             __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
(void)memset (sim_rem_con_tmxr.ldsc, 0, sizeof(*sim_rem_con_tmxr.ldsc)*lines);
sim_rem_buf = (char **)realloc (sim_rem_buf, sizeof(*sim_rem_buf)*lines);
if (!sim_rem_buf)
  {
    fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
             __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
(void)memset (sim_rem_buf, 0, sizeof(*sim_rem_buf)*lines);
sim_rem_buf_size = (int32 *)realloc (sim_rem_buf_size, sizeof(*sim_rem_buf_size)*lines);
if (!sim_rem_buf_size)
  {
    fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
             __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
(void)memset (sim_rem_buf_size, 0, sizeof(*sim_rem_buf_size)*lines);
sim_rem_buf_ptr = (int32 *)realloc (sim_rem_buf_ptr, sizeof(*sim_rem_buf_ptr)*lines);
if (!sim_rem_buf_ptr)
  {
    fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
             __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
(void)memset (sim_rem_buf_ptr, 0, sizeof(*sim_rem_buf_ptr)*lines);
sim_rem_single_mode = (t_bool *)realloc (sim_rem_single_mode, sizeof(*sim_rem_single_mode)*lines);
if (!sim_rem_single_mode)
  {
    fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
             __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
(void)memset (sim_rem_single_mode, 0, sizeof(*sim_rem_single_mode)*lines);
sim_rem_read_timeouts = (uint32 *)realloc (sim_rem_read_timeouts, sizeof(*sim_rem_read_timeouts)*lines);
if (!sim_rem_read_timeouts)
  {
    fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
             __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
(void)memset (sim_rem_read_timeouts, 0, sizeof(*sim_rem_read_timeouts)*lines);
sim_rem_command_buf = (char *)realloc (sim_rem_command_buf, 4*CBUFSIZE+1);
if (!sim_rem_command_buf)
  {
    fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
             __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
    (void)raise(SIGUSR2);
    /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
    abort();
  }
(void)memset (sim_rem_command_buf, 0, 4*CBUFSIZE+1);
return SCPE_OK;
}

static t_stat sim_set_rem_timeout (int32 flag, CONST char *cptr)
{
int32 timeout;
t_stat r;

if (cptr == NULL)
    return SCPE_ARG;
timeout = (int32) get_uint (cptr, 10, 3600, &r);
if (r != SCPE_OK)
    return r;
if (sim_rem_active_number >= 0)
    sim_rem_read_timeouts[sim_rem_active_number] = timeout;
else
    sim_rem_read_timeout = timeout;
return SCPE_OK;
}

/* Set keyboard map */

t_stat sim_set_kmap (int32 flag, CONST char *cptr)
{
DEVICE *dptr = sim_devices[0];
int32 val, rdx;
t_stat r;

if ((cptr == NULL) || (*cptr == 0))
    return SCPE_2FARG;
if (dptr->dradix == 16) rdx = 16;
else rdx = 8;
val = (int32) get_uint (cptr, rdx, 0177, &r); //-V536
if ((r != SCPE_OK) ||
    ((val == 0) && (flag & KMAP_NZ)))
    return SCPE_ARG;
*(cons_kmap[flag & KMAP_MASK]) = val;
return SCPE_OK;
}

/* Show keyboard map */

t_stat sim_show_kmap (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
if (sim_devices[0]->dradix == 16)
    fprintf (st, "%s = %X\r\n", show_con_tab[flag].name, *(cons_kmap[flag & KMAP_MASK]));
else fprintf (st, "%s = %o\r\n", show_con_tab[flag].name, *(cons_kmap[flag & KMAP_MASK]));
return SCPE_OK;
}

/* Set printable characters */

t_stat sim_set_pchar (int32 flag, CONST char *cptr)
{
DEVICE *dptr = sim_devices[0];
uint32 val, rdx;
t_stat r;

if ((cptr == NULL) || (*cptr == 0))
    return SCPE_2FARG;
if (dptr->dradix == 16) rdx = 16;
else rdx = 8;
val = (uint32) get_uint (cptr, rdx, 0xFFFFFFFF, &r);
if ((r != SCPE_OK) ||
    ((val & 0x00002400) == 0))
    return SCPE_ARG;
sim_tt_pchar = val;
return SCPE_OK;
}

/* Set input speed (bps) */

t_stat sim_set_cons_speed (int32 flag, CONST char *cptr)
{
return tmxr_set_line_speed (&sim_con_ldsc, cptr);
}

t_stat sim_show_cons_speed (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
if (sim_con_ldsc.rxbps) {
    fprintf (st, "Speed = %ld", (long)sim_con_ldsc.rxbps);
    if (sim_con_ldsc.rxbpsfactor != TMXR_RX_BPS_UNIT_SCALE)
        fprintf (st, "*%.0f", sim_con_ldsc.rxbpsfactor/TMXR_RX_BPS_UNIT_SCALE);
    fprintf (st, " bps\r\n");
    }
return SCPE_OK;
}

/* Set log routine */

t_stat sim_set_logon (int32 flag, CONST char *cptr)
{
char gbuf[CBUFSIZE];
t_stat r;
time_t now;

if ((cptr == NULL) || (*cptr == 0))                     /* need arg */
    return SCPE_2FARG;
cptr = get_glyph_nc (cptr, gbuf, 0);                    /* get file name */
if (*cptr != 0)                                         /* now eol? */
    return SCPE_2MARG;
sim_set_logoff (0, NULL);                               /* close cur log */
r = sim_open_logfile (gbuf, (sim_switches & SWMASK ('B')) == SWMASK ('B'),
                            &sim_log, &sim_log_ref);    /* open log */
if (r != SCPE_OK)                                       /* error? */
    return r;
if (!sim_quiet)
    printf ("Logging to file \"%s\"\r\n",
             sim_logfile_name (sim_log, sim_log_ref));
fprintf (sim_log, "Logging to file \"%s\"\r\n",
             sim_logfile_name (sim_log, sim_log_ref));  /* start of log */
time(&now);
fprintf (sim_log, "Logging to file \"%s\" at %s", sim_logfile_name (sim_log, sim_log_ref), ctime(&now));
return SCPE_OK;
}

/* Set nolog routine */

t_stat sim_set_logoff (int32 flag, CONST char *cptr)
{
if (cptr && (*cptr != 0))                               /* now eol? */
    return SCPE_2MARG;
if (sim_log == NULL)                                    /* no log? */
    return SCPE_OK;
if (!sim_quiet)
    printf ("Log file closed\r\n");
fprintf (sim_log, "Log file closed\r\n");
sim_close_logfile (&sim_log_ref);                       /* close log */
sim_log = NULL;
return SCPE_OK;
}

/* Show log status */

t_stat sim_show_log (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
if (cptr && (*cptr != 0))
    return SCPE_2MARG;
if (sim_log)
    fprintf (st, "Logging enabled to \"%s\"\r\n",
                 sim_logfile_name (sim_log, sim_log_ref));
else fprintf (st, "Logging disabled\r\n");
return SCPE_OK;
}

/* Set debug routine */

t_stat sim_set_debon (int32 flag, CONST char *cptr)
{
char gbuf[CBUFSIZE];
t_stat r;
time_t now;

sim_deb_switches = sim_switches;                        /* save debug switches */
if ((cptr == NULL) || (*cptr == 0))                     /* need arg */
    return SCPE_2FARG;
cptr = get_glyph_nc (cptr, gbuf, 0);                    /* get file name */
if (*cptr != 0)                                         /* now eol? */
    return SCPE_2MARG;
r = sim_open_logfile (gbuf, FALSE, &sim_deb, &sim_deb_ref);

if (r != SCPE_OK)
    return r;

if (sim_deb_switches & SWMASK ('R')) {
    clock_gettime(CLOCK_REALTIME, &sim_deb_basetime);
    if (!(sim_deb_switches & (SWMASK ('A') | SWMASK ('T'))))
        sim_deb_switches |= SWMASK ('T');
    }
if (!sim_quiet) {
    sim_printf ("Debug output to \"%s\"\r\n",
                sim_logfile_name (sim_deb, sim_deb_ref));
    if (sim_deb_switches & SWMASK ('P'))
        sim_printf ("   Debug messages contain current PC value\r\n");
    if (sim_deb_switches & SWMASK ('T'))
        sim_printf ("   Debug messages display time of day as hh:mm:ss.msec%s\r\n",
                    sim_deb_switches & SWMASK ('R') ? " relative to the start of debugging" : "");
    if (sim_deb_switches & SWMASK ('A'))
        sim_printf ("   Debug messages display time of day as seconds.msec%s\r\n",
                    sim_deb_switches & SWMASK ('R') ? " relative to the start of debugging" : "");
    time(&now);
    fprintf (sim_deb, "Debug output to \"%s\" at %s",
             sim_logfile_name (sim_deb, sim_deb_ref), ctime(&now));
    show_version (sim_deb, NULL, NULL, 0, NULL);
    }
if (sim_deb_switches & SWMASK ('N'))
    sim_deb_switches &= ~SWMASK ('N');          /* Only process the -N flag initially */

return SCPE_OK;
}

t_stat sim_debug_flush (void)
{
int32 saved_quiet = sim_quiet;
int32 saved_sim_switches = sim_switches;
int32 saved_deb_switches = sim_deb_switches;
struct timespec saved_deb_basetime = sim_deb_basetime;
char saved_debug_filename[CBUFSIZE];

if (sim_deb == NULL)                                    /* no debug? */
    return SCPE_OK;

if (sim_deb == sim_log) {                               /* debug is log */
    (void)fflush (sim_deb);                             /* fflush is the best we can do */
    return SCPE_OK;
    }

strcpy (saved_debug_filename, sim_logfile_name (sim_deb, sim_deb_ref));

sim_quiet = 1;
sim_set_deboff (0, NULL);
sim_switches = saved_deb_switches;
sim_set_debon (0, saved_debug_filename);
sim_deb_basetime = saved_deb_basetime;
sim_switches = saved_sim_switches;
sim_quiet = saved_quiet;
return SCPE_OK;
}

/* Set nodebug routine */

t_stat sim_set_deboff (int32 flag, CONST char *cptr)
{
if (cptr && (*cptr != 0))                               /* now eol? */
    return SCPE_2MARG;
if (sim_deb == NULL)                                    /* no debug? */
    return SCPE_OK;
sim_close_logfile (&sim_deb_ref);
sim_deb = NULL;
sim_deb_switches = 0;
if (!sim_quiet)
    sim_printf ("Debug output disabled\r\n");
return SCPE_OK;
}

/* Show debug routine */

t_stat sim_show_debug (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
int32 i;

if (cptr && (*cptr != 0))
    return SCPE_2MARG;
if (sim_deb) {
    fprintf (st, "Debug output enabled to \"%s\"\r\n",
             sim_logfile_name (sim_deb, sim_deb_ref));
    if (sim_deb_switches & SWMASK ('P'))
        fprintf (st, "   Debug messages contain current PC value\r\n");
    if (sim_deb_switches & SWMASK ('T'))
        fprintf (st, "   Debug messages display time of day as hh:mm:ss.msec%s\r\n",
                 sim_deb_switches & SWMASK ('R') ? " relative to the start of debugging" : "");
    if (sim_deb_switches & SWMASK ('A'))
        fprintf (st, "   Debug messages display time of day as seconds.msec%s\r\n",
                 sim_deb_switches & SWMASK ('R') ? " relative to the start of debugging" : "");
    for (i = 0; (dptr = sim_devices[i]) != NULL; i++) {
        if (!(dptr->flags & DEV_DIS) &&
            (dptr->flags & DEV_DEBUG) &&
            (dptr->dctrl)) {
            fprintf (st, "Device: %-6s ", dptr->name);
            show_dev_debug (st, dptr, NULL, 0, NULL);
            }
        }
    for (i = 0; sim_internal_device_count && (dptr = sim_internal_devices[i]); ++i) {
        if (!(dptr->flags & DEV_DIS) &&
            (dptr->flags & DEV_DEBUG) &&
            (dptr->dctrl)) {
            fprintf (st, "Device: %-6s ", dptr->name);
            show_dev_debug (st, dptr, NULL, 0, NULL);
            }
        }
    }
else fprintf (st, "Debug output disabled\r\n");
return SCPE_OK;
}

/* SET CONSOLE command */

/* Set console to Telnet port (and parameters) */

t_stat sim_set_telnet (int32 flag, CONST char *cptr)
{
char *cvptr, gbuf[CBUFSIZE];
CTAB *ctptr;
t_stat r;

if ((cptr == NULL) || (*cptr == 0))
    return SCPE_2FARG;
while (*cptr != 0) {                                      /* do all mods */
    cptr = get_glyph_nc (cptr, gbuf, ',');                /* get modifier */
    if ((cvptr = strchr (gbuf, '=')))                     /* = value? */
        *cvptr++ = 0;
    (void)get_glyph (gbuf, gbuf, 0);                      /* modifier to UC */
    if ((ctptr = find_ctab (set_con_telnet_tab, gbuf))) { /* match? */
        r = ctptr->action (ctptr->arg, cvptr);            /* do the rest */
        if (r != SCPE_OK)
            return r;
        }
    else {
        if (sim_con_tmxr.master)                        /* already open? */
            sim_set_notelnet (0, NULL);                 /* close first */
        r = tmxr_attach (&sim_con_tmxr, &sim_con_unit, gbuf);/* open master socket */
        if (r == SCPE_OK)
            sim_activate_after(&sim_con_unit, 1000000); /* check for connection in 1 second */
        else
            return r;
        }
    }
return SCPE_OK;
}

/* Close console Telnet port */

t_stat sim_set_notelnet (int32 flag, CONST char *cptr)
{
if (cptr && (*cptr != 0))                               /* too many arguments? */
    return SCPE_2MARG;
if (sim_con_tmxr.master == 0)                           /* ignore if already closed */
    return SCPE_OK;
return tmxr_close_master (&sim_con_tmxr);               /* close master socket */
}

/* Show console Telnet status */

t_stat sim_show_telnet (FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag, CONST char *cptr)
{
if (cptr && (*cptr != 0))
    return SCPE_2MARG;
if ((sim_con_tmxr.master == 0) &&
    (sim_con_ldsc.serport == 0))
    fprintf (st, "Connected to console window\r\n");
else {
    if (sim_con_ldsc.serport) {
        fprintf (st, "Connected to ");
        tmxr_fconns (st, &sim_con_ldsc, -1);
        }
    else
        if (sim_con_ldsc.sock == 0)
            fprintf (st, "Listening on port %s\r\n", sim_con_tmxr.port);
        else {
            fprintf (st, "Listening on port %s, connection from %s\r\n",
                sim_con_tmxr.port, sim_con_ldsc.ipad);
            tmxr_fconns (st, &sim_con_ldsc, -1);
            }
    tmxr_fstats (st, &sim_con_ldsc, -1);
    }
return SCPE_OK;
}

/* Set console to Buffering  */

t_stat sim_set_cons_buff (int32 flg, CONST char *cptr)
{
char cmdbuf[CBUFSIZE];

(void)sprintf(cmdbuf, "BUFFERED%c%s", cptr ? '=' : '\0', cptr ? cptr : "");
return tmxr_open_master (&sim_con_tmxr, cmdbuf);      /* open master socket */
}

/* Set console to NoBuffering */

t_stat sim_set_cons_unbuff (int32 flg, CONST char *cptr)
{
char cmdbuf[CBUFSIZE];

(void)sprintf(cmdbuf, "UNBUFFERED%c%s", cptr ? '=' : '\0', cptr ? cptr : "");
return tmxr_open_master (&sim_con_tmxr, cmdbuf);      /* open master socket */
}

/* Set console to Logging */

t_stat sim_set_cons_log (int32 flg, CONST char *cptr)
{
char cmdbuf[CBUFSIZE];

(void)sprintf(cmdbuf, "LOG%c%s", cptr ? '=' : '\0', cptr ? cptr : "");
return tmxr_open_master (&sim_con_tmxr, cmdbuf);      /* open master socket */
}

/* Set console to NoLogging */

t_stat sim_set_cons_nolog (int32 flg, CONST char *cptr)
{
char cmdbuf[CBUFSIZE];

(void)sprintf(cmdbuf, "NOLOG%c%s", cptr ? '=' : '\0', cptr ? cptr : "");
return tmxr_open_master (&sim_con_tmxr, cmdbuf);      /* open master socket */
}

t_stat sim_show_cons_log (FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag, CONST char *cptr)
{
if (cptr && (*cptr != 0))
    return SCPE_2MARG;
if (sim_con_tmxr.ldsc->txlog)
    fprintf (st, "Log File being written to %s\r\n", sim_con_tmxr.ldsc->txlogname);
else
    fprintf (st, "No Logging\r\n");
return SCPE_OK;
}

t_stat sim_show_cons_buff (FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag, CONST char *cptr)
{
if (cptr && (*cptr != 0))
    return SCPE_2MARG;
if (!sim_con_tmxr.ldsc->txbfd)
    fprintf (st, "Unbuffered\r\n");
else
    fprintf (st, "Buffer Size = %lld\r\n",
             (long long)sim_con_tmxr.ldsc->txbsz);
return SCPE_OK;
}

/* Set console to Serial port (and parameters) */

/* Show the console expect rules and state */

t_stat sim_show_cons_expect (FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag, CONST char *cptr)
{
return sim_exp_show (st, &sim_con_expect, cptr);
}

/* Log File Open/Close/Show Support */

/* Open log file */

t_stat sim_open_logfile (const char *filename, t_bool binary, FILE **pf, FILEREF **pref)
{
char gbuf[CBUFSIZE - 1];
const char *tptr;

if ((filename == NULL) || (*filename == 0))             /* too few arguments? */
    return SCPE_2FARG;
tptr = get_glyph (filename, gbuf, 0);
if (*tptr != 0)                                         /* now eol? */
    return SCPE_2MARG;
sim_close_logfile (pref);
*pf = NULL;
if (strcmp (gbuf, "LOG") == 0) {                        /* output to log? */
    if (sim_log == NULL)                                /* any log? */
        return SCPE_ARG;
    *pf = sim_log;
    *pref = sim_log_ref;
    if (*pref)
        ++(*pref)->refcount;
    }
else if (strcmp (gbuf, "DEBUG") == 0) {                 /* output to debug? */
    if (sim_deb == NULL)                                /* any debug? */
        return SCPE_ARG;
    *pf = sim_deb;
    *pref = sim_deb_ref;
    if (*pref)
        ++(*pref)->refcount;
    }
else if (strcmp (gbuf, "STDOUT") == 0) {                /* output to stdout? */
    *pf = stdout;
    *pref = NULL;
    }
else if (strcmp (gbuf, "STDERR") == 0) {                /* output to stderr? */
    *pf = stderr;
    *pref = NULL;
    }
else {
    *pref = (FILEREF *)calloc (1, sizeof(**pref));
    if (!*pref)
        return SCPE_MEM;
    (void)get_glyph_nc (filename, gbuf, 0);             /* reparse */
    (void)strncpy ((*pref)->name, gbuf, sizeof((*pref)->name) - 1);
    (*pref)->name[sizeof((*pref)->name) - 1] = '\0';
    if (sim_switches & SWMASK ('N'))                    /* if a new log file is requested */
        *pf = sim_fopen (gbuf, (binary ? "w+b" : "w+"));/*   then open an empty file */
    else                                                /* otherwise */
        *pf = sim_fopen (gbuf, (binary ? "a+b" : "a+"));/*   append to an existing file */
    if (*pf == NULL) {                                  /* error? */
        FREE (*pref);
        *pref = NULL;
        return SCPE_OPENERR;
        }
    (*pref)->file = *pf;
    (*pref)->refcount = 1;                               /* need close */
    }
return SCPE_OK;
}

/* Close log file */

t_stat sim_close_logfile (FILEREF **pref)
{
if (NULL == *pref)
    return SCPE_OK;
(*pref)->refcount = (*pref)->refcount  - 1;
if ((*pref)->refcount > 0) {
    *pref = NULL;
    return SCPE_OK;
    }
fclose ((*pref)->file);
FREE (*pref);
*pref = NULL;
return SCPE_OK;
}

/* Show logfile support routine */

const char *sim_logfile_name (const FILE *st, FILEREF *ref)
{
if (!st)
    return "";
if (st == stdout)
    return "STDOUT";
if (st == stderr)
    return "STDERR";
if (!ref)
    return "";
return ref->name;
}

/* Check connection before executing
   (including a remote console which may be required in master mode) */

t_stat sim_check_console (int32 sec)
{
int32 c, trys = 0;

if (sim_rem_master_mode) {
    for (;trys < sec; ++trys) {
        sim_rem_con_poll_svc (&sim_rem_con_unit[0]);
        if (sim_rem_con_tmxr.ldsc[0].conn)
            break;
        if ((trys % 10) == 0) {                         /* Status every 10 sec */
            sim_printf ("Waiting for Remote Console connection\r\n");
            (void)fflush (stdout);
            if (sim_log)                                /* log file? */
                (void)fflush (sim_log);
            }
        sim_os_sleep (1);                               /* wait 1 second */
        }
    if ((sim_rem_con_tmxr.ldsc[0].conn) &&
        (!sim_con_ldsc.serport) &&
        (sim_con_tmxr.master == 0) &&
        (sim_con_console_port)) {
        tmxr_linemsgf (&sim_rem_con_tmxr.ldsc[0], "\r\nConsole port must be Telnet or Serial with Master Remote Console\r\n");
        tmxr_linemsgf (&sim_rem_con_tmxr.ldsc[0], "Goodbye\r\n");
        while (tmxr_send_buffered_data (&sim_rem_con_tmxr.ldsc[0]))
            sim_os_ms_sleep (100);
        sim_os_ms_sleep (100);
        tmxr_reset_ln (&sim_rem_con_tmxr.ldsc[0]);
        sim_printf ("Console port must be Telnet or Serial with Master Remote Console\r\n");
        return SCPE_EXIT;
        }
    }
if (trys == sec) {
    return SCPE_TTMO;                                   /* timed out */
    }
if (sim_con_ldsc.serport)
    if (tmxr_poll_conn (&sim_con_tmxr) >= 0)
        sim_con_ldsc.rcve = 1;                          /* rcv enabled */
if ((sim_con_tmxr.master == 0) ||                       /* console or not Telnet? done */
    (sim_con_ldsc.serport))
    return SCPE_OK;
if (sim_con_ldsc.conn || sim_con_ldsc.txbfd) {          /* connected or buffered ? */
    tmxr_poll_rx (&sim_con_tmxr);                       /* poll (check disconn) */
    if (sim_con_ldsc.conn || sim_con_ldsc.txbfd) {      /* still connected? */
        if (!sim_con_ldsc.conn) {
            sim_printf ("Running with Buffered Console\r\n"); /* print transition */
            (void)fflush (stdout);
            if (sim_log)                                /* log file? */
                (void)fflush (sim_log);
            }
        return SCPE_OK;
        }
    }
for (; trys < sec; trys++) {                            /* loop */
    if (tmxr_poll_conn (&sim_con_tmxr) >= 0) {          /* poll connect */
        sim_con_ldsc.rcve = 1;                          /* rcv enabled */
        if (trys) {                                     /* if delayed */
            sim_printf ("Running\r\n");                 /* print transition */
            (void)fflush (stdout);
            if (sim_log)                                /* log file? */
                (void)fflush (sim_log);
            }
        return SCPE_OK;                                 /* ready to proceed */
        }
    c = sim_os_poll_kbd ();                             /* check for stop char */
    if ((c == SCPE_STOP) || stop_cpu)
        return SCPE_STOP;
    if ((trys % 10) == 0) {                             /* Status every 10 sec */
        sim_printf ("Waiting for console Telnet connection\r\n");
        (void)fflush (stdout);
        if (sim_log)                                    /* log file? */
            (void)fflush (sim_log);
        }
    sim_os_sleep (1);                                   /* wait 1 second */
    }
return SCPE_TTMO;                                       /* timed out */
}

/* Get Send object address for console */

SEND *sim_cons_get_send (void)
{
return &sim_con_send;
}

/* Get Expect object address for console */

EXPECT *sim_cons_get_expect (void)
{
return &sim_con_expect;
}

/* Display console Queued input data status */

t_stat sim_show_cons_send_input (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
return sim_show_send_input (st, &sim_con_send);
}

/* Poll for character */

t_stat sim_poll_kbd (void)
{
t_stat c;

if (!sim_localopc)
  return SCPE_OK;

if (sim_send_poll_data (&sim_con_send, &c))                 /* injected input characters available? */
    return c;
if (!sim_rem_master_mode) {
    if ((sim_con_ldsc.rxbps) &&                             /* rate limiting && */
        (sim_gtime () < sim_con_ldsc.rxnexttime))           /* too soon? */
        return SCPE_OK;                                     /* not yet */
    c = sim_os_poll_kbd ();                                 /* get character */
    if (c == SCPE_STOP) {                                   /* ^E */
        stop_cpu = 1;                                       /* Force a stop (which is picked up by sim_process_event */
        return SCPE_OK;
        }
    if ((sim_con_tmxr.master == 0) &&                       /* not Telnet? */
        (sim_con_ldsc.serport == 0)) {                      /* and not serial? */
        if (c && sim_con_ldsc.rxbps)                        /* got something && rate limiting? */
            sim_con_ldsc.rxnexttime =                       /* compute next input time */
                floor (sim_gtime () + ((sim_con_ldsc.rxdelta * sim_timer_inst_per_sec ())/sim_con_ldsc.rxbpsfactor));
        return c;                                           /* in-window */
        }
    if (!sim_con_ldsc.conn) {                               /* no telnet or serial connection? */
        if (!sim_con_ldsc.txbfd)                            /* unbuffered? */
            return SCPE_LOST;                               /* connection lost */
        if (tmxr_poll_conn (&sim_con_tmxr) >= 0)            /* poll connect */
            sim_con_ldsc.rcve = 1;                          /* rcv enabled */
        else                                                /* fall through to poll reception */
            return SCPE_OK;                                 /* unconnected and buffered - nothing to receive */
        }
    }
tmxr_poll_rx (&sim_con_tmxr);                               /* poll for input */
if ((c = (t_stat)tmxr_getc_ln (&sim_con_ldsc)))             /* any char? */
    return (c & (SCPE_BREAK | 0377)) | SCPE_KFLAG;
return SCPE_OK;
}

/* Output character */

t_stat sim_putchar (int32 c)
{
sim_exp_check (&sim_con_expect, c);
if ((sim_con_tmxr.master == 0) &&                       /* not Telnet? */
    (sim_con_ldsc.serport == 0)) {                      /* and not serial port */
    if (sim_log)                                        /* log file? */
        fputc (c, sim_log);
    return sim_os_putchar (c);                          /* in-window version */
    }
if (!sim_con_ldsc.conn) {                               /* no Telnet or serial connection? */
    if (!sim_con_ldsc.txbfd)                            /* unbuffered? */
        return SCPE_LOST;                               /* connection lost */
    if (tmxr_poll_conn (&sim_con_tmxr) >= 0)            /* poll connect */
        sim_con_ldsc.rcve = 1;                          /* rcv enabled */
    }
tmxr_putc_ln (&sim_con_ldsc, c);                        /* output char */
tmxr_poll_tx (&sim_con_tmxr);                           /* poll xmt */
return SCPE_OK;
}

/* Tab stop array handling

   *desc points to a uint8 array of length val

   Columns with tabs set are non-zero; columns without tabs are 0 */

t_stat sim_ttinit (void)
{
sim_con_tmxr.ldsc->mp = &sim_con_tmxr;
sim_register_internal_device (&sim_con_telnet);
tmxr_startup ();
return sim_os_ttinit ();
}

t_stat sim_ttrun (void)
{
if (!sim_con_tmxr.ldsc->uptr) {                         /* If simulator didn't declare its input polling unit */
    sim_con_unit.dynflags &= ~UNIT_TM_POLL;             /* we can't poll asynchronously */
    sim_con_unit.dynflags |= TMUF_NOASYNCH;             /* disable asynchronous behavior */
    }
#if 0
else {
    }
#endif
tmxr_start_poll ();
return sim_os_ttrun ();
}

t_stat sim_ttcmd (void)
{
tmxr_stop_poll ();
return sim_os_ttcmd ();
}

t_stat sim_ttclose (void)
{
t_stat r1 = tmxr_shutdown ();
t_stat r2 = sim_os_ttclose ();

if (r1 != SCPE_OK)
    return r1;

return r2;
}

t_bool sim_ttisatty (void)
{
return sim_os_ttisatty ();
}

/* Platform specific routine definitions */

#if defined (_WIN32)

# include <fcntl.h>
# include <io.h>
# include <windows.h>
# define RAW_MODE 0
static HANDLE std_input;
static HANDLE std_output;
static DWORD saved_mode;

/* Note: This routine catches all the potential events which some aspect
         of the windows system can generate.  The CTRL_C_EVENT won't be
         generated by a  user typing in a console session since that
         session is in RAW mode.  In general, Ctrl-C on a simulator's
         console terminal is a useful character to be passed to the
         simulator.  This code does nothing to disable or affect that. */

static BOOL WINAPI
ControlHandler(DWORD dwCtrlType)
    {
    DWORD Mode;
    extern void int_handler (int sig);

    switch (dwCtrlType)
        {
        case CTRL_BREAK_EVENT:      // Use CTRL-Break or CTRL-C to simulate
        case CTRL_C_EVENT:          // SERVICE_CONTROL_STOP in debug mode
            int_handler(0);
            return TRUE;
        case CTRL_CLOSE_EVENT:      // Window is Closing
        case CTRL_LOGOFF_EVENT:     // User is logging off
            if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &Mode))
                return TRUE;        // Not our User, so ignore
            /*FALLTHRU*/ /* fall through */ /* fallthrough */
        case CTRL_SHUTDOWN_EVENT:   // System is shutting down
            int_handler(0);
            return TRUE;
        }
    return FALSE;
    }

static t_stat sim_os_ttinit (void)
{
SetConsoleCtrlHandler( ControlHandler, TRUE );
std_input = GetStdHandle (STD_INPUT_HANDLE);
std_output = GetStdHandle (STD_OUTPUT_HANDLE);
if ((std_input) &&                                      /* Not Background process? */
    (std_input != INVALID_HANDLE_VALUE))
    GetConsoleMode (std_input, &saved_mode);            /* Save Mode */
return SCPE_OK;
}

static t_stat sim_os_ttrun (void)
{
if ((std_input) &&                                      /* If Not Background process? */
    (std_input != INVALID_HANDLE_VALUE) &&
    (!GetConsoleMode(std_input, &saved_mode) ||         /* Set mode to RAW */
     !SetConsoleMode(std_input, RAW_MODE)))
    return SCPE_TTYERR;
if (sim_log) {
    (void)fflush (sim_log);
    _setmode (_fileno (sim_log), _O_BINARY);
    }
return SCPE_OK;
}

static t_stat sim_os_ttcmd (void)
{
if (sim_log) {
    (void)fflush (sim_log);
    _setmode (_fileno (sim_log), _O_TEXT);
    }
if ((std_input) &&                                      /* If Not Background process? */
    (std_input != INVALID_HANDLE_VALUE) &&
    (!SetConsoleMode(std_input, saved_mode)))           /* Restore Normal mode */
    return SCPE_TTYERR;
return SCPE_OK;
}

static t_stat sim_os_ttclose (void)
{
return SCPE_OK;
}

static t_bool sim_os_ttisatty (void)
{
DWORD Mode;

return (std_input) && (std_input != INVALID_HANDLE_VALUE) && GetConsoleMode (std_input, &Mode);
}

static t_stat sim_os_poll_kbd (void)
{
int c = -1;
DWORD nkbevents, nkbevent;
INPUT_RECORD rec;

sim_debug (DBG_TRC, &sim_con_telnet, "sim_os_poll_kbd()\r\n");

if ((std_input == NULL) ||                              /* No keyboard for */
    (std_input == INVALID_HANDLE_VALUE))                /* background processes */
    return SCPE_OK;
if (!GetNumberOfConsoleInputEvents(std_input, &nkbevents))
    return SCPE_TTYERR;
while (c == -1) {
    if (0 == nkbevents)
        return SCPE_OK;
    if (!ReadConsoleInput(std_input, &rec, 1, &nkbevent))
        return SCPE_TTYERR;
    if (0 == nkbevent)
        return SCPE_OK;
    --nkbevents;
    if (rec.EventType == KEY_EVENT) {
        if (rec.Event.KeyEvent.bKeyDown) {
            if (0 == rec.Event.KeyEvent.uChar.UnicodeChar) {     /* Special Character/Keys? */
                if (rec.Event.KeyEvent.wVirtualKeyCode == VK_PAUSE) /* Pause/Break Key */
                    c = sim_brk_char | SCPE_BREAK;
                else
                    if (rec.Event.KeyEvent.wVirtualKeyCode == '2')  /* ^@ */
                        c = 0;                                      /* return NUL */
            } else
                c = rec.Event.KeyEvent.uChar.AsciiChar;
            }
      }
    }
if ((c & 0177) == sim_del_char)
    c = 0177;
if ((c & 0177) == sim_int_char)
    return SCPE_STOP;
if ((sim_brk_char && ((c & 0177) == sim_brk_char)) || (c & SCPE_BREAK))
    return SCPE_BREAK;
return c | SCPE_KFLAG;
}

# define BELL_CHAR         7         /* Bell Character */
# define BELL_INTERVAL_MS  500       /* No more than 2 Bell Characters Per Second */
static t_stat sim_os_putchar (int32 c)
{
DWORD unused;
static uint32 last_bell_time;

if (!sim_localopc)
  return SCPE_OK;

if (c != 0177) {
    if (c == BELL_CHAR) {
        uint32 now = sim_os_msec ();

        if ((now - last_bell_time) > BELL_INTERVAL_MS) {
            WriteConsoleA(std_output, &c, 1, &unused, NULL);
            last_bell_time = now;
            }
        }
    else
        WriteConsoleA(std_output, &c, 1, &unused, NULL);
    }
return SCPE_OK;
}

#elif defined (BSDTTY)

/* BSD Routines */

# include <sgtty.h>
# include <fcntl.h>
# include <unistd.h>

struct sgttyb  cmdtty,     runtty;                       /* V6/V7 stty data */
struct tchars  cmdtchars,  runtchars;                    /* V7 editing */
struct ltchars cmdltchars, runltchars;                   /* 4.2 BSD editing */
int cmdfl,runfl;                                         /* TTY flags */

static t_stat sim_os_ttinit (void)
{
cmdfl = fcntl (0, F_GETFL, 0);                           /* get old flags  and status */
runfl = cmdfl | FNDELAY;
if (ioctl (0, TIOCGETP, &cmdtty) < 0)
    return SCPE_TTIERR;
if (ioctl (0, TIOCGETC, &cmdtchars) < 0)
    return SCPE_TTIERR;
if (ioctl (0, TIOCGLTC, &cmdltchars) < 0)
    return SCPE_TTIERR;
runtty              = cmdtty;                            /* initial run state */
runtty.sg_flags     = cmdtty.sg_flags & ~(ECHO|CRMOD) | CBREAK;
runtchars.t_intrc   = sim_int_char;                      /* interrupt */
runtchars.t_quitc   = 0xFF;                              /* no quit */
runtchars.t_startc  = 0xFF;                              /* no host sync */
runtchars.t_stopc   = 0xFF;
runtchars.t_eofc    = 0xFF;
runtchars.t_brkc    = 0xFF;
runltchars.t_suspc  = 0xFF;                              /* no specials of any kind */
runltchars.t_dsuspc = 0xFF;
runltchars.t_rprntc = 0xFF;
runltchars.t_flushc = 0xFF;
runltchars.t_werasc = 0xFF;
runltchars.t_lnextc = 0xFF;
return SCPE_OK;                                          /* return success */
}

static t_stat sim_os_ttrun (void)
{
runtchars.t_intrc = sim_int_char;                        /* in case changed */
(void)fcntl (0, F_SETFL, runfl);                         /* non-block mode */
if (ioctl (0, TIOCSETP, &runtty) < 0)
    return SCPE_TTIERR;
if (ioctl (0, TIOCSETC, &runtchars) < 0)
    return SCPE_TTIERR;
if (ioctl (0, TIOCSLTC, &runltchars) < 0)
    return SCPE_TTIERR;
return SCPE_OK;
}

static t_stat sim_os_ttcmd (void)
{
(void)fcntl (0, F_SETFL, cmdfl);                         /* block mode */
if (ioctl (0, TIOCSETP, &cmdtty) < 0)
    return SCPE_TTIERR;
if (ioctl (0, TIOCSETC, &cmdtchars) < 0)
    return SCPE_TTIERR;
if (ioctl (0, TIOCSLTC, &cmdltchars) < 0)
    return SCPE_TTIERR;
return SCPE_OK;
}

static t_stat sim_os_ttclose (void)
{
return sim_ttcmd ();
}

static t_bool sim_os_ttisatty (void)
{
return isatty (fileno (stdin));
}

static t_stat sim_os_poll_kbd (void)
{
int status;
unsigned char buf[1];

status = read (0, buf, 1);
if (status != 1) return SCPE_OK;
if (sim_brk_char && (buf[0] == sim_brk_char))
    return SCPE_BREAK;
if (sim_int_char && (buf[0] == sim_int_char))
    return SCPE_STOP;
return (buf[0] | SCPE_KFLAG);
}

static t_stat sim_os_putchar (int32 out)
{
char c;

if (!sim_localopc)
  return SCPE_OK;

c = out;
if (write (1, &c, 1)) {};
return SCPE_OK;
}

/* POSIX UNIX routines, from Leendert Van Doorn */

#else

# include <termios.h>
# include <unistd.h>

struct termios cmdtty, runtty;

static t_stat sim_os_ttinit (void)
{
if (!isatty (fileno (stdin)))                                  /* skip if !tty */
    return SCPE_OK;
if (tcgetattr (0, &cmdtty) < 0)                                /* get old flags */
    return SCPE_TTIERR;
runtty                = cmdtty;
runtty.c_lflag        = runtty.c_lflag & ~(ECHO | ICANON);     /* no echo or edit */
runtty.c_oflag        = runtty.c_oflag & ~OPOST;               /* no output edit */
runtty.c_iflag        = runtty.c_iflag & ~ICRNL;               /* no cr conversion */
runtty.c_cc[VINTR]    = sim_int_char;                          /* interrupt */
runtty.c_cc[VQUIT]    = 0;                                     /* no quit */
runtty.c_cc[VERASE]   = 0;
runtty.c_cc[VKILL]    = 0;
runtty.c_cc[VEOF]     = 0;
runtty.c_cc[VEOL]     = 0;
runtty.c_cc[VSTART]   = 0;                                     /* no host sync */
runtty.c_cc[VSUSP]    = 0;
runtty.c_cc[VSTOP]    = 0;
# if defined (VREPRINT)
runtty.c_cc[VREPRINT] = 0;                                     /* no specials */
# endif
# if defined (VDISCARD)
runtty.c_cc[VDISCARD] = 0;
# endif
# if defined (VWERASE)
runtty.c_cc[VWERASE]  = 0;
# endif
# if defined (VLNEXT)
runtty.c_cc[VLNEXT]   = 0;
# endif
runtty.c_cc[VMIN]     = 0;                                     /* no waiting */
runtty.c_cc[VTIME]    = 0;
# if defined (VDSUSP)
runtty.c_cc[VDSUSP]   = 0;
# endif
# if defined (VSTATUS)
runtty.c_cc[VSTATUS]  = 0;
# endif
return SCPE_OK;
}

static t_stat sim_os_ttrun (void)
{
if (!isatty (fileno (stdin)))                           /* skip if !tty */
    return SCPE_OK;
runtty.c_cc[VINTR] = sim_int_char;                      /* in case changed */
# if defined(__ANDROID__)
#  define TCSA_TYPE TCSANOW
(void)fflush(stdout);
(void)fflush(stderr);
# else
#  define TCSA_TYPE TCSAFLUSH
# endif
if (tcsetattr (0, TCSA_TYPE, &runtty) < 0)
    return SCPE_TTIERR;
return SCPE_OK;
}

static t_stat sim_os_ttcmd (void)
{
if (!isatty (fileno (stdin)))                           /* skip if !tty */
    return SCPE_OK;
# if defined(__ANDROID__)
(void)fflush(stdout);
(void)fflush(stderr);
# endif
if (tcsetattr (0, TCSA_TYPE, &cmdtty) < 0)
    return SCPE_TTIERR;
return SCPE_OK;
}

static t_stat sim_os_ttclose (void)
{
return sim_ttcmd ();
}

static t_bool sim_os_ttisatty (void)
{
return isatty (fileno (stdin));
}

static t_stat sim_os_poll_kbd (void)
{
int status;
unsigned char buf[1];

status = read (0, buf, 1);
if (status != 1) return SCPE_OK;
if (sim_brk_char && (buf[0] == sim_brk_char))
    return SCPE_BREAK;
if (sim_int_char && (buf[0] == sim_int_char))
    return SCPE_STOP;
return (buf[0] | SCPE_KFLAG);
}

static t_stat sim_os_putchar (int32 out)
{
char c;

if (!sim_localopc)
  return SCPE_OK;

c = out;
(void)!write (1, &c, 1);
return SCPE_OK;
}

#endif

/* Decode a string.

   A string containing encoded control characters is decoded into the equivalent
   character string.  Escape targets @, A-Z, and [\]^_ form control characters
   000-037.
*/
#define ESC_CHAR '~'

static void decode (char *decoded, const char *encoded)
{
char c;

/*LINTED E_EQUALITY_NOT_ASSIGNMENT*/
while ((c = *decoded++ = *encoded++))                   /* copy the character */
    if (c == ESC_CHAR) {                                /* does it start an escape? */
        if ((isalpha ((unsigned char)*encoded)) ||      /* is next character "A-Z" or "a-z"? */
            (*encoded == '@') ||                        /*   or "@"? */
            ((*encoded >= '[') && (*encoded <= '_')))   /*   or "[\]^_"? */

            *(decoded - 1) = *encoded++ & 037;          /* convert back to control character */
        else {
            if ((*encoded == '\0') ||                   /* single escape character at EOL? */
                 (*encoded++ != ESC_CHAR))              /*   or not followed by another escape? */
                decoded--;                              /* drop the encoding */
            }
        }
return;
}

/* Set console halt */

static t_stat sim_set_halt (int32 flag, CONST char *cptr)
{
if (flag == 0)                                              /* no halt? */
    sim_exp_clrall (&sim_con_expect);                       /* disable halt checks */
else {
    char *mbuf;
    char *mbuf2;

    if (cptr == NULL || *cptr == 0)                         /* no match string? */
        return SCPE_2FARG;                                  /* need an argument */

    sim_exp_clrall (&sim_con_expect);                       /* make sure that none currently exist */

    mbuf = (char *)malloc (1 + strlen (cptr));
    if (!mbuf)
      {
        fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                 __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
        abort();
      }
    decode (mbuf, cptr);                                    /* save decoded match string */

    mbuf2 = (char *)malloc (3 + strlen(cptr));
    if (!mbuf2)
      {
        fprintf (stderr, "\rFATAL: Out out memory! Aborting at %s[%s:%d]\r\n",
                 __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
        abort();
      }
    (void)sprintf (mbuf2, "%s%s%s",
                   (sim_switches & SWMASK ('A')) ? "\r\n" : "", mbuf,
                   (sim_switches & SWMASK ('I')) ? ""   : "\r\n");
    FREE (mbuf);
    mbuf = sim_encode_quoted_string ((uint8 *)mbuf2, strlen (mbuf2));
    sim_exp_set (&sim_con_expect, mbuf, 0, sim_con_expect.after, EXP_TYP_PERSIST, NULL);
    FREE (mbuf);
    FREE (mbuf2);
    }

return SCPE_OK;
}

/* Set console response */

static t_stat sim_set_response (int32 flag, CONST char *cptr)
{
if (flag == 0)                                          /* no response? */
    sim_send_clear (&sim_con_send);
else {
    uint8 *rbuf;

    if (cptr == NULL || *cptr == 0)
        return SCPE_2FARG;                              /* need arg */

    rbuf = (uint8 *)malloc (1 + strlen(cptr));
    if (!rbuf)
      {
        fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                 __func__, __FILE__, __LINE__);
#if defined(USE_BACKTRACE)
# if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
# endif /* if defined(SIGUSR2) */
#endif /* if defined(USE_BACKTRACE) */
        abort();
      }

    decode ((char *)rbuf, cptr);                        /* decod string */
    sim_send_input (&sim_con_send, rbuf, strlen((char *)rbuf), 0, 0); /* queue it for output */
    FREE (rbuf);
    }

return SCPE_OK;
}

/* Set console delay */

static t_stat sim_set_delay (int32 flag, CONST char *cptr)
{
int32 val;
t_stat r;

if (cptr == NULL || *cptr == 0)                         /* no argument string? */
    return SCPE_2FARG;                                  /* need an argument */

val = (int32) get_uint (cptr, 10, INT_MAX, &r);         /* parse the argument */

if (r == SCPE_OK)                                       /* parse OK? */
    sim_con_expect.after = val;                         /* save the delay value */

return r;
}
