#!/usr/bin/python2.7
"""
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
"""

'''
Measure power as a function of frequency.
Generate a CSV report for printing
'''
import os
import subprocess
import sys
import time


# Parameters to change
durationSecond = 300 # At least 40 for accuracy
burstSize = 64
cutoffSecond = 15 # How much to wait before we start measuring power after running synth
measureSecond = durationSecond - cutoffSecond - 5
measureHz = 10
measureSamples = measureSecond * measureHz
testMode = 'u'
delaySecond = 0

# Monsoon script should be in current dir
monsoonPath = "./monsoon.py"


def cpuPath(cpuIndex):
    return "/sys/devices/system/cpu/cpu" + cpuIndex + "/cpufreq/scaling_available_frequencies"

'''
@param cpu index of the CPU to query
@return an array of frequencies for the CPU
'''
def getFrequencies(cpuIndex):
    path = cpuPath(cpuIndex)
    frequencyStrings = adbCommand(["cat", cpuPath(cpuIndex)]).strip().split(' ')
    frequencies = [int(l.strip()) for l in frequencyStrings]
    frequencies.sort(reverse = True)
    return frequencies


def collectAverageCurrent():
    time.sleep(cutoffSecond) # Wait for synthmark to settle
    measureCommand = [monsoonPath, "--samples", str(measureSamples), "--hz", str(measureHz)]
    # Muting the stderr of the Monsoon because it is noisy
    with open(os.devnull, 'w') as nullFile:
        monsoonOutput = subprocess.check_output(measureCommand, stderr = nullFile).strip()

    # For debugging purposes
    # print "monsoon ========================================================================"
    # print monsoonOutput
    # print "monsoon ========================================================================"

    lines = monsoonOutput.splitlines()
    currentSamples = [float(i) for i in lines]
    # Calculate mean and var of the current
    n = float(len(currentSamples))
    m = sum(currentSamples) / n
    s = sum((x-m)**2 for x in currentSamples) / n
    return m, s


# Lock the CPU at a freq, run synthmark, then the monsoon to take current measurements
# @param cpuIndex Index to run synthmark on
# @param numVoices Number of voices for Synthmark to stress
# @param frequency The clock to lock the CPU at
# @return current The mean current during the middle of the synthmark test
# @return var The variance of the current across its samples
# @return util The CPU utilization during the test
def measureCurrent(cpuIndex, numVoices, frequency):
    # launch SynthMark by test_once.sh, that is include actions as below
    # 1. hold wakelock
    # 2. set cpu_freq
    # 3. simulate power key
    # 4. exec synthmark
    #######Input arg###########
    # $1 = testMode, $2 = numVoices, $3 = delaySecond
    # $4 = durationSecond, $5 = cpuIndex, $6 = frequency
    # ex: adb shell "nohup /system/bin/test_once.sh u 100 5 15 7 1574400 &" &

    # Make sure the old results are cleaned
    adbCommand(["rm", "-f", "/sdcard/result.txt"])
    command = ["nohup", "/system/bin/test_once.sh", testMode, str(numVoices), str(delaySecond)]
    command = command + [str(durationSecond), str(cpuIndex), str(frequency), str(burstSize), "&> /dev/null &"]
    # print "Running command: ", command

    adbCommand(command)
    current, var = collectAverageCurrent()

    synthmarkOutput = ""
    # Try for 20 seconds to read the results from synthmark
    for i in range(10):
        try:
            synthmarkOutput = adbCommand(["cat", "/sdcard/result.txt"])
            break
        except subprocess.CalledProcessError:
            # print "Sleeping till synthmark finishes"
            time.sleep(2)

    # If we never get it, we will assume we overloaded
    # Gather results from SynthMark, check utilization status
    # If Utilization is over 90%, we will skip lower frequecy case.
    util = 100
    listSynthmark = synthmarkOutput.splitlines()
    for line in listSynthmark:
        if ("UtilizationMark = " ) in line:
            util = float(line.strip('UtilizationMark = '))*100
            break

    return current, var, util


# This function turns passthrough on, runs the provided adb shell command, and
# turns passthrough off.
# @param String arr of commands to run through adb shell
# @return Both the stdout and stderr of the commands run
def adbCommand(stringArgs):
    command = ["adb", "shell"] + stringArgs
    # We mute stderr of the monsoon because it is very noisy
    with open(os.devnull, 'w') as nullFile:
        subprocess.call([monsoonPath, "--usbpassthrough", "on"], stderr = nullFile)
        subprocess.call(["adb", "wait-for-device"])
        result = subprocess.check_output(command, stderr=subprocess.STDOUT)
        subprocess.call([monsoonPath, "--usbpassthrough", "off"], stderr = nullFile)
    return result


def measurePowerSeries(cpuIndex, numVoices):
    print "Measure Power series:", "cpuIndex = ", cpuIndex, "numVoices = ", numVoices

    frequencies = getFrequencies(cpuIndex)
    print "Measuring frequences: ", frequencies
    print "Runs of ", durationSecond, "s at ", measureHz, "hz"
    currentArray = []
    varArray = []
    utilArray = []
    for frequency in frequencies:
        current, var, util = measureCurrent(cpuIndex, numVoices, frequency)
        currentArray.append(current)
        varArray.append(var)
        utilArray.append(util)
        if util > 90:
            break
            # We stop if the cpu util goes above 90 percent for a run
            # This prevents throttling at lower frequencies

    print "CSV_BEGIN"
    print "Frequency, Power, Variance, Utilization"
    for i in range(len(currentArray)):
        print frequencies[i], ', ', currentArray[i], ', ', varArray[i], ', ', utilArray[i]
    print "CSV_END"


def usage():
    print 'Usage: ', sys.argv[0], ' cpuIndex numVoices'


def main():
    if len(sys.argv) != 3:
          usage()
          sys.exit(1)
    cpuIndex = sys.argv[1]
    numVoices = sys.argv[2]

    # before start task, we need to root device (verity should already be
    # disabled) as well as push the local script
    with open(os.devnull, 'w') as nullFile:
      # ADB commands have needless stdout
      subprocess.call([monsoonPath, "--usbpassthrough", "on"], stderr = nullFile)
      subprocess.call(["adb", "wait-for-device", "root"], stdout = nullFile)
      subprocess.call(["adb", "wait-for-device", "remount"], stdout = nullFile)
      # Push the script which runs synthmark locally on the phone to the device
      subprocess.call(["adb", "push", "./test_once.sh", "/system/bin"], stdout = nullFile)
      # Turning off usb just in case
      subprocess.call([monsoonPath, "--usbpassthrough", "off"], stderr = nullFile)

    # Run the main measurements
    measurePowerSeries(cpuIndex, numVoices)
    # Reboot after we finish
    adbCommand(["reboot"])
    subprocess.call([monsoonPath, "--usbpassthrough", "on"])


main()
