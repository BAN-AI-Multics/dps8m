# DPS/8M simulator: src/dps8/UnitTests/GNUMakefile
# vim: filetype=make:tabstop=4:tw=76
#
###############################################################################
#
# Copyright (c) 2018-2021 The DPS8M Development Team
#
# All rights reserved.
#
# This software is made available under the terms of the ICU
# License, version 1.8.1 or later.  For more details, see the
# LICENSE.md file at the top-level directory of this distribution.
#
###############################################################################

ifdef MAKETRACE
  _SHELL := $(SHELL)
  SHELL = $(info [TRACE] src/dps8/UnitTests/GNUmakefile: [$@])$(_SHELL)
endif

###############################################################################

 PP=as8pp
AS8=as8+

###############################################################################

.PHONY: all
all: TestIT.test

###############################################################################

.PHONY: TestFault.test
TestFault.test: TestFaultKernel.oct TestFaultSeg1.oct TestFaultSeg2.oct

###############################################################################

TestFaultKernel.oct: TestFaultKernel.as8
	$(PP)  <  TestFaultKernel.as8 > TestFaultKernel.pp
	$(AS8) -o TestFaultKernel.oct   TestFaultKernel.pp

###############################################################################

TestFaultSeg1.oct: TestFaultSeg1.as8
	$(PP)   < TestFaultSeg1.as8 > TestFaultSeg1.pp
	$(AS8) -o TestFaultSeg1.oct   TestFaultSeg1.pp

###############################################################################

TestFaultSeg2.oct: TestFaultSeg2.as8
	$(PP)   < TestFaultSeg2.as8 > TestFaultSeg2.pp
	$(AS8) -o TestFaultSeg2.oct   TestFaultSeg2.pp

###############################################################################

.PHONY: TestIT.test
TestIT.oct TestIT.test: TestIT.as8
	$(PP)   < TestIT.as8 > TestIT.pp
	$(AS8) -o TestIT.oct   TestIT.pp

###############################################################################

print-% : ; $(info local: $* is a $(flavor $*) variable set to [$($*)]) @true

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
