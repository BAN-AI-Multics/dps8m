
<!-- SPDX-License-Identifier: LicenseRef-CF-GAL -->
<!-- SPDX-FileCopyrightText: 2022-2025 The DPS8M Development Team -->
<!-- scspell-id: 9f846ffc-13ce-11f0-804d-80ee73e9b8e7 -->

<!-- pagebreak -->

# Tips and Tricks

## Protecting DPS8M in low-memory situations

If the simulator is terminated unexpectedly, data loss or corruption could occur.  Users may wish to exempt the **DPS8M** process from being terminated in low memory situations.

* Exempting the simulator from the operating system out-of-memory handler does *not* necessarily ensure stability in out-of-memory situations.  The system could decide to kill other important system processes leaving the machine in a hung or otherwise unusable state.  It is always best to ensure that enough memory is free to avoid triggering the out-of-memory condition in the first place.

### Exempting DPS8M from the Linux OOM Killer

Linux users may wish to exempt the **DPS8M** process from the Linux "**OOM** (***out-of-memory***) **Killer**" to avoid unexpected terminations in low memory situations.

Before initiating a kernel panic due to total memory exhaustion, the Linux *OOM Killer* is activated to inspect all running processes and assigns a score to each one.  It then terminates the process with the highest score, repeating the procedure until enough memory has been freed to avoid a kernel panic.

The actual scoring algorithm is complex and varies greatly depending on the Linux kernel version.  The algorithm may be further tuned by the vendor or distributor.  The score is generally based on a large number of factors including the amount of memory allocated by a process, the length of time a process has been running, the status of overall system memory overcommitment, the user running the process, any cgroups-based configuration constraints, and most importantly, an "`oom_adj`" value.

