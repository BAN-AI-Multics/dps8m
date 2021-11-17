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

UIFLAGS=-Wall -Wextra
CFLAGS+=$(UIFLAGS)

###############################################################################

ifneq "$(findstring icc,$(CC))" ""
  CFLAGS += -diag-disable=11074,11076,10148
endif

###############################################################################

.PHONY: all
all: unifdef$(EXE)

###############################################################################

unifdef$(EXE): unifdef.o unifdef.h version.h unifdef.c
	@$(PRINTF) '%s\n' "CC: unifdef"
	@$(SETV); $(CC) $(CFLAGS) $(LDFLAGS) -o unifdef$(EXE) unifdef.o $(UILIBS)

###############################################################################

.PHONY: clean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean
endif
clean:
	@$(PRINTF) '%s\n' "CLEAN: unifdef"
	@$(SETV); $(RMF) "unifdef$(EXE)"  > /dev/null 2>&1  ||  $(TRUE)
	@$(SETV); $(RMF) "unifdef.exe"    > /dev/null 2>&1  ||  $(TRUE)
	@$(SETV); $(RMF) "unifdef.o"      > /dev/null 2>&1  ||  $(TRUE)

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
