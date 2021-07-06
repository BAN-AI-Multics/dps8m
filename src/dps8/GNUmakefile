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
# LICENSE file at the top-level directory of this distribution.
#
###############################################################################

###############################################################################
#
# Condition builds
#
#  TESTING=1    Enables sim_debug and additional runtime checks
#
###############################################################################

.PHONY: all
all:

###############################################################################

INSTALL_ROOT ?= $(PREFIX)
INSTALL_BIN  ?= $(INSTALL_ROOT)/bin

###############################################################################

include ../Makefile.mk

###############################################################################

ifneq (,$(wildcard Makefile.local))
include Makefile.local
endif

###############################################################################

ifdef TRACKER
CFLAGS += -DTRACKER -DTESTING
endif

###############################################################################

ifdef TESTING
CFLAGS += -DTESTING
endif

###############################################################################

#ifeq ($(LOADER),1)
CFLAGS += -DLOADER
#endif

###############################################################################

ifeq ($(OS),Windows_NT)
CROSS=MINGW64
endif

###############################################################################

ifeq ($(THREADZ),1)
CFLAGS += -pthread -DTHREADZ
LDFLAGS += -pthread
LIBS += -lpthread
endif

###############################################################################

ifeq ($(ISOLTS),1)
LOCKLESS=1
CFLAGS += -DISOLTS
CFLAGS += -DWAM
endif

###############################################################################

ifeq ($(LOCKLESS),1)
CFLAGS += -pthread -DLOCKLESS 
LDFLAGS += -pthread
LIBS += -lpthread
endif

###############################################################################

ifeq ($(SPEED),1)
SPEED=1
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
LIBS += -lwinmm -lws2_32 -lmingwex -luv -lwsock32 -lws2_32 \
        -L../../../libuv/.libs
endif

###############################################################################

CFLAGS += -I../decNumber -I$(SIMHx)

###############################################################################
# Fix for gcc reporting bogus 'conversion' warnings

ifneq ($(W),)
ifeq ($(CC),clang)
CFLAGS += -Wall -Wconversion -Wsign-conversion
else
CFLAGS += -Wall -Wno-conversion -Wsign-conversion
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
LDFLAGS += -lrt
endif
endif

###############################################################################
# Configuration switches

ifeq ($(CROSS),MINGW64)
LOCALLIBS += $(SIMHx)/simh.a ../decNumber/decNumber.a 
else
LOCALLIBS += $(SIMHx)/simh.a ../decNumber/decNumber.a
endif

###############################################################################
# What needs -ldl? 

ifeq ($(OS),Windows_NT)
    LIBS += -lgcc -lm -lwinmm -lws2_32 -lmingwex $(LIBUV) -lwsock32 \
            -lpthread -lgnurx -lpsapi -liphlpapi -luserenv
else
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
endif

###############################################################################

ifneq ($(CROSS),MINGW64)
C_SRCS  = ./dps8_absi.c
endif
C_SRCS += ./dps8_addrmods.c 
C_SRCS += ./dps8_append.c
C_SRCS += ./dps8_cable.c
C_SRCS += ./dps8_console.c
C_SRCS += ./dps8_cpu.c
C_SRCS += ./dps8_crdpun.c
C_SRCS += ./dps8_crdrdr.c
C_SRCS += ./dps8_decimal.c
C_SRCS += ./dps8_disk.c
C_SRCS += ./dps8_eis.c
C_SRCS += ./dps8_faults.c
C_SRCS += ./dps8_fnp2.c
C_SRCS += ./dps8_fnp2_iomcmd.c
C_SRCS += ./dps8_iefp.c
C_SRCS += ./dps8_ins.c
C_SRCS += ./dps8_iom.c
#ifeq ($(LOADER),1)
C_SRCS += ./dps8_loader.c
#endif
C_SRCS += ./dps8_math.c
C_SRCS += ./dps8_math128.c
C_SRCS += ./dps8_mt.c
C_SRCS += ./dps8_opcodetable.c
C_SRCS += ./dps8_prt.c
C_SRCS += ./dps8_scu.c
C_SRCS += ./dps8_simh.c
ifneq ($(CROSS),MINGW64)
C_SRCS += ./dps8_socket_dev.c
endif
ifeq ($(TRACKER),1)
C_SRCS += ./tracker.c
endif
C_SRCS += ./dps8_sys.c
C_SRCS += ./dps8_urp.c
C_SRCS += ./dps8_utils.c
C_SRCS += ./fnpuv.c
C_SRCS += ./fnptelnet.c
C_SRCS += ./libtelnet.c
C_SRCS += ./hdbg.c
# The system_state code relies on M_SHARED.
ifneq ($(CROSS),MINGW64)
#ifeq ($(M_SHARED),1)
C_SRCS += ./shm.c
#endif
endif
ifneq ($(CROSS),MINGW64)
C_SRCS += ./udplib.c
endif
ifeq ($(THREADZ),1)
C_SRCS += ./threadz.c
endif
ifeq ($(LOCKLESS),1)
C_SRCS += ./threadz.c
endif
C_SRCS += ./uvutil.c
ifeq ($(TRACKER),1)
H_SRCS += ./tracker.h
endif

###############################################################################

OBJS  := $(patsubst %.c,%.o,$(C_SRCS))

###############################################################################

ifneq ($(CROSS),MINGW64)
H_SRCS  = dps8_absi.h
endif
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
#ifeq ($(LOADER),1)
H_SRCS += dps8_loader.h
#endif
H_SRCS += dps8_math.h
H_SRCS += dps8_math128.h
#H_SRCS += dps8_mp.h
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
H_SRCS += hdbg.h
# The system_state code relies on M_SHARED.
#ifeq ($(M_SHARED),1)
H_SRCS += shm.h
#endif
ifneq ($(CROSS),MINGW64)
H_SRCS += udplib.h
endif
H_SRCS += utarray.h
H_SRCS += uthash.h
H_SRCS += utlist.h
ifeq ($(THREADZ),1)
H_SRCS += ./threadz.h
endif
ifeq ($(LOCKLESS),1)
H_SRCS += ./threadz.h
endif
H_SRCS += ./uvutil.h

###############################################################################

.PHONY: all
all: prt2pdf unifdef $(TAGS) locallibs_then_dps8

###############################################################################

.PHONY: install
install: all
	@$(SETV); $(MKDIR) "$(DESTDIR)$(INSTALL_BIN)"
	@$(SETV); $(CP) "dps8$(EXE)" "$(DESTDIR)$(INSTALL_BIN)/dps8$(EXE)"

###############################################################################

.PHONY: cscope
cscope:
	@$(PRINTF) '%s\n' "CSCOPE: dps8"
	@$(SETV); $(CSCOPE) $(C_SRCS)

###############################################################################

.PHONY: android
android:
	@$(PRINTF) '%s\n' "NDK-BUILD: Android, ABI=arm64-v8"
	@$(SETV); $(NDKBUILD) APP_BUILD_SCRIPT=./Android.mk \
                          APP_ABI=arm64-v8a \
                          APP_PLATFORM=android-21

###############################################################################

.PHONY: printuk
printuk:
	@$(PRINTF) '%s\n' "*** Begin printuk (git untracked files):"
	@$(SETV); ($(CD) "../.." && $(GIT) status -s --untracked-files="all" \
	    2> /dev/null | $(GREP) '^? ' || $(TRUE))
	@$(PRINTF) '%s\n' "*** End printuk"

###############################################################################

.PHONY: printmod
printmod:
	@$(PRINTF) '%s\n' "*** Begin printmod (git tracked modified files)"
	@$(SETV); ($(CD) "../.." && $(GIT) status -s 2> /dev/null | \
	    $(GREP) '^\ \?M ' || $(TRUE))
	@$(PRINTF) '%s\n' "*** End printmod"

###############################################################################

blinkenLights: blinkenLights.c shm.o
	@$(PRINTF) '%s\n' "LD: blinkenLights"
	@$(SETV); $(CC) $(CFLAGS) $(LDFLAGS) blinkenLights.c \
        -o blinkenLights shm.o `$(PKGCONFIG) --cflags --libs gtk+-3.0`

###############################################################################

blinkenLights2: blinkenLights2.c shm.o
	@$(PRINTF) '%s\n' "LD: blinkenLights2s"
	@$(SETV); $(CC) $(CFLAGS) $(LDFLAGS) blinkenLights2.c \
        -o blinkenLights2 shm.o `$(PKGCONFIG) --cflags --libs gtk+-3.0` -lrt

###############################################################################

