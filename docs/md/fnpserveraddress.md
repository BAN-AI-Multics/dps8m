
<!-- SPDX-License-Identifier: LicenseRef-CF-GAL -->
<!-- SPDX-FileCopyrightText: 2022-2025 The DPS8M Development Team -->
<!-- scspell-id: 65b8cba8-340c-11ed-9515-80ee73e9b8e7 -->

The `FNPSERVERADDRESS` command directs the simulator to bind the simulated FNP (*front-end network processor*) **TELNET** server to the specified "**`<address>`**" of the host system.  The **TELNET** server answers incoming connections and presents a list of open communication channels to the user.  If the `FNPSERVERADDRESS` command is not issued prior to FNP bootload, a default "**`<address>`**" of "`0.0.0.0`" is used (*i.e.* listening to all addresses).  Only IPv4 addresses may be specified, using quad-dotted decimal notation.

        FNPSERVERADDRESS <address>

**Example**

* Listen on "`127.0.0.1`":
        FNPSERVERADDRESS 127.0.0.1

See also: `FNPSERVERPORT`.

