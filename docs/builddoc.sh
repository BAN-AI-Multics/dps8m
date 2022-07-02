#!/usr/bin/env sh
# shellcheck disable=SC2086
# vim: filetype=sh:tabstop=4:tw=80:expandtab
# SPDX-License-Identifier: FSFAP
# scspell-id: d4b774db-f631-11ec-90ff-80ee73e9b8e7

################################################################################
#
# Copyright (c) 2021-2022 The DPS8M Development Team
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered "AS-IS",
# without any warranty.
#
################################################################################

# WARNING: Removes all `*.pdf`, `*.tmp`, and `*.state` files during operation.
# WARNING: Installs the templates to `${HOME}/.local/share/pandoc/templates/`.

################################################################################
# Initialization

 set -e
 export SHELL=/bin/sh > /dev/null 2>&1

################################################################################
# Install template

 ${MKDIR:-mkdir} -p  \
     "${HOME:-}/.local/share/pandoc/templates"
 ${CPF:-cp} -f  \
     "./latex/eisvogel-dps8.latex"  \
     "${HOME:-}/.local/share/pandoc/templates/"
 set +e

################################################################################
# Highlight style

 LISTSTYLE="--listings"                   # LaTeX listings
#LISTSTYLE="--highlight-style pygments"   # Haskell skylighting

################################################################################
# Inputs

 INPUTS="yaml/docinfo.yml" # <--------------------- Start of document
 INPUTS="${INPUTS:-} md/authors.md" # <------------ Authors
#INPUTS="${INPUTS:-} md/acknowledgements.md" # <--- Acknowledgements
#INPUTS="${INPUTS:-} md/forward.md" # <------------ Forward
#INPUTS="${INPUTS:-} md/preface.md" # <------------ Preface
#INPUTS="${INPUTS:-} md/introduction.md" # <------- Introduction
#INPUTS="${INPUTS:-} md/prologue.md" # <----------- Prologue
#INPUTS="${INPUTS:-} md/buildsource.md" # <-------- Building from Source Code
 INPUTS="${INPUTS:-} commandref.md" # <------------ Simulator Command Reference
 INPUTS="${INPUTS:-} md/licensing.md" # <---------- Licensing Terms and Legal

################################################################################
# Check (some) basic prerequisites

 command -v "ansifilter" > /dev/null 2>&1 ||
   { printf '%s\n' "Error: ansifilter not found."; exit 1; }

 command -v "git" > /dev/null 2>&1 ||
   { printf '%s\n' "Error: git not found."; exit 1; }

 command -v "awk" > /dev/null 2>&1 ||
   { printf '%s\n' "Error: awk not found."; exit 1; }

 command -v "dot" > /dev/null 2>&1 ||
   { printf '%s\n' "Error: dot (graphviz) not found."; exit 1; }

 command -v "${PANDOC:-pandoc}" > /dev/null 2>&1 ||
   { printf '%s\n' "Error: ${PANDOC:-pandoc} not found."; exit 1; }

 command -v "qpdf" > /dev/null 2>&1 ||
   { printf '%s\n' "Error: qpdf not found." exit 1; }

 command -v "xelatex" > /dev/null 2>&1 ||
   { printf '%s\n' "Error: xelatex not found."; exit 1; }

 command -v "pdftk" > /dev/null 2>&1 ||
   { printf '%s\n' "Error: pdftk (pdftk-java) not found."; exit 1; }

 command -v "pdfinfo" > /dev/null 2>&1 ||
   { printf '%s\n' "Error: pdfinfo (poppler) not found."; exit 1; }

 command -v "gs" > /dev/null 2>&1 ||
   { printf '%s\n' "Error: gs (ghostscript) not found."; exit 1; }

 command -v "bc" > /dev/null 2>&1 ||
   { printf '%s\n' "Error: bc not found."; exit 1; }

 command -v "fmt" > /dev/null 2>&1 ||
   { printf '%s\n' "Error: fmt not found."; exit 1; }

 command -v "xargs" > /dev/null 2>&1 ||
   { printf '%s\n' "Error: xargs not found."; exit 1; }

 command -v "seq" > /dev/null 2>&1 ||
   { printf '%s\n' "Error: seq not found."; exit 1; }

 command -v "file" > /dev/null 2>&1 ||
   { printf '%s\n' "Error: file utility not found."; exit 1; }

################################################################################
# Check compiled executables

 test -x "../src/dps8/dps8" ||
   { printf '%s\n' "Error: dps8 not found."; exit 1; }

 test -x "../src/punutil/punutil" ||
   { printf '%s\n' "Error: punutil not found."; exit 1; }

 test -x "../src/unifdef/unifdef" ||
   { printf '%s\n' "Error: unifdef not found."; exit 1; }

 test -x "../src/prt2pdf/prt2pdf" ||
   { printf '%s\n' "Error: prt2pdf not found."; exit 1; }

