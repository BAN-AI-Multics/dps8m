#!/usr/bin/env sh
# shellcheck disable=SC2015,SC2086,SC2140
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: FSFAP
# scspell-id: cebcbf57-f631-11ec-9a6c-80ee73e9b8e7

##############################################################################
#
# Copyright (c) 2021-2022 The DPS8M Development Team
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered "AS-IS",
# without any warranty.
#
##############################################################################

# Wait for lock
test -z "${FLOCKER:-}" && printf '%s\n' "Waiting for lock ..." ;             \
test "$(uname -s 2> /dev/null)" = "Linux" && {                               \
FLOCK_COMMAND="$( command -v flock 2> /dev/null )" && {                      \
[ "${FLOCKER:-}" != "${0}" ] && exec env FLOCKER="${0}" "${FLOCK_COMMAND:?}" \
  -e --verbose "${0}" "${0}" "${@}" || :; } ; } ;

##############################################################################
# Configuration

# NOTE: Setting the environment variables BUILDDOC_SKIP_REBUILD or
#       BUILDDOC_FORCE_REBUILD will override the image age checks.

# Fedora version to use
DEFAULT_RELEASE="37"

# Make command to use
DB_GMAKE="gmake"

# Maximum age of image
OLD_DAYS="60"

##############################################################################

unset FLOCKER       > "/dev/null" 2>&1 || true
unset FLOCK_COMMAND > "/dev/null" 2>&1 || true

##############################################################################

test -z "${VERBOSE:-}" ||                                                    \
    printf 'DEBUG: %s\n' "Start of script."

##############################################################################

test -n "${BUILDDOC_FORCE_REBUILD:-}" &&                                     \
  test -n "${BUILDDOC_SKIP_REBUILD:-}" &&                                    \
    { printf '%s\n'                                                          \
       "Error: Both BUILDDOC_SKIP_REBUILD and BUILDDOC_FORCE_REBUILD set." ; \
      exit 1 ; }

##############################################################################

set -u

##############################################################################

export SHELL="/bin/sh"

##############################################################################

test -z "${1:-}" ||                                                          \
    { TRELVER="$( printf %s\\n "${1:?}" | tr -cd '[0-9]rawhide\n' )" ;       \
      printf '%s\n' "${TRELVER:?}" | ${GREP:-grep} -q -E                     \
        "(^rawhide$|^[0-9]+$)" || { printf '%s\n'                            \
          "Error: Invalid argument." ; exit 1 ; } ;                          \
            RELEASEVER="${TRELVER:?}" && export RELEASEVER ||                \
      { printf '%s\n' "Error: Unable to set RELEASEVER." ; exit 1 ; } ; }

##############################################################################

test -f "./shell/builddoc-docker.sh" ||                                      \
  { printf '%s\n' "Error: Not running in docs directory?" ; exit 1 ; }

##############################################################################

test -f "./docker/Dockerfile" ||                                             \
  { printf '%s\n' "Error: No Dockerfile could be found." ; exit 1 ; }

##############################################################################

# shellcheck disable=SC2155
export PDIR="$( ( cd .. && pwd -P ) )" && { export PDIR &&                   \
test -d "${PDIR:?}" ||                                                       \
  { printf '%s\n' "Error: Bad directory ${PDIR:-}" ; exit 1 ; } ; } ||       \
    { printf '%s\n' "Error: Unable to determine directory." ; exit 1 ; }

##############################################################################

command -v "lscpu" > "/dev/null" 2>&1 ||
  { printf '%s\n' "Error: lscpu command not found." ; exit 1 ; }

##############################################################################

command -v "jq" > "/dev/null" 2>&1 ||
  { printf '%s\n' "Error: jq command not found." ; exit 1 ; }

##############################################################################

command -v "docker" > "/dev/null" 2>&1 ||
  { printf '%s\n' "Error: docker command not found." ; exit 1 ; }

##############################################################################

printf '%s' "Testing Docker: "

${DOCKER:-docker} version > "/dev/null" 2>&1 ||                              \
  { printf '%s\n' "ERROR." ; exit 1 ; }

${DOCKER:-docker} run --rm -it "hello-world:latest" > "/dev/null" 2>&1 ||    \
  { printf '%s\n' "ERROR." ; exit 1 ; }

printf '%s\n' "OK."

##############################################################################

# shellcheck disable=SC2155,SC2126
export NUM_CPUS="$( lscpu -b --extended 2> "/dev/null"  |                    \
                    grep -v '^CPU '     2> "/dev/null"  |                    \
                    wc -l               2> "/dev/null"  |                    \
                    tr -cd '\r[0-9]\n'  2> "/dev/null" || true )"

test -z "${NUM_CPUS:-}" && export NUM_CPUS=1 || true

test "${NUM_CPUS:-}" -lt 1 && export NUM_CPUS=1 || true

