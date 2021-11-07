#!/usr/bin/env bash
# shellcheck disable=SC2129
############################################################################
# Copyright (c) 2021 Jeffrey H. Johnson <trnsz@pobox.com>                  #
# Copyright (c) 2021 The DPS8M Development Team                            #
#                                                                          #
# Copying and distribution of this file, with or without modification,     #
# are permitted in any medium without royalty provided the copyright       #
# notice and this notice are preserved.  This file is offered "AS-IS",     #
# without any warranty.                                                    #
############################################################################
# Requires: Modern GNU tools such as make, coreutils, bash, awk, sed, etc. #
############################################################################
# Setting the environment variable DUMPSCRIPT to a value prints the script #
############################################################################
{ test -d "./src/dps8" || { printf '%s\n' "Error: No sources"; exit 1; }; };
{ test -z "${COMPILER:-}" && { export COMPILER="ccache gcc" || exit 1; }; };
BLDFILE=$(mktemp 2> /dev/null) ||                                          \
  { printf '%s\n' "Error: Couldn't create build tempfile."  >&2; exit 1 ; };
MAKFILE=$(mktemp 2> /dev/null) ||                                          \
  { printf '%s\n' "Error: Couldn't create script tempfile." >&2; exit 1 ; };
# shellcheck disable=SC2064                                                #
trap "rm -f \"${BLDFILE:?}\" \"${MAKFILE:?}\" 2> /dev/null || true" EXIT INT
printf '%s\n' "exec 2> /dev/null"                           > "${BLDFILE:?}"
printf '%s\n' 'X="None"'                                   >> "${BLDFILE:?}"
for option in                                                              \
       L68                                                                 \
       NO_LOCKLESS                                                         \
       TESTING                                                             \
       NEED_128                                                            \
       HDBG                                                                \
       ROUND_ROBIN                                                         \
       TRACKER                                                             \
       ;                                                                   \
do                                                                         #
  SX=$(printf '%s\n'                                                       \
    "X=\$(join -j 2 -t \"\"                                                \
      <(printf '%s\n' \"${option:?}=1 \" \"None \")                        \
      <(printf '%s\n' \"\${X:?}\" \"None \"))")                            #
  printf '%s\n' "${SX:?}"                                  >> "${BLDFILE:?}"
done                                                                       #
printf '%s\n' "CTLS=\$(printf '%s\n' \"\${X:?}\" | \\"     >> "${BLDFILE:?}"
printf '%s\n' "  sed 's/None//g' | while read -r line; do" >> "${BLDFILE:?}"
printf '%s\n' "    printf '%s\\n' \"\${line:?}\" |                         \
                     sed 's/ /\\n/g' | sort -u | awk                       \
                       '{ line=line \" \" \$0 } END                        \
                         { print line }'"                  >> "${BLDFILE:?}"
printf '%s\n' "  done | sed -e 's/^ \\+//g' -e 's/ \$//g'                  \
                   | sort -u)"                             >> "${BLDFILE:?}"
printf '%s\n' "printf '%s\\n' \"\${CTLS:?}\" | \\"         >> "${BLDFILE:?}"
printf '%s\n' "awk '{ print \"make ${JFLAGS:-} CC=\\\"${COMPILER:?}\\\"    \
                 \"\$0\" && printf %s\\\\\\\n \\\"SHOW VERSION\\\" |       \
                   ./src/dps8/dps8 && make distclean && \\\\\"} END        \
                       { print \"true\" }'"                >> "${BLDFILE:?}"
printf '%s' "Generating build script ..."                                >&2
set -e && env bash < "${BLDFILE:?}" > "${MAKFILE:?}" && rm -f "${BLDFILE:?}"
test -z "${DUMPSCRIPT:-}" || { printf '%s\n' " OK" && cat "${MAKFILE:?}" | \
   tr -s ' '; rm -f "${MAKFILE:?}"; exit 0; };                             \
printf ' %sbuilds.\nStarting builds in 5s (^C to abort) ...\n'             \
  "$(wc -l <(grep -v "^true$" "${MAKFILE:?}") | cut -d '/' -f 1)" && sleep 5
make "clean"  &&  env bash -x "${MAKFILE:?}" || true && rm -f "${MAKFILE:?}"
############################################################################
