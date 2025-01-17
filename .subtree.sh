#!/usr/bin/env sh
# SPDX-License-Identifier: MIT-0
# Copyright (c) 2024-2025 The DPS8M Development Team
# scspell-id: a947428c-19f4-11ef-8936-80ee73e9b8e7

set -eu

test -n "${SIGN_CMD:-}" || {
  test -z "${SIGN_CMD:-}" && SIGN_CMD="-S"
}

set -x

# shellcheck disable=SC2086
git subtree pull --prefix src/libsir https://github.com/aremmell/libsir.git master --squash ${SIGN_CMD:-}

set +x

printf '\n%s\n' "Remember, don't squash when merging after subtree updates!" 2> /dev/null || true
