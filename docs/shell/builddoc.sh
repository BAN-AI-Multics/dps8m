#!/usr/bin/env sh
# shellcheck disable=SC2015,SC2086
# vim: filetype=sh:tabstop=4:ai:expandtab
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
# Locking

 # Wait for lock
 test -z "${FLOCKER:-}" && printf '%s\n' "Waiting for lock ... " ;             \
 test "$( uname -s 2> /dev/null )" = "Linux" && {                              \
 FLOCK_COMMAND="$( command -v flock 2> /dev/null )" && {                       \
 [ "${FLOCKER:-}" != "${0}" ] && exec env FLOCKER="${0}" "${FLOCK_COMMAND:?}"  \
   -e --verbose "${0}" "${0}" "${@}" || :; } ; } ;

################################################################################

unset FLOCKER       > "/dev/null" 2>&1 || true
unset FLOCK_COMMAND > "/dev/null" 2>&1 || true

################################################################################
# Initialization

 set -e
 test -z "${VERBOSE:-}" || set -x
 export SHELL=/bin/sh > "/dev/null" 2>&1

################################################################################
# Install template

 ${MKDIR:-mkdir} -p  \
     "${HOME:-}/.local/share/pandoc/templates"
 ${CPF:-cp} -f  \
     "./latex/eisvogel-dps8.latex"  \
     "${HOME:-}/.local/share/pandoc/templates/"
 set +e

################################################################################
# Inputs

 test -z "${BUILDDOC_HTML:-}" &&  \
 INPUTS="yaml/docinfo-post.yml" # <---------------- Start of LaTeX document
 INPUTS="${INPUTS:-} md/authors.md" # <------------ Authors
#INPUTS="${INPUTS:-} md/acknowledgements.md" # <--- Acknowledgements
 INPUTS="${INPUTS:-} md/pre-overview.md" # <------- (setup) Building from Source Code
 INPUTS="${INPUTS:-} hpages/content/Overview/_index.md" # <------- Introduction
 INPUTS="${INPUTS:-} md/post-overview.md" # <------ (finish) Building from Source Code
 INPUTS="${INPUTS:-} md/obtain.md" # <------------- Obtaining the simulator
 INPUTS="${INPUTS:-} md/pre-source.md" # <--------- (setup) Building from Source Code
 INPUTS="${INPUTS:-} hpages/content/Documentation/Source_Compilation/_index.md" # <-------- Building from Source Code
 INPUTS="${INPUTS:-} md/post-source.md" # <-------- (finish) Building from Source Code
 INPUTS="${INPUTS:-} md/commandref.md" # <--------- Simulator Command Reference
 INPUTS="${INPUTS:-} md/licensing.md" # <---------- Licensing Terms and Legal

################################################################################
# Check (some) basic prerequisites

 command -v "ansifilter" > "/dev/null" 2>&1 ||
   { printf '%s\n' "Error: ansifilter not found."; exit 1; }

 command -v "php" > "/dev/null" 2>&1 ||
   { printf '%s\n' "Error: php not found."; exit 1; }

 command -v "git" > "/dev/null" 2>&1 ||
   { printf '%s\n' "Error: git not found."; exit 1; }

 command -v "awk" > "/dev/null" 2>&1 ||
   { printf '%s\n' "Error: awk not found."; exit 1; }

 command -v "dot" > "/dev/null" 2>&1 ||
   { printf '%s\n' "Error: dot (graphviz) not found."; exit 1; }

 command -v "${PANDOC:-pandoc}" > "/dev/null" 2>&1 ||
   { printf '%s\n' "Error: ${PANDOC:-pandoc} not found."; exit 1; }

 command -v "qpdf" > "/dev/null" 2>&1 ||
   { printf '%s\n' "Error: qpdf not found."; exit 1; }

 command -v "xelatex" > "/dev/null" 2>&1 ||
   { printf '%s\n' "Error: xelatex not found."; exit 1; }

 command -v "pdftk" > "/dev/null" 2>&1 ||
   { printf '%s\n' "Error: pdftk (pdftk-java) not found."; exit 1; }

 command -v "pdfinfo" > "/dev/null" 2>&1 ||
   { printf '%s\n' "Error: pdfinfo (poppler) not found."; exit 1; }

 command -v "gs" > "/dev/null" 2>&1 ||
   { printf '%s\n' "Error: gs (ghostscript) not found."; exit 1; }

 command -v "bc" > "/dev/null" 2>&1 ||
   { printf '%s\n' "Error: bc not found."; exit 1; }

 command -v "fmt" > "/dev/null" 2>&1 ||
   { printf '%s\n' "Error: fmt not found."; exit 1; }

 command -v "xargs" > "/dev/null" 2>&1 ||
   { printf '%s\n' "Error: xargs not found."; exit 1; }

 command -v "seq" > "/dev/null" 2>&1 ||
   { printf '%s\n' "Error: seq not found."; exit 1; }

 command -v "file" > "/dev/null" 2>&1 ||
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

 test -x "./shell/toolinfo.sh" ||
   { printf '%s\n' "Error: No toolinfo.sh script found."; exit 1; }

 test -x "./shell/builddoc.sh" ||
   { printf '%s\n' "Error: Invalid working directory."; exit 1; }

