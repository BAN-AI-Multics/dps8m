# DPS8M simulator: src/Makefile.mk
# vim: filetype=make:tabstop=4:cc=79:noet:ai::list:listchars=tab\:\>\-
# SPDX-License-Identifier: MIT-0
# scspell-id: 1cea05fd-f62b-11ec-b08e-80ee73e9b8e7
# Copyright (c) 2014-2016 Charles Anthony
# Copyright (c) 2016 Michal Tomek
# Copyright (c) 2021 Jeffrey H. Johnson
# Copyright (c) 2021-2024 The DPS8M Development Team

###############################################################################

.NOTPARALLEL:

###############################################################################
# src/Makefile.mk: Macro definitions and operating system detection defaults
###############################################################################

###############################################################################
# Default configuration

COMMAND    ?= command
TRUE       := true
FALSE      := false
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
ENV        := env
CCACHE     ?= ccache
BASENAME   ?= basename
SHELL      ?= /bin/sh
SH         ?= $(SHELL)
UNAME      ?= uname
CPPI       ?= cppi
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
DIRNAME    ?= dirname
PKGCONFIG  ?= pkg-config
GIT        ?= git
CUT        ?= cut
HEAD       ?= head
TAIL       ?= tail
WGET       ?= wget
UNIQ       ?= uniq
CKSUM      ?= cksum
EXPAND     ?= expand
GPG        ?= gpg --batch --status-fd --with-colons
REUSETOOL  ?= reuse
WC         ?= wc
SED        ?= $(ENV) PATH="$$($(COMMAND) -p $(ENV) $(GETCONF) PATH)" sed
AWK        ?= $(shell $(COMMAND) -v gawk 2> /dev/null ||                      \
                $(ENV) PATH="$$($(COMMAND) -p $(ENV) $(GETCONF) PATH)"        \
                  sh -c "$(COMMAND) -v awk" || $(PRINTF) %s\\n awk)
CMP        ?= cmp
CKSUM      ?= cksum
WEBDL      ?= wget
TEE        ?= tee
CD         ?= cd
ORSTLINT   ?= /opt/oracle/developerstudio12.6/bin/lint
TR         ?= tr
ADVZIP     ?= advzip
CAT        ?= cat
GIT        ?= git
GCWD       ?= pwd
WC         ?= wc
EXPAND     ?= expand
RMNF       ?= rm
RMF        ?= $(RMNF) -f
RMDIR      ?= rmdir
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
MAKEDEPEND ?= makedepend
MAKETAR    ?= $(TAR) --owner=$(GTARUSER) --group=$(GTARGROUP) --posix -c      \
                     --transform 's/^/.\/dps8\//g' -$(ZCTV)
TARXT      ?= tar
COMPRESS   ?= gzip -f -9
GUNZIP     ?= gzip -d
GZIP       ?= gzip
GZCAT      ?= $(GZIP) -dc
COMPRESSXT ?= gz
KITNAME    ?= sources
SCSPELLCMD ?= scspell
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
    ifneq "$(findstring gcc,$(CC))" ""
      CFLAGS += -fpie
    endif
  endif
endif

###############################################################################
# Allow override of -DUSE_FLOCK=1 BSD extension detection logic

OVR_FLOCK ?= -DUSE_FLOCK=1

###############################################################################

ifeq ($(UNAME_S),Haiku)
  OS = Haiku
  CFLAGS += $(OVR_FLOCK) -DUSE_FCNTL=1
endif

###############################################################################

ifeq ($(UNAME_S),OpenBSD)
  OS = OpenBSD
  CFLAGS += $(OVR_FLOCK) -DUSE_FCNTL=1 -I/usr/local/include
  LDFLAGS += -L/usr/local/lib
endif

###############################################################################

