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
	$(PP) < TestFaultKernel.as8 > TestFaultKernel.pp
	$(AS8) -o TestFaultKernel.oct TestFaultKernel.pp

###############################################################################

TestFaultSeg1.oct: TestFaultSeg1.as8
	$(PP) < TestFaultSeg1.as8 > TestFaultSeg1.pp
	$(AS8) -o TestFaultSeg1.oct TestFaultSeg1.pp

###############################################################################

TestFaultSeg2.oct: TestFaultSeg2.as8
	$(PP) < TestFaultSeg2.as8 > TestFaultSeg2.pp
	$(AS8) -o TestFaultSeg2.oct TestFaultSeg2.pp

###############################################################################

.PHONY: TestIT.test
TestIT.oct TestIT.test: TestIT.as8
	$(PP) < TestIT.as8 > TestIT.pp
	$(AS8) -o TestIT.oct TestIT.pp

###############################################################################
