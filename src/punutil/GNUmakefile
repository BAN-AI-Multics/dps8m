# DPS/8M simulator: src/punutil/GNUmakefile
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

include ../Makefile.mk

###############################################################################

.PHONY: all
.NOTPARALLEL: all
punutil$(EXE) all: punutil.o
	@$(TEST) -f punutil$(EXE) || \
		$(PRINTF) '%s\n' "LD: punutil" || $(TRUE)
	@$(SETV); $(TEST) -f punutil$(EXE) || \
		$(CC) $(LDFLAGS) -o punutil$(EXE) punutil.o

###############################################################################

.PHONY: clean
.NOTPARALLEL: clean
clean:
	@$(PRINTF) '%s\n' "CLEAN: punutil"
	@$(SETV); $(RMF) punutil$(EXE) > /dev/null 2>&1 || $(TRUE)
	@$(SETV); $(RMF) punutil.exe > /dev/null 2>&1 || $(TRUE)
	@$(SETV); $(RMF) punutil.o > /dev/null 2>&1 || $(TRUE)

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
