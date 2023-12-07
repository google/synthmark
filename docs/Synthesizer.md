# SynthMark Synthesizer Architecture

The audio workload in SynthMark needs to be familiar and typical of an actual synthesizer.
So we based the architecture on a common "subtractive synthesis" voice architecture used in many analog syntheizers.
It contains the following modules:

* 1 band-limited sawtooth oscillator
* 1 band-limited squarewave oscillator
* 1 sinewave LFO for vibrato
* 1 biquad resonant low-pass filter
* 2 ADSR envelopes for filter cutoff and voice amplitude

The code that runs one voice is in
"[synth/SimpleVoice.h](https://github.com/google/synthmark/blob/master/source/synth/SimpleVoice.h)".
