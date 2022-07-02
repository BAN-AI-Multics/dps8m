#!/usr/bin/env bash
# vim: filetype=sh:tabstop=4:expandtab
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

# Converts dps8 flat help to Pandoc-flavored Markdown; requires GNU tooling.

############################################################################

set -e

############################################################################

rm -f ./commandref.md > /dev/null 2>&1 || true
rm -f ./temp1.tmp     > /dev/null 2>&1 || true
rm -f ./temp2.tmp     > /dev/null 2>&1 || true

############################################################################

# shellcheck disable=SC1001,SC2016
( ( echo HELP | ../src/dps8/dps8 -t -q | tail -n +2 | grep -v '^$' | tr -s ' ' | tr ' ' '\n' | grep -v '^$' | grep -vw 'HELP' | sort | xargs -I{} echo echo\ \;echo\ \XXXXXXXXXXXX\ {}\;echo help\ -f\ {}\|../src/dps8/dps8\ -t\ -q\;echo\;echo |sh) | grep -Ev '(DPS8/M help.$|^L68 help.$)' ) | expand | sed -e 's/XXXXXXXXXXXX/#/' -e 's/^1\.1\.//' -e 's/^[0-9]\+ /## /' -e 's/^[0-9]\+\.[0-9]\+ /## /' -e 's/^[0-9]\+\.[0-9]\+\.[0-9]\+ /### /' -e 's/^[0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+ /#### /' -e 's/^#/##/' -e 's/^    //' -e '' -e 's/^        /             /' -e '/^[[:alnum:]]/s/</\\</g' -e '/^[[:alnum:]]/s/>/\\>/g' -e '/^[[:alnum:]]/s/_/\\_/g' | ansifilter -T | tee temp2.tmp && printf '%s\n\n' '<!-- pagebreak -->' '# Simulator Command Reference' 'This chapter provides a reference for simulator commands.' '* This information is also available from within the simulator, accessible by using the `HELP` command.' '' > temp1.tmp && cat temp2.tmp >> temp1.tmp && rm -f temp2.tmp && mv -f temp1.tmp commandref.md

############################################################################
