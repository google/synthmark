[Docs home](README.md)

# How to Run for Android CDD

## IMPORTANT: The Android CDD says to use a specific commit of SynthMark. But there have been important fixes made in SynthMark since that commit. Please use the latest version of SynthMark.

## Running SynthMark as an App

1. Change the device Settings so the Display will stay on for a long time.
1. Build the **latest** SynthMark app using the instructions in [HowToBuild.md](docs/HowToBuild.md).
1. Make sure that "AutomatedSuite" appears in the menu above the "RUN TEST" button.
1. Press the "RUN TEST" button.
1. Wait several minutes. The LatencyMark tests can take a while.

When test test finishes, you should see a special section for CDD. It will look something like this:

    CDD_SUMMARY_BEGIN -------
    voicemark.90 = 67.89
    latencymark.fixed.little = 4
    latencymark.dynamic.little = 10
    CDD_SUMMARY_END ---------
    
If you want to save a copy of the report, then scroll back up and tap the "SHARE RESULT" button.
You can then, for example, email the report to yourself on write it to Drive.

## Running SynthMark as a Command Line Executable

1. Build the latest SynthMark command line executable using the instructions in [HowToBuild.md](docs/HowToBuild.md).
1. Run the executable using the instructions in [HowToRunCommand.md](docs/HowToRunCommand.md).
1. To run 'A'utomated test, enter:

    adb shell synthmark -ta

## Interpreting the Results

**voicemark.90** is a measure of how many synthesizer "voices" can be generated simultaneously. A "voice" is equivalent to one classic subtractive synthesizer tone eu=quivalent to the Mini Moog. It is a measure of CPU performance.

**latencymark.fixed.little** is the latency in milliseconds required for the top level buffer when playing a constant number of voices. It is a measure of CPU scheduling jitter for a steady workload. If this high then there may be drivers that are disabling interrupts.

**latencymark.dynamic.little** is the latency in milliseconds required for the top level buffer when playing a number of voices that goes from low to high every few seconds. This is a measure of how quickly the CPU scheduler can increase the clock frequency in response to a changing workload. If this is high then the CPU scheduler is responding too slowly.

