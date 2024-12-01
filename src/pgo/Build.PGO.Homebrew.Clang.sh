#!/usr/bin/env bash
# scspell-id: 02f2e258-63e9-11ef-8480-80ee73e9b8e7
# SPDX-License-Identifier: MIT-0
# Copyright (c) 2021-2024 The DPS8M Development Team
# shellcheck disable=SC2312

set -e

# Sanity test
printf '%s\n' "Checking for PGO script ..."
test -x "./src/pgo/Build.PGO.Homebrew.Clang.sh" \
  || {
    printf '%s\n' "ERROR: Unable to find PGO script!"
    exit 1
  }

# Homebrew LLVM
brew info llvm 2> /dev/null | grep -q '^Installed' \
  || {
    printf '%s\n' "ERROR: Homebrew LLVM not installed."
    exit 1
  }

# Homebrew LLD
brew info lld 2> /dev/null | grep -q '^Installed' \
  || {
    printf '%s\n' "ERROR: Homebrew LLD not installed."
    exit 1
  }

# Compiler
PATH="$(brew --prefix llvm)"/bin:"${PATH:-}"
export PATH
CC="$(brew --prefix llvm)"/bin/clang
export CC

# Test
"${CC:?}" --version

# LIBUVVER
test -z "${LIBUVVER:-}" && LIBUVVER="libuvrel"
export LIBUVVER

# Setup
printf '\n%s\n' "Setting up PGO build ..."
PROFILE_PATH="$(pwd -P)/.profile_data"
export PROFILE_PATH
test -d "${PROFILE_PATH}" && rm -rf "${PROFILE_PATH}"
mkdir -p "${PROFILE_PATH}"
export BASE_LDFLAGS="${LDFLAGS:-}"
export BASE_CFLAGS="-Dftello64=ftello -Doff64_t=off_t -Dfseeko64=fseeko \
  -Dfopen64=fopen -fno-profile-sample-accurate -fno-semantic-interposition \
  ${CFLAGS:-}"
export LLVM_PROFILE_FILE="${PROFILE_PATH:?}/profile.%p.profraw"

# Profile
printf '\n%s\n' "Generating profile build ..."
export CFLAGS="-fprofile-generate=${PROFILE_PATH:?}/profile.%p.profraw \
  ${BASE_CFLAGS:?} -Wno-ignored-optimization-argument"
export LDFLAGS="${BASE_LDFLAGS:-} ${CFLAGS:?} -fuse-ld=lld"
${MAKE:-make} distclean "${@}" HOMEBREW_LIB= HOMEBREW_INC=
${MAKE:-make} "${LIBUVVER:?}" "${@}" HOMEBREW_LIB= HOMEBREW_INC=
${MAKE:-make} "${@}" HOMEBREW_LIB= HOMEBREW_INC=
printf '\n%s\n' "Generating profile ..."
(cd src/perf_test && ../dps8/dps8 -r ./nqueensx.ini)
"$(brew --prefix llvm)/bin/llvm-profdata" merge --sparse=true \
  --gen-partial-profile=true -output="${PROFILE_PATH:?}/final.profdata" \
  "${PROFILE_PATH:?}"/profile.*.profraw

# Build
printf '\n%s\n' "Generating final build ..."
export CFLAGS="-fprofile-use=${PROFILE_PATH:?}/final.profdata \
  ${BASE_CFLAGS:?} -Wno-ignored-optimization-argument"
export LDFLAGS="${BASE_LDFLAGS:-} ${CFLAGS:?} -fuse-ld=lld"
${MAKE:-make} distclean "${@}" HOMEBREW_LIB= HOMEBREW_INC=
${MAKE:-make} "${LIBUVVER:?}" "${@}" HOMEBREW_LIB= HOMEBREW_INC=
${MAKE:-make} "${@}" HOMEBREW_LIB= HOMEBREW_INC=
