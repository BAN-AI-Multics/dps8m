#!/usr/bin/env sh
# vim: filetype=sh:tabstop=4:ai:expandtab
# SPDX-License-Identifier: MIT-0
# scspell-id: 0a0e19f0-4894-11ed-8c0a-80ee73e9b8e7
# Copyright (c) 2022-2024 The DPS8M Development Team

###############################################################################

# Initialize

export SHELL=/bin/sh

###############################################################################

# Strict

set -eu 2> /dev/null 2>&1

###############################################################################

# Global variables

# CPUs for parallel make
CPUS="$(grep -c '^model name' /proc/cpuinfo 2> /dev/null || printf '%s\n' '4')"
export CPUS

# Output filename
OUTPUT_NSIS="dps8m-setup.exe"
export OUTPUT_NSIS

# Final filename
FINAL_NSIS="dps8m-git-win-setup.exe"
export FINAL_NSIS

# Global CFLAGS
#GCFLAGS="-pg"

# Global LDFLAGS
#GLDFLAGS="-pg"

###############################################################################

# Sanity check 1/3

test -x "./build-nsis.sh" ||
  {
    printf '%s\n' "FATAL: No executable ./build-nsis.sh; aborting!"
    exit 1
  }

###############################################################################

# Sanity check 2/3

test -f "./setup.nsi" ||
  {
    printf '%s\n' "FATAL: No ./setup.nsi script; aborting!"
    exit 1
  }

###############################################################################

# Sanity check 3/3

test -f "../GNUmakefile" ||
  {
    printf '%s\n' "FATAL: No ../GNUmakefile; aborting!"
    exit 1
  }

###############################################################################

# Startup notice

test -z "${USE_CI:-}" ||
  {
    printf '\r%s\r\n\r\n' "NOTICE: USE_CI enabled."
    sleep 2 > /dev/null 2>&1
  }

###############################################################################

# Initial clean-up

do_cleanup()
{
printf '%s\n' "######  Clean-up  ###########################################"
rm -f  "${OUTPUT_NSIS:?}"                     2> /dev/null &&              \
rm -f  "${FINAL_NSIS:?}"                      2> /dev/null &&              \
rm -f  "./share/LICENSE.md"                   2> /dev/null &&              \
rm -rf "${HOME:-}/libbacktrace-build"         2> /dev/null &&              \
rm -rf "${HOME:-}/libbacktrace-win32-i686"    2> /dev/null &&              \
rm -rf "${HOME:-}/libbacktrace-win32-x86_64"  2> /dev/null &&              \
rm -rf "${HOME:-}/libuv-build"                2> /dev/null &&              \
rm -rf "${HOME:-}/libuv-win32-i686"           2> /dev/null &&              \
rm -rf "${HOME:-}/libuv-win32-x86_64"         2> /dev/null &&              \
time "${MAKE:-make}" -C ".." -j "${CPUS:?}" distclean
}

###############################################################################

# Build ZIP source kit

do_sourcekit()
{
# XXX TODO: Use already built ZIP source kit for GitLab CI/CD
printf '%s\n' "######  Build source kit  ###################################"
mkdir -p ./source &&                                                       \
( cd .. && time "${MAKE:-make}" distclean &&                               \
  time "${MAKE:-make}" zipdist &&                                          \
  mv -f "sources.zip" "./.nsis/source/dps8m-sources.zip" &&                \
  time "${MAKE:-make}" distclean )
}

###############################################################################

# Build DPS8M Omnibus documentation

