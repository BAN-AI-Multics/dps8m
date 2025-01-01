# DPS8M simulator: src/blinkenLights2/GNUmakefile
# vim: filetype=make:tabstop=4:ai:cc=79:noexpandtab:list:listchars=tab\:\>\-
# SPDX-License-Identifier: BSD-2-Clause
# scspell-id: 062647fc-bf02-11ef-bfc1-80ee73e9b8e7

###############################################################################
#
# Copyright (c) 2021-2025 The DPS8M Development Team
#
# This software is made available under the terms of the ICU License.
# See the LICENSE.md file at the top-level directory of this distribution.
#
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
all: blinkenLights2$(EXE)
	-@$(TRUE)

###############################################################################

blinkenLights2$(EXE): blinkenLights2.c
	@pkg-config gtk+-3.0 gdk-3.0 --cflags --libs > /dev/null 2>&1 ||          \
     { $(PRINTF) '%s\n'                                                       \
        "FAILED: pkg-config gtk+-3.0 gdk-3.0 --cflags --libs!";               \
       exit 1; }
	@$(SETV); env CI_SKIP_MKREBUILD=1 $(MAKE) -C "../.." libsir
	@$(SETV); env CI_SKIP_MKREBUILD=1 $(MAKE) -C "../dps8" shm.o
	@$(PRINTF) '%s\n' "CC: blinkenLights2"
	-@$(SETV); $(CC) $(CFLAGS)                                                \
     $$(pkg-config gtk+-3.0 gdk-3.0 --cflags --libs)                          \
      -DLOCKLESS -DM_SHARED                                                   \
       "../dps8/shm.o" ../libsir/build/obj/src/*.o                            \
        -o blinkenLights2$(EXE) blinkenLights2.c                              \
         $(LDFLAGS) $(MATHLIB) $(DUMALIBS)

###############################################################################

.PHONY: clean distclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean distclean
endif
clean distclean:
	@$(PRINTF) '%s\n' "CLEAN: blinkenLights2"
	-@$(SETV); $(RMF) "blinkenLights2$(EXE)" > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "blinkenLights2.exe"   > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) "blinkenLights2.o"     > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) ./*.gcda               > /dev/null 2>&1  ||  $(TRUE)
	-@$(SETV); $(RMF) ./*.ln                 > /dev/null 2>&1  ||  $(TRUE)

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
