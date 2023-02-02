#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: FSFAP
# scspell-id: 2596a7a7-f631-11ec-b516-80ee73e9b8e7

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

../../../dps8m_devel_tools/src/as8+/as8+ segldr_bootx.as8 -o segldr_bootx.oct

##############################################################################

./oct2bin < segldr_bootx.oct > segldr_bootx

##############################################################################
