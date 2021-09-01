# DPS/8M simulator: src/dps8/GNUmakefile
# vim: filetype=make:tabstop=4:tw=76
#
###############################################################################
#
# Copyright (c) 2013-2016 Harry Reed
# Copyright (c) 2013-2016 Charles Anthony
# Copyright (c) 2016 Jean-Michel Merliot
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

ifndef LIBUV
  LIBUV = -luv
endif
LIBS += $(LIBUV)

###############################################################################

ifeq ($(CROSS),MINGW64)
  CFLAGS += -DCROSS_MINGW64 -I../../../libuv/include
  ifneq ($(W),)
    CFLAGS += -Wno-misleading-indentation
  endif
  ifeq ($(shell $(UNAME) -o 2> /dev/null || true >/dev/null 2>&1),Cygwin)
    LIBS += -lwinmm -lws2_32 -lwsock32 -lws2_32 -luserenv -liphlpapi $(LIBUV)
  else
    LIBS += -lwinmm -lws2_32 -lmingwex -lwsock32 -lws2_32 -luserenv           \
            -liphlpapi $(LIBUV) -L../../../libuv/.libs
  endif
endif

###############################################################################

CFLAGS += -I../decNumber -I$(SIMHx)

###############################################################################
# Fix for gcc reporting bogus warnings

ifneq ($(W),)
ifeq ($(CC),clang)
CFLAGS += -Wall -Wconversion -Wsign-conversion
else
CFLAGS += -Wall -Wno-conversion -Wsign-conversion -Wno-psabi -Wno-abi
endif
endif

###############################################################################

ifneq ($(M32),)
  CFLAGS += -DNEED_128
  ifneq ($(W),)
    CFLAGS += -Wno-format-zero-length -Wno-sign-conversion
  endif
endif

###############################################################################
# The system_state code relies on M_SHARED.

ifneq ($(CROSS),MINGW64)
  CFLAGS += -DM_SHARED
  ifneq ($(OS),OSX)
    LIBS += -lrt
  endif
endif

###############################################################################
# What needs -ldl?

OSTYPE = $(shell $(UNAME) -s)
ifeq ($(OSTYPE),FreeBSD)
  LIBS += -lm -pthread
endif
ifeq ($(OSTYPE),Linux)
  ifeq ($(CROSS),MINGW64)
    LIBS += -lm
  else
    LIBS += -lm -ldl
  endif
endif

###############################################################################
# Need to link math library with explicit order?

ifeq ($(OSTYPE),GNU)
  LIBS += -lm
endif

###############################################################################

ifneq ($(CROSS),MINGW64)
  C_SRCS  = dps8_absi.c
endif
C_SRCS += bsd_random.c
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
C_SRCS += dps8_mt.c
C_SRCS += dps8_opcodetable.c
C_SRCS += dps8_prt.c
C_SRCS += dps8_scu.c
C_SRCS += dps8_simh.c
ifneq ($(CROSS),MINGW64)
  C_SRCS += dps8_socket_dev.c
endif
ifeq ($(TRACKER),1)
  C_SRCS += tracker.c
endif
C_SRCS += dps8_sys.c
C_SRCS += dps8_urp.c
C_SRCS += dps8_utils.c
C_SRCS += fnpuv.c
C_SRCS += fnptelnet.c
C_SRCS += libtelnet.c
C_SRCS += hdbg.c
C_SRCS += segldr.c
ifneq ($(CROSS),MINGW64)
  C_SRCS += shm.c
endif
ifneq ($(CROSS),MINGW64)
  C_SRCS += udplib.c
endif
ifeq ($(THREADZ),1)
  C_SRCS += threadz.c
endif
ifeq ($(LOCKLESS),1)
  C_SRCS += threadz.c
endif
C_SRCS += utfile.c
C_SRCS += uvutil.c

###############################################################################

OBJS  := $(patsubst %.c,%.o,$(C_SRCS))

###############################################################################

ifneq ($(CROSS),MINGW64)
  H_SRCS  = dps8_absi.h
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
H_SRCS += dps8_prom.h
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
H_SRCS += hdbg.h
H_SRCS += segldr.h
H_SRCS += shm.h
ifneq ($(CROSS),MINGW64)
  H_SRCS += udplib.h
endif
H_SRCS += utarray.h
H_SRCS += uthash.h
H_SRCS += utfile.h
H_SRCS += utlist.h
ifeq ($(THREADZ),1)
  H_SRCS += threadz.h
endif
ifeq ($(LOCKLESS),1)
  H_SRCS += threadz.h
