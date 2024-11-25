#!/usr/bin/env sh
# SPDX-License-Identifier: MIT-0
# scspell-id: 6727225e-25f8-11ed-bafc-80ee73e9b8e7
# Copyright (c) 2022-2024 The DPS8M Development Team

##########################################################################

set -eu

##########################################################################

test -x "hugovars.sh" ||
  {
	printf '%s\n' "ERROR: 'hugovars.sh' not found."
	exit 1
  }

##########################################################################

# shellcheck disable=SC1091
. ./hugovars.sh

##########################################################################

${HUGO:-hugo} server           \
  -b "http://108.226.107.96/"  \
  --bind 0.0.0.0               \
  -D                           \
  --disableFastRender          \
  --enableGitInfo              \
  --ignoreCache                \
  --noHTTPCache                \
  -p 31313

##########################################################################
