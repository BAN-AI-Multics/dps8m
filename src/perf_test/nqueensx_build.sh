#!/usr/bin/env sh
set -eu > /dev/null 2>&1
# Assemble
../../../dps8m_devel_tools/src/as8+/as8+ nqueensx.as8 -o nqueensx.oct
# Convert from as8m load format to packed 72 bit words
../../../dps8m_devel_tools/src/as8+/oct2bin < nqueensx.oct > nqueensx