do_omnibus()
{
# XXX TODO: Use already built DPS8M Omnibus Documentation for GitLab CI/CD
printf '%s\n' "######  Build DPS8M Omnibus Documentation  ##################"
mkdir -p ./share &&                                                        \
( cd .. && time "${MAKE:-make}" distclean &&                               \
  env CFLAGS="${GCFLAGS:-}" LDFLAGS="${GLDFLAGS:-}"                        \
    WITH_BACKTRACE=1 NATIVE=1 time "${MAKE:-make}" -j "${CPUS:?}" &&       \
  env CFLAGS="${GCFLAGS:-}" LDFLAGS="${GLDFLAGS:-}"                        \
    WITH_BACKTRACE=1 NATIVE=1 time "${MAKE:-make}" docspdf &&              \
  mv -f "docs/dps8-omnibus.pdf" "./.nsis/share/dps8m-omnibus.pdf" &&       \
  "${MAKE:-make}" distclean )
}

###############################################################################

# Verification

do_verify()
{
printf '%s\n' "######  Verification  #######################################"
test -f "./share/dps8m-omnibus.pdf" ||
  {
    printf '%s\n' "ERROR: No Omnibus docs!"
    exit 1
  }
test -f "./source/dps8m-sources.zip" ||
  {
    printf '%s\n' "ERROR: No sourcekit!"
    exit 1
  }
}

###############################################################################

# Build 32-bit libuv v1.x branch

do_uv32()
{
printf '%s\n' "######  Build 32-bit Windows libuv  #########################"
mkdir -p "${HOME:-}/libuv-build" &&                                        \
mkdir -p "${HOME:-}/libuv-win32-i686" &&                                   \
( cd "${HOME:-}/libuv-build" &&                                            \
    wget -v "https://github.com/libuv/libuv/archive/v1.x.zip" &&           \
    unzip -xa "v1.x.zip" && cd "libuv-1.x" && sh ./autogen.sh &&           \
    env CFLAGS="${GCFLAGS:-}" LDFLAGS="${GLDFLAGS:-}"                      \
     CI_SKIP_MKREBUILD=1                                                   \
      ./configure --prefix="${HOME:-}/libuv-win32-i686"                    \
       --enable-static --disable-shared --host="i686-w64-mingw32" &&       \
    "${MAKE:-make}" -j "${CPUS:?}" && "${MAKE:-make}" install )
}

###############################################################################

# Build 64-bit libuv v1.x branch

do_uv64()
{
printf '%s\n' "######  Build 64-bit Windows libuv  #########################"
mkdir -p "${HOME:-}/libuv-build" &&                                        \
mkdir -p "${HOME:-}/libuv-win32-x86_64" &&                                 \
( cd "${HOME:-}/libuv-build/libuv-1.x" && "${MAKE:-make}" distclean &&     \
    sh ./autogen.sh && env CFLAGS="${GCFLAGS:-}" LDFLAGS="${GLDFLAGS:-}"   \
     CI_SKIP_MKREBUILD=1
      ./configure --prefix="${HOME:-}/libuv-win32-x86_64"                  \
       --enable-static --disable-shared --host="x86_64-w64-mingw32ucrt" && \
    "${MAKE:-make}" -j "${CPUS:?}" && "${MAKE:-make}" install &&           \
    "${MAKE:-make}" clean )
}

###############################################################################

# Build 32-bit static dps8

do_sim32()
{
printf '%s\n' "######  Build 32-bit Windows dps8  ##########################"
( cd .. && "${MAKE:-make}" clean && env CC="i686-w64-mingw32-gcc"          \
    CI_SKIP_MKREBUILD=1                                                    \
    CFLAGS="${GCFLAGS:-}                                                   \
            -I${HOME:-}/libuv-win32-i686/include                           \
            -D__MINGW64__ -D_WIN32                                         \
            -pthread"                                                      \
    LDFLAGS="${GLDFLAGS:-}                                                 \
             -static                                                       \
             -L${HOME:-}/libuv-win32-i686/lib                              \
             -lpthread"                                                    \
    LOCALLIBS="-lws2_32 -lpsapi -liphlpapi -lshell32 -lmincore -lversion   \
               -luserenv -luser32 -ldbghelp -lole32 -luuid -lcryptbase"    \
    NEED_128=1                                                             \
    "${MAKE:-make}" CROSS="MINGW64" -j "${CPUS:?}" ) &&                    \
mkdir -p "bin/32-bit" &&                                                   \
  rm -f "bin/32-bit/dps8.exe" &&                                           \
    cp -f "../src/dps8/dps8.exe" "bin/32-bit/dps8.exe" &&                  \
  rm -f "bin/32-bit/punutil/punutil.exe" &&                                \
    cp -f "../src/punutil/punutil.exe" "bin/32-bit/punutil.exe" &&         \
  rm -f "bin/32-bit/prt2pdf/prt2pdf.exe" &&                                \
    cp -f "../src/prt2pdf/prt2pdf.exe" "bin/32-bit/prt2pdf.exe"
}

