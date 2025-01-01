#!/usr/bin/env sh
# shellcheck disable=SC2015,SC2016,SC2312
# vim: filetype=sh:tabstop=4:ai:tw=80:expandtab
# SPDX-License-Identifier: MIT-0
# scspell-id: 4c3b133e-f632-11ec-992b-80ee73e9b8e7
# Copyright (c) 2022-2025 The DPS8M Development Team

################################################################################
# Exit immediately on lock failure

test "$(uname -s 2> /dev/null)" = "Linux" && {                                 \
FLOCK_COMMAND="$( command -v flock 2> /dev/null )" && {                        \
[ "${FLOCKER:-}" != "${0}" ] && exec env                                       \
FLOCKER="${0}" "${FLOCK_COMMAND:?}" -en "${0}" "${0}" "${@}" || : ; } ; } ;

################################################################################

unset FLOCKER       > "/dev/null" 2>&1 || true
unset FLOCK_COMMAND > "/dev/null" 2>&1 || true

################################################################################

printf '\n%s\n' \
"##############################################################################"
printf '%s' \
"###                  DPS8M Omnibus Documentation Generator                 ###"
printf '\n%s\n\n' \
"##############################################################################"

################################################################################

set -e

################################################################################

exec 2> "/dev/null"; (

################################################################################
# ansifilter

${PRINTF:-printf}       '* ansifilter   -  %s\n\n'                            \
"$( ${ANSF:-ansifilter} '--version'     2>&1                              |   \
    ${GREP:-grep}       '-iE'           '(ansifilter)'                    |   \
      ${XARGS:-xargs} )"

################################################################################
# cc

${PRINTF:-printf}       '* cc           -  %s\n\n'                            \
"$( ${CC:-cc}           '--version'     2>&1                              |   \
    ${HEAD:-head}       '-n'            '1'                               |   \
      ${SED:-sed}       -e 's/^gcc //'  -e 's/^cc //'                     |   \
        ${XARGS:-xargs} )"

################################################################################
# dot

${PRINTF:-printf}       '* graphviz     -  %s\n\n'                            \
"$( ${GRAPHVIZ:-dot}    '-v'            2>&1 < "/dev/null"                |   \
    ${GREP:-grep}       '-iE'           ' version '                       |   \
      ${SED:-sed}       's/dot - //'                                      |   \
        ${XARGS:-xargs} )"

################################################################################
# ghostscript

${PRINTF:-printf}       '* ghostscript  -  %s\n\n'                            \
"$( ${GSCRIPT:-gs}      '-v'            2>&1                              |   \
    ${AWK:-awk}         '/Ghostscript/  { print $0 }'                     |   \
      ${XARGS:-xargs} )"

################################################################################
# git

${PRINTF:-printf}       '* git          -  %s\n\n'                            \
"$( ${GIT:-git}         '--version'     2>&1                              |   \
    ${AWK:-awk}         '/^git /        { print $0 }'                     |   \
      ${XARGS:-xargs} )"

################################################################################
# make

${PRINTF:-printf}       '* make         -  %s\n\n'                            \
"$( ${MAKE:-make}       '--version'     2>&1                              |   \
    ${GREP:-grep}       '-iE'           '(make|^Built for)'               |   \
      ${XARGS:-xargs} )"

################################################################################
# pandoc

${PRINTF:-printf}       '* pandoc       -  %s\n\n'                            \
"$( ${PANDOC:-pandoc}   '--version'     2>&1                              |   \
    ${GREP:-grep}       '-iE'           '(^pandoc|^Compiled with|\..*,)'  |   \
    ${XARGS:-xargs} )"

################################################################################
# pdfinfo

${PRINTF:-printf}       '* pdfinfo      -  %s\n\n'                            \
"$( ${PDFINFO:-pdfinfo} '-v'            2>&1                              |   \
    ${AWK:-awk}         '/^pdfinfo /    { print $0 }'                     |   \
      ${XARGS:-xargs} )"

################################################################################
# pdftk

${PRINTF:-printf}       '* pdftk        -  %s\n\n'                            \
"$( ( ${PDFTK:-pdftk}   '--version'     2>&1                              |   \
        ${AWK:-awk}     '/^pdftk /      { print $0 }'                     |   \
          ${XARGS:-xargs} )                                               |   \
            ${SED:-sed} 's/ a Handy Tool for Manipulating PDF Documents/;/' )"

################################################################################
# perl

${PRINTF:-printf}       '* perl         -  %s\n\n'                            \
"$( ${PERL:-perl}       '--version'     2>&1                              |   \
    ${AWK:-awk}         '/^This is pe/  { print $0 }'                     |   \
      ${SED:-sed}                       's/^This is //'                   |   \
        ${XARGS:-xargs} )"

################################################################################
# qpdf

${PRINTF:-printf}       '* qpdf         -  %s\n\n'                            \
"$( ${QPDF:-qpdf}       '--version'     2>&1                              |   \
    ${AWK:-awk}         '/^qpdf /       { print $0 }'                     |   \
      ${XARGS:-xargs} )"

################################################################################
# xelatex

${PRINTF:-printf}       '* xelatex      -  %s\n\n'                            \
"$( ( ${XETEX:-xelatex} '--version'     2>&1                              |   \
        ${GREP:-grep}   '-iE'           '(^xetex|version)'                |   \
          ${SED:-sed}                   's/ less toxic i hope/;/'         |   \
            ${XARGS:-xargs} )                                             |   \
              ${TR:-tr} -d              ';'                               |   \
                ${SED:-sed}  's/ Compiled with /; Compiled with /g' )"

################################################################################

) | ${FMT:-fmt} -s -w 60 | "${SED:-sed}" 's/^\([^*]\)/                  \1/'

################################################################################

printf '%s\n' \
"##############################################################################"

################################################################################
