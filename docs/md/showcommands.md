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

"**`QUEUE`**" (*abbreviated* "`Q`") shows information about the simulator's event queue.

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
  * a statement regarding support availability for the build (or lack thereof),
  * the name and architecture of the operating system that was used to build the simulator,
  * the name, version, and vendor of the compiler used to build the simulator,
  * information about the person (*or automated process*) that performed the build,
  * the name, version, and architecture of the host operating system executing the simulator,
  * and information regarding licensing terms and conditions.

**Example**

SHOWVERSIONHERE

### CPU

The following device-specific "`SHOW`" commands are available for the "`CPU`" device. If the "`SHOW CPU`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* "`CPU`**`0`**") is assumed.

**Example**

        SHOW CPU COMMAND
        SHOW CPUn COMMAND

#### CONFIG

"**`CONFIG`**" shows configuration details (set via "`SET CPU CONFIG`") for the "`CPU`" unit specified.

**Example**

SHOWCPUCONFIGHERE

#### NUNITS

"**`NUNITS`**" shows the number of "`CPU`" units configured (set via "`SET CPU NUNITS`").

**Example**

SHOWCPUNUNITSHERE

#### KIPS

"**`KIPS`**" shows the global CPU lockup fault scaling factor (set via "`SET CPU KIPS`").

**Example**

SHOWCPUKIPSHERE

#### STALL

"**`STALL`**" shows the currently configured stall points (set via "`SET CPU STALL`").

**Example**

SHOWCPUSTALLHERE

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET CPU DEBUG`").

**Example**

SHOWCPUDEBUGHERE


### DISK

The following device-specific "`SHOW`" commands are available for the "`DISK`" device. If the "`SHOW DISK`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* "`DISK`**`0`**") is assumed.

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET DISKn NAME`").

**Example**

SHOWDISKNAMEHERE

#### NUNITS

"**`NUNITS`**" shows the number of "`DISK`" units configured (set via "`SET DISK NUNITS`").

**Example**

SHOWDISKNUNITSHERE

#### TYPE

"**`TYPE`**" shows the currently configured disk type (set via "`SET DISK TYPE`").

**Example**

SHOWDISKTYPEHERE

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET DISK DEBUG`").

**Example**

SHOWDISKDEBUGHERE


### FNP

The following device-specific "`SHOW`" commands are available for the "`FNP`" device. If the "`SHOW FNP`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* "`FNP`**`0`**") is assumed.

#### CONFIG

"**`CONFIG`**" shows configuration details (set via "`SET FNPn CONFIG`") for the "`FNP`" unit specified.

**Example**

SHOWFNPCONFIGHERE

#### IPC_NAME

"**`IPC_NAME`**" shows the IPC name for the "`FNP`" unit specified (set via "`SET FNPn IPC_NAME`").

**Example**

SHOWFNPIPCNAMEHERE

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET FNPn NAME`").

**Example**

SHOWFNPNAMEHERE

#### NUNITS

"**`NUNITS`**" shows the number of "`FNP`" units configured (set via "`SET FNP NUNITS`").

**Example**

SHOWFNPNUNITSHERE

#### SERVICE

"**`SERVICE`**" shows the configured service for each line for the "`FNP`" unit specified.

* **NOTE**: Only the first ten lines of command output have been reproduced here; the remaining output has been excluded for brevity.

**Example**

SHOWFNPSERVICEHERE

#### STATUS

"**`STATUS`**" shows detailed line status information for the "`FNP`" unit specified.

* **NOTE**: Only the status of the first line has been reproduced here; the remaining output has been excluded for brevity.

**Example**

SHOWFNPSTATUSHERE

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET FNP DEBUG`").

**Example**

SHOWFNPDEBUGHERE

### IOM

The following device-specific "`SHOW`" commands are available for the "`IOM`" device. If the "`SHOW IOM`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* "`IOM`**`0`**") is assumed.

#### CONFIG

**Example**

"**`CONFIG`**" shows configuration details (set via "`SET IOMn CONFIG`") for the "`IOM`" unit specified.

SHOWIOMCONFIGHERE

#### NUNITS

"**`NUNITS`**" shows the number of "`IOM`" units configured (set via "`SET IOM NUNITS`").

**Example**

SHOWIOMNUNITSHERE

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET IOM DEBUG`").

