# DPS8M documentation: docs/GNUmakefile
# vim: filetype=make:tabstop=4:tw=78:noexpandtab
# SPDX-License-Identifier: ICU
# scspell-id: 09779b94-f632-11ec-8a12-80ee73e9b8e7

##############################################################################
#
# Copyright (c) 2022 The DPS8M Development Team
#
# All rights reserved.
#
# This software is made available under the terms of the ICU
# License, version 1.8.1 or later.  For more details, see the
# LICENSE.md file at the top-level directory of this distribution.
#
##############################################################################

.DEFAULT_GOAL := all

##############################################################################

.PHONY: docs all
.NOTPARALLEL: docs all
docs all:
	@./builddoc.sh

##############################################################################

.PHONY: clean distclean superclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean distclean superclean
endif
clean distclean superclean:
	@rm -f  ./temp1.tmp           > /dev/null  2>&1 || true
	@rm -f  ./temp2.tmp           > /dev/null  2>&1 || true
	@rm -f  ./*.state             > /dev/null  2>&1 || true
	@rm -f  ./.*.state            > /dev/null  2>&1 || true
	@rm -f  ./yaml/docinfo.yml    > /dev/null  2>&1 || true
	@rm -f  ./*.pdf               > /dev/null  2>&1 || true
	@rm -f  ./*.tmp               > /dev/null  2>&1 || true
	@rm -f  ./complete.out        > /dev/null  2>&1 || true
	@rm -f  ./commandref.md       > /dev/null  2>&1 || true

##############################################################################

.PHONY: dep depend
ifneq (,$(findstring dep,$(MAKECMDGOALS)))
.NOTPARALLEL: dep depend
endif
dep depend:
	@true

##############################################################################

.PHONY: strip
strip:
	@true

##############################################################################

.PHONY: cleandep depclean cleandepend dependclean
cleandep depclean cleandepend dependclean:
	@true

##############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
