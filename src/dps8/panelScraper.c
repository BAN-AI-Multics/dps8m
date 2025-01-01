/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 2395ba22-655a-11ef-85db-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2013-2024 Charles Anthony
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

// XXX APU match enable code not implemented

// XXX control points not done

// XXX panel4_red_ready_light_state
// XXX cpu.panel7_enabled_light_state

// CFLAGS += -DL68
// CFLAGS += -DPANEL
// CFLAGS += -DWAM
// LOCALOBJS += panelScraper.o
// LOCALLIBS += -lpthread

// socat -d -d pty,raw,echo=0,link=/tmp/com_local pty,raw,echo=0,link=/tmp/com_remote
// socat -d -d pty,raw,echo=0,link=/dev/ttyUSB1 pty,raw,echo=0,link=/tmp/com_remote

#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>

#include "panelScraper.h"

#include "../font8x8/font8x8.h"

#include "dps8.h"
#include "dps8_sys.h"
#include "dps8_cpu.h"

#undef cpu
static cpu_state_t * volatile panel_cpup;
#define cpu (* panel_cpup)

#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#if defined(__MINGW64__) || defined(__MINGW32__)
# define FD HANDLE
#else
# include <poll.h>
# define FD int
#endif

#if !defined MIN
# define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#endif

static pthread_t scraperThread;

static int scraperThreadArg;

#define msgLen 4096
static char message [msgLen];
static int c_offset = 0;

// Lamp state

#define nrows 16
#define ncols 40
static bool banks [nrows] [ncols];
#define bank_a banks [ 0]
#define bank_b banks [ 1]
#define bank_c banks [ 2]
#define bank_d banks [ 3]
#define bank_e banks [ 4]
#define bank_f banks [ 5]
#define bank_g banks [ 6]
#define bank_h banks [ 7]
#define bank_i banks [ 8]
#define bank_j banks [ 9]
#define bank_k banks [10]
#define bank_l banks [11]
#define bank_m banks [12]
#define bank_n banks [13]
#define bank_o banks [14]
#define bank_p banks [15]

#define SETL(lights, light_os, from, n) \
  for (uint i = 0; i < (n); i ++) \
    lights [(light_os) + i] = !! ((from) & (1llu << ((n) - i - 1)));

#define SETLA(lights, light_os, from, n) \
  for (uint i = 0; i < (n); i ++) \
    lights [(light_os) + i] = !! from [i];

#define SETL1(lights, light_os, from) \
  lights [(light_os)] = !! (from);

