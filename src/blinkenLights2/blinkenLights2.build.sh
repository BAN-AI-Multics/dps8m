#!/usr/bin/env sh
set -eux > /dev/null 2>&1
env "${MAKE:-make}" -C "../dps8" "shm.o" || gmake -C "../dps8" "shm.o"
# shellcheck disable=SC2086
${CC:-cc} blinkenLights2.c $(env pkg-config gtk+-3.0 --cflags --libs) -I"../simh" -I"../dps8" -DLOCKLESS -DM_SHARED "../dps8/shm.o" -o blinkenLights2
