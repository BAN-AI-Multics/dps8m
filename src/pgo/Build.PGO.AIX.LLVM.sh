#!/usr/bin/env bash
# scspell-id: 0a925678-8811-11ef-8000-922037bebb33
# SPDX-License-Identifier: MIT-0
# Copyright (c) 2021-2025 The DPS8M Development Team
# shellcheck disable=SC2312

# PGO for LLVM on AIX: Mainline Clang & IBM Open XL C/C++ V17

# Example: IBM Open XL C/C++ 17.1.3:
#          env CFLAGS="-DHAVE_POPT=1 -D_ALL_SOURCE -mcpu=power8"     \
#          PULIBS="-lpopt" ATOMICS="AIX" AWK="gawk" OBJECT_MODE="64" \
#          PATH="/opt/freeware/bin:${PATH:?}"                        \
#          TOOLPREFIX="/opt/IBM/openxlC/17.1.3/bin/ibm-"             \
#          CC="/opt/IBM/openxlC/17.1.3/bin/ibm-clang_r"              \
#          ./src/pgo/Build.PGO.AIX.LLVM.sh

# Example: IBM Open XL C/C++ 17.1.2:
#          env CFLAGS="-DHAVE_POPT=1 -D_ALL_SOURCE -mcpu=power8"     \
#          PULIBS="-lpopt" ATOMICS="AIX" AWK="gawk" OBJECT_MODE="64" \
#          PATH="/opt/freeware/bin:${PATH:?}"                        \
#          TOOLPREFIX="/opt/IBM/openxlC/17.1.2/bin/ibm-"             \
#          CC="/opt/IBM/openxlC/17.1.2/bin/ibm-clang_r"              \
#          ./src/pgo/Build.PGO.AIX.LLVM.sh

# Example: IBM Open XL C/C++ 17.1.1:
#          env CFLAGS="-DHAVE_POPT=1 -D_ALL_SOURCE -mcpu=power8"     \
#          PULIBS="-lpopt" ATOMICS="AIX" AWK="gawk" OBJECT_MODE="64" \
#          PATH="/opt/freeware/bin:${PATH:?}"                        \
#          TOOLPREFIX="/opt/IBM/openxlC/17.1.1/bin/ibm-"             \
#          CC="/opt/IBM/openxlC/17.1.1/bin/ibm-clang_r"              \
#          PROFDATA_TEST="-h" NEED_128=1                             \
#          ./src/pgo/Build.PGO.AIX.LLVM.sh

# Example: Mainline LLVM Clang 19:
#          env CFLAGS="-DHAVE_POPT=1 -D_ALL_SOURCE -mcpu=power8"     \
#          PULIBS="-lpopt" ATOMICS="AIX" AWK="gawk" OBJECT_MODE="64" \
#          PATH="/opt/freeware/bin:/opt/llvm/bin:${PATH:?}"          \
#          CC="/opt/llvm/bin/clang"                                  \
#          ./src/pgo/Build.PGO.AIX.LLVM.sh

set -e

# Sanity test
printf '%s\n' "Checking for PGO script ..."
test -x "./src/pgo/Build.PGO.AIX.LLVM.sh" \
  || {
    printf '%s\n' "ERROR: Unable to find PGO script!"
    exit 1
  }

# TOOLPREFIX
test -z "${TOOLPREFIX:-}" \
  || printf '\nTOOLPREFIX: "%s"\n' "${TOOLPREFIX:?}"

# TOOLSUFFIX
test -z "${TOOLSUFFIX:-}" \
  || printf '\nTOOLSUFFIX: "%s"\n' "${TOOLSUFFIX:?}"

# CC
test -z "${CC:-}" && CC="${TOOLPREFIX:-}clang${TOOLSUFFIX:-}"
export CC

# Test CC
printf '\nCC: %s\n' "${CC:?}"
${CC:?} --version

# PROFDATA
test -z "${PROFDATA:-}" \
  && PROFDATA="${TOOLPREFIX:-}llvm-profdata${TOOLSUFFIX:-}"
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
export BASE_LDFLAGS="${LDFLAGS:-}"
export BASE_CFLAGS="-fno-profile-sample-accurate ${CFLAGS:-}"
export LLVM_PROFILE_FILE="${PROFILE_PATH:?}/profile.%p.profraw"

# Profile
printf '\n%s\n' "Generating profile build ..."
export CFLAGS="-fprofile-generate=${PROFILE_PATH:?}/profile.%p.profraw \
  ${BASE_CFLAGS:?}"
export LDFLAGS="${BASE_LDFLAGS:-} ${CFLAGS:?}"
${MAKE:-gmake} distclean "${@}"
test -z "${NO_PGO_LIBUV:-}" && ${MAKE:-gmake} "${LIBUVVER:?}" "${@}" || true
${MAKE:-gmake} "${@}"
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
${MAKE:-gmake} distclean "${@}"
test -z "${NO_PGO_LIBUV:-}" && ${MAKE:-gmake} "${LIBUVVER:?}" "${@}" || true
${MAKE:-gmake} "${@}"
