# DPS8M simulator: src/unifdef/GNUmakefile
# vim: filetype=make:tabstop=4:ai:cc=79:noexpandtab:list:listchars=tab\:\>\-
# SPDX-License-Identifier: BSD-2-Clause
# scspell-id: 8122e0e8-f631-11ec-8c75-80ee73e9b8e7

###############################################################################
#
# Copyright (c) 2021 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2021-2023 The DPS8M Development Team
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
###############################################################################

###############################################################################
#
# This Software ("Unifdef") is distributed under a two-clause BSD copyright
# license. Unifdef was derived from software contributed to U.C. Berkeley by
# Dave Yost. It was re-written to support ANSI C by Tony Finch. The original
# version of Unifdef carried a four-clause BSD copyright license. None of its
# code remains in this version - though some of the names remain - so it now
# carries the more liberal two-clause BSD license.
#
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

.PHONY: silent silently
silent silently:
	-@$(MAKE) --no-print-directory -s "all" > /dev/null 2>&1 || $(MAKE) "all"

###############################################################################

.PHONY: all
all: unifdef$(EXE)
	-@$(TRUE)

###############################################################################

unifdef$(EXE): unifdef.o unifdef.h version.h unifdef.c
	@$(PRINTF) '%s\n' "LD: unifdef"
	@$(SETV); $(CC) $(CFLAGS)                                                 \
      -o unifdef$(EXE) unifdef.o $(LDFLAGS) $(MATHLIB) $(DUMALIBS)

###############################################################################

.PHONY: clean distclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean distclean
endif
clean distclean:
	@$(PRINTF) '%s\n' "CLEAN: unifdef"
	-@$(SETV); $(RMF) "unifdef$(EXE)" > /dev/null 2>&1 || $(TRUE)
	-@$(SETV); $(RMF) "unifdef.exe"   > /dev/null 2>&1 || $(TRUE)
	-@$(SETV); $(RMF) "unifdef.o"     > /dev/null 2>&1 || $(TRUE)
	-@$(SETV); $(RMF) ./*.gcda        > /dev/null 2>&1 || $(TRUE)
	-@$(SETV); $(RMF) ./*.ln          > /dev/null 2>&1 || $(TRUE)

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
