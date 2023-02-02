#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: FSFAP
# scspell-id: 4d5c9d65-f62c-11ec-8223-80ee73e9b8e7

##############################################################################
#
# Copyright (c) 2021-2023 The DPS8M Development Team
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered "AS-IS",
# without any warranty.
#
##############################################################################

export SHELL=/bin/sh

##############################################################################

command -v stdbuf > /dev/null 2>&1 && STDBUF="stdbuf -o L" || STDBUF="exec"

##############################################################################

test -d ./run && test -d ../perf_test && cd ./run && mkdir -p perf_test &&   \
  cp -Rf ../../perf_test/* perf_test && cd perf_test &&                      \
    ( ${STDBUF:?} ../dps8 -t -q nqueensx.ini > ../../perf.log 2>&1 )

##############################################################################
