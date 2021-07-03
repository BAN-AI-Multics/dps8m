#!/usr/bin/env sh
# shellcheck disable=SC2236,SC3040,SC2015
# vim: filetype=sh:tabstop=4:tw=76

###############################################################################
# License

# Copyright (c) 2021 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2021 The DPS8M Development Team
#
# All rights reserved.
#
# This software is made available under the terms of the ICU
# License, version 1.8.1 or later.  For more details, see the
# LICENSE file at the top-level directory of this distribution.

###############################################################################
# Check for csh as sh.

# shellcheck disable=SC2006,SC2046,SC2065
test _$(printf '%s' "asdf" 2> /dev/null) != "_asdf" > /dev/null &&
	printf '%s\n' \
		"Error: This shell seems to be csh, which is not supported." &&
	exit 1

###############################################################################
# Sanity check

MAKE="${MAKE:?Error: MAKE undefined}"

###############################################################################
# Attempt to disable pedantic verification during environment normalization.

set +e > /dev/null 2>&1
set +u > /dev/null 2>&1

###############################################################################
# Attempt to setup a standard POSIX locale.

eval export $(command -p env -i locale 2> /dev/null) > /dev/null 2>&1 || true
DUALCASE=1 && export DUALCASE || true
LC_ALL=C && export LC_ALL || true
LANGUAGE=C && export LANGUAGE || true
LANG=C && export LANG || true
(unset CDPATH > /dev/null 2>&1) > /dev/null 2>&1 || true
unset CDPATH > /dev/null 2>&1 || true

###############################################################################
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

###############################################################################
# Sanitization for bash.

elif [ ".${BASH_VERSION:-}" != "." ] &&
	(set -o posix 2> /dev/null) > /dev/null 2>&1; then
	set -o posix > /dev/null 2>&1 || true
	set -o pipefail > /dev/null 2>&1 || true
	POSIXLY_CORRECT=1 && export POSIXLY_CORRECT || true
	POSIX_ME_HARDER=1 && export POSIX_ME_HARDER || true
	unalias -a > /dev/null 2>&1 || true > /dev/null 2>&1
fi

###############################################################################
# Clear environment.

for as_var in BASH_ENV ENV MAIL MAILPATH; do
	# shellcheck disable=SC1083
	eval test x\${"${as_var:?}"+set} = xset &&
		( # /* xset */
			(unset "${as_var:-}") ||
				{ # /* unset */
					printf >&2 '%s\n' \
						"Error: clear environment failed."
					exit 1
				}
		) &&
		unset "${as_var:-}" || true
done # /* as_var */

###############################################################################
# Set UTC timezone.

{ TZ="UTC" && export TZ; } ||
	{
		printf >&2 '%s\n' \
			"Error: Exporting TZ to environment failed."
		exit 1
	}

###############################################################################
# Set format string for dates.

{ DATEFORMAT="%Y-%m-%d %H:%M:%S ${TZ:?}" && export DATEFORMAT; } ||
	{
		printf >&2 '%s\n' \
			"Error: Exporting DATEFORMAT to environment failed."
		exit 1
	}

###############################################################################
# Attempt to enable pedantic error checking.

set -u > /dev/null 2>&1
set -e > /dev/null 2>&1

###############################################################################
# get_git_ptch() returns the number of additional commits from the last tag.

get_git_ptch()
{ # /* get_git_ptch() */
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
							exit 1
						} # /* GITVER */
				GITPAT=$(git describe --abbrev=40 --dirty='*' \
					--tags --always 2> /dev/null) ||
						{ # /* GITPAT */
							printf '%s\n' \
								"Error: git describe --abbrev=40 failed."
							exit 1
						} # /* GITPAT */
				OIFS="$IFS"
				# shellcheck disable=SC2086
				PATDIF=$(printf '%s\n' ${GITPAT:-XXXXXX} | \
					sed -e "s/$(printf '%s' ${GITVER:-XXXXXX} | \
					sed -e 's/*//')//" | sed -e 's/\(^.*-[0-9]+-g.*\)/\1/' | \
					sed	-e "s/${GITVER:-XXXXXX}//" | cut -d '-' -f 2) ||
					{ # /* PATDIF */
						IFS="${OIFS:?}" || true
						printf >&2 '%s\n' \
							"Error: sed/cut transform failed."
						exit 1
					}
			IFS="${OIFS:?}"
			fi # /* GITTEST */ 
		else # /* GIT */
			printf >&2 '%s\n' \
				"Error: git version failed."
			exit 1
		fi
	else # /* TRUE */
		printf >&2 '%s\n' \
			"Error: command true failed."
		exit 1
	fi
	if [ -n "${PATDIF:-}" ] &&
		[ ! -z "${PATDIF:-}" ]; then
		printf '%s\n' \
			"${PATDIF:?}"
	else
		exit 1
	fi
} # /* get_git_ptch() */

