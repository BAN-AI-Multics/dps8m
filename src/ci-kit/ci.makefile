all : 
	@echo "s1  Pull and build"
	@echo "s2  Build working directory"
	@echo "s3  Test MR12.7_install.ini"
	@echo "s4  Setup Yoyodyne"
	@echo "s5  Run ci_t1.expect"


.PHONY : s1 s2 s3 s4 s5 s6

s1:
	@echo "Start Stage 1: Pull and build"
	rm -Rf ./dps8m
	git clone https://gitlab.com/dps8m/dps8m.git
	cd ./dps8m && git checkout master
	cd ./dps8m && git status
	cd ./dps8m/src/dps8 && $(MAKE) LOCKLESS=1 clean all
	@echo "End Stage 1"

s2:
	@echo "Start Stage 2: Build working directory"
	rm -Rf ./run
	mkdir ./run
	mkdir ./run/tapes
	mkdir ./run/tapes/12.7
	mkdir ./run/tapes/general
	mkdir ./run/disks
	cp ./dps8m/src/dps8/dps8 ./run
	cp ./tapes/12.7* ./run/tapes/12.7
	cp ./tapes/foo.tap ./run/tapes/general
	cp ./ini/* ./run
	cp ./ec/* ./run
	@echo "End Stage 2"

s3:
	@echo "Start Stage 3: Test MR12.7_install.ini"
	cd ./run && CPUPROFILE=install.prof.out ./dps8 -v MR12.7_install.ini
	@echo "End Stage 3"

s4:
	@echo "Start Stage 4: Setup Yoyodyne"
	cd ./run && CPUPROFILE=yoyodyne.prof.out ./dps8 -v yoyodyne.ini
	@echo "End Stage 4"

s5:
	@echo "Start Stage 5: Run ci_t1.expect"
	CPUPROFILE=run.prof.out ./ci_t1.expect 0
	@echo "End Stage 5"

.PHONY : diff

diff :
	echo "=============================" >ci_full.log
	echo "=         ci.log            =" >>ci_full.log
	echo "=============================" >>ci_full.log
	cat ci.log >>ci_full.log
	echo "=============================" >>ci_full.log
	echo "=        ci_t2.log          =" >>ci_full.log
	echo "=============================" >>ci_full.log
	cat ci_t2.log >>ci_full.log
	echo "=============================" >>ci_full.log
	echo "=        ci_t3.log          =" >>ci_full.log
	echo "=============================" >>ci_full.log
	cat ci_t3.log >>ci_full.log
	dos2unix -f < ci_full.log | ./tidy > new.log
	dos2unix -f < ci_full.log.ref | ./tidy > old.log
	meld new.log old.log &

.PHONY : diff_files

diff_files :
	dos2unix -f < ci.log | ./tidy > new.log
	dos2unix -f < ci.log.ref | ./tidy > old.log

kit:
	tar cfz kit.tgz ci.makefile ci ci_full.log.ref *.expect tidy README.txt tapes/* ec/* ini/*

