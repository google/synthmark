#!/system/bin/sh

# This script will run in device shell.
# It should run in device background.
#
# Input parameter
# $1 = mode, $2 = num of voice, $3 = delayed time
# $4 = exec_time, $5 = cpu index, $6 = cpu frequency
# 7 = boost mode
#
# default path for result is sdcard/
# name is v$2_c$5_f$6_${postfix}.txt. If file existed, will add postfix.


# [todo] checking each value is valid or not, currenttly just run it

mode="$1"
voice_num="$2"
delayed_time="$3"
exec_time="$4"
cpu_index="$5"
cpu_freq="$6"
burst_size="$7"
postfix=1
file_path="/sdcard/"
file_name="result.txt"

#echo "Running command:"
#echo synthmark -t${mode} -n${voice_num} -d${delayed_time} -s${exec_time} -c${cpu_index} -b${burst_size}
#echo "CPU Freq: ${cpu_freq}"

input keyevent 26

echo 123 > "/sys/power/wake_lock"

cpupath="/sys/devices/system/cpu/cpu${cpu_index}/cpufreq"

stop performanced
stop vendor.perfd

echo "performance" >> ${cpupath}/scaling_governor
echo ${cpu_freq} >> ${cpupath}/scaling_min_freq
echo ${cpu_freq} >> ${cpupath}/scaling_max_freq
echo ${cpu_freq} >> ${cpupath}/scaling_setspeed

synthmark -t${mode} -n${voice_num} -d${delayed_time} -s${exec_time} -c${cpu_index} -b${burst_size} > /sdcard/tmp 2>&1

mv /sdcard/tmp ${file_path}${file_name}

###########################################
# test part.
###########################################

echo 123 > "/sys/power/wake_unlock"


