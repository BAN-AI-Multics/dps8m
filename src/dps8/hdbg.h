/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 5e1e2bcf-f62f-11ec-8cca-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2016 Charles Anthony
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#if defined(TESTING)

# if !defined(Hdbg_def)
#  define Hdbg_def

t_stat hdbg_size (int32 arg, UNUSED const char * buf);
t_stat hdbg_print (int32 arg, UNUSED const char * buf);
t_stat hdbg_cpu_mask (UNUSED int32 arg, const char * buf);
t_stat hdbgSegmentNumber (UNUSED int32 arg, const char * buf);
t_stat hdbgBlacklist (UNUSED int32 arg, const char * buf);

void hdbgTrace (const char * ctx);
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
void hdbgIEFP (enum hdbgIEFP_e type, word15 segno, word18 offset, const char * ctx);
void hdbgMRead (word24 addr, word36 data, const char * ctx);
void hdbgMWrite (word24 addr, word36 data, const char * ctx);
void hdbgFault (_fault faultNumber, _fault_subtype subFault,
                const char * faultMsg, const char * ctx);
void hdbgIntrSet (uint inum, uint cpuUnitIdx, uint scuUnitIdx, const char * ctx);
void hdbgIntr (uint intr_pair_addr, const char * ctx);
void hdbgNote (const char * ctx, const char * fmt, ...)
#  if defined(__GNUC__)
  __attribute__ ((format (printf, 2, 3)))
#  endif /* if defined(__GNUC__) */
;

// Keep sync'd with regNames
enum hregs_t
  {
    hreg_A,
    hreg_Q,
    hreg_X0, hreg_X1, hreg_X2, hreg_X3, hreg_X4, hreg_X5, hreg_X6, hreg_X7,
    hreg_AR0, hreg_AR1, hreg_AR2, hreg_AR3, hreg_AR4, hreg_AR5, hreg_AR6, hreg_AR7,
    hreg_PR0, hreg_PR1, hreg_PR2, hreg_PR3, hreg_PR4, hreg_PR5, hreg_PR6, hreg_PR7,
    hreg_Y, hreg_Z,
    hreg_IR,
    hreg_DSBR,
  };
void hdbgRegR (enum hregs_t type, word36 data, const char * ctx);
void hdbgRegW (enum hregs_t type, word36 data, const char * ctx);
struct par_s;
void hdbgPARegR (enum hregs_t type, struct par_s * data, const char * ctx);
void hdbgPARegW (enum hregs_t type, struct par_s * data, const char * ctx);
struct dsbr_s;
void hdbgAPURead (word15 segno, word18 offset, word24 final, word36 data, const char * ctx);
void hdbgAPUWrite (word15 segno, word18 offset, word24 final, word36 data, const char * ctx);

#  define HDBGMRead(a, d, c) hdbgMRead (a, d, c)
#  define HDBGMWrite(a, d, c) hdbgMWrite (a, d, c)
#  define HDBGIEFP(t, s, o, c) hdbgIEFP (t, s, o, c);
#  define HDBGAPURead(s, o, f, d, c) hdbgAPURead (s, o, f, d, c)
#  define HDBGAPUWrite(s, o, f, d, c) hdbgAPUWrite (s, o, f, d, c)
#  define HDBGRegAR(c) hdbgRegR (hreg_A, cpu.rA, c)
#  define HDBGRegAW(c) hdbgRegW (hreg_A, cpu.rA, c)
#  define HDBGRegQR(c) hdbgRegR (hreg_Q, cpu.rQ, c)
#  define HDBGRegQW(c) hdbgRegW (hreg_Q, cpu.rQ, c)
#  define HDBGRegXR(i, c) hdbgRegR (hreg_X0+(i), (word36) cpu.rX[i], c)
#  define HDBGRegXW(i, c) hdbgRegW (hreg_X0+(i), (word36) cpu.rX[i], c)
#  define HDBGRegYR(c) hdbgRegR (hreg_Y, (word36) cpu.rY, c)
#  define HDBGRegYW(c) hdbgRegW (hreg_Y, (word36) cpu.rY, c)
#  define HDBGRegZR(r, c) hdbgRegR (hreg_Z, (word36) r, c)
#  define HDBGRegZW(r, c) hdbgRegW (hreg_Z, (word36) r, c)
#  define HDBGRegPRR(i, c) hdbgPARegR (hreg_PR0+(i), & cpu.PAR[i], c)
#  define HDBGRegPRW(i, c) hdbgPARegW (hreg_PR0+(i), & cpu.PAR[i], c)
#  define HDBGRegARR(i, c) hdbgPARegR (hreg_AR0+(i), & cpu.PAR[i], c)
#  define HDBGRegARW(i, c) hdbgPARegW (hreg_AR0+(i), & cpu.PAR[i], c)
#  define HDBGRegIR(c) hdbgRegW (hreg_IR, (word36) cpu.cu.IR, c)
#  define HDBGTrace(c) hdbgTrace(c)
#  define HDBGIntr(i, c) hdbgIntr(i, c)
#  define HDBGIntrSet(i, c, s, ctx) hdbgIntrSet(i, c, s, ctx)
#  define HDBGFault(n, s, m, c) hdbgFault(n, s, m, c)
#  define HDBGPrint() hdbgPrint()
# endif /* if !defined(Hdbg_def) */
#endif /* if defined(TESTING) */
