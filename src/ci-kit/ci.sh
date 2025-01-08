#!/usr/bin/env sh
# shellcheck disable=SC2310,SC2312
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: MIT-0
# scspell-id: 628508d5-f62b-11ec-99af-80ee73e9b8e7
# Copyright (c) 2021-2025 The DPS8M Development Team

##############################################################################

printf '#### %s\n' "Begin ${0} (${$})"

##############################################################################

test -x "/bin/sh" ||
  {
    printf '%s\n'  \
      '*** Error: No "/bin/sh" executable.'
    exit 1
  }

##############################################################################

export SHELL=/bin/sh

##############################################################################

# shellcheck disable=SC2009
GIT_DIFF_OUT="$(git diff 2> /dev/null)"
test -z "${GIT_DIFF_OUT:-}" ||
  {
    printf '%s\n'  \
      "*** Note: Dirty tree! \"git diff\" output:"
    printf '%s\n' "${GIT_DIFF_OUT:-}"
    printf '%s\n' "" "*** Continuing in 5 seconds ... "
    sleep 5 > /dev/null 2>&1
  }

##############################################################################

# shellcheck disable=SC2009
TMUX_SESSIONS="$(tmux list-sessions 2> /dev/null |  \
                   grep '^ci-kit' 2> /dev/null)"
test -z "${TMUX_SESSIONS:-}" ||
  {
    printf '%s\n'  \
      '*** Note: CI-Kit tmux sessions possibly active:'
    printf '%s\n' "${TMUX_SESSIONS:-}"
    printf '%s\n' "" "*** Continuing in 5 seconds ... "
    sleep 5 > /dev/null 2>&1
  }

##############################################################################

# shellcheck disable=SC2009
DPS8_SESSIONS="$(ps -ef 2> /dev/null | grep 'dps8.*yoyo' 2> /dev/null |  \
                   grep -v ' grep' 2> /dev/null)"
test -z "${DPS8_SESSIONS:-}" ||
  {
    printf '%s\n'  \
      '*** Note: CI-Kit dps8 sessions possibly active:'
    printf '%s\n' "${DPS8_SESSIONS:-}"
    printf '%s\n' "" "*** Continuing in 5 seconds ... "
    sleep 5 > /dev/null 2>&1
  }

##############################################################################

export DUMA_DISABLE_BANNER=1
export DUMA_OUTPUT_FILE="dumalog.txt"
export DUMA_OUTPUT_STDOUT=0
export DUMA_OUTPUT_STDERR=0
export DUMA_MEMCPY_OVERLAP=1

##############################################################################

# Strict
set -e > /dev/null 2>&1

##############################################################################

T_JOB_ID="$(date 2> /dev/null | tr -cd 0-9 2> /dev/null)$$" &&
  export T_JOB_ID 2> /dev/null
test -z "${CI_JOB_ID:-}" &&
  export CI_JOB_ID="${T_JOB_ID:?}"

##############################################################################

test -f "./tapes/foo.tap" ||
  {
    printf '%s\n'  \
      '*** Note: "./tapes/foo.tap" missing; running "./init.sh"'
    ./init.sh ||
      exit 1
  }

##############################################################################

test -d ./run &&
  rm -rf ./run > /dev/null 2>&1

##############################################################################

command -v tmux > /dev/null 2>&1 ||
  {
    printf '%s\n'  \
      '*** Error: "tmux" not found in PATH.'
    exit 1
  }

##############################################################################

command -v ncat > /dev/null 2>&1 ||
  {
    printf '%s\n'  \
      '*** Warning: "ncat" not found in PATH!'
    sleep 6
  }

##############################################################################

command -v "stdbuf" > /dev/null 2>&1 &&
  STDBUF="stdbuf -o L" ||
    STDBUF="exec"

##############################################################################

command -v "${AWK:-awk}" > /dev/null 2>&1 ||
  {
    printf '%s\n'  \
      "Error: \"${AWK:-awk}\" not found in PATH."
    exit 1
  }

##############################################################################

# Port status?
NCSTATUS="(open)" && test -z "${NCAT:-}" && NCAT="ncat"
${NCAT:?} "--version" 2>&1 | grep -q '^Ncat:.*nmap\.org' ||
  {
    NCAT="true;false;:"
    NCSTATUS="(unverified)"
  }

##############################################################################

# Port ${1} bindable?
portCanBind()
  {
    # shellcheck disable=SC2248,SC2086
    timeout 1 ${NCAT:?} -l "127.0.0.1" "${1:?}" > /dev/null 2>&1
    rc="${?}"
    if [ "${rc:-}" -gt 2 ]; then
      {
        printf '%s\n' "${1:?}"
        return 0
      }
    fi
    printf '%s\n' "-1"
    return 1
  }

##############################################################################

# Port ${1} open?
portFree()
  {
    ${NCAT:?} -n --send-only "127.0.0.1" "${1:?}"  \
      < /dev/null > /dev/null 2>&1 ||
        {
          printf '%s\n' "${1:?}"
          return 0
        }
    printf '%s\n' "-1"
    return 1
  }

##############################################################################

