<!-- SPDX-License-Identifier: MIT-0
     Copyright (c) 2024-2025 The DPS8M Development Team
-->
### APT / DNF / Yum / Brew / Pacman Repositories

* [**Stable Release**](#stable-release)
* [**Bleeding Edge**](#bleeding-edge)

### Overview

* [**APT**](https://wiki.debian.org/AptCLI) repos are available for users of [**DEB**](https://wiki.debian.org/Teams/Dpkg)-based [**Linux**](https://kernel.org/) distributions (*e.g.* [*Ubuntu*](https://ubuntu.com/), [*Debian*](https://www.debian.org/), [*Raspberry Pi OS*](https://www.raspberrypi.com/software/), [*Mint*](https://linuxmint.com/), etc.)
  * **DEB** packages are provided for **`armhf`**, **`arm64`**, **`i386`**, **`powerpc`**, **`ppc64`**, **`ppc64el`**, **`riscv64`**, **`sparc64`**, **`loong64`**, **`s390x`**, and **`amd64`** systems.

[]()
[]()

* [**Termux**](https://termux.dev/) APT repos are available for [**Android**](https://www.android.com/) users.
  * **Termux** DEB packages are provided for **`arm`**, **`aarch64`**, **`i686`**, **`x86_64`** devices.

[]()
[]()

* [**DNF**](https://github.com/rpm-software-management/dnf) / [**Yum**](http://yum.baseurl.org/) repos are available for users of [**RPM**](https://rpm.org/)-based [**Linux**](https://kernel.org/) distributions (*e.g.* [*Fedora*](https://fedoraproject.org/), [*RHEL*](https://www.redhat.com/en/technologies/linux-platforms/enterprise-linux), [*CentOS*](https://www.centos.org/), [*Amazon Linux 2*](https://aws.amazon.com/amazon-linux-2), etc.)
  * **RPM** packages are provided for **`aarch64`**, **`i686`**, **`ppc64`**, **`ppc64le`**, **`riscv64`**, **`s390x`**, and **`x86_64`** systems.

[]()
[]()

* [**Brew**](https://formulae.brew.sh/) formulae are available for users of the [**Homebrew**](https://brew.sh/) package manager on [*macOS*](https://www.apple.com/macos/) (and [*Linux*](https://docs.brew.sh/Homebrew-on-Linux)).
  * **Brew** formulae are provided for **`arm64`** (Apple Silicon) and **`x86_64`** (Intel) systems.

[]()
[]()

* [**Pacman**](https://pacman.archlinux.page/) repos are available for users of [**Arch Linux**](https://archlinux.org/) and related distributions, including [**Arch Linux 32**](https://archlinux32.org/), and [**Arch Linux ARM**](https://archlinuxarm.org/).
  * **Pacman** packages are provided for **`armv7h`**, **`aarch64`**, **`i686`**, and **`x86_64`** systems.

## Stable Release

* [**APT** Instructions](#apt)
* [**Termux** Instructions](#termux)
* [**DNF** Instructions](#dnf)
* [**Yum** Instructions](#yum)
* [**Brew** Instructions](#brew)
* [**Pacman** Instructions](#pacman)

### APT

* **Linux** using **APT** (*as root*):

  ```sh
  apt install wget
  wget -O "/usr/share/keyrings/dps8m.gpg" "https://dps8m.gitlab.io/repo/pubkey.gpg"
  wget -O "/etc/apt/sources.list.d/dps8m-stable.list" "https://dps8m.gitlab.io/repo/deb/dps8m-stable.list"
  apt update
  apt install dps8m
  ```
<hr>

### Termux

* **Android** with **Termux**:

  ```sh
  pkg install wget
  wget -O "/data/data/com.termux/files/usr/share/termux-keyring/dps8m.gpg" "https://dps8m.gitlab.io/repo/pubkey.gpg"
  wget -O "/data/data/com.termux/files/usr/etc/apt/sources.list.d/dps8m-stable.list" "https://dps8m.gitlab.io/repo/ndk/dps8m-stable.list"
  pkg update
  pkg install dps8m
  ```

<hr>

### DNF

* Using **DNF** (*as root*):

  ```sh
  rpm --import "https://dps8m.gitlab.io/repo/pubkey.asc"
  dnf -y install 'dnf-command(config-manager)'
  dnf config-manager --add-repo "https://dps8m.gitlab.io/repo/rpm/dps8m.repo"
  dnf install dps8m
  ```

<hr>

### Yum

* Using **Yum** (*as root*):

  ```sh
  rpm --import "https://dps8m.gitlab.io/repo/pubkey.asc"
  yum -y install yum-utils
  yum-config-manager --add-repo "https://dps8m.gitlab.io/repo/rpm/dps8m.repo"
  yum install dps8m
  ```

<hr>

### Brew

* Using **Brew**:

  ```sh
  brew install dps8m
  ```

<hr>

### Pacman

* Using **Pacman** (*as root*):
  ```sh
  pacman-key --init
  curl -fsSL "https://dps8m.gitlab.io/repo/pubkey.asc" | pacman-key -a "-"
  pacman-key --lsign-key "391768816BEC63EAEFB4F4FCB10A52DBDE7A6FAA"
  ```

  []()
  []()

  * Add the following entry to your `/etc/pacman.conf` file:
    []()

    []()
    ```sh
    [dps8m-stable]
    Server = https://dps8m.gitlab.io/repo/arch/stable/$arch
    ```

  []()
  []()

  * Install DPS8M (*Stable Release*):
    []()

    []()
    ```sh
    pacman -Syu dps8m
    ```

<hr>

## Bleeding Edge

* [**APT** Instructions](#apt-1)
* [**Termux** Instructions](#termux-1)
* [**DNF** Instructions](#dnf-1)
* [**Yum** Instructions](#yum-1)
* [**Brew** Instructions](#brew-1)
* [**Pacman** Instructions](#pacman-1)

### APT

* Using **APT** (*as root*):

  ```sh
  apt install wget
  wget -O "/usr/share/keyrings/dps8m.gpg" "https://dps8m.gitlab.io/repo/pubkey.gpg"
  wget -O "/etc/apt/sources.list.d/dps8m-bleeding.list" "https://dps8m.gitlab.io/repo/deb/dps8m-bleeding.list"
  apt update
  apt install dps8m
  ```

<hr>

### Termux

* **Android** with **Termux**:

  ```sh
  pkg install wget
  wget -O "/data/data/com.termux/files/usr/share/termux-keyring/dps8m.gpg" "https://dps8m.gitlab.io/repo/pubkey.gpg"
  wget -O "/data/data/com.termux/files/usr/etc/apt/sources.list.d/dps8m-bleeding.list" "https://dps8m.gitlab.io/repo/ndk/dps8m-bleeding.list"
  pkg update
  pkg install dps8m
  ```

<hr>

### DNF

* Using **DNF** (*as root*):

  ```sh
  rpm --import "https://dps8m.gitlab.io/repo/pubkey.asc"
  dnf -y install 'dnf-command(config-manager)'
  dnf config-manager --add-repo "https://dps8m.gitlab.io/repo/rpm/dps8m-bleeding.repo"
  dnf install dps8m
  ```

<hr>

### Yum

* Using **Yum** (*as root*):

  ```sh
  rpm --import "https://dps8m.gitlab.io/repo/pubkey.asc"
  yum -y install yum-utils
  yum-config-manager --add-repo "https://dps8m.gitlab.io/repo/rpm/dps8m-bleeding.repo"
  yum install dps8m
  ```

<hr>

### Brew

* Using **Brew**:
  ```sh
  brew install dps8m --HEAD
  ```

  * **NOTE**: `brew` only upgrades *Bleeding Edge* installations when using the `--fetch-HEAD` option, for example:

    ```sh
    brew upgrade dps8m --fetch-HEAD
    ```

<hr>

### Pacman

* Using **Pacman** (*as root*):
  ```sh
  pacman-key --init
  curl -fsSL "https://dps8m.gitlab.io/repo/pubkey.asc" | pacman-key -a "-"
  pacman-key --lsign-key "391768816BEC63EAEFB4F4FCB10A52DBDE7A6FAA"
  ```

  []()
  []()

  * Add the following entry to your `/etc/pacman.conf` file (*above* **`dps8m-stable`**, if present):
    []()

    []()
    ```sh
    [dps8m-bleeding]
    Server = https://dps8m.gitlab.io/repo/arch/bleeding/$arch
    ```

  []()
  []()

  * Install DPS8M (*Bleeding Edge*):
    []()

    []()
    ```sh
    pacman -Syu dps8m
    ```

<hr>
