#!/usr/bin/env bash
# scspell-id: f585612c-63e8-11ef-9fc2-80ee73e9b8e7
# SPDX-License-Identifier: MIT-0
# Copyright (c) 2021-2025 The DPS8M Development Team
# shellcheck disable=SC2312

# Sanity test
printf '%s\n' "Checking for PGO script ..."
test -x "./src/pgo/Build.PGO.Clang.sh" \
  || {
    printf '%s\n' "ERROR: Unable to find PGO script!"
    exit 1
  }

# TOOLSUFFIX
test -z "${TOOLSUFFIX:-}" \
  || printf '\nTOOLSUFFIX: "%s"\n' "${TOOLSUFFIX:?}"

# CC
test -z "${CC:-}" && CC="clang${TOOLSUFFIX:-}"
export CC

# Ubuntu?
test -z "${TOOLSUFFIX:-}" && {
  grep -q 'Ubuntu' /etc/*elease > /dev/null 2>&1 && {
    RSUFFIX="$("${CC:?}" --version 2> /dev/null \
      | grep -o '[1-9]\+\.[0-9]\+\.[0-9]\+' 2> /dev/null \
      | head -1 2> /dev/null \
      | cut -d'.' -f1 2> /dev/null)"
    test -z "${RSUFFIX:-}" || {
      printf '%s ' "*** Ubuntu detected:"
      printf '%s\n' "You may need to set TOOLSUFFIX=\"-${RSUFFIX:?}\""
      printf '    %s\n' "i.e. 'env TOOLSUFFIX=-${RSUFFIX:?} ${0:?}'"
      sleep 3
    }
  }
}

set -e

# Test CC
printf '\nCC: %s\n' "${CC:?}"
${CC:?} --version

# LLD
test -z "${LLD:-}" && LLD="ld.lld${TOOLSUFFIX:-}"
export LLD

# Test LLD
printf '\nLLD: %s\n' "${LLD:?}"
${LLD:?} --version

# AR
test -z "${AR:-}" && AR="llvm-ar${TOOLSUFFIX:-}"
export AR

# Test AR
printf '\nAR: %s\n' "${AR:?}"
${AR:?} --version

# RANLIB
test -z "${RANLIB:-}" && RANLIB="llvm-ranlib${TOOLSUFFIX:-}"
export RANLIB

# Test RANLIB
printf '\nRANLIB: %s\n' "${RANLIB:?}"
${RANLIB:?} --version

# PROFDATA
test -z "${PROFDATA:-}" && PROFDATA="llvm-profdata${TOOLSUFFIX:-}"
export PROFDATA

# Test PROFDATA
printf '\nPROFDATA: %s\n' "${PROFDATA:?}"
command -v "${PROFDATA:?}" \
  || { printf '%s\n' "${PROFDATA:?} not found!"; exit 1; }

# LIBUVVER
test -z "${LIBUVVER:-}" && LIBUVVER="libuvrel"
export LIBUVVER

# Setup
printf '\n%s\n' "Setting up PGO build ..."
PROFILE_PATH="$(pwd -P)/.profile_data"
export PROFILE_PATH
test -d "${PROFILE_PATH}" && rm -rf "${PROFILE_PATH}"
mkdir -p "${PROFILE_PATH}"
# shellcheck disable=SC2155
export BASE_LDFLAGS="${LDFLAGS:-} -fuse-ld=$(command -v "${LLD:?}")"
export BASE_CFLAGS="-fno-profile-sample-accurate ${CFLAGS:-}"
export LLVM_PROFILE_FILE="${PROFILE_PATH:?}/profile.%p.profraw"

# Profile
printf '\n%s\n' "Generating profile build ..."
export CFLAGS="-fprofile-generate=${PROFILE_PATH:?}/profile.%p.profraw \
  ${BASE_CFLAGS:?}"
export LDFLAGS="${BASE_LDFLAGS:-} ${CFLAGS:?}"
${MAKE:-make} distclean "${@}"
test -z "${NO_PGO_LIBUV:-}" && ${MAKE:-make} "${LIBUVVER:?}" "${@}" || true
${MAKE:-make} "${@}"
printf '\n%s\n' "Generating profile ..."
(cd src/perf_test && ../dps8/dps8 -r ./nqueensx.ini)
"${PROFDATA:?}" merge --sparse=true \
  --gen-partial-profile=true -output="${PROFILE_PATH:?}/final.profdata" \
  "${PROFILE_PATH:?}"/profile.*.profraw

# Build
printf '\n%s\n' "Generating final build ..."
export CFLAGS="-fprofile-use=${PROFILE_PATH:?}/final.profdata \
  ${BASE_CFLAGS:?}"
export LDFLAGS="${BASE_LDFLAGS:-} ${CFLAGS:?}"
${MAKE:-make} distclean "${@}"
test -z "${NO_PGO_LIBUV:-}" && ${MAKE:-make} "${LIBUVVER:?}" "${@}" || true
${MAKE:-make} "${@}"