endif
ifeq ($(TRACKER),1)
  H_SRCS += tracker.h
endif
H_SRCS += uvutil.h

###############################################################################

SIMH_OBJS = ../simh/scp.o        ../simh/sim_console.o  ../simh/sim_disk.o    \
            ../simh/sim_ether.o  ../simh/sim_fio.o      ../simh/sim_serial.o  \
            ../simh/sim_sock.o   ../simh/sim_tape.o     ../simh/sim_timer.o   \
            ../simh/sim_tmxr.o   ../simh/sim_video.o

###############################################################################

DECN_OBJS = ../decNumber/decContext.o  ../decNumber/decNumber.o

###############################################################################

.PHONY: all
all: sysdefs.h unifdef prt2pdf punutil $(TAGS) locallibs_then_dps8

###############################################################################

.PHONY: install
.NOTPARALLEL: install
install: all
	@$(SETV); $(MKDIR) "$(DESTDIR)$(INSTALL_BIN)"
	@$(SETV); $(CP) "dps8$(EXE)" "$(DESTDIR)$(INSTALL_BIN)/dps8$(EXE)"

###############################################################################

.PHONY: cscope
.NOTPARALLEL: cscope
cscope:
	@$(PRINTF) '%s\n' "CSCOPE: dps8"
	@$(SETV); $(CSCOPE) $(C_SRCS)

###############################################################################

.PHONY: printuk
.NOTPARALLEL: printuk
printuk:
	@$(PRINTF) '%s\n' "*** Begin printuk (git untracked files):"
	@$(SETV); ($(CD) "../.." && $(GIT) status -s --untracked-files="all"      \
	    2> /dev/null | $(GREP) '^? ' || $(TRUE))
	@$(PRINTF) '%s\n' "*** End printuk"

###############################################################################

.PHONY: printmod
.NOTPARALLEL: printmod
printmod:
	@$(PRINTF) '%s\n' "*** Begin printmod (git tracked modified files)"
	@$(SETV); ($(CD) "../.." && $(GIT) status -s 2> /dev/null |               \
	    $(GREP) '^\ \?M ' || $(TRUE))
	@$(PRINTF) '%s\n' "*** End printmod"

###############################################################################

.NOTPARALLEL: blinkenLights
blinkenLights: blinkenLights.c shm.o
	@$(PRINTF) '%s\n' "LD: blinkenLights"
	@$(SETV); $(CC) $(CFLAGS) $(LDFLAGS) blinkenLights.c                      \
        -o blinkenLights shm.o `$(PKGCONFIG) --cflags --libs gtk+-3.0`

###############################################################################

.NOTPARALLEL: blinkenLights2
blinkenLights2: blinkenLights2.c shm.o
	@$(PRINTF) '%s\n' "LD: blinkenLights2s"
	@$(SETV); $(CC) $(CFLAGS) $(LDFLAGS) blinkenLights2.c                     \
        -o blinkenLights2 shm.o `$(PKGCONFIG) --cflags --libs gtk+-3.0` -lrt

###############################################################################

.NOTPARALLEL: blinkenLightsCurses
blinkenLightsCurses: blinkenLightsCurses.c shm.o
	@$(PRINTF) '%s\n' "LD: blinkenLightsCurses"
	@$(SETV); $(CC) $(CFLAGS) $(LDFLAGS) blinkenLightsCurses.c                \
        -o blinkenLightsCurses shm.o -lncurses

###############################################################################

.PHONY: all ctags
dps8$(EXE): $(OBJS) $(LOCALOBJS) $(LOCALLIBS) $(DECN_OBJS) $(SIMH_OBJS) ctags
	@$(PRINTF) '%s\n' "LD: dps8$(EXE)"
	@$(SETV); $(CC) -o dps8$(EXE) $(LDFLAGS) $(OBJS) $(LOCALOBJS)             \
        $(DECN_OBJS) $(SIMH_OBJS) $(LOCALLIBS) $(LIBS)

###############################################################################

.PHONY: FORCE
.NOTPARALLEL: ver.h
ifeq (,$(wildcard ../../.git))
# not git
ifeq (,$(wildcard ver.h~))
# not git, not kit
ver.h:
	@$(SETV); $(RMF) "./ver.h"
	@$(PRINTF) '%s\n' "WARNING: This source tree does not appear to be a"    \
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
	@$(SETV); $(TEST) -f "ver.h" || { ./make_ver.sh ||                        \
        $(SHELL) ./make_ver.sh ||                                             \
            { $(PRINTF) '%s\n' "WARNING: Unable to run \"make_ver.sh\"."      \
                               "     *** Using default fallback data!";       \
              $(TOUCH) "ver.h"; }; }