static void update_apu (void)
  {
    uint n = cpu.APU_panel_scroll_select_n_sw;

    switch (cpu.APU_panel_scroll_wheel_sw)
      {
        //  1 "PTW ASSOCIATIVE MEMORY MATCH LOGIC REGISTER N (N=0-15)"
        case 0:
          {
            if (cpu.APU_panel_scroll_select_ul_sw)
              { // Upper
                // 0:14 SEGMENT POINTER
                // 15:26 PAGE NUMBER
                // 27 FULL
                // 32:35 USAGE CNT
                SETL (bank_a, 0+3, cpu.PTWAM[n].POINTER, 15);
                SETL (bank_a, 15+3, cpu.PTWAM[n].PAGENO, 12);
                SETL1 (bank_a, 27+3, cpu.PTWAM[n].FE);
                SETL (bank_a, 32+3, cpu.PTWAM[n].USE, 4);
              }
            else
              { // Lower
                // Empty
              }
          }
          break;

        //  2 "PTW ASSOCIATIVE MEMORY REGISTER N (N=0-15)"
        case 1:
          {
            if (cpu.APU_panel_scroll_select_ul_sw)
              { // Upper
                // 0:17 PAGE ADDRESS
                // 29 MOD
                SETL (bank_a, 0+3, cpu.PTWAM[n].ADDR, 18);
                SETL1 (bank_a, 29+3, cpu.PTWAM[n].M);
              }
            else
              { // Lower
                // Empty
              }
          }
          break;

        //   3 "SDW ASSOCIATIVE MEMORY MATCH LOGIC REGISTER N (N=0-15)"
        case 2:
          {
            if (cpu.APU_panel_scroll_select_ul_sw)
              { // Upper
                // 0:14 SEGMENT POINTER
                // 27 FULL
                // 32:35 USAGE CNT
                SETL (bank_a, 0+3, cpu.SDWAM[n].POINTER, 15);
                SETL1 (bank_a, 27+3, cpu.SDWAM[n].FE);
                SETL (bank_a, 32+3, cpu.SDWAM[n].USE, 4);
              }
            else
              { // Lower
                // Empty
              }
          }
          break;

        //   4 "SDW ASSOCIATIVE MEMORY REGISTER N (N=0-15)"
        case 3:
          {
            if (cpu.APU_panel_scroll_select_ul_sw)
              { // Upper
                // 0:23 ADDRESS
                // 24:26 R1
                // 27:29 R2
                // 30:32 R3
                SETL (bank_a, 0+3, cpu.SDWAM[n].ADDR, 24);
                SETL (bank_a, 24+3, cpu.SDWAM[n].R1, 3);
                SETL (bank_a, 27+3, cpu.SDWAM[n].R2, 3);
                SETL (bank_a, 30+3, cpu.SDWAM[n].R2, 3);
              }
            else
              { // Lower
                // 1:14 SEGMENT BOUNDARY
                // 15 READ
                // 16 EXEC
                // 17 WRITE
                // 18 PRIV
                // 19 UNPG
                // 20 GATE
                // 22:35 CALL LIMITER
                SETL (bank_a, 1+3, cpu.SDWAM[n].BOUND, 14);
                SETL1 (bank_a, 15+3, cpu.SDWAM[n].R);
                SETL1 (bank_a, 16+3, cpu.SDWAM[n].E);
                SETL1 (bank_a, 17+3, cpu.SDWAM[n].W);
                SETL1 (bank_a, 18+3, cpu.SDWAM[n].P);
                SETL1 (bank_a, 19+3, cpu.SDWAM[n].U);
                SETL1 (bank_a, 20+3, cpu.SDWAM[n].G);
                SETL (bank_a, 22+3, cpu.SDWAM[n].EB, 14);
              }
          }
          break;

        //   5 "DESCRIPTOR REGISTER BANK-ADDRESS 0 -- LAST PTW OR DSPTW FETCHED FROM STORE"
        case 4:
          {
            if (cpu.APU_panel_scroll_select_ul_sw)
              { // Upper
                // 0:17 PAGE ADDRESS
                // 26 ACSD // XXX Guessing: on: DSPTW; off: PTW
                SETL (bank_a, 0+3, cpu.lastPTWOffset, 18);
                SETL1 (bank_a, 26+3, cpu.lastPTWIsDS);
              }
            else
              { // Lower
                // 1:14 SEGMENT BOUNDARY
                // 15 READ
                // 16 EXEC
                // 17 WRITE
                // 18 PRIV
                // 19 UNPG
                // 20 GATE
                // 22:35 CALL LIMITER
                SETL (bank_a, 1+3, cpu.SDWAM[n].BOUND, 14);
                SETL1 (bank_a, 15+3, cpu.SDWAM[n].R);
                SETL1 (bank_a, 16+3, cpu.SDWAM[n].E);
                SETL1 (bank_a, 17+3, cpu.SDWAM[n].W);
                SETL1 (bank_a, 18+3, cpu.SDWAM[n].P);
                SETL1 (bank_a, 19+3, cpu.SDWAM[n].U);
                SETL1 (bank_a, 20+3, cpu.SDWAM[n].G);
                SETL (bank_a, 22+3, cpu.SDWAM[n].EB, 14);
              }
          }
          break;

        //   6 "DESCRIPTOR REGISTER BANK-ADDRESS 1 -- DSBR REGISTER"
        case 5:
          {
            if (cpu.APU_panel_scroll_select_ul_sw)
              { // Upper
                // 0:23 ADDRESS
                SETL (bank_a, 0+3, cpu.DSBR.ADDR, 24);
              }
            else
              { // Lower
                SETL (bank_a, 1+3, cpu.SDWAM[n].BOUND, 14);
                SETL1 (bank_a, 15+3, cpu.SDWAM[n].R);
                SETL1 (bank_a, 16+3, cpu.SDWAM[n].E);
                SETL1 (bank_a, 17+3, cpu.SDWAM[n].W);
                SETL1 (bank_a, 18+3, cpu.SDWAM[n].P);
                SETL1 (bank_a, 19+3, cpu.SDWAM[n].U);
                SETL1 (bank_a, 20+3, cpu.SDWAM[n].G);
                SETL (bank_a, 22+3, cpu.SDWAM[n].EB, 14);
              }
          }
          break;

        //   7 "DESCRIPTOR REGISTER BANK-ADDRESS 2 -- LAST SDW FETCHED FROM STORE"
        case 6:
          {
            if (cpu.APU_panel_scroll_select_ul_sw)
              { // Upper
                // 0:23 ADDRESS
                // 24:26 R1
                // 27:29 R2
                // 30:32 R3
                SETL (bank_a, 0+3, cpu.SDW0.ADDR, 24);
                SETL (bank_a, 24+3, cpu.SDW0.R1, 3);
                SETL (bank_a, 27+3, cpu.SDW0.R2, 3);
                SETL (bank_a, 30+3, cpu.SDW0.R2, 3);
              }
            else
              { // Lower
                // 1:14 SEGMENT BOUNDARY
                // 15 READ
                // 16 EXEC
                // 17 WRITE
                // 18 PRIV
                // 19 UNPG
                // 20 GATE
                // 22:35 CALL LIMITER
                SETL (bank_a, 1+3, cpu.SDW0.BOUND, 14);
                SETL1 (bank_a, 15+3, cpu.SDW0.R);
                SETL1 (bank_a, 16+3, cpu.SDW0.E);
                SETL1 (bank_a, 17+3, cpu.SDW0.W);
                SETL1 (bank_a, 18+3, cpu.SDW0.P);
                SETL1 (bank_a, 19+3, cpu.SDW0.U);
                SETL1 (bank_a, 20+3, cpu.SDW0.G);
                SETL (bank_a, 22+3, cpu.SDW0.EB, 14);
              }
          }
          break;

        //   8 "APU DATA OUT BUS"
        // XXX No idea; put address and finalAddress in the display.
        case 7:
          {
            if (cpu.APU_panel_scroll_select_ul_sw)
              { // Upper
                // XXX No idea; guess offset
                // XXX 0-17 offset
                SETL (bank_a, 0+3, cpu.APUDataBusOffset, 18);
                // XXX 18-35 no idea
              }
            else
              { // Lower
                // XXX No idea; guess final address
                // XXX 0-35 final address
                SETL (bank_a, 0+3, cpu.APUDataBusAddr, 24);
              }
          }
          break;

        //   9 "ACCESS VIOLATION FAULT REGISTER"
        case 8:
          {
            if (cpu.APU_panel_scroll_select_ul_sw)
              { // Upper
                // 0:23 APU MEMORY ADDRESS REGISTER // XXX Guessing with APUMemAdr
                // 33:35 RALR
                SETL (bank_a, 0+3, cpu.APUMemAddr, 24);
                SETL (bank_a, 24+3, cpu.rRALR, 3);
              }
            else
              { // Lower
                // 0:15 IRO -- OOSB
                // 18:28 APU OP CODE REGISTER
                // XXX Guessing that IRO--OOSB  is the fault set generated
                // XXX by the APU; saved in acvFaults
                word16 flts = (word16) cpu.acvFaults;
                SETL (bank_a, 3+3, flts, 16);
                // XXX APUCycleBits is 12 bits, only 11 are on the display.
                // XXX Ignoring the high bit (PI_AP)
                word11 cyc = (cpu.cu.APUCycleBits >> 3) & MASK11;
                SETL (bank_a, 18+3, cyc, 11);
              }
          }
          break;

        //  10 "POINTER REGISTER N (N=0-7)"
        case 9:
          {
            if (cpu.APU_panel_scroll_select_ul_sw)
              { // Upper
                // 3:17 SEGMENT NUMBER
                // 18:20 RING
                // 33:35 RING HOLD
                n &= 7;
                SETL (bank_a, 3+3, cpu.PAR[n].SNR, 15);
                SETL (bank_a, 18+3, cpu.PAR[n].RNR, 3);
                SETL (bank_a, 33+3, cpu.RSDWH_R1, 3);
              }
            else
              { // Lower
                // 0 !FLT
                // 1:2 DIR-FLT#
                // 3: ACV

                // XXX Assuming '!F' refers to the DF bit in the working SDW. It may refer to
                // the DF bit in the match trap.
                SETL1 (bank_a, 0+3, cpu.SDW0.DF);
                SETL (bank_a, 1+3, cpu.SDW0.FC, 2);

                // XXX Guessing that ACV is the fault set generated by the APU; saved in acvFaults
                word16 flts = (word16) cpu.acvFaults;
                SETL (bank_a, 3+3, flts, 16);
                SETL (bank_a, 30+3, cpu.TPR.TBR, 6);
              }
          }
          break;

        //  11 "STORE CONTROL UNIT-PAIR 1"
        case 10:
          {
            if (cpu.APU_panel_scroll_select_ul_sw)
              { // Upper
                // 0:35 CU word 0
                SETL (bank_a, 0+3, cpu.PPR.PRR, 3);
                SETL (bank_a, 3+3, cpu.PPR.PSR, 15);
                SETL1 (bank_a, 18+3, cpu.PPR.P);
                SETL1 (bank_a, 19+3, cpu.cu.XSF);
                SETL1 (bank_a, 20+3, cpu.cu.SDWAMM);
                SETL1 (bank_a, 21+3, cpu.cu.SD_ON);
                SETL1 (bank_a, 22+3, cpu.cu.PTWAMM);
                SETL1 (bank_a, 23+3, cpu.cu.PT_ON);
                SETL (bank_a, 24+3, cpu.cu.APUCycleBits, 12);
              }
            else
              { // Lower
                // 0:35 CU word 1
                SETL1 (bank_a,  0+3, cpu.cu.IRO_ISN);
                SETL1 (bank_a,  1+3, cpu.cu.OEB_IOC);
                SETL1 (bank_a,  2+3, cpu.cu.EOFF_IAIM);
                SETL1 (bank_a,  3+3, cpu.cu.ORB_ISP);
                SETL1 (bank_a,  4+3, cpu.cu.ROFF_IPR);
                SETL1 (bank_a,  5+3, cpu.cu.OWB_NEA);
                SETL1 (bank_a,  6+3, cpu.cu.WOFF_OOB);
                SETL1 (bank_a,  7+3, cpu.cu.NO_GA);
                SETL1 (bank_a,  8+3, cpu.cu.OCB);
                SETL1 (bank_a,  9+3, cpu.cu.OCALL);
                SETL1 (bank_a, 10+3, cpu.cu.BOC);
                SETL1 (bank_a, 11+3, cpu.cu.PTWAM_ER);
                SETL1 (bank_a, 12+3, cpu.cu.CRT);
                SETL1 (bank_a, 13+3, cpu.cu.RALR);
                SETL1 (bank_a, 14+3, cpu.cu.SDWAM_ER);
                SETL1 (bank_a, 15+3, cpu.cu.OOSB);
                SETL1 (bank_a, 16+3, cpu.cu.PARU);
                SETL1 (bank_a, 17+3, cpu.cu.PARL);
                SETL1 (bank_a, 18+3, cpu.cu.ONC1);
                SETL1 (bank_a, 19+3, cpu.cu.ONC2);
                SETL (bank_a, 20+3, cpu.cu.IA, 4);
                SETL (bank_a, 24+3, cpu.cu.IACHN, 3);
                SETL (bank_a, 27+3, cpu.cu.CNCHN, 3);
                SETL (bank_a, 30+3, cpu.cu.FI_ADDR, 5);
                word1 fi = cpu.cycle == INTERRUPT_cycle ? 0 : 1;
                SETL1 (bank_a, 35+3, fi);
              }
          }
          break;

        //  12 "STORE CONTROL UNIT-PAIR 2"
        case 11:
          {
            if (cpu.APU_panel_scroll_select_ul_sw)
              { // Upper
                // 0:35 CU word 2
                SETL (bank_a,  0+3,  cpu.TPR.TRR, 3);
                SETL (bank_a,  3+3, cpu.TPR.TSR, 15);
                SETL (bank_a, 27+3, (word3) cpu.switches.cpu_num, 3);
                SETL (bank_a, 30+3, cpu.cu.delta, 6);
              }
            else
              { // Lower
                // 0:35 CU word 3
                //SETL (bank_a, 18+3, cpu.cu.TSN_PRNO [0], 3);
                //SETL (bank_a, 21+3, cpu.cu.TSN_VALID [0], 1);
                //SETL (bank_a, 22+3, cpu.cu.TSN_PRNO [1], 3);
                //SETL (bank_a, 25+3, cpu.cu.TSN_VALID [1], 1);
                //SETL (bank_a, 26+3, cpu.cu.TSN_PRNO [2], 3);
                //SETL (bank_a, 29+3, cpu.cu.TSN_VALID [3], 1);
                SETL (bank_a, 30+3, cpu.TPR.TBR, 6);

              }
          }
          break;

        //  13 "STORE CONTROL UNIT-PAIR 3 (UPPER DOES NOT DISPLAY)"
        case 12:
          {
            if (cpu.APU_panel_scroll_select_ul_sw)
              { // Upper
                // 0:35 CU word 4
              }
            else
              { // Lower
                // 0:35 CU word 5
                SETL (bank_a,  0+3, cpu.TPR.CA, 18);
                SETL1 (bank_a, 18+3, cpu.cu.repeat_first);
                SETL1 (bank_a, 19+3, cpu.cu.rpt);
                SETL1 (bank_a, 20+3, cpu.cu.rd);
                SETL1 (bank_a, 21+3, cpu.cu.rl);
                SETL1 (bank_a, 22+3, cpu.cu.pot);
                // 23, 1 PON Prepare operand no tally // XXX Missing
                SETL1 (bank_a, 24+3, cpu.cu.xde);
                SETL1 (bank_a, 25+3, cpu.cu.xdo);
                // 26, 1 ITP Execute ITP indirect cycle // XXX Missing
                SETL1 (bank_a, 27+3, cpu.cu.rfi);
                // 28, 1 ITS Execute ITS indirect cycle // XXX Missing
                SETL1 (bank_a, 29+3, cpu.cu.FIF);
                SETL (bank_a, 30+3, cpu.cu.CT_HOLD, 6);
              }
          }
          break;

        //  14 "STORE CONTROL UNIT-PAIR 4"
// Jeff 1Feb17 "However, the APU SCROLL does not have a wire on the 14
// position, so 14 is going to show 15 instead of 13."
        //case 13:
        case 15:
          {
            if (cpu.APU_panel_scroll_select_ul_sw)
              { // Upper
                // 0:35 CU word 6
                SETL (bank_a,  0+3, cpu.cu.IWB, 36);
              }
            else
              { // Lower
                // 0:35 CU word 7
                SETL (bank_a,  0+3, cpu.cu.IRODD, 36);
              }
          }
          break;

        default:
          {
            time_t t = time (NULL);
            SETL (bank_a, 0+3, (word36) t, 36);
          }
          break;
      }
  }

