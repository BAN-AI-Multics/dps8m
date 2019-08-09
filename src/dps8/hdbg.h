/*
 Copyright 2016 by Charles Anthony

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */
#ifndef HDBG_H
#define HDBG_H

void hdbg_mark (void);
t_stat hdbg_size (int32 arg, UNUSED const char * buf);
t_stat hdbg_print (int32 arg, UNUSED const char * buf);
#ifdef HDBG
void hdbgTrace (const char * ctx);
void hdbgPrint (void);
void hdbgMRead (word24 addr, word36 data, const char * ctx);
void hdbgMWrite (word24 addr, word36 data, const char * ctx);
void hdbgFault (_fault faultNumber, _fault_subtype subFault,
                const char * faultMsg, const char * ctx);
void hdbgIntrSet (uint inum, uint cpuUnitIdx, uint scuUnitIdx, const char * ctx);
void hdbgIntr (uint intr_pair_addr, const char * ctx);
// Keep sync'd with regNames
enum hregs_t
  {
    hreg_A,
    hreg_Q,
    hreg_X0, hreg_X1, hreg_X2, hreg_X3, hreg_X4, hreg_X5, hreg_X6, hreg_X7,
    hreg_AR0, hreg_AR1, hreg_AR2, hreg_AR3, hreg_AR4, hreg_AR5, hreg_AR6, hreg_AR7,
    hreg_PR0, hreg_PR1, hreg_PR2, hreg_PR3, hreg_PR4, hreg_PR5, hreg_PR6, hreg_PR7,
    hreg_DSBR
  };
void hdbgRegR (enum hregs_t type, word36 data, const char * ctx);
void hdbgRegW (enum hregs_t type, word36 data, const char * ctx);
struct par_s;
void hdbgPARegR (enum hregs_t type, struct par_s * data, const char * ctx);
void hdbgPARegW (enum hregs_t type, struct par_s * data, const char * ctx);
struct dsbr_s;
void hdbgDSBRRegR (enum hregs_t type, struct dsbr_s * data, const char * ctx);
void hdbgDSBRRegW (enum hregs_t type, struct dsbr_s * data, const char * ctx);
void hdbgAPURead (word15 segno, word18 offset, word36 data, const char * ctx);
void hdbgAPUWrite (word15 segno, word18 offset, word36 data, const char * ctx);
#endif

#ifdef HDBG
#define HDBGMRead(a, d, c) hdbgMRead (a, d, c)
#define HDBGMWrite(a, d, c) hdbgMWrite (a, d, c)
#define HDBGAPURead(s, o, d, c) hdbgAPURead (s, o, d, c)
#define HDBGAPUWrite(s, o, d, c) hdbgAPUWrite (s, o, d, c)
#define HDBGRegAR(c) hdbgRegR (hreg_A, cpu.rA, c)
#define HDBGRegAW(c) hdbgRegW (hreg_A, cpu.rA, c)
#define HDBGRegQR(c) hdbgRegR (hreg_Q, cpu.rQ, c)
#define HDBGRegQW(c) hdbgRegW (hreg_Q, cpu.rQ, c)
#define HDBGRegXR(i, c) hdbgRegR (hreg_X0+(i), (word36) cpu.rX[i], c)
#define HDBGRegXW(i, c) hdbgRegW (hreg_X0+(i), (word36) cpu.rX[i], c)
#define HDBGRegPRR(i, c) hdbgPARegR (hreg_PR0+(i), & cpu.PAR[i], c)
#define HDBGRegPRW(i, c) hdbgPARegW (hreg_PR0+(i), & cpu.PAR[i], c)
#define HDBGRegARR(i, c) hdbgPARegR (hreg_AR0+(i), & cpu.PAR[i], c)
#define HDBGRegARW(i, c) hdbgPARegW (hreg_AR0+(i), & cpu.PAR[i], c)
#define HDBGRegDSBRR(i, c) hdbgDSBRRegR (hreg_AR0+(i), & cpu.PAR[i], c)
#define HDBGRegDSBRW(i, c) hdbgDSBRRegW (hreg_AR0+(i), & cpu.PAR[i], c)
#define HDBGTrace(c) hdbgTrace(c)
#define HDBGIntr(i, c) hdbgIntr(i, c)
#define HDBGIntrSet(i, c, s, ctx) hdbgIntrSet(i, c, s, ctx)
#define HDBGFault(n, s, m, c) hdbgFault(n, s, m, c)
#define HDBGPrint() hdbgPrint()
#else
#define HDBGMRead(a, d, c) 
#define HDBGMWrite(a, d, c)
#define HDBGAPURead(s, o, d, c)
#define HDBGAPUWrite(s, o, d, c)
#define HDBGRegAR(c)
#define HDBGRegAW(c)
#define HDBGRegQR(c)
#define HDBGRegQW(c)
#define HDBGRegXR(i, c)
#define HDBGRegXW(i, c)
#define HDBGRegPRR(i, c)
#define HDBGRegPRW(i, c)
#define HDBGRegARR(i, c)
#define HDBGRegARW(i, c)
#define HDBGRegDSBRR(i, c)
#define HDBGRegDSBRW(i, c)
#define HDBGTrace(c)
#define HDBGIntr(i, c)
#define HDBGIntrSet(i, c, s)
#define HDBGFault(n, s, m, c)
#define HDBGPrint()
#endif
#endif
