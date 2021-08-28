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

void setupEISoperands (cpu_state_t *cpu_p);
void abd (cpu_state_t *cpu_p);
void awd (cpu_state_t *cpu_p);
void a4bd (cpu_state_t *cpu_p);
void axbd (cpu_state_t *cpu_p, uint sz);

void sbd (cpu_state_t *cpu_p);
void swd (cpu_state_t *cpu_p);
void s4bd (cpu_state_t *cpu_p);
void s9bd (cpu_state_t *cpu_p);
void sxbd (cpu_state_t *cpu_p, uint sz);
void asxbd (cpu_state_t *cpu_p, uint sz, bool sub);

void s4bd (cpu_state_t *cpu_p);
void cmpc (cpu_state_t *cpu_p);
void scd (cpu_state_t *cpu_p);
void scdr (cpu_state_t *cpu_p);
void scm (cpu_state_t *cpu_p);
void scmr (cpu_state_t *cpu_p);
void tct (cpu_state_t *cpu_p);
void tctr (cpu_state_t *cpu_p);
void mlr (cpu_state_t *cpu_p);
void mrl (cpu_state_t *cpu_p);
void mve (cpu_state_t *cpu_p);
void mvne (cpu_state_t *cpu_p);
void mvt (cpu_state_t *cpu_p);
void cmpn (cpu_state_t *cpu_p);
void mvn (cpu_state_t *cpu_p);
//void csl (bool isSZTL);
void csl (cpu_state_t *cpu_p);
void sztl (cpu_state_t *cpu_p);
void csr (cpu_state_t *cpu_p);
void sztr (cpu_state_t *cpu_p);
void cmpb (cpu_state_t *cpu_p);
void btd (cpu_state_t *cpu_p);
void dtb (cpu_state_t *cpu_p);
void ad2d (cpu_state_t *cpu_p);
void ad3d (cpu_state_t *cpu_p);
void sb2d (cpu_state_t *cpu_p);
void sb3d (cpu_state_t *cpu_p);
void mp2d (cpu_state_t *cpu_p);
void mp3d (cpu_state_t *cpu_p);
void dv2d (cpu_state_t *cpu_p);
void dv3d (cpu_state_t *cpu_p);
