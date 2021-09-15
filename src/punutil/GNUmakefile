# DPS/8M simulator: src/punutil/GNUmakefile
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

PUFLAGS=-Wall -Wextra
CFLAGS+=$(PUFLAGS)

###############################################################################

.PHONY: all

all: punutil$(EXE)

###############################################################################


punutil$(EXE): punutil.c
	@$(PRINTF) '%s\n' "CC: punutil"
	@$(SETV); $(CC) $(CFLAGS) $(LDFLAGS) -o punutil$(EXE) punutil.c

###############################################################################

.PHONY: clean

clean:
	@$(PRINTF) '%s\n' "CLEAN: punutil"
	@$(SETV); $(RMF) "punutil$(EXE)" > /dev/null 2>&1  ||  $(TRUE)
	@$(SETV); $(RMF) "punutil.exe"   > /dev/null 2>&1  ||  $(TRUE)
	@$(SETV); $(RMF) "punutil.o"     > /dev/null 2>&1  ||  $(TRUE)
	@$(SETV); $(RMF) "cflags.L"      > /dev/null 2>&1  ||  $(TRUE)

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
