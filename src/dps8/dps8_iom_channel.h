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

//typedef enum 
  //{
    //cm_LPW_init_state, // No TDCWs encountered; state is:
                       ////    PCW64 (pcw64_pge): on   PAGE CHAN
                       ////    PCW64 (pcw64_pge): off  EXT MODE CHAN
    //cm_real_LPW_real_DCW,
    //cm_ext_LPW_real_DCW,
    //cm_paged_LPW_seg_DCW
  //} chanMode_t;


typedef enum chanStat
  {
    chanStatNormal = 0,
    chanStatUnexpectedPCW = 1,
    chanStatInvalidInstrPCW = 2,
    chanStatIncorrectDCW = 3,
    chanStatIncomplete = 4,
    chanStatUnassigned = 5,
    chanStatParityErrPeriph = 6,
    chanStatParityErrBus = 7
  } chanStat;

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
    word1 PCW_65_AUX;  // XXX
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
    word2  IDCW_CONTROL; // 0 terminate, 2 process, 3 marker
    word6  IDCW_CHAN_CMD;
    word6  IDCW_COUNT; 
    // DDCW only
    /*word18*/ uint DDCW_ADDR; // Allow overflow detection
    word12 DDCW_TALLY;
    word2  DDCW_22_23_TYPE; // '2' indicates TDCW
    // xDCW
    word3  DCW_18_20_CP; // '7' indicates IDCW // XXX pg 30; the indirect data service needs to use this.

    word6 ADDR_EXT; // 3.2.2, 3.2.3.1
    word1 SEG;  // pg B21

    enum { /* PGE */ cm1, cm2, cm3a, cm3b, cm4, cm5,
           /* EXT */ cm1e, cm2e } chanMode;

// XXX CP XXXX
// "Specifies the positions of the first character withe the first word
// of the block. The byte size, defined by the channel, determines
// what CP vaues are valid/
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
// For sub-word channels which depent on IOM Central for packing and
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
    word6 recordResidue;
    word12 tallyResidue;
    word3 charPos;
    bool isRead;
    // isOdd can be ignored; see http://ringzero.wikidot.com/wiki:cac-2015-10-22
    // bool isOdd;
    bool initiate;

    chanStat chanStatus;

    bool lsFirst;

    bool wasTDCW;

    bool masked;

    bool in_use;

    bool start;

// This iom code is "incorrectly structured. The list service reads DCWs and
// pushes them to the handler; the handlers pull IOTs from the list service.
// this means that the IOT disconnect/proceed bit doesn't get propageted back
// up to the DCW list service. By placing 'ptro' in this structure, the 
// handlers can signal disconnect back up.

    bool ptro;

  } iom_chan_data_t;

extern iom_chan_data_t iom_chan_data [N_IOM_UNITS_MAX] [MAX_CHANNELS];

// Indirect data service data type
// typedef enum 
//   {   
//     idsTypeW36  // Incoming data is array of word36 
//   } idsType;

typedef enum
  {
    direct_load,
    direct_store,
    direct_read_clear,
  } iom_direct_data_service_op;

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
    imwTerminatePic = 1,
    imwMarkerPic = 2,
    imwSpecialPic = 3
  };


int iom_list_service (uint iom_unit_idx, uint chan, bool * sendp, bool * uffp);
void iom_direct_data_service (uint iom_unit_idx, uint chan, word24 daddr, word36 * data,
                           iom_direct_data_service_op op);
void iom_indirect_data_service (uint iom_unit_idx, uint chan, word36 * data,
                             uint * cnt, bool write);
int send_terminate_interrupt (uint iom_unit_idx, uint chanNum);
void iom_interrupt (uint scuUnitNum, uint iom_unit_idx);
int send_marker_interrupt (uint iom_unit_idx, int chan);
int send_general_interrupt (uint iom_unit_idx, uint chan, enum iomImwPics pic);
int send_special_interrupt (uint iom_unit_idx, uint chanNum, uint devCode, 
                            word8 status0, word8 status1);
