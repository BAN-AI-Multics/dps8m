# DPS8M simulator: src/empty/GNUmakefile
# vim: filetype=make:tabstop=4:cc=79:ai:noexpandtab:list:listchars=tab\:\>\-
# SPDX-License-Identifier: MIT-0
# scspell-id: 9653fd78-f630-11ec-9e48-80ee73e9b8e7
# Copyright (c) 2022-2024 The DPS8M Development Team

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

LIBUTIL ?= -lutil
ifneq ($(UNAME_S),AIX)
  ifneq ($(UNAME_S),SunOS)
    ifneq ($(UNAME_S),Haiku)
      EMPTYLIBS ?= $(LIBUTIL)
    endif
  endif
endif

###############################################################################

LIBBSD ?= -lbsd
ifeq ($(UNAME_S),Haiku)
  EMPTYLIBS ?= $(LIBBSD)
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
