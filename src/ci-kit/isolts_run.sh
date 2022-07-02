#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:tw=78:expandtab
# SPDX-License-Identifier: FSFAP
# scspell-id: 3ab793df-f62c-11ec-8165-80ee73e9b8e7

##############################################################################
#
# Copyright (c) 2021-2022 The DPS8M Development Team
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

exec ${STDBUF:?} expect -f isolts_run.expect 2>&1

##############################################################################
