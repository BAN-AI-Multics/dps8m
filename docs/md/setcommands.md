<!-- SPDX-License-Identifier: LicenseRef-CF-GAL -->
<!-- SPDX-FileCopyrightText: 2022-2025 The DPS8M Development Team -->
<!-- scspell-id: 65252877-3180-11ed-bcd8-80ee73e9b8e7 -->

### CPU Configuration

#### L68 (DPS8M)
"**`L68`**" configures the "`CPU`" unit specified to simulate a **Level 68** processor, where "`1`" or "`enable`" configures the CPU as a **Level 68** and "`0`" or "`disable`" configures the CPU as a **DPS‑8/M**.

"**`DPS8M`**" configures the "`CPU`" unit specified to simulate a **DPS‑8/M** processor, where "`1`" or "`enable`" configures the CPU as a **DPS‑8/M** and "`0`" or "`disable`" configures the CPU as a **Level 68**:

        L68=<0 or 1>
        L68=<disable or enable>
        DPS8M=<0 or 1>
        DPS8M=<disable or enable>

**Examples**

* Configure CPU**`0`** to simulate a **Level 68** processor. (The default is to simulate a **DPS‑8/M**):
        SET CPU0 L68=1
        SET CPU0 DPS8M=0

* Configure CPU**`1`** to simulate a **DPS‑8/M** processor:
        SET CPU0 L68=0
        SET CPU0 DPS8M=1

<!------------------------------------------------------------------------------------->

<!-- br -->

#### RESET
"**`RESET`**" will reset the specified "`CPU`" unit (or *all* "`CPU`" units when no unit is explicitly specified) when passed an argument of "`1`".

        RESET=<1>

**Examples**

* Reset all CPUs:
        SET CPU RESET=1


* Reset CPU**`0`**:
        SET CPU0 RESET=1

<!------------------------------------------------------------------------------------->

<!-- br -->

#### INITIALIZE
"**`INITIALIZE`**" will initialize the specified "`CPU`" unit (or *all* "`CPU`" units when no unit is explicitly specified) when passed an argument of "`1`".

        INITIALIZE=<1>

**Examples**

* Initialize all CPUs:
        SET CPU INITIALIZE=1


* Initialize CPU**`0`**:
        SET CPU0 INITIALIZE=1

<!------------------------------------------------------------------------------------->

<!-- br -->

#### INITIALIZEANDCLEAR (IAC)
"**`INITIALIZEANDCLEAR`**" (*abbreviated* "**`IAC`**") will initialize the specified "`CPU`" unit and clear it's state (or initialize *all* "`CPU`" units and clear their states when no unit is explicitly specified) when passed an argument of "`1`".

        INITIALIZEANDCLEAR=<1>
        IAC=<1>

**Examples**

* Initialize and clear CPUs:
        SET CPU INITIALIZEANDCLEAR=1


* Initialize and clear CPU**`0`**:
        SET CPU0 IAC=1

<!------------------------------------------------------------------------------------->

<!-- br -->

#### NUNITS
"**`NUNITS`**" configures the number of "`CPU`" units.

        NUNITS=<n>

**Example**

* Set the number of "`CPU`" units to "`6`" (*the default number*):
        SET CPU NUNITS=6

<!------------------------------------------------------------------------------------->

<!-- br -->

#### KIPS
"**`KIPS`**" configures the global CPU lockup fault timer scaling factor.

        KIPS=<n>

When simulating multiple processors, occasional spurious "*Fault in idle process*"
messages may be seen when additional CPU's are brought online. This is caused
by a lockup fault occurring while the Multics hardcore is waiting on a flag to
be set by the CPU being brought online.

Latency guarantees of the original hardware prevent this condition from occurring when
the hardware is properly functioning.  Unfortunately, without introducing strict
hard-realtime requirements for simulated operations such as requiring the simulator
execute under the control of a hard-realtime RTOS, there is no practical way to provide
Multics these precision timing guarantees.

When running on original hardware, the lockup fault would be triggered by an independent
watchdog timer that monitors the CPU's polling of interrupts.  If interrupts would be
inhibited for too long, this watchdog timer would run out and trigger the lockup fault.
Multics rightfully considers a lockup fault in hardcore as indicative of a serious
(*possibly fatal*) problem with the faulting CPU.

Unfortunately, what is a clear sign of hardware trouble on real hardware (*i.e* a CPU
that is struggling to get up to speed, or running at fluctuating speeds) is the normal,
expected, and unavoidable reality when the simulator is running on a general purpose
system.

Configuring the "`KIPS`" scaling factor loosens this hard realtime requirement and
avoids spurious lockup faults.

Note that if "`KIPS`" is set to a large value, a genuine hardcore error or broken
userspace process will still be detected, although it will take slightly longer
for the lockup fault to occur.

**Example**

* Set the global CPU lockup fault scaling factor for 8 MIPS:
        SET CPU KIPS=8000


It is not expected that the "`KIPS`" value will require modification by most
users under normal operating circumstances. See also: "**`STALL`**".

<!------------------------------------------------------------------------------------->

<!-- br -->

#### STALL
"**`STALL`**" configures a stall (*i.e.* a delay) of "**`<iterations>`**" when
the IC reaches the address specified by "**`<segno>`**" and "**`<offset>`**", identified by
"**`<num>`**".

        STALL=<num>=<segno>:<offset>=<iterations>

* "**`<num>`**" is a number allowing for the identification (and modification) of multiple stall points.
* "**`<segno>`**" is the segment number of the Multics segment at which to stall.
* "**`<offset>`**" is the offset within the segment specified by "**`<segno>`**" at which to stall.
* "**`<iterations>`**" is a positive integer representing the number of iterations to loop.

**Example**

* Configure two stalls, #`1` for `250` iterations at "`0132:011227`", and #`2` for `1250` iterations at "`0122:052137`":
        SET CPU STALL=1=0132:011227=250
        SET CPU STALL=2=0122:052137=1250

<!------------------------------------------------------------------------------------->

<!-- br -->

#### DEBUG (NODEBUG)
"**`DEBUG`**" enables CPU debugging and/or enables specified debugging options.

"**`NODEBUG`**" disables CPU debugging and/or disables specified debugging options.

        DEBUG                                 ; Enables debugging
        NODEBUG                               ; Disables debugging
        DEBUG=<list-of-options>               ; Enables specified debugging options
        NODEBUG=<list-of-options>             ; Disables specified debugging options

* The "**`<list-of-options>`**" is a semicolon ("`;`") delimited list of one or more of the following options:

|                   |                  |                   |                  |                  |
|:-----------------:|:----------------:|:-----------------:|:----------------:|:----------------:|
| **`TRACE`**       | **`TRACEEXT`**   | **`MESSAGES`**    | **`REGDUMPAQI`** | **`REGDUMPIDX`** |
| **`REGDUMPPR`**   | **`REGDUMPPPR`** | **`REGDUMPDSBR`** | **`REGDUMPFLT`** | **`REGDUMP`**    |
| **`ADDRMOD`**     | **`APPENDING`**  | **`NOTIFY`**      | **`INFO`**       | **`ERR`**        |
| **`WARN`**        | **`DEBUG`**      | **`ALL`**         | **`FAULT`**      | **`INTR`**       |
| **`CORE`**        | **`CYCLE`**      | **`CAC`**         | **`FINAL`**      | **`AVC`**        |

**Examples**

* Enable the debugging options "`TRACE`" and "`FAULT`" for all "`CPU`" units:
        SET CPU DEBUG=TRACE;FAULT


* Disable all debugging options for all "`CPU`" units:
        SET CPU NODEBUG


* Enable the debugging option "`CYCLE`" for CPU**`0`** and disable all debugging options for all other "`CPU`" units:
        SET CPU NODEBUG
        SET CPU0 DEBUG=CYCLE

<!------------------------------------------------------------------------------------->

<!-- br -->

#### CONFIG

The following "**`CPU`**" configuration options are associated with a specified "`CPU`" unit (*i.e.* CPU**`n`**) and are configured using the "`SET`" command (*i.e.* "**`SET CPUn CONFIG`**"):

        SET CPUn CONFIG=<set-option>

* The following "**`<set-option>`**"'s are available, in the form of "**`<option>=<value>`**", for example:
        SET CPUn CONFIG=<option>=<value>

<!------------------------------------------------------------------------------------->

<!-- br -->

##### FAULTBASE
\
\
"**`FAULTBASE`**" configures the fault base of the specified "`CPU`" unit:

        FAULTBASE=<value>

**Example**

* For Multics operation, configure the "**`<value>`**" to "**`Multics`**" (for CPU**`0`**):
        SET CPU0 CONFIG=FAULTBASE=Multics

<!------------------------------------------------------------------------------------->

<!-- br -->

##### NUM
\
\
"**`NUM`**" configures the CPU number of the specified "`CPU`" unit:

        NUM=<n>

**Example**

* Set the CPU number of CPU**`0`** to **`1`**.
        SET CPU0 CONFIG=NUM=1

<!------------------------------------------------------------------------------------->

<!-- br -->

##### DATA
\
\
"**`DATA`**" configures the CPU switches of the specified "`CPU`" unit:

        DATA=<word>

**Example**

* Set CPU**`0`** switches to "`024000717200`" (*the default value*).
        SET CPU0 CONFIG=DATA=024000717200

**See Also**

Refer to *GB61-01* **Operators Guide, Appendix A** for more details.

<!------------------------------------------------------------------------------------->

<!-- br -->

##### STOPNUM
\
\
"**`STOPNUM`**" configures the CPU switches of the specified "`CPU`" unit so that Multics will stop during boot at the check stop specified by "**`<n>`**".

        STOPNUM=<n>

**Example**

* Set CPU**`0`** switches so Multics will stop during boot at check stop #`2030`.
        SET CPU0 CONFIG=STOPNUM=2030

<!------------------------------------------------------------------------------------->

<!-- br -->

##### MODE
\
\
"**`MODE`**" configures the operating mode of specified "`CPU`" unit as indicated by "**`<value>`**":

        MODE=<value>

* The supported "**`<value>`**"'s are:

| &nbsp;**`<value>`**                  | Operating mode             |
| ------------------------------------ | -------------------------- |
| &nbsp;"**`0`**" or "**`GCOS`** "     | **GCOS** operating mode    |
| &nbsp;"**`1`**" or "**`Multics`**"   | **Multics** operating mode |

**Examples**

* Set the operating mode of CPU**`1`** to **GCOS** operating mode:
        SET CPU1 CONFIG=MODE=0

* Set the operating mode of CPU**`0`** to **Multics** operating mode:
        SET CPU0 CONFIG=MODE=Multics

<!------------------------------------------------------------------------------------->

<!-- br -->

##### SPEED
\
\
"**`SPEED`**" configures the CPU speed setting of the specified `CPU` unit, where an "**`<identifier>`**" of "**`0`**" indicates a **DPS‑8/M Model 70**.

        SPEED=<identifier>

**Example**

* Configure CPU**`0`** with a speed setting of "**`0`**", indicating a **DPS‑8/M Model 70** (*the default setting*):
        SET CPU0 CONFIG SPEED=0

<!------------------------------------------------------------------------------------->

<!-- br -->

##### PORT
\
\
"**`PORT`**" selects the CPU port "**`<p>`**" for which subsequent "`SET CPUn CONFIG=`" configuration commands apply:

        PORT=<p>

**Example**

* Configure `ASSIGNMENT`, `INTERLACE`, `ENABLE`, `INIT_ENABLE`, and `STORE_SIZE` for port "**A**" on CPU**`0`**:
        SET CPU0 CONFIG=PORT=A
        SET CPU0   CONFIG=ASSIGNMENT=0
        SET CPU0   CONFIG=INTERLACE=0
        SET CPU0   CONFIG=ENABLE=1
        SET CPU0   CONFIG=INIT_ENABLE=1
        SET CPU0   CONFIG=STORE_SIZE=4M

<!------------------------------------------------------------------------------------->

<!-- br -->

##### ASSIGNMENT
\
\
"**`ASSIGNMENT`**" configures the CPU port assignment to "**`<a>`**" of "`CPU`" unit "**`n`**"'s CPU port "**`x`**", as specified by a previous "**`SET CPUn CONFIG=PORT=x`**" command:

        ASSIGNMENT=<a>

**Example**

* Configure the CPU port assignment of CPU**`1`**'s port "**`B`**" to "**`2`**":
        SET CPU1 CONFIG=PORT=B
        SET CPU1   CONFIG=ASSIGNMENT=2

<!------------------------------------------------------------------------------------->

<!-- br -->

##### INTERLACE
\
\
"**`INTERLACE`**" configures the position of the *interlace* switch for the currently selected CPU port:

        INTERLACE=<0, 1, 2>

**Example**

* Configure the position of the *interlace* switch for the currently selected CPU port of CPU**`0`** to "**`1`**":
        SET CPU0 CONFIG=INTERLACE=1

<!------------------------------------------------------------------------------------->

<!-- br -->

