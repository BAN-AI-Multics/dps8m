#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: MIT
# scspell-id: 43a1f702-f62b-11ec-9572-80ee73e9b8e7
# Copyright (c) 2021 Charles Anthony
# Copyright (c) 2021 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2021-2023 The DPS8M Development Team

###############################################################################

set -e > /dev/null 2>&1

###############################################################################

test -z "${VERBOSE:-}" || \
  { set -x > /dev/null 2>&1; }

###############################################################################

# shellcheck disable=SC2312
env "${MAKE:-make}" -C "../dps8" "shm.o" || \
  gmake -C "../dps8" "shm.o"

###############################################################################

# shellcheck disable=SC2086,SC2046,SC2312
${CC:-cc} blinkenLights2.c $(env pkg-config gtk+-3.0 --cflags --libs)  \
  ${MATHLIB:--lm} -I"../simh" -I"../dps8" -DLOCKLESS -DM_SHARED        \
    "../dps8/shm.o" -o blinkenLights2

###############################################################################
