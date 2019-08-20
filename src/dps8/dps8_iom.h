/*
 Copyright (c) 2007-2013 Michael Mondy
 Copyright 2012-2016 by Harry Reed
 Copyright 2013-2016 by Charles Anthony

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

#ifdef IO_THREADZ
extern __thread uint this_iom_idx;
extern __thread uint this_chan_num;
//extern __thread bool thisIOMHaveLock;
#endif

// Boot device: CARD/TAPE;
enum config_sw_bootlood_device_e { CONFIG_SW_BLCT_CARD, CONFIG_SW_BLCT_TAPE };



enum config_sw_OS_t
  {
    CONFIG_SW_STD_GCOS, 
    CONFIG_SW_EXT_GCOS,
    CONFIG_SW_MULTICS  // "Paged"
  };
    
enum config_sw_model_t
  {
    CONFIG_SW_MODEL_IOM, 
    CONFIG_SW_MODEL_IMU 
  };

// typedef enum iom_status_t
//   {
//     iom_stat_normal = 0,
//     iomStatLPWTRO = 1,
//     iomStat2TDCW = 2,
//     iomStatBoundaryError = 3,
//     iomStatAERestricted = 4,
//     iomStatIDCWRestricted = 5,
//     iomStatCPDiscrepancy = 6,
//     iomStatParityErr = 7
//   } iom_status_t;


struct iom_unit_switches
  {
    bool power;

    // Interrupt multiplex base address: 12 toggles
    word12 interrupt_base_address;

    // Mailbox base aka IOM base address: 9 toggles
    // Note: The IOM number is encoded in the lower two bits

    // AM81, pg 60 shows an image of a Level 68 IOM configuration panel
    // The switches are arranged and labeled
    //
    //  12   13   14   15   16   17   18   --   --  --  IOM
    //                                                  NUMBER
    //   X    X    X    X    X    X    X                X     X
    //
    word9 mbx_base_address;
            
    // Not an actual switch...
    enum config_sw_model_t model; // IOM or IMU

    // OS: Three position switch: GCOS, EXT GCOS, Multics
    enum config_sw_OS_t os; // = CONFIG_SW_MULTICS;

    // Bootload device: Toggle switch CARD/TAPE
    enum config_sw_bootlood_device_e boot_card_tape; // = CONFIG_SW_BLCT_TAPE; 

    // Bootload tape IOM channel: 6 toggles
    word6 boot_tape_chan; // = 0; 

    // Bootload cardreader IOM channel: 6 toggles
    word6 boot_card_chan; // = 1;

    // Bootload: pushbutton

    // Sysinit: pushbutton

    // Bootload SCU port: 3 toggle AKA "ZERO BASE S.C. PORT NO"
    // "the port number of the SC through which which connects are to
    // be sent to the IOM
    word3 boot_port; // = 0; // was configSwBootloadPort; 

    // 8 Ports: CPU/IOM connectivity
    // Port configuration: 3 toggles/port 
    // Which SCU number is this port attached to 
    uint port_address[N_IOM_PORTS]; // = { 0, 1, 2, 3, 4, 5, 6, 7 }; 

    // Port interlace: 1 toggle/port
    uint port_interlace[N_IOM_PORTS]; // = { 0, 0, 0, 0, 0, 0, 0, 0 };

    // Port enable: 1 toggle/port
    uint port_enable[N_IOM_PORTS]; // = { 0, 0, 0, 0, 0, 0, 0, 0 };

    // Port system initialize enable: 1 toggle/port // XXX What is this
    uint port_init_enable[N_IOM_PORTS]; // = { 0, 0, 0, 0, 0, 0, 0, 0 };

    // Port half-size: 1 toggle/port // XXX what is this
    uint port_half_size[N_IOM_PORTS]; // = { 0, 0, 0, 0, 0, 0, 0, 0 }; 

    // Port store size: 1 8 pos. rotary/port
    uint port_store_size[N_IOM_PORTS]; // = { 0, 0, 0, 0, 0, 0, 0, 0 };

    // other switches:
    //   alarm disable
    //   test/normal

  };

struct iom_unit_data_s
  {
    // Configuration switches
    struct iom_unit_switches switches;

    // iom_status_t iomStatus;

    uint invoking_scu_unit_idx; // the unit number of the SCU that did the connect.

    // IMUs have a boot drive number
    uint boot_drive;
  };
extern struct iom_unit_data_s iom_unit_data[N_IOM_UNITS_MAX];

extern DEVICE iom_dev;


//
// iom_cmd_t returns:
//
//  0: ok
//  1; ignored cmd, drop connect.
//  2: did command, don't do DCW list
//  3; command pending, don't sent terminate interrupt
// -1: error

#define IOM_CMD_OK	0
#define IOM_CMD_IGNORED 1
#define IOM_CMD_NO_DCW	2
#define IOM_CMD_PENDING 3
#define IOM_CMD_ERROR	-1

typedef int iom_cmd_t (uint iom_unit_idx, uint chan);
void iom_init (void);
#ifdef PANEL
void do_boot (void);
#endif
#ifdef IO_THREADZ
void * iom_thread_main (void * arg);
void * chan_thread_main (void * arg);
#endif

t_stat boot2 (UNUSED int32 arg, UNUSED const char * buf);
t_stat iom_unit_reset_idx (uint iom_unit_idx);

#if defined(IO_ASYNC_PAYLOAD_CHAN) || defined(IO_ASYNC_PAYLOAD_CHAN_THREAD)
void iom_process (void);
#endif
#ifdef IO_THREADZ
#ifdef EARLY_CREATE
void create_iom_threads (void);
#endif // EARLY_CREATE
#endif // IO_THREADZ
uint get_iom_num (uint iom_unit_idx);