##### ENABLE
\
\
"**`ENABLE`**" configures whether the currently selected CPU port is enabled, where "`0`" or "`disable`" is disabled and "`1`" or "`enable`" is enabled:

        ENABLE=<0 or 1>
        ENABLE=<disable or enable>

<!------------------------------------------------------------------------------------->

<!-- br -->

##### INIT_ENABLE
\
\
"**`INIT_ENABLE`**" configures whether *init* is enabled for the currently selected CPU port, where "`0`" or "`disable`" is disabled and "`1`" or "`enable`" is enabled:

        INIT_ENABLE=<0 or 1>
        INIT_ENABLE=<disable or enable>

<!------------------------------------------------------------------------------------->

<!-- br -->

##### STORE_SIZE
\
\
"**`STORE_SIZE`**" configures the size of memory (in *megawords*) for the currently selected CPU port:

        STORE_SIZE=<num>M

**Example**

* Configure the memory size of the currently selected CPU port on CPU**`0`** to "**`4`**" megawords:
        SET CPU0 CONFIG=STORE_SIZE=4M

<!------------------------------------------------------------------------------------->

<!-- br -->

##### ENABLE_CACHE
\
\
"**`ENABLE_CACHE`**" configures whether the CPU cache is enabled for a CPU with CPU cache, where "`0`" or "`disable`" is disabled and "`1`" or "`enable`" is enabled:

        ENABLE_CACHE=<0 or 1>
        ENABLE_CACHE=<disable or enable>

**Example**

* Configure CPU**`0`** to enable the CPU cache, if installed (*the default setting*):
        SET CPU0 CONFIG=ENABLE_CACHE=1

<!------------------------------------------------------------------------------------->

<!-- br -->

##### SDWAM
\
\
"**`SDWAM`**" configures whether SDW associative memory is enabled, where "`0`" or "`disable`" is disabled and "`1`" or "`enable`" is enabled:

        SDWAM=<0 or 1>
        SDWAM=<disable or enable>

**Example**

* Configure CPU**`0`** to enable SDW associative memory (*the default setting*):
        SET CPU0 CONFIG=SDWAM=1

<!------------------------------------------------------------------------------------->

<!-- br -->

##### PTWAM
\
\
"**`PTWAM`**" configures whether PTW associative memory is enabled, where "`0`" or "`disable`" is disabled and "`1`" or "`enable`" is enabled:

        SDWAM=<0 or 1>
        SDWAM=<enable or disable>

**Example**

* Configure CPU**`0`** to enable PTW associative memory (*the default setting*):
        SET CPU0 CONFIG=PTWAM=1

<!------------------------------------------------------------------------------------->

<!-- br -->

##### DIS_ENABLE
\
\
"**`DIS_ENABLE`**" configures whether DIS (*delay until interrupt serviced*) handling is enabled, where "`0`" or "`disable`" is disabled and "`1`" or "`enable`" is enabled:

        DIS_ENABLE=<0 or 1>
        DIS_ENABLE=<disable or enable>

**Example**

* Configure CPU**`0`** to enable DIS handling (*the default setting*):
        SET CPU0 CONFIG=DIS_ENABLE=1

<!------------------------------------------------------------------------------------->

<!-- br -->

##### STEADY_CLOCK
\
\
"**`STEADY_CLOCK`**" configures whether the *steady CPU clock* should be enabled, where "`0`" or "`disable`" is disabled and "`1`" or "`enable`" is enabled:

        STEADY_CLOCK=<0 or 1>
        STEADY_CLOCK=<disable or enable>

**Example**

* Configure CPU**`0`** with the *steady CPU clock*:
        SET CPU0 CONFIG=STEADY_CLOCK=1

<!------------------------------------------------------------------------------------->

<!-- br -->

##### HALT_ON_UNIMPLEMENTED
\
\
"**`HALT_ON_UNIMPLEMENTED`**" configures whether the simulator should halt when the specified "`CPU`" unit encounters an unimplemented instruction, where "`0`" or "`disable`" is disabled and "`1`" or "`enable`" is enabled.

        HALT_ON_UNIMPLEMENTED=<0 or 1>
        HALT_ON_UNIMPLEMENTED=<disable or enable>

**Example**

* Configure CPU**`0`** to **not** halt when encountering an unimplemented instruction (*the default setting*):
        SET CPU0 CONFIG=HALT_ON_UNIMPLEMENTED=0

<!------------------------------------------------------------------------------------->

<!-- br -->

##### ENABLE_WAM
\
\
"**`ENABLE_WAM`**" configures whether WAM (*word associative memory*) is enabled, where "`0`" or "`disable`" is disabled and "`1`" or "`enable`" is enabled:

        ENABLE_WAM=<0 or 1>
        ENABLE_WAM=<disable or enable>

**Example**

* Configure CPU**`0`** to disable word associative memory (*the default setting*):
        SET CPU0 CONFIG=ENABLE_WAM=0

<!------------------------------------------------------------------------------------->

<!-- br -->

##### REPORT_FAULTS
\
\
"**`REPORT_FAULTS`**" configures whether fault reporting is enabled, where "`0`" or "`disable`" is disabled and "`1`" or "`enable`" is enabled:

        REPORT_FAULTS=<0 or 1>
        REPORT_FAULTS=<disable or enable>

**Example**

* Configure CPU**`0`** to disable fault reporting (*the default setting*):
        SET CPU0 CONFIG=REPORT_FAULTS=0

<!------------------------------------------------------------------------------------->

<!-- br -->

##### TRO_ENABLE
\
\
"**`TRO_ENABLE`**" configures whether timer runout (*TRO*) is enabled, where "`0`" or "`disable`" is disabled and "`1`" or "`enable`" is enabled:

        TRO_ENABLE=<0 or 1>
        TRO_ENABLE=<disable or enable>

**Example**

* Configure CPU**`0`** to enable TRO (*the default setting*):
        SET CPU0 CONFIG=TRO_ENABLE=1

<!------------------------------------------------------------------------------------->

<!-- br -->

##### DRL_FATAL
\
\
"**`DRL_FATAL`**" configures whether a DRL fault is fatal (stopping the simulator), where "`0`" or "`disable`" is disabled and "`1`" or "`enable`" is enabled:

        DRL_FATAL=<0 or 1>
        DRL_FATAL=<disable or enable>

**Example**

* Configure CPU**`0`** to not treat DRL faults as fatal (*the default setting*):
        SET CPU0 CONFIG=DRL_FATAL=0

<!------------------------------------------------------------------------------------->

<!-- br -->

##### USEMAP
\
\
"**`USEMAP`**" configures whether to enable mapping, where "`0`" or "`disable`" is disabled and "`1`" or "`enable`" is enabled:

        USEMAP=<0 or 1>
        USEMAP=<disable or enable>

