/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 838fb98b-f62e-11ec-8e20-80ee73e9b8e7
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

void tidy_cu (void);
void cu_safe_store(void);
#if defined(MATRIX)
void initializeTheMatrix (void);
void addToTheMatrix (uint32 opcode, bool opcodeX, bool a, word6 tag);
t_stat display_the_matrix (int32 arg, const char * buf);
#endif /* if defined(MATRIX) */
t_stat prepareComputedAddress (void);   // new
void cu_safe_restore(void);
void fetchInstruction(word18 addr);
t_stat executeInstruction (void);
void doRCU (void) NO_RETURN;
void traceInstruction (uint flag);
bool tstOVFfault (void);
bool chkOVF (void);
