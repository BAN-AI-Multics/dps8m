<!-- SPDX-License-Identifier: LicenseRef-DPS8M-Doc OR LicenseRef-CF-GAL -->
<!-- SPDX-FileCopyrightText: 2022-2023 The DPS8M Development Team -->
<!-- scspell-id: scspell-id: 8e8d9a46-3233-11ed-968f-80ee73e9b8e7 -->
The `AUTOINPUT` command (abbreviated `AI`) provides the specified input to the
primary operator console (OPC0):

        AUTOINPUT <string>

* To send a **`<CR><LF>`** to the console, include "**`\n`**" in the *string*.
* Specifying "**`\z`**" as the content of the *string* will end all autoinput from the invoking script.
* The `AUTOINPUT` command can open the console for input only when a specific *matching string* is found.
  * To specify a *matching string*, use the form of "**`\yString\y`**" for a substring match, or "**`\xString\x`**" for an exact match.

**Examples**

        ; Opens the console when AUTOINPUT sees the "M->" string.
        ; Any line of text containing this string will match.
        AUTOINPUT \yM->\y

        ; Open the console when AUTOINPUT sees the "Ready" string.
        ; A line of text containing only this exact string will match.
        AUTOINPUT \xReady\x

