<!-- SPDX-License-Identifier: MIT
     Copyright (c) 2016-2024 The DPS8M Development Team -->

The simulator is distributed in various forms, including an easy-to-build [**source code distribution**](https://dps8m.gitlab.io/dps8m/Releases/), which can be built simply via **`make`** on *most* systems.

<!-- start nopdf -->

The following sections document ***only*** some platform-specific details necessary to build the simulator, and are
**not** intended to be a general reference or to replace the
[**full documentation**](https://dps8m.gitlab.io/dps8m/Documentation/).

Review the complete [**DPS8M Omnibus Documentation**](https://dps8m.gitlab.io/dps8m/Documentation/) for additional details.

<!-- br -->

<!-- toc -->

- [General Information](#general-information)

[]()

- [FreeBSD](#freebsd)
  * [FreeBSD prerequisites](#freebsd-prerequisites)
  * [Standard FreeBSD compilation](#standard-freebsd-compilation)
  * [Optimized FreeBSD compilation](#optimized-freebsd-compilation)
  * [blinkenLights2 on FreeBSD](#blinkenlights2-on-freebsd)
  * [Additional FreeBSD Notes](#additional-freebsd-notes)

[]()

- [NetBSD](#netbsd)
  * [NetBSD prerequisites](#netbsd-prerequisites)
  * [Standard NetBSD compilation](#standard-netbsd-compilation)
  * [Optimized NetBSD compilation](#optimized-netbsd-compilation)
  * [Compilation using Clang](#compilation-using-clang)
  * [blinkenLights2 on NetBSD](#blinkenlights2-on-netbsd)

[]()

- [OpenBSD](#openbsd)
  * [OpenBSD prerequisites](#openbsd-prerequisites)
  * [Standard OpenBSD compilation](#standard-openbsd-compilation)
  * [Optimized OpenBSD compilation](#optimized-openbsd-compilation)
  * [Compilation using Clang](#compilation-using-clang-1)
  * [Additional OpenBSD Notes](#additional-openbsd-notes)

[]()

- [DragonFly BSD](#dragonfly-bsd)
  * [DragonFly BSD prerequisites](#dragonfly-bsd-prerequisites)
  * [Standard DragonFly BSD compilation](#standard-dragonfly-bsd-compilation)
  * [Optimized DragonFly BSD compilation](#optimized-dragonfly-bsd-compilation)
  * [Compiling using Clang](#compiling-using-clang)

[]()

- [Solaris](#solaris)
  * [Solaris prerequisites](#solaris-prerequisites)
  * [Solaris compilation](#solaris-compilation)
    + [GCC](#gcc)
    + [Clang](#clang)
    + [Oracle Developer Studio](#oracle-developer-studio)

[]()

- [OpenIndiana (illumos)](#openindiana)
  * [OpenIndiana (illumos) prerequisites](#openindiana-prerequisites)
  * [Standard OpenIndiana (illumos) compilation](#standard-openindiana-compilation)
  * [Compiling using Clang](#compiling-using-clang-1)

[]()

- [AIX](#aix)
  * [AIX prerequisites](#aix-prerequisites)
  * [AIX compilation](#aix-compilation)
    + [IBM Open XL C/C++ for AIX](#ibm-open-xl-cc-for-aix)
    + [IBM XL C/C++ for AIX](#ibm-xl-cc-for-aix)
    + [GCC](#gcc-1)

[]()

- [Haiku](#haiku)
  * [Haiku prerequisites](#haiku-prerequisites)
  * [Standard Haiku compilation](#standard-haiku-compilation)
  * [Compiling using Clang](#compiling-using-clang-2)
  * [Additional Haiku Notes](#additional-haiku-notes)

[]()

- [GNU/Hurd](#gnuhurd)

[]()

- [Linux](#linux)
  * [Linux compilers](#linux-compilers)
  * [Linux prerequisites](#linux-prerequisites)
  * [Standard Linux compilation](#standard-linux-compilation)
  * [Alternative Linux compilation](#alternative-linux-compilation)
    + [Clang](#clang-1)
    + [Intel oneAPI DPC++/C++](#intel-oneapi-dpcc)
    + [AMD Optimizing C/C++](#amd-optimizing-cc)
      - [AOCC with AMD Optimized CPU Libraries](#aocc-with-amd-optimized-cpu-libraries)
    + [Oracle Developer Studio](#oracle-developer-studio-1)
    + [IBM Open XL C/C++ for Linux](#ibm-xl-cc-for-linux)
    + [IBM XL C/C++ for Linux](#ibm-xl-cc-for-linux)
    + [NVIDIA HPC SDK C Compiler](#nvidia-hpc-sdk-c-compiler)
    + [Arm HPC C/C++ Compiler for Linux](#arm-hpc-cc-compiler-for-linux)
      - [ACFL with Arm Performance Libraries](#acfl-with-arm-performance-libraries)
  * [Linux cross-compilation](#linux-cross-compilation)
    + [IBM Advance Toolchain](#ibm-advance-toolchain)
    + [Arm GNU Toolchain](#arm-gnu-toolchain)
      - [Linux/ARMv7-HF](#linuxarmv7-hf)
      - [Linux/ARM64](#linuxarm64)
      - [Linux/ARM64BE](#linuxarm64be)
    + [Linaro GNU Toolchain](#linaro-gnu-toolchain)
      - [Linux/ARMv7-HF](#linuxarmv7-hf-1)
      - [Linux/ARM64](#linuxarm64-1)
    + [crosstool-NG](#crosstool-ng)
      - [Linux/RV64](#linuxrv64)
      - [Linux/i686](#linuxi686)
      - [Linux/ARMv6-HF](#linuxarmv6-hf)
      - [Linux/PPC64le](#linuxppc64le)
  * [Additional Linux Notes](#additional-linux-notes)

[]()

- [macOS](#macos)
  * [macOS prerequisites](#macos-prerequisites)
  * [macOS compilation](#macos-compilation)
    + [Xcode](#xcode)
    + [Intel C/C++ Compiler Classic for macOS](#intel-cc-compiler-classic-for-macos)
  * [macOS cross-compilation](#macos-cross-compilation)
    + [ARM64](#arm64)
    + [Intel](#intel)
    + [Universal](#universal)

[]()

- [Windows](#windows)
  * [Cygwin](#cygwin)
    + [Cygwin prerequisites](#cygwin-prerequisites)
    + [Standard Cygwin compilation](#standard-cygwin-compilation)
    + [Cygwin-hosted cross-compilation to MinGW](#cygwin-hosted-cross-compilation-to-mingw)
      - [Windows i686](#windows-i686)
      - [Windows x86_64](#windows-x86_64)
    + [Unix-hosted cross-compilation to Cygwin](#unix-hosted-cross-compilation-to-cygwin)
  * [MSYS2](#msys2)
  * [Unix-hosted LLVM-MinGW Clang cross-compilation](#unix-hosted-llvm-mingw-clang-cross-compilation)
    + [Windows i686](#windows-i686-1)
    + [Windows x86_64](#windows-x86_64-1)
    + [Windows ARMv7](#windows-armv7)
    + [Windows ARM64](#windows-arm64)
  * [Unix-hosted MinGW-w64 GCC cross-compilation](#unix-hosted-mingw-w64-gcc-cross-compilation)
    + [Windows i686](#windows-i686-2)
    + [Windows x86_64](#windows-x86_64-2)

<!-- tocstop -->

<!-- stop nopdf -->

<!-- br -->

<br>

## General Information

* For optimal performance, building the simulator from source is highly recommended.

[]()

* Building on a **64-bit** platform is **strongly encouraged for optimal performance**.

[]()

* **The DPS8M Development Team** recommends most users download a [**source kit distribution**](https://dps8m.gitlab.io/dps8m/Releases/).
  * A source kit requires approximately **6.5 MiB** of storage space to decompress and **28 MiB** to
    build.

[]()

* Advanced users may prefer to clone the
  [**git repository**](https://gitlab.com/dps8m/dps8m) which contains additional tools not required
  for simulator operation, but useful to developers.
  * The [**git repository**](https://gitlab.com/dps8m/dps8m) requires approximately **175 MiB** of storage
    space to clone and **250 MiB** to build.

<!-- br -->

The following sections document ***only*** platform-specific variations, and are **not** intended to be a general or exhaustive reference.

<br>

---

<br>

<!-- pagebreak -->

## FreeBSD

* Ensure you are running a [supported release](https://www.freebsd.org/releases/) of
  [**FreeBSD**](https://www.freebsd.org/) on a [supported platform](https://www.freebsd.org/platforms/).
  * The current release versions of **FreeBSD**/[**amd64**](https://www.freebsd.org/platforms/amd64/) and **FreeBSD**/[**arm64**](https://www.freebsd.org/platforms/arm/) are regularly tested by **The DPS8M Development Team**.

### FreeBSD prerequisites

Install the required prerequisites (using FreeBSD Packages or Ports):

* Using FreeBSD Packages (as *root*):

  ```sh
  pkg install gmake libuv
  ```

* Using FreeBSD Ports (as *root*):

  ```sh
  cd /usr/ports/devel/gmake/ && make install clean
  cd /usr/ports/devel/libuv/ && make install clean
  ```

### Standard FreeBSD compilation

* Build the simulator (*standard build*) from the top-level source directory (using **GNU Make**):

  ```sh
  gmake
  ```

### Optimized FreeBSD compilation

* **FreeBSD** provides the **Clang** compiler as part of the base system.  While *sufficient* to build
  the simulator, we recommend that version 12 or later of the **GNU C** (`gcc`) compiler be used for
  optimal performance.
* At the time of writing, **GCC 13.1** is available for **FreeBSD** systems and is the version of GCC
  currently recommended by **The DPS8M Development Team**.
  \
  \
  It can be installed via FreeBSD Packages
  or Ports:

  * Using FreeBSD Packages (as *root*):

    ```sh
    pkg install gcc13
    ```

  * Using FreeBSD Ports (as *root*):

    ```sh
    cd /usr/ports/lang/gcc13/ && make install clean
    ```

* Build the simulator from the top-level source directory (using **GNU Make**):

  ```sh
  env CC="gcc13" LDFLAGS="-Wl,-rpath=/usr/local/lib/gcc13" gmake
  ```

### blinkenLights2 on FreeBSD

* To build the **blinkenLights2** front-panel display, install it's prerequisites via FreeBSD Packages or Ports:

  * Using FreeBSD Packages (as *root*):

    ```sh
    pkg install pkgconf gtk3
    ```

  * Using FreeBSD Ports (as *root*):

    ```sh
    cd /usr/ports/devel/pkgconf/ && make install clean
    cd /usr/ports/x11-toolkits/gtk30/ && make install clean
    ```

* Build **blinkenLights2** from the top-level source directory (using **GNU Make**):

  ```sh
  gmake blinkenLights2
  ```

### Additional FreeBSD Notes

* When running on **FreeBSD**, **DPS8M** utilizes [**FreeBSD-specific** atomic operations](https://www.freebsd.org/cgi/man.cgi?query=atomic).

* The **FreeBSD-specific** **`atomic_testandset_64`** operation is currently not implemented in all versions of **FreeBSD** or on all platforms **FreeBSD** supports (*e.g.* **powerpc64**, or **arm64** prior to **13.0-RELEASE**).
  \
  \
  If you are unable to build the simulator because this atomic operation is unimplemented on your platform, specify `ATOMICS="GNU"` as an argument to `gmake`, or export this value in the shell environment before compiling.

<br>

---

<br>

## NetBSD

* Ensure you are running a [supported release](https://www.netbsd.org/releases/formal.html) of
  [**NetBSD**](https://www.netbsd.org) on a [supported platform](https://www.netbsd.org/ports/).
  * **NetBSD**/[**amd64**](https://www.netbsd.org/ports/amd64/) is regularly tested by **The DPS8M Development Team**.

[]()

* **DPS8M** is fully supported on **NetBSD 9.2** or later.

### NetBSD prerequisites

Install the required prerequisites (using NetBSD Packages or [pkgsrc](https://www.pkgsrc.org/)):

* Using NetBSD Packages (as *root*):

  ```sh
  pkgin in gmake libuv
  ```

* Using pkgsrc (as *root*):

  ```sh
  cd /usr/pkgsrc/devel/gmake/ && make install clean
  cd /usr/pkgsrc/devel/libuv/ && make install clean
  ```

### Standard NetBSD compilation

* Build the simulator (*standard build*) from the top-level source directory (using **GNU Make**):

  ```sh
  gmake
  ```

### Optimized NetBSD compilation

* **NetBSD** provides an older version of **GCC** (or **Clang**) as part of the base system (depending on the platform).
  While *sufficient* to build the simulator, we recommend that version 12 or later of the **GNU C** (`gcc`) compiler
  be used for optimal performance.

* At the time of writing, **GCC 13.2** is available for **NetBSD 9** systems and is the version of GCC currently
  recommended by **The DPS8M Development Team**.
  \
  \
  It can be installed via Packages or pkgsrc.

  * Using NetBSD Packages (as *root*):

    ```sh
    pkgin in gcc13
    ```

  * Using pkgsrc (as *root*):

    ```sh
    cd /usr/pkgsrc/lang/gcc13/ && make install clean
    ```

* Build the simulator from the top-level source directory (using **GNU Make**):

  ```sh
  env CC="/usr/pkg/gcc13/bin/gcc" LDFLAGS="Wl,-rpath=/usr/pkg/gcc13/lib" gmake
  ```

### Compilation using Clang

* **GCC** is recommended for optimal performance, but compilation using **Clang** (and linking
  using **LLD**, the LLVM linker) is supported.
  \
  \
  Both **Clang** and **LLD** can be installed via
  Packages or pkgsrc.

  * Using NetBSD Packages (as *root*):

    ```sh
    pkgin in clang lld
    ```

  * Using pkgsrc (as *root*):

    ```sh
    cd /usr/pkgsrc/lang/clang/ && make install clean
    cd /usr/pkgsrc/devel/lld/ && make install clean
    ```

* Build the simulator from the top-level source directory (using **GNU Make**):

  ```sh
  env CC="clang" \
    LDFLAGS="-L/usr/lib -L/usr/pkg/lib -fuse-ld="$(command -v ld.lld)" gmake
  ```

### blinkenLights2 on NetBSD

* To build the **blinkenLights2** front-panel display, install it's prerequisites via NetBSD Packages or pkgsrc:

  * Using NetBSD Packages (as *root*):

    ```sh
    pkg install pkgconf gtk3+
    ```

  * Using pkgsrc (as *root*):

    ```sh
    cd /usr/pkgsrc/devel/pkgconf/ && make install clean
    cd /usr/pkgsrc/x11/gtk3/ && make install clean
    ```

* Build **blinkenLights2** from the top-level source directory (using **GNU Make**):

  ```sh
  gmake blinkenLights2
  ```

<br>

---

<br>

## OpenBSD

* Ensure you are running an [up-to-date](https://man.openbsd.org/syspatch.8) and
  [supported release](https://www.openbsd.org/) of [**OpenBSD**](https://www.openbsd.org/) on a
  [supported platform](https://www.openbsd.org/plat.html).
  * **OpenBSD**/[**amd64**](https://www.openbsd.org/amd64.html) and
    **OpenBSD**/[**arm64**](https://www.openbsd.org/arm64.html) are regularly tested by
    **The DPS8M Development Team**.

[]()

* The following instructions were verified using **OpenBSD 7.4** on
  [**amd64**](https://www.openbsd.org/amd64.html) and
  [**arm64**](https://www.openbsd.org/arm64.html).

### OpenBSD prerequisites

Install the required prerequisites (using OpenBSD Packages or Ports):

* Using OpenBSD Packages (as *root*):

  ```sh
  pkg_add gmake libuv
  ```

* Using OpenBSD Ports (as *root*):

  ```sh
  cd /usr/ports/devel/gmake/ && make install clean
  cd /usr/ports/devel/libuv/ && make install clean
  ```

### Standard OpenBSD compilation

* Build the simulator (*standard build*) from the top-level source directory (using **GNU Make**):

  ```sh
  gmake
  ```

### Optimized OpenBSD compilation

* **OpenBSD** provides an older version of **GCC** (or **Clang**) as part of the base system (depending on the platform).
  While *sufficient* to build the simulator, we recommend that a recent version of the **GNU assembler** (`gas`)
  and version 11 or later of the **GNU C** (`gcc`) compiler be used for optimal performance.

* At the time of writing, appropriate versions of the **GNU assembler** and **GNU C** (version **11.2**)
  are available for **OpenBSD**.  (In addition, **LLD**, the LLVM linker, may be required.)  These tools have
  been tested and are highly recommended by **The DPS8M Development Team**.
  \
  \
  They can be installed via OpenBSD Packages or Ports:

  * Using OpenBSD Packages (as *root*):

    ```sh
    pkg_add gas gcc
    ```

  * Using OpenBSD Ports (as *root*):

    ```sh
    cd /usr/ports/devel/gas/ && make install clean
    cd /usr/ports/lang/gcc/11/ && make install clean
    ```

* LLVM **LLD** 16.0.6 or later is recommended for linking, even when using **GCC 11.2** for compilation.
  **LLD** is the default linker on ***most*** (*but not all*) supported OpenBSD platforms.  To determine the linker
  in use, examine the output of '`ld --version`'.
  \
  \
  If the linker identifies itself by a name *other* than **LLD**
  (*e.g.* "**`GNU ld`**" or similar), **LLD** must be installed via OpenBSD Packages or Ports.

  * Using OpenBSD Packages (as *root*):

    ```sh
    pkg_add llvm
    ```

  * Using OpenBSD Ports (as *root*):

    ```sh
    cd /usr/ports/devel/llvm/ && make install clean
    ```

* Configure **GCC** to execute the correct assembler and/or linker:

  ```sh
  mkdir -p ~/.override
  test -x /usr/local/bin/gas && ln -fs /usr/local/bin/gas ~/.override/as
  test -x /usr/local/bin/lld && ln -fs /usr/local/bin/lld ~/.override/ld
  ```

* Build the simulator from the top-level source directory (using **GNU Make**):

  ```sh
  env CC="egcc" CFLAGS="-B ~/.override" gmake
  ```

### Compilation using Clang

* **GCC** is recommended for optimal performance, but compilation using **Clang** is supported.
* A version of **Clang** newer than the base system version may be available via the '**`llvm`**' package or port.
* Once installed, it can be used for compilation by setting `CC=/usr/local/bin/clang`.

### Additional OpenBSD Notes

* At the time of writing, **OpenBSD**/[**luna88k**](https://www.openbsd.org/luna88k.html) has not been tested.

<br>

---

<br>

## DragonFly BSD

* At the time of writing, [**DragonFly BSD 6.4.0**](https://www.dragonflybsd.org/download/) was current and used to verify the following instructions.

### DragonFly BSD prerequisites

* Install the required prerequisites using DragonFly BSD DPorts (as *root*):

  ```sh
  pkg install gmake libuv
  ```

### Standard DragonFly BSD compilation

* Build the simulator (*standard build*) from the top-level source directory (using **GNU Make**):

  ```sh
  env CFLAGS="-I/usr/local/include"  \
       LDFLAGS="-L/usr/local/lib"    \
       ATOMICS="GNU"                 \
    gmake
  ```

### Optimized DragonFly BSD compilation

* **DragonFly BSD** provides an older version of **GCC** as part of the base system.  While this compiler is
  *sufficient* to build the simulator, we recommend that version 12 or later of the **GNU C** (`gcc`)
  compiler be used for optimal performance.

* At the time of writing, **GCC 13.1** is available for DragonFly BSD and recommended by **The DPS8M Development Team**.

  * **GCC 13.1** may be installed using DragonFly BSD DPorts (as *root*):

    ```sh
    pkg install gcc13
    ```

* Build the simulator from the top-level source directory (using **GNU Make**):

  ```sh
  env CC="gcc12" CFLAGS="-I/usr/local/include"                    \
      LDFLAGS="-L/usr/local/lib -Wl,-rpath=/usr/local/lib/gcc13"  \
      ATOMICS="GNU"                                               \
    gmake
  ```

### Compiling using Clang

* **GCC** is recommended for optimal performance, but compilation using **Clang** is supported.
* At the time of writing, **Clang 16** is available for DragonFly BSD and recommended by **The DPS8M Development Team**.
* While some optional utilities may fail to build using **Clang** on DragonFly, the simulator (`src/dps8/dps8`) is fully tested with each DragonFly release.

  * **Clang 16** may be installed using DragonFly BSD DPorts (as *root*):

    ```sh
    pkg install llvm16
    ```

* Build the simulator from the top-level source directory (using **GNU Make**):

  ```sh
  env CC="clang16" CFLAGS="-I/usr/include -I/usr/local/include"  \
      LDFLAGS="-L/usr/lib -L/usr/local/lib -fuse-ld=lld"         \
      ATOMICS="GNU"                                              \
    gmake
  ```

<br>

---

<br>

## Solaris

* Ensure your **Solaris** installation is reasonably current. [**Oracle Solaris**](https://www.oracle.com/solaris) **11.4 SRU42** or later is recommended.

[]()

* The simulator can be built on **Solaris** using the **GCC**, **Clang**, and **Oracle Developer Studio** compilers.
  * **GCC 11** is the recommended compiler for optimal performance on all Intel-based **Solaris** systems.
    * **GCC 11** can be installed from the standard IPS repository via '**`pkg install gcc-11`**'.
  * Link-time optimization (*LTO*) is supported ***only*** when building with **GCC** version 10 or later.
    * The `NO_LTO=1` build option should be specified when using earlier versions of the **GCC** compiler.
  * Building with **Clang 11** or later is also supported (*but not recommended due to lack of LTO support*).
    * **Clang 11** can be installed from the standard IPS repository via '**`pkg install clang@11 llvm@11`**'.
  * Building with the **Oracle Developer Studio 12.6** (`suncc`) compiler is also supported.

[]()

* Note that building for **Solaris** using the **Oracle Developer Studio** compiler currently requires a non-trivial amount of `CFLAGS` to be specified. This will be simplified in a future release of the simulator.

### Solaris prerequisites

* Install the required prerequisites from the standard IPS repository (as *root*):

  ```sh
  pkg install gnu-make gnu-binutils gnu-sed gnu-grep gnu-tar gawk \
    gnu-coreutils pkg-config libtool autoconf automake wget
  ```

### Solaris compilation

The following commands will download and build a local static **`libuv`** before compiling the simulator.

If a site-provided **`libuv`** library has been installed (in the "**`/usr/local`**" prefix), the
**`libuvrel`** stage of the build may be omitted.

Build **`libuv`** and the simulator from the top-level source directory (using **GNU Make**):

#### GCC

* Build using **GCC**:

  * Build `libuv`:

    ```sh
    env TAR="gtar" TR="gtr" CC="gcc" gmake libuvrel
    ```

  * Build the simulator:

    ```sh
    env TR="gtr" CC="gcc" gmake
    ```

#### Clang

* Build using **Clang**:

  * Build `libuv`:

    ```sh
    env TAR="gtar" NO_LTO=1 TR="gtr" CC="clang" gmake libuvrel
    ```

  * Build the simulator:

    ```sh
    env NO_LTO=1 TR="gtr" CC="clang" gmake
    ```

#### Oracle Developer Studio

* Build using **Oracle Developer Studio 12.6**:

  * Build `libuv`:

    ```sh
    env TAR="gtar" NO_LTO=1 SUNPRO=1 NEED_128=1 TR="gtr" CSTD="c11"        \
        CFLAGS="-DNO_C_ELLIPSIS -Qy -xO5 -m64 -xlibmil -xCC -mt -xlibmopt  \
        -fno-semantic-interposition -xprefetch=auto -xprefetch_level=3"    \
        CC="/opt/developerstudio12.6/bin/suncc"                            \
      gmake libuvrel
    ```

  * Build the simulator:

    ```sh
    env NO_LTO=1 SUNPRO=1 NEED_128=1 TR="gtr" CSTD="c11"                   \
        CFLAGS="-DNO_C_ELLIPSIS -Qy -xO5 -m64 -xlibmil -xCC -mt -xlibmopt  \
        -fno-semantic-interposition -xprefetch=auto -xprefetch_level=3"    \
        CC="/opt/developerstudio12.6/bin/suncc"                            \
      gmake
    ```

<br>

---

<br>

## OpenIndiana

* Ensure your [**OpenIndiana**](https://www.openindiana.org/) installation is up-to-date.
  * **OpenIndiana** *Hipster* **January 2024** was used to verify these instructions.

[]()

* **GCC 13.2** is currently the recommended compiler for optimal performance.
  * **GCC 13.2** can be installed from the standard IPS repository via '**`pkg install gcc-13`**'.
  * Link-time optimization (*LTO*) is supported ***only*** when building with **GCC** version 10 or later.
  * The `NO_LTO=1` build option should be specified when using earlier versions of the **GCC** compiler.

[]()

* Building with **Clang** (version 13 or later) is also supported (*but not recommended due to lack of LTO support*).
  * **Clang 17** can be installed from the standard IPS repository via '**`pkg install clang-17`**'.

### OpenIndiana prerequisites

* Install the required prerequisites from the standard IPS repository (as *root*):

  ```sh
  pkg install gnu-make libuv gnu-binutils gnu-coreutils
  ```

### Standard OpenIndiana compilation

* Build the simulator from the top-level source directory (using **GNU Make** and **GCC 13**):

  ```sh
  env CC="gcc-13" gmake
  ```

### Compiling using Clang

* Build the simulator from the top-level source directory (using **GNU Make** and **Clang 17**):

  ```sh
  env NO_LTO=1 CC="clang-17" gmake
  ```

<br>

---

<br>

## AIX

* Ensure you are running a [supported release](https://www.ibm.com/support/pages/aix-support-lifecycle-information) of [**IBM AIX®**](https://www.ibm.com/products/aix) on a [supported platform](https://www.ibm.com/support/pages/system-aix-maps).
  * **IBM AIX®** **7.2** (*TL5* *SP7*) and **7.3** (*TL2* *SP1*) on [**POWER8®**, **POWER9™**, and **Power10**](https://www.ibm.com/it-infrastructure/power) systems are regularly tested by **The DPS8M Development Team**.

[]()

* The simulator can be built for **64-bit** **AIX®** using [**IBM XL C/C++ for AIX**](https://www.ibm.com/products/xl-c-aix-compiler-power) (**`xlc`**), [**IBM Open XL C/C++ for AIX**](https://www.ibm.com/products/open-xl-cpp-aix-compiler-power) (**`ibm-clang`**), or **GNU C** (**`gcc`**).
* **The DPS8M Development Team** recommends building with **IBM Open XL C/C++ V17.1.2** (or later) or **GCC 11** (or later) for optimal performance.
* [**IBM Open XL C/C++ for AIX V17.1.2 Fix Pack 2** (February 2024)](https://www.ibm.com/products/open-xl-cpp-aix-compiler-power) is the *minimum* recommended version of the **Open XL C/C++** compiler on **POWER8**, **POWER9**, and **Power10** systems.
* [**IBM XL C/C++ for AIX V16.1.0 Fix Pack 16** (February 2024)](https://www.ibm.com/support/pages/ibm-xl-cc-aix-161) is the *minimum* recommended version of the **IBM XL C/C++** compiler on **POWER8** and **POWER9** systems.

[]()

* Ensure that you are using the [latest available XL compiler and associated PTF updates](https://www.ibm.com/support/pages/aix-os-levels-supported-xl-compilers) for your **IBM AIX** OS level.

[]()

* When building the simulator using **GNU C**, it recommended to use **GCC 11** (or later) for optimal performance.
  * **GCC 11** can be installed from the [IBM AIX Toolbox for Open Source Software](https://www.ibm.com/support/pages/aix-toolbox-open-source-software-overview) repository.

[]()

* Note that building for **IBM AIX** currently requires a non-trivial number of options to be specified *after* the `gmake` command, which overrides various build defaults appropriate for **Linux**, **macOS**, and **BSD** systems, but not IBM **AIX**.  This will be simplified in a future release of the simulator.

### AIX prerequisites

* Install the required prerequisites from the [IBM AIX Toolbox for Open Source Software](https://www.ibm.com/support/pages/aix-toolbox-open-source-software-overview) repository (as *root*):

  ```sh
  /opt/freeware/bin/dnf install sed gmake libuv libuv-devel popt coreutils gawk
  ```

* *Optionally* install **GCC 11** (or **GCC 12** on **AIX 7.3**) from the [IBM AIX Toolbox for Open Source Software](https://www.ibm.com/support/pages/aix-toolbox-open-source-software-overview) repository (as *root*):

  * Install **GCC 11**:

    ```sh
    /opt/freeware/bin/dnf install gcc gcc11
    ```

  * Install **GCC 12** (on **AIX 7.3**):

    ```sh
    /opt/freeware/bin/dnf install gcc gcc12
    ```

[]()

* GCC availability from the [IBM AIX Toolbox for Open Source Software](https://www.ibm.com/support/pages/aix-toolbox-open-source-software-overview) site varies depending on **IBM AIX** version.
* IBM does not provide support for **GCC** through **IBM AIX** support cases; we recommend using **Open XL C/C++** where possible.

### AIX compilation

Build the simulator from the top-level source directory (using **GNU Make**):

#### IBM Open XL C/C++ for AIX

* Using **IBM Open XL C/C++ for AIX V17.1.2**:

  ```sh
  env PATH="/opt/freeware/bin:${PATH}"                                     \
      CC="/opt/IBM/openxlC/17.1.2/bin/ibm-clang_r"                         \
      ATOMICS="AIX"                                                        \
      AWK="gawk"                                                           \
      OBJECT_MODE=64                                                       \
      NEED_128=1                                                           \
    gmake PULIBS="-lpopt"                                                  \
          LDFLAGS="-L/opt/freeware/lib -L/usr/local/lib -flto=auto -b64"   \
          LIBS="-lpthread -luv -lbsd -lm"                                  \
          CFLAGS="-flto=auto -I/opt/freeware/include -I/usr/local/include  \
                  -I../simh -I../decNumber -DUSE_FLOCK=1 -DUSE_FCNTL=1     \
                  -DHAVE_POPT=1 -DNEED_128=1 -DAIX_ATOMICS=1 -m64          \
                  -DLOCKLESS=1 -D_ALL_SOURCE -D_GNU_SOURCE -O3             \
                  -U__STRICT_POSIX__ -fno-strict-aliasing -mcpu=power8"
  ```

  * When building on IBM **POWER9** (or **Power10**) systems, ‘`-mcpu=power9`’ (*or* ‘`-mcpu=power10`’) should replace ‘`-mcpu=power8`’ in the above compiler invocation.

  * Refer to the [**IBM Open XL C/C++ for AIX V17.1.2 documentation**](https://www.ibm.com/docs/en/openxl-c-and-cpp-aix/17.1.2) for additional information.

#### IBM XL C/C++ for AIX

* Using **IBM XL C/C++ for AIX V16.1.0**:

  ```sh
  env PATH="/opt/freeware/bin:${PATH}"                                      \
      ATOMICS="AIX"                                                         \
      AWK="gawk"                                                            \
      NO_LTO=1                                                              \
      OBJECT_MODE=64                                                        \
    gmake CC="/opt/IBM/xlC/16.1.0/bin/xlc_r"                                \
          NEED_128=1                                                        \
          USE_POPT=1                                                        \
          PULIBS="-lpopt"                                                   \
          LDFLAGS="-L/opt/freeware/lib -L/usr/local/lib -b64"               \
          LIBS="-luv -lbsd -lm"                                             \
          CFLAGS="-O3 -qhot -qarch=pwr8 -qalign=natural -qtls -DUSE_POPT=1  \
                  -DUSE_FLOCK=1 -DUSE_FCNTL=1 -DAIX_ATOMICS=1 -DNEED_128=1  \
                  -DLOCKLESS=1 -I/opt/freeware/include -I../simh            \
                  -I../decNumber -I/usr/local/include -D_GNU_SOURCE         \
                  -D_ALL_SOURCE -U__STRICT_POSIX__"
  ```

  * When building on **POWER9** systems, '`-qarch=pwr9`' should replace '`-qarch=pwr8`' in the above compiler invocation.

  * Compilation using higher optimization levels
    (*i.e.* '`-O4`' or '`-O5`' replacing '`-O3 -qhot -qarch=pwr8`') and/or enabling
    automatic parallelization (*i.e.* '`-qsmp`') *may* be possible, but the resulting
    binaries have *not* been benchmarked or extensively tested by **The DPS8M Development Team**.

  * Refer to the [**IBM XL C/C++ for AIX V16.1 Optimization and Tuning Guide**](https://www.ibm.com/docs/en/xl-c-and-cpp-aix/16.1?topic=category-optimization-tuning) for additional information.

#### GCC

* Using **GCC 11**:

  ```sh
  env PATH="/opt/freeware/bin:${PATH}" CC="gcc-11" \
    ATOMICS="AIX" NO_LTO=1 gmake
  ```

* Using **GCC 12**:

  ```sh
  env PATH="/opt/freeware/bin:${PATH}" CC="gcc-12" \
    ATOMICS="AIX" NO_LTO=1 gmake
  ```

<br>

---

<br>

## Haiku

* Ensure you are running a recent release of [**Haiku**](https://www.haiku-os.org/) on a [supported **64-bit** platform](https://www.haiku-os.org/guides/building/port_status).
  * Use the '**SoftwareUpdater**' application to ensure your **Haiku** installation is up-to-date.
* **The DPS8M Development Team** regularly tests the simulator using the [nightly **Haiku** **x86_64** snapshots](https://download.haiku-os.org/nightly-images/x86_64/).
  * **Haiku** **x86_64** (**`hrev57498`**) was used to verify the following instructions.

### Haiku prerequisites

The default **Haiku** installation includes the required header files, the recommended compiler (at the time of writing, **GCC 13.2**), and most of the necessary development utilities (*i.e.* **GNU Make**) required to build **DPS8M**. The remaining prerequisites are available via the standard package management tools.

* Install the required prerequisites (from *HaikuPorts* using `pkgman` or the '**HaikuDepot**' application):
   * `libuv`
   * `libuv_devel`
   * `getconf`

### Standard Haiku compilation

* Build the simulator (*standard build*) from the top-level source directory (using **GNU Make**):

  ```sh
  make
  ```

### Compiling using Clang

* Building with **GCC** is strongly recommended for optimal performance, but compilation using **Clang** is also supported (*although not recommended, due to the lack of support for LTO optimization*).
  * At the time of writing, **Clang 17** is available from *HaikuPorts* (as the '**`llvm17_clang`**' and '**`llvm17_lld`**' packages), installable using `pkgman` or the '**HaikuDepot**' application.

* Build the simulator from the top-level source directory (using **GNU Make**):

  ```sh
  env CSTD="c11" CC="clang" CFLAGS="-fPIC" NO_LTO=1 make
  ```

### Additional Haiku Notes

* **DPS8M** on **32-bit** **Haiku** platforms (*i.e.* **x86**, **x86_gcc2**) is **not** supported at this time.

<br>

---

<br>

## GNU/Hurd

* **DPS8M** is supported on [**GNU/Hurd**](https://www.gnu.org/software/hurd/) when using [**Debian GNU/Hurd 2021**](https://www.debian.org/ports/hurd/) (or later).

[]()

* **GCC 11** (or later) is the recommended compiler for optimal performance.
  * Compilation is also supported using **Clang 11** or later.

[]()

* Building on **GNU/Hurd** should be essentially the same as building on [**Debian GNU/Linux**](#linux).

<br>

---

<br>

## Linux

* Most major **Linux** distributions using the [**GNU C Library**](https://www.gnu.org/software/libc/), [**Bionic**](https://developer.android.com/), [**uClibc-ng**](https://uclibc-ng.org/), and [**musl-libc**](https://musl.libc.org/) are supported.
  * [**Debian GNU/Linux**](https://www.debian.org/) and derivatives ([**Raspberry Pi OS**](https://www.raspberrypi.com/software/)), **Red Hat** variants ([**Fedora**](https://fedoraproject.org/), [**CentOS Stream**](https://www.centos.org/centos-stream/), [**RHEL**](https://www.redhat.com/en/technologies/linux-platforms/enterprise-linux)) and compatibles ([**AlmaLinux**](https://almalinux.org/), [**Amazon Linux**](https://aws.amazon.com/amazon-linux-2/), [**Oracle Linux**](https://www.oracle.com/linux/)),  [**Alpine**](https://www.alpinelinux.org/), **SUSE** ([**SLES**](https://www.suse.com/products/server/), [**OpenSUSE**](https://www.opensuse.org/)), [**Void**](https://voidlinux.org/), and [**Ubuntu**](https://ubuntu.com/) are regularly tested on **Intel**, **ARM**, and **POWER** systems.

### Linux compilers

* **GCC** **12** or later is recommended for optimal performance on most architectures including **Intel** and **ARM**.
  * **The DPS8M Development Team** regularly tests and supports a wide range of Linux compilers, including **Clang**, AMD Optimizing C/C++ (**AOCC**), Arm C/C++ Compiler (**ARMClang**), GNU C (**GCC**) (*version* **9**+), IBM Advance Toolchain for Linux, IBM XL C/C++ for Linux (**XLC**), IBM Open XL C/C++ for Linux (**IBMClang**), Intel oneAPI DPC++/C++ (**ICX**), NVIDIA HPC SDK C Compiler (**NVC**), and Oracle Developer Studio (**SunCC**).

  * **Red Hat** offers the [**Red Hat Developer Toolset**](https://developers.redhat.com/products/developertoolset/) for **Red Hat Enterprise Linux** and **CentOS Stream**, which provides up-to-date versions of **GCC** on a rapid release cycle, with *full support*.
    * The *Toolset* packages are also included in various "clone" distributions such as **AlmaLinux**. These tools are regularly tested and highly recommended by **The DPS8M Development Team**. Check your distribution packager manager (*i.e.* '**`dnf search`**') for packages named '**`gcc-toolset-12`**', '**`gcc-toolset-13`**', or similar.

  * **Canonical** similarly offers two [**Ubuntu Toolchain PPAs**](https://wiki.ubuntu.com/ToolChain#Toolchain_Updates), one providing **GCC** updates for release branches, and the other providing new **GCC** versions for both current and **LTS** releases, maintained by the Ubuntu Toolchain team.
    * For example, at the time of writing, Ubuntu 22.04 LTS ships **GCC 11.3** and **GCC 12.1**, and the **Toolchain PPAs** ship **GCC 12.3** and **GCC 13.1**. Although these packages are *not formally supported* by Canonical, they are regularly and successfully used by **The DPS8M Development Team**.

  * **Intel®** **C++ Compiler** **Classic** (**ICC**) **for Linux** is ***no longer supported*** for building **DPS8M** (*as of* ***R3.0.0***):
    * Users should upgrade to the current version of the [**Intel® oneAPI DPC++/C++** (**ICX**) **Compiler**](https://www.intel.com/content/www/us/en/developer/tools/oneapi/dpc-compiler.html).
    * **ICC** remains a supported compiler for building **DPS8M** on **Intel**-based **macOS** systems **only**.
    * Intel® has *retired* support for **ICC**.

* Cross-compilation is supported. Popular targets including various **Linux** platforms, **Microsoft Windows** on **Intel** and **ARM** (using the **MinGW-w64** and **LLVM-MinGW** toolchains) and **Linux on POWER** (using the **IBM Advance Toolchain for Linux**) are regularly built and tested.


### Linux prerequisites

Users of some **Red Hat** variants may need to enable the **PowerTools** repository or the **CodeReady Builder** AppStream to install **`libuv`**:

* RHEL 8:

  ```sh
  subscription-manager repos --enable \
    "codeready-builder-for-rhel-8-$(arch)-rpms"
  ```

* CentOS Stream 8:

  ```sh
  dnf config-manager --set-enabled "powertools"
  ```

* RHEL 9:

  ```sh
  subscription-manager repos --enable \
    "codeready-builder-for-rhel-9-$(arch)-rpms"
  ```

* CentOS Stream 9:

  ```sh
  dnf config-manager --set-enabled "crb"
  ```

Install the required prerequisites using a distribution package manager:

* Using **dnf** (for most **rpm**-based distributions) (as *root*):

  ```sh
  dnf install "@Development Tools" libuv-devel
  ```
* Using **apt** (for most **deb**-based distributions) (as *root*):

  ```sh
  apt install build-essential libuv1-dev
  ```

### Standard Linux compilation

* Build the simulator (*standard build*) from the top-level source directory (using **GNU Make**):

  ```sh
  make
  ```

### Alternative Linux compilation

To use a compiler other than the default it is *usually* sufficient to simply set the **`CC`** environment variable (*if* the compiler accepts command-line arguments compatible with **GCC** or **Clang**). Other compilers are supported as well, but require additional configuration.

Examples of building the simulator on **Linux** using various popular compilers follows:

#### Clang

* Build the simulator using **Clang** (**Clang 15** *or later recommended*):

  ```sh
  env CC="clang" make
  ```

#### Intel oneAPI DPC++/C++

* Build the simulator using **Intel oneAPI DPC++/C++** (**ICX**):

  ```sh
  source /opt/intel/oneapi/setvars.sh && \
  env CC="icx" make
  ```

#### AMD Optimizing C/C++

* Build the simulator using **AMD Optimizing C/C++** (**AOCC**), version 4.1.0, (with **AOCC**-provided **AMD LibM**):

  ```sh
  export AOCCVER="4.1.0" &&                               \
  export AOCLPATH="/opt/AMD/aocc-compiler-${AOCCVER}" &&  \
  source ${AOCCPATH}/setenv_AOCC.sh &&                    \
  env CC="clang" CFLAGS="-mllvm -vector-library=AMDLIBM"  \
      LDFLAGS="-Wno-unused-command-line-argument"         \
      LOCALLIBS="-lalm"                                   \
    make
  ```

##### AOCC with AMD Optimized CPU Libraries

* Build the simulator using **AMD Optimizing C/C++** (**AOCC**), version 4.1.0, with **AMD Optimized CPU Libraries** (**AOCL**) (**AMD AOCL-LibM** and **AMD AOCL-LibMem**), version 4.1.0:

  ```sh
  export AOCCVER="4.1.0" &&                                               \
  export AOCCPATH="/opt/AMD/aocc-compiler-${AOCCVER}" &&                  \
  export AOCLVER="4.1.0" &&                                               \
  export AOCLPATH="/opt/AMD/aocl/aocl-linux-aocc-${AOCLVER}" &&           \
  export LD_LIBRARY_PATH="${AOCLPATH}/lib:${LD_LIBRARY_PATH}" &&          \
  source ${AOCCPATH}/setenv_AOCC.sh &&                                    \
  env CC="clang" CFLAGS="-mllvm -vector-library=AMDLIBM"                  \
      LDFLAGS="-Wno-unused-command-line-argument -L${AOCLPATH}/aocc/lib"  \
      LOCALLIBS="-lalm -laocl-libmem"                                     \
    make
  ```

#### Oracle Developer Studio

* Build the simulator using **Oracle Developer Studio** (**SunCC**) for Linux, version 12.6:

  ```sh
  env CFLAGS="-DNO_C_ELLIPSIS -Qy -xO5 -m64 -xlibmil -xCC -mt  \
              -xlibmopt -fno-semantic-interposition            \
              -xprefetch=auto -xprefetch_level=3"              \
      CC="/opt/oracle/developerstudio12.6/bin/suncc"           \
      NO_LTO=1 SUNPRO=1 NEED_128=1 CSTD="c11"                  \
    make
  ```

#### IBM Open XL C/C++ for Linux

* Build the simulator using **IBM Open XL C/C++ for Linux V17.1.1 Fix Pack 2** (*February 2024*) for Linux on POWER:

  ```sh
  env CFLAGS="-mcpu=power8"                         \
      CC="/opt/ibm/openxlC/17.1.1/bin/ibm-clang_r"  \
    make
  ```

* When building on IBM **POWER9** (or **Power10**) systems, ‘`-mcpu=power9`’ (*or* ‘`-mcpu=power10`’) should replace ‘`-mcpu=power8`’ in the above compiler invocation.

* Refer to the [**IBM Open XL C/C++ for Linux V17.1.1 documentation**](https://www.ibm.com/docs/en/openxl-c-and-cpp-lop/17.1.1) for additional information.

#### IBM XL C/C++ for Linux

* Build the simulator using **IBM XL C/C++ for Linux V16.1.1 Fix Pack 15** (*September 2023*) for Linux on POWER:

  ```sh
  env CFLAGS="-qtls -qarch=pwr8"          \
      CC="/opt/ibm/xlC/16.1.1/bin/c99_r"  \
      CSTD="c11" NO_LTO=1                 \
    make
  ```

  * When building on **POWER9** or higher systems, '`-qarch=pwr9`' should replace '`-qarch=pwr8`' in the above compiler invocation.

  * Compilation using higher optimization levels (*e.g.* '`-O4`', '`-O5`', '`-qhot`', *etc.*) and/or enabling automatic parallelization (*i.e.* '`-qsmp`') may be possible, but the resulting binaries have *not* been benchmarked or tested by **The DPS8M Development Team**.

#### NVIDIA HPC SDK C Compiler

* Build the simulator using **NVIDIA HPC SDK C Compiler** (**NVC**), version 24.1, for Linux/**x86_64** (also available for Linux/**ARM64** and Linux/**OpenPOWER**):

  ```sh
  export NVCVER="24.1" &&                                             \
  export NVCPATH="/opt/nvidia/hpc_sdk/Linux_x86_64/${NVCVER}/bin" &&  \
  env CFLAGS="-noswitcherror" CC="${NVCPATH}/nvc" NO_LTO=1            \
    make OPTFLAGS="-fast -O4"
  ```

  * The **NVIDIA HPC SDK C Compiler** is the successor to the **PGI C Compiler** product. If you are using the earlier **PGI C Compiler** (**PGCC**), adjust paths appropriately, and replace '**`nvc`**' with '**`pgcc`**' in the above invocation.

  * **DPS8M** is known to trigger bugs in many versions of the **PGCC** and **NVC** compilers, such as:
    ```text
    NVC++-F-0000-Internal compiler error. add_cilis(): bad jmp code 1056
    ```
    If you encounter this (*or similar*) compiler errors, try adding '`-Mnovect`' to '`OPTFLAGS`' as a workaround.

#### Arm HPC C/C++ Compiler for Linux

The **Arm HPC C/C++ Compiler for Linux** with **Arm Performance Libraries** (*also available as a component of* **Arm Allinea Studio**) provides a packaged **Clang**/**LLVM**-based toolchain with optimized math and string libraries, validated against common ARM HPC platforms.

Note the following examples *do not* make use of [**Environment Modules**](https://modules.sourceforge.net/) and/or [**Lmod**](https://lmod.readthedocs.org), commonly used to manage compiler and development tool installations in HPC environments.

If your site uses modules (*i.e.* `module avail`), loading the appropriate module is usually preferred to setting paths manually.  Contact your system administrator for site-specific configuration details and recommended local compiler flags.

* Build the simulator using the [**Arm HPC C/C++ Compiler for Linux** (**ARMClang**)](https://developer.arm.com/Tools%20and%20Software/Arm%20Compiler%20for%20Linux), version 23.10, for Linux/**ARM64**:

  ```sh
  export ACFLVER="23.10" &&                                 \
  export ACFLCMP="arm-linux-compiler-${ACFLVER}" &&         \
  export ACFLTYP="Generic-AArch64_RHEL-9_aarch64-linux" &&  \
  export ACFLPATH="/opt/arm/${ACFLCMP}_${ACFLTYP}" &&       \
  export PATH="${ACFLPATH}/bin:${PATH}" &&                  \
  env CFLAGS="-mcpu=native"                                 \
      CC="armclang"                                         \
    make
  ```

##### ACFL with Arm Performance Libraries

* Build the simulator using the [**Arm HPC C/C++ Compiler for Linux** (**ARMClang**)](https://developer.arm.com/Tools%20and%20Software/Arm%20Compiler%20for%20Linux) with the integrated [**Arm Performance Libraries** (**ArmPL**)](https://developer.arm.com/Tools%20and%20Software/Arm%20Performance%20Libraries), version 23.10, for Linux/**ARM64**:

  ```sh
  export ACFLVER="23.10" &&                                 \
  export ACFLCMP="arm-linux-compiler-${ACFLVER}" &&         \
  export ACFLTYP="Generic-AArch64_RHEL-9_aarch64-linux" &&  \
  export ACFLPATH="/opt/arm/${ACFLCMP}_${ACFLTYP}" &&       \
  export PATH="${ACFLPATH}:${PATH}" &&                      \
  env CFLAGS="-mcpu=native -armpl"                          \
      LDFLAGS="-armpl"                                      \
      CC="armclang"                                         \
    make OPTFLAGS="-Ofast"
  ```

* Build the simulator using the [**Arm HPC C/C++ Compiler for Linux** (**ARMClang**)](https://developer.arm.com/Tools%20and%20Software/Arm%20Compiler%20for%20Linux) with the integrated [**Arm Performance Libraries** (**ArmPL**)](https://developer.arm.com/Tools%20and%20Software/Arm%20Performance%20Libraries), version 23.10, for Linux/**ARMv8-A+SVE2** (*Scalable Vector Extensions*):

  ```sh
  export ACFLVER="23.10" &&                                 \
  export ACFLCMP="arm-linux-compiler-${ACFLVER}" &&         \
  export ACFLTYP="Generic-AArch64_RHEL-9_aarch64-linux" &&  \
  export ACFLPATH="/opt/arm/${ACFLCMP}_${ACFLTYP}" &&       \
  export PATH="${ACFLPATH}:${PATH}" &&                      \
  env CFLAGS="-march=armv8-a+sve2 -mcpu=native -armpl=sve"  \
      LDFLAGS="-armpl=sve"                                  \
      CC="armclang"                                         \
    make OPTFLAGS="-Ofast"
  ```

### Linux cross-compilation

The following commands will download and cross-compile a local static **`libuv`** and then cross-compile the simulator.

#### IBM Advance Toolchain

* Using the **IBM Advance Toolchain** **V17** to cross-compile for Linux/**POWER**:

  * Build `libuv`:

    ```sh
    env CC="/opt/at17.0/bin/powerpc64le-linux-gnu-gcc"  \
        LOCAL_CONFOPTS="--host=powerpc64le-linux-gnu"   \
        CFLAGS="-mcpu=power8 -mtune=power8"             \
      make libuvrel
    ```

  * Build the simulator:

    ```sh
    env CC="/opt/at17.0/bin/powerpc64le-linux-gnu-gcc"  \
        CFLAGS="-mcpu=power8 -mtune=power8"             \
      make
    ```

  * When targeting **POWER9** or **Power10** processors, '`power9`' or '`power10`' should replace '`power8`' in the above compiler invocation.

  * The **IBM Advance Toolchain** versions **14**, **15**, **16**, and **17** have been extensively tested and used for cross-compiling **DPS8M**.

#### Arm GNU Toolchain

The [**GNU Toolchain for the Arm Architecture**](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain) (referred to as the “**Arm GNU Toolchain**”) provides regularly updated, high-quality, validated Linux/**ARM** cross-compilers running on Microsoft Windows, Linux, and macOS.

##### Linux/ARMv7-HF

* Using the [**Arm GNU Toolchain**](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain) on Linux/x86_64, version **13.2.Rel1**, to cross-compile for Linux/**ARMv7-HF** (*hardware floating point*):

  * Build `libuv`:

    ```sh
    export AGTREL="arm-gnu-toolchain-13.2.rel1-x86_64" &&             \
    export AGTPATH="/opt/${AGTREL}-arm-none-linux-gnueabihf/bin/" &&  \
    env CC="${AGTPATH}/arm-none-linux-gnueabihf-gcc"                  \
        CFLAGS="-march=armv7-a+fp"                                    \
        LOCAL_CONFOPTS="--host=arm-none-linux-gnueabihf"              \
      make libuvrel
    ```

  * Build the simulator:

    ```sh
    env CC="${AGTPATH}/arm-none-linux-gnueabihf-gcc"                  \
        CFLAGS="-march=armv7-a+fp"                                    \
         NEED_128=1                                                   \
      make
    ```

##### Linux/ARM64

* Using the [**Arm GNU Toolchain**](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain) on Linux/x86_64, version **13.2.Rel1**, to cross-compile for Linux/**ARM64**:

  * Build `libuv`:

    ```sh
    export AGTREL="arm-gnu-toolchain-13.2.rel1-x86_64" &&             \
    export AGTPATH="/opt/${AGTREL}-aarch64-none-linux-gnu/bin/" &&    \
    env CC="${AGTPATH}/aarch64-none-linux-gnu-gcc"                    \
        LOCAL_CONFOPTS="--host=aarch64-none-linux-gnu"                \
      make libuvrel
    ```

  * Build the simulator:

    ```sh
    env CC="${AGTPATH}/aarch64-none-linux-gnu-gcc" \
      make
    ```

##### Linux/ARM64BE

* Using the [**Arm GNU Toolchain**](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain) on Linux/x86_64, version **13.2.Rel1**, to cross-compile for Linux/**ARM64BE** (*big endian*):

  * Build `libuv`:

    ```sh
    export AGTREL="arm-gnu-toolchain-13.2.rel1-x86_64" &&                \
    export AGTPATH="/opt/${AGTREL}-aarch64_be-none-linux-gnu/bin/" &&    \
    env CC="${AGTPATH}/aarch64_be-none-linux-gnu-gcc"                    \
        LOCAL_CONFOPTS="--host=aarch64_be-none-linux-gnu"                \
      make libuvrel
    ```

  * Build the simulator:

    ```sh
    env CC="${AGTPATH}/aarch64_be-none-linux-gnu-gcc" \
      make
    ```

#### Linaro GNU Toolchain

The [**Linaro**](https://www.linaro.org/) [**GNU Toolchain Integration Builds**](https://snapshots.linaro.org/gnu-toolchain) provides Linux/**ARM** and Linux/**ARM64** reference toolchains, closely tracking upstream repositories, allowing developers to easily test new compiler and processor features before the next [**Arm GNU Toolchain**](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain) release.

##### Linux/ARMv7-HF

* Using the [**Linaro GNU Toolchain Integration Build**](https://snapshots.linaro.org/gnu-toolchain) on Linux/x86_64, version **14.0.0-2023.06**, to cross-compile for Linux/**ARMv7-HF** (*hardware floating point*):

  * Build `libuv`:

    ```sh
    export LINREL="gcc-linaro-14.0.0-2023.06-x86_64" &&            \
    export LINPATH="/opt/${LINREL}_arm-linux-gnueabihf/bin/" &&    \
    env CC="${LINPATH}/arm-linux-gnueabihf-gcc"                    \
        CFLAGS="-march=armv7-a+fp"                                 \
        LOCAL_CONFOPTS="--host=arm-linux-gnueabihf"                \
      make libuvrel
    ```

  * Build the simulator:

    ```sh
    env CC="${LINPATH}/arm-linux-gnueabihf-gcc"                    \
        CFLAGS="-march=armv7-a+fp"                                 \
        NEED_128=1                                                 \
      make
    ```

##### Linux/ARM64

* Using the [**Linaro GNU Toolchain Integration Build**](https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain) on Linux/x86_64, version **14.0.0-2023.06**, to cross-compile for Linux/**ARM64**:

  * Build `libuv`:

    ```sh
    export LINREL="gcc-linaro-14.0.0-2023.06-x86_64" &&          \
    export LINPATH="/opt/${LINREL}_aarch64-linux-gnu/bin/" &&    \
    env CC="${LINPATH}/aarch64-linux-gnu-gcc"                    \
        LOCAL_CONFOPTS="--host=aarch64-linux-gnu"                \
      make libuvrel
    ```

  * Build the simulator:

    ```sh
    env CC="${LINPATH}/aarch64-linux-gnu-gcc" \
      make
    ```

#### crosstool-NG

[**crosstool-NG**](https://crosstool-ng.github.io/) is a versatile cross-toolchain generator, which can be used to generate **GCC**-based toolchains for a huge variety of architectures and operating systems (*mainly* **Linux**).

**DPS8M** is regularly built by **The DPS8M Development Team** for many **Linux** architectures using **crosstool-NG** generated toolchains, utilizing both the [**glibc**](https://www.gnu.org/software/libc/) and [**musl**](https://musl.libc.org/) C libraries. The following **CT-NG** examples are intended to be instructive, but are by no means exhaustive.

##### Linux/RV64

* Using a **crosstool-NG** generated **GCC**+**musl** toolchain to cross-compile for Linux/**RV64** (**64-bit RISC-V** *static binary*):

  * Build `libuv`:

    ```sh
    export CTNG="riscv64-local-linux-musl" &&               \
    env CC="/home/user/x-tools/${CTNG}/bin/${CTNG}-gcc"     \
        LOCAL_CONFOPTS="--host=${CTNG}"                     \
      make libuvrel
    ```

  * Build the simulator:

    ```sh
    env CC="/home/user/x-tools/${CTNG}/bin/${CTNG}-gcc"     \
        LDFLAGS="-static"                                   \
        LOCALLIBS="-latomic"                                \
      make
    ```

##### Linux/i686

* Using a **crosstool-NG** generated **GCC**+**musl** toolchain to cross-compile for Linux/**i686** (**32-bit** *static binary*):

  * Build `libuv`:

    ```sh
    export CTNG="i686-local-linux-musl" &&                  \
    env CC="/home/user/x-tools/${CTNG}/bin/${CTNG}-gcc"     \
        LOCAL_CONFOPTS="--host=${CTNG}"                     \
      make libuvrel
    ```

  * Build the simulator:

    ```sh
    env CC="/home/user/x-tools/${CTNG}/bin/${CTNG}-gcc"     \
        LDFLAGS="-static"                                   \
        LOCALLIBS="-latomic"                                \
        NEED_128=1                                          \
      make
    ```

##### Linux/ARMv6-HF

* Using a **crosstool-NG** generated **GCC**+**glibc** toolchain to cross-compile for Linux/**ARMv6-HF** (*hardware floating point*):

  * Build `libuv`:

    ```sh
    export CTNG="armv6-local-linux-gnueabihf" &&            \
    env CC="/home/user/x-tools/${CTNG}/bin/${CTNG}-gcc"     \
        LOCAL_CONFOPTS="--host=${CTNG}"                     \
        CFLAGS="-march=armv6+fp"                            \
      make libuvrel
    ```

  * Build the simulator:

    ```sh
    env CC="/home/user/x-tools/${CTNG}/bin/${CTNG}-gcc"     \
        CFLAGS="-march=armv6+fp"                            \
        LOCALLIBS="-latomic"                                \
        NEED_128=1                                          \
      make
    ```

##### Linux/PPC64le

* Using a **crosstool-NG** generated **GCC**+**musl** toolchain to cross-compile for Linux/**PPC64le** (**64-bit POWER9** *little endian static binary*):

  * Build `libuv`:

    ```sh
    export CTNG="powerpc64le-local-linux-musl" &&           \
    env CC="/home/user/x-tools/${CTNG}/bin/${CTNG}-gcc"     \
        LOCAL_CONFOPTS="--host=${CTNG}"                     \
        CFLAGS="-mcpu=power9"                               \
      make libuvrel
    ```

  * Build the simulator:

    ```sh
    env CC="/home/user/x-tools/${CTNG}/bin/${CTNG}-gcc"     \
        CFLAGS="-mcpu=power9"                               \
        LDFLAGS="-static"                                   \
        LOCALLIBS="-latomic"                                \
      make
    ```

### Additional Linux Notes

* Although normally handled automatically, when building on (*or cross-compiling to*) some
  32-bit targets using a compiler lacking support for 128-bit integer types, it may be
  necessary to explicitly set the **`NEED_128=1`** build option via the environment or as
  an argument to **`make`**.

<br>

---

<br>

## macOS

* Ensure you are running a [supported](https://support.apple.com/en-us/HT201260) release of
  [**macOS**](https://apple.com/macos) with [current updates](https://support.apple.com/en-us/HT201541) applied.
  * Both **Intel** and **ARM64** systems are regularly tested by **The DPS8M Development Team**.

[]()

* [**Xcode**](https://developer.apple.com/xcode/) is required; it is **strongly recommended** to use the most recent release for optimal performance.
  * Building with [**Intel® C++ Compiler Classic for macOS**](https://www.intel.com/content/www/us/en/developer/articles/tool/oneapi-standalone-components.html#dpcpp-cpp) (**`icc`**), version **2021.9.0** (with update **2022.6.0** minimum, **2023.1.0** recommended) and version **2021.10.0** is supported.
  * While **ICC** remains a supported compiler for building **DPS8M** on **Intel**-based **macOS** systems, Intel® has *retired* support for **ICC**.
  * At the time of writing, building the simulator on **macOS** using **GCC** is ***not recommended***.
* The following instructions were verified using **macOS 14.2.1** with **Xcode 15.2** (Apple Clang 15.0.0).

### macOS prerequisites

* [**Homebrew**](https://brew.sh/) is the recommended package manager for installing build prerequisites:

  ```sh
  brew install libuv pkg-config
  ```

* Installation and configuration of [**Homebrew**](https://brew.sh/) and its prerequisites (*i.e.* Xcode CLT) is outside the scope of the **DPS8M** documentation.  Refer to the [**Homebrew Installation Documentation**](https://docs.brew.sh/Installation) if support is required.

* Users of other package managers (*e.g.* [pkgsrc](https://www.pkgsrc.org/), [MacPorts](https://www.macports.org/)) must set the **`CFLAGS`** (*e.g.* '`-I/opt/include`'), **`LDFLAGS`** (*e.g.* '`-L/opt/lib`'), and **`LIBUV`** (*e.g.* '`-luv`') environment variables appropriately.

### macOS compilation

Build the simulator from the top-level source directory (using **GNU Make**):

#### Xcode

* Build using **Xcode**:

  ```sh
  make
  ```

#### Intel C/C++ Compiler Classic for macOS

* Build using **Intel® C/C++ Compiler Classic for macOS** (**`icc`**):

  ```sh
  env CC="icc" make
  ```

  * Use of the `NATIVE=1` build option (*i.e.* `env CC="icc" NATIVE=1 make`) is suggested for performance when building with **ICC**, if the binary is not intended for redistribution.

### macOS cross-compilation

The following commands will download and cross-compile a local static **`libuv`** and then
cross-compile the simulator using **Xcode**.

You **must** perform a '**`make distclean`**' before building for a different target.

* Install required prerequisites (using [**Homebrew**](https://brew.sh/)):

  ```sh
  brew install wget pkg-config autoconf automake libtool coreutils
  ```

Build the simulator from the top-level source directory (using **GNU Make**):

#### ARM64

* Cross-compilation targeting **ARM64** **macOS** 12:

  * Build `libuv`:

    ```sh
    make distclean &&                                \
    env CFLAGS="-target arm64-apple-macos12          \
                -mmacosx-version-min=12.0"           \
        LOCAL_CONFOPTS="--host=arm64-apple-darwin"   \
      make libuvrel HOMEBREW_INC= HOMEBREW_LIB=
    ```

  * Build the simulator:

    ```sh
    env CFLAGS="-target arm64-apple-macos12          \
                -mmacosx-version-min=12.0"           \
        LDFLAGS="-target arm64-apple-macos12         \
                 -mmacosx-version-min=12.0"          \
      make HOMEBREW_INC= HOMEBREW_LIB=
    ```

#### Intel

* Cross-compilation targeting **Intel** **macOS** 10.15:

  * Build `libuv`:

    ```sh
    make distclean &&                                \
    env CFLAGS="-target x86_64-apple-macos10.15      \
                -mmacosx-version-min=10.15"          \
        LOCAL_CONFOPTS="--host=x86_64-apple-darwin"  \
      make libuvrel HOMEBREW_INC= HOMEBREW_LIB=
    ```

  * Build the simulator:

    ```sh
    env CFLAGS="-target x86_64-apple-macos10.15      \
                -mmacosx-version-min=10.15"          \
        LDFLAGS="-target x86_64-apple-macos10.15     \
                 -mmacosx-version-min=10.15"         \
      make HOMEBREW_INC= HOMEBREW_LIB=
    ```

#### Universal

* The following more complex example builds a **macOS** ***Universal Binary***.
  * The ***Universal Binary*** will support *three* architectures: **ARM64**, **Intel**, and **Intel Haswell**.
  * The simulator (and **`libuv`**) will be cross-compiled three times each, once for each architecture.
  * The [**`lipo`**](https://developer.apple.com/documentation/apple-silicon/building-a-universal-macos-binary) utility will be used to create the universal **`dps8`** binary (in the top-level build directory).

* Cross-compilation targeting **ARM64**, **Intel**, **Intel Haswell** (**AVX2**):

  * Build **ARM64** `libuv`:

    ```sh
    make distclean &&                                \
    env CFLAGS="-target arm64-apple-macos11          \
                -mmacosx-version-min=11.0"           \
        LOCAL_CONFOPTS="--host=arm64-apple-darwin"   \
      make libuvrel HOMEBREW_INC= HOMEBREW_LIB=
    ```

  * Build **ARM64** `dps8`:

    ```sh
    env CFLAGS="-target arm64-apple-macos11          \
                -mmacosx-version-min=11.0"           \
        LDFLAGS="-target arm64-apple-macos11         \
                 -mmacosx-version-min=11.0"          \
      make HOMEBREW_INC= HOMEBREW_LIB= &&            \
    cp -f "src/dps8/dps8" "dps8.arm64"
    ```

  * Build **Intel** `libuv`:

    ```sh
    make distclean &&                                \
    env CFLAGS="-target x86_64-apple-macos10.15      \
                -mmacosx-version-min=10.15"          \
        LOCAL_CONFOPTS="--host=x86_64-apple-darwin"  \
      make libuvrel HOMEBREW_INC= HOMEBREW_LIB=
    ```

  * Build **Intel** `dps8`:

    ```sh
    env CFLAGS="-target x86_64-apple-macos10.15      \
                -mmacosx-version-min=10.15"          \
        LDFLAGS="-target x86_64-apple-macos10.15     \
                 -mmacosx-version-min=10.15"         \
      make HOMEBREW_INC= HOMEBREW_LIB= &&            \
    cp -f "src/dps8/dps8" "dps8.x86_64"
    ```

  * Build **Intel Haswell** `libuv`:

    ```sh
    make distclean &&                                \
    env CFLAGS="-target x86_64h-apple-macos10.15     \
                -mmacosx-version-min=10.15           \
                -march=haswell"                      \
        LOCAL_CONFOPTS="--host=x86_64-apple-darwin"  \
      make libuvrel HOMEBREW_INC= HOMEBREW_LIB=
    ```

  * Build **Intel Haswell** `dps8`:

    ```sh
    env CFLAGS="-target x86_64h-apple-macos10.15     \
                -mmacosx-version-min=10.15           \
                -march=haswell"                      \
        LDFLAGS="-target x86_64h-apple-macos10.15    \
                -mmacosx-version-min=10.15"          \
      make HOMEBREW_INC= HOMEBREW_LIB= &&            \
    cp -f "src/dps8/dps8" "dps8.x86_64h"
    ```

  * Create the **Universal Binary** using `lipo`:

    ```sh
    lipo -create -output "dps8"                      \
      "dps8.x86_64" "dps8.x86_64h" "dps8.arm64" &&   \
    make distclean && rm -f                          \
      "dps8.x86_64" "dps8.x86_64h" "dps8.arm64" &&   \
    lipo -detailed_info "dps8"
    ```

<br>

---

<br>

## Windows

* Ensure you are running a supported release of Microsoft **Windows** on a supported platform.
  * Microsoft **Windows** **10** and **11** on **x86_64** and **i686** are regularly tested by **The DPS8M Development Team**.

[]()

* Microsoft **Windows** supports various development and runtime environments, including [**MSVCRT**](https://docs.microsoft.com/en-us/cpp/)[/**MinGW**](https://www.mingw-w64.org/), [**Cygwin**](https://www.cygwin.com/), [**Midipix**](https://midipix.org/), [**MSYS2**](https://www.msys2.org/), [**UWIN**](https://github.com/att/uwin), [**UWP**](https://docs.microsoft.com/en-us/windows/uwp/), and others.
  * Care must be taken to avoid mixing incompatible libraries and tools.

[]()

* Cross-compilation is supported.  Builds targeting Microsoft **Windows** (**MinGW** and **Cygwin**) running on **x86_64**, **i686**, **ARMv7**, and **ARM64** platforms are regularly cross-compiled from a variety of UNIX-like systems (using **LLVM-MinGW** and **MinGW-GCC**), and from Microsoft **Windows** using **Cygwin**.

[]()

* Microsoft **Windows** also provides **Linux** compatibility via the **Windows Subsystem for Linux** (**WSL**).
  * **Windows Subsystem for Linux** users should refer to the **Linux** sections of the documentation.

### Cygwin

* Ensure you are running a current and up-to-date version of [**Cygwin**](https://cygwin.com/).

[]()

* Only the **64-bit** version of **Cygwin** is regularly tested by **The DPS8M Development Team**.
  * Although the **32-bit** version of **Cygwin** is not regularly tested (*and not recommended due to suboptimal performance*), it *should* work for building **DPS8M** (with the `NEED_128=1` build option).

#### Cygwin prerequisites

* Compilation problems in the **Cygwin** environment are often caused by incomplete or interrupted package installations, or by the installation of packages using non-standard tools (*e.g.* `apt-cyg`), resulting in missing files and dangling or missing symbolic links.

  * Before attempting to build **DPS8M** using **Cygwin**:
    1. First, update the **Cygwin** **`setup.exe`** application to the [latest available version](https://cygwin.com/install.html).
    2. Update ***all*** installed packages using the new **Cygwin** **`setup.exe`** application.
    3. Install the required prerequisite packages using **Cygwin** **`setup.exe`**:
       * `autoconf`
       * `cmake`
       * `cygcheck`
       * `gcc`
       * `libtool`
       * `libuv1`
       * `libuv1-devel`
       * `make`
       * `pkg-config`
       * `unzip`
       * `wget`
    4. **Most importantly**, invoke the **`cygcheck`** utility (*i.e.* `cygcheck -cv`) to verify the integrity of all currently installed packages and correct any problems before continuing.

#### Standard Cygwin compilation

* Build the simulator from the top-level source directory (using **GNU Make**):

  ```sh
  env CFLAGS="-DUSE_FLOCK=1 -DUSE_FCNTL=1" make
  ```

#### Cygwin-hosted cross-compilation to MinGW

The following commands will download and cross-compile a local native **`libuv`** library and then cross-compile the simulator.

You **must** perform a '`make distclean`' followed by an '`rm -rf ${HOME}/libuv-build`' before building for a different target (or changing build flags).

In the following cross-compilation examples, the *latest* **`libuv`** sources (from the `v1.x` *git* branch) are used, but the current official release (available from `https://libuv.org/`) can also be used.

##### Windows i686

* Using **GCC** (*the* **Cygwin** **`mingw64-i686-gcc-core`** *package*) to cross-compile a native **32-bit** Windows executable (*not depending on Cygwin*):

  * Build `libuv`:

    ```sh
    mkdir -p "${HOME}/libuv-build" &&                                \
    mkdir -p "${HOME}/libuv-win32-i686" &&                           \
    ( cd "${HOME}/libuv-build" &&                                    \
      wget -v "https://github.com/libuv/libuv/archive/v1.x.zip" &&   \
      unzip -xa "v1.x.zip" && cd "libuv-1.x" &&                      \
      mkdir -p "build" && cd "build" &&                              \
      cmake .. -DCMAKE_SYSTEM_NAME="Windows"                         \
               -DCMAKE_SYSTEM_VERSION="6.1"                          \
               -DCMAKE_C_COMPILER="i686-w64-mingw32-gcc"             \
               -DCMAKE_INSTALL_PREFIX="${HOME}/libuv-win32-i686" &&  \
      cmake --build . && cmake --install . )
    ```

  * Build the simulator:

    ```sh
    env CFLAGS="-I${HOME}/libuv-win32-i686/include -D__MINGW64__     \
                -DUSE_FLOCK=1 -DUSE_FCNTL=1"                         \
        CC="i686-w64-mingw32-gcc"                                    \
        LDFLAGS="-L${HOME}/libuv-win32-i686/lib" NEED_128=1          \
      make CROSS="MINGW64"
    ```

  * The compiled native binary will require `libwinpthread-1.dll` (located at `/usr/i686-w64-mingw32/sys-root/mingw/bin/libwinpthread-1.dll`) and `libuv.dll` (located at `${HOME}/libuv-win32-i686/bin/libuv.dll`) at runtime.
    * It is sufficient to copy these files into the directory containing the `dps8.exe` binary.

##### Windows x86_64

* Using **GCC** (*the* **Cygwin** **`mingw64-x86_64-gcc-core`** *package*) to cross-compile a native **64-bit** Windows executable (*not depending on Cygwin*):

  * Build `libuv`:

    ```sh
    mkdir -p "${HOME}/libuv-build" &&                                  \
    mkdir -p "${HOME}/libuv-win32-x86_64" &&                           \
    ( cd "${HOME}/libuv-build" &&                                      \
      wget -v "https://github.com/libuv/libuv/archive/v1.x.zip" &&     \
      unzip -xa "v1.x.zip" && cd "libuv-1.x" &&                        \
      mkdir -p "build" && cd "build" &&                                \
      cmake .. -DCMAKE_SYSTEM_NAME="Windows"                           \
               -DCMAKE_SYSTEM_VERSION="6.1"                            \
               -DCMAKE_C_COMPILER="x86_64-w64-mingw32-gcc"             \
               -DCMAKE_INSTALL_PREFIX="${HOME}/libuv-win32-x86_64" &&  \
      cmake --build . && cmake --install . )
    ```

  * Build the simulator:

    ```sh
    env CFLAGS="-I${HOME}/libuv-win32-x86_64/include -D__MINGW64__     \
                -DUSE_FLOCK=1 -DUSE_FCNTL=1"                           \
        CC="x86_64-w64-mingw32-gcc"                                    \
        LDFLAGS="-L${HOME}/libuv-win32-x86_64/lib"                     \
      make CROSS="MINGW64"
    ```

  * The compiled native binary will require `libwinpthread-1.dll` (located at `/usr/x86_64-w64-mingw32/sys-root/mingw/bin/libwinpthread-1.dll`) and `libuv.dll` (located at `${HOME}/libuv-win32-x86_64/bin/libuv.dll`) at runtime.
    * It is sufficient to copy these files into the directory containing the `dps8.exe` binary.

#### Unix-hosted cross-compilation to Cygwin

This section documents the procedure for building Windows Cygwin binaries using a Unix-based host system to download and cross-compile **`libuv`** library and then cross-compile the simulator.

In the following cross-compilation examples, the *latest* **`libuv`** sources (from the `v1.x` *git* branch) are used, but the current official release (available from `https://libuv.org/`) can also be used.

* Using **GCC** from the [**Fedora Cygwin cross-compiler toolchain**](https://copr.fedorainfracloud.org/coprs/yselkowitz/cygwin/) (maintained by [Yaakov Selkowitz](mailto:yselkowitz@cygwin.com)) to cross-compile a **64-bit** Windows Cygwin executable (*depending on `cygwin1.dll`*):

  * Build `libuv`:

    ```sh
    mkdir -p "${HOME}/libuv-build" &&                               \
    mkdir -p "${HOME}/libuv-cygwin-x64" &&                          \
    ( cd "${HOME}/libuv-build" &&                                   \
      wget -v "https://github.com/libuv/libuv/archive/v1.x.zip" &&  \
      unzip -xa "v1.x.zip" && cd "libuv-1.x" && sh ./autogen.sh &&  \
      cygwin64-configure --prefix="${HOME}/libuv-cygwin-x64"        \
        --enable-static --disable-shared &&                         \
      make && make install )
    ```

  * Build the simulator:

    ```sh
    cygwin64-make
    ```

* Using **GCC** from the [**Fedora Cygwin cross-compiler toolchain**](https://copr.fedorainfracloud.org/coprs/yselkowitz/cygwin/) to cross-compile a **32-bit** Windows Cygwin executable (*depending on `cygwin1.dll`*):

  * Build `libuv`:

    ```sh
    mkdir -p "${HOME}/libuv-build" &&                               \
    mkdir -p "${HOME}/libuv-cygwin-x86" &&                          \
    ( cd "${HOME}/libuv-build" &&                                   \
      wget -v "https://github.com/libuv/libuv/archive/v1.x.zip" &&  \
      unzip -xa "v1.x.zip" && cd "libuv-1.x" && sh ./autogen.sh &&  \
      cygwin32-configure --prefix="${HOME}/libuv-cygwin-x86"        \
        --enable-static --disable-shared &&                         \
      make && make install )
    ```

  * Build the simulator:

    ```sh
    env NEED_128=1 cygwin32-make
    ```

### MSYS2

* **DPS8M** can be built as a native **MSYS2** application without special configuration, using the "**MSYS2 Environment**".

[]()

* Cross-compilation using the **MSYS2**-provided **MINGW32**, **MINGW64**, **UCRT64**, **CLANG32**, **CLANG64**, and **CLANGARM64** environments is *currently untested*.

### Unix-hosted LLVM-MinGW Clang cross-compilation

The [**LLVM-MinGW Clang**](https://github.com/mstorsjo/llvm-mingw) toolchain supports building native Windows binaries (**i686**, **x86_64**, **ARMv7**, and **ARM64** systems) on *non*-**Windows** host systems (or using the **Windows Subsystem for Linux**).

The [**LLVM-MinGW Docker Container**](https://hub.docker.com/r/mstorsjo/llvm-mingw/) provides pre-built and fully configured **LLVM-MinGW** toolchains (including appropriate compiler symlinks) which are regularly used by **The DPS8M Development Team**.

In the following cross-compilation examples, the *latest* **`libuv`** sources (from the `v1.x` *git* branch) are used, but the current official release (available from `https://libuv.org/`) can also be used.

#### Windows i686

* Using **Clang** (*the* **LLVM-MinGW** *compiler*) to cross-compile a local static `libuv` library and a native **32-bit** Windows/**i686** executable:

  * Build `libuv`:

    ```sh
    mkdir -p "${HOME}/libuv-build" &&                                  \
    mkdir -p "${HOME}/libuv-win32-i686" &&                             \
    ( cd "${HOME}/libuv-build" &&                                      \
      wget -v "https://github.com/libuv/libuv/archive/v1.x.zip" &&     \
      unzip -xa "v1.x.zip" && cd "libuv-1.x" && sh ./autogen.sh &&     \
      ./configure --prefix="${HOME}/libuv-win32-i686"                  \
        --enable-static --disable-shared --host="i686-w64-mingw32" &&  \
      make && make install )
    ```

  * Build the simulator:

    ```sh
    env CC="i686-w64-mingw32-clang"                                    \
        CFLAGS="-I${HOME}/libuv-win32-i686/include -D__MINGW64__"      \
        LDFLAGS="-L${HOME}/libuv-win32-i686/lib" NEED_128=1            \
      make CROSS="MINGW64"
    ```

#### Windows x86_64

* Using **Clang** (*the* **LLVM-MinGW** *compiler*) to cross-compile a local static `libuv` library and a native **64-bit** Windows/**x86_64** executable:

  * Build `libuv`:

    ```sh
    mkdir -p "${HOME}/libuv-build" &&                                    \
    mkdir -p "${HOME}/libuv-win32-x86_64" &&                             \
    ( cd "${HOME}/libuv-build" &&                                        \
      wget -v "https://github.com/libuv/libuv/archive/v1.x.zip" &&       \
      unzip -xa "v1.x.zip" && cd "libuv-1.x" && sh ./autogen.sh &&       \
      ./configure --prefix="${HOME}/libuv-win32-x86_64"                  \
        --enable-static --disable-shared --host="x86_64-w64-mingw32" &&  \
      make && make install )
    ```

  * Build the simulator:

    ```sh
    env CC="x86_64-w64-mingw32-clang"                                    \
        CFLAGS="-I${HOME}/libuv-win32-x86_64/include -D__MINGW64__"      \
        LDFLAGS="-L${HOME}/libuv-win32-x86_64/lib"                       \
      make CROSS="MINGW64"
    ```

#### Windows ARMv7

* Using **Clang** (*the* **LLVM-MinGW** *compiler*) to cross-compile a local static `libuv` library and a native **32-bit** Windows/**ARMv7** executable:

  * Build `libuv`:

    ```sh
    mkdir -p "${HOME}/libuv-build" &&                                   \
    mkdir -p "${HOME}/libuv-win32-armv7" &&                             \
    ( cd "${HOME}/libuv-build" &&                                       \
      wget -v "https://github.com/libuv/libuv/archive/v1.x.zip" &&      \
      unzip -xa "v1.x.zip" && cd "libuv-1.x" && sh ./autogen.sh &&      \
      ./configure --prefix="${HOME}/libuv-win32-armv7"                  \
        --enable-static --disable-shared --host="armv7-w64-mingw32" &&  \
      make && make install )
    ```

  * Build the simulator:

    ```sh
    env CC="armv7-w64-mingw32-clang"                                    \
        CFLAGS="-I${HOME}/libuv-win32-armv7/include -D__MINGW64__"      \
        LDFLAGS="-L${HOME}/libuv-win32-armv7/lib"                       \
        NEED_128=1                                                      \
      make CROSS="MINGW64"
    ```

#### Windows ARM64

* Using **Clang** (*the* **LLVM-MinGW** *compiler*) to cross-compile a local static `libuv` library and a native **64-bit** Windows/**ARM64** executable:

  * Build `libuv`:

    ```sh
    mkdir -p "${HOME}/libuv-build" &&                                     \
    mkdir -p "${HOME}/libuv-win32-arm64" &&                               \
    ( cd "${HOME}/libuv-build" &&                                         \
      wget -v "https://github.com/libuv/libuv/archive/v1.x.zip" &&        \
      unzip -xa "v1.x.zip" && cd "libuv-1.x" && sh ./autogen.sh &&        \
      ./configure --prefix="${HOME}/libuv-win32-arm64"                    \
        --enable-static --disable-shared --host="aarch64-w64-mingw32" &&  \
      make && make install )
    ```

  * Build the simulator:

    ```sh
    env CC="aarch64-w64-mingw32-clang"                                    \
        CFLAGS="-I${HOME}/libuv-win32-arm64/include -D__MINGW64__"        \
        LDFLAGS="-L${HOME}/libuv-win32-arm64/lib"                         \
      make CROSS="MINGW64"
    ```

### Unix-hosted MinGW-w64 GCC cross-compilation

The [**MinGW-w64 GCC**](https://www.mingw-w64.org/) toolchain supports building native Windows (**i686** and **x86_64**) executables on *non*-**Windows** host systems (or **Windows** using the **Windows Subsystem for Linux**).

* [Many **MinGW-w64 toolchains** are available](https://www.mingw-w64.org/downloads/) for a wide variety of host platforms and operating systems.
* Version **9.0** is the *minimum* version of **MinGW-w64** tested with **DPS8M**; version **11.0** is *recommended*.
* **The DPS8M Development Team** regularly cross-compiles **Windows** executables using **GCC**-based **MinGW-w64** toolchains on **Alpine Linux** and **Fedora Linux** host systems.

In the following cross-compilation examples, the *latest* **`libuv`** sources (from the `v1.x` *git* branch) are used, but the current official release (available from `https://libuv.org/`) can also be used.

#### Windows i686

* Using **GCC** (*the* **MinGW-w64** *compiler*) to cross-compile a local static `libuv` library and a native **32-bit** Windows/**i686** executable:

  * Build `libuv`:

    ```sh
    mkdir -p "${HOME}/libuv-build" &&                                  \
    mkdir -p "${HOME}/libuv-win32-i686" &&                             \
    ( cd "${HOME}/libuv-build" &&                                      \
      wget -v "https://github.com/libuv/libuv/archive/v1.x.zip" &&     \
      unzip -xa "v1.x.zip" && cd "libuv-1.x" && sh ./autogen.sh &&     \
      ./configure --prefix="${HOME}/libuv-win32-i686"                  \
        --enable-static --disable-shared --host="i686-w64-mingw32" &&  \
      make && make install )
    ```

  * Build the simulator:

    ```sh
    env CC="i686-w64-mingw32-gcc"                                      \
        CFLAGS="-I${HOME}/libuv-win32-i686/include                     \
                -D__MINGW64__ -pthread"                                \
        LDFLAGS="-L${HOME}/libuv-win32-i686/lib -lpthread"             \
        NEED_128=1                                                     \
      make CROSS="MINGW64"
    ```

#### Windows x86_64

* Using **GCC** (*the* **MinGW-w64** *compiler*) to cross-compile a local static `libuv` library and a native **64-bit** Windows/**x86_64** executable:

  * Build `libuv`:

    ```sh
    mkdir -p "${HOME}/libuv-build" &&                                    \
    mkdir -p "${HOME}/libuv-win32-x86_64" &&                             \
    ( cd "${HOME}/libuv-build" &&                                        \
      wget -v "https://github.com/libuv/libuv/archive/v1.x.zip" &&       \
      unzip -xa "v1.x.zip" && cd "libuv-1.x" && sh ./autogen.sh &&       \
      ./configure --prefix="${HOME}/libuv-win32-x86_64"                  \
        --enable-static --disable-shared --host="x86_64-w64-mingw32" &&  \
      make && make install )
    ```

  * Build the simulator:

    ```sh
    env CC="x86_64-w64-mingw32-gcc"                                      \
        CFLAGS="-I${HOME}/libuv-win32-x86_64/include                     \
                -D__MINGW64__ -pthread"                                  \
        LDFLAGS="-L${HOME}/libuv-win32-x86_64/lib -lpthread"             \
      make CROSS="MINGW64"
    ```

<br>

---

<br>
