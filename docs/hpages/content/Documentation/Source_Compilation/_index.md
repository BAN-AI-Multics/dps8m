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

Install the required prerequisites (using FreeBSD Packages or Ports.)

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

* Build the simulator (*with standard settings*) from the top-level directory (using **GNU Make**):

  ```sh
  gmake
  ```

### Optimized FreeBSD compilation

* **FreeBSD** provides the **Clang** compiler as part of the base system.  While *sufficient* to build
  the simulator, we recommend that version 10 or later of the **GNU C** (`gcc`) compiler be used for
  optimal performance.
* At the time of writing, **GCC 12** is available for **FreeBSD** systems and is the version of GCC
  currently recommended by **The DPS8M Development Team**.  It can be installed via Packages or Ports.

  * Using FreeBSD Packages (as *root*):

    ```sh
    pkg install gcc12
	```
  * Using FreeBSD Ports (as *root*):

    ```sh
    cd /usr/ports/lang/gcc12/ && make install clean
	```

* Build the simulator from the top-level directory (using **GNU Make**):

  ```sh
  env CC="gcc12" LDFLAGS="-Wl,-rpath=/usr/local/lib/gcc12" gmake
  ```

### Additional FreeBSD Notes

* On **FreeBSD**, the simulator will use [**FreeBSD-specific** atomic operations](https://www.freebsd.org/cgi/man.cgi?query=atomic).
* The **FreeBSD**-specific **`atomic_testandset_64`** operation is currently not implemented on every platform **FreeBSD** supports (such as **powerpc64**). If you are unable to build the simulator due to an unimplemented atomic operation, specify `ATOMICS="GNU"` as an argument to `gmake`, or set this value in the shell environment before beginning the compilation.

<br>

---

## NetBSD

* Ensure you are running a [supported release](https://www.netbsd.org/releases/formal.html) of
  [**NetBSD**](https://www.netbsd.org) on a [supported platform](https://www.netbsd.org/ports/).
  * **NetBSD**/[**amd64**](https://www.netbsd.org/ports/amd64/) is regularly tested by **The DPS8M Development Team**.

### NetBSD prerequisites

Install the required prerequisites (using NetBSD Packages or [pkgsrc](https://www.pkgsrc.org/).)

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

* Build the simulator (*with standard settings*) from the top-level directory (using **GNU Make**):

  ```sh
  gmake
  ```

### Optimized NetBSD compilation

* **NetBSD** provides an older version of **GCC** or **Clang** as part of the base system (depending on the platform.)
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

* Build the simulator from the top-level directory (using **GNU Make**):

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

* Build the simulator from the top-level directory (using **GNU Make**):

  ```sh
  env CC="clang" \
    LDFLAGS="-L/usr/lib -L/usr/pkg/lib -fuse-ld="$(command -v ld.lld)" gmake
  ```

<br>

---

## OpenBSD

* Ensure you are running a [up-to-date](https://man.openbsd.org/syspatch.8) and
  [supported release](https://www.openbsd.org/) of [**OpenBSD**](https://www.openbsd.org/) on a
  [supported platform](https://www.openbsd.org/plat.html).
  * **OpenBSD**/[**amd64**](https://www.openbsd.org/amd64.html) and
    **OpenBSD**/[**arm64**](https://www.openbsd.org/arm64.html) are regularly tested by
	**The DPS8M Development Team**.
* The following instructions were verified using **OpenBSD 7.1** on
  [**amd64**](https://www.openbsd.org/amd64.html) and
  [**arm64**](https://www.openbsd.org/arm64.html).

### OpenBSD prerequisites

Install the required prerequisites (using OpenBSD Packages or Ports.)

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

* Build the simulator (*with standard settings*) from the top-level directory (using **GNU Make**):

  ```sh
  gmake
  ```

### Optimized OpenBSD compilation

* **OpenBSD** provides an older version of **GCC** or **Clang** as part of the base system (depending on the platform.)
  While *sufficient* to build the simulator, we recommend that a recent version of the **GNU assembler** (`gas`)
  and version 10 or later of the **GNU C** (`gcc`) compiler be used for optimal performance.

* At the time of writing, appropriate versions of the **GNU assembler** and **GNU C** (**GCC 11**)
  are available for **OpenBSD**.  (In addition, **LLD**, the LLVM linker, may be required.)  These tools have
  been tested and are highly recommended by **The DPS8M Development Team**. They can be installed via Packages or Ports.

  * Using OpenBSD Packages (as *root*):

	```sh
	pkg_add gas gcc
	```

  * Using OpenBSD Ports (as *root*):

    ```sh
	cd /usr/ports/devel/gas/ && make install clean
    cd /usr/ports/lang/gcc/11/ && make install clean
	```

* LLVM **LLD** (13.0.0 or later) is recommended for linking, even when using **GCC 11** for compilation.
  **LLD** is the default linker on ***most*** (*but not all*) supported OpenBSD platforms.  To determine the linker
  in use, examine the output of `ld --version`.  If a linker *other* than **LLD** is reported (e.g.
  *`GNU ld version 2.17`*, or similar), **LLD** must be installed, via Packages or Ports.

  * Using OpenBSD Packages (as *root*):

    ```sh
	pkg_add llvm
    ```

  * Using OpenBSD Ports (as *root*):

    ```sh
    cd /usr/ports/devel/llvm/ && make install clean
    ```

* **GCC** must be configured to execute the correct assembler (and linker).

  ```sh
  mkdir -p ~/.override
  test -x /usr/local/bin/gas && ln -fs /usr/local/bin/gas ~/.override/as
  test -x /usr/local/bin/lld && ln -fs /usr/local/bin/lld ~/.override/ld
  ```

* Build the simulator from the top-level directory (using **GNU Make**):

  ```sh
  env CC="egcc" CFLAGS="-B ~/.override" gmake
  ```

### Compilation using Clang

* **GCC** is recommended for optimal performance, but compilation using **Clang** is supported.
* A version of **Clang** newer than the base system version may be available via the **`llvm`** package or port.
* Once installed, it may be used by for compilation by setting `CC=/usr/local/bin/clang`.

### Additional OpenBSD Notes

* At the time of writing, **OpenBSD**/[**luna88k**](https://www.openbsd.org/luna88k.html) has not been tested.
  It should be possible to build the simulator for this architecture using the `gcc` compiler provided by the
  base system and specifying the `NO_LTO=1` build option.

<br>

---

## DragonFly BSD

* Ensure you are using the [**current release**](https://www.dragonflybsd.org/).
  Only the current release is supported by the upstream.
* At the time of writing, [**6.2.2-RELEASE**](https://www.dragonflybsd.org/download/) was current
  and used to verify the following instructions.

### DragonFly BSD prerequisites

* Install the required prerequisites (using DragonFly BSD DPorts.)

  ```sh
  pkg install gmake libuv
  ```

### Standard DragonFly BSD compilation

* Build the simulator (*with standard settings*) from the top-level directory (using **GNU Make**):

  ```sh
  env CFLAGS="-I/usr/local/include" \
     LDFLAGS="-L/usr/local/lib"     \
	 ATOMICS="GNU" gmake
  ```

### Optimized DragonFly BSD compilation

* **DragonFly BSD** provides an older version of **GCC** as part of the base system.  While this compiler is
  *sufficient* to build the simulator, we recommend that version 10 or later of the **GNU C** (`gcc`)
  compiler be used for optimal performance.

* At the time of writing, **GCC 11** is available and recommended by **The DPS8M Development Team**.

  * **GCC 11** may be installed using DragonFly BSD DPorts:

    ```sh
    pkg install gcc11
    ```

* Build the simulator from the top-level directory (using **GNU Make**):

  ```sh
  env CC="gcc11" CFLAGS="-I/usr/local/include"                 \
    LDFLAGS="-L/usr/local/lib -Wl,-rpath=/usr/local/lib/gcc11" \
    ATOMICS="GNU" gmake
  ```

### Compiling using Clang

* **GCC** is recommended for optimal performance, but compilation using **Clang** is supported.
* At the time of writing, **Clang 14** is available and recommended by **The DPS8M Development Team**.

  * **Clang 14** may be installed using DragonFly BSD DPorts.

	```sh
	pkg install llvm14
	```

* Build the simulator from the top-level directory (using **GNU Make**):

  ```sh
  env CC="clang14" CFLAGS="-I/usr/include -I/usr/local/include" \
    LDFLAGS="-L/usr/lib -L/usr/local/lib -fuse-ld=lld"          \
	ATOMICS="GNU" gmake
  ```

<br>

---

## Solaris

* Ensure you are using an up-to-date release. [**Oracle Solaris**](https://support.oracle.com/knowledge/Sun%20Microsystems/2433412_1.html) **11.4 SRU42** is recommended.
* The simulator can be built on **Solaris** using **GCC**, **Clang**, and **Oracle Developer Studio 12.6**.
* **GCC 11** is the recommended compiler for optimal performance on all Intel-based **Solaris** systems.
  * **GCC 11** can be installed from the standard IPS repository via `pkg install gcc-11`.
* Building with **Clang 11** or later is supported (*but not recommended due to lack of LTO support*).
  * **Clang 11** can be installed from the standard IPS repository via `pkg install clang@11 llvm@11`
* Building with the **Oracle Developer Studio 12.6** (`suncc`) compiler is also supported.
* Link-time optimization (*LTO*) is supported ***only*** when building with **GCC** version 10 or later.
  * The `NO_LTO=1` build option should be specified when compiling with an older version of GCC.

### Solaris prerequisites

* Install the required prerequisites from the standard IPS repository (as *root*):

  ```sh
  pkg install gnu-make gnu-binutils gnu-sed gnu-grep gnu-tar \
    gnu-coreutils gawk pkg-config autoconf automake wget
  ```

### Solaris compilation

The following commands download and build a local static **`libuv`** for use by the simulator.  If a site-provided
**`libuv`** library is installed in the **`/usr/local`** prefix, the **`libuvrel`** target and **`-j 1`** argument
may be omitted.

Build the simulator from the top-level directory (using **GNU Make**):

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

* Ensure your **OpenIndiana** installation is up-to-date. *Hipster* **2022-08-26** was used for verification.
* **GCC 11** is the recommended compiler for optimal performance.
  * **GCC 11** can be installed from the standard IPS repository via `pkg install gcc-11`.
* Building with **Clang 13** or later is also supported (*but not recommended due to lack of LTO support*).
  * **Clang 13** can be installed from the standard IPS repository via `pkg install clang-13`.
* Link-time optimization (*LTO*) is supported ***only*** when building with GCC version 10 or later.

### OpenIndiana prerequisites

* Install the requires prerequisites from the standard IPS repository (as *root*):

  ```sh
  pkg install gnu-make libuv gnu-binutils gnu-coreutils
  ```

### OpenIndiana compilation

Build the simulator from the top-level directory (using **GNU Make**):

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

* *TBD*

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

* *TBD*

<br>

---

## Windows

* *TBD*

<br>

---
