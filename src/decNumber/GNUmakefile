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
# LICENSE file at the top-level directory of this distribution.
#
###############################################################################

include ../Makefile.mk

###############################################################################

C_SRCS = decContext.c decNumber.c

###############################################################################

OBJS = decContext.o decNumber.o

###############################################################################

CFLAGS += -Wall -Wextra -Wno-uninitialized

###############################################################################

.PHONY: all
decNumber.a all: $(OBJS)
	@$(PRINTF) '%s\n' "AR: $@"
	@$(SETV); $(AR) cr $@ $?

###############################################################################

.PHONY: clean
clean:
	@$(PRINTF) '%s\n' "CLEAN: decNumber"
	-@$(SETV); $(RMF) $(OBJS) "decNumber.a" || $(TRUE)

###############################################################################

.PHONY: dependents
$(OBJS): decContext.h  decimal128.h  decNumber.h  decQuad.h \
         decDouble.h   decDPD.h      decimal32.h  decNumberLocal.h \
		 decSingle.h   decimal64.h   decPacked.h

###############################################################################

.SECONDARY:

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