endif

###############################################################################

.NOTPARALLEL: slte.inc
slte.inc: system_book_12_3.ascii
	@$(SETV); ./buildSLTE.sh

###############################################################################

.PHONY: locallibs_then_dps8
locallibs_then_dps8:
	@($(SETV); $(CD) "../decNumber" && $(MAKE) -C "." -f "GNUmakefile" "all")
	@$(SETV); MAKE="$(MAKE)" $(MAKE) -C "." -f "GNUmakefile" "ver.h"
	@($(SETV); $(CD) "$(SIMHx)" && $(MAKE) -C "." -f "GNUmakefile" "all")
	@$(SETV); $(MAKE) -C "." -f "GNUmakefile" "dps8$(EXE)"

###############################################################################

prt2pdf:
	@$(SETV); $(MAKE) -C "../prt2pdf" "all"

###############################################################################

unifdef:
	-@$(SETV); $(MAKE) -C "../unifdef"

###############################################################################

punutil:
	-@$(SETV); $(MAKE) -C "../punutil"

###############################################################################

udplib: udplib.c udplib.h
udplib$(EXE): udplib.c udplib.h
	@$(SETV); $(CC) udplib.c -o udplib -DTEST=1

###############################################################################

.PHONY: ctags
.NOTPARALLEL: ctags tags TAGS ver.h
tags TAGS ctags: $(C_SRCS) $(H_SRCS) ver.h
	@$(PRINTF) '%s\n' "CTAGS: dps8"
	-@$(SETV); $(CTAGS) $(C_SRCS) $(H_SRCS)                                   \
        $(SIMHx)/*.[ch] ../decNumber/dec*.[ch] 2> /dev/null || $(TRUE)
	@$(PRINTF) '%s\n' "ETAGS: dps8"
	-@$(SETV); $(ETAGS) $(C_SRCS) $(H_SRCS)                                   \
        $(SIMHx)/*.[ch] ../decNumber/dec*.[ch] 2> /dev/null || $(TRUE)

###############################################################################

.PHONY: tapes
.NOTPARALLEL: tapes
tapes: 88534.tap 88631.tap 88632.tap 88633.tap 88634.tap 88635.tap 88636.tap  \
       99020.tap 93085.tap 98570.tap 99019.tap 98577.tap 20184.tap 20185.tap  \
       20186.tap 20187.tap 20188.tap t4d_b.2.tap 98580_1er.tap 98576_1er.tap

%.tap:
	@$(SETV); $(WEBDL) \
       "https://bitsavers.trailing-edge.com/bits/Honeywell/multics/tape/$@.gz"
	@$(SETV); $(GUNZIP) "$@.gz"

###############################################################################

.PHONY: superclean realclean reallyclean distclean clean
.NOTPARALLEL: superclean realclean reallyclean distclean clean
superclean realclean reallyclean: distclean
	@$(SETV); $(CCACHE) -cC > /dev/null 2>&1  ||  $(TRUE)

###############################################################################

.PHONY: distclean clean
.NOTPARALLEL: distclean clean
distclean: clean
	@$(SETV); $(RMF) "../../dps8m.state"      ||  $(TRUE)
	@$(SETV); $(RMF) "../dps8m.state"         ||  $(TRUE)
	@$(SETV); $(RMF) "dps8m.state"            ||  $(TRUE)
	@$(SETV); $(RMF) "restoreMultics.log"     ||  $(TRUE)

###############################################################################

.PHONY: distclean clean
.NOTPARALLEL: distclean clean
clean:
	@$(SETV); $(MAKE) -C "$(SIMHx)"      -f "GNUmakefile"  "clean"
	@$(SETV); $(MAKE) -C "../decNumber"  -f "GNUmakefile"  "clean"
	@$(SETV); $(MAKE) -C "../prt2pdf"    -f "GNUmakefile"  "clean"
	@$(SETV); $(MAKE) -C "../punutil"    -f "GNUmakefile"  "clean"
	@$(SETV); $(MAKE) -C "../unifdef"    -f "GNUmakefile"  "clean"
	@$(SETV); $(MAKE) "clean_dps8"
	@$(SETV); $(RMF)  ../../"$(KITNAME)"."$(TARXT)"    ||  $(TRUE)
	@$(SETV); $(RMF)  ../../"$(KITNAME)"."$(TARXT)".*  ||  $(TRUE)
	@$(SETV); $(RMF)  "../../.builder.txt"             ||  $(TRUE)
	@$(SETV); $(RMF)  "../../.buildos.txt"             ||  $(TRUE)
	@$(SETV); $(RMF)  "ver.h"                          ||  $(TRUE)
	@$(SETV); $(RMF)  "compdef.out"                    ||  $(TRUE)
	@$(SETV); $(RMF)  "procdef.out"                    ||  $(TRUE)
	@$(SETV); $(RMF)  "procsrt.out"                    ||  $(TRUE)
	@$(SETV); $(RMF)  "unifdef.out"                    ||  $(TRUE)
	@$(SETV); $(RMF)  "sysdefs.h"                      ||  $(TRUE)
	@$(SETV); $(RMF)  "useddef.txt"                    ||  $(TRUE)

###############################################################################

.PHONY: clean_dps8
.NOTPARALLEL: clean_dps8
clean_dps8:
	@$(PRINTF) '%s\n' "CLEAN: dps8"
	@$(SETV); $(RMF) $(OBJS) $(C_DEPS) $(EXECUTABLES) "dps8$(EXE)"            \
        "tags" "TAGS" $(C_SRCS:.c=.d) $(wildcard $(C_SRCS:.c=.d.[0-9]*))      \
            "dps8.exe" "errnos.h" "udplib"  ||  $(TRUE)

###############################################################################

.PHONY: kit dist
.NOTPARALLEL: kit dist
kit dist:
	@$(PRINTF) '%s\n' "KIT: dps8"
	@$(PRINTF) '%s\n' "ARCHIVE: $(KITNAME).$(TARXT)"
	@$(SETV); $(RMF) ../../"$(KITNAME)"."$(TARXT)"                  ||  $(TRUE)
	@$(SETV); $(RMF) ../../"$(KITNAME)"."$(TARXT)"."$(COMPRESSXT)"  ||  $(TRUE)
	@$(SETV); MAKE="$(MAKE)" $(MAKE) "ver.h"
	@$(SETV); $(CP) "./ver.h" "./ver.h~"
	@$(SETV); ($(CD) "../.." &&                                               \
		$(MAKETAR) "$(KITNAME).$(TARXT)" $$( $(GIT) ls-files               |  \
         $(GREP) -v '^.git' | $(GREP) -v '^.whitesource'                   |  \
          $(GREP) -v '^.lgtm.yml' | $(GREP) -v '/.Attic/'                  |  \
           $(GREP) -v '^src/dps8/make_ver.sh'                              |  \
            $(GREP) -v '^src/Makefile.scc' | $(GREP) -v 'ci-kit'           |  \
             $(GREP) -v '^src/dps8/listings/' )                               \
                              'src/dps8/ver.h~' )
	@$(PRINTF) '%s\n'                                                         \
		"COMPRESS: $(KITNAME).$(TARXT) -> $(KITNAME).$(TARXT).$(COMPRESSXT)"
	@$(SETV); $(COMPRESS) "../../$(KITNAME).$(TARXT)"

###############################################################################

.NOTPARALLEL: unifdef.out
unifdef.out: unifdef
	@$(PRINTF) '%s\n' "UNIFDEF: *.[ch]"
	@($(SETV); $(CD) ".." 2> /dev/null && $(FIND) $$($(GIT) ls-files          \
      2> /dev/null 2> /dev/null | $(GREP) ".*\.[ch]" 2> /dev/null )           \
        -exec "./unifdef/unifdef" "-s" "{}" 2> /dev/null \; |                 \
          $(SORT) -u 2> /dev/null) > "./unifdef.out" ||                       \
            $(TOUCH) "./unifdef.out" > /dev/null 2>&1

###############################################################################

.NOTPARALLEL: procdef.out
procdef.out:
	@$(PRINTF) '%s\n' "PROCDEF: $(CPPCPP)"
	@($(SETV); ($(CPPCPP) -dM - < /dev/null 2> /dev/null ;                    \
      $(CPPCPP) -dD - < /dev/null 2> /dev/null) | $(GREP) "^#define"          \
        2> /dev/null | $(GREP) -v "^$$" 2> /dev/null | $(SORT) -u             \
          2> /dev/null) | $(SED) 's/^#define //g' |                           \
            $(SORT) > "./procdef.out" ||                                      \
              $(TOUCH) "./procdef.out" > /dev/null 2>&1

###############################################################################

.NOTPARALLEL: procsrt.out
procsrt.out: procdef.out
	@$(PRINTF) '%s\n' "PROCSRT: procdef.out"
	@$(SETV); $(CUT) -d ' ' -f 1 < "./procdef.out" 2> /dev/null |             \
      $(SORT) > "./procsrt.out" 2> /dev/null ||                               \
        $(TOUCH) "./procsrt.out" > /dev/null 2>&1

###############################################################################

.NOTPARALLEL: compdef.out
compdef.out: procsrt.out unifdef.out
	@$(PRINTF) '%s\n' "COMM: procsrt.out unifdef.out"
	@$(SETV); $(COMM) -12 "procsrt.out" "unifdef.out" > "./compdef.out"       \
      2> /dev/null || $(TOUCH) "./compdef.out" 2> /dev/null

###############################################################################

.NOTPARALLEL: useddef.txt
useddef.txt: compdef.out
	@$(PRINTF) '%s\n' "USEDDEF: compdef.out"
	@$(SETV); $(SH) -c 'while read -r line; do $(GREP) -i "$${line:-}"        \
      "./procdef.out" 2> /dev/null; done < "compdef.out" 2> /dev/null |       \
        $(SORT) -u | $(GREP) -v -e "^__VERSION__"  -e "^__clang_version__"    \
          -e "^__GCC_HAVE_SYNC_COMPARE_AND_SWAP_." -e "^__ORDER_.*_ENDIAN__"  \
            2> /dev/null > "useddef.txt"' || $(TRUE) > /dev/null 2>&1;        \
              $(TOUCH) "./useddef.txt" > /dev/null 2>&1

###############################################################################

.NOTPARALLEL: errnos.h
errnos.h:
	@$(PRINTF) '%s\n'                                                         \
        '#include <errno.h>' > "./dump.c"
	@$(SETV); $(CC) -E -dD ./dump.c |                                         \
        $(AWK) '/^#define E/ { printf "{\"%s\",%s},\n", $$2, $$2 }'           \
            > "./errnos.h"
	@$(SETV); $(RMF) "./dump.c"

###############################################################################

.NOTPARALLEL: sysdefs.h
sysdefs.h: useddef.txt
	@$(PRINTF) '%s\n' "SYSDEFS: useddef.txt"
	@$(SETV); $(TR) -d '=' < "./useddef.txt" 2> /dev/null |                   \
      $(TR) ' ' '=' 2> /dev/null | $(SED) -e 's/^/-D/g' 2> /dev/null |        \
        $(TR) '\n' ' ' 2> /dev/null | $(SED) -e 's/ $$//'                     \
          -e 's/^-D$$//g' | $(SED) -e 's/^/#define\ SYSDEFS_USED\ \"/'        \
            -e 's/$$/\"/' 2> /dev/null > "./sysdefs.n" ||                     \
              $(TOUCH) "./sysdefs.n" > /dev/null 2>&1
	@$(SETV); $(CMP) "./sysdefs.n" "./sysdefs.h" 2> /dev/null ||              \
        $(CP) "./sysdefs.n" "./sysdefs.h" 2> /dev/null ||                     \
            $(TOUCH) "sysdefs.h"
	@$(SETV); $(RMF) "sysdefs.n"

###############################################################################

blinkenLights2.o:   dps8.h
blinkenLights2.o:   dps8_simh.h
blinkenLights2.o:   dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
blinkenLights2.o:   dps8_sys.h
blinkenLights2.o:   hdbg.h
blinkenLights2.o:   shm.h
dps8_absi.o:        dps8.h
dps8_absi.o:        dps8_simh.h
dps8_absi.o:        dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_absi.o:        dps8_iom.h
dps8_absi.o:        dps8_absi.h dps8_sys.h
dps8_absi.o:        dps8_scu.h
dps8_absi.o:        dps8_cable.h dps8_cpu.h hdbg.h dps8_utils.h udplib.h
dps8_addrmods.o:    dps8.h
dps8_addrmods.o:    dps8_simh.h
dps8_addrmods.o:    dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_addrmods.o:    dps8_addrmods.h dps8_sys.h
dps8_addrmods.o:    dps8_faults.h
dps8_addrmods.o:    dps8_scu.h dps8_iom.h dps8_cable.h dps8_cpu.h hdbg.h
dps8_addrmods.o:    dps8_append.h dps8_ins.h dps8_iefp.h dps8_opcodetable.h
dps8_addrmods.o:    dps8_utils.h
dps8_append.o:      dps8_simh.h
dps8_append.o:      dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_append.o:      dps8_sys.h
dps8_append.o:      dps8_scu.h
dps8_append.o:      dps8_iom.h dps8_cable.h dps8_cpu.h hdbg.h dps8_append.h
dps8_append.o:      dps8_addrmods.h dps8_utils.h
dps8_cable.o:       dps8.h
dps8_cable.o:       dps8_simh.h
dps8_cable.o:       dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_cable.o:       dps8_iom.h
dps8_cable.o:       dps8_mt.h dps8_socket_dev.h dps8_scu.h dps8_sys.h
dps8_cable.o:       dps8_faults.h
dps8_cable.o:       dps8_cable.h dps8_cpu.h hdbg.h dps8_state.h dps8_console.h
dps8_cable.o:       dps8_disk.h dps8_fnp2.h libtelnet.h dps8_urp.h
dps8_cable.o:       dps8_crdrdr.h
dps8_cable.o:       dps8_crdpun.h dps8_prt.h dps8_utils.h dps8_absi.h
dps8_console.o:     dps8.h
dps8_console.o:     dps8_simh.h
dps8_console.o:     dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_console.o:     dps8_iom.h
dps8_console.o:     dps8_console.h dps8_sys.h
dps8_console.o:     dps8_faults.h
dps8_console.o:     dps8_scu.h dps8_cable.h dps8_cpu.h hdbg.h dps8_mt.h
dps8_console.o:     dps8_disk.h dps8_utils.h libtelnet.h
dps8_cpu.o:         dps8.h
dps8_cpu.o:         dps8_simh.h
dps8_cpu.o:         dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_cpu.o:         dps8_addrmods.h
dps8_cpu.o:         dps8_sys.h
dps8_cpu.o:         uvutil.h dps8_faults.h dps8_scu.h dps8_iom.h dps8_cable.h
dps8_cpu.o:         dps8_cpu.h hdbg.h dps8_append.h dps8_ins.h dps8_state.h
dps8_cpu.o:         dps8_math.h dps8_iefp.h dps8_console.h dps8_fnp2.h
dps8_cpu.o:         libtelnet.h
dps8_cpu.o:         dps8_socket_dev.h dps8_crdrdr.h dps8_absi.h dps8_utils.h
dps8_cpu.o:         dps8_opcodetable.h
dps8_crdpun.o:      dps8.h
dps8_crdpun.o:      dps8_simh.h
dps8_crdpun.o:      dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_crdpun.o:      dps8_iom.h
dps8_crdpun.o:      dps8_crdpun.h dps8_sys.h
dps8_crdpun.o:      dps8_scu.h
dps8_crdpun.o:      dps8_cable.h dps8_cpu.h hdbg.h dps8_utils.h
dps8_crdpun.o:      utfile.h
dps8_crdrdr.o:      dps8.h
dps8_crdrdr.o:      dps8_simh.h
dps8_crdrdr.o:      dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_crdrdr.o:      dps8_iom.h
dps8_crdrdr.o:      dps8_crdrdr.h dps8_sys.h
dps8_crdrdr.o:      dps8_scu.h
dps8_crdrdr.o:      dps8_cable.h dps8_cpu.h hdbg.h dps8_utils.h
dps8_decimal.o:     dps8_simh.h
dps8_decimal.o:     dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_decimal.o:     dps8_sys.h
dps8_decimal.o:     dps8_faults.h
dps8_decimal.o:     dps8_scu.h dps8_iom.h dps8_cable.h dps8_cpu.h hdbg.h
dps8_decimal.o:     dps8_decimal.h dps8_eis.h dps8_utils.h
dps8_dia.o:         dps8_simh.h
dps8_dia.o:         dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_dia.o:         dps8_dia.h
dps8_dia.o:         dps8_sys.h
dps8_dia.o:         dps8_scu.h
dps8_dia.o:         dps8_cpu.h hdbg.h dps8_iom.h dps8_cable.h dps8_utils.h
dps8_dia.o:         udplib.h
dps8_disk.o:        dps8_simh.h
dps8_disk.o:        dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_disk.o:        dps8_iom.h
dps8_disk.o:        dps8_disk.h dps8_sys.h
dps8_disk.o:        dps8_scu.h
dps8_disk.o:        dps8_cable.h dps8_cpu.h hdbg.h dps8_utils.h
dps8_eis.o:         dps8_simh.h
dps8_eis.o:         dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_eis.o:         dps8_sys.h
dps8_eis.o:         dps8_scu.h
dps8_eis.o:         dps8_iom.h dps8_cable.h dps8_cpu.h hdbg.h dps8_iefp.h
dps8_eis.o:         dps8_decimal.h dps8_ins.h dps8_eis.h dps8_utils.h
dps8_faults.o:      dps8_simh.h
dps8_faults.o:      dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_faults.o:      dps8_sys.h
dps8_faults.o:      dps8_scu.h
dps8_faults.o:      dps8_iom.h dps8_cable.h dps8_cpu.h hdbg.h dps8_append.h
dps8_faults.o:      dps8_ins.h dps8_utils.h
dps8_fnp2.o:        dps8_simh.h
dps8_fnp2.o:        dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_fnp2.o:        dps8_sys.h
dps8_fnp2.o:        dps8_scu.h
dps8_fnp2.o:        dps8_iom.h dps8_cable.h dps8_cpu.h hdbg.h dps8_fnp2.h
dps8_fnp2.o:        libtelnet.h fnptelnet.h fnpuv.h dps8_utils.h utlist.h
dps8_fnp2_iomcmd.o: dps8.h
dps8_fnp2_iomcmd.o: dps8_simh.h dps8_math128.h dps8_hw_consts.h
dps8_fnp2_iomcmd.o: dps8_em_consts.h dps8_sys.h
dps8_fnp2_iomcmd.o: dps8_scu.h
dps8_fnp2_iomcmd.o: dps8_iom.h dps8_cable.h dps8_cpu.h hdbg.h dps8_fnp2.h
dps8_fnp2_iomcmd.o: libtelnet.h dps8_fnp2_iomcmd.h dps8_utils.h fnpuv.h
dps8_iefp.o:        dps8.h
dps8_iefp.o:        dps8_simh.h
dps8_iefp.o:        dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_iefp.o:        dps8_sys.h
dps8_iefp.o:        dps8_scu.h
dps8_iefp.o:        dps8_iom.h dps8_cable.h dps8_cpu.h hdbg.h dps8_append.h
dps8_iefp.o:        dps8_iefp.h dps8_addrmods.h dps8_utils.h
dps8_ins.o:         dps8_simh.h
dps8_ins.o:         dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_ins.o:         dps8_addrmods.h
dps8_ins.o:         dps8_sys.h
dps8_ins.o:         uvutil.h dps8_faults.h dps8_scu.h dps8_iom.h dps8_cable.h
dps8_ins.o:         dps8_cpu.h hdbg.h dps8_append.h dps8_eis.h dps8_ins.h
dps8_ins.o:         dps8_math.h
dps8_ins.o:         dps8_opcodetable.h dps8_decimal.h dps8_iefp.h dps8_utils.h
dps8_ins.o:         ver.h
dps8_ins.o:         dps8_prom.h
dps8_iom.o:         dps8.h
dps8_iom.o:         dps8_simh.h
dps8_iom.o:         dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_iom.o:         dps8_sys.h
dps8_iom.o:         uvutil.h dps8_faults.h dps8_scu.h dps8_iom.h dps8_cable.h
dps8_iom.o:         dps8_cpu.h hdbg.h dps8_console.h dps8_fnp2.h libtelnet.h
dps8_iom.o:         dps8_utils.h
dps8_math128.o:     dps8.h
dps8_math128.o:     dps8_simh.h
dps8_math128.o:     dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_math.o:        dps8_simh.h
dps8_math.o:        dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_math.o:        dps8_sys.h
dps8_math.o:        dps8_scu.h
dps8_math.o:        dps8_iom.h dps8_cable.h dps8_cpu.h hdbg.h dps8_ins.h
dps8_math.o:        dps8_math.h
dps8_math.o:        dps8_utils.h
dps8_mt.o:          dps8_simh.h
dps8_mt.o:          dps8_math128.h dps8_hw_consts.h dps8_em_consts.h dps8_mt.h
dps8_mt.o:          dps8_sys.h
dps8_mt.o:          dps8_scu.h
dps8_mt.o:          dps8_iom.h dps8_cable.h dps8_cpu.h hdbg.h dps8_utils.h
dps8_opcodetable.o: dps8.h
dps8_opcodetable.o: dps8_simh.h dps8_math128.h dps8_hw_consts.h
dps8_opcodetable.o: dps8_em_consts.h dps8_opcodetable.h
dps8_prt.o:         dps8.h
dps8_prt.o:         dps8_simh.h
dps8_prt.o:         dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_prt.o:         dps8_iom.h
dps8_prt.o:         dps8_prt.h dps8_sys.h
dps8_prt.o:         dps8_scu.h
dps8_prt.o:         dps8_cable.h dps8_cpu.h hdbg.h dps8_utils.h
dps8_prt.o:         utfile.h
dps8_scu.o:         dps8.h
dps8_scu.o:         dps8_simh.h
dps8_scu.o:         dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_scu.o:         dps8_sys.h
dps8_scu.o:         uvutil.h dps8_faults.h dps8_scu.h dps8_iom.h dps8_cable.h
dps8_scu.o:         dps8_cpu.h hdbg.h dps8_utils.h
dps8_simh.o:        dps8.h
dps8_simh.o:        dps8_simh.h
dps8_simh.o:        dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_simh.o:        dps8_sys.h
dps8_simh.o:        uvutil.h
dps8_socket_dev.o:  dps8_simh.h
dps8_socket_dev.o:  dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_socket_dev.o:  dps8_socket_dev.h dps8_sys.h
dps8_socket_dev.o:  dps8_faults.h
dps8_socket_dev.o:  dps8_iom.h dps8_cable.h dps8_cpu.h hdbg.h dps8_utils.h
dps8_socket_dev.o:  errnos.h
dps8_sys.o:         dps8_simh.h
dps8_sys.o:         dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_sys.o:         dps8_console.h
dps8_sys.o:         dps8_sys.h
dps8_sys.o:         dps8_scu.h
dps8_sys.o:         dps8_iom.h dps8_cable.h dps8_cpu.h hdbg.h dps8_state.h
dps8_sys.o:         dps8_ins.h
dps8_sys.o:         dps8_math.h dps8_mt.h dps8_socket_dev.h dps8_disk.h
dps8_sys.o:         dps8_append.h
dps8_sys.o:         dps8_fnp2.h libtelnet.h dps8_crdrdr.h dps8_crdpun.h
dps8_sys.o:         dps8_prt.h
dps8_sys.o:         dps8_urp.h dps8_absi.h dps8_utils.h shm.h utlist.h
dps8_sys.o:         ver.h
dps8_urp.o:         dps8.h
dps8_urp.o:         dps8_simh.h
dps8_urp.o:         dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_urp.o:         dps8_iom.h
dps8_urp.o:         dps8_urp.h dps8_sys.h
dps8_urp.o:         dps8_scu.h
dps8_urp.o:         dps8_cable.h dps8_cpu.h hdbg.h dps8_utils.h
dps8_utils.o:       dps8_simh.h
dps8_utils.o:       dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
dps8_utils.o:       dps8_sys.h
dps8_utils.o:       dps8_scu.h
dps8_utils.o:       dps8_iom.h dps8_cable.h dps8_cpu.h hdbg.h dps8_ins.h
dps8_utils.o:       dps8_opcodetable.h dps8_utils.h
fnptelnet.o:        dps8_simh.h
fnptelnet.o:        dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
fnptelnet.o:        dps8_sys.h
fnptelnet.o:        dps8_scu.h
fnptelnet.o:        dps8_iom.h dps8_cable.h dps8_cpu.h hdbg.h dps8_fnp2.h
fnptelnet.o:        libtelnet.h dps8_utils.h fnpuv.h fnptelnet.h
fnpuv.o:            dps8_simh.h
fnpuv.o:            dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
fnpuv.o:            dps8_scu.h
fnpuv.o:            dps8_sys.h
fnpuv.o:            dps8_cable.h
fnpuv.o:            dps8_cpu.h hdbg.h dps8_fnp2.h libtelnet.h dps8_utils.h
fnpuv.o:            fnpuv.h
fnpuv.o:            fnptelnet.h
hdbg.o:             dps8.h
hdbg.o:             dps8_simh.h
hdbg.o:             dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
hdbg.o:             dps8_sys.h
hdbg.o:             uvutil.h dps8_cpu.h hdbg.h dps8_utils.h
libtelnet.o:        libtelnet.h
shm.o:              shm.h
threadz.o:          dps8.h
threadz.o:          dps8_simh.h
threadz.o:          dps8_math128.h dps8_hw_consts.h dps8_em_consts.h
threadz.o:          dps8_sys.h
threadz.o:          uvutil.h dps8_cpu.h hdbg.h dps8_faults.h dps8_iom.h
threadz.o:          dps8_utils.h
threadz.o:          threadz.h
tracker.o:          tracker.h
udplib.o:           h316_imp.h
uvutil.o:           dps8.h
uvutil.o:           dps8_hw_consts.h
uvutil.o:           dps8_em_consts.h dps8_sys.h uvutil.h dps8_cpu.h hdbg.h
uvutil.o:           dps8_utils.h
uvutil.o:           libtelnet.h
utfile.o:           utfile.h
bsd_random.o:       bsd_random.h

###############################################################################

ifdef LIBS
  export LIBS
  ifneq (,$(wildcard ../Makefile.mk))
    include ../Makefile.mk
  endif
endif

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
