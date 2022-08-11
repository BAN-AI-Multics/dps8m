<!-- vim: set ft=markdown ts=2 sw=2 tw=79 cc=80 et spell nolist wrap lbr :-->
<!-- vim: set ruler hlsearch incsearch autoindent wildmenu wrapscan :-->
<!-- SPDX-License-Identifier: LicenseRef-DPS8M-Doc OR LicenseRef-CF-GAL -->
<!-- SPDX-FileCopyrightText: 2021-2022 The DPS8M Development Team -->
<!-- scspell-id: 10e2bc0b-fc68-11ec-9584-80ee73e9b8e7 -->

\begin{tcolorbox}[title=dps8 --help,colback=black!2!white]
\begin{Verbatim}
DPS8/M simulator X2.0.1-rc2-docs-20220702+1149

 USAGE: dps8 { [ SWITCHES ] ... } { < SCRIPT > }

 Invokes the DPS8/M simulator, with optional switches and/or script file.

 Switches:
  -e, -E            Aborts script processing immediately upon any error
  -h, -H, --help    Prints only this informational help text and exit
  -k, -K            Disables all support for exclusive file locking
  -l, -L            Reports but ignores all exclusive file locking errors
  -o, -O            Makes scripting ON conditions and actions inheritable
  -q, -Q            Disables printing of non-fatal informational messages
  -r, -R            Enables an unlinked ephemeral system state file
  -s, -S            Enables a randomized persistent system state file
  -t, -T            Disables fsync and creation/usage of system state file
  -v, -V            Prints commands read from script file before execution
  --version         Prints only the simulator identification text and exit

 This software is made available under the terms of the ICU License, version
 1.8.1 or later.  For complete details, see the "LICENSE.md" file included
 with the software or https://gitlab.com/dps8m/dps8m/-/blob/master/LICENSE.md
\end{Verbatim}
\end{tcolorbox}

\begin{tcolorbox}[title=dps8 --version,colback=black!2!white]
\begin{Verbatim}
DPS8/M simulator X2.0.1-rc2-docs-20220702+1149
\end{Verbatim}
\end{tcolorbox}

\begin{tcolorbox}[title=SHOW BUILDINFO,colback=black!2!white]
\begin{Verbatim}
 Build Information:
      Compilation info: cc -Wall -O3 -g3 -fno-strict-aliasing
      -DCLOCK_REALTIME=CLOCK_REALTIME_COARSE -DUSE_FLOCK=1 -DUSE_FCNTL=1
      -std=c99 -U__STRICT_ANSI__ -D_GNU_SOURCE -flto=auto -pthread -DLOCKLESS
      -DVER_CURRENT_TIME=2022-07-04 07:17 UTC -lpthread -luv -lrt -ldl
  Relevant definitions: -D__amd64__=1 -D__BYTE_ORDER__=__ORDER_LITTLE_ENDIAN__
  -D__FLOAT_WORD_ORDER__=__ORDER_LITTLE_ENDIAN__ -D__GNUC__=12 -D__linux=1
  -D__linux__=1 -D__SIZEOF_LONG__=8 -D__STDC__=1 -D__unix__=1 -D__x86_64__=1
    Event loop library: Built with libuv 1.44.1 (release); 1.44.1 in use
          Math library: decNumber 3.68 (Mike Cowlishaw and contributors)
     Atomic operations: GNU-style
          File locking: POSIX-style fcntl() and BSD-style flock() locking
\end{Verbatim}
\end{tcolorbox}

\begin{tcolorbox}[title=SHOW VERSION,colback=black!2!white,breakable=true]
\begin{Verbatim}
 DPS8/M Simulator:
   Version: X2.0.1-rc2-docs-20220702+1149
    Commit: d4741c186157111100f40fd8d95cc6b53f62404a
  Modified: 2022-07-04 03:12 UTC - Kit Prepared: 2022-07-04 07:17 UTC
  Compiled: 2022-07-04 07:17 UTC

 Build Information:
  Build OS: Linux 5.17.13-300.fc36.x86_64 x86_64
  Compiler: GCC 12.1.1 20220507 (Red Hat 12.1.1-1) x86_64
  Built by: jhj@brshpc0

 Host System Information:
   Host OS: Linux 5.17.13-300.fc36.x86_64 x86_64

 This software is made available under the terms of the ICU License,
 version 1.8.1 or later.  For complete details, see the "LICENSE.md"
 included or https://gitlab.com/dps8m/dps8m/-/blob/master/LICENSE.md
\end{Verbatim}
\end{tcolorbox}

\begin{tcblisting}{colback=black!2!white,listing only,
  title=XML sources,
  listing engine=minted, minted language=xml}
<?xml version="1.0"?>
<project name="Package tcolorbox" default="documentation" basedir=".">
  <description>
    Apache Ant build file (http://ant.apache.org/)
  </description>
</project>
\end{tcblisting}

\begin{tcblisting}{colback=black!2!white,listing only,
  listing engine=minted,minted language=text,
  hbox,enhanced,drop fuzzy shadow,before=\begin{center},after=\end{center},
  title=DPS8M Documentation Source Tree}
.
├── builddoc-docker.sh
├── builddoc.sh
├── commandref.md
├── docker
│   └── Dockerfile
├── GNUmakefile
├── latex
│   └── eisvogel-dps8.latex
├── lua
│   └── pagebreak
│       ├── pagebreak.lua
│       └── README.md
├── md
│   ├── acknowledgements.md
│   ├── authors.md
│   ├── buildsource.md
│   ├── forward.md
│   ├── introduction.md
│   ├── licensing.md
│   ├── preface.md
│   └── prologue.md
├── pdf
│   ├── color-rings.pdf
│   └── color-rings.pdf.license
├── php
│   └── i2f.php
├── ps
│   └── rings
│       └── multics-rings.ps
├── README.md
├── shell
│   ├── refgen.sh
│   ├── removeblank.sh
│   └── toolinfo.sh
├── test.md
└── yaml
    ├── docinfo-post.yml
    └── docinfo.yml

13 directories, 38 files
\end{tcblisting}
