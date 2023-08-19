#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: MIT-0
# scspell-id: f90d785c-a3ed-11ed-a539-80ee73e9b8e7
# Copyright (c) 2023 The DPS8M Development Team

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
