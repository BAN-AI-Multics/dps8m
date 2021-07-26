/*
 * Copyright (c) 2016 Charles Anthony
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 */

#ifndef HDBG_H
#define HDBG_H

void hdbg_mark (void);
t_stat hdbg_size (int32 arg, UNUSED const char * buf);
t_stat hdbg_print (int32 arg, UNUSED const char * buf);
#ifdef HDBG
void hdbgTrace (void);
void hdbgPrint (void);
enum hdbgIEFP_e
  {
    hdbgIEFP_abs_bar_read,
    hdbgIEFP_abs_read,
    hdbgIEFP_bar_read,
    hdbgIEFP_read,
    hdbgIEFP_abs_bar_write,
    hdbgIEFP_abs_write,
    hdbgIEFP_bar_write,
    hdbgIEFP_write
  };
void hdbgIEFP (enum hdbgIEFP_e type, word15 segno, word18 offset);
void hdbgMRead (word24 addr, word36 data);
void hdbgMWrite (word24 addr, word36 data);
void hdbgFault (_fault faultNumber, _fault_subtype subFault,
                const char * faultMsg);
void hdbgIntrSet (uint inum, uint cpuUnitIdx, uint scuUnitIdx);
void hdbgIntr (uint intr_pair_addr);
enum hregs_t
  {
    hreg_A,
    hreg_Q,
    hreg_X0, hreg_X1, hreg_X2, hreg_X3, hreg_X4, hreg_X5, hreg_X6, hreg_X7,
    hreg_AR0, hreg_AR1, hreg_AR2, hreg_AR3, hreg_AR4, hreg_AR5, hreg_AR6, hreg_AR7,
    hreg_PR0, hreg_PR1, hreg_PR2, hreg_PR3, hreg_PR4, hreg_PR5, hreg_PR6, hreg_PR7,
    hreg_Y, hreg_Z,
    hreg_IR
  };
void hdbgReg (enum hregs_t type, word36 data);
struct par_s;
void hdbgPAReg (enum hregs_t type, struct par_s * data);
#endif

#ifdef HDBG
#define HDBGMRead(a, d) hdbgMRead (a, d)
#define HDBGMWrite(a, d) hdbgMWrite (a, d)
#define HDBGRegA() hdbgReg (hreg_A, cpu.rA)
#define HDBGRegQ() hdbgReg (hreg_Q, cpu.rQ)
#define HDBGRegY() hdbgReg (hreg_Y, cpu.rY)
#define HDBGRegZ(z) hdbgReg (hreg_Z, z)
#define HDBGRegIR() hdbgReg (hreg_IR, cpu.cu.IR)
#define HDBGRegX(i) hdbgReg (hreg_X0+(i), (word36) cpu.rX[i])
#define HDBGRegPR(i) hdbgPAReg (hreg_PR0+(i), & cpu.PAR[i]);
#define HDBGRegAR(i) hdbgPAReg (hreg_AR0+(i), & cpu.PAR[i]);
#define HDBGIEFP(t,s,o) hdbgIEFP (t, s, o);
#else
#define HDBGMRead(a, d)
#define HDBGMWrite(a, d)
#define HDBGRegA()
#define HDBGRegQ()
#define HDBGRegY()
#define HDBGRegZ(z)
#define HDBGRegIR()
#define HDBGRegX(i)
#define HDBGRegPR(i)
#define HDBGRegAR(i)
#define HDBGIEFP(t,s,o)
#endif
#endif