################################################################################
# Check working directory

 test -x "./toolinfo.sh" ||
   { printf '%s\n' "Error: No toolinfo.sh script found."; exit 1; }

 test -x "./builddoc.sh" ||
   { printf '%s\n' "Error: Invalid working directory."; exit 1; }

################################################################################
# Software versions

 ( command -p env sh "./toolinfo.sh"        2> "/dev/null" |
     ${SED:-sed} 's/\(^\*.* -\)$/\1 ???/' ) 2> "/dev/null"

################################################################################
# Clean-up

 rm -f  ./*.state             > /dev/null  2>&1 || true
 rm -f  ./.*.state            > /dev/null  2>&1 || true
 rm -f  ./yaml/docinfo.yml    > /dev/null  2>&1 || true
 rm -f  ./*.pdf               > /dev/null  2>&1 || true
 rm -f  ./*.tmp               > /dev/null  2>&1 || true
 rm -f  ./complete.out        > /dev/null  2>&1 || true
 rm -f  ./commandref.md       > /dev/null  2>&1 || true

################################################################################
# Stop on errors

 set -e

################################################################################
# Global variables

 printf '\n%s\n' \
   "  ########################################################################"

 BUILDUTC=$(env TZ=UTC date -u +"%Y-%m-%d %H:%M:%S %Z")
   printf '    Documentation build date : %s\n' "${BUILDUTC:?}"

 PROMVERX="$(printf '%s%s\n'                                                 \
          "$(grep VER_H_GIT_RELT ../src/dps8/ver.h | cut -d '"' -f2)"        \
          "$(grep VER_H_PROM_VER_TEXT ../src/dps8/ver.h | cut -d '"' -f2)" | \
          tr -s ' ' | tr -d ' /')"

 BUILDVER="$(../src/dps8/dps8 --version 2>&1 |
             awk '/ simulator / { print $3 }' | tr -d ' */')"
   printf '    DPS8M Simulator version  : %s\n' "${BUILDVER:?}"

 BUILDGIT="$(env TZ=UTC git rev-parse HEAD | tr -d ' */')"
   printf '    Distribution commit hash : %s\n' "${BUILDGIT:?}"

 PRT2PDFV="$(../src/prt2pdf/prt2pdf -v 2>&1 |
             awk '/^prt2pdf version / { print $3 }' | tr -d ' */')"
   printf '    Prt2PDF version          : %s\n' "${PRT2PDFV:?}"

 PUNUTILV="$(../src/punutil/punutil --version 2>&1 < /dev/null |
             awk '/^Version/ { print $2 }' | tr -d ' */')"
   printf '    PunUtil version          : %s\n' "${PUNUTILV:?}"

 UNIFDEFV="$(../src/unifdef/unifdef -v 2>&1 |
             awk '/Version: / { print $2 }' | tr -d ' */')"
   printf '    Unifdef version          : %s\n' "${UNIFDEFV:?}"

 MCMBVERS="$(../src/mcmb/mcmb -v 2>&1 | cut -d ' ' -f 5- | tr -d '*/')"
   printf '    mcmb version             : %s\n' "${MCMBVERS:?}"

 VMPCVERS="$(../src/vmpctool/vmpctool -V 2>&1 | cut -d ' ' -f 2 | tr -d '*/')"
   printf '    vmpctool version         : %s\n' "${VMPCVERS:?}"

 EMPTYVER="$(../src/empty/empty -h 2>&1 | head -n 1 | sed -e 's/^empty-//' |
             awk '/usage:/ { print $1 }' | tr -d ' */')"
   printf '    empty version            : %s\n' "${EMPTYVER:?}"

 LASTMODV="$(cd .. && env TZ=UTC git log -1  \
             --format="%cd"                  \
             --date=format-local:'%Y-%m-%d %H:%M:%S UTC' | tr -d '*')"
   printf '    Last modification date   : %s\n' "${LASTMODV:?}"

 if [ "${ONESIDE:-0}" -eq 1 ] 2> /dev/null; then
   NOBLANKS=1
   ONESIDE="-V classoption=oneside"
   printf '    Document layout          : %s\n' "One-sided print output"
 else
   ONESIDE=""
   printf '    Document layout          : %s\n' "Standard book output"
   unset NOBLANKS > /dev/null 2>&1 || true
   unset ONESIDE > /dev/null 2>&1 || true
 fi

 printf '%s\n\n' \
   "  ########################################################################"

################################################################################
# Output filename

 export OUTPUTPDF="dps8-${BUILDVER:-unknown}.pdf"

 rm -f "${OUTPUTPDF:?}" > /dev/null 2>&1 || true

