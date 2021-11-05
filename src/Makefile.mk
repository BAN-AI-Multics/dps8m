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
# src/Makefile.mk: Macro definitions and operating system detection defaults
###############################################################################
# Default configuration

COMMAND    ?= command
TRUE       ?= true
SET        ?= set
ifdef V
    V = 1
    VERBOSE = 1
    export V
    export VERBOSE
endif
ifdef VERBOSE
       ZCTV = cvf
       SETV = $(SET) -x
else
       ZCTV = cf
       SETV = $(TRUE)
       MAKEFLAGS += --no-print-directory
endif
GETCONF    ?= getconf
ENV        ?= env
CCACHE     ?= ccache
SHELL      ?= sh
SH         ?= $(SHELL)
UNAME      ?= uname
COMM       ?= comm
ECHO       ?= echo
BREW       ?= brew
GREP       ?= $(ENV) PATH="$$($(COMMAND) -p $(ENV) $(GETCONF) PATH)" grep
SORT       ?= sort
CPPCPP     ?= $(CC) -E
CPPCPP2    ?= $(CC) -qshowmacros=pre -E /dev/null < /dev/null 2> /dev/null
CPPCPP3    ?= $(CC) -qshowmacros -E /dev/null < /dev/null 2> /dev/null
CPPCPP4    ?= $(CC) -xdumpmacros=defs -E /dev/null < /dev/null 2>&1
PREFIX     ?= /usr/local
CSCOPE     ?= cscope
MKDIR      ?= mkdir -p
NDKBUILD   ?= ndk-build
DIRNAME    ?= dirname
PKGCONFIG  ?= pkg-config
GIT        ?= git
CUT        ?= cut
UNIQ       ?= uniq
EXPAND     ?= expand
WC         ?= wc
SED        ?= $(ENV) PATH="$$($(COMMAND) -p $(ENV) $(GETCONF) PATH)" sed
AWK        ?= $(ENV) PATH="$$($(COMMAND) -p $(ENV) $(GETCONF) PATH)" awk
CMP        ?= cmp
CKSUM      ?= cksum
WEBDL      ?= wget
TEE        ?= tee
CD         ?= cd
ORSTLINT   ?= /opt/oracle/developerstudio12.6/bin/lint
GLRUNNER   ?= gitlab-runner
TR         ?= tr
ADVZIP     ?= advzip
CAT        ?= cat
GIT        ?= git
WC         ?= wc
EXPAND     ?= expand
CMAKE      ?= cmake
RMNF       ?= rm
RMF        ?= $(RMNF) -f
CTAGS      ?= ctags
ETAGS      ?= etags
GTAGS      ?= gtags
FIND       ?= find
CP         ?= cp -f
INFOZIP    ?= zip
ZIP        ?= $(INFOZIP)
TOUCH      ?= touch
NL         ?= nl
TEST       ?= test
SLEEP      ?= sleep
PASTE      ?= paste
PRINTF     ?= printf
TAR        ?= tar
XARGS      ?= xargs
GTARUSER   ?= dps8m
GTARGROUP  ?= $(GTARUSER)
MAKETAR    ?= $(TAR) --owner=$(GTARUSER) --group=$(GTARGROUP) --posix -c      \
                     --transform 's/^/.\/dps8\//g' -$(ZCTV)
TARXT      ?= tar
COMPRESS   ?= gzip -f -9
GUNZIP     ?= gzip -d
COMPRESSXT ?= gz
KITNAME    ?= sources
SIMHx       = ../simh

###############################################################################

ifndef MAKEFILE_LOCAL_A
  ifneq (,$(wildcard ../GNUmakefile.local))
    MAKEFILE_LOCAL_A=1
    include ../GNUmakefile.local
    $(info INCLUDE: ../GNUmakefile.local)
  endif
endif

###############################################################################

ifndef MAKEFILE_LOCAL_B
  ifneq (,$(wildcard GNUmakefile.local))
    MAKEFILE_LOCAL_B=1
    include GNUmakefile.local
    $(info INCLUDE: GNUmakefile.local)
  endif