###############################################################################

# Build 64-bit static dps8

do_sim64()
{
printf '%s\n' "######  Build 64-bit Windows dps8  ##########################"
( cd .. && "${MAKE:-make}" clean && env CC="x86_64-w64-mingw32ucrt-gcc"    \
    CI_SKIP_MKREBUILD=1                                                    \
    CFLAGS="${GCFLAGS:-}                                                   \
            -I${HOME:-}/libuv-win32-x86_64/include                         \
            -D__MINGW64__ -D_WIN32                                         \
            -pthread"                                                      \
    LDFLAGS="${GLDFLAGS:-} -static                                         \
             -L${HOME:-}/libuv-win32-x86_64/lib                            \
             -lpthread"                                                    \
    LOCALLIBS="-lws2_32 -lpsapi -liphlpapi -lshell32 -lmincore -lversion   \
               -luserenv -luser32 -ldbghelp -lole32 -luuid -lcryptbase"    \
    "${MAKE:-make}" CROSS="MINGW64" -j "${CPUS:?}" ) &&                    \
mkdir -p "bin/64-bit" &&                                                   \
  rm -f "bin/64-bit/dps8.exe" &&                                           \
    cp -f "../src/dps8/dps8.exe" "bin/64-bit/dps8.exe" &&                  \
  rm -f "bin/64-bit/punutil/punutil.exe" &&                                \
    cp -f "../src/punutil/punutil.exe" "bin/64-bit/punutil.exe" &&         \
  rm -f "bin/64-bit/prt2pdf/prt2pdf.exe" &&                                \
    cp -f "../src/prt2pdf/prt2pdf.exe" "bin/64-bit/prt2pdf.exe"
}

###############################################################################

# Build installer

do_nsis()
{
printf '%s\n' "######  Build DPS8M NSIS Installer  #########################"
cp -f "../LICENSE.md" "./share/LICENSE.md" &&                              \
makensis -HDRINFO &&                                                       \
env PATH=.:"${PATH:?}" makensis -V4 -- "setup.nsi" &&                      \
mv -f "${OUTPUT_NSIS:?}" "${FINAL_NSIS:?}" && file "${FINAL_NSIS:?}" &&    \
ls -la "${FINAL_NSIS:?}"
}

###############################################################################

# Copy CI files

test -z "${USE_CI:-}" ||
  {
    mkdir -p "share" &&                                                \
      cp -f "../docs/dps8-omnibus.pdf" "./share/dps8m-omnibus.pdf" &&  \
    mkdir -p "source" &&                                               \
      cp -f "../dps8m-git-src.tar.gz" "./source/dps8m-sources.zip"
  }

# Clean-up

do_cleanup

test -z "${USE_CI:-}" &&
  {
    rm -f  "./share/dps8m-omnibus.pdf"   2> /dev/null
    rm -f  "./source/dps8m-sources.zip"  2> /dev/null
  }

# Source kit, Omnibus docs

test -z "${USE_CI:-}" &&
  {
    do_sourcekit
    do_omnibus
  }

# Verify

do_verify

# Build libuv

do_uv32
do_uv64

# Build dps8

do_sim32
do_sim64

# Build installer

do_nsis

###############################################################################
