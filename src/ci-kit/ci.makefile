# DPS8M simulator: src/ci-kit/ci.makefile
# vim: filetype=make:tabstop=4:ai:cc=79:noexpandtab:list:listchars=tab\:\>\-
# SPDX-License-Identifier: ICU
# scspell-id: 5ccd4788-f62b-11ec-954c-80ee73e9b8e7

###############################################################################
#
# Copyright (c) 2018-2023 The DPS8M Development Team
#
# All rights reserved.
#
# This software is made available under the terms of the ICU
# License, version 1.8.1 or later.  For more details, see the
# LICENSE.md file at the top-level directory of this distribution.
#
###############################################################################

### Initial Setup #############################################################

# Default console port
ifndef CONPORT
  CONPORT=6001
endif
export CONPORT

# Default FNP port
ifndef FNPPORT
  FNPPORT=9180
endif
export FNPPORT

# Default tools
AWK      ?= $(shell command -v gawk 2> /dev/null  ||                          \
                env PATH="$$(command -p env getconf PATH)"                    \
                    sh -c "command -v awk")
GREP     ?= $(shell command -v ggrep 2> /dev/null ||                          \
                env PATH="$$(command -p env getconf PATH)"                    \
                    sh -c "command -v grep")
SED      ?= $(shell command -v gsed 2> /dev/null  ||                          \
                env PATH="$$(command -p env getconf PATH)"                    \
                    sh -c "command -v sed")
TR       ?= tr
CAT      ?= cat
DOS2UNIX ?= dos2unix

### Instructions ##############################################################

.PHONY: all help
.NOTPARALLEL: all help
all help:
	@printf '%s\n' "  "
	@printf '%s\n' "  Targets"
	@printf '%s\n' "  ======="
	@printf '%s\n' "  "
	@printf '%s\n' "  s1 ..................... Build simulator"
	@printf '%s\n' "  s2 ..................... Build CI-Kit working directory"
	@printf '%s\n' "  s2p .................... Warm caches for s3"
	@printf '%s\n' "  s3 ..................... Run MR12.7_install.ini"
	@printf '%s\n' "  s3p .................... Warm caches for s4"
	@printf '%s\n' "  s4 ..................... Setup Yoyodyne system"
	@printf '%s\n' "  s4p .................... Warm caches for s5"
	@printf '%s\n' "  s5 ..................... Run ci_t1.expect"
	@printf '%s\n' "  s6 ..................... Run isolts.expect"
	@printf '%s\n' "  s7 ..................... Run performance test"
	@printf '%s\n' "  diff ................... Post-process results"
	@printf '%s\n' "  "

### Stage 1 - Build simulator #################################################

.PHONY: s1
.NOTPARALLEL: s1
s1:
	@printf '\n%s\n' "### Start Stage 1: Build simulator ####################"
ifndef NOREBUILD
	( cd ../.. && $(MAKE) superclean > /dev/null 2>&1 && env NATIVE=1 $(MAKE) 2>&1 )
endif
ifdef NOREBUILD
	@printf '%s\n' "  *** NOREBUILD set - skipping simulator build ***"
endif
	-@sleep 2 > /dev/null 2>&1 || true
	@test -x "../dps8/dps8" || {                                              \
        printf '\n%s\n\n' "ERROR: No dps8 executable";                        \
        exit 1; }
	-@sleep 2 > /dev/null 2>&1 || true
	@printf '\n%s\n' "### End Stage 1 #######################################"
	@sleep 2 > /dev/null 2>&1 || true

### Stage 2 - Build working directory #########################################

.PHONY: s2
.NOTPARALLEL: s2
s2: ../dps8/dps8
	@printf '\n%s\n' "### Start Stage 2: Build CI-Kit working directory #####"
	@rm -f    ./.yoyodyne.s4
	@rm -Rf   ./run
	@mkdir -p ./run
	@mkdir -p ./run/tapes
	@mkdir -p ./run/tapes/12.7
	@mkdir -p ./run/tapes/general
	@mkdir -p ./run/disks
	@printf %s\\n "sn: 0" > ./run/serial.txt 2> /dev/null || true
	@cp -fp ../dps8/dps8    ./run
	@cp -fp ./tapes/12.7*   ./run/tapes/12.7
	@cp -fp ./tapes/foo.tap ./run/tapes/general
	@cp -fp ./ini/* ./run
	@cp -fp ./ec/*  ./run
	@printf '\n%s\n' "### End Stage 2 #######################################"
	@sleep 2 > /dev/null 2>&1 || true

### Stage 2p - Warm caches for s3 #############################################

.PHONY: s2p
.NOTPARALLEL: s2p
s2p:
	-@test -x ../vmpctool/vmpctool && printf '\n%s\n'                         \
        "### Priming caches ####################################"             \
            || true
	-@test -x ../vmpctool/vmpctool || printf '%s\n' "" || true
	-@test -x ../vmpctool/vmpctool && ../vmpctool/vmpctool -hft               \
        ./tapes/*.tap || true
	-@test -x ../vmpctool/vmpctool && printf '\n%s\n'                         \
        "### Cache primed ######################################"             \
            || true

### Stage 3 - Run MR12.7_install.ini ##########################################

.PHONY: s3
.NOTPARALLEL: s3
s3: ../dps8/dps8
	@printf '%s\n' "" || true
	@printf '%s\n' "### Start Stage 3: Test MR12.7_install.ini ############"  \
        || true
	@rm -f ./run/disks/root.dsk.reloaded > /dev/null 2>&1 || true
	@rm -f ./run/disks/newinstall.dsk    > /dev/null 2>&1 || true
	cd ./run && time env CPUPROFILE=install.prof.out ./dps8 -t MR12.7_install.ini 2>&1
	@cp -fp ./run/disks/newinstall.dsk ./run/disks/yoyodyne.dsk
	@printf '%s\n' ""
	@printf '%s\n' "### End Stage 3 #######################################"  \
        || true
	@sleep 2 > /dev/null 2>&1 || true

### Stage 3p - Warm caches for s4 #############################################

.PHONY: s3p
.NOTPARALLEL: s3p
s3p:
	-@test -x ../vmpctool/vmpctool && printf '\n%s\n'                         \
        "### Priming caches ####################################"             \
            || true
	-@test -x ../vmpctool/vmpctool || printf '%s\n' "" || true
	-@test -x ../vmpctool/vmpctool && ../vmpctool/vmpctool -hft               \
      ./tapes/*.tap ./run/disks/yoyodyne.dsk || true
	-@test -x ../vmpctool/vmpctool && printf '\n%s\n'                         \
        "### Cache primed ######################################"             \
            || true

### Stage 4 - Setup Yoyodyne ##################################################

.PHONY: s4
.NOTPARALLEL: s4
s4: ./run/disks/yoyodyne.dsk ../dps8/dps8
	@printf '\n%s\n' "### Start Stage 4: Setup Yoyodyne #####################"
	cd ./run && time env CPUPROFILE=yoyodyne.prof.out ./dps8 -t yoyodyne.ini 2>&1
	@printf '\n%s\n' "### End Stage 4 #######################################"
	@sleep 2 > /dev/null 2>&1 || true

### Stage 4p - Warm caches for s5 #############################################

.PHONY: s4p
.NOTPARALLEL: s4p
s4p:
	-@test -x ../vmpctool/vmpctool && printf '\n%s\n'                         \
        "### Priming caches ####################################"             \
            || true
	-@test -x ../vmpctool/vmpctool || printf '%s\n' "" || true
	-@test -x ../vmpctool/vmpctool && ../vmpctool/vmpctool -hft               \
        ./run/tapes/* ./run/disks/yoyodyne.dsk || true
	-@test -x ../vmpctool/vmpctool && printf '\n%s\n'                         \
        "### Cache primed ######################################"             \
            || true

### Stage 5 - Run ci_t1.expect ################################################

.PHONY: s5
.NOTPARALLEL: s5
s5: ./run/disks/yoyodyne.dsk ../dps8/dps8 ./.yoyodyne.s4
	@printf '\n%s\n' "### Start Stage 5: Run ci_t1.expect ###################"
	time env CPUPROFILE=run.prof.out ./ci_t1.sh 0 2>&1
	@printf '\n%s\n' "### End Stage 5 #######################################"
	@sleep 2 > /dev/null 2>&1 || true

### Stage 6 - Run isolts.expect ###############################################

.PHONY: s6
.NOTPARALLEL: s6
s6: ./run/disks/yoyodyne.dsk ../dps8/dps8 ./.yoyodyne.s4
	@printf '\n%s\n' "### Start Stage 6: Run isolts.expect ##################"
	time env CPUPROFILE=isolts.prof.out ./isolts.sh 0 2>&1
	@printf '\n%s\n' "### End Stage 6 #######################################"
	@sleep 2 > /dev/null 2>&1 || true

### Stage 7 - Run performance test ############################################

.PHONY: s7
.NOTPARALLEL: s7
s7: ../dps8/dps8
	@printf '\n%s\n' "### Start Stage 7: Run performance test ###############"
	time env CPUPROFILE=perf.prof.out ./perf.sh 0 2>&1
	@printf '\n%s\n' "### End Stage 7 #######################################"
	@sleep 2 > /dev/null 2>&1 || true

### Post-processing 1 #########################################################

.PHONY: diff tidy
.NOTPARALLEL: diff tidy
diff: ci.log ci_t2.log ci_t3.log isolts.log perf.log
	@printf '%s\n'  "####################################"   >  ci_full.log
	@printf '%s\n'  "#########  CI Log: Part 1  #########"  >>  ci_full.log
	@printf '%s\n'  "####################################"  >>  ci_full.log
	@$(CAT) ci.log                                          >>  ci_full.log
	@printf '%s\n'  "####################################"  >>  ci_full.log
	@printf '%s\n'  "#########  CI Log: Part 2  #########"  >>  ci_full.log
	@printf '%s\n'  "####################################"  >>  ci_full.log
	@$(CAT) ci_t2.log                                       >>  ci_full.log
	@printf '%s\n'  "####################################"  >>  ci_full.log
	@printf '%s\n'  "#########  CI Log: Part 3  #########"  >>  ci_full.log
	@printf '%s\n'  "####################################"  >>  ci_full.log
	@$(CAT) ci_t3.log                                       >>  ci_full.log
	@printf '%s\n'  "####################################"  >>  ci_full.log
	@printf '%s\n'  "#############  ISOLTS  #############"  >>  ci_full.log
	@printf '%s\n'  "####################################"  >>  ci_full.log
	@$(CAT) isolts.log                                      >>  ci_full.log
	@printf '%s\n'  "####################################"  >>  ci_full.log
	@printf '%s\n'  "########  Performance Test  ########"  >>  ci_full.log
	@printf '%s\n'  "####################################"  >>  ci_full.log
	@$(CAT) perf.log                                        >>  ci_full.log
	@printf '%s\n'  "####################################"  >>  ci_full.log
	@printf '%s\n'  "##############  EOF  ###############"  >>  ci_full.log
	@printf '%s\n'  "####################################"  >>  ci_full.log
	@$(TR) -d '\0' < ci_full.log.ref | $(TR) -cd '[:print:][:space:]\n' |     \
     $(SED) 's/\r//g' | $(AWK) '{ print "\n"$$0"\r" }'                  |     \
     $(GREP) -v '^$$' | $(DOS2UNIX) -f | ./tidy.sh     >  old.log
	@$(TR) -d '\0' < ci_full.log | $(TR) -cd '[:print:][:space:]\n'     |     \
     $(SED) 's/\r//g' | $(AWK) '{ print "\n"$$0"\r" }'                  |     \
     $(GREP) -v '^$$' | $(DOS2UNIX) -f | ./tidy.sh     >  new.log
	@printf '%s\n' "Done; you may now compare old.log and new.log"

### Post-processing 2 #########################################################

.PHONY: diff_files tidy
.NOTPARALLEL: diff_files tidy
diff_files: ci.log.ref ci.log
	@$(TR) -d '\0' < ci.log.ref | $(TR) -cd '[:print:][:space:]\n' |          \
     $(SED) 's/\r//g' | $(AWK) '{ print "\n"$$0"\r" }'             |          \
     $(GREP) -v '^$$' | $(DOS2UNIX) -f | ./tidy.sh     > old.log
	@$(TR) -d '\0' < ci.log  | $(TR) -cd '[:print:][:space:]\n'    |          \
     $(SED) 's/\r//g' | $(AWK) '{ print "\n"$$0"\r" }'             |          \
     $(GREP) -v '^$$' | $(DOS2UNIX) -f | ./tidy.sh     > new.log
	@printf '%s\n' "Done; you may now compare old.log and new.log"

### Done ######################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
