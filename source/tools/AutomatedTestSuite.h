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

#ifndef SYNTHMARK_AUTOMATED_TEST_SUITE_H
#define SYNTHMARK_AUTOMATED_TEST_SUITE_H

#include <cmath>
#include <cstdint>

#include "tools/LatencyMarkHarness.h"
#include "tools/TimingAnalyzer.h"
#include "tools/UtilizationMarkHarness.h"
#include "tools/VirtualAudioSink.h"
#include "tools/VoiceMarkHarness.h"

constexpr double kHighLowThreshold = 0.1; // Difference between CPUs to consider them BIG-little.
constexpr double kMaxUtilization = 0.9; // Maximum load that we will push the CPU to.

/**
 * Run an automated test, analyze the results and print a report.
 * 1) Determine what CPUs are available
 * 2) Run VoiceMark on each CPU to determine big vs little CPUs
 * 3) Run LatencyMark with a light load, a heavy load, then an alternating load.
 * 4) Print report with analysis.
 */
class AutomatedTestSuite : public ITestHarness {

public:
    AutomatedTestSuite(AudioSinkBase *audioSink,
            SynthMarkResult *result,
            LogTool *logTool = nullptr)
    : mAudioSink(audioSink)
    , mResult(result)
    , mLogTool(logTool)
    {
        setNumVoices(kSynthmarkNumVoicesLatency);

        // Constant seed to obtain a fixed pattern of pseudo-random voices
        // for experiment repeatability and reproducibility
        //srand(time(NULL)); // "Random" seed
        srand(0); // Fixed seed
    }

    virtual ~AutomatedTestSuite() {
    }

    virtual void setNumVoices(int32_t numVoices) {
        mNumVoices = numVoices;
    }

    virtual void setDelayNoteOnSeconds(int32_t delayNotesOn) {
        mDelayNotesOn = delayNotesOn;
    }

    virtual const char *getName() {
        return "Automated Test Suite";
    }

    virtual int32_t runTest(int32_t sampleRate, int32_t framesPerBurst, int32_t numSeconds) {
        int32_t err = measureLowHighCpuPerformance(sampleRate, framesPerBurst, 10);
        if (err) return err;

        // Test both sizes of CPU if needed. BIG or little
        if (mHaveBigLittle) {
            err = measureLatency(sampleRate, framesPerBurst, numSeconds,
                                 mLittleCpu, mVoiceMarkMaxLittle);
            if (err) return err;
        }
        err = measureLatency(sampleRate, framesPerBurst, numSeconds,
                             mBigCpu, mVoiceMarkMaxBig);
        return err;
    }

private:

    virtual int32_t measureLowHighCpuPerformance(int32_t sampleRate,
                                                 int32_t framesPerBurst,
                                                 int32_t numSeconds) {
        std::stringstream resultMessage;
        int numCPUs = HostTools::getCpuCount();
        SynthMarkResult result1;
        VoiceMarkHarness *harness = new VoiceMarkHarness(mAudioSink, &result1);
        harness->setTargetCpuLoad(kMaxUtilization);
        harness->setInitialVoiceCount(mNumVoices);
        harness->setDelayNoteOnSeconds(mDelayNotesOn);

        int lowCpu = (1 * numCPUs) / 4;
        mAudioSink->setRequestedCpu(lowCpu);
        int32_t err = harness->runTest(sampleRate, framesPerBurst, numSeconds);
        if (err) return err;

        double voiceMarkMaxLow = result1.getMeasurement();
        double voiceMarkMaxHigh = voiceMarkMaxLow;


        int highCpu = (3 * numCPUs) / 4;
        // If there are more than 2 CPUs than measure one each from top and bottom half in
        // case they are heterogeneous.
        if (lowCpu != highCpu) {
            mAudioSink->setRequestedCpu(highCpu);
            harness->runTest(sampleRate, framesPerBurst, numSeconds);
            voiceMarkMaxHigh = result1.getMeasurement();

            double averageVoiceMark = 0.5 * (voiceMarkMaxHigh + voiceMarkMaxLow);
            double threshold = kHighLowThreshold * averageVoiceMark;

            mHaveBigLittle = (abs(voiceMarkMaxHigh - voiceMarkMaxLow) > threshold);
        }

        if (mHaveBigLittle) {
            if (mVoiceMarkMaxLittle > voiceMarkMaxLow) {
                mLittleCpu = lowCpu;
                mVoiceMarkMaxLittle = voiceMarkMaxLow;
                mBigCpu = highCpu;
                mVoiceMarkMaxBig = voiceMarkMaxHigh;
            } else {
                mLittleCpu = highCpu;
                mVoiceMarkMaxLittle = voiceMarkMaxHigh;
                mBigCpu = lowCpu;
                mVoiceMarkMaxBig = voiceMarkMaxLow;
            }
            resultMessage << "# The CPU seems to be heterogeneous. Assume BIG-little.\n";
            resultMessage << "cpu.little = " << mLittleCpu << std::endl;
            resultMessage << "cpu.big = " << mBigCpu << std::endl;
        } else {
            mLittleCpu = highCpu;
            mVoiceMarkMaxLittle = voiceMarkMaxHigh;
            mBigCpu = highCpu;
            mVoiceMarkMaxBig = voiceMarkMaxHigh;
            resultMessage << "# The CPU seems to be homogeneous.\n";
            resultMessage << "cpu = " << mLittleCpu << std::endl;
        }

        resultMessage << "voice.mark." << cpuToBigLittle(lowCpu) << " = " << voiceMarkMaxLow << std::endl;
        if (lowCpu != highCpu) {
            resultMessage << "voice.mark." << cpuToBigLittle(highCpu) << " = " << voiceMarkMaxHigh << std::endl;
        }

        std::cout << result1.getResultMessage();

        mResult->appendMessage(resultMessage.str());
        return SYNTHMARK_RESULT_SUCCESS;
    }

