#!/usr/bin/env sh
# shellcheck disable=SC2086,SC2140
# vim: filetype=sh:tabstop=4:tw=78:expandtab
# SPDX-License-Identifier: FSFAP
# scspell-id: cebcbf57-f631-11ec-9a6c-80ee73e9b8e7

############################################################################
#
# Copyright (c) 2021-2022 The DPS8M Development Team
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered "AS-IS",
# without any warranty.
#
############################################################################

set -u

############################################################################

export SHELL="/bin/sh"

############################################################################

test -f "./builddoc-docker.sh" ||
  {
    printf '%s\n' "Error: Not running in docs directory?"
    exit 1
  }

############################################################################

test -f "Dockerfile.in" ||
  {
    printf '%s\n' "Error: No Dockerfile.in in current directory."
    exit 1
  }

############################################################################

# shellcheck disable=SC2155
export PDIR="$( ( cd .. && pwd -P ) )"

test -d "${PDIR:?}" ||
  {
    printf '%s\n' "Error: Bad directory ${PDIR:-}"
    exit 1
  }

############################################################################

printf '%s' "Checking Docker: "

${DOCKER:-docker} version > "/dev/null" 2>&1 ||
  {
    printf '%s\n' "ERROR"
    exit 1
  }

${DOCKER:-docker} run --rm -it "hello-world:latest" > "/dev/null" 2>&1 ||
  {
    printf '%\n' "ERROR"
    exit 1
  }

printf '%s\n' "OK"

############################################################################

export DOCBUILD="sh -c \"git config --global --add safe.directory /out &&  \
                   cd /out/docs && gmake -C .. superclean &&               \
                     gmake -C .. && ./builddoc.sh\""

############################################################################

set -e

############################################################################

eval ${DOCKER:-docker} run --rm -v "${PDIR:?}":"/out" -it                  \
  "dps8m-docs-helper:Dockerfile" "${DOCBUILD:?}" ||
    {
      printf '%s\n' "Building dps8m-docs-helper container ..." &&
        ${SED:-sed} "s/##RELEASEVER##/${RELEASEVER:-36}/" Dockerfile.in |  \
          ${DOCKER:-docker} build -t "dps8m-docs-helper:Dockerfile" "-"    \
            && eval ${DOCKER:-docker} run --rm -v "${PDIR:?}":"/out" -it   \
              "dps8m-docs-helper:Dockerfile" "${DOCBUILD:?}"
    }

############################################################################
