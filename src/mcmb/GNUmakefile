# DPS/8M simulator: src/mcmb/GNUmakefile
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

#CMBFLAGS=-Wall -Wextra -Wpedantic
CFLAGS+=$(CMBFLAGS)

###############################################################################

ifneq "$(findstring icc,$(CC))" ""
  CFLAGS += -diag-disable=11074,11076,10148
endif

###############################################################################

.PHONY: all
all: mcmb$(EXE)

###############################################################################

mcmb$(EXE): mcmb.c
	@$(PRINTF) '%s\n' "CC: mcmb"
	@$(SETV); $(CC) $(CFLAGS) $(LDFLAGS) -o mcmb$(EXE) mcmb.c $(CMBLIBS)

###############################################################################

.PHONY: clean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean
endif
clean:
	@$(PRINTF) '%s\n' "CLEAN: mcmb"
	@$(SETV); $(RMF) "mcmb$(EXE)" > /dev/null 2>&1  ||  $(TRUE)
	@$(SETV); $(RMF) "mcmb.exe"   > /dev/null 2>&1  ||  $(TRUE)
	@$(SETV); $(RMF) "mcmb.o"     > /dev/null 2>&1  ||  $(TRUE)

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
