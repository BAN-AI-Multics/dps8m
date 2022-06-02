#!/usr/bin/env bash
# Converts dps8 flat help to PFL; requires GNU tooling.
set -e

rm -f ./temp1.tmp > /dev/null 2>&1 || true
rm -f ./temp2.tmp > /dev/null 2>&1 || true

( ( echo HELP | ../src/dps8/dps8 -q | tail -n +2 | grep -v '^$' | tr -s ' ' | tr ' ' '\n' | grep -v '^$' | grep -vw 'HELP' | sort | xargs -I{} echo echo\ \;echo\ \XXXXXXXXXXXX\ {}\;echo help\ -f\ {}\|../src/dps8/dps8\ -q\;echo\;echo |sh) | grep -Ev '(DPS8/M help.$|^L68 help.$)' ) | expand | sed -e 's/XXXXXXXXXXXX/#/' -e 's/^1\.1\.//' -e 's/^[0-9]\+ /## /' -e 's/^[0-9]\+\.[0-9]\+ /## /' -e 's/^[0-9]\+\.[0-9]\+\.[0-9]\+ /### /' -e 's/^[0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+ /#### /' -e 's/^#/##/' -e 's/^    //' -e '' -e 's/^        /             /' -e '/^[[:alnum:]]/s/</\\</g' -e '/^[[:alnum:]]/s/>/\\>/g' -e '/^[[:alnum:]]/s/_/\\_/g' | ansifilter -T | tee temp2.tmp && printf '%s\n\n' '<!-- pagebreak -->' '# Simulator Command Reference' 'This chapter provides a reference for simulator commands.' '* This information is also available from within the simulator, accessible by using the `HELP` command.' '' > temp1.tmp && cat temp2.tmp >> temp1.tmp && rm -f temp2.tmp && mv -f temp1.tmp commandref.md
