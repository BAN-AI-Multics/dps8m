/*
 * Copyright (c) 2019 Charles Anthony
 * Copyright (c) 2021 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 */

struct system_state_s
  {
    char commit_id [41];
    vol word36 M [MEMSIZE];
    cpu_state_t cpus [N_CPU_UNITS_MAX];
    struct cables_s cables;
  };

extern struct system_state_s * system_state;