ifeq ($(UNAME_S),NetBSD)
  OS = NetBSD
  CFLAGS  += -Wno-char-subscripts -I/usr/pkg/include                          \
                 $(OVR_FLOCK) -DUSE_FCNTL=1
  LDFLAGS += -L/usr/pkg/lib -Wl,-R/usr/pkg/lib
endif

###############################################################################

ifeq ($(OS), OSX)
  msys_version = 0
else
  msys_version :=                                                             \
    $(if $(findstring Msys, $(shell                                           \
        $(UNAME) -o 2> /dev/null)),$(word 1,                                  \
            $(subst ., ,$(shell $(UNAME) -r))),0)
endif
export msys_version

###############################################################################
# Try to be somewhat smart about finding an appropriate cross-compiler

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
# Determine NATIVE compiler flags for ICC, GCC, and Clang

ifdef NATIVE
  ifndef NO_NATIVE
    ifndef SUNPRO
      ifndef SUNLINT
      # compiler invocation: Try `-march=native`, fall back to `-mcpu=native`
      NATIVEFLAGS:=$(shell $(CC) -march=native -E -                           \
                       < /dev/null > /dev/null 2>&1                           \
                         && $(PRINTF) '%s\n' "-march=native"                  \
                           || $(PRINTF) '%s\n' "-mcpu=native")
        ifeq "$(findstring clang,$(CC))" ""
          ifeq "$(findstring gcc,$(CC))" ""
            ifeq "$(findstring icc,$(CC))" ""
              # Other compilers
            else
              # Intel C Compiler Classic (no longer supported)
              CFLAGS += -xHost -unroll-aggressive -qopt-mem-layout-trans=3    \
                          -qopt-prefetch=3 -qoverride-limits -fpermissive     \
                            -diag-disable=10121
            endif
          else
            # GCC
            CFLAGS  += $(NATIVEFLAGS)
          endif
        else
          # Clang
          CFLAGS  += $(NATIVEFLAGS) -Wno-ignored-optimization-argument
        endif
      endif
    endif
  endif
endif

###############################################################################
# Default CFLAGS, optimizations, and math library

MATHLIB ?= -lm

ifeq (,$(findstring libuv,$(MAKECMDGOALS)))
  ifdef CI_JOB_ID
    ALLOW_STRICT_ALIASING = 1
    export ALLOW_STRICT_ALIASING
  endif
endif

ifdef ALLOW_STRICT_ALIASING
  STRICT_ALIASING = -fstrict-aliasing
else
  STRICT_ALIASING = -fno-strict-aliasing
endif

ifndef SUNPRO
  ifdef DUMA
    CFLAGS += $(shell $(CC) -E -ftrivial-auto-var-init=pattern -              \
                < /dev/null > /dev/null 2>&1 && printf '%s\n'                 \
                  "-ftrivial-auto-var-init=pattern")
  endif
  ifdef TESTING
    CFLAGS += $(shell $(CC) -E -ftrivial-auto-var-init=zero -                 \
                < /dev/null > /dev/null 2>&1 && printf '%s\n'                 \
                  "-ftrivial-auto-var-init=zero")
  endif
endif

_DEBUGOPTFLAG := -g
ifndef TESTING
  OPTFLAGS = -O3 $(_DEBUGOPTFLAG) -U_FORTIFY_SOURCE -fno-stack-protector
  ifdef DUMA
    CFLAGS   += -I../dps8 -I. -include dps8_duma.h
    OPTFLAGS += -DDUMA=1
    DUMALIBS ?= -l:libduma.a
    export DUMALIBS
  endif
else
  OPTFLAGS = -O0 -g3 -fno-inline -ggdb -fstack-protector-all
  ifdef DUMA
    CFLAGS   += -I../dps8 -I. -include dps8_duma.h
    OPTFLAGS += -DDUMA=1
    DUMALIBS ?= -l:libduma.a
    export DUMALIBS
  endif
endif

