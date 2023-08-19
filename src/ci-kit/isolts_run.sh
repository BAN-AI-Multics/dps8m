#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: MIT-0
# scspell-id: 3ab793df-f62c-11ec-8165-80ee73e9b8e7
# Copyright (c) 2021-2023 The DPS8M Development Team

##############################################################################

export SHELL=/bin/sh

##############################################################################

command -v stdbuf > /dev/null 2>&1 && STDBUF="stdbuf -o L" || STDBUF="exec"

##############################################################################

exec ${STDBUF:?} expect -f isolts_run.expect 2>&1

##############################################################################
