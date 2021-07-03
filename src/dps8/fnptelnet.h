/*
 * Copyright (c) 2016 Charles Anthony
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE file at the top-level directory of this distribution.
 */

void fnpTelnetInit (void);
void fnp3270Init (void);
void * ltnConnect (uv_tcp_t * client);
void * ltnConnect3270 (uv_tcp_t * client);
void ltnRaw (telnet_t * client);
void ltnEOR (telnet_t * tclient);
void ltnDialout (telnet_t * client);
