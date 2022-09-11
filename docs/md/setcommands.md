<!-- SPDX-License-Identifier: LicenseRef-DPS8M-Doc OR LicenseRef-CF-GAL -->
<!-- SPDX-FileCopyrightText: 2022 The DPS8M Development Team -->
<!-- scspell-id: 65252877-3180-11ed-bcd8-80ee73e9b8e7 -->

### CPU Configuration

CPU configuration options are associated with a specified CPU unit (*i.e.* CPU**`n`**) and are configured using the `SET` command (*i.e.* **`SET CPUn CONFIG`**):

        SET CPUn CONFIG=<set-option>

The following **`<set-option>`**'s are available, in the form of **`<option>=<value>`**, for example:

        SET CPUn CONFIG=<option>=<value>

<!------------------------------------------------------------------------------------->

#### FAULTBASE
**`FAULTBASE`** configures the fault base of the specified CPU unit:

        FAULTBASE=<value>

##### Example
For Multics operation, configure the **`<value>`** to **`2`** (for CPU**`0`**):
        SET CPU0 CONFIG=FAULTBASE=2

<!------------------------------------------------------------------------------------->

#### NUM
**`NUM`** configures the CPU number of the specified CPU unit:

        NUM=<n>

##### Example
Set CPU number (**`<n>`**) of CPU**`0`** to **`1`**.
        SET CPU0 CONFIG=NUM=1

<!------------------------------------------------------------------------------------->

#### DATA
**`DATA`** configures the CPU switches of the specified CPU unit:

        DATA=<word>

##### Example
Set CPU**`0`** switches to **`024000717200`** (*the default value*).
        SET CPU0 CONFIG=DATA=024000717200

<!------------------------------------------------------------------------------------->

#### STOPNUM
**`STOPNUM`** configures the CPU switches of the specified CPU unit so that Multics will stop during boot at the check stop specified by **`n`**.

        STOPNUM=<n>

##### Example
Set CPU**`0`** switches so Multics will stop during boot at check stop `#`**`2020`**.
        SET CPU0 CONFIG=STOPNUM=2030


<!------------------------------------------------------------------------------------->

#### MODE
**`MODE`** configures the operating mode of specified CPU unit as indicated by **`<value>`**:

        MODE=<value>

* The supported **`<value>`**'s are:

| **`<value>`** | **Mode**         |
| ------------- | ---------------- |
| **`0`**       | **GCOS** mode    |
| **`1`**       | **Multics** mode |

##### Example
Set the operating mode of CPU**`0`** to **Multics mode**:
        SET CPU0 CONFIG=MODE=1

<!------------------------------------------------------------------------------------->

#### SPEED
**`SPEED`** configures the CPU speed setting of the specified CPU unit:

        SPEED=<num>

<!------------------------------------------------------------------------------------->

#### PORT
**`PORT`** selects the CPU port for which subsequent "`SET CPUn CONFIG=`" configuration commands apply:

        PORT=A

##### Example
Configure `ASSIGNMENT`, `INTERLACE`, `ENABLE`, `INIT_ENABLE`, and `STORE_SIZE` for port **A** on CPU**`0`**:
        SET CPU0 CONFIG=PORT=A
        SET CPU0 CONFIG=ASSIGNMENT=0
        SET CPU0 CONFIG=INTERLACE=0
        SET CPU0 CONFIG=ENABLE=1
        SET CPU0 CONFIG=INIT_ENABLE=1
        SET CPU0 CONFIG=STORE_SIZE=4M

<!------------------------------------------------------------------------------------->

#### ASSIGNMENT
**`ASSIGNMENT`** configures the CPU port assignment of CPU unit **`n`**'s CPU port **`x`**, as specified by a previous **`SET CPUn CONFIG=PORT=x`** command:

        ASSIGNMENT=<n>

##### Example
Configure the CPU port assignment of CPU unit **`0`**'s currently selected CPU port to **`0`**:

        SET CPU0 CONFIG=ASSIGNMENT=0

<!------------------------------------------------------------------------------------->

#### INTERLACE
**`INTERLACE`** configures whether interlacing is enabled for the currently selected CPU port:

        INTERLACE=<0, 1, 2>

