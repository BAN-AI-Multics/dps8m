<!-- SPDX-License-Identifier: MIT-0 -->
<!-- scspell-id: 6a2354d8-f90e-11ec-8827-80ee73e9b8e7 -->
<!-- Copyright (c) 2021-2025 The DPS8M Development Team -->

# Performance Testing

These performance testing tools are intended to measure simulated CPU
performance and allows for evaluation of changes to the CPU code.

## Overview

The tools consist of:

* `--perftest [filename]` (or rebuilding the simulator with the
  `PERF_STRIP=1` build option) runs the simulator in a restricted
  environment with only a working CPU.  All I/O is completely
  disabled.  Running the simulator will load a memory image from the
  disk (by default, "strip.mem") and begin executing it. The run
  times are recorded and statistics output that can be used to
  evaluate the impact of the changes on CPU performance.

* "`SEGLDR`": `SEGLDR` is an SCP command that allows the assembly in dps8m
  memory of segments (benchmark code, stack, libraries and initialization)
  and saving or restoring that memory to/from a host file.

## Files

* Note: Several files have an "`x`" variant.  There is a bug in the
  bootloader code that prevents the standard Multics procedure
  entry/exit code from working; the "`x`" variants have been patched
  to use a simplified entry/exit procedure.

* `nqueens.pl1`: The benchmark code; it counts the number of solutions
  to the N-queens problem.

* `bound_library_wired_`: A copy of Multics `bound_library_wired_` segment.

* `nqueensx`, `nqueensx.as8`, `nqueensx_build.sh`: The benchmark segment,
  source, and build script. (`nqueensx` differs from `nqueens` in that
  the procedure entry/exit code as been removed due to bugs in `segldr_boot`.)

* `segldr_bootx`, `seglgr_bootx.as8`, `segldr_bootx_build.sh`: The bootstrap
  segment, source, and build script.

* `strip_prep.ini`: This script builds the `strip.mem` file.

* `strip.ini`: This script runs the `strip.mem` code on the simulator.

* `test.sh`: An example of how to run the benchmark on a stable host CPU.
  * NOTE: This script alters the performance settings of the host system,
    and WILL NEED TO BE CUSTOMIZED for a particular system configuration.