###############################################################################
# get_git_date returns the date as of the current git commit.

get_git_date()
{ # /* get_git_date() */
	if command -p true 2> /dev/null 1>&2; then # /* TRUE */
		if command git version 2> /dev/null 1>&2; then # /* GIT */
			GITTEST=$(git version 2> /dev/null)
			if [ -n "${GITTEST:-}" ] &&
				[ ! -z "${GITTEST:-}" ]; then # /* GITTEST */
				# /* NOTE(jhj): SQ-QUOTE'd output! */
				GIT_SHA=$(git rev-parse --sq --verify '@' 2> /dev/null) ||
					{ # /* GIT_SHA */
						printf >&2 '%s\n' \
							"Error: git rev-parse --sq failed."
						exit 1
					}
			fi # /* GITTEST */ 
		else # /* GIT */
			printf >&2 '%s\n' \
				"Error: git version failed."
			exit 1
		fi
	else # /* TRUE */
		printf >&2 '%s\n' \
			"Error: command true failed."
		exit 1
	fi
	if [ -n "${GIT_SHA:-}" ] &&
		[ ! -z "${GIT_SHA:-}" ]; then
		# /* NOTE(jhj): Safe to eval SQ-QUOTE'd output! */
		eval printf '%s\n' \
			"${GIT_SHA:?}"
	else
		exit 1
	fi
} # /* get_git_date() */

###############################################################################
# get_git_vers() returns a formatted version from the current git environment.

# TODO(jhj): Document examples!

get_git_vers()
{ # /* get_git_vers() /*
	if command -p true 2> /dev/null 1>&2; then # /* TRUE */
		if command git version 2> /dev/null 1>&2; then # /* GIT */
			GITTEST=$(git version 2> /dev/null)
			if [ -n "${GITTEST:-}" ] &&
				[ ! -z "${GITTEST:-}" ]; then # /* GITTEST */
				BRANCH=$(git branch --show-current 2> /dev/null)
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
			exit 1
		fi # /* endif GIT */
	else # /* TRUE */
		printf >&2 '%s\n' \
			"Error: command true failed."
		exit 1
	fi # /* endif TRUE */

	GIT_SOURCE_INFO="${GIT_OUT:-0.0.0}"
	GIT_SOURCE_XFRM=$(printf '%s\n' "${GIT_SOURCE_INFO:?}" |
		sed -e 's/^R//' 2> /dev/null) ||
			{ # /* sed */
				printf >&2 '%s\n' \
					"Error: sed transform failed."
				exit 1
			}

	if [ -n "${GIT_SOURCE_XFRM:-}" ] &&
		[ ! -z "${GIT_SOURCE_XFRM:-}" ]; then
		printf '%s\n' \
			"${GIT_SOURCE_XFRM:?}"
	else
		printf '%s\n' \
			"${GIT_SOURCE_INFO:?}"
	fi
} # /* get_git_vers() */

###############################################################################
# get_git_hash() returns the SHA-1 hash of the current git commit.

get_git_hash()
{ # /* get_git_hash() */
	if command -p true 2> /dev/null 1>&2; then # /* TRUE */
		if command git version 2> /dev/null 1>&2; then # /* GIT */
			GITTEST=$(git version 2> /dev/null)
			if [ -n "${GITTEST:-}" ] &&
				[ ! -z "${GITTEST:-}" ]; then # /* GITTEST */
				# /* NOTE(jhj): Require SQ-QUOTE'd output */
				GIT_SHA=$(git rev-parse --sq --verify '@' 2> /dev/null) ||
					{ # /* GIT_SHA */
						printf >&2 '%s\n' \
							"Error: git rev-parse --sq failed."
						exit 1
					}
			fi # /* GITTEST */ 
		else # /* GIT */
			printf >&2 '%s\n' \
				"Error: git version failed."
			exit 1
		fi
	else # /* TRUE */
		printf >&2 '%s\n' \
			"Error: command true failed."
		exit 1
	fi
	if [ -n "${GIT_SHA:-}" ] &&
		[ ! -z "${GIT_SHA:-}" ]; then
		# /* NOTE(jhj): Safe to eval only if SQ-QUOTE'd */
		eval printf '%s' \
			"${GIT_SHA:?}"
	else
		exit 1
	fi
} # /* get_git_hash() */

