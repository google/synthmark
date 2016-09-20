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
#include "tools/VoiceMarkHarness.h"

#define DEFAULT_SECONDS            10
#define DEFAULT_FRAMES_PER_BURST   SYNTHMARK_FRAMES_PER_BURST
#define DEFAULT_INITIAL_VOICES     10
#define DEFAULT_PERCENT_CPU        ((int)(100 * SYNTHMARK_TARGET_CPU_LOAD))

void usage(const char *name) {
    printf("%s -p{percentCPU} -i{initialCount} -s{seconds} -b{burstSize}\n", name);
    printf("    percentCPU to render, default = %d\n", DEFAULT_PERCENT_CPU);
    printf("    initialCount starting number of voices, default = %d\n", DEFAULT_INITIAL_VOICES);
    printf("    seconds required to run the test, default = %d\n", DEFAULT_SECONDS);
    printf("    burstSize frames read by virtual hardware at one time, default = %d\n",
            DEFAULT_FRAMES_PER_BURST);
}

int main(int argc, char **argv)
{
    int32_t percentCpu = DEFAULT_PERCENT_CPU;
    int32_t framesPerBurst = DEFAULT_FRAMES_PER_BURST;
    int32_t numSeconds = DEFAULT_SECONDS;
    int32_t initialVoiceCount = DEFAULT_INITIAL_VOICES;

    SynthMarkResult result;
    VirtualAudioSink audioSink;
    VoiceMarkHarness harness(&audioSink, &result);

    // Parse command line arguments.
    for (int iarg = 1; iarg < argc; iarg++) {
        char * arg = argv[iarg];
        if (arg[0] == '-') {
            switch(arg[1]) {
                case 'p':
                    percentCpu = atoi(&arg[2]);
                    break;
                case 'i':
                    initialVoiceCount = atoi(&arg[2]);
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
    if (percentCpu < 1 || percentCpu > 100) {
        printf("Invalid percent CPU = %d\n", percentCpu);
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

    printf("percentCpu = %d\n", percentCpu);

    harness.open(SYNTHMARK_SAMPLE_RATE, SAMPLES_PER_FRAME,
            SYNTHMARK_FRAMES_PER_RENDER, framesPerBurst);
    harness.setTargetCpuLoad(percentCpu * 0.01);
    harness.setInitialVoiceCount(initialVoiceCount);
    harness.measure(numSeconds);
    harness.close();

    std::cout << result.getResultMessage() << "\n";
    return result.getResultCode();
}

