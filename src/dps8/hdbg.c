/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 56a3950b-f62f-11ec-9ec8-80ee73e9b8e7
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

// History Debugging

#if defined(TESTING)

# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>

# include "dps8.h"
# include "dps8_sys.h"
# include "dps8_cpu.h"
# include "dps8_utils.h"
# include "dps8_memalign.h"
# include "hdbg.h"

# include "dps8_faults.h"

# if defined(FREE)
#  undef FREE
# endif /* if defined(FREE) */
# define FREE(p) do  \
  {                  \
    free((p));       \
    (p) = NULL;      \
  } while(0)

enum hevtType {
  hevtEmpty = 0,
  hevtTrace,
  hevtM,
  hevtAPU,
  hevtFault,
  hevtIntrSet,
  hevtIntr,
  hevtReg,
  hevtPAReg,
  hevtDSBRReg,
  hevtIEFP,
  hevtNote,
};

struct hevt {
  enum hevtType type;
  uint64 time;
  uint cpu_idx;
  char ctx[16];
  bool rw; // F: read  T: write
  union {
    struct {
      addr_modes_e addrMode;
      word15 segno;
      word18 ic;
      word3 ring;
      word36 inst;
    } trace;

    struct {
      word24 addr;
      word36 data;
    } memref;

    struct {
      _fault faultNumber;
      _fault_subtype subFault;
      char faultMsg [64];
    } fault;

    struct {
      uint inum;
      uint cpuUnitIdx;
      uint scuUnitIdx;
    } intrSet;

    struct {
      uint intr_pair_addr;
    } intr;

    struct {
      enum hregs_t type;
      word36 data;
    } reg;

    struct {
      enum hregs_t type;
      struct par_s data;
    } par;

    struct {
      enum hdbgIEFP_e type;
      word15 segno;
      word18 offset;
    } iefp;

    struct {
      enum hregs_t type;
      struct dsbr_s data;
    } dsbr;

    struct {
      enum hregs_t type;
      word15 segno;
      word18 offset;
      word24 final;
      word36 data;
    } apu;
    struct {
# define NOTE_SZ 64
      char noteBody [NOTE_SZ];
    } note;
  };
};

static struct hevt * hevents = NULL;
static long hdbgSize = 0;
static long hevtPtr = 0;
static long hevtMark = 0;
static long hdbgSegNum = -1;
static bool blacklist[MAX18];
static long hdbgCPUMask = 0;

static void createBuffer (void) {
  if (hevents) {
    FREE (hevents);
    hevents = NULL;
  }
  if (hdbgSize <= 0)
    return;
# if !defined(_AIX)
  hevents = aligned_malloc (sizeof (struct hevt) * hdbgSize);
# else
  hevents = malloc (sizeof (struct hevt) * hdbgSize);
# endif
  if (! hevents) {
    sim_printf ("hdbg createBuffer failed\n");
    return;
  }
  (void)memset (hevents, 0, sizeof (struct hevt) * hdbgSize);

  hevtPtr = 0;
}

static long hdbg_inc (void) {
  //hevtPtr = (hevtPtr + 1) % hdbgSize;
  long ret = __sync_fetch_and_add (& hevtPtr, 1l) % hdbgSize;

  if (hevtMark > 0) {
    long ret = __sync_fetch_and_sub (& hevtMark, 1l);
    if (ret <= 0)
      hdbgPrint ();
  }
  return ret;
}

# define hev(t, tf, filter) \
  if (! hevents) \
    goto done; \
  if (filter && hdbgSegNum >= 0 && hdbgSegNum != cpu.PPR.PSR) \
    goto done; \
  if (filter && hdbgSegNum > 0 && blacklist[cpu.PPR.IC]) \
    goto done; \
  if (hdbgCPUMask && (hdbgCPUMask & (1 << current_running_cpu_idx))) \
    goto done; \
  unsigned long p = hdbg_inc (); \
  hevents[p].type = t; \
  hevents[p].cpu_idx = current_running_cpu_idx; \
  hevents[p].time = cpu.cycleCnt; \
  strncpy (hevents[p].ctx, ctx, 15); \
  hevents[p].ctx[15] = 0; \
  hevents[p].rw = tf;

# define FILTER true
# define NO_FILTER false

# define WR true
# define RD false