static void update_cpts (void)
  {
    // switch ()
    // XXX
    switch (cpu.CP_panel_wheel_sw)
      {
        case 0:
          SETLA (bank_b, 0+3, cpu.cpt [0], 36);
          SETLA (bank_c, 0+3, cpu.cpt [1], 36);
          break;
        case 1:
          SETLA (bank_b, 0+3, cpu.cpt [2], 36);
          SETLA (bank_c, 0+3, cpu.cpt [3], 36);
          break;
        case 2:
          SETLA (bank_b, 0+3, cpu.cpt [4], 36);
          SETLA (bank_c, 0+3, cpu.cpt [5], 36);
          break;
        case 3:
          SETLA (bank_b, 0+3, cpu.cpt [6], 36);
          SETLA (bank_c, 0+3, cpu.cpt [7], 36);
          break;
        case 4:
          SETLA (bank_b, 0+3, cpu.cpt [8], 36);
          SETLA (bank_c, 0+3, cpu.cpt [9], 36);
          break;
        case 5:
          SETLA (bank_b, 0+3, cpu.cpt [10], 36);
          SETLA (bank_c, 0+3, cpu.cpt [11], 36);
          break;
        case 6:
          SETLA (bank_b, 0+3, cpu.cpt [12], 36);
          SETLA (bank_c, 0+3, cpu.cpt [13], 36);
          break;
        case 7:
          SETLA (bank_b, 0+3, cpu.cpt [14], 36);
          SETLA (bank_c, 0+3, cpu.cpt [15], 36);
          break;
        case 8:
          SETLA (bank_b, 0+3, cpu.cpt [16], 36);
          SETLA (bank_c, 0+3, cpu.cpt [17], 36);
          break;
        case 9:
          SETLA (bank_b, 0+3, cpu.cpt [18], 36);
          SETLA (bank_c, 0+3, cpu.cpt [19], 36);
          break;
        case 10:
          SETLA (bank_b, 0+3, cpu.cpt [20], 36);
          SETLA (bank_c, 0+3, cpu.cpt [21], 36);
          break;
        case 11:
          SETLA (bank_b, 0+3, cpu.cpt [22], 36);
          SETLA (bank_c, 0+3, cpu.cpt [23], 36);
          break;
        case 12:
          SETLA (bank_b, 0+3, cpu.cpt [24], 36);
          SETLA (bank_c, 0+3, cpu.cpt [25], 36);
          break;
        case 13:
          SETLA (bank_b, 0+3, cpu.cpt [26], 36);
          SETLA (bank_c, 0+3, cpu.cpt [27], 36);
          break;
      }
    //SETL (bank_b, 0, cpu.rA, 36);
    SETL1 (bank_b, 36+3, cpu.panel4_red_ready_light_state);
    //SETL (bank_c, 0, cpu.rA, 36);
    SETL1 (bank_c, 36+3, cpu.panel7_enabled_light_state);
  }

static void update_data_1 (void)
  {
    // XXX No idea what data is supposed to be displayed; displaying the address and data.
    SETL (bank_d, 0+3, cpu.portAddr [cpu.DATA_panel_d1_sw], 24);
    SETL (bank_e, 0+3, cpu.portData [cpu.DATA_panel_d1_sw], 36);
  }

