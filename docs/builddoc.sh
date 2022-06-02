#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:tw=78:expandtab
# shellcheck disable=SC2086
# WARNING: Removes all `*.pdf`, `*.tmp`, and `*.state` files during operation.
# WARNING: Installs the templates to `${HOME}/.local/share/pandoc/templates/`.

##############################################################################
# Initialization

set -e
export SHELL=/bin/sh > /dev/null 2>&1
mkdir -p "${HOME:-}/.local/share/pandoc/templates"
cp -f "./eisvogel-dps8.latex" "${HOME:-}/.local/share/pandoc/templates/"
set +e

##############################################################################
# Highlight style

LISTSTYLE="--listings"                   # LaTeX listings
#LISTSTYLE="--highlight-style pygments"  # Haskell skylighting

##############################################################################
# Inputs

 INPUTS="docinfo.yml" # <------------------------ Start of document
 INPUTS="${INPUTS:-} authors.md" # <------------- Authors
#INPUTS="${INPUTS:-} acknowledgements.md" <------ Acknowledgements
#INPUTS="${INPUTS:-} forward.md" # <------------- Forward
#INPUTS="${INPUTS:-} preface.md" # <------------- Preface
#INPUTS="${INPUTS:-} introduction.md" # <-------- Introduction
#INPUTS="${INPUTS:-} prologue.md" # <------------ Prologue
#INPUTS="${INPUTS:-} buildsource.md" # <--------- Building from Source Code
 INPUTS="${INPUTS:-} commandref.md" # <---------- Simulator Command Reference
 INPUTS="${INPUTS:-} licensing.md" # <----------- Licensing Terms and Legal

##############################################################################
# Check (some) basic prerequisites

command -v "git" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: git not found."; exit 1; }

command -v "awk" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: awk not found."; exit 1; }

command -v "${PANDOC:-pandoc}" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: ${PANDOC:-pandoc} not found."; exit 1; }

command -v "qpdf" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: qpdf not found." exit 1; }

command -v "xelatex" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: xelatex not found."; exit 1; }

test -x "../src/dps8/dps8" ||
  { printf '%s\n' "Error: dps8 not found."; exit 1; }

test -x "../src/punutil/punutil" ||
  { printf '%s\n' "Error: punutil not found."; exit 1; }

test -x "../src/unifdef/unifdef" ||
  { printf '%s\n' "Error: unifdef not found."; exit 1; }

test -x "../src/prt2pdf/prt2pdf" ||
  { printf '%s\n' "Error: prt2pdf not found."; exit 1; }

command -v "pdftk" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: pdftk (pdftk-java) not found."; exit 1; }

command -v "pdfinfo" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: pdfinfo (poppler) not found."; exit 1; }

command -v "gs" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: gs (ghostscript) not found."; exit 1; }

command -v "bc" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: bc not found."; exit 1; }

command -v "seq" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: seq not found."; exit 1; }

command -v "file" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: file utility not found."; exit 1; }

test -x "./builddoc.sh" ||
  { printf '%s\n' "Error: Invalid working directory"; exit 1; }

##############################################################################
# Clean-up

rm -f  ./*.state        > /dev/null  2>&1 || true
rm -f  ./.*.state       > /dev/null  2>&1 || true
rm -f  ./docinfo.yml    > /dev/null  2>&1 || true
rm -f  ./*.pdf          > /dev/null  2>&1 || true
rm -f  ./*.tmp          > /dev/null  2>&1 || true
rm -f  ./complete.out   > /dev/null  2>&1 || true
rm -f  ./commandref.md  > /dev/null  2>&1 || true

ln -fs ./color-rings/color-rings.pdf ./color-rings.pdf || exit 2

##############################################################################
# Stop on errors

set -e

##############################################################################
# Global variables

printf '\n%s\n' \
    "########################################################################"

BUILDUTC=$(env TZ=UTC date -u +"%Y-%m-%d %H:%M:%S %Z")
  printf '  Documentation build date : %s\n' "${BUILDUTC:?}"

PROMVERX="$(printf '%s%s\n'                                                 \
         "$(grep VER_H_GIT_RELT ../src/dps8/ver.h | cut -d '"' -f2)"        \
         "$(grep VER_H_PROM_VER_TEXT ../src/dps8/ver.h | cut -d '"' -f2)" | \
         tr -s ' ' | tr -d ' /')"

BUILDVER="$(../src/dps8/dps8 --version 2>&1 |
            awk '/ simulator / { print $3 }' | tr -d ' */')"
  printf '  DPS8M Simulator version  : %s\n' "${BUILDVER:?}"

BUILDGIT="$(env TZ=UTC git rev-parse HEAD | tr -d ' */')"
  printf '  Distribution commit hash : %s\n' "${BUILDGIT:?}"

PRT2PDFV="$(../src/prt2pdf/prt2pdf -v 2>&1 |
            awk '/^prt2pdf version / { print $3 }' | tr -d ' */')"
  printf '  Prt2PDF version          : %s\n' "${PRT2PDFV:?}"

PUNUTILV="$(../src/punutil/punutil --version 2>&1 < /dev/null |
            awk '/^Version/ { print $2 }' | tr -d ' */')"
  printf '  PunUtil version          : %s\n' "${PUNUTILV:?}"

UNIFDEFV="$(../src/unifdef/unifdef -v 2>&1 |
            awk '/Version: / { print $2 }' | tr -d ' */')"
  printf '  Unifdef version          : %s\n' "${UNIFDEFV:?}"

MCMBVERS="$(../src/mcmb/mcmb -v 2>&1 | cut -d ' ' -f 5- | tr -d '*/')"
  printf '  mcmb version             : %s\n' "${MCMBVERS:?}"

VMPCVERS="$(../src/vmpctool/vmpctool -V 2>&1 | cut -d ' ' -f 2 | tr -d '*/')"
  printf '  vmpctool version         : %s\n' "${VMPCVERS:?}"

LASTMODV="$(cd .. && env TZ=UTC git log -1  \
            --format="%cd"                  \
            --date=format-local:'%Y-%m-%d %H:%M:%S UTC' | tr -d '*')"
  printf '  Last modification date   : %s\n' "${LASTMODV:?}"

#if [ "${ONESIDE:-0}" -eq 1 ]; then
#  ONESIDE="-V classoption=oneside"
#  printf '  Document layout          : %s\n' "One-sided output"
#else
#  ONESIDE=""
#  printf '  Document layout          : %s\n' "Standard book output"
#  unset ONESIDE > /dev/null 2>&1 || true
#fi

printf '%s\n\n' \
    "########################################################################"

##############################################################################
# Output filename

export OUTPUTPDF="dps8-${BUILDVER:-unknown}.pdf"

rm -f "${OUTPUTPDF:?}" > /dev/null 2>&1 || true

##############################################################################
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
    -e "s/##BUILDGIT##/${BUILDGIT:?}/"  \
         docinfo.yml.in                 \
           > docinfo.yml

##############################################################################
# Generate Command Reference

# shellcheck disable=SC2016
(
 stdbuf -o L ./refgen.sh |
  stdbuf -o L grep --line-buffered '^## ' |
   stdbuf -o L sed 's/^## //' |
    stdbuf -o L xargs -n1 \
     printf '%s\n' 
) |
stdbuf -o L \
awk '{ printf("\r Generating command reference: %s                 \r",$0); }'
printf '\r%s\r' "                                                            "

##############################################################################
# Processing - Stage 1

printf '%s' "[ 0% ........ "

sed                                        \
    -e 's/^----$//g'                       \
    -e 's/<!-- pagebreak -->/\\newpage/g'  \
    -e 's/<!-- br -->/\&nbsp;/g'           \
    ${INPUTS}                              \
 | ${PANDOC:-pandoc}                       \
    -V geometry:margin=0.7in               \
    ${ONESIDE}                             \
    --top-level-division=chapter           \
    --lua-filter pagebreak.lua             \
    --toc-depth=6                          \
    --toc                                  \
    --dpi=1200                             \
    --shift-heading-level-by=0             \
    --template eisvogel-dps8               \
    --pdf-engine=xelatex                   \
    ${LISTSTYLE:?}                         \
    -f markdown                            \
    -t pdf -                               \
    -o stage1.pdf

printf '%s' "20% ........ "

##############################################################################
# Postprocessing - Stage 2

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

##############################################################################
# Postprocessing - Stage 3

if [ "${NOBLANKS:-0}" -eq 1 ]; then
  ./removeblank.sh    \
      "./stage2.pdf"  \
      "./stage3.pdf"
fi

printf '%s' "60% ........ "

##############################################################################
# Postprocessing - Stage 4

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

##############################################################################
# Finalize - Stage 5

if [ "${NOBLANKS:-0}" -eq 1 ]; then
  mv -f "./stage4.pdf"   "./complete.out"
else
  mv -f "./stage2.pdf"   "./complete.out"
fi

rm -f    ./*.state       > /dev/null  2>&1 || true
rm -f    ./.*.state      > /dev/null  2>&1 || true
rm -f    ./docinfo.yml   > /dev/null  2>&1 || true
rm -f    ./*.pdf         > /dev/null  2>&1 || true
rm -f    ./*.tmp         > /dev/null  2>&1 || true
rm -f    ./commandref.md > /dev/null  2>&1 || true
mv -f   "./complete.out" "./${OUTPUTPDF:?}"
cp -f   "./${OUTPUTPDF:?}" "./dps8-omnibus.pdf"
test -f "./${OUTPUTPDF:?}" || exit 3

printf '%s\n\n' "100% ]"

##############################################################################
