#!/usr/bin/env sh
# SPDX-License-Identifier: FSFAP
# scspell-id: 8824f39a-25f8-11ed-8241-80ee73e9b8e7
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

rm -rf ./resources > /dev/null 2>&1 || true
rm -rf ./public    > /dev/null 2>&1 || true

##########################################################################
