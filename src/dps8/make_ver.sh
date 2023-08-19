#!/usr/bin/env sh
# shellcheck disable=SC2236,SC3040,SC2015,SC2310,SC2312,SC2317
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: MIT
# scspell-id: 97aea429-f62f-11ec-b49f-80ee73e9b8e7
# Copyright (c) 2020-2023 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2021-2023 The DPS8M Development Team

##############################################################################
# Check for csh as sh.

# shellcheck disable=SC2006,SC2046,SC2065,SC2116
test _`echo asdf 2>/dev/null` != _asdf >/dev/null &&\
    printf '%s\n' "Warning: make_ver.sh does not support csh as sh." &&\
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
# Set UTC timezone.

{ TZ="UTC" && export TZ; } ||
    {
        printf >&2 '%s\n' \
            "Error: Exporting TZ to environment failed."
        exit 0
    }

##############################################################################
# Set format string for dates.

{ DATEFORMAT="%Y-%m-%d %H:%M ${TZ:-UTC}" && export DATEFORMAT; } ||
    {
        printf >&2 '%s\n' \
            "Error: Exporting DATEFORMAT to environment failed."
        exit 0
    }

{ DATEFALLBACK="2020-01-01 00:00 UTC" && export DATEFALLBACK; } ||
    {
        printf >&2 '%s\n' \
            "Error: Exporting DATEFALLBACK to environment failed."
        exit 0
    }

##############################################################################
# Attempt to enable pedantic error checking.

set -u > /dev/null 2>&1
set -e > /dev/null 2>&1

##############################################################################
# Debug script?

#DEBUG_MAKE_VER=1

if  [ ! -z "${DEBUG_MAKE_VER:-}" ] || \
    [   -n "${DEBUG_MAKE_VER:-}" ]; then
    debug_print()
        {
            printf >&2 '\r******** DEBUG: %s\n' "$*" ||
                true > /dev/null 2>&1
        }
    debug_print "Script debugging is enabled."
else
    debug_print()
        {
            true > /dev/null 2>&1
        }
fi

debug_print "Start of make_ver.sh processing."

##############################################################################

debug_print "Start of OS exception processing."

EARLYUNAME="$(command -p env uname -s 2> /dev/null || \
    true > /dev/null 2>&1)"

if [ "${EARLYUNAME:-}" = "SunOS" ]; then
    debug_print "Enabling Solaris exceptions."
    OS_SUNOS=1 && export OS_SUNOS
    NPATH="$(command -p env getconf PATH 2> /dev/null)" && \
        PATH="${NPATH:-${PATH:-}}:${PATH:-}" && \
            export PATH 2> /dev/null || \
                true > /dev/null 2>&1
fi

if [ "${EARLYUNAME:-}" = "AIX" ]; then
    debug_print "Enabling AIX exceptions."
    OS_IBMAIX=1 && export OS_IBMAIX
fi

debug_print "End of OS exception processing."

##############################################################################
# get_git_ptch() returns the number of additional commits from the last tag.

get_git_ptch()
{ # /* get_git_ptch() */
    debug_print "Start of get_git_ptch()"
    if command -p true 2> /dev/null 1>&2; then # /* TRUE */
        if command git version 2> /dev/null 1>&2; then # /* GIT */
            GITTEST=$(git version 2> /dev/null)
            if [ -n "${GITTEST:-}" ] &&
                [ ! -z "${GITTEST:-}" ]; then # /* GITTEST */
                GITVER=$(git describe --abbrev=0 --dirty='*' \
                    --tags --always 2> /dev/null) ||
                        { # /* GITVER */
                            printf '%s\n' \
                                "Error: git describe --abbrev=0 failed."
                            exit 0
                        } # /* GITVER */
                GITPAT=$(git describe --abbrev=40 --dirty='*' \
                    --tags --always 2> /dev/null) ||
                        { # /* GITPAT */
                            printf '%s\n' \
                                "Error: git describe --abbrev=40 failed."
                            exit 0
                        } # /* GITPAT */
                # shellcheck disable=SC2250
                OIFS="$IFS"
                # shellcheck disable=SC2086
                PATDIF=$(printf '%s\n' ${GITPAT:-XXXXXX} | sed -e "s/$(printf '%s\n' ${GITVER:-XXXXXX} | sed -e 's/*//')//" 2> /dev/null | sed -e 's/\(^.*-[0-9]+-g.*\)/\1/' 2> /dev/null | sed -e "s/${GITVER:-XXXXXX}//" 2> /dev/null | cut -d '-' -f 2 2> /dev/null ) ||
                            { # /* PATDIF */
                                IFS="${OIFS:?}" || true > /dev/null 2>&1
                                printf >&2 '%s\n' \
                                    "Error: sed/cut transform failed."
                                exit 0
                            }
                IFS="${OIFS:?}"
            fi # /* GITTEST */
        else # /* GIT */
            printf >&2 '%s\n' \
                "Error: git version failed."
            exit 0
        fi
    else # /* TRUE */
        printf >&2 '%s\n' \
            "Error: command true failed."
        exit 0
    fi
    if [ -n "${PATDIF:-}" ] &&
        [ ! -z "${PATDIF:-}" ]; then
        printf '%s\n' \
            "${PATDIF:?}"
    else
        exit 0
    fi
    debug_print "End of get_git_ptch()"
} # /* get_git_ptch() */

##############################################################################
# get_git_date returns the date as of the current git commit.

