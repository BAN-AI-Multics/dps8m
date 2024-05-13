/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 8e66ce24-f62e-11ec-8690-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2022 Charles Anthony
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

// IDCW Instruction                   18-20  111
// TDCW Transfer                      18-20 !111,  22-23 10
// IOTD IO Transfer and disconnect    18-20 !111,  22-23 00
// IOTP IO Transfer and proceed       18-20 !111,  22-23 01
// IONTP IO Non-Transfer and proceed  18-20 !111,  22-23 11

#if defined(IO_THREADZ)
extern __thread uint this_iom_idx;
extern __thread uint this_chan_num;
//extern __thread bool thisIOMHaveLock;
#endif

//typedef enum
  //{
    //cm_LPW_init_state,       // No TDCWs encountered; state is:
                             ////    PCW64 (pcw64_pge): on   PAGE CHAN
                             ////    PCW64 (pcw64_pge): off  EXT MODE CHAN
    //cm_real_LPW_real_DCW,
    //cm_ext_LPW_real_DCW,
    //cm_paged_LPW_seg_DCW
  //} chanMode_t;

typedef enum chanStat
  {
    chanStatNormal          = 0,
    chanStatUnexpectedPCW   = 1,
    chanStatInvalidInstrPCW = 2,
    chanStatIncorrectDCW    = 3,
    chanStatIncomplete      = 4,
    chanStatUnassigned      = 5,
    chanStatParityErrPeriph = 6,
    chanStatParityErrBus    = 7
  } chanStat;

// Due to lack of documentation, chan_cmd is largely ignored
//
// iom_chan_control_words.incl.pl1
//
//   SINGLE_RECORD       init ("00"b3),
//   NONDATA             init ("02"b3),
//   MULTIRECORD         init ("06"b3),
//   SINGLE_CHARACTER    init ("10"b3)
//
// bound_tolts_/mtdsim_.pl1
//
//    idcw.chan_cmd = "40"b3;           /* otherwise set special cont. cmd */
//
// bound_io_tools/exercise_disk.pl1
//
//   idcw.chan_cmd = INHIB_AUTO_RETRY; /* inhibit mpc auto retries */
//   dcl     INHIB_AUTO_RETRY       bit (6) int static init ("010001"b);  // 021
//
// poll_mpc.pl1:
//  /* Build dcw list to get statistics from EURC MPC */
//  idcw.chan_cmd = "41"b3;            /* Indicate special controller command */
//  /* Build dcw list to get configuration and statistics from DAU MSP */
//  idcw.chan_cmd = "30"b3;                           /* Want list in dev# order */
//
// tape_ioi_io.pl1:
//   idcw.chan_cmd = "03"b3;          /* data security erase */
//   dcw.chan_cmd = "30"b3;           /* use normal values, auto-retry */

// iom_word_macros.incl.alm
//
// Channel control
#define CHAN_CTRL_TERMINATE 0
#define CHAN_CTRL_PROCEED   2
#define CHAN_CTRL_MARKER    3

// Channel command
#define CHAN_CMD_RECORD      0
#define CHAN_CMD_NONDATA     2
#define CHAN_CMD_MULTIRECORD 6
#define CHAN_CMD_CHARACTER   8

#define IS_IDCW(p)     ((p)->DCW_18_20_CP == 07)
#define IS_NOT_IDCW(p) ((p)->DCW_18_20_CP != 07)
#define IS_TDCW(p)     ((p)->DCW_18_20_CP !=  7 && (p)->DDCW_22_23_TYPE == 2)
#define IS_IOTD(p)     ((p)->DCW_18_20_CP !=  7 && (p)->DDCW_22_23_TYPE == 0)
#define IS_IONTP(p)    ((p)->DCW_18_20_CP !=  7 && (p)->DDCW_22_23_TYPE == 3)

// exercise_disk.pl1
#define CHAN_CMD_INHIB_AUTO_RETRY    021
// load_mpc.pl1
#define CHAN_CMD_SPECIAL_CTLR        040
// poll_mpc.pl1
#define CHAN_CMD_DEV_ORDER           030
#define CHAN_CMD_SPECIAL_CTLR2       041
// tape_ioi_io.pl1
#define CHAN_CMD_DATA_SECURITY_ERASE  03
#define CHAN_CMD_NORM_AUTO_TRY       030

