---
weight: 3000
title: DPS8M Releases
---
<!-- SPDX-License-Identifier: ICU -->
<!-- Copyright (c) 2022-2023 The DPS8M Development Team -->
### Current Releases
* [**Stable Release**](#stable-release)
* [**Bleeding Edge**](#bleeding-edge)

## Stable Release
* Stable releases are cryptographically signed by members of **The DPS8M Development Team**.
* You can [**download the OpenPGP public keyring**](../keyring.asc) required to authenticate your distribution here.
* Review the [**release notes**](https://gitlab.com/dps8m/dps8m/-/wikis/DPS8M-R3.0.0-Release-Notes) and [**errata**](https://gitlab.com/dps8m/dps8m/-/wikis/R3.0.0%20Errata) for more details.

## DPS8M R3.0.0 ― 2022-11-23
{{< tabs "stablesrctable" >}}
{{< tab "Source" >}}
| File                                                                           | Format               | Verification |
|:------------------------------------------------------------------------------ |:-------------------- |:------------ |
| [**dps8m-r3.0.0-src.tar.lz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-src.tar.lz) | tar&nbsp;+&nbsp;lzip | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-src.tar.lz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-src.tar.lz.asc)&nbsp;\]</small>  |
{{< expand "Alternate formats" "…" >}}
| File                                                                           | Format               | Verification |
|:------------------------------------------------------------------------------ |:-------------------- |:------------ |
| [**dps8m-r3.0.0-src.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-src.tar.gz) | tar&nbsp;+&nbsp;gzip | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-src.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-src.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-src.7z**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-src.7z)         | 7-Zip (LZMA)         | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-src.7z.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-src.7z.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-src.zip**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-src.zip)       | ZIP (DEFLATE)        | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-src.zip.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-src.zip.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-src.zpaq**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-src.zpaq)     | ZPAQ7                | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-src.zpaq.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-src.zpaq.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /tab >}}
{{< tab "AIX" >}}

{{< expand "Power ISA (64-bit)" "…" >}}
| File                                                                                       | Platform             | Verification |
|:------------------------------------------------------------------------------------------ |:-------------------- |:------------ |
| [**dps8m-r3.0.0-aixv7-p10.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-p10.tar.gz) | Power10              | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-p10.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-p10.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-aixv7-pw9.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-pw9.tar.gz) | POWER9               | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-pw9.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-pw9.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-aixv7-pw8.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-pw8.tar.gz) | POWER8 (GCC&nbsp;12) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-pw8.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-pw8.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-aixv7-gcc.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-gcc.tar.gz) | POWER8 (GCC&nbsp;10) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-gcc.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-gcc.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-aixv7-xlc.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-xlc.tar.gz) | POWER8 (XL&nbsp;C&nbsp;16) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-xlc.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-xlc.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-aixv7-pw7.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-pw7.tar.gz) | POWER7               | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-pw7.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-pw7.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-aixv7-pw6.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-pw6.tar.gz) | POWER6               | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-pw6.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-aixv7-pw6.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /tab >}}
{{< tab "Android" >}}

{{< expand "Android 10.0 (API Level 29)" "…" >}}
{{< expand "ARM" "…" >}}
{{< expand "ARM64" "…" >}}
| File                                                                                       | Verification |
|:------------------------------------------------------------------------------------------ |:------------ |
| [**dps8m-r3.0.0-ndk29-a64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk29-a64.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk29-a64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk29-a64.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "ARM (32-bit)" "…" >}}
| File                                                                                       | Verification |
|:------------------------------------------------------------------------------------------ |:------------ |
| [**dps8m-r3.0.0-ndk29-a32.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk29-a32.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk29-a32.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk29-a32.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}

{{< expand "Intel x86" "…" >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       | Verification |
|:------------------------------------------------------------------------------------------ |:------------ |
| [**dps8m-r3.0.0-ndk29-x64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk29-x64.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk29-x64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk29-x64.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "Intel ix86 (32-bit)" "…" >}}
| File                                                                                       | Verification |
|:------------------------------------------------------------------------------------------ |:------------ |
| [**dps8m-r3.0.0-ndk29-x32.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk29-x32.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk29-x32.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk29-x32.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}
{{< /expand >}}

{{< expand "Android 7.0 (API Level 24)" "…" >}}
{{< expand "ARM" "…" >}}
{{< expand "ARM64" "…" >}}
| File                                                                                       | Verification |
|:------------------------------------------------------------------------------------------ |:------------ |
| [**dps8m-r3.0.0-ndk24-a64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk24-a64.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk24-a64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk24-a64.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "ARM (32-bit)" "…" >}}
| File                                                                                       | Verification |
|:------------------------------------------------------------------------------------------ |:------------ |
| [**dps8m-r3.0.0-ndk24-a32.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk24-a32.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk24-a32.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk24-a32.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}

{{< expand "Intel x86" "…" >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       | Verification |
|:------------------------------------------------------------------------------------------ |:------------ |
| [**dps8m-r3.0.0-ndk24-x64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk24-x64.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk24-x64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk24-x64.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "Intel ix86 (32-bit)" "…" >}}
| File                                                                                       | Verification |
|:------------------------------------------------------------------------------------------ |:------------ |
| [**dps8m-r3.0.0-ndk24-x32.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk24-x32.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk24-x32.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-ndk24-x32.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}
{{< /expand >}}
{{< /tab >}}
{{< tab "BSD" >}}

{{< expand "FreeBSD 13.1" "…" >}}
{{< expand "Intel x86" "…" >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       | Platform                           | Verification |
|:------------------------------------------------------------------------------------------ |:---------------------------------- |:------------ |
| [**dps8m-r3.0.0-fbd13-x64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-x64.tar.gz) | x86_64‑v4 (AVX‑512)                | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-x64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-x64.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-fbd13-x63.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-x63.tar.gz) | x86_64‑v3 (AVX2&nbsp;+&nbsp;FMA)   | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-x63.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-x63.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-fbd13-x62.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-x62.tar.gz) | x86_64‑v2 (SSE4&nbsp;+&nbsp;SSSE3) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-x62.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-x62.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-fbd13-x61.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-x61.tar.gz) | x86_64                             | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-x61.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-x61.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "Intel ix86 (32-bit)" "…" >}}
| File                                                                                       | Platform | Verification |
| :----------------------------------------------------------------------------------------- | :------- |:------------ |
| [**dps8m-r3.0.0-fbd13-x32.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-x32.tar.gz) | i686     | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-x32.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-x32.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}

{{< expand "ARM64 (little-endian)" "…" >}}
| File                                                                                       | Platform | Verification |
| :----------------------------------------------------------------------------------------- | :------- |:------------ |
| [**dps8m-r3.0.0-fbd13-a64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-a64.tar.gz) | ARMv8‑A  | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-a64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-a64.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "PowerPC (32-bit, big-endian)" "…" >}}
| File                                                                                       | Platform   | Verification |
| :----------------------------------------------------------------------------------------- | :--------- |:------------ |
| [**dps8m-r3.0.0-fbd13-a32.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-a32.tar.gz) | G3&nbsp;/&nbsp;PPC740 | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-a32.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-fbd13-a32.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}

