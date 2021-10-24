# DPS/8M simulator: src/decNumber/GNUmakefile
# vim: filetype=make:tabstop=4:tw=76
#
###############################################################################
#
# Copyright (c) 2014-2016 Charles Anthony
# Copyright (c) 2016 Michal Tomek
# Copyright (c) 2021 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2021 The DPS8M Development Team
#
# All rights reserved.
#
# This software is made available under the terms of the ICU
# License, version 1.8.1 or later.  For more details, see the
# LICENSE.md file at the top-level directory of this distribution.
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
CFLAGS += -Wall -Wextra -Wno-uninitialized  -Wno-stringop-overflow            \
          -Wpedantic -Wno-stringop-overread -Wno-unknown-warning-option
endif
endif

###############################################################################

.PHONY: all decNumber.a
all decNumber.a: $(OBJS)

###############################################################################

.PHONY: clean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean
endif
clean:
	@$(PRINTF) '%s\n' "CLEAN: decNumber"
	-@$(SETV); $(RMF) $(OBJS) "decNumber.a"  ||  $(TRUE) > /dev/null 2>&1

###############################################################################

.PHONY: dependents
$(OBJS): decContext.h  decimal128.h  decNumber.h  decQuad.h                   \
         decDouble.h   decDPD.h      decimal32.h  decNumberLocal.h            \
		 decSingle.h   decimal64.h   decPacked.h

###############################################################################

.SECONDARY:

###############################################################################

decContext.o: decContext.h decimal128.h decNumber.h decQuad.h decDouble.h
decContext.o: decDPD.h decimal32.h decNumberLocal.h decSingle.h decimal64.h
decContext.o: decPacked.h
decNumber.o:  decContext.h decimal128.h decNumber.h decQuad.h decDouble.h
decNumber.o:  decDPD.h decimal32.h decNumberLocal.h decSingle.h decimal64.h
decNumber.o:  decPacked.h

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
