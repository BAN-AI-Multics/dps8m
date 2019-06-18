/*
 Copyright 2016 by Charles Anthony

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

// history debugging
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dps8.h"
#include "dps8_sys.h"
#include "dps8_cpu.h"
#include "dps8_utils.h"
#include "hdbg.h"


#ifdef HDBG
#include "dps8_faults.h"

enum hevtType
  {
    hevtEmpty = 0,
    hevtTrace,
    hevtM,
    hevtAPU,
    // hevtIWBUpdate,
    // hevtRegs,
    hevtFault,
    hevtIntrSet,
    hevtIntr,
    hevtReg,
    hevtPAReg,
    hevtDSBRReg,
  };

struct hevt
  {
    enum hevtType type;
    uint64 time;
    uint cpu_idx;
    char ctx[16];
    bool rw; // F: read  T: write
    union
      {
        struct
          {
            addr_modes_e addrMode;
            word15 segno;
            word18 ic;
            word3 ring;
            word36 inst;
          } trace;

        struct
          {
            word24 addr;
            word36 data;
          } memref;

        struct
          {
            _fault faultNumber;
            _fault_subtype subFault;
            char faultMsg[64];
          } fault;

        struct
          {
            uint inum;
            uint cpuUnitIdx;
            uint scuUnitIdx;
          } intrSet;

        struct
          {
            uint intr_pair_addr;
          } intr;

        struct
          {
            enum hregs_t type;
            word36 data;
          } reg;

        struct
          {
            enum hregs_t type;
            struct par_s data;
          } par;

        struct
          {
            enum hregs_t type;
            struct dsbr_s data;
          } dsbr;

        struct
          {
            enum hregs_t type;
            word15 segno;
            word18 offset;
            word36 data;
          } apu;
      };
  };

static struct hevt * hevents = NULL;
static unsigned long hdbgSize = 0;
static unsigned long hevtPtr = 0;
static unsigned long hevtMark = 0;

#if 0
//#define CPUN if (current_running_cpu_idx < 1 || current_running_cpu_idx > 3) return
#define CPUN if (current_running_cpu_idx != 3) return
#else
#define CPUN
#endif

static void createBuffer (void)
  {
    if (hevents)
      {
        free (hevents);
        hevents = NULL;
      }
    if (! hdbgSize)
      return;
    hevents = malloc (sizeof (struct hevt) * hdbgSize);
    if (! hevents)
      {
        sim_printf ("hdbg createBuffer failed\n");
        return;
      }
    memset (hevents, 0, sizeof (struct hevt) * hdbgSize);

    hevtPtr = 0;
  }

static unsigned long hdbg_inc (void)
  {
    return __sync_fetch_and_add (& hevtPtr, 1l) % hdbgSize;
  }

void hdbgTrace (const char * ctx)
  {
CPUN;
    if (! hevents)
      goto done;
#ifdef ISOLTS
if (current_running_cpu_idx == 0)
  goto done;
#endif
    unsigned long p = hdbg_inc ();
    hevents[p].type = hevtTrace;
    hevents[p].cpu_idx = current_running_cpu_idx;
    hevents[p].time = cpu.cycleCnt;
    strncpy (hevents[p].ctx, ctx, 15);
    hevents[p].ctx[15] = 0;
    hevents[p].trace.addrMode = get_addr_mode ();
    hevents[p].trace.segno = cpu.PPR.PSR;
    hevents[p].trace.ic = cpu.PPR.IC;
    hevents[p].trace.ring = cpu.PPR.PRR;
    hevents[p].trace.inst = cpu.cu.IWB;
done: ;
  }

void hdbgMRead (word24 addr, word36 data, const char * ctx)
  {
CPUN;
    if (! hevents)
      goto done;
#ifdef ISOLTS
if (current_running_cpu_idx == 0)
  goto done;
#endif
    unsigned long p = hdbg_inc ();
    hevents[p].type = hevtM;
    hevents[p].cpu_idx = current_running_cpu_idx;
    hevents[p].time = cpu.cycleCnt;
    strncpy (hevents[p].ctx, ctx, 15);
    hevents[p].ctx[15] = 0;
    hevents[p].rw = false; hevents[p].memref.addr = addr;
    hevents[p].memref.data = data;
done: ;
  }

void hdbgMWrite (word24 addr, word36 data, const char * ctx)
  {
CPUN;
    if (! hevents)
      goto done;
#ifdef ISOLTS
if (current_running_cpu_idx == 0)
  goto done;
#endif
    unsigned long p = hdbg_inc ();
    hevents[p].type = hevtM;
    hevents[p].cpu_idx = current_running_cpu_idx;
    hevents[p].time = cpu.cycleCnt;
    strncpy (hevents[p].ctx, ctx, 15);
    hevents[p].ctx[15] = 0;
    hevents[p].rw = true;
    hevents[p].memref.addr = addr;
    hevents[p].memref.data = data;
done: ;
  }

void hdbgAPURead (word15 segno, word18 offset, word36 data, const char * ctx)
  {
CPUN;
    if (! hevents)
      goto done;
#ifdef ISOLTS
if (current_running_cpu_idx == 0)
  goto done;
#endif
    unsigned long p = hdbg_inc ();
    hevents[p].type = hevtAPU;
    hevents[p].cpu_idx = current_running_cpu_idx;
    hevents[p].time = cpu.cycleCnt;
    strncpy (hevents[p].ctx, ctx, 15);
    hevents[p].ctx[15] = 0;
    hevents[p].rw = false;
    hevents[p].apu.segno = segno;
    hevents[p].apu.offset = offset;
    hevents[p].apu.data = data;
done: ;
  }

void hdbgAPUWrite (word15 segno, word18 offset, word36 data, const char * ctx)
  {
CPUN;
    if (! hevents)
      goto done;
#ifdef ISOLTS
if (current_running_cpu_idx == 0)
  goto done;
#endif
    unsigned long p = hdbg_inc ();
    hevents[p].type = hevtAPU;
    hevents[p].cpu_idx = current_running_cpu_idx;
    hevents[p].time = cpu.cycleCnt;
    strncpy (hevents[p].ctx, ctx, 15);
    hevents[p].ctx[15] = 0;
    hevents[p].rw = true;
    hevents[p].apu.segno = segno;
    hevents[p].apu.offset = offset;
    hevents[p].apu.data = data;
done: ;
  }

void hdbgFault (_fault faultNumber, _fault_subtype subFault,
                const char * faultMsg, const char * ctx)
  {
CPUN;
    if (! hevents)
      goto done;
#ifdef ISOLTS
if (current_running_cpu_idx == 0)
  goto done;
#endif
    unsigned long p = hdbg_inc ();
    hevents[p].type = hevtFault;
    hevents[p].cpu_idx = current_running_cpu_idx;
    hevents[p].time = cpu.cycleCnt;
    strncpy (hevents[p].ctx, ctx, 15);
    hevents[p].ctx[15] = 0;
    hevents[p].fault.faultNumber = faultNumber;
    hevents[p].fault.subFault = subFault;
    strncpy (hevents[p].fault.faultMsg, faultMsg, 63);
    hevents[p].fault.faultMsg[63] = 0;
done: ;
  }

void hdbgIntrSet (uint inum, uint cpuUnitIdx, uint scuUnitIdx, const char * ctx)
  {
CPUN;
    if (! hevents)
      goto done;
#ifdef ISOLTS
if (current_running_cpu_idx == 0)
  goto done;
#endif
    unsigned long p = hdbg_inc ();
    hevents[p].type = hevtIntrSet;
    hevents[p].cpu_idx = current_running_cpu_idx;
    hevents[p].time = cpu.cycleCnt;
    strncpy (hevents[p].ctx, ctx, 15);
    hevents[p].ctx[15] = 0;
    hevents[p].intrSet.inum = inum;
    hevents[p].intrSet.cpuUnitIdx = cpuUnitIdx;
    hevents[p].intrSet.scuUnitIdx = scuUnitIdx;
done: ;
  }

void hdbgIntr (uint intr_pair_addr, const char * ctx)
  {
CPUN;
    if (! hevents)
      goto done;
#ifdef ISOLTS
if (current_running_cpu_idx == 0)
  goto done;
#endif
    unsigned long p = hdbg_inc ();
    hevents[p].type = hevtIntr;
    hevents[p].cpu_idx = current_running_cpu_idx;
    hevents[p].time = cpu.cycleCnt;
    strncpy (hevents[p].ctx, ctx, 15);
    hevents[p].ctx[15] = 0;
    hevents[p].intr.intr_pair_addr = intr_pair_addr;
done: ;
  }

void hdbgRegR (enum hregs_t type, word36 data, const char * ctx)
  {
CPUN;
    if (! hevents)
      goto done;
#ifdef ISOLTS
if (current_running_cpu_idx == 0)
  goto done;
#endif
    unsigned long p = hdbg_inc ();
    hevents[p].type = hevtReg;
    hevents[p].cpu_idx = current_running_cpu_idx;
    hevents[p].time = cpu.cycleCnt;
    strncpy (hevents[p].ctx, ctx, 15);
    hevents[p].ctx[15] = 0;
    hevents[p].rw = false;
    hevents[p].reg.type = type;
    hevents[p].reg.data = data;
done: ;
  }


void hdbgRegW (enum hregs_t type, word36 data, const char * ctx)
  {
CPUN;
    if (! hevents)
      goto done;
#ifdef ISOLTS
if (current_running_cpu_idx == 0)
  goto done;
#endif
    unsigned long p = hdbg_inc ();
    hevents[p].type = hevtReg;
    hevents[p].cpu_idx = current_running_cpu_idx;
    hevents[p].time = cpu.cycleCnt;
    strncpy (hevents[p].ctx, ctx, 15);
    hevents[p].ctx[15] = 0;
    hevents[p].rw = true;
    hevents[p].reg.type = type;
    hevents[p].reg.data = data;
done: ;
  }


void hdbgPARegR (enum hregs_t type, struct par_s * data, const char * ctx)
  {
CPUN;
    if (! hevents)
      goto done;
#ifdef ISOLTS
if (current_running_cpu_idx == 0)
  goto done;
#endif
    unsigned long p = hdbg_inc ();
    hevents[p].type = hevtPAReg;
    hevents[p].cpu_idx = current_running_cpu_idx;
    hevents[p].time = cpu.cycleCnt;
    strncpy (hevents[p].ctx, ctx, 15);
    hevents[p].ctx[15] = 0;
    hevents[p].rw = false;
    hevents[p].par.type = type;
    hevents[p].par.data =  * data;
done: ;
  }

void hdbgPARegW (enum hregs_t type, struct par_s * data, const char * ctx)
  {
CPUN;
    if (! hevents)
      goto done;
#ifdef ISOLTS
if (current_running_cpu_idx == 0)
  goto done;
#endif
    unsigned long p = hdbg_inc ();
    hevents[p].type = hevtPAReg;
    hevents[p].cpu_idx = current_running_cpu_idx;
    hevents[p].time = cpu.cycleCnt;
    strncpy (hevents[p].ctx, ctx, 15);
    hevents[p].ctx[15] = 0;
    hevents[p].rw = true;
    hevents[p].par.type = type;
    hevents[p].par.data =  * data;
done: ;
  }

void hdbgDSBRRegR (enum hregs_t type, struct dsbr_s * data, const char * ctx)
  {
CPUN;
    if (! hevents)
      goto done;
#ifdef ISOLTS
if (current_running_cpu_idx == 0)
  goto done;
#endif
    unsigned long p = hdbg_inc ();
    hevents[p].type = hevtDSBRReg;
    hevents[p].cpu_idx = current_running_cpu_idx;
    hevents[p].time = cpu.cycleCnt;
    strncpy (hevents[p].ctx, ctx, 15);
    hevents[p].ctx[15] = 0;
    hevents[p].rw = false;
    hevents[p].dsbr.type = type;
    hevents[p].dsbr.data =  * data;
done: ;
  }

void hdbgDSBRRegW (enum hregs_t type, struct dsbr_s * data, const char * ctx)
  {
CPUN;
    if (! hevents)
      goto done;
#ifdef ISOLTS
if (current_running_cpu_idx == 0)
  goto done;
#endif
    unsigned long p = hdbg_inc ();
    hevents[p].type = hevtDSBRReg;
    hevents[p].cpu_idx = current_running_cpu_idx;
    hevents[p].time = cpu.cycleCnt;
    strncpy (hevents[p].ctx, ctx, 15);
    hevents[p].ctx[15] = 0;
    hevents[p].rw = true;
    hevents[p].dsbr.type = type;
    hevents[p].dsbr.data =  * data;
done: ;
  }

static FILE * hdbgOut = NULL;

static void printM (struct hevt * p)
  {
    fprintf (hdbgOut, "DBG(%"PRId64")> CPU %d FINAL: %s %s %08o %012"PRIo64"\n",
                p -> time, 
                p -> cpu_idx,
                p->ctx,
                p->rw ? "write" : "read ",
                p -> memref.addr, p -> memref.data);
  }

static void printAPU (struct hevt * p)
  {
    fprintf (hdbgOut, "DBG(%"PRId64")> CPU %d APU: %s %s %05o:%06o %012"PRIo64"\n",
                p->time, 
                p->cpu_idx,
                p->ctx,
                p->rw ? "write" : "read ",
                p->apu.segno,
                p->apu.offset,
                p->apu.data);
  }

static void printTrace (struct hevt * p)
  {
    char buf[256];
    if (p -> trace.addrMode == ABSOLUTE_mode)
      {
        fprintf (hdbgOut, "DBG(%"PRId64")> CPU %d TRACE: %s %06o %o %012"PRIo64" (%s)\n",
                    p -> time, 
                    p -> cpu_idx,
                    p->ctx,
                    p -> trace.ic, p -> trace.ring,
                    p -> trace.inst, disassemble (buf, p -> trace.inst));
      }
    else
      {
        fprintf (hdbgOut, "DBG(%"PRId64")> CPU %d TRACE: %s %05o:%06o %o %012"PRIo64" (%s)\n",
                    p -> time, p -> cpu_idx, p->ctx, p -> trace.segno,
                    p -> trace.ic, p -> trace.ring,
                    p -> trace.inst, disassemble (buf, p -> trace.inst));
      }
  }

static void printFault (struct hevt * p)
  {
    fprintf (hdbgOut, "DBG(%"PRId64")> CPU %d FAULT: %s Fault %d(0%o), sub %"PRId64"(0%"PRIo64"), '%s'\n",
                p -> time, 
                p -> cpu_idx,
                p->ctx,
                p -> fault.faultNumber, p -> fault.faultNumber,
                p -> fault.subFault.bits, p -> fault.subFault.bits,
                p -> fault.faultMsg);
  }

static void printIntrSet (struct hevt * p)
  {
    fprintf (hdbgOut, "DBG(%"PRId64")> CPU %d INTR_SET: %s number %d(0%o), CPU %u SCU %u\n",
                p -> time, 
                p -> cpu_idx,
                p->ctx,
                p -> intrSet.inum, p -> intrSet.inum,
                p -> intrSet.cpuUnitIdx,
                p -> intrSet.scuUnitIdx);
  }

static void printIntr (struct hevt * p)
  {
    fprintf (hdbgOut, "DBG(%"PRId64")> CPU %d INTR: %s Interrupt pair address %o\n",
                p -> time, 
                p -> cpu_idx,
                p->ctx,
                p -> intr.intr_pair_addr);
  }

// Keep sync'd with hregs_t
static char * regNames[] =
  {
    "A ",
    "Q ",
    "X0", "X1", "X2", "X3", "X4", "X5", "X6", "X7",
    "AR0", "AR1", "AR2", "AR3", "AR4", "AR5", "AR6", "AR7",
    "PR0", "PR1", "PR2", "PR3", "PR4", "PR5", "PR6", "PR7",
    "DSBR"
  };

static void printReg (struct hevt * p)
  {
    if (p->reg.type >= hreg_X0 && p->reg.type <= hreg_X7)
      fprintf (hdbgOut, "DBG(%"PRId64")> CPU %d REG: %s %s %s %06"PRIo64"\n",
                  p->time, 
                  p->cpu_idx,
                  p->ctx,
                  p->rw ? "write" : "read ",
                  regNames[p->reg.type],
                  p->reg.data);
    else
      fprintf (hdbgOut, "DBG(%"PRId64")> CPU %d REG: %s %s  %s %012"PRIo64"\n",
                  p->time, 
                  p->cpu_idx,
                  p->ctx,
                  p->rw ? "write" : "read ",
                  regNames[p->reg.type],
                  p->reg.data);
  }

static void printPAReg (struct hevt * p)
  {
    if (p->reg.type >= hreg_PR0 && p->reg.type <= hreg_PR7)
      fprintf (hdbgOut, "DBG(%"PRId64")> CPU %d REG: %s %s %s "
               "%05o:%06o BIT %2o RNR %o\n",
               p->time, 
               p->cpu_idx,
               p->ctx,
               p->rw ? "write" : "read ",
               regNames[p->reg.type],
               p->par.data.SNR,
               p->par.data.WORDNO,
               p->par.data.PR_BITNO,
               p->par.data.RNR);
    else
      fprintf (hdbgOut, "DBG(%"PRId64")> CPU %d REG: %s write %s "
               "%05o:%06o CHAR %o BIT %2o RNR %o\n",
               p->time, 
               p->cpu_idx,
               p->ctx,
               regNames[p->reg.type],
               p->par.data.SNR,
               p->par.data.WORDNO,
               p->par.data.AR_CHAR,
               p->par.data.AR_BITNO,
               p->par.data.RNR);
  }

static void printDSBRReg (struct hevt * p)
  {
    fprintf (hdbgOut, "DBG(%"PRId64")> CPU %d REG: %s %s %s "
             "%05o:%06o BIT %2o RNR %o\n",
             p->time, 
             p->cpu_idx,
             p->ctx,
             p->rw ? "write" : "read ",
             regNames[p->reg.type],
             p->par.data.SNR,
             p->par.data.WORDNO,
             p->par.data.PR_BITNO,
             p->par.data.RNR);
  }

void hdbgPrint (void)
  {
    sim_printf ("hdbg print\n");
    if (! hevents)
      goto done;
    struct hevt * t = hevents;
    hevents = NULL;
    hdbgOut = fopen ("hdbg.list", "w");
    if (! hdbgOut)
      {
        sim_printf ("can't open hdbg.list\n");
        goto done;
      }
    time_t curtime;
    time (& curtime);
    fprintf (hdbgOut, "%s\n", ctime (& curtime));

    for (unsigned long p = 0; p < hdbgSize; p ++)
      {
        unsigned long q = (hevtPtr + p) % hdbgSize;
        struct hevt * evtp = t + q;
        switch (evtp -> type)
          {
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
                
            case hevtFault:
              printFault (evtp);
              break;
                
            case hevtIntrSet:
              printIntrSet (evtp);
              break;
                
            case hevtIntr:
              printIntr (evtp);
              break;
                
            case hevtPAReg:
              printPAReg (evtp);
              break;
                
            case hevtReg:
              printReg (evtp);
              break;
                
            case hevtDSBRReg:
              printDSBRReg (evtp);
              break;
                
            default:
              fprintf (hdbgOut, "hdbgPrint ? %d\n", evtp -> type);
              break;
          }
      }
    fclose (hdbgOut);
done: ;
  }

void hdbg_mark (void)
  {
    hevtMark = hdbgSize;
    sim_printf ("hdbg mark set to %ld\n", hevtMark);
  }

// set buffer size 
t_stat hdbg_size (UNUSED int32 arg, const char * buf)
  {
    hdbgSize = strtoul (buf, NULL, 0);
    sim_printf ("hdbg size set to %ld\n", hdbgSize);
    createBuffer ();
    return SCPE_OK;
  }

t_stat hdbg_print (UNUSED int32 arg, const char * buf)
  {
    hdbgPrint ();
    return SCPE_OK;
  }
#endif // HDBG
