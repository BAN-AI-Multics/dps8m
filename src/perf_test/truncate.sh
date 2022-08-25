#!/usr/bin/env sh
# vim: filetype=sh:tabstop=2:tw=74:expandtab
# vim: ruler:hlsearch:incsearch:autoindent:wildmenu:wrapscan
# SPDX-License-Identifier: FSFAP
# scspell-id: c4edc562-1d84-11ed-b16c-80ee73e9b8e7

##########################################################################
#
# Copyright (c) 2022 The DPS8M Development Team
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered "AS-IS",
# without any warranty.
#
##########################################################################

# Truncate input file at last non-NULL byte and add NULL-termination.
# Requires: GNU awk, GNU coreutils truncate, xxd

##########################################################################

set -eu

##########################################################################

test -z "${1:-}" 2> /dev/null &&
  {
    printf '\r%s\n' "ERROR: No input file specified." >&2
    exit 1
  }

##########################################################################

# Disallow self-truncation
test "$(basename "${1:-}" 2> /dev/null)" = \
  "$(basename "${0:?}" 2> /dev/null)" 2> /dev/null &&
    {
      printf '\r%s\n' "ERROR: Bad input file specified." >&2
      exit 1
    }

##########################################################################

test -f "${1:-}" 2> /dev/null ||
  {
    printf '\r%s\n' "ERROR: Input file not found." >&2
    exit 1
  }

##########################################################################

printf '\rScanning %s ... ' "${1:?}" >&2

##########################################################################

# shellcheck disable=SC2016
B_OFFSET="$(
  exec 2> /dev/null
  ${XXD:-xxd} -c 1 "${1:?}" |
    ${GREP:-grep} -v '^.*: 00  .$' |
    ${TR:-tr} -d ':' |
    ${AWK:-awk} '{ printf("%d\n", strtonum("0x"$1)+2); }' |
    ${TAIL:-tail} -n 1 || printf '%s\n' "0"
)"

##########################################################################

# shellcheck disable=SC2015
test "${B_OFFSET:-0}" -gt 1 2> /dev/null &&
  {
    printf '\rTruncating %s at %d bytes.\n' "${1:?}" "${B_OFFSET:?}" >&2
    ${TRUNCATE:-truncate} -s "${B_OFFSET:?}" "${1:?}"
  } ||
  {
    printf '\r%s\n' "ERROR: Unable to truncate '${1:?}'" >&2
    exit 1
  }

##########################################################################