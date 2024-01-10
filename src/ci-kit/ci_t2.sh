#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: MIT-0
# scspell-id: ae1f7c15-f62b-11ec-b2a3-80ee73e9b8e7
# Copyright (c) 2021-2024 The DPS8M Development Team

##############################################################################

export SHELL=/bin/sh

##############################################################################

command -v stdbuf > /dev/null 2>&1 && STDBUF="stdbuf -o L" || STDBUF="exec"

##############################################################################

exec ${STDBUF:?} expect -f ci_t2.expect 2>&1

##############################################################################
