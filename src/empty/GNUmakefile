# DPS8M simulator: src/empty/GNUmakefile
# vim: filetype=make:tabstop=4:tw=79:noexpandtab
# SPDX-License-Identifier: BSD-3-Clause
# scspell-id: 9653fd78-f630-11ec-9e48-80ee73e9b8e7

###############################################################################
#
# Copyright (c) 2005-2015 Mikhail E. Zakharov
# Copyright (c) 2022 The DPS8M Development Team
#
# empty was written by Mikhail E. Zakharov. This software was based
# on the basic idea of pty version 4.0, Copyright (c) 1992 Daniel J.
# Bernstein, but no code was ported from pty4.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  1. Redistributions of source code must retain the above copyright
#     notice immediately at the beginning of the file without
#     modification, this list of conditions, and the following
#     disclaimer.
#
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions, and the following disclaimer
#     in the documentation and/or other materials provided with the
#     distribution.
#
#  3. The name of the author may not be used to endorse or promote
#     products derived from this software without specific prior
#     written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#
###############################################################################

.DEFAULT_GOAL := all

###############################################################################

ifneq (,$(wildcard ../Makefile.mk))
  include ../Makefile.mk
endif

###############################################################################

ifneq "$(findstring icc,$(CC))" ""
  CFLAGS += -diag-disable=11074,11076,10148
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

ifneq ($(UNAME_S),AIX)
  ifneq ($(UNAME_S),SunOS)
    EMPTYLIBS ?= -lutil
  endif
endif

###############################################################################

empty$(EXE): empty.c ../dpsprintf/dpsprintf.o
	@$(PRINTF) '%s\n' "CC: empty"
	@$(SETV); $(CC) $(CFLAGS) $(LDFLAGS) "../dpsprintf/dpsprintf.o"           \
      -o empty$(EXE) empty.c $(EMPTYLIBS) $(MATHLIB) $(DUMALIBS)

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
	-@$(SETV); $(RMF) "../dpsprintf/dpsprintf.o"  > /dev/null 2>&1  ||  $(TRUE)

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
