# How to Measure Frequency vs Current

The "autopower.py" script measures the current used by an Android device
while running SynthMark.

### Requirements
* phone with battery replacement cables
* Monsoon power monitor device to measure the current
* The `monsoon.py` script requires `absl-py` and `pyserial`

### Setup
Plug the battery replacement leads into the correct power supply ports of the
monsoon. Then, plug the phone into the USB-A port on the front of the device,
and connect the USB B port on the front to the system running the script. This
will allow the system running the script to control the device over USB via adb,
while passing through the power monitor will prevent power leakage from the USB
port to retain accurate readings. Connect the power at the back of the monsoon
to a wall outlet, and the usb on the back also to the system running the script.
This usb connection is responsible for collecting data from the Monsoon itself.

### Power Parameters
In order to start pixel phones, it needs to be supplied with 4.3V from the Monsoon
(use the `voltage` flag). Make sure to check the appropriate voltage supply for the
device which you are measuring. The phone will draw as much current as required, however
set the `maxcurrent` and `current` to 8. In order for the phone to start it will
probably need to be connected via USB to an AC power source, but can be disconnected
after boot. Make sure the phone has usb debugging turned on with, root access and
verity disabled. If using USB, be sure to turn off passthrough with the `usbpassthrough`
flag prior to taking measurements. The `autopower.py` script will manage this
for you.

### Description
The script will run synthmark with the specified number of voices on the
specified cpu while using the monsoon.py script in this directory to measure
currents. Parameters for monsoon can be found at the beginning of autoscript
(e.g. test duration, polling frequency, burst size etc.). Autopower will then
run Synthmark and measure current on all of the available frequencies of the
specified CPU until the CPU utilization hits 90 percent (the frequency cutoff
varies based on the processor and number of voices used). A CSV of the frequency
and output, in addition to the Utilization amounts are output by the script,
shown below.

### Sample Command

sample execute command and result as below:

       autopower.py [#cpu_index][#number of voice]

terminal command :

    ./autopower.py 1 10

output :

```
Measure Power series:
cpuIndex =  1
numVoices =  10
['300000', '364800', '441600', '518400', '595200', '672000', '748800', '825600', '883200',
 '960000', '1036800', '1094400', '1171200', '1248000', '1324800', '1401600', '1478400',
 '1555200', '1670400', '1747200', '1824000', '1900800']
Running commad:
synthmark -tu -n5 -d0 -s1200 -c1 -b64
CPU Freq: 1900800
...
...
...
frequency, power
1900800 ,  0.0359895671417
1824000 ,  0.0350909232188
1747200 ,  0.0329979046102
1670400 ,  0.0327739143336
1555200 ,  0.0307380068734
1478400 ,  0.0302207765298
1401600 ,  0.0296904155909
1324800 ,  0.0290603939648
1248000 ,  0.0286247076278
1171200 ,  0.0279141186924
1094400 ,  0.0275313983236
1036800 ,  0.0271547088013
960000 ,  0.0266884647108
883200 ,  0.0261830462699
825600 ,  0.026064635373
748800 ,  0.0259630794635
672000 ,  0.0255719751886
595200 ,  0.0256979027661
518400 ,  0.0251472430847
441600 ,  0.0262972288349
364800 ,  0.0262504838223
300000 ,  0.025920926404
CSV_END
```
