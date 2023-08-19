#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: MIT-0
# scspell-id: 2596a7a7-f631-11ec-b516-80ee73e9b8e7
# Copyright (c) 2021-2023 The DPS8M Development Team

##############################################################################

../../../dps8m_devel_tools/src/as8+/as8+ segldr_bootx.as8 -o segldr_bootx.oct

##############################################################################

./oct2bin < segldr_bootx.oct > segldr_bootx

##############################################################################
