/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: c48941bf-f62e-11ec-b993-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2007-2013 Michael Mondy
 * Copyright (c) 2012-2016 Harry Reed
 * Copyright (c) 2013-2016 Charles Anthony
 * Copyright (c) 2016 Michal Tomek
 * Copyright (c) 2021-2023 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
 *
 * ---------------------------------------------------------------------------
 */

unsigned char PROM[1024];
memset (PROM, 255, sizeof (PROM));
sprintf ((char *) PROM, \
"%11s%11u%6s%32s%1s%19s%3s%3s%3s%3s%8s%1s%26s%2s%20s%20s",
  "DPS 8/EM   ",          //    0-10  CPU model          ("XXXXXXXXXXX"/%11s)
#ifdef DPS8_SCP
  0,
#else
  cpu.switches.serno,     //   11-21  CPU serial         ("DDDDDDDDDDD"/%11d)
#endif
#ifdef VER_H_PROM_SHIP
  VER_H_PROM_SHIP,        //   22-27  CPU ship date            ("YYMMDD"/%6s)
#else
  "200101",               //   22-27  CPU ship date            ("YYMMDD"/%6s)
#endif /* VER_H_PROM_SHIP */
  "\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377",
                          //   28-59                                     (%32s)
  "1",                    //      60  layout_version number           ("N"/%1s)
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377",
                          //   61-79                                     (%19s)
#ifdef VER_H_PROM_MAJOR_VER
  VER_H_PROM_MAJOR_VER,   //   80-82  major release number          ("NNN"/%3s)
#else
  "999",                  //   80-82  major release number          ("NNN"/%3s)
#endif /* VER_H_PROM_MAJOR_VER */
#ifdef VER_H_PROM_MINOR_VER
  VER_H_PROM_MINOR_VER,   //   83-85  minor release number          ("NNN"/%3s)
#else
  "999",                  //   83-85  minor release number          ("NNN"/%3s)
#endif /* VER_H_PROM_MINOR_VER */
#ifdef VER_H_PROM_PATCH_VER
  VER_H_PROM_PATCH_VER,   //   86-88  patch version number          ("NNN"/%3s)
#else
  "999",                  //   86-88  patch version number          ("NNN"/%3s)
#endif /* VER_H_PROM_PATCH_VER */
#ifdef VER_H_PROM_OTHER_VER
  VER_H_PROM_OTHER_VER,   //   89-91  iteration number              ("NNN"/%3s)
#else
  "999",                  //   89-91  iteration number              ("NNN"/%3s)
#endif /* VER_H_PROM_OTHER_VER */
  "\377\377\377\377\377\377\377\377",
                          //   98-99                                      (%8s)
#ifdef VER_H_GIT_RELT
  VER_H_GIT_RELT,         //     100  rel type                        ("X"/%1s)
#else
  "X",                    //     100  rel type                        ("X"/%1s)
#endif /* VER_H_GIT_RELT */
#ifdef VER_H_PROM_VER_TEXT
  VER_H_PROM_VER_TEXT,    // 101-127  rel   ("XXXXXXXXXXXXXXXXXXXXXXXXXX"/%26s)
#else
  "Unknown                   ", // 101-127  ("XXXXXXXXXXXXXXXXXXXXXXXXXX"/%26s)
#endif /* VER_H_PROM_VER_TEXT */
  "\377\377",             // 128-129                                      (%2s)
#ifdef BUILD_PROM_OSA_TEXT
  BUILD_PROM_OSA_TEXT,    // 130-149   build arch ("XXXXXXXXXXXXXXXXXXXX"/%20s)
#else
# ifdef VER_H_PROM_OSA_TEXT
  VER_H_PROM_OSA_TEXT,    // 130-149   build arch ("XXXXXXXXXXXXXXXXXXXX"/%20s)
# else
  "Unknown             ", // 130-149   build arch ("XXXXXXXXXXXXXXXXXXXX"/%20s)
# endif /* VER_H_PROM_OSA_TEXT */
#endif /* BUILD_PROM_OSA_TEXT */
#ifdef BUILD_PROM_OSV_TEXT
  BUILD_PROM_OSV_TEXT     // 150-169   build os   ("XXXXXXXXXXXXXXXXXXXX"/%20s)
#else
# ifdef VER_H_PROM_OSV_TEXT
  VER_H_PROM_OSV_TEXT     // 150-169   build os   ("XXXXXXXXXXXXXXXXXXXX"/%20s)
# else
  "Unknown             "  // 150-169   build os   ("XXXXXXXXXXXXXXXXXXXX"/%20s)
# endif /* VER_H_PROM_OSV_TEXT */
#endif /* BUILD_PROM_OSV_TEXT */
                          //     170  ------ sprintf adds trailing NULL ------
  );
for (int iz = 0; iz < 1024; ++iz) {
    if (PROM[iz] == 255) {
        PROM[iz] = '\0';
    }
}
