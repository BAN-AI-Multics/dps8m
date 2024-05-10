/*
 * scp.h: simulator control program headers
 *
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: MIT
 * scspell-id: 9166cfe3-f62a-11ec-beb9-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 1993-2009 Robert M. Supnik
 * Copyright (c) 2021-2023 Jeffrey H. Johnson
 * Copyright (c) 2021-2024 The DPS8M Development Team
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

#if !defined(SIM_SCP_H_)
# define SIM_SCP_H_      0

/* run_cmd parameters */

# define RU_RUN          0                          /* run               */
# define RU_GO           1                          /* go                */
# define RU_STEP         2                          /* step              */
# define RU_NEXT         3                          /* step or step/over */
# define RU_CONT         4                          /* continue          */
# define RU_BOOT         5                          /* boot              */

/* exdep_cmd parameters */

# define EX_D            0                          /* deposit     */
# define EX_E            1                          /* examine     */
# define EX_I            2                          /* interactive */

/* brk_cmd parameters */

# define SSH_ST          0                          /* set   */
# define SSH_SH          1                          /* show  */
# define SSH_CL          2                          /* clear */

/* get_sim_opt parameters */

# define CMD_OPT_SW      001                        /* switches    */
# define CMD_OPT_OF      002                        /* output file */
# define CMD_OPT_SCH     004                        /* search      */
# define CMD_OPT_DFT     010                        /* defaults    */

/* Command processors */

t_stat reset_cmd (int32 flag, CONST char *ptr);
t_stat exdep_cmd (int32 flag, CONST char *ptr);
t_stat eval_cmd (int32 flag, CONST char *ptr);
t_stat load_cmd (int32 flag, CONST char *ptr);
t_stat run_cmd (int32 flag, CONST char *ptr);
void run_cmd_message (const char *unechod_cmdline, t_stat r);
t_stat attach_cmd (int32 flag, CONST char *ptr);
t_stat detach_cmd (int32 flag, CONST char *ptr);
# if 0 /* Needs updating for dps8 */
t_stat assign_cmd (int32 flag, CONST char *ptr);
t_stat deassign_cmd (int32 flag, CONST char *ptr);
# endif /* Needs updating for dps8 */
t_stat exit_cmd (int32 flag, CONST char *ptr);
t_stat set_cmd (int32 flag, CONST char *ptr);
t_stat show_cmd (int32 flag, CONST char *ptr);
t_stat brk_cmd (int32 flag, CONST char *ptr);
t_stat do_cmd (int32 flag, CONST char *ptr);
t_stat goto_cmd (int32 flag, CONST char *ptr);
t_stat return_cmd (int32 flag, CONST char *ptr);
t_stat shift_cmd (int32 flag, CONST char *ptr);
t_stat call_cmd (int32 flag, CONST char *ptr);
t_stat on_cmd (int32 flag, CONST char *ptr);
t_stat noop_cmd (int32 flag, CONST char *ptr);
t_stat assert_cmd (int32 flag, CONST char *ptr);
t_stat send_cmd (int32 flag, CONST char *ptr);
t_stat expect_cmd (int32 flag, CONST char *ptr);
t_stat help_cmd (int32 flag, CONST char *ptr);
t_stat screenshot_cmd (int32 flag, CONST char *ptr);
t_stat spawn_cmd (int32 flag, CONST char *ptr);
t_stat echo_cmd (int32 flag, CONST char *ptr);

/* Allow compiler to help validate printf style format arguments */
# if !defined (__GNUC__)
#  define GCC_FMT_ATTR(n, m)
# endif

# if !defined(GCC_FMT_ATTR)
#  define GCC_FMT_ATTR(n, m) __attribute__ ((format (__printf__, n, m)))
# endif

/* Utility routines */