get_git_date()
{ # /* get_git_date() */
    debug_print "Start of get_git_date()"
    if command -p true 2> /dev/null 1>&2; then # /* TRUE */
        if command git version 2> /dev/null 1>&2; then # /* GIT */
            GITTEST=$(git version 2> /dev/null)
            if [ -n "${GITTEST:-}" ] &&
                [ ! -z "${GITTEST:-}" ]; then # /* GITTEST */
                # /* NOTE(johnsonjh): SQ-QUOTE'd output! */
                GIT_SHA=$(git rev-parse --sq --verify '@' 2> /dev/null) ||
                    { # /* GIT_SHA */
                        printf >&2 '%s\n' \
                            "Error: git rev-parse --sq failed."
                        exit 0
                    }
            fi # /* GITTEST */
        else # /* GIT */
            printf >&2 '%s\n' \
                "Error: git version failed."
            exit 0
        fi
    else # /* TRUE */
        printf >&2 '%s\n' \
            "Error: command true failed."
        exit 0
    fi
    if [ -n "${GIT_SHA:-}" ] &&
        [ ! -z "${GIT_SHA:-}" ]; then
        # /* NOTE(johnsonjh): Safe to eval SQ-QUOTE'd output! */
        eval printf '%s\n' \
            "${GIT_SHA:?}"
    else
        exit 0
    fi
    debug_print "End of get_git_date()"
} # /* get_git_date() */

##############################################################################
# get_git_vers() returns a formatted version from the current git environment.

get_git_vers()
{ # /* get_git_vers() /*
    debug_print "Start of get_git_vers()"
    if command -p true 2> /dev/null 1>&2; then # /* TRUE */
        if command git version 2> /dev/null 1>&2; then # /* GIT */
            GITTEST=$(git version 2> /dev/null)
            if [ -n "${GITTEST:-}" ] &&
                [ ! -z "${GITTEST:-}" ]; then # /* GITTEST */
                BRANCH=$(git rev-parse --abbrev-ref HEAD 2> /dev/null ||
                    true > /dev/null 2>&1)
                GITVER=$(git describe --abbrev=0 --dirty='*' --tags --always \
                    2> /dev/null)
                GITPAT=$(git describe --abbrev=40 --dirty='*' --tags --always \
                    2> /dev/null)
                if [ ! -n "${BRANCH:-}" ] ||
                    [ -z "${BRANCH:-}" ]; then # /* BRANCH */
                    BRANCH="nobranch"
                fi # /* endif BRANCH */
                if [ "${BRANCH:-}" = "master" ] ||
                    [ "${BRANCH:-}" = "main" ] ||
                    [ "${BRANCH:-}" = "trunk" ] ||
                    [ "${BRANCH:-}" = "nobranch" ]; then
                    if [ -n "${GITVER:-}" ] &&
                        [ ! -z "${GITVER:-}" ]; then
                        GIT_OUT=$(printf '%s' \
                            "${GITVER:?}")
                    fi # /* endif GITVER */
                else # /* BRANCH */
                    if [ -n "${GITVER:-}" ] &&
                        [ ! -z "${GITVER:-}" ]; then
                        GIT_OUT=$(printf '%s' \
                            "${GITVER:?}-${BRANCH:?}")
                    fi # /* endif GITVER */
                fi # /* endif BRANCH */
            fi # /* GITTEST */
        else # /* GIT */
            printf >&2 '%s\n' \
                "Error: git version failed."
            exit 0
        fi # /* endif GIT */
    else # /* TRUE */
        printf >&2 '%s\n' \
            "Error: command true failed."
        exit 0
    fi # /* endif TRUE */

    GIT_SOURCE_INFO="${GIT_OUT:-0.0.0}"
    GIT_SOURCE_XFRM=$(printf '%s\n' "${GIT_SOURCE_INFO:?}" |
        tr -d ' ' 2> /dev/null | \
            sed -e 's/_/-/g' 2> /dev/null) ||
                { # /* sed */
                    printf >&2 '%s\n' \
                        "Error: sed transform failed."
                    exit 0
                }
    if [ -n "${GIT_SOURCE_XFRM:-}" ] &&
        [ ! -z "${GIT_SOURCE_XFRM:-}" ]; then
        printf '%s\n' \
            "${GIT_SOURCE_XFRM:?}"
    else
        printf '%s\n' \
            "${GIT_SOURCE_INFO:?}"
    fi
    debug_print "End of get_git_vers()"
} # /* get_git_vers() */

##############################################################################
# get_git_hash() returns the SHA-1 hash of the current git commit.

get_git_hash()
{ # /* get_git_hash() */
    debug_print "Start of get_git_hash()"
    if command -p true 2> /dev/null 1>&2; then # /* TRUE */
        if command git version 2> /dev/null 1>&2; then # /* GIT */
            GITTEST=$(git version 2> /dev/null)
            if [ -n "${GITTEST:-}" ] &&
                [ ! -z "${GITTEST:-}" ]; then # /* GITTEST */
                # /* NOTE(johnsonjh): Require SQ-QUOTE'd output */
                GIT_SHA=$(git rev-parse --sq --verify '@' 2> /dev/null) ||
                    { # /* GIT_SHA */
                        printf >&2 '%s\n' \
                            "Error: git rev-parse --sq failed."
                        exit 0
                    }
            fi # /* GITTEST */
        else # /* GIT */
            printf >&2 '%s\n' \
                "Error: git version failed."
            exit 0
        fi
    else # /* TRUE */
        printf >&2 '%s\n' \
            "Error: command true failed."
        exit 0
    fi
    if [ -n "${GIT_SHA:-}" ] &&
        [ ! -z "${GIT_SHA:-}" ]; then
        # /* NOTE(johnsonjh): Safe to eval only if SQ-QUOTE'd */
        eval printf '%s' \
            "${GIT_SHA:?}"
    else
        exit 0
    fi
    debug_print "End of get_git_hash()"
} # /* get_git_hash() */

