#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: MIT
# scspell-id: 67af6a69-f62c-11ec-b355-80ee73e9b8e7
# Copyright (c) 2021-2023 The DPS8M Development Team

##############################################################################

export SHELL=/bin/sh

##############################################################################

set -u > /dev/null 2>&1

##############################################################################

test -z "${SED:-}" &&
  {
    SED="$(command -v gsed 2> /dev/null || \
            env PATH="$(command -p env getconf PATH)" sh -c "command -v sed")"
    export SED
  }

##############################################################################

test -z "${TR:-}" && TR="tr" && export TR

##############################################################################

${SED:?} 's///g' | \
${SED:?} 's///g' | \
${TR:?} -d '\000'  | \
${SED:?} -r '
s/^[0-9][0-9][0-9][0-9][.][0-9]  /????.?  /
s/^r [0-9][0-9]:[0-9][0-9] [0-9]+[.][0-9]+ [0-9]+$/r ??:?? ?.??? /
s/^simCycles = [0-9]+$/simCycles = ???????/
s/^cpuCycles = [0-9]+$/cpuCycles = ???????/
s/^Timer runout faults = [0-9]+$/Timer runout faults = ???????/
s/^Connect faults = [0-9]+$/Connect faults = ???????/
s/^Access violation faults = [0-9]+$/Access violation faults = ???????/
s/^bce \(early\) [0-9][0-9][0-9][0-9][.][0-9]:/bce (early) ????.?:/
s/^bce \(boot\) [0-9][0-9][0-9][0-9][.][0-9]:/bce (boot) ????.?:/
s/^(Sunday|Monday|Tuesday|Wednesday|Thursday|Friday|Saturday), (January|February|March|April|May|June|July|August|September|October|November|December) [0-9]+, [0-9][0-9][0-9][0-9] [0-9]+:[0-9][0-9]:[0-9][0-9] pst/dayofweek, month day, year time pst/
s/^(Sun|Mon|Tue|Wed|Thu|Fri|Sat) (Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)[ ]+[0-9]+ [0-9][0-9]:[0-9][0-9]:[0-9][0-9] PST [0-9][0-9][0-9][0-9]/dayofweek month day time PST year/
s/(Sun|Mon|Tue|Wed|Thu|Fri|Sat) (Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)[ ]+[0-9]+ [0-9][0-9]:[0-9][0-9]:[0-9][0-9] [0-9][0-9][0-9][0-9]/dayofweek month day time year/
s/^Current system time is:.+$/Current system time is dayofweek, month day, year time pst./
s/^Multics MR12\.[0-9]+[a-z]* - [0-9][0-9]\/[0-9][0-9]\/[0-9][0-9]  [0-9][0-9][0-9][0-9]\.[0-9] pst (Sun|Mon|Tue|Wed|Thu|Fri|Sat)$/Multics MR12.?? - ??\/??\/?? ????.? pst ???/
s/[0-9][0-9]\/[0-9][0-9]\/[0-9][0-9]  [0-9][0-9][0-9][0-9]\.[0-9] pst (Sun|Mon|Tue|Wed|Thu|Fri|Sat)/??\/??\/?? ????.? pst ???/
s/^DBG\([0-9]+\)/DBG(??????????)/
s/^ [0-9][0-9][0-9][0-9][ ]+(vinc|s0  |s1  |as  |cord|bk  |ut  |prta) r [0-9][0-9]:[0-9][0-9] [0-9]+[.][0-9]+ [0-9]+$/ ????  \1 r ??:?? ?.? ?/
s/^ [0-9][0-9][0-9][0-9]  as   act_ctl_: shutdown, [0-9] [0-9]+\.[0-9]+ [0-9]+\.[0-9]+ [0-9]+\.[0-9]+ [0-9]+\.[0-9]+ [0-9]+:[0-9]+:[0-9]+ \$[0-9]+\.[0-9]+$/ ????  as   act_ctl_: shutdown, ? ?.?? ?.?? ?.?? ?.?? ?:?:? $???.??/
s/^ [0-9][0-9][0-9][0-9]  ut   166172[ ]+[0-9]+[ ]+[0-9]+[ ]+42218[ ]+[0-9]+[ ]+[0-9]+  pb     root$/ ????  ut   166172  ??????  ??   42218   ?????  ??  pb     root/
s/^ [0-9][0-9][0-9][0-9][ ]+(vinc|s0  |s1  |as  |cord|bk  |ut  |prta|puna|rdra) / ????  \1 /
s/^Directed fault 0 faults = [0-9]+$/Directed fault 0 faults = ?/
s/^Directed fault 1 faults = [0-9]+$/Directed fault 1 faults = ?/
s/^Fault tag 2 faults = [0-9]+$/Fault tag 2 faults = ?/
s/\(git [0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f]\)/(git ????????)/
s/\(pvid [0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]\)/(pvid ????????????)/
s/ lvid [0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]\)/ lvid ????????????)/
s/dska_00a  29950[ ]+[0-9]+[ ]+[0-9]+[ ]+14974[ ]+[0-9]+[ ]+[0-9]+[ ]+[0-9]+[ ]+rpv      pb    root/dska_00a  29950 ?????  ??  14974 ?????  ??    ? rpv      pb    root/
s/dska_00b  68111[ ]+[0-9]+[ ]+[0-9]+[ ]+13622[ ]+[0-9]+[ ]+[0-9]+[ ]+[0-9]+[ ]+root2    pb    root/dska_00b  68111 ?????  ??  13622 ?????  ??    ? root2    pb    root/
s/dska_00c  68111[ ]+[0-9]+[ ]+[0-9]+[ ]+13622[ ]+[0-9]+[ ]+[0-9]+[ ]+[0-9]+[ ]+root3    pb    root/dska_00c  68111 ?????  ??  13622 ?????  ??    ? root3    pb    root/
s/YourPasswordXWXWXWXWXWXW986986986986............/YourPasswordXWXWXWXWXWXW986986986986?????????????/
s/^[0-9][0-9]:[0-9][0-9]:[0-9][0-9] 100([0-9]+)  0 r [0-9]+:[0-9]+ [0-9]+\.[0-9]+ [0-9]+/??:??:?? 100\1  0 r ??:?? ?.??? ?/
s/^[0-9][0-9]:[0-9][0-9]:[0-9][0-9] 100/??:??:?? 100/
s/^r  [0-9][0-9][0-9][0-9]$/r ????/
' | \
${SED:?} '
s/^yoyodyne.ini-[0-9]*>/yoyodyne.ini-???>/
s/ PVID [0-9]..........[0-9] / PVID ????????????/
s/ LVID [0-9]..........[0-9] / LVID ????????????/
s/^  Commit: ......................................../  Commit: ????/
s/^DPS8\/M simulator .*$/DPS8\/M simulator ????/
s/^L68 simulator .*$/L68 simulator ????/
s/^FNP telnet server port set to .*/FNP telnet server port set to ????/
s/^.FNP emulation: TELNET server port set to .*/FNP emulation: TELNET server port set to ????/
s/^    Commit: ......................................../    Commit: ????/
s/^   Version: .*$/   Version: ????/
s/^  Modified: [1-9].*/Modified: ????/
s/^  Compiled: [1-9].*/Compiled: ????/
s/^  Build OS: .*$/  Build OS: ????/
s/^  Compiler: .*$/  Compiler: ????/
s/^  Built by: .*$/  Built by: ????/
s/^   Host OS: .*$/   Host OS: ????/
s/^      Compilation info: .*$/      Compilation info: ????/
s/^  Relevant definitions: .*$/  Relevant definitions: ????/
s/^    Event loop library: Built with libuv .*$/    Event loop library: Built with libuv ????/
s/^Console . port set to .*/Console ? port set to ????/
s/^PVID .*o$/PVID ????/
s/^LVID .*o$/LVID ????/
s/>process_dir_dir>!............../>process_dir_dir>!............../
s/Segment quota used changed from .*$/Segment quota used changed from ???? to ????/
s/Directory quota used changed from .*$/Directory quota used changed from ???? to ????/
s/ \$[1-9].* (logo)/ ???.??? (logo)/
s/Automatic update: users = [0-9].*//
s/^cycles .* [1-9].*$/cycles ????/
s/^instructions .* [1-9].*$/instructions ????/
s/^lockCnt .* [0-9].*$/lockCnt ????/
s/^lockImmediate .* [0-9].*$/lockImmediate ????/
s/^lockWait .* [0-9].*$/lockWait ????/
s/^lockWaitMax .* [0-9].*$/lockWaitMax ????/
s/^lockYield .* [0-9].*$/lockYield ????/
s/FNP emulation: listening .. .*/FNP emulation: listening ?? ????/
s/^ ????  ut   [1-9].*root/ ????  ut   ????/
s/^Cutoff date: .*$/Cutoff date: ????/
s/YourPasswordXWXWXWXWXWXW986986986986//
s/^Registered .*/Registered ????/
s/0 r [0-9][0-9]:[0-9][0-9] .*[0-9]/0 r ??:?? ????/
s/^r [0-9][0-9]:[0-9][0-9] -[0-9].*/r ??:?? -???? ????/
s/^MR12.._install.ini-[0-9]*>/MR12.?_install.ini-???>/
s/^yoyodyne_start.ini-[0-9]*>/yoyodyne_start.ini-???>/
s/^??:??:?? [0-9][0-9][0-9][0-9][0-9][0-9][0-9]/??:??:?? ???????/
s/^Timestamp: .*\./Timestamp: ????/
s/^ [0-9][0-9][0-9][0-9] / ???? /
s/^quota = [1-9].*; used = [1-9].*$/quota = ?????; used = ?????/
s/ .*[0-9] messages copied into:/ ???? messages copied into:/
s/ .*[0-9] messages processed,/ ???? messaged processed,/
s/ syserr seq num [0-9].*$/ syserr seq num ????./
s/ : login word is ....../ : login word is ??????/
s/ 0:[0-9][0-9] / ?:?? /
s/^dsk. 0.. ..... .....  ..  ..... .....  ..   .. rpv/dsk? ??? ????? ?????  ??  ????? ?????  ??   ?? rpv/
s/ Charge for request [0-9].*$/ Charge for request ????/
s/^spawn telnet .*/spawn telnet ????/
s/^Map attached to file .*$/Map attached to file ????/
s/^MIPS AVE = [0-9].*/MIPS AVE = ????/
s/^CPU . cache: ON .*[0-9].*$/CPU ? cache: ON      ????/
s/^PID: [0-9][0-9].*  LOCK ID: [0-9][0-9].*$/PID: ????  LOCK ID: ????/
s/prta.spool.*prt/prta.spool.??????.prt/
s/^ \+Elapsed: [0-9]\+.*//
s/^ \+dps 8.em.*[0-9]\+.*//
s/^Log >sl1>syserr_log from .*//
'

##############################################################################