void hdbgTrace (const char * ctx) {
  cpu_state_t * cpup = _cpup;
  hev (hevtTrace, RD, FILTER);
  hevents[p].trace.addrMode = get_addr_mode (cpup);
  hevents[p].trace.segno    = cpu.PPR.PSR;
  hevents[p].trace.ic       = cpu.PPR.IC;
  hevents[p].trace.ring     = cpu.PPR.PRR;
  hevents[p].trace.inst     = IWB_IRODD;
done: ;
}

void hdbgMRead (word24 addr, word36 data, const char * ctx) {
  cpu_state_t * cpup = _cpup;
  hev (hevtM, RD, FILTER);
  hevents[p].memref.addr = addr;
  hevents[p].memref.data = data;
done: ;
}

void hdbgMWrite (word24 addr, word36 data, const char * ctx) {
  cpu_state_t * cpup = _cpup;
  hev (hevtM, WR, FILTER);
  hevents[p].memref.addr = addr;
  hevents[p].memref.data = data;
done: ;
}

void hdbgAPURead (word15 segno, word18 offset, word24 final, word36 data, const char * ctx) {
  cpu_state_t * cpup = _cpup;
  hev (hevtAPU, RD, FILTER);
  hevents[p].apu.segno  = segno;
  hevents[p].apu.offset = offset;
  hevents[p].apu.final  = final;
  hevents[p].apu.data   = data;
done: ;
}

void hdbgAPUWrite (word15 segno, word18 offset, word24 final, word36 data, const char * ctx) {
  cpu_state_t * cpup = _cpup;
  hev (hevtAPU, WR, FILTER);
  hevents[p].apu.segno  = segno;
  hevents[p].apu.offset = offset;
  hevents[p].apu.final  = final;
  hevents[p].apu.data   = data;
done: ;
}

void hdbgFault (_fault faultNumber, _fault_subtype subFault, const char * faultMsg, const char * ctx) {
  cpu_state_t * cpup = _cpup;
  hev (hevtFault, RD, FILTER);
  hevents[p].fault.faultNumber  = faultNumber;
  hevents[p].fault.subFault     = subFault;
  strncpy (hevents[p].fault.faultMsg, faultMsg, 63);
  hevents[p].fault.faultMsg[63] = 0;
done: ;
}

void hdbgIntrSet (uint inum, uint cpuUnitIdx, uint scuUnitIdx, const char * ctx) {
  cpu_state_t * cpup = _cpup;
  hev (hevtIntrSet, RD, FILTER);
  hevents[p].intrSet.inum       = inum;
  hevents[p].intrSet.cpuUnitIdx = cpuUnitIdx;
  hevents[p].intrSet.scuUnitIdx = scuUnitIdx;
done: ;
}

void hdbgIntr (uint intr_pair_addr, const char * ctx) {
  cpu_state_t * cpup = _cpup;
  hev (hevtIntr, RD, FILTER);
  hevents[p].cpu_idx             = current_running_cpu_idx;
  hevents[p].time                = cpu.cycleCnt;
  strncpy (hevents[p].ctx, ctx, 15);
  hevents[p].ctx[15]             = 0;
  hevents[p].intr.intr_pair_addr = intr_pair_addr;
done: ;
}

void hdbgRegR (enum hregs_t type, word36 data, const char * ctx) {
  cpu_state_t * cpup = _cpup;
  hev (hevtReg, RD, FILTER);
  hevents[p].reg.type = type;
  hevents[p].reg.data = data;
done: ;
}

void hdbgRegW (enum hregs_t type, word36 data, const char * ctx) {
  cpu_state_t * cpup = _cpup;
  hev (hevtReg, WR, FILTER);
  hevents[p].reg.type = type;
  hevents[p].reg.data = data;
done: ;
}

void hdbgPARegR (enum hregs_t type, struct par_s * data, const char * ctx) {
  cpu_state_t * cpup = _cpup;
  hev (hevtPAReg, RD, FILTER);
  hevents[p].par.type = type;
  hevents[p].par.data = * data;
done: ;
}

void hdbgPARegW (enum hregs_t type, struct par_s * data, const char * ctx) {
  cpu_state_t * cpup = _cpup;
  hev (hevtPAReg, WR, FILTER);
    hevents[p].par.type = type;
    hevents[p].par.data = * data;
done: ;
}

# if 0
void hdbgDSBRRegR (enum hregs_t type, struct dsbr_s * data, const char * ctx) {
  hev (hevtDSBRReg, RD, FILTER);
    hevents[p].dsbr.type = type;
    hevents[p].dsbr.data = * data;
done: ;
}

