<!-- SPDX-License-Identifier: MIT-0 -->
<!-- scspell-id: ebdd502d-f777-11ec-9466-80ee73e9b8e7 -->
<!-- Copyright (c) 2021-2024 The DPS8M Development Team -->

# DPS8/M + Multics Continuous Integration Scripts

-------------------------------------------------

## Introduction

 The original purpose of these scripts was to provide a build and
 test environment for Continuous Integration of the DPS8/M simulator.

## Requirements

 These scripts were designed to run under **Linux** and are regularly tested
 with **Red Hat Enterprise Linux**, **Fedora**, **Debian**, **Ubuntu**,
 and **OpenSUSE**.

 Other operating systems, specifically, IBM **AIX**, Apple **macOS**, Oracle
 **Solaris**, **FreeBSD**, and illumos **OpenIndiana** have also been used
 with various levels of success.

 The following packages are required (*beyond the basic build prerequisites*):
   * [dos2unix](https://waterlan.home.xs4all.nl/dos2unix.html)
     (packaged with "*unix2dos*")
   * [Expect](https://core.tcl-lang.org/expect/)
   * [GNU Coreutils](https://www.gnu.org/software/coreutils/)
   * [GNU Diffutils](https://www.gnu.org/software/diffutils/)
   * [GNU sed](https://www.gnu.org/software/sed/)
   * [GNU Wget](https://www.gnu.org/software/wget/)
   * [lzip](https://www.nongnu.org/lzip/)
   * [tmux](https://tmux.github.io/)

 A BSD-derived `telnet` client is required, such as:
   * [GNU Inetutils](https://www.gnu.org/software/inetutils/)
   * [illumos Telnet](https://github.com/illumos/illumos-gate/tree/master/usr/src/cmd/cmd-inet/)

 The following packages are optional, but highly recommended:
   * [aria2](https://aria2.github.io/) - multi-connection download utility
   * [mksh](http://www.mirbsd.org/mksh.htm) - the MirBSD™ Korn Shell, a
       high quality Korn Shell implementation
   * [libfaketime](https://github.com/wolfcw/libfaketime/) - used to set a
       fixed starting date for CI-Kit runs, enhancing log reproducibility
   * [ncat](https://nmap.org/ncat/) - used to probe for available ports
   * [ansifilter](https://gitlab.com/saalen/ansifilter) - ANSI sequence filter
   * [moreutils](https://joeyh.name/code/moreutils/) - timestamp filter

 In addition, a visual difference comparison tool is highly useful to
 verify the output. Any of the following tools (*listed alphabetically*)
 are known to be sufficient for this task:
   * [Beyond Compare](https://www.scootersoftware.com/)
   * [Code Compare](https://www.devart.com/codecompare/)
   * [Diffinity](https://truehumandesign.se/s_diffinity.php)
   * [DiffMerge](https://sourcegear.com/diffmerge/)
   * [ExamDiff Pro](https://www.prestosoft.com/edp_examdiffpro.asp)
   * [Guiffy](https://www.guiffy.com/)
   * [KDiff3](https://github.com/KDE/kdiff3/)
   * [Meld](https://meldmerge.org/)
   * [Araxis Merge](https://www.araxis.com/merge/)
   * [P4Merge](https://www.perforce.com/downloads/visual-merge-tool/)
   * [WinMerge](https://github.com/winmerge/winmerge/)
   * [xxdiff](https://furius.ca/xxdiff/)
   * Vim/NeoVim
     * [NeoVim Diff](https://neovim.io/doc/user/diff.html)
     * [Vim Diff](https://vimhelp.org/diff.txt.html)
     * [DiffChar](https://github.com/rickhowe/diffchar.vim#readme)

## Usage

 1. Run the initialization script. This will check for various prerequisites,
    copy the required tape images from `/var/cache/tapes` if present, or
    download them (using `wget`) if they are not.
```sh
      ./init.sh
```

 2. Set the `NOREBUILD` environment variable if you have already built the
    simulator, and want the tests run against the that compiled binary.
```sh
      export NOREBUILD=1
```

 3. Set the `MAKE` environment variable if GNU Make is not installed as `make`.
 ```sh
      export MAKE=gmake
 ```

 4. Ensure you do not have any non-default settings in `~/.telnetrc`, and
    then use the `ci.sh` shell script to begin the run.
```sh
      ./ci.sh
```
 * For FreeBSD systems, after installing the CI-Kit dependencies,
   (`coreutils`, `unix2dos`, `expect`, `nmap`, `mksh`, `gmake`,
   `tmux`, `lzip`, etc.), the run should be started with `ci-fbsd.sh`.

   ```sh
   ./ci-fbsd.sh
   ```

 5. Once the run completes, normalize the new output so the results can be
    (*visually*) compared against the included known good reference log file:
```sh
      make -f ci.makefile diff
```

 6. If this run looks good, (optionally) replace `ci_full.log.ref` with
    `ci_full.log` to update the known good reference log file.

### Aborting

The best way to abort the run and end all processes will vary depending on the
operating system. On modern Linux systems, the most straightforward way is to
interrupt the script (`Control-Z`), kill the process, then the `tmux` session.

Example:
```text
^Z
$ kill -9 %1
[1]  + 5730821 killed     ./ci.sh
$ tmux ls
cikit-1010045678912345678-0: 1 windows (created Fri Dec 25 11:12:13 2222)
$ tmux kill-session -t cikit-1010045678912345678-0
```

You should examine your login session and system process table to ensure that
the simulator and any related processes have been terminated.

## Files

**NOTE**: ***This section is out of date.***

* `ci.sh`
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

* `init.sh`
  This script will download the Multics MR12.8 tapes and place them in the
  tapes directory.

* `tidy.sh`
  This is a shell script that attempts to remove some of the variation in the
  log files to allow meld to only show significant changes. It does a fairly
  good job but there are still some things it doesn't catch. Also, it can't
  do anything about lines being out of order (which can happen quite a bit).

## History

   *Charles Anthony* wrote the initial version of these scripts.
   *Dean Anderson* made significant changes to overcome some of the
   original limitations in the scripts. *Jeffrey Johnson* made additional
   enhancements and adapted the scripts for compatibility with the GitLab
   CI/CD environment.

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

## Issues

   It turns out that the original intent of automating a CI process with
   these scripts does not work well. The main issue is that there is enough
   variation in the log files that automated comparison is very difficult.

   Most likely it will take a sophisticated program to do the comparison or
   a different approach to automating this testing.

   These scripts are still useful for performing manual testing runs so they
   are being kept around for that purpose.