t_stat sim_process_event (void);
t_stat sim_activate (UNIT *uptr, int32 interval);
t_stat _sim_activate (UNIT *uptr, int32 interval);
t_stat sim_activate_abs (UNIT *uptr, int32 interval);
t_stat sim_activate_after (UNIT *uptr, uint32 usecs_walltime);
t_stat _sim_activate_after (UNIT *uptr, uint32 usecs_walltime);
t_stat sim_cancel (UNIT *uptr);
t_bool sim_is_active (UNIT *uptr);
int32 sim_activate_time (UNIT *uptr);
t_stat sim_run_boot_prep (int32 flag);
double sim_gtime (void);
uint32 sim_grtime (void);
int32 sim_qcount (void);
t_stat attach_unit (UNIT *uptr, CONST char *cptr);
t_stat detach_unit (UNIT *uptr);
t_stat assign_device (DEVICE *dptr, const char *cptr);
t_stat deassign_device (DEVICE *dptr);
t_stat reset_all (uint32 start_device);
t_stat reset_all_p (uint32 start_device);
const char *sim_dname (DEVICE *dptr);
const char *sim_uname (UNIT *dptr);
int sim_isspace (char c);
int sim_islower (char c);
int sim_isalpha (char c);
int sim_isprint (char c);
int sim_isdigit (char c);
int sim_isgraph (char c);
int sim_isalnum (char c);
CONST char *get_sim_opt (int32 opt, CONST char *cptr, t_stat *st);
const char *put_switches (char *buf, size_t bufsize, uint32 sw);
CONST char *get_glyph (const char *iptr, char *optr, char mchar);
CONST char *get_glyph_nc (const char *iptr, char *optr, char mchar);
CONST char *get_glyph_quoted (const char *iptr, char *optr, char mchar);
CONST char *get_glyph_cmd (const char *iptr, char *optr);
t_value get_uint (const char *cptr, uint32 radix, t_value max, t_stat *status);
CONST char *get_range (DEVICE *dptr, CONST char *cptr, t_addr *lo, t_addr *hi,
    uint32 rdx, t_addr max, char term);
