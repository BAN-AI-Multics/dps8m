#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:tw=76:expandtab

##############################################################################
# Initialization

export SHELL=/bin/sh > /dev/null 2>&1

##############################################################################
# Check (some) basic prerequisites

command -v "pdftk" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: pdftk (pdftk-java) not found."; exit 1; }

command -v "pdfinfo" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: pdfinfo (poppler) not found."; exit 1; }

command -v "gs" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: gs (ghostscript) not found."; exit 1; }

command -v "bc" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: bc not found."; exit 1; }

command -v "basename" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: basename not found."; exit 1; }

command -v "tee" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: tee not found."; exit 1; }

command -v "awk" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: awk not found."; exit 1; }

command -v "seq" > /dev/null 2>&1 ||
  { printf '%s\n' "Error: seq not found."; exit 1; }

##############################################################################
# Check for input and output

IN="${1:?}"
OUT="${2:?}"

##############################################################################
# Transform filename

filename="$(basename "${IN:?}")"
filename="${filename%.*}"

##############################################################################
# Analyze input PDF

PAGES=$(pdfinfo "${IN:?}" | grep -a '^Pages:' | tr -dc '0-9')

##############################################################################
# Find non-blank pages

non_blank() {
  # shellcheck disable=SC2086
  for i in $(seq 1 "${PAGES:?}")
  do
    PERCENT=$(gs -o -                                  \
               -dFirstPage="${i:?}"                    \
               -dLastPage="${i:?}"                     \
               -sDEVICE="ink_cov"                      \
               "${IN:?}" | grep "CMYK" 2> /dev/null |  \
              awk 'BEGIN { sum=0; }
                 {sum += $1 + $2 + $3 + $4;}
                 END { printf "%.5f\n", sum } ')
    if [ "$(printf '%s\n' "${PERCENT:?} > 0.001" | bc -q)" -eq 1 ]; then
      printf '%s\n' "${i:?}"
    fi
  done | tee "${filename:?}.tmp"
}

##############################################################################
# Finalize

# shellcheck disable=SC2046
pdftk "${IN:?}" cat $(non_blank) output "${OUT:?}"

##############################################################################
