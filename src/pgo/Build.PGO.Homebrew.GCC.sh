#!/usr/bin/env bash
# scspell-id: 03710116-6767-11ef-a8a4-80ee73e9b8e7
# SPDX-License-Identifier: MIT-0
# Copyright (c) 2021-2024 The DPS8M Development Team
# shellcheck disable=SC2312

set -eu

# Sanity test
printf '%s\n' "Checking for PGO script ..."
test -x "./src/pgo/Build.PGO.Homebrew.GCC.sh" \
  || { printf '%s\n' "ERROR: Unable to find PGO script!"; exit 1; }

# Homebrew
brew info gcc 2> /dev/null | grep -q '^Installed' \
  || { printf '%s\n' "ERROR: Homebrew GCC not installed."; exit 1; }

# Compiler
PATH="$(brew --prefix gcc)"/bin:"${PATH:-}"
export PATH

CC_PREFIX="$(brew --prefix gcc)"
CC="$(ls -1 ${CC_PREFIX:?}/bin/gcc-[0-9][0-9] | head -1)"
export CC

# Test
"${CC:?}" --version

# Setup
RUNS=3
printf '\n%s\n' "Setting up PGO build ..."
PROFILE_PATH="$(pwd -P)/.profile_data"
export PROFILE_PATH
test -d "${PROFILE_PATH}" && rm -rf "${PROFILE_PATH}"
mkdir -p "${PROFILE_PATH}"
export BASE_LDFLAGS="${LDFLAGS:-}"
export BASE_CFLAGS="-Dftello64=ftello -Doff64_t=off_t -Dfseeko64=fseeko \
  -Dfopen64=fopen -fipa-pta -fweb -fprofile-partial-training \
  -fprofile-correction -fprofile-dir=\"${PROFILE_PATH:?}\" ${CFLAGS:-}"
export LIBUVVER="libuvrel"

# Base
printf '\n%s\n' "Generating baseline build ..."
export CFLAGS="${BASE_CFLAGS:?}"
export LDFLAGS="${BASE_LDFLAGS:-} ${CFLAGS:?}"
${MAKE:-make} distclean "${@}" HOMEBREW_LIB= HOMEBREW_INC=
${MAKE:-make} "${LIBUVVER:?}" "${@}" HOMEBREW_LIB= HOMEBREW_INC=
${MAKE:-make} "${@}" HOMEBREW_LIB= HOMEBREW_INC=
printf '\n%s' "Running baseline benchmarks ... "
SMIPS=$(cd src/perf_test && for i in $(seq 1 "${RUNS}"); do
  printf '%s' "(${i:?}/${RUNS:?}) " >&2
  ../dps8/dps8 -r ./nqueensx.ini | grep MIPS
done | tr -cd '\n.0123456789' \
  | awk '{for (i=1;i<=NF;++i) {sum+=$i; ++n}} END {printf "%.4f\n", sum/n}')

# Profile
printf '\n%s\n' "Generating profile build ..."
export CFLAGS="-fprofile-generate ${BASE_CFLAGS:?}"
export LDFLAGS="${BASE_LDFLAGS:-} ${CFLAGS:?}"
${MAKE:-make} distclean "${@}" HOMEBREW_LIB= HOMEBREW_INC=
${MAKE:-make} "${LIBUVVER:?}" "${@}" HOMEBREW_LIB= HOMEBREW_INC=
${MAKE:-make} "${@}" HOMEBREW_LIB= HOMEBREW_INC=
printf '\n%s\n' "Generating profile ..."
(cd src/perf_test && ../dps8/dps8 -r ./nqueensx.ini)
./src/empty/empty || true
./src/prt2pdf/prt2pdf -h || true
./src/punutil/punutil -v < /dev/null || true
./src/mcmb/mcmb -X mul 7 11 37 41 43 47 53 59 61 67 71 73 79 83 89 97 || true

# Build
printf '\n%s\n' "Generating final build ..."
export CFLAGS="-fprofile-use -Wno-missing-profile ${BASE_CFLAGS:?}"
export LDFLAGS="${BASE_LDFLAGS:-} ${CFLAGS:?}"
${MAKE:-make} distclean "${@}" HOMEBREW_LIB= HOMEBREW_INC=
${MAKE:-make} "${LIBUVVER:?}" "${@}" HOMEBREW_LIB= HOMEBREW_INC=
${MAKE:-make} "${@}" HOMEBREW_LIB= HOMEBREW_INC=

# Final
printf '\n%s' "Running final benchmarks ... "
EMIPS=$(cd src/perf_test && for i in $(seq 1 "${RUNS}"); do
  printf '%s' "(${i:?}/${RUNS:?}) " >&2
  ../dps8/dps8 -r ./nqueensx.ini | grep MIPS
done | tr -cd '\n.0123456789' \
  | awk '{for (i=1;i<=NF;++i) {sum+=$i; ++n}} END {printf "%.4f\n", sum/n}')
printf '\nBefore : %s\nAfter  : %s\n' "${SMIPS:?}" "${EMIPS:?}"
