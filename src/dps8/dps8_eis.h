/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 359ee1b9-f62e-11ec-b168-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

void setupEISoperands (cpu_state_t * cpup);
void abd (cpu_state_t * cpup);
void awd (cpu_state_t * cpup);
void a4bd (cpu_state_t * cpup);
void axbd (cpu_state_t * cpup, uint sz);

void sbd (cpu_state_t * cpup);
void swd (cpu_state_t * cpup);
void s4bd (cpu_state_t * cpup);
void s9bd (cpu_state_t * cpup);
void sxbd (cpu_state_t * cpup, uint sz);
void asxbd (cpu_state_t * cpup, uint sz, bool sub);

void s4bd (cpu_state_t * cpup);
void cmpc (cpu_state_t * cpup);
void scd (cpu_state_t * cpup);
void scdr (cpu_state_t * cpup);
void scm (cpu_state_t * cpup);
void scmr (cpu_state_t * cpup);
void tct (cpu_state_t * cpup);
void tctr (cpu_state_t * cpup);
void mlr (cpu_state_t * cpup);
void mrl (cpu_state_t * cpup);
void mve (cpu_state_t * cpup);
void mvne (cpu_state_t * cpup);
void mvt (cpu_state_t * cpup);
void cmpn (cpu_state_t * cpup);
void mvn (cpu_state_t * cpup);
//void csl (bool isSZTL);
void csl (cpu_state_t * cpup);
void sztl (cpu_state_t * cpup);
void csr (cpu_state_t * cpup);
void sztr (cpu_state_t * cpup);
void cmpb (cpu_state_t * cpup);
void btd (cpu_state_t * cpup);
void dtb (cpu_state_t * cpup);
void ad2d (cpu_state_t * cpup);
void ad3d (cpu_state_t * cpup);
void sb2d (cpu_state_t * cpup);
void sb3d (cpu_state_t * cpup);
void mp2d (cpu_state_t * cpup);
void mp3d (cpu_state_t * cpup);
void dv2d (cpu_state_t * cpup);
void dv3d (cpu_state_t * cpup);
