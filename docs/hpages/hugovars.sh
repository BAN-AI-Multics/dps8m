#!/usr/bin/env sh
# SPDX-License-Identifier: FSFAP
# scspell-id: 95f5e6a0-25f8-11ed-8120-80ee73e9b8e7
# Copyright (c) 2022-2023 The DPS8M Development Team

##########################################################################
# Copying and distribution of this file, with or without
# modification, are permitted in any medium without royalty
# provided the copyright notice and this notice are preserved.
# This file is offered "AS-IS", without any warranty.
##########################################################################

set -eu

##########################################################################

test -f "config.toml" ||
  {
	printf '%s\n' "ERROR: 'config.toml' not found."
	exit 1
  }

##########################################################################

# Build date (assumes GNU date)   TODO: Check for failure of date -u
# shellcheck disable=SC2155
export HUGO_PARAMS_buildDate="$(date -u '+%Y-%m-%d %H:%M:%S %Z.')"

##########################################################################
