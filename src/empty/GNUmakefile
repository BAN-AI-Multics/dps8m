# DPS8M simulator: src/empty/GNUmakefile
# vim: filetype=make:tabstop=4:tw=79:noexpandtab:list:listchars=tab\:\>\-
# vim: ruler:hlsearch:incsearch:autoindent:wildmenu:wrapscan:colorcolumn=79
# SPDX-License-Identifier: FSFAP
# scspell-id: 9653fd78-f630-11ec-9e48-80ee73e9b8e7

###############################################################################
#
# Copyright (c) 2022 The DPS8M Development Team
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered "AS-IS",
# without any warranty.
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
ifeq ($(CROSS),MINGW64)
  WINDOWS_COMPILE=1
endif # MINGW64
ifeq ($(CROSS),MINGW32)
  WINDOWS_COMPILE=1
endif # MINGW32
ifeq ($(CYGWIN_MINGW_CROSS),1)
  WINDOWS_COMPILE=1
endif # CYGWIN_MINGW_CROSS
ifneq ($(WINDOWS_COMPILE),1)
all: empty$(EXE)
	-@$(TRUE)
else # !WINDOWS_COMPILE
all:
	-@$(TRUE)
endif # WINDOWS_COMPILE

###############################################################################

ifneq ($(UNAME_S),AIX)
  ifneq ($(UNAME_S),SunOS)
    EMPTYLIBS ?= -lutil
  endif
endif

###############################################################################

empty$(EXE): empty.c
	@$(PRINTF) '%s\n' "CC: empty"
	@$(SETV); $(CC) $(CFLAGS)                                                 \
      -o empty$(EXE) empty.c $(LDFLAGS) $(EMPTYLIBS) $(MATHLIB) $(DUMALIBS)

###############################################################################

.PHONY: clean distclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean distclean
endif
clean distclean:
	@$(PRINTF) '%s\n' "CLEAN: empty"
	-@$(SETV); $(RMF) "empty$(EXE)" > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "empty.exe"   > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "empty.o"     > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) ./*.gcda      > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) ./*.ln        > /dev/null 2>&1  ||  $(TRUE)

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