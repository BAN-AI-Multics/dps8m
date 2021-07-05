# DPS/8M simulator: GNUmakefile
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
# LICENSE file at the top-level directory of this distribution.
#
###############################################################################

###############################################################################
# Configuration:
#
#        V=1       Enable VERBOSE mode for compilation
#        W=1       Enable extra warnings (-Wall) 
#
###############################################################################
# Build

.PHONY: build default all
build default all:
	@$(MAKE) -C "src/dps8"

###############################################################################
# Install

.PHONY: install
install:
	@$(MAKE) -C "src/dps8" "install"

###############################################################################
# Clean up compiled objects and executables.

.PHONY: clean
.NOTPARALLEL: clean
clean:
	@$(MAKE) -C "src/dps8" "clean"

###############################################################################
# Cleans everything `clean` does, plus version info, logs, and state files.

.PHONY: distclean
distclean: clean
	@$(MAKE) -C "src/dps8" "distclean"

###############################################################################
# Cleans everything `distclean` does, plus attempts to flush compiler caches.

.PHONY: superclean realclean reallyclean
superclean realclean reallyclean: distclean
	@$(MAKE) -C "src/dps8" "superclean"

###############################################################################
# List files that are not known to git.

.PHONY: printuk
printuk:
	@$(MAKE) -C "src/dps8" "printuk"

###############################################################################
# List files known to git that have been modified.

.PHONY: printmod
printmod:
	@$(MAKE) -C "src/dps8" "printmod"

###############################################################################
# Create a distribution archive kit.

.PHONY: kit dist
kit dist:
	@$(MAKE) -C "src/dps8" "kit"

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
