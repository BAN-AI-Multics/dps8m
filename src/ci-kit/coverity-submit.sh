#!/usr/bin/env sh
# vim: filetype=sh:tabstop=2:ai:expandtab
# SPDX-License-Identifier: MIT-0
# scspell-id: 3ca5a01e-a3f1-11ed-9267-80ee73e9b8e7
# Copyright (c) 2023-2025 The DPS8M Development Team

##############################################################################

test -d "./.git" ||
  {
    printf '%s\n' \
      '#### Error: Not in top-level git repository.'
    exit 1
  }

##############################################################################

test -x "/bin/sh" ||
  {
    printf '%s\n' \
      '#### Error: No "/bin/sh" executable.'
    exit 1
  }

##############################################################################

tar --version | grep -q "GNU" ||
  {
    printf '%s\n' \
      '#### Error: GNU tar is required.'
    exit 1
  }

##############################################################################

test -z "${COVERITY_TOKEN:-}" &&
  {
    printf '%s\n' \
      '#### Error: COVERITY_TOKEN is unset.'
    exit 1
  }

##############################################################################

test -z "${COVERITY_EMAIL:-}" &&
  {
    printf '%s\n' \
      '#### Error: COVERITY_EMAIL is unset.'
    exit 1
  }

##############################################################################

test -z "${COVERITY_PROJECT:-}" &&
  {
    printf '%s\n' \
      '#### Error: COVERITY_PROJECT is unset.'
    exit 1
  }

##############################################################################

test -z "${COVERITY_DLDIR:-}" &&
  {
    printf '%s\n' \
      '#### Warning: COVERITY_DLDIR is unset, using defaults.'
    COVERITY_DLDIR="${HOME:?}/Downloads/Coverity"
    export COVERITY_DLDIR
  }

##############################################################################

mkdir -p "${COVERITY_DLDIR:?}" ||
  {
    printf '%s\n' \
      "#### Error: Bad ${COVERITY_DLDIR:?} directory."
    exit 1
  }

##############################################################################

wget --version > /dev/null ||
  {
    printf '%s\n' \
      '#### Error: No usable wget in PATH.'
    exit 1
  }

##############################################################################

curl --version > /dev/null ||
  {
    printf '%s\n' \
      '#### Error: No usable curl in PATH.'
    exit 1
  }

##############################################################################

# shellcheck disable=SC2312
test "$(uname -s 2> /dev/null)" = "Linux" ||
  {
    printf '%s\n' \
      '#### Error: Not running on Linux.'
    exit 1
  }

##############################################################################

uname -mp 2> /dev/null | grep -q -E '(amd64|x86_64)' 2> /dev/null ||
  {
    printf '%s\n' \
      '#### Error: Not running on x86_64 platform.'
    exit 1
  }

##############################################################################

printf '%s\n\n' "Starting in 5s ... "
sleep 5 > /dev/null 2>&1 || true

##############################################################################

printf '#### %s\n' "Begin ${0} (${$})"

##############################################################################

export SHELL=/bin/sh

##############################################################################

set -e > /dev/null 2>&1

##############################################################################

rm -rf ./cov-int            > /dev/null 2>&1
rm -rf ./.coverity          > /dev/null 2>&1
rm -f  ./dps8m-simulator.xz > /dev/null 2>&1

##############################################################################

printf '%s\n' '#### Notice: Downloading latest Coverity Tools checksum.'

wget --quiet                                                           \
      "https://scan.coverity.com/download/linux64"                     \
     --post-data                                                       \
       "token=${COVERITY_TOKEN:?}&project=${COVERITY_PROJECT:?}&md5=1" \
     -O "${COVERITY_DLDIR:?}/coverity_tool.md5" || true

touch -f "${COVERITY_DLDIR:?}/coverity_tool.md5"
touch -f "${COVERITY_DLDIR:?}/coverity_tool.last.md5"

##############################################################################

cmp -s "${COVERITY_DLDIR:?}/coverity_tool.last.md5" \
       "${COVERITY_DLDIR:?}/coverity_tool.md5" ||
  {
    printf '%s\n' \
      '#### Notice: Update needed due to checksum change.'
    COVERITY_NEED_UPDATE=1
    export COVERITY_NEED_UPDATE=1
  }

##############################################################################

# shellcheck disable=SC2312
test "$(2> /dev/null md5sum "${COVERITY_DLDIR:?}/coverity_tool.tgz" | \
        2> /dev/null cut -d ' ' -f 1)" =                              \
          "$(cat "${COVERITY_DLDIR:?}/coverity_tool.md5")" ||
  {
    test -z "${COVERITY_NEED_UPDATE:-}" && printf '%s\n' \
      '#### Notice: Update needed due to failed verification.'
    COVERITY_NEED_UPDATE=1
    export COVERITY_NEED_UPDATE=1
  }

##############################################################################

test -z "${COVERITY_NEED_UPDATE:-}" ||
  {
    printf '%s\n' '#### Notice: Downloading latest Coverity Tools archive.'
    wget --quiet                                                    \
          "https://scan.coverity.com/download/linux64"              \
         --post-data                                                \
          "token=${COVERITY_TOKEN:?}&project=${COVERITY_PROJECT:?}" \
         -O "${COVERITY_DLDIR:?}/coverity_tool.tgz" || true
  }

##############################################################################

cp -f "${COVERITY_DLDIR:?}/coverity_tool.md5" \
      "${COVERITY_DLDIR:?}/coverity_tool.last.md5" > /dev/null 2>&1 || true

##############################################################################

printf '%s' '#### Notice: Extracting Coverity Tools archive .'
mkdir -p ".coverity" &&                                          \
tar zxf "${COVERITY_DLDIR:?}/coverity_tool.tgz"                  \
    --strip-components=1 -C ".coverity" --checkpoint=5000 2>&1 | \
        awk '{ printf("."); }' && printf '%s\n' " OK."

##############################################################################

printf '%s\n' '#### Notice: Building DPS8M for Coverity analysis.'
env TZ=UTC PATH="./.coverity/bin:${PATH:?}"   \
    ./.coverity/bin/cov-build --dir "cov-int" \
      make all blinkenLights2

##############################################################################

printf '%s\n' '#### Notice: Packaging Coverity submission.'
tar caf dps8m-simulator.xz cov-int

##############################################################################

printf '%s\n' '#### Notice: Uploading Coverity submission.'
# shellcheck disable=SC2312
curl --form token="${COVERITY_TOKEN:?}"          \
     --form email="${COVERITY_EMAIL:?}"          \
     --form file=@"dps8m-simulator.xz"           \
     --form version="$(date -u "+R9-%s")"        \
     --form description="$(date -u "+DPS8M-%s")" \
  "https://scan.coverity.com/builds?project=${COVERITY_PROJECT:?}"

##############################################################################

printf '%s\n' '#### Notice: Coverity submission complete.'
rm -f dps8m-simulator.xz

##############################################################################

printf '#### %s\n' "End ${0} (${$})"

##############################################################################
