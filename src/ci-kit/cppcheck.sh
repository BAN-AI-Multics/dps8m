#!/usr/bin/env sh
# Requires: GNU tools
set -eu

test -d "./.git" || {
  printf '%s\n' "Error: Not in top-level git repository."
  exit 1
}

MAKE="command -p env make"
CPPCHECK="cppcheck"
CPPCHECKS="warning,style,performance,portability,information"
CPPDEFINE='-DDECNUMDIGITS=126 -U__VERSION__ -D_GNU_SOURCE -DDECBUFFER=32
           -U_WIN32 -U__STRICT_POSIX__ -Dint32_t=int32 -DUSE_POPT -DNEED_128=1
           -DCPPCHECK=1 -U__MINGW64__ -U__MINGW32__ -UVMS -U__VAX
           -U__USE_POSIX199309 -UCROSS_MINGW32 -UCROSS_MINGW64 -DPRIo64="llo"
           -DPRId64="lld" -DPRIu64="llu" -U__OPEN64__ -U_MSC_FULL_VER
           -U_MSC_BUILD -UVER_H_GIT_HASH -UVER_H_GIT_PATCH -UVER_H_GIT_VERSION
           -DBUILDINFO_scp="CPPCHECK" -USYSDEFS_USED -UVER_H_GIT_DATE
           -UVER_H_PREP_DATE -UVER_H_PREP_USER -U__WATCOMC__  -USIM_COMPILER
           -UVER_CURRENT_TIME -U__DECC -U__DECC_VER'
COMPILERS="gcc clang"
QUIET="--suppress=shadowArgument --suppress=shadowVariable \
       --suppress=shadowFunction --suppress=ConfigurationNotChecked"
EXTRA="--inconclusive ${QUIET:-}"

count_cpus()
{
  set +e
  CPUS_MTH1="$(
    lscpu -b -p='core' 2> /dev/null |
      grep -v '^#' | sort -run | head -n 1 |
      awk '{ print $1+1 }' || printf '%s\n' "1"
  )"
  CPUS_MTH2="$(
    sort -u < /proc/cpuinfo 2> /dev/null |
      cut -d ':' -f 1 | uniq -c | sort -n | tail -n 1 |
      rev | tr -s ' ' | cut -d ' ' -f 2 | rev |
      awk '{ print $1+0 }' || printf '%s\n' "1"
  )"
  set -e
  test "${CPUS_MTH1:-1}" -ge "${CPUS_MTH2:-1}" &&
    {
      printf '%s\n' "${CPUS_MTH1:?}"
    } || printf '%s\n' "${CPUS_MTH2:?}"
}

include_paths()
{
  set +e
  test "${#}" -gt 0 ||
    {
      printf '%s\n' "Error: include_paths() requires 1 or more arguments."
      set -e
      exit 1
    }
  comp="${1:?}"
  shift
  (
    printf '%s\n' "$(
      (
        "${comp:?}" -E -Wp,-v - < /dev/null 2>&1 || true
      ) |
        grep -v -E '(^#.*$|^ignoring .*|^End of search list.$|^[^ ].*$|^$)' |
        xargs -I{} printf '-I%s\n' {}
    )"
    # shellcheck disable=SC2086
    printf '\n%s\n' ${*:-}
  ) |
    sort -u |
    tr '\n' ' '
  set -e
}

full_line()
{
  set +e
  printf '%s\r' ""
  LINECHAR="#"
  printf '%*s\n' "${COLUMNS:-$(tput cols || printf '%s\n' "72")}" '' |
    tr ' ' "${LINECHAR:--}"
  printf '%s\r' ""
  set -e
}

title_line()
{
  set +e
  printf '%s\r' ""
  # shellcheck disable=SC2015
  test "${#}" -gt 0 &&
    test "${#}" -lt 2 ||
    {
      printf '%s\n' "Error: title_line() requires exactly 1 argument."
      set -e
      exit 1
    }
  TITLEWORD=" ${1:?} "
  TITLELEN="${#TITLEWORD}"
  LINECHAR="#"
  HALFWIDTH="$(printf '((%d/2)-1)-(((%d+2)/2)-1)\n' \
    "${COLUMNS:-$(tput cols || printf '%s\n' "72")}" "${TITLELEN:-1}" |
    bc 2> /dev/null || printf '%s\n' "36")"
  TITLELINE="$(printf '%s%s%s' "$(printf '%*s' "${HALFWIDTH:?}" '' |
    tr ' ' "${LINECHAR:--}")" \
    "$(printf '%s' "${TITLEWORD:?}")" \
    "$(printf '%*s\n' "${HALFWIDTH:?}" '' | tr ' ' "${LINECHAR:--}")")"
  TITLELLEN="${#TITLELINE}"
  SLACKLEN="$(printf '%d-%d\n' \
    "${COLUMNS:-$(tput cols || printf '%s\n' "72")}" "${TITLELLEN:-1}" |
    bc 2> /dev/null || printf '%s\n' "0")"
  printf '%s' "${TITLELINE:?}"
  test "${SLACKLEN:-0}" -gt 0 && printf '%*s\n' "${SLACKLEN:?}" '' |
    tr ' ' "${LINECHAR:--}"
  printf '%s\r' ""
  set -e
}

do_cppcheck()
{
  set +e
  printf '%s\r\n' ""
  test "${#}" -gt 1 ||
    {
      printf '%s\n' "Error: cppcheck($*) requires 2 or more arguments."
      set -e
      exit 1
    }
  unit="${1:-}"
  shift
  infiles="${*:-}"
  for compiler in ${COMPILERS:?}; do
    command -v "${compiler:?}" > /dev/null 2>&1 &&
      {
        # shellcheck disable=SC2086
        includes="$(include_paths "${compiler:?}" ${includes:-})" ||
          {
            printf '%s\n' "Error: include_paths(${compiler:?}): failed."
            set -e
            exit 1
          }
      }
  done
  set -e
  title_line "${unit:?}"
  # shellcheck disable=SC2086,SC2248
  eval ${CPPCHECK:?} ${EXTRA:-} -j "$(count_cpus)"             \
    --enable="${CPPCHECKS:?}" --force -UBUILDINFO_"${unit:?}"  \
    ${CPPDEFINE:?} ${includes:?} ${infiles:?} ${EXTRA:-}       \
    --file-filter="./src/*" --inline-suppr --max-ctu-depth=8   \
    --platform="unix64" --std="c11" --std="c++11"              \
    --suppress="*:/usr/*" --suppress="checkLibraryNoReturn"    \
    --suppress="funcArgNamesDifferent"                         \
    --suppress="unmatchedSuppression"                          \
    --suppress="variableScope"                                 \
    --include="$(pwd -L)/src/dps8/ver.h"
  full_line
  printf '%s\n' ""
}

(
  set +e
  cd src/dps8 &&
  ${MAKE:?} clean -j "$(count_cpus)" > /dev/null 2>&1 &&
  ${MAKE:?} "ver.h" "errnos.h" -j "$(count_cpus)" > /dev/null 2>&1
) || {
    printf '%s\n' "Error: Unable to prep source tree."
    set -e
    exit 1
}

do_cppcheck "unifdef" "./src/unifdef"
do_cppcheck "punutil" "./src/punutil"
do_cppcheck "prt2pdf" "./src/prt2pdf"
do_cppcheck "dps8" "./src/decNumber" "./src/simh" "./src/dps8"