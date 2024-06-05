#!/usr/bin/env sh
# shellcheck disable=SC2310,SC2312,SC2320
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: MIT-0
# scspell-id: cbd99c3f-f62b-11ec-a7f0-80ee73e9b8e7
# Copyright (c) 2021-2024 The DPS8M Development Team

###############################################################################
# Requires: Cppcheck, Clang, GCC, GNU tools, lscpu, tput (from [n]curses)
###############################################################################

${TEST:-test} -d "./.git" ||
  {
    printf '%s\n' "Error: Not in top-level git repository."
    exit 1
  }

###############################################################################

export SHELL=/bin/sh

###############################################################################

${TEST:-test} -d "./.cppbdir" ||
  {
    ${MKDIR:-mkdir} -p "./.cppbdir" ||
      {
        printf '%s\n' "Error: Unable to create .cppbdir directory."
        exit 1
      }
  }

###############################################################################

set -eu 2> /dev/null 2>&1

###############################################################################

${TEST:-test} -z "${MAKE:-}" && MAKE="command -p env make"

###############################################################################

CPPCHECK="cppcheck"

###############################################################################

CPPCHECK_HTMLREPORT="cppcheck-htmlreport"

###############################################################################

CPPCHECKS="warning,performance,portability"

###############################################################################

CPPDEFINE='-DBUILDINFO_scp="CPPCHECK"
           -D__CPPCHECK__=1
           -DCPPCHECK=1
           -DDECBUFFER=32
           -DDECNUMDIGITS=126
           -D_GNU_SOURCE
           -Dint32_t=int32
           -DPRId64="lld"
           -DPRIo64="llo"
           -DPRIu64="llu"
           -UAF_INET6
           -UFILE_DEVICE_BLUETOOTH
           -UFILE_DEVICE_CRYPT_PROVIDER
           -UFILE_DEVICE_FIPS
           -UFILE_DEVICE_INFINIBAND
           -UFILE_DEVICE_VMBUS
           -UFILE_DEVICE_WPD
           -UINVALID_HANDLE
           -UIOCTL_DISK_GET_DRIVE_GEOMETRY
           -UIOCTL_DISK_GET_DRIVE_GEOMETRY_EX
           -UIOCTL_STORAGE_EJECT_MEDIA
           -UIOCTL_STORAGE_GET_HOTPLUG_INFO
           -UIOCTL_STORAGE_READ_CAPACITY
           -UIPV6_V6ONLY
           -U_MSC_BUILD
           -U_MSC_FULL_VER
           -U__OPEN64__
           -USD_BOTH
           -USIGHUP
           -USIGPIPE
           -USIM_COMPILER
           -USO_EXCLUSIVEADDRUSE
           -U__STRICT_POSIX__
           -USYSDEFS_USED
           -UTCP_NODELAYACK
           -U__USE_POSIX199309
           -UVDSUSP
           -UVER_CURRENT_TIME
           -UVER_H_GIT_DATE
           -UVER_H_GIT_HASH
           -UVER_H_GIT_PATCH
           -UVER_H_GIT_VERSION
           -UVER_H_PREP_DATE
           -UVER_H_PREP_USER
           -U__VERSION__
           -UVSTATUS
           -U__WATCOMC__'

###############################################################################

COMPILERS="gcc clang"

###############################################################################

for i in ${COMPILERS:?}
do
  {
    command -v "${i:?}" > "/dev/null" 2>&1 ||
      {
        printf '%s\n' "Error: ${i:?} not found."
        exit 1
      }
  }
done

###############################################################################

test -z "${HTMLOUT:-}" ||
{
  XMLARGS="--xml --xml-version=2"
  XMLGEN=" 2> \${unit:?}.xml"
}

###############################################################################

test -z "${NOQUIET:-}" &&
{
  QUIET="--suppress=shadowArgument   --suppress=shadowVariable           \
         --suppress=shadowFunction   --suppress=ConfigurationNotChecked  \
         --suppress=unknownMacro     --suppress=memleakOnRealloc         \
         --suppress=internalAstError                                     \
         --suppress=syntaxError:/usr/include/stdlib.h"
}