typedef volatile struct
  {

// scratch pad

    // packed LPW
    word36 LPW;
    // unpacked LPW
    word18 LPW_DCW_PTR;
    word1 LPW_18_RES;
    word1 LPW_19_REL;
    word1 LPW_20_AE;
    word1 LPW_21_NC;
    word1 LPW_22_TAL;
    word1 LPW_23_REL;
    word12 LPW_TALLY;

    // packed LPWX
    word36 LPWX;
    // unpacked LPWX
    word18 LPWX_BOUND; // MOD 2 (pg B16) 0-2^19; ie val = LPX_BOUND * 2
    word18 LPWX_SIZE;  // MOD 1 (pg B16) 0-2^18

// PCW_63_PTP indicates paging mode; indicates that a page table
// is available.
// XXX pg B11: cleared by a terminate interrupt service with the
// character size bit of the transaction command = 1. (bit 32)
// what is the 'transaction command.?

    // packed PCW
    word36 PCW0, PCW1;
    // unpacked PCW
    word6 PCW_CHAN;
    word6 PCW_AE;
    // Pg B2: "Absolute location (MOD 64) of the channels Page Table"
    word18 PCW_PAGE_TABLE_PTR;
    word1 PCW_63_PTP;
    word1 PCW_64_PGE;
    word1 PCW_65_AUX; // XXX
    word1 PCW_21_MSK; // Sometimes called 'M' // see 3.2.2, pg 25

    // packed DCW
    word36 DCW;
    // unpacked DCW
    // TDCW only
    word18 TDCW_DATA_ADDRESS;
    word1  TDCW_34_RES;
    word1  TDCW_35_REL;
    // TDCW, PCW 64 = 0
    word1  TDCW_33_EC;
    // TDCW, PCW 64 = 1
    word1  TDCW_31_SEG;
    word1  TDCW_32_PDTA;
    word1  TDCW_33_PDCW;
    // IDCW only
    word6  IDCW_DEV_CMD;
    word6  IDCW_DEV_CODE;
    word6  IDCW_AE;
    word1  IDCW_EC;
    word2  IDCW_CHAN_CTRL; // 0 terminate, 2 process, 3 marker
    word6  IDCW_CHAN_CMD;
    // POLTS sets this to one if there are IOTxes.
    word6  IDCW_COUNT;
    // DDCW only
    /*word18*/ uint DDCW_ADDR; // Allow overflow detection
    word12 DDCW_TALLY;
    word2  DDCW_22_23_TYPE; // '2' indicates TDCW
    // xDCW
    word3  DCW_18_20_CP; // '7' indicates IDCW
    // XXX pg 30; the indirect data service needs to use this.

    word6 ADDR_EXT; // 3.2.2, 3.2.3.1
    word1 SEG;  // pg B21

    enum { /* PGE */ cm1, cm2, cm3a, cm3b, cm4, cm5,
           /* EXT */ cm1e, cm2e } chanMode;

// XXX CP XXXX
// "Specifies the positions of the first character with the first word
// of the block. The byte size, defined by the channel, determines
// what CP values are valid/
//  6 bit: 0-5; 9 bit: 0-4; 18 bit: 0-1; 36 bit: 0-6
//
// For word channels, CP is sent to the channel during list service,
// and is zeros when placed in the mailbox for subsequent data
// services to the channel.
//
//  [CAC: I think that this can be elided. To implement correctly,
//  iom_list_service and/or doPayloadChannel would have to know the
//  word or sub-word functionality of the channel. But it would
//  be simpler to let the device handler just access the CP data,
//  and make it's own decisions about "zeros". After all, it is
//  not clear what a non-zero CP means for a word channel.]
//
// For sub-word channels which depend on IOM Central for packing and
// unpacking words, [IOM Central uses and updates the CP to access
// sub-words].
//
//  [CAC: Again, I think that this can be elided. It is simpler to
//  to have the device handler pack and unpack.]
//

// LPW addressing mode

    //enum { LPW_REAL, LPW_EXT, LPW_PAGED, LPW_SEG } lpwAddrMode;

    // pg B2: "Scratchpad area for two Page Table Words ... one
    // for the DCW List (PTW-LPW) and one for the data (PTW-DCW).

    // PTW format: (pg B8)
    //   4-17: Address of page table, mod 64
    //   31: WRC: Write control bit (1: page may be written)
    //   32: HSE: Housekeeping
    //   33: PGP: Page present
    // To read or write PGP must be 1
    // To write, WRC must be 1, HSE 0; system fault 15 on fail
// pg b8: PTWs are used iff (indirect store and LPw 23 (segmented)), or
// direct store.
//
//  ADDR  0-13 <- PTW 4-17
//       14-23 <- LPW 8-17; DCW 8-17; direct channel address 14-23

    word36 PTW_DCW;  // pg B8.
    word36 PTW_LPW;  // pg B6.

// pg b11 defines two PTW flags to indicate the validity of the
// PTW_DCW and PTW_LPW; it is simpler to simply always fetch
// the PTWs on demand.

//  flag
    //chanMode_t chanMode;

    // true if the DCW is from a PCW
    bool isPCW;

    // Information accumulated for status service.
    word12 stati;
    uint dev_code;
    // Initialized to IDCW_COUNT; decremented when a IDCW that expects an IOTx is processed by the channel.
    // XXX POLTS console code drives this; if it turns out to be common across channels, the decrement should
    // be moved into the IOM.
    word6 recordResidue;
    word12 tallyResidue;
    word3 charPos;
    bool isRead;
    // isOdd can be ignored; see https://ringzero.wikidot.com/wiki:cac-2015-10-22
    // bool isOdd;
    bool initiate;

    chanStat chanStatus;

    bool lsFirst;

    bool wasTDCW;

    bool masked;

    bool in_use;

    bool start;

  } iom_chan_data_t;

