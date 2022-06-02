#!/usr/bin/env sh
# shellcheck disable=SC2140
# Docs-in-Docker v0.0.1

test -f ./builddoc-docker.sh ||
  {
    printf '%s\n' "Error: Not running in docs directory?"
    exit 1
  }

test -f Dockerfile ||
  {
    printf '%s\n' "Error: No Dockerfile in current directory."
    exit 1
  }

# shellcheck disable=SC2155
export PDIR="$( ( cd .. && pwd -P ) )" 

test -d "${PDIR:?}" ||
  {
    printf '%s\n' "Error: Bad directory ${PDIR:-}"
    exit 1
  }

printf '%s' "Checking Docker: "

docker version > /dev/null 2>&1 ||
  {
    printf '%s\n' "ERROR"
    exit 1
  }

docker run --rm -it hello-world:latest > /dev/null 2>&1 ||
  {
    printf '%\n' "ERROR"
    exit 1
  }

printf '%s\n' "OK"

export DOCBUILD="sh -c \"git config --global --add safe.directory /out && \
                 cd /out/docs && gmake -C .. superclean && \
                 gmake -C .. && ./builddoc.sh\""

set -e

eval docker run --rm -v "${PDIR:?}":"/out" -it \
  "dps8m-docs-helper:Dockerfile" "${DOCBUILD:?}" ||
    {
      printf '%s\n' "Building dps8m-docs-helper container ..." &&
      docker build -t "dps8m-docs-helper:Dockerfile" - < Dockerfile &&
      eval docker run --rm -v "${PDIR:?}":"/out" -it \
        "dps8m-docs-helper:Dockerfile" "${DOCBUILD:?}"
    }
