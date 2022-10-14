#!/usr/bin/env bash
# shellcheck disable=SC2015
# vim: filetype=sh:tabstop=4:ai:expandtab
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

# Convert dps8 flat help to Pandoc-accepted Markdown; requires GNU tooling!!

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

rm -f "./md/_cmdout.md"      > /dev/null 2>&1 || true
rm -f "./md/showtemp.md"     > /dev/null 2>&1 || true
rm -f "./md/commandref.md"   > /dev/null 2>&1 || true
rm -f "./temp1.tmp"          > /dev/null 2>&1 || true
rm -f "./temp2.tmp"          > /dev/null 2>&1 || true
rm -f "./pdf/neato.pdf"      > /dev/null 2>&1 || true
rm -f "./pdf/storage.pdf"    > /dev/null 2>&1 || true
rm -f "./pdf/iomcon.pdf"     > /dev/null 2>&1 || true
rm -f "./pdf/gvsubset.pdf"   > /dev/null 2>&1 || true

############################################################################

cp -f "./md/showcommands.md" "./md/showtemp.md"

############################################################################
# Command Reference chapter

# shellcheck disable=SC1001,SC2016
( ( echo HELP | "../src/dps8/dps8" -t -q | tail -n +2 | grep -v '^$' | tr -s ' ' | tr ' ' '\n' | grep -v '^$' | grep -vwE '(HELP|IGNORE|BREAK|NOBREAK|EXPECT|NOEXPECT|SEND|EXAMINE|DEPOSIT|IEXAMINE|IDEPOSIT|XF|RESTART|AI|AI2|UNCABLE|CABLE_SHOW|CABLE_RIPOUT|NOLOCALOPC|FNPSERVER3270PORT|UNLOAD|NOLUF)' | sort | xargs -I{} echo echo\ \;echo\ \XXXXXXXXXXXX\ {}\;echo help\ -f\ {}\|../src/dps8/dps8\ -t\ -q\;echo\;echo |sh) | grep -Ev '(DPS8/M help.$|^L68 help.$)' ) | expand | sed -e 's/XXXXXXXXXXXX/#/' -e 's/^1\.1\.//' -e 's/^[0-9]\+ /## /' -e 's/^[0-9]\+\.[0-9]\+ /## /' -e 's/^[0-9]\+\.[0-9]\+\.[0-9]\+ /### /' -e 's/^[0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+ /#### /' -e 's/^#/##/' -e 's/^    //' -e '' -e 's/^        /             /' -e '/^[[:alnum:]]/s/</\\</g' -e '/^[[:alnum:]]/s/>/\\>/g' -e '/^[[:alnum:]]/s/_/\\_/g' | ansifilter -T | tee "./temp2.tmp" && printf '%s\n\n' '<!-- pagebreak -->' '# Simulator Command Reference' 'This chapter provides reference documentation for the **DPS8M** simulator command set.' '* This information is also available from within the simulator; it is accessible by using the interactive `HELP` command.' '' '<!-- br -->' '' > "./temp1.tmp" && sed -e 's/^## /\n\n<!-- br -->\n\n## /g' -e 's/^\* \* /   * /g' < "./temp2.tmp" >> "./temp1.tmp" && rm -f "./temp2.tmp" && mv -f "./temp1.tmp" "./md/commandref.md"

############################################################################
# Replacements

# AUTOINPUT
sed -e '/^Set primary console auto-input$/ {' -e 'r md/autoinput.md' -e 'd' -e '}' -i "./md/commandref.md"
# AUTOINPUT2
sed -e '/^Set secondary console auto-input$/ {' -e 'r md/autoinput2.md' -e 'd' -e '}' -i "./md/commandref.md"
# CLRAUTOINPUT
sed -e '/^Clear primary console auto-input$/ {' -e 'r md/clrautoinput.md' -e 'd' -e '}' -i "./md/commandref.md"
# CLRAUTOINPUT2
sed -e '/^Clear secondary console auto-input$/ {' -e 'r md/clrautoinput2.md' -e 'd' -e '}' -i "./md/commandref.md"
# CABLE
sed -e '/^String a cable.$/ {' -e 'r md/cable.md' -e 'd' -e '}' -i "./md/commandref.md"
# SET
sed -e '/^See the Omnibus documentation for a complete SET command reference.$/ {' -e 'r md/setcommands.md' -e 'd' -e '}' -i "./md/commandref.md"
# FNPSERVERADDRESS
sed -e '/^Set the FNP dialin server binding address$/ {' -e 'r md/fnpserveraddress.md' -e 'd' -e '}' -i "./md/commandref.md"
# FNPSERVERPORT
sed -e '/^Set the FNP dialin TELNET port number$/ {' -e 'r md/fnpserverport.md' -e 'd' -e '}' -i "./md/commandref.md"
# LUF
sed -e '/^Enable normal LUF handling$/ {' -e 'r md/luf.md' -e 'd' -e '}' -i "./md/commandref.md"
# LOAD
sed -e '/^Mount disk or tape image and signal Multics$/ {' -e 'r md/load.md' -e 'd' -e '}' -i "./md/commandref.md"

