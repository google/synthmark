# Introduction to SynthMark

SynthMark is a benchmarking suite for CPU performance and scheduling that models a polyphonic synthesizer.

SynthMark currently includes several benchmarks.

## VoiceMark

This measures the number of voices that will use X% of a single CPU.

## LatencyMark

This measures the size of the smallest buffer that can be used without glitching (underruns).

Relevant parameters are:
* num_voices = base line CPU load in voices
* num_voice_high = alternate higher load for testing CPU governor response to a changing load

Latency can be expressed in frames or the equivalent number of milliseconds. This is
measured using the VirtualAudioSink and is affected by the CPU performance,
thread scheduler, and the behavior of drivers that may be disabling interrupts.
This does not reflect the latency of the platform audio system or hardware.

## JitterMark

This measures the behavior of the thread scheduler and the CPU governor.
It generates CSV data for a histogram. Columns in the histogram include:

* wakeup = how many times the audio task woke up that late
* render = how many times it took that long to calculate the audio data

The wakeup time is affected by the thread scheduler You want to see most wakeups close to zero.
If you see sporadic late wakups then it may be due to preemption from ill behaved drivers
or other high priority tasks

## UtilizationMark

This measures the % CPU load for a fixed number of voices.

Benchmarks do not monitor or control for radios, temperature, other apps, etc.
Such monitoring or controlling is expected to be provided as part of the test conditions, not the tool.
