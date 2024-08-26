#!/usr/bin/env bash
# scspell-id: fc744980-63e8-11ef-ab99-80ee73e9b8e7
# shellcheck disable=SC2312

set -eu

# Compiler
test -z "${CC:-}" && CC="gcc"
export CC

# Test
unzip -v
"${CC:?}" --version
"${MAKE:?}" --version

# Setup
RUNS=5
printf '\n%s\n' "Setting up PGO build ..."
PROFILE_PATH="$(pwd -P)/.profile_data"
export PROFILE_PATH
test -d "${PROFILE_PATH}" && rm -rf "${PROFILE_PATH}"
mkdir -p "${PROFILE_PATH}"
export BASE_LDFLAGS="${LDFLAGS:-}"
export BASE_CFLAGS="-Dftello64=ftello -Doff64_t=off_t -Dfseeko64=fseeko -Dfopen64=fopen -fipa-pta -fweb -fprofile-partial-training -fprofile-correction -fprofile-dir=\"${PROFILE_PATH:?}\" ${CFLAGS:-}"
export LIBUVVER="libuvrel"
export STDERR="/dev/stderr"

# Base
printf '\n%s\n' "Generating baseline build ..."
export CFLAGS="${BASE_CFLAGS:?}"
export LDFLAGS="${BASE_LDFLAGS:-} ${CFLAGS:?}"
${MAKE:-make} distclean
${MAKE:-make} "${LIBUVVER:?}"
${MAKE:-make}
printf '\n%s\n' "Running baseline benchmark ..."
SMIPS=$(cd src/perf_test && for i in $(seq 1 "${RUNS}"); do printf '%s' "(${i:?}/${RUNS:?}) " >&2; ../dps8/dps8 -r ./nqueensx.ini | grep MIPS | tee "${STDERR}"; done | avg)

# Profile
printf '\n%s\n' "Generating profile build ..."
export CFLAGS="-fprofile-generate ${BASE_CFLAGS:?}"
export LDFLAGS="${BASE_LDFLAGS:-} ${CFLAGS:?}"
${MAKE:-make} distclean
${MAKE:-make} "${LIBUVVER:?}"
${MAKE:-make}
printf '\n%s\n' "Generating profile ..."
(cd src/perf_test && ../dps8/dps8 -r ./nqueensx.ini)
./src/novdso/novdso || true
./src/empty/empty || true
./src/prt2pdf/prt2pdf -h || true
./src/punutil/punutil -v < /dev/null || true
./src/mcmb/mcmb -X mul 7 11 37 41 43 47 53 59 61 67 71 73 79 83 89 97 || true

# Build
printf '\n%s\n' "Generating final build ..."
export CFLAGS="-fprofile-use -Wno-missing-profile ${BASE_CFLAGS:?}"
export LDFLAGS="${BASE_LDFLAGS:-} ${CFLAGS:?}"
${MAKE:-make} distclean
${MAKE:-make} "${LIBUVVER:?}"
${MAKE:-make}

# Final
printf '\n%s\n' "Running final benchmark ..."
EMIPS=$(cd src/perf_test && for i in $(seq 1 "${RUNS}"); do printf '%s' "(${i:?}/${RUNS:?}) " >&2; ../dps8/dps8 -r ./nqueensx.ini | grep MIPS | tee "${STDERR}"; done | avg)
printf '\nBefore : %s\nAfter  : %s\n' "${SMIPS:?}" "${EMIPS:?}"
# shellcheck disable=SC2046
printf 'Change : %s%%\n' $(printf '%s\n' "scale=6;((${EMIPS:?}-${SMIPS:?})/${EMIPS:?})*100" | bc -l | dd bs=1 count=6 2> /dev/null)