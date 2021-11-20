#!/bin/bash

# Set a CPU to as static a configurtaion that we can manage; run the benchmark; restore the cpu.

# Save the existing settings

GOV=`cat /sys/devices/system/cpu/cpu15/cpufreq/scaling_governor`
MAX=`cat /sys/devices/system/cpu/cpu15/cpufreq/scaling_max_freq`
MIN=`cat /sys/devices/system/cpu/cpu15/cpufreq/scaling_min_freq`
BOO=`cat /sys/devices/system/cpu/cpufreq/boost`

# Set the CPU to static

sudo sh -c "\
     echo 'userspace' > /sys/devices/system/cpu/cpu15/cpufreq/scaling_governor; \
     echo "1800000" > /sys/devices/system/cpu/cpu15/cpufreq/scaling_max_freq; \
     echo "1800000" > /sys/devices/system/cpu/cpu15/cpufreq/scaling_min_freq; \
     echo 0 > /sys/devices/system/cpu/cpufreq/boost"

# Run the benchmark

     #taskset --cpu-list 15 ../dps8/dps8 nqueensx.ini
     taskset --cpu-list 15 ../dps8/dps8 

# Restore the CPU

sudo sh -c "\
     echo "$GOV" > /sys/devices/system/cpu/cpu15/cpufreq/scaling_governor; \
     echo "$MAX" > /sys/devices/system/cpu/cpu15/cpufreq/scaling_max_freq; \
     echo "$MIN" > /sys/devices/system/cpu/cpu15/cpufreq/scaling_min_freq; \
     echo "$BOO" > /sys/devices/system/cpu/cpufreq/boost"
