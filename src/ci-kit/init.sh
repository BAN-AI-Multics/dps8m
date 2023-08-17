#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: MIT
# scspell-id: 20185df2-f62c-11ec-80d0-80ee73e9b8e7
# Copyright (c) 2021-2023 The DPS8M Development Team

##############################################################################

set -e > /dev/null 2>&1

##############################################################################

printf '#### %s\n' "Begin ${0} (${$})"

##############################################################################

export SHELL=/bin/sh

##############################################################################

mkdir -vp tapes 2> /dev/null

##############################################################################

cd tapes ||
  {
    printf '%s\n' "*** Error: unable to enter tapes directory."
    exit 1
  }

##############################################################################

command -v telnet > /dev/null 2>&1 ||
  {
    printf '%s\n' "*** Error: No telnet in PATH."
    exit 1
  }

##############################################################################

command -v dos2unix > /dev/null 2>&1 ||
  {
    printf '%s\n' "*** Error: No dos2unix in PATH."
    exit 1
  }

##############################################################################

command -v expect > /dev/null 2>&1 ||
  {
    printf '%s\n' "*** Error: No expect in PATH."
    exit 1
  }

##############################################################################

command -v wget > /dev/null 2>&1 ||
  {
    printf '%s\n' "*** Error: No wget in PATH."
    exit 1
  }
WGET="wget -c -t 0" && export WGET

##############################################################################

# shellcheck disable=SC2015
command -v aria2c > /dev/null 2>&1 &&
  {
    WGET="aria2c -s 5 -j 5 -x 5 -k 8M -m 10" && export WGET
  } || true

##############################################################################

command -v lzip > /dev/null 2>&1 ||
  {
    printf '%s\n' "*** Error: No lzip in PATH."
    exit 1
  }

##############################################################################

command -v tmux > /dev/null 2>&1 ||
  {
    printf '%s\n' "*** Error: No tmux in PATH."
    exit 1
  }

##############################################################################

MD5=$(command -v md5sum 2> /dev/null) ||
  MD5=$(command -v md5  2> /dev/null)
test -z "${MD5:-}" &&
  {
    printf '%s\n' "*** Error: No \"${MD5:-md5}\" in PATH."
    exit 1
  }

##############################################################################

printf '%s\n' "*** Preparing tape files..."

##############################################################################

MRSITE="https://s3.amazonaws.com/eswenson-multics/public/releases/" &&
  export MRSITE > /dev/null 2>&1

##############################################################################

# Link tape files, copy if linking fails
for tape in               \
          "12.8EXEC.tap"  \
  "12.8LDD_STANDARD.tap"  \
      "12.8LISTINGS.tap"  \
          "12.8MISC.tap"  \
       "12.8MULTICS.tap"  \
     "12.8UNBUNDLED.tap"; \
    do
      test -z "${NO_CI_SYMLINK_TAPES:-}" &&
        {
          test -f "${tape:?}" ||
            {
              ln -s "/var/cache/tapes/${tape:?}" . \
                > /dev/null 2>&1 || true
            }
        }
      test -f "${tape:?}" ||
        cp -f "/var/cache/tapes/${tape:?}" . \
          > /dev/null 2>&1 || true
done

##############################################################################

touch "12.8MULTICS.tap" 2> /dev/null || true
${MD5:-} "12.8MULTICS.tap" 2> /dev/null |
  grep -wq 18f64bf8f1906e4eb32d35f2b747f0b5 ||
    {
      rm -f "12.8MULTICS.tap"
      ${WGET:?} \
        "${MRSITE:?}MR12.8/12.8MULTICS.tap"
    }

##############################################################################

touch "12.8EXEC.tap" 2> /dev/null || true
${MD5:-} "12.8EXEC.tap" 2> /dev/null |
  grep -wq 7adfa5f9cc0a18c572e83b6a9d58d627 ||
    {
      rm -f "12.8EXEC.tap"
      ${WGET:?} \
        "${MRSITE:?}MR12.8/12.8EXEC.tap"
    }

##############################################################################

touch "12.8LDD_STANDARD.tap" 2> /dev/null || true
${MD5:-} "12.8LDD_STANDARD.tap" 2> /dev/null |
  grep -wq af1041aa3e547f5789ad749c4484bcf2 ||
    {
      rm -f "12.8LDD_STANDARD.tap"
      ${WGET:?} \
        "${MRSITE:?}MR12.8/12.8LDD_STANDARD.tap"
    }

##############################################################################

touch "12.8UNBUNDLED.tap" 2> /dev/null || true
${MD5:-} "12.8UNBUNDLED.tap" 2> /dev/null |
  grep -wq 007aaa69b196df67196816e5c1186771 ||
    {
      rm -f "12.8UNBUNDLED.tap"
      ${WGET:?} \
        "${MRSITE:?}MR12.8/12.8UNBUNDLED.tap"
    }

##############################################################################

touch "12.8MISC.tap" 2> /dev/null || true
${MD5:-} "12.8MISC.tap" 2> /dev/null |
  grep -wq 92983e9616afe9fd6783b7ee232ccac0 ||
    {
      rm -f "12.8MISC.tap"
      ${WGET:?} \
        "${MRSITE:?}MR12.8/12.8MISC.tap"
    }

##############################################################################

touch "12.8LISTINGS.tap" 2> /dev/null || true
${MD5:-} "12.8LISTINGS.tap" 2> /dev/null |
  grep -wq 3f62e1a67854cbde1c40576d073a4499 ||
    {
      rm -f "12.8LISTINGS.tap"
      ${WGET:?} \
        "${MRSITE:?}MR12.8/12.8LISTINGS.tap"
    }

##############################################################################

touch "12.8AML.tap" 2> /dev/null || true
${MD5:-} "12.8AML.tap" 2> /dev/null |
  grep -wq 36e2fa1923e50670fba6ba2f784ca74e ||
    {
      rm -f "12.8AML.tap"
      ${WGET:?} \
        "${MRSITE:?}MR12.8/12.8AML.tap"
    }

##############################################################################

# Stash tapes 12*.tap in /var/cache/tapes
test -z "${STASH_CI_TAPES:-}" ||
  {
    mkdir -p "/var/cache/tapes" > /dev/null 2>&1 ||
      true
    # shellcheck disable=SC2015,SC2065
    test -d "/var/cache/tapes" > /dev/null 2>&1 &&
      cp -f 12*.tap "/var/cache/tapes" > /dev/null 2>&1 ||
        true
  }

##############################################################################

lzip -t "foo.tap.lz" &&
  lzip -dc "foo.tap.lz" \
    > "foo.tap"

##############################################################################

cd ..

##############################################################################

printf '#### %s\n' "End ${0} (${$})"

##############################################################################
