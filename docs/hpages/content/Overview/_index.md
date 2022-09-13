<!-- SPDX-License-Identifier: ICU
     Copyright (c) 2019-2021 Dean S. Anderson
     Copyright (c) 2022 The DPS8M Development Team
 -->
* **DPS8M** is a simulator of the **36‑bit** GE Large Systems / Honeywell / Bull 600/6000‑series mainframe computers (Honeywell 6180, Honeywell Series‑60 ∕ Level‑68, and Honeywell ∕ Bull **DPS‑8/M**) descended from the **GE‑645** and engineered to support the [**Multics** operating system](https://swenson.org/multics_wiki/).

## GE ∕ Honeywell ∕ Bull DPS-8/M Processor

* Characteristics of the **DPS-8/M** processor:
  * **36-bit** native word size
  * **18-bit** address size (allowing a segment size of *256 thousand* words)
  * **15-bit** segment numbers (allowing *32 thousand segments* of *256 thousand words*)
  * Two **36-bit** accumulators (**`A`** &  **`Q`**)
  * Eight **18-bit** index registers (**`X0`** … **`X7`**)
  * Eight pointer/address registers
    * Each containing a **15-bit** segment number, **18-bit** address, and **6-bit** identifier
  * String manipulation instructions (**move** & **compare**)
  * Advanced mathematical instructions supporting:
    * **36-** and **72-bit** integer and floating point arithmetic
      * Hexadecimal floating point option (**HFP**)
    * **10-digit** decimal arithmetic
    * Decimal number formatting (*e.g.* **COBOL** or **PL/I** "*PIC*")
  * Direct Memory Access I/O (**DMA**)

## The DPS8M Simulator

* **DPS8M** simulates not only the **DPS‑8/M** central processor unit, but the **complete mainframe system** with associated peripheral devices, including:
  * Operator Consoles (**OPC**)
  * Central Processing Units (**CPU**)
  * Input/Output Multiplexers (**IOM**)
  * System Control Units (**SCU**)
  * Front-End Network Processors (**FNP**)
  * ABSI Interfaces
  * Tape Drives
  * Disk Storage Units
  * Printers
  * Card Readers
  * Card Punches

* **DPS8M** is [**open source software**](https://dps8m.gitlab.io/dps8m/License_Information) developed by **The DPS8M Development Team** and **many contributors**.

* **DPS8M** supports most <u>operating systems</u> conforming to *IEEE Std 1003.1-2008* (**POSIX.1-2008**), many <u>C compilers</u> conforming to *ISO/IEC 9899:1999* (**C99**), and numerous <u>hardware architectures</u>.

  * <u>Operating systems</u> supported are **AIX**, **FreeBSD**, **NetBSD**, **OpenBSD**, **DragonFly BSD**, **Haiku**, **GNU/Hurd**, **illumos OpenIndiana**, **Linux**, **macOS**, **Solaris**, and **Windows**.

  * <u>C compilers</u> supported are **Clang**, **LLVM-MinGW**, AMD Optimizing C/C++ (**AOCC**),
    Arm C/C++ Compiler (**ARMClang**), GNU C (**GCC**), IBM Advance Toolchain,
    IBM XL C/C++ (**XLC**), Intel oneAPI DPC++/C++ (**ICX**),
    Intel C++ Compiler Classic for macOS (**ICC**), and Oracle Developer Studio (**SunCC**).

  * <u>Hardware architectures</u> supported are **Intel x86** (i686, x86_64), **ARM** (ARMv6, ARMv7, ARM64), **PowerPC** (PPC, PPC64, PPC64le), **RISC-V** (RV64), and **m68k** (68020+).

* Various releases of **DPS8M** have been ported to **embedded systems**, **cell phones**, **tablets**, **handheld gaming consoles**, **wireless routers**, and even modern **mainframes**.  A full-featured port should be possible for any 32- or 64-bit platform with appropriate hardware atomic operations and able to support the [**libuv**](https://libuv.org/) asynchronous I/O library.

* The simulator is distributed as an [**easy-to-build**](../Documentation/Source_Compilation)
  source code [**distribution**](../Releases) (buildable simply via **`make`** on most systems) or as ready-to-run [**pre-compiled binaries**](../Releases) for several popular platforms.

* **DPS8M** development is hosted courtesy of [**GitLab**](https://gitlab.com/dps8m/dps8m), providing [version control](https://gitlab.com/dps8m/dps8m), [issue tracking](https://gitlab.com/dps8m/dps8m/-/issues), and [CI/CD services](https://gitlab.com/dps8m/dps8m/-/pipelines).

* For additional information or assistance, review the available [**documentation**](../Documentation) and [**community resources**](../Community).