{{< expand "NetBSD 9.3" "…" >}}
{{< expand "Intel x86" "…" >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       | Platform                           | Verification |
|:------------------------------------------------------------------------------------------ |:---------------------------------- |:------------ |
| [**dps8m-r3.0.0-nbd93-x64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-x64.tar.gz) | x86_64‑v4 (AVX‑512)                | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-x64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-x64.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-nbd93-x63.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-x63.tar.gz) | x86_64‑v3 (AVX2&nbsp;+&nbsp;FMA)   | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-x63.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-x63.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-nbd93-x62.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-x62.tar.gz) | x86_64‑v2 (SSE4&nbsp;+&nbsp;SSSE3) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-x62.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-x62.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-nbd93-x61.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-x61.tar.gz) | x86_64                             | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-x61.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-x61.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "Intel ix86 (32-bit)" "…" >}}
| File                                                                                       | Platform | Verification |
|:------------------------------------------------------------------------------------------ |:-------- |:------------ |
| [**dps8m-r3.0.0-nbd93-686.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-686.tar.gz) | i686     | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-686.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-686.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-nbd93-586.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-586.tar.gz) | i586     | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-586.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-586.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}

{{< expand "ARM64 (little-endian)" "…" >}}
| File                                                                                       | Platform | Verification |
| :----------------------------------------------------------------------------------------- | :------- |:------------ |
| [**dps8m-r3.0.0-nbd93-a64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-a64.tar.gz) | ARMv8‑A  | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-a64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-a64.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "SPARC" "…" >}}
{{< expand "UltraSPARC" "…" >}}
| File                                                                                       | Verification |
|:------------------------------------------------------------------------------------------ |:------------ |
| [**dps8m-r3.0.0-nbd93-s64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-s64.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-s64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-s64.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "SPARC (32-bit)" "…" >}}
| File                                                                                       | Verification |
|:------------------------------------------------------------------------------------------ |:------------ |
| [**dps8m-r3.0.0-nbd93-s32.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-s32.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-s32.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-s32.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}

{{< expand "PowerPC (32-bit, big-endian)" "…" >}}
| File                                                                                       | Platform   | Verification |
|:------------------------------------------------------------------------------------------ |:---------- |:------------ |
| [**dps8m-r3.0.0-nbd93-970.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-970.tar.gz) | G5&nbsp;/&nbsp;PPC970  | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-970.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-970.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-nbd93-pg4.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-pg4.tar.gz) | G4&nbsp;/&nbsp;PPC7400 | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-pg4.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-pg4.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-nbd93-740.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-740.tar.gz) | G3&nbsp;/&nbsp;PPC740  | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-740.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-740.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-nbd93-603.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-603.tar.gz) | G2&nbsp;/&nbsp;PPC603  | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-603.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-603.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-nbd93-601.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-601.tar.gz) | G1&nbsp;/&nbsp;PPC601  | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-601.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-601.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "Alpha" "…" >}}
| File                                                                                       | Platform | Verification |
|:------------------------------------------------------------------------------------------ |:-------- |:------------ |
| [**dps8m-r3.0.0-nbd93-ax6.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-ax6.tar.gz) | EV6      | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-ax6.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-ax6.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-nbd93-ax5.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-ax5.tar.gz) | EV5      | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-ax5.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-ax5.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-nbd93-axp.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-axp.tar.gz) | EV4      | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-axp.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-axp.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "VAX" "…" >}}
| File                                                                                       | Verification |
|:------------------------------------------------------------------------------------------ |:------------ |
| [**dps8m-r3.0.0-nbd93-vax.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-vax.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-vax.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-nbd93-vax.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}

{{< expand "OpenBSD 7.2" "…" >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       | Platform                           | Verification |
|:------------------------------------------------------------------------------------------ |:---------------------------------- |:------------ |
| [**dps8m-r3.0.0-obd72-x64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-obd72-x64.tar.gz) | x86_64‑v4 (AVX‑512)                | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-obd72-x64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-obd72-x64.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-obd72-x63.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-obd72-x63.tar.gz) | x86_64‑v3 (AVX2&nbsp;+&nbsp;FMA)   | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-obd72-x63.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-obd72-x63.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-obd72-x62.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-obd72-x62.tar.gz) | x86_64‑v2 (SSE4&nbsp;+&nbsp;SSSE3) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-obd72-x62.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-obd72-x62.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-obd72-x61.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-obd72-x61.tar.gz) | x86_64                             | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-obd72-x61.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-obd72-x61.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< expand "ARM64 (little-endian)" "…" >}}
| File                                                                                       | Platform                           | Verification |
|:------------------------------------------------------------------------------------------ |:---------------------------------- |:------------ |
| [**dps8m-r3.0.0-obd72-a64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-obd72-a64.tar.gz) | ARMv8-A                             | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-obd72-a64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-obd72-a64.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}

{{< expand "DragonFly BSD 6.2.2" "…" >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       | Platform                           | Verification |
|:------------------------------------------------------------------------------------------ |:---------------------------------- |:------------ |
| [**dps8m-r3.0.0-df622-644.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-df622-644.tar.gz) | x86_64‑v4 (AVX‑512)                | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-df622-644.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-df622-644.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-df622-643.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-df622-643.tar.gz) | x86_64‑v3 (AVX2&nbsp;+&nbsp;FMA)   | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-df622-643.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-df622-643.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-df622-642.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-df622-642.tar.gz) | x86_64‑v2 (SSE4&nbsp;+&nbsp;SSSE3) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-df622-642.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-df622-642.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-df622-641.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-df622-641.tar.gz) | x86_64                             | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-df622-641.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-df622-641.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}
{{< /tab >}}
{{< tab "Haiku" >}}

{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       | Platform | Verification |
|:------------------------------------------------------------------------------------------ |:-------- |:------------ |
| [**dps8m-r3.0.0-haiku-x64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-haiku-x64.tar.gz) | x86_64   | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-haiku-x64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-haiku-x64.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /tab >}}
{{< tab "Linux" >}}

{{< expand "Intel x86" "…" >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       | Platform                           | Verification |
| :----------------------------------------------------------------------------------------  | :--------------------------------- |:------------ |
| [**dps8m-r3.0.0-linux-644.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-644.tar.gz) | x86_64‑v4 (AVX‑512)                | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-644.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-644.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-643.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-643.tar.gz) | x86_64‑v3 (AVX2&nbsp;+&nbsp;FMA)   | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-643.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-643.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-642.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-642.tar.gz) | x86_64‑v2 (SSE4&nbsp;+&nbsp;SSSE3) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-642.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-642.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-641.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-641.tar.gz) | x86_64                             | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-641.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-641.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "Intel ix86 (32-bit)" "…" >}}
| File                                                                                       | Platform | Verification |
| :----------------------------------------------------------------------------------------- | :------- |:------------ |
| [**dps8m-r3.0.0-linux-686.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-686.tar.gz) | i686     | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-686.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-686.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-586.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-586.tar.gz) | i586     | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-586.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-586.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}

