#!/usr/bin/env sh
# shellcheck disable=SC2129,SC2248,SC2250
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: MIT-0
# scspell-id: bce8f1e5-f62f-11ec-9d8e-80ee73e9b8e7
# Copyright (c) 2016-2021 Charles Anthony
# Copyright (c) 2021-2024 The DPS8M Development Team

##############################################################################

T=../../../bitsavers.trailing-edge.com/bits/Honeywell/multics/tape/

##############################################################################

( cd ../tapeUtils && ${MAKE:-make} restore_tape )

##############################################################################

../tapeUtils/restore_tape MR12.3 ${T:?}/88534.tap ${T:?}/88631.tap           \
     > restoreMultics.log

##############################################################################

../tapeUtils/restore_tape MR12.3 ${T:?}/98570.tap ${T:?}/99019.tap           \
    >> restoreMultics.log

##############################################################################

../tapeUtils/restore_tape MR12.3 ${T:?}/88632.tap ${T:?}/88633.tap           \
       ${T:?}/88634.tap ${T:?}/88635.tap ${T:?}/88636.tap ${T:?}/99020.tap   \
    >> restoreMultics.log

##############################################################################

../tapeUtils/restore_tape MR12.3 ${T:?}/93085.tap                            \
    >> restoreMultics.log

##############################################################################
