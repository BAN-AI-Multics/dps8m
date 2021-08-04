# DPS/8M simulator: src/prt2pdf/GNUmakefile
# vim: filetype=make:tabstop=4:tw=76
#
###############################################################################
#
# Copyright (c) 2013-2016 Charles Anthony
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

.PHONY: all osconf
.NOTPARALLEL: all osconf
prt2pdf$(EXE) all: osconf prt2pdf.o cflags.L
	@$(TEST) -f prt2pdf$(EXE) ||                                              \
		$(PRINTF) '%s\n' "LD: prt2pdf" || $(TRUE)
	@$(SETV); $(TEST) -f prt2pdf$(EXE) ||                                     \
		$(CC) $(LDFLAGS) -o prt2pdf$(EXE) prt2pdf.o

###############################################################################

.PHONY: clean
.NOTPARALLEL: clean
clean:
	@$(PRINTF) '%s\n' "CLEAN: prt2pdf"
	-@$(SETV); $(RMF) "prt2pdf$(EXE)"  > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "prt2pdf.exe"    > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "prt2pdf.o"      > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "cflags.L"       > /dev/null 2>&1  ||  $(TRUE)

###############################################################################

prt2pdf.o: prt2pdf.c
prt2pdf.o: cflags.L

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
