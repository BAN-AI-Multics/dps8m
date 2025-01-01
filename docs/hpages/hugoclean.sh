#!/usr/bin/env sh
# SPDX-License-Identifier: MIT-0
# scspell-id: 8824f39a-25f8-11ed-8241-80ee73e9b8e7
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

rm -rf ./resources > /dev/null 2>&1 || true
rm -rf ./public    > /dev/null 2>&1 || true

##########################################################################
