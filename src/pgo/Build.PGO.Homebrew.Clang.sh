#!/usr/bin/env bash
# scspell-id: 02f2e258-63e9-11ef-8480-80ee73e9b8e7
# SPDX-License-Identifier: MIT-0
# Copyright (c) 2021-2024 The DPS8M Development Team
# shellcheck disable=SC2312

set -eu

# Compiler
PATH="$(brew --prefix llvm)"/bin:"${PATH:-}"
export PATH
CC="$(brew --prefix llvm)"/bin/clang
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
export BASE_CFLAGS="-Dftello64=ftello -Doff64_t=off_t -Dfseeko64=fseeko -Dfopen64=fopen -fno-profile-sample-accurate ${CFLAGS:-}"
export LIBUVVER="libuvrel"
export LLVM_PROFILE_FILE="${PROFILE_PATH:?}/profile.%p.profraw"

# Base
printf '\n%s\n' "Generating baseline build ..."
export CFLAGS="${BASE_CFLAGS:?} -Wno-ignored-optimization-argument"
export LDFLAGS="${BASE_LDFLAGS:-} ${CFLAGS:?} -fuse-ld=lld"
${MAKE:-make} distclean "${@}" HOMEBREW_LIB= HOMEBREW_INC=
${MAKE:-make} "${LIBUVVER:?}" "${@}" HOMEBREW_LIB= HOMEBREW_INC=
${MAKE:-make} "${@}" HOMEBREW_LIB= HOMEBREW_INC=
printf '\n%s' "Running baseline benchmarks ... "
SMIPS=$(cd src/perf_test && for i in $(seq 1 "${RUNS}"); do printf '%s' "(${i:?}/${RUNS:?}) " >&2; ../dps8/dps8 -r ./nqueensx.ini | grep MIPS; done | tr -cd '\n.0123456789' | awk '{for (i=1;i<=NF;++i) {sum+=$i; ++n}} END {printf "%.4f\n", sum/n}')

# Profile
printf '\n%s\n' "Generating profile build ..."
export CFLAGS="-fprofile-generate=${PROFILE_PATH:?}/profile.%p.profraw ${BASE_CFLAGS:?} -Wno-ignored-optimization-argument"
export LDFLAGS="${BASE_LDFLAGS:-} ${CFLAGS:?} -fuse-ld=lld"
${MAKE:-make} distclean "${@}" HOMEBREW_LIB= HOMEBREW_INC=
${MAKE:-make} "${LIBUVVER:?}" "${@}" HOMEBREW_LIB= HOMEBREW_INC=
${MAKE:-make} "${@}" HOMEBREW_LIB= HOMEBREW_INC=
printf '\n%s\n' "Generating profile ..."
(cd src/perf_test && ../dps8/dps8 -r ./nqueensx.ini)
"$(brew --prefix llvm)/bin/llvm-profdata" merge --sparse=true --gen-partial-profile=true -output="${PROFILE_PATH:?}/final.profdata" "${PROFILE_PATH:?}"/profile.*.profraw

# Build
printf '\n%s\n' "Generating final build ..."
export CFLAGS="-fprofile-use=${PROFILE_PATH:?}/final.profdata ${BASE_CFLAGS:?} -Wno-ignored-optimization-argument"
export LDFLAGS="${BASE_LDFLAGS:-} ${CFLAGS:?} -fuse-ld=lld"
${MAKE:-make} distclean "${@}" HOMEBREW_LIB= HOMEBREW_INC=
${MAKE:-make} "${LIBUVVER:?}" "${@}" HOMEBREW_LIB= HOMEBREW_INC=
${MAKE:-make} "${@}" HOMEBREW_LIB= HOMEBREW_INC=

# Final
printf '\n%s' "Running final benchmarks ... "
EMIPS=$(cd src/perf_test && for i in $(seq 1 "${RUNS}"); do printf '%s' "(${i:?}/${RUNS:?}) " >&2; ../dps8/dps8 -r ./nqueensx.ini | grep MIPS; done | tr -cd '\n.0123456789' | awk '{for (i=1;i<=NF;++i) {sum+=$i; ++n}} END {printf "%.4f\n", sum/n}')
printf '\nBefore : %s\nAfter  : %s\n' "${SMIPS:?}" "${EMIPS:?}"