void hdbgDSBRRegW (enum hregs_t type, struct dsbr_s * data, const char * ctx) {
  hev (hevtDSBRReg, WR, FILTER);
    hevents[p].dsbr.type = type;
    hevents[p].dsbr.data = * data;
done: ;
}
# endif

void hdbgIEFP (enum hdbgIEFP_e type, word15 segno, word18 offset, const char * ctx) {
  cpu_state_t * cpup = _cpup;
  hev (hevtIEFP, RD, FILTER);
  hevents [p].iefp.type   = type;
  hevents [p].iefp.segno  = segno;
  hevents [p].iefp.offset = offset;
done: ;
}

void hdbgNote (const char * ctx, const char * fmt, ...) {
  cpu_state_t * cpup = _cpup;
  hev (hevtNote, RD, NO_FILTER);
  va_list arglist;
  va_start (arglist, fmt);
  (void)vsnprintf (hevents [p].note.noteBody, NOTE_SZ - 1, fmt, arglist);
  va_end (arglist);
done: ;
}

static FILE * hdbgOut = NULL;

static void printM (struct hevt * p) {
  (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d FINAL: %s %s %08o %012llo\n",
                 (unsigned long long int)p->time,
                 p->cpu_idx,
                 p->ctx,
                 p->rw ? "write" : "read ",
                 p->memref.addr,
                 (unsigned long long int)p->memref.data);
}

static void printAPU (struct hevt * p) {
  (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d APU: %s %s %05o:%06o %08o %012llo\n",
                 (unsigned long long int)p->time,
                 p->cpu_idx,
                 p->ctx,
                 p->rw ? "write" : "read ",
                 p->apu.segno,
                 p->apu.offset,
                 p->apu.final,
                 (unsigned long long int)p->apu.data);
}

static void printTrace (struct hevt * p) {
  char buf[256];
  if (p -> trace.addrMode == ABSOLUTE_mode) {
    (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d TRACE: %s %06o %o %012llo (%s)\n",
                   (unsigned long long int)p->time,
                   p->cpu_idx,
                   p->ctx,
                   p->trace.ic,
                   p->trace.ring,
                   (unsigned long long int)p->trace.inst,
                   disassemble (buf, p->trace.inst));
  } else {
    (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d TRACE: %s %05o:%06o %o %012llo (%s)\n",
                   (unsigned long long int)p->time,
                   p->cpu_idx,
                   p->ctx,
                   p->trace.segno,
                   p->trace.ic,
                   p->trace.ring,
                   (unsigned long long int)p->trace.inst,
                   disassemble (buf, p->trace.inst));
  }
}

static void printFault (struct hevt * p) {
  (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d FAULT: %s Fault %d(0%o), sub %llu(0%llo), '%s'\n",
                 (unsigned long long int)p->time,
                 p->cpu_idx,
                 p->ctx,
                 p->fault.faultNumber,
                 p->fault.faultNumber,
                 (unsigned long long int)p->fault.subFault.bits,
                 (unsigned long long int)p->fault.subFault.bits,
                 p->fault.faultMsg);
}

static void printIntrSet (struct hevt * p) {
  (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d INTR_SET: %s number %d(0%o), CPU %u SCU %u\n",
                 (unsigned long long int)p->time,
                 p->cpu_idx,
                 p->ctx,
                 p->intrSet.inum,
                 p->intrSet.inum,
                 p->intrSet.cpuUnitIdx,
                 p->intrSet.scuUnitIdx);
}

static void printIntr (struct hevt * p) {
  (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d INTR: %s Interrupt pair address %o\n",
                 (unsigned long long int)p->time,
                 p->cpu_idx,
                 p->ctx,
                 p->intr.intr_pair_addr);
}

// Keep sync'd with hregs_t
static char * regNames[] = {
   "A  ",
   "Q  ",
    "X0",  "X1",  "X2",  "X3",  "X4",  "X5",  "X6",  "X7",
   "AR0", "AR1", "AR2", "AR3", "AR4", "AR5", "AR6", "AR7",
   "PR0", "PR1", "PR2", "PR3", "PR4", "PR5", "PR6", "PR7",
   "Y  ", "Z  ",
   "IR ",
  "DSBR",
};

