# Licenses and Acknowledgements

<!-- toc -->

- [DPS8M Software](#dps8m)
  * [DPS8M Simulator](#dps8m-simulator)
    * [The DPS8M Development Team](#dps8m-development-team)
- [Third-party Software](#third-party-software)
  * [Unifdef](#unifdef)
  * [UDPLib](#udplib)
  * [UtHash](#uthash)
  * [SimH](#simh)
  * [PunUtil](#punutil)
  * [Prt2PDF](#prt2pdf)
    * [Txt2PDF](#txt2pdf)
  * [decNumber](#decnumber)
  * [libTELNET](#libtelnet)
  * [Shell Routines](#shell-routines)
  * [LineHistory](#linehistory)
  * [BSD Random](#bsd-random)
  * [AMD LibM](#amd-libm)
  * [mimalloc](#mimalloc)
  * [musl](#musl)
  * [Multics Software Materials and Documentation](#multics-software-materials-and-documentation)
- [Scope of Intended Application](#scope-of-intended-application)
- [Disclaimer of Liability and Endorsement](#disclaimer-of-liability-and-endorsement)

<!-- tocstop -->

## DPS8M

[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgitlab.com%2Fdps8m%2Fdps8m.svg?type=large)](https://app.fossa.com/projects/git%2Bgitlab.com%2Fdps8m%2Fdps8m?ref=badge_large)

### DPS8M Simulator

* The **DPS8M Simulator** (“**DPS8M**”) is distributed under the ICU License.

```text
ICU License —— ICU 1.8.1 to ICU 57.1

COPYRIGHT AND PERMISSION NOTICE

Copyright (c) 2006‑2021 Michael Mondy, Harry Reed, Charles Anthony, and others
Copyright (c) 2012 Dave Jordan
Copyright (c) 2015‑2016 Craig Ruff
Copyright (c) 2015‑2021 Eric Swenson
Copyright (c) 2016 Jean‑Michel Merliot
Copyright (c) 2017‑2021 Jeffrey H. Johnson
Copyright (c) 2018‑2021 Juergen Weiss
Copyright (c) 2021 Dean S. Anderson
Copyright (c) 2021 The DPS8M Development Team

All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
“Software”), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, provided that the above
copyright notice(s) and this permission notice appear in all copies of
the Software and that both the above copyright notice(s) and this
permission notice appear in supporting documentation.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY
SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Except as contained in this notice, the name of a copyright holder
shall not be used in advertising or otherwise to promote the sale, use
or other dealings in this Software without prior written authorization
of the copyright holder.

All trademarks and registered trademarks mentioned herein are the
property of their respective owners.
```

### DPS8M Development Team

* ***The DPS8M Development Team*** is, in alphabetical order:

  * Dean S. Anderson,
  * Charles Anthony
    \<[charles.unix.pro@gmail.com](mailto:charles.unix.pro@gmail.com)\>,
  * Jeffrey H. Johnson
    \<[trnsz@pobox.com](mailto:trnsz@pobox.com)\>,
  * Jean-Michel Merliot,
  * Michael Mondy
    \<[michael.mondy@coffeebird.net](mailto:michael.mondy@coffeebird.net)\>,
  * Harry Reed
    \<[doon386@reedclan.org](mailto:doon386@reedclan.org)\>,
  * Craig Ruff,
  * Eric Swenson
    \<[eric@swenson.org](mailto:eric@swenson.org)\>,
  * M⸻ T⸻
  * Juergen Weiss
    \<[weiss@uni-mainz.de](mailto:weiss@uni-mainz.de)\>,
  * M. Williams (OrangeSquid),
  * *and others ...*

----

## Third-party Software

* **DPS8M** incorporates, adapts, or utilizes software from
  third-parties. This software is copyrighted by their respective
  owners and licensed as indicated in the following disclosures.

----

### Unifdef

* **Unifdef** is a utility that selectively processes conditional C
  preprocessor directives derived from software contributed to the
  *Computer Systems Research Group* at the *University of California,
  Berkeley* by Dave Yost. It was rewritten to support ANSI C by Tony
  Finch, and modified for portability by **The DPS8M Development Team**.
  It is distributed under a two-clause BSD licence.

```text
Copyright (c) 2002‑2020 Tony Finch <dot@dotat.at>
Copyright (c) 2021 The DPS8M Development Team

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS “AS IS” AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.
```

----

### UDPLib

* **UDPLib** is a library that implements the BBN ARPAnet IMP/TIP
  Modem/Host Interface over UDP. It was written by Robert Armstrong,
  and is distributed under a modified BSD license.

```text
Copyright (c) 2013 Robert Armstrong, bob@jfcl.com

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the “Software”),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

  * The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
ROBERT ARMSTRONG BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Robert Armstrong shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from Robert Armstrong.
```

----

### UtHash

* **UtHash** (*UtList*, *UtArray*, and *UtHash*), a hash table for C
  structures, was written by
  [Troy D. Hanson](https://troydhanson.github.io/uthash/) and modified
  by **The DPS8M Development Team**. It is distributed under a modified
  one-clause BSD license.

```text
Copyright (c) 2005‑2021 Troy D. Hanson
    http://troydhanson.github.io/uthash/
Copyright (c) 2021 The DPS8M Development Team

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS
IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

----

### SimH

* **SimH** is a portable systems simulation framework, written by
  Robert M. Supnik and others, and distributed under a modified MIT license.

```text
Copyright (c) 1993‑2012 Robert M Supnik
Copyright (c) 2002‑2007 David T. Hittner
Copyright (c) 2005‑2016 Richard Cornwell
Copyright (c) 2007‑2008 Howard M. Harte
Copyright (c) 2008‑2009 J. David Bryan
Copyright (c) 2011‑2013 Matt Burke
Copyright (c) 2011‑2015 Mark Pizzolato
Copyright (c) 2013 Timothe Litt

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the “Software”),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

  * The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the authors shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the authors.
```

----

### PunUtil

* **PunUtil** is a utility to process the output of the **DPS8M** punch
  device. It was written by Dean S. Anderson of **The DPS8M Development
  Team** and is distributed under the ICU License.

```text
ICU License —— ICU 1.8.1 to ICU 57.1

COPYRIGHT AND PERMISSION NOTICE

Copyright (c) 2021 The DPS8M Development Team

All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
“Software”), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, provided that the above
copyright notice(s) and this permission notice appear in all copies of
the Software and that both the above copyright notice(s) and this
permission notice appear in supporting documentation.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY
SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Except as contained in this notice, the name of a copyright holder
shall not be used in advertising or otherwise to promote the sale, use
or other dealings in this Software without prior written authorization
of the copyright holder.

All trademarks and registered trademarks mentioned herein are the
property of their respective owners.
```

----

### Prt2PDF

* **Prt2PDF** is a utility to convert the output of the simulated
  **DPS8M** line printer device to ISO 32000 Portable Document Format
  (*PDF*), and is distributed under the ICU License.

```text
ICU License —— ICU 1.8.1 to ICU 57.1

COPYRIGHT AND PERMISSION NOTICE

Copyright (c) 2013‑2016 Charles Anthony <charles.unix.pro@gmail.com>
Copyright (c) 2006 John S. Urban, USA. <urbanjost@comcast.net>

All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
“Software”), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, provided that the above
copyright notice(s) and this permission notice appear in all copies of
the Software and that both the above copyright notice(s) and this
permission notice appear in supporting documentation.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY
SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Except as contained in this notice, the name of a copyright holder
shall not be used in advertising or otherwise to promote the sale, use
or other dealings in this Software without prior written authorization
of the copyright holder.

All trademarks and registered trademarks mentioned herein are the
property of their respective owners.
```

----

#### Txt2PDF

* **Prt2PDF** is derived from **Txt2PDF**, a utility to transform ASCII
  text files to Adobe Acrobat PDF documents, written by P. G. Womack, and
  distributed with the following restrictions.

```text
* Copyright (c) 1998 P. G. Womack, Diss, Norfolk, UK.

“Do what you like, but don’t claim you wrote it.”
```

----

### decNumber

* **decNumber** is an ANSI C reference implementation of the *IBM General
  Decimal Arithmetic* standard, implementing decimal floating‑point
  arithmetic and the ‘*Strawman 4d*’ decimal encoding formats described
  by the revised IEEE 754 specification.  It was written by IBM and IEEE
  Fellow Mike Cowlishaw, with contributions by Matthew Hagerty, John Matzka,
  Klaus Kretzschmar, Stefan Krah, and The DPS8M Development Team.  It is
  distributed under the ICU License.

```text
ICU License —— ICU 1.8.1 to ICU 57.1

COPYRIGHT AND PERMISSION NOTICE

Copyright (c) 1995‑2010 International Business Machines Corporation and others

All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
“Software”), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, provided that the above
copyright notice(s) and this permission notice appear in all copies of
the Software and that both the above copyright notice(s) and this
permission notice appear in supporting documentation.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY
SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Except as contained in this notice, the name of a copyright holder
shall not be used in advertising or otherwise to promote the sale, use
or other dealings in this Software without prior written authorization
of the copyright holder.

All trademarks and registered trademarks mentioned herein are the
property of their respective owners.
```

----

### LibTELNET

* **LibTELNET** is an implementation of the protocol specified by the Network
  Working Group of the Internet Engineering Task Force, as described in
  RFC‑854, RFC‑855, RFC‑1091, RFC‑1143, RFC‑1408, and RFC‑1572 — the TELNET
  protocol and the Q method for TELNET protocol option negotiation — written
  by Sean Middleditch and other contributors.

  * **LibTELNET** has been identified by **The DPS8M Development Team** as
    being free of known restrictions under copyright law, including all
    related and neighboring rights.

  * **The DPS8M Development Team** makes no warranties about **LibTELNET**,
    and disclaims liability for all uses of **LibTELNET** to the fullest
    extent permitted by applicable law.

```text
The author or authors of this code dedicate any and all copyright interest
in this code to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and successors.
We intend this dedication to be an overt act of relinquishment in perpetuity
of all present and future rights to this code under copyright law.
```

----

### Shell Routines

* Shell scripts may adapt or incorporate POSIX™ shell functions, routines, and
  techniques (“the Routines”), written or distributed by
  [Thorsten “mirabilos” Glaser](https://www.mirbsd.org/).

  * The Routines have been identified as being free of known restrictions
    under copyright law, including all related and neighboring rights.

  * The Routines are in the public domain as they do not meet the threshold
    of originality required for copyright protection in most jurisdictions.

  * The Routines are also distributed under the terms and conditions of the
    BSD “Zero Clause” license, or at your option, any other license which
    meets the Open Source Definition and is approved by Open Source
    Initiative, should the Routines **not** be in the public domain in your
    legal jurisdiction.

```text
Copyright (c) 2011 Thorsten “mirabilos” Glaser <m@mirbsd.org>

Permission to use, copy, modify, and/or distribute this software
for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
```

----

### LineHistory

* **LineHistory** is a small and self-contained line editor, implementing
  *Emacs*-style line editing functionality similar to GNU *readline* or BSD
  *libedit*. It is derived from the *linenoise* library written by Salvatore
  “antirez” Sanfilippo and Pieter Noordhuis and distributed under a two-clause
  BSD license.

```text
Copyright (c) 2010-2016, Salvatore Sanfilippo <antirez at gmail dot com>
Copyright (c) 2010-2013, Pieter Noordhuis <pcnoordhuis at gmail dot com>
Copyright (c) 2021 The DPS8M Development Team

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
“AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

----

### BSD Random

* **BSD Random** is a collection of random number generation functions
  derived from software originally developed for the *Berkeley Software
  Distribution* by the *Computer Systems Research Group* at the *University
  of California, Berkeley*, and copyrighted by *The Regents of the University
  of California*.  It is distributed under a three-clause BSD license.

```text
Copyright (c) 1983‑1991 The Regents of the University of California
Copyright (c) 2021 The DPS8M Development Team

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of the University nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS “AS IS” AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.
```

----

### AMD LibM

* **AMD LibM**, a component of *AOCL*, the *AMD Optimizing CPU Libraries*,
  is a software library containing a collection of basic math functions
  optimized for AMD x86-64 processor-based machines, and provides
  high-performance scalar and vector variants of many core C99 math functions.
  It was developed by *Advanced Micro Devices, Inc.* and is distributed under
  the terms of a three-clause BSD license.

```text
BSD 3 Clause

Copyright (c) 2017-2021, Advanced Micro Devices, Inc.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

   * Neither the name of the copyright holder nor the names of its contributors
     may be used to endorse or promote products derived from this software
     without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

----

### mimalloc

* **mimalloc** is a compact, portable, high-performance, general-purpose
  free-list multi-sharding implementation of `malloc`, initially developed by
  Daan Leijen of the *Research In Software Engineering* group at *Microsoft
  Research* to support the run-time systems of the *Lean* and *Koka* languages.
  It is distributed under the terms of the MIT License.

```text
MIT License

Copyright (c) 2018-2021 Microsoft Corporation, Daan Leijen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

  * The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

----

### musl

* **musl** is a lightweight standards-conformant implementation of the C
  standard library, which includes the interfaces defined in the base language
  standard, POSIX™, and widely agreed-upon extensions. It was written by
  *Rich "dalias" Felker* and other contributors, and is distributed under the
  terms of the MIT License.

```text
Copyright (c) 2005-2020 Rich Felker, et al.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

  * The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

----
### Multics Software Materials and Documentation

* **DPS8** includes some **Multics** software materials and documentation.
  These materials and documentation are distributed under the terms of the
  **Multics License**.

```text
Copyright (c) 1972 by the Massachusetts Institute of Technology and
  Honeywell Information Systems, Inc.
Copyright (c) 2006 by Bull HN Information Systems, Inc.
Copyright (c) 2006 by Bull SAS.

All rights reserved.

Historical Background

* This edition of the Multics software materials and documentation is
  provided and donated to the Massachusetts Institute of Technology by
  Group BULL including BULL HN Information Systems, Inc. as a
  contribution to computer science knowledge.

* This donation is made also to give evidence of the common contributions
  of the Massachusetts Institute of Technology, Bell Laboratories,
  General Electric, Honeywell Information Systems, Inc., Honeywell BULL,
  Inc., Groupe BULL and BULL HN Information Systems, Inc. to the
  development of this operating system.

* Multics development was initiated by the Massachusetts Institute of
  Technology Project MAC (1963-1970), renamed the MIT Laboratory for
  Computer Science and Artificial Intelligence in the mid 1970s, under
  the leadership of Professor Fernando José Corbató.  Users consider
  that Multics provided the best software architecture for managing
  computer hardware properly and for executing  programs.  Many
  subsequent operating systems incorporated Multics principles.

* Multics was distributed in 1975 to 2000 by Group Bull in Europe, and
  in the U.S. by Bull HN Information Systems, Inc., as successor in
  interest by change in name only to Honeywell Bull, Inc. and Honeywell
  Information Systems, Inc.

Permission to use, copy, modify, and distribute these programs and their
documentation for any purpose and without fee is hereby granted, provided
that this copyright notice and the above historical background appear in
all copies and that both the copyright notice and historical background
and this permission notice appear in supporting documentation, and that
the names of MIT, HIS, BULL or BULL HN not be used in advertising or
publicity pertaining to distribution of the programs without specific
prior written permission.
```

----

## Scope of Intended Application

* The **DPS8M Simulator** is **not designed or intended** for use in any
  safety-critical, life-critical, or life-sustaining applications or
  activities.

* These applications and activies include, but are not limited to,
  military, nuclear reactor control, power generation and transmission,
  factory automation, industrial process control, robotics, avionics,
  aerospace, air traffic control, emergency communications, railway,
  marine, motor vehicle, fire suppression, pharmaceutical, medical or
  safety devices, any conventional, nuclear, biological or chemical
  weaponry, or any other applications or activities that could reasonably
  be expected to potentially impact human health and safety or lead to
  environmental damage.

----

## Disclaimer of Liability and Endorsement

* While **The DPS8M Development Team** (“*the Team*”) has expended reasonable
  efforts to make the information available in this document as accurate as
  possible, the Team makes no claims, promises, or guarantees regarding
  accuracy, completeness, or adequacy of any information contained in this
  document.

* **The DPS8M Development Team** expressly disclaims liability for any
  errors and omissions in the contents of this document.

* **NO WARRANTY OF ANY KIND**, either express or implied, including but not
  limited to the warranties of non-infringement, merchantability, or fitness
  for a particular purpose, is given with respect to the contents of this
  document or the contents of linked external resources.  Linked external
  resources are not under the control of **The DPS8M Development Team**.

* Any reference in this document to any specific entity, process, or service,
  or the use of any trade, firm, or corporation name, or any links to
  external resources are provided for information and convenience purposes
  only, and in no way constitute endorsement, sponsorship, affiliation, or
  recommendation by **The DPS8M Development Team**.

----
