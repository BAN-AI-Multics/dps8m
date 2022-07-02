/*
 * vim: filetype=c:tabstop=4:tw=100:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: a3d21800-f62e-11ec-8838-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2021-2022 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

long double EAQToIEEElongdouble(void);
#if defined(__MINGW32__) || defined(__MINGW64__)
double EAQToIEEEdouble(void);
#endif
#ifndef QUIET_UNUSED
float72 IEEElongdoubleToFloat72(long double f);
void IEEElongdoubleToEAQ(long double f0);
double float36ToIEEEdouble(float36 f36);
float36 IEEEdoubleTofloat36(double f);
#endif
void ufa (bool sub);
void ufs (void);
void fno (word8 * E, word36 * A, word36 * Q);

void fneg (void);
void ufm (void);
void fdv (void);
void fdi (void);
void frd (void);
void fcmp(void);
void fcmg(void);

//void dufa (void);
//void dufs (void);
void dufa (bool subtract);
void dufm (void);
void dfdv (void);
void dfdi (void);
void dfrd (void);
void dfcmp (void);
void dfcmg (void);

void dvf (void);

void dfstr (word36 *Ypair);
void fstr(word36 *CY);
