# DPS/8M simulator: src/dpsprintf/GNUmakefile
# vim: filetype=make:tabstop=4:tw=78:noexpandtab

###############################################################################
#
# Copyright (c) 2015 Jeff Roberts
# Copyright (c) 2017 Sean Barrett
# Copyright (c) 2020 Peter Miller
# Copyright (c) 2020-2021 Jeffrey H. Johnson <trnsz@pobox.com>
# Copyright (c) 2021-2022 The DPS8M Development Team
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
#  * The above copyright notice and this permission notice shall be
#    included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
    ifeq "$(findstring icc,$(CC))" ""
      CFLAGS += -Wno-uninitialized -Wno-stringop-overflow                     \
                -Wno-stringop-overread -Wno-literal-range                     \
                -Wno-unknown-warning-option -Wno-overflow
    else
      CFLAGS += -diag-disable=11074,11076,10148
    endif
  endif
endif

###############################################################################

.PHONY: all
all dpsprintf: dpsprintf.o
	-@$(TRUE)

###############################################################################

.PHONY: clean distclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean distclean
endif
clean distclean:
	@$(PRINTF) '%s\n' "CLEAN: dpsprintf"
	-@$(SETV); $(RMF) "dpsprintf.o" > /dev/null 2>&1  ||  $(TRUE)

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
