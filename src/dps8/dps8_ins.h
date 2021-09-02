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

void tidy_cu (cpu_state_t *cpuPtr);
void cu_safe_store(cpu_state_t *cpuPtr);
#ifdef MATRIX
void initializeTheMatrix (void);
void addToTheMatrix (uint32 opcode, bool opcodeX, bool a, word6 tag);
t_stat display_the_matrix (int32 arg, const char * buf);
#endif
t_stat prepareComputedAddress (void);   // new
void cu_safe_restore(cpu_state_t *cpuPtr);
void fetchInstruction(cpu_state_t *cpuPtr, word18 addr);
t_stat executeInstruction (cpu_state_t *cpuPtr);
void traceInstruction (cpu_state_t *cpuPtr, uint flag);
bool tstOVFfault (cpu_state_t *cpuPtr);
bool chkOVF (cpu_state_t *cpuPtr);
