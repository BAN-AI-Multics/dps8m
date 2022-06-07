#!/usr/bin/env sh
# shellcheck disable=SC2248
# vim: filetype=sh:tabstop=2:tw=100:expandtab

####################################################################################################
#
# Copyright (c) 2021-2022 The DPS8M Development Team
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered "AS-IS",
# without any warranty.
#
####################################################################################################

# Set a CPU to as static a configuration that we can manage; run the benchmark; restore the CPU.
CPU=15

# Save the existing settings
GOV="$(cat /sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_governor 2> /dev/null)"
MAX="$(cat /sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_max_freq 2> /dev/null)"
MIN="$(cat /sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_min_freq 2> /dev/null)"
BOO="$(cat /sys/devices/system/cpu/cpufreq/boost 2>/dev/null)"

# Set the CPU to static
sudo sh -c " \
  printf %\\\\n \"userspace\" > \"/sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_governor\"; \
  printf %\\\\n \"1800000\"   > \"/sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_max_freq\"; \
  printf %\\\\n \"1800000\"   > \"/sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_min_freq\"; \
  printf %\\\\n \"0\" > \"/sys/devices/system/cpu/cpufreq/boost\""

# Run the benchmark
  #taskset --cpu-list "${CPU:?}" ../dps8/dps8 nqueensx.ini
  taskset --cpu-list "${CPU:?}" ../dps8/dps8

# Restore the CPU
sudo sh -c " \
  printf %\\\\n \"${GOV:-}\" > \"/sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_governor\"; \
  printf %\\\\n \"${MAX:-}\" > \"/sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_max_freq\"; \
  printf %\\\\n \"${MIN:-}\" > \"/sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_min_freq\"; \
  printf %\\\\n \"${BOO:-}\" > \"/sys/devices/system/cpu/cpufreq/boost\""
