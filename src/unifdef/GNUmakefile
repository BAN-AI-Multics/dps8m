# DPS8M simulator: src/unifdef/GNUmakefile
# vim: filetype=make:tabstop=4:ai:cc=79:noexpandtab:list:listchars=tab\:\>\-
# SPDX-License-Identifier: MIT-0
# scspell-id: 8122e0e8-f631-11ec-8c75-80ee73e9b8e7
# Copyright (c) 2021 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2021-2023 The DPS8M Development Team

###############################################################################

.DEFAULT_GOAL := all

###############################################################################

ifneq (,$(wildcard ../Makefile.mk))
  include ../Makefile.mk
endif

###############################################################################

ifneq "$(findstring icc,$(CC))" ""
  CFLAGS += -diag-disable=11074,11076,10148,10441
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

unifdef$(EXE): unifdef.o unifdef.h version.h unifdef.c
	@$(PRINTF) '%s\n' "LD: unifdef"
	@$(SETV); $(CC) $(CFLAGS)                                                 \
      -o unifdef$(EXE) unifdef.o $(LDFLAGS) $(MATHLIB) $(DUMALIBS)

###############################################################################

.PHONY: clean distclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean distclean
endif
clean distclean:
	@$(PRINTF) '%s\n' "CLEAN: unifdef"
	-@$(SETV); $(RMF) "unifdef$(EXE)" > /dev/null 2>&1 || $(TRUE)
	-@$(SETV); $(RMF) "unifdef.exe"   > /dev/null 2>&1 || $(TRUE)
	-@$(SETV); $(RMF) "unifdef.o"     > /dev/null 2>&1 || $(TRUE)
	-@$(SETV); $(RMF) ./*.gcda        > /dev/null 2>&1 || $(TRUE)
	-@$(SETV); $(RMF) ./*.ln          > /dev/null 2>&1 || $(TRUE)

###############################################################################

ifneq (,$(wildcard ./Dependency))
  include ./Dependency
endif

###############################################################################

ifneq (,$(wildcard ../Makefile.dev))
.PHONY: dep depend
ifneq (,$(findstring dep,$(MAKECMDGOALS)))
.NOTPARALLEL: dep depend
endif
dep depend: ../Makefile.dev
	@( $(CD) "../.." && $(MAKE) -j 1 -s "depend" )

.PHONY: cleandep depclean cleandepend dependclean
ifneq (,$(findstring dep,$(MAKECMDGOALS)))
.NOTPARALLEL: cleandep depclean cleandepend dependclean
endif
cleandep depclean cleandepend dependclean:
	@( $(CD) "../.." && $(MAKE) -j 1 -s "depclean" )
endif

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