**Example**

* Configure CPU**`0`** to disable mapping (*the default setting*):
        SET CPU0 CONFIG=USEMAP=0

<!------------------------------------------------------------------------------------->

<!-- br -->

##### ADDRESS
\
\
"**`ADDRESS`**" configures the CPU address to the specified "**`<word>`**":

        ADDRESS=<word>

**Example**

* Configure CPU**`0`** address to "`000000000000`" (*the default setting*):
        SET CPU0 CONFIG=ADDRESS=000000000000


**See also**

Refer to *GB61-01* **Operators Guide, Appendix A** for more details.

<!------------------------------------------------------------------------------------->

<!-- br -->

##### PROM_INSTALLED
\
\
"**`PROM_INSTALLED`**" configures whether a CPU identification PROM is installed, where "`0`" or "`disable`" is *not installed* and "`1`" or "`enable`" is *installed*:

        PROM_INSTALLED=<0 or 1>
        PROM_INSTALLED=<disable or enable>

**Example**

* Configure CPU**`0`** to install a CPU identification PROM (*the default setting for DPS‑8/M processors*):
        SET CPU0 CONFIG=PROM_INSTALLED=1

<!------------------------------------------------------------------------------------->

<!-- br -->

##### HEX_MODE_INSTALLED
\
\
"**`HEX_MODE_INSTALLED`**" configures whether the hexadecimal floating point (**HFP**) CPU option is installed, where "`0`" or "`disable`" is *not installed* and "`1`" or "`enable`" is *installed*:

        HEX_MODE_INSTALLED=<0 or 1>
        HEX_MODE_INSTALLED=<disable or enable>

**Example**

* Configure CPU**`0`** to install the hexadecimal floating point (**HFP**) CPU option:
        SET CPU0 CONFIG=HEX_MODE_INSTALLED=1

<!------------------------------------------------------------------------------------->

<!-- br -->

##### CACHE_INSTALLED
\
\
"**`CACHE_INSTALLED`**" configures whether the CPU cache option is installed, where "`0`" or "`disable`" is *not installed* and "`1`" or "`enable`" is *installed*:

        CACHE_INSTALLED=<0 or 1>
        CACHE_INSTALLED=<disable or enable>

**Example**

* Configure CPU**`0`** to install the CPU cache option (*the default setting*):
        SET CPU0 CONFIG=CACHE_INSTALLED=1

<!------------------------------------------------------------------------------------->

<!-- br -->

##### CLOCK_SLAVE_INSTALLED
\
\
"**`CLOCK_SLAVE_INSTALLED`**" configures whether the CPU clock slave is installed, where "`0`" or "`disable`" is *not installed* and "`1`" or "`enable`" is *installed*:

        CLOCK_SLAVE_INSTALLED=<0 or 1>
        CLOCK_SLAVE_INSTALLED=<disable or enable>

**Example**

* Configure CPU**`0`** to install the CPU clock slave (*the default setting*):
        SET CPU0 CONFIG=CLOCK_SLAVE_INSTALLED=1

<!------------------------------------------------------------------------------------->

<!-- br -->

##### ENABLE_EMCALL
\
\
"**`ENABLE_EMCALL`**" configures whether *emcall* is enabled, where "`0`" or "`disable`" is disabled and "`1`" or "`enable`" is enabled:

        ENABLE_EMCALL=<0 or 1>
        ENABLE_EMCALL=<disable or enable>

**Example**

* Configure CPU**`0`** to disable *emcall*:
        SET CPU0 CONFIG=ENABLE_EMCALL=0

<!------------------------------------------------------------------------------------->

<!-- br -->

##### ISOLTS_MODE
\
\
"**`ISOLTS_MODE`**" configures the CPU in such a way that facilitates running *ISOLTS* diagnostics, where "`0`" or "`disable`" is disabled and "`1`" or "`enable`" is enabled:

        ISOLTS_MODE=<0 or 1>
        ISOLTS_MODE=<disable or enable>

**Example**

* Configure CPU**`0`** for *ISOLTS* testing:
        SET CPU0 CONFIG=ISOLTS_MODE=1

<!------------------------------------------------------------------------------------->

<!-- br -->

##### NODIS
\
\
"**`NODIS`**" configures whether normal CPU initial DIS state is disabled, where "`1`" or "`enable`" enables the disabling of the normal state and "`0`" or "`disable`" disables the disabling the normal state if such disabling was previously enabled:

        NODIS=<0 or 1>
        NODIS=<disable or enable>

**Example**

* Configure CPU**`0`** to disable the normal initial DIS state:
        SET CPU0 CONFIG=NODIS=1

<!------------------------------------------------------------------------------------->

<!-- br -->

##### L68_MODE
\
\
"**`L68_MODE`**" configures the "`CPU`" unit specified to simulate a **Level 68** processor, where "`1`" or "`enable`" configures the CPU as a **Level 68** and "`0`" or "`disable`" configures the CPU as a **DPS‑8/M**.

        L68_MODE=<0 or 1>
        L68_MODE=<disable or enable>

**Examples**

* Configure CPU**`0`** to simulate a **Level 68** processor. (The default is to simulate a **DPS‑8/M**):
        SET CPU0 CONFIG=L68_MODE=1

<!------------------------------------------------------------------------------------->

<!-- br -->

### IOM Configuration

#### NUNITS
"**`NUNITS`**" configures the number of "`IOM`" units.

        NUNITS=<n>

**Example**

* Set the number of "`IOM`" units to "`2`" (*the default number*):
        SET IOM NUNITS=2

<!-- br -->

<!------------------------------------------------------------------------------------->

#### DEBUG (NODEBUG)

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### RESET

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### CONFIG

The following "**`IOM`**" configuration options are associated with a specified "`IOM`" unit (*i.e.* "IOM**`n`**") and are configured using the "`SET`" command (*i.e.* "**`SET IOMn CONFIG`**"):

        SET IOMn CONFIG=<set-option>

* The following "**`<set-option>`**"'s are available, in the form of "**`<option>=<value>`**", for example:
        SET IOMn CONFIG=<option>=<value>

<!-- br -->

<!------------------------------------------------------------------------------------->

##### PORT
\
\
"**`PORT`**" selects the IOM port "**`<p>`**" for which subsequent "`SET IOMn CONFIG=`" configuration commands apply:

        PORT=<p>

**Example**

* Select port "**`0`**" of `IOM`**`0`** for subsequent "`SET IOM0 CONFIG=`" configuration commands:
        SET IOM0 CONFIG=PORT=0

<!-- br -->