* There may be additional considerations involved if you are running the simulator in a namespace or container (*e.g.* Docker, Kubernetes, LXC, Virtuozzo/OpenVZ, etc.) or using a Linux distribution with vendor kernel modifications or additional system daemons (*e.g.* Android's '[`lmkd`](https://source.android.com/docs/core/perf/lmkd)', systemd's '[`systemd-oomd`](https://www.freedesktop.org/software/systemd/man/latest/systemd-oomd.service.html)', Facebook's '[`oomd`](https://github.com/facebookincubator/oomd/)', the '[`earlyoom`](https://github.com/rfjakob/earlyoom)' daemon, etc.) that are outside the scope of this documentation.  Consult your Linux distribution documentation for further details.

Users who are running the simulator in production environments should adjust the "`oom_adj`" value so other processes are considered for termination first (or to exempt the simulator completely).  The "`oom_adj`" is one of `32` values ranging from '`14`' through '`-17`', with '`14`' indicating the process should most likely be selected by the *OOM Killer* and '`-16`' indicating the process should most likely *not* be selected by the *OOM Killer*.  The special value of '`-17`' exempts the process from consideration entirely.

> At the time of writing, this functionality is *not* integrated in the simulator itself, because allowing a non-`root` process to *securely* adjust it's own "`oom_adj`" value requires adding a compile-time dependency on the "`libcap`" headers and a run-time dependency on the "`libcap`" library.

* To adjust the "`oom_adj`" value to '`-17`' for all running "`dps8`" processes, use the following shell commands:
  ```dps8
  pgrep "dps8" | while read pid; do
    printf '%s\n' "-17" | sudo tee "/proc/${pid:?}/oom_adj"
  done
  ```

* If running with appropriate privileges (*i.e.* from `root`'s crontab) you can use the following shell commands:
  ```dps8
  pgrep "dps8" | while read pid; do
    printf '%s\n' "-17" > "/proc/${pid:?}/oom_adj"
  done
  ```

#### Automatic Linux `oom_adj` adjustment

If you want to automate the process of adjusting the "`oom_adj`" value, or completely exempt the simulator from consideration by the Linux *OOM Killer* each time the simulator starts, you can do so using "`sudo`" and a "`bash`" script.

1. First, create the file "`/usr/local/sbin/dps8-oom`" containing the "`dps8-oom`" script.  This script is available from "[`https://gitlab.com/dps8m/misc-scripts/-/raw/master/dps8-oom`](https://gitlab.com/dps8m/misc-scripts/-/raw/master/dps8-oom)":
   * Download using '`wget`':
     ```sh
     wget -O - https://gitlab.com/dps8m/misc-scripts/-/raw/master/dps8-oom |\
       sudo tee /usr/local/sbin/dps8-oom > /dev/null
     ```
   * Download using '`curl`':
     ```sh
     curl -fSsL https://gitlab.com/dps8m/misc-scripts/-/raw/master/dps8-oom |\
       sudo tee /usr/local/sbin/dps8-oom > /dev/null
     ```

2. Ensure the script has secure ownership and proper permissions:
   ```sh
   sudo chown root:root /usr/local/sbin/dps8-oom
   sudo chmod 755 /usr/local/sbin/dps8-oom
   ```

3. Next, use the "`sudo visudo`" command to edit the "`sudoers`" file and add the following line:
   ```sh
   username ALL=(ALL) NOPASSWD: /usr/local/sbin/dps8-oom
   ```
   * You should replace "`username`" in the text above with the actual name of the user that runs the simulator.  If you want to allow all users of a particular group to run the script, prefix the group name with the "`%`" symbol (*i.e.* "`%wheel`").

4. Finally, at the top of your simulator script (`INI` file), add the following line:
   ```sh
   ! sudo /usr/local/sbin/dps8-oom
   ```

 * You will see a message similar to the following when the "`dps8-oom`" script is called from the simulator:
   ```sh
   PID 375995 oom_adj value adjusted successfully.
   ```

### Exempting DPS8M from FreeBSD OOM termination

The **FreeBSD** operating system will automatically terminate processes that are not explicitly protected if all memory and swap space is exhausted.  FreeBSD users may wish to protect the **DPS8M** process to avoid unexpected termination of the simulator in low memory situations.

The system decides which processes to terminate based on a large number of factors and heuristics which vary from release to release, but include process priority, memory usage, resource consumption, interactive vs. non-interactive status, and most importantly, the setting of the process '`protect`' flag.

Users running the simulator on FreeBSD in production environments should set the '`protect`' flag for the simulator process using the "[`protect(1)`](https://man.freebsd.org/cgi/man.cgi?query=protect&sektion=1&format=html)" utility.

* To set the '`protect`' flag for all running "`dps8`" processes, use the following shell command (*as `root`*):
  ```sh
  pgrep dps8 | xargs protect -p
  ```
  * For more information on the "`protect(1)`" utility, see the FreeBSD `man` pages (*i.e.* '`man protect`').

#### Automatic FreeBSD OOM protection

If you want to automate the process of setting the '`protect`' flag each time the simulator starts, you can do so using "`sudo`" and a shell script.  These instructions assume your user is member of the "`wheel`" group.

1. First, install and configure the "`sudo`" package (*as `root`*):
   ```sh
   pkg install sudo
   ```
   * Run "`visudo -f /usr/local/etc/sudoers.d/0000-wheel`" (*as `root`*) and add the following line to enable "`sudo`" for the "`wheel`" group:
     ```sh
     %wheel ALL=(ALL) ALL
     ```

2. Create the file "`/usr/local/sbin/dps8-oom`" containing the "`dps8-oom-fbsd`" script.  This script is available from "[`https://gitlab.com/dps8m/misc-scripts/-/raw/master/dps8-oom-fbsd`](https://gitlab.com/dps8m/misc-scripts/-/raw/master/dps8-oom-fbsd)":
   ```sh
   sudo fetch https://gitlab.com/dps8m/misc-scripts/-/raw/master/dps8-oom-fbsd \
     -o /usr/local/sbin/dps8-oom
   ```

3. Ensure the script has secure ownership and proper permissions:
   ```sh
   sudo chown root:wheel /usr/local/sbin/dps8-oom
   sudo chmod 755 /usr/local/sbin/dps8-oom
   ```

4. Run "`sudo visudo -f /usr/local/etc/sudoers.d/9999-dps8-oom`" and add the following line:
   ```sh
   username ALL=(ALL) NOPASSWD: /usr/local/sbin/dps8-oom
   ```
   * You should replace "`username`" in the text above with the actual name of the user that runs the simulator.  If you want to allow all users of a particular group to run the script, prefix the group name with the "`%`" symbol (*i.e.* "`%wheel`").

5. Finally, at the top of your simulator script (`INI` file), add the following line:
   ```sh
   ! sudo /usr/local/sbin/dps8-oom
   ```

* You will see a message similar to the following when the "`dps8-oom`" script is called from the simulator:
  ```sh
  PID 55290 protected successfully.
  ```

***NOTE***: If "`sudo`" asks for a password to run the script (executed via the full absolute path) even after the above configuration you should examine the output of "`sudo -l`" to review your rule ordering, and ensure that the entry for "`dps8-oom`" appears *after* any generic "`(ALL) ALL`" or similar entries.  See the "[`sudo`](https://www.sudo.ws/)" documentation for further details.