static void update_data_2 (void)
  {
    // BAR/ADR0    ADR1   ADR2   ADR3   ADR4   ADR5   ADR6   ADR7   CACHE
    // XXX No idea
    // XXX BAR No idea
    // CACHE No idea
    // XXX PAR registers?
    // XXX upper: PR
    //   0- 5: BITNO
    //   6-17: SNR
    //  18-35: WORDNO
    // XXX lower:
    //   0-17: WORDNO
    //  18-19: CHAR
    //  20-23: BITNO
    //
    SETL (bank_d, 0+3, cpu.PAR[cpu.DATA_panel_d2_sw].PR_BITNO, 6);
    SETL (bank_d, 6+3, cpu.PAR[cpu.DATA_panel_d2_sw].SNR, 15);
    SETL (bank_d, 18+3, cpu.PAR[cpu.DATA_panel_d2_sw].WORDNO, 18);

    SETL (bank_e, 0+3, cpu.PAR[cpu.DATA_panel_d2_sw].WORDNO, 18);
    SETL (bank_e, 18+3, cpu.PAR[cpu.DATA_panel_d2_sw].AR_CHAR, 2);
    SETL (bank_e, 20+3, cpu.PAR[cpu.DATA_panel_d2_sw].AR_BITNO, 4);
  }

static void update_data_3 (void)
  {
    // PTR/LEN1    PTR/LEN2   PTR/LEN2   ZX/LEN   ZER/ZGR   EA2   ERR/TIMER   SCR/LVL   IBICT   TALLY
    // Decimal unit data.
    switch (cpu.DATA_panel_d3_sw)
      {
        case 0: // PTR_LEN 1
          // 1U
          // XXX Guessing CU PTR 1
          // XXX 0-18 D1_PTR_W
          SETL (bank_d, 0+3, cpu.du.D1_PTR_W, 18);
          // XXX 18-35 probably D1_PTR_B, not tracked.
          // 1L
          // XXX No idea, probably LEVEL and D1_RES, which are not tracked
          break;
        case 1: // PTR_LEN 2
          // 2U
          // XXX Guessing CU PTR 2
          // XXX 0-18 D2_PTR_W
          SETL (bank_d, 0+3, cpu.du.D2_PTR_W, 18);
          // XXX 18-35 probably D2_PTR_B, not tracked.
          // 2L
          // XXX No idea, probably LEVEL and D2_RES, which are not tracked
          break;
        case 2: // PTR_LEN 3
          // 3U
          // XXX Guessing CU PTR 3
          // XXX 0-18 D3_PTR_W
          SETL (bank_d, 0+3, cpu.du.D3_PTR_W, 18);
          // XXX 18-35 probably D3_PTR_B, not tracked.
          // 3L
          // XXX No idea, probably LEVEL and D1_RES, which are not tracked
          break;
        case 3: // ZX/LEN
          // XXX No idea
          break;
        case 4: // ZER/ZGR
          // XXX No idea
          break;
        case 5: // EA2
          // XXX No idea
          break;
        case 6: // ERR/TIMER
          // XXX 6U No idea
          // XXX 6L Guessing RT
          SETL (bank_e, 0+3, cpu.rTR, 27);
          break;
        case 7: // SCR/LVL
          // XXX No idea
          break;
        case 8: // IBICT
          // XXX No idea Ins. Buffer IC Tracker?
          // XXX Guess U IBW_IRODD, L IC
          SETL (bank_d, 0+3, IWB_IRODD, 36);
          SETL (bank_e, 0+3, cpu.PPR.IC, 18);
          break;
        case 9: // TALLY
          // XXX No idea; addrmod TALLY? DU CHTALLY?
          SETL (bank_e, 0+3, cpu.AM_tally, 12);
          break;
      }
  }

static void update_data_4 (void)
  {
    // XXX No idea
  }

static void update_data_5 (void)
  {
    switch (cpu.DATA_panel_d3_sw)
      {
        case 0: // ZUPK
          // XXX No idea
          break;
        case 1: // RALN
          // XXX No idea
          break;
        case 2: // ZALN
          // XXX No idea
          break;
        case 3: // ZBLN
          // XXX No idea
          break;
        case 4: // ZPKS
          // XXX No idea
          break;
        case 5: // R1WRT
          // XXX No idea
          break;
        case 6: // R2WRT
          // XXX No idea
          break;
        case 7: // CH-TALLY
          SETL (bank_d, 0+3, cpu.du.CHTALLY, 24);
          break;
        case 8: // ZAS
          // XXX No idea
          break;
      }
  }

static void update_data_6 (void)
  {
    // XXX No idea
  }

static void update_data_7 (void)
  {
    // XXX No idea
  }

static void update_data_scroll (void)
  {
    switch (cpu.DATA_panel_wheel_sw)
      {
        case 0: // 1 AQ
          {
            SETL (bank_d, 0+3, cpu.rA, 36);
            SETL (bank_e, 0+3, cpu.rQ, 36);
          }
          break;

        case 1: // 2 X0/X1/X2/X3
          {
            SETL (bank_d, 0+3, cpu.rX[0], 18);
            SETL (bank_d, 18+3, cpu.rX[1], 18);
            SETL (bank_e, 0+3, cpu.rX[2], 18);
            SETL (bank_e, 18+3, cpu.rX[3], 18);
          }
          break;

        case 2: // 3 X4/X5/X6/X7
          {
            SETL (bank_d, 0+3, cpu.rX[4], 18);
            SETL (bank_d, 18+3, cpu.rX[5], 18);
            SETL (bank_e, 0+3, cpu.rX[6], 18);
            SETL (bank_e, 18+3, cpu.rX[7], 18);
          }
          break;

        case 3: // 4 EAQ
          {
            SETL (bank_d, 0+3, cpu.rE, 8);
            SETL (bank_d, 8+3, cpu.rA >> 8, 36 - 8);
            SETL (bank_e, 0+3, cpu.rA, 8);
            SETL (bank_e, 0+3, cpu.rQ >> 8, 36 - 8);
          }
          break;

        case 4: // 5 EA
          {
            // XXX Guessing rE
            SETL (bank_d, 0+3, cpu.rE, 8);
          }
          break;

        case 5: // 6 RACT
          {
            // XXX No idea
          }
          break;

        case 6: // 7
          {
            // The contents of the APU history register
//printf ("%u\n", cpu.DATA_panel_hr_sel_sw);
            SETL (bank_d, 0+3, cpu.history[L68_APU_HIST_REG][cpu.DATA_panel_hr_sel_sw][0], 36);
            SETL (bank_e, 0+3, cpu.history[L68_APU_HIST_REG][cpu.DATA_panel_hr_sel_sw][1], 36);
#if 0
            // 7U
            // This is the same data that is sent to the APU history register
            // 0:14 ESN
            SETL (bank_d, 0+3, cpu.TPR.TSR, 15);
            // 15 !SNR
            SETL1 (bank_d, 15+3, ! (cpu.apu.state & apu_ESN_SNR));
            // 16 !TSR
            SETL1 (bank_d, 16+3, ! (cpu.apu.state & apu_ESN_TSR));
            // 17 FDPT
            SETL1 (bank_d, 17+3, !! (cpu.apu.state & apu_FDPT));
            // 18 MDPT
            SETL1 (bank_d, 18+3, !! (cpu.apu.state & apu_MDPT));
            // 19 FSDW
            SETL1 (bank_d, 19+3, !! (cpu.apu.state & apu_FSDP));
            // 20 FPTW
            SETL1 (bank_d, 20+3, !! (cpu.apu.state & apu_FPTW));
            // 21 FPT2 XXX Not tracked
            // 22 MPTW
            SETL1 (bank_d, 22+3, !! (cpu.apu.state & apu_MPTW));
            // 23 FANP
            SETL1 (bank_d, 23+3, !! (cpu.apu.state & apu_FANP));
            // 24 FAP
            SETL1 (bank_d, 24+3, !! (cpu.apu.state & apu_FAP));
            // 25 SDMF  XXX Guessing SDWAMM
            SETL1 (bank_d, 25+3, !! cpu.cu.SDWAMM);
            // 26:29 MADR-SDW  Guessing SDWAMR
            SETL (bank_d, 26+3, cpu.SDWAMR, 4);
            // 30 PTMF  XXX Guessing PTWAMM
            SETL1 (bank_d, 30+3, !! cpu.cu.PTWAMM);
            // 31:34 MADR-PTW  XXX Guessing PTWAMR
            SETL (bank_d, 31+3, cpu.PTWAMR, 4);
            // 35 DACV+DF  XXX Guess
            SETL1 (bank_d, 35+3, cpu.DACVpDF);

            // 7L
            // 0:23 RMA XXX Guess FA
            SETL (bank_e, 0, cpu.APUDataBusAddr, 24);
            // 24:26 TRR
            SETL (bank_e, 24+3,  cpu.TPR.TRR, 3);
            // 34 FLT HLD  XXX Guess
            SETL1 (bank_e, 35+3, !! (cpu.apu.state & apu_FLT));
#endif
          }
          break;

        case 7: // 8
          {
            // 8U Empty
            // 8L 0:35 DIRECT BUFFER XXX GUess directOperand
            SETL (bank_e, 0+3, cpu.ou.directOperand, 36);

          }
          break;

        case 8: // 9
          {
            // 9U 0:31 High 32 bits of the FAULT REGISTER
            SETL (bank_d, 0+3, cpu.faultRegister [0] & 0777777777760LLU, 36);
            // 9L Empty
          }
          break;

        case 9: // 10
          {
            // XXX The upper and lower are the same thing?
            SETL (bank_d, 0+3, cpu.MR.r, 36);
            memcpy (bank_e, bank_d, sizeof (bank_e));
          }
          break;

        case 10: // 11
          {
            // The contents of the CU history register
            SETL (bank_d, 0+3, cpu.history[CU_HIST_REG][cpu.DATA_panel_hr_sel_sw][0], 36);
            SETL (bank_e, 0+3, cpu.history[CU_HIST_REG][cpu.DATA_panel_hr_sel_sw][1], 36);
          }
          break;

        case 11: // 12
          {
            // The contents of the OU history register
            SETL (bank_d, 0+3, cpu.history[L68_OU_HIST_REG][cpu.DATA_panel_hr_sel_sw][0], 36);
            SETL (bank_e, 0+3, cpu.history[L68_OU_HIST_REG][cpu.DATA_panel_hr_sel_sw][1], 36);
          }
          break;

        case 12: // 13
          {
            // The contents of the DU history register
            SETL (bank_d, 0+3, cpu.history[L68_DU_HIST_REG][cpu.DATA_panel_hr_sel_sw][0], 36);
            SETL (bank_e, 0+3, cpu.history[L68_DU_HIST_REG][cpu.DATA_panel_hr_sel_sw][1], 36);
          }
          break;

        case 13: // 14
          {

            SETL (bank_d, 0+3, cpu.cu.IWB, 36);
            SETL (bank_e, 0+3, cpu.cu.IRODD, 36);
          }
          break;
      }
  }