<!------------------------------------------------------------------------------------->

##### ADDR
\
\
* TBD (Address of the port)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### INTERLACE
\
\
* TBD (Interlace switch setting)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### ENABLE
\
\
* TBD (Port enabled or disabled)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### INITENABLE
\
\
* TBD (Init enabled or disabled for port)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### HALFSIZE
\
\
* TBD (Halfsize setting)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### STORE_SIZE
\
\
* TBD (Size of memory - one of 32 64 128 256 512 1024 2048 4096 32K 64K 128K 256K 512K 1024K 2048K 4096K 1M 2M 4M, 4M is default.)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### MODEL
\
\
"**`MODEL`**" configures the IOM model, where "**`<model>`**" is **`iom`** or **`imu`**.

        MODEL=<model>

**Example**

* Set the model of **IOM`0`** to "`iom`" (*the default model*):
        SET IOM0 CONFIG=MODEL=iom

<!-- br -->

<!------------------------------------------------------------------------------------->

##### OS
\
\
"**`OS`**" configures the allowed operating system for the `IOM` unit specified, where "**`<os>`**" is **`gcos`**, **`gcosext`**, or **`multics`**.

        OS=<os>

**Example**

* Set the allowed operating system mode for **IOM`0`** to **`multics`** (*the default allowed mode*):
        SET IOM0 CONFIG=MODE=multics

<!-- br -->

<!------------------------------------------------------------------------------------->

##### BOOT
\
\
"**`BOOT`**" configures the device to boot from for the `IOM` unit specified, where "**`<value>`**" is **`disk`** or **`tape`**.

        BOOT=<value>

**Example**

* Set **IOM`0`** to boot from "`tape`" (*the default boot device setting*):
        SET IOM0 CONFIG=BOOT=tape

<!-- br -->

<!------------------------------------------------------------------------------------->

##### IOM_BASE
\
\
"**`IOM_BASE`**" configures the base address for the `IOM` unit specified, where "**`<base_value>`**" is **`Multics`** (`014`), **`Multics1`** (`014`), **`Multics2`** (`020`), **`Multics3`** (`024`), or **`Multics4`** (`030`).

        IOM_BASE=<base_value>

**Example**

* Set the base address of **IOM`0`** to **`Multics`** (`014`) (*the default base address*):
        SET IOM0 CONFIG=IOM_BASE=Multics

<!-- br -->

<!------------------------------------------------------------------------------------->

##### MULTIPLEX_BASE
\
\
"**`MULTIPLEX_BASE`**" configures the multiplex base address for the `IOM` unit specified, where "**`<n>`**" is "`0120`" (for **IOM`0`**) or "`0121`" (for **IOM`1`**).

        MULTIPLEX_BASE=<n>

**Example**

* Set the multiplex base address of **IOM`0`** to `0120` (*the default multiplex base address*):
        SET IOM0 CONFIG=MULTIPLEX_BASE=0120

<!-- br -->

<!------------------------------------------------------------------------------------->

##### TAPECHAN
\
\
"**`TAPECHAN`**" configures the default tape channel for the `IOM` unit specified:

        TAPECHAN=<n>

**Example**

* Set the default tape channel of **IOM`0`** to `012` (*the default tape channel setting*):
        SET IOM0 CONFIG=TAPECHAN=012

<!-- br -->

<!------------------------------------------------------------------------------------->

##### CARDCHAN
\
\
"**`CARDCHAN`**" configures the default card channel for the `IOM` unit specified:

        CARDCHAN=<n>

**Example**

* Set the default card channel of **IOM`0`** to `011` (*the default card channel setting*):
        SET IOM0 CONFIG=CARDCHAN=011

<!-- br -->

<!------------------------------------------------------------------------------------->

##### SCUPORT
\
\
"**`SCUPORT`**" configures which port on the SCU the `IOM` unit specified will be connected to.

        SCUPORT=<n>

**Example**

* Set the SCU port of **IOM`0`** to `0` (*the default port*):
        SET IOM0 CONFIG=SCUPORT=0

<!------------------------------------------------------------------------------------->

<!-- br -->

### SCU Configuration

#### DEBUG (NODEBUG)

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS
"**`NUNITS`**" configures the number of "`SCU`" units.

        NUNITS=<n>

**Example**

* Set the number of `SCU` units to "`4`" (*the default number*):
        SET SCU NUNITS=6

<!-- br -->

<!------------------------------------------------------------------------------------->

#### RESET

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### CONFIG

The following "**`SCU`**" configuration options are associated with a specified "`SCU`" unit (*i.e.* "SCU**`n`**") and are configured using the "`SET`" command (*i.e.* "**`SET SCUn CONFIG`**"):

        SET SCUn CONFIG=<set-option>

* The following "**`<set-option>`**"'s are available, in the form of "**`<option>=<value>`**", for example:
        SET SCUn CONFIG=<option>=<value>

<!-- br -->

<!------------------------------------------------------------------------------------->

##### MODE
\
\
* TBD (0 or 1)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### MASKA
\
\
* TBD (7 for SCU0 by default, and "off" for other SCUs.)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### MASKB
\
\
* TBD ("off" for all SCUs by default)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### PORT0
\
\
* TBD (0 or 1)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### PORT1
\
\
* TBD (0 or 1)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### PORT2
\
\
* TBD (0 or 1)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### PORT3
\
\
* TBD (0 or 1)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### PORT4
\
\
* TBD (0 or 1)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### PORT5
\
\
* TBD (0 or 1)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### PORT6
\
\
* TBD (0 or 1)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### PORT7
\
\
* TBD (0 or 1)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### LWRSTORESIZE
\
\
* TBD (32 64 128 256 512 1024 2048 4096 32K 64K 128K 256K 512K 1024K 2048K 4096K 1M 2M 4M, 4M is default)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### CYCLIC
\
\
* TBD (0040 by default)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### NEA
\
\
* TBD (0200 by default)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### ONL
\
\
* TBD (014 by default)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### INT
\
\
* TBD (0 or 1)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### LW
\
\
* TBD (0 or 1)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### ELAPSED_DAYS
\
\
* TBD (0 or 1)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### STEADY_CLOCK
\
\
* TBD (0 or 1, disable or enable)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### BULLET_TIME
\
\
* TBD (0 or 1, disable or enable)

<!-- br -->

<!------------------------------------------------------------------------------------->

##### CLOCK_DELTA
\
\
"**`CLOCK_DELTA`**" modifies the SCU clock to run a user-specified number of seconds ahead (or behind) the host clock, where "`0`", the default, is no modification.

        CLOCK_DELTA=<-2147483648 to 2147483647>