##############################################################################
# get_git_date() returns the date in UTC of the current git commit.

get_git_date()
{ # /* get_git_date() /*
    debug_print "Start of get_git_date()"
    if command -p true 2> /dev/null 1>&2; then # /* TRUE */
        if command git version 2> /dev/null 1>&2; then # /* GIT */
            GITTEST=$(git version 2> /dev/null)
            if [ -n "${GITTEST:-}" ] &&
                [ ! -z "${GITTEST:-}" ]; then # /* GITTEST */
                CDDATE=""$(git show -s --format="%cd" \
                    --date="format:${DATEFORMAT:?}" 2> /dev/null) ||
                        { # /* CDDATE */
                            printf '%s\n' \
                                "Error: git show failed."
                            exit 0
                        } # /* CDDATE */
                GIT_DATE_OUT="$(printf '%s' \
                    "${CDDATE:?}")"
            fi # /* GITTEST */
        else # /* GIT */
            printf >&2 '%s\n' \
                "Error: git version failed."
            exit 0
        fi # /* endif GIT */
    else # /* TRUE */
        printf >&2 '%s\n' \
            "Error: command true failed."
        exit 0
    fi # /* endif TRUE */

    printf '%s\n' \
        "${GIT_DATE_OUT:?}"
    debug_print "End of get_git_date()"
} # /* get_git_date() */

##############################################################################
# get_utc_date() returns the current date in UTC format or placeholder text.

get_utc_date()
{ # /* get_utc_date() */
    debug_print "Start of get_utc_date()"
    UTC_BLD_DATE=$(command -p env TZ=UTC \
        date -u "+${DATEFORMAT:?}" 2> /dev/null) ||
            { # /* UTC_BLD_DATE */
                printf >&2 '%s\n' \
                    "Warning: Using fallback date \"${DATEFALLBACK:?}\""
                UTC_BLD_DATE="${DATEFALLBACK:?}"
            }
    if [ -n "${UTC_BLD_DATE:-}" ] &&
        [ ! -z "${UTC_BLD_DATE:-}" ]; then
        UTC_BLD_DATE_INFO="${UTC_BLD_DATE:?}"
    else # /* UTC_BLD_DATE_INFO */
        {
            printf >&2 '%s\n' \
                "Warning: Using fallback date \"${DATEFALLBACK:?}\""
            UTC_BLD_DATE_INFO="${DATEFALLBACK:?}"
        }
    fi

    printf '%s\n' \
        "${UTC_BLD_DATE_INFO:-}"
    debug_print "End of get_utc_date()"
} # /* get_utc_date() */

##############################################################################
# get_bld_user() returns "username", "username@host", or .builder.txt contents

get_bld_user()
{ # /* get_bld_user() */
    debug_print "Start of get_bld_user()"
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
    debug_print "End of get_bld_user()"
} # /* get_bld_user() */

##############################################################################
# get_bld_osys() returns the OS name, contents of .buildos.txt, or "Unknown"

get_bld_osys()
{ # /* get_bld_osys() */
   debug_print "Start of get_bld_osys()"
   if [ -n "${OS_IBMAIX:-}" ]; then
    BLD_OSYS="IBM AIX $(command -p env oslevel -s      2> /dev/null        | \
     cut -d '-' -f 1-3                                 2> /dev/null        | \
     grep '[1-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]' 2> /dev/null        | \
      sed -e 's/-/ TL/'      -e 's/-/ SP/'             2> /dev/null        | \
      sed -e 's/ SP00$//'    -e 's/ TL00$//'           2> /dev/null        | \
      sed -e 's/ TL0/ TL/'   -e 's/ SP0/ SP/'          2> /dev/null        | \
      sed -e 's/.\{1\}/&./g' -e 's/\. \./ /g'          2> /dev/null          \
          -e 's/T\.L\./TL/'  -e 's/S\.P\./SP/'                               \
          -e 's/\.$//'       -e 's/0\.0\.0 / /'                              \
          -e 's/0\.0 / /'    -e 's/0 / /'                                    \
          -e 's/\. / /'                                                   || \
          head -n 1 /usr/lpp/bos/aix_release.level     2> /dev/null        | \
             sed -e 's/\.0\.0\.0$//'                                         \
                 -e 's/\.0\.0$//'                                            \
                 -e 's/\.0$//'                         2> /dev/null       || \
                    true                               1> /dev/null 2>&1)"
    else
        BLD_OSYS="$( (command -p env uname -mrs        2> /dev/null       || \
        true                                           1> /dev/null 2>&1)  | \
               tr -d '*'                               2> /dev/null       || \
                   true                                1> /dev/null 2>&1)"
    fi
    if [ -f "../../.buildos.txt" ]; then
        # shellcheck disable=SC2002
        BLD_OSYS="$(cat ../../.buildos.txt             2> /dev/null        | \
            tr -d '\"'                                 2> /dev/null        | \
            tr -d '*'                                  2> /dev/null       || \
             true                                      1> /dev/null 2>&1)"
    fi
    printf '%s\n' \
        "${BLD_OSYS:-Unknown}" | \
            tr -s ' '                                  2> /dev/null        ||
                { # /* BLD_OSYS */
                    printf >&2 '%s\n' \
                        "Error: tr failed."
                }
    debug_print "End of get_bld_osys()"
} # /* get_bld_osys() */

##############################################################################
# get_bld_osnm() returns the OS name or "Unknown"

get_bld_osnm()
{ # /* get_bld_osnm() */
    debug_print "Start of get_bld_osnm()"
    BLD_OSNM="$( (command -p env uname -s 2> /dev/null ||
        true > /dev/null 2>&1) | \
            tr -d '*' 2> /dev/null || \
                true   > /dev/null 2>&1)"
    printf '%s\n' \
        "${BLD_OSNM:-Unknown}" | \
            tr -s ' ' 2> /dev/null ||
                { # /* BLD_OSNM */
                    printf >&2 '%s\n' \
                        "Error: tr failed."
                }
    debug_print "End of get_bld_osnm()"
} # /* get_bld_osnm() */

##############################################################################
# get_bld_osar() returns the OS arch (POSIX machine type) or "Unknown"

get_bld_osar()
{ # /* get_bld_osar() */
    debug_print "Start of get_bld_osar()"
    if [ -n "${OS_IBMAIX:-}" ]; then
      BLD_OSAR="$( (command -p env uname -p 2> /dev/null || \
          command -p env uname -M 2> /dev/null || \
              uname -M 2> /dev/null || uname -p 2> /dev/null || \
                  printf '%s\n' 'Unknown') | \
                      sed -e 's/^AIX //' 2> /dev/null | \
                          sed -e 's/ (emulated by .....)//' 2> /dev/null | \
                              tr -d '*' 2> /dev/null || \
                                  true  1> /dev/null 2>&1)"
    fi
    test "${BLD_OSAR:-}" = "unknown" 2> /dev/null && BLD_OSAR=""
    if [ -z "${BLD_OSAR:-}" ]; then
      BLD_OSAR="$( (command -p env uname -m 2> /dev/null ||
          true > /dev/null 2>&1) | \
              tr -d '*' 2> /dev/null || \
                true    1> /dev/null 2>&1)"
    fi
    printf '%s\n' \
        "${BLD_OSAR:-Unknown}" | \
            tr -s ' ' 2> /dev/null ||
                { # /* BLD_OSAR */
                    printf >&2 '%s\n' \
                        "Error: tr failed."
                }
    debug_print "End of get_bld_osar()"
} # /* get_bld_osar() */

##############################################################################
# get_bld_osvr() returns the OS version (POSIX release name) or "Unknown"

get_bld_osvr()
{ # /* get_bld_osvr() */
    debug_print "Start of get_bld_osvr()"
    BLD_OSVR="$( (command -p env uname -r 2> /dev/null ||
        true > /dev/null 2>&1) | \
            tr -d '*' 2> /dev/null || \
                true   > /dev/null 2>&1)"
    printf '%s\n' \
        "${BLD_OSVR:-Unknown}" | \
            tr -s ' ' 2> /dev/null ||
                { # /* BLD_OSVR */
                    printf >&2 '%s\n' \
                        "Error: tr failed."
                }
    debug_print "End of get_bld_osvr()"
} # /* get_bld_osvr() */

##############################################################################
# Gather information or complain with an error.

debug_print "Start of gather and print phase"

debug_print "BUILD_VER"
BUILD_VER="$(get_git_vers)" ||
    { # /* BUILD_VER */
        printf >&2 '%s\n' \
            "Error: get_git_vers() failed."
        exit 0
    } # /* BUILD_VER */

debug_print "BUILD_DTE"
BUILD_DTE="$(get_git_date)" ||
    { # /* BUILD_DTE */
        printf >&2 '%s\n' \
            "Error: get_git_date() failed."
        exit 0
    } # /* BUILD_DTE */

debug_print "BUILD_DTE_SHORT"
BUILD_DTE_SHORT="${BUILD_DTE%% *}"

debug_print "BUILD_UTC"
BUILD_UTC="$(get_utc_date)" ||
    { # /* BUILD_UTC */
        printf >&2 '%s\n' \
            "Error: get_utc_date() failed."
        exit 0
    } # /* BUILD_UTC */

debug_print "BUILD_SHA"
BUILD_SHA="$(get_git_hash)" ||
    { # /* BUILD_SHA */
        printf >&2 '%s\n' \
            "Error: get_git_hash() failed."
        exit 0
    } # /* BUILD_SHA */

debug_print "BUILD_PAT"
BUILD_PAT="$(get_git_ptch)" ||
    { # /* BUILD_PAT */
        printf >&2 '%s\n' \
            "Error: get_git_ptch() failed."
        exit 0
    } # /* BUILD_PAT */

debug_print "BUILD_USR"
BUILD_USR="$(get_bld_user)" ||
    { # /* BUILD_USR */
        printf >&2 '%s\n' \
            "Error: get_bld_user() failed."
        exit 0
    } # /* BUILD_USR */

debug_print "BUILD_SNM"
BUILD_SNM="$(get_bld_osnm)" ||
    { #/ * BUILD_SNM */
        printf >&2 '%s\n' \
            "Error: get_bld_osnm() failed."
        exit 0
    } #/* BUILD_SNM */

debug_print "BUILD_SVR"
BUILD_SVR="$(get_bld_osvr)" ||
    { #/ * BUILD_SVR */
        printf >&2 '%s\n' \
            "Error: get_bld_osvr() failed."
        exit 0
    } #/* BUILD_SVR */

