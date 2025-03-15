# DPS8M simulator: GNUmakefile
# vim: nocp:filetype=make:tabstop=4:ai:noet:cc=78:list:listchars=tab\:\>\-
# SPDX-License-Identifier: MIT-0
# scspell-id: bfdf848e-f631-11ec-9b72-80ee73e9b8e7
# Copyright (c) 2016 Charles Anthony
# Copyright (c) 2021-2024 Jeffrey H. Johnson
# Copyright (c) 2021-2025 The DPS8M Development Team

##############################################################################
# Build flags:
#
# <------------------- Maintain spacing and formatting -------------------->
#
#      Build flag (ex: make V=1)               Description of build flag
#  #########################################################################
#
#           ATOMICS=AIX|BSD|GNU|SYNC       Define specific atomic operations
#             CROSS=MINGW64                Enable MinGW-64 cross-compilation
#              DUMA=1                      Enable the libDUMA malloc library
#            NATIVE=1                      Enable native arch/cpu via CFLAGS
#          NEED_128=1                      Enable provided 128-bit int types
#     NO_C_ELLIPSIS=1                      Disables the C ellipsis extension
#         NO_LOCALE=1                      Disable support for native locale
#       NO_LOCKLESS=1                      Enable legacy (non-lockless) code
#            NO_LTO=1                      Disables the use of LTO for build
#           PANEL68=1                      Enable custom L68 panel interface
#        PERF_STRIP=1                      Enable CPU-only benchmarking mode
#               QNX=1                      Force QNX compilation without qcc
#       ROUND_ROBIN=1                      Enable non-threaded multiple CPUs
#           TESTING=1                      Enable developmental testing mode
#                 V=1                      Enable verbose compilation output
#                 W=1                      Enable extra compilation warnings
#     WITH_ABSI_DEV=1                      Enable (UNSTABLE) ABSI/IMP device
#    WITH_BACKTRACE=1                      Enable building with libbacktrace
#      WITH_MGP_DEV=1                      Enable (UNSTABLE) Chaosnet device
#
# <------------------- Maintain spacing and formatting -------------------->
#
##############################################################################

.DEFAULT_GOAL := all

##############################################################################
# Pre-build setup

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

export MAKE
export MAKE_TOPLEVEL

##############################################################################
# DPS8M Simulator
    # XXXX:    # ---------------------- DPS8/M Simulator --------------------
##############################################################################

##############################################################################
# Builds everything

.PHONY: build default all dps8 .rebuild.env
build default all dps8: .rebuild.env                                         \
    # build:    # Builds the DPS8/M simulator and tools
	-@$(PRINTF) '%s\n' "BUILD: Starting simulator and tools build"           \
      2> /dev/null || $(TRUE)
	@$(SETV); $(MAKE) -s -C "." ".rebuild.env";                              \
      $(TEST) -f ".needrebuild" && $(MAKE) -C "." "clean" || $(TRUE);        \
        $(MAKE) -C "src/dps8" "all" &&                                       \
          $(PRINTF) '%s\n' "BUILD: Successful simulator and tools build"     \
            2> /dev/null || $(TRUE)

##############################################################################
# Print build flags help

.PHONY: flags options .rebuild.env
ifneq (,$(findstring flags,$(MAKECMDGOALS)))
.NOTPARALLEL: flags
endif
ifneq (,$(findstring options,$(MAKECMDGOALS)))
.NOTPARALLEL: options
endif
options:                                                                     \
    # options:    # Display help for optional build flags
	-@$(PRINTF) '%s\n' "Optional build flags:" 2> /dev/null || $(TRUE)
	-@$(PRINTF) '\n     %s\n' "Usage: $(MAKE) { FLAG=VALUE ... }"            \
      2> /dev/null || $(TRUE)
	-@$(PRINTF) '%s\n' "      e.g. \"$(MAKE) TESTING=1 V=1 W=1\""            \
      2> /dev/null || $(TRUE)
	-@$(PRINTF) '\n%s\n'                                                     \
     "      Build flag (ex: make V=1)             Description of build flag" \
        2> /dev/null || $(TRUE)
	-@$(PRINTF) ' %s '                                                       \
      "----------------------------------"                                   \
        " -------------------------------------"                             \
          2> /dev/null || $(TRUE)
	-@$(PRINTF) '\n%s\n' "" 2> /dev/null || $(TRUE)
	@$(GREP) '^#.*=' "GNUmakefile" 2> /dev/null |                            \
      $(GREP) -v 'vim:' 2> /dev/null | $(GREP) -v 'Build flag'               \
       2> /dev/null | $(SED) 's/^#  //' 2> /dev/null ||                      \
		$(PRINTF) '%s\n' "Error: Unable to display help." || $(TRUE)
	-@$(PRINTF) '%s\n' "" 2> /dev/null || $(TRUE)

##############################################################################
# Builds punutil tool