endif

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
  msys_version :=                                                             \
    $(if $(findstring Msys, $(shell                                           \
        $(UNAME) -o)),$(word 1,                                               \
            $(subst ., ,$(shell $(UNAME) -r))),0)
endif
export msys_version

###############################################################################
# Try to be smart about finding an appropriate compiler - but not too smart ...

ifeq ($(CROSS),MINGW32)
  ifeq ($(CYGWIN_MINGW_CROSS),1)
    CC = x86_64-w32-mingw32-gcc
  endif
  EXE = .exe
  ifneq ($(msys_version),0)
    CC = gcc
    EXE = .exe
  endif
endif

ifeq ($(CROSS),MINGW64)
  ifeq ($(CYGWIN_MINGW_CROSS),1)
    CC = x86_64-w64-mingw32-gcc
  endif
  ifneq ($(msys_version),0)
    CC = gcc
    EXE = .exe
  else
    AR = ar
  endif
  EXE = .exe
else
  CC ?= clang
endif

###############################################################################
# Default FLAGS

ifndef SUNPRO
  CFLAGS  += -Wall -g3 -O3 -fno-strict-aliasing
endif

CFLAGS  += $(X_FLAGS)
LDFLAGS += $(X_FLAGS)

###############################################################################
# Windows/MinGW-W32/MinGW-W64

ifeq ($(OS),Windows_NT)
  ifeq ($(CROSS),MINGW64)
    CFLAGS  += -I../mingw_include -I../mingw64_include
    LDFLAGS += -L../mingw_lib -L ../mingw64_lib
  endif
  ifeq ($(CROSS),MINGW32)
    CFLAGS  += -I../mingw_include -I../mingw32_include
    LDFLAGS += -L../mingw_lib -L ../mingw32_lib
  endif

###############################################################################
# macOS

else
  UNAME_S := $(shell $(UNAME) -s 2> /dev/null)
  ifeq ($(UNAME_S),Darwin)
    ifndef LIBUV
      ifndef NO_HOMEBREW_LIBS
        HOMEBREW_LIB := $(shell $(PRINTF) '%s\n' $$($(BREW) list -1 libuv     \
         2> /dev/null < /dev/null | $(GREP) -E 'libuv.(a|so)$$' 2> /dev/null  \
          | 2> /dev/null $(XARGS) -n 1 $(DIRNAME) 2> /dev/null | $(SORT) -u   \
           2> /dev/null | while read -r brewpath; do $(PRINTF) '\\-L%s '      \
            "$${brewpath:?}"; done))
        export HOMEBREW_LIB
        HOMEBREW_INC := $(shell $(PRINTF) '%s\n' $$($(BREW) list -1 libuv     \
         2> /dev/null < /dev/null | $(GREP) -E 'uv.h$$' 2> /dev/null          \
          | 2> /dev/null $(XARGS) -n 1 $(DIRNAME) 2> /dev/null | $(SORT) -u   \
           2> /dev/null | while read -r brewpath; do $(PRINTF) '\\-I%s '      \
            "$${brewpath:?}"; done))
        export HOMEBREW_INC
        NO_HOMEBREW_LIBS=complete
        export NO_HOMEBREW_LIBS
      endif
    endif
  CFLAGS += $(HOMEBREW_INC)
  LDFLAGS += $(HOMEBREW_LIB)
  endif

###############################################################################
# FreeBSD

  ifeq ($(UNAME_S),FreeBSD)
    CFLAGS += -I/usr/local/include -pthread
    LDFLAGS += -L/usr/local/lib
  endif

###############################################################################
# IBM AIX

