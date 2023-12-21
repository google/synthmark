# Introduction to SynthMark

SynthMark is a benchmarking suite for CPU performance and scheduling that models a polyphonic synthesizer.

SynthMark currently includes several benchmarks.

## VoiceMark

This measures the number of [voices](Synthesizer.md) that will use X% of a single CPU.

A “voice” is a unit of computation based on a two oscillator
subtractive synthesizer that is common in plugins and sound formats
such as Downloadable Sounds (DLS) and Sound Font (SF2). It is roughly
equivalent to a “Mini Moog” synthesizer.

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

## Clock Ramp

In this test, the worload is suddenly increase, which can the CPU to saturate and miss its real-time deadlines.
This measure how long it takes for the CPU scheduler to raise the frequency enough to meat its deadlines again.

You will need to specify both a Low and High number of voices.
If the CPU does not become saturated then the test is invalid.
In that case, you may need to raise the High number of voices.

## Automated Test

This test combines several benchmarks and then provides a summary of the system performance.

It measures the performance of a BIG and little CPU. It then runs latency analysis for a changing workload.
This measures the ability of the CPU scheduler to run real-time audio workloads
with low latency and without glitching.

It produces three metrics in a section labelled "CDD_SUMMARY".

* **voicemark.90** is the number of voices that can be rendered when running at 90% of its maximum CPU load.
* **latencymark.fixed.little** is the minimum latency of the
top level buffer closest to the app for a constant workload. The DSP
and other lower level components might add 4-6 msec of latency when
using EXCLUSIVE MMAP mode in AAudio. This would give us a total output
latency of around 20 msec.
* **latencymark.dynamic.little** measures latency for a changing workload,
which is common in interactive applications. Higher latency is required
because the scheduler may not increase the clock rate rapidly enough to
meet the changing demand.

(Note that the latency values are for the top level of a virtual audio
device with 2 msec buffers. So SynthMark can achieve lower latency values
than the actual audio framework with multiple levels and a DSP.)


