/* sim_rev.h: simulator revisions and current rev level

   Copyright (c) 1993-2012 Robert M Supnik
   Copyright (c) 2021 The DPS8M Development Team

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Robert M Supnik shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Robert M Supnik.
*/

#ifndef SIM_REV_H_
#define SIM_REV_H_      0

#ifndef SIM_MAJOR
#define SIM_MAJOR       4
#endif
#ifndef SIM_MINOR
#define SIM_MINOR       0
#endif
#ifndef SIM_PATCH
#define SIM_PATCH       0
#endif
#ifndef SIM_DELTA
#define SIM_DELTA       0
#endif

#ifndef SIM_VERSION_MODE
#define SIM_VERSION_MODE "Beta"
#endif

#if defined(SIM_NEED_GIT_COMMIT_ID)
#include ".git-commit-id.h"
#endif

#if !defined(SIM_GIT_COMMIT_ID)
#define SIM_GIT_COMMIT_ID c420925a756861a9548470d23299474e1dcfc84f
#endif

/*
 *  The comment section below reflects the manual editing process which was in place
 *  prior to the use of the git source control system at https://gihub.com/simh/simh
 */

/*
   V3.9 revision history

patch   date            module(s) and fix(es)

  0     01-May-2012     scp.c:
                        - added *nix READLINE support (Mark Pizzolato)
                        - fixed handling of DO with no arguments (Dave Bryan)
                        - fixed "SHOW DEVICE" with only one enabled unit (Dave Bryan)
                        - clarified some help messages (Mark Pizzolato)
                        - added "SHOW SHOW" and "SHOW <dev> SHOW" commands
                              (Mark Pizzolato)
                        - fixed bug in deposit stride for numeric input (John Dundas)

                        sim_console.c
                        - added support for BREAK key on Windows (Mark Pizzolato)

                        sim_ether.c
                        - major revision (Dave Hittner and Mark Pizzolato)
                        - fixed array overrun which caused SEGFAULT on hosts with many
                          devices which libpcap can access.
                        - fixed duplicate MAC address detection to work reliably on
                              switch connected LANs

                        sim_tmxr.c:
                        - made telnet option negotiation more reliable, works with
                              PuTTY as console (Mark Pizzolato)

   V3.8 revision history

  1     08-Feb-09       scp.c:
                        - revised RESTORE unit logic for consistency
                        - "detach_all" ignores error status returns if shutting
                              down (Dave Bryan)
                        - DO cmd missing params now default to null string
                              (Dave Bryan)
                        - DO cmd sub_args now allows "\\" to specify literal
                              backslash (Dave Bryan)
                        - decommitted MTAB_VAL
                        - fixed implementation of MTAB_NC
                        - fixed warnings in help printouts
                        - fixed "SHOW DEVICE" with only one enabled unit (Dave Bryan)

                        sim_tape.c:
                        - fixed signed/unsigned warnings in sim_tape_set_fmt
                              (Dave Bryan)

                        sim_tmxr.c, sim_tmxr.h:
                        - added line connection order to tmxr_poll_conn,
                          added tmxr_set_lnorder and tmxr_show_lnorder (Dave Bryan)
                        - print device and line to which connection was made
                              (Dave Bryan)
                        - added three new standardized SHOW routines

                        all terminal multiplexers:
                        - revised for new common SHOW routines in TMXR library
                        - rewrote set size routines not to use MTAB_VAL

  0     15-Jun-08       scp.c:
                        - fixed bug in local/global register search (Mark Pizzolato)
                        - fixed bug in restore of RO units (Mark Pizzolato)
                        - added SET/SHO/NO BR with default argument (Dave Bryan)

                        sim_tmxr.c
                        - worked around Telnet negotiation problem with QCTerm
                              (Dave Bryan)

   V3.7 revision history

  3     02-Sep-07       scp.c:
                        - fixed bug in SET THROTTLE command

  2     12-Jul-07       sim_ether.c (Dave Hittner):
                        - fixed non-ethernet device removal loop (Naoki Hamada)
                        - added dynamic loading of wpcap.dll;
                        - corrected exceed max index bug in ethX lookup
                        - corrected failure to look up ethernet device names in
                          the registry on Windows XP x64

                        sim_timer.c:
                        - fixed idle timer event selection algorithm

  1     14-May-07       scp.c:
                        - modified sim_instr invocation to call sim_rtcn_init_all
                        - fixed bug in get_sim_opt (reported by Don North)
                        - fixed bug in RESTORE with changed memory size
                        - added global 'RESTORE in progress' flag
                        - fixed breakpoint actions in DO command file processing
                          (Dave Bryan)

  0     30-Jan-07       scp.c:
                        - implemented throttle commands
                        - added -e to control error processing in DO command files
                          (Dave Bryan)

                        sim_console.c:
                        - fixed handling of non-printable characters in KSR mode

                        sim_tape.c:
                        - fixed bug in reverse operations for P7B-format tapes
                        - fixed bug in reverse operations across erase gaps

                        sim_timer.c:
                        - added throttle support
                        - added idle support (based on work by Mark Pizzolato)

   V3.6 revision history

  1     25-Jul-06       sim_console.c:
                        - implemented SET/SHOW PCHAR

  0     15-May-06       scp.c:
                        - revised save file format to save options, unit capacity

                        sim_tape.c, sim_tape.h:
                        - added support for finite reel size
                        - fixed bug in P7B write record

                        most magtapes:
                        - added support for finite reel size

   V3.5 revision history

patch   date            module(s) and fix(es)

  2     07-Jan-06       scp.c:
                        - added breakpoint spaces
                        - added REG_FIT support

                        sim_console.c: added ASCII character processing routines

                        sim_tape.c, sim_tape.h:
                        - added write support for P7B format
                        - fixed bug in write forward (Dave Bryan)

  1     15-Oct-05       All CPU's, other sources: fixed declaration inconsistencies
                        (Sterling Garwood)

  0     1-Sep-05        Note: most source modules have been edited to improve
                        readability and to fix declaration and cast problems in C++

                        all instruction histories: fixed reversed arguments to calloc

                        scp.c: revised to trim trailing spaces on file inputs

                        sim_sock.c: fixed SIGPIPE error on Unix

                        sim_ether.c: added Windows user-defined adapter names
                           (Timothe Litt)

                        sim_tape.c: fixed misallocation of TPC map array

                        sim_tmxr.c: added support for SET <unit> DISCONNECT

                        - added SET PASLn DISCONNECT

   V3.4 revision history

  0     01-May-04       scp.c:
                        - fixed ASSERT code
                        - revised syntax for SET DEBUG (Dave Bryan)
                        - revised interpretation of fprint_sym, fparse_sym returns
                        - moved DETACH sanity tests into detach_unit

                        sim_sock.h and sim_sock.c:
                        - added test for WSAEINPROGRESS (Tim Riker)

   V3.3 revision history

  2     08-Mar-05       scp.c: added ASSERT command (Dave Bryan)

  0     23-Nov-04       scp.c:
                        - added reset_all_p (powerup)
                        - fixed comma-separated SET options (Dave Bryan)
                        - changed ONLINE/OFFLINE to ENABLED/DISABLED (Dave Bryan)
                        - modified to flush device buffers on stop (Dave Bryan)
                        - changed HELP to suppress duplicate command displays

                        sim_console.c:
                        - moved SET/SHOW DEBUG under CONSOLE hierarchy

   V3.2 revision history

  3     03-Sep-04       scp.c:
                        - added ECHO command (Dave Bryan)
                        - qualified RESTORE detach with SIM_SW_REST

                        sim_console: added OS/2 EMX fixes (Holger Veit)

                        sim_sock.h: added missing definition for OS/2 (Holger Veit)

  2     17-Jul-04       scp.c: fixed problem ATTACHing to read only files
                        (John Dundas)

                        sim_console.c: revised Windows console code (Dave Bryan)

                        sim_fio.c: fixed problem in big-endian read
                        (reported by Scott Bailey)

  1     10-Jul-04       scp.c: added SET/SHOW CONSOLE subhierarchy

  0     04-Apr-04       scp.c:
                        - added sim_vm_parse_addr and sim_vm_fprint_addr
                        - added REG_VMAD
                        - moved console logging to SCP
                        - changed sim_fsize to use descriptor rather than name
                        - added global device/unit show modifiers
                        - added device debug support (Dave Hittner)
                        - moved device and unit flags, updated save format

                        sim_ether.c:
                        - further generalizations (Dave Hittner, Mark Pizzolato)

                        sim_tmxr.h, sim_tmxr.c:
                        - added tmxr_linemsg
                        - changed TMXR definition to support variable number of lines

                        sim_libraries:
                        - new console library (sim_console.h, sim_console.c)
                        - new file I/O library (sim_fio.h, sim_fio.c)
                        - new timer library (sim_timer.h, sim_timer.c)

                        all terminal multiplexors: revised for tmxr library changes

   V3.1 revision history

  0     29-Dec-03       sim_defs.h, scp.c: added output stall status

                        all console emulators: added output stall support

                        sim_ether.c (Dave Hittner, Mark Pizzolato, Anders Ahgren):
                        - added Alpha/VMS support
                        - added FreeBSD, Mac OS/X support
                        - added TUN/TAP support
                        - added DECnet duplicate address detection

   V3.0 revision history

  2     15-Sep-03       scp.c:
                        - fixed end-of-file problem in dep, idep
                        - fixed error on trailing spaces in dep, idep

  0     15-Jun-03       scp.c:
                        - added ASSIGN/DEASSIGN
                        - changed RESTORE to detach files
                        - added u5, u6 unit fields
                        - added USE_ADDR64 support
                        - changed some structure fields to unsigned

                        scp_tty.c: added extended file seek

                        sim_sock.c: fixed calling sequence in stubs

                        sim_tape.c:
                        - added E11 and TPC format support
                        - added extended file support

                        sim_tmxr.c: fixed bug in SHOW CONNECTIONS

                        all magtapes:
                        - added multiformat support
                        - added extended file support
*/

#endif
