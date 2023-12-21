# Running SynthMark from Command Line

Run the default VoiceMark benchmark.
On Linux:

    ./synthmark.app

On Android, run as root so that SynthMark can request SCHED_FIFO:

    adb root
    adb shell synthmark

If you run with the -h option you will get some “help” information:


    SynthMark version 1.26
    synthmark -t{test} -n{numVoices} -d{noteOnDelay} -p{percentCPU} -r{sampleRate} -s{seconds} -b{burstSize} -c{cpuAffinity}
        -t{test}, v=voice, l=latency, j=jitter, u=utilization, s=series_util, c=clock_ramp, a=automated, default is v
        -a{audioLevel} 0 = normal thread, 1 = audio callback (default), 2 = audio output
        -b{burstSize} frames read by virtual hardware at one time, default = 96
        -B{bursts} initial buffer size in bursts, default = 1
        -c{cpuAffinity} index of CPU to run on, default = UNSPECIFIED
        -d{noteOnDelay} seconds to delay the first NoteOn, default = 0
        -f{enable} use SCHED_FIFO for normal thread, 0 = off, 1 = on (default)
        -n{numVoices} to render, default = 8
        -N{numVoices} to render for toggling high load, only for -t{l|j|c|s}
        -m{voicesMode} algorithm to choose the number of voices in the range
          [-n, -N]. This value can be 'l' for a linear increment, 'r' for a
          random choice, or 's' to switch between -n and -N. default = s
        -p{percentCPU} target load, default = 50
        -r{sampleRate} should be typical, 44100, 48000, etc. default is 48000
        -s{seconds} to run the test, latencyMark may take longer, default is 10
        -u{utilClampLevel} 0 = off (default), 1 = on, 2 = on verbose, >2 = fixed
               Using utilClamp helps the scheduler adapt to dynamic workloads.
        -w{workloadHintsEnabled} 0 = no (default), 1 = give workload hints to scheduler
        -z{enable} use ADPF for performance hints, 0 = off (default), 1 = on

## Running and Interpreting each Test

### VoiceMark

VoiceMark measures CPU performance and the behavior of the CPU governor.

Run VoiceMark with the percent CPU set to 50% for 20 seconds. Then run it again at 80%. Note that the number of voices
may not be linear because of the CPU frequency governor.

    synthmark -tv -s20 -p50
    synthmark -tv -s20 -p80

### JitterMark

JitterMark measures thread scheduling, preemption and the behavior of the CPU governor.

Run the JitterMark with 20 voices, and 96 frame burst size.

    adb shell synthmark -tj -n20 -b96

You can copy and paste the CSV output into a spreadsheet and plot a histogram. In the chart below, for example, we plotted the LOG base 2 of the counts against milliseconds.
Jitter Histogram for SCHED_FIFO Thread on Pixel 7 Pro.

<img width="724" alt="jittermark_histogram" src="https://github.com/google/synthmark/assets/5175913/8e55b31b-7e7e-44db-9aa0-4b80d0cde9e7">

### LatencyMark

LatencyMark measures the output latency on the virtual audio device that is required to avoid glitches.

Run the LatencyMark with 4 voices.

    adb shell synthmark -tl -n4
    
## Performance Suite

These tests are designed to give an overall measure of the real-time performance of the device.

Step #1 - Prepare the device

Ensure the device is stationary and the screen is off (both screen and movement can influence governor behaviour)

Run as root so that SynthMark can request SCHED_FIFO.

    adb root

Step #2 : Measure maximum voices, M

Run VoiceMark90 for 30 seconds (any less than this results in unreliable data caused by varying CPU frequency) to obtain M voices at 90% CPU load

    adb shell synthmark -tv -s30 -p90
    
Look for a report of the number of voices. Here is an example:

VoiceMark_90 = 192.191, normalized to 100% = 213.546

Step #3: JitterMark

The theoretical maximum number of voices in this example is 213. We calculate N = M / 2

    N = 213 / 2 = 106

Now run JitterMark with N.

    adb shell synthmark -tj -n106

Step #4: LatencyMark with light steady Load

In this test we measure the minimum latency required to generate a modest number voices without audible glitches.

    adb shell synthmark -tl -n16

Step #4: LatencyMark with variable load

In this test we increase and decrease the load to see how the CPU governor responds.

    adb shell synthmark -tl -n16 -N64