blinkenLightsPi: blinkenLightsPi.c shm.o
	@$(PRINTF) '%s\n' "LD: blinkenLightsPi"
	@$(SETV); $(CC) $(CFLAGS) $(LDFLAGS) blinkenLightsPi.c \
        -o blinkenLightsPi shm.o 

###############################################################################

blinkenLightsCurses: blinkenLightsCurses.c shm.o
	@$(PRINTF) '%s\n' "LD: blinkenLightsCurses"
	@$(SETV); $(CC) $(CFLAGS) $(LDFLAGS) blinkenLightsCurses.c \
        -o blinkenLightsCurses shm.o -lncurses

###############################################################################

dps8$(EXE) : $(OBJS) $(LOCALOBJS) $(LOCALLIBS) 
	@$(PRINTF) '%s\n' "LD: dps8$(EXE)"
	@$(SETV); $(LD) -o dps8$(EXE) $(LDFLAGS) $(OBJS) $(LOCALOBJS) \
        $(LOCALLIBS) $(LIBS)

###############################################################################

.PHONY: FORCE
ifeq (,$(wildcard ../../.git))
# not git
ifeq (,$(wildcard ver.h~))
# not git, not kit
ver.h:
	@$(PRINTF) '%s\n' "Error: Unable to create ver.h!"; exit 1
else
# kit
ver.h:
	@$(SETV); $(CP) "./ver.h~" "./ver.h"
endif
else
# git
ver.h: FORCE
	@$(SETV); ./make_ver.sh || \
		$(SHELL) ./make_ver.sh || \
			{ $(PRINTF) '%s\n' "Error running make_ver.sh"; exit 1; }
endif

###############################################################################

slte.inc : system_book_12_3.ascii
	@$(SETV); ./buildSLTE.sh

###############################################################################

.PHONY: locallibs_then_dps8
locallibs_then_dps8: 
	@($(SETV); $(CD) $(SIMHx) && $(MAKE) -f "GNUmakefile")
	@($(SETV); $(CD) "../decNumber" && $(MAKE))
	@$(SETV); $(ENV) MAKE="$(MAKE)" \
        $(MAKE) "ver.h"
	@$(SETV); $(MAKE) "dps8$(EXE)"

###############################################################################

prt2pdf:
	@$(SETV); $(MAKE) -C "../prt2pdf"

###############################################################################

unifdef:
	-@$(SETV); $(MAKE) -C "../unifdef"

###############################################################################

udplib: udplib.c udplib.h
	@$(SETV); $(CC) udplib.c -o udplib -DTEST=1

###############################################################################

.PHONY: zeromq czmq
../lib/libzmq.a zeromq zmq:
	@($(SETV); $(CD) "../zeromq-4.1.0" && \
        ./configure --prefix="$$(pwd)/../" && \
            $(MAKE) && \
                $(MAKE) check && \
                    $(MAKE) install)

###############################################################################

.PHONY: czmq
../lib/libczmq.so czmq:
	@($(SETV); $(CD) "../czmq-master-ff0d028b16" && \
		$(CMAKE) "." -DZEROMQ_LIBRARIES="../lib/libzmq.so" \
            -DZEROMQ_INCLUDE_DIRS="../include/" \
            -DCMAKE_INSTALL_PREFIX=".." && \
                $(MAKE) && \
                    $(MAKE) install)

###############################################################################

.PHONY: zyre
../lib/libzyre.so zyre:
	@($(SETV); $(CD) "../zyre-master-9d7e62d2c9" && \
        $(CMAKE) "." -DCZMQ_LIBRARIES="../lib/libczmq.so" \
            -DCZMQ_INCLUDE_DIRS="../include" \
            -DZEROMQ_LIBRARIES="../lib/libzmq.so" \
            -DZEROMQ_INCLUDE_DIRS="../include/" \
            -DCMAKE_INSTALL_PREFIX=".." && \
                $(MAKE) && \
                    $(MAKE) install)

###############################################################################