################################################################################
# Generate docinfo.yml

 sed                                     \
     -e "s/##PROMVERX##/${PROMVERX:?}/"  \
     -e "s/##BUILDUTC##/${BUILDUTC:?}/"  \
     -e "s/##BUILDVER##/${BUILDVER:?}/"  \
     -e "s/##LASTMODV##/${LASTMODV:?}/"  \
     -e "s/##UNIFDEFV##/${UNIFDEFV:?}/"  \
     -e "s/##PUNUTILV##/${PUNUTILV:?}/"  \
     -e "s/##PRT2PDFV##/${PRT2PDFV:?}/"  \
     -e "s/##MCMBVERS##/${MCMBVERS:?}/"  \
     -e "s/##VMPCVERS##/${VMPCVERS:?}/"  \
     -e "s/##EMPTYVER##/${EMPTYVER:?}/"  \
     -e "s/##BUILDGIT##/${BUILDGIT:?}/"  \
          yaml/docinfo.yml.in            \
            > yaml/docinfo.yml

################################################################################
# Generate Command Reference

# shellcheck disable=SC2016
 (
  stdbuf -o L ./refgen.sh |
   stdbuf -o L grep --line-buffered '^## ' |
    stdbuf -o L sed 's/^## //' |
     stdbuf -o L xargs -n1 \
      printf '%s\n'
 ) |
 stdbuf -o L awk \
   '{ printf("\r   Generating command reference: %s                 \r",$0); }'
 printf '\r%s\r' "                                                            "

################################################################################
# Processing - Stage 1 -- Assembly

 printf '%s' "  [ 0% ........ "

 sed                                           \
     -e 's/^----$//g'                          \
     -e 's/<!-- pagebreak -->/\\newpage/g'     \
     -e 's/<!-- br -->/\&nbsp;/g'              \
     ${INPUTS}                                 \
  | ${PANDOC:-pandoc}                          \
     -V geometry:margin=0.7in                  \
     ${ONESIDE}                                \
     --top-level-division=chapter              \
     --lua-filter lua/pagebreak/pagebreak.lua  \
     --toc-depth=6                             \
     --toc                                     \
     --dpi=1200                                \
     --shift-heading-level-by=0                \
     --template eisvogel-dps8                  \
     --pdf-engine=xelatex                      \
     ${LISTSTYLE:?}                            \
     -f markdown                               \
     -t pdf -                                  \
     -o stage1.pdf

 printf '%s' "20% ........ "

################################################################################
# Postprocessing - Stage 2 -- Compress

 qpdf                            \
     --flatten-rotation          \
     --recompress-flate          \
     --compression-level=9       \
     --object-streams=generate   \
     --linearize                 \
     --verbose                   \
     "./stage1.pdf"              \
     "./stage2.pdf"  > /dev/null

 printf '%s' "40% ........ "

################################################################################
# Postprocessing - Stage 3 -- Minimize

 if [ "${NOBLANKS:-0}" -eq 1 ]; then
   ./removeblank.sh    \
       "./stage2.pdf"  \
       "./stage3.pdf"
 fi

 printf '%s' "60% ........ "

################################################################################
# Postprocessing - Stage 4 -- Recompress

 if [ "${NOBLANKS:-0}" -eq 1 ]; then
   qpdf                            \
       --flatten-rotation          \
       --recompress-flate          \
       --compression-level=9       \
       --object-streams=generate   \
       --linearize                 \
       --verbose                   \
       "./stage3.pdf"              \
       "./stage4.pdf"  > /dev/null
 fi

 printf '%s' "80% ........ "

################################################################################
# Finalize - Stage 5 -- Rename

 if [ "${NOBLANKS:-0}" -eq 1 ]; then
   mv -f "./stage4.pdf"   "./complete.out"
 else
   mv -f "./stage2.pdf"   "./complete.out"
 fi

 rm -f    ./*.state            > /dev/null  2>&1 || true
 rm -f    ./.*.state           > /dev/null  2>&1 || true
 rm -f    ./yaml/docinfo.yml   > /dev/null  2>&1 || true
 rm -f    ./*.pdf              > /dev/null  2>&1 || true
 rm -f    ./*.tmp              > /dev/null  2>&1 || true
 rm -f    ./commandref.md      > /dev/null  2>&1 || true
 mv -f   "./complete.out" "./${OUTPUTPDF:?}"
 cp -f   "./${OUTPUTPDF:?}" "./dps8-omnibus.pdf"
 test -f "./${OUTPUTPDF:?}" || exit 3

 printf '%s\n\n' "100% ]"

 printf '%s\n' \
"##############################################################################"

################################################################################