{{< expand "Power ISA" "…" >}}
{{< expand "Power ISA (64-bit, little-endian)" "…" >}}
| File                                                                                       | Platform             | Verification |
| :----------------------------------------------------------------------------------------- | :------------------- |:------------ |
| [**dps8m-r3.0.0-linux-p10.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-p10.tar.gz) | Power10              | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-p10.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-p10.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-att.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-att.tar.gz) | Power10 (AT&nbsp;16) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-att.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-att.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-pw9.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-pw9.tar.gz) | POWER9               | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-pw9.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-pw9.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-atn.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-atn.tar.gz) | POWER9 (AT&nbsp;16)  | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-atn.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-atn.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-oxl.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-oxl.tar.gz) | POWER9 (OXLC&nbsp;17 + AT&nbsp;16) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-oxl.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-oxl.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-pw8.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-pw8.tar.gz) | POWER8               | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-pw8.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-pw8.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-ate.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-ate.tar.gz) | POWER8 (AT&nbsp;16)  | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-ate.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-ate.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-ox8.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-ox8.tar.gz) | POWER8 (OXLC&nbsp;17 + glibc&nbsp;2.36+) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-ox8.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-ox8.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-xlc.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-xlc.tar.gz) | POWER8 (XLC&nbsp;16 + glibc&nbsp;2.36+) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-xlc.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-xlc.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "Power ISA (64-bit, big-endian)" "…" >}}
| File                                                                                       | Platform  | Verification |
| :----------------------------------------------------------------------------------------- | :-------- |:------------ |
| [**dps8m-r3.0.0-linux-bpt.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bpt.tar.gz) | Power10   | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bpt.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bpt.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-bp9.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp9.tar.gz) | POWER9    | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp9.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp9.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-bp8.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp8.tar.gz) | POWER8    | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp8.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp8.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-bp7.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp7.tar.gz) | POWER7    | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp7.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp7.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-bp6.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp6.tar.gz) | POWER6    | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp6.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp6.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-bp5.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp5.tar.gz) | POWER5    | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp5.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp5.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-970.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-970.tar.gz) | G5&nbsp;/&nbsp;PPC970 | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-970.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-970.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-bp4.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp4.tar.gz) | POWER4    | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp4.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp4.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-bp3.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp3.tar.gz) | POWER3    | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp3.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-bp3.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-rs6.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-rs6.tar.gz) | RS64      | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-rs6.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-rs6.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-e65.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-e65.tar.gz) | e6500     | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-e65.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-e65.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "PowerPC (32-bit, big-endian)" "…" >}}
| File                                                                                       | Platform    | Verification |
| :----------------------------------------------------------------------------------------- | :---------- |:------------ |
| [**dps8m-r3.0.0-linux-g4e.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-g4e.tar.gz) | G4e&nbsp;/&nbsp;PPC7450 | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-g4e.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-g4e.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-g4a.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-g4a.tar.gz) | G4&nbsp;/&nbsp;PPC7400  | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-g4a.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-g4a.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-740.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-740.tar.gz) | G3&nbsp;/&nbsp;PPC740   | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-740.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-740.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-603.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-603.tar.gz) | G2&nbsp;/&nbsp;PPC603   | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-603.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-603.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-601.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-601.tar.gz) | G1&nbsp;/&nbsp;PPC601   | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-601.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-601.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "PowerPC (32-bit, little-endian)" "…" >}}
| File                                                                                       | Verification |
| :----------------------------------------------------------------------------------------- |:------------ |
| [**dps8m-r3.0.0-linux-ple.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-ple.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-ple.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-ple.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}

{{< expand "ARM" "…" >}}
{{< expand "ARM64 (little-endian)" "…" >}}
| File                                                                                       | Platform  | Verification |
| :----------------------------------------------------------------------------------------- | :-------- |:------------ |
| [**dps8m-r3.0.0-linux-a8a.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a8a.tar.gz) | ARMv8‑A   | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a8a.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a8a.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-a8r.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a8r.tar.gz) | ARMv8‑R   | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a8r.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a8r.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "ARM64 (big-endian)" "…" >}}
| File                                                                                       | Platform         | Verification |
| :----------------------------------------------------------------------------------------- | :--------------- |:------------ |
| [**dps8m-r3.0.0-linux-a8b.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a8b.tar.gz) | ARMv8‑A&nbsp;BE8 | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a8b.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a8b.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "ARM (32-bit, little-endian)" "…" >}}
| File                                                                                       | Platform | Verification |
| :----------------------------------------------------------------------------------------- | :------- |:------------ |
| [**dps8m-r3.0.0-linux-a7f.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a7f.tar.gz) | ARMv7‑HF | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a7f.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a7f.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-a7s.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a7s.tar.gz) | ARMv7‑A  | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a7s.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a7s.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-a6f.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a6f.tar.gz) | ARMv6+FP | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a6f.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a6f.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-a6s.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a6s.tar.gz) | ARMv6    | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a6s.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a6s.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-a5s.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a5s.tar.gz) | ARMv5‑T  | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a5s.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-a5s.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}

{{< expand "m68k" "…" >}}
| File                                                                                       | Platform | Verification |
| :----------------------------------------------------------------------------------------- | :------- |:------------ |
| [**dps8m-r3.0.0-linux-060.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-060.tar.gz) | 68060    | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-060.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-060.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-040.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-040.tar.gz) | 68040    | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-040.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-040.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-030.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-030.tar.gz) | 68030    | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-030.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-030.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-linux-020.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-020.tar.gz) | 68020    | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-020.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-020.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "SPARC" "…" >}}
{{< expand "UltraSPARC" "…" >}}
| File                                                                                       | Platform         | Verification |
| :----------------------------------------------------------------------------------------- | :--------------- |:------------ |
| [**dps8m-r3.0.0-linux-s64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-s64.tar.gz) | glibc&nbsp;2.12+ | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-s64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-s64.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "SPARC (32-bit)" "…" >}}
| File                                                                                       | Platform         | Verification |
| :----------------------------------------------------------------------------------------- | :--------------- |:------------ |
| [**dps8m-r3.0.0-linux-s32.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-s32.tar.gz) | glibc&nbsp;2.12+ | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-s32.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-s32.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}