.PHONY: punutil .rebuild.env
punutil: .rebuild.env                                                        \
    # punutil:    # Builds the punch card conversion tool
	-@$(PRINTF) '%s\n' "BUILD: Starting punutil build" 2> /dev/null ||       \
        $(TRUE)
	@$(MAKE) -s -C "." ".rebuild.env";                                       \
      $(TEST) -f ".needrebuild" && $(MAKE) -C "." "clean" || $(TRUE);        \
        $(MAKE) -C "src/punutil" "all" &&                                    \
          $(PRINTF) '%s\n' "BUILD: Successful punutil build" 2> /dev/null || \
            $(TRUE)

##############################################################################
# Builds prt2pdf tool

.PHONY: prt2pdf .rebuild.env
prt2pdf: .rebuild.env                                                        \
    # prt2pdf:    # Builds the prt2pdf PDF rendering tool
	-@$(PRINTF) '%s\n' "BUILD: Starting prt2pdf build" 2> /dev/null ||       \
        $(TRUE)
	@$(MAKE) -s -C "." ".rebuild.env";                                       \
      $(TEST) -f ".needrebuild" && $(MAKE) -C "." "clean" || $(TRUE);        \
        $(MAKE) -C "src/prt2pdf" "all" &&                                    \
          $(PRINTF) '%s\n' "BUILD: Successful prt2pdf build" 2> /dev/null || \
            $(TRUE)

##############################################################################
# Builds mcmb tool

.PHONY: mcmb .rebuild.env
mcmb: .rebuild.env                                                           \
    # mcmb:    # Builds the minicmb combinatorics tool
	-@$(PRINTF) '%s\n' "BUILD: Starting mcmb build" 2> /dev/null || $(TRUE)
	-@$(MAKE) -s -C "." ".rebuild.env";                                      \
      $(TEST) -f ".needrebuild" && $(MAKE) -C "." "clean" || $(TRUE);        \
        $(MAKE) -C "src/mcmb" "all" &&                                       \
          $(PRINTF) '%s\n' "BUILD: Successful mcmb build" 2> /dev/null ||    \
            $(TRUE)

##############################################################################
# Builds libsir

.PHONY: libsir .rebuild.env
libsir: .rebuild.env                                                         \
    # libsir:    # Builds the libsir logging library
	-@$(PRINTF) '%s\n' "BUILD: Starting libsir build" 2> /dev/null ||        \
        $(TRUE)
	-@$(MAKE) -s -C "." ".rebuild.env";                                      \
      $(TEST) -f ".needrebuild" && $(MAKE) -C "." "clean" || $(TRUE);        \
        $(MAKE) -C "src/dps8" "libsir" &&                                    \
          $(PRINTF) '%s\n' "BUILD: Successful libsir build"                  \
            2> /dev/null || $(TRUE)

##############################################################################
# Builds blinkenLights2

.PHONY: blinkenLights2 .rebuild.env
blinkenLights2: .rebuild.env                                                 \
    # blinkenLights2:    # Builds the blinkenLights2 front panel
	-@$(PRINTF) '%s\n' "BUILD: Starting blinkenLights2 build"                \
        2> /dev/null || $(TRUE)
ifeq ($(V),1)
	@$(SETV); $(MAKE) -s -C "." ".rebuild.env";                              \
      $(TEST) -f ".needrebuild" && $(RMF) blinkenLights2 || $(TRUE);         \
        ( cd src/blinkenLights2 && $(ENV) VERBOSE=1                          \
          $(MAKE) ) &&                                                       \
            $(PRINTF) '%s\n' "BUILD: Successful blinkenLights2 build"        \
              2> /dev/null || $(TRUE)
else
	@$(MAKE) -s -C "." ".rebuild.env";                                       \
      $(TEST) -f ".needrebuild" && $(RMF) blinkenLights2 || $(TRUE);         \
        ( cd src/blinkenLights2 && $(MAKE) ) &&                              \
          $(PRINTF) '%s\n' "BUILD: Successful blinkenLights2 build"          \
            2> /dev/null || $(TRUE)
endif

##############################################################################
# Install

.PHONY: install
ifneq (,$(findstring install,$(MAKECMDGOALS)))
.NOTPARALLEL: install
endif
install:                                                                     \
    # install:    # Builds and installs the sim and tools
	-@$(PRINTF) '%s\n' "BUILD: Starting install" || $(TRUE)
	@$(MAKE) -C "src/dps8" "install" &&                                      \
      $(PRINTF) '%s\n' "BUILD: Successful install" || $(TRUE)

##############################################################################
# Clean up compiled objects and executables

.PHONY: clean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: clean
endif
clean:                                                                       \
    # clean:    # Cleans up executable and object files
	-@$(PRINTF) '%s\n' "BUILD: Starting clean" 2> /dev/null || $(TRUE)
	@$(RMF)  ".needrebuild"  ||  $(TRUE)
	@$(RMF)  ".rebuild.vne"  ||  $(TRUE)
	@$(MAKE) -C "src/dps8" "clean" &&                                        \
      $(PRINTF) '%s\n' "BUILD: Successful clean" 2> /dev/null || $(TRUE)