    int32_t measureLatencyOnce(int32_t sampleRate,
                               int32_t framesPerBurst,
                               int32_t numSeconds,
                               int cpu,
                               int32_t numVoices,
                               int32_t numVoicesHigh,
                               double *latencyPtr) {
        std::stringstream resultMessage;
        SynthMarkResult result1;
        LatencyMarkHarness *harness = new LatencyMarkHarness(mAudioSink, &result1);
        harness->setDelayNoteOnSeconds(mDelayNotesOn);
        harness->setNumVoices(numVoices);
        harness->setNumVoicesHigh(numVoicesHigh);

        mAudioSink->setRequestedCpu(cpu);
        int32_t err = harness->runTest(sampleRate, framesPerBurst, numSeconds);
        if (err) return err;

        std::cout << result1.getResultMessage();

        double latencyFrames = result1.getMeasurement();
        std::stringstream suffix;
        suffix << "." << cpu << "." << numVoices << "." << numVoicesHigh;
        resultMessage << "audio.latency.frames" << suffix.str() << " = " << latencyFrames << std::endl;
        double latencyMillis = latencyFrames * SYNTHMARK_MILLIS_PER_SECOND / sampleRate;
        resultMessage << "audio.latency.msec" << suffix.str() << " = " << latencyMillis << std::endl;
        mResult->appendMessage(resultMessage.str());
        *latencyPtr = latencyFrames;
        return SYNTHMARK_RESULT_SUCCESS;
    }

    const char *cpuToBigLittle(int cpu) {
        if (cpu == mBigCpu) return "big";
        else if (cpu == mLittleCpu) return "little";
        else return "cpux";
    }

    int32_t measureLatency(int32_t sampleRate,
                           int32_t framesPerBurst,
                           int32_t numSeconds,
                           int cpu,
                           int32_t numVoicesMax) {
        std::stringstream message;
        int32_t voiceMarkLow = 1 * numVoicesMax / 4;
        int32_t voiceMarkHigh = 3 * numVoicesMax / 4;

        // Test latency with a low number of voices.
        double lightLatency;
        int32_t err = measureLatencyOnce(sampleRate, framesPerBurst, numSeconds,
                                         cpu, voiceMarkLow, voiceMarkLow, &lightLatency);
        message << "# Latency in frames with a steady light CPU load.\n";
        message << "latency.light." << cpuToBigLittle(cpu) << " = " << lightLatency << std::endl;
        if (err) return err;

        // Test latency with a high number of voices.
        double heavyLatency;
        err = measureLatencyOnce(sampleRate, framesPerBurst, numSeconds,
                                 cpu, voiceMarkHigh, voiceMarkHigh, &heavyLatency);
        if (err) return err;
        message << "# Latency in frames with a steady heavy CPU load.\n";
        message << "latency.heavy." << cpuToBigLittle(cpu) << " = " << heavyLatency << std::endl;

        // Alternate low to high to stress the CPU governor.
        double mixedLatency;
        err = measureLatencyOnce(sampleRate, framesPerBurst, numSeconds,
                                 cpu, voiceMarkLow, voiceMarkHigh, &mixedLatency);
        if (err) return err;
        message << "# Latency in frames when alternating between light and heavy CPU load.\n";
        message << "latency.mixed." << cpuToBigLittle(cpu) << " = " << mixedLatency << std::endl;

        // Analysis
        double highOverLow = heavyLatency / lightLatency;
        double mixedOverHigh = mixedLatency / heavyLatency;
        message << "latency.mixed.over.heavy." << cpuToBigLittle(cpu)
                << " = " << mixedOverHigh << std::endl;
        if (mixedOverHigh > 1.1) {
            message << "# Dynamic load on CPU " << cpu << " has higher latency than"
                    << " a steady heavy load.\n";
            message << "# This suggests that the CPU governor is not responding\n";
            message << "# quickly enough to sudden changes in load.\n";
        }
        message << std::endl;
        mResult->appendMessage(message.str());
        return err;
    }

private:

    int32_t          mNumVoices = 8;
    int32_t          mDelayNotesOn = 0;

    int              mBigCpu = 0;  // A CPU index in low half of CPUs available.
    double           mVoiceMarkMaxBig = 0.0;

    int              mLittleCpu = 0; // A CPU index in high half of CPUs available.
    double           mVoiceMarkMaxLittle = 0.0;

    bool             mHaveBigLittle = false;

    AudioSinkBase   *mAudioSink = nullptr;
    SynthMarkResult *mResult = nullptr;
    LogTool         *mLogTool = nullptr;
};

#endif //SYNTHMARK_AUTOMATED_TEST_SUITE_H
