/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "SynthMarkCommand.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <stdlib.h>

#include "SynthMark.h"
#include "synth/IncludeMeOnce.h"
#include "synth/Synthesizer.h"
#include "tools/AutomatedTestSuite.h"
#include "tools/ClockRampHarness.h"
#include "tools/JitterMarkHarness.h"
#include "tools/ITestHarness.h"
#include "tools/LatencyMarkHarness.h"
#include "tools/TimingAnalyzer.h"
#include "tools/UtilizationMarkHarness.h"
#include "tools/UtilizationSeriesHarness.h"
#include "tools/VirtualAudioSink.h"
#include "tools/VoiceMarkHarness.h"

constexpr char kDefaultTestCode         = 'v';
constexpr int  kDefaultSeconds          = 10;
constexpr int  kDefaultFramesPerBurst   = 96; // 2 msec at 48000 Hz
constexpr int  kDefaultBufferSizeBursts = 1;
constexpr int  kDefaultNumVoices        = 8;
constexpr int  kDefaultNoteOnDelay      = 0;
constexpr int  kDefaultPercentCpu       = 50;

static void usage(const char *name) {
    printf("SynthMark version %d.%d\n", SYNTHMARK_MAJOR_VERSION, SYNTHMARK_MINOR_VERSION);
    printf("%s -t{test} -n{numVoices} -d{noteOnDelay} -p{percentCPU} -r{sampleRate}"
           " -s{seconds} -b{burstSize} -c{cpuAffinity}\n", name);
    printf("    -t{test}, v=voice, l=latency, j=jitter, u=utilization"
           ", s=series_util, c=clock_ramp, a=automated, default is %c\n",
           kDefaultTestCode);
    printf("    -n{numVoices} to render, default = %d\n", kDefaultNumVoices);
    printf("    -N{numVoices} to render for toggling high load, LatencyMark only\n");
    printf("    -m{voicesMode} algorithm to choose the number of voices in the range\n"
           "      [-n, -N]. This value can be 'l' for a linear increment, 'r' for a\n"
           "      random choice, or 's' to switch between -n and -N. default = s\n");
    printf("    -d{noteOnDelay} seconds to delay the first NoteOn, default = %d\n",
           kDefaultNoteOnDelay);
    printf("    -w{workloadHintsEnabled} 0 = no (default), 1 = give workload hints to scheduler\n");
    printf("    -p{percentCPU} target load, default = %d\n", kDefaultPercentCpu);
    printf("    -r{sampleRate} should be typical, 44100, 48000, etc. default is %d\n",
           kSynthmarkSampleRate);
    printf("    -s{seconds} to run the test, latencyMark may take longer, default is %d\n",
           kDefaultSeconds);
    printf("    -b{burstSize} frames read by virtual hardware at one time, default = %d\n",
           kDefaultFramesPerBurst);
    printf("    -B{bursts} initial buffer size in bursts, default = %d\n",
           kDefaultBufferSizeBursts);
    printf("    -c{cpuAffinity} index of CPU to run on, default = UNSPECIFIED\n");
    printf("    -a{enable} 0 for normal thread, 1 for audio callback, default = 1\n");
}

#define TEXT_ERROR "ERROR: "

/**
 * Convert the input string to a positive integer.
 * @param input
 * @param message
 * @return -1 if an invalid number and print the message
 */
long stringToPositiveInteger(const char *input, const char *message) {
    char *end;
    errno = 0;
    long result = strtol(input,	&end, 10);
    if ((errno != 0) || isgraph(*end)) {
        printf(TEXT_ERROR "argument %s invalid : %s\n", input, message);
        result = -1;
    }
    return result;
}

