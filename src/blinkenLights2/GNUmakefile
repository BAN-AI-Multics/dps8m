# DPS/8M simulator: src/blinkenLights2/GNUmakefile
# vim: filetype=make:tabstop=4:tw=76
#
###############################################################################
#
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

ifneq (,$(wildcard ../Makefile.mk))
  include ../Makefile.mk
endif

###############################################################################

BLFLAGS=-Og -Wno-aggressive-loop-optimizations -Wno-unknown-warning-option
CFLAGS+=$(BLFLAGS)

###############################################################################

GTKVER="gtk+-3.0"
PKGCFG=$(shell $(ENV) $(PKGCONFIG) $(GTKVER) --cflags        ||               \
         $(PRINTF) '%s\n'                                                     \
           "Error running \"$(ENV) $(PKGCONFIG) $(GTKVER) --cflags\""         \
             >&2)
CFLAGS+=$(PKGCFG) -I../dps8 -DM_SHARED

###############################################################################

PKGLFG=$(shell $(ENV) $(PKGCONFIG) $(GTKVER) --libs          ||               \
         $(PRINTF) '%s\n'                                                     \
           "Error running \"$(ENV) $(PKGCONFIG) $(GTKVER) --libs\""           \
             >&2)
LDFLAGS+=$(PKGLFG)

###############################################################################

.PHONY: all
all: blinkenLights2$(EXE)

###############################################################################

blinkenLights2$(EXE): blinkenLights2.o ../dps8/shm.o
	@$(PRINTF) '%s\n' "LD: blinkenLights2"
	@$(SETV); $(CC) $(CFLAGS) -I"../simh" -I"../dps8" -DM_SHARED              \
      $(PKGCFG) $(LDFLAGS) "../dps8/shm.o" -o blinkenLights2$(EXE)            \
        blinkenLights2.o $(BLLIBS)

###############################################################################

.PHONY: clean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean
endif
clean:
	@$(PRINTF) '%s\n' "CLEAN: blinkenLights2"
	@$(SETV); $(RMF) "blinkenLights2$(EXE)" > /dev/null 2>&1  ||  $(TRUE)
	@$(SETV); $(RMF) "blinkenLights2.exe"   > /dev/null 2>&1  ||  $(TRUE)
	@$(SETV); $(RMF) "blinkenLights2.o"     > /dev/null 2>&1  ||  $(TRUE)

###############################################################################

blinkenLights2.o:   ../dps8/dps8.h
blinkenLights2.o:   ../dps8/dps8_simh.h
blinkenLights2.o:   ../dps8/dps8_math128.h
blinkenLights2.o:   ../dps8/dps8_hw_consts.h
blinkenLights2.o:   ../dps8/dps8_em_consts.h
blinkenLights2.o:   ../dps8/dps8_sys.h
blinkenLights2.o:   ../dps8/hdbg.h
blinkenLights2.o:   ../dps8/shm.h

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
