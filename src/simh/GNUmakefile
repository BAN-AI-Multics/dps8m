# DPS8M simulator: src/simh/GNUmakefile
# vim: filetype=make:tabstop=4:ai:cc=79:noexpandtab:list:listchars=tab\:\>\-
# SPDX-License-Identifier: MIT-0
# scspell-id: 5f6d7a56-f62a-11ec-b245-80ee73e9b8e7
# Copyright (c) 2016 Charles Anthony
# Copyright (c) 2021 Jeffrey H. Johnson
# Copyright (c) 2021-2025 The DPS8M Development Team

###############################################################################

.DEFAULT_GOAL := all

###############################################################################

ifneq (,$(wildcard ../Makefile.mk))
  include ../Makefile.mk
endif

###############################################################################

ifneq (,$(wildcard ../Makefile.var))
  include ../Makefile.var
endif

###############################################################################

ifndef SUNPRO
  ifndef SUNLINT
    ifneq ($(W),)
      CFLAGS += -Wno-array-bounds -Wno-pointer-bool-conversion                \
                -Wno-unknown-warning-option -Wno-unevaluated-expression
    endif
  endif
endif

###############################################################################

ifdef VER_CURRENT_TIME
  CFLAGS += -DVER_CURRENT_TIME="\"$(VER_CURRENT_TIME)\""
endif

###############################################################################

ifdef BUILD_PROM_OSV_TEXT
  CFLAGS += -DBUILD_PROM_OSV_TEXT="\"$(BUILD_PROM_OSV_TEXT)\""
endif

###############################################################################

ifdef BUILD_PROM_OSA_TEXT
  CFLAGS += -DBUILD_PROM_OSA_TEXT="\"$(BUILD_PROM_OSA_TEXT)\""
endif

###############################################################################

ifdef BUILD_BY_USER
  CFLAGS += -DBUILD_BY_USER="\"$(BUILD_BY_USER)\""
endif

###############################################################################

C_SRCS = scp.c       sim_console.c  sim_disk.c   sim_fio.c   sim_hints.c      \
         sim_sock.c  sim_tape.c     sim_timer.c  sim_tmxr.c  linehistory.c

###############################################################################

OBJS = scp.o       sim_console.o  sim_disk.o   sim_fio.o   sim_hints.o        \
       sim_sock.o  sim_tape.o     sim_timer.o  sim_tmxr.o  linehistory.o

###############################################################################

.PHONY: all simh.a
simh.a all: ../dps8/ver.h $(OBJS)
	-@$(TRUE)

###############################################################################

../dps8/ver.h:
	@($(SETV); $(CD) "../dps8" && $(MAKE) "ver.h")

###############################################################################

.PHONY: clean distclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean distclean
endif
clean distclean:
	@$(PRINTF) '%s\n' "CLEAN: simh"
	-@$(SETV); $(RMF) $(OBJS)              ||  $(TRUE) > /dev/null 2>&1
	-@$(SETV); $(RMF) "simh.a"             ||  $(TRUE) > /dev/null 2>&1
	-@$(SETV); $(RMF) "tags"               ||  $(TRUE) > /dev/null 2>&1
	-@$(RMF) "dps8m.state"                 ||  $(TRUE) > /dev/null 2>&1
	-@$(RMF) dps8m*.state                  ||  $(TRUE) > /dev/null 2>&1
	-@$(RMF) "l68.state"                   ||  $(TRUE) > /dev/null 2>&1
	-@$(RMF) l68*.state                    ||  $(TRUE) > /dev/null 2>&1
	-@$(RMF) ".dps8m.state"                ||  $(TRUE) > /dev/null 2>&1
	-@$(RMF) ".l68.state"                  ||  $(TRUE) > /dev/null 2>&1
	-@$(RMF) .dps8m*.state                 ||  $(TRUE) > /dev/null 2>&1
	-@$(RMF) .l68*.state                   ||  $(TRUE) > /dev/null 2>&1
	-@$(RMF) ./*.gcda 2> /dev/null         ||  $(TRUE) > /dev/null 2>&1
	-@$(RMF) ./*.ln   2> /dev/null         ||  $(TRUE) > /dev/null 2>&1

###############################################################################

.PHONY: dependents
.SECONDARY:

###############################################################################

ifdef CFLAGS
  export CFLAGS
  ifneq (,$(wildcard ../Makefile.mk))
    include ../Makefile.mk
  endif
endif

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