# Generate random ephemeral port
randomPort()
  {
    sleep 1 > /dev/null 2>&1
    # shellcheck disable=SC2312
    ${AWK:-awk} "BEGIN                                            \
      { srand($( ( ${DATE:-date} -u 2> /dev/null;                 \
         ${PS:-ps} 2> /dev/null; ${PS:-ps} auxw 2> /dev/null ) |  \
           ${CKSUM:-cksum} 2> /dev/null |                         \
             ${TR:-tr} -cd '0-9\n' 2> /dev/null |                 \
               ${DD:-dd} bs=1 count=5 2> /dev/null ) );           \
        printf( \"%d\\n\", ( 16380 * rand() ) + 49154 );          \
      }"
  }

##############################################################################

# Assign a port - racy - but better than nothing ...
assignOpenPort()
  {
    while ( true ); do
      portCanBind "$(portFree "$(randomPort)")" && break
    done
  }

##############################################################################

# Tool configuration
printf '***           make: "%s"\n'  \
  "${MAKE:-make}"
printf '***           tail: "%s"\n'  \
  "${TAIL:-tail}"
printf '***         stdbuf: "%s"\n'  \
  "${STDBUF:?}"

##############################################################################

# Port configuration
while test "${CONPORT:-0}" = "${FNPPORT:-0}"; do
  CONPORT="$(assignOpenPort)" &&
    printf '***   console port: %s\n'  \
      "\"${CONPORT:?}\" ${NCSTATUS:?}" &&
        export CONPORT 2> /dev/null
  FNPPORT="$(assignOpenPort)" &&
    printf '***       FNP port: %s\n'  \
      "\"${FNPPORT:?}\" ${NCSTATUS:?}" &&
        export FNPPORT 2> /dev/null
done

##############################################################################

rm -f ./*.log 2> /dev/null

##############################################################################

# Timestamp
(exec ${STDBUF:?} date -u "+Timestamp: %Y-%m-%d %H:%M:%S %Z." 2> /dev/null |
  ${STDBUF:?} "${TEE:-tee}" -i -a "ci.log" > /dev/null 2>&1)

##############################################################################

# Suppress hook warnings
SKIP_HOOK=1 &&
  export SKIP_HOOK 2> /dev/null

##############################################################################

# Suppress rebuild?
NRB="" > /dev/null 2>&1
# shellcheck disable=SC2015,SC2016,SC2065
test "0${NOREBUILD:-}" -eq 1 > /dev/null 2>&1 &&
  export NRB="NOREBUILD=1"   > /dev/null 2>&1 ||
  export NRB="REBUILD=1"     > /dev/null 2>&1
export NOREBUILD 2> /dev/null

##############################################################################

# Configuration
printf '*** %s\n' "   tmux job ID: \"cikit-${T_JOB_ID:?}-0\""
printf '*** %s\n' "  tmux channel: \"cikit-${T_JOB_ID:?}-1\""
export TMUX='' 2> /dev/null

##############################################################################

# Run CI-Kit in background tmux session
# shellcheck disable=SC2015,SC2048,SC2086,SC2140,SC2248
eval tmux -u -2 new -d -s cikit-${T_JOB_ID:?}-0                       \
  "\"export CI_JOB_ID=\"${CI_JOB_ID:?}\";                             \
   export FNPPORT=\"${FNPPORT:?}\"; export CONPORT=\"${CONPORT:?}\";  \
    ${STDBUF:?} ${MAKE:-make} ${NRB:-} --no-print-directory           \
     -f "ci.makefile" ${*} \"s1\" \"s2\" \"s2p\" \"s3\" \"s3p\"       \
      \"s4\" \"s4p\" \"s5\" \"s6\" \"s7\" 2>&1 |                      \
       ${STDBUF:?} ${TEE:-tee} -i -a ci.log;                          \
        tmux wait -S cikit-${T_JOB_ID:?}-1 \"" &&                     \
  tmux wait cikit-${T_JOB_ID:?}-1 &
BGJOB="${!}" && export BGJOB
printf '***  %s\n' "tmux wait PID: \"${BGJOB:?}\""

##############################################################################

# Live tail log files
# shellcheck disable=SC2248
TSTAMPER="$(./timestamp.sh 2> /dev/null)"
printf '*** %s\n' "   timestamper: \"${TSTAMPER:-cat}\""
# shellcheck disable=SC2086,SC2248
( eval ${STDBUF:?} "${TAIL:-tail}" -q -s 0.1 -F -n "+0" --pid="${BGJOB:?}"  \
   "ci.log" "ci_t2.log" "ci_t3.log" "isolts.log" "perf.log"                 \
     "ci_t1.debug.log" "ci_t2.debug.log" "ci_t3.debug.log"                  \
       "isolts.debug.log" "isolts_run.debug.log" 2> /dev/null )             \
  | eval ${TSTAMPER:-cat}

##############################################################################

wait "${BGJOB:?}" &&
  ( exec ${STDBUF:?} date -u "+Timestamp: %Y-%m-%d %H:%M:%S %Z."  \
      2> /dev/null | ${STDBUF:?} "${TEE:-tee}" -i -a "ci.log"     \
        > /dev/null 2>&1)

##############################################################################

printf '#### %s\n' "End ${0} (${$})"

##############################################################################