static void update_data (void)
  {
    switch (cpu.DATA_panel_ds_sw)
      {
        case 0: // #1
          update_data_1 ();
          break;
        case 1: // #2
          update_data_2 ();
          break;
        case 2: // #3
          update_data_3 ();
          break;
        case 3: // SCROLL
          update_data_scroll ();
          break;
        case 4: // #4
          update_data_4 ();
          break;
        case 5: // #5
          update_data_5 ();
          break;
        case 6: // #6
          update_data_6 ();
          break;
        case 7: // #7
          update_data_7 ();
          break;
        default:
          break;
      }
  }

static void update_display (void)
  {
    SETL (bank_f, 0, cpu.cu.IR, 18);
    SETL (bank_f, 18, cpu.history_cyclic [0], 4);
    SETL (bank_f, 22, cpu.history_cyclic [1], 4);
    SETL (bank_f, 26, cpu.history_cyclic [2], 4);
    SETL (bank_f, 30, cpu.history_cyclic [3], 4);
    SETL (bank_g,  0, cpu.PPR.IC, 18);

    // XXX Simplifying assumption: 4 * 4MW SCUS, ports DEFG idle.
    SETL (bank_g, 18, cpu.portSelect == 0 ? 1 : 0, 1);
    SETL (bank_g, 19, cpu.portSelect == 1 ? 1 : 0, 1);
    SETL (bank_g, 20, cpu.portSelect == 2 ? 1 : 0, 1);
    SETL (bank_g, 21, cpu.portSelect == 3 ? 1 : 0, 1);

    // XXX OU/DU/CU/APU ST/LD no idea

    // XXX EXC INT Guess execute interrupt
    bool exc_int = cpu.cycle == INTERRUPT_cycle ||
                   cpu.cycle == INTERRUPT_EXEC_cycle /*||
                   cpu.cycle == INTERRUPT_EXEC2_cycle*/;
    SETL (bank_g, 34, exc_int ? 1 : 0, 1);

    // XXX INS FCH Guess instruction fetch
    bool ins_fch = cpu.cycle == INTERRUPT_cycle ||
                   cpu.cycle == FAULT_cycle ||
                   cpu.cycle == FETCH_cycle;
    SETL (bank_g, 35, ins_fch ? 1 : 0, 1);

    // XXX DIR BUF no idea. DIRECT OPERAND?

    // XXX RSW 1/2 no idea. y from the last RSW instruction?

    // XXX MP CYL no idea.

    SETL (bank_h, 3, cpu.cu.IWB, 36);

    // XXX Guess: showing EIS descriptor #1
    SETL (bank_i, 3, cpu.IWRAddr, 36);

    // XXX Guess: IWR ADDR is CA
    SETL (bank_j, 3, cpu.PPR.IC, 18);

// prepare_state is PIA POA         RIW SIW POT PON RAW SAW
// display is       PIA POA POL POP RIW SIW POT PON RAW SAW
    word2 pia_poa = cpu.prepare_state >> 6;
    SETL (bank_j, 21, pia_poa, 2);
    SETL (bank_j, 23, cpu.du.POL, 2);
    SETL (bank_j, 24, cpu.du.POP, 1);
    SETL (bank_j, 25, cpu.prepare_state, 6);

    SETL (bank_j, 31, cpu.wasXfer, 1);
    SETL (bank_j, 32, cpu.cu.xde, 1);
    SETL (bank_j, 33, cpu.cu.xdo, 1);
    SETL1 (bank_j, 34, USE_IRODD);
    SETL (bank_j, 35, cpu.cu.rpt, 1);
    SETL (bank_j, 36, cpu.cu.rl, 1);
    SETL (bank_j, 37, cpu.cu.rd, 1);
    // XXX M/S Master/Slave (BAR)?
    SETL (bank_j, 38, TSTF (cpu.cu.IR, I_NBAR), 1);

    // XXX Looks like EIS state data
    // XXX D1 Descriptor #1, SEL DMA DMB FRS A/1 RD1 no idea
    // XXX D1 Descriptor #2, SEL DMA DMB FRS A/1 RD1 no idea
    // XXX D1 Descriptor #3, SEL DMA DMB FRS A/1 RD1 no idea
    SETL (bank_k, 21, cpu.dataMode, 7);
    // XXX CTA CTB CT1 CTL no idea
    // XXX L<W pg 60 '-DLVL>WD-SZ load with count less than word size'
    // XXX EXH Exhaust tally/count
    // XXX EXR IDR RD END

    // 0:17 CA
    // ZONE XXX no idea
    // COMMAND XXX no idea
    // ZAC/PAR XXX no idea
    SETL (bank_l,  3, cpu.TPR.CA, 18);

    SETL (bank_m, 3, cpu.ou.RS, 9);
    SETL (bank_m, 12, cpu.ou.characterOperandSize ? 1 : 0, 1);
    SETL (bank_m, 13, cpu.ou.characterOperandOffset & MASK3, 3);
    SETL (bank_m, 16, cpu.ou.crflag, 1);
    SETL (bank_m, 17, cpu.ou.directOperandFlag ? 1 : 0, 1);
    SETL (bank_m, 18, cpu.ou.RS, 9);
    SETL (bank_m, 27, cpu.ou.cycle, 9);
    SETL (bank_m, 36, cpu.ou.RB1_FULL, 1);
    SETL (bank_m, 37, cpu.ou.RP_FULL, 1);
    SETL (bank_m, 38, cpu.ou.RS_FULL, 1);

    //    Label OU Hist
    //  0 FDUD  FDUD     Decimal Unit Idle
    SETL1 (bank_n, 3, cpu.du.cycle2 & du2_DUD);
    //  1 GDLD
    //  2 GLP1
    //  3 GLP2
    //  4 GEA1  FA/Il Descriptor l active
    SETL1 (bank_n, 7, cpu.du.cycle1 & du1_FA_I1);
    //  5 GEM1
    //  6 GED1
    //  7 GDB   DGDB Decimal to binary execution gate
    SETL1 (bank_n, 10, cpu.du.cycle2 & du2_DGDB);
    //  8 GBD   DGBD Binary to decimal execution gate
    SETL1 (bank_n, 11, cpu.du.cycle2 & du2_DGBD);
    //  9 GSP
    // 10 GED2
    // 11 GEA2  FA/I2 Descriptor 2 active
    SETL1 (bank_n, 14, cpu.du.cycle1 & du1_FA_I2);
    // 12 GADD
    // 13 GCMP
    // 14 GMSY
    // 15 GMA
    // 16 GMS
    // 17 GQDF
    // 18 GQPA
    // 19 GQR1
    // 20 GQR2
    // 21 GRC
    // 22 GRND
    // 23 GCLZ
    // 24 GEDJ
    // 25 GEA3  FA/I3 Descriptor 3 active
    SETL1 (bank_n, 28, cpu.du.cycle1 & du1_FA_I3);
    // 26 GEAM
    // 27 GEDC
    // 28 GSTR  GSTR Numeric store gate
    SETL1 (bank_n, 31, cpu.du.cycle2 & du2_GSTR);
    // 29 GSDR
    // 30 NSTR  ANSTR Alphanumeric store gate
    SETL1 (bank_n, 32, cpu.du.cycle2 & du2_ANSTR);
    // 31 SDUD
    // 32 ?
    // 33 ?
    // 34 ?
    // 35 FLTG
    // 36 FRND  FRND Rounding flag
    SETL1 (bank_n, 39, cpu.du.cycle2 & du2_FRND);

    //  0 ALD1  ANLD1 Alphanumeric operand one load gate
    SETL1 (bank_o, 3, cpu.du.cycle2 & du2_ANLD1);
    //  1 ALD2  ANLD2 Alphanumeric operand one load gate
    SETL1 (bank_o, 4, cpu.du.cycle2 & du2_ANLD2);
    //  2 NLD1  GLDP1 Numeric operand one load gate
    SETL1 (bank_o, 5, cpu.du.cycle2 & du2_NLD1);
    //  3 NLD2  GLDP2 Numeric operand one load gate
    SETL1 (bank_o, 6, cpu.du.cycle2 & du2_NLD2);
    //  4 LWT1  LDWRT1 Load rewrite register one gate
    SETL1 (bank_o, 7, cpu.du.cycle2 & du2_LDWRT1);
    //  5 LWT2  LDWRT2 Load rewrite register two gate
    SETL1 (bank_o, 8, cpu.du.cycle2 & du2_LDWRT2);
    //  6 ASTR
    //  7 ANPK
    //  8 FGCH
    //  9 XMOP  FEXMOP Execute MOP gate
    SETL1 (bank_o, 12, cpu.du.cycle2 & du2_FEXOP);
    // 10 BLNK  FBLNK Blanking gate
    SETL1 (bank_o, 13, cpu.du.cycle2 & du2_FBLNK);
    // 11 ?
    // 12 ?
    // 13 CS=0
    // 14 CU=0
    // 15 FI=0
    // 16 CU=V
    // 17 UM<V
    // 18 ?
    // 19 ?
    // 20 ?
    // 21 ?
    // 22 ?
    // 23 ?
    // 24 ?
    // 25 ?
    // 26 ?
    // 27 ?
    // 28 L<128  -FLEN<128 Length less than 128
    SETL1 (bank_o, 31, !(cpu.du.cycle2 & du2_nFLEN_128));
    // 29 END SEQ  -FEND-SEQ End sequence flag
    SETL1 (bank_o, 32, !(cpu.du.cycle2 & du2_nFEND_SEQ));
    // 30 ?
    // 31 ?
    // 32 ?
    // 33 ?
    // 34 ?
    // 35 ?
    // 36 ?

    SETL (bank_p, 3, cpu.apu.state, 34);

    // XXX No idea
  }