###############################################################################

EXTRA="--cppcheck-build-dir=./.cppbdir ${QUIET:-}"

###############################################################################

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
  test "${CPUS_MTH1:-1}" -ge "${CPUS_MTH2:-1}" 2> "/dev/null" &&
    {
      printf '%s\n' "${CPUS_MTH1:?}"
    } || printf '%s\n' "${CPUS_MTH2:?}"
}

###############################################################################

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
      # shellcheck disable=SC2086,SC2048
      printf '\n%s\n' ${*:-}
    ) |
      sort -u |
        tr '\n' ' '
    set -e
}

###############################################################################

full_line()
{
  set +e
  LINECHAR="#"
  printf '%*s\n' "${COLUMNS:-$(tput cols 2> /dev/null ||
    printf '%s\n' "72")}" '' | tr ' ' "${LINECHAR:--}"
  set -e
}

###############################################################################

title_line()
{
  set +e
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
    "${COLUMNS:-$(tput cols 2> /dev/null ||
      printf '%s\n' "72")}" "${TITLELEN:-1}" | bc 2> /dev/null ||
        printf '%s\n' "36")"
  TITLELINE="$(printf '%s%s%s' "$(printf '%*s' "${HALFWIDTH:?}" '' |
      tr ' ' "${LINECHAR:--}")" \
      "$(printf '%s' "${TITLEWORD:?}")" \
        "$(printf '%*s\n' "${HALFWIDTH:?}" '' | tr ' ' "${LINECHAR:--}")")"
  TITLELLEN="${#TITLELINE}"
  SLACKLEN="$(printf '%d-%d\n' \
    "${COLUMNS:-$(tput cols 2> /dev/null ||
      printf '%s\n' "72")}" "${TITLELLEN:-1}" |
        bc 2> /dev/null || printf '%s\n' "0")"
  printf '%s' "${TITLELINE:?}"
  test "${SLACKLEN:-0}" -gt 0 && printf '%*s\n' "${SLACKLEN:?}" '' |
    tr ' ' "${LINECHAR:--}"
  set -e
}

###############################################################################

do_cppcheck()
{
  set +e
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
        # shellcheck disable=SC2086,SC2310
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
  ( ${RMF:-rm} -f "${unit:?}.xml" > "/dev/null" 2>&1 || true )
  # shellcheck disable=SC2086,SC2248
  eval ${CPPCHECK:?} ${EXTRA:-}  -j "$(count_cpus)" ${XMLARGS:-}          \
    --enable="${CPPCHECKS:?}" --force -UBUILDINFO_"${unit:?}"             \
    ${CPPDEFINE:?} ${includes:?} ${infiles:?} ${EXTRA:-}                  \
    --file-filter="./src/*" --inline-suppr --max-ctu-depth="16"           \
    --platform="unix64" --std="c11" --std="c++11" --suppress="*:/usr/*"   \
    --suppress="checkLibraryNoReturn" --suppress="funcArgNamesDifferent"  \
    --suppress="unmatchedSuppression" --suppress="variableScope"          \
    --include="$(pwd -L)/src/dps8/ver.h" --check-level="exhaustive"       \
    ${XMLGEN:-} ;                                                         \
  test -z "${HTMLOUT:-}" ||
  {
    ${MKDIR:-mkdir} -p "./cppcheck"          \
      > "/dev/null" 2>&1 || true ;           \
    ${RMF:-rm} -rf "./cppcheck/${unit:?}"    \
      > "/dev/null" 2>&1 || true ;           \
    eval ${CPPCHECK_HTMLREPORT:?}            \
      --source-dir="."                       \
      --title="${unit:?}"                    \
      --file="${unit:?}.xml"                 \
      --report-dir="./cppcheck/${unit:?}" |  \
        ${GREP-grep} -v "^$" ;               \
    ( ${RMDIR:-rmdir} "./cppcheck/"*         \
        > "/dev/null" 2>&1 || true ) ;       \
    ( ${RMF:-rm} -f "${unit:?}.xml"          \
        > "/dev/null" 2>&1 || true ) ;       \
  }
  full_line
  printf '%s\n' ""
}

