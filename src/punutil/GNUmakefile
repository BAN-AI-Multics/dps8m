# DPS8M simulator: src/punutil/GNUmakefile
# vim: filetype=make:tabstop=4:cc=79:ai:noexpandtab:list:listchars=tab\:\>\-
# SPDX-License-Identifier: MIT-0
# scspell-id: 5daf1b6a-f631-11ec-9e7c-80ee73e9b8e7
# Copyright (c) 2021-2025 The DPS8M Development Team

###############################################################################

.NOTPARALLEL:

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
all: punutil$(EXE)
	-@$(TRUE)

###############################################################################

punutil$(EXE): punutil.c
	@$(PRINTF) '%s\n' "CC: punutil"
	@$(SETV); $(CC) $(CFLAGS)                                                 \
      -o punutil$(EXE) punutil.c $(LDFLAGS) $(PULIBS) $(MATHLIB) $(DUMALIBS)

###############################################################################

.PHONY: clean distclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean distclean
endif
clean distclean:
	@$(PRINTF) '%s\n' "CLEAN: punutil"
	-@$(SETV); $(RMF) "punutil$(EXE)" > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "punutil.exe"   > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "punutil.o"     > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) ./*.ln          > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) ./*.gcda        > /dev/null 2>&1  ||  $(TRUE)

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
