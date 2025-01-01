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
 * Copyright (c) 2021-2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

void tidy_cu (cpu_state_t * cpup);
void cu_safe_store(cpu_state_t * cpup);
#if defined(MATRIX)
void initializeTheMatrix (void);
void addToTheMatrix (uint32 opcode, bool opcodeX, bool a, word6 tag);
t_stat display_the_matrix (int32 arg, const char * buf);
#endif /* if defined(MATRIX) */
t_stat prepareComputedAddress (void);   // new
void cu_safe_restore(cpu_state_t * cpup);
void fetchInstruction(cpu_state_t * cpup, word18 addr);
t_stat executeInstruction (cpu_state_t * cpup);
NO_RETURN void doRCU (cpu_state_t * cpup);
void traceInstruction (uint flag);
bool tstOVFfault (cpu_state_t * cpup);
bool chkOVF (cpu_state_t * cpup);
