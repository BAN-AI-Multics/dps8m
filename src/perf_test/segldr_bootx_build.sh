#!/usr/bin/env sh
../../../dps8m_devel_tools/src/as8+/as8+ segldr_bootx.as8 -o segldr_bootx.oct
./oct2bin < segldr_bootx.oct > segldr_bootx