ifndef SUNPRO
  CFLAGS  += -Wall $(OPTFLAGS) $(STRICT_ALIASING)
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
# macOS / OS X

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
  CFLAGS += $(OVR_FLOCK) -DUSE_FCNTL=1 $(HOMEBREW_INC)
  LDFLAGS += $(HOMEBREW_LIB)
  endif

###############################################################################
# FreeBSD

  ifeq ($(UNAME_S),FreeBSD)
    CFLAGS += -I/usr/local/include $(OVR_FLOCK) -DUSE_FCNTL=1 -pthread
    LDFLAGS += -L/usr/local/lib
  endif

###############################################################################
# IBM i (OS/400) PASE

  ifeq ($(UNAME_S),OS400)
    KRNBITS:=$(shell getconf KERNEL_BITMODE 2> /dev/null || printf '%s' "64")
    CFLAGS += $(OVR_FLOCK) -DUSE_FCNTL=1 -DHAVE_POPT=1 -maix$(KRNBITS)        \
              -Wl,-b$(KRNBITS) -pthread -D_THREAD_SAFE -Wl,-brtl
    LDFLAGS += $(MATHLIB) -lpthread -lpopt -lbsd -lutil -maix$(KRNBITS)       \
               -Wl,-b$(KRNBITS) -pthread -D_THREAD_SAFE -Wl,-brtl
    CC?=gcc
    export NO_LTO=1
    export ATOMICS=AIX
    export LIBRT=
  endif

###############################################################################
# IBM AIX

  ifeq ($(UNAME_S),AIX)
    KRNBITS:=$(shell getconf KERNEL_BITMODE 2> /dev/null || printf '%s' "64")
    CFLAGS += $(OVR_FLOCK) -DUSE_FCNTL=1 -DHAVE_POPT=1 -maix$(KRNBITS)        \
              -Wl,-b$(KRNBITS)
    LDFLAGS += $(MATHLIB) -lpthread -lpopt -lbsd -maix$(KRNBITS)              \
               -Wl,-b$(KRNBITS)
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

# OpenIndiana illumos and Oracle Solaris 11: GCC 7+, Clang 9+, and SunCC work.

  ifeq ($(UNAME_S),SunOS)
    ifeq ($(shell $(UNAME) -o 2> /dev/null),illumos)
      ISABITS :=$(shell isainfo -b 2> /dev/null || printf '%s' "64")
      CFLAGS  +=-I/usr/local/include -m$(ISABITS) -DUSE_FCNTL=1
      LDFLAGS +=-L/usr/local/lib -lsocket -lnsl $(MATHLIB) -lpthread          \
                -lmtmalloc -ldl -m$(ISABITS)
    endif
    ifeq ($(shell $(UNAME) -o 2> /dev/null),Solaris)
      ISABITS :=$(shell isainfo -b 2> /dev/null || printf '%s' "64")
      CFLAGS  +=-I/usr/local/include -m$(ISABITS) -DUSE_FCNTL=1
      LDFLAGS +=-L/usr/local/lib -lsocket -lnsl $(MATHLIB) -lpthread          \
                -lmtmalloc -lkstat -ldl -m$(ISABITS)
      CC?=gcc
    endif
  endif
endif

###############################################################################
# FreeBSD atomics

ifeq ($(UNAME_S),FreeBSD)
  ifeq ($(CROSS),)
    ifneq ($(CYGWIN_MINGW_CROSS),1)
      ATOMICS?=BSD
    endif
  endif
endif

###############################################################################
# Linux (architecture-specific)

