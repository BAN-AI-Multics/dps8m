#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: MIT-0
# scspell-id: 557fb5c2-f62b-11ec-a98f-80ee73e9b8e7
# Copyright (c) 2021-2025 The DPS8M Development Team

####################################################################################################

export SHELL=/bin/sh > /dev/null 2>&1

####################################################################################################

command -v "gawk" > /dev/null 2>&1 ||
  {
    printf '%s\n'  \
      "*** Error: No GNU awk command available - try: pkg install gawk."
    exit 1
  }

####################################################################################################

command -v "gsed" > /dev/null 2>&1 ||
  {
    printf '%s\n'  \
      "*** Error: No GNU sed command available - try: pkg install gsed."
    exit 1
  }

####################################################################################################

command -v "dos2unix" > /dev/null 2>&1 ||
  {
    printf '%s\n'  \
      "*** Error: No dos2unix command available - try: pkg install unix2dos."
    exit 1
  }

####################################################################################################

command -v "tmux" > /dev/null 2>&1 ||
  {
    printf '%s\n'  \
      "*** Error: No tmux command available - try: pkg install tmux."
    exit 1
  }

####################################################################################################

command -v "expect" > /dev/null 2>&1 ||
  {
    printf '%s\n'  \
      "*** Error: No expect command available - try: pkg install expect."
    exit 1
  }

####################################################################################################

command -v "moreutils-ts" > /dev/null 2>&1 ||
  {
    printf '%s\n'  \
      "*** Error: No moreutils-ts command available - try: pkg install moreutils."
    exit 1
  }

####################################################################################################

command -v "ansifilter" > /dev/null 2>&1 ||
  {
    printf '%s\n'  \
      "*** Error: No ansifilter command available - try: pkg install ansifilter."
    exit 1
  }

####################################################################################################

command -v "gmake" > /dev/null 2>&1 ||
  {
    printf '%s\n'  \
      "*** Error: No GNU make command available - try: pkg install gmake."
    exit 1
  }

####################################################################################################

command -v "gtail" > /dev/null 2>&1 ||
  {
    printf '%s\n'  \
      "*** Error: No gtail command available - try: pkg install coreutils."
    exit 1
  }

####################################################################################################

command -v "gdate" > /dev/null 2>&1 ||
  {
    printf '%s\n'  \
      "*** Error: No gdate command available - try: pkg install coreutils."
    exit 1
  }

####################################################################################################

command -v "mksh" > /dev/null 2>&1 ||
  {
    printf '%s\n'  \
      "*** Error: No mksh command available - try: pkg install mksh."
    exit 1
  }

####################################################################################################

command -v "ncat" > /dev/null 2>&1 ||
  {
    printf '%s\n'  \
      "*** Error: No ncat command available - try: pkg install nmap."
    exit 1
  }

####################################################################################################

${SHELL:?} -c \
    'export SHELL=/bin/sh ; env TAIL=gtail MAKE=gmake DATE=gdate mksh ./ci.sh'

####################################################################################################
