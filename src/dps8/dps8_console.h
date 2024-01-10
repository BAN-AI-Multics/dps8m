/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 68960bb1-f62d-11ec-83fb-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

extern UNIT opc_unit [N_OPC_UNITS_MAX];
extern DEVICE opc_dev;

void console_init(void);
void console_exit (void);
void console_attn_idx (int opc_unit_idx);
int add_opc_autoinput (int32 flag, const char * cptr);
int clear_opc_autoinput (int32 flag, const char * cptr);
iom_cmd_rc_t opc_iom_cmd (uint iomUnitIdx, uint chan);
int check_attn_key (void);
void consoleProcess (void);
void startRemoteConsole (void);
