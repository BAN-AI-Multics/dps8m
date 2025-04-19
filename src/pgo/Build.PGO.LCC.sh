#!/usr/bin/env bash
# scspell-id: 280cf1ec-e2a0-11ef-bca9-80ee73e9b8e7
# SPDX-License-Identifier: MIT-0
# Copyright (c) 2021-2025 The DPS8M Development Team
# shellcheck disable=SC2312

# PGO для МЦСТ Эльбрус 2000 (MCST Elbrus) LCC v1.26+
# Протестировано на LCC v1.27.17 и v1.29.06 на ОС Эльбрус и Альт

# Примечание: В настоящее время это не создает и не связывает статически
# libuv.  Это связано с некоторыми обнаруженными проблемами, которые могут
# быть исправлены в более поздних версиях компилятора LCC!

# Создание профиля с использованием бенчмарка "nqueensx" с текущим LCC
# недостаточно хорошо, поскольку компилятор LCC 1.27-1.29 не поддерживает
# GCC-эквивалент `-fprofile-partial-training`, а также
# `-fno-profile-sample-accurate` и `--sparse=true` из LLVM!
#
# Установите для переменной окружения значение `STAGE1_ONLY` или
# `STAGE2_ONLY', если вы собираетесь выполнять собственное профилирование.
# Обязательно переместите файлы `eprof.*` в каталог `src/dps8` перед
# началом `STAGE2_ONLY`.

# Используйте на свой страх и риск!

set -e

# Banner
printf '%s\n' "Прежде чем продолжить, прочтите комментарии в этом файле!"
sleep 2 2> /dev/null || true

# Sanity test
printf '\n%s\n' "Проверка скрипта PGO ..."
test -x "./src/pgo/Build.PGO.LCC.sh" \
  || {
    printf '%s\n' "ОШИБКА: Не удалось найти скрипт PGO!"
    exit 1
  }

# Ensure MCST tooling is in our PATH
printf '%s\n' "${PATH:-}" | grep -q '/opt/mcst/bin' \
  || {
    PATH="/opt/mcst/bin:${PATH:-}"
    export PATH
  }

# Enable CI_SKIP_MKREBUILD
CI_SKIP_MKREBUILD=1
export CI_SKIP_MKREBUILD

# CC
test -z "${CC:-}" && CC="lcc"
export CC

# Test CC
printf '\nCC: %s\n' "${CC:?}"
${CC:?} --version

# AR
test -z "${AR:-}" && AR="ar"
export AR

# Test AR
printf '\nAR: %s\n' "${AR:?}"
command -v "${AR:?}" \
  || { printf '%s\n' "${AR:?} not found!"; exit 1; }

# RANLIB
test -z "${RANLIB:-}" && RANLIB="ranlib"
export RANLIB

# Test RANLIB
printf '\nRANLIB: %s\n' "${RANLIB:?}"
command -v "${RANLIB:?}" \
  || { printf '%s\n' "${RANLIB:?} not found!"; exit 1; }

# Setup
printf '\n%s\n' "Настройка сборки PGO ..."
# LCC-specific optimizations are enabled because CC is set to `*lcc*`.
# Look for "MCST" in `src/Makefile.mk` file for more details.
# Leave the leading space here, to avoid "variable unset" errors.
export BASE_LDFLAGS=" ${LDFLAGS:-}"
export BASE_CFLAGS=" ${CFLAGS:-}"

# Profile build
test "${STAGE2_ONLY:-}" \
  || {
    printf '\n%s\n' "Создание профиля сборки ..."
    export CFLAGS="-fprofile-generate ${BASE_CFLAGS:?}"
    export LDFLAGS="${BASE_LDFLAGS:?} ${CFLAGS:?}"
    # We make clean, not distclean, in case the user builds a local libuv!
    ${MAKE:-make} clean "${@}"
    ${MAKE:-make} dps8 "${@}"

    # Exit here if `STAGE1_ONLY` is set.
    test -z "${STAGE1_ONLY:-}" \
      || {
       printf '%s\n' 'Выход сейчас, так как `STAGE1_ONLY` был включен!'
       printf '%s\n' 'Создайте собственные данные профилирования'
       printf '%s\n' 'и поместите их в каталог `./src/dps8`.'
       exit 0
     }

    # Profile
    printf '\n%s\n' "Создать профиль ..."
    (cd src/perf_test && ../dps8/dps8 -r ./nqueensx.ini)
  }

# Final build
printf '\n%s\n' "Создание окончательной оптимизированной сборки ..."
export CFLAGS="-fprofile-use ${BASE_CFLAGS:?}"
export LDFLAGS="${BASE_LDFLAGS:?} ${CFLAGS:?}"
# We make clean, not distclean, in case the user builds a local libuv!
${MAKE:-make} clean "${@}"
# Move profiling data where lcc expects to find it, if not STAGE2_ONLY
test -z "${STAGE2_ONLY:-}" && \
  mv -f src/perf_test/eprof.*.sum src/dps8 2> /dev/null || true
# Create temporary empty mcmb/tap2raw/prt2pdf/punutil (so we won't build them)
touch src/mcmb/mcmb src/tap2raw/tap2raw src/prt2pdf/prt2pdf src/punutil/punutil || true
${MAKE:-make} dps8 "${@}"
# Cleanup temporary empty mcmb/tap2raw/prt2pdf/punutil
rm -f src/mcmb/mcmb src/tap2raw/tap2raw src/prt2pdf/prt2pdf src/punutil/punutil 2> /dev/null
rm -f src/perf_test/eprof.*.sum src/dps8/eprof.*.sum 2> /dev/null
# Now actually build these (non-PGO'd) utilities, using regular flags.
export CFLAGS="${BASE_CFLAGS:?}"
export LDFLAGS="${BASE_LDFLAGS:?}"
${MAKE:-make} mcmb tap2raw prt2pdf punutil "${@}"