test -z "${VERBOSE:-}" ||                                                    \
    printf 'DEBUG: %s\n' "${NUM_CPUS:?} CPUs online."

##############################################################################

test -z "${VERBOSE:-}" || EXPORT_VERBOSE_MODE='export V=1'

export BUILDTAG="printf %s\\n                                                \
    'The DPS8M Development Team - Omnibus Documentation Build'               \
      > ../.builder.txt"

export DOCBUILD="sh -c \"unset VERBOSE ; git config --global --add           \
                         safe.directory /out && ${EXPORT_VERBOSE_MODE:-true} \
                         && cd /out/docs/ && ${DB_GMAKE:?} -j${NUM_CPUS:?}   \
                         -C .. superclean && ${DB_GMAKE:?} -j${NUM_CPUS:?}   \
                         -C .. && ./shell/builddoc.sh && ${DB_GMAKE:?}       \
                         -j${NUM_CPUS:?} -C .. distclean > /dev/null 2>&1\""

##############################################################################

IMG_OK=0 ; ${DOCKER:-docker} "image" "inspect"                               \
  "dps8m-helper-${RELEASEVER:-${DEFAULT_RELEASE:?}}:Dockerfile"              \
    > "/dev/null" 2>&1 && IMG_OK=1

test -z "${VERBOSE:-}" ||                                                    \
    printf 'DEBUG: %s\n' "IMG_OK set to \"${IMG_OK:?}\"."

test "${IMG_OK:-0}" -eq 1 &&                                                 \
{

IMG_DATE="$( date -d "$( 2> "/dev/null" "docker" "image" "inspect"           \
             dps8m-helper-${RELEASEVER:-${DEFAULT_RELEASE:?}}:Dockerfile |   \
              jq -ca '.[]|{Created}' 2> "/dev/null" | cut -d \" -f4          \
               2> "/dev/null")" +%s  2> "/dev/null" )"

test -z "${VERBOSE:-}" ||                                                    \
    printf 'DEBUG: %s\n' "Docker image timestamp: \"${IMG_DATE:-0}\"."

OLD_DATE="$( date -d "${OLD_DAYS:?} days ago" +%s 2> "/dev/null" )"

AGE_DIFF="$(( IMG_DATE - OLD_DATE ))"

test -n "${BUILDDOC_FORCE_REBUILD:-}" && AGE_DIFF="-1"

test -z "${VERBOSE:-}" ||                                                    \
    printf 'DEBUG: %s\n' "Docker image TTL: \"${AGE_DIFF:-0}\"."

test "${AGE_DIFF:-0}" -lt 1 &&                                               \
    {                                                                        \
        test -z "${BUILDDOC_FORCE_REBUILD:-}" && printf '%s' "Image " ;      \
        test -z "${BUILDDOC_FORCE_REBUILD:-}" && printf '%s'                 \
        "\"dps8m-helper-${RELEASEVER:-${DEFAULT_RELEASE:?}}:Dockerfile\"" ;  \
        test -z "${BUILDDOC_FORCE_REBUILD:-}" && printf '%s\n'               \
          " is more than ${OLD_DAYS:?} days old." ;                          \
        test -n "${BUILDDOC_SKIP_REBUILD:-}" || ${DOCKER:-docker} image rm   \
          -f dps8m-helper-${RELEASEVER:-${DEFAULT_RELEASE:?}}:Dockerfile ;   \
    } ;
}

##############################################################################

test -n "${ONESIDE:-}" && DO_ONESIDE="--env ONESIDE=1" && export DO_ONESIDE

##############################################################################

eval ${DOCKER:-docker} "run" --rm -v "${PDIR:?}":"/out" ${DO_ONESIDE:-} -it  \
 "dps8m-helper-${RELEASEVER:-${DEFAULT_RELEASE:?}}:Dockerfile"               \
  "${DOCBUILD:?}" ||                                                         \
    {                                                                        \
      printf '%s\n' "Building Docker image ..." &&                           \
      ${SED:-sed} "s/##RELEASEVER##/${RELEASEVER:-${DEFAULT_RELEASE:?}}/"    \
      "./docker/Dockerfile" | ${DOCKER:-docker} "build" -t                   \
      "dps8m-helper-${RELEASEVER:-${DEFAULT_RELEASE:?}}:Dockerfile" "-" &&   \
      eval ${DOCKER:-docker} "run" --rm -v "${PDIR:?}":"/out"                \
        ${DO_ONESIDE:-} -it                                                  \
          "dps8m-helper-${RELEASEVER:-${DEFAULT_RELEASE:?}}:Dockerfile"      \
            "${DOCBUILD:?}" ;                                                \
    }

##############################################################################

${RMF:-rm} -f ../.builder.txt > "/dev/null" 2>&1

##############################################################################

test -z "${VERBOSE:-}" ||                                                    \
    printf 'DEBUG: %s\n' "End of script."

##############################################################################
