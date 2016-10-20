/*
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
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "SynthMark.h"
#include "tools/TimingAnalyzer.h"
#include "synth/Synthesizer.h"
#include "tools/VirtualAudioSink.h"
#include "tools/VoiceMarkHarness.h"
#include "tools/LatencyMarkHarness.h"
#include "tools/JitterMarkHarness.h"
#include "synth/IncludeMeOnce.h"


#define DEFAULT_TEST_CODE          'v'
#define DEFAULT_SECONDS            10
#define DEFAULT_FRAMES_PER_BURST   SYNTHMARK_FRAMES_PER_BURST
#define DEFAULT_NUM_VOICES         8
#define DEFAULT_NOTE_ON_DELAY      0
#define DEFAULT_PERCENT_CPU        ((int)(100 * SYNTHMARK_TARGET_CPU_LOAD))

void usage(const char *name) {
    printf("%s -t{test} -n{numVoices} -d{noteOnDelay} -p{percentCPU} -r{sampleRate}"
           " -s{seconds} -b{burstSize} -f\n", name);
    printf("    -t{test}, v=voiceMark, l=latencyMark, j=jitterMark, default is %c\n",
           DEFAULT_TEST_CODE);
    printf("    -n{numVoices} to render, default = %d\n", DEFAULT_NUM_VOICES);
    printf("    -d{noteOnDelay} seconds to delay the first NoteOn, default = %d\n",
           DEFAULT_NOTE_ON_DELAY);
    printf("    -p{percentCPU} target load, default = %d\n", DEFAULT_PERCENT_CPU);
    printf("    -r{sampleRate} should be typical, 44100, 48000, etc. default is %d\n",
           SYNTHMARK_SAMPLE_RATE);
    printf("    -s{seconds} to run the test, latencyMark may take longer, default is %d\n",
           DEFAULT_SECONDS);
    printf("    -b{burstSize} frames read by virtual hardware at one time , default = %d\n",
            DEFAULT_FRAMES_PER_BURST);
    printf("    -f to use SCHED_FIFO - IMPORTANT for latency and jitter tests\n");
}

int main(int argc, char **argv)
{
    int32_t percentCpu = DEFAULT_PERCENT_CPU;
    int32_t sampleRate = SYNTHMARK_SAMPLE_RATE;
    int32_t framesPerBurst = DEFAULT_FRAMES_PER_BURST;
    int32_t numSeconds = DEFAULT_SECONDS;
    int32_t numVoices = DEFAULT_NUM_VOICES;
    int32_t numSecondsDelayNoteOn = DEFAULT_NOTE_ON_DELAY;
    bool schedFifoEnabled = false;
    char testCode = DEFAULT_TEST_CODE;

    TestHarnessBase *harness = NULL;

    SynthMarkResult result;
    VirtualAudioSink audioSink;

    // Parse command line arguments.
    for (int iarg = 1; iarg < argc; iarg++) {
        char * arg = argv[iarg];
        if (arg[0] == '-') {
            switch(arg[1]) {
                case 'p':
                    percentCpu = atoi(&arg[2]);
                    break;
                case 'n':
                    numVoices = atoi(&arg[2]);
                    break;
                case 'd':
                    numSecondsDelayNoteOn = atoi(&arg[2]);
                    break;
                case 'r':
                    sampleRate = atoi(&arg[2]);
                    break;
                case 's':
                    numSeconds = atoi(&arg[2]);
                    break;
                case 'b':
                    framesPerBurst = atoi(&arg[2]);
                    break;
                case 'f':
                    schedFifoEnabled = true;
                    break;
                case 't':
                    testCode = arg[2];
                    break;
                case 'h': // help
                case '?': // help
                    usage(argv[0]);
                    return 0;
                default:
                    printf("Unrecognized switch -%c\n", arg[1]);
                    usage(argv[0]);
                    return 1;
            }
        } else {
            usage(argv[0]);
        }
    }
    if (percentCpu < 1 || percentCpu > 100) {
        printf("Invalid percent CPU = %d\n", percentCpu);
        usage(argv[0]);
        return 1;
    }
    if (numVoices < 1 || numVoices > SYNTHMARK_MAX_VOICES) {
        printf("Invalid num voices = %d\n", numVoices);
        usage(argv[0]);
        return 1;
    }
    if (numSeconds < 1) {
        printf("Invalid duration in seconds = %d\n", numSeconds);
        usage(argv[0]);
        return 1;
    }
    if (numSecondsDelayNoteOn < 0 || numSecondsDelayNoteOn > numSeconds){
        printf("Invalid delay for note on = %d\n", numSecondsDelayNoteOn);
        return 1;
    }
    if (framesPerBurst < 4) {
        printf("Block size too small = %d\n", framesPerBurst);
        usage(argv[0]);
        return 1;
    }

    // Create a test harness and set the parameters.
    switch(testCode) {
        case 'v':
            {
                VoiceMarkHarness *voiceHarness = new VoiceMarkHarness(&audioSink, &result);
                voiceHarness->setTargetCpuLoad(percentCpu * 0.01);
                voiceHarness->setInitialVoiceCount(numVoices);
                harness = voiceHarness;
            }
            break;
        case 'l':
            harness = new LatencyMarkHarness(&audioSink, &result);
            harness->setDelayNotesOn(numSecondsDelayNoteOn);
            break;
        case 'j':
            harness = new JitterMarkHarness(&audioSink, &result);
            harness->setDelayNotesOn(numSecondsDelayNoteOn);
            break;
        default:
            printf("ERROR - unrecognized testCode = %c\n", testCode);
            usage(argv[0]);
            return 1;
            break;
    }
    harness->setSchedFifoEnabled(schedFifoEnabled);
    harness->setNumVoices(numVoices);

    // Print specified parameters.
    printf("--- %s V%d.%d ---\n", harness->getName(), SYNTHMARK_MAJOR_VERSION, SYNTHMARK_MINOR_VERSION);
    printf("  numVoices      = %6d\n", numVoices);
    printf("  noteOnDelay    = %6d\n", numSecondsDelayNoteOn);
    printf("  percentCpu     = %6d\n", percentCpu);
    printf("  framesPerBurst = %6d\n", framesPerBurst);
    printf("--- wait at least %d seconds for benchmark to complete ---\n", numSeconds);
    fflush(stdout);

    // Run the benchmark.
    harness->open(sampleRate, SAMPLES_PER_FRAME, SYNTHMARK_FRAMES_PER_RENDER, framesPerBurst);
    harness->measure(numSeconds);
    harness->close();

    // Print the test results.
    printf("RESULTS BEGIN\n");
    std::cout << result.getResultMessage();
    printf("RESULTS END\n");
    printf("Benchmark complete.\n");

    return result.getResultCode();
}
