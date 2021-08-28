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

long double EAQToIEEElongdouble(cpu_state_t *cpu_p);
double EAQToIEEEdouble(cpu_state_t *cpu_p);
#ifndef QUIET_UNUSED
float72 IEEElongdoubleToFloat72(long double f);
void IEEElongdoubleToEAQ(long double f0);
double float36ToIEEEdouble(float36 f36);
float36 IEEEdoubleTofloat36(double f);
#endif
void ufa (cpu_state_t *cpu_p, bool sub);
void ufs (void);
void fno (cpu_state_t *cpu_p, word8 * E, word36 * A, word36 * Q);

void fneg (cpu_state_t *cpu_p);
void ufm (cpu_state_t *cpu_p);
void fdv (cpu_state_t *cpu_p);
void fdi (cpu_state_t *cpu_p);
void frd (cpu_state_t *cpu_p);
void fcmp(cpu_state_t *cpu_p);
void fcmg(cpu_state_t *cpu_pvoid);

void dufa (cpu_state_t *cpu_p, bool subtraact);
void dufm (cpu_state_t *cpu_p);
void dfdv (cpu_state_t *cpu_p);
void dfdi (cpu_state_t *cpu_p);
void dfrd (cpu_state_t *cpu_p);
void dfcmp (cpu_state_t *cpu_p);
void dfcmg (cpu_state_t *cpu_p);

void dvf (cpu_state_t *cpu_p);

void dfstr (cpu_state_t *cpu_p, word36 *Ypair);
void fstr(cpu_state_t *cpu_p, word36 *CY);
