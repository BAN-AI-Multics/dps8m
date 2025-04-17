# DPS8M simulator: src/tap2raw/GNUmakefile
# vim: filetype=make:tabstop=4:ai:cc=79:noexpandtab:list:listchars=tab\:\>\-
# SPDX-License-Identifier: MIT-0
# scspell-id: 131e6164-1afc-11f0-96cf-80ee73e9b8e7
# Copyright (c) 2013-2016 Charles Anthony
# Copyright (c) 2021-2024 Jeffrey H. Johnson
# Copyright (c) 2021-2025 The DPS8M Development Team

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

.PHONY: all
all: tap2raw$(EXE)
	-@$(TRUE)

###############################################################################

tap2raw$(EXE): tap2raw.c
	@$(PRINTF) '%s\n' "CC: tap2raw"     ||  $(TRUE)
	@$(SETV); $(CC) $(CFLAGS)                                                 \
      -o tap2raw$(EXE) tap2raw.c $(LDFLAGS) $(MATHLIB) $(DUMALIBS)

###############################################################################

.PHONY: clean distclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean distclean
endif
clean distclean:
	@$(PRINTF) '%s\n' "CLEAN: tap2raw"
	-@$(SETV); $(RMF) "tap2raw$(EXE)"  > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "tap2raw.exe"    > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "tap2raw.o"      > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) ./*.gcda         > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) ./*.ln           > /dev/null 2>&1  ||  $(TRUE)

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
