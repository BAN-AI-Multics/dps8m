# DPS/8M simulator: src/unifdef/GNUmakefile
# vim: filetype=make:tabstop=4:tw=76
#
##############################################################################
#
# Copyright (c) 2021 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2021 The DPS8M Development Team
#
# This Makefile is made available under the terms of the ICU
# License, version 1.8.1 or later.  For more details, see the
# LICENSE.md file at the top-level directory of this distribution.
#
###############################################################################
#
# This Software ("Unifdef") is distributed under the two-clause BSD copyright
# licence. Unifdef was derived from software contributed to Berkeley by Dave
# Yost. It was rewritten to support ANSI C by Tony Finch. The original version
# of unifdef.c carried the four-clause BSD copyright licence. None of its code
# remains in this version (though some of the names remain) so it now carries
# the more liberal two-clause BSD licence.
#
###############################################################################

.DEFAULT_GOAL := all

###############################################################################

ifneq (,$(wildcard ../Makefile.mk))
  include ../Makefile.mk
endif

###############################################################################

UIFLAGS=-Wall -Wextra -Wpedantic -D_FORTIFY_SOURCE=2
CFLAGS+=$(UIFLAGS)

###############################################################################

.PHONY: all
unifdef$(EXE) all: unifdef.o
	@$(TEST) -f unifdef$(EXE)            ||                                   \
		$(PRINTF) '%s\n' "LD: unifdef"   ||  $(TRUE)
	-@$(SETV); $(TEST) -f unifdef$(EXE)  ||                                   \
		$(CC) $(CFLAGS) $(LDFLAGS) -o unifdef$(EXE) unifdef.o

###############################################################################

.PHONY: clean
.NOTPARALLEL: clean
clean:
	@$(PRINTF) '%s\n' "CLEAN: unifdef"
	@$(SETV); $(RMF) "unifdef$(EXE)"  > /dev/null 2>&1  ||  $(TRUE)
	@$(SETV); $(RMF) "unifdef.exe"    > /dev/null 2>&1  ||  $(TRUE)
	@$(SETV); $(RMF) "unifdef.o"      > /dev/null 2>&1  ||  $(TRUE)
	@$(SETV); $(RMF) "cflags.L"       > /dev/null 2>&1  ||  $(TRUE)

###############################################################################

unifdef.o: unifdef.h
unifdef.o: version.h
unifdef.o: unifdef.c
unifdef.o: cflags.L

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
