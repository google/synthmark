'''
Measure power as a function of frequency.
Generate a CSV report for printing
'''
import os
import subprocess
import sys
import time

simulation = False

durationSecond = 1200
boostSize = 64
measureSecond = durationSecond - 2
measureHz = 5
measureSamples = measureSecond * measureHz
serialNo = 4507
testMode = 'u'
delaySecond = 0

discardSecond = 5

monsoonPath = "/home/rioskao/project/Synthmark/Synthmark/monsoon.py"


def cpuPath(cpuIndex):
    return "/sys/devices/system/cpu/cpu" + cpuIndex + "/cpufreq/scaling_available_frequencies"

'''
@param cpu index of the CPU to query
@return an array of frequencies for the CPU
'''
def getFrequencies(cpuIndex):
    path = cpuPath(cpuIndex)
    if simulation:
        freqtext = subprocess.check_output(["echo", "330000, 480000, 678000"]) # fake
    else:
        freqtext = subprocess.check_output(["adb", "shell", "cat", cpuPath(cpuIndex)]) # real
    frequencyStrings = freqtext.strip().split(' ')
    print frequencyStrings
    # convert text array to integer array"50"
    frequencies = []
    for freqString in frequencyStrings:
        freqInt = int(freqString.strip())
        frequencies.append(freqInt)

    # reverse those freq
    frequencies.sort(reverse = True)
    return frequencies

def collectAverageCurrent():
    if simulation:
        monsoonOutput = "123 4.0\n234 5.0\n345 6.0"
    else:
        measureCommand = ["python", monsoonPath, "--timestamp", "--samples", str(measureSamples), "--hz", str(measureHz), "--serialno", str(serialNo)]
        monsoonOutput = subprocess.check_output(measureCommand)
# TODO args
    monsoonOutput = monsoonOutput.strip()
    #print "monsoon ========================================================================"
    #print monsoonOutput
    #print "monsoon ========================================================================"

    lines = monsoonOutput.splitlines()
    currentSum = 0.0
    currentCount = 0
    for line in lines:
        if currentCount > (discardSecond * measureHz):
            currentText = (line.split(' '))[1].strip()
            current = float(currentText)
            currentSum += current
        currentCount += 1
    #print "total sample counts" + str(currentCount)
    #print "last sample counts we keep" + str((currentCount - discardSecond * measureHz))
    return currentSum / (currentCount - discardSecond * measureHz)

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

    subprocess.call(["adb", "shell", "rm", "/sdcard/result.txt"])

    command = ["adb", "shell", "nohup /system/bin/test_once.sh " + testMode + " " + str(numVoices) + " " + str(delaySecond) + " " + str(durationSecond) + " " + str(cpuIndex) + " " + str(frequency) + " " + str(boostSize) + " " + "&"]

    subprocess.Popen(command)

#wait 1 second to make sure adb command executed.
    time.sleep(1)

# start Monsoon
# stop Monsoon and average the results
# Monsoon will run with usbpassthrough auto mode, usb would disconnect while start monsoon.
    current = collectAverageCurrent()
    print "Result of measurement: Current = ", current

#temprorarily, we just wait 10 second.
#########
#[TODO]need a mechanism to check synthmark has done.
    time.sleep(10)

# gather results from SynthMark, check utilization status
# If Utilization is over 90%, we will skip lower frequecy case.
    synthmarkOutput = subprocess.check_output(["adb", "shell", "cat", "/sdcard/result.txt"])
    listSynthmark = synthmarkOutput.splitlines()
    for line in listSynthmark:
        if ("UtilizationMark = " ) in line:
            print line
            util = float(line.strip('UtilizationMark = '))*100
            if util > 90:
                overload = True
                break
            else:
                overload = False
        else:
            overload = False

    return current, overload;

def measurePowerSeries(cpuIndex, numVoices):
    print "Measure Power series:"
    print "cpuIndex = ", cpuIndex
    print "numVoices = ", numVoices

# turn on USB passthrough
    setupUSBCommand = ["python", monsoonPath, "--usbpassthrough", "auto"]
    subprocess.call(setupUSBCommand)

    frequencies = getFrequencies(cpuIndex)
    currentArray = []

    for frequency in frequencies:
        current, overload = measureCurrent(cpuIndex, numVoices, frequency)
        currentArray.append(current)
        if overload == True:
            break

    index = 0
    print "CSV_BEGIN"
    print "frequency, power"
    for current in currentArray:
        print frequencies[index], ', ', currentArray[index]
        index += 1
    print "CSV_END"

def usage():
    print 'Usage: ', sys.argv[0], ' cpuIndex numVoices'

def main():
    if len(sys.argv) != 3:
          usage()
          sys.exit(1)
    cpuIndex = sys.argv[1]
    numVoices = sys.argv[2]

# before start task, we need to root device fire
    subprocess.call(["adb", "wait-for-device", "root"])
    subprocess.call(["adb", "wait-for-device", "remount"])

    measurePowerSeries(cpuIndex, numVoices)

main()
