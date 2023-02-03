#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: FSFAP
# scspell-id: 5a7e9ca5-f62c-11ec-b7cb-80ee73e9b8e7

##############################################################################
#
# Copyright (c) 2023 The DPS8M Development Team
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered "AS-IS",
# without any warranty.
#
##############################################################################

test -d "./.git" ||
  {
    printf '%s\n' \
      '*** Error: Not in top-level git repository.'
    exit 1
  }

##############################################################################

test -x "/bin/sh" ||
  {
    printf '%s\n' \
      '*** Error: No "/bin/sh" executable.'
    exit 1
  }

##############################################################################

pvs-studio-analyzer -h > /dev/null 2>&1 ||
  {
    printf '%s\n' \
      '*** Error: No working "pvs-studio-analyzer" in PATH.'
    exit 1
  }

##############################################################################

plog-converter -h 2>&1 | grep -q PVS-Studio > /dev/null 2>&1 ||
  {
    printf '%s\n' \
      '*** Error: No working "plog-converter" in PATH.'
    exit 1
  }

##############################################################################

bear --version 2>&1 > /dev/null 2>&1 ||
  {
    printf '%s\n' \
      '*** Error: No working "bear" in PATH.'
    exit 1
  }

##############################################################################

printf '%s\n\n' "Starting in 5s, saving output to ./pvsreport"
sleep 5 > /dev/null 2>&1 || true

##############################################################################

printf '#### %s\n' "Begin ${0} (${$})"

##############################################################################

export SHELL=/bin/sh

##############################################################################

set -eu > /dev/null 2>&1

##############################################################################

rm -rf ./pvsreport                    > /dev/null 2>&1
rm -f  ./log.pvs                      > /dev/null 2>&1
rm -f  ./compile_commands.json        > /dev/null 2>&1
rm -f  ./compile_commands.events.json > /dev/null 2>&1

##############################################################################

# shellcheck disable=SC2086
${MAKE:-make} --no-print-directory "clean"

##############################################################################

unset MBS; test -z "${MAKEFLAGS:-}" &&
MBS="-j $(grep -c '^model name' /proc/cpuinfo 2> /dev/null || printf %s\\n 4)"

##############################################################################

# shellcheck disable=SC2086
env TZ=UTC bear -- ${MAKE:-make} --no-print-directory ${MBS:-}

##############################################################################

# shellcheck disable=SC2086
pvs-studio-analyzer analyze --intermodular -o log.pvs ${MBS:-} || true
plog-converter -a "GA:1,2" -t fullhtml log.pvs -o pvsreport \
    -d V576,V629,V701,V768 || true

##############################################################################

mkdir -p pvsreport
touch pvsreport/index.html

##############################################################################

printf '%s\n' \
  '*** Check the "pvsreport" directory for output.'

##############################################################################

printf '#### %s\n' "End ${0} (${$})"

##############################################################################