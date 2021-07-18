# DPS/8M simulator: src/Makefile.mk
# vim: filetype=make:tabstop=4:tw=76
#
###############################################################################
#
# Copyright (c) 2014-2016 Charles Anthony
# Copyright (c) 2016 Michal Tomek
# Copyright (c) 2021 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2021 The DPS8M Development Team
#
# All rights reserved.
#
# This software is made available under the terms of the ICU
# License, version 1.8.1 or later.  For more details, see the
# LICENSE.md file at the top-level directory of this distribution.
#
###############################################################################

###############################################################################
# Default configuration

TRUE      ?= true
SET       ?= set
ifdef V
      VERBOSE = 1
endif
ifdef VERBOSE
       ZCTV = cvf
       SETV = $(SET) -x
else
       ZCTV = cf
       SETV = $(TRUE)
endif
ENV        ?= env
CCACHE     ?= ccache
SHELL      ?= sh
SH         ?= $(SHELL)
UNAME      ?= uname
COMM       ?= comm
CPPCPP     ?= $(CC) -E
PREFIX     ?= /usr/local
CSCOPE     ?= cscope
MKDIR      ?= mkdir -p
NDKBUILD   ?= ndk-build
PKGCONFIG  ?= pkg-config
GIT        ?= git
GREP       ?= grep
SORT       ?= sort
CUT        ?= cut
SED        ?= sed
AWK        ?= awk
WEBDL      ?= wget
CD         ?= cd
CMAKE      ?= cmake
RMNF       ?= rm
RMF        ?= $(RMNF) -f
CTAGS      ?= ctags
FIND       ?= find
CP         ?= cp -f
TOUCH      ?= touch
TEST       ?= test
PRINTF     ?= printf
TAR        ?= tar
MAKETAR    ?= $(TAR) --owner=dps8m --group=dps8m --posix -c --transform 's/^/.\/dps8\//g' -$(ZCTV)
TARXT      ?= $(TAR)
COMPRESS   ?= gzip -f -9
GUNZIP     ?= gzip -d
COMPRESSXT ?= gz
KITNAME    ?= sources

###############################################################################

SIMHx=../simh

###############################################################################

ifneq ($(OS),Windows_NT)
  UNAME_S := $(shell  $(UNAME) -s)
  ifeq ($(UNAME_S),Darwin)
    OS = OSX
  endif
endif

###############################################################################

ifeq ($(OS), OSX)
  msys_version = 0
else
  msys_version := \
    $(if $(findstring Msys, $(shell \
        $(UNAME) -o)),$(word 1, \
            $(subst ., ,$(shell $(UNAME) -r))),0)
endif

###############################################################################

ifeq ($(msys_version),0)
else
#  CROSS=MINGW64
endif
ifeq ($(CROSS),MINGW64)
  CC = x86_64-w64-mingw32-gcc
  LD = x86_64-w64-mingw32-gcc
ifeq ($(msys_version),0)
  AR = x86_64-w64-mingw32-ar
else
  AR = ar
endif
  EXE = .exe
else
CC ?= clang
LD ?= clang
endif

###############################################################################
# Fallback for compiler: clang -> gcc -> cc

ifeq ($(CC),clang)
ifneq ($(shell clang --version 2> /dev/null | $(GREP) -q "." 2> /dev/null && \
	printf '%s\n' "1"),1)
CC = gcc
LD = gcc
endif
endif

ifeq ($(LD),clang)
ifneq ($(shell clang --version 2> /dev/null | $(GREP) -q "." 2> /dev/null && \
	printf '%s\n' "1"),1)
CC = gcc
LD = gcc
endif
endif

ifeq ($(CC),gcc)
ifneq ($(shell gcc --version 2> /dev/null | $(GREP) -q "." 2> /dev/null && \
	printf '%s\n' "1"),1)
CC = cc
LD = cc
endif
endif

ifeq ($(LD),gcc)
ifneq ($(shell gcc --version 2> /dev/null | $(GREP) -q "." 2> /dev/null && \
	printf '%s\n' "1"),1)
CC = cc
LD = cc
endif
endif

ifeq ($(LD),ld)
LD = $(CC)
endif

###############################################################################

# Default FLAGS
CFLAGS  += -g -O3 -fno-strict-aliasing
CFLAGS  += $(X_FLAGS)
LDFLAGS += $(X_FLAGS)

###############################################################################
# Windows MINGW

ifeq ($(OS),Windows_NT)
#    CC = gcc
#    LD = gcc
ifeq ($(CROSS),MINGW64)
    CFLAGS  += -I../mingw_include
    LDFLAGS += -L../mingw_lib  
endif

###############################################################################
# macOS

else
    UNAME_S := $(shell $(UNAME) -s 2> /dev/null)
    ifeq ($(UNAME_S),Darwin)
      CFLAGS += -I/usr/local/include
      LDFLAGS += -L/usr/local/lib
    endif

###############################################################################
# FreeBSD

    ifeq ($(UNAME_S),FreeBSD)
      CFLAGS += -I/usr/local/include -pthread
      LDFLAGS += -L/usr/local/lib
    endif

###############################################################################
# SunOS

# OpenIndiana illumos: GCC 7, GCC 10, Clang 9, all supported.
# Oracle Solaris 11: GCC only. Clang and SunCC need some work.

    ifeq ($(UNAME_S),SunOS)
      ifeq ($(shell $(UNAME) -o 2> /dev/null),illumos)
        ISABITS=$(shell isainfo -b 2> /dev/null || printf '%s' "64")
        CFLAGS += -I/usr/local/include -m$(ISABITS)
        LDFLAGS += -L/usr/local/lib -lsocket -lnsl -lm -lpthread -m$(ISABITS)
      endif
      ifeq ($(shell $(UNAME) -o 2> /dev/null),Solaris)
        ISABITS=$(shell isainfo -b 2> /dev/null || printf '%s' "64")
        CFLAGS += -I/usr/local/include -m$(ISABITS)
        LDFLAGS += -L/usr/local/lib -lsocket -lnsl -lm -lpthread -luv -lkstat -ldl -m$(ISABITS)
        CC = gcc
        LD = gcc
      endif
    endif
endif

###############################################################################

ifneq ($(M32),)
CC = gcc
LD = gcc
endif

###############################################################################

CFLAGS += -I../decNumber -I$(SIMHx) -I../include
CFLAGS += -std=c99
CFLAGS += -U__STRICT_ANSI__  
CFLAGS += -D_GNU_SOURCE
CFLAGS += -DUSE_READER_THREAD
CFLAGS += -DUSE_INT64

###############################################################################

ifneq ($(W),)
CFLAGS += -Wno-array-bounds
endif

###############################################################################

include ../Makefile.var

###############################################################################

LDFLAGS   += -g

###############################################################################

MAKEFLAGS += --no-print-directory

###############################################################################

%.o: %.c
	@$(PRINTF) '%s\n' "CC: $<"
	@$(SETV); $(CC) -c $(CFLAGS) $(CPPFLAGS) $(X_FLAGS) $< -o $@

###############################################################################

# This file is included as '../Makefile.mk', so local includes needs the ../
ifneq (,$(wildcard ../Makefile.local))
include ../Makefile.local
endif

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