.PHONY: ctags
tags ctags: $(C_SRCS) $(H_SRCS) ver.h
	@$(PRINTF) '%s\n' "TAGS: dps8"
	-@$(SETV); $(CTAGS) $(C_SRCS) $(H_SRCS) \
        $(SIMHx)/*.[ch] ../decNumber/dec*.[ch]

###############################################################################

.PHONY: tapes
tapes: 88534.tap 88631.tap 88632.tap 88633.tap 88634.tap 88635.tap 88636.tap \
       99020.tap 93085.tap 98570.tap 99019.tap 98577.tap 20184.tap 20185.tap \
       20186.tap 20187.tap 20188.tap t4d_b.2.tap 98580_1er.tap 98576_1er.tap

%.tap:
	@$(SETV); $(WEBDL) \
       "https://bitsavers.trailing-edge.com/bits/Honeywell/multics/tape/$@.gz"
	@$(SETV); $(GUNZIP) "$@.gz"

###############################################################################

.PHONY: superclean realclean reallyclean
superclean realclean reallyclean: distclean
	@$(SETV); $(CCACHE) -cC > /dev/null 2>&1 || $(TRUE)

###############################################################################

.PHONY: distclean
distclean: clean
	-@$(SETV); $(RMF) "ver.h" || $(TRUE)
	-@$(SETV); $(RMF) "../../dps8m.state" || $(TRUE)
	-@$(SETV); $(RMF) "restoreMultics.log" || $(TRUE)

###############################################################################

.PHONY: clean
clean:
	@$(SETV); $(MAKE) -C "$(SIMHx)" -f "GNUmakefile" "clean"
	@$(SETV); $(MAKE) -C "../decNumber" "clean"
	@$(SETV); $(MAKE) -C "../prt2pdf" "clean"
	@$(SETV); $(MAKE) -C "../unifdef" "clean"
	@$(SETV); $(MAKE) "clean_dps8"
	@$(SETV); $(RMF) ../../"$(KITNAME)"."$(TARXT)" || $(TRUE)
	@$(SETV); $(RMF) ../../"$(KITNAME)"."$(TARXT)".* || $(TRUE)

###############################################################################

.PHONY: clean_dps8
clean_dps8:
	@$(PRINTF) '%s\n' "CLEAN: dps8"
	-@$(SETV); $(RMF) $(OBJS) $(C_DEPS) $(EXECUTABLES) "dps8$(EXE)" \
        "tags" $(C_SRCS:.c=.d) $(wildcard $(C_SRCS:.c=.d.[0-9]*)) \
            "dps8.exe" "errnos.h" "udplib" || $(TRUE)

###############################################################################

.PHONY: kit dist
kit dist: 
	@$(PRINTF) '%s\n' "KIT: dps8"
	@$(PRINTF) '%s\n' "ARCHIVE: $(KITNAME).$(TARXT)"
	@$(SETV); $(RMF) ../../"$(KITNAME)"."$(TARXT)" || $(TRUE)
	@$(SETV); $(RMF) ../../"$(KITNAME)"."$(TARXT)"."$(COMPRESSXT)" || $(TRUE)
	@$(SETV); $(ENV) MAKE="$(MAKE)" $(MAKE) "ver.h"
	@$(SETV); $(CP) "./ver.h" "./ver.h~"
	@$(SETV); ($(CD) "../.." && \
		$(MAKETAR) "$(KITNAME).$(TARXT)" $$( $(GIT) ls-files | \
			$(GREP) -v '^.git' | $(GREP) -v '^.whitesource' | \
				$(GREP) -v '^src/dps8/listings/' )  'src/dps8/ver.h~' )
	@$(PRINTF) '%s\n' \
		"COMPRESS: $(KITNAME).$(TARXT) -> $(KITNAME).$(TARXT).$(COMPRESSXT)"
	@$(SETV); $(COMPRESS) "../../$(KITNAME).$(TARXT)"

###############################################################################

dps8_socket_dev.c: errnos.h
errnos.h:
	@$(PRINTF) '%s\n' \
        '#include <errno.h>' > ./dump.c
	@$(SETV); $(CC) -E -dD ./dump.c | \
        $(AWK) '/^#define E/ { printf "{\"%s\",%s},\n", $$2, $$2 }' \
            > ./errnos.h
	@$(SETV); $(RMF) ./dump.c

###############################################################################

%.d: %.c
	@$(SET) -e; $(RMNF) $@ 2> /dev/null; \
        $(SETV); $(CC) -MM $(CFLAGS) $< > $@.$$$$; \
            $(SED) 's/ ver.h //g' $@.$$$$ | \
                $(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@; \
                    $(RMF) $@.$$$$;

###############################################################################

-include $(C_SRCS:.c=.d)

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