####################################################################################################
# SHOW BUILDINFO
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW BUILDINFO' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW BUILDINFO" | ../src/dps8/dps8 -q -t | fmt -w80 | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWBUILDINFOHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW CLOCKS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW CLOCKS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW CLOCKS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWCLOCKSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW DEVICES
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW DEVICES' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW DEVICES" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    grep -v 'simulator configuration' | grep -v '^$' | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWDEVICESHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW PROM
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW PROM' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW PROM" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    grep -v '^$' | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWPROMHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW VERSION
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW VERSION' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW VERSION" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWVERSIONHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW CPU CONFIG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW CPU0 CONFIG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW CPU0 CONFIG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWCPUCONFIGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW CPU NUNITS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW CPU NUNITS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW CPU NUNITS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWCPUNUNITSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW CPU KIPS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW CPU KIPS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW CPU KIPS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWCPUKIPSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW CPU STALL
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' "sim> SET CPU STALL=0=132:3737=12500" >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW CPU STALL' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SET CPU STALL=0=132:3737=12500" "SHOW CPU STALL" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWCPUSTALLHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW CPU DEBUG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW CPU DEBUG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW CPU DEBUG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWCPUDEBUGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW DISK NAME
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW DISK0 NAME' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW DISK0 NAME" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWDISKNAMEHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW DISK NUNITS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW DISK NUNITS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW DISK NUNITS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWDISKNUNITSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW DISK TYPE
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW DISK TYPE' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW DISK TYPE" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWDISKTYPEHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW DISK DEBUG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW DISK DEBUG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW DISK DEBUG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWDISKDEBUGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW DISK DEBUG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW FNP0 CONFIG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW FNP0 CONFIG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWFNPCONFIGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW FNP0 IPC_NAME
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW FNP0 IPC_NAME' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW FNP0 IPC_NAME" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWFNPIPCNAMEHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW FNP0 NAME
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW FNP0 NAME' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW FNP0 NAME" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWFNPNAMEHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW FNP NUNITS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW FNP NUNITS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW FNP NUNITS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWFNPNUNITSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW FNP SERVICE
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW FNP0 SERVICE' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW FNP0 SERVICE" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' | head -n 11 >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWFNPSERVICEHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW FNP STATUS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW FNP0 STATUS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW FNP0 STATUS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' | head -n 27 >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWFNPSTATUSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW FNP DEBUG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW FNP DEBUG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW FNP DEBUG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWFNPDEBUGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW IOM CONFIG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW IOM0 CONFIG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW IOM0 CONFIG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWIOMCONFIGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW IOM NUNITS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW IOM NUNITS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW IOM NUNITS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWIOMNUNITSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW IOM DEBUG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW IOM DEBUG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW IOM DEBUG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWIOMDEBUGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW IPC NAME
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW IPC0 NAME' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW IPC0 NAME" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWIPCNAMEHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW IPC NUNITS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW IPC NUNITS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW IPC NUNITS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWIPCNUNITSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW MSP NAME
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW MSP0 NAME' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW MSP0 NAME" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWMSPNAMEHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW MSP NUNITS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW MSP NUNITS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW MSP NUNITS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWMSPNUNITSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW MTP BOOT_DRIVE
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW MTP0 BOOT_DRIVE' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW MTP0 BOOT_DRIVE" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWMTPBOOTDRIVEHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW MTP NAME
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW MTP0 NAME' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW MTP0 NAME" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWMTPNAMEHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW MTP NUNITS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW MTP NUNITS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW MTP NUNITS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWMTPNUNITSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW MTP DEBUG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW MTP DEBUG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW MTP DEBUG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWMTPDEBUGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW OPC CONFIG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW OPC0 CONFIG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW OPC0 CONFIG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWOPCCONFIGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW OPC ADDRESS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW OPC0 ADDRESS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW OPC0 ADDRESS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWOPCADDRESSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW OPC AUTOINPUT
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW OPC0 AUTOINPUT' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW OPC0 AUTOINPUT" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWOPCAUTOINPUTHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW OPC NAME
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW OPC0 NAME' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW OPC0 NAME" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWOPCNAMEHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW OPC NUNITS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW OPC NUNITS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW OPC NUNITS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWOPCNUNITSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW OPC PORT
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW OPC0 PORT' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW OPC0 PORT" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWOPCPORTHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW OPC PW
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW OPC0 PW' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW OPC0 PW" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWOPCPWHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW OPC DEBUG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW OPC DEBUG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW OPC DEBUG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWOPCDEBUGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW PRT CONFIG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW PRT0 CONFIG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW PRT0 CONFIG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWPRTCONFIGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW PRT MODEL
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW PRT0 MODEL' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW PRT0 MODEL" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWPRTMODELHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW PRT NAME
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW PRT0 NAME' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW PRT0 NAME" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWPRTNAMEHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW PRT NUNITS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW PRT NUNITS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW PRT NUNITS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWPRTNUNITSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW PRT PATH
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW PRT PATH' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW PRT PATH" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWPRTPATHHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW PRT DEBUG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW PRT DEBUG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW PRT DEBUG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWPRTDEBUGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW PRT DEBUG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW PUN0 CONFIG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW PUN0 CONFIG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWPUNCONFIGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW PUN NAME
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW PUN0 NAME' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW PUN0 NAME" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWPUNNAMEHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW PUN NUNITS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW PUN NUNITS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW PUN NUNITS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWPUNNUNITSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW PUN PATH
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW PUN PATH' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW PUN PATH" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWPUNPATHHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW PUN DEBUG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW PUN DEBUG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW PUN DEBUG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWPUNDEBUGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW RDR NAME
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW RDR0 NAME' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW RDR0 NAME" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWRDRNAMEHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW RDR NUNITS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW RDR NUNITS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW RDR NUNITS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWRDRNUNITSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW RDR PATH
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW RDR PATH' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW RDR PATH" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWRDRPATHHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW RDR DEBUG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW RDR DEBUG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW RDR DEBUG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWRDRDEBUGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW SCU CONFIG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW SCU0 CONFIG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW SCU0 CONFIG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWSCUCONFIGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW SCU NUNITS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW SCU NUNITS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW SCU NUNITS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWSCUNUNITSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW SCU STATE
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW SCU0 STATE' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW SCU0 STATE" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWSCUSTATEHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW SCU DEBUG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW SCU DEBUG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW SCU DEBUG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWSCUDEBUGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW TAPE ADD_PATH
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW TAPE ADD_PATH' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW TAPE ADD_PATH" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWTAPEADDPATHHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW TAPE CAPACITY
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW TAPE0 CAPACITY' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW TAPE0 CAPACITY" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWTAPECAPACITYHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW TAPE DEFAULT_PATH
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW TAPE DEFAULT_PATH' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW TAPE DEFAULT_PATH" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWTAPEDEFAULTPATHHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW TAPE NAME
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW TAPE0 NAME' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW TAPE0 NAME" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWTAPENAMEHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW TAPE NUNITS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW TAPE NUNITS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW TAPE NUNITS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWTAPENUNITSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW TAPE DEBUG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW TAPE DEBUG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW TAPE DEBUG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWTAPEDEBUGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW URP NAME
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW URP0 NAME' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW URP0 NAME" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWURPNAMEHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW URP NUNITS
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW URP NUNITS' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW URP NUNITS" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWURPNUNITSHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# SHOW URP DEBUG
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> SHOW URP DEBUG' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "SHOW URP DEBUG" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWURPDEBUGHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/showtemp.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# CABLE SHOW
printf '%s\n' '```dps8' >> "./md/_cmdout.md"
printf '%s\n' 'sim> CABLE SHOW' >> "./md/_cmdout.md"
## Note: Unicode space used with sed!
printf '%s\n' "CABLE SHOW" | ../src/dps8/dps8 -q -t | ansifilter -T | expand | \
    sed 's/^ / /' >> "./md/_cmdout.md"
