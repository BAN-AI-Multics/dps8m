<!-- SPDX-License-Identifier: MIT-0
     Copyright (c) 2019-2021 Dean S. Anderson
     Copyright (c) 2016-2024 The DPS8M Development Team
 -->
**DPS8M** is a simulator of the **36‑bit** GE Large Systems / Honeywell / Bull 600/6000‑series mainframe computers (Honeywell 6180, Honeywell Series‑60 ∕ Level‑68, and Honeywell ∕ Bull **DPS‑8/M**) descended from the **GE‑645** and engineered to support the [**Multics** operating system](https://swenson.org/multics_wiki/).

<!-- br -->

<!-- start nopdf -->

* [**DPS-8/M Hardware Overview**](#processor-characteristics)
* [**DPS8M Simulator Overview**](#the-dps8m-simulator)

<!-- br -->

<!-- stop nopdf -->

## GE ∕ Honeywell ∕ Bull DPS-8/M Processor

<!-- br -->

### Processor characteristics

* Hardware-based enforcement of access restrictions
* Seven hierarchical protection rings (`0`&nbsp;…&nbsp;`7`)
* Segmentation and paging
* 36- & 72-bit fixed-point integer, fixed-point fraction, and floating point arithmetic
* Big-endian word ordering
* Two's complement numeric representation
* Hexadecimal floating point (*HFP*) option (*range of* ±10 *to the* 153*rd* *power*)
* Hardware rounding and normalization
* Content-addressable associative memory for intermediate storage of address and control information
* Address modification and appending
* Absolute address computation at execution time
* High-resolution asynchronous alarm timer (512 KHz; 1.953125 μs *precision*)
* Direct memory access (*DMA*) I/O
* Multilevel fault and priority interrupt handling
* Deferred handling of low-priority faults
* Instruction and operand caching (with *selective clear* and *bypass*)

<!-- pagebreak -->

### Functional organization

<!-- br -->

#### Appending unit

* Controls data input/output to main memory
* Interfaces with caches
* Performs main memory selection and interlace control
* Does address appending and virtual address translation
* Controls fault recognition

<!-- br -->

#### Associative memory assembly

* Provides register-based access to pointers to most recently used segments and pages
* Reduces the need for multiple memory accesses (before obtaining an absolute address of an operand or instruction)

<!-- br -->

#### Control unit

* Performs address modification
* Controls mode of operation
* Performs interrupt recognition
* Decodes instruction words and indirect words
* Performs timer register loading and decrementing

<!-- br -->

#### Operation unit

* Performs fixed- and floating-point binary (*base*-2) arithmetic
* Does shifting and boolean operations

<!-- br -->

#### Decimal unit

* Performs decimal (*base*-10) arithmetic
* Decimal number formatting (*e.g.* **COBOL** or **PL ∕ I** "**`PIC`**")
* Specialized character-string and bit-string operations (*e.g.* **PL ∕ I** "**`INDEX`**")

<!-- pagebreak -->

<!-- br -->

### Modes of operation

* Three memory addressing modes
  * **absolute** mode
  * **append** mode
  * **BAR** mode
* Two instruction execution modes
  * **normal** mode
  * **privileged** mode

<!-- br -->

### Native data sizes

* **36-bit** native word size
* **4-** and **6-bit** "character" sizes
* **9-bit** byte size
* **18-bit** half-word size
* **72-bit** double-word (*word pair*) size
* **15-bit** segment size (**32,768** *segments*)
* **18-bit** address size (**262,144** *words per segment*)

<!-- br -->

### Interrupt handling

* **32** interrupt cells *per* **SCU**
  * interrupt cells are organized in a numbered priority chain

<!-- br -->

### Fault handling

* **27** fault conditions (*expandable to* 32)
  * **32** fault priority levels
  * **7** fault priority groups

<!-- pagebreak -->

### Instruction repertoire

* **547** instructions (*expandable to* 1024)
  * **456** <u>basic</u> instructions (*in* **7** *functional classes*)
    * **181** fixed-point binary arithmetic instructions
    * **85** boolean operation instructions
    * **75** pointer register operation instructions
    * **36** control flow instructions
    * **34** floating-point binary arithmetic instructions
    * **28** privileged instructions
    * **17** miscellaneous instructions
  * **91** <u>Extended Instruction Set</u> (**EIS**) instructions
    * **62** single-word and **29** multi-word instructions, *operating on*:
      * **4-**, **6-**, and **9-bit** alphanumeric strings,
      * **4-** and **9-bit** numeric strings, *and*
      * bit strings
    * **21** opcodes for moving, comparison, scanning, conversion, and translation
    * **20** opcodes for loading, storing, and modifying address pointers and lengths
    * **17** "*micro-operations*" for control of string move and edit operations
    * **8** opcodes for decimal arithmetic

### Registers

* Two **36-bit** accumulator registers (**`A`**&nbsp;&&nbsp;**`Q`**)
* One **72-bit** accumulator-quotient register (**`AQ`**)
* Eight **18-bit** index registers (**`X0`**&nbsp;…&nbsp;**`X7`**)
* One **8-bit** exponent register (**`E`**)
* One **80-bit** exponent-accumulator-quotient register (**`EAQ`**)
* One **14-bit** indicator register (**`IR`**)
* One **18-bit** base address register (**`BAR`**)
* One **27-bit** timer register (**`TR`**)
* One **3-bit** ring alarm register (**`RALR`**)
* Eight **42-bit** pointer registers (**`PR0`**&nbsp;…&nbsp;**`PR7`**)
* Eight **24-bit** address registers (**`AR0`**&nbsp;…&nbsp;**`AR7`**)
* One **37-bit** procedure pointer register (**`PPR`**)
* One **42-bit** temporary pointer register (**`TPR`**)
* One **51-bit** descriptor segment base register (**`DSBR`**)
* One **35-bit** fault register (**`FR`**)
* One **33-bit** mode register (**`MR`**)
* One **28-bit** cache mode register (**`CMR`**)
* Sixteen **51-bit** page table word associative memory (**`PTWAM`**) registers
* Sixteen **108-bit** segment descriptor word associative memory (**`SDWAM`**) registers
* Sixteen **72-bit** control unit (**`CU`**) history registers
* Sixteen **72-bit** operations unit (**`OU`**) history registers
* Sixteen **72-bit** decimal unit (**`DU`**) history registers
* Sixteen **72-bit** appending unit (**`APU`**) history registers
* Five **36-bit** configuration switch data (**`CSD`**) registers
* One **288-bit** control unit data (**`CUD`**) register
* One **288-bit** decimal unit data (**`DUD`**) register

## The DPS8M Simulator

<!-- br -->

### Simulator overview

<!-- br -->

* **DPS8M** is [**open source software**](https://dps8m.gitlab.io/dps8m/License_Information) developed by **The DPS8M Development Team** and **many contributors**.

* **DPS8M** supports most <u>operating systems</u> conforming to *IEEE Std 1003.1-2008* (**POSIX.1-2008**), many <u>C compilers</u> conforming to *ISO/IEC 9899:2011* (**C11**), and numerous <u>hardware architectures</u>.

  * <u>Operating systems</u> supported are **AIX**, **FreeBSD**, **NetBSD**, **OpenBSD**, **DragonFly BSD**, **Haiku**, **GNU/Hurd**, **illumos OpenIndiana**, **Linux**, **macOS**, **Solaris**, **SerenityOS**, and **Windows**.

  * <u>C compilers</u> supported are **Clang**, **LLVM-MinGW**, AMD Optimizing C/C++ (**AOCC**), Arm C/C++ Compiler (**ARMClang**), GNU C (**GCC**), IBM Advance Toolchain (**AT**), IBM XL C/C++ (**XLC**), IBM Open XL C/C++ (**IBMClang**), Intel oneAPI DPC++/C++ (**ICX**), NVIDIA HPC SDK C Compiler (**NVC**), and Oracle Developer Studio (**SunCC**).

  * <u>Hardware architectures</u> actively supported include **Intel x86** (i686, x86_64), **ARM** (ARMv6, ARMv7, ARM64), **LoongArch** (LA64), **MIPS** (MIPS, MIPS64), **OpenRISC** (OR1200, MOR1KX), **PowerPC** (PPC, PPC64, PPC64le), **RISC-V** (RV64), **SPARC** (SPARC, UltraSPARC), **SuperH** (SH-4A), **m68k** (68020+), and **IBM Z** (s390x, z13+).

* Various releases of **DPS8M** have been ported to **embedded systems**, **cell phones**, **tablets**, **handheld gaming consoles**, **wireless routers**, and even modern **mainframes**.  A full-featured port should be possible for any 32- or 64-bit platform with appropriate hardware atomic operations and able to support the [**libuv**](https://libuv.org/) asynchronous I/O library.

* The simulator is distributed as an [**easy-to-build**](https://dps8m.gitlab.io/dps8m/Documentation/Source_Compilation) source code [**distribution**](https://dps8m.gitlab.io/dps8m/Releases) (buildable simply via '**`make`**' on most systems) or as ready-to-run [**pre-compiled binaries**](https://dps8m.gitlab.io/dps8m/Releases) for several popular platforms.

* **DPS8M** development is hosted courtesy of [**GitLab**](https://gitlab.com/dps8m/dps8m), providing [version control](https://gitlab.com/dps8m/dps8m), [issue tracking](https://gitlab.com/dps8m/dps8m/-/issues), and [CI/CD services](https://gitlab.com/dps8m/dps8m/-/pipelines).
[]()

#### Security

* Static application security testing by:
  * [**PVS-Studio**](https://pvs-studio.com/en/pvs-studio/?utm_source=github&utm_medium=organic&utm_campaign=open_source) - A static analysis tool for code quality, security, and safety.
  * [**Coverity® Scan**](https://scan.coverity.com/) - Find and fix defects in Java, C/C++, C#, JavaScript, Ruby, or Python code.
  * [**Oracle Developer Studio**](https://www.oracle.com/application-development/developerstudio/) - Performance, security, and thread analysis tools to help write higher quality code.
  * [**Cppcheck**](https://cppcheck.sourceforge.io/) - A static analysis tool for C/C++ code.
  * [**Clang Static Analyzer**](https://clang-analyzer.llvm.org/) - A source code analysis tool that finds bugs in C, C++, and Objective-C programs.
  * [**Flawfinder**](https://dwheeler.com/flawfinder/) - Examine C/C++ source code for possible security weaknesses.

<!-- pagebreak -->

<!-- br -->

### Supported components

<!-- br -->

**DPS8M** simulates not only the **DPS‑8/M** CPU, but an *entire* Honeywell / Bull ***Distributed Processing System*** mainframe.

<!-- br -->

* *{6×}* Central Processing Units (**CPU**)
* *{8×}* Front-End Network Processors (**FNP**)
* *{4×}* System Control Units (**SCU**)
* *{2×}* Operator Consoles (**OPC**)
* *{2×}* Input/Output Multiplexers (**IOM**)
* *{10×}* Unit Record Processors (**URP**)
* Integrated Peripheral Controllers (**IPC**)
* Magnetic Tape Processors (**MTP**)
* Mass Storage Processors (**MSP**)
* Microprogrammed Peripheral Controllers (**MPC**)
* Socket Controllers (**SKC**)
* ABSI Interfaces
* Tape Drives
* Disk Storage Units
* Printers
* Card Readers
* Card Punches
* DIA cabling

<!-- br -->

### Unsupported components

<!-- br -->

* Diagnostic Processor Unit (**DPU**)
* Chaosnet Interfaces (**MGP**)
* Information Multiplexer Units (**IMU**)
* Maintenance Channel Adapter (**MCA**)
* Remote Maintenance Interface (**RMI**)
* Multidrop Interfaces (**MDI**)
* New System Architecture Extensions (**VU**)
* DPS-6 (*Level-6*) Satellite Processors
* DIA Port Expanders
