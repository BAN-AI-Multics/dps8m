<!-- SPDX-License-Identifier: LicenseRef-DPS8M-Doc OR LicenseRef-CF-GAL -->
<!-- SPDX-FileCopyrightText: 2022 The DPS8M Development Team -->
<!-- scspell-id: da80a3c8-3246-11ed-9446-80ee73e9b8e7 -->

### BUILDINFO (B)

"**`BUILDINFO`**" (*abbreviated* "`B`") shows build-time compilation information including the complete compiler flags (*i.e.* "`CFLAGS`", "`LDFLAGS`", etc.) used when building the simulator, the libraries the simulator is linked against and their versions, relevant definitions set by the C preprocessor at build-time, the types of file locking mechanisms available, and the style of atomic operation primitives in use.

**Example**

SHOWBUILDINFOHERE

### CLOCKS (CL)

"**`CLOCKS`**" (*abbreviated* "`CL`") shows the current wall clock time and internal timer details.

**Example**

SHOWCLOCKSHERE

### CONFIGURATION (C)

"**`CONFIGURATION`**" (*abbreviated* "`C`") shows a detailed overview of the current simulator configuration.

* See the **Default Base System Configuration** section of the **Simulator Defaults** chapter for example output of the "`SHOW CONFIGURATION`" command.

### DEFAULT_BASE_SYSTEM (D)

"**`DEFAULT_BASE_SYSTEM`**" (*abbreviated* "`D`") shows the *default base system configuration script* which is executed automatically at simulator startup or via the "**`DEFAULT_BASE_SYSTEM`**" command.

* See the documentation for the "**`DEFAULT_BASE_SYSTEM`**" command earlier in this chapter and the **Default Base System Script** section of the **Simulator Defaults** chapter for additional details.

### DEVICES (DEV)

"**`DEVICES`**" (*abbreviated* "`DEV`") shows devices by name and number of units (*i.e.* "**`NUNITS`**") configured.

**Example**

SHOWDEVICESHERE

### MODIFIERS (M)

"**`MODIFIERS`**" (*abbreviated* "`M`") shows a summary of available "`SET`" commands for all devices.

### ON (O)

"**`ON`**" (*abbreviated* "`O`") shows information about "`ON`" condition actions.

### PROM (P)

"**`PROM`**" (*abbreviated* "`P`") shows the CPU ID PROM initialization data.

**Example**

SHOWPROMHERE

### QUEUE (Q)

"**`Q`**" (*abbreviated* "`Q`") shows information about the simulator's event queue.

### SHOW (S)

"**`SHOW`**" (*abbreviated* "`S`") shows a summary of available "`SHOW`" commands for all devices.

### TIME (T)

"**`TIME`**" (*abbreviated* "`T`") shows information about the simulated timer.

### VERSION (V)

"**`VERSION`**" (*abbreviated* "`V`") shows the version of the simulator.

* If available, additional related information will be shown, which may include:
  * a *git commit hash* to uniquely identify the simulator build,
  * the date and time the simulator was last modified,
  * the date and time the *source kit distribution* the simulator was built from was prepared,
  * the date and time the simulator was compiled,
  * a statement regarding support status for the build (or lack thereof),
  * the name and architecture of the operating system that was used to build the simulator,
  * the name and version of the compiler used to build the simulator,
  * information about the person (or automated process) that performed the build,
  * the name, version, and architecture of the host operating system executing the simulator,
  * and information regarding licensing terms and conditions.

**Example**

SHOWVERSIONHERE