* Setting "`CLOCK_DELTA`" to a negative value is useful to enable booting older Multics releases (*i.e.* MR12.3 and MR12.5) that are *not* Y2K-compliant without modification.

* Setting "`CLOCK_DELTA`" to a positive value is useful for Multics development or working with disks that were used "in the future".

* The *GNU* "`date`" tool (with the help of the Unix shell) can be used to calculate an appropriate value.  The following example calculates a value representing a modification of *5 years*:
  ```dps8
  printf '%s\n' "$(( $(date --date='5 years' +%s) - $(date +%s) ))"
  ```
  * macOS and BSD users usually have this tool available as "`gdate`" (part of the *GNU coreutils* package, which may require separate installation).

* It is the responsibility of the user to set a reasonable "`CLOCK_DELTA`" value.
  * Multics may not handle times in the year 2043 and beyond.

  * Undefined behavior may result if the modified time is beyond *January 19 2038 03:14:07 UTC* on a host system that measures time as seconds elapsed since *January 1 1970 00:00:00 UTC* (*the Unix epoch*) and stores the result in a signed 32-bit integer (*i.e.* Unix systems using 32-bit `time_t`).

* It is not expected that the "`CLOCK_DELTA`" value will require modification by most users under normal operating circumstances.

**Examples**

* Configure the SCU**`0`** clock to run *5 years* ahead of the host clock:
        SET SCU0 CONFIG=CLOCK_DELTA=157766400

* Configure the SCU**`0`** clock to run *33 years, 6 months* behind of the host clock:
        SET SCU0 CONFIG=CLOCK_DELTA=-1057017600

<!-- br -->

<!------------------------------------------------------------------------------------->

<!-- br -->

### TAPE Configuration

#### DEBUG (NODEBUG)

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### DEFAULT_PATH

"`DEFAULT_PATH`" configures the default path to use when searching for tape files.

        DEFAULT_PATH=<path>

* Setting this option will clear any previous "`ADD_PATH`" parameters.
* If "`DEFAULT_PATH`" is *not* set, the simulator will use the current working directory of the host operating system.
* When using relative paths (beginning with "`.`" or "`..`") the starting point is the current working directory of the host operating system.

**Example**

* Set the default path to search for tape files to "`./tapes`":
        SET TAPE DEFAULT_PATH=./tapes

<!-- br -->

<!------------------------------------------------------------------------------------->

#### ADD_PATH

"`ADD_PATH`" adds a new tape search path at the end of the current search path list.

        ADD_PATH=<prefix=directory>

* When using relative paths (beginning with "`.`" or "`..`"), the starting point is the current working directory of the host operating system.
* If "`SET DEFAULT_PATH`" is used, all previous "`ADD_PATH`" entries will be removed.
* New tape search paths cannot be added unless a "`DEFAULT_PATH`" has been explicitly set.

**Example**

* Look for tapes with a volume name starting with "`BK`" in the directory "`./tapes/backups`":
        SET TAPE ADD_PATH=BK=./tapes/backups

**Notes**

* When the tape search path table is evaluated during a tape file lookup, it is processed in order, with the first matching entry being accepted.
* For example, if the following commands are given, a tape with a volume name of "`BK001`" will be placed in the "`./tapes/billing`" directory and not the "`./tapes/backups`" directory, since it begins with the substring "`B`", matching the first entry in the search table:
        SET TAPE DEFAULT_PATH=./tapes/general
        SET TAPE ADD_PATH=B=./tapes/billing
        SET TAPE ADD_PATH=BK=./tapes/backups

* To get the intended effect, the commands may be reordered to ensure names starting with "`BK`" are checked before names starting with "`B`":
        SET TAPE DEFAULT_PATH=./tapes/general
        SET TAPE ADD_PATH=BK=./tapes/backups
        SET TAPE ADD_PATH=B=./tapes/billing

* The simulator "`ATTACH`" command used in script files (usually for mounting the boot tape) is *not* aware of the alternative tape search paths.  If a directory is needed for an "`ATTACH`" command, the full path should be specified, for example:

        ATTACH -r tape0 ./tapes/12.8/12.8MULTICS.tap

<!-- br -->

<!------------------------------------------------------------------------------------->

#### CAPACITY_ALL

"`CAPACITY_ALL`" configures the default maximum size (in megabytes) that can be written to a tape file for all tape devices.

        CAPACITY_ALL=<n>

* If "`CAPACITY_ALL`" is *not* set, the simulator will default to using a maximum size of **`88`MB**.

**Example**

* Set the default maximum size of tape files for all tape devices to **`40`MB**:
        SET TAPE CAPACITY_ALL=40

<!-- br -->

<!------------------------------------------------------------------------------------->

#### CAPACITY

"`CAPACITY`" configures the maximum size (in megabytes) that can be written to a tape file for a specific tape devices.

        CAPACITY=<n>

* If "`CAPACITY`" is *not* set, the simulator will default to using a maximum size of **`88`MB**.

**Example**

* Set the maximum size of tape files for the "`TAPE`**`1`**" device to **`40`MB**:
        SET TAPE1 CAPACITY_ALL=40

<!-- br -->

<!------------------------------------------------------------------------------------->

#### REWIND

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### READY

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NAME

"**`NAME`**" configures the device name of the specified "`TAPE`" unit.

        NAME=<name>

**Example**

* Set the device name of "`TAPE`**`1`**" to "`tapa_01`" (*the default device name*).
        SET TAPE1 NAME=tapa_01

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS

"**`NUNITS`**" configures the number of "`TAPE`" units.

        NUNITS=<n>

**Example**

* Set the number of "`TAPE`" units to "`16`" (*the default number*):
        SET TAPE NUNITS=16

<!------------------------------------------------------------------------------------->

<!-- br -->

### MTP Configuration

#### DEBUG (NODEBUG)

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NAME
"**`NAME`**" configures the device name of the specified "`MTP`" unit.

        NAME=<name>

**Example**

* Set the device name of **MTP`0`** to "`MTP0`" (*the default name*).
        SET MTP0 NAME=MTP0

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS
"**`NUNITS`**" configures the number of "`MTP`" units.

        NUNITS=<n>

**Example**

* Set the number of "`MTP`" units to "`1`" (*the default number*):
        SET MTP NUNITS=6

<!-- br -->

<!------------------------------------------------------------------------------------->

#### BOOT_DRIVE

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### CONFIG

The following "**`MTP`**" configuration options are associated with a specified "`MTP`" unit (*i.e.* "MTP**`n`**") and are configured using the "`SET`" command (*i.e.* "**`SET MTPn CONFIG`**"):

        SET MTPn CONFIG=<set-option>

* The following "**`<set-option>`**"'s are available, in the form of "**`<option>=<value>`**", for example:
        SET MTPn CONFIG=<option>=<value>

<!------------------------------------------------------------------------------------->

<!-- br -->

### IPC Configuration

#### NAME
"**`NAME`**" configures the device name of the specified "`IPC`" unit.

        NAME=<name>

**Example**

* Set the device name of **`IPC0`** to "`IPC0`" (*the default name*).
        SET IPC0 NAME=IPC0

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS
"**`NUNITS`**" configures the number of "`IPC`" units.

        NUNITS=<n>

**Example**

* Set the number of "`IPC`" units to "`2`" (*the default number*):
        SET IPC NUNITS=6

<!------------------------------------------------------------------------------------->

<!-- br -->

### MSP Configuration

#### NAME
"**`NAME`**" configures the device name of the specified "`MSP`" unit.

        NAME=<name>

**Example**

* Set the device name of **MSP`0`** to "`MSP0`" (*the default name*).
        SET MSP0 NAME=MSP0

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS
"**`NUNITS`**" configures the number of "`MSP`" units.

        NUNITS=<n>

**Example**

* Set the number of "`MSP`" units to "`2`" (*the default number*):
        SET MSP NUNITS=2

<!------------------------------------------------------------------------------------->

<!-- br -->

### Disk Configuration

#### DEBUG (NODEBUG)

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### TYPE

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### READY

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NAME
"**`NAME`**" configures the device name of the specified "`DISK`" unit.

        NAME=<name>

**Example**

* Set the device name of **DISK`0`** to "`disk_00`" (*the default name*).
        SET DISK0 NAME=disk_00

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS
"**`NUNITS`**" configures the number of "`DISK`" units.

        NUNITS=<n>

**Example**

* Set the number of "`DISK`" units to "`26`" (*the default number*):
        SET DISK NUNITS=26

<!------------------------------------------------------------------------------------->

<!-- br -->

### RDR Configuration

#### DEBUG (NODEBUG)

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### PATH

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NAME
"**`NAME`**" configures the device name of the specified "`RDR`" unit.

        NAME=<name>

**Example**

* Set the device name of **RDR`0`** to "`rdra`" (*the default name*).
        SET RDR0 NAME=rdra

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS
"**`NUNITS`**" configures the number of "`RDR`" units.

        NUNITS=<n>

**Example**

* Set the number of "`RDR`" units to "`3`" (*the default number*):
        SET RDR NUNITS=3

<!------------------------------------------------------------------------------------->

<!-- br -->

<!-- pagebreak -->

### PUN Configuration

#### Overview

The following commands configure simulated card punch devices.

The simulated card punch devices operate by writing simulated punched card decks to *spool files*.  The location of the spool files on the host system is controlled with the "`SET PUN PATH`" command.

The *spool file* will contain all ***banner***, ***lace***, and ***trailer*** cards that Multics punches.  The ***lace*** cards are parsed to extract job information that is used to name the spool files on the host system.

* The simulator includes a utility, **`punutil`**, that can be used to extract the various cards from the deck in easily usable formats (such as ASCII text).

#### Card format

Each column in a punched card has **twelve** rows, designated from top to bottom:

|       |       |       |       |       |       |       |       |       |       |       |       |
| ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| '`&`' | '`-`' | '`0`' | '`1`' | '`2`' | '`3`' | '`4`' | '`5`' | '`6`' | '`7`' | '`8`' | '`9`' |

Given the above, an **`80`-column** punched card would then look like:

               ┌───┬───┬───┬───┬───┬───────────────────┬───┬───┬───┬───┬───┐
               │ & │ & │ & │ & │ & │                   │ & │ & │ & │ & │ & │
               │ - │ - │ - │ - │ - │                   │ - │ - │ - │ - │ - │
               │ 0 │ 0 │ 0 │ 0 │ 0 │                   │ 0 │ 0 │ 0 │ 0 │ 0 │
               │ 1 │ 1 │ 1 │ 1 │ 1 │                   │ 1 │ 1 │ 1 │ 1 │ 1 │
               │ 2 │ 2 │ 2 │ 2 │ 2 │                   │ 2 │ 2 │ 2 │ 2 │ 2 │
               │ 3 │ 3 │ 3 │ 3 │ 3 │ [… +70 columns …] │ 3 │ 3 │ 3 │ 3 │ 3 │
               │ 4 │ 4 │ 4 │ 4 │ 4 │                   │ 4 │ 4 │ 4 │ 4 │ 4 │
               │ 5 │ 5 │ 5 │ 5 │ 5 │                   │ 5 │ 5 │ 5 │ 5 │ 5 │
               │ 6 │ 6 │ 6 │ 6 │ 6 │                   │ 6 │ 6 │ 6 │ 6 │ 6 │
               │ 7 │ 7 │ 7 │ 7 │ 7 │                   │ 7 │ 7 │ 7 │ 7 │ 7 │
               │ 8 │ 8 │ 8 │ 8 │ 8 │                   │ 8 │ 8 │ 8 │ 8 │ 8 │
               │ 9 │ 9 │ 9 │ 9 │ 9 │                   │ 9 │ 9 │ 9 │ 9 │ 9 │
               └───┴───┴───┴───┴───┴───────────────────┴───┴───┴───┴───┴───┘

The most obvious way to represent a punched card column in a *spool file* is to assign a single bit per punch.  This means that there are **`12` bits** for each column.  When looking at a hexadecimal dump of a *spool file*, each column occupies **three** **`4`-bit** nibbles. The following representation is *big-endian*:

          ┌───────────────────────────────────┬────────────────────────────────────┐
          │              Column 1             │             Column 2               │
          ├──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬───┤
          │ &  -  0  1  2  3  4  5  6  7  8  9  &  -  0  1  2  3  4  5  6  7  8  9 │
          ├──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼───┤
          │  Nibble 1 │ Nibble 2  │  Nibble 3 │  Nibble 1 │  Nibble 2 │  Nibble 3  │
          ├──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼───┤
          │         Byte 1        │         Byte 2        │         Byte 3         │
          ├──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼───┤
          │ 7  6  5  4  3  2  1  0  7  6  5  4  3  2  1  0  7  6  5  4  3  2  1  0 │
          └──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴───┘

#### DEBUG (NODEBUG)

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### PATH

"`PATH`" configures the path that will be used as the location to write punch *spool files*.

        PATH=<path>

**Example**

* Set the punch spool file path to "`/home/tom/punches`":
        SET PUN PATH=/home/tom/punches

[]()
[]()

[]()
[]()

**Notes**

