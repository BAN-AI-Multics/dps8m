#!/usr/bin/env sh
# scspell-id: a947428c-19f4-11ef-8936-80ee73e9b8e7

set -eu

test -n "${SIGN_CMD:-}" || {
  test -z "${SIGN_CMD:-}" && SIGN_CMD="-S"
}

# shellcheck disable=SC2086
git subtree pull --prefix src/libsir https://github.com/aremmell/libsir.git master --squash ${SIGN_CMD:-}

# shellcheck disable=SC2086
git subtree pull --prefix src/libbacktrace https://github.com/ianlancetaylor/libbacktrace master --squash ${SIGN_CMD:-}

printf '\n%s\n' "Remember, don't squash when merging after subtree updates!" 2> /dev/null || true
