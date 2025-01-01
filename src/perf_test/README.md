<!-- SPDX-License-Identifier: MIT-0 -->
<!-- scspell-id: 6a2354d8-f90e-11ec-8827-80ee73e9b8e7 -->
<!-- Copyright (c) 2021-2025 The DPS8M Development Team -->

```text
Performance Testing
----------- -------

The performance testing tools are intended to measure simulated CPU
performance; this measurement allows for evaluation of code changes
to the CPU code.

The tools consist of:

"PERF_STRIP":  Building the code with "make PERF_STRIP=1" produces
a simulator that has only a working CPU. I/O is completely disabled.
Running the simulator will load a memory image from the file "strip.mem"
and begin executing it. By then applying changes to the simulator code,
rebuilding and re-running, the run times can be compared to evaluate
the impact of the changes on CPU performance.

"SEGLDR" SEGLDR is a SIMH command that allows the assembly in dps8m
memory of segments (benchmark code, stack, libraries and initialization);
and saving or restoring that memory to/from a host file.

Files
-----

(Note; several files have an "x" variant. There is a bug in the
bootloader code that prevents the standard Multics procedure
entry/exit code from working; the "x" variants have been patched
to use a simplified entry/exit procedure.

nqueens.pl1 is the benchmark code; it counts the number of solutions
to the n-queens problem.

bound_library_wired_: Copy of Multics bound_library_wired_ segment.

nqueensx, nqueensx.as8, nqueensx_build.sh; The benchmark
   segment, source and build script. (nqueensx differs from
   nqueens in that the procedure entry/exit code as been removed
   due to bugs in segldr_boot.)

segldr_bootx, seglgr_bootx.as8, segldr_bootx_build.sh; The bootstrap
   segment, source and build script.

strip_prep.ini: This file builds the strip.mem file.

strip.ini: Run the strip.mem code on a non-"PERF_STRIP" build.

test.sh: Run the benchmark on a stable host CPU -- NOTE: this script
    alters the performance settings of the host system, and WILL
    NEED TO BE CHANGED for other systems.
```