debug_print "BUILD_SAR"
BUILD_SAR="$(get_bld_osar)" ||
    { #/ * BUILD_SAR */
        printf >&2 '%s\n' \
            "Error: get_bld_osar() failed."
        exit 0
    } #/* BUILD_SAR */

debug_print "BUILD_SYS"
BUILD_SYS="$(get_bld_osys)" ||
    { # /* BUILD_SYS */
        printf >&2 '%s\n' \
            "Error: get_bld_osys() failed."
        exit 0
    } # /* BUILD_SYS */

debug_print "BUILD_SYS"
BUILD_SYS="$(get_bld_osys)" ||
    { # /* BUILD_SYS */
        printf >&2 '%s\n' \
            "Error: get_bld_osys() failed."
        exit 0
    } # /* BUILD_SYS */

debug_print "End of gather and print phase"

##############################################################################
# Release type heuristic

debug_print "Start release type heuristics"

# /*
#  * Release types:
#  * A: Alpha
#  * B: Beta
#  * C: Candidate
#  * D: Development
#  * R: Release
#  * X: Unknown
#  */

debug_print "Setting BUILD_TMP_RLT"
BUILD_TMP_RLT="$(get_git_vers)" ||
    { # /* BUILD_TMP_RLT */
        printf >&2 '%s\n' \
            "Error: get_git_vers() failed setting BUILD_TMP_RLT."
        exit 0
    } # /* BUILD_TMP_RLT */
debug_print "BUILD_TMP_RLT is ${BUILD_TMP_RLT:-}"

debug_print "Setting BUILD_TMP_RLT_FIRST"
BUILD_TMP_RLT_FIRST="$(printf '%s' "${BUILD_TMP_RLT:-Z}" |
    tr -cd '[:alpha:]' 2> /dev/null |
        sed -e 's/^\(.\).*/\1/' 2> /dev/null |
            tr '[:lower:]' '[:upper:]' 2> /dev/null)"
debug_print "BUILD_TMP_RLT_FIRST is ${BUILD_TMP_RLT_FIRST:-}"

debug_print "Setting BUILD_TMP_RLT_LAST2"
BUILD_TMP_RLT_LAST2="$(git describe --abbrev=0 --dirty='' --tags --always \
    2> /dev/null | tr -cd '[:alpha:]' 2> /dev/null |
        sed -e 's/.*\(..\)/\1/' 2> /dev/null |
            tr '[:upper:]' '[:lower:]' 2> /dev/null)"
debug_print "BUILD_TMP_RLT_LAST2 is ${BUILD_TMP_RLT_LAST2:-}"

# /* ... Start Rules ... */

# /* Our default state is "X" */
debug_print "Setting release type to 'X'"
BUILD_RLT="X"

# /*
#  * Our short version string starts off with "R"?
#  *    Set BUILD_RLT="R"
#  */
if [ "${BUILD_TMP_RLT_FIRST:-}" = "R" ]; then
    debug_print "Setting release type to 'R'"
    BUILD_RLT="R"
fi

# /*
#  * Our short version string ends with "rc"?
#  *    Set BUILD_RC="C"
#  */
if [ "${BUILD_TMP_RLT_LAST2:-}" = "rc" ]; then
    debug_print "Setting release type to 'C'"
    BUILD_RLT="C"
fi

# /*
#  *  Git tags suffixes we use should be in the following forms,
#  *     with the preferred form listed last:
#  *
#  *     rcN[NN] .......... for candidates
#  *
#  *     alphaN[NN]
#  *     atN[NN] .......... for alphas
#  *
#  *     betaN[NN]
#  *     btN[NN] .......... for betas
#  *
#  *     developerN[NN]
#  *     developN[NN]
#  *     devN[NN]
#  *     dpN[NN]
#  *     deN[NN]
#  *     dtN[NN]
#  *     drN[NN] .......... for dev
#  *
#  *  Thus, from here, we can use this "last2" comparison table:
#  *
#  *    rc ................................. rc    (C)
#  *    at / ha ............................ alpha (A)
#  *    bt / ta ............................ beta  (B)
#  *    er / op / ev / dp / de / dt / dr ... dev   (D)
#  */

# /*
#  * Our short version string indicates an alpha?
#  *    Set BUILD_RC="A"
#  */
if  [ "${BUILD_TMP_RLT_LAST2:-}" = "at" ] || \
    [ "${BUILD_TMP_RLT_LAST2:-}" = "ha" ]; then
    debug_print "Setting release type to 'A'"
    BUILD_RLT="A"
fi

# /*
#  * Our short version string indicates a beta?
#  *    Set BUILD_RC="B"
#  */
if  [ "${BUILD_TMP_RLT_LAST2:-}" = "bt" ] || \
    [ "${BUILD_TMP_RLT_LAST2:-}" = "ta" ]; then
    debug_print "Setting release type to 'B'"
    BUILD_RLT="B"
fi

# /*
#  * Our short version string indicates a dev release?
#  *    Set BUILD_RC="D"
#  */
if  [ "${BUILD_TMP_RLT_LAST2:-}" = "er" ] || \
    [ "${BUILD_TMP_RLT_LAST2:-}" = "op" ] || \
    [ "${BUILD_TMP_RLT_LAST2:-}" = "ev" ] || \
    [ "${BUILD_TMP_RLT_LAST2:-}" = "dp" ] || \
    [ "${BUILD_TMP_RLT_LAST2:-}" = "de" ] || \
    [ "${BUILD_TMP_RLT_LAST2:-}" = "dt" ] || \
    [ "${BUILD_TMP_RLT_LAST2:-}" = "dr" ]; then
    debug_print "Setting release type to 'D'"
    BUILD_RLT="D"