#define Z(z) memset (z, 0, sizeof (z))
#define O(z) memset (z, 1, sizeof (z))

#define nibble(bank, n) \
  '@' + \
  (bank [n + 0] ? 020 : 0) + \
  (bank [n + 1] ? 010 : 0) + \
  (bank [n + 2] ? 004 : 0) + \
  (bank [n + 3] ? 002 : 0) + \
  (bank [n + 4] ? 001 : 0)

#define nibbles(bank) nibble (bank,  0), nibble (bank,  5), \
                      nibble (bank, 10), nibble (bank, 15), \
                      nibble (bank, 20), nibble (bank, 25), \
                      nibble (bank, 30), nibble (bank, 35)

static void lwrite (FD fd, const void * buf, size_t count)
  {
#if defined(__MINGW64__) || defined(__MINGW32__)
    DWORD bytes_written;
    if(!WriteFile(fd, buf, count, &bytes_written, NULL))
    {
        fprintf(stderr, "Error\n");
        CloseHandle(fd);
        exit (1);
    }
#else
    write (fd, buf, count);
#endif
  }

static void send_lamp_data (FD fd)
  {
    char buf [129];
    sprintf (buf, "0%c%c%c%c%c%c%c%c\n", nibbles (bank_a));
    lwrite (fd, buf, 10);
    sprintf (buf, "1%c%c%c%c%c%c%c%c\n", nibbles (bank_b));
    lwrite (fd, buf, 10);
    sprintf (buf, "2%c%c%c%c%c%c%c%c\n", nibbles (bank_c));
    lwrite (fd, buf, 10);
    sprintf (buf, "3%c%c%c%c%c%c%c%c\n", nibbles (bank_d));
    lwrite (fd, buf, 10);
    sprintf (buf, "4%c%c%c%c%c%c%c%c\n", nibbles (bank_e));
    lwrite (fd, buf, 10);
    sprintf (buf, "5%c%c%c%c%c%c%c%c\n", nibbles (bank_f));
    lwrite (fd, buf, 10);
    sprintf (buf, "6%c%c%c%c%c%c%c%c\n", nibbles (bank_g));
    lwrite (fd, buf, 10);
    sprintf (buf, "7%c%c%c%c%c%c%c%c\n", nibbles (bank_h));
    lwrite (fd, buf, 10);
    sprintf (buf, "8%c%c%c%c%c%c%c%c\n", nibbles (bank_i));
    lwrite (fd, buf, 10);
    sprintf (buf, "9%c%c%c%c%c%c%c%c\n", nibbles (bank_j));
    lwrite (fd, buf, 10);
    sprintf (buf, ":%c%c%c%c%c%c%c%c\n", nibbles (bank_k));
    lwrite (fd, buf, 10);
    sprintf (buf, ";%c%c%c%c%c%c%c%c\n", nibbles (bank_l));
    lwrite (fd, buf, 10);
    sprintf (buf, "<%c%c%c%c%c%c%c%c\n", nibbles (bank_m));
    lwrite (fd, buf, 10);
    sprintf (buf, "=%c%c%c%c%c%c%c%c\n", nibbles (bank_n));
    lwrite (fd, buf, 10);
    sprintf (buf, ">%c%c%c%c%c%c%c%c\n", nibbles (bank_o));
    lwrite (fd, buf, 10);
    sprintf (buf, "?%c%c%c%c%c%c%c%c\n", nibbles (bank_p));
    lwrite (fd, buf, 10);
#if defined(__MINGW64__) || defined(__MINGW32__)
#else
    syncfs (fd);
#endif
  }