printf '%s\n' '```' >> "./md/_cmdout.md"
cat ./md/_cmdout.md
sed -e '/^SHOWCABLESHOWHERE$/ {' -e 'r md/_cmdout.md' -e 'd' -e '}' -i "./md/commandref.md"
rm -f "./md/_cmdout.md" 2> /dev/null 2>&1

####################################################################################################
# CABLE GRAPH NEATO
printf '%s\n' "CABLE GRAPH" | ../src/dps8/dps8 -q -t | neato -Goverlap=prism -Gmode="ipsep" \
  -Gsep="+6" -Gratio=1.30 -Gsplines=true -Gconcentrate=true -Tpdf -o pdf/neato.pdf

####################################################################################################
# CABLE GRAPH SUBSET
( echo "CABLE_RIPOUT"   ; echo "SHOW DEFAULT"  | ../src/dps8/dps8 -q -t | grep -wi cable |    \
    grep -E '(CPU|SCU)' ; echo "CABLE GRAPH" ) | ../src/dps8/dps8 -q -t | dot -Goverlap=false \
      -Gratio=1.30 -Gsplines=true -Tpdf -o pdf/gvsubset.pdf

####################################################################################################
# CABLE GRAPH IOM CONTROLLER SUBSET
( echo "CABLE_RIPOUT" ; echo "show default" | ../src/dps8/dps8 -q -t | grep -wi cable | \
   grep -E 'IOM' ; echo "CABLE GRAPH" ) | ../src/dps8/dps8 -q -t | grep -v SCU | sfdp   \
      -Gratio=0.70 -Grankdir=LR -Goverlap=false -Gsplines=true -Tpdf -o pdf/iomcon.pdf