{{< expand "LoongArch" "…" >}}
| File                                                                                       | Platform         | Verification |
| :----------------------------------------------------------------------------------------- | :--------------- |:------------ |
| [**dps8m-r3.0.0-linux-l64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-l64.tar.gz) | glibc&nbsp;2.12+ | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-l64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-l64.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "OpenRISC" "…" >}}
| File                                                                                       | Platform             | Verification |
| :----------------------------------------------------------------------------------------- | :------------------- |:------------ |
| [**dps8m-r3.0.0-linux-ork.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-ork.tar.gz) | OpenRISC&nbsp;OR1200 | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-ork.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-ork.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "RISC-V" "…" >}}
| File                                                                                       | Platform | Verification |
| :----------------------------------------------------------------------------------------- | :------- |:------------ |
| [**dps8m-r3.0.0-linux-r64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-r64.tar.gz) | RV64     | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-r64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-r64.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "MIPS" "…" >}}
{{< expand "MIPS64 (little-endian)" "…" >}}
| File                                                                                       | Verification |
| :----------------------------------------------------------------------------------------- |:------------ |
| [**dps8m-r3.0.0-linux-m6l.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-m6l.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-m6l.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-m6l.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "MIPS64 (big-endian)" "…" >}}
| File                                                                                       | Verification |
| :----------------------------------------------------------------------------------------- |:------------ |
| [**dps8m-r3.0.0-linux-m64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-m64.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-m64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-m64.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "MIPS (32-bit, little-endian)" "…" >}}
| File                                                                                       | Verification |
| :----------------------------------------------------------------------------------------- |:------------ |
| [**dps8m-r3.0.0-linux-mpl.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-mpl.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-mpl.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-mpl.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "MIPS (32-bit, big-endian)" "…" >}}
| File                                                                                       | Verification |
| :----------------------------------------------------------------------------------------- |:------------ |
| [**dps8m-r3.0.0-linux-mps.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-mps.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-mps.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-mps.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}

{{< expand "SuperH" "…" >}}
{{< expand "SH-4A (32-bit, little-endian)" "…" >}}
| File                                                                                       | Verification |
| :----------------------------------------------------------------------------------------- |:------------ |
| [**dps8m-r3.0.0-linux-l4a.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-l4a.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-l4a.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-l4a.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< expand "SH-4A (32-bit, big-endian)" "…" >}}
| File                                                                                       | Verification |
| :----------------------------------------------------------------------------------------- |:------------ |
| [**dps8m-r3.0.0-linux-b4a.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-b4a.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-b4a.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-b4a.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}

{{< expand "z/Architecture" "…" >}}
| File                                                                                       | Verification |
| :----------------------------------------------------------------------------------------- |:------------ |
| [**dps8m-r3.0.0-linux-390.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-390.tar.gz) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-390.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-linux-390.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /tab >}}
{{< tab "macOS" >}}

{{< expand "Universal Binary" "…" >}}
| File                                                                                       | Platform          | Verification |
|:------------------------------------------------------------------------------------------ |:----------------- |:------------ |
| [**dps8m-r3.0.0-macos-uni.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-macos-uni.tar.gz) | macOS&nbsp;10.13+ | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-macos-uni.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-macos-uni.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "Intel x86 (AMD64)" "…" >}}
| File                                                                                       | Platform          | Verification |
|:------------------------------------------------------------------------------------------ |:----------------- |:------------ |
| [**dps8m-r3.0.0-mac10-x86.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mac10-x86.tar.gz) | macOS&nbsp;10.13+ | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mac10-x86.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mac10-x86.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-macos-x86.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-macos-x86.tar.gz) | Latest&nbsp;SDK   | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-macos-x86.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-macos-x86.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "Apple Silicon (ARM64)" "…" >}}
| File                                                                                       | Platform          | Verification |
|:------------------------------------------------------------------------------------------ |:----------------- |:------------ |
| [**dps8m-r3.0.0-mac11-a64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mac11-a64.tar.gz) | macOS&nbsp;11.00+ | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mac11-a64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mac11-a64.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /tab >}}
{{< tab "SunOS" >}}

{{< expand "Oracle Solaris 11.4" "…" >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                     | Platform                           | Verification |
| :--------------------------------------------------------------------------------------- |:---------------------------------- |:------------ |
| [**dps8m-r3.0.0-s114-644.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-s114-644.tar.gz) | x86_64‑v4 (AVX‑512)                | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-s114-644.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-s114-644.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-s114-643.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-s114-643.tar.gz) | x86_64‑v3 (AVX2&nbsp;+&nbsp;FMA)   | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-s114-643.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-s114-643.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-s114-642.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-s114-642.tar.gz) | x86_64‑v2 (SSE4&nbsp;+&nbsp;SSSE3) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-s114-642.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-s114-642.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-s114-641.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-s114-641.tar.gz) | x86_64                             | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-s114-641.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-s114-641.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}

{{< expand "illumos OpenIndiana" "…" >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                     | Platform                           | Verification |
| :--------------------------------------------------------------------------------------- |:---------------------------------- |:------------ |
| [**dps8m-r3.0.0-iloi-644.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-iloi-644.tar.gz) | x86_64‑v4 (AVX‑512)                | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-iloi-644.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-iloi-644.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-iloi-643.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-iloi-643.tar.gz) | x86_64‑v3 (AVX2&nbsp;+&nbsp;FMA)   | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-iloi-643.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-iloi-643.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-iloi-642.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-iloi-642.tar.gz) | x86_64‑v2 (SSE4&nbsp;+&nbsp;SSSE3) | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-iloi-642.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-iloi-642.tar.gz.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-iloi-641.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-iloi-641.tar.gz) | x86_64                             | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-iloi-641.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-iloi-641.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}
{{< /tab >}}
{{< tab "Windows" >}}

{{< expand "Intel x86" "…" >}}
{{< expand "Intel x86 Installer" "…" >}}
| File                                                                                 | Platform                | Verification |
|:------------------------------------------------------------------------------------ |:----------------------- |:------------ |
| [**dps8m-r3.0.0-win-setup.exe**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-win-setup.exe) | x86_64&nbsp;+&nbsp;i686 | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-win-setup.exe.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-win-setup.exe.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       | Platform                  | Verification |
|:------------------------------------------------------------------------------------------ |:------------------------- |:------------ |
| [**dps8m-r3.0.0-mingw-x86.zip**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mingw-x86.zip)       | x86_64                    | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mingw-x86.zip.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mingw-x86.zip.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-cygwin-64.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-cygwin-64.tar.gz) | Cygwin&nbsp;/&nbsp;x86_64 | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-cygwin-64.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-cygwin-64.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "Intel ix86 (32-bit)" "…" >}}
| File                                                                                       | Platform                | Verification |
|:------------------------------------------------------------------------------------------ |:----------------------- |:------------ |
| [**dps8m-r3.0.0-mingw-x32.zip**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mingw-x32.zip)       | i686                    | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mingw-x32.zip.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mingw-x32.zip.asc)&nbsp;\]</small>  |
| [**dps8m-r3.0.0-cygwin-32.tar.gz**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-cygwin-32.tar.gz) | Cygwin&nbsp;/&nbsp;i686 | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-cygwin-32.tar.gz.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-cygwin-32.tar.gz.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}

{{< expand "ARM" "…" >}}
{{< expand "ARM64" "…" >}}
| File                                                                                 | Platform | Verification |
|:------------------------------------------------------------------------------------ |:-------- |:------------ |
| [**dps8m-r3.0.0-mingw-a64.zip**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mingw-a64.zip) | ARMv8    | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mingw-a64.zip.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mingw-a64.zip.asc)&nbsp;\]</small>  |
{{< /expand >}}

