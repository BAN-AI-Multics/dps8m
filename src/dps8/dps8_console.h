/*
 Copyright 2013-2016 by Charles Anthony

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

extern UNIT opc_unit [N_OPC_UNITS_MAX];
extern DEVICE opc_dev;

enum console_model { m6001 = 0, m6601 = 1 };

// Hangs off the device structure
typedef struct opc_state_t
  {
    char device_name [MAX_DEV_NAME_LEN];
    enum console_model model;
    enum console_mode { opc_no_mode, opc_read_mode, opc_write_mode } io_mode;
    // SIMH console library has only putc and getc; the SIMH terminal
    // library has more features including line buffering.
#define console_buf_size 81
    unsigned char buf[console_buf_size];
    unsigned char *tailp;
    unsigned char *readp;
    unsigned char *auto_input;
    unsigned char *autop;
    bool echo;
    
    // stuff saved from the Read ASCII command
    time_t startTime;
    uint tally;
    uint daddr;
    UNIT * unitp;
    int chan;

    // Generate "accept" command when dial_ctl announces dialin console
    int autoaccept;
    // Replace empty console input with "@"
    int noempty;
    // ATTN flushes typeahead buffer
    int attn_flush;
    // Run in background (don't read the keyboard).
    int bg;
    int rcpwatch;
    bool attn_pressed;
    bool simh_attn_pressed;
#define simh_buffer_sz 4096
    char simh_buffer[simh_buffer_sz];
    int simh_buffer_cnt;

    uv_access console_access;

    // ^T 
    //unsigned long keyboard_poll_cnt;

 } opc_state_t;

extern opc_state_t console_state[N_OPC_UNITS_MAX];

void console_init(void);
void console_attn_idx (int opc_unit_idx);
int add_opc_autoinput (int32 flag, const char * cptr);
int add_opc_autostream (int32 flag, const char * cptr);
int clear_opc_autoinput (int32 flag, const char * cptr);
int opc_iom_cmd (uint iomUnitIdx, uint chan);
int check_attn_key (void);
void consoleProcess (void);
t_stat set_console_port (UNUSED int32 arg, const char * buf);
t_stat set_console_address (UNUSED int32 arg, const char * buf);
t_stat set_console_pw (UNUSED int32 arg, const char * buf);
void startRemoteConsole (void);
