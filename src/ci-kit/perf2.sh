#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: MIT-0
# scspell-id: scspell-id: dbc4b6d4-b75d-11ef-a4f2-80ee73e9b8e7
# Copyright (c) 2021-2024 The DPS8M Development Team

##############################################################################

export SHELL=/bin/sh

##############################################################################

command -v stdbuf > /dev/null 2>&1 && STDBUF="stdbuf -o L" || STDBUF="exec"

##############################################################################

test -d ./run && test -d ../perf_test && cd ./run && mkdir -p perf_test &&   \
  cp -Rf ../../perf_test/* perf_test && cd perf_test &&                      \
    ( ${STDBUF:?} ../dps8 --perftest > ../../perf2.log 2>&1 )

##############################################################################
