#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:tw=76:expandtab

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

../../../dps8m_devel_tools/src/as8+/as8+ segldr_bootx.as8 -o segldr_bootx.oct
./oct2bin < segldr_bootx.oct > segldr_bootx
