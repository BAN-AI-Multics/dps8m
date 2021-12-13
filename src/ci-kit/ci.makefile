### Initial Setup
ifndef CONPORT
  CONPORT=6001
endif
export CONPORT

ifndef FNPPORT
  FNPPORT=9180
endif
export FNPPORT

### Instructions
.PHONY: all
all:
	@printf '%s\n' "Targets:"
	@printf '%s\n' ""
	@printf '%s\n' " s1 ......... Build simulator"
	@printf '%s\n' " s2 ......... Build working directory"
	@printf '%s\n' " s2p ........ Warm caches for s3"
	@printf '%s\n' " s3 ......... Run MR12.7_install.ini"
	@printf '%s\n' " s3p ........ Warm caches for s4"
	@printf '%s\n' " s4 ......... Setup Yoyodyne"
	@printf '%s\n' " s5 ......... Run ci_t1.expect"
	@printf '%s\n' " s6 ......... Run isolts.expect"
	@printf '%s\n' " s7 ......... Run performance test"
	@printf '%s\n' " diff ....... Post-process results"

### Stage 1
.PHONY: s1
s1:
	@printf '%s\n' "Start Stage 1: Build simulator"
ifndef NOREBUILD
	(cd ../.. && $(MAKE) superclean > /dev/null 2>&1 && $(MAKE) 2>&1)
endif
	@printf '%s\n' "End Stage 1"

### Stage 2
.PHONY: s2
s2:
	@printf '%s\n' "Start Stage 2: Build working directory"
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
	@printf '%s\n' "End Stage 2"

### Stage 2p
.PHONY: s2p
s2p:
	-@test -x ../vmpctool/vmpctool && ../vmpctool/vmpctool -qhft ./tapes/foo.tap ./tapes/*LISTINGS.tap ./tapes/*MISC.tap ./tapes/*UNBUNDLED.tap ./tapes/*LDD_STANDARD.tap ./tapes/*EXEC.tap ./tapes/*MULTICS.tap 2> /dev/null || true

### Stage 3
.PHONY: s3
s3:
	@printf '%s\n' "Start Stage 3: Test MR12.7_install.ini"
	cd ./run && env CPUPROFILE=install.prof.out ./dps8 -r MR12.7_install.ini 2>&1
	@printf '%s\n' "End Stage 3"

### Stage 3p
.PHONY: s3p
s3p: s2p

### Stage 4
.PHONY: s4
s4:
	@printf '%s\n' "Start Stage 4: Setup Yoyodyne"
	cd ./run && env CPUPROFILE=yoyodyne.prof.out ./dps8 -r yoyodyne.ini 2>&1
	@printf '%s\n' "End Stage 4"

### Stage 5
.PHONY: s5
s5:
	@printf '%s\n' "Start Stage 5: Run ci_t1.expect"
	env CPUPROFILE=run.prof.out ./ci_t1.sh 0 2>&1
	@printf '%s\n' "End Stage 5"

### Stage 6
.PHONY: s6
s6:
	@printf '%s\n' "Start Stage 6: Run isolts.expect"
	env CPUPROFILE=isolts.prof.out ./isolts.sh 0 2>&1
	@printf '%s\n' "End Stage 6"

### Stage 7
.PHONY: s7
s7:
	@printf '%s\n' "Start Stage 7: Run performance test"
	env CPUPROFILE=perf.prof.out ./perf.sh 0 2>&1
	@printf '%s\n' "End Stage 7"

### Post-processing
.PHONY: diff
diff:
	@printf '%s\n'  "============================="    >  ci_full.log
	@printf '%s\n'  "=         ci.log            ="   >>  ci_full.log
	@printf '%s\n'  "============================="   >>  ci_full.log
	@cat ci.log                                       >>  ci_full.log
	@printf '%s\n'  "============================="   >>  ci_full.log
	@printf '%s\n'  "=        ci_t2.log          ="   >>  ci_full.log
	@printf '%s\n'  "============================="   >>  ci_full.log
	@cat ci_t2.log                                    >>  ci_full.log
	@printf '%s\n'  "============================="   >>  ci_full.log
	@printf '%s\n'  "=        ci_t3.log          ="   >>  ci_full.log
	@printf '%s\n'  "============================="   >>  ci_full.log
	@cat ci_t3.log                                    >>  ci_full.log
	-@ls -1 ./run/printers/prta/* 2> /dev/null        >>  ci_full.log
	@printf '%s\n'  "============================="   >>  ci_full.log
	@printf '%s\n'  "=        isolts.log         ="   >>  ci_full.log
	@printf '%s\n'  "============================="   >>  ci_full.log
	@cat isolts.log                                   >>  ci_full.log
	@printf '%s\n'  "============================="   >>  ci_full.log
	@printf '%s\n'  "=        perf.log           ="   >>  ci_full.log
	@printf '%s\n'  "============================="   >>  ci_full.log
	@cat perf.log                                     >>  ci_full.log
	@tr -d '\0' < ci_full.log.ref | tr -cd '[:print:][:space:]\n' | \
     sed 's/\r//g' | awk '{ print "\n"$$0"\r" }'                  | \
     grep -v '^$$' | dos2unix -f | ./tidy.sh           >  old.log
	@tr -d '\0' < ci_full.log | tr -cd '[:print:][:space:]\n'     | \
     sed 's/\r//g' | awk '{ print "\n"$$0"\r" }'                  | \
     grep -v '^$$' | dos2unix -f | ./tidy.sh           >  new.log
	@printf '%s\n' "Done; you may now compare old.log and new.log"

### Post-processing
.PHONY: diff_files
diff_files:
	@tr -d '\0' < ci.log.ref | tr -cd '[:print:][:space:]\n'      | \
     sed 's/\r//g' | awk '{ print "\n"$$0"\r" }'                  | \
     grep -v '^$$' | dos2unix -f | ./tidy.sh           > old.log
	@tr -d '\0' < ci.log  | tr -cd '[:print:][:space:]\n'         | \
     sed 's/\r//g' | awk '{ print "\n"$$0"\r" }'                  | \
     grep -v '^$$' | dos2unix -f | ./tidy.sh           > new.log
	@printf '%s\n' "Done; you may now compare old.log and new.log"

# EOF
