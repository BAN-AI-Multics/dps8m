#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: MIT
# scspell-id: 0cacef59-f631-11ec-9362-80ee73e9b8e7
# Copyright (c) 2021-2023 The DPS8M Development Team

##############################################################################

set -eu > /dev/null 2>&1

##############################################################################

# Assemble
../../../dps8m_devel_tools/src/as8+/as8+ nqueensx.as8 -o nqueensx.oct

##############################################################################

# Convert from as8m load format to packed 72 bit words
../../../dps8m_devel_tools/src/as8+/oct2bin < nqueensx.oct > nqueensx

##############################################################################