####################################################################################################
# CABLE GRAPH STORAGE SUBSET

( echo "CABLE_RIPOUT" ; echo "show default" | ../src/dps8/dps8 -q -t | grep -wi cable |  \
   grep -E 'MSP|IPC|MTP|DISK|TAPE|IOM' ; echo "CABLE GRAPH" ) | ../src/dps8/dps8 -q -t | \
      grep -vE 'SCU|URP|FNP|OPC' | neato -Gratio=1.25 -Grankdir=LR -Goverlap=false       \
         -Gsplines=true -Tpdf -o pdf/storage.pdf

####################################################################################################
# Completed SHOW
sed -e '/^See the Omnibus documentation for a complete SHOW command reference.$/ {' -e 'r md/showtemp.md' -e 'd' -e '}' -i "./md/commandref.md"

####################################################################################################
# DEFAULT_BASE_SYSTEM
sed -e '/^Set configuration to defaults$/ {' -e 'r md/defaultbasesystem.md' -e 'd' -e '}' -i "./md/commandref.md"

############################################################################
# Aliases

sed -i 's/^## AUTOINPUT$/## AUTOINPUT (AI)/'        "./md/commandref.md"
sed -i 's/^## AUTOINPUT2$/## AUTOINPUT2 (AI2)/'     "./md/commandref.md"
sed -i 's/^## PROCEED$/## PROCEED (IGNORE)/'        "./md/commandref.md"
sed -i 's/^## EXIT$/## EXIT (QUIT, BYE)/'           "./md/commandref.md"
sed -i 's/^## BOOT$/## BOOT (BO)/'                  "./md/commandref.md"
sed -i 's/^## CABLE$/## CABLE (C)/'                 "./md/commandref.md"
sed -i 's/^## LOAD$/## LOAD (UNLOAD)/'              "./md/commandref.md"
sed -i 's/^## LUF$/## LUF (NOLUF)/'                 "./md/commandref.md"
sed -i 's/^## DETACH$/## DETACH (DET)/'             "./md/commandref.md"
sed -i 's/^## ATTACH$/## ATTACH (AT)/'              "./md/commandref.md"
sed -i 's/^## CONTINUE$/## CONTINUE (CO)/'          "./md/commandref.md"
sed -i 's/^## NEXT$/## NEXT (N)/'                   "./md/commandref.md"
sed -i 's/^## LOCALOPC$/## LOCALOPC (NOLOCALOPC)/'  "./md/commandref.md"
sed -i 's/^## STEP$/## STEP (S)/'                   "./md/commandref.md"
sed -i 's/^## RESTART$/## RESTART (XF)/'            "./md/commandref.md"
sed -i 's/^## RUN$/## RUN (RU)/'                    "./md/commandref.md"

############################################################################
# Cleanup

sed -i 's/^### RUN$//'                              "./md/commandref.md"
sed -i 's/^### SET$//'                              "./md/commandref.md"
sed -i 's/^### RETURN$//'                           "./md/commandref.md"
sed -i 's/^### GO$//'                               "./md/commandref.md"
sed -i 's/^### ON$//'                               "./md/commandref.md"
sed -i 's/^### GOTO$//'                             "./md/commandref.md"
sed -i 's/^### NEXT$//'                             "./md/commandref.md"
sed -i 's/^### SHOW$//'                             "./md/commandref.md"
sed -i 's/^### PROCEED or IGNORE$//'                "./md/commandref.md"
sed -i 's/^### Step Execution$//'                   "./md/commandref.md"
sed -i 's/^### Executing System Commands$//'        "./md/commandref.md"
sed -i 's/^### Evaluating Instructions$//'          "./md/commandref.md"
sed -i 's/^### Exiting the Simulator$//'            "./md/commandref.md"
sed -i 's/^### Call a subroutine$//'                "./md/commandref.md"
sed -i 's/^### Testing Assertions$//'               "./md/commandref.md"
sed -i 's/^### Detaching devices$//'                "./md/commandref.md"
sed -i 's/^### Displaying Arbitrary Text$//'        "./md/commandref.md"
sed -i 's/^### Resetting Devices$//'                "./md/commandref.md"
sed -i 's/^### Shift Parameters$//'                 "./md/commandref.md"
sed -i 's/^### Shift parameters$//'                 "./md/commandref.md"
sed -i 's/^### Booting the system$//'               "./md/commandref.md"
sed -i 's/^### Continuing Execution$//'             "./md/commandref.md"
sed -i 's/^### Attaching devices$//'                "./md/commandref.md"
sed -i 's/^### Processing Command Files$//'         "./md/commandref.md"
sed -i 's/^### Local Operator Console$//'           "./md/commandref.md"
sed -i 's/^### Testing Conditions$//'               "./md/commandref.md"
sed -i 's/^DEVICES is not available in Resetting Devices$//' \
                                                    "./md/commandref.md"

############################################################################
# Simulator Defaults chapter

printf '\n%s\n' '# Simulator Defaults' >> "./md/commandref.md"
printf '\n%s\n' '## Default Base System Script' >> "./md/commandref.md"
printf '%s\n' 'The following script defines the *default base system* configuration, and is executed automatically at simulator startup.  It may also be explicitly re-executed via the "**`DEFAULT_BASE_SYSTEM`**" command.' '' '* See the "**`DEFAULT_BASE_SYSTEM`**" command documentation for more information.' >> "./md/commandref.md"
printf '\n\n%s\n' '```dps8' >> "./md/commandref.md"
printf '%s' ";; " >> "./md/commandref.md"
../src/dps8/dps8 --version | ansifilter -T | expand >> "./md/commandref.md"
printf '%s\n' "SHOW DEFAULT_BASE_SYSTEM_SCRIPT" | \
  ../src/dps8/dps8 -q -t | ansifilter -T | expand >> "./md/commandref.md"
printf '\n%s\n' '```' >> "./md/commandref.md"
printf '\n%s\n' '## Default Base System Configuration' >> "./md/commandref.md"
printf '%s\n' 'The following listing of configuration details, generated by the "**`SHOW CONFIGURATION`**" command, shows the configuration state of the simulator immediately after the execution of the *default base system script* (documented in the previous section of this chapter).' '' 'This configuration state is the default state upon simulator startup.  The simulator may also be explicitly reconfigured to this state after startup by using the "**`DEFAULT_BASE_SYSTEM`**" command.' '' '* **NOTE**: The configuration details of individual FNP lines have been excluded for brevity.' '' '* See the documentation for the "**`DEFAULT_BASE_SYSTEM`**" command (in the **Simulator Command Reference** chapter) and the "**Default Base System Script**" section of this chapter for additional details.' >> "./md/commandref.md"
printf '\n\n%s\n' '```dps8' >> "./md/commandref.md"
printf '%s\n' "SHOW CONFIGURATION" | \
    ../src/dps8/dps8 -q -t | grep -v '^$' | \
    grep -v '[a-h]\.[0-9][0-9][0-9]: undefined' >> "./md/commandref.md"
printf '%s\n' '```' >> "./md/commandref.md"
printf '\n%s\n' '## Default Base System Cable Dump' >> "./md/commandref.md"
printf '%s\n' 'The following cabling configuration listing, generated by the "**`CABLE DUMP`**" command, shows the cabling configuration of the simulator immediately after the execution of the *default base system script* (documented in an earlier section of this chapter).' '' '* See the documentation for the "**`CABLE`**" commands ("`CABLE`", "`UNCABLE`", "`CABLE_RIPOUT`", "`CABLE_SHOW`", "`CABLE DUMP`", "`CABLE GRAPH`") in the **Simulator Command Reference** chapter for additional details.' '' >> "./md/commandref.md"
printf '\n\n%s\n' '```dps8' >> "./md/commandref.md"
# Unicode space used in sed below.
printf '%s\n' "CABLE DUMP" | \
    ../src/dps8/dps8 -q -t | \
      expand | sed 's/^ / /' >> "./md/commandref.md"
printf '%s\n' '```' >> "./md/commandref.md"

############################################################################

rm -f "./md/_cmdout.md"       > /dev/null 2>&1 || true
rm -f "./md/showtemp.md"      > /dev/null 2>&1 || true

############################################################################
