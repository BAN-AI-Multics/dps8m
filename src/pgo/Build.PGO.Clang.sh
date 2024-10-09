#!/usr/bin/env bash
# scspell-id: f585612c-63e8-11ef-9fc2-80ee73e9b8e7
# SPDX-License-Identifier: MIT-0
# Copyright (c) 2021-2024 The DPS8M Development Team
# shellcheck disable=SC2312

set -eu

# Sanity test
printf '%s\n' "Checking for PGO script ..."
test -x "./src/pgo/Build.PGO.Clang.sh" \
  || { printf '%s\n' "ERROR: Unable to find PGO script!"; exit 1; }

# Show TOOLSUFFIX
test -z "${TOOLSUFFIX:-}" \
  || printf 'TOOLSUFFIX: "%s"\n' "${TOOLSUFFIX:?}"

# Compiler
test -z "${CC:-}" && CC="clang${TOOLSUFFIX:-}"
export CC

# Test CC
${CC:?} --version

# LLD
LLD="$(command -v "ld.lld${TOOLSUFFIX:-}")"
export LLD

# Test LLD
${LLD:?} --version

# AR (TOOLSUFFIX)
test -z "${TOOLSUFFIX:-}" \
  || { AR="llvm-ar${TOOLSUFFIX:-}"; export AR; }
test -z "${AR:-}" || printf 'AR: %s\n' "${AR:-}"

# Test AR
test -z "${AR:-}" || ${AR:?} --version

# RANLIB (TOOLSUFFIX)
test -z "${TOOLSUFFIX:-}" \
  || { RANLIB="llvm-ranlib${TOOLSUFFIX:-}"; export RANLIB; }
test -z "${RANLIB:-}" || printf 'RANLIB: %s\n' "${RANLIB:-}"

# Test RANLIB
test -z "${RANLIB:-}" || ${RANLIB:?} --version

# Setup
RUNS=3
printf '\n%s\n' "Setting up PGO build ..."
PROFILE_PATH="$(pwd -P)/.profile_data"
export PROFILE_PATH
test -d "${PROFILE_PATH}" && rm -rf "${PROFILE_PATH}"
mkdir -p "${PROFILE_PATH}"
export BASE_LDFLAGS="${LDFLAGS:-} -fuse-ld=${LLD:?}"
export BASE_CFLAGS="-Dftello64=ftello -Doff64_t=off_t -Dfseeko64=fseeko \
  -Dfopen64=fopen -fno-profile-sample-accurate ${CFLAGS:-}"
export LIBUVVER="libuvrel"
export LLVM_PROFILE_FILE="${PROFILE_PATH:?}/profile.%p.profraw"

# Base
printf '\n%s\n' "Generating baseline build ..."
export CFLAGS="${BASE_CFLAGS:?}"
export LDFLAGS="${BASE_LDFLAGS:-} ${CFLAGS:?}"
${MAKE:-make} distclean "${@}"
${MAKE:-make} "${LIBUVVER:?}" "${@}"
${MAKE:-make} "${@}"
printf '\n%s' "Running baseline benchmarks ... "
SMIPS=$(cd src/perf_test && for i in $(seq 1 "${RUNS}"); do
  printf '%s' "(${i:?}/${RUNS:?}) " >&2
  ../dps8/dps8 -r ./nqueensx.ini | grep MIPS
done | tr -cd '\n.0123456789' \
  | awk '{for (i=1;i<=NF;++i) {sum+=$i; ++n}} END {printf "%.4f\n", sum/n}')

# Profile
printf '\n%s\n' "Generating profile build ..."
export CFLAGS="-fprofile-generate=${PROFILE_PATH:?}/profile.%p.profraw \
  ${BASE_CFLAGS:?}"
export LDFLAGS="${BASE_LDFLAGS:-} ${CFLAGS:?}"
${MAKE:-make} distclean "${@}"
${MAKE:-make} "${LIBUVVER:?}" "${@}"
${MAKE:-make} "${@}"
printf '\n%s\n' "Generating profile ..."
(cd src/perf_test && ../dps8/dps8 -r ./nqueensx.ini)
"llvm-profdata${TOOLSUFFIX:-}" merge --sparse=true \
  --gen-partial-profile=true -output="${PROFILE_PATH:?}/final.profdata" \
  "${PROFILE_PATH:?}"/profile.*.profraw

# Build
printf '\n%s\n' "Generating final build ..."
export CFLAGS="-fprofile-use=${PROFILE_PATH:?}/final.profdata \
  ${BASE_CFLAGS:?}"
export LDFLAGS="${BASE_LDFLAGS:-} ${CFLAGS:?}"
${MAKE:-make} distclean "${@}"
${MAKE:-make} "${LIBUVVER:?}" "${@}"
${MAKE:-make} "${@}"

# Final
printf '\n%s' "Running final benchmarks ... "
EMIPS=$(cd src/perf_test && for i in $(seq 1 "${RUNS}"); do
  printf '%s' "(${i:?}/${RUNS:?}) " >&2
  ../dps8/dps8 -r ./nqueensx.ini | grep MIPS
done | tr -cd '\n.0123456789' \
  | awk '{for (i=1;i<=NF;++i) {sum+=$i; ++n}} END {printf "%.4f\n", sum/n}')
printf '\nBefore : %s\nAfter  : %s\n' "${SMIPS:?}" "${EMIPS:?}"
