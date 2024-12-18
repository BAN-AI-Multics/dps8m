# DPS8M simulator: src/dps8m/GNUmakefile
# vim: filetype=make:tabstop=4:cc=79:ai:noexpandtab:list:listchars=tab\:\>\-
# SPDX-License-Identifier: ICU
# scspell-id: 42ec66a7-f62f-11ec-810d-80ee73e9b8e7

###############################################################################
#
# Copyright (c) 2013-2016 Harry Reed
# Copyright (c) 2013-2016 Charles Anthony
# Copyright (c) 2016 Jean-Michel Merliot
# Copyright (c) 2016 Michal Tomek
# Copyright (c) 2021 Jeffrey H. Johnson
# Copyright (c) 2021-2024 The DPS8M Development Team
#
# This software is made available under the terms of the ICU License.
# See the LICENSE.md file at the top-level directory of this distribution.
#
###############################################################################

.NOTPARALLEL:

###############################################################################

.DEFAULT_GOAL := all

###############################################################################

INSTALL_ROOT ?= $(PREFIX)
INSTALL_BIN  ?= $(INSTALL_ROOT)/bin
###############################################################################

ifneq (,$(wildcard ../Makefile.mk))
  include ../Makefile.mk
endif

###############################################################################

ifdef BUILD_PROM_OSV_TEXT
  CFLAGS += -DBUILD_PROM_OSV_TEXT="\"$(BUILD_PROM_OSV_TEXT)\""
endif

###############################################################################

ifdef BUILD_PROM_OSA_TEXT
  CFLAGS += -DBUILD_PROM_OSA_TEXT="\"$(BUILD_PROM_OSA_TEXT)\""
endif

###############################################################################
# If using Cygwin as a host for cross-compilation, set CYGWIN_MINGW_CROSS=1

ifneq ($(CYGWIN_MINGW_CROSS),1)
  ifeq ($(shell $(UNAME) -o 2> /dev/null || true >/dev/null 2>&1),Cygwin)
    CROSS=""
  endif
endif

###############################################################################

