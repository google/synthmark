[Docs home](docs/README.md)

# How to Run for Android CDD

1. Change the device Settings so the Display will stay on for a long time.
1. Build the latest SynthMark app using the instructions in [HowToBuild.md](docs/HowToBuild.md).
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