<!------------------------------------------------------------------------------------->

#### ENABLE
**`ENABLE`** configures whether the currently selected CPU port is enabled:

        ENABLE=<0 or 1>

<!------------------------------------------------------------------------------------->

#### INIT_ENABLE
**`INIT_ENABLE`** configures whether *init* is enabled for the currently selected CPU port, where `0` is disabled and `1` is enabled:

        INIT_ENABLE=<0 or 1>

<!------------------------------------------------------------------------------------->

#### STORE_SIZE
**`STORE_SIZE`** configures the size of memory (in *megawords*) for the currently selected CPU port:

        STORE_SIZE=<num>M

##### Example
Configure the memory size of the currently selected CPU port on CPU**`0`** to **`4`** megawords:

        SET CPU0 CONFIG=STORE_SIZE=4M

<!------------------------------------------------------------------------------------->

#### ENABLE_CACHE
**`ENABLE_CACHE`** configures whether the CPU cache is enabled for a CPU with CPU cache, where `0` is disabled and `1` is enabled:

        ENABLE_CACHE=<0 or 1>

##### Example
Configure CPU**`0`** to enable the CPU cache, if CPU cache is installed (*the default setting*):
        SET CPU0 CONFIG=ENABLE_CACHE=1

<!------------------------------------------------------------------------------------->

#### SDWAM
**`SDWAM`** configures whether SDW associative memory is enabled, where `0` is disabled and `1` is enabled:

        SDWAM=<0 or 1>

##### Examples
Configure CPU**`0`** to enable SDW associative memory (*the default setting*):
        SET CPU0 CONFIG=SDWAM=1

<!------------------------------------------------------------------------------------->

#### PTWAM
**`PTWAM`** configures whether PTW associative memory is enabled, where `0` is disabled and `1` is enabled:

        SDWAM=<0 or 1>

##### Examples
Configure CPU**`0`** to enable PTW associative memory (*the default setting*):
        SET CPU0 CONFIG=PTWAM=1

<!------------------------------------------------------------------------------------->

#### DIS_ENABLE
**`DIS_ENABLE`** configures whether DIS handling is enabled, where `0` is disabled and `1` is enabled:

        DIS_ENABLE=<0 or 1>

##### Example
Configure CPU**`0`** to enable DIS handling (*the default setting*):
        SET CPU0 CONFIG=DIS_ENABLE=1

<!------------------------------------------------------------------------------------->

#### STEADY_CLOCK
**`STEADY_CLOCK`** configures whether a *steady CPU clock* should be enabled, where `0` is disabled and `1` is enabled:

        STEADY_CLOCK=<0 or 1>

##### Example
Configure CPU**`0`** with a *steady CPU clock*:
        SET CPU0 CONFIG=STEADY_CLOCK=1

<!------------------------------------------------------------------------------------->

#### HALT_ON_UNIMPLEMENTED
**`HALT_ON_UNIMPLEMENTED`** configures whether the simulator should halt when the specified CPU unit encounters an unimplemented instruction, where `0` is disabled and `1` is enabled.

        HALT_ON_UNIMPLEMENTD=<0 or 1>

##### Example
Configure CPU**`0`** to **not** halt when encountering an unimplemented instruction (*the default setting*):
        SET CPU0 CONFIG=HALT_ON_UNIMPLEMENTED=0

<!------------------------------------------------------------------------------------->

#### ENABLE_WAM
**`ENABLE_WAM`** configures whether WAM (*word associative memory*) is enabled, where `0` is disabled and `1` is enabled:

        ENABLE_WAM=<0 or 1>

##### Example
Configure CPU**`0`** to disable word associative memory (*the default setting*):
        SET CPU0 CONFIG=ENABLE_WAM=0


<!------------------------------------------------------------------------------------->

#### REPORT_FAULTS
**`REPORT_FAULTS`** configures whether fault reporting is enabled, where `0` is disabled and `1` is enabled:

        REPORT_FAULTS=<0 or 1>

##### Example
Configure CPU**`0`** to disable fault reporting (*the default setting*):
        SET CPU0 CONFIG=REPORT_FAULTS=0

<!------------------------------------------------------------------------------------->