extern iom_chan_data_t iom_chan_data [N_IOM_UNITS_MAX] [MAX_CHANNELS];

extern DEVICE iom_dev;

// Indirect data service data type
typedef enum
  {
    idsTypeW36  // Incoming data is array of word36
  } idsType;

typedef enum
  {
    direct_load,
    direct_store,
    direct_read_clear,
  } iom_direct_data_service_op;

#if 0
typedef struct pcw_t
  {
    // Word 1
    uint dev_cmd;    // 6 bits; 0..5
    uint dev_code;   // 6 bits; 6..11
    uint ext;        // 6 bits; 12..17; address extension
    uint cp;         // 3 bits; 18..20, must be all ones

// From iom_control.alm:
//  " At this point we would normally set idcw.ext_ctl.  This would allow IOM's
//  " to transfer to DCW lists which do not reside in the low 256K.
//  " Unfortunately, the PSIA does not handle this bit properly.
//  " As a result, we do not set the bit and put a kludge in pc_abs so that
//  " contiguous I/O buffers are always in the low 256K.
//  "
//  " lda       =o040000,dl         " set extension control in IDCW
//  " orsa      ab|0

    uint mask;    // extension control or mask; 1 bit; bit 21
    uint control;    // 2 bits; bit 22..23
    uint chan_cmd;   // 6 bits; bit 24..29;
    // AN87 says: 00 single record xfer, 02 non data xfer,
    // 06 multi-record xfer, 10 single char record xfer
    uint chan_data;  // 6 bits; bit 30..35; often some sort of count
    //

    // Word 2
    uint chan;       // 6 bits; bits 3..8 of word 2
    uint ptPtr;        // 18 bits; bits 9..26 of word 2
    uint ptp;    // 1 bit; bit 27 of word 2
    uint pcw64_pge;    // 1 bit; bit 28 of word 2
    uint aux;    // 1 bit; bit 29 of word 2
  } pcw_t;
#endif

#define IOM_MBX_LPW     0
#define IOM_MBX_LPWX    1
#define IOM_MBX_SCW     2
#define IOM_MBX_DCW     3

