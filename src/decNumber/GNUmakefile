# DPS8M simulator: src/decNumber/GNUmakefile
# vim: filetype=make:tabstop=4:ai:cc=79:noexpandtab:list:listchars=tab\:\>\-
# SPDX-License-Identifier: ICU
# scspell-id: df455d55-f62c-11ec-8d80-80ee73e9b8e7

###############################################################################
#
# Copyright (c) 2014-2016 Charles Anthony
# Copyright (c) 2016 Michal Tomek
# Copyright (c) 2021 Jeffrey H. Johnson
# Copyright (c) 2021-2024 The DPS8M Development Team
#
# This software is made available under the terms of the ICU License.
# See the LICENSE.md file at the top-level directory of this distribution.
#
###############################################################################

.DEFAULT_GOAL := all

###############################################################################

ifneq (,$(wildcard ../Makefile.mk))
  include ../Makefile.mk
endif

###############################################################################

C_SRCS = decContext.c decNumber.c

###############################################################################

OBJS = decContext.o decNumber.o

###############################################################################

ifndef SUNPRO
  ifndef SUNLINT
    ifeq "$(findstring icc,$(CC))" "" # All compilers except Intel ICC
      CFLAGS += -Wall -Wextra -Wno-uninitialized  -Wno-stringop-overflow      \
                -Wpedantic -Wno-stringop-overread -Wno-unknown-warning-option
    else # Intel ICC
      CFLAGS += -Wall -Wextra -Wpedantic -diag-disable=11074,11076,10148,10441
    endif
  endif
endif

###############################################################################

.PHONY: all decNumber.a
all decNumber.a: $(OBJS)
	-@$(TRUE)

###############################################################################

.PHONY: clean distclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean distclean
endif
clean distclean:
	@$(PRINTF) '%s\n' "CLEAN: decNumber"
	-@$(SETV); $(RMF) $(OBJS) "decNumber.a"       ||  $(TRUE) > /dev/null 2>&1
	-@$(SETV); $(RMF) ./*.gcda > /dev/null 2>&1   ||  $(TRUE)
	-@$(SETV); $(RMF) ./*.ln   > /dev/null 2>&1   ||  $(TRUE)

###############################################################################

.PHONY: dependents
$(OBJS): decContext.h  decimal128.h  decNumber.h  decQuad.h                   \
         decDouble.h   decDPD.h      decimal32.h  decNumberLocal.h            \
         decSingle.h   decimal64.h   decPacked.h

###############################################################################

.SECONDARY:

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
