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

test -f "hugoclean.sh" ||
  {
	printf '%s\n' "ERROR: 'hugoclean.sh' not found."
	exit 1
  }

##########################################################################

./hugoclean.sh

##########################################################################

${HUGO:-hugo}                         \
  -b "https://dps8m.gitlab.io/dps8m"  \
  -D                                  \
  --enableGitInfo                     \
  -v

##########################################################################
