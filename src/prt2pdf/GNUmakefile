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
# LICENSE file at the top-level directory of this distribution.
#
###############################################################################

include ../Makefile.mk

###############################################################################

.PHONY: all
.NOTPARALLEL: all
prt2pdf$(EXE) all: prt2pdf.o
	@$(TEST) -f prt2pdf$(EXE) || \
		$(PRINTF) '%s\n' "LD: prt2pdf" || $(TRUE)
	@$(SETV); $(TEST) -f prt2pdf$(EXE) || \
		$(CC) $(LDFLAGS) -o prt2pdf$(EXE) prt2pdf.o

###############################################################################

.PHONY: clean
.NOTPARALLEL: clean
clean:
	@$(PRINTF) '%s\n' "CLEAN: prt2pdf"
	@$(SETV); $(RMF) prt2pdf$(EXE) > /dev/null 2>&1 || $(TRUE)
	@$(SETV); $(RMF) prt2pdf.exe > /dev/null 2>&1 || $(TRUE)
	@$(SETV); $(RMF) prt2pdf.o > /dev/null 2>&1 || $(TRUE)

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
