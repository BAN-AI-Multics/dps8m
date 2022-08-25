#!/usr/bin/env bash
# shellcheck disable=SC2015
# vim: filetype=sh:tabstop=4:expandtab
# vim: ruler:hlsearch:incsearch:autoindent:wildmenu:wrapscan
# SPDX-License-Identifier: FSFAP
# scspell-id: 3f9cac60-f632-11ec-9ef6-80ee73e9b8e7

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

# Convert dps8 flat help to Pandoc-accepted Markdown; requires GNU tooling.

############################################################################

# Exit immediately on lock failure
test "$(uname -s 2> /dev/null)" = "Linux" && {                             \
FLOCK_COMMAND="$( command -v flock 2> /dev/null )" && {                    \
[ "${FLOCKER:-}" != "${0}" ] && exec env                                   \
FLOCKER="${0}" "${FLOCK_COMMAND:?}" -en "${0}" "${0}" "${@}" || : ; } ; }

############################################################################

unset FLOCKER               > "/dev/null" 2>&1 || true
unset FLOCK_COMMAND         > "/dev/null" 2>&1 || true

############################################################################

set -e

############################################################################

rm -f "./md/commandref.md"  > /dev/null 2>&1 || true
rm -f "./temp1.tmp"         > /dev/null 2>&1 || true
rm -f "./temp2.tmp"         > /dev/null 2>&1 || true

############################################################################

# shellcheck disable=SC1001,SC2016
( ( echo HELP | "../src/dps8/dps8" -t -q | tail -n +2 | grep -v '^$' | tr -s ' ' | tr ' ' '\n' | grep -v '^$' | grep -vw 'HELP' | sort | xargs -I{} echo echo\ \;echo\ \XXXXXXXXXXXX\ {}\;echo help\ -f\ {}\|../src/dps8/dps8\ -t\ -q\;echo\;echo |sh) | grep -Ev '(DPS8/M help.$|^L68 help.$)' ) | expand | sed -e 's/XXXXXXXXXXXX/#/' -e 's/^1\.1\.//' -e 's/^[0-9]\+ /## /' -e 's/^[0-9]\+\.[0-9]\+ /## /' -e 's/^[0-9]\+\.[0-9]\+\.[0-9]\+ /### /' -e 's/^[0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+ /#### /' -e 's/^#/##/' -e 's/^    //' -e '' -e 's/^        /             /' -e '/^[[:alnum:]]/s/</\\</g' -e '/^[[:alnum:]]/s/>/\\>/g' -e '/^[[:alnum:]]/s/_/\\_/g' | ansifilter -T | tee "./temp2.tmp" && printf '%s\n\n' '<!-- pagebreak -->' '# Simulator Command Reference' 'This chapter provides reference documentation for the **DPS8M** simulator command set.' '* This information is also available from within the simulator; it is accessible by using the interactive `HELP` command.' '' '<!-- br -->' '' > "./temp1.tmp" && sed -e 's/^## /\n\n<!-- br -->\n\n## /g' -e 's/^\* \* /   * /g' < "./temp2.tmp" >> "./temp1.tmp" && rm -f "./temp2.tmp" && mv -f "./temp1.tmp" "./md/commandref.md"

############################################################################