<!-- SPDX-License-Identifier: MIT-0
     Copyright (c) 2024 The DPS8M Development Team
-->
## DNF/Yum Repositories

* A *DNF*/*Yum* repo is available for users of **RPM**-based **Linux** distributions (*e.g.* *Fedora*, *RHEL*, *CentOS*, *Amazon Linux 2*, etc.)
* Packages are provided for `aarch64`, `i686`, `ppc64`, `ppc64le`, `riscv64`, `s390x`, and `x86_64` systems.

[]()
[]()

### Stable Release

* Using DNF (*as root*):
  ```sh
  dnf -y install 'dnf-command(config-manager)'
  dnf config-manager --add-repo https://dps8m.gitlab.io/repo/rpm/dps8m.repo
  dnf install dps8m
  ```

* Using Yum (*as root*):
  ```sh
  yum -y install yum-utils
  yum-config-manager --add-repo https://dps8m.gitlab.io/repo/rpm/dps8m.repo
  yum install dps8m
  ```

[]()
[]()

### Bleeding Edge

* Using DNF (*as root*):
  ```sh
  dnf -y install 'dnf-command(config-manager)'
  dnf config-manager --add-repo https://dps8m.gitlab.io/repo/rpm/dps8m-bleeding.repo
  dnf install dps8m
  ```

* Using Yum (*as root*):
  ```sh
  yum -y install yum-utils
  yum-config-manager --add-repo https://dps8m.gitlab.io/repo/rpm/dps8m-bleeding.repo
  yum install dps8m
  ```