fi

debug_print "Release type set to '${BUILD_RLT:-}'"

# /* ... End Rules ... */

debug_print "End release type heuristics"

##############################################################################

debug_print "Start of version splitting"

debug_print "Setting BUILD_TMP_VER_SPLIT_INIT"
BUILD_TMP_VER_SPLIT_INIT="$(get_git_vers)" ||
    { # /* BUILD_TMP_VER_SPLIT_INIT */
        printf >&2 '%s\n' \
            "Error: get_git_vers() failed setting BUILD_TMP_VER_SPLIT_INIT."
        exit 0
    } # /* BUILD_TMP_VER_SPLIT_INIT */
debug_print "BUILD_TMP_VER_SPLIT_INIT is ${BUILD_TMP_VER_SPLIT_INIT:-}"

debug_print "Setting BUILD_TMP_VER_SPLIT"
# shellcheck disable=SC2086
BUILD_TMP_VER_SPLIT="$(printf '%s\n' \
    ${BUILD_TMP_VER_SPLIT_INIT:-999.999.999.999} | \
        tr -d '/[:alpha:]*'       2> /dev/null | \
            tr -s ' '             2> /dev/null | \
                cut -d '-' -f 1-2 2> /dev/null | \
                    tr '-' '.'    2> /dev/null | \
                    tr '_' '.'    2> /dev/null | \
                    tr '.' '\n'   2> /dev/null )"
# shellcheck disable=SC2086
debug_print "BUILD_TMP_VER_SPLIT is" "[" ${BUILD_TMP_VER_SPLIT:-} "]"

# /* Avoiding awk, m4, +/- syntax, extended regex, and substitution! */

# /* TODO: Abort if parts < 3 and give useful error message. */

# /* How many parts? */
VER_H_PROM_VER_PART_COUNT="$(printf '%s\n' "${BUILD_TMP_VER_SPLIT:-}" | \
    wc -l 2> /dev/null || \
        printf '%s\n' "0")"
debug_print "VER_H_PROM_VER_PART_COUNT is ${VER_H_PROM_VER_PART_COUNT:-0}"

# /* No major?  Really bad! */
if [ "${VER_H_PROM_VER_PART_COUNT:-0}" -lt 1 ]; then
        printf >&2 '%s\n' \
            "Warning: Unable to determine major version."
        FALLBACK_MAJOR=1
fi

# /* No minor?  Bad! */
if [ "${VER_H_PROM_VER_PART_COUNT:-0}" -lt 2 ]; then
        printf >&2 '%s\n' \
            "Warning: Unable to determine minor version."
        FALLBACK_MINOR=1
fi

# /* No patch?  Not good! */
if [ "${VER_H_PROM_VER_PART_COUNT:-0}" -lt 3 ]; then
        printf >&2 '%s\n' \
            "Warning: Unable to determine patch version."
        FALLBACK_PATCH=1
fi

# /* Major, clamped to 999 */
if [ "${FALLBACK_MAJOR:-0}" -ne 1 ]; then
    VER_H_PROM_MAJOR_VER="$(printf '%s\n' "${BUILD_TMP_VER_SPLIT:-}" | \
        head -n 1 2> /dev/null | \
            tail -n 1 2> /dev/null | \
                head -n 1 2> /dev/null)" || true
    if [ "${VER_H_PROM_MAJOR_VER:-0}" -gt 999 ]; then
        VER_H_PROM_MAJOR_VER="999"
    fi
else
    VER_H_PROM_MAJOR_VER="0"
fi
debug_print "VER_H_PROM_MAJOR_VER is ${VER_H_PROM_MAJOR_VER:-}"

# /* Minor, clamped to 999 */
if [ "${FALLBACK_MINOR:-0}" -ne 1 ]; then
    VER_H_PROM_MINOR_VER="$(printf '%s\n' "${BUILD_TMP_VER_SPLIT:-}" | \
        head -n 2 2> /dev/null | \
            tail -n 1 2> /dev/null | \
                head -n 1 2> /dev/null)" || true
    if [ "${VER_H_PROM_MINOR_VER:-0}" -gt 999 ]; then
        VER_H_PROM_MINOR_VER="999"
    fi
else
    VER_H_PROM_MINOR_VER="0"
fi
debug_print "VER_H_PROM_MINOR_VER is ${VER_H_PROM_MINOR_VER:-}"

# /* Patch, clamped to 999 */
if [ "${FALLBACK_PATCH:-0}" -ne 1 ]; then
    VER_H_PROM_PATCH_VER="$(printf '%s\n' "${BUILD_TMP_VER_SPLIT:-}" | \
        head -n 3 2> /dev/null | \
            tail -n 1 2> /dev/null | \
                head -n 1 2> /dev/null)" || true
    if [ "${VER_H_PROM_PATCH_VER:-0}" -gt 999 ]; then
        VER_H_PROM_PATCH_VER="999"
    fi
else
    VER_H_PROM_PATCH_VER="0"
fi
debug_print "VER_H_PROM_PATCH_VER is ${VER_H_PROM_PATCH_VER:-}"

# /* Other, clamped to 999 */
if [ "${VER_H_PROM_VER_PART_COUNT:-0}" -gt 3 ]; then
    VER_H_PROM_OTHER_VER="$(printf '%s\n' "${BUILD_TMP_VER_SPLIT:-}" | \
        head -n 4 2> /dev/null | \
            tail -n 1 2> /dev/null | \
                head -n 1 2> /dev/null)" || true
else
    VER_H_PROM_OTHER_VER="0"
