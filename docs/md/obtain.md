
<!-- SPDX-License-Identifier: LicenseRef-DPS8M-Doc OR LicenseRef-CF-GAL -->
<!-- SPDX-FileCopyrightText: 2022 The DPS8M Development Team -->
<!-- scspell-id: 054d4b7a-41fb-11ed-ab70-80ee73e9b8e7 -->

<!-- pagebreak -->

# Obtaining the simulator

Official releases of the simulator are available from:

<!-- br -->

|    |    |    |
| -- | -- | -- |
| &nbsp;&nbsp;[**The DPS8M Simulator Homepage**](https://dps8m.gitlab.io/) <br> | [https://dps8m.gitlab.io/](https://dps8m.gitlab.io/) | *Binary distributions and source kits* |
| <br> &nbsp;&nbsp;[**GitLab DPS8M `git` Repository**](https://gitlab.com/dps8m/dps8m/) | [https://gitlab.com/dps8m/dps8m/](https://gitlab.com/dps8m/dps8m/) | *Developmental source code* |

<!-- br -->

<!-- br -->

## Binary distributions

<!-- br -->

* **The** [**DPS8M Simulator Homepage**](https://dps8m.gitlab.io/) offers [**binary releases**](https://dps8m.gitlab.io/dps8m/Releases/) for several popular operating systems, including:

  * **AIX**
  * **Linux**
  * **macOS**
  * **Windows**
  * **Solaris**
  * **illumos**
  * *and others* …

<!-- br -->

* Executables are provided for many hardware architectures, including:

  * **POWER** &nbsp; (*e.g.* IBM Power Systems, Raptor Talos™, Freescale PowerPC …)
  * **ARM** &nbsp; (*e.g.* Raspberry Pi, Pine64, Orange Pi …)
  * **ARM64** / **AArch64** &nbsp; (*e.g.* Apple M1, Fujitsu A64FX …)
  * **x86** / **ix86**
  * **x86_64** / **AMD64**

[]()

<!-- br -->

<!-- br -->

**NOTE**: Other operating systems and hardware architectures are supported when building the from source code.

<!-- pagebreak -->

## Source code distributions

### Source kits

The [**DPS8M Simulator Homepage**](https://dps8m.gitlab.io/) offers downloadable [**source kit distributions**](https://dps8m.gitlab.io/dps8m/Releases/) for released versions of the simulator, bleeding edge snapshots, and historical releases.

* **The DPS8M Development Team** recommends most new users who wish to build from source code download a source kit distribution from the **DPS8M Releases** section of [**The DPS8M Simulator Homepage**](https://dps8m.gitlab.io/).

* Advanced users and developers may wish to clone the **`git`** repository and work with the `master` branch.

<!-- br -->

### Git repository

**DPS8M** is development is coordinated using the [**`git`**](https://git-scm.com/) distributed version control system, hosted courtesy of [*GitLab*](https://gitlab.com/), providing project management tools, [wiki and web hosting](https://dps8m.gitlab.io), [issue tracking](https://gitlab.com/dps8m/dps8m/-/issues), and [CI/CD (*continuous integration*/*continuous delivery*) services](https://gitlab.com/dps8m/dps8m/-/pipelines).

Most development takes place on branches in the **`git`** repository, which are merged into the `master` branch after GitLab CI/CD verification and other manual testing.  Simulator releases are cut from the `master` branch.  The head of the **`git`** `master` branch is the version of the simulator used by most of the development team, and *should* be stable enough for daily usage, although regressions and new bugs may be encountered periodically.

* Clone the repository via HTTPS:

  ```sh
  git clone https://gitlab.com/dps8m/dps8m.git
  ```

* Users with a *GitLab* account may clone the repository via SSH:

  ```sh
  git clone git@gitlab.com:/dps8m/dps8m.git
  ```

  * After cloning the repository, it can be updated by executing the following command from the repository directory:

    ```sh
	git pull
	```
<!-- br -->

#### Git mirroring

Users who wish to **mirror** the [**git repository**](https://gitlab.com/dps8m/dps8m) for backup or archival purposes (*i.e.* copying **all** remote-tracking branches, tags, notes, references, etc.) should use the mirroring functionality of `git`:

* Mirror the repository via HTTPS:

  ```sh
  git clone --mirror https://gitlab.com/dps8m/dps8m.git
  ```

* Users with a *GitLab* account may mirror the repository via SSH:

  ```sh
  git clone --mirror git@gitlab.com:/dps8m/dps8m.git
  ```

  * After cloning the repository, it can be updated, including removing local branches when they are removed upstream, by executing the following command from the repository directory:

    ```sh
    git remote update --prune
    ```

<!-- br -->

