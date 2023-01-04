/*
 * scp_help.h: hierarchical help definitions
 *
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: X11
 * scspell-id: 9896d74a-f62a-11ec-9179-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2013 Timothe Litt
 * Copyright (c) 2021-2023 The DPS8M Development Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the author shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from the author.
 *
 * ---------------------------------------------------------------------------
 */

#ifndef SCP_HELP_H_
# define SCP_HELP_H_  0

/* The structured help uses help text that defines a hierarchy of information
 * organized into topics and subtopics.
 *
 * The structure of the help text is:
 *
 * Lines beginning with whitespace are displayed as part of the current topic, except:
 *   * The leading white space is replaced by a standard indentation of 4 spaces.
 *     Additional indentation, where appropriate, can be obtained with '+', 4 spaces each.
 *
 *   * The following % escapes are recognized:
 *     * %D    - Inserts the name of the device     (e.g. "DSK").
 *     * %U    - Inserts the name of the unit       (e.g. "DSK0").
 *     * %S    - Inserts the current simulator name (e.g. "DPS-8/M")
 *     * %#s   - Inserts the string supplied in the "#"th optional argument to the help
 *               routine.  # starts with 1.  Any embedded newlines will cause following
 *               text to be indented.
 *     * %#H   - Appends the #th optional argument to the help text.  Use to add common
 *               help to device specific help.  The text is placed AFTER the current help
 *               string, and after any previous %H inclusions.  Parameter numbers restart
 *               with the new string, following the last parameter used by the previous tree.
 *     * %%    - Inserts a literal %.
 *     * %+    - Inserts a literal +.
 *      - Any other escape is reserved and will cause an exception.  However, the goal
 *        is to provide help, not a general formatting facility.  Use sprintf to a
 *        local buffer, and pass that as a string if more general formatting is required.
 *
 * Lines beginning with a number introduce a subtopic of the device.  The number indicates
 * the subtopic's place in the help hierarchy.  Topics offered as Additional Information
 * under the device's main topic are at level 1.  Their sub-topics are at level 2, and
 * so on.  Following the number is a string that names the sub-topic.  This is displayed,
 * and what the user types to access the information.  Whitespace in the topic name is
 * typed as an underscore (_).  Topic names beginning with '$' invoke other kinds of help,
 * These are:
 *    $Registers     - Displays the device register help
 *    $Set commands  - Displays the standard SET command help.
 *    $Show commands - Displays the standard SHOW command help.
 *
 * For these special topics, any text that you provide will be added after
 * the output from the system routines.  This allows you to add more information, or
 * an introduction to subtopics with more detail.
 *
 * Topic names that begin with '?' are conditional topics.
 * Some devices adopt radically different personalities at runtime,
 * e.g. when attached to a processor with different bus.
 * In rare cases, it's better not to include help that doesn't apply.
 * For these cases, ?#, where # is a 1-based parameter number, can be used
 * to selectively include a topic.  If the specified parameter is TRUE
 * (a string with the value "T", "t" or '1'), the topic will be visible.
 * If the parameter is FALSE (NULL, or a string with any other value),
 * the topic will not be visible.
 *
 * If both $ and ? are used, ? comes first.
 *
 * Guidelines:
 *   Help should be concise and easy to understand.
 *
 *   The main topic should be short - less than a sceenful when presented with the
 *   subtopic list.
 *
 *   Keep line lengths to 76 columns or less.
 *
 *   Follow the subtopic naming conventions (under development) for a consistent style:
 *
 *   At the top level, the device should be summarized in a few sentences.
 *   The subtopics for detail should be:
 *     Hardware Description - The details of the hardware.  Feeds & speeds are OK here.
 *          Models          -   If the device was offered in distinct models, a subtopic for each.
 *          Registers       -   Register descriptions
 *
 *     Configuration         - How to configure the device under SimH.  SET commands.
 *          Operating System -   If the device needs special configuration for a particular
 *                               OS, a subtopic for each such OS goes here.
 *          Files            - If the device uses external files (tapes, cards, disks, configuration)
 *                             A subtopic for each here.
 *          Examples         - Provide usable examples for configuring complex devices.
 *
 *     Operation             - How to operate the device under SimH.  Attach, runtime events
 *                             (e.g. how to load cards or mount a tape)
 *
 *     Monitoring            - How to obtain status (SHOW commands)
 *
 *     Restrictions          - If some aspects of the device aren't emulated, list them here.
 *
 *     Debugging             - Debugging information
 *
 *     Related Devices       - If devices are configured or used together, list the other devices here.
 *                             E.G. The DEC KMC/DUP are two hardware devices that are closely related;
 *                             The KMC controlls the DUP on behalf of the OS.
 *
 * API:
 *  To make use of this type of help in your device, create (or replace) a help routine with one that
 *   calls scp_help.  Most of the arguments are the same as those of the device help routine.
 *
 *  t_stat scp_help (FILE *st, DEVICE *dptr,
 *                   UNIT *uptr, int flag, const char *help, char *cptr, ...)
 *
 *  If you need to pass the variable argument list from another routine, use:
 *
 *  t_stat scp_vhelp (FILE *st, DEVICE *dptr,
 *                    UNIT *uptr, int flag, const char *help, char *cptr, va_list ap)
 */

# define T(level, text) #level " " #text "\n"
# define L(text) " " #text "\n"

#endif