static void set_message (void)
  {
    int nchars = (int) strlen (message);
    int nbits = nchars * 8;
    int r, c;

#define start_row 7
    for (r = 0; r < 0 + 8; r ++)
      {
        for (c = 0; c < ncols; c ++)
          {
            int c2 = (c + c_offset) % nbits;
            int char2 = c2 / 8;
            unsigned char fontbits = font8x8_basic [(unsigned int) message [(unsigned int) char2]] [r];
            int whichbit = c2 % 8;
            unsigned char bit1 = fontbits & (1 << (/*7 -*/ whichbit));
            banks [r + start_row] [c] = !!bit1;
//printw ("r %d c %d c2 %d char2 %d fontbits %02x whichbit %d bit1 %02x\n", r, c, c2, char2, fontbits, whichbit, bit1);
          }
      }
    c_offset = (c_offset + 1) % nbits;
  }

static int time_handler (FD fd)
  {
    //if (cpu.DATA_panel_trackers_sw == 0) // A
if (0)
      {
        int sec1 = (int) (clock () / CLOCKS_PER_SEC) & 1;
        if (sec1)
          {
            Z (bank_a);
            Z (bank_b);
            Z (bank_c);
            Z (bank_d);
            Z (bank_e);
            Z (bank_f);
            Z (bank_g);
            Z (bank_h);
            Z (bank_i);
            Z (bank_j);
            Z (bank_k);
            Z (bank_l);
            Z (bank_m);
            Z (bank_n);
            Z (bank_o);
            Z (bank_p);
          }
        else
          {
            O (bank_a);
            O (bank_b);
            O (bank_c);
            O (bank_d);
            O (bank_e);
            O (bank_f);
            O (bank_g);
            O (bank_h);
            O (bank_i);
            O (bank_j);
            O (bank_k);
            O (bank_l);
            O (bank_m);
            O (bank_n);
            O (bank_o);
            O (bank_p);
          }
        send_lamp_data (fd);
        return TRUE;
      }

    Z (bank_a);
    Z (bank_b);
    Z (bank_c);
    Z (bank_d);
    Z (bank_e);
    Z (bank_f);
    Z (bank_g);
    Z (bank_h);
    Z (bank_i);
    Z (bank_j);
    Z (bank_k);
    Z (bank_l);
    Z (bank_m);
    Z (bank_n);
    Z (bank_o);
    Z (bank_p);

    update_apu ();
    update_cpts ();
    update_data ();
    update_display ();
    if (message [0])
      set_message ();
    send_lamp_data (fd);
    return TRUE;
  }

static unsigned char port_buf [9];
static bool port_data [40];
static int port_state = 0;

static void update_port (FD port_fd)
  {
more:;
#if !defined(__MINGW64__)
# if !defined(__MINGW32__)
    if (poll (& (struct pollfd) { .fd = port_fd, .events = POLLIN }, 1, 0) == 1)
# endif
#endif
      {
        unsigned char ch;
#if defined(__MINGW64__) || defined(__MINGW32__)
        DWORD bytes_read;
        ReadFile(port_fd, &ch, 1, &bytes_read, NULL);
        ssize_t nr = bytes_read;
#else
        ssize_t nr = read (port_fd, & ch, 1);
#endif
        if (nr != 1)
          {
            static int first = 1;
            if (first)
              {
                perror ("read");
                first = 0;
              }
            port_state = 0;
            return;
            //exit (1);
          }
//printf ("%c", ch);
        if (ch & 040)
          {
            port_state = 0;
          }
        if (port_state < 0 || port_state > 8)
          {
            //exit (1);
            port_state = 0;
            return;
          }
        port_buf [port_state ++] = ch;
        if (port_state < 9)
          goto more;
//printf ("\n");
//printf ("got one\n");

        //for (int i = 0; i < 9; i ++)
          //printf ("%c", port_buf [i]);
        //printf ("\n");

        port_data [ 0] = !! (port_buf [1] & 020);
        port_data [ 1] = !! (port_buf [1] & 010);
        port_data [ 2] = !! (port_buf [1] & 004);
        port_data [ 3] = !! (port_buf [1] & 002);
        port_data [ 4] = !! (port_buf [1] & 001);

        port_data [ 5] = !! (port_buf [2] & 020);
        port_data [ 6] = !! (port_buf [2] & 010);
        port_data [ 7] = !! (port_buf [2] & 004);
        port_data [ 8] = !! (port_buf [2] & 002);
        port_data [ 9] = !! (port_buf [2] & 001);

        port_data [10] = !! (port_buf [3] & 020);
        port_data [11] = !! (port_buf [3] & 010);
        port_data [12] = !! (port_buf [3] & 004);
        port_data [13] = !! (port_buf [3] & 002);
        port_data [14] = !! (port_buf [3] & 001);

        port_data [15] = !! (port_buf [4] & 020);
        port_data [16] = !! (port_buf [4] & 010);
        port_data [17] = !! (port_buf [4] & 004);
        port_data [18] = !! (port_buf [4] & 002);
        port_data [19] = !! (port_buf [4] & 001);

        port_data [20] = !! (port_buf [5] & 020);
        port_data [21] = !! (port_buf [5] & 010);
        port_data [22] = !! (port_buf [5] & 004);
        port_data [23] = !! (port_buf [5] & 002);
        port_data [24] = !! (port_buf [5] & 001);

        port_data [25] = !! (port_buf [6] & 020);
        port_data [26] = !! (port_buf [6] & 010);
        port_data [27] = !! (port_buf [6] & 004);
        port_data [28] = !! (port_buf [6] & 002);
        port_data [29] = !! (port_buf [6] & 001);

        port_data [30] = !! (port_buf [7] & 020);
        port_data [31] = !! (port_buf [7] & 010);
        port_data [32] = !! (port_buf [7] & 004);
        port_data [33] = !! (port_buf [7] & 002);
        port_data [34] = !! (port_buf [7] & 001);

        port_data [35] = !! (port_buf [8] & 020);
        port_data [36] = !! (port_buf [8] & 010);
        port_data [37] = !! (port_buf [8] & 004);
        port_data [38] = !! (port_buf [8] & 002);
        port_data [39] = !! (port_buf [8] & 001);

        //for (int i = 0; i < 40; i ++)
          //printf ("%d", port_data [i] ? 1 : 0);
        //printf ("\n");

        port_state = 0;

#define SETS(word, data, from, n) \
  for (uint i = 0; i < (n); i ++) \
    if (data [from + i]) \
      (word) |= 1llu << ((n) - 1 - i); \
    else \
      (word) &= ~ (1llu << ((n) - 1 - i))

        switch (port_buf [0])
          {
            case '0': // A
              SETS (cpu.APU_panel_segno_sw, port_data, 0, 15);
              SETS (cpu.APU_panel_enable_match_ptw_sw, port_data, 15, 1);
              SETS (cpu.APU_panel_enable_match_sdw_sw, port_data, 16, 1);
              SETS (cpu.APU_panel_scroll_select_ul_sw, port_data, 17, 1);
              SETS (cpu.APU_panel_scroll_select_n_sw, port_data, 18, 4);
              SETS (cpu.APU_panel_scroll_wheel_sw, port_data, 22, 4);
              break;

            case '1': // B
              SETS (cpu.switches.data_switches, port_data, 0, 36);
              break;

            case '2': // C
              SETS (cpu.switches.addr_switches, port_data, 0, 18);
              SETS (cpu.APU_panel_enter_sw, port_data, 18, 1);
              SETS (cpu.APU_panel_display_sw, port_data, 19, 1);
              SETS (cpu.panelInitialize, port_data, 20, 1);
              SETS (cpu.CP_panel_wheel_sw, port_data, 21, 4);
              break;

            case '3': // D
              SETS (cpu.DATA_panel_ds_sw, port_data, 0, 4);
        //for (int i = 0; i < 40; i ++)
          //printf ("%d", port_data [i] ? 1 : 0);
        //printf ("\n");
//printf ("set to %d\n", cpu.DATA_panel_ds_sw);
              switch (cpu.DATA_panel_ds_sw)
                {
                  case 0:
                    SETS (cpu.DATA_panel_d1_sw, port_data, 4, 4);
                    break;
                  case 1:
                    SETS (cpu.DATA_panel_d2_sw, port_data, 4, 4);
                    break;
                  case 2:
                    SETS (cpu.DATA_panel_d3_sw, port_data, 4, 4);
                    break;
                  case 3:
                    SETS (cpu.DATA_panel_wheel_sw, port_data, 4, 4);
                    break;
                  case 4:
                    SETS (cpu.DATA_panel_d4_sw, port_data, 4, 4);
                    break;
                  case 5:
                    SETS (cpu.DATA_panel_d5_sw, port_data, 4, 4);
                    break;
                  case 6:
                    SETS (cpu.DATA_panel_d6_sw, port_data, 4, 4);
                    break;
                  case 7:
                    SETS (cpu.DATA_panel_d7_sw, port_data, 4, 4);
                    break;
                }
              SETS (cpu.DATA_panel_addr_stop_sw, port_data, 8, 4);
              SETS (cpu.DATA_panel_enable_sw, port_data, 12, 1);
              SETS (cpu.DATA_panel_validate_sw, port_data, 13, 1);
              SETS (cpu.DATA_panel_auto_fast_sw, port_data, 14, 1);
              SETS (cpu.DATA_panel_auto_slow_sw, port_data, 15, 1);
              SETS (cpu.DATA_panel_cycle_sw, port_data, 16, 3);
              SETS (cpu.DATA_panel_step_sw, port_data, 19, 1);
              SETS (cpu.DATA_panel_s_trig_sw, port_data, 20, 1);
              SETS (cpu.DATA_panel_execute_sw, port_data, 21, 1);
              SETS (cpu.DATA_panel_scope_sw, port_data, 22, 1);
              SETS (cpu.DATA_panel_init_sw, port_data, 23, 1);
              SETS (cpu.DATA_panel_exec_sw, port_data, 24, 1);
              SETS (cpu.DATA_panel_hr_sel_sw, port_data, 25, 4);
              SETS (cpu.DATA_panel_trackers_sw, port_data, 29, 4);
              break;
          }
        goto more;
      }
  }

