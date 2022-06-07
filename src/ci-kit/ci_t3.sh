#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:tw=78:expandtab

############################################################################
#
# Copyright (c) 2021-2022 The DPS8M Development Team
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered "AS-IS",
# without any warranty.
#
############################################################################

export SHELL=/bin/sh
command -v stdbuf > /dev/null 2>&1 && STDBUF="stdbuf -o L" || STDBUF="exec"
exec ${STDBUF:?} expect -f ci_t3.expect 2>&1