static void printReg (struct hevt * p) {
  if (p->reg.type == hreg_IR)
    (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d REG: %s %s %s %012llo Z%o N%o C %o O%o T%o \n",
                   (unsigned long long int)p->time,
                   p->cpu_idx,
                   p->ctx,
                   p->rw ? "write" : "read ",
                   regNames[p->reg.type],
                   (unsigned long long int)p->reg.data,
                   TSTF (p->reg.data, I_ZERO),
                   TSTF (p->reg.data, I_NEG),
                   TSTF (p->reg.data, I_CARRY),
                   TSTF (p->reg.data, I_OFLOW),
                   TSTF (p->reg.data, I_TALLY));
  else if (p->reg.type >= hreg_X0 && p->reg.type <= hreg_X7)
    (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d REG: %s %s %s %06llo\n",
                   (unsigned long long int)p->time,
                   p->cpu_idx,
                   p->ctx,
                   p->rw ? "write" : "read ",
                   regNames[p->reg.type],
                   (unsigned long long int)p->reg.data);
  else
    (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d REG: %s %s  %s %012llo\n",
                   (unsigned long long int)p->time,
                   p->cpu_idx,
                   p->ctx,
                   p->rw ? "write" : "read ",
                   regNames[p->reg.type],
                   (unsigned long long int)p->reg.data);
}

static void printPAReg (struct hevt * p)
{
  if (p->reg.type >= hreg_PR0 && p->reg.type <= hreg_PR7)
    (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d REG: %s %s %s %05o:%06o BIT %2o RNR %o\n",
                   (unsigned long long int)p->time,
                    p->cpu_idx,
                    p->ctx,
                    p->rw ? "write" : "read ",
                    regNames[p->reg.type],
                    p->par.data.SNR,
                    p->par.data.WORDNO,
                    p->par.data.PR_BITNO,
                    p->par.data.RNR);
  else
    (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d REG: %s write %s %05o:%06o CHAR %o BIT %2o RNR %o\n",
                   (unsigned long long int)p->time,
                   p->cpu_idx,
                   p->ctx,
                   regNames[p->reg.type],
                   p->par.data.SNR,
                   p->par.data.WORDNO,
                   p->par.data.AR_CHAR,
                   p->par.data.AR_BITNO,
                   p->par.data.RNR);
}

static void printDSBRReg (struct hevt * p) {
  (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d REG: %s %s %s %05o:%06o BIT %2o RNR %o\n",
                 (unsigned long long int)p->time,
                 p->cpu_idx,
                 p->ctx,
                 p->rw ? "write" : "read ",
                 regNames[p->reg.type],
                 p->par.data.SNR,
                 p->par.data.WORDNO,
                 p->par.data.PR_BITNO,
                 p->par.data.RNR);
}

static void printIEFP (struct hevt * p) {
  switch (p->iefp.type) {
    case hdbgIEFP_abs_bar_read:
      (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d IEFP ABS BAR READ:  |%06o\n",
                     (unsigned long long int)p->time,
                     p->cpu_idx,
                     p->iefp.offset);
      break;

    case hdbgIEFP_abs_read:
      (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d IEFP ABS     READ:  :%06o\n",
                     (unsigned long long int)p->time,
                     p->cpu_idx,
                     p->iefp.offset);
      break;

    case hdbgIEFP_bar_read:
      (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d IEFP APP BAR READ:  %05o|%06o\n",
                     (unsigned long long int)p->time,
                     p->cpu_idx,
                     p->iefp.segno,
                     p->iefp.offset);
      break;

    case hdbgIEFP_read:
      (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d IEFP APP     READ:  %05o:%06o\n",
                     (unsigned long long int)p->time,
                     p->cpu_idx,
                     p->iefp.segno,
                     p->iefp.offset);
      break;

    case hdbgIEFP_abs_bar_write:
      (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d IEFP ABS BAR WRITE: |%06o\n",
                     (long long unsigned int)p->time,
                     p->cpu_idx,
                     p->iefp.offset);
      break;

    case hdbgIEFP_abs_write:
      (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d IEFP ABS     WRITE: :%06o\n",
                     (long long unsigned int)p->time,
                     p->cpu_idx,
                     p->iefp.offset);
      break;

    case hdbgIEFP_bar_write:
      (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d IEFP APP BAR WRITE: %05o|%06o\n",
                     (unsigned long long int)p->time,
                     p->cpu_idx,
                     p->iefp.segno,
                     p->iefp.offset);
      break;

    case hdbgIEFP_write:
      (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d IEFP APP     WRITE: %05o:%06o\n",
                     (unsigned long long int)p->time,
                     p->cpu_idx,
                     p->iefp.segno,
                     p->iefp.offset);
      break;

    default:
      (void)fprintf (hdbgOut, "DBG(%llu)> CPU %d IEFP ??? ??? WRITE: %05o?%06o\n",
                     (long long unsigned int)p->time,
                     p->cpu_idx,
                     p->iefp.segno,
                     p->iefp.offset);
      break;
  }
}

