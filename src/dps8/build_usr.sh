#!/usr/bin/env sh
# shellcheck disable=SC3040,SC2015
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: ICU
# scspell-id: 8108c60c-3cb2-11ed-810d-80ee73e9b8e7

##############################################################################
#
# Copyright (c) 2020-2021 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2021-2022 The DPS8M Development Team
#
# All rights reserved.
#
# This software is made available under the terms of the ICU
# License, version 1.8.1 or later.  For more details, see the
# LICENSE.md file at the top-level directory of this distribution.
#
##############################################################################

##############################################################################
# Check for csh as sh.

# shellcheck disable=SC2006,SC2046,SC2065,SC2116
test _`echo asdf 2>/dev/null` != _asdf >/dev/null &&\
    printf '%s\n' "Warning: build_usr.sh does not support csh as sh." &&\
    exit 1

##############################################################################
# Attempt to disable pedantic verification during environment normalization.

set +e > /dev/null 2>&1
set +u > /dev/null 2>&1

##############################################################################
# Attempt to setup a standard POSIX locale.

eval export $(command -p env locale 2> /dev/null) > /dev/null 2>&1 || true
DUALCASE=1 && export DUALCASE || true
LC_ALL=C && export LC_ALL || true
LANGUAGE=C && export LANGUAGE || true
LANG=C && export LANG || true
(unset CDPATH > /dev/null 2>&1) > /dev/null 2>&1 || true
unset CDPATH > /dev/null 2>&1 || true

##############################################################################
# Sanitization for zsh.

if [ ".${ZSH_VERSION:-}" != "." ] &&
    (emulate sh 2> /dev/null) > /dev/null 2>&1; then
    emulate sh > /dev/null 2>&1 || true
    NULLCMD=: && export NULLCMD || true
    # shellcheck disable=SC2142
    alias -g '${1+"$@"}'='"$@"' > /dev/null 2>&1 || true
    unalias -a > /dev/null 2>&1 || true > /dev/null 2>&1
    unalias -m '*' > /dev/null 2>&1 || true
    disable -f -m '*' > /dev/null 2>&1 || true
    setopt pipefail > /dev/null 2>&1 || true
    POSIXLY_CORRECT=1 && export POSIXLY_CORRECT || true
    POSIX_ME_HARDER=1 && export POSIX_ME_HARDER || true

##############################################################################
# Sanitization for bash.

elif [ ".${BASH_VERSION:-}" != "." ] &&
    (set -o posix 2> /dev/null) > /dev/null 2>&1; then
    set -o posix > /dev/null 2>&1 || true > /dev/null 2>&1
    set -o pipefail > /dev/null 2>&1 || true > /dev/null 2>&1
    POSIXLY_CORRECT=1 && export POSIXLY_CORRECT || true > /dev/null 2>&1
    POSIX_ME_HARDER=1 && export POSIX_ME_HARDER || true > /dev/null 2>&1
    unalias -a > /dev/null 2>&1 || true > /dev/null 2>&1
fi

##############################################################################
# Clear environment.

for as_var in BASH_ENV ENV MAIL MAILPATH; do
    # shellcheck disable=SC1083
    eval test x\${"${as_var:?}"+set} = xset &&
        ( # /* xset */
            (unset "${as_var:-}") 2> /dev/null ||
                { # /* unset */
                    printf >&2 '%s\n' \
                        "Error: clear environment failed."
                    exit 0
                }
        ) &&
        unset "${as_var:-}" 2> /dev/null || \
            true > /dev/null 2>&1
done # /* as_var */

##############################################################################
# Attempt to enable pedantic error checking.

set -u > /dev/null 2>&1
set -e > /dev/null 2>&1

##############################################################################

get_bld_user()
{ # /* get_bld_user() */
    MYNODENAME=$( (command -p env uname -n 2> /dev/null || \
        true > /dev/null 2>&1) | \
            cut -d " " -f 1 2> /dev/null || \
                true > /dev/null 2>&1)
    WHOAMIOUTP=$(command -p env who am i 2> /dev/null || \
        true > /dev/null 2>&1)
    WHOAMIOUTG=$(command -p env whoami 2> /dev/null || \
        true > /dev/null 2>&1)
    PREPAREDBY="${WHOAMIOUTP:-${WHOAMIOUTG:-}}"
    # shellcheck disable=SC2002
    BUILDERTXT="$(cat ../../.builder.txt 2> /dev/null | \
        tr -d '\"' 2> /dev/null || \
            true > /dev/null 2>&1)"

    printf '%s\n' \
        "${BUILDERTXT:-${PREPAREDBY:-Unknown}@${MYNODENAME:-Unknown}}"  | \
            tr -s ' ' 2> /dev/null | sed -e 's/@Unknown//' 2> /dev/null | \
                sed -e 's/\"//' \
                    -e 's/\%//' \
                    -e 's/\\//' \
                    -e "s/'//" 2> /dev/null ||
                        { # /* PREPAREDBY */
                            printf >&2 '%s\n' \
                                "Error: sed failed."
                            exit 0
                        }
} # /* get_bld_user() */

##############################################################################

BUILD_USR="$(get_bld_user)" ||
    { # /* BUILD_USR */
        printf >&2 '%s\n' \
            "Error: get_bld_user() failed."
        exit 0
    } # /* BUILD_USR */

##############################################################################

printf '%s\n' "${BUILD_USR:-Unknown}"

##############################################################################

# Local Variables:
# mode: sh
# sh-shell: sh
# sh-indentation: 4
# sh-basic-offset: 4
# tab-width: 4
# End:
