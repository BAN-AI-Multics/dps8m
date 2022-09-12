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

############################################################################

cp -f "./md/showcommands.md" "./md/showtemp.md"

############################################################################
# Command Reference chapter

# shellcheck disable=SC1001,SC2016
( ( echo HELP | "../src/dps8/dps8" -t -q | tail -n +2 | grep -v '^$' | tr -s ' ' | tr ' ' '\n' | grep -v '^$' | grep -vwE '(HELP|IGNORE|BREAK|NOBREAK|EXPECT|NOEXPECT|SEND|EXAMINE|DEPOSIT|IEXAMINE|IDEPOSIT|XF|RESTART|AI|AI2)' | sort | xargs -I{} echo echo\ \;echo\ \XXXXXXXXXXXX\ {}\;echo help\ -f\ {}\|../src/dps8/dps8\ -t\ -q\;echo\;echo |sh) | grep -Ev '(DPS8/M help.$|^L68 help.$)' ) | expand | sed -e 's/XXXXXXXXXXXX/#/' -e 's/^1\.1\.//' -e 's/^[0-9]\+ /## /' -e 's/^[0-9]\+\.[0-9]\+ /## /' -e 's/^[0-9]\+\.[0-9]\+\.[0-9]\+ /### /' -e 's/^[0-9]\+\.[0-9]\+\.[0-9]\+\.[0-9]\+ /#### /' -e 's/^#/##/' -e 's/^    //' -e '' -e 's/^        /             /' -e '/^[[:alnum:]]/s/</\\</g' -e '/^[[:alnum:]]/s/>/\\>/g' -e '/^[[:alnum:]]/s/_/\\_/g' | ansifilter -T | tee "./temp2.tmp" && printf '%s\n\n' '<!-- pagebreak -->' '# Simulator Command Reference' 'This chapter provides reference documentation for the **DPS8M** simulator command set.' '* This information is also available from within the simulator; it is accessible by using the interactive `HELP` command.' '' '<!-- br -->' '' > "./temp1.tmp" && sed -e 's/^## /\n\n<!-- br -->\n\n## /g' -e 's/^\* \* /   * /g' < "./temp2.tmp" >> "./temp1.tmp" && rm -f "./temp2.tmp" && mv -f "./temp1.tmp" "./md/commandref.md"

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
# SET
sed -e '/^See the Omnibus documentation for a complete SET command reference.$/ {' -e 'r md/setcommands.md' -e 'd' -e '}' -i "./md/commandref.md"
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
# Completed SHOW
sed -e '/^See the Omnibus documentation for a complete SHOW command reference.$/ {' -e 'r md/showtemp.md' -e 'd' -e '}' -i "./md/commandref.md"
# DEFAULT_BASE_SYSTEM
sed -e '/^Set configuration to defaults$/ {' -e 'r md/defaultbasesystem.md' -e 'd' -e '}' -i "./md/commandref.md"

############################################################################
# Aliases

sed -i 's/^## AUTOINPUT$/## AUTOINPUT (AI)/'     "./md/commandref.md"
sed -i 's/^## AUTOINPUT2$/## AUTOINPUT2 (AI2)/'  "./md/commandref.md"
sed -i 's/^## PROCEED$/## PROCEED (IGNORE)/'     "./md/commandref.md"
sed -i 's/^## EXIT$/## EXIT (QUIT, BYE)/'        "./md/commandref.md"
sed -i 's/^## BOOT$/## BOOT (BO)/'               "./md/commandref.md"
sed -i 's/^## LOAD$/## LOAD (LO)/'               "./md/commandref.md"
sed -i 's/^## DETACH$/## DETACH (DET)/'          "./md/commandref.md"
sed -i 's/^## ATTACH$/## ATTACH (AT)/'           "./md/commandref.md"
sed -i 's/^## CONTINUE$/## CONTINUE (CONT, CO)/' "./md/commandref.md"
sed -i 's/^## NEXT$/## NEXT (N)/'                "./md/commandref.md"
sed -i 's/^## STEP$/## STEP (S)/'                "./md/commandref.md"
sed -i 's/^## RESTART$/## RESTART (XF)/'         "./md/commandref.md"
sed -i 's/^## RUN$/## RUN (RU)/'                 "./md/commandref.md"

############################################################################
# Cleanup

sed -i 's/^### RUN$//'                           "./md/commandref.md"
sed -i 's/^### SET$//'                           "./md/commandref.md"
sed -i 's/^### RETURN$//'                        "./md/commandref.md"
sed -i 's/^### GO$//'                            "./md/commandref.md"
sed -i 's/^### ON$//'                            "./md/commandref.md"
sed -i 's/^### GOTO$//'                          "./md/commandref.md"
sed -i 's/^### NEXT$//'                          "./md/commandref.md"
sed -i 's/^### SHOW$//'                          "./md/commandref.md"
sed -i 's/^### PROCEED or IGNORE$//'             "./md/commandref.md"
sed -i 's/^### Step Execution$//'                "./md/commandref.md"
sed -i 's/^### Executing System Commands$//'     "./md/commandref.md"
sed -i 's/^### Evaluating Instructions$//'       "./md/commandref.md"
sed -i 's/^### Exiting the Simulator$//'         "./md/commandref.md"
sed -i 's/^### Call a subroutine$//'             "./md/commandref.md"
sed -i 's/^### Testing Assertions$//'            "./md/commandref.md"
sed -i 's/^### Detaching devices$//'             "./md/commandref.md"
sed -i 's/^### Displaying Arbitrary Text$//'     "./md/commandref.md"
sed -i 's/^### Resetting Devices$//'             "./md/commandref.md"
sed -i 's/^### Shift Parameters$//'              "./md/commandref.md"
sed -i 's/^### Shift parameters$//'              "./md/commandref.md"
sed -i 's/^### Booting the system$//'            "./md/commandref.md"
sed -i 's/^### Continuing Execution$//'          "./md/commandref.md"
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
printf '%s\n' "SHOW CONFIGURATION" | ansifilter -T | \
    ../src/dps8/dps8 -q -t | grep -v '^$' | expand | \
    grep -v '[a-h]\.[0-9][0-9][0-9]: undefined' >> "./md/commandref.md"
printf '%s\n' '```' >> "./md/commandref.md"

############################################################################

rm -f "./md/_cmdout.md"      > /dev/null 2>&1 || true
rm -f "./md/showtemp.md"     > /dev/null 2>&1 || true

############################################################################
env cp -f ./md/commandref.md /tmp/commandref.md
