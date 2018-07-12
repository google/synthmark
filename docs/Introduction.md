# Introduction to SynthMark

SynthMark is a benchmarking suite for CPU performance and scheduling that models a polyphonic synthesizer.

SynthMark currently includes several benchmarks.

* VoiceMarkX is the number of voices that will use X% of a single CPU.
* LatencyMark is the size of the smallest buffer that can be used, while generating
N standard voices, and have NO application level underruns for some period of time.
It can be expressed in frames or the equivalent number of milliseconds. This is
measured using the VirtualAudioSink and is affected by the CPU performance,
thread scheduler, and the behavior of drivers that may be disabling interrupts.
This does not reflect the latency of the platform audio system or hardware.
* JitterMark is a reflection of the reliability of the thread wakeup time and the CPU governor.
It generates CSV data for a histogram.
* UtilizationMark is a measurement of the % CPU load for a fixed number of voices.

Benchmarks do not monitor or control for radios, temperature, other apps, etc.
Such monitoring or controlling is expected to be provided as part of the test conditions, not the tool.