ifeq ($(UNAME_S),Linux)
  ifeq ($(CROSS),)
    ifneq ($(CYGWIN_MINGW_CROSS),1)
      CFLAGS += $(OVR_FLOCK) -DUSE_FCNTL=1
    endif
  endif
  ifndef UNAME_M
    UNAME_M:=$(shell $(UNAME) -m 2> /dev/null)
  endif
  ifeq ($(UNAME_M),m68k)
    export NEED_128=1
  endif
  ifeq ($(UNAME_M),m88k)
    export NO_LTO=1
    export NEED_128=1
  endif
  ifeq ($(UNAME_M),arml)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),arm)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv4l)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv4hf)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv4)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv4t)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv5l)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv5hf)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv5)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv5t)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv5te)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv5tej)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv6l)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv6)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv6j)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv6k)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv6z)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv6kz)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv6zk)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv6t2)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv6-m)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv6hf)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv6)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv6s-m)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv7)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv7l)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv7hf)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv7-a)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv7ve)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv7-r)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv7-m)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),armv7e-m)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),iwmmxt)
    export NEED_128=1
    ATOMICS?=SYNC
  endif
  ifeq ($(UNAME_M),iwmmxt2)
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
# C Standard: Defaults to c11, but gnu11 or c1x may be needed on some systems.

CSTD?=c11

###############################################################################

.PHONY: rebuild.env rebuild.vne .rebuild.env .rebuild.vne FORCE
ifndef REBUILDVNE
FORCE:
rebuild.env rebuild.vne .rebuild.env .rebuild.vne: FORCE
	-@$(PRINTF) '%s\n' "BUILD: Checksum build environment" || $(TRUE)
	@$(SETV); ( $(SET) 2> /dev/null ; $(ENV) 2> /dev/null ) |                 \
      $(GREP) -ivE '(stat|line|time|date|random|seconds|pid|user|\?|hist)' |  \
      $(GREP) -ivE '(old|tty|prev|^_|man|pwd|cwd|columns|gid|uid|grp)' |      \
      $(GREP) -ivE '(tmout|user|^ps.|anyerror|argv|^command|^killring)' |     \
      $(GREP) -ivE '(^shlvl|^mailcheck|^jobmax|^hist|^term|^opterr)' |        \
      $(GREP) -ivE '(^groups|^bash_arg|^dirstack|^psvar|idle)' 2> /dev/null | \
        $(CKSUM) > ".rebuild.vne" 2> /dev/null || $(TRUE)
	@$(SETV); $(TEST) -f ".rebuild.env" || ( $(SET) 2> /dev/null ;            \
      $(ENV) 2> /dev/null ) | $(GREP) -ivE '(stat|line|time|date|random)' |   \
      $(GREP) -ivE '(seconds|pid|user|\?|hist|old|tty|prev|^_|man|pwd|cwd)' | \
      $(GREP) -ivE '(columns|gid|uid|grp|tmout|user|^ps.|anyerror|argv)' |    \
      $(GREP) -ivE '(^command|^killring|^shlvl|^mailcheck|^jobmax|^hist)' |   \
      $(GREP) -ivE '(^term|^opterr|^groups|^bash_arg|^dirstack|^psvar|idle)'  \
        2> /dev/null | $(CKSUM) > ".rebuild.env" 2> /dev/null || $(TRUE)
	@$(SETV); $(PRINTF) '%s\n' "$$($(CAT) .rebuild.env)"                      \
        "$$($(CAT) .rebuild.vne)" > /dev/null 2>&1
	@$(SETV); $(RMF) ".needrebuild"; $(CMP) ".rebuild.env" ".rebuild.vne"     \
        > /dev/null 2>&1 || { $(TOUCH) ".rebuild.vne";  $(PRINTF) '%s'        \
          "BUILD: Checksum updated: $$($(HEAD) -n 1 ".rebuild.env" |          \
     $(TR) -cd "0-9")"; $(CP) ".rebuild.vne" ".rebuild.env" > /dev/null;      \
     $(TEST) -z "$$CI_SKIP_MKREBUILD" && $(TOUCH) ".needrebuild" > /dev/null; \
     $(PRINTF) '%s\n' " -> $$($(HEAD) -n 1 ".rebuild.env" |                   \
       $(TR) -cd "0-9")"; }
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
	@$(PRINTF) '%s\n' "CC: $$($(BASENAME) "$<")"
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
