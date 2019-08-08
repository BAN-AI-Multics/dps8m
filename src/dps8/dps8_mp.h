/*
 Copyright 2014-2016 by Charles Anthony

 All rights reserved.

 This software is made available under the terms of the
 ICU License -- ICU 1.8.1 and later.
 See the LICENSE file at the top-level directory of this distribution and
 at https://sourceforge.net/p/dps8m/code/ci/master/tree/LICENSE
 */

// Multipass data

#ifdef MULTIPASS

typedef struct multipassStats
  {
    struct ppr_s PPR;
    word36 inst;

    word36 A, Q, E, X [8], IR, TR, RALR;
    struct par_s PAR [8];
    word3 TRR;
    word15 TSR;
    word6 TBR;
    word18 CA;

    struct dsbr_s DSBR;

    _fault faultNumber;
    _fault_subtype subFault;

    uint intr_pair_addr;
    cycles_e cycle;

    uint64 cycles;


    uint64 diskSeeks;
    uint64 diskWrites;
    uint64 diskReads;

    
  } multipassStats;

extern multipassStats * multipassStatsPtr;

#endif
