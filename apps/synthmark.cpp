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

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <tools/AutomatedTestSuite.h>

#include "SynthMark.h"
#include "synth/IncludeMeOnce.h"
#include "synth/Synthesizer.h"
#include "tools/JitterMarkHarness.h"
#include "tools/ITestHarness.h"
#include "tools/LatencyMarkHarness.h"
#include "tools/TimingAnalyzer.h"
#include "tools/UtilizationMarkHarness.h"
#include "tools/VirtualAudioSink.h"
#include "tools/VoiceMarkHarness.h"

constexpr char kDefaultTestCode         = 'v';
constexpr int  kDefaultSeconds          = 10;
constexpr int  kDefaultFramesPerBurst   = 96; // 2 msec at 48000 Hz
constexpr int  kDefaultNumVoices        = 8;
constexpr int  kDefaultNoteOnDelay      = 0;
constexpr int  kDefaultPercentCpu       = 50;

void usage(const char *name) {
    printf("SynthMark version %d.%d\n", SYNTHMARK_MAJOR_VERSION, SYNTHMARK_MINOR_VERSION);
    printf("%s -t{test} -n{numVoices} -d{noteOnDelay} -p{percentCPU} -r{sampleRate}"
           " -s{seconds} -b{burstSize} -c{cpuAffinity}\n", name);
    printf("    -t{test}, v=voice, l=latency, j=jitter, u=utilization, default is %c\n",
           kDefaultTestCode);
    printf("    -n{numVoices} to render, default = %d\n", kDefaultNumVoices);
    printf("    -N{numVoices} to render for toggling high load, LatencyMark only\n");
    printf("    -m{voicesMode} algorithm to choose the number of voices in the range\n"
           "      [-n, -N]. This value can be 'l' for a linear increment, 'r' for a\n"
           "      random choice, or 's' to switch between -n and -N. default = s\n");
    printf("    -d{noteOnDelay} seconds to delay the first NoteOn, default = %d\n",
           kDefaultNoteOnDelay);
    printf("    -p{percentCPU} target load, default = %d\n", kDefaultPercentCpu);
    printf("    -r{sampleRate} should be typical, 44100, 48000, etc. default is %d\n",
           kSynthmarkSampleRate);
    printf("    -s{seconds} to run the test, latencyMark may take longer, default is %d\n",
           kDefaultSeconds);
    printf("    -b{burstSize} frames read by virtual hardware at one time, default = %d\n",
           kDefaultFramesPerBurst);
    printf("    -c{cpuAffinity} index of CPU to run on, default = UNSPECIFIED\n");
    printf("    -a{enable} 0 for normal thread, 1 for audio callback, default = 1\n");
}

#define TEXT_ERROR "ERROR: "