int synthmark_command_main(int argc, char **argv)
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
    bool    workloadHintsEnabled = false;
    int32_t bufferSizeBursts = kDefaultBufferSizeBursts;
    VoicesMode voicesMode = VOICES_UNDEFINED;
    char testCode = kDefaultTestCode;

    ITestHarness    *harness = nullptr;
    SynthMarkResult  result;
    LogTool          logTool;
    VirtualAudioSink audioSink(logTool);

    printf("# SynthMark V%d.%d\n", SYNTHMARK_MAJOR_VERSION, SYNTHMARK_MINOR_VERSION);

    // Parse command line arguments.
    for (int iarg = 1; iarg < argc; iarg++) {
        char * arg = argv[iarg];
        int temp;
        if (arg[0] == '-') {
            switch(arg[1]) {
                case 'a':
                    temp = stringToPositiveInteger(&arg[2], "-a");
                    if (temp < 0) return 1;
                    useAudioThread = (temp > 0);
                    break;
                case 'c':
                    if ((cpuAffinity = stringToPositiveInteger(&arg[2], "-c")) < 0) return 1;
                    break;
                case 'p':
                    if ((percentCpu = stringToPositiveInteger(&arg[2], "-p")) < 0) return 1;
                    break;
                case 'n':
                    if ((numVoices = stringToPositiveInteger(&arg[2], "-n")) < 0) return 1;
                    break;
                case 'N':
                    if ((numVoicesHigh = stringToPositiveInteger(&arg[2], "-N")) < 0) return 1;
                    break;
                case 'd':
                    if ((numSecondsDelayNoteOn = stringToPositiveInteger(&arg[2], "-d")) < 0) return 1;
                    break;
                case 'r':
                    if ((sampleRate = stringToPositiveInteger(&arg[2], "-r")) < 0) return 1;
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
                    if ((numSeconds = stringToPositiveInteger(&arg[2], "-s")) < 0) return 1;
                    break;
                case 'b':
                    if ((framesPerBurst = stringToPositiveInteger(&arg[2], "-b")) < 0) return 1;
                    break;
                case 'B':
                    if ((bufferSizeBursts = stringToPositiveInteger(&arg[2], "-B")) < 0) return 1;
                    break;
                case 't':
                    testCode = arg[2];
                    break;
                case 'w':
                    temp = stringToPositiveInteger(&arg[2], "-w");
                    if (temp < 0) return 1;
                    workloadHintsEnabled = (temp > 0);
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
    if (voicesMode != VOICES_UNDEFINED && numVoicesHigh == 0) {
        printf(TEXT_ERROR "-N must be set when using -m\n");
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
    audioSink.setDefaultBufferSizeInBursts(bufferSizeBursts);

    // Create a test harness and set the parameters.
    switch(testCode) {
        case 'v':
        {
            VoiceMarkHarness *voiceHarness = new VoiceMarkHarness(&audioSink, &result, logTool);
            voiceHarness->setTargetCpuLoad(percentCpu * 0.01);
            voiceHarness->setInitialVoiceCount(numVoices);
            harness = voiceHarness;
        }
            break;

        case 'l':
        {
            LatencyMarkHarness *latencyHarness = new LatencyMarkHarness(&audioSink, &result, logTool);
            latencyHarness->setNumVoicesHigh(numVoicesHigh);
            latencyHarness->setVoicesMode(voicesMode);
            latencyHarness->setInitialBursts(bufferSizeBursts);
            harness = latencyHarness;
        }
            break;

        case 'j':
        {
            JitterMarkHarness *jitterHarness = new JitterMarkHarness(&audioSink, &result, logTool);
            jitterHarness->setNumVoicesHigh(numVoicesHigh);
            jitterHarness->setVoicesMode(voicesMode);
            harness = jitterHarness;
        }
            break;

        case 'c':
        {
            ClockRampHarness *clockHarness = new ClockRampHarness(&audioSink, &result, logTool);
            clockHarness->setNumVoicesHigh(numVoicesHigh);
            clockHarness->setVoicesMode(voicesMode);
            harness = clockHarness;
        }
            break;

        case 'u':
        {
            UtilizationMarkHarness *utilizationHarness
                    = new UtilizationMarkHarness(&audioSink, &result, logTool);
            harness = utilizationHarness;
        }
            break;

        case 'a':
        {
            AutomatedTestSuite *testSuite = new AutomatedTestSuite(&audioSink, &result, logTool);
            harness = testSuite;
        }
            break;

        case 's':
        {
            UtilizationSeriesHarness *seriesHarness
                    = new UtilizationSeriesHarness(&audioSink, &result, logTool);
            seriesHarness->setNumVoicesHigh(numVoicesHigh);
            harness = seriesHarness;
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
    HostCpuManager::setWorkloadHintsEnabled(workloadHintsEnabled);

    // Print specified parameters.
    printf("  test.name            = %s\n",  harness->getName());
    printf("  num.voices           = %6d\n", numVoices);
    printf("  num.voices.high      = %6d\n", numVoicesHigh);
    printf("  voices.mode          = %6d\n", voicesMode);
    printf("  note.on.delay        = %6d\n", numSecondsDelayNoteOn);
    printf("  target.cpu.percent   = %6d\n", percentCpu);
    printf("  buffer.size.bursts   = %6d\n", bufferSizeBursts);
    printf("  frames.per.burst     = %6d\n", framesPerBurst);
    printf("  msec.per.burst       = %6.2f\n", ((framesPerBurst * 1000.0) / sampleRate));
    printf("  cpu.affinity         = %6d\n", cpuAffinity);
    printf("  cpu.count            = %6d\n", HostTools::getCpuCount());
    printf("  audio.thread         = %6d\n", (useAudioThread ? 1 : 0));
    printf("  workload.hints       = %6d\n", (workloadHintsEnabled ? 1 : 0));
    printf("# wait at least %d seconds for benchmark to complete\n", numSeconds);
    fflush(stdout);

    // Run the benchmark, poll for logs and print them.
    harness->launch(sampleRate, framesPerBurst, numSeconds);
    do {
            if (logTool.hasLogs()) {
                std::cout << logTool.readLog();
                fflush(stdout);
            } else {
                //std::cout << ".";
                //fflush(stdout);
                usleep(50 * 1000);
            }
    } while (harness->isRunning());

    if (logTool.hasLogs()) {
        std::cout << logTool.readLog();
        fflush(stdout);
    }

    // Print the test results.
    std::cout << result.getResultMessage();
    fflush(stdout);

    return result.getResultCode();
}