# Operating System: IBM AIX 6.1, 7.1, and 7.2 for IBM POWER7+ on IBM pSeries.
# libuv: IBM AIX Toolbox libuv 1.38.1 (libuv-{devel}-1.38.1-1), or later, and,
# libpopt: IBM AIX Toolbox libpopt 1.18 (libpopt-1.18-1) or later is required.

  ifeq ($(UNAME_S),AIX)
    KRNBITS=$(shell getconf KERNEL_BITMODE 2> /dev/null || printf '%s' "64")
    CFLAGS += -DUSE_FLOCK=1 -DHAVE_POPT=1 -maix$(KRNBITS) -Wl,-b$(KRNBITS)
    LDFLAGS += -lm -lpthread -lpopt -lbsd -maix$(KRNBITS) -Wl,-b$(KRNBITS)
    CC?=gcc
  endif

###############################################################################
# GNU/Hurd

# Debian GNU/Hurd 2021: GNU-Mach 1.8+git20210821 / Hurd-0.9 / i686-AT386
# GCC: Debian 10.2.1-6+hurd.1 and Debian GCC 11.2.0-2
# Clang: Debian clang version 11.0.1-2 and Debian clang version 12.0.1-1

  ifeq ($(UNAME_S),GNU)
    export NEED_128=1
    export OS=GNU
  endif

###############################################################################
# SunOS

# OpenIndiana illumos: GCC 7, GCC 10, Clang 9, all supported.
# Oracle Solaris 11: GCC only. Clang and SunCC need some work.

  ifeq ($(UNAME_S),SunOS)
    ifeq ($(shell $(UNAME) -o 2> /dev/null),illumos)
      ISABITS=$(shell isainfo -b 2> /dev/null || printf '%s' "64")
      CFLAGS  +=-I/usr/local/include -m$(ISABITS)
      LDFLAGS +=-L/usr/local/lib -lsocket -lnsl -lm -lpthread -m$(ISABITS)
    endif
    ifeq ($(shell $(UNAME) -o 2> /dev/null),Solaris)
      ISABITS=$(shell isainfo -b 2> /dev/null || printf '%s' "64")
      CFLAGS  +=-I/usr/local/include -m$(ISABITS)
      LDFLAGS +=-L/usr/local/lib -lsocket -lnsl -lm -lpthread -luv -lkstat    \
                -ldl -m$(ISABITS)
      CC?=gcc
    endif
  endif
endif

###############################################################################
# Linux (architecture-specific)

# Various 32-bit Linux platforms
ifeq ($(UNAME_S),Linux)
  ifndef UNAME_M
    UNAME_M=$(shell $(UNAME) -m 2> /dev/null)
  endif
  ifeq ($(UNAME_M),m68k)
   export NEED_128=1
  endif
  ifeq ($(UNAME_M),arm)
   export NEED_128=1
   ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv5)
   export NEED_128=1
   ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv6)
   export NEED_128=1
   ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv7l)
   export NEED_128=1
   ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),ppc)
   export NEED_128=1
  endif
  ifeq ($(UNAME_M),ppcle)
   export NEED_128=1
  endif
  ifeq ($(UNAME_M),powerpc)
   export NEED_128=1
  endif
  ifeq ($(UNAME_M),sh4)
   export NEED_128=1
  endif
  ifeq ($(UNAME_M),unicore32)
   export NEED_128=1
  endif
  ifeq ($(UNAME_M),nios2)
   export NEED_128=1
  endif
  ifeq ($(UNAME_M),m32r)
   export NEED_128=1
  endif
  ifeq ($(UNAME_M),arm32)
   export NEED_128=1
  endif
  ifeq ($(UNAME_M),arm11)
   export NEED_128=1
  endif
  ifeq ($(UNAME_M),rv32)
   export NEED_128=1
  endif
  export ATOMICS
endif

###############################################################################
# C Standard: Defaults to c99, but c1x or c11 may be needed for modern atomics.

CSTD?=c99

###############################################################################