#### TRO_ENABLE
**`TRO_ENABLE`** configures whether timer run-off (TRO) is enabled, where `0` is disabled and `1` is enabled:

        TRO_ENABLE=<0 or 1>

##### Example
Configure CPU**`0`** to enable TRO (*the default setting*):

        SET CPU0 CONFIG=TRO_ENABLE=1

<!------------------------------------------------------------------------------------->

#### Y2K
**`Y2K`** configures whether the ***Year 2000 compatibility workaround*** is enabled, where `0` is disabled and `1` is enabled.

* The ***Year 2000 compatibility workaround*** modifies the system clock to enable older Multics releases (*i.e.* MR12.3 and MR12.5) that are not **Y2K**-*compliant* to be booted and used without modification.

        Y2K=<0 or 1>

##### Example
Configure CPU**`0`** to enable the ***Year 2000 compatibility workaround***:
        SET CPU0 CONFIG=Y2K=1

<!------------------------------------------------------------------------------------->

#### DRL_FATAL
**`DRL_FATAL`** configures whether a *DRL* fault is fatal (stopping the simulator), where `0` is disabled and `1` is enabled:

        DRL_FATAL=<0 or 1>

##### Example
Configure CPU**`0`** to not treat *DRL* faults as fatal (*the default setting*):
        SET CPU0 CONFIG=DRL_FATAL=0

<!------------------------------------------------------------------------------------->

#### USEMAP
**`USE_MAP`** configures whether to enable mapping, where `0` is disabled and `1` is enabled:

        USEMAP=<0 or 1>

##### Example
Configure CPU**`0`** to disable mapping (*the default setting*):
        SET CPU0 CONFIG=USEMAP=0

<!------------------------------------------------------------------------------------->

#### ADDRESS
**`ADDRESS`** configures the CPU address to the specified **`<word>`**:

        ADDRESS=<word>

##### Example
Configure CPU**`0`** address to `000000000000` (*the default setting*):
        SET CPU0 CONFIG=ADDRESS=000000000000

<!------------------------------------------------------------------------------------->

#### PROM_INSTALLED
**`PROM_INSTALLED`** configures whether a CPU identification PROM is installed, where `0` is *not installed* and `1` is *installed*:

        PROM_INSTALLED=<0 or 1>

##### Example
Configure CPU**`0`** to install a CPU identification PROM (*the default setting for DPS-8/M processors*):
        SET CPU0 CONFIG=PROM_INSTALLED=1

<!------------------------------------------------------------------------------------->

#### HEX_MODE_INSTALLED
**`HEX_MODE_INSTALLED`** configures whether the hexadecimal floating point (HFP) CPU option is installed, where `0` is *not installed* and `1` is *installed*:

        HEX_MODE_INSTALLED=<0 or 1>

##### Example
Configure CPU**`0`** to install the hexadecimal floating point (HFP) CPU option (*the default setting for DPS-8/M processors*):
        SET CPU0 CONFIG=HEX_MODE_INSTALLED=1

<!------------------------------------------------------------------------------------->

#### CACHE_INSTALLED
**`CACHE_INSTALLED`** configures whether the CPU cache option is installed, where `0` is *not installed* and `1` is *installed*:

        CACHE_INSTALLED=<0 or 1>

##### Examples
Configure CPU**`0`** to install the CPU cache option (*the default setting*):
        SET CPU0 CONFIG=CACHE_INSTALLED=1

<!------------------------------------------------------------------------------------->

#### CLOCK_SLAVE_INSTALLED
**`CLOCK_SLAVE_INSTALLED`** configures whether the CPU clock slave is installed, where `0` is *not installed* and `1` is *installed*:

        CLOCK_SLAVE_INSTALLED=<0 or 1>

##### Example
Configure CPU**`0`** to install the CPU clock slave (*the default setting*):
        SET CPU0 CONFIG=CLOCK_SLAVE_INSTALLED=1

<!------------------------------------------------------------------------------------->

#### ENABLE_EMCALL
**`ENABLE_EMCALL`** configures whether *emcall* is enabled, where `0` is disabled and `1` is enabled:

        ENABLE_EMCALL=<0 or 1>

