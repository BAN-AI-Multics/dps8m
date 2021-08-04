# DPS/8M simulator: src/GNUmakefile
# vim: filetype=make:tabstop=4:tw=76
#
###############################################################################
#
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

###############################################################################
#
# This is a fallback and wraper for the case of the user executing "gmake" or
# "make" from within the `src` directory, rather than from one of the source
# subdirectories or the top-level of the source tree, and also works around a
# very specific FreeBSD-related CI/CD pipeline corner case in a harmless way.
#
###############################################################################

###############################################################################
# BSD Make fallback (BSDmake parsing GNUmakefile)

.PHONY: all
.DEFAULT: all
all:
	@$(MAKE) -C ".." $@

###############################################################################
# GNU Make fallback

.PHONY: build default all %
%:
	@$(MAKE) -C ".." $@

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
