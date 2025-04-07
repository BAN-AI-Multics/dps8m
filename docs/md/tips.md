
<!-- SPDX-License-Identifier: LicenseRef-CF-GAL -->
<!-- SPDX-FileCopyrightText: 2022-2025 The DPS8M Development Team -->
<!-- scspell-id: 9f846ffc-13ce-11f0-804d-80ee73e9b8e7 -->

<!-- pagebreak -->

# Tips and Tricks

## Exempting DPS8M from the Linux OOM Killer

If the simulator is terminated unexpectedly, data loss or corruption could occur.  Linux users may wish to exempt the **DPS8M** process from the Linux "**OOM** (***out-of-memory***) **Killer**" to avoid unexpected terminations in low memory situations.

Before initiating a kernel panic due to total memory exhaustion, the Linux *OOM Killer* is activated to inspect all running processes and assigns a score to each one.  It then terminates the process with the highest score, repeating the procedure until enough memory has been freed to avoid a kernel panic.

The actual scoring algorithm is complex and varies greatly depending on the Linux kernel version.  The algorithm may be further tuned by the vendor or distributor.  The score is generally based on a large number of factors including the amount of memory allocated by a process, the length of time a process has been running, the status of overall system memory overcommitment, the user running the process, any cgroups-based configuration constraints, and most importantly, an "`oom_adj`" value.

* There may be additional considerations involved if you are running the simulator in a namespace or container (*e.g.* Docker, Kubernetes, LXC, Virtuozzo/OpenVZ, etc.) or using a Linux distribution with vendor kernel modifications or additional system daemons (*e.g.* Android's '[`lmkd`](https://source.android.com/docs/core/perf/lmkd)', systemd's '[`systemd-oomd`](https://www.freedesktop.org/software/systemd/man/latest/systemd-oomd.service.html)', Facebook's '[`oomd`](https://github.com/facebookincubator/oomd/)', the '[`earlyoom`](https://github.com/rfjakob/earlyoom)' daemon, etc.) that are outside the scope of this documentation.  Consult your Linux distribution documentation for further details.

* Exempting the simulator from the *OOM Killer* does not necessarily ensure stability in out-of-memory situations.  The *OOM Killer* could decide to kill another important system process leaving the machine in a hung or otherwise unusable state.  It is always best to ensure that enough memory is free to avoid triggering the *OOM Killer* in the first place.

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

### Automatic `oom_adj` adjustment

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

3. Next, use the "`sudo visudo`" command to edit the "`sudoers`" file and add the following lines:
   ```sh
   username ALL=(ALL) NOPASSWD: /usr/local/sbin/dps8-oom
   ```
   * You should replace "`username`" in the text above with the actual name of the user that runs the simulator.  If you want to allow all users of a particular group to run the script, prefix the group name with the "`%`" symbol (*i.e.* "`%wheel`").

4. Finally, at the top of your simulator script (`INI` file), add the following line:
   ```sh
   ! /usr/local/sbin/dps8-oom
   ```

 * You will see a message similar to the following when the "`dps8-oom`" script is called from the simulator:
   ```sh
   PID 375995 oom_adj value adjusted successfully.
   ```

