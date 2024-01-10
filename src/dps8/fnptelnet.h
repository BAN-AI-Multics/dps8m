/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: 2a1a0489-f62f-11ec-9a33-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2016 Charles Anthony
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

void fnpTelnetInit (void);
void fnp3270Init (void);
void * ltnConnect (uv_tcp_t * client);
void * ltnConnect3270 (uv_tcp_t * client);
void ltnRaw (telnet_t * client);
void ltnEOR (telnet_t * tclient);
void ltnDialout (telnet_t * client);