/* From AN70-1 May84
 *  ... The IOM determines an interrupt
 * number. (The interrupt number is a five bit value, from 0 to 31.
 * The high order bits are the interrupt level.
 *
 * 0 - system fault
 * 1 - terminate
 * 2 - marker
 * 3 - special
 *
 * The low order three bits determines the IOM and IOM channel
 * group.
 *
 * 0 - IOM 0 channels 32-63
 * 1 - IOM 1 channels 32-63
 * 2 - IOM 2 channels 32-63
 * 3 - IOM 3 channels 32-63
 * 4 - IOM 0 channels 0-31
 * 5 - IOM 1 channels 0-31
 * 6 - IOM 2 channels 0-31
 * 7 - IOM 3 channels 0-31
 *
 *   3  3     3   3   3
 *   1  2     3   4   5
 *  ---------------------
 *  | pic | group | iom |
 *  -----------------------------
 *       2       1     2
 */

enum iomImwPics
  {
    imwSystemFaultPic = 0,
    imwTerminatePic   = 1,
    imwMarkerPic      = 2,
    imwSpecialPic     = 3
  };

int send_general_interrupt (uint iom_unit_idx, uint chan, enum iomImwPics pic);

int send_special_interrupt (uint iom_unit_idx, uint chanNum, uint devCode,
                            word8 status0, word8 status1);
//
// iom_cmd_t returns:
//
//  0: ok
//  1; ignored cmd, drop connect.
//  2: did command, don't do DCW list
//  3; command pending, don't sent terminate interrupt
// -1: error

//#define IOM_CMD_OK       0
//#define IOM_CMD_IGNORED  1
//#define IOM_CMD_NO_DCW   2
//#define IOM_CMD_PENDING  3
//#define IOM_CMD_ERROR   -1

typedef enum
  {
     IOM_CMD_ERROR   = -1,
     IOM_CMD_PROCEED =  0,
     IOM_CMD_RESIDUE,
     IOM_CMD_DISCONNECT,
     IOM_CMD_PENDING
  } iom_cmd_rc_t;

typedef iom_cmd_rc_t iom_cmd_t (uint iom_unit_idx, uint chan);
int iom_list_service (uint iom_unit_idx, uint chan,
                      bool * ptro, bool * sendp, bool * uffp);
int send_terminate_interrupt (uint iom_unit_idx, uint chanNum);
void iom_interrupt (uint scuUnitNum, uint iom_unit_idx);
void iom_direct_data_service (uint iom_unit_idx, uint chan, word24 daddr, word36 * data,
                              iom_direct_data_service_op op);
void iom_indirect_data_service (uint iom_unit_idx, uint chan, word36 * data,
                                uint * cnt, bool write);
void iom_init (void);
int send_marker_interrupt (uint iom_unit_idx, int chan);
#if defined(PANEL68)
void do_boot (void);
#endif
#if defined(IO_THREADZ)
void * iom_thread_main (void * arg);
void * chan_thread_main (void * arg);
#endif
void iom_core_read (uint iom_unit_idx, word24 addr, word36 *data, UNUSED const char * ctx);
void iom_core_read2 (uint iom_unit_idx, word24 addr, word36 *even, word36 *odd, UNUSED const char * ctx);
void iom_core_write (uint iom_unit_idx, word24 addr, word36 data, UNUSED const char * ctx);
void iom_core_write2 (uint iom_unit_idx, word24 addr, word36 even, word36 odd, UNUSED const char * ctx);
void iom_core_read_lock (uint iom_unit_idx, word24 addr, word36 *data, UNUSED const char * ctx);
void iom_core_write_unlock (uint iom_unit_idx, word24 addr, word36 data, UNUSED const char * ctx);
t_stat iom_unit_reset_idx (uint iom_unit_idx);

#if defined(IO_ASYNC_PAYLOAD_CHAN) || defined(IO_ASYNC_PAYLOAD_CHAN_THREAD)
void iomProcess (void);
#endif

char iomChar (uint iomUnitIdx);
#if defined(TESTING)
void dumpDCW (word36 DCW, word1 LPW_23_REL);
#endif
