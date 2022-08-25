#!/usr/bin/env sh
# SPDX-License-Identifier: FSFAP
# Copyright (c) 2022 The DPS8M Development Team

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
