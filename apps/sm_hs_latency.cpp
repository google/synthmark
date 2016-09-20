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
#include "SynthMark.h"
#include "tools/TimingAnalyzer.h"
#include "synth/Synthesizer.h"
#include "tools/VirtualAudioSink.h"
#include "tools/LatencyMarkHarness.h"

#define DEFAULT_SECONDS            10
#define DEFAULT_FRAMES_PER_BURST  SYNTHMARK_FRAMES_PER_BURST
#define DEFAULT_NUM_VOICES         8

void usage(const char *name) {
    printf("%s -n{numVoices} -r{sampleRate} -s{seconds} -b{burstSize}\n", name);
    printf("    numVoices to render, default = %d\n", DEFAULT_NUM_VOICES);
    printf("    rate should be typical, 44100, 48000, etc. default is %d\n", SYNTHMARK_SAMPLE_RATE);
    printf("    seconds required to be glitch free, default is %d\n", DEFAULT_SECONDS);
    printf("    burstSize frames read by virtual hardware at one time , default = %d\n",
            DEFAULT_FRAMES_PER_BURST);
}

int main(int argc, char **argv)
{
    int32_t sampleRate = SYNTHMARK_SAMPLE_RATE;
    int32_t framesPerBurst = DEFAULT_FRAMES_PER_BURST;
    int32_t numSeconds = DEFAULT_SECONDS;
    int32_t numVoices = DEFAULT_NUM_VOICES;

    SynthMarkResult result;
    VirtualAudioSink audioSink;
    LatencyMarkHarness harness(&audioSink, &result);

    // Parse command line arguments.
    for (int iarg = 1; iarg < argc; iarg++) {
        char * arg = argv[iarg];
        if (arg[0] == '-') {
            switch(arg[1]) {
                case 'n':
                    numVoices = atoi(&arg[2]);
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
    if (framesPerBurst < 4) {
        printf("Block size too small = %d\n", framesPerBurst);
        usage(argv[0]);
        return 1;
    }

    // Run the benchmark.
    harness.open(sampleRate, SAMPLES_PER_FRAME, SYNTHMARK_FRAMES_PER_RENDER, framesPerBurst);
    harness.measure(numSeconds);
    harness.close();

    std::cout << result.getResultMessage() << "\n";

    printf("Test complete.\n");
    return result.getResultCode();
}


