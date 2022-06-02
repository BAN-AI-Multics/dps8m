# DPS/8M simulator: src/mcmb/GNUmakefile
# vim: filetype=make:tabstop=4:tw=78:noexpandtab

###############################################################################
#
# Copyright (c) 2021-2022 The DPS8M Development Team
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

ifneq "$(findstring icc,$(CC))" ""
  CFLAGS += -diag-disable=11074,11076,10148
endif

###############################################################################

.PHONY: all
ifeq ($(CROSS),MINGW64)
  WINDOWS_COMPILE=1
endif # MINGW64
ifeq ($(CROSS),MINGW32)
  WINDOWS_COMPILE=1
endif # MINGW32
ifeq ($(CYGWIN_MINGW_CROSS),1)
  WINDOWS_COMPILE=1
endif # CYGWIN_MINGW_CROSS
ifneq ($(WINDOWS_COMPILE),1)
all: mcmb$(EXE)
	-@$(TRUE)
else # !WINDOWS_COMPILE
all:
	-@$(TRUE)
endif # WINDOWS_COMPILE

###############################################################################

mcmb$(EXE): mcmb.c ../dpsprintf/dpsprintf.o
	@$(PRINTF) '%s\n' "CC: mcmb"
	-@$(SETV); $(CC) $(CFLAGS) $(LDFLAGS) "../dpsprintf/dpsprintf.o"          \
      -o mcmb$(EXE) mcmb.c $(MATHLIB) $(CMBLIBS) $(DUMALIBS)

###############################################################################

.PHONY: clean distclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean distclean
endif
clean distclean:
	@$(PRINTF) '%s\n' "CLEAN: mcmb"
	-@$(SETV); $(RMF) "mcmb$(EXE)" > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "mcmb.exe"   > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "mcmb.o"     > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "../dpsprintf/dpsprintf.o" > /dev/null 2>&1  ||  $(TRUE)

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
