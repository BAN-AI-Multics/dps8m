/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: a3d21800-f62e-11ec-8838-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

long double EAQToIEEElongdouble(cpu_state_t * cpup);
#if defined(__MINGW32__) || defined(__MINGW64__)
double EAQToIEEEdouble(void);
#endif
#if !defined(QUIET_UNUSED)
float72 IEEElongdoubleToFloat72(long double f);
void IEEElongdoubleToEAQ(long double f0);
double float36ToIEEEdouble(float36 f36);
float36 IEEEdoubleTofloat36(double f);
#endif /* if !defined(QUIET_UNUSED) */
void ufa (cpu_state_t * cpup, bool sub, bool normalize);
void ufs (cpu_state_t * cpup);
void fno (cpu_state_t * cpup, word8 * E, word36 * A, word36 * Q);
void fno_ext (cpu_state_t * cpup, int * e0, word8 * E, word36 * A, word36 * Q);

void fneg (cpu_state_t * cpup);
void ufm (cpu_state_t * cpup, bool normalize);
void fdv (cpu_state_t * cpup);
void fdi (cpu_state_t * cpup);
void frd (cpu_state_t * cpup);
void fcmp(cpu_state_t * cpup);
void fcmg(cpu_state_t * cpup);

//void dufa (void);
//void dufs (void);
void dufa (cpu_state_t * cpup, bool subtract, bool normalize);
void dufm (cpu_state_t * cpup, bool normalize);
void dfdv (cpu_state_t * cpup);
void dfdi (cpu_state_t * cpup);
void dfrd (cpu_state_t * cpup);
void dfcmp (cpu_state_t * cpup);
void dfcmg (cpu_state_t * cpup);

void dvf (cpu_state_t * cpup);

void dfstr (cpu_state_t * cpup, word36 *Ypair);
void fstr(cpu_state_t * cpup, word36 *CY);