SHOWIOMDEBUGHERE

### IPC

The following device-specific "`SHOW`" commands are available for the "`IPC`" device. If the "`SHOW IPC`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* "`IPC`**`0`**") is assumed.

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET IPCn NAME`").

**Example**

SHOWIPCNAMEHERE

#### NUNITS

"**`NUNITS`**" shows the number of "`IPC`" units configured (set via "`SET IPC NUNITS`").

**Example**

SHOWIPCNUNITSHERE

### MSP

The following device-specific "`SHOW`" commands are available for the "`MSP`" device. If the "`SHOW MSP`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* "`MSP`**`0`**") is assumed.

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET MSPn NAME`").

**Example**

SHOWMSPNAMEHERE

#### NUNITS

"**`NUNITS`**" shows the number of "`MSP`" units configured (set via "`SET MSP NUNITS`").

**Example**

SHOWMSPNUNITSHERE

### MTP

The following device-specific "`SHOW`" commands are available for the "`MTP`" device. If the "`SHOW MTP`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* "`MTP`**`0`**") is assumed.

#### BOOT_DRIVE

"**`BOOT_DRIVE`**" shows the currently configured boot drive for the unit specified (set via "`SET MTPn BOOT_DRIVE`").

**Example**

SHOWMTPBOOTDRIVEHERE

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET MTPn NAME`").

**Example**

SHOWMTPNAMEHERE

#### NUNITS

"**`NUNITS`**" shows the number of "`MTP`" units configured (set via "`SET MTP NUNITS`").

**Example**

SHOWMTPNUNITSHERE

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET MTP DEBUG`").

**Example**

SHOWMTPDEBUGHERE

### OPC

The following device-specific "`SHOW`" commands are available for the "`OPC`" device. If the "`SHOW OPC`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* "`OPC`**`0`**") is assumed.

#### CONFIG

"**`CONFIG`**" shows configuration details (set via "`SET OPCn CONFIG`") for the "`OPC`" unit specified.

**Example**

SHOWOPCCONFIGHERE

#### ADDRESS

"**`ADDRESS`**" shows the currently configured address for the specified unit (set via "`SET OPCn ADDRESS`").

**Example**

SHOWOPCADDRESSHERE

#### AUTOINPUT

"**`AUTOINPUT`**" shows the autoinput buffer of the specified `OPC` unit.

**Example**

SHOWOPCAUTOINPUTHERE

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET OPCn NAME`").

**Example**

SHOWOPCNAMEHERE

#### NUNITS

"**`NUNITS`**" shows the number of "`OPC`" units configured (set via "`SET OPC NUNITS`").

**Example**

SHOWOPCNUNITSHERE

#### PORT

"**`PORT`**" shows the currently configured port for the specified unit (set via "`SET OPCn PORT`").

**Example**

SHOWOPCPORTHERE

#### PW

"**`PW`**" shows the currently configured password for the specified unit (set via "`SET OPCn PW`").

**Example**

SHOWOPCPWHERE

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET OPC DEBUG`").

**Example**

SHOWOPCDEBUGHERE


### PRT

The following device-specific "`SHOW`" commands are available for the "`PRT`" device. If the "`SHOW PRT`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* "`PRT`**`0`**") is assumed.

#### CONFIG

"**`CONFIG`**" shows configuration details (set via "`SET PRTn CONFIG`") for the "`PRT`" unit specified.

**Example**

SHOWPRTCONFIGHERE

#### MODEL

"**`MODEL`**" shows configured model (set via "`SET PRTn MODEL`") for the "`PRT`" unit specified.

**Example**

SHOWPRTMODELHERE

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET PRTn NAME`").

**Example**

SHOWPRTNAMEHERE

#### NUNITS

"**`NUNITS`**" shows the number of "`PRT`" units configured (set via "`SET PRT NUNITS`").

**Example**

SHOWPRTNUNITSHERE

#### PATH

"**`PATH`**" shows the currently configured output path (set via "`SET PRT PATH`").

* An empty path is equivalent to the host operating system current working directory.

**Example**

SHOWPRTPATHHERE

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET PRT DEBUG`").

**Example**

SHOWPRTDEBUGHERE

### PUN