##### Example
Configure CPU**`0`** to disable *emcall*:
        SET CPU0 CONFIG=ENABLE_EMCALL=0

<!------------------------------------------------------------------------------------->

#### ISOLTS_MODE
**`ISOLTS_MODE`** configures the CPU in such a way to facilitate running *ISOLTS* diagnostics, where `0` is disabled and `1` is enabled:

        ISOLTS_MODE=<0 or 1>

##### Example
Configure CPU**`0`** for *ISOLTS* testing:
        SET CPU0 CONFIG=ISOLTS_MODE=1


<!------------------------------------------------------------------------------------->

#### NODIS
**`NODIS`** configures whether normal CPU initial DIS state is disabled, where `1` enables the disabling of the normal state and `0` disables the disabling the normal state if such disabling was previously enabled:

        NODIS=<0 or 1>

##### Example
Configure CPU**`0`** to disable the normal initial DIS state:
        SET CPU0 CONFIG=NODIS=1

<!------------------------------------------------------------------------------------->

#### L68_MODE (L68, DPS8M)
**`L68_MODE`** configures the CPU unit specified to simulate a **Level 68** processor, where `1` configures the CPU as a **Level 68** and `0` configures the CPU as a **DPS-8/M**.

**`DPS8M`** configures the CPU unit specified to simulate a **DPS-8/M** processor, where `1` configures the CPU as a **DPS-8/M** and `0` configures the CPU as a **Level 68**:

        L68_MODE=<0 or 1>
        L68=<0 or 1>
        DPS8M=<0 or 1>

##### Examples
Configure CPU**`0`** to simulate a **Level 68** processor. (The default is to simulate a **DPS-8/M**):
        SET CPU0 CONFIG=L68_MODE=1
        SET CPU0 CONFIG=L68=1
        SET CPU0 CONFIG=DPS8M=0

Configure **all** CPU units to simulate the **Level 68** processor:
       SET CPU CONFIG=L68=1

Configure **all** CPU units to simulate the **DPS-8/M** processor (*the default setting*):
       SET CPU CONFIG=DPS8M=1

<!------------------------------------------------------------------------------------->

#### RESET
**`RESET`** will reset all CPUs when passed an argument of `1`.

        RESET=<1>

##### Example
Reset CPUs:
        SET CPU RESET=1

<!------------------------------------------------------------------------------------->

#### INITIALIZE
**`INITIALIZE`** will initialize all CPUs when passed an argument of `1`.

        INITIALIZE=<1>

##### Example
Initialize CPUs:
        SET CPU INITIALIZE=1

<!------------------------------------------------------------------------------------->

#### INITIALIZEANDCLEAR (IAC)
**`INITIALIZEANDCLEAR`** (*abbreviated* `IAC`) will initialize all CPUs and clear their state when passed an argument of `1`.

        INITIALIZEANDCLEAR=<1>
        IAC=<1>

##### Example
Initialize and clear CPUs:
        SET CPU INITIALIZEANDCLEAR=1
        SET CPU IAC=1

<!------------------------------------------------------------------------------------->

#### NUNITS
**`NUNITS`** configures the number of CPU units.

        NUNITS=<n>

##### Example
Set the number of CPU units to `6` (*the default setting*):
        SET CPU NUNITS=6

<!------------------------------------------------------------------------------------->

#### KIPS
**`KIPS`** configures the global CPU lockup fault timer scaling factor.

        KIPS=<n>

When simulating multiple processors, occasional spurious "**`Fault in idle process`**"
errors may be seen when additional CPU's are being brought online. This is caused
by a lockup fault occurring while the Multics hardcore is waiting on a flag to
be set by the CPU being brought online.

Latency guarantees in the original hardware prevent this from ever occurring on
properly functioning hardware.  Unfortunately, without introducing a strict
hard-realtime requirement for simulated operations (thus requiring users run the
simulator only under control of a capable RTOS, or possibly on a completely idle
system - without no other processes active - or other unreasonable restrictions),
there is no practical way to provide Multics such precision timing guarantees.

When running on original hardware, the lockup fault would be triggered by an independent
watchdog timer that monitors the CPU's polling of interrupts.  If interrupts would be
inhibited for too long a duration, this watchdog timer would run out and trigger the
lockup fault.  Multics rightfully considers receiving a lockup fault in hardcore as
indicative of a serious (and fatal) problem with the faulting CPU.

