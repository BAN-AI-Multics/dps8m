#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:tw=76:expandtab

IFS=
# shellcheck disable=SC2086
UNIFDEF="$(printf '%s' ${0} | sed 's/unifdef\.wrapper\.sh/unifdef/')"
command -v timeout > /dev/null 2>&1 && HT=1
test -z "${HT:-}" || \
  { timeout --preserve-status 20 "${UNIFDEF:?}" "${@}"; exit "${?}"; };
test -z "${HT:-}" && \
  { "${UNIFDEF:?}" "${@}"; exit "${?}"; };
exit 1