.PHONY: rebuild.env rebuild.vne .rebuild.env .rebuild.vne FORCE
ifndef REBUILDVNE
FORCE:
rebuild.env rebuild.vne .rebuild.env .rebuild.vne: FORCE
	@$(SETV); ( $(SET) 2> /dev/null ; $(ENV) 2> /dev/null ) | $(GREP) -ivE '(stat|line|time|date|random|seconds|pid|user|\?|hist|old|tty|prev|^_|man|pwd|cwd|columns|gid|uid|grp|tmout|user|^ps.|anyerror|argv|^command|^killring|^shlvl|^mailcheck|^jobmax|^hist|^term|^opterr|^groups|^bash_arg|^dirstack|^psvar|idle)' 2> /dev/null | $(CKSUM) > ".rebuild.vne" 2> /dev/null || $(TRUE)
	@$(SETV); $(TEST) -f ".rebuild.env" || ( $(SET) 2> /dev/null ; $(ENV) 2> /dev/null ) | $(GREP) -ivE '(stat|line|time|date|random|seconds|pid|user|\?|hist|old|tty|prev|^_|man|pwd|cwd|columns|gid|uid|grp|tmout|user|^ps.|anyerror|argv|^command|^killring|^shlvl|^mailcheck|^jobmax|^hist|^term|^opterr|^groups|^bash_arg|^dirstack|^psvar|idle)' 2> /dev/null | $(CKSUM) > ".rebuild.env" 2> /dev/null || $(TRUE)
	@$(SETV); $(PRINTF) '%s\n' "$$($(CAT) .rebuild.env)" "$$($(CAT) .rebuild.vne)" > /dev/null 2>&1
	@$(SETV); $(RMF) ".needrebuild"; $(CMP) ".rebuild.env" ".rebuild.vne" > /dev/null 2>&1 || { $(TOUCH) ".rebuild.vne"; $(CP) ".rebuild.vne" ".rebuild.env"; $(TOUCH) ".needrebuild"; }
	@$(SETV); $(RMF) ".rebuild.vne"
REBUILDVNE=1
export REBUILDVNE
endif

###############################################################################

CFLAGS += -I../decNumber -I$(SIMHx)
CFLAGS += -std=$(CSTD)
CFLAGS += -U__STRICT_ANSI__
CFLAGS += -D_GNU_SOURCE

###############################################################################

ifneq ($(W),)
  CFLAGS +=-Wno-array-bounds
endif

###############################################################################

ifdef OS_IBMAIX
  export OS_IBMAIX
endif

###############################################################################

ifneq (,$(wildcard ../Makefile.var))
  include ../Makefile.var
endif

###############################################################################

%.o: %.c
	@$(PRINTF) '%s\n' "CC: $<"
	@$(SETV); $(CC) $(CFLAGS) $(CPPFLAGS) $(X_FLAGS) -c "$<" -o "$@"          \
    -DBUILDINFO_"$(shell $(PRINTF) '%s\n' "$@"                            |   \
    $(CUT) -f 1 -d '.'                                      2> /dev/null  |   \
    $(TR) -cd '\ [:alnum:]_'                                2> /dev/null ||   \
    $(PRINTF) '%s\n' "fallback"                             2> /dev/null      \
    )"="\"$(shell $(PRINTF) '%s\n'                                            \
    '$(CC) $(CFLAGS) $(CPPFLAGS) $(X_CFLAGS) $(LDFLAGS) $(LOCALLIBS) $(LIBS)' \
    | $(TR) -d '\\"'                                        2> /dev/null  |   \
    $(SED)  -e 's/ -DVER_CURRENT_TIME=....................... /\ /g'          \
            -e 's/^[ \t]*//;s/[ \t]*$$//'                   2> /dev/null  |   \
     $(AWK) -v RS='[,[:space:]]+' '!a[$$0]++{ printf "%s %s ", $$0, RT }'     \
         2> /dev/null | $(SED) -e 's/-I\.\.\/decNumber/\ /g'                  \
             -e 's/-I\.\.\/simh/\ /' -e 's/-I\.\.\/include/\ /g'              \
                 2> /dev/null | $(TR) -s ' '                             ||   \
                     $(PRINTF) '%s\n' "fallback")\""

###############################################################################

ifneq (,$(wildcard ../Makefile.env))
  include ../Makefile.env
endif

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