Unfortunately, what is a clear sign of fatal trouble on the real hardware
(*i.e* a CPU that is struggling to get up to speed or running at a fluctuating speeds)
is the normal, expected, and unavoidable reality on the simulator.

Configuring the "`KIPS`" scaling factor allows for loosening of these hard realtime
requirements and avoids spurious lockup faults.

Note that if "`KIPS`" is set to a large value, a genuine hardcore error or broken userspace process
will still be detected, though it will take slightly longer for the lockup fault to occur.

##### Example
Set the global CPU lockup fault scaling factor for 8 MIPS:
        SET CPU KIPS=8000

* **NOTE**: It is not expected that the "`KIPS`" value will require modification by most
users under normal operating circumstances.

* **SEE ALSO**: "**`STALL`**".

<!------------------------------------------------------------------------------------->

#### STALL
**`STALL`** configures a stall (*i.e.* a delay) of **`<iterations>`** when
the IC reaches the address specified by **`<segno>`** and **`<offset>`**, identified by
**`<num>`**.

        STALL=<num>=<segno>:<offset>=<iterations>

* **`<num>`** is a number allowing for the identification (and modification) of multiple stall points.
* **`<segno>`** is the segment number of the Multics segment at which to stall.
* **`<offset>`** is the offset within the segment specified by **`<segno>`** at which to stall.
* **`<iterations>`** is a positive integer representing the number of iterations to loop.

##### Example
Configure two stalls, #`1` for `250` iterations at `0132:011227`, and #`2` for `1250` iterations at `0122:052137`:
        SET CPU STALL=1=0132:011227=250
        SET CPU STALL=2=0122:052137=1250

<!------------------------------------------------------------------------------------->

#### DEBUG (NODEBUG)
**`DEBUG`** enables CPU debugging and enables specific debugging options.

**`NODEBUG`** disables CPU debugging and disables specific debugging options.

        DEBUG                                 ; Enables debugging
        NODEBUG                               ; Disables debugging
        DEBUG=<list-of-options>               ; Enables specified debugging options
        NODEBUG=<list-of-options>             ; Disables specified debugging options

* The **`<list-of-options>`** is a semicolon ("`;`") delimited list of one or more of the following options:

|                   |                  |                   |                  |                  |
|:-----------------:|:----------------:|:-----------------:|:----------------:|:----------------:|
| **`TRACE`**       | **`TRACEEXT`**   | **`MESSAGES`**    | **`REGDUMPAQI`** | **`REGDUMPIDX`** |
| **`REGDUMPPR`**   | **`REGDUMPPPR`** | **`REGDUMPDSBR`** | **`REGDUMPFLT`** | **`REGDUMP`**    |
| **`ADDRMOD`**     | **`APPENDING`**  | **`NOTIFY`**      | **`INFO`**       | **`ERR`**        |
| **`WARN`**        | **`DEBUG`**      | **`ALL`**         | **`FAULT`**      | **`INTR`**       |
| **`CORE`**        | **`CYCLE`**      | **`CAC`**         | **`FINAL`**      | **`AVC`**        |

##### Examples
\
\
Enable the debugging options "`TRACE`" and "`FAULT`" for all CPU units:
        SET CPU DEBUG=TRACE;FAULT

Disable all debugging options for all CPU units:
        SET CPU NODEBUG

Enable the debugging option "`CYCLE`" for CPU**`0`** and disable all debugging options for all other CPU units:
        SET CPU NODEBUG
        SET CPU0 DEBUG=CYCLE

<!------------------------------------------------------------------------------------->

### IOM Configuration

* TBD

### SCU Configuration

* TBD

### TAPE Configuration

* TBD

### MTP Configuration

* TBD

### IPC Configuration

* TBD

### MSP Configuration

* TBD

### Disk Configuration

* TBD

### RDR Configuration

* TBD

### PUN Configuration

* TBD

### PRT Configuration

* TBD

### FNP Configuration

* TBD

### OPC Configuration

* TBD

### URP Configuration

* TBD


