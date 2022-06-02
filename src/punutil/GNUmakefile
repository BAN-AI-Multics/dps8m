# DPS/8M simulator: src/punutil/GNUmakefile
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
all: punutil$(EXE)
	-@$(TRUE)

###############################################################################

punutil$(EXE): punutil.c ../dpsprintf/dpsprintf.o
	@$(PRINTF) '%s\n' "CC: punutil"
	@$(SETV); $(CC) $(CFLAGS) $(LDFLAGS) "../dpsprintf/dpsprintf.o"           \
      -o punutil$(EXE) punutil.c $(MATHLIB) $(DUMALIBS)

###############################################################################

.PHONY: clean distclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean distclean
endif
clean distclean:
	@$(PRINTF) '%s\n' "CLEAN: punutil"
	-@$(SETV); $(RMF) "punutil$(EXE)" > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "punutil.exe"   > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "punutil.o"     > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "../dpsprintf/dpsprintf.o"  > /dev/null 2>&1  ||  $(TRUE)

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
