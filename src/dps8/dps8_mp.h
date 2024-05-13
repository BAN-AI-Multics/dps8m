/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: a8a08b3c-f62e-11ec-bfa3-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2014-2016 Charles Anthony
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

// Multipass data

#if defined(MULTIPASS)

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
