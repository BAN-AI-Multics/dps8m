NOTE: Code signing is a macOS security technology. Once a program has
been signed, the system can detect any changes, whether the change was
introduced accidentally or by malicious code. The code signing process
may be performed by the developer or the end user.

At this time, The DPS8M Development Team does not distribute signed
macOS binaries, however, the stable release builds distributed from
https://dps8m.gitlab.io/ may be authenticated by verifying SHA-256
hashes and by validating their OpenPGP-compatible digital signatures.

Many newer Macintosh systems, including all Apple Silicon-based Macs
require the binaries to be code signed before they can be executed.

From the Apple menu, start "System Settings" or "System Preferences",
then select "Privacy & Security" and ensure "Allow applications from"
is set to "Anywhere". From a Terminal window, use the "codesign"
tool distributed with Xcode or Xcode Command-Line Tools, by running:

  xattr -d com.apple.quarantine dps8; codesign -f -s "-" dps8
  xattr -d com.apple.quarantine tap2raw; codesign -f -s "-" tap2raw
  xattr -d com.apple.quarantine prt2pdf; codesign -f -s "-" prt2pdf
  xattr -d com.apple.quarantine punutil; codesign -f -s "-" punutil

Depending on your system architecture, your version of macOS, and
your system settings, even after signing the binaries, you might
still receive a notice such as:

  "dps8" can't be opened because Apple cannot check it for
  malicious software. This software needs to be updated.
  Contact the developer for more information.

In this case, click on "Show in Finder" button, Control-click the
icon, choose "Open" from the shortcut menu, and then click "Open".
You are now able to run the program without additional restrictions.

It may be necessary to repeat these steps if the simulator files are
copied to a different folder, even if the files have not been modified.
