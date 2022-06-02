# DPS/8M simulator: src/prt2pdf/GNUmakefile
# vim: filetype=make:tabstop=4:tw=78:noexpandtab

###############################################################################
#
# Copyright (c) 2013-2016 Charles Anthony
# Copyright (c) 2021 Jeffrey H. Johnson <trnsz@pobox.com>
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
all: prt2pdf$(EXE)
	-@$(TRUE)

###############################################################################

prt2pdf$(EXE): prt2pdf.c ../dpsprintf/dpsprintf.o
	@$(PRINTF) '%s\n' "CC: prt2pdf"     ||  $(TRUE)
	@$(SETV); $(CC) $(CFLAGS) $(LDFLAGS) "../dpsprintf/dpsprintf.o"           \
      -o prt2pdf$(EXE) prt2pdf.c $(MATHLIB) $(DUMALIBS)

###############################################################################

.PHONY: clean distclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean distclean
endif
clean distclean:
	@$(PRINTF) '%s\n' "CLEAN: prt2pdf"
	-@$(SETV); $(RMF) "prt2pdf$(EXE)"  > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "prt2pdf.exe"    > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "prt2pdf.o"      > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "../dpsprintf/dpsprintf.o"  > /dev/null 2>&1  ||  $(TRUE)

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