static void printNote (struct hevt * p) {
  (void)fprintf (hdbgOut, "DBG(%llu)> Note: %s\n",
                 (long long unsigned int)p->time,
                 p->note.noteBody);
}

void hdbgPrint (void) {
  sim_printf ("hdbg print\n");
  if (! hevents)
    goto done;
  struct hevt * t = hevents;
  hevents = NULL;
  hdbgOut = fopen ("hdbg.list", "w");
  if (! hdbgOut) {
    sim_printf ("can't open hdbg.list\n");
    goto done;
  }
  time_t curtime;
  time (& curtime);
  (void)fprintf (hdbgOut, "%s\n", ctime (& curtime));

  for (unsigned long p = 0; p < hdbgSize; p ++) {
    unsigned long q = (hevtPtr + p) % hdbgSize;
    struct hevt * evtp = t + q;
    switch (evtp -> type) {
      case hevtEmpty:
        break;

      case hevtTrace:
        printTrace (evtp);
        break;

      case hevtM:
        printM (evtp);
        break;

      case hevtAPU:
        printAPU (evtp);
        break;

# if 0
      case hevtIWBUpdate:
        printIWBUpdate (evtp);
        break;
# endif

# if 0
      case hevtRegs:
        printRegs (evtp);
        break;
# endif

      case hevtFault:
        printFault (evtp);
        break;

      case hevtIntrSet:
        printIntrSet (evtp);
        break;

      case hevtIntr:
        printIntr (evtp);
        break;

      case hevtReg:
        printReg (evtp);
        break;

      case hevtPAReg:
        printPAReg (evtp);
        break;

      case hevtDSBRReg:
        printDSBRReg (evtp);
        break;

      case hevtIEFP:
        printIEFP (evtp);
        break;

      case hevtNote:
        printNote (evtp);
        break;

      default:
        (void)fprintf (hdbgOut, "hdbgPrint ? %d\n", evtp -> type);
        break;
    }
  }
  fclose (hdbgOut);

  int fd = open ("M.dump", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    sim_printf ("can't open M.dump\n");
    goto done;
  }
  // cast discards volatile
  /* ssize_t n = */ write (fd, (const void *) M, MEMSIZE * sizeof (word36));
  close (fd);
done: ;
}

# if 0
void hdbg_mark (void) {
  hevtMark = hdbgSize;
  sim_printf ("hdbg mark set to %ld\n", (long) hevtMark);
}
# endif

t_stat hdbg_cpu_mask (UNUSED int32 arg, const char * buf)
  {
    hdbgCPUMask = strtoul (buf, NULL, 0);
    sim_printf ("hdbg CPU mask set to %ld\n", (long) hdbgCPUMask);
    return SCPE_OK;
  }

// set buffer size
t_stat hdbg_size (UNUSED int32 arg, const char * buf) {
  hdbgSize = strtoul (buf, NULL, 0);
  sim_printf ("hdbg size set to %ld\n", (long) hdbgSize);
  createBuffer ();
  return SCPE_OK;
}

// set target segment number
t_stat hdbgSegmentNumber (UNUSED int32 arg, const char * buf) {
  hdbgSegNum = strtoul (buf, NULL, 8);
  sim_printf ("hdbg target segment number set to %lu\n", hdbgSize);
  createBuffer ();
  return SCPE_OK;
}

t_stat hdbgBlacklist (UNUSED int32 arg, const char * buf) {
  char work[strlen (buf) + 1];
  if (sscanf (buf, "%s", work) != 1)
    return SCPE_ARG;
  if (strcasecmp (work, "init") == 0) {
    (void)memset (blacklist, 0, sizeof (blacklist));
    return SCPE_OK;
  }
  uint low, high;
  if (sscanf (work, "%o-%o", & low, & high) != 2)
    return SCPE_ARG;
  if (low > MAX18 || high > MAX18)
    return SCPE_ARG;
  for (uint addr = low; addr <= high; addr ++)
    blacklist[addr] = true;
  return SCPE_OK;
}

t_stat hdbg_print (UNUSED int32 arg, const char * buf) {
  hdbgPrint ();
  return SCPE_OK;
}

#endif // TESTING
