/*
 * vim: filetype=c:tabstop=2:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: a9bac148-ffaf-11ef-87da-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2025 Jeffrey H. Johnson
 * Copyright (c) 2025 The DPS8M Development Team
 *
 * This software is made available under the terms of the ICU License.
 * See the LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

#if !defined(INCLUDED_DPS8_TOPO_H)
# define INCLUDED_DPS8_TOPO_H

# include <stdint.h>
# include <stdbool.h>

extern bool dps8_topo_used;
uint32_t get_core_count(void);

#endif
