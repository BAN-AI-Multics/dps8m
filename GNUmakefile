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
# LICENSE.md file at the top-level directory of this distribution.
#
###############################################################################

###############################################################################
# Configuration:
#
#     Build flag (ex: make V=1)           Description of build flag
#    ###########################    #####################################
#
#                  V=1                Enable verbose compilation output
#                  W=1                Enable extra compilation warnings
#            TESTING=1                Enable developmental testing code
#        NO_LOCKLESS=1                Revert to previous threading code
#              CROSS=MINGW64          Enable MinGW-64 cross-compilation
#
#    ******* The following flags are intended for development and *******
#    ******* may have non-intuitive side-effects or requirements! *******
#
#            TRACKER=1                Adds instruction snapshot support
#               HDBG=1                Enable hardware debugging support
#        ROUND_ROBIN=1                Support un-threaded multiple CPUs
#             ISOLTS=1                Support for running ISOLOTS tests
#                WAM=1                Enable PTW/SDW associative memory
#           NEED_128=1                Enable 128-bit types work-arounds
#        USE_BUILDER="String"         Enable a custom "Built by" string
#        USE_BUILDOS="String"         Enable a custom "Built OS" string
#
###############################################################################
# Makefile tracing.

ifdef MAKETRACE
  _SHELL := $(SHELL)
  SHELL = $(info [TRACE] GNUmakefile: [$@])$(_SHELL)
endif

###############################################################################
# Pre-build exceptions.

include src/Makefile.pre

ifdef OS_IBMAIX
    export OS_IBMAIX
endif

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
# Variable debugging.

print-% : ; $(info top: $* is a $(flavor $*) variable set to [$($*)]) @true

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
