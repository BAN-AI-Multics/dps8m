# DPS8M simulator: src/novdso/GNUmakefile
# vim: filetype=make:tabstop=4:ai:cc=79:noexpandtab:list:listchars=tab\:\>\-
# SPDX-License-Identifier: MIT-0
# scspell-id: 4887ce61-435a-11ed-b4c1-80ee73e9b8e7
# Copyright (c) 2020 Daniel Teunis
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
  SKIP_COMPILATION=1
endif # MINGW64
ifeq ($(CROSS),MINGW32)
  SKIP_COMPILATION=1
endif # MINGW32
ifeq ($(CYGWIN_MINGW_CROSS),1)
  SKIP_COMPILATION=1
endif # CYGWIN_MINGW_CROSS
ifeq ($(UNAME_S),AIX)
  SKIP_COMPILATION=1
endif # AIX
ifneq ($(SKIP_COMPILATION),1)
all: novdso$(EXE)
	-@$(TRUE)
else # !SKIP_COMPILATION
all:
	-@$(TRUE)
endif # SKIP_COMPILATION

###############################################################################

novdso$(EXE): novdso.c
	@$(PRINTF) '%s\n' "CC: novdso"
	@$(SETV); $(CC) $(CFLAGS)                                                 \
      -o novdso$(EXE) novdso.c $(LDFLAGS) $(MATHLIB) $(DUMALIBS)

###############################################################################

.PHONY: clean distclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean distclean
endif
clean distclean:
	@$(PRINTF) '%s\n' "CLEAN: novdso"
	-@$(SETV); $(RMF) "novdso$(EXE)" > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "novdso.exe"   > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "novdso.o"     > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) ./*.ln         > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) ./*.gcda       > /dev/null 2>&1  ||  $(TRUE)

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
