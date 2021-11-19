# DPS8/M + Multics Continuous Integration Scripts


## INTRODUCTION

 The original purpose of these scripts was to provide a build and
 test environment for Continuous Integration of the DPS8/M simulator.
 *Charles Anthony* wrote the initial version of these scripts and
 *Dean Andersom* made significant changes to overcome some of the
 original issues in the scripts. *Jeffrey Johnson* adapted the process
 to run in the GitLab CI/CD environment.


## REQUIREMENTS

 These scripts were designed to run under **Linux** and have been tested
 with **Red Hat Enterprise Linux**, **Fedora**, **Ubuntu**, and
 **OpenSUSE**.

 Other operating systems, specifically IBM **AIX**, Apple **macOS**, Oracle
 **Solaris**, and illumos **OpenIndiana** have also been used successfuly.

 The following packages are required (beyond the build prerequisites):

   * [Bash](https://www.gnu.org/software/bash/)
   * [dos2unix](https://waterlan.home.xs4all.nl/dos2unix.html)
   * [Expect](https://core.tcl-lang.org/expect/)
   * [GNU Coreutils](https://www.gnu.org/software/coreutils/)
   * [GNU Diffutils](https://www.gnu.org/software/diffutils/)
   * [GNU sed](https://www.gnu.org/software/sed/)
   * [GNU Wget](https://www.gnu.org/software/wget/)
   * [lzip](https://www.nongnu.org/lzip/)
   * **BSD**-derived `telnet`, such as,
     * [Apple Telnet](https://opensource.apple.com/)
     * [BAN TELNET](https://github.com/BAN-AI-Multics/ban-telnet)
     * [GNU Inetutils](https://www.gnu.org/software/inetutils/)
     * [illumos telnet](https://github.com/illumos/illumos-gate/tree/master/usr/src/cmd/cmd-inet)
     * [NetKit Telnet-SSL](https://github.com/marado/netkit-telnet-ssl)

 In addition, a visual difference comparison tool is highly useful to
 verify the output. Any of the following tools (*listed alphabetically*)
 are known to be sufficient for this task:

   * [Beyond Compare](https://www.scootersoftware.com/)
   * [Code Compare](https://www.devart.com/codecompare)
   * [Comparison Tool](https://www.eclipse.org/)
   * [diffoscope](https://diffoscope.org/)
   * [ExamDiff](https://www.prestosoft.com/)
   * [Guiffy](https://www.guiffy.com)
   * [Meld](https://meldmerge.org/)
   * [Merge](https://www.araxis.com/merge/)
   * [NeoVim diff](https://neovim.io/doc/user/diff.html)
   * [P4Merge](https://www.perforce.com/downloads/visual-merge-tool)
   * [Vim diff](https://vimhelp.org/diff.txt.html)
   * [WinMerge](https://github.com/winmerge/winmerge)
   * [Xdiff](https://www.plasticscm.com/features/xmerge)
   * [xxdiff](https://furius.ca/xxdiff/)


## USAGE

 1. Run the initialization script to check for prerequisite utilities and
    copy the required tape images from /var/cache/tapes (or download them
    if they are not available):
```sh
      ./init
```

 2. Set the `NOREBUILD` environment variable if you have already built the
    simulator, and want the tests to run against that compiled binary.
```sh
      export NOREBUILD=1
```

 3. Set the `MAKE` variable if GNU Make is not `make` on your system.
 ```sh
      export MAKE=gmake
 ```

 4. Run the main script file with:
```sh
      ./ci
```
 5. Once the run completes, you can normalize the logs so they can be
    (*visually*) compared against the known-good reference log file:
```sh
      make -f ci.makefile diff
```

 6. If this run looks good, (optionally) replace the `ci_full.log` with the
    `ci_full.log.ref` to update the known-good reference log file.


## FILES

**NOTE**: ***This section is is out of date.***

* `ci`
  This is the launch script start automate starting the process. It mainly
  invokes `make` to run the `ci.makefile`.

* `ci_full.log.ref`
  This is the reference log file that can be used to verify the output.

* `ci.makefile`
  This is a Makefile used to perform all the steps needed in the process.

* `ci_t1.expect`
  This is the main `expect` script. It starts the emulator with the init
  script `yoyodyne_start.ini` that will cold boot a clean Multics 12.6f
  system. After that it loads a card deck and the launches the `ci_t2.sh`
  and `ci_t3.sh` `expect` scripts, in that order.

* `ci_t2.expect`
  This script does the following:
    1. logs in to the "*Clayton*" account created during the cold boot process
    2. tests with a tape "foo"
    3. loads an MIT backup tape
    4. runs a bunch of tests (`eis_tester`, `test_cpu`)
    5. does an `instr_speed` and `check_cpu_speed`
    6. creates simple test programs in: PL/1, Pascal, APL, Cobol, Lisp, Basic,
         Fortran, C and, BCPL
    7. creates a simple database with MRDS
    8. uses FAST to create and run a Basic and Fortran program
    9. uses `map355` to build the FNP code
   10. prints most (if not all) of the meters
   11. does some disk management and reporting commands

* `ci_t3.expect` (called by `ci_t3.sh`)
  This script does the following:
    1. logs in to the "*Clayton*" account
    2. invokes `isolts`

* `init`
  This script will download the Multics MR12.7 tapes and place them in the
  tapes directory.

* `tidy`
  This is a shell script that attempts to remove some of the variation in the
  log files to allow meld to only show significant changes. It does a fairly
  good job but there are still some things it doesn't catch. Also, it can't
  do anything about lines being out of order (which can happen quite a bit).


## HISTORY

   The initial version of the scripts did everything in a single script.
   Since both the Multics console and a telnet session were used, it
   required the `expect` script to deal with switching between the emulator
   console and a `telnet` session. This had a side effect that, if the
   timing was off slightly, the entire script execution could get "wedged"
   and lock up.

   An updated approach was implemented that uses multiple scripts and
   separated the emulator console use from the telnet session. This proved to
   be most effective in resolving the "wedging" issues and also removed some
   intermixing of the logs.


## ISSUES

   It turns out that the original intent of automating a CI process with
   these scripts does not work well. The main issue is that there is enough
   variation in the log files that automated comparison is very difficult.

   Most likely it will take a sophisticated program to do the comparison or
   a different approach to automating this testing.

   These scripts are still useful for performing manual testing runs so they
   are being kept around for that purpose.