int main(int argc, char **argv)
{
    int32_t percentCpu = kDefaultPercentCpu;
    int32_t sampleRate = kSynthmarkSampleRate;
    int32_t framesPerBurst = kDefaultFramesPerBurst;
    int32_t numSeconds = kDefaultSeconds;
    int32_t numVoices = kDefaultNumVoices;
    int32_t numVoicesHigh = 0;
    int32_t numSecondsDelayNoteOn = kDefaultNoteOnDelay;
    int32_t cpuAffinity = SYNTHMARK_CPU_UNSPECIFIED;
    bool    useAudioThread = true;
    VoicesMode voicesMode = VOICES_UNDEFINED;
    char testCode = kDefaultTestCode;

    ITestHarness *harness = nullptr;

    SynthMarkResult result;
    VirtualAudioSink audioSink;

    printf("# SynthMark V%d.%d\n", SYNTHMARK_MAJOR_VERSION, SYNTHMARK_MINOR_VERSION);

    // Parse command line arguments.
    for (int iarg = 1; iarg < argc; iarg++) {
        char * arg = argv[iarg];
        if (arg[0] == '-') {
            switch(arg[1]) {
                case 'a':
                    useAudioThread = (atoi(&arg[2]) != 0);
                    break;
                case 'c':
                    cpuAffinity = atoi(&arg[2]);
                    break;
                case 'p':
                    percentCpu = atoi(&arg[2]);
                    break;
                case 'n':
                    numVoices = atoi(&arg[2]);
                    break;
                case 'N':
                    numVoicesHigh = atoi(&arg[2]);
                    break;
                case 'd':
                    numSecondsDelayNoteOn = atoi(&arg[2]);
                    break;
                case 'r':
                    sampleRate = atoi(&arg[2]);
                    break;
                case 'm':
                    switch (arg[2]) {
                        case 'r':
                            voicesMode = VOICES_RANDOM;
                            break;
                        case 'l':
                            voicesMode = VOICES_LINEAR_LOOP;
                            break;
                        case 's':
                        default:
                            voicesMode = VOICES_SWITCH;
                            break;
                    }
                    break;
                case 's':
                    numSeconds = atoi(&arg[2]);
                    break;
                case 'b':
                    framesPerBurst = atoi(&arg[2]);
                    break;
                case 't':
                    testCode = arg[2];
                    break;
                case 'h': // help
                case '?': // help
                    usage(argv[0]);
                    return 0;
                default:
                    printf(TEXT_ERROR "Unrecognized switch: %s\n", arg);
                    usage(argv[0]);
                    return 1;
            }
        } else {
            printf(TEXT_ERROR "Unrecognized argument: %s\n", arg);
            usage(argv[0]);
            return 1;
        }
    }
    if (percentCpu < 1 || percentCpu > 100) {
        printf(TEXT_ERROR "Invalid percent CPU = %d\n", percentCpu);
        usage(argv[0]);
        return 1;
    }
    if ((numVoices < 1 && numVoicesHigh <= 0)
        || numVoices < 0
        || numVoices > kSynthmarkMaxVoices) {
        printf(TEXT_ERROR "Invalid num voices = %d\n", numVoices);
        usage(argv[0]);
        return 1;
    }
    if (numVoicesHigh != 0 && numVoicesHigh < numVoices) {
        printf(TEXT_ERROR "Invalid num voices high = %d\n", numVoicesHigh);
        usage(argv[0]);
        return 1;
    }
    if (numVoicesHigh != 0 && testCode != 'l') {
        printf(TEXT_ERROR "Num voices high only supported for LatencyMark\n");
        usage(argv[0]);
        return 1;
    }
    if (voicesMode != VOICES_UNDEFINED && numVoicesHigh == 0) {
        printf(TEXT_ERROR "Random voices only supported for LatencyMark\n");
        usage(argv[0]);
        return 1;
    }
    if (numSeconds < 1) {
        printf(TEXT_ERROR "Invalid duration in seconds = %d\n", numSeconds);
        usage(argv[0]);
        return 1;
    }
    if (numSecondsDelayNoteOn < 0 || numSecondsDelayNoteOn > numSeconds){
        printf(TEXT_ERROR "Invalid delay for note on = %d\n", numSecondsDelayNoteOn);
        return 1;
    }
    if (framesPerBurst < 4) {
        printf(TEXT_ERROR "Block size too small = %d\n", framesPerBurst);
        usage(argv[0]);
        return 1;
    }

    audioSink.setRequestedCpu(cpuAffinity);

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
            {
                LatencyMarkHarness *latencyHarness = new LatencyMarkHarness(&audioSink, &result);
                latencyHarness->setNumVoicesHigh(numVoicesHigh);
                latencyHarness->setVoicesMode(voicesMode);
                harness = latencyHarness;
            }
            break;
        case 'j':
            harness = new JitterMarkHarness(&audioSink, &result);
            break;
        case 'u':
            {
                UtilizationMarkHarness *utilizationHarness = new UtilizationMarkHarness(&audioSink, &result);
                utilizationHarness->setNumVoices(numVoices);
                harness = utilizationHarness;
            }
            break;
        case 'a':
            {
                AutomatedTestSuite *testSuite = new AutomatedTestSuite(&audioSink, &result);
                harness = testSuite;
            }
            break;
        default:
            printf(TEXT_ERROR "unrecognized testCode = %c\n", testCode);
            usage(argv[0]);
            return 1;
            break;
    }
    harness->setNumVoices(numVoices);
    harness->setDelayNoteOnSeconds(numSecondsDelayNoteOn);
    harness->setThreadType(useAudioThread
                           ? HostThreadFactory::ThreadType::Audio
                           : HostThreadFactory::ThreadType::Default);

    // Print specified parameters.
    printf("  test.name          = %s\n", harness->getName());
    printf("  num.voices         = %6d\n", numVoices);
    printf("  num.voices.high    = %6d\n", numVoicesHigh);
    printf("  voices.mode        = %6d\n", voicesMode);
    printf("  note.on.delay      = %6d\n", numSecondsDelayNoteOn);
    printf("  target.cpu.percent = %6d\n", percentCpu);
    printf("  frames.per.burst   = %6d\n", framesPerBurst);
    printf("  msec.per.burst     = %6.2f\n", ((framesPerBurst * 1000.0) / sampleRate));
    printf("  cpu.affinity       = %6d\n", cpuAffinity);
    printf("  cpu.count          = %6d\n", HostTools::getCpuCount());
    printf("  audio.thread       = %6d\n", (useAudioThread ? 1 : 0));
    printf("# wait at least %d seconds for benchmark to complete\n", numSeconds);
    fflush(stdout);

    // Run the benchmark.
    harness->runTest(sampleRate, framesPerBurst, numSeconds);

    // Print the test results.
    printf(TEXT_RESULTS_BEGIN "\n");
    std::cout << result.getResultMessage();
    printf(TEXT_RESULTS_END "\n");
    printf("# Benchmark complete.\n");

    return result.getResultCode();
}
