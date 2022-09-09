<!-- SPDX-License-Identifier: ICU
     Copyright (c) 2022 The DPS8M Development Team -->

The simulator is distributed in various forms, including an easy-to-build [**source code distribution**](https://dps8m.gitlab.io/dps8m/Releases/),
which can be built simply via **`make`** on *most* systems. The following sections document ***only***
platform-specific variations, and are **not** intended to be a general or exhaustive reference.

Review the complete [**DPS8M Omnibus Documentation**](https://dps8m.gitlab.io/dps8m/Documentation/) for
additional details.

* [**General Information**](#general-information)
* [**FreeBSD**](#freebsd)
* [**NetBSD**](#netbsd)
* [**OpenBSD**](#openbsd)
* [**DragonFly BSD**](#dragonfly-bsd)
* [**Solaris**](#solaris)
* [**OpenIndiana**](#openindiana)
* [**AIX**](#aix)
* [**Haiku**](#haiku)
* [**GNU/Hurd**](#gnuhurd)
* [**Linux**](#linux)
* [**macOS**](#macos)
* [**Windows**](#windows)

## General Information

* For optimal performance, building the simulator from source is highly recommended.
* Building on a **64-bit** platform is **strongly encouraged**.

[]()

* **The DPS8M Development Team** recommends most users download a [**source kit distribution**](https://dps8m.gitlab.io/dps8m/Releases/).
  * A source kit requires approximately **8 MiB** of storage space to decompress and **45 MiB** to
    build.

[]()

* Advanced users may prefer to clone the
  [**git repository**](https://gitlab.com/dps8m/dps8m) which contains additional tools not required
  for simulator operation, but useful to developers.
  * The [**git repository**](https://gitlab.com/dps8m/dps8m) requires approximately **175 MiB** of storage
    space to clone and **250 MiB** to build.

<br>

---

## FreeBSD

* Ensure you are running a [supported release](https://www.freebsd.org/releases/) of
  [**FreeBSD**](https://www.freebsd.org/) on a [supported platform](https://www.freebsd.org/platforms/).
  * **FreeBSD**/[**amd64**](https://www.freebsd.org/platforms/amd64/) and **FreeBSD**/[**arm64**](https://www.freebsd.org/platforms/arm/) are regularly tested by **The DPS8M Development Team**.

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
  the simulator, we recommend that version 10 or later of the **GNU C** (`gcc`) compiler be used for
  optimal performance.
* At the time of writing, **GCC 12** is available for **FreeBSD** systems and is the version of GCC
  currently recommended by **The DPS8M Development Team**.  It can be installed via FreeBSD Packages
  or Ports:

  * Using FreeBSD Packages (as *root*):

    ```sh
    pkg install gcc12
    ```

  * Using FreeBSD Ports (as *root*):

    ```sh
    cd /usr/ports/lang/gcc12/ && make install clean
    ```

* Build the simulator from the top-level source directory (using **GNU Make**):

  ```sh
  env CC="gcc12" LDFLAGS="-Wl,-rpath=/usr/local/lib/gcc12" gmake
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
* The **FreeBSD-specific** **`atomic_testandset_64`** operation is currently not implemented on every platform **FreeBSD** supports (such as **powerpc64**). If you are unable to build the simulator because this atomic operation is unimplemented on your platform, specify `ATOMICS="GNU"` as an argument to `gmake`, or export this value in the shell environment before compiling.

<br>

---

## NetBSD

* Ensure you are running a [supported release](https://www.netbsd.org/releases/formal.html) of
  [**NetBSD**](https://www.netbsd.org) on a [supported platform](https://www.netbsd.org/ports/).
  * **NetBSD**/[**amd64**](https://www.netbsd.org/ports/amd64/) is regularly tested by **The DPS8M Development Team**.

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

* **NetBSD** provides an older version of **GCC** (or **Clang**) as part of the base system (depending on the platform.)
  While *sufficient* to build the simulator, we recommend that version 10 or later of the **GNU C** (`gcc`) compiler
  be used for optimal performance.

* At the time of writing, **GCC 12** is available for **NetBSD** systems and is the version of GCC currently
  recommended by **The DPS8M Development Team**.  It can be installed via Packages or pkgsrc.

  * Using NetBSD Packages (as *root*):

    ```sh
    pkgin in gcc12
    ```

  * Using pkgsrc (as *root*):

    ```sh
    cd /usr/pkgsrc/lang/gcc12 && make install clean
    ```

* Build the simulator from the top-level source directory (using **GNU Make**):

  ```sh
  env CC="/usr/pkg/gcc12/bin/gcc" LDFLAGS="Wl,-rpath=/usr/pkg/gcc12/lib" gmake
  ```

### Compilation using Clang

* **GCC** is recommended for optimal performance, but compilation using **Clang** (and linking
  using **LLD**, the LLVM linker) is supported.  Both **Clang** and **LLD** can be installed via
  Packages or pkgsrc.

  * Using NetBSD Packages (as *root*):

    ```sh
    pkgin in clang lld
    ```

  * Using pkgsrc (as *root*):

    ```sh
    cd /usr/pkgsrc/lang/clang && make install clean
    cd /usr/pkgsrc/devel/lld && make install clean
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

## OpenBSD

* Ensure you are running an [up-to-date](https://man.openbsd.org/syspatch.8) and
  [supported release](https://www.openbsd.org/) of [**OpenBSD**](https://www.openbsd.org/) on a
  [supported platform](https://www.openbsd.org/plat.html).
  * **OpenBSD**/[**amd64**](https://www.openbsd.org/amd64.html) and
    **OpenBSD**/[**arm64**](https://www.openbsd.org/arm64.html) are regularly tested by
    **The DPS8M Development Team**.
* The following instructions were verified using **OpenBSD 7.1** on
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

* **OpenBSD** provides an older version of **GCC** (or **Clang**) as part of the base system (depending on the platform.)
  While *sufficient* to build the simulator, we recommend that a recent version of the **GNU assembler** (`gas`)
  and version 10 or later of the **GNU C** (`gcc`) compiler be used for optimal performance.

* At the time of writing, appropriate versions of the **GNU assembler** and **GNU C** (version **11**)
  are available for **OpenBSD**.  (In addition, **LLD**, the LLVM linker, may be required.)  These tools have
  been tested and are highly recommended by **The DPS8M Development Team**. They can be installed via
  OpenBSD Packages or Ports:

  * Using OpenBSD Packages (as *root*):

    ```sh
    pkg_add gas gcc
    ```

  * Using OpenBSD Ports (as *root*):

    ```sh
    cd /usr/ports/devel/gas/ && make install clean
    cd /usr/ports/lang/gcc/11/ && make install clean
    ```

* LLVM **LLD** 13.0.0 or later is recommended for linking, even when using **GCC 11** for compilation.
  **LLD** is the default linker on ***most*** (*but not all*) supported OpenBSD platforms.  To determine the linker
  in use, examine the output of `ld --version`.  If the linker identifies itself by a name *other* than **LLD**
  (*e.g.* **`GNU ld`** or similar), **LLD** must be installed via OpenBSD Packages or Ports.

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
* A version of **Clang** newer than the base system version may be available via the **`llvm`** package or port.
* Once installed, it can be used for compilation by setting `CC=/usr/local/bin/clang`.

### Additional OpenBSD Notes

* At the time of writing, **OpenBSD**/[**luna88k**](https://www.openbsd.org/luna88k.html) has not been tested.
  It should be possible to build the simulator for this architecture using the `gcc` compiler provided by the
  base system and specifying the `NO_LTO=1` build option.

<br>

---

## DragonFly BSD

* At the time of writing [**DragonFly BSD 6.2.2**](https://www.dragonflybsd.org/download/) was current
  and used to verify the following instructions.

### DragonFly BSD prerequisites

* Install the required prerequisites (using DragonFly BSD DPorts):

  ```sh
  pkg install gmake libuv
  ```

### Standard DragonFly BSD compilation

* Build the simulator (*standard build*) from the top-level source directory (using **GNU Make**):

  ```sh
  env CFLAGS="-I/usr/local/include" \
     LDFLAGS="-L/usr/local/lib"     \
     ATOMICS="GNU" gmake
  ```

### Optimized DragonFly BSD compilation

* **DragonFly BSD** provides an older version of **GCC** as part of the base system.  While this compiler is
  *sufficient* to build the simulator, we recommend that version 10 or later of the **GNU C** (`gcc`)
  compiler be used for optimal performance.

* At the time of writing, **GCC 11** is available for DragonFly BSD and recommended by **The DPS8M Development Team**.

  * **GCC 11** may be installed using DragonFly BSD DPorts:

    ```sh
    pkg install gcc11
    ```

* Build the simulator from the top-level source directory (using **GNU Make**):

  ```sh
  env CC="gcc11" CFLAGS="-I/usr/local/include"                 \
    LDFLAGS="-L/usr/local/lib -Wl,-rpath=/usr/local/lib/gcc11" \
    ATOMICS="GNU" gmake
  ```

### Compiling using Clang

* **GCC** is recommended for optimal performance, but compilation using **Clang** is supported.
* At the time of writing, **Clang 14** is available for DragonFly BSD and recommended by **The DPS8M Development Team**.

  * **Clang 14** may be installed using DragonFly BSD DPorts:

    ```sh
    pkg install llvm14
    ```

* Build the simulator from the top-level source directory (using **GNU Make**):

  ```sh
  env CC="clang14" CFLAGS="-I/usr/include -I/usr/local/include" \
    LDFLAGS="-L/usr/lib -L/usr/local/lib -fuse-ld=lld"          \
    ATOMICS="GNU" gmake
  ```

<br>

---

## Solaris

* Ensure your **Solaris** installation is reasonably current. [**Oracle Solaris**](https://www.oracle.com/solaris) **11.4 SRU42** or later is recommended.
* The simulator can be built on **Solaris** using the **GCC**, **Clang**, and **Oracle Developer Studio** compilers.
* **GCC 11** is the recommended compiler for optimal performance on all Intel-based **Solaris** systems.
  * **GCC 11** can be installed from the standard IPS repository via **`pkg install gcc-11`**.
* Building with **Clang 11** or later is also supported (*but not recommended due to lack of LTO support*).
  * **Clang 11** can be installed from the standard IPS repository via **`pkg install clang@11 llvm@11`**.
* Building with the **Oracle Developer Studio 12.6** (`suncc`) compiler is also supported.
* Link-time optimization (*LTO*) is supported ***only*** when building with **GCC** version 10 or later.
  * The `NO_LTO=1` build option should be specified when using earlier versions of the **GCC** compiler.

### Solaris prerequisites

* Install the required prerequisites from the standard IPS repository (as *root*):

  ```sh
  pkg install gnu-make gnu-binutils gnu-sed gnu-grep \
    gnu-tar libtool gnu-coreutils gawk pkg-config    \
    autoconf automake wget
  ```

### Solaris compilation

The following commands will download and build a local static **`libuv`** before compiling the simulator.
If a site-provided **`libuv`** library has been installed (in the **`/usr/local`** prefix), the
**`libuvrel`** target (and **`-j 1`** argument) may be omitted.

Build the simulator from the top-level source directory (using **GNU Make**):

* Using **GCC**:

  ```sh
  env TAR="gtar" TR="gtr" CC="gcc" gmake libuvrel all -j 1
  ```

* Using **Clang**:

  ```sh
  env TAR="gtar" NO_LTO=1 TR="gtr" CC="clang" gmake libuvrel all -j 1
  ```

* Using **Oracle Developer Studio 12.6**:

  ```sh
  env TAR="gtar" NO_LTO=1 SUNPRO=1 NEED_128=1 TR="gtr" CSTD="c11"   \
  CFLAGS="-DNO_C_ELLIPSIS -Qy -xO5 -m64 -xlibmil -xCC -mt -xlibmopt \
  -fno-semantic-interposition -xprefetch=auto -xprefetch_level=3"   \
  CC="/opt/developerstudio12.6/bin/suncc" gmake libuvrel all -j 1
  ```

<br>

---

## OpenIndiana

* Ensure your [**OpenIndiana**](https://www.openindiana.org/) installation is up-to-date.
  *Hipster* **2022-08-26** was used for verification.
* **GCC 11** is currently the recommended compiler for optimal performance.
  * **GCC 11** can be installed from the standard IPS repository via **`pkg install gcc-11`**.
* Building with **Clang 13** or later is also supported (*but not recommended due to lack of LTO support*).
  * **Clang 13** can be installed from the standard IPS repository via **`pkg install clang-13`**.
* Link-time optimization (*LTO*) is supported ***only*** when building with **GCC** version 10 or later.
  * The `NO_LTO=1` build option should be specified when using earlier versions of the **GCC** compiler.

### OpenIndiana prerequisites

* Install the requires prerequisites from the standard IPS repository (as *root*):

  ```sh
  pkg install gnu-make libuv gnu-binutils gnu-coreutils
  ```

### OpenIndiana compilation

Build the simulator from the top-level source directory (using **GNU Make**):

* Using **GCC**:

  ```sh
  env CC="gcc-11" gmake
  ```

* Using **Clang**:

  ```sh
  env NO_LTO=1 CC="clang-13" gmake
  ```

<br>

---

## AIX

* Ensure you are running a [supported release](https://www.ibm.com/support/pages/aix-support-lifecycle-information) of [**IBM AIX®**](https://www.ibm.com/products/aix) on a [supported platform](https://www.ibm.com/support/pages/system-aix-maps).
* **AIX** **7.2** and **7.3** on [**POWER8®** and **POWER9™**](https://www.ibm.com/it-infrastructure/power)
  are regularly tested by **The DPS8M Development Team**.
* The simulator can be built on **AIX** using **GNU C** (**`gcc`**) or [**IBM XL C/C++ for AIX**](https://www.ibm.com/products/xl-c-aix-compiler-power) (**`xlc`**).
  * [**IBM XL C/C++ for AIX V16.1 Service Pack 10** (*IJ36514*)](https://www.ibm.com/support/pages/ibm-xl-cc-aix-161) is the recommended compiler release for optimal performance on **POWER8** and **POWER9** systems.
    * At the time of writing, [*IBM Open XL C/C++ for AIX V17.1*](https://www.ibm.com/products/open-xl-cpp-aix-compiler-power) is ***not supported***.  Support for *IBM Open XL C/C++ for AIX V17.1* will be available in a future release.
* When building the simulator using **GNU C**, it recommended to use **GCC 10** or later for optimal performance.
  * **GCC 10** can be installed from the [IBM AIX® Toolbox for Open Source Software](https://www.ibm.com/support/pages/aix-toolbox-open-source-software-overview) repository.

### AIX prerequisites

* Install the required prerequisites from the [IBM AIX® Toolbox for Open Source Software](https://www.ibm.com/support/pages/aix-toolbox-open-source-software-overview) repository (as *root*):

  ```sh
  /opt/freeware/bin/dnf install gmake libuv libuv-devel popt coreutils gawk sed
  ```

* *Optionally* install **GCC 10** from the [IBM AIX® Toolbox for Open Source Software](https://www.ibm.com/support/pages/aix-toolbox-open-source-software-overview) repository (as *root*):

  ```sh
  /opt/freeware/bin/dnf install gcc gcc10
  ```

### AIX compilation

Build the simulator from the top-level source directory (using **GNU Make**):

* Using **IBM XL C/C++ for AIX V16.1**:

  ```sh
  env PATH="/opt/freeware/bin:${PATH}" ATOMICS="AIX" AWK="gawk" NO_LTO=1 \
  OBJECT_MODE=64 gmake CC="/opt/IBM/xlC/16.1.0/bin/xlc_r" NEED_128=1     \
  USE_POPT=1 PULIBS="-lpopt" LDFLAGS="-L/opt/freeware/lib -b64"          \
  LIBS="-luv -lbsd -lm" CFLAGS="-O3 -qhot -qarch=pwr8 -qalign=natural    \
  -qtls -DUSE_POPT=1 -DUSE_FLOCK=1 -DUSE_FCNTL=1 -DAIX_ATOMICS=1         \
  -DNEED_128=1 -DLOCKLESS=1 -I/opt/freeware/include -I../simh            \
  -I../decNumber -D_GNU_SOURCE -D_ALL_SOURCE -U__STRICT_POSIX__"
  ```

* Using **GCC 10**:

  ```sh
  env PATH="/opt/freeware/bin:${PATH}" CC="gcc-10" ATOMICS="AIX" \
    NO_LTO=1 gmake
  ```

<br>

---

## Haiku

* *TBD*

<br>

---

## GNU/Hurd

* *TBD*

<br>

---

## Linux

* *TBD*

<br>

---

## macOS

* Ensure you are running a [supported](https://support.apple.com/en-us/HT201260) release of
  [**macOS**](https://apple.com/macos) with [current updates](https://support.apple.com/en-us/HT201541) applied.
  * Both **Intel** and **ARM64** systems are regularly tested by
    **The DPS8M Development Team**.
* [**Xcode**](https://developer.apple.com/xcode/) is required; it is **strongly recommended** to use the most recent release for optimal performance.
  * Building with [**Intel® C++ Compiler Classic for macOS**](https://www.intel.com/content/www/us/en/developer/articles/tool/oneapi-standalone-components.html#dpcpp-cpp) (**`icc`**) **2022.6.0** or later is also supported.
  * At the time of writing, building the simulator on **macOS** using **GCC** is ***not recommended***.
* The following instructions were verified using **macOS 12.5.1** with **Xcode 14.0** (Apple Clang 14.0.0).

### macOS prerequisites

* [**Homebrew**](https://brew.sh/) is the recommended package manager for installing build prerequisites:

  ```sh
  brew install libuv pkg-config
  ```

* Users of other package managers (*e.g.* [pkgsrc](https://www.pkgsrc.org/), [MacPorts](https://www.macports.org/)) must set the **`CFLAGS`** (*e.g.* `-I/opt/include`), **`LDFLAGS`** (*e.g.* `-L/opt/lib`), and **`LIBUV`** (*e.g.* `-luv`) environment variables appropriately.

### macOS compilation

Build the simulator from the top-level source directory (using **GNU Make**):

* Using **Xcode**:

  ```sh
  make
  ```

* Using **Intel® C++ Compiler Classic for macOS** (**`icc`**):

  ```sh
  env CC="icc" CFLAGS="-xHost" make
  ```

### macOS cross-compilation

The following commands will download and cross-compile a local static **`libuv`** and then
cross-compile the simulator.  You **must** perform a **`make distclean`** before building
for a different target.

* Install required prerequisites using [**Homebrew**](https://brew.sh/):

  ```sh
  brew install wget pkg-config autoconf automake libtool coreutils
  ```

Build the simulator from the top-level source directory (using **GNU Make**):

* Cross-compilation targeting **ARM64** **macOS** 11:

  ```sh
  make distclean &&                                                  \
  env CFLAGS="-target arm64-apple-macos11 -mmacosx-version-min=11.0" \
  LOCAL_CONFOPTS="--host=arm64-apple-darwin" make libuvrel           \
  HOMEBREW_INC= HOMEBREW_LIB= &&                                     \
  env CFLAGS="-target arm64-apple-macos11 -mmacosx-version-min=11.0" \
  LDFLAGS="-target arm64-apple-macos11 -mmacosx-version-min=11.0"    \
  make HOMEBREW_INC= HOMEBREW_LIB=
  ```

* Cross-compilation targeting **Intel** **macOS** 10.15:

  ```sh
  make distclean &&                                                       \
  env CFLAGS="-target x86_64-apple-macos10.15 -mmacosx-version-min=10.15" \
  LOCAL_CONFOPTS="--host=x86_64-apple-darwin" make libuvrel               \
  HOMEBREW_INC= HOMEBREW_LIB= &&                                          \
  env CFLAGS="-target x86_64-apple-macos10.15 -mmacosx-version-min=10.15" \
  LDFLAGS="-target x86_64-apple-macos10.15 -mmacosx-version-min=10.15"    \
  make HOMEBREW_INC= HOMEBREW_LIB=
  ```

* Build a **macOS** Universal Binary (**ARM64**, **Intel**, **Haswell**):

  ```sh
  make distclean && \
  env CFLAGS="-target arm64-apple-macos11 -mmacosx-version-min=11.0"      \
   LOCAL_CONFOPTS="--host=arm64-apple-darwin" make libuvrel               \
    HOMEBREW_INC= HOMEBREW_LIB= &&                                        \
  env CFLAGS="-target arm64-apple-macos11 -mmacosx-version-min=11.0"      \
   LDFLAGS="-target arm64-apple-macos11 -mmacosx-version-min=11.0"        \
    make HOMEBREW_INC= HOMEBREW_LIB= &&                                   \
  cp -f "src/dps8/dps8" "dps8.arm64" &&                                   \
  make distclean &&                                                       \
  env CFLAGS="-target x86_64-apple-macos10.15 -mmacosx-version-min=10.15" \
   LOCAL_CONFOPTS="--host=x86_64-apple-darwin" make libuvrel              \
    HOMEBREW_INC= HOMEBREW_LIB= &&                                        \
  env CFLAGS="-target x86_64-apple-macos10.15 -mmacosx-version-min=10.15" \
   LDFLAGS="-target x86_64-apple-macos10.15 -mmacosx-version-min=10.15"   \
  make HOMEBREW_INC= HOMEBREW_LIB= &&                                     \
  cp -f "src/dps8/dps8" "dps8.x86_64" &&                                  \
  make distclean &&                                                       \
  env CFLAGS="-target x86_64h-apple-macos10.15 -mmacosx-version-min=10.15 \
   -march=haswell" LOCAL_CONFOPTS="--host=x86_64-apple-darwin"            \
  make libuvrel HOMEBREW_INC= HOMEBREW_LIB= &&                            \
  env CFLAGS="-target x86_64h-apple-macos10.15 -mmacosx-version-min=10.15 \
   -march=haswell" LDFLAGS="-target x86_64h-apple-macos10.15              \
    -mmacosx-version-min=10.15" make HOMEBREW_INC= HOMEBREW_LIB= &&       \
  cp -f "src/dps8/dps8" "dps8.x86_64h" &&                                 \
  lipo -create -output dps8 dps8.x86_64 dps8.x86_64h dps8.arm64 &&        \
  make distclean && rm -f dps8.x86_64 dps8.x86_64h dps8.arm64 &&          \
  rm -rf "./__.SYMDEF SORTED" "./src/empty/empty.dSYM" 2> /dev/null &&    \
  lipo -detailed_info "dps8"
  ```

<br>

---

## Windows

* *TBD*

<br>

---
