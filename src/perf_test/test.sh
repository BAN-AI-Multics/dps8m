#!/usr/bin/env sh
# shellcheck disable=SC2016,SC2086,SC2248
# vim: filetype=sh:tabstop=2:ai:expandtab
# SPDX-License-Identifier: MIT
# scspell-id: 428ea8be-f631-11ec-9caa-80ee73e9b8e7
# Copyright (c) 2021-2023 The DPS8M Development Team

####################################################################################################
# Linux-only: Set-up a static CPU configuration, run a benchmark, and restore the original settings.
####################################################################################################

${PRINTF:-printf} '%s\n' "* Script started."

####################################################################################################

sudo sh -c "true 2> /dev/null"

####################################################################################################

${TEST:-test} -f "../dps8/dps8" 2> /dev/null ||
  {
    ${PRINTF:-printf} '%s\n' "  * ERROR: ../dps8/dps8 not found!"
    exit 1
  }

####################################################################################################

# Find a CPU

CPU="$(                                                                                            \
         {                                                                                         \
           ${LSCPU:-lscpu} -b --extended 2> /dev/null |                                            \
           ${GREP:-grep} -v '^CPU ' 2> /dev/null |                                                 \
           ${SORT:-sort} -rn 2> /dev/null |                                                        \
           ${HEAD:-head} -n 1 2> /dev/null |                                                       \
           ${AWK:-awk} '/[0-9]/ { print $4 }' ;                                                    \
         } || ${PRINTF:-printf} '%s\n' '0'                                                         \
      )" 2> /dev/null

${PRINTF:-printf} '\n%s\n' "  * Using CPU #${CPU:?} for benchmark"

####################################################################################################

# Save the existing settings

GOV="$(${CAT:-cat} /sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_governor  2> /dev/null)"

${PRINTF:-printf} '%s\n' "    * CPU #${CPU:?} governor : ${GOV:-unset}"

MAX="$(${CAT:-cat} /sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_max_freq  2> /dev/null)"

${PRINTF:-printf} '%s\n' "    * CPU #${CPU:?} max_freq : ${MAX:-unset}"

MIN="$(${CAT:-cat} /sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_min_freq  2> /dev/null)"

${PRINTF:-printf} '%s\n' "    * CPU #${CPU:?} min_freq : ${MIN:-unset}"

BOO="$(${CAT:-cat} /sys/devices/system/cpu/cpufreq/boost                         2> /dev/null)"

${PRINTF:-printf} '%s\n' "    * CPU #${CPU:?} boost    : ${BOO:-unset}"

####################################################################################################

# Set the CPU to static

${PRINTF:-printf} '\n%s\n' "  * Reconfiguring CPU #${CPU:?}"

USEGOV="userspace"

${PRINTF:-printf} '%s\n' "    * Setting CPU #${CPU:?} governor  : ${USEGOV:?}"

CPUFRE="1800000"

${PRINTF:-printf} '%s\n' "    * Setting CPU #${CPU:?} frequency : ${CPUFRE:?}"

${SUDO:-sudo} sh -c "                                                                              \
 printf %s\\\n \"${USEGOV:?}\" > \"/sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_governor\"  \
   2> /dev/null ;                                                                                  \
 printf %s\\\n \"${CPUFRE:?}\" > \"/sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_max_freq\"  \
   2> /dev/null ;                                                                                  \
 printf %s\\\n \"${CPUFRE:?}\" > \"/sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_min_freq\"  \
   2> /dev/null ;                                                                                  \
 printf %s\\\n \"0\"           > \"/sys/devices/system/cpu/cpufreq/boost\" 2> /dev/null"           \
   2> /dev/null

####################################################################################################

# Clean-up

rm -f dps8m.state  > /dev/null 2>&1 || true
rm -f .dps8m.state > /dev/null 2>&1 || true

####################################################################################################

# Run the benchmark

BENCHMARK="nqueensx.ini"

${PRINTF:-printf} '\n%s\n' "  * Running benchmark on CPU #${CPU:?}"

${TASKSET:-taskset} --cpu-list "${CPU:?}" "../dps8/dps8" "${BENCHMARK:-}"

####################################################################################################

# Restore the CPU

${PRINTF:-printf} '\n%s\n' "  * Restoring original CPU configuration"

${SUDO:-sudo} sh -c "                                                                              \
  printf %s\\\n \"${GOV:-}\" > \"/sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_governor\"    \
    2> /dev/null ;                                                                                 \
  printf %s\\\n \"${MAX:-}\" > \"/sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_max_freq\"    \
    2> /dev/null ;                                                                                 \
  printf %s\\\n \"${MIN:-}\" > \"/sys/devices/system/cpu/cpu${CPU:?}/cpufreq/scaling_min_freq\"    \
    2> /dev/null ;                                                                                 \
  printf %s\\\n \"${BOO:-}\" > \"/sys/devices/system/cpu/cpufreq/boost\" 2> /dev/null"             \
    2> /dev/null

####################################################################################################

${PRINTF:-printf} '\n%s\n\n' "* Script complete."

####################################################################################################
