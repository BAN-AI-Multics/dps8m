# DPS/8M simulator: src/telco/GNUmakefile
# vim: filetype=make:tabstop=4:tw=76
#
###############################################################################
#
# Copyright (c) 2022 Charles Anthony
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

ifneq (,$(wildcard ../Makefile.mk))
  include ../Makefile.mk
endif

###############################################################################

PRFLAGS=-Wall -Wextra
CFLAGS+=$(PRFLAGS)

###############################################################################

ifneq "$(findstring icc,$(CC))" ""
  CFLAGS += -diag-disable=11074,11076,10148
endif

###############################################################################

.PHONY: all
all: telco2$(EXE)

###############################################################################

telco$(EXE): telco.c
	@$(PRINTF) '%s\n' "CC: telco"     ||  $(TRUE)
	@$(SETV); $(CC) $(CFLAGS) $(LDFLAGS) -o telco$(EXE) telco.c $(PRLIBS)

###############################################################################

telco2$(EXE): telco2.c
	@$(PRINTF) '%s\n' "CC: telco2"     ||  $(TRUE)
	@$(SETV); $(CC) $(CFLAGS) $(LDFLAGS) -o telco2$(EXE) telco2.c $(PRLIBS) -luv

###############################################################################

.PHONY: clean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean
endif
clean:
	@$(PRINTF) '%s\n' "CLEAN: telco"
	-@$(SETV); $(RMF) "telco$(EXE)"  > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "telco.exe"    > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "telco.o"      > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "telco2$(EXE)" > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "telco2.exe"   > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "telco2.o"     > /dev/null 2>&1  ||  $(TRUE)

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
