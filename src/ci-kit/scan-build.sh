#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:tw=78:expandtab
# SPDX-License-Identifier: FSFAP
# scspell-id: 5a7e9ca5-f62c-11ec-b7cb-80ee73e9b8e7

##############################################################################
#
# Copyright (c) 2022 The DPS8M Development Team
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

scan-build -h > /dev/null 2>&1 ||
  {
    printf '%s\n' \
      '*** Error: No "scan-build" in PATH.'
    exit 1
  }

##############################################################################

printf '%s\n\n' "Starting in 5s, saving output to ./out"
sleep 5 > /dev/null 2>&1 || true

##############################################################################

printf '#### %s\n' "Begin ${0} (${$})"

##############################################################################

export SHELL=/bin/sh

##############################################################################

set -eu > /dev/null 2>&1

##############################################################################

rm -rf ./out > /dev/null 2>&1

##############################################################################

# shellcheck disable=SC2086
${MAKE:-make} --no-print-directory "clean"

##############################################################################

# shellcheck disable=SC2086
env TZ=UTC scan-build -no-failure-reports -o ./out -maxloop 16               \
 -enable-checker  optin.portability.UnixAPI                                  \
 -enable-checker  security.FloatLoopCounter                                  \
 -enable-checker  security.insecureAPI.bcmp                                  \
 -enable-checker  security.insecureAPI.bcopy                                 \
 -disable-checker deadcode.DeadStores                                        \
 ${MAKE:-make} --no-print-directory                                          \
  -j "$(grep -c '^model name' /proc/cpuinfo 2> /dev/null || printf %s\\n 4)"

##############################################################################

# shellcheck disable=SC2086
${MAKE:-make} --no-print-directory "clean"

##############################################################################

# Move results from out/subdirectory to out
( mv -f ./out/*/* ./out > /dev/null 2>&1 || true) > /dev/null 2>&1 || true

##############################################################################

# Remove out/subdirectory
( rmdir ./out/* > /dev/null 2>&1 || true ) > /dev/null 2>&1 || true

##############################################################################

# Remove out (if empty)
rmdir ./out > /dev/null 2>&1 || true

##############################################################################

printf '#### %s\n' "End ${0} (${$})"

##############################################################################