{{< expand "ARM (32-bit)" "…" >}}
| File                                                                                 | Platform | Verification |
|:------------------------------------------------------------------------------------ |:-------- |:------------ |
| [**dps8m-r3.0.0-mingw-av7.zip**](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mingw-av7.zip) | ARMv7    | <small>\[&nbsp;[SHA256](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mingw-av7.zip.sha256)&nbsp;\|&nbsp;[PGP](https://dps8m.gitlab.io/dps8m-archive/R3.0.0/dps8m-r3.0.0-mingw-av7.zip.asc)&nbsp;\]</small>  |
{{< /expand >}}
{{< /expand >}}
{{< /tab >}}
{{< /tabs >}}

## Bleeding Edge

* Bleeding edge snapshots are automatically generated by [**GitLab CI/CD**](https://gitlab.com/dps8m/dps8m/-/pipelines) from the *git* [`master`](https://gitlab.com/dps8m/dps8m/-/tree/master) branch.
* These bleeding edge snapshots were last updated **{{< buildDate >}}**
* Review the [**release notes**](https://gitlab.com/dps8m/dps8m/-/wikis/DPS8M-R3.0.1-Release-Notes) for a summary of changes since the last release.

{{< tabs "unstablesrctable" >}}
{{< tab "Source" >}}
| File                                                                           | Format               |
|:------------------------------------------------------------------------------ |:-------------------- |
| [**dps8m-git-src.tar.lz**](https://dps8m.gitlab.io/dps8m/dps8m-git-src.tar.lz) | tar&nbsp;+&nbsp;lzip |
{{< expand "Alternate formats" "…" >}}
| File                                                                           | Format               |
|:------------------------------------------------------------------------------ |:-------------------- |
| [**dps8m-git-src.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-src.tar.gz) | tar&nbsp;+&nbsp;gzip |
| [**dps8m-git-src.7z**](https://dps8m.gitlab.io/dps8m/dps8m-git-src.7z)         | 7-Zip (LZMA)         |
| [**dps8m-git-src.zip**](https://dps8m.gitlab.io/dps8m/dps8m-git-src.zip)       | ZIP (DEFLATE)        |
| [**dps8m-git-src.zpaq**](https://dps8m.gitlab.io/dps8m/dps8m-git-src.zpaq)     | ZPAQ7                |
{{< /expand >}}
{{< /tab >}}
{{< tab "AIX" >}}

{{< expand "Power ISA (64-bit)" "…" >}}
| File                                                                                       | Platform                   |
|:------------------------------------------------------------------------------------------ |:-------------------------- |
| [**dps8m-git-aixv7-p10.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-aixv7-p10.tar.gz) | Power10                    |
| [**dps8m-git-aixv7-pw9.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-aixv7-pw9.tar.gz) | POWER9                     |
| [**dps8m-git-aixv7-pw8.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-aixv7-pw8.tar.gz) | POWER8 (GCC&nbsp;12)       |
| [**dps8m-git-aixv7-gcc.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-aixv7-gcc.tar.gz) | POWER8 (GCC&nbsp;11)       |
| [**dps8m-git-aixv7-xlc.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-aixv7-xlc.tar.gz) | POWER8 (XL&nbsp;C&nbsp;16) |
| [**dps8m-git-aixv7-pw7.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-aixv7-pw7.tar.gz) | POWER7                     |
| [**dps8m-git-aixv7-pw6.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-aixv7-pw6.tar.gz) | POWER6                     |
{{< /expand >}}
{{< /tab >}}
{{< tab "Android" >}}

{{< expand "Android 10.0 (API Level 29)" "…" >}}
{{< expand "ARM" "…" >}}
{{< expand "ARM64" "…" >}}
| File                                                                                       |
|:------------------------------------------------------------------------------------------ |
| [**dps8m-git-ndk29-a64.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-ndk29-a64.tar.gz) |
{{< /expand >}}

{{< expand "ARM (32-bit)" "…" >}}
| File                                                                                       |
|:------------------------------------------------------------------------------------------ |
| [**dps8m-git-ndk29-a32.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-ndk29-a32.tar.gz) |
{{< /expand >}}
{{< /expand >}}

{{< expand "Intel x86" "…" >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       |
|:------------------------------------------------------------------------------------------ |
| [**dps8m-git-ndk29-x64.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-ndk29-x64.tar.gz) |
{{< /expand >}}

{{< expand "Intel ix86 (32-bit)" "…" >}}
| File                                                                                       |
|:------------------------------------------------------------------------------------------ |
| [**dps8m-git-ndk29-x32.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-ndk29-x32.tar.gz) |
{{< /expand >}}
{{< /expand >}}
{{< /expand >}}

{{< expand "Android 7.0 (API Level 24)" "…" >}}
{{< expand "ARM" "…" >}}
{{< expand "ARM64" "…" >}}
| File                                                                                       |
|:------------------------------------------------------------------------------------------ |
| [**dps8m-git-ndk24-a64.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-ndk24-a64.tar.gz) |
{{< /expand >}}

{{< expand "ARM (32-bit)" "…" >}}
| File                                                                                       |
|:------------------------------------------------------------------------------------------ |
| [**dps8m-git-ndk24-a32.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-ndk24-a32.tar.gz) |
{{< /expand >}}
{{< /expand >}}

{{< expand "Intel x86" "…" >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       |
|:------------------------------------------------------------------------------------------ |
| [**dps8m-git-ndk24-x64.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-ndk24-x64.tar.gz) |
{{< /expand >}}

{{< expand "Intel ix86 (32-bit)" "…" >}}
| File                                                                                       |
|:------------------------------------------------------------------------------------------ |
| [**dps8m-git-ndk24-x32.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-ndk24-x32.tar.gz) |
{{< /expand >}}
{{< /expand >}}
{{< /expand >}}
{{< /tab >}}
{{< tab "BSD" >}}

{{< expand "FreeBSD 13.1" "…" >}}
{{< expand "Intel x86" "…" >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       | Platform                           |
|:------------------------------------------------------------------------------------------ |:---------------------------------- |
| [**dps8m-git-fbd13-x64.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-fbd13-x64.tar.gz) | x86_64‑v4 (AVX‑512)                |
| [**dps8m-git-fbd13-x63.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-fbd13-x63.tar.gz) | x86_64‑v3 (AVX2&nbsp;+&nbsp;FMA)   |
| [**dps8m-git-fbd13-x62.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-fbd13-x62.tar.gz) | x86_64‑v2 (SSE4&nbsp;+&nbsp;SSSE3) |
| [**dps8m-git-fbd13-x61.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-fbd13-x61.tar.gz) | x86_64                             |
{{< /expand >}}

{{< expand "Intel ix86 (32-bit)" "…" >}}
| File                                                                                       | Platform |
| :----------------------------------------------------------------------------------------- | :------- |
| [**dps8m-git-fbd13-x32.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-fbd13-x32.tar.gz) | i686     |
{{< /expand >}}
{{< /expand >}}

{{< expand "ARM64 (little-endian)" "…" >}}
| File                                                                                       | Platform |
| :----------------------------------------------------------------------------------------- | :------- |
| [**dps8m-git-fbd13-a64.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-fbd13-a64.tar.gz) | ARMv8‑A  |
{{< /expand >}}

{{< expand "PowerPC (32-bit, big-endian)" "…" >}}
| File                                                                                       | Platform   |
| :----------------------------------------------------------------------------------------- | :--------- |
| [**dps8m-git-fbd13-a32.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-fbd13-a32.tar.gz) | G3&nbsp;/&nbsp;PPC740 |
{{< /expand >}}
{{< /expand >}}

{{< expand "NetBSD 9.3" "…" >}}
{{< expand "Intel x86" "…" >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       | Platform                           |
|:------------------------------------------------------------------------------------------ |:---------------------------------- |
| [**dps8m-git-nbd93-x64.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-x64.tar.gz) | x86_64‑v4 (AVX‑512)                |
| [**dps8m-git-nbd93-x63.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-x63.tar.gz) | x86_64‑v3 (AVX2&nbsp;+&nbsp;FMA)   |
| [**dps8m-git-nbd93-x62.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-x62.tar.gz) | x86_64‑v2 (SSE4&nbsp;+&nbsp;SSSE3) |
| [**dps8m-git-nbd93-x61.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-x61.tar.gz) | x86_64                             |
{{< /expand >}}

{{< expand "Intel ix86 (32-bit)" "…" >}}
| File                                                                                       | Platform |
|:------------------------------------------------------------------------------------------ |:-------- |
| [**dps8m-git-nbd93-686.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-686.tar.gz) | i686     |
| [**dps8m-git-nbd93-586.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-586.tar.gz) | i586     |
{{< /expand >}}
{{< /expand >}}

{{< expand "ARM64 (little-endian)" "…" >}}
| File                                                                                       | Platform |
| :----------------------------------------------------------------------------------------- | :------- |
| [**dps8m-git-nbd93-a64.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-a64.tar.gz) | ARMv8‑A  |
{{< /expand >}}

{{< expand "SPARC" "…" >}}
{{< expand "UltraSPARC" "…" >}}
| File                                                                                       |
|:------------------------------------------------------------------------------------------ |
| [**dps8m-git-nbd93-s64.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-s64.tar.gz) |
{{< /expand >}}

{{< expand "SPARC (32-bit)" "…" >}}
| File                                                                                       |
|:------------------------------------------------------------------------------------------ |
| [**dps8m-git-nbd93-s32.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-s32.tar.gz) |
{{< /expand >}}
{{< /expand >}}

{{< expand "PowerPC (32-bit, big-endian)" "…" >}}
| File                                                                                       | Platform   |
|:------------------------------------------------------------------------------------------ |:---------- |
| [**dps8m-git-nbd93-970.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-970.tar.gz) | G5&nbsp;/&nbsp;PPC970  |
| [**dps8m-git-nbd93-pg4.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-pg4.tar.gz) | G4&nbsp;/&nbsp;PPC7400 |
| [**dps8m-git-nbd93-740.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-740.tar.gz) | G3&nbsp;/&nbsp;PPC740  |
| [**dps8m-git-nbd93-603.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-603.tar.gz) | G2&nbsp;/&nbsp;PPC603  |
| [**dps8m-git-nbd93-601.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-601.tar.gz) | G1&nbsp;/&nbsp;PPC601  |
{{< /expand >}}

{{< expand "Alpha" "…" >}}
| File                                                                                       | Platform |
|:------------------------------------------------------------------------------------------ |:-------- |
| [**dps8m-git-nbd93-ax6.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-ax6.tar.gz) | EV6      |
| [**dps8m-git-nbd93-ax5.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-ax5.tar.gz) | EV5      |
| [**dps8m-git-nbd93-axp.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-axp.tar.gz) | EV4      |
{{< /expand >}}

{{< expand "VAX" "…" >}}
| File                                                                                       |
|:------------------------------------------------------------------------------------------ |
| [**dps8m-git-nbd93-vax.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-nbd93-vax.tar.gz) |
{{< /expand >}}
{{< /expand >}}

{{< expand "OpenBSD 7.2" "…" >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       | Platform                           |
|:------------------------------------------------------------------------------------------ |:---------------------------------- |
| [**dps8m-git-obd72-x64.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-obd72-x64.tar.gz) | x86_64‑v4 (AVX‑512)                |
| [**dps8m-git-obd72-x63.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-obd72-x63.tar.gz) | x86_64‑v3 (AVX2&nbsp;+&nbsp;FMA)   |
| [**dps8m-git-obd72-x62.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-obd72-x62.tar.gz) | x86_64‑v2 (SSE4&nbsp;+&nbsp;SSSE3) |
| [**dps8m-git-obd72-x61.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-obd72-x61.tar.gz) | x86_64                             |
{{< /expand >}}
{{< /expand >}}

{{< expand "DragonFly BSD 6.4.0" "…" >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       | Platform                           |
|:------------------------------------------------------------------------------------------ |:---------------------------------- |
| [**dps8m-git-df640-644.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-df640-644.tar.gz) | x86_64‑v4 (AVX‑512)                |
| [**dps8m-git-df640-643.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-df640-643.tar.gz) | x86_64‑v3 (AVX2&nbsp;+&nbsp;FMA)   |
| [**dps8m-git-df640-642.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-df640-642.tar.gz) | x86_64‑v2 (SSE4&nbsp;+&nbsp;SSSE3) |
| [**dps8m-git-df640-641.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-df640-641.tar.gz) | x86_64                             |
{{< /expand >}}
{{< /expand >}}
{{< /tab >}}
{{< tab "Haiku" >}}

{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       | Platform |
|:------------------------------------------------------------------------------------------ |:-------- |
| [**dps8m-git-haiku-x64.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-haiku-x64.tar.gz) | x86_64   |
{{< /expand >}}
{{< /tab >}}
{{< tab "Linux" >}}

{{< expand "Intel x86" "…" >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       | Platform                           |
| :----------------------------------------------------------------------------------------  | :--------------------------------- |
| [**dps8m-git-linux-644.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-644.tar.gz) | x86_64‑v4 (AVX‑512)                |
| [**dps8m-git-linux-643.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-643.tar.gz) | x86_64‑v3 (AVX2&nbsp;+&nbsp;FMA)   |
| [**dps8m-git-linux-642.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-642.tar.gz) | x86_64‑v2 (SSE4&nbsp;+&nbsp;SSSE3) |
| [**dps8m-git-linux-641.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-641.tar.gz) | x86_64                             |
{{< /expand >}}

{{< expand "Intel ix86 (32-bit)" "…" >}}
| File                                                                                       | Platform |
| :----------------------------------------------------------------------------------------- | :------- |
| [**dps8m-git-linux-686.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-686.tar.gz) | i686     |
| [**dps8m-git-linux-586.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-586.tar.gz) | i586     |
{{< /expand >}}
{{< /expand >}}

{{< expand "Power ISA" "…" >}}
{{< expand "Power ISA (64-bit, little-endian)" "…" >}}
| File                                                                                       | Platform             |
| :----------------------------------------------------------------------------------------- | :------------------- |
| [**dps8m-git-linux-p10.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-p10.tar.gz) | Power10              |
| [**dps8m-git-linux-att.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-att.tar.gz) | Power10 (AT&nbsp;16) |
| [**dps8m-git-linux-pw9.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-pw9.tar.gz) | POWER9               |
| [**dps8m-git-linux-atn.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-atn.tar.gz) | POWER9 (AT&nbsp;16)  |
| [**dps8m-git-linux-oxl.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-oxl.tar.gz) | POWER9 (OXLC&nbsp;17 + AT&nbsp;16) |
| [**dps8m-git-linux-pw8.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-pw8.tar.gz) | POWER8               |
| [**dps8m-git-linux-ate.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-ate.tar.gz) | POWER8 (AT&nbsp;16)  |
| [**dps8m-git-linux-ox8.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-ox8.tar.gz) | POWER8 (OXLC&nbsp;17 + glibc&nbsp;2.36+) |
| [**dps8m-git-linux-xlc.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-xlc.tar.gz) | POWER8 (XLC&nbsp;16 + glibc&nbsp;2.36+) |
{{< /expand >}}

{{< expand "Power ISA (64-bit, big-endian)" "…" >}}
| File                                                                                       | Platform  |
| :----------------------------------------------------------------------------------------- | :-------- |
| [**dps8m-git-linux-bpt.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-bpt.tar.gz) | Power10   |
| [**dps8m-git-linux-bp9.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-bp9.tar.gz) | POWER9    |
| [**dps8m-git-linux-bp8.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-bp8.tar.gz) | POWER8    |
| [**dps8m-git-linux-bp7.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-bp7.tar.gz) | POWER7    |
| [**dps8m-git-linux-bp6.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-bp6.tar.gz) | POWER6    |
| [**dps8m-git-linux-bp5.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-bp5.tar.gz) | POWER5    |
| [**dps8m-git-linux-970.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-970.tar.gz) | G5&nbsp;/&nbsp;PPC970 |
| [**dps8m-git-linux-bp4.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-bp4.tar.gz) | POWER4    |
| [**dps8m-git-linux-bp3.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-bp3.tar.gz) | POWER3    |
| [**dps8m-git-linux-rs6.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-rs6.tar.gz) | RS64      |
| [**dps8m-git-linux-e65.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-e65.tar.gz) | e6500     |
{{< /expand >}}

{{< expand "PowerPC (32-bit, big-endian)" "…" >}}
| File                                                                                       | Platform    |
| :----------------------------------------------------------------------------------------- | :---------- |
| [**dps8m-git-linux-g4e.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-g4e.tar.gz) | G4e&nbsp;/&nbsp;PPC7450 |
| [**dps8m-git-linux-g4a.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-g4a.tar.gz) | G4&nbsp;/&nbsp;PPC7400  |
| [**dps8m-git-linux-740.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-740.tar.gz) | G3&nbsp;/&nbsp;PPC740   |
| [**dps8m-git-linux-603.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-603.tar.gz) | G2&nbsp;/&nbsp;PPC603   |
| [**dps8m-git-linux-601.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-601.tar.gz) | G1&nbsp;/&nbsp;PPC601   |
{{< /expand >}}

{{< expand "PowerPC (32-bit, little-endian)" "…" >}}
| File                                                                                       |
| :----------------------------------------------------------------------------------------- |
| [**dps8m-git-linux-ple.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-ple.tar.gz) |
{{< /expand >}}
{{< /expand >}}

{{< expand "ARM" "…" >}}
{{< expand "ARM64 (little-endian)" "…" >}}
| File                                                                                       | Platform  |
| :----------------------------------------------------------------------------------------- | :-------- |
| [**dps8m-git-linux-a8a.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-a8a.tar.gz) | ARMv8‑A   |
| [**dps8m-git-linux-a8r.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-a8r.tar.gz) | ARMv8‑R   |
{{< /expand >}}

{{< expand "ARM64 (big-endian)" "…" >}}
| File                                                                                       | Platform         |
| :----------------------------------------------------------------------------------------- | :--------------- |
| [**dps8m-git-linux-a8b.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-a8b.tar.gz) | ARMv8‑A&nbsp;BE8 |
{{< /expand >}}

{{< expand "ARM (32-bit, little-endian)" "…" >}}
| File                                                                                       | Platform |
| :----------------------------------------------------------------------------------------- | :------- |
| [**dps8m-git-linux-a7f.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-a7f.tar.gz) | ARMv7‑HF |
| [**dps8m-git-linux-a7s.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-a7s.tar.gz) | ARMv7‑A  |
| [**dps8m-git-linux-a6f.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-a6f.tar.gz) | ARMv6+FP |
| [**dps8m-git-linux-a6s.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-a6s.tar.gz) | ARMv6    |
| [**dps8m-git-linux-a5s.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-a5s.tar.gz) | ARMv5‑T  |
{{< /expand >}}
{{< /expand >}}

{{< expand "m68k" "…" >}}
| File                                                                                       | Platform |
| :----------------------------------------------------------------------------------------- | :------- |
| [**dps8m-git-linux-060.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-060.tar.gz) | 68060    |
| [**dps8m-git-linux-040.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-040.tar.gz) | 68040    |
| [**dps8m-git-linux-030.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-030.tar.gz) | 68030    |
| [**dps8m-git-linux-020.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-020.tar.gz) | 68020    |
{{< /expand >}}

{{< expand "SPARC" "…" >}}
{{< expand "UltraSPARC" "…" >}}
| File                                                                                       | Platform         |
| :----------------------------------------------------------------------------------------- | :--------------- |
| [**dps8m-git-linux-s64.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-s64.tar.gz) | glibc&nbsp;2.12+ |
{{< /expand >}}

{{< expand "SPARC (32-bit)" "…" >}}
| File                                                                                       | Platform         |
| :----------------------------------------------------------------------------------------- | :--------------- |
| [**dps8m-git-linux-s32.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-s32.tar.gz) | glibc&nbsp;2.12+ |
{{< /expand >}}
{{< /expand >}}

{{< expand "LoongArch" "…" >}}
| File                                                                                       | Platform         |
| :----------------------------------------------------------------------------------------- | :--------------- |
| [**dps8m-git-linux-l64.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-l64.tar.gz) | glibc&nbsp;2.12+ |
{{< /expand >}}

{{< expand "OpenRISC" "…" >}}
| File                                                                                       | Platform             |
| :----------------------------------------------------------------------------------------- | :------------------- |
| [**dps8m-git-linux-ork.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-ork.tar.gz) | OpenRISC&nbsp;OR1200 |
{{< /expand >}}

{{< expand "RISC-V" "…" >}}
| File                                                                                       | Platform |
| :----------------------------------------------------------------------------------------- | :------- |
| [**dps8m-git-linux-r64.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-r64.tar.gz) | RV64     |
{{< /expand >}}

{{< expand "MIPS" "…" >}}
{{< expand "MIPS64 (little-endian)" "…" >}}
| File                                                                                       |
| :----------------------------------------------------------------------------------------- |
| [**dps8m-git-linux-m6l.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-m6l.tar.gz) |
{{< /expand >}}

{{< expand "MIPS64 (big-endian)" "…" >}}
| File                                                                                       |
| :----------------------------------------------------------------------------------------- |
| [**dps8m-git-linux-m64.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-m64.tar.gz) |
{{< /expand >}}

{{< expand "MIPS (32-bit, little-endian)" "…" >}}
| File                                                                                       | Platform                   |
| :----------------------------------------------------------------------------------------- |:-------------------------- |
| [**dps8m-git-linux-mpl.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-mpl.tar.gz) | MIPS-I&nbsp;(hard-float)   |
| [**dps8m-git-linux-m2l.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-m2l.tar.gz) | MIPS32R2&nbsp;(hard-float) |
<!--| [**dps8m-git-linux-m2s.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-m2s.tar.gz) | MIPS32R2&nbsp;(soft-float) |-->
<!--| [**dps8m-git-linux-msl.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-msl.tar.gz) | MIPS-I&nbsp;(soft-float)   |-->
{{< /expand >}}

{{< expand "MIPS (32-bit, big-endian)" "…" >}}
| File                                                                                       | Platform                   |
| :----------------------------------------------------------------------------------------- |:-------------------------- |
| [**dps8m-git-linux-mps.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-mps.tar.gz) | MIPS-I&nbsp;(hard-float)   |
| [**dps8m-git-linux-mp2.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-mp2.tar.gz) | MIPS32R2&nbsp;(hard-float) |
<!--| [**dps8m-git-linux-ms2.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-ms2.tar.gz) | MIPS32R2&nbsp;(soft-float) |-->
<!--| [**dps8m-git-linux-mss.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-mss.tar.gz) | MIPS-I&nbsp;(soft-float)   |-->
{{< /expand >}}
{{< /expand >}}

{{< expand "SuperH" "…" >}}
{{< expand "SH-4A (32-bit, little-endian)" "…" >}}
| File                                                                                       |
| :----------------------------------------------------------------------------------------- |
| [**dps8m-git-linux-l4a.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-l4a.tar.gz) |
{{< /expand >}}
{{< expand "SH-4A (32-bit, big-endian)" "…" >}}
| File                                                                                       |
| :----------------------------------------------------------------------------------------- |
| [**dps8m-git-linux-b4a.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-b4a.tar.gz) |
{{< /expand >}}
{{< /expand >}}

{{< expand "z/Architecture" "…" >}}
| File                                                                                       |
| :----------------------------------------------------------------------------------------- |
| [**dps8m-git-linux-390.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-linux-390.tar.gz) |
{{< /expand >}}
{{< /tab >}}
{{< tab "macOS" >}}

{{< expand "Universal Binary (x86_64 / x86_64h / ARM64)" "…" >}}
| File                                                                                       | Platform          |
|:------------------------------------------------------------------------------------------ |:----------------- |
| [**dps8m-git-macos-uni.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-macos-uni.tar.gz) | macOS&nbsp;10.13+ |
{{< /expand >}}

{{< /tab >}}
{{< tab "SunOS" >}}

{{< expand "Oracle Solaris 11.4" "…" >}}
{{< expand "UltraSPARC" "…" >}}
| File                                                                                     | Platform                           |
| :--------------------------------------------------------------------------------------- |:---------------------------------- |
| [**dps8m-git-s114-sv9.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-s114-sv9.tar.gz) | UltraSPARC (SPARC&nbsp;V9+)        |
{{< /expand >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                     | Platform                           |
| :--------------------------------------------------------------------------------------- |:---------------------------------- |
| [**dps8m-git-s114-644.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-s114-644.tar.gz) | x86_64‑v4 (AVX‑512)                |
| [**dps8m-git-s114-643.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-s114-643.tar.gz) | x86_64‑v3 (AVX2&nbsp;+&nbsp;FMA)   |
| [**dps8m-git-s114-642.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-s114-642.tar.gz) | x86_64‑v2 (SSE4&nbsp;+&nbsp;SSSE3) |
| [**dps8m-git-s114-641.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-s114-641.tar.gz) | x86_64                             |
{{< /expand >}}
{{< /expand >}}

{{< expand "illumos OpenIndiana" "…" >}}
{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                     | Platform                           |
| :--------------------------------------------------------------------------------------- |:---------------------------------- |
| [**dps8m-git-iloi-644.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-iloi-644.tar.gz) | x86_64‑v4 (AVX‑512)                |
| [**dps8m-git-iloi-643.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-iloi-643.tar.gz) | x86_64‑v3 (AVX2&nbsp;+&nbsp;FMA)   |
| [**dps8m-git-iloi-642.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-iloi-642.tar.gz) | x86_64‑v2 (SSE4&nbsp;+&nbsp;SSSE3) |
| [**dps8m-git-iloi-641.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-iloi-641.tar.gz) | x86_64                             |
{{< /expand >}}
{{< /expand >}}
{{< /tab >}}
{{< tab "Windows" >}}

{{< expand "Intel x86" "…" >}}
{{< expand "Intel x86 Installer" "…" >}}
| File                                                                                 | Platform                |
|:------------------------------------------------------------------------------------ |:----------------------- |
| [**dps8m-git-win-setup.exe**](https://dps8m.gitlab.io/dps8m/dps8m-git-win-setup.exe) | x86_64&nbsp;+&nbsp;i686 |
{{< /expand >}}

{{< expand "Intel x86_64 (AMD64)" "…" >}}
| File                                                                                       | Platform                  |
|:------------------------------------------------------------------------------------------ |:------------------------- |
| [**dps8m-git-mingw-x86.zip**](https://dps8m.gitlab.io/dps8m/dps8m-git-mingw-x86.zip)       | x86_64                    |
| [**dps8m-git-cygwin-64.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-cygwin-64.tar.gz) | Cygwin&nbsp;/&nbsp;x86_64 |
{{< /expand >}}

{{< expand "Intel ix86 (32-bit)" "…" >}}
| File                                                                                       | Platform                |
|:------------------------------------------------------------------------------------------ |:----------------------- |
| [**dps8m-git-mingw-x32.zip**](https://dps8m.gitlab.io/dps8m/dps8m-git-mingw-x32.zip)       | i686                    |
| [**dps8m-git-cygwin-32.tar.gz**](https://dps8m.gitlab.io/dps8m/dps8m-git-cygwin-32.tar.gz) | Cygwin&nbsp;/&nbsp;i686 |
{{< /expand >}}
{{< /expand >}}

{{< expand "ARM" "…" >}}
{{< expand "ARM64" "…" >}}
| File                                                                                 | Platform |
|:------------------------------------------------------------------------------------ |:-------- |
| [**dps8m-git-mingw-a64.zip**](https://dps8m.gitlab.io/dps8m/dps8m-git-mingw-a64.zip) | ARMv8    |
{{< /expand >}}

{{< expand "ARM (32-bit)" "…" >}}
| File                                                                                 | Platform |
|:------------------------------------------------------------------------------------ |:-------- |
| [**dps8m-git-mingw-av7.zip**](https://dps8m.gitlab.io/dps8m/dps8m-git-mingw-av7.zip) | ARMv7    |
{{< /expand >}}
{{< /expand >}}
{{< /tab >}}
{{< /tabs >}}

## Historical Archives

* An [**archive of historical releases is available**](Historical_Archives) for educational and research purposes.
