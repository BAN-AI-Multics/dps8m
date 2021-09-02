/*
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 */

void setupEISoperands (cpu_state_t *cpuPtr);
void abd (cpu_state_t *cpuPtr);
void awd (cpu_state_t *cpuPtr);
void a4bd (cpu_state_t *cpuPtr);
void axbd (cpu_state_t *cpuPtr, uint sz);

void sbd (cpu_state_t *cpuPtr);
void swd (cpu_state_t *cpuPtr);
void s4bd (cpu_state_t *cpuPtr);
void s9bd (cpu_state_t *cpuPtr);
void sxbd (cpu_state_t *cpuPtr, uint sz);
void asxbd (cpu_state_t *cpuPtr, uint sz, bool sub);

void s4bd (cpu_state_t *cpuPtr);
void cmpc (cpu_state_t *cpuPtr);
void scd (cpu_state_t *cpuPtr);
void scdr (cpu_state_t *cpuPtr);
void scm (cpu_state_t *cpuPtr);
void scmr (cpu_state_t *cpuPtr);
void tct (cpu_state_t *cpuPtr);
void tctr (cpu_state_t *cpuPtr);
void mlr (cpu_state_t *cpuPtr);
void mrl (cpu_state_t *cpuPtr);
void mve (cpu_state_t *cpuPtr);
void mvne (cpu_state_t *cpuPtr);
void mvt (cpu_state_t *cpuPtr);
void cmpn (cpu_state_t *cpuPtr);
void mvn (cpu_state_t *cpuPtr);
//void csl (bool isSZTL);
void csl (cpu_state_t *cpuPtr);
void sztl (cpu_state_t *cpuPtr);
void csr (cpu_state_t *cpuPtr);
void sztr (cpu_state_t *cpuPtr);
void cmpb (cpu_state_t *cpuPtr);
void btd (cpu_state_t *cpuPtr);
void dtb (cpu_state_t *cpuPtr);
void ad2d (cpu_state_t *cpuPtr);
void ad3d (cpu_state_t *cpuPtr);
void sb2d (cpu_state_t *cpuPtr);
void sb3d (cpu_state_t *cpuPtr);
void mp2d (cpu_state_t *cpuPtr);
void mp3d (cpu_state_t *cpuPtr);
void dv2d (cpu_state_t *cpuPtr);
void dv3d (cpu_state_t *cpuPtr);
