# DPS/8M simulator: src/simh/GNUmakefile
# vim: filetype=make:tabstop=4:tw=76
#
###############################################################################
#
# Copyright (c) 2016 Charles Anthony
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

include ../Makefile.mk

###############################################################################

ifneq ($(W),)
CFLAGS += -Wno-array-bounds -Wno-pointer-bool-conversion \
          -Wno-unknown-warning-option
endif

ifdef VER_CURRENT_TIME
CFLAGS += -DVER_CURRENT_TIME="\"$(VER_CURRENT_TIME)\""
endif

###############################################################################

C_SRCS = scp.c sim_console.c sim_disk.c sim_ether.c sim_fio.c sim_serial.c \
         sim_sock.c sim_tape.c sim_timer.c sim_tmxr.c sim_video.c

###############################################################################

OBJS = scp.o sim_console.o sim_disk.o sim_ether.o sim_fio.o sim_serial.o \
       sim_sock.o sim_tape.o sim_timer.o sim_tmxr.o sim_video.o

###############################################################################

ifneq ($(W),)
CFLAGS += -Wno-array-bounds -Wno-unevaluated-expression
endif

###############################################################################

.PHONY: all
simh.a all: $(OBJS)
	@$(PRINTF) '%s\n' "AR: $@"
	@$(SETV); $(AR) cr $@ $?

###############################################################################

.PHONY: clean
clean:
	@$(PRINTF) '%s\n' "CLEAN: simh"
	-@$(SETV); $(RMF) $(OBJS) "simh.a" "tags" || $(TRUE)

###############################################################################

.PHONY: dependents
.SECONDARY:

###############################################################################

# Loccal Variables:
# mode: make
# tab-width: 4
# End:
