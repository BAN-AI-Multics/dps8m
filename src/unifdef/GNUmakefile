# DPS/8M simulator: src/unifdef/GNUmakefile
# vim: filetype=make:tabstop=4:tw=78:noexpandtab

##############################################################################
#
# Copyright (c) 2021 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2021-2022 The DPS8M Development Team
#
# This Makefile is made available under the terms of the ICU
# License, version 1.8.1 or later.  For more details, see the
# LICENSE.md file at the top-level directory of this distribution.
#
###############################################################################
#
# This Software ("Unifdef") is distributed under a two-clause BSD copyright
# licence. Unifdef was derived from software contributed to U.C. Berkeley by
# Dave Yost. It was re-written to support ANSI C by Tony Finch. The original
# version of Unifdef carried a four-clause BSD copyright licence. None of its
# code remains in this version - though some of the names remain - so it now
# carries the more liberal two-clause BSD licence.
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

.PHONY: silent silently
silent silently:
	-@$(MAKE) --no-print-directory -s "all" > /dev/null 2>&1 || $(MAKE) "all"

###############################################################################

.PHONY: all
all: unifdef$(EXE)
	-@$(TRUE)

###############################################################################

unifdef$(EXE): unifdef.o unifdef.h version.h unifdef.c ../dpsprintf/dpsprintf.o
	@$(PRINTF) '%s\n' "CC: unifdef"
	@$(SETV); $(CC) $(CFLAGS) $(LDFLAGS) "../dpsprintf/dpsprintf.o"           \
      -o unifdef$(EXE) unifdef.o $(MATHLIB) $(DUMALIBS)

###############################################################################

.PHONY: clean distclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean distclean
endif
clean distclean:
	@$(PRINTF) '%s\n' "CLEAN: unifdef"
	-@$(SETV); $(RMF) "unifdef$(EXE)"  > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "unifdef.exe"    > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "unifdef.o"      > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "../dpsprintf/dpsprintf.o"  > /dev/null 2>&1  ||  $(TRUE)

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
