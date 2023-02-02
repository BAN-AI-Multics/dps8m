
<!-- SPDX-License-Identifier: LicenseRef-DPS8M-Doc OR LicenseRef-CF-GAL -->
<!-- SPDX-FileCopyrightText: 2022-2023 The DPS8M Development Team -->
<!-- scspell-id: 4132ae42-340e-11ed-83d3-80ee73e9b8e7 -->

The `FNPSERVERPORT` command directs the simulator to listen for FNP (*front-end network processor*) **TELNET** server connections on the specified "**`<port>`**" of the host system.  If the `FNPSERVERPORT` command is not issued prior to FNP bootload, a default "**`<port>`**" of "`6180`" is used.

        FNPSERVERPORT <port>

**Example**

* Listen on port "`6180`" (*the default port*):
        FNPSERVERPORT 6180

See also: `FNPSERVERADDRESS`.