* When unset, all punch spool files will be written to the current working directory of the host operating system.
* When set, the simulator expects a subdirectory to exist for each named punch device - the simulator will **not** automatically create these subdirectories.  For example, if "`PATH`" is set to "`/home/tom/punches`" and three punch devices are configured (`puna`, `punb`, `punc`), then the following directory structure must exist for punch jobs to be output properly:
         /home
         └── tom
             └── punches
                 ├── puna
                 ├── punb
                 └── punc

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NAME
"**`NAME`**" configures the device name of the specified "`PUN`" unit.

        NAME=<name>

**Example**

* Set the device name of **PUN`0`** to "`puna`" (*the default name*).
        SET PUN0 NAME=puna

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS
"**`NUNITS`**" configures the number of "`PUN`" units.

        NUNITS=<n>

**Example**

* Set the number of "`PUN`" units to "`3`" (*the default number*):
        SET PUN NUNITS=2

<!-- br -->

<!------------------------------------------------------------------------------------->

#### CONFIG

The following "**`PUN`**" configuration options are associated with a specified "`PUN`" unit (*i.e.* "PUN**`n`**") and are configured using the "`SET`" command (*i.e.* "**`SET PUNn CONFIG`**"):

        SET PUNn CONFIG=<set-option>

* The following "**`<set-option>`**"'s are available, in the form of "**`<option>=<value>`**", for example:
        SET PUNn CONFIG=<option>=<value>

<!-- br -->

<!------------------------------------------------------------------------------------->

##### LOGCARDS
\
\
"`LOGCARDS`" configures the simulator to output a significant amount of diagnostic details regarding a punch device to the simulator console.  This includes an "ASCII art" representation of the punched cards, where asterisks (`*`) are used to represent the punched holes.  It is recommended to enable this option only if you are attempting to diagnose a card punch issue.

        SET PUNn CONFIG=LOGCARDS=<0 or 1>
        SET PUNn CONFIG=LOGCARDS=<disable or enable>

**Example**

* Turn on logging of card punch diagnostic data for punch device "PUN**`0`**":
        SET PUN0 CONFIG=LOGCARDS=1

[]()
[]()

[]()
[]()

* Turn off logging of card punch diagnostic data for punch device "PUN**`1`**":
        SET PUN1 CONFIG=LOGCARDS=disable

<!------------------------------------------------------------------------------------->

<!-- br -->

### PRT Configuration

#### DEBUG (NODEBUG)

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### PATH

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### MODEL

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### READY

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NAME
"**`NAME`**" configures the device name of the specified "`PRT`" unit.

        NAME=<name>

**Example**

* Set the device name of **PRT`0`** to "`prta`" (*the default name*).
        SET PRT0 NAME=prta

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS
"**`NUNITS`**" configures the number of "`PRT`" units.

        NUNITS=<n>

**Example**

* Set the number of "`PRT`" units to "`4`" (*the default number*):
        SET PRT NUNITS=4

<!-- br -->

<!------------------------------------------------------------------------------------->

#### CONFIG

The following "**`PRT`**" configuration options are associated with a specified "`PRT`" unit (*i.e.* "PRT**`n`**") and are configured using the "`SET`" command (*i.e.* "**`SET PRTn CONFIG`**"):

        SET PRTn CONFIG=<set-option>

* The following "**`<set-option>`**"'s are available, in the form of "**`<option>=<value>`**", for example:
        SET PRTn CONFIG=<option>=<value>

<!-- br -->

<!------------------------------------------------------------------------------------->

##### SPLIT
\
\
* TBD

<!------------------------------------------------------------------------------------->

<!-- br -->

### FNP Configuration

#### DEBUG (NODEBUG)

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS
"**`NUNITS`**" configures the number of "`FNP`" units.

        NUNITS=<n>

**Example**

* Set the number of "`FNP`" units to "`8`" (*the default number*):
        SET FNP NUNITS=8

<!-- br -->

<!------------------------------------------------------------------------------------->

#### CONFIG

The following "**`FNP`**" configuration options are associated with a specified "`FNP`" unit (*i.e.* "FNP**`n`**") and are configured using the "`SET`" command (*i.e.* "**`SET FNPn CONFIG`**"):

        SET FNPn CONFIG=<set-option>

* The following "**`<set-option>`**"'s are available, in the form of "**`<option>=<value>`**", for example:
        SET FNPn CONFIG=<option>=<value>

<!-- br -->

<!------------------------------------------------------------------------------------->

##### MAILBOX
\
\
* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

##### IPC_NAME
\
\
* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

##### SERVICE
\
\
* TBD

<!------------------------------------------------------------------------------------->

<!-- br -->

### OPC Configuration

#### DEBUG (NODEBUG)

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### AUTOINPUT

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NAME
"**`NAME`**" configures the device name of the specified "`OPC`" unit.

        NAME=<name>

**Example**

* Set the device name of **OPC`0`** to "`OPC0`" (*the default name*).
        SET OPC0 NAME=OPC0

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS
"**`NUNITS`**" configures the number of "`OPC`" units.

        NUNITS=<n>

**Example**

* Set the number of "`OPC`" units to "`2`" (*the default number*):
        SET OPC NUNITS=2

<!-- br -->

<!------------------------------------------------------------------------------------->

#### CONFIG

The following "**`OPC`**" configuration options are associated with a specified "`OPC`" unit (*i.e.* "OPC**`n`**") and are configured using the "`SET`" command (*i.e.* "**`SET OPCn CONFIG`**"):

        SET OPCn CONFIG=<set-option>

* The following "**`<set-option>`**"'s are available, in the form of "**`<option>=<value>`**", for example:
        SET OPCn CONFIG=<option>=<value>

<!-- br -->

<!------------------------------------------------------------------------------------->

##### AUTOACCEPT
\
\
* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

##### NOEMPTY
\
\
* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

##### ATTN_FLUSH
\
\
* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

##### PORT
\
\
* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

##### ADDRESS
\
\
* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

##### PW
\
\
* TBD

<!------------------------------------------------------------------------------------->

<!-- br -->

### URP Configuration

#### DEBUG (NODEBUG)

* TBD

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NAME
"**`NAME`**" configures the device name of the specified "`URP`" unit.

        NAME=<name>

**Example**

* Set the device name of **URP`0`** to "`urpa`" (*the default name*).
        SET URP0 NAME=urpa

<!-- br -->

<!------------------------------------------------------------------------------------->

#### NUNITS
"**`NUNITS`**" configures the number of "`URP`" units.

        NUNITS=<n>

**Example**

* Set the number of "`URP`" units to "`10`" (*the default number*):
        SET URP NUNITS=10