The following device-specific "`SHOW`" commands are available for the "`PUN`" device. If the "`SHOW PUN`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* "`PUN`**`0`**") is assumed.

#### CONFIG

"**`CONFIG`**" shows configuration details (set via "`SET PUNn CONFIG`") for the "`PUN`" unit specified.

**Example**

SHOWPUNCONFIGHERE

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET PUNn NAME`").

**Example**

SHOWPUNNAMEHERE

#### NUNITS

"**`NUNITS`**" shows the number of "`PUN`" units configured (set via "`SET PUN NUNITS`").

**Example**

SHOWPUNNUNITSHERE

#### PATH

"**`PATH`**" shows the currently configured output path. (set via "`SET PUN PATH`").

* An empty path is equivalent to the host operating system current working directory.

**Example**

SHOWPUNPATHHERE

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET PUN DEBUG`").

**Example**

SHOWPUNDEBUGHERE


### RDR

The following device-specific "`SHOW`" commands are available for the "`RDR`" device. If the "`SHOW RDR`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* "`RDR`**`0`**") is assumed.

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET RDRn NAME`").

**Example**

SHOWRDRNAMEHERE

#### NUNITS

"**`NUNITS`**" shows the number of "`RDR`" units configured (set via "`SET RDR NUNITS`").

**Example**

SHOWRDRNUNITSHERE

#### PATH

"**`PATH`**" shows the currently configured card reader directory path. (set via "`SET RDR PATH`").

* An empty path is equivalent to the host operating system current working directory.

**Example**

SHOWRDRPATHHERE

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET RDR DEBUG`").

**Example**

SHOWRDRDEBUGHERE


### SCU

The following device-specific "`SHOW`" commands are available for the "`SCU`" device. If the "`SHOW SCU`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* "`SCU`**`0`**") is assumed.

#### CONFIG

"**`CONFIG`**" shows configuration details (set via "`SET SCUn CONFIG`") for the "`SCU`" unit specified.

**Example**

SHOWSCUCONFIGHERE

#### NUNITS

"**`NUNITS`**" shows the number of "`SCU`" units configured (set via "`SET SCU NUNITS`").

**Example**

SHOWSCUNUNITSHERE

#### STATE

"**`STATE`**" shows detailed information about the state of the "`SCU`" unit specified.

**Example**

SHOWSCUSTATEHERE

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET SCU DEBUG`").

**Example**

SHOWSCUDEBUGHERE


### TAPE

The following device-specific "`SHOW`" commands are available for the "`TAPE`" device. If the "`SHOW TAPE`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* "`TAPE`**`0`**") is assumed.

#### ADD_PATH

"**`ADD_PATH`**" shows the currently configured tape directory search paths (set via "`SET TAPE ADD_PATH`").

**Example**

SHOWTAPEADDPATHHERE

#### CAPACITY

"**`CAPACITY`**" shows the storage capacity of the specified "`TAPE`" unit.

**Example**

SHOWTAPECAPACITYHERE

#### DEFAULT_PATH

"**`DEFAULT_PATH`**" shows the currently configured default tape path (set via "`SET TAPE DEFAULT_PATH`").

* An empty path is equivalent to the host operating system current working directory.

**Example**

SHOWTAPEDEFAULTPATHHERE

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET TAPEn NAME`").

**Example**

SHOWTAPENAMEHERE

#### NUNITS

"**`NUNITS`**" shows the number of "`TAPE`" units configured (set via "`SET TAPE NUNITS`").

**Example**

SHOWTAPENUNITSHERE

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET TAPE DEBUG`").

**Example**

SHOWTAPEDEBUGHERE


### URP

The following device-specific "`SHOW`" commands are available for the "`URP`" device. If the "`SHOW URP`" command can operate on a specific unit, but no unit is specified, the first unit (*i.e.* "`URP`**`0`**") is assumed.

#### NAME

"**`NAME`**" shows the currently configured device name (set via "`SET URPn NAME`").

**Example**

SHOWURPNAMEHERE

#### NUNITS

"**`NUNITS`**" shows the number of "`URP`" units configured (set via "`SET URP NUNITS`").

**Example**

SHOWURPNUNITSHERE

#### DEBUG

"**`DEBUG`**" shows the currently configured debug options (set via "`SET URP DEBUG`").

**Example**

SHOWURPDEBUGHERE