fi
if [ "${VER_H_PROM_OTHER_VER:-0}" -gt 999 ]; then
    VER_H_PROM_OTHER_VER="999"
fi
debug_print "VER_H_PROM_OTHER_VER is ${VER_H_PROM_OTHER_VER:-}"

VER_H_PROM_MAJOR_VER_FMT=$(printf '%3.3d\n' "${VER_H_PROM_MAJOR_VER:-0}")
VER_H_PROM_MINOR_VER_FMT=$(printf '%3.3d\n' "${VER_H_PROM_MINOR_VER:-0}")
VER_H_PROM_PATCH_VER_FMT=$(printf '%3.3d\n' "${VER_H_PROM_PATCH_VER:-0}")
VER_H_PROM_OTHER_VER_FMT=$(printf '%3.3d\n' "${VER_H_PROM_OTHER_VER:-0}")
debug_print "End of version splitting"

##############################################################################
# Construct PROM string VER_H_PROM_VER_TEXT

debug_print "Start VER_H_PROM_VER_TEXT processing"
VER_H_PROM_VER_TMP_TEXT="${VER_H_PROM_MAJOR_VER:-999}.${VER_H_PROM_MINOR_VER:-999}.${VER_H_PROM_PATCH_VER:-999}"
debug_print "BUILD_RLT is ${BUILD_RLT:-}"
if   [ "${BUILD_RLT:?Error: BUILD_RLT unset.}" = "R" ]; then
    VER_H_PROM_VER_TMP_EXTRA=" "
elif [ "${BUILD_RLT:?Error: BUILD_RLT unset.}" = "A" ]; then
    VER_H_PROM_VER_TMP_EXTRA="-alpha${VER_H_PROM_OTHER_VER:?Error: No iteration.}"
elif [ "${BUILD_RLT:?Error: BUILD_RLT unset.}" = "B" ]; then
    VER_H_PROM_VER_TMP_EXTRA="-beta${VER_H_PROM_OTHER_VER:?Error: No iteration.}"
elif [ "${BUILD_RLT:?Error: BUILD_RLT unset.}" = "C" ]; then
    VER_H_PROM_VER_TMP_EXTRA="-rc${VER_H_PROM_OTHER_VER:?Error: No iteration.}"
elif [ "${BUILD_RLT:?Error: BUILD_RLT unset.}" = "D" ]; then
    VER_H_PROM_VER_TMP_EXTRA="-dev${VER_H_PROM_OTHER_VER:?Error: No iteration.}"
else
    VER_H_PROM_VER_TMP_EXTRA=" " # note_627092833
fi
VER_H_PROM_VER_UNPADDED_TEXT="${VER_H_PROM_VER_TMP_TEXT:?}${VER_H_PROM_VER_TMP_EXTRA:?}"
VER_H_PROM_VER_FIXUP_TEXT="$(printf '%s\n' "${VER_H_PROM_VER_UNPADDED_TEXT:?}" |  \
    sed -e 's/-rc/\./' -e 's/-alpha/\./'     -e 's/-at/\./'      -e 's/-beta/\./' \
    -e 's/-bt/\./'     -e 's/-developer/\./' -e 's/-develop/\./' -e 's/-dev/\./'  \
    -e 's/-de/\./'     -e 's/-dt/\./'        -e 's/-dr/\./'   | sed 's/\.$//')"
VER_H_PROM_VER_TEXT="$(printf '"%-30s"\n' "${VER_H_PROM_VER_FIXUP_TEXT:?}")"
debug_print "VER_H_PROM_VER_TEXT is ${VER_H_PROM_VER_TEXT:-}"
debug_print "End VER_H_PROM_VER_TEXT processing"

debug_print "Start ver fixup and exceptions"

# /*
#  * Now, do we have any post-tag patches per git?
#  *    If so, we go back to default of "X"
#  */
BUILD_PAT="$(printf '%s\n' "${BUILD_PAT:-0}" | \
    tr -cd '[:digit:]' 2> /dev/null || \
        true > /dev/null 2>&1)"

# /* TODO(johnsonjh): Add additional branch-name heuristic here */
if [ "${BUILD_PAT:-0}" -ne 0 ]; then
    debug_print " ${BUILD_PAT:-} post-tag patches found."
    debug_print "  Setting release type to 'X'"
    BUILD_RLT="X"
fi

debug_print "End ver fixup and exceptions"

##############################################################################
# Construct PROM string VER_H_PROM_OSA_TEXT

debug_print "Start VER_H_PROM_OSA_TEXT processing"
VER_H_PROM_OSA_UNPADDED_TEXT="${BUILD_SAR:-Unknown}"
VER_H_PROM_OSA_TEXT="\"$(printf '%-20s\n' \
    "${VER_H_PROM_OSA_UNPADDED_TEXT:?}" |
        cut -b 1-20 2> /dev/null)\""
debug_print "VER_H_PROM_OSA_TEXT is ${VER_H_PROM_OSA_TEXT:-}"
debug_print "End VER_H_PROM_OSA_TEXT processing"

##############################################################################
# Construct PROM string VER_H_PROM_OSV_TEXT

debug_print "Start VER_H_PROM_OSV_TEXT processing"
VER_H_PROM_OSV_UNPADDED_TEXT="${BUILD_SNM:-Unknown} ${BUILD_SVR:-}"
VER_H_PROM_OSV_TEXT="\"$(printf '%-20s\n' \
    "${VER_H_PROM_OSV_UNPADDED_TEXT:?}" |
        cut -b 1-20 2> /dev/null)\""
