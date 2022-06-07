# DPS/8M simulator: src/simh/GNUmakefile
# vim: filetype=make:tabstop=4:tw=76:noexpandtab

###############################################################################
#
# Copyright (c) 2016 Charles Anthony
# Copyright (c) 2021 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2021-2022 The DPS8M Development Team
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

C_SRCS = scp.c       sim_console.c  sim_disk.c   sim_fio.c   dispatch.c       \
         sim_sock.c  sim_tape.c     sim_timer.c  sim_tmxr.c  linehistory.c

###############################################################################

OBJS = scp.o       sim_console.o  sim_disk.o   sim_fio.o   dispatch.o         \
       sim_sock.o  sim_tape.o     sim_timer.o  sim_tmxr.o  linehistory.o

###############################################################################

.PHONY: all simh.a
simh.a all: ../dps8/ver.h ../dps8/sysdefs.h $(OBJS)
	-@$(TRUE)

###############################################################################

../dps8/sysdefs.h:
	@($(SETV); $(CD) "../dps8" && $(MAKE) "sysdefs.h")

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
	-@$(RMF) "../dpsprintf/dpsprintf.o"    ||  $(TRUE) > /dev/null 2>&1
	-@$(SETV); $(RMF) "../dps8/sysdefs.n"  ||  $(TRUE) > /dev/null 2>&1

###############################################################################

.PHONY: dependents
.SECONDARY:

###############################################################################

scp.o:         ../dpsprintf/dpsprintf.h
scp.o:         sim_defs.h
scp.o:         sim_timer.h
scp.o:         sim_disk.h sim_tape.h sim_sock.h
scp.o:         sim_tmxr.h
scp.o:         ../dps8/dps8_duma.h
scp.o:         ../dps8/ver.h
scp.o:         ../dps8/dps8_prom.h
scp.o:         ../dps8/sysdefs.h
scp.o:         ../decNumber/decContext.h
scp.o:         ../decNumber/decNumberLocal.h
scp.o:         ../dps8/dps8_iom.h
scp.o:         ../dps8/dps8_fnp2.h
scp.o:         linehistory.h
scp.o:         dispatch.h
linehistory.o: linehistory.h ../dpsprintf/dpsprintf.h
dispatch.o:    dispatch.h
sim_console.o: sim_defs.h ../dpsprintf/dpsprintf.h
sim_console.o: scp.h sim_console.h sim_timer.h
sim_console.o: sim_fio.h
sim_console.o: sim_tmxr.h sim_sock.h
sim_disk.o:    sim_defs.h ../dpsprintf/dpsprintf.h
sim_disk.o:    scp.h
sim_disk.o:    sim_console.h sim_timer.h
sim_disk.o:    sim_disk.h
sim_disk.o:    sim_sock.h
sim_fio.o:     sim_defs.h ../dpsprintf/dpsprintf.h
sim_fio.o:     scp.h
sim_fio.o:     sim_console.h sim_timer.h
sim_fio.o:     sim_fio.h
sim_fio.o:     ../decNumber/decContext.h
sim_fio.o:     ../decNumber/decNumberLocal.h
sim_sock.o:    sim_sock.h ../dpsprintf/dpsprintf.h
sim_tape.o:    sim_defs.h
sim_tape.o:    scp.h
sim_tape.o:    sim_console.h sim_timer.h
sim_tape.o:    sim_tape.h
sim_timer.o:   sim_defs.h ../dpsprintf/dpsprintf.h
sim_timer.o:   scp.h
sim_timer.o:   sim_console.h sim_timer.h
sim_timer.o:   sim_fio.h
sim_tmxr.o:    sim_defs.h ../dpsprintf/dpsprintf.h
sim_tmxr.o:    scp.h
sim_tmxr.o:    sim_console.h sim_timer.h
sim_tmxr.o:    sim_fio.h
sim_tmxr.o:    sim_sock.h

###############################################################################

ifdef CFLAGS
  export CFLAGS
  ifneq (,$(wildcard ../Makefile.mk))
    include ../Makefile.mk
  endif
endif

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
