#!/usr/bin/env sh
# SPDX-License-Identifier: MIT-0
# scspell-id: 95f5e6a0-25f8-11ed-8120-80ee73e9b8e7
# Copyright (c) 2022-2025 The DPS8M Development Team

##########################################################################

set -e

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
