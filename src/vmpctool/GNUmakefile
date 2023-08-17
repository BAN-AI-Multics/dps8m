# DPS8M simulator: src/vmpctool/GNUmakefile
# vim: filetype=make:tabstop=4:ai:cc=79:noexpandtab:list:listchars=tab\:\>\-
# SPDX-License-Identifier: MIT
# scspell-id: a704c048-f631-11ec-88ba-80ee73e9b8e7
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
ifneq (,$(findstring CYGWIN,$(UNAME_S)))
  SKIP_COMPILATION=1
endif # Cygwin
ifneq (,$(findstring cygwin,$(CC)))
  SKIP_COMPILATION=1
endif # Cygwin cross
ifneq ($(SKIP_COMPILATION),1)
all: vmpctool$(EXE)
	-@$(TRUE)
else # !SKIP_COMPILATION
all:
	-@$(TRUE)
endif # SKIP_COMPILATION

###############################################################################

vmpctool$(EXE): vmpctool.c
	@$(PRINTF) '%s\n' "CC: vmpctool"
	@$(SETV); $(CC) $(CFLAGS)                                                 \
      -o vmpctool$(EXE) vmpctool.c $(LDFLAGS) $(MATHLIB) $(DUMALIBS)

###############################################################################

.PHONY: clean distclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean distclean
endif
clean distclean:
	@$(PRINTF) '%s\n' "CLEAN: vmpctool"
	-@$(SETV); $(RMF) "vmpctool$(EXE)" > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "vmpctool.exe"   > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "vmpctool.o"     > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) ./*.ln           > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) ./*.gcda      > /dev/null 2>&1  ||  $(TRUE)

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
