#!/usr/bin/env sh
# shellcheck disable=SC2310,SC2312

# Begin
printf '#### %s\n' "Begin ${0} (${$})"

# Strict
set -eu > /dev/null 2>&1

# Job ID
T_JOB_ID="$(date 2> /dev/null | tr -cd 0-9 2> /dev/null)$$" &&
  export T_JOB_ID 2> /dev/null
test -z "${CI_JOB_ID:-}" &&
  export CI_JOB_ID="${T_JOB_ID:?}"

# Sanity check
test -f "./tapes/foo.tap" ||
  {
	printf '%s\n' \
      'Error: "./tapes/foo.tap" missing; running "./init.sh"'
    ./init.sh ||
      exit 1
  }

# Clean-up 1/2
test -d ./run &&
  rm -rf ./run > /dev/null 2>&1

# Check for tmux
command -v tmux > /dev/null 2>&1 ||
  {
    printf '%s\n' \
      'Error: "tmux" not found in PATH.'
    exit 1
  }

# Check for stdbuf
command -v "stdbuf" > /dev/null 2>&1 &&
  STDBUF="stdbuf -o L" ||
    STDBUF="exec"

# Check for faketime
command -v "faketime" > /dev/null 2>&1 &&
  FAKETIME='faketime --exclude-monotonic -m 2025-05-05' ||
    FAKETIME="env TZ=UTC"

# Check for awk
command -v "${AWK:-awk}" > /dev/null 2>&1 ||
  {
    printf '%s\n' \
      "Error: \"${AWK:-awk}\" not found in PATH."
    exit 1
  }

# Port status?
NCSTATUS="(open)" && test -z "${NCAT:-}" && NCAT="ncat"
${NCAT:?} "--version" 2>&1 | grep -q '^Ncat:.*nmap\.org' ||
  {
    NCAT="true;false;:"
    NCSTATUS="(unverified)"
  }

# Port ${1} bindable?
portCanBind()
  {
    # shellcheck disable=SC2248
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

# Port ${1} open?
portFree()
  {
    ${NCAT:?} -n --send-only "127.0.0.1" "${1:?}" \
      < /dev/null > /dev/null 2>&1 ||
        {
          printf '%s\n' "${1:?}"
          return 0
        }
    printf '%s\n' "-1"
    return 1
  }

# Generate random ephemeral port
randomPort()
  {
    # shellcheck disable=SC2312
    ${AWK:-awk} "BEGIN                                           \
      { srand($( ( ${DATE:-date} -u 2> /dev/null;                \
         ${PS:-ps} 2> /dev/null; ${PS:-ps} auxw 2> /dev/null ) | \
           ${CKSUM:-cksum} 2> /dev/null |                        \
             ${TR:-tr} -cd '0-9\n' 2> /dev/null |                \
               ${DD:-dd} bs=1 count=5 2> /dev/null ) );          \
        printf( \"%d\\n\", ( 16380 * rand() ) + 49154 );         \
      }"
  }

# Assign a port - racy - but better than nothing ...
assignOpenPort()
  {
    while ( true ); do
      portCanBind "$(portFree "$(randomPort)")" && break
    done
  }

# Tool configuration
printf '***           make: "%s"\n' \
  "${MAKE:-make}"
printf '***           tail: "%s"\n' \
  "${TAIL:-tail}"
printf '***       faketime: "%s"\n' \
  "${FAKETIME:?}"
printf '***         stdbuf: "%s"\n' \
  "${STDBUF:?}"

# Port configuration
while test "${CONPORT:-0}" = "${FNPPORT:-0}"; do
  CONPORT="$(assignOpenPort)" &&
    printf '***   console port: %s\n' \
      "\"${CONPORT:?}\" ${NCSTATUS:?}" &&
        export CONPORT 2> /dev/null
  FNPPORT="$(assignOpenPort)" &&
    printf '***       FNP port: %s\n' \
      "\"${FNPPORT:?}\" ${NCSTATUS:?}" &&
        export FNPPORT 2> /dev/null
done

# Cleanup 2/2
rm -f ./*.log 2> /dev/null

# Timestamp
(exec ${STDBUF:?} date -u "+Timestamp: %Y-%m-%d %H:%M:%S %Z." 2> /dev/null |
  ${STDBUF:?} "${TEE:-tee}" -i -a "ci.log" > /dev/null 2>&1)

# Suppress hook warnings
SKIP_HOOK=1 &&
  export SKIP_HOOK 2> /dev/null

# Configuration
export NOREBUILD 2> /dev/null
printf '*** %s\n' "   tmux job ID: \"cikit-${T_JOB_ID:?}-0\""
printf '*** %s\n' "  tmux channel: \"cikit-${T_JOB_ID:?}-1\""
export TMUX='' 2> /dev/null

# Run CI-Kit in background tmux session
# shellcheck disable=SC2048,SC2086,SC2248
eval tmux -u -2 new -d -s cikit-${T_JOB_ID:?}-0                          \
  "\"export CI_JOB_ID=\"${CI_JOB_ID:?}\";                                \
    export FNPPORT=\"${FNPPORT:?}\"; export CONPORT=\"${CONPORT:?}\";    \
      eval ${FAKETIME:?} ${STDBUF:?} ${MAKE:-make} --no-print-directory  \
        -f "ci.makefile" ${*} \"s1\" \"s2\" \"s2p\" \"s3\" \"s3p\"       \
          \"s4\" \"s5\" \"s6\" \"s7\" 2>&1 |                             \
            ${STDBUF:?} ${TEE:-tee} -i -a ci.log;                        \
              tmux wait -S cikit-${T_JOB_ID:?}-1 \"" &&                  \
  tmux wait cikit-${T_JOB_ID:?}-1 &
BGJOB="${!}" && export BGJOB
printf '***  %s\n' "tmux wait PID: \"${BGJOB:?}\""

# Live tail log files
# shellcheck disable=SC2248
eval ${STDBUF:?} "${TAIL:-tail}" -q -s 0.1 -F -n "+0" --pid="${BGJOB:?}" \
  "ci.log" "ci_t2.log" "ci_t3.log" "isolts.log" "perf.log" 2> /dev/null

# Finish
wait "${BGJOB:?}" &&
  (exec ${STDBUF:?} date -u "+Timestamp: %Y-%m-%d %H:%M:%S %Z." 2> /dev/null |
    ${STDBUF:?} "${TEE:-tee}" -i -a "ci.log" > /dev/null 2>&1)

# End
printf '#### %s\n' "End ${0} (${$})"

# EOF