ifndef LIBSIR
  LIBSIR = $(wildcard ../libsir/build/obj/src/*.o)
endif

###############################################################################

ifndef LIBUV
  LIBUV = -luv
endif
LIBS += $(LIBUV)

###############################################################################
# Support for MinGW-W32, MinGW-W64, LLVM-MinGW32/64 (i686/x86_64/ARM/AArch64)

ifneq ($(msys_version),0)
  CROSS=MINGW64
  MINGW_FLAVOR=64
  MINGW_CROSS=1
  export CROSS
  export MINGW_FLAVOR
  export MINGW_CROSS
endif

ifeq ($(CROSS),MINGW32)
  MINGW_CROSS=1
  MINGW_FLAVOR=32
  M32=$(MINGW_FLAVOR)
  ifeq ($(msys_version),)
    MINGWEX=-lmingwex
    export MINGWEX
  endif
  export MINGW_CROSS
  export MINGW_FLAVOR
  export M32
endif

ifeq ($(CROSS),MINGW64)
  MINGW_CROSS=1
  MINGW_FLAVOR=64
  ifeq ($(msys_version),)
    MINGWEX=-lmingwex
    export MINGWEX
  endif
  export MINGW_CROSS
  export MINGW_FLAVOR
endif

ifeq ($(MINGW_CROSS),1)
  CFLAGS += -DCROSS_MINGW$(MINGW_FLAVOR) -I../../../libuv/include
  # Cross-compilation with Cygwin host to MinGW target
  ifeq ($(CYGWIN_MINGW_CROSS),1)
    LIBS += -lwinmm -lws2_32 -lssp $(MINGWEX) -lwsock32 -luserenv -liplpapi   \
            -ldbghelp -lversion -lmincore -lcryptbase -lshlwapi               \
            -L../../../libuv/.libs $(LIBUV)
  endif
  ifeq ($(shell $(UNAME) -o 2> /dev/null || true >/dev/null 2>&1),Cygwin)
    # Standard Cygwin build
    LIBS += -lwinmm -lws2_32 -lwsock32 -luserenv -liphlpapi -ldbghelp         \
            -lversion -lmincore -lcryptbase -lshlwapi $(LIBUV)
  else
    # Cross-compilation to MinGW
    LIBS += -lwinmm -lws2_32 -lssp $(MINGWEX) -lwsock32 -luserenv -liphlpapi  \
            -ldbghelp -lversion -lmincore -lcryptbase -lshlwapi               \
            -L../../../libuv/.libs $(LIBUV)
  endif
endif

###############################################################################
# Include directories

CFLAGS += -I../decNumber -I$(SIMHx) -I$(LIBSIRx)

###############################################################################
# Warning level tweaks

ifneq ($(W),)
  ifeq ($(CC),clang)
    CFLAGS += -Wall -Wconversion -Wsign-conversion -Wno-unknown-warning-option
  else
    ifeq ($(CC),gcc)
      CFLAGS += -Wall -Wno-conversion -Wsign-conversion -Wno-psabi -Wno-abi
    else
      CFLAGS += -Wall
    endif
  endif
endif

###############################################################################
# Need to create a 128-bit type?

ifneq ($(M32),)
  CFLAGS += -DNEED_128
  ifneq ($(W),)
    CFLAGS += -Wno-format-zero-length -Wno-sign-conversion
  endif
endif

###############################################################################
# Need to link with -lnetwork?

ifeq ($(UNAME_S),Haiku)
  LIBS += -lnetwork
endif

###############################################################################
# The system_state code relies on M_SHARED.

LIBRT ?= -lrt
ifneq ($(MINGW_CROSS),1)
  CFLAGS += -DM_SHARED
  ifneq ($(OS),OSX)
    ifneq ($(UNAME_S),OpenBSD)
      ifneq ($(UNAME_S),Haiku)
        LIBS += $(LIBRT)
      endif
    endif
  endif
endif

###############################################################################
# What needs -ldl?

LIBDL ?= -ldl
OSTYPE = $(shell $(UNAME) -s 2> /dev/null)
ifeq ($(OSTYPE),AIX)
  LIBS += $(LIBDL)
endif
ifeq ($(OSTYPE),FreeBSD)
  LIBS += $(LIBDL) -pthread
endif
ifeq ($(OSTYPE),Linux)
  ifndef CROSS
    ifndef MINGW_CROSS
      LIBS += $(LIBDL)
    endif
  endif
endif

###############################################################################

C_SRCS  = bsd_random.c
ifneq ($(MINGW_CROSS),1)
  C_SRCS += shm.c
endif
ifeq ($(WITH_ABSI_DEV),1)
  ifneq ($(MINGW_CROSS),1)
    C_SRCS += dps8_absi.c
    C_SRCS += udplib.c
    ENABLED_UDPLIB_C=1
  endif
endif
C_SRCS += dps8_addrmods.c
C_SRCS += dps8_append.c
C_SRCS += dps8_cable.c
C_SRCS += dps8_console.c
C_SRCS += dps8_cpu.c
C_SRCS += dps8_crdpun.c
C_SRCS += dps8_crdrdr.c
C_SRCS += dps8_decimal.c
C_SRCS += dps8_disk.c
C_SRCS += dps8_eis.c
C_SRCS += dps8_faults.c
C_SRCS += dps8_fnp2.c
C_SRCS += dps8_fnp2_iomcmd.c
C_SRCS += dps8_iefp.c
C_SRCS += dps8_ins.c
C_SRCS += dps8_iom.c
C_SRCS += dps8_math.c
C_SRCS += dps8_math128.c
ifeq ($(WITH_MGP_DEV),1)
  ifneq ($(MINGW_CROSS),1)
    C_SRCS += dps8_mgp.c
  endif
endif
C_SRCS += dps8_mt.c
C_SRCS += dps8_opcodetable.c
C_SRCS += dps8_prt.c
C_SRCS += dps8_scu.c
C_SRCS += dps8_simh.c
ifeq ($(WITH_SOCKET_DEV),1)
  ifneq ($(MINGW_CROSS),1)
    C_SRCS += dps8_socket_dev.c
  endif
endif
ifeq ($(TESTING),1)
  C_SRCS += tracker.c
endif
C_SRCS += dps8_sys.c
C_SRCS += dps8_urp.c
C_SRCS += dps8_utils.c
C_SRCS += fnpuv.c
C_SRCS += fnptelnet.c
C_SRCS += libtelnet.c
ifeq ($(PANEL68),1)
  C_SRCS += panelScraper.c
endif
C_SRCS += hdbg.c
C_SRCS += segldr.c
C_SRCS += ucache.c
ifeq ($(THREADZ),1)
  C_SRCS += threadz.c
  ENABLED_THREADZ_C=1
endif
ifeq ($(LOCKLESS),1)
  ifneq ($(ENABLED_THREADZ_C),1)
    C_SRCS += threadz.c
  endif
endif
C_SRCS += utfile.c
C_SRCS += uvutil.c

###############################################################################

OBJS  := $(patsubst %.c,%.o,$(C_SRCS))

###############################################################################

ifneq ($(WITH_ABSI_DEV),1)
  H_SRCS  = dps8_absi.h
endif
ifneq ($(WITH_MGP_DEV),1)
  H_SRCS  = dps8_mgp.h
endif
H_SRCS += bsd_random.h
H_SRCS += dps8_addrmods.h
H_SRCS += dps8_append.h
H_SRCS += dps8_cable.h
H_SRCS += dps8_console.h
H_SRCS += dps8_cpu.h
H_SRCS += dps8_crdpun.h
H_SRCS += dps8_crdrdr.h
H_SRCS += dps8_decimal.h
H_SRCS += dps8_disk.h
H_SRCS += dps8_eis.h
H_SRCS += dps8_em_consts.h
H_SRCS += dps8_faults.h
H_SRCS += dps8_fnp2.h
H_SRCS += dps8_fnp2_iomcmd.h
H_SRCS += libtelnet.h
H_SRCS += dps8.h
H_SRCS += dps8_hw_consts.h
H_SRCS += dps8_iefp.h
H_SRCS += dps8_ins.h
H_SRCS += dps8_iom.h
H_SRCS += dps8_math.h
H_SRCS += dps8_math128.h
H_SRCS += dps8_mt.h
H_SRCS += dps8_opcodetable.h
H_SRCS += dps8_prt.h
H_SRCS += dps8_socket_dev.h
H_SRCS += dps8_scu.h
H_SRCS += dps8_simh.h
H_SRCS += dps8_state.h
H_SRCS += dps8_sys.h
H_SRCS += dps8_urp.h
H_SRCS += dps8_utils.h
H_SRCS += fnpuv.h
H_SRCS += fnptelnet.h
H_SRCS += h316_imp.h
ifeq ($(PANEL68),1)
  C_SRCS += panelScraper.h
endif
H_SRCS += hdbg.h
H_SRCS += segldr.h
H_SRCS += shm.h
ifneq ($(MINGW_CROSS),1)
  H_SRCS += udplib.h
endif
H_SRCS += utfile.h
ifeq ($(THREADZ),1)
  H_SRCS += threadz.h
endif
ifeq ($(LOCKLESS),1)
  H_SRCS += threadz.h
endif
ifeq ($(TESTING),1)
  H_SRCS += tracker.h
endif
H_SRCS += uvutil.h
H_SRCS += ucache.h

###############################################################################

SIMH_OBJS = ../simh/sim_console.o  ../simh/sim_disk.o  ../simh/sim_tape.o     \
            ../simh/sim_fio.o      ../simh/sim_sock.o  ../simh/scp.o          \
            ../simh/sim_timer.o    ../simh/sim_tmxr.o  ../simh/linehistory.o

###############################################################################

DECN_OBJS = ../decNumber/decContext.o  ../decNumber/decNumber.o

###############################################################################

.PHONY: all
all:  ver.h libsir prt2pdf punutil mcmb empty locallibs_then_dps8 $(TAGS)
	-@$(TRUE)

###############################################################################

.PHONY: install
ifneq (,$(findstring install,$(MAKECMDGOALS)))
.NOTPARALLEL: install clean
endif
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean install
endif
install:
	@$(TEST) -f "dps8$(EXE)" || { $(PRINTF) '%s\n' "Error: No dps8 binary.";  \
                                  exit 1; }
	@$(SETV); $(MKDIR) "$(DESTDIR)$(INSTALL_BIN)"
	@$(SETV); $(CP) "dps8$(EXE)" "$(DESTDIR)$(INSTALL_BIN)/dps8$(EXE)"

###############################################################################

.PHONY: printuk
printuk:
	@$(PRINTF) '%s\n' "*** Begin printuk (git untracked files):"
	@$(SETV); ($(CD) "../.." && $(GIT) status -s --untracked-files="all"      \
        2> /dev/null | $(GREP) '^? ' || $(TRUE))
	@$(PRINTF) '%s\n' "*** End printuk"

###############################################################################

.PHONY: printmod
printmod:
	@$(PRINTF) '%s\n' "*** Begin printmod (git tracked modified files)"
	@$(SETV); ($(CD) "../.." && $(GIT) status -s 2> /dev/null |               \
        $(GREP) '^ \?M ' || $(TRUE))
	@$(PRINTF) '%s\n' "*** End printmod"

###############################################################################

.PHONY: all ctags
dps8$(EXE): $(OBJS) $(LOCALOBJS) $(DECN_OBJS) $(SIMH_OBJS) ctags
	@$(PRINTF) '%s\n' "LD: dps8$(EXE)"
ifeq ($(TESTING),1)
	@$(PRINTF) '%s\n' "LD: $(OBJS)"
endif
	@$(SETV); $(CC) -o dps8$(EXE) $(LDFLAGS) $(OBJS) $(LOCALOBJS)             \
           $(MATHLIB) $(DECN_OBJS) $(SIMH_OBJS) $(LIBSIR)                     \
            $(LIBS) $(LOCALLIBS) $(DUMALIBS)

###############################################################################

.PHONY: FORCE
ifeq (,$(wildcard ../../.git))
# not git
ifeq (,$(wildcard ver.h~))
# not git, not kit
ver.h:
	@$(SETV); $(RMF) "./ver.h"
	@$(PRINTF) '%s\n' "WARNING: This source tree does not appear to be a"     \
                      "         full git clone or a complete source kit."
	@$(SETV); $(TOUCH) "./ver.h"
else
# kit
ver.h:
	@$(SETV); $(CP) "./ver.h~" "./ver.h"
endif
else
# git
ver.h: FORCE
ifneq ($(USE_BUILDER),)
	@$(SETV); $(PRINTF) '%s\n' "$(USE_BUILDER)" > "../../.builder.txt"
endif
ifneq ($(USE_BUILDOS),)
	@$(SETV); $(PRINTF) '%s\n' "$(USE_BUILDOS)" > "../../.buildos.txt"
endif
ifeq ($(MAKE_TOPLEVEL),1)
	@$(RMF) "ver.h" > /dev/null 2>&1
endif
	@$(TEST) -f "ver.h" && $(PRINTF) '%s\n'                                   \
                     "WARNING: Git re-build: Skipping \"make_ver.sh\"."       \
                     " *** Use \"$(MAKE) clean\" or build from top-level!" || \
                         $(TRUE) > /dev/null 2>&1
	@$(SETV); $(TEST) -f "ver.h" || { $(TOUCH) "ver.h"; ./make_ver.sh ||      \
              { $(PRINTF) '%s\n' "WARNING: Unable to run \"make_ver.sh\"."    \
                                 "     *** Using default fallback data!";     \
                $(RMF) "ver.h"; $(TOUCH) "ver.h"; }; }
endif

###############################################################################

slte.inc: system_book_12_3.ascii
	@$(SETV); ./buildSLTE.sh

###############################################################################

.PHONY: libsir
ifeq ($(TESTING),1)
  M_SIR_TESTING="SIR_DEBUG=1"
endif
ifneq ($(V),1)
  M_SIR_QFLAG=-s
endif
libsir:
	@($(SETV); $(CD) "../libsir" &&                                           \
      $(PRINTF) '%s\n' "LIBSIR: libsir_s.a" &&                                \
      $(ENV) CC="$(CC)"                                                       \
             CFLAGS='$(CFLAGS)'                                               \
             LDFLAGS='$(LDFLAGS)'                                             \
      $(MAKE) $(M_SIR_QFLAG) -C "." "static"                                  \
        SIR_NO_PLUGINS=1 SIR_NO_SHARED=1                                      \
        SIR_USE_EOL_CRLF=1 FORTIFY_FLAGS= $(M_SIR_TESTING))

###############################################################################

.PHONY: locallibs_then_dps8
locallibs_then_dps8: ver.h errnos.h libsir
	@($(SETV); $(CD) "../decNumber" && $(MAKE) -C "." -f "GNUmakefile" "all")
	@($(SETV); $(CD) "$(SIMHx)" && $(MAKE) -C "." -f "GNUmakefile" "all")
	@$(SETV); $(MAKE) -C "." -f "GNUmakefile" "dps8$(EXE)"

###############################################################################

.PHONY: prt2pdf
prt2pdf:
	@$(SETV); $(MAKE) -C "../prt2pdf" "all"

###############################################################################

.PHONY: punutil
punutil:
	-@$(SETV); $(MAKE) -C "../punutil" "all"

###############################################################################

.PHONY: mcmb
mcmb:
	-@$(SETV); $(MAKE) -C "../mcmb" "all"

###############################################################################

.PHONY: empty
empty:
	-@$(SETV); $(MAKE) -C "../empty" "all"

###############################################################################

udplib$(EXE): udplib.c udplib.h
	@$(SETV); $(CC) udplib.c -o udplib -DTEST=1

###############################################################################

.PHONY: ctags
tags TAGS ctags: $(C_SRCS) $(H_SRCS) ver.h
	@$(PRINTF) '%s\n' "CTAGS: dps8"
	-@$(SETV); $(CTAGS) $(C_SRCS) $(H_SRCS)                                   \
        $(SIMHx)/*.[ch] ../decNumber/dec*.[ch]       2> /dev/null || $(TRUE)
	@$(PRINTF) '%s\n' "ETAGS: dps8"
	-@$(SETV); $(ETAGS) $(C_SRCS) $(H_SRCS)                                   \
        $(SIMHx)/*.[ch] ../decNumber/dec*.[ch]       2> /dev/null || $(TRUE)
	@$(PRINTF) '%s\n' "GTAGS: dps8"
	-@($(SETV); $(CD) ../.. && $(GIT) ls-files       2> /dev/null  |          \
       $(GREP) '\.[ch]'                              2> /dev/null  |          \
         $(GREP) -E '(dps8|decNumber|simh|libsir)'   2> /dev/null  |          \
         $(GREP) -vE '(blinkenLights|\.Attic/)'      2> /dev/null  |          \
         $(GREP) -vE '(libsir/example|libsir/tests)' 2> /dev/null  |          \
         $(GREP) -vE '(libsir/bindings|libsir/docs)' 2> /dev/null  |          \
         $(GREP) -vE '(libsir/sir\.code-workspace)'  2> /dev/null  |          \
         $(GREP) -vE '(libsir/utils|libsir/plugins)' 2> /dev/null  |          \
         $(GREP) -vE '(libsir/include/sir\.hh)'      2> /dev/null  |          \
         $(GREP) -vE '(libbacktrace/)'               2> /dev/null  |          \
       $(GTAGS) -f -                                 2> /dev/null) || $(TRUE)
	@$(PRINTF) '%s\n' "CSCOPE: dps8"
	-@$(SETV); $(CSCOPE) -bcq $(C_SRCS) $(H_SRCS)                             \
        $(SIMHx)/*.[ch] ../decNumber/dec*.[ch]     2> /dev/null   || $(TRUE)

###############################################################################

.PHONY: tapes
tapes: 88534.tap 88631.tap 88632.tap 88633.tap 88634.tap 88635.tap 88636.tap  \
       99020.tap 93085.tap 98570.tap 99019.tap 98577.tap 20184.tap 20185.tap  \
       20186.tap 20187.tap 20188.tap t4d_b.2.tap 98580_1er.tap 98576_1er.tap

%.tap:
	@$(SETV); $(WEBDL) \
       "https://bitsavers.trailing-edge.com/bits/Honeywell/multics/tape/$@.gz"
	@$(SETV); $(GUNZIPT) "$@.gz"

###############################################################################

.PHONY: superclean realclean reallyclean distclean clean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: superclean realclean reallyclean distclean clean
endif
superclean realclean reallyclean: distclean
	@$(SETV); $(CCACHE) -cC > /dev/null 2>&1  ||  $(TRUE)

###############################################################################

.PHONY: distclean clean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: distclean clean
endif
distclean: clean
	@$(SETV); $(MAKE) -C "../libsir"       "distclean"              || $(TRUE)
	@$(SETV); $(MAKE) -C "../libbacktrace" "distclean" 2> /dev/null || $(TRUE)
	@$(RMF) *.gc??                           2> /dev/null  ||  $(TRUE)
	@$(RMF) ../blinkenLights2/*.gc??         2> /dev/null  ||  $(TRUE)
	@$(RMF) ../decNumber/*.gc??              2> /dev/null  ||  $(TRUE)
	@$(RMF) ../mcmb/*.gc??                   2> /dev/null  ||  $(TRUE)
	@$(RMF) ../prt2pdf/*.gc??                2> /dev/null  ||  $(TRUE)
	@$(RMF) ../punutil/*.gc??                2> /dev/null  ||  $(TRUE)
	@$(RMF) ../simh/*.gc??                   2> /dev/null  ||  $(TRUE)
	@$(RMF) ../empty/*.gc??                  2> /dev/null  ||  $(TRUE)
	@$(RMF) *.prof*                          2> /dev/null  ||  $(TRUE)
	@$(RMF) ../blinkenLights2/*.prof*        2> /dev/null  ||  $(TRUE)
	@$(RMF) ../decNumber/*.prof*             2> /dev/null  ||  $(TRUE)
	@$(RMF) ../mcmb/*.prof*                  2> /dev/null  ||  $(TRUE)
	@$(RMF) ../prt2pdf/*.prof*               2> /dev/null  ||  $(TRUE)
	@$(RMF) ../simh/*.prof*                  2> /dev/null  ||  $(TRUE)
	@$(RMF) ../empty/*.prof*                 2> /dev/null  ||  $(TRUE)
	@$(RMF) ../punutil/*.prof*               2> /dev/null  ||  $(TRUE)
	@$(RMF) ../../dps8m*.state               2> /dev/null  ||  $(TRUE)
	@$(RMF) ../dps8m*.state                  2> /dev/null  ||  $(TRUE)
	@$(RMF) dps8m*.state                     2> /dev/null  ||  $(TRUE)
	@$(RMF) panelScraper.o                   2> /dev/null  ||  $(TRUE)
	@$(RMF) "../../dps8m.state"                            ||  $(TRUE)
	@$(RMF) "../dps8m.state"                               ||  $(TRUE)
	@$(RMF) "dps8m.state"                                  ||  $(TRUE)
	@$(RMF) "../../l68.state"                              ||  $(TRUE)
	@$(RMF) "../l68.state"                                 ||  $(TRUE)
	@$(RMF) "l68.state"                                    ||  $(TRUE)
	@$(RMF) ../../l68*.state                 2> /dev/null  ||  $(TRUE)
	@$(RMF) ../l68*.state                    2> /dev/null  ||  $(TRUE)
	@$(RMF) l68*.state                       2> /dev/null  ||  $(TRUE)
	@$(RMF) "../../.dps8m.state"                           ||  $(TRUE)
	@$(RMF) "../.dps8m.state"                              ||  $(TRUE)
	@$(RMF) ".dps8m.state"                                 ||  $(TRUE)
	@$(RMF) ../../.dps8m*.state             2> /dev/null   ||  $(TRUE)
	@$(RMF) ../.dps8m*.state                2> /dev/null   ||  $(TRUE)
	@$(RMF) .dps8m*.state                   2> /dev/null   ||  $(TRUE)
	@$(RMF) "../../.l68.state"                             ||  $(TRUE)
	@$(RMF) "../.l68.state"                                ||  $(TRUE)
	@$(RMF) ".l68.state"                                   ||  $(TRUE)
	@$(RMF) ../../.l68*.state               2> /dev/null   ||  $(TRUE)
	@$(RMF) ../.l68*.state                  2> /dev/null   ||  $(TRUE)
	@$(RMF) .l68*.state                     2> /dev/null   ||  $(TRUE)
	@$(SETV); $(RMF) "restoreMultics.log"                  ||  $(TRUE)
	@$(SETV); $(RMF) "TAGS"                                ||  $(TRUE)
	@$(SETV); $(RMF) "tags"                                ||  $(TRUE)
	@$(SETV); $(RMF) "../../GPATH"                         ||  $(TRUE)
	@$(SETV); $(RMF) "../../GRTAGS"                        ||  $(TRUE)
	@$(SETV); $(RMF) "../../GTAGS"                         ||  $(TRUE)
	@$(SETV); $(RMF) "cscope.in.out"                       ||  $(TRUE)
	@$(SETV); $(RMF) "cscope.out"                          ||  $(TRUE)
	@$(SETV); $(RMF) "cscope.po.out"                       ||  $(TRUE)
	@$(SETV); $(TEST) -d "../../libuv-local"               &&                 \
              $(RMF) -r  "../../libuv-local"               ||  $(TRUE)
	@$(SETV); $(TEST) -d "../../.git"                      &&                 \
              $(RMF) "ver.h~"                              ||  $(TRUE)

###############################################################################

.PHONY: distclean clean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: distclean clean
endif
clean:
	@$(SETV); $(MAKE) -C "$(SIMHx)"          -f "GNUmakefile"  "clean"
	@$(SETV); $(MAKE) -C "../decNumber"      -f "GNUmakefile"  "clean"
	@$(SETV); $(MAKE) -C "../prt2pdf"        -f "GNUmakefile"  "clean"
	@$(SETV); $(MAKE) -C "../punutil"        -f "GNUmakefile"  "clean"
	@$(SETV); $(MAKE) -C "../mcmb"           -f "GNUmakefile"  "clean"
	@$(SETV); $(MAKE) -C "../empty"          -f "GNUmakefile"  "clean"
	@$(SETV); ( cd ../blinkenLights2 && $(RMF) blinkenLights2 || $(TRUE) )
	@$(SETV); $(MAKE) -C "."                 -f "GNUmakefile"  "clean_dps8"
	@$(SETV); $(MAKE) -C "../libsir"       "clean"               ||  $(TRUE)
	@$(SETV); $(MAKE) -C "../libbacktrace" "clean" 2> /dev/null  ||  $(TRUE)
	@$(SETV); $(RMF)  ../../"$(KITNAME)"."$(TARXT)"    ||  $(TRUE)
	@$(SETV); $(RMF)  ../../"$(KITNAME)"."$(TARXT)".*  ||  $(TRUE)
	@$(SETV); $(RMF)  ../../"$(KITNAME)"."TMP"         ||  $(TRUE)
	@$(SETV); $(RMF)  ../../"$(KITNAME)"."zip"         ||  $(TRUE)
	@$(SETV); $(TEST) -d "../../dps8"                  &&                     \
              $(RMF) -r "../../dps8"                   ||  $(TRUE)
	@$(SETV); $(RMF)  "../../.builder.txt"             ||  $(TRUE)
	@$(SETV); $(RMF)  "../../.buildos.txt"             ||  $(TRUE)
	@$(SETV); $(RMF)  "ver.h"                          ||  $(TRUE)
	@$(SETV); $(RMF)  ./ver.h.* > /dev/null 2>&1       ||  $(TRUE)
	@$(SETV); $(RMF)  "dump.i"                         ||  $(TRUE)
	@$(SETV); $(RMF)  "amd_memcpy.o"                   ||  $(TRUE)
	@$(SETV); $(RMF)  "../amd_memcpy.o"                ||  $(TRUE)
	@$(SETV); $(RMF)  "../../amd_memcopy.o"            ||  $(TRUE)
	@$(SETV); $(RMF)  "./null.i"                       ||  $(TRUE)
	@$(SETV); $(RMF) "dps8_absi.o"       2> /dev/null  ||  $(TRUE)
	@$(SETV); $(RMF) "dps8_mgp.o"        2> /dev/null  ||  $(TRUE)
	@$(SETV); $(RMF) "dps8_socket_dev.o" 2> /dev/null  ||  $(TRUE)
	@$(SETV); $(RMF) "udplib.o"          2> /dev/null  ||  $(TRUE)
	@$(SETV); $(RMF) ./*.ln > /dev/null 2>&1           ||  $(TRUE)
	@$(SETV); $(RMF) ./*.gcda > /dev/null 2>&1         ||  $(TRUE)

###############################################################################

.PHONY: clean_dps8
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean_dps8
endif
clean_dps8:
	@$(PRINTF) '%s\n' "CLEAN: dps8"
	@$(SETV); $(RMF) $(OBJS) $(C_DEPS) $(EXECUTABLES) "dps8$(EXE)"            \
      "tags" "TAGS" $(C_SRCS:.c=.d) $(wildcard $(C_SRCS:.c=.d.[0-9]*))        \
        "dps8.exe" "errnos.h" "udplib.exe" "udplib" "tracker.o" "dump.[icd]"  \
          2> /dev/null  ||  $(TRUE)

###############################################################################

.PHONY: kit dist
kit dist:
	@$(PRINTF) '%s\n' "KIT: dps8"
	@$(PRINTF) '%s\n' "ARCHIVE: $(KITNAME).$(TARXT)"
	@$(SETV); $(RMF) ../../"$(KITNAME)"."$(TARXT)"                  || $(TRUE)
	@$(SETV); $(RMF) ../../"$(KITNAME)"."$(TARXT)"."$(COMPRESSXT)"  || $(TRUE)
	@$(SETV); MAKE="$(MAKE)" $(MAKE) "ver.h"
	@$(SETV); $(CP) "./ver.h" "./ver.h~"
	@$(SETV); ($(CD) "../.." &&                                               \
        $(MAKETAR) "$(KITNAME).$(TARXT)" $$( $(GIT) ls-files               |  \
         $(GREP) -v '^.git' | $(GREP) -v '^.whitesource'                   |  \
          $(GREP) -v '^.lgtm.yml' | $(GREP) -v '/.Attic/'                  |  \
           $(GREP) -v '^src/dps8/make_ver.sh' | $(GREP) -v '^docs'         |  \
            $(GREP) -v '^src/Makefile.scc' | $(GREP) -v 'ci-kit'           |  \
             $(GREP) -v '^src/dps8/listings/' | $(GREP) -v '.reuse/dep5'   |  \
              $(GREP) -v '^.scspell' | $(GREP) -v '^LICENSES/'             |  \
               $(GREP) -v '^.*\.license$$' | $(GREP) -v 'src/Makefile.doc' |  \
                $(GREP) -v '^src/perf_test/' | $(GREP) -v '^.nsis/'        |  \
                 $(GREP) -v '^.hpkg/' | $(GREP) -v '^.editorconfig'        |  \
                  $(GREP) -v '^src/Makefile.dev' | $(GREP) -v '^.gtags' )     \
                              'src/dps8/ver.h~' )
	@$(PRINTF) '%s\n'                                                         \
       "COMPRESS: $(KITNAME).$(TARXT) -> $(KITNAME).$(TARXT).$(COMPRESSXT)"
	@$(SETV); $(COMPRESS) "../../$(KITNAME).$(TARXT)"

###############################################################################
# Makes a ZIP format kit

.PHONY: zipkit zipdist
zipkit zipdist:
	@$(PRINTF) '%s\n' "ZIPKIT: dps8"
	@$(PRINTF) '%s\n' "ARCHIVE: $(KITNAME).zip"
	@$(SETV); $(RMF) ../../"$(KITNAME)"."zip"                       || $(TRUE)
	@$(SETV); $(RMF) ../../"$(KITNAME)"."TMP"                       || $(TRUE)
	@$(SETV); MAKE="$(MAKE)" $(MAKE) "ver.h"
	@$(SETV); $(CP) "./ver.h" "./ver.h~"
	@$(SETV); ($(CD) "../.." &&                                               \
        $(MAKETAR) "$(KITNAME).TMP" $$( $(GIT) ls-files                    |  \
         $(GREP) -v '^.git' | $(GREP) -v '^.whitesource'                   |  \
          $(GREP) -v '^.lgtm.yml' | $(GREP) -v '/.Attic/'                  |  \
           $(GREP) -v '^src/dps8/make_ver.sh' | $(GREP) -v '^docs'         |  \
            $(GREP) -v '^src/Makefile.scc' | $(GREP) -v 'ci-kit'           |  \
             $(GREP) -v '^.scspell' | $(GREP) -v '^LICENSES/'              |  \
              $(GREP) -v '^.*\.license$$' | $(GREP) -v 'src/Makefile.doc'  |  \
               $(GREP) -v '^src/dps8/listings/'                            |  \
                $(GREP) -v '^src/perf_test/' | $(GREP) -v '^.nsis/'        |  \
                 $(GREP) -v '^.hpkg/' | $(GREP) -v '^.editorconfig'        |  \
                  $(GREP) -v '.reuse/dep5' | $(GREP) -v '^.gtags' )           \
                             'src/dps8/ver.h~' )
	@$(SETV); ($(CD) "../.." && $(TAR) xf "$(KITNAME)"."TMP"              &&  \
      $(RMF) "$(KITNAME)"."zip"                                           &&  \
        $(TR) -s ' ' < "./src/dps8/ver.h~" 2> /dev/null                    |  \
          $(GREP) -E '(VER_H_GIT_VERSION|VER_H_GIT_HASH)' 2> /dev/null     |  \
            $(CUT) -d ' ' -f 3- 2> /dev/null | $(TR) -d '"' 2> /dev/null   |  \
              $(PASTE) - - 2> /dev/null                                    |  \
                $(SED) -e 's/^/\r\nDPS\/8M Simulator /' -e 's/\t/\r\n(/'      \
                           -e 's/$$/)\r\n\r\n.\n/'                         |  \
                  $(INFOZIP) -q -r -o -z -nw -9 "$(KITNAME)"."zip" "./dps8"   \
                    2> /dev/null && $(INFOZIP) -q -T "$(KITNAME)"."zip"       \
                      2> /dev/null &&  $(RMF) "$(KITNAME)"."TMP"              \
                        2> /dev/null && $(RMF) -r "./dps8" 2> /dev/null   &&  \
                          $(PRINTF) 'ZIPKIT: %s\n'                            \
                            "$(KITNAME).zip created successfully")
	@$(SETV); { $(COMMAND) -v $(ADVZIP) > /dev/null 2>&1                  &&  \
      $(PRINTF) '%s\n' "ADVZIP: $(KITNAME).zip"                           &&  \
        $(ADVZIP) -qz4k "../../sources.zip" || $(TRUE); } || $(TRUE)

###############################################################################

errnos.h:
	@$(PRINTF) '%s\n' '#include <errno.h>' > "./dump.c" 2> /dev/null
	@$(SETV); ($(CC) -E -dD ./dump.c 2> /dev/null;                            \
               $(CC) -qshowmacros -E ./dump.c 2> /dev/null) | $(SORT) -u |    \
        $(AWK) '/^#define E/ { printf "{\"%s\",%s},\n", $$2, $$2 } END        \
                               { print "" }' > "./errnos.h"
	@$(SETV); $(RMF) "./dump.c"

###############################################################################

ifdef LIBS
  export LIBS
  ifneq (,$(wildcard ../Makefile.mk))
    include ../Makefile.mk
  endif
endif

###############################################################################

ifneq (,$(wildcard ./Dependency))
  include ./Dependency
endif

###############################################################################

ifneq (,$(wildcard ../Makefile.dev))
.PHONY: dep depend
ifneq (,$(findstring dep,$(MAKECMDGOALS)))
.NOTPARALLEL: dep depend
endif
dep depend: ../Makefile.dev
	@( $(CD) "../.." && $(MAKE) -j 1 -s "depend" )

.PHONY: cleandep depclean cleandepend dependclean
ifneq (,$(findstring dep,$(MAKECMDGOALS)))
.NOTPARALLEL: cleandep depclean cleandepend dependclean
endif
cleandep depclean cleandepend dependclean:
	@( $(CD) "../.." && $(MAKE) -j 1 -s "depclean" )
endif

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
