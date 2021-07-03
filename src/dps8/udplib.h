/*
 * Copyright (c) 2015-2016 Charles Anthony
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE file at the top-level directory of this distribution.
 */

#define NOLINK  (-1)
#define PFLG_FINAL 00001
int udp_create (const char * premote, int32_t * plink);
int udp_release (int32_t link);
int udp_send (int32_t link, uint16_t * pdata, uint16_t count, uint16_t flags);
int udp_receive (int32_t link, uint16_t * pdata, uint16_t maxbufg);