################################################################################
# Software versions

 ( command -p env sh "./shell/toolinfo.sh"        2> "/dev/null" |
     ${SED:-sed} 's/\(^\*.* -\)$/\1 ???/' )       2> "/dev/null"

################################################################################
# Clean-up

 rm -f  ./*.state                  > "/dev/null"  2>&1 || true
 rm -f  ./.*.state                 > "/dev/null"  2>&1 || true
 rm -f  ./yaml/docinfo-post.yml    > "/dev/null"  2>&1 || true
 rm -f  ./*.pdf                    > "/dev/null"  2>&1 || true
 rm -f  ./*.tmp                    > "/dev/null"  2>&1 || true
 rm -f  ./stage*.zip               > "/dev/null"  2>&1 || true
 rm -rf ./s1epub3.tmp              > "/dev/null"  2>&1 || true
 rm -rf ./s1temp.tmp               > "/dev/null"  2>&1 || true
 rm -f  ./complete.out             > "/dev/null"  2>&1 || true
 rm -f  ./md/commandref.md         > "/dev/null"  2>&1 || true
 rm -f  ./md/commandref.out        > "/dev/null"  2>&1 || true
 rm -rf ./temp.out                 > "/dev/null"  2>&1 || true

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

 PUNUTILV="$(../src/punutil/punutil --version 2>&1 < "/dev/null" |
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

 if [ "${BUILDDOC_HTML:-0}" -eq 1 ] 2> "/dev/null"; then
   ONESIDE=""
   printf '    Document layout          : %s\n' "HTML output"
   unset NOBLANKS > "/dev/null" 2>&1 || true
   unset ONESIDE > "/dev/null" 2>&1 || true
 else
   if [ "${ONESIDE:-0}" -eq 1 ] 2> "/dev/null"; then
     NOBLANKS=1
     ONESIDE="-V classoption=oneside"
     printf '    Document layout          : %s\n' "One-sided print output"
   else
     ONESIDE=""
     printf '    Document layout          : %s\n' "Standard book output"
     unset NOBLANKS > "/dev/null" 2>&1 || true
     unset ONESIDE > "/dev/null" 2>&1 || true
   fi
 fi

 printf '%s\n\n' \
   "  ########################################################################"

################################################################################
# Output filename

 export OUTPUTPDF="dps8-${BUILDVER:-unknown}.pdf"

 rm -f "${OUTPUTPDF:?}" > "/dev/null" 2>&1 || true

################################################################################
# Generate docinfo-post.yml

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
          yaml/docinfo.yml               \
            > yaml/docinfo-post.yml

################################################################################
# Generate Command Reference

# shellcheck disable=SC2016
 (
  stdbuf -o L ./shell/refgen.sh |
   stdbuf -o L grep --line-buffered '^## ' |
    stdbuf -o L sed 's/^## //' |
     stdbuf -o L xargs -n1 \
      printf '%s\n'
 ) |
 stdbuf -o L awk \
   '{ printf("\r   Generating command reference: %s                 \r",$0); }'
 printf '\r%s\r' "                                                            "

 # Converted indented code blocks to fenced blocks
 ./php/i2f.php "md/commandref.md"

 # Abuse GNU cat to compress consecutive empty lines
 cat -s "md/commandref.md" > "md/commandref.out" &&  \
     mv -f "md/commandref.out" "md/commandref.md"

 # Transform dps8 fenced blocks to Verbatims in commandref.md
${SED:-sed} -i -e 's/```dps8/\\begin{tcolorbox}\[colback=black!2!white,breakable=true\]\n\\begin{Verbatim}\[fontsize=\\small\]/' -e 's/```/\\end{Verbatim}\n\\end{tcolorbox}/' "md/commandref.md"

 # Abuse GNU cat to compress consecutive empty lines again
 cat -s "md/commandref.md" > "md/commandref.out" &&  \
     mv -f "md/commandref.out" "md/commandref.md"

 # Ensure no empty Verbatims
 perl -i -0pe 's/\n\n\\end{Verbatim}/\n\\end{Verbatim}/g' "md/commandref.md"
 perl -i -0pe 's/\\begin{Verbatim}\[fontsize=\\small\]\n\\end{Verbatim}/\\begin{Verbatim}\[fontsize=\\small\]\n\.\.\.\n\\end{Verbatim}/g' "md/commandref.md"

 test -z "${BUILDDOC_SAVE_INTERMEDIATE:-}" ||
   {
      SAVE_TEE="tee input.sav"
      grep -v 'scspell-id' "md/commandref.md" > "cmdref.sav" || true
   }

################################################################################
# Processing - Stage 1 -- Assembly

 test -z "${BUILDDOC_HTML:-}" &&
   {
      PDTLSEC="--shift-heading-level-by=0"
      printf '%s' "  [ 0% ........ "
   }

 # Use template if not generating HTML
 test -z "${BUILDDOC_HTML:-}" ||
   {
      HTML_EXT="epub3" ;                     \
      PDTLDIV="section" ;                    \
      PDTLSEC="--section-divs                \
               --columns=78                  \
               --strip-comments              \
               --epub-chapter-level=2        \
               --shift-heading-level-by=0"
   }

 # Template selection
 test -z "${BUILDDOC_HTML:-}" &&  \
        PDTMPLTE="--template eisvogel-dps8"

 # Use pagebreak filter
 PDLUAFLT="--lua-filter lua/pagebreak/pagebreak.lua"

 # Create document with pandoc
 ${SED:-sed}                                      \
     -e 's/^----$//g'                             \
     -e 's/<!-- pagebreak -->/\\newpage/g'        \
     -e 's/<!-- br -->/\&nbsp;/g'                 \
     ${INPUTS:?}                                  \
  | ./shell/nopdf.sh                              \
  | ${SAVE_TEE:-cat} | ${PANDOC:-pandoc}          \
     -V geometry:margin=0.7in                     \
     ${ONESIDE:-}                                 \
     ${PDLUAFLT:-}                                \
     ${PDTMPLTE:-}                                \
     ${PDTLSEC:-}                                 \
     --top-level-division=${PDTLDIV:-chapter}     \
     --toc-depth=6                                \
     --toc                                        \
     --dpi=1200                                   \
     --pdf-engine=xelatex                         \
     --pdf-engine-opt=-shell-escape               \
     --pdf-engine-opt=-output-directory=temp.out  \
     --listings                                   \
     -f markdown                                  \
     -t ${HTML_EXT:-pdf}                          \
     "-"                                          \
     -o stage1.${HTML_EXT:-pdf}

 test -f "./input.sav" &&  \
     sed -i 's/scspell-id:/scspell-no:/g' "./input.sav"

 # HTML output ...
 test -z "${BUILDDOC_HTML:-}" ||
   {
      mv -f "stage1.${HTML_EXT:?}"         \
        "stage1${HTML_EXT:?}.zip" &&       \
      mkdir -p "s1temp.tmp" &&             \
        unzip -xoa -d "s1temp.tmp"         \
          "stage1${HTML_EXT:?}.zip" &&     \
        rm -f "stage1${HTML_EXT:?}.zip" ;  \
      exit 0
   }

 # PDF output ...
 printf '%s' "20% ........ "

################################################################################
# Postprocessing - Stage 2 -- Compress

 qpdf                              \
     --flatten-rotation            \
     --recompress-flate            \
     --compression-level=9         \
     --object-streams=generate     \
     --linearize                   \
     --verbose                     \
     "./stage1.pdf"                \
     "./stage2.pdf"  > "/dev/null"

 printf '%s' "40% ........ "

################################################################################
# Postprocessing - Stage 3 -- Minimize

 if [ "${NOBLANKS:-0}" -eq 1 ]; then
   ./shell/removeblank.sh    \
       "./stage2.pdf"  \
       "./stage3.pdf"
 fi

 printf '%s' "60% ........ "

################################################################################
# Postprocessing - Stage 4 -- Recompress

 if [ "${NOBLANKS:-0}" -eq 1 ]; then
   qpdf                              \
       --flatten-rotation            \
       --recompress-flate            \
       --compression-level=9         \
       --object-streams=generate     \
       --linearize                   \
       --verbose                     \
       "./stage3.pdf"                \
       "./stage4.pdf"  > "/dev/null"
 fi

 printf '%s' "80% ........ "

################################################################################
# Finalize - Stage 5 -- Rename

 if [ "${NOBLANKS:-0}" -eq 1 ]; then
   mv -f "./stage4.pdf"   "./complete.out"
 else
   mv -f "./stage2.pdf"   "./complete.out"
 fi

 rm -f    ./*.state                 > "/dev/null"  2>&1 || true
 rm -f    ./.*.state                > "/dev/null"  2>&1 || true
 rm -f    ./yaml/docinfo-post.yml   > "/dev/null"  2>&1 || true
 rm -f    ./*.pdf                   > "/dev/null"  2>&1 || true
 rm -f    ./*.tmp                   > "/dev/null"  2>&1 || true
 rm -f    ./stage*.zip              > "/dev/null"  2>&1 || true
 rm -rf   ./s1epub3.tmp             > "/dev/null"  2>&1 || true
 rm -rf   ./s1temp.tmp              > "/dev/null"  2>&1 || true
 rm -f    ./md/commandref.md        > "/dev/null"  2>&1 || true
 rm -f    ./md/commandref.out       > "/dev/null"  2>&1 || true
 rm -rf   ./temp.out                > "/dev/null"  2>&1 || true
 mv -f   "./complete.out" "./${OUTPUTPDF:?}"
 cp -f   "./${OUTPUTPDF:?}" "./dps8-omnibus.pdf"
 test -f "./${OUTPUTPDF:?}" || exit 3

 printf '%s\n\n' "100% ]"

 printf '%s\n' \
"##############################################################################"

################################################################################
