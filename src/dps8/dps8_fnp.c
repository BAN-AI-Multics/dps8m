#include <stdio.h>
#include "dps8.h"
#include "dps8_fnp.h"
#include "dps8_sys.h"
#include "dps8_utils.h"
#include "dps8_cpu.h"
#include "dps8_iom.h"
#include "fnpp.h"

static t_stat fnpShowConfig (FILE *st, UNIT *uptr, int val, void *desc);
static t_stat fnpSetConfig (UNIT * uptr, int value, char * cptr, void * desc);
static t_stat fnpShowNUnits (FILE *st, UNIT *uptr, int val, void *desc);
static t_stat fnpSetNUnits (UNIT * uptr, int32 value, char * cptr, void * desc);

static int fnpIOMCmd (UNIT * unitp, pcw_t * p);
//static int fnpIOT (UNIT * unitp, dcw_t * dcwp, bool *  disc);

#define N_FNP_UNITS_MAX 16
#define N_FNP_UNITS 1 // default

static t_stat fnpSVC (UNIT *up);

static UNIT fnp_unit [N_FNP_UNITS_MAX] = {
    {UDATA (& fnpSVC, UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& fnpSVC, UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& fnpSVC, UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& fnpSVC, UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& fnpSVC, UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& fnpSVC, UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& fnpSVC, UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& fnpSVC, UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& fnpSVC, UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& fnpSVC, UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& fnpSVC, UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& fnpSVC, UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& fnpSVC, UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& fnpSVC, UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& fnpSVC, UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL},
    {UDATA (& fnpSVC, UNIT_DISABLE | UNIT_IDLE, 0), 0, 0, 0, 0, 0, NULL, NULL}
};

static DEBTAB fnpDT [] =
  {
    { "NOTIFY", DBG_NOTIFY },
    { "INFO", DBG_INFO },
    { "ERR", DBG_ERR },
    { "WARN", DBG_WARN },
    { "DEBUG", DBG_DEBUG },
    { "ALL", DBG_ALL }, // don't move as it messes up DBG message
    { NULL, 0 }
  };

static MTAB fnpMod [] =
  {
    {
      MTAB_XTD | MTAB_VUN | MTAB_NMO | MTAB_VALR, /* mask */
      0,            /* match */
      "CONFIG",     /* print string */
      "CONFIG",         /* match string */
      fnpSetConfig,         /* validation routine */
      fnpShowConfig, /* display routine */
      NULL,          /* value descriptor */
      NULL   // help string
    },

    {
      MTAB_XTD | MTAB_VDV | MTAB_NMO | MTAB_VALR, /* mask */
      0,            /* match */
      "NUNITS",     /* print string */
      "NUNITS",         /* match string */
      fnpSetNUnits, /* validation routine */
      fnpShowNUnits, /* display routine */
      "Number of FNP units in the system", /* value descriptor */
      NULL          // help
    },
    { 0, 0, NULL, NULL, NULL, NULL, NULL, NULL }
  };

#define FNP_UNIT_NUM(uptr) ((uptr) - fnp_unit)

static t_stat fnpReset (DEVICE * dptr);

DEVICE fnpDev = {
    "FNP",           /* name */
    fnp_unit,          /* units */
    NULL,             /* registers */
    fnpMod,           /* modifiers */
    N_FNP_UNITS,       /* #units */
    10,               /* address radix */
    31,               /* address width */
    1,                /* address increment */
    8,                /* data radix */
    9,                /* data width */
    NULL,             /* examine routine */
    NULL,             /* deposit routine */
    fnpReset,         /* reset routine */
    NULL,             /* boot routine */
    NULL,             /* attach routine */
    NULL,             /* detach routine */
    NULL,             /* context */
    DEV_DEBUG,        /* flags */
    0,                /* debug control flags */
    fnpDT,            /* debug flag names */
    NULL,             /* memory size change */
    NULL,             /* logical name */
    NULL,             // attach help
    NULL,             // help
    NULL,             // help context
    NULL,             // device description
};

static struct fnpUnitData
  {
//-    enum { no_mode, read_mode, write_mode, survey_mode } io_mode;
//-    uint8 * bufp;
//-    t_mtrlnt tbc; // Number of bytes read into buffer
//-    uint words_processed; // Number of Word36 processed from the buffer
//-    int rec_num; // track tape position
    uint mailboxAddress;
  } fnpUnitData [N_FNP_UNITS_MAX];

static struct
  {
    int iomUnitNum;
    int chan_num;
    int dev_code;
  } cables_from_ioms_to_fnp [N_FNP_UNITS_MAX];

#if 0
static int findFNPUnit (int iomUnitNum, int chan_num, int dev_code)
  {
    for (int i = 0; i < N_FNP_UNITS_MAX; i ++)
      {
        if (iomUnitNum == cables_from_ioms_to_fnp [i] . iomUnitNum &&
            chan_num     == cables_from_ioms_to_fnp [i] . chan_num     &&
            dev_code     == cables_from_ioms_to_fnp [i] . dev_code)
          return i;
      }
    return -1;
  }
#endif

int lookupFnpsIomUnitNumber (int fnpUnitNum)
  {
    return cables_from_ioms_to_fnp [fnpUnitNum] . iomUnitNum;
  }

void fnpInit(void)
  {
    memset(fnpUnitData, 0, sizeof(fnpUnitData));
    for (int i = 0; i < N_FNP_UNITS_MAX; i ++)
      cables_from_ioms_to_fnp [i] . iomUnitNum = -1;
    fnppInit ();
  }

static t_stat fnpReset (DEVICE * dptr)
  {
    for (int i = 0; i < (int) dptr -> numunits; i ++)
      {
        sim_cancel (& fnp_unit [i]);
      }
    fnppReset (dptr);
    return SCPE_OK;
  }

t_stat cableFNP (int fnpUnitNum, int iomUnitNum, int chan_num, int dev_code)
  {
    if (fnpUnitNum < 0 || fnpUnitNum >= (int) fnpDev . numunits)
      {
        sim_printf ("cableFNP: fnpUnitNum out of range <%d>\n", fnpUnitNum);
        return SCPE_ARG;
      }

    if (cables_from_ioms_to_fnp [fnpUnitNum] . iomUnitNum != -1)
      {
        sim_printf ("cableFNP: socket in use\n");
        return SCPE_ARG;
      }

    // Plug the other end of the cable in
    t_stat rc = cable_to_iom (iomUnitNum, chan_num, dev_code, DEVT_DN355, chan_type_PSI, fnpUnitNum, & fnpDev, & fnp_unit [fnpUnitNum], fnpIOMCmd);
    if (rc)
      return rc;

    cables_from_ioms_to_fnp [fnpUnitNum] . iomUnitNum = iomUnitNum;
    cables_from_ioms_to_fnp [fnpUnitNum] . chan_num = chan_num;
    cables_from_ioms_to_fnp [fnpUnitNum] . dev_code = dev_code;

    return SCPE_OK;
  }
 
static int fnpCmd (UNIT * unitp, pcw_t * pcwp, bool * disc)
  {
    int fnpUnitNum = FNP_UNIT_NUM (unitp);
    int iomUnitNum = cables_from_ioms_to_fnp [fnpUnitNum] . iomUnitNum;
    //struct fnpUnitData * p = & fnpUnitData [fnpUnitNum];
    * disc = false;

    int chan = pcwp-> chan;

    iomChannelData_ * chan_data = & iomChannelData [iomUnitNum] [chan];
    if (chan_data -> ptp)
      {
        sim_printf ("PTP in fnp; dev_cmd %o\n", pcwp -> dev_cmd);
        //sim_err ("PTP in fnp\n");
      }
    chan_data -> stati = 0;
sim_printf ("fnp cmd %d\n", pcwp -> dev_cmd);
    switch (pcwp -> dev_cmd)
      {
        case 000: // CMD 00 Request status
          {
sim_printf ("fnp cmd request status\n");
            chan_data -> stati = 04000;
            //disk_statep -> io_mode = no_mode;
            sim_debug (DBG_NOTIFY, & fnpDev, "Request status %d\n", fnpUnitNum);
            chan_data -> initiate = true;
          }
          break;

        default:
          {
            chan_data -> stati = 04501;
            sim_debug (DBG_ERR, & fnpDev,
                       "%s: Unknown command 0%o\n", __func__, pcwp -> dev_cmd);
            break;
          }
      }

    //status_service (iomUnitNum, chan, false);

    return 0;
  }

#if 0
// How many 16 biy words are needed to hold 'tally' 36 bit words.
static uint unpack36to16Size (uint tally)
  {
    uint bc = tally * 36;
    //return (bc + 15) / 16; // This would include a trailing partial word
    return bc / 16; // No trailing partial word
  }

static void unpack36to16 (word36 * in, uint tally, word16 * out)
  {
    for (uint i = 0; i < tally; i += 4)
      {
        word16 w16 = getbits36 (in [i], 0, 16);
        * out ++ = w16; // 1
        w16 = getbits36 (in [i], 16, 16);
        * out ++ = w16; // 2
        if (i - tally < 2)
          break;
        w16 = getbits36 (in [i], 32, 4) << 12 | getbits36 (in [i + 1], 0, 12);
        * out ++ = w16; // 3
        w16 = getbits36 (in [i + 1], 12, 16);
        * out ++ = w16; // 4
        if (i - tally < 3)
          break;
        w16 = getbits36 (in [i + 1], 28, 8) << 8 | getbits36 (in [i + 2], 0, 8);
        * out ++ = w16; // 5
        w16 = getbits36 (in [i + 2], 8, 16);
        * out ++ = w16; // 6
        if (i - tally < 4)
          break;
        w16 = getbits36 (in [i + 2], 28, 8) << 4 | getbits36 (in [i + 3], 0, 4);
        * out ++ = w16; // 7
        w16 = getbits36 (in [i + 3], 4, 16);
        * out ++ = w16; // 8
        w16 = getbits36 (in [i + 3], 20, 16);
        * out ++ = w16; // 9
      }
  }
#endif

struct dn355_submailbox
  {
    word36 word1; // dn355_no; is_hsla; la_no; slot_no
    word36 word2; // cmd_data_len; op_code; io_cmd
    word36 command_data [3];
    word36 word6; // data_addr, word_cnt;
    word36 pad3 [2];
  };

struct fnp_submailbox
  {
     word36 mystery [28];
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

static void dmpmbx (uint mailboxAddress)
  {
    struct mailbox * mbxp = (struct mailbox *) & M [mailboxAddress];
    sim_printf ("dia_pcw          %012llo\n", mbxp -> dia_pcw);
    sim_printf ("term_inpt_mpx_wd %012llo\n", mbxp -> term_inpt_mpx_wd);
    sim_printf ("num_in_use       %012llo\n", mbxp -> num_in_use);
    sim_printf ("mbx_used_flags   %012llo\n", mbxp -> mbx_used_flags);
    for (uint i = 0; i < 8; i ++)
      {
        sim_printf ("mbx %d\n", i);
        struct dn355_submailbox * smbxp = & (mbxp -> dn355_sub_mbxes [i]);
        sim_printf ("    word1        %012llo\n", smbxp -> word1);
        sim_printf ("    word2        %012llo\n", smbxp -> word2);
        sim_printf ("    command_data %012llo\n", smbxp -> command_data [0]);
        sim_printf ("                 %012llo\n", smbxp -> command_data [1]);
        sim_printf ("                 %012llo\n", smbxp -> command_data [2]);
        sim_printf ("    word6        %012llo\n", smbxp -> word6);
      }
  }

/*
 * fnpIOMCmd()
 *
 */

static int fnpIOMCmd (UNIT * unitp, pcw_t * pcwp)
  {
    int fnpUnitNum = FNP_UNIT_NUM (unitp);
    int iomUnitNum = cables_from_ioms_to_fnp [fnpUnitNum] . iomUnitNum;
    struct fnpUnitData * p = & fnpUnitData [fnpUnitNum];

    // First, execute the command in the PCW, and then walk the 
    // payload channel mbx looking for IDCWs.

// Ignore a CMD 051 in the PCW
    if (pcwp -> dev_cmd == 051)
      return 1;
#if 0
sim_printf ("fnpIOMCmd\n");
sim_printf ("  [%lld]\n", sim_timell ());
sim_printf (" pcwp -> dev_cmd %02o\n", pcwp -> dev_cmd);
sim_printf (" pcwp -> dev_code %02o\n", pcwp -> dev_code);
sim_printf (" pcwp -> ext %02o\n", pcwp -> ext);
sim_printf (" pcwp -> cp %0o\n", pcwp -> cp);
sim_printf (" pcwp -> mask %0o\n", pcwp -> mask);
sim_printf (" pcwp -> control %0o\n", pcwp -> control);
sim_printf (" pcwp -> chan_cmd %0o\n", pcwp -> chan_cmd);
sim_printf (" pcwp -> chan_data %0o\n", pcwp -> chan_data);
sim_printf (" pcwp -> chan %0o\n", pcwp -> chan);
sim_printf (" pcwp -> ptPtr %0o\n", pcwp -> ptPtr);
sim_printf (" pcwp -> ptp %0o\n", pcwp -> ptp);
sim_printf (" pcwp -> pcw64_pge %0o\n", pcwp -> pcw64_pge);
sim_printf (" pcwp -> aux %0o\n", pcwp -> aux);
#endif
    bool disc;
//sim_printf ("1 st call to fnpCmd\n");
    fnpCmd (unitp, pcwp, & disc);

// 60132445 FEP Coupler EPS
// 2.2.1 Cpntrol Intercommunication
//
// "In Level 66 momory, at a location known to the coupler and
// to Level 6 software is a mailbox area consisting to an Overhead
// mailbox and 7 Channel mailboxes."

    bool ok = true;
    struct mailbox * mbxp = (struct mailbox *) & M [p -> mailboxAddress];

    word36 dia_pcw;
    //fetch_abs_word (p -> mailboxAddress, & dia_pcw, "fnpIOMCmd get dia_pcw");
    dia_pcw = mbxp -> dia_pcw;
sim_printf ("mbx %08o:%012llo\n", p -> mailboxAddress, dia_pcw);

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
// Operation       C         A        B        D
// Interrupt L6   071       ---      Int.     Level
// Bootload L6    072    L66 Addr  L66 Addr  L66 Addr
//                       A6-A23    A0-A2     A3-A5
// Interrupt L66  073      ---      ---     Intr Cell
// 
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

    uint chanNum = getbits36 (dia_pcw, 24, 6);
    uint command = getbits36 (dia_pcw, 30, 6);
    word36 bootloadStatus = 0;

    if (command == 072) // bootload
      {
sim_printf ("bootload\n");
#if 0
        uint a6_23 = getbits36 (dia_pcw, 0, 18);
        uint a0_2 = getbits36 (dia_pcw, 24, 3);
        uint a3_5 = getbits36 (dia_pcw, 27, 3);
        uint l66Address = (a0_2 << 21) | (a3_5 << 18) | a6_23;
sim_printf ("l66Address %o\n", l66Address);
sim_printf (" pcwp -> ptPtr %0o\n", pcwp -> ptPtr);

        uint pageTable = pcwp -> ptPtr * 64u;

//sim_printf ("    %012llo\n%012llo\n", M [pageTable], M [pageTable + 1]);
//sim_printf ("    %012llo\n%012llo\n", M [pageTable + 2], M [pageTable + 3]);

        uint l66AddressPage = l66Address / 1024u;
        word36 ptw;
        fetch_abs_word (pageTable + l66AddressPage, & ptw, "fnpIOMCmd get ptw");
sim_printf ("ptw %012llo\n", ptw);
        uint page = getbits36 (ptw, 4, 14);
sim_printf ("page %o\n", page);
        uint addr = page * 1024u + l66Address % 1024u;
sim_printf ("addr %o\n", addr);

        word36 tcw;
        fetch_abs_word (addr, & tcw, "fnpIOMCmd get tcw");
sim_printf ("tcw %012llo\n", tcw);

        uint tally = getbits36 (tcw, 24, 12);
sim_printf ("tally %d (%o)\n", tally, tally);
        for (uint i = 0; i < tally; i ++)
          {
            word36 data;
            fetch_abs_word (addr + 1 + i, & data, "fnpIOMCmd get data");
            sim_printf ("%4d %012llo\n", i, data);
          }

        uint tally16 = unpack36to16Size (tally);
        word16 buffer16 [tally16];

        unpack36to16 (& M [addr + 1], tally, buffer16);

        for (uint i = 0; i < tally16; i ++)
          {
            sim_printf ("%4d  %06o %04x\n", i, buffer16 [i], buffer16 [i]);
          }
 #endif
        



      }
    else if (command == 071) // interrupt L6
      {
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



        dmpmbx (p -> mailboxAddress);
        uint cell = getbits36 (dia_pcw, 27, 3);
sim_printf ("interrupt FNP\n");
sim_printf ("mbx #%d\n", cell);
        if (cell > 7)
          {
            sim_printf ("fnp illegal cell number %d\n", cell);
            // doFNPfault (...) // XXX
            goto fail;
          }
        struct dn355_submailbox * smbxp = & (mbxp -> dn355_sub_mbxes [cell]);

        word36 word2 = smbxp -> word2;
        uint cmd_data_len = getbits36 (word2, 9, 9);
        uint op_code = getbits36 (word2, 18, 9);
        uint io_cmd = getbits36 (word2, 27, 9);

        word36 word1 = smbxp -> word1;
        uint dn355_no = getbits36 (word1, 0, 3);
        uint is_hsla = getbits36 (word1, 8, 1);
        uint la_no = getbits36 (word1, 9, 3);
        uint slot_no = getbits36 (word1, 12, 6);
        uint terminal_id = getbits36 (word1, 18, 18);

        switch (io_cmd)
          {
            case 3: // wcd (write control data)
              {
                switch (op_code)
                  {
                    case  4: // accept_calls
                      {
                        sim_printf ("fnp accept calls\n");
                        sim_printf ("  dn355_no %d\n", dn355_no);
                        sim_printf ("  is_hsla %d\n", is_hsla);
                        sim_printf ("  la_no %d\n", la_no);
                        sim_printf ("  slot_no %d\n", slot_no);
                        sim_printf ("  terminal_id %d\n", terminal_id);
                        word36 command_data0 = smbxp -> command_data [0];
                        uint bufferAddress = getbits36 (command_data0, 0, 18);
                        sim_printf ("  buffer address %06o\n", bufferAddress);
                        // call fnp (accept calls);
                      }
                      break;

                    case 34: // alter_parameters
                      {
                        sim_printf ("fnp alter parameters\n");
                        sim_printf ("  dn355_no %d\n", dn355_no);
                        sim_printf ("  is_hsla %d\n", is_hsla);
                        sim_printf ("  la_no %d\n", la_no);
                        sim_printf ("  slot_no %d\n", slot_no);
                        sim_printf ("  terminal_id %d\n", terminal_id);
                        // The docs insist the subype is in word2, but I think
                        // it is in command data...
                        //uint subtype = getbits36 (word2, 0, 9);
                        uint subtype = getbits36 (smbxp -> command_data [0], 0, 9);
                        uint flag = getbits36 (smbxp -> command_data [0], 17, 1);
                        sim_printf ("  subtype %d\n", subtype);
                        switch (subtype)
                          {
                            case 16: // Listen
                              {
                                sim_printf ("fnp alter paramter Listen %d\n", flag);
                                // call fno (alter parameter listen) // XX
                              }
                            break;

                            case  1: // Breakchar
                            case  2: // Nocontrol
                            case  3: // Fullduplex
                            case  4: // Break
                            case  5: // Errormsg
                            case  6: // Meter
                            case  7: // Sensepos
                            case  8: // Crecho
                            case  9: // Lfecho
                            case 10: // Lock
                            case 11: // Msg
                            case 12: // Upstate
                            case 13: // Dumpoutput
                            case 14: // Tabecho
                            case 15: // Setbusy
                            case 17: // Hndlquit
                            case 18: // Chngstring
                            case 19: // Wru
                            case 20: // Echoplex
                            case 21: // Xmit_hold
                            case 22: // Dumpinput
                            case 23: // Replay
                            case 24: // Polite
                            case 25: // Block_xfer
                            case 26: // Set_buffer_size
                            case 27: // Breakall
                            case 28: // Prefixn1
                            case 29: // Input_flow_control
                            case 30: // Output_flow_control
                            case 31: // Odd_parity
                            case 32: // Eight_bit_in
                            case 33: // Eight_bit_out
                              {
                                sim_printf ("fnp unimplemented subtype %d\n", subtype);
                                // doFNPfault (...) // XXX
                                goto fail;
                              }

                            default:
                              {
                                sim_printf ("fnp illegal subtype %d\n", subtype);
                                // doFNPfault (...) // XXX
                                goto fail;
                              }
                          } // switch (subtype)
                        //word36 command_data0 = smbxp -> command_data [0];
                        //uint bufferAddress = getbits36 (command_data0, 0, 18);
                        //sim_printf ("  buffer address %06o\n", bufferAddress);

                        // call fnp (accept calls);
                      }
                      break;

                    case  0: // terminal_accepted
                    case  1: // disconnect_this_line
                    case  2: // disconnect_all_lines
                    case  3: // dont_accept_calls
                    case  5: // input_accepted
                    case  6: // set_line_type
                    case  7: // enter_receive
                    case  8: // set_framing_chars
                    case  9: // blast
                    case 10: // accept_direct_output
                    case 11: // accept_last_output
                    case 12: // dial
                    //case 13: // ???
                    case 14: // reject_request_temp
                    //case 15: // ???
                    case 16: // terminal_rejected
                    case 17: // disconnect_accepted
                    case 18: // init_complete
                    case 19: // dump_mem
                    case 20: // patch_mem
                    case 21: // fnp_break
                    case 22: // line_control
                    case 23: // sync_msg_size
                    case 24: // set_echnego_break_table
                    case 25: // start_negotiated_echo
                    case 26: // stop_negotiated_echo
                    case 27: // init_echo_negotiation
                    //case 28: // ???
                    case 29: // break_acknowledged
                    case 30: // input_fc_chars
                    case 31: // output_fc_chars
                    //case 32: // ???
                    //case 33: // ???
                    case 35: // checksum_error
                    case 36: // report_meters
                    case 37: // set_delay_table
                      {
                        sim_printf ("fnp unimplemented opcode %d\n", op_code);
                        // doFNPfault (...) // XXX
                        goto fail;
                      }

                    default:
                      {
                        sim_printf ("fnp illegal opcode %d\n", op_code);
                        // doFNPfault (...) // XXX
                        goto fail;
                      }
                  } // switch op_code

                // Set the TIMW

                putbits36 (& mbxp -> term_inpt_mpx_wd, cell, 1, 1);

                // Kick the cpu

              } // case wcd
              break;

            case 1: // rcd (read contol data)
            case 2: // rtx (read text)
            case 4: // wtx (write text)
              {
                sim_printf ("fnp unimplemented io_cmd %d\n", io_cmd);
                // doFNPfault (...) // XXX
                goto fail;
              }
            default:
              {
                sim_printf ("fnp illegal io_cmd %d\n", io_cmd);
                // doFNPfault (...) // XXX
                goto fail;
              }
          } // switch (io_cmd)
      }
    if (ok)
      {
        store_abs_word (p -> mailboxAddress, 0, "fnpIOMCmd clear dia_pcw");
        putbits36 (& bootloadStatus, 0, 1, 1); // real_status = 1
        putbits36 (& bootloadStatus, 3, 3, 0); // major_status = BOOTLOAD_OK;
        putbits36 (& bootloadStatus, 9, 8, 0); // substatus = BOOTLOAD_OK;
        putbits36 (& bootloadStatus, 17, 17, 0); // channel_no = 0;
        store_abs_word (p -> mailboxAddress + 6, bootloadStatus, "fnpIOMCmd set bootload status");
      }
    else
      {
fail:
        putbits36 (& dia_pcw, 18, 1, 1); // set bit 18
        store_abs_word (p -> mailboxAddress, dia_pcw, "fnpIOMCmd set error bit");
      }

sim_printf ("end of list service; sending terminate interrupt\n");
    send_terminate_interrupt (iomUnitNum, pcwp -> chan);

    return 1;
  }

static t_stat fnpSVC (UNIT * unitp)
  {
    int fnpUnitNum = FNP_UNIT_NUM (unitp);
    int iomUnitNum = cables_from_ioms_to_fnp [fnpUnitNum] . iomUnitNum;
    int chanNum = cables_from_ioms_to_fnp [fnpUnitNum] . chan_num;
    pcw_t * pcwp = & iomChannelData [iomUnitNum] [chanNum] . pcw;
    fnpIOMCmd (unitp, pcwp);
    return SCPE_OK;
  }
    

#if 0
static int fnpIOT (UNIT * unitp, dcw_t * dcwp, bool *  disc)
  {
    int fnpUnitNum = FNP_UNIT_NUM (unitp);
    int iomUnitNum = cables_from_ioms_to_fnp [fnpUnitNum] . iomUnitNum;
    * disc = false;
    if (dcwp -> type == 0) // IOTD
      * disc = true;

  }

static int fnp_iom_io (UNIT * unitp, uint chan, uint dev_code, uint * tally, uint * cp, word36 * wordp, word12 * stati)
  {
    //sim_debug (DBG_DEBUG, & fnpDev, "%s\n", __func__);
    int fnpUnitNum = FNP_UNIT_NUM (unitp);
    //int iomUnitNum = cables_from_ioms_to_fnp [fnpUnitNum] . iomUnitNum;
//--     
//--     int dev_unit_num;
//--     DEVICE* devp = get_iom_channel_dev (iomUnitNum, chan, dev_code, & dev_unit_num);
//--     if (devp == NULL || devp->units == NULL) {
//--         *majorp = 05;
//--         *subp = 2;
//--         sim_debug (DBG_ERR, &fnpDev, "fnp_iom_io: Internal error, no device and/or unit for channel 0%o\n", chan);
//--         return 1;
//--     }
//--     UNIT * unitp = & devp -> units [dev_unit_num];
//--     // BUG: no dev_code
//--     
    struct fnpUnitData * tape_statep = & fnpUnitData [fnpUnitNum];
    
    if (tape_statep -> io_mode == no_mode)
      {
        // no prior read or write command
        * stati = 05302; // MPC Device Data Alert Inconsistent command
        sim_debug (DBG_ERR, & fnpDev, "%s: Bad channel %d\n", __func__, chan);
        return 1;
      }
    else if (tape_statep -> io_mode == read_mode)
      {
        while (* tally)
          {
            // read
            if (extractWord36FromBuffer (tape_statep -> bufp, tape_statep -> tbc, & tape_statep -> words_processed, wordp) != 0)
              {
                // BUG: There isn't another word to be read from the tape buffer,
                // but the IOM wants  another word.
                // BUG: How did this tape hardware handle an attempt to read more
                // data than was present?
                // One answer is in bootload_tape_label.alm which seems to assume
                // a 4000 all-clear status.
                // Boot_tape_io.pl1 seems to assume that "short reads" into an
                // over-large buffer should not yield any error return.
                // So we'll set the flags to all-ok, but return an out-of-band
                // non-zero status to make the iom stop.
                // BUG: See some of the IOM status fields.
                // BUG: The IOM should be updated to return its DCW tally residue
                // to the caller.
                * stati = 04000;
                if (sim_tape_wrp (unitp))
                  * stati |= 1;
                sim_debug (DBG_WARN, & fnpDev,
                           "%s: Read buffer exhausted on channel %d\n",
                           __func__, chan);
                return 1;
              }
            wordp ++;
            (* tally) --;
          }
        * stati = 04000; // BUG: do we need to detect end-of-record?
        if (sim_tape_wrp (unitp))
          * stati |= 1;
        return 0;
      }
    else if (tape_statep -> io_mode == survey_mode)
      {
        //        2 survey_data,
        //          3 handler (16) unaligned,
        //            4 pad1 bit (1),               // 0
        //            4 reserved bit (1),           // 1
        //            4 operational bit (1),        // 2
        //            4 ready bit (1),              // 3
        //            4 number uns fixed bin (5),   // 4-8
        //            4 pad2 bit (1),               // 9
        //            4 speed uns fixed bin (3),    // 10-12
        //            4 nine_track bit (1),         // 13
        //            4 density uns fixed bin (4);  // 14-17
        
        return 0;
      }
    else
      {
        // write
        sim_debug (DBG_ERR, & fnpDev, "%s: Write I/O Unimplemented\n",
                   __func__);
        * stati = 04340; // Reflective end of tape mark found while trying to write
        return 1;
      }
    
//--     /*notreached*/
//--     *majorp = 0;
//--     *subp = 0;
//--     sim_debug (DBG_ERR, &fnpDev, "fnp_iom_io: Internal error.\n");
//--     cancel_run(STOP_BUG);
//    return 1;
  }
#endif

static t_stat fnpShowNUnits (UNUSED FILE * st, UNUSED UNIT * uptr, 
                              UNUSED int val, UNUSED void * desc)
  {
    sim_printf("Number of FNO units in system is %d\n", fnpDev . numunits);
    return SCPE_OK;
  }

static t_stat fnpSetNUnits (UNUSED UNIT * uptr, UNUSED int32 value, 
                             char * cptr, UNUSED void * desc)
  {
    int n = atoi (cptr);
    if (n < 1 || n > N_FNP_UNITS_MAX)
      return SCPE_ARG;
    fnpDev . numunits = (uint32) n;
    return fnppSetNunits (uptr, value, cptr, desc);
  }

static t_stat fnpShowConfig (UNUSED FILE * st, UNIT * uptr, UNUSED int val, 
                             UNUSED void * desc)
  {
    uint fnpUnitNum = FNP_UNIT_NUM (uptr);
    if (fnpUnitNum >= fnpDev . numunits)
      {
        sim_debug (DBG_ERR, & fnpDev, 
                   "fnpShowConfig: Invalid unit number %d\n", fnpUnitNum);
        sim_printf ("error: invalid unit number %u\n", fnpUnitNum);
        return SCPE_ARG;
      }

    sim_printf ("FNP unit number %u\n", fnpUnitNum);
    struct fnpUnitData * p = fnpUnitData + fnpUnitNum;

    sim_printf ("FNP Mailbox Address:         %04o(8)\n", p -> mailboxAddress);
#if 0
    char * os = "<out of range>";
    switch (p -> configSwOS)
      {
        case CONFIG_SW_STD_GCOS:
          os = "Standard GCOS";
          break;
        case CONFIG_SW_EXT_GCOS:
          os = "Extended GCOS";
          break;
        case CONFIG_SW_MULTICS:
          os = "Multics";
          break;
      }
    char * blct = "<out of range>";
    switch (p -> configSwBootloadCardTape)
      {
        case CONFIG_SW_BLCT_CARD:
          blct = "CARD";
          break;
        case CONFIG_SW_BLCT_TAPE:
          blct = "TAPE";
          break;
      }

    sim_printf ("Allowed Operating System: %s\n", os);
    sim_printf ("FNP Base Address:         %03o(8)\n", p -> configSwIomBaseAddress);
    sim_printf ("Multiplex Base Address:   %04o(8)\n", p -> configSwMultiplexBaseAddress);
    sim_printf ("Bootload Card/Tape:       %s\n", blct);
    sim_printf ("Bootload Tape Channel:    %02o(8)\n", p -> configSwBootloadMagtapeChan);
    sim_printf ("Bootload Card Channel:    %02o(8)\n", p -> configSwBootloadCardrdrChan);
    sim_printf ("Bootload Port:            %02o(8)\n", p -> configSwBootloadPort);
    sim_printf ("Port Address:            ");
    int i;
    for (i = 0; i < N_FNP_PORTS; i ++)
      sim_printf (" %03o", p -> configSwPortAddress [i]);
    sim_printf ("\n");
    sim_printf ("Port Interlace:          ");
    for (i = 0; i < N_FNP_PORTS; i ++)
      sim_printf (" %3o", p -> configSwPortInterface [i]);
    sim_printf ("\n");
    sim_printf ("Port Enable:             ");
    for (i = 0; i < N_FNP_PORTS; i ++)
      sim_printf (" %3o", p -> configSwPortEnable [i]);
    sim_printf ("\n");
    sim_printf ("Port Sysinit Enable:     ");
    for (i = 0; i < N_FNP_PORTS; i ++)
      sim_printf (" %3o", p -> configSwPortSysinitEnable [i]);
    sim_printf ("\n");
    sim_printf ("Port Halfsize:           ");
    for (i = 0; i < N_FNP_PORTS; i ++)
      sim_printf (" %3o", p -> configSwPortHalfsize [i]);
    sim_printf ("\n");
    sim_printf ("Port Storesize:           ");
    for (i = 0; i < N_FNP_PORTS; i ++)
      sim_printf (" %3o", p -> configSwPortStoresize [i]);
    sim_printf ("\n");
#endif
 
    return SCPE_OK;
  }

//
// set fnp0 config=<blah> [;<blah>]
//
//    blah = mailbox=n
//
//--//           multiplex_base=n
//--//           os=gcos | gcosext | multics
//-//---//           boot=card | tape
//--//           tapechan=n
//-//---//           cardchan=n
//--//           scuport=n
//--//           port=n   // set port number for below commands
//--//             addr=n
//--//             interlace=n
//--//             enable=n
//--//             initenable=n
//--//             halfsize=n
//--//             storesize=n
//--//          bootskip=n // Hack: forward skip n records after reading boot record

#if 0
static config_value_list_t cfg_os_list [] =
  {
    { "gcos", CONFIG_SW_STD_GCOS },
    { "gcosext", CONFIG_SW_EXT_GCOS },
    { "multics", CONFIG_SW_MULTICS },
    { NULL, 0 }
  };

static config_value_list_t cfg_boot_list [] =
  {
    { "card", CONFIG_SW_BLCT_CARD },
    { "tape", CONFIG_SW_BLCT_TAPE },
    { NULL, 0 }
  };

static config_value_list_t cfg_base_list [] =
  {
    { "multics", 014 },
    { "multics1", 014 }, // boot fnp
    { "multics2", 020 },
    { "multics3", 024 },
    { "multics4", 030 },
    { NULL, 0 }
  };

static config_value_list_t cfg_size_list [] =
  {
    { "32", 0 },
    { "64", 1 },
    { "128", 2 },
    { "256", 3 },
    { "512", 4 },
    { "1024", 5 },
    { "2048", 6 },
    { "4096", 7 },
    { "32K", 0 },
    { "64K", 1 },
    { "128K", 2 },
    { "256K", 3 },
    { "512K", 4 },
    { "1024K", 5 },
    { "2048K", 6 },
    { "4096K", 7 },
    { "1M", 5 },
    { "2M", 6 },
    { "4M", 7 },
    { NULL, 0 }
  };
#endif

static config_list_t fnp_config_list [] =
  {
    /*  0 */ { "mailbox", 0, 07777, NULL },
#if 0
    /*  0 */ { "os", 1, 0, cfg_os_list },
    /*  1 */ { "boot", 1, 0, cfg_boot_list },
    /*  2 */ { "fnp_base", 0, 07777, cfg_base_list },
    /*  3 */ { "multiplex_base", 0, 0777, NULL },
    /*  4 */ { "tapechan", 0, 077, NULL },
    /*  5 */ { "cardchan", 0, 077, NULL },
    /*  6 */ { "scuport", 0, 07, NULL },
    /*  7 */ { "port", 0, N_FNP_PORTS - 1, NULL },
    /*  8 */ { "addr", 0, 7, NULL },
    /*  9 */ { "interlace", 0, 1, NULL },
    /* 10 */ { "enable", 0, 1, NULL },
    /* 11 */ { "initenable", 0, 1, NULL },
    /* 12 */ { "halfsize", 0, 1, NULL },
    /* 13 */ { "store_size", 0, 7, cfg_size_list },
#endif

    { NULL, 0, 0, NULL }
  };

static t_stat fnpSetConfig (UNIT * uptr, UNUSED int value, char * cptr, UNUSED void * desc)
  {
    uint fnpUnitNUm = FNP_UNIT_NUM (uptr);
    if (fnpUnitNUm >= fnpDev . numunits)
      {
        sim_debug (DBG_ERR, & fnpDev, "fnpSetConfig: Invalid unit number %d\n", fnpUnitNUm);
        sim_printf ("error: fnpSetConfig: invalid unit number %d\n", fnpUnitNUm);
        return SCPE_ARG;
      }

    struct fnpUnitData * p = fnpUnitData + fnpUnitNUm;

    config_state_t cfg_state = { NULL, NULL };

    for (;;)
      {
        int64_t v;
        int rc = cfgparse ("fnpSetConfig", cptr, fnp_config_list, & cfg_state, & v);
        switch (rc)
          {
            case -2: // error
              cfgparse_done (& cfg_state);
              return SCPE_ARG; 

            case -1: // done
              break;
            case 0: // OS
              p -> mailboxAddress = v;
              break;

#if 0
            case 1: // BOOT
              p -> configSwBootloadCardTape = v;
              break;

            case 2: // FNP_BASE
              p -> configSwIomBaseAddress = v;
              break;

            case 3: // MULTIPLEX_BASE
              p -> configSwMultiplexBaseAddress = v;
              break;

            case 4: // TAPECHAN
              p -> configSwBootloadMagtapeChan = v;
              break;

            case 5: // CARDCHAN
              p -> configSwBootloadCardrdrChan = v;
              break;

            case 6: // SCUPORT
              p -> configSwBootloadPort = v;
              break;

            case 7: // PORT
              port_num = v;
              break;

#if 0
                // all of the remaining assume a valid value in port_num
                if (/* port_num < 0 || */ port_num > 7)
                  {
                    sim_debug (DBG_ERR, & fnpDev, "fnpSetConfig: cached PORT value out of range: %d\n", port_num);
                    sim_printf ("error: fnpSetConfig: cached PORT value out of range: %d\n", port_num);
                    break;
                  } 
#endif
            case 8: // ADDR
              p -> configSwPortAddress [port_num] = v;
              break;

            case 9: // INTERLACE
              p -> configSwPortInterface [port_num] = v;
              break;

            case 10: // ENABLE
              p -> configSwPortEnable [port_num] = v;
              break;

            case 11: // INITENABLE
              p -> configSwPortSysinitEnable [port_num] = v;
              break;

            case 12: // HALFSIZE
              p -> configSwPortHalfsize [port_num] = v;
              break;

            case 13: // STORE_SIZE
              p -> configSwPortStoresize [port_num] = v;
              break;
#endif

            default:
              sim_debug (DBG_ERR, & fnpDev, "fnpSetConfig: Invalid cfgparse rc <%d>\n", rc);
              sim_printf ("error: fnpSetConfig: invalid cfgparse rc <%d>\n", rc);
              cfgparse_done (& cfg_state);
              return SCPE_ARG; 
          } // switch
        if (rc < 0)
          break;
      } // process statements
    cfgparse_done (& cfg_state);
    return SCPE_OK;
  }