##############################################################################
# Cleans everything `clean` does, plus version info, logs, and state files

.PHONY: distclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: distclean
endif
distclean: clean                                                             \
    # distclean:    # Cleans up tree to pristine conditions
	-@$(PRINTF) '%s\n' "BUILD: Starting distclean" 2> /dev/null || $(TRUE)
	@$(RMF)   ".needrebuild"                   || $(TRUE)
	@$(RMF)   ".rebuild.env"                   || $(TRUE)
	@$(RMF)   ".rebuild.vne"                   || $(TRUE)
	@$(RMF)   -rf "./out"                      || $(TRUE)
	@$(RMF)   "cppcheck.txt"                   || $(TRUE)
	@$(RMF)   "./orstlint.txt"                 || $(TRUE)
	@$(RMF)   -rf "./cppcheck"                 || $(TRUE)
	@$(RMF)   -rf "./pvsreport"                || $(TRUE)
	@$(RMF)   -rf "./cov-int"                  || $(TRUE)
	@$(RMF)   -rf "./.coverity"                || $(TRUE)
	@$(RMF)   -f "./log.pvs"                   || $(TRUE)
	@$(RMF)   -f "./compile_commands.json"     || $(TRUE)
	@$(RMF)   "./"*".xml"                      || $(TRUE)
	@$(RMDIR) "./.cppbdir"  > /dev/null  2>&1  || $(TRUE)
	@$(MAKE)  -C "src/dps8" "distclean" &&                                   \
      $(PRINTF) '%s\n' "BUILD: Successful distclean" 2> /dev/null || $(TRUE)

##############################################################################
# Cleans everything `distclean` does, plus attempts to flush compiler caches

.PHONY: superclean realclean reallyclean
ifneq (,$(findstring clean,$(MAKECMDGOALS)))
.NOTPARALLEL: superclean realclean reallyclean
endif
superclean realclean reallyclean: distclean                                  \
    # superclean:    # Cleans up tree fully and flush ccache
	-@$(PRINTF) '%s\n' "BUILD: Starting superclean" 2> /dev/null || $(TRUE)
	@$(MAKE) -C "src/dps8" "superclean" &&                                   \
      $(PRINTF) '%s\n' "BUILD: Successful superclean" 2> /dev/null || $(TRUE)

##############################################################################

ifneq (,$(wildcard src/Makefile.env))
  include src/Makefile.env
endif

##############################################################################
# Help and Debugging Targets                                                 \
    # XXXX:    # --------------------- Help and Debugging -------------------
##############################################################################

ifneq (,$(wildcard src/Makefile.scc))
  include src/Makefile.scc
endif

##############################################################################

ifneq (,$(wildcard src/Makefile.loc))
  include src/Makefile.loc
endif

##############################################################################

ifneq (,$(wildcard src/Makefile.dev))
  include src/Makefile.dev
endif

##############################################################################

ifneq (,$(wildcard src/Makefile.doc))
  include src/Makefile.doc
endif

##############################################################################

ifneq (,$(wildcard src/Makefile.dep))
  include src/Makefile.dep
endif

##############################################################################
# Print help output

.PHONY: help info
ifneq (,$(findstring help,$(MAKECMDGOALS)))
.NOTPARALLEL: help
endif
ifneq (,$(findstring info,$(MAKECMDGOALS)))
.NOTPARALLEL: info
endif
help info:                                                                   \
    # help:    # Display this list of Makefile targets
	@$(GREP) -E '^.* # .*:    # .*$$' $(MAKEFILE_LIST) 2> /dev/null        | \
        $(AWK) 'BEGIN { FS = "    # " };                                     \
          { printf "%s%-18s %-40s (%-19s)\n", $$1, $$2, $$3, $$1 }'          \
            2> /dev/null | $(CUT) -d ':' -f 2- 2> /dev/null                | \
              $(SED) -e 's/:\ \+)$$/)/' -e 's/XXXX:/\n/g' 2> /dev/null     | \
                $(SED) -e 's/---- (.*)$$/------------\n/'                    \
                 -e 's/:  )/)/g' -e 's/:       )/)/g'                        \
                  -e 's/              ---/-------------/' 2> /dev/null     | \
                    $(GREP) -v 'GREP' 2> /dev/null                         | \
                      $(GREP) -v '_LIST' 2> /dev/null | $(SED) 's/^n//g'   | \
                       $(SED) 's/n$$//g'                                  || \
                        { $(PRINTF) '%s\n' "Error: Unable to display help."; \
                          $(TRUE) > /dev/null 2>&1; }                     || \
                            $(TRUE) > /dev/null 2>&1
	@$(PRINTF) '%s\n' "" 2> /dev/null || $(TRUE) > /dev/null 2>&1

##############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
