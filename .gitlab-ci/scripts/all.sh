#!/usr/bin/env sh

# Docker build all script
################################################################################
#
# SPDX-License-Identifier: MIT
# scspell-id: 006cf8ac-c974-11ef-8cb1-80ee73e9b8e7
#
# Copyright (c) 2018-2024 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2024-2025 The DPS8M Development Team
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
################################################################################

printf '%s\n' "Sanity check ..."

test -f ./scripts/all.sh 2> /dev/null ||
  {
    printf '%s\n' "ERROR: No ./scripts/all.sh from working directory."
    exit 1
  }

set -eu > /dev/null 2>&1

printf '\n%s\n' "Processing base ..."

(cd base && ../scripts/build.sh && ../scripts/push.sh) ||
  {
    printf '%s\n' "ERROR: base processing failed."
    exit 1
  }

printf '%s\n' "Successfully processed base image."

printf '\n%s\n' "Processing all CI images ..."

for dir in $(printf '%s\n' * | grep -Ev '(^base$|^scripts$)' ); do
    printf '\nProcessing %s ...\n' "${dir:?}"
    (cd "${dir:?}" && ../scripts/build.sh && ../scripts/push.sh) ||
      {
        printf 'ERROR: %s processing failed.\n' "${dir:?}"
        exit 2
      }
    printf 'Successfully processed %s image.\n' "${dir:?}"
done

printf '\n%s\n' "Successfully processed all CI images."
