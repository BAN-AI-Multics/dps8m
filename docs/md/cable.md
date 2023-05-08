<!-- SPDX-License-Identifier: LicenseRef-DPS8M-Doc OR LicenseRef-CF-GAL -->
<!-- SPDX-FileCopyrightText: 2022-2023 The DPS8M Development Team -->
<!-- scspell-id: 7988dd71-3344-11ed-8b11-80ee73e9b8e7 -->
The `CABLE` command (*abbreviated* `C`) connects (or "*strings*") a simulated cable between devices.

        CABLE <device> <port/channel> <device> {port/channel}

**Examples**

* Connect a cable from (*system controller unit*) "`SCU`**`i`**" port "**`j`**" to (*central processing unit*) "`CPU`**`k`**" port "**`l`**".
        CABLE SCUi j CPUk l

* Connect a cable from "`SCU`**`i`**" port "**`j`**" to (*input/output multiplexer*) "`IOM`**`k`**" port "**`l`**".
        CABLE SCUi j IOMk l

* Connect a cable from "`IOM`**`i`**" channel "**`j`**" to (*magnetic tape processor*) "`MTP`**`k`**" port "**`l`**", where "**`l`**" defaults to "**`0`**" when not specified.
        CABLE IOMi j MTPk
        CABLE IOMi j MTPk l

* Connect a cable from "`IOM`**`i`**" channel "**`j`**" to (*mass storage processor*) "`MSP`**`k`**" port "**`l`**", where "**`l`**" defaults to "**`0`**" when not specified.
        CABLE IOMi j MSPk
        CABLE IOMi j MSPk l

* Connect a cable from "`IOM`**`i`**" channel "**`j`**" to (*integrated peripheral controller*) "`IPC`**`k`**" port "**`l`**", where "**`l`**" defaults to "**`0`**" when not specified.
        CABLE IOMi j IPCk
        CABLE IOMi j IPCk l

* Connect a cable from "`IOM`**`i`**" channel "**`j`**" to (*operator console*) "`OPC`**`k`**".
        CABLE IOMi j OPCk

* Connect a cable from "`IOM`**`i`**" channel "**`j`**" to (*front-end network processor*) "`FNP`**`k`**".
        CABLE IOMi j FNPk

* Connect a cable from "`MTP`**`i`**" device code "**`j`**" to (*tape drive*) "`TAPE`**`k`**".
        CABLE MTPi j TAPEk

* Connect a cable from "`IPC`**`i`**" device code "**`j`**" to (*disk drive*) "`DISK`**`k`**".
        CABLE IPCi j DISKk

* Connect a cable from "`MSP`**`i`**" device code "**`j`**" to "`DISK`**`k`**".
        CABLE MSPi j DISKk

* Connect a cable from (*unit record processor*) "`URP`**`i`**" device code "**`j`**" to (*card reader*) "`RDR`**`k`**".
        CABLE URPi j RDRk

* Connect a cable from "`URP`**`i`**" device code "**`j`**" to (*card punch*) "`PUN`**`k`**".
        CABLE URPi j PUNk

* Connect a cable from "`URP`**`i`**" device code "**`j`**" to (*printer*) "`PRT`**`k`**".
        CABLE URPi j PRTk

<!-- br -->

### UNCABLE (U)

The "**`UNCABLE`**" command (*abbreviated* `U`) removes (or "*unstrings*") a simulated cable.

        UNCABLE <device> <port/channel> <device>

**Example**

* Unstring the cable connecting "`IOM`**`0`**" channel "**`12`**" to "`MSP`**`0`**".
        UNCABLE IOM0 12 MSP0

<!-- br -->

### CABLE_RIPOUT

The "**`CABLE_RIPOUT`**" command (*alias* "**`CABLE RIPOUT`**") removes (or *unstrings*) ***all*** cables from the configuration.

**Example**

        CABLE RIPOUT
        CABLE_RIPOUT

<!-- br -->

### CABLE_SHOW

The "**`CABLE_SHOW`**" command (*alias* "**`CABLE SHOW`**") prints the current cabling configuration in human readable form.

**Example**

SHOWCABLESHOWHERE

<!-- br -->

### CABLE DUMP

The "**`CABLE DUMP`**" command prints the current cabling configuration in great detail.

<!-- br -->

### CABLE GRAPH

The "**`CABLE GRAPH`**" command prints the current cabling configuration in the "**DOT**" graph description language (suitable for rendering with *GraphViz*, etc).

<!-- pagebreak -->

#### Complete Cabling Graph

The following is a **complete** cabling graph of the *default base system*:

\captionsetup{labelformat=empty}
![](./pdf/neato.pdf)
\begin{figure}[!h]
\caption{}
\end{figure}

<!-- pagebreak -->

#### CPU / SCU / IOM Cabling Graph

The following graph shows the cabling configuration of the *default base system*'s "**`CPU`**", "**`SCU`**", and "**`IOM`**" devices:

\captionsetup{labelformat=empty}
![](./pdf/gvsubset.pdf)
\begin{figure}[!h]
\caption{}
\end{figure}

<!-- pagebreak -->

#### Storage Cabling Graph

The following graph shows the cabling configuration of the *default base system*'s **storage** devices:

\captionsetup{labelformat=empty}
![](./pdf/storage.pdf)
\begin{figure}[!h]
\caption{}
\end{figure}

<!-- pagebreak -->

#### Controller Cabling Graph

The following graph shows the cabling configuration of the *default base system*'s **controller** devices:

\captionsetup{labelformat=empty}
![](./pdf/iomcon.pdf){height=60%}
\begin{figure}[!h]
\caption{}
\end{figure}

See the "**Simulator Defaults**" chapter for more details (including the full output of the "`CABLE DUMP`" command).

