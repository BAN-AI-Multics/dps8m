#!/usr/bin/env sh
# vim: filetype=shell:tabstop=4:tw=76:expandtab

###############################################################################
#
# Copyright (c) 2021 Charles Anthony
# Copyright (c) 2021 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2021 The DPS8M Development Team
#
# All rights reserved.
#
# This software is made available under the terms of the ICU
# License, version 1.8.1 or later.  For more details, see the
# LICENSE.md file at the top-level directory of this distribution.
#
###############################################################################

set -e > /dev/null 2>&1
test -z "${VERBOSE:-}" || { set -x > /dev/null 2>&1; }
env "${MAKE:-make}" -C "../dps8" "shm.o" || gmake -C "../dps8" "shm.o"
# shellcheck disable=SC2086,SC2046
${CC:-cc} blinkenLights2.c $(env pkg-config gtk+-3.0 --cflags --libs) -I"../simh" -I"../dps8" -DLOCKLESS -DM_SHARED "../dps8/shm.o" -o blinkenLights2
