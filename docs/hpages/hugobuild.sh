#!/usr/bin/env sh
# SPDX-License-Identifier: MIT-0
# scspell-id: 81cc5dc8-25f8-11ed-a431-80ee73e9b8e7
# Copyright (c) 2022-2025 The DPS8M Development Team

##########################################################################

set -eu

##########################################################################

test -f "config.toml" ||
  {
	printf '%s\n' "ERROR: 'config.toml' not found."
	exit 1
  }

##########################################################################

test -f "hugoclean.sh" ||
  {
	printf '%s\n' "ERROR: 'hugoclean.sh' not found."
	exit 1
  }

##########################################################################

# shellcheck disable=SC1091
./hugoclean.sh

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

${HUGO:-hugo}                         \
  -b "https://dps8m.gitlab.io/dps8m"  \
  --enableGitInfo

##########################################################################
