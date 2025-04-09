<!-- SPDX-License-Identifier: LicenseRef-CF-GAL -->
<!-- SPDX-FileCopyrightText: 2022-2025 The DPS8M Development Team -->
<!-- scspell-id: da80a3c8-3246-11ed-9446-80ee73e9b8e7 -->

<!-- br -->

<!------------------------------------------------------------------------------------->

### BUILDINFO (B)

"**`BUILDINFO`**" (*abbreviated* "`B`") shows build-time compilation information including the complete compiler flags (*i.e.* "`CFLAGS`", "`LDFLAGS`", etc.) used when building the simulator, the libraries the simulator is statically and dynamically linked against and their versions, relevant definitions set by the C preprocessor at build-time, the types of file locking mechanisms available, and the style of atomic operation primitives in use.

**Example**

SHOWBUILDINFOHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### CLOCKS (CL)

"**`CLOCKS`**" (*abbreviated* "`CL`") shows the current wall clock time and internal timer details.

**Example**

SHOWCLOCKSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### CONFIGURATION (C)

"**`CONFIGURATION`**" (*abbreviated* "`C`") shows a detailed overview of the current simulator configuration.

* See the **Default Base System Configuration** section of the **Simulator Defaults** chapter for example output of the "`SHOW CONFIGURATION`" command.

<!-- br -->

<!------------------------------------------------------------------------------------->

### DEFAULT_BASE_SYSTEM (D)

"**`DEFAULT_BASE_SYSTEM`**" (*abbreviated* "`D`") shows the *default base system configuration script* which is executed automatically at simulator startup or via the "**`DEFAULT_BASE_SYSTEM`**" command.

* See the documentation for the "**`DEFAULT_BASE_SYSTEM`**" command earlier in this chapter and the **Default Base System Script** section of the **Simulator Defaults** chapter for additional details.

<!-- br -->

<!------------------------------------------------------------------------------------->

### DEVICES (DEV)

"**`DEVICES`**" (*abbreviated* "`DEV`") shows devices by name and number of units (*i.e.* "**`NUNITS`**") configured.

**Example**

SHOWDEVICESHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### MODIFIERS (M)

"**`MODIFIERS`**" (*abbreviated* "`M`") shows a summary of available "`SET`" commands for all devices.

<!-- br -->

<!------------------------------------------------------------------------------------->

### ON (O)

"**`ON`**" (*abbreviated* "`O`") shows information about "`ON`" condition actions.

<!-- br -->

<!------------------------------------------------------------------------------------->

### PROM (P)

"**`PROM`**" (*abbreviated* "`P`") shows the CPU ID PROM initialization data.

**Example**

SHOWPROMHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### QUEUE (Q)

"**`QUEUE`**" (*abbreviated* "`Q`") shows information about the simulator's event queue.

<!-- br -->

<!------------------------------------------------------------------------------------->

### SHOW (S)

"**`SHOW`**" (*abbreviated* "`S`") shows a summary of available "`SHOW`" commands for all devices.

<!-- br -->

<!------------------------------------------------------------------------------------->

### TIME (T)

"**`TIME`**" (*abbreviated* "`T`") shows information about the simulated timer.

<!-- br -->

<!------------------------------------------------------------------------------------->

### VERSION (V)

"**`VERSION`**" (*abbreviated* "`V`") shows the version of the simulator.

* If available, additional related information will be shown, which may include:
  * a *git commit hash* to uniquely identify the simulator build,
  * the date and time the simulator was last modified,
  * the date and time the *source kit distribution* the simulator was built from was prepared,
  * the date and time the simulator was compiled,
  * a statement regarding support availability for the build (or lack thereof),
  * the name and architecture of the operating system that was used to build the simulator,
  * the name, version, and vendor of the compiler used to build the simulator,
  * information about the person (*or automated process*) that performed the build,
  * the name, version, and architecture of the host operating system executing the simulator,
  * and information regarding licensing terms and conditions.

**Example**

SHOWVERSIONHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### CPU

The following device-specific "`SHOW`" commands are available for the "`CPU`" device. If the "`SHOW CPU`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* `CPU`**`0`**) is assumed.

**Example**

        SHOW CPU COMMAND
        SHOW CPUn COMMAND

<!-- br -->

<!------------------------------------------------------------------------------------->

#### CONFIG

"**`CONFIG`**" shows configuration details (set via "`SET CPU CONFIG`") for the "`CPU`" unit specified.

**Example**

SHOWCPUCONFIGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS

"**`NUNITS`**" shows the number of "`CPU`" units configured (set via "`SET CPU NUNITS`").

**Example**

SHOWCPUNUNITSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### KIPS

"**`KIPS`**" shows the global CPU lockup fault scaling factor (set via "`SET CPU KIPS`").

**Example**

SHOWCPUKIPSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### STALL

"**`STALL`**" shows the currently configured stall points (set via "`SET CPU STALL`").

**Example**

SHOWCPUSTALLHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET CPU DEBUG`").

**Example**

SHOWCPUDEBUGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### DISK

The following device-specific "`SHOW`" commands are available for the "`DISK`" device. If the "`SHOW DISK`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* `DISK`**`0`**) is assumed.

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET DISKn NAME`").

**Example**

SHOWDISKNAMEHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS

"**`NUNITS`**" shows the number of "`DISK`" units configured (set via "`SET DISK NUNITS`").

**Example**

SHOWDISKNUNITSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### TYPE

"**`TYPE`**" shows the currently configured disk type (set via "`SET DISK TYPE`").

**Example**

SHOWDISKTYPEHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET DISK DEBUG`").

**Example**

SHOWDISKDEBUGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### FNP

The following device-specific "`SHOW`" commands are available for the "`FNP`" device. If the "`SHOW FNP`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* `FNP`**`0`**) is assumed.

<!-- br -->

<!------------------------------------------------------------------------------------->

#### CONFIG

"**`CONFIG`**" shows configuration details (set via "`SET FNPn CONFIG`") for the "`FNP`" unit specified.

**Example**

SHOWFNPCONFIGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### IPC_NAME

"**`IPC_NAME`**" shows the IPC name for the "`FNP`" unit specified (set via "`SET FNPn IPC_NAME`").

**Example**

SHOWFNPIPCNAMEHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET FNPn NAME`").

**Example**

SHOWFNPNAMEHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS

"**`NUNITS`**" shows the number of "`FNP`" units configured (set via "`SET FNP NUNITS`").

**Example**

SHOWFNPNUNITSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### SERVICE

"**`SERVICE`**" shows the configured service for each line for the "`FNP`" unit specified.

* **NOTE**: Only the first ten lines of command output have been reproduced here; the remaining output has been excluded for brevity.

**Example**

SHOWFNPSERVICEHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### STATUS

"**`STATUS`**" shows detailed line status information for the "`FNP`" unit specified.

* **NOTE**: Only the status of the first line has been reproduced here; the remaining output has been excluded for brevity.

**Example**

SHOWFNPSTATUSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET FNP DEBUG`").

**Example**

SHOWFNPDEBUGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### IOM

The following device-specific "`SHOW`" commands are available for the "`IOM`" device. If the "`SHOW IOM`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* `IOM`**`0`**) is assumed.

<!-- br -->

<!------------------------------------------------------------------------------------->

#### CONFIG

**Example**

"**`CONFIG`**" shows configuration details (set via "`SET IOMn CONFIG`") for the "`IOM`" unit specified.

SHOWIOMCONFIGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS

"**`NUNITS`**" shows the number of "`IOM`" units configured (set via "`SET IOM NUNITS`").

**Example**

SHOWIOMNUNITSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET IOM DEBUG`").

SHOWIOMDEBUGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### IPC

The following device-specific "`SHOW`" commands are available for the "`IPC`" device. If the "`SHOW IPC`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* `IPC`**`0`**) is assumed.

<!------------------------------------------------------------------------------------->

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET IPCn NAME`").

**Example**

SHOWIPCNAMEHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS

"**`NUNITS`**" shows the number of "`IPC`" units configured (set via "`SET IPC NUNITS`").

**Example**

SHOWIPCNUNITSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### MSP

The following device-specific "`SHOW`" commands are available for the "`MSP`" device. If the "`SHOW MSP`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* `MSP`**`0`**) is assumed.

<!------------------------------------------------------------------------------------->

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET MSPn NAME`").

**Example**

SHOWMSPNAMEHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS

"**`NUNITS`**" shows the number of "`MSP`" units configured (set via "`SET MSP NUNITS`").

**Example**

SHOWMSPNUNITSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### MTP

The following device-specific "`SHOW`" commands are available for the "`MTP`" device. If the "`SHOW MTP`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* `MTP`**`0`**) is assumed.

<!------------------------------------------------------------------------------------->

#### BOOT_DRIVE

"**`BOOT_DRIVE`**" shows the currently configured boot drive for the unit specified (set via "`SET MTPn BOOT_DRIVE`").

**Example**

SHOWMTPBOOTDRIVEHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET MTPn NAME`").

**Example**

SHOWMTPNAMEHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS

"**`NUNITS`**" shows the number of "`MTP`" units configured (set via "`SET MTP NUNITS`").

**Example**

SHOWMTPNUNITSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET MTP DEBUG`").

**Example**

SHOWMTPDEBUGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### OPC

The following device-specific "`SHOW`" commands are available for the "`OPC`" device. If the "`SHOW OPC`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* `OPC`**`0`**) is assumed.

<!------------------------------------------------------------------------------------->

#### CONFIG

"**`CONFIG`**" shows configuration details (set via "`SET OPCn CONFIG`") for the "`OPC`" unit specified.

**Example**

SHOWOPCCONFIGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### ADDRESS

"**`ADDRESS`**" shows the currently configured address for the specified unit (set via "`SET OPCn ADDRESS`").

**Example**

SHOWOPCADDRESSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### AUTOINPUT

"**`AUTOINPUT`**" shows the autoinput buffer of the specified `OPC` unit.

**Example**

SHOWOPCAUTOINPUTHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET OPCn NAME`").

**Example**

SHOWOPCNAMEHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS

"**`NUNITS`**" shows the number of "`OPC`" units configured (set via "`SET OPC NUNITS`").

**Example**

SHOWOPCNUNITSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### PORT

"**`PORT`**" shows the currently configured port for the specified unit (set via "`SET OPCn PORT`").

**Example**

SHOWOPCPORTHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### PW

"**`PW`**" shows the currently configured password for the specified unit (set via "`SET OPCn PW`").

**Example**

SHOWOPCPWHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET OPC DEBUG`").

**Example**

SHOWOPCDEBUGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### PRT

The following device-specific "`SHOW`" commands are available for the "`PRT`" device. If the "`SHOW PRT`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* `PRT`**`0`**) is assumed.

<!-- br -->

<!------------------------------------------------------------------------------------->

#### CONFIG

"**`CONFIG`**" shows configuration details (set via "`SET PRTn CONFIG`") for the "`PRT`" unit specified.

**Example**

SHOWPRTCONFIGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### MODEL

"**`MODEL`**" shows configured model (set via "`SET PRTn MODEL`") for the "`PRT`" unit specified.

**Example**

SHOWPRTMODELHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET PRTn NAME`").

**Example**

SHOWPRTNAMEHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS

"**`NUNITS`**" shows the number of "`PRT`" units configured (set via "`SET PRT NUNITS`").

**Example**

SHOWPRTNUNITSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### PATH

"**`PATH`**" shows the currently configured output path (set via "`SET PRT PATH`").

* The default path is the current working directory of the host operating system.

**Example**

SHOWPRTPATHHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET PRT DEBUG`").

**Example**

SHOWPRTDEBUGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### PUN

The following device-specific "`SHOW`" commands are available for the "`PUN`" device. If the "`SHOW PUN`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* `PUN`**`0`**) is assumed.

For details on usage and configuration of "`PUN`" devices, see the "`SET PUN`" section of this documentation.

