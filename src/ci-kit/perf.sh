#!/usr/bin/env sh
export SHELL=/bin/sh
command -v stdbuf > /dev/null 2>&1 && STDBUF="stdbuf -o L" || STDBUF="exec"
test -d ./run && test -d ../perf_test && cd ./run && mkdir -p perf_test && \
cp -Rf ../../perf_test/* perf_test && cd perf_test && \
( ${STDBUF:?} ../dps8 -r -q nqueensx.ini > ../../perf.log 2>&1 )