###############################################################################
# get_git_date() returns the date in UTC of the current git commit.

get_git_date()
{ # /* get_git_date() /*
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
							exit 1
						} # /* CDDATE */
				GIT_DATE_OUT="$(printf '%s' \
					"${CDDATE:?}")"
			fi # /* GITTEST */
		else # /* GIT */
			printf >&2 '%s\n' \
				"Error: git version failed."
			exit 1
		fi # /* endif GIT */
	else # /* TRUE */
		printf >&2 '%s\n' \
			"Error: command true failed."
		exit 1
	fi # /* endif TRUE */

	printf '%s\n' \
		"${GIT_DATE_OUT:?}"
} # /* get_git_date() */

###############################################################################
# get_utc_date() returns the current date in UTC format or placeholder text.

get_utc_date()
{ # /* get_utc_date() */
	UTC_BLD_DATE=$(command -p env -i TZ=UTC \
		date -u "+${DATEFORMAT:?}" 2> /dev/null) ||
			{ # /* UTC_BLD_DATE */
				printf >&2 '%s\n' \
					"Error: date failed."
				exit 1
			}
	if [ -n "${UTC_BLD_DATE:-}" ] &&
		[ ! -z "${UTC_BLD_DATE:-}" ]; then
		UTC_BLD_DATE_INFO="${UTC_BLD_DATE:?}"
	else # /* UTC_BLD_DATE_INFO */
		UTC_BLD_DATE_INFO="" # /* TODO(jhj): Add placeholder text! */
	fi

	printf '%s\n' \
		"${UTC_BLD_DATE_INFO:-}"
} # /* get_utc_date() */

###############################################################################
# Gather information or complain with an error.

BUILD_VER="$(get_git_vers)" ||
	{ # /* BUILD_VER */
		printf >&2 '%s\n' \
			"Error: get_git_vers() failed."
		exit 1
	} # /* BUILD_VER */

BUILD_DTE="$(get_git_date)" ||
	{ # /* BUILD_DTE */
		printf >&2 '%s\n' \
			"Error: get_git_date() failed."
		exit 1
	} # /* BUILD_DTE */

BUILD_UTC="$(get_utc_date)" ||
	{ # /* BUILD_UTC */
		printf >&2 '%s\n' \
			"Error: get_utc_date() failed."
		exit 1
	} # /* BUILD_UTC */

BUILD_SHA="$(get_git_hash)" ||
	{ # /* BUILD_SHA */
		printf >&2 '%s\n' \
			"Error: get_git_hash() failed."
		exit 1
	} # /* BUILD_SHA */

BUILD_PAT="$(get_git_ptch)" ||
	{ # /* BUILD_PAT */
		printf >&2 '%s\n' \
			"Error: get_git_ptch() failed."
		exit 1
	} # /* BUILD_PAT */

###############################################################################
# Write out information or complain with an error.

# shellcheck disable=SC1003
printf '%s\n' \
	'/* NOTICE: Auto-generated by "make_ver.sh" */' \
	"/* CHANGES MADE TO THIS FILE MAY BE LOST!! */" \
	"" \
	"#ifndef GENERATED_MAKE_VER_H" \
	"#define GENERATED_MAKE_VER_H" \
	"" \
	'#define VER_H_GIT_DATE \' \
	"    \"${BUILD_DTE:-1900-01-01}\"" \
	"" \
	'#define VER_H_GIT_VERSION \' \
	"    \"${BUILD_VER:-0.0.0}\"" \
	"" \
	'#define VER_H_GIT_PATCH \' \
	"    \"${BUILD_PAT:-9999}\"" \
	"" \
	'#define VER_H_GIT_HASH \' \
	"    \"${BUILD_SHA:-0000000000000000000000000000000000000000}\"" \
	"" \
	'#define VER_H_PREP_DATE \' \
	"    \"${BUILD_UTC:-1900-01-01}\"" \
	"" \
	"#endif /* GENERATED_MAKE_VER_H */" \
	> "./ver.h" ||
	{ # /* printf > ./ver.h */
		printf >&2 '%s\n' \
			"Error: writing ver.h failed."
		rm -f "./ver.h" > /dev/null 2>&1 || true
		exit 1
	} # /* printf > ./ver.h */

###############################################################################

# Local Variables:
# mode: sh
# sh-shell: sh
# sh-indentation: 4
# sh-basic-offset: 4
# tab-width: 4
# End:
