#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: FSFAP
# scspell-id: 9482306e-f631-11ec-a6ed-80ee73e9b8e7

############################################################################
#
# Copyright (c) 2021-2023 The DPS8M Development Team
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered "AS-IS",
# without any warranty.
#
############################################################################

IFS=

############################################################################

# shellcheck disable=SC2086
UNIFDEF="$(printf '%s' ${0} | sed 's/unifdef\.wrapper\.sh/unifdef/')"

############################################################################

command -v timeout > /dev/null 2>&1 && HT=1

############################################################################

test -z "${HT:-}" || \
  { timeout --preserve-status 20 "${UNIFDEF:?}" "${@}"; exit "${?}"; };

############################################################################

test -z "${HT:-}" && \
  { "${UNIFDEF:?}" "${@}"; exit "${?}"; };

############################################################################

exit 1

############################################################################
