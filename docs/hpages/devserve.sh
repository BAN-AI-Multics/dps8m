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

${HUGO:-hugo} server           \
  -b "http://108.226.107.96/"  \
  --bind 0.0.0.0               \
  -D                           \
  --disableFastRender          \
  --enableGitInfo              \
  --ignoreCache                \
  --noHTTPCache                \
  -p 31313                     \
  -v

##########################################################################