###############################################################################

(
  set +e
  cd src/dps8 &&
  ${MAKE:?} clean -j "$(count_cpus)" > /dev/null 2>&1 &&
    ${MAKE:?} "ver.h" "errnos.h" "sysdefs.h"  \
      -j "$(count_cpus)"                      \
        > /dev/null 2>&1
) || {
    printf '%s\n' "Error: Unable to prep source tree."
    set -e
    exit 1
}

###############################################################################

full_line
title_line "Running ${CPPCHECK:?} with up to $(count_cpus) parallel tasks"
full_line
printf '%s\n' ""

###############################################################################

title_line "$(printf '%s' 'empty')" >&2
printf '%s\n' "" >&2
do_cppcheck "empty" "./src/empty"
printf '%s\n' "" >&2

###############################################################################

title_line "$(printf '%s' 'unifdef')" >&2
printf '%s\n' "" >&2
do_cppcheck "unifdef" "./src/unifdef"
printf '%s\n' "" >&2

###############################################################################

title_line "$(printf '%s' 'punutil')" >&2
printf '%s\n' "" >&2
do_cppcheck "punutil" "./src/punutil"
printf '%s\n' "" >&2

###############################################################################

title_line "$(printf '%s' 'mcmb')" >&2
printf '%s\n' "" >&2
do_cppcheck "mcmb" "./src/mcmb"
printf '%s\n' "" >&2

###############################################################################

title_line "$(printf '%s' 'prt2pdf')" >&2
printf '%s\n' "" >&2
do_cppcheck "prt2pdf" "./src/prt2pdf"
printf '%s\n' "" >&2

###############################################################################

title_line "$(printf '%s' 'blinkenLights2')" >&2
printf '%s\n' "" >&2
do_cppcheck "blinkenLights2" "./src/blinkenLights2"
printf '%s\n' "" >&2

###############################################################################

# dps8 (excluding sim_tmxr)
title_line "$(printf '%s' 'dps8')" >&2
printf '%s\n' "" >&2
do_cppcheck "dps8"                     \
  "./src/decNumber"                    \
  "$(find ./src/simh -name '*.[ch]' |  \
     grep -v 'sim_tmxr\.[ch]')"        \
  "./src/dps8"
printf '%s\n' "" >&2

###############################################################################

${TEST:-test} -z "${HTMLOUT:-}" ||
{
  ${RM:-rm} -f "cppcheck/index.html" 2> "/dev/null"
  # shellcheck disable=SC2086
  (                                                     \
    ${TEST:-test} -d "cppcheck" &&                      \
      cd "cppcheck" &&                                  \
        ${GREP:-grep} -l                                \
          '<tr><td></td><td>0</td><td>total</td></tr>'  \
            "./"*"/index.html" |                        \
              ${XARGS:-xargs} -I{}                      \
                ${RMF:-rm} -f {}                        \
                  > "/dev/null" 2>&1                    \
  )
  # shellcheck disable=SC2016,SC2094
  (                                                       \
    ${TEST:-test} -d "cppcheck" &&                        \
      cd "cppcheck" &&                                    \
        ${FIND:-find} -name "index.html" -print |         \
          ${AWK:-awk}                                     \
           '{ print "<A HREF=\""$0"\">"$0"</A><BR>" }' |  \
            ${SED:-sed} -e 's/\/index.html<\/A>/<\/A>/'   \
                        -e 's/>\.\//>/'                   \
                        -e 's/"\.\//"/'                   \
              | ${GREP:-grep} -v '>.<' > "index.html" ;   \
  )
}

###############################################################################
