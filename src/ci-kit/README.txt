DPS8/M + Multics Continuous Integration Scripts

INTRODUCTION

The original purpose of these scripts was to provide a build and test
environment for Continuous Integration of the DPS8/M emulator. Charles
Anthony wrote the inital version of these scripts and Dean Anderson made
significant changes to overcome some of the original issues in the scripts.


REQUIREMENTS

These scripts were designed to run under Linux and have been tested with
Ubuntu and PopOS.

You need to have all the packages installed to build the DPS8/M emulator
(see emulator documentation).

You also need the following packages installed:
    expect
    bash
    tcsh
    meld
    lzip
    dos2unix
    GNU sed
    GNU coreutils
    GNU diffutils
    BSD-derived telnet (GNU inetutils, netkit, BAN telnet, etc.)
    
    
USAGE

1) Unpack the archive into a directory.

2) Run the initialization script to download the tape images:
      ./init

3) Set the NOREBUILD environment variable if you have already built the
   simulator, and wish to the the scripts against that compiled binary.

4) Run the main script file with:
      ./ci

5) Once the run completes, you can check the differences in the log files
   against a "reference" log file with:
      make -f ci.makefile diff

6) If this run looks good, copy the ci_full.log to ci_full.log.ref as the new
   reference log for the next run.
      
      
FILES

ci
  This is the "launch" script that starts the whole process. It mainly invokes
  make to run the ci.makefile.

ci_full.log.ref
  This is the "reference" log file that will be used in the final comparison.

ci.makefile
  This is the makefile used to perform all the steps needed in the process.

ci_t1.expect
  This is the main "expect" script. It starts the emulator with the init script
  yoyodyne_start.ini that will cold boot a clean Multics 12.6f system. After 
  that it loads a card deck and the launches the ci_t2.sh and ci_t3.sh expect
  scripts in that order. It then monitors the emulator console to act as the
  "operator" for those scripts. When it detects that they have finished, it
  shuts down the emulator.
  
ci_t2.expect
  This script does the following:
    1) logs in to the "Clayton" account created during the cold boot process
    2) tests with a tape "foo"
    3) loads an MIT backup tape
    4) runs a bunch of tests (eis_tester, test_cpu) 
    5) does an instr_speed and check_cpu_speed
    6) creates simple test programs in: PL/1, Pascal, APL, Cobol, Lisp, Basic, 
         Fortran, C and, BCPL
    7) creates a simple database with MRDS
    8) uses FAST to create and run a Basic and Fortran program
    9) uses map355 to build the FNP code
   10) prints most (if not all) of the meters
   11) does some disk management and reporting commands
   
ci_t3.expect (called by ci_t3.sh)
  This script does the following:
    1) logs in to the "Clayton" account
    2) invokes isolts

init
  This script will download the Multics MR12.7 tapes and place them in the tapes
  directory.    

tidy
  This is a shell script that attempts to remove some of the variation in the
  log files to allow meld to only show significant changes. It does a fairly
  good job but there are still some things it doesn't catch. Also, it can't
  do anything about lines being out of order (which can happen quite a bit).
  

HISTORY

The initial version of the scripts did everything in a single script. Since
both the Multics console and a telnet session were used, it required the
"expect" script to deal with switching between the emulator console and a
telnet session. This had a side effect that, if the timing was off slightly,
the entire script execution could get "wedged" and lock up.

An updated approach was implemented that uses multiple scripts and separated
the emulator console use from the telnet session. This proved to be most
effective in resolving the "wedging" issues and also removed some intermixing
of the logs.


ISSUES

It turns out that the original intent of automating a CI process with these
scripts does not really work. The main issue is that there is enough variation
in the log files that an automated comparision is very difficult to do.

Most likely it will take a sophisticated program to do the log comparisions or
a different approach to automating this testing.

These scripts are still useful for performing manual testing runs so they are
being kept around for that purpose.