debug_print "VER_H_PROM_OSV_TEXT is ${VER_H_PROM_OSV_TEXT:-}"
debug_print "End VER_H_PROM_OSV_TEXT processing"

##############################################################################
# Construct PROM string PROM_SHIP

debug_print "Start VER_H_PROM_SHIP processing"
PROM_SHIP="$(printf '%s\n' "${BUILD_DTE:?Error: BUILD_DTE unset.}" |
    cut -d ' ' -f 1 2> /dev/null |
        tr -d '\ \-' 2> /dev/null |
          cut -c 3- 2> /dev/null)"
debug_print "End VER_H_PROM_SHIP processing"

##############################################################################
# Munge herald and ver string

debug_print "Start BUILD_VER fixup"

BUILD_VER="$(printf '%s\n' "${BUILD_VER:-0.0.0}" |
    sed -e 's/^[AbBbCcDdRrXxZz]//')"
BUILD_VER="${BUILD_RLT:-X}${BUILD_VER:-0.0.0}"
debug_print "BUILD_VER is ${BUILD_VER:-}"
debug_print "End BUILD_VER fixup"

##############################################################################
# Write out information or complain with an error.

debug_print "Start write out ver.h file."
FALLBACK_SHA="0000000000000000000000000000000000000000"

# shellcheck disable=SC1003
printf '%s\n'                                                                \
  '/* NOTICE: Auto-generated by "make_ver.sh" */'                            \
  ""                                                                         \
  "#ifndef GENERATED_MAKE_VER_H"                                             \
  "# define GENERATED_MAKE_VER_H"                                            \
  ""                                                                         \
  "# define VER_H_GIT_DATE           \"${BUILD_DTE:-2020-01-01 00:00 UTC}\"" \
  "# define VER_H_GIT_DATE_SHORT     \"${BUILD_DTE_SHORT:-2020-01-01}\""     \
  "# define VER_H_GIT_RELT           \"${BUILD_RLT:-X}\""                    \
  "# define VER_H_GIT_VERSION        \"${BUILD_VER:-X0.0.0}\""               \
  "# define VER_H_GIT_PATCH          \"${BUILD_PAT:-9999}\""                 \
  "# define VER_H_GIT_PATCH_INT      ${BUILD_PAT:-9999}"                     \
  "# define VER_H_GIT_HASH           \"${BUILD_SHA:-${FALLBACK_SHA:?}}\""    \
  "# define VER_H_PREP_DATE          \"${BUILD_UTC:-2020-01-01}\""           \
  "# define VER_H_PREP_USER          \"${BUILD_USR:-Unknown}\""              \
  "# define VER_H_PREP_OSNM          \"${BUILD_SNM:-Unknown}\""              \
  "# define VER_H_PREP_OSVR          \"${BUILD_SVR:-Unknown}\""              \
  "# define VER_H_PREP_OSAR          \"${BUILD_SAR:-Unknown}\""              \
  "# define VER_H_PREP_OSVN          \"${BUILD_SYS:-Unknown}\""              \
  "# define VER_H_PROM_VER_TEXT      ${VER_H_PROM_VER_TEXT:?}"               \
  "# define VER_H_PROM_OSV_TEXT      ${VER_H_PROM_OSV_TEXT:?}"               \
  "# define VER_H_PROM_OSA_TEXT      ${VER_H_PROM_OSA_TEXT:?}"               \
  "# define VER_H_PROM_SHIP          \"${PROM_SHIP:-200101}\""               \
  "# define VER_H_PROM_SHIP_INT      ${PROM_SHIP:-200101}"                   \
  "# define VER_H_PROM_MAJOR_VER     \"${VER_H_PROM_MAJOR_VER_FMT:-999}\""   \
  "# define VER_H_PROM_MAJOR_VER_INT ${VER_H_PROM_MAJOR_VER:-999}"           \
  "# define VER_H_PROM_MINOR_VER     \"${VER_H_PROM_MINOR_VER_FMT:-999}\""   \
  "# define VER_H_PROM_MINOR_VER_INT ${VER_H_PROM_MINOR_VER:-999}"           \
  "# define VER_H_PROM_PATCH_VER     \"${VER_H_PROM_PATCH_VER_FMT:-999}\""   \
  "# define VER_H_PROM_PATCH_VER_INT ${VER_H_PROM_PATCH_VER:-999}"           \
  "# define VER_H_PROM_OTHER_VER     \"${VER_H_PROM_OTHER_VER_FMT:-9999}\""  \
  "# define VER_H_PROM_OTHER_VER_INT ${VER_H_PROM_OTHER_VER:-999}"           \
  ""                                                                         \
  "#endif /* GENERATED_MAKE_VER_H */"                                        \
  > "./ver.h.${$}" ||
      { # /* printf > ./ver.h.${$} */
          printf >&2 '%s\n' \
              "Error: writing ver.h.${$} failed."
          rm -f "./ver.h" > /dev/null 2>&1 || \
              true > /dev/null 2>&1
          rm -f "./ver.h.${$}" > /dev/null 2>&1 || \
              true > /dev/null 2>&1
          exit 0
      } # /* printf > ./ver.h.${$} */
  mv -f "./ver.h.${$}" "./ver.h"
debug_print "Successfully wrote ver.h.${$} file and moved to ver.h."

##############################################################################

debug_print "End of make_ver.sh processing."

# Local Variables:
# mode: sh
# sh-shell: sh
# sh-indentation: 4
# sh-basic-offset: 4
# tab-width: 4
# End:
