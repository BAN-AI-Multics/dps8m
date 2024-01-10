# DPS8M simulator: src/dps8/UnitTests/GNUMakefile
# vim: filetype=make:tabstop=4:ai:cc=79:noexpandtab:list:listchars=tab\:\>\-
# SPDX-License-Identifier: MIT-0
# scspell-id: 2734923a-f630-11ec-af39-80ee73e9b8e7
# Copyright (c) 2018-2024 The DPS8M Development Team

###############################################################################

PP  = as8pp
AS8 = as8+

###############################################################################

.PHONY: all
.NOTPARALLEL: all
all: TestIT.test

###############################################################################

.PHONY: TestFault.test
.NOTPARALLEL: TestFault.test
TestFault.test: TestFaultKernel.oct TestFaultSeg1.oct TestFaultSeg2.oct

###############################################################################

.NOTPARALLEL: TestFaultKernel.oct
TestFaultKernel.oct: TestFaultKernel.as8
	$(PP)  <  TestFaultKernel.as8 > TestFaultKernel.pp
	$(AS8) -o TestFaultKernel.oct   TestFaultKernel.pp

###############################################################################

.NOTPARALLEL: TestFaultSeg1.oct
TestFaultSeg1.oct: TestFaultSeg1.as8
	$(PP)   < TestFaultSeg1.as8 > TestFaultSeg1.pp
	$(AS8) -o TestFaultSeg1.oct   TestFaultSeg1.pp

###############################################################################

.NOTPARALLEL: TestFaultSeg2.oct
TestFaultSeg2.oct: TestFaultSeg2.as8
	$(PP)   < TestFaultSeg2.as8 > TestFaultSeg2.pp
	$(AS8) -o TestFaultSeg2.oct   TestFaultSeg2.pp

###############################################################################

.PHONY: TestIT.test TestIT.oct
.NOTPARALLEL: TestIT.oct TestIT.test
TestIT.oct TestIT.test: TestIT.as8
	$(PP)   < TestIT.as8 > TestIT.pp
	$(AS8) -o TestIT.oct   TestIT.pp

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
