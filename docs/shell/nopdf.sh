#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: FSFAP
# scspell-id: f788dd21-3e56-11ed-ad69-80ee73e9b8e7

################################################################################
#
# Copyright (c) 2022-2023 The DPS8M Development Team
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered "AS-IS",
# without any warranty.
#
################################################################################

# Requires GNU sed.

sed '/<!-- start nopdf -->/,/<!-- stop nopdf -->/{//!d}' -

################################################################################