static volatile bool scraperRunning = false;
static volatile bool scraperCancel = false;

static void * scraperThreadMain (void * arg)
  {
    sim_printf ("panel scraper\n");
    uint cpuNum = (uint) * (int *) arg;
    panel_cpup = cpus + cpuNum;

    const char *device = "/dev/ttyUSB1";

    struct termios  config;
    int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd == -1)
      {
        printf( "failed to open port\n" );
        goto die;
      }
    if(tcgetattr(fd, &config) < 0)
      {
        printf ("can't get\n");
        close (fd);
        goto die;
      }

    // Input flags - Turn off input processing
    //
    // convert break to null byte, no CR to NL translation,
    // no NL to CR translation, don't mark parity errors or breaks
    // no input parity check, don't strip high bit off,
    // no XON/XOFF software flow control
    //
    config.c_iflag &= ~(unsigned int) (IGNBRK | BRKINT | ICRNL |
                        INLCR | PARMRK | INPCK | ISTRIP | IXON);

    //
    // Output flags - Turn off output processing
    //
    // no CR to NL translation, no NL to CR-NL translation,
    // no NL to CR translation, no column 0 CR suppression,
    // no Ctrl-D suppression, no fill characters, no case mapping,
    // no local output processing
    //
    // config.c_oflag &= ~(OCRNL | ONLCR | ONLRET |
    //                     ONOCR | ONOEOT| OFILL | OLCUC | OPOST);
    config.c_oflag = 0;

    //
    // No line processing
    //
    // echo off, echo newline off, canonical mode off,
    // extended input processing off, signal chars off
    //
    config.c_lflag &= ~(unsigned int) (ECHO | ECHONL | ICANON | IEXTEN | ISIG);

    //
    // Turn off character processing
    //
    // clear current char size mask, no parity checking,
    // no output processing, force 8 bit input
    //
    config.c_cflag &= ~(unsigned int) (CSIZE | PARENB);
    config.c_cflag |= CS8;

    //
    // One input byte is enough to return from read()
    // Inter-character timer off
    //
    config.c_cc[VMIN]  = 1;
    config.c_cc[VTIME] = 0;

    //
    // Communication speed (simple version, using the predefined
    // constants)
    //
    if(cfsetispeed(&config, B38400) < 0 || cfsetospeed(&config, B38400) < 0)
      {
        printf ("can't set speed\n");
        close (fd);
        goto die;
      }

     //
     // Finally, apply the configuration
     //
     if(tcsetattr(fd, TCSAFLUSH, &config) < 0)
       {
         printf ("can't set %d\n", errno);
         close (fd);
         goto die;
       }

    while (1)
      {
        if (scraperCancel)
          goto die;
        time_handler (fd);
        update_port (fd);
        //sim_sleep (1);
        //sim_usleep (42000);  // 114 bytes, 11 bit serial data 38400 baud
        sim_usleep (80000);
      }

die:;
    if (fd != -1)
      close (fd);
    fd = -1;
    scraperRunning = false;
    return NULL;
  }

void panelScraperInit (void)
  {
    scraperRunning = false;
    scraperCancel = false;
  }

int panelScraperStart (void)
  {
    if (scraperRunning)
      {
        sim_printf ("scraper already running\n");
        return SCPE_ARG;
      }

    scraperCancel = false;

    // Assume cpu 0 for now
    scraperThreadArg = 0;

    int rc = pthread_create (& scraperThread, NULL, scraperThreadMain,
                             & scraperThreadArg);
    if (rc == 0)
      scraperRunning = true;
    else
      {
        sim_printf ("pthread_create returned %d\n", rc);
        return SCPE_ARG;
      }
    return SCPE_OK;
  }

int panelScraperStop (void)
  {
    if (! scraperRunning)
      {
        sim_printf ("scraper not running\n");
        return SCPE_ARG;
      }

    scraperCancel = true;

    sim_printf ("scraper signaled\n");
    return SCPE_OK;
  }

int panelScraperMsg (const char * msg)
  {
    c_offset = 0;  // Ignore race condition; at worst it might briefly display
                   // wrong characters
    if (msg)
      {
        strncpy (message, msg, sizeof (message));
        message [msgLen - 1] = 0;
      }
    else
      message [0] = 0;
    return SCPE_OK;
  }