#### CONFIG

"**`CONFIG`**" shows configuration details (set via "`SET PUNn CONFIG`") for the "`PUN`" unit specified.

**Example**

SHOWPUNCONFIGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET PUNn NAME`").

**Example**

SHOWPUNNAMEHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS

"**`NUNITS`**" shows the number of "`PUN`" units configured (set via "`SET PUN NUNITS`").

**Example**

SHOWPUNNUNITSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### PATH

"**`PATH`**" shows the currently configured output path. (set via "`SET PUN PATH`").

* An empty path is equivalent to the host operating system current working directory.

**Example**

SHOWPUNPATHHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET PUN DEBUG`").

**Example**

SHOWPUNDEBUGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### RDR

The following device-specific "`SHOW`" commands are available for the "`RDR`" device. If the "`SHOW RDR`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* `RDR`**`0`**) is assumed.

For details on usage and configuration of "`RDR`" devices, see the "`SET RDR`" section of this documentation.

<!------------------------------------------------------------------------------------->

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET RDRn NAME`").

**Example**

SHOWRDRNAMEHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS

"**`NUNITS`**" shows the number of "`RDR`" units configured (set via "`SET RDR NUNITS`").

**Example**

SHOWRDRNUNITSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### PATH

"**`PATH`**" shows the currently configured card reader directory path. (set via "`SET RDR PATH`").

* An empty path is equivalent to the host operating system current working directory.

**Example**

SHOWRDRPATHHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET RDR DEBUG`").

**Example**

SHOWRDRDEBUGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### SCU

The following device-specific "`SHOW`" commands are available for the "`SCU`" device. If the "`SHOW SCU`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* `SCU`**`0`**) is assumed.

#### CONFIG

"**`CONFIG`**" shows configuration details (set via "`SET SCUn CONFIG`") for the "`SCU`" unit specified.

**Example**

SHOWSCUCONFIGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS

"**`NUNITS`**" shows the number of "`SCU`" units configured (set via "`SET SCU NUNITS`").

**Example**

SHOWSCUNUNITSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### STATE

"**`STATE`**" shows detailed information about the state of the "`SCU`" unit specified.

**Example**

SHOWSCUSTATEHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET SCU DEBUG`").

**Example**

SHOWSCUDEBUGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### TAPE

The following device-specific "`SHOW`" commands are available for the "`TAPE`" device. If the "`SHOW TAPE`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* `TAPE`**`0`**) is assumed.

<!------------------------------------------------------------------------------------->

#### ADD_PATH

"**`ADD_PATH`**" shows the currently configured tape directory search paths (set via "`SET TAPE ADD_PATH`").

**Example**

SHOWTAPEADDPATHHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### CAPACITY

"**`CAPACITY`**" shows the storage capacity of the specified "`TAPE`" unit.

**Example**

SHOWTAPECAPACITYHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### DEFAULT_PATH

"**`DEFAULT_PATH`**" shows the currently configured default tape path (set via "`SET TAPE DEFAULT_PATH`").

* An empty path is equivalent to the host operating system current working directory.

**Example**

SHOWTAPEDEFAULTPATHHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET TAPEn NAME`").

**Example**

SHOWTAPENAMEHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS

"**`NUNITS`**" shows the number of "`TAPE`" units configured (set via "`SET TAPE NUNITS`").

**Example**

SHOWTAPENUNITSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET TAPE DEBUG`").

**Example**

SHOWTAPEDEBUGHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

### URP

The following device-specific "`SHOW`" commands are available for the "`URP`" device. If the "`SHOW URP`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* `URP`**`0`**) is assumed.

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET URPn NAME`").

**Example**

SHOWURPNAMEHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS

"**`NUNITS`**" shows the number of "`URP`" units configured (set via "`SET URP NUNITS`").

**Example**

SHOWURPNUNITSHERE

<!-- br -->

<!------------------------------------------------------------------------------------->

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET URP DEBUG`").

**Example**

SHOWURPDEBUGHERE

