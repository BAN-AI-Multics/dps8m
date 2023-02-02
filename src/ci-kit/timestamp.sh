#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: FSFAP
# scspell-id: 74e820f7-f62c-11ec-8aa1-80ee73e9b8e7

##############################################################################
#
# Copyright (c) 2021-2023 The DPS8M Development Team
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered "AS-IS",
# without any warranty.
#
##############################################################################

HAVE_STDBUF=0
command -v "stdbuf" > /dev/null 2>&1 &&
    {
      HAVE_STDBUF=1
      TS_STDBUF="stdbuf -o L"
    }

##############################################################################

HAVE_FILTER=0
command ansifilter --version 2>&1 | \
  grep -q 'ansifilter version' 2> /dev/null &&
    {
      HAVE_FILTER=1
      TS_FILTER="ansifilter -T"
    }

##############################################################################

HAVE_STAMPER=0
command ts --help 2>&1 | \
  grep -q  'usage:.*-s.*\[-m\].*\[format\]' 2> /dev/null &&
    {
      HAVE_STAMPER=1
      TS_STAMPER="ts -s -m '[%H:%M:%.S]'"
    }
test "${HAVE_STAMPER:-0}" -eq 0 2> /dev/null &&
    {
        command moreutils-ts --help 2>&1 | \
          grep -q  'usage:.*-s.*\[-m\].*\[format\]' 2> /dev/null &&
            {
              HAVE_STAMPER=1
              TS_STAMPER="moreutils-ts -s -m '[%H:%M:%.S]'"
            }
    }

##############################################################################

test "${HAVE_FILTER:-0}" -eq 1          2> /dev/null &&
    test "${HAVE_STAMPER:-0}" -eq 1     2> /dev/null &&
        test "${HAVE_STDBUF:-0}" -eq 1  2> /dev/null &&
            TS_TIMESTAMPER="${TS_STDBUF:?} ${TS_FILTER:?} | ${TS_STAMPER:?}"
printf '%s\n' "${TS_TIMESTAMPER:-cat}"

##############################################################################
