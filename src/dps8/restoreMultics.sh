#!/usr/bin/env sh
# shellcheck disable=SC2129,SC2248,SC2250

T=../../../bitsavers.trailing-edge.com/bits/Honeywell/multics/tape/
(cd ../tapeUtils && make restore_tape)

../tapeUtils/restore_tape MR12.3 $T/88534.tap $T/88631.tap > restoreMultics.log
../tapeUtils/restore_tape MR12.3 $T/98570.tap $T/99019.tap >> restoreMultics.log
../tapeUtils/restore_tape MR12.3 $T/88632.tap $T/88633.tap $T/88634.tap $T/88635.tap $T/88636.tap $T/99020.tap >> restoreMultics.log
../tapeUtils/restore_tape MR12.3 $T/93085.tap >> restoreMultics.log