t_stat sim_decode_quoted_string (const char *iptr, uint8 *optr, uint32 *osize);
char *sim_encode_quoted_string (const uint8 *iptr, size_t size);
void fprint_buffer_string (FILE *st, const uint8 *buf, size_t size);
t_value strtotv (CONST char *cptr, CONST char **endptr, uint32 radix);
int Fprintf (FILE *f, const char *fmt, ...) GCC_FMT_ATTR(2, 3);
t_stat fprint_val (FILE *stream, t_value val, uint32 rdx, uint32 wid, uint32 fmt);
t_stat sprint_val (char *buf, t_value val, uint32 rdx, size_t wid, uint32 fmt);
const char *sim_fmt_secs (double seconds);
const char *sprint_capac (DEVICE *dptr, UNIT *uptr);
char *read_line (char *cptr, int32 size, FILE *stream);
void fprint_reg_help (FILE *st, DEVICE *dptr);
void fprint_set_help (FILE *st, DEVICE *dptr);
void fprint_show_help (FILE *st, DEVICE *dptr);
CTAB *find_cmd (const char *gbuf);
DEVICE *find_dev (const char *ptr);
DEVICE *find_unit (const char *ptr, UNIT **uptr);
DEVICE *find_dev_from_unit (UNIT *uptr);
t_stat sim_register_internal_device (DEVICE *dptr);
void sim_sub_args (char *in_str, size_t in_str_size, char *do_arg[]);
REG *find_reg (CONST char *ptr, CONST char **optr, DEVICE *dptr);
CTAB *find_ctab (CTAB *tab, const char *gbuf);
C1TAB *find_c1tab (C1TAB *tab, const char *gbuf);
SHTAB *find_shtab (SHTAB *tab, const char *gbuf);
t_stat get_aval (t_addr addr, DEVICE *dptr, UNIT *uptr);
BRKTAB *sim_brk_fnd (t_addr loc);
uint32 sim_brk_test (t_addr bloc, uint32 btyp);
void sim_brk_clrspc (uint32 spc, uint32 btyp);
void sim_brk_npc (uint32 cnt);
void sim_brk_setact (const char *action);
const char *sim_brk_message(void);
t_stat sim_send_input (SEND *snd, uint8 *data, size_t size, uint32 after, uint32 delay);
t_stat sim_show_send_input (FILE *st, const SEND *snd);
t_bool sim_send_poll_data (SEND *snd, t_stat *stat);
t_stat sim_send_clear (SEND *snd);
t_stat sim_set_expect (EXPECT *exp, CONST char *cptr);
t_stat sim_set_noexpect (EXPECT *exp, const char *cptr);
t_stat sim_exp_set (EXPECT *exp, const char *match, int32 cnt, uint32 after, int32 switches, const char *act);
t_stat sim_exp_clr (EXPECT *exp, const char *match);
t_stat sim_exp_clrall (EXPECT *exp);
t_stat sim_exp_show (FILE *st, CONST EXPECT *exp, const char *match);
t_stat sim_exp_showall (FILE *st, const EXPECT *exp);
t_stat sim_exp_check (EXPECT *exp, uint8 data);
t_stat show_version (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat set_dev_debug (DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat show_dev_debug (FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
const char *sim_error_text (t_stat stat);
t_stat sim_string_to_stat (const char *cptr, t_stat *cond);
void sim_printf (const char *fmt, ...) GCC_FMT_ATTR(1, 2);
t_stat sim_messagef (t_stat stat, const char *fmt, ...) GCC_FMT_ATTR(2, 3);
void sim_data_trace(DEVICE *dptr, UNIT *uptr, const uint8 *data, const char *position, size_t len, const char *txt, uint32 reason);
void sim_debug_bits_hdr (uint32 dbits, DEVICE* dptr, const char *header,
    BITFIELD* bitdefs, uint32 before, uint32 after, int terminate);
void sim_debug_bits (uint32 dbits, DEVICE* dptr, BITFIELD* bitdefs,
    uint32 before, uint32 after, int terminate);
void _sim_debug (uint32 dbits, DEVICE* dptr, const char *fmt, ...) GCC_FMT_ATTR(3, 4);
# define sim_debug(dbits, dptr, ...)                                           \
    do {                                                                       \
         if ((sim_deb != NULL) && ((dptr != NULL) && ((dptr)->dctrl & dbits))) \
         _sim_debug (dbits, dptr, __VA_ARGS__);                                \
       }                                                                       \
    while (0)
void fprint_stopped_gen (FILE *st, t_stat v, REG *pc, DEVICE *dptr);
# define SCP_HELP_FLAT   (1u << 31)       /* Force flat help when prompting is not possible */
# define SCP_HELP_ONECMD (1u << 30)       /* Display one topic, do not prompt */
# define SCP_HELP_ATTACH (1u << 29)       /* Top level topic is ATTACH help */
t_stat scp_help (FILE *st, DEVICE *dptr,
                 UNIT *uptr, int32 flag, const char *help, const char *cptr, ...);
t_stat scp_vhelp (FILE *st, DEVICE *dptr,
                  UNIT *uptr, int32 flag, const char *help, const char *cptr, va_list ap);
t_stat scp_helpFromFile (FILE *st, DEVICE *dptr,
                         UNIT *uptr, int32 flag, const char *help, const char *cptr, ...);
t_stat scp_vhelpFromFile (FILE *st, DEVICE *dptr,
                          UNIT *uptr, int32 flag, const char *help, const char *cptr, va_list ap);

/* Global data */

extern DEVICE *sim_dflt_dev;
extern int32 sim_interval;
extern int32 sim_switches;
extern int32 sim_iglock;
extern int32 sim_nolock;
extern int32 sim_randompst;
extern int32 sim_quiet;
extern int32 sim_localopc;
extern int32 sim_randstate;
extern int32 sim_nostate;
extern int32 sim_step;
extern t_stat sim_last_cmd_stat;                        /* Command Status */
extern FILE *sim_log;                                   /* log file */
extern FILEREF *sim_log_ref;                            /* log file file reference */
extern FILE *sim_deb;                                   /* debug file */
extern FILEREF *sim_deb_ref;                            /* debug file file reference */
extern int32 sim_deb_switches;                          /* debug display flags */
extern struct timespec sim_deb_basetime;                /* debug base time for relative time output */
extern DEVICE **sim_internal_devices;
extern uint32 sim_internal_device_count;
extern UNIT *sim_clock_queue;
extern int32 sim_is_running;
extern t_bool sim_processing_event;                     /* Called from sim_process_event */
extern char *sim_prompt;                                /* prompt string */
extern const char *sim_savename;                        /* Simulator Name used in Save/Restore files */
extern t_value *sim_eval;
extern volatile int32 stop_cpu;
extern uint32 sim_brk_types;                            /* breakpoint info */
extern uint32 sim_brk_dflt;
extern uint32 sim_brk_summ;
extern uint32 sim_brk_match_type;
extern t_addr sim_brk_match_addr;
extern BRKTYPTAB *sim_brk_type_desc;                      /* type descriptions */
extern FILE *stdnul;
extern t_bool sim_asynch_enabled;

/* VM interface */

extern char sim_name[];
extern DEVICE *sim_devices[];
extern REG *sim_PC;
extern const char *sim_stop_messages[];
extern t_stat sim_instr (void);
extern int32 sim_emax;
extern t_stat fprint_sym (FILE *ofile, t_addr addr, t_value *val,
    UNIT *uptr, int32 sw);
extern t_stat parse_sym (CONST char *cptr, t_addr addr, UNIT *uptr, t_value *val,
    int32 sw);

/* The per-simulator init routine is a weak global that defaults to NULL
   The other per-simulator pointers can be overriden by the init routine */

extern void (*sim_vm_init) (void);
extern char *(*sim_vm_read) (char *ptr, int32 size, FILE *stream);
extern void (*sim_vm_post) (t_bool from_scp);
extern CTAB *sim_vm_cmd;
extern void (*sim_vm_sprint_addr) (char *buf, DEVICE *dptr, t_addr addr);
extern void (*sim_vm_fprint_addr) (FILE *st, DEVICE *dptr, t_addr addr);
extern t_addr (*sim_vm_parse_addr) (DEVICE *dptr, CONST char *cptr, CONST char **tptr);
extern t_bool (*sim_vm_fprint_stopped) (FILE *st, t_stat reason);
extern t_value (*sim_vm_pc_value) (void);
extern t_bool (*sim_vm_is_subroutine_call) (t_addr **ret_addrs);
extern const char *xstrerror_l(int errnum);

# define SIM_FRONTPANEL_VERSION   2

/**

    sim_panel_start_simulator       A starts a simulator with a particular
                                    configuration

        sim_path            the path to the simulator binary
        sim_config          the configuration to run the simulator with
        device_panel_count  the number of sub panels for connected devices

    Note 1: - The path specified must be either a fully specified path or
              it could be merely the simulator name if the simulator binary
              is located in the current PATH.
            - The simulator binary must be built from the same version
              simh source code that the front-panel API was acquired from
              (the API and the simh framework must speak the same language)

    Note 2: - Configuration file specified should contain device setup
              statements (enable, disable, CPU types and attach commands).
              It should not start a simulator running.

 */

typedef struct PANEL PANEL;

PANEL *
sim_panel_start_simulator (const char *sim_path,
                           const char *sim_config,
                           size_t device_panel_count);

PANEL *
sim_panel_start_simulator_debug (const char *sim_path,
                                 const char *sim_config,
                                 size_t device_panel_count,
                                 const char *debug_file);

/**

    sim_panel_add_device_panel - creates a sub panel associated
                                 with a specific simulator panel

        simulator_panel     the simulator panel to connect to
        device_name         the simulator's name for the device

 */
PANEL *
sim_panel_add_device_panel (PANEL *simulator_panel,
                            const char *device_name);

/**

    sim_panel_destroy   to shutdown a panel or sub panel.

    Note: destroying a simulator panel will also destroy any
          related sub panels

 */
int
sim_panel_destroy (PANEL *panel);

/**

   The front-panel API exposes the state of a simulator via access to
   simh register variables that the simulator and its devices define.
   These registers certainly include any architecturally described
   registers (PC, PSL, SP, etc.), but also include anything else
   the simulator uses as internal state to implement the running
   simulator.

   The registers that a particular front-panel application might need
   access to are described by the application by calling:

   sim_panel_add_register
   sim_panel_add_register_array
and
   sim_panel_add_register_indirect

        name         the name the simulator knows this register by
        device_name  the device this register is part of.  Defaults to
                     the device of the panel (in a device panel) or the
                     default device in the simulator (usually the CPU).
        element_count number of elements in the register array
        size         the size (in local storage) of the buffer which will
                     receive the data in the simulator's register
        addr         a pointer to the location of the buffer which will
                     be loaded with the data in the simulator's register

 */
int
sim_panel_add_register (PANEL *panel,
                        const char *name,
                        const char *device_name,
                        size_t size,
                        void *addr);

int
sim_panel_add_register_array (PANEL *panel,
                              const char *name,
                              const char *device_name,
                              size_t element_count,
                              size_t size,
                              void *addr);

int
sim_panel_add_register_indirect (PANEL *panel,
                                 const char *name,
                                 const char *device_name,
                                 size_t size,
                                 void *addr);
/**

    A panel application has a choice of two different methods of getting
    the values contained in the set of registers it has declared interest in via
    the sim_panel_add_register API.

       1)  The values can be polled (when ever it is desired) by calling
           sim_panel_get_registers().
       2)  The panel can call sim_panel_set_display_callback() to specify a
           callback routine and a periodic rate that the callback routine
           should be called.  The panel API will make a best effort to deliver
           the current register state at the desired rate.

   Note 1: The buffers described in a panel's register set will be
           dynamically revised as soon as data is available from the
           simulator.  The callback routine merely serves as a notification
           that a complete register set has arrived.
   Note 2: The callback routine should, in general, not run for a long time
           or front-panel interactions with the simulator may be disrupted.
           Setting a flag, signaling an event or posting a message are
           reasonable activities to perform in a callback routine.

 */
int
sim_panel_get_registers (PANEL *panel, unsigned long long *simulation_time);

typedef void (*PANEL_DISPLAY_PCALLBACK)(PANEL *panel,
                                        unsigned long long simulation_time,
                                        void *context);

int
sim_panel_set_display_callback (PANEL *panel,
                                PANEL_DISPLAY_PCALLBACK callback,
                                void *context,
                                int callbacks_per_second);

/**

    When a front panel application needs to change the running
    state of a simulator one of the following routines should
    be called:

    sim_panel_exec_halt     - Stop instruction execution
    sim_panel_exec_boot     - Boot a simulator from a specific device
    sim_panel_exec_run      - Start/Resume a simulator running instructions
    sim_panel_exec_step     - Have a simulator execute a single step

 */
int
sim_panel_exec_halt (PANEL *panel);

int
sim_panel_exec_boot (PANEL *panel, const char *device);

int
sim_panel_exec_run (PANEL *panel);

int
sim_panel_exec_step (PANEL *panel);

/**

    When a front panel application wants to describe conditions that
    should stop instruction execution an execution or an output
    should be used.  To established or clear a breakpoint, one of
    the following routines should be called:

    sim_panel_break_set          - Establish a simulation breakpoint
    sim_panel_break_clear        - Cancel/Delete a previously defined
                                   breakpoint
    sim_panel_break_output_set   - Establish a simulator output
                                   breakpoint
    sim_panel_break_output_clear - Cancel/Delete a previously defined
                                   output breakpoint

    Note: Any breakpoint switches/flags must be located at the
          beginning of the condition string

 */

int
sim_panel_break_set (PANEL *panel, const char *condition);

int
sim_panel_break_clear (PANEL *panel, const char *condition);

int
sim_panel_break_output_set (PANEL *panel, const char *condition);

int
sim_panel_break_output_clear (PANEL *panel, const char *condition);

/**

    When a front panel application needs to change or access
    memory or a register one of the following routines should
    be called:

    sim_panel_gen_examine        - Examine register or memory
    sim_panel_gen_deposit        - Deposit to register or memory
    sim_panel_mem_examine        - Examine memory location
    sim_panel_mem_deposit        - Deposit to memory location
    sim_panel_set_register_value - Deposit to a register or memory
                                   location

 */

/**

   sim_panel_gen_examine

        name_or_addr the name the simulator knows this register by
        size         the size (in local storage) of the buffer which will
                     receive the data returned when examining the simulator
        value        a pointer to the buffer which will be loaded with the
                     data returned when examining the simulator
 */

int
sim_panel_gen_examine (PANEL *panel,
                       const char *name_or_addr,
                       size_t size,
                       void *value);
/**

   sim_panel_gen_deposit

        name_or_addr the name the simulator knows this register by
        size         the size (in local storage) of the buffer which
                     contains the data to be deposited into the simulator
        value        a pointer to the buffer which contains the data to
                     be deposited into the simulator
 */

int
sim_panel_gen_deposit (PANEL *panel,
                       const char *name_or_addr,
                       size_t size,
                       const void *value);

/**

   sim_panel_mem_examine

        addr_size    the size (in local storage) of the buffer which
                     contains the memory address of the data to be examined
                     in the simulator
        addr         a pointer to the buffer containing the memory address
                     of the data to be examined in the simulator
        value_size   the size (in local storage) of the buffer which will
                     receive the data returned when examining the simulator
        value        a pointer to the buffer which will be loaded with the
                     data returned when examining the simulator
 */

int
sim_panel_mem_examine (PANEL *panel,
                       size_t addr_size,
                       const void *addr,
                       size_t value_size,
                       void *value);

/**

   sim_panel_mem_deposit

        addr_size    the size (in local storage) of the buffer which
                     contains the memory address of the data to be deposited
                     into the simulator
        addr         a pointer to the buffer containing the memory address
                     of the data to be deposited into the simulator
        value_size   the size (in local storage) of the buffer which will
                     contains the data to be deposited into the simulator
        value        a pointer to the buffer which contains the data to be
                     deposited into the simulator
 */

int
sim_panel_mem_deposit (PANEL *panel,
                       size_t addr_size,
                       const void *addr,
                       size_t value_size,
                       const void *value);

/**
   sim_panel_set_register_value

        name        the name of a simulator register or a memory address
                    which is to receive a new value
        value       the new value in character string form.  The string
                    must be in the native/natural radix that the simulator
                    uses when referencing that register

 */
int
sim_panel_set_register_value (PANEL *panel,
                              const char *name,
                              const char *value);

/**

    When a front panel application may needs to change the media
    in a simulated removable media device one of the following
    routines should be called:

    sim_panel_mount    - mounts the indicated media file on a device
    sim_panel_dismount - dismounts the currently mounted media file
                         from a device

 */

/**
   sim_panel_mount

        device      the name of a simulator device/unit
        switches    any switches appropriate for the desire attach
        path        the path on the local system to be attached

 */
int
sim_panel_mount (PANEL *panel,
                 const char *device,
                 const char *switches,
                 const char *path);

/**
   sim_panel_dismount

        device      the name of a simulator device/unit

 */
int
sim_panel_dismount (PANEL *panel,
                    const char *device);

typedef enum {
    Halt,       /* Simulation is halted (instructions not being executed) */
    Run,        /* Simulation is executing instructions */
    Error       /* Panel simulator is in an error state and should be */
                /* closed (destroyed).  sim_panel_get_error might help */
                /* explain why */
    } OperationalState;

OperationalState
sim_panel_get_state (PANEL *panel);

/**

    All APIs routines which return an int return 0 for
    success and -1 for an error.

    An API which returns an error (-1), will not change the panel state.

    sim_panel_get_error     - the details of the most recent error
    sim_panel_clear_error   - clears the error buffer

 */

const char *sim_panel_get_error (void);
void sim_panel_clear_error (void);

/**

    The panel<->simulator wire protocol can be traced if protocol problems arise.

    sim_panel_set_debug_file    - Specifies the log file to record debug traffic
    sim_panel_set_debug_mode    - Specifies the debug detail to be recorded
    sim_panel_flush_debug       - Flushes debug output to disk

 */
void
sim_panel_set_debug_file (PANEL *panel, const char *debug_file);

void
sim_panel_set_debug_mode (PANEL *panel, int debug_bits);

void
sim_panel_flush_debug (PANEL *panel);

#endif
