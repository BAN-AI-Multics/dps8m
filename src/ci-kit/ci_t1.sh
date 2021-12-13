#!/usr/bin/env sh
command -v stdbuf > /dev/null 2>&1 && STDBUF="stdbuf -o L" || STDBUF="exec"
exec ${STDBUF:?} expect -f ci_t1.expect 2>&1
