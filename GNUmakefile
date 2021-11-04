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
#     Build flag (ex: make V=1)             Description of build flag
#    ###########################    #########################################
#
#                  V=1                  Enable verbose compilation output
#                  W=1                  Enable extra compilation warnings
#            TESTING=1                  Enable developmental testing code
#        NO_LOCKLESS=1                  Enable (old) non-threaded MP mode
#                L68=1                  Build as H6180/Level-68 simulator
#              CROSS=MINGW64            Enable MinGW-64 cross-compilation
#
#    ********* The following flags are intended for development and *********
#    ********* may have non-intuitive side-effects or requirements! *********
#
#            TRACKER=1                  Adds instruction snapshot support
#               HDBG=1                  Enables extended history debugger
#        ROUND_ROBIN=1                  Support un-threaded multiple CPUs
#             ISOLTS=1                  Support execution of ISOLTS tests
#                WAM=1                  Enable PTW/SDW associative memory
#           NEED_128=1                  Enable 128-bit types work-arounds
#        USE_BUILDER="String"           Enable a custom "Built by" string
#        USE_BUILDOS="String"           Enable a custom "Build OS" string
#            ATOMICS=AIX|BSD|GNU|SYNC   Define specific atomic operations
#
###############################################################################

.DEFAULT_GOAL := all

###############################################################################
# Pre-build exceptions.

MAKE_TOPLEVEL = 1

ifneq (,$(wildcard src/Makefile.pre))
  include src/Makefile.pre
  ifdef OS_IBMAIX
    export OS_IBMAIX
  endif
endif

ifneq (,$(wildcard src/Makefile.mk))
  include src/Makefile.mk
endif

export MAKE_TOPLEVEL

###############################################################################
# DPS8M Simulator
    # XXXX:    # ---------------------- DPS8/M Simulator --------------------
###############################################################################

###############################################################################
# Build.

.PHONY: build default all
build default all:                                                            \
    # build:    # Builds the DPS8/M simulator and tools
	@$(MAKE) -C "src/dps8" "all"

###############################################################################
# Install.

.PHONY: install
install:                                                                      \
    # install:    # Builds and installs the sim and tools
	@$(MAKE) -C "src/dps8" "install"

###############################################################################
# Clean up compiled objects and executables.

.PHONY: clean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
$(warn blah)
.NOTPARALLEL: clean
endif
clean:                                                                        \
    # clean:    # Cleans up executable and object files
	@$(MAKE) -C "src/dps8" "clean"

###############################################################################
# Cleans everything `clean` does, plus version info, logs, and state files.

.PHONY: distclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: distclean
endif
distclean: clean                                                              \
    # distclean:    # Cleans up tree to pristine conditions
	@$(MAKE) -C "src/dps8" "distclean"

###############################################################################
# Cleans everything `distclean` does, plus attempts to flush compiler caches.

.PHONY: superclean realclean reallyclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: superclean realclean reallyclean
endif
superclean realclean reallyclean: distclean                                   \
    # superclean:    # Cleans up tree fully and flush ccache
	@$(MAKE) -C "src/dps8" "superclean"

###############################################################################

ifneq (,$(wildcard src/Makefile.env))
  include src/Makefile.env
endif

###############################################################################
# Help and Debugging Targets                                                  \
    # XXXX:    # --------------------- Help and Debugging -------------------
###############################################################################

ifneq (,$(wildcard src/Makefile.scc))
  include src/Makefile.scc
endif

###############################################################################

ifneq (,$(wildcard src/Makefile.dev))
  include src/Makefile.dev
endif

###############################################################################

ifneq (,$(wildcard src/Makefile.dep))
  include src/Makefile.dep
endif

###############################################################################

.PHONY: help info
help info:                                                                    \
    # help:    # Display this list of Makefile targets
	@$(GREP) -E '^.* # .*:    # .*$$' $(MAKEFILE_LIST) 2> /dev/null         | \
		$(AWK) 'BEGIN { FS = "    # " };                                      \
          { printf "%s%-18s %-40s (%-19s)\n", $$1, $$2, $$3, $$1 }'           \
		    2> /dev/null | $(CUT) -d ':' -f 2- 2> /dev/null                 | \
              $(SED) -e 's/:\ \+)$$/)/' -e 's/XXXX:/\n/g' 2> /dev/null      | \
                $(SED) -e 's/---- (.*)$$/------------\n/'                     \
				 -e 's/:  )/)/g' -e 's/:       )/)/g'                         \
                  -e 's/              ---/-------------/' 2> /dev/null      | \
                    $(GREP) -v 'GREP' 2> /dev/null                          | \
                      $(GREP) -v '_LIST' 2> /dev/null | $(SED) 's/^n//g'    | \
					   $(SED) 's/n$$//g'                                   || \
					    { $(PRINTF) '%s\n' "Error: Unable to display help.";  \
                          $(TRUE) > /dev/null 2>&1; }                      || \
                            $(TRUE) > /dev/null 2>&1
	@$(PRINTF) '%s\n' "" 2> /dev/null || $(TRUE) > /dev/null 2>&1

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
