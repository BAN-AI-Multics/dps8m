#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: FSFAP
# scspell-id: 20185df2-f62c-11ec-80d0-80ee73e9b8e7

##############################################################################
#
# Copyright (c) 2021-2023 The DPS8M Development Team
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered "AS-IS",
# without any warranty.
#
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
          "12.7EXEC.tap"  \
  "12.7LDD_STANDARD.tap"  \
      "12.7LISTINGS.tap"  \
          "12.7MISC.tap"  \
       "12.7MULTICS.tap"  \
     "12.7UNBUNDLED.tap"; \
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

touch "12.7MULTICS.tap" 2> /dev/null || true
${MD5:-} "12.7MULTICS.tap" 2> /dev/null |
  grep -wq 746ff9fddc3ce837df5eec83020f58ba ||
    {
      rm -f "12.7MULTICS.tap"
      wget -c -t 0 \
        "${MRSITE:?}MR12.7/12.7MULTICS.tap"
    }

##############################################################################

touch "12.7EXEC.tap" 2> /dev/null || true
${MD5:-} "12.7EXEC.tap" 2> /dev/null |
  grep -wq 9faacd6f79cb6755a3e6c45415bdd168 ||
    {
      rm -f "12.7EXEC.tap"
      wget -c -t 0 \
        "${MRSITE:?}MR12.7/12.7EXEC.tap"
    }

##############################################################################

touch "12.7LDD_STANDARD.tap" 2> /dev/null || true
${MD5:-} "12.7LDD_STANDARD.tap" 2> /dev/null |
  grep -wq f1f5a0b9c61107ad368253ee7dd21b66 ||
    {
      rm -f "12.7LDD_STANDARD.tap"
      wget -c -t 0 \
        "${MRSITE:?}MR12.7/12.7LDD_STANDARD.tap"
    }

##############################################################################

touch "12.7UNBUNDLED.tap" 2> /dev/null || true
${MD5:-} "12.7UNBUNDLED.tap" 2> /dev/null |
  grep -wq 64384cd30f0b7acf60e17b318b9c459d ||
    {
      rm -f "12.7UNBUNDLED.tap"
      wget -c -t 0 \
        "${MRSITE:?}MR12.7/12.7UNBUNDLED.tap"
    }

##############################################################################

touch "12.7MISC.tap" 2> /dev/null || true
${MD5:-} "12.7MISC.tap" 2> /dev/null |
  grep -wq fc117d9dc58d1077271a9ceb0c93e020 ||
    {
      rm -f "12.7MISC.tap"
      wget -c -t 0 \
        "${MRSITE:?}MR12.7/12.7MISC.tap"
    }

##############################################################################

touch "12.7LISTINGS.tap" 2> /dev/null || true
${MD5:-} "12.7LISTINGS.tap" 2> /dev/null |
  grep -wq b64f6ca3670d4859c0fb42d9eae860e4 ||
    {
      rm -f "12.7LISTINGS.tap"
      wget -c -t 0 \
        "${MRSITE:?}MR12.7/12.7LISTINGS.tap"
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
