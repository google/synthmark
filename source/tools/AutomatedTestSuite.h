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
#include "TestHarnessParameters.h"


/**
 * Run an automated test, analyze the results and print a report.
 * 1) Determine what CPUs are available
 * 2) Run VoiceMark on each CPU to determine big vs little CPUs
 * 3) Run LatencyMark with a light load, a heavy load, then an alternating load.
 * 4) Print report with analysis.
 *
 * The current code:
 *     assumes that the CPU numbering depends on their capacity,
 *     assumes that both lowCpu and highCpu are online,
 *     assume the existence of only two architectures, homogeneous or BIG.little.
 * TODO handle more architectures.
 */
class AutomatedTestSuite : public TestHarnessParameters {

public:
    AutomatedTestSuite(AudioSinkBase *audioSink,
            SynthMarkResult *result, LogTool &logTool)
    : TestHarnessParameters(audioSink, result, logTool)
    {
        // Constant seed to obtain a fixed pattern of pseudo-random voices
        // for experiment repeatability and reproducibility
        //srand(time(NULL)); // "Random" seed
        srand(0); // Fixed seed
    }

    virtual ~AutomatedTestSuite() {
    }

    const char *getName() const override {
        return "Automated Test Suite";
    }

    double framesToMillis(double frames) {
        return frames * SYNTHMARK_MILLIS_PER_SECOND / getSampleRate();
    }

    int32_t runTest(int32_t sampleRate, int32_t framesPerBurst, int32_t numSeconds) override {
        mLogTool.log("\nSynthMark Version " SYNTHMARK_VERSION_TEXT "\n");

        mLogTool.log("\n-------- CPU Performance ------------\n");
        int32_t err = measureLowHighCpuPerformance(sampleRate, framesPerBurst, 10);
        if (err) return err;

        mLogTool.log("\n-------- LATENCY ------------\n");
        // Test both sizes of CPU if needed. BIG or little
        LatencyResult result;

        double latencyMarkFixedLittleFrames = 999999.0; // prevent compiler warnings
        double latencyMarkDynamicLittleFrames = 999999.0; // prevent compiler warnings

        if (mHaveBigLittle) {
            result = measureLatency(sampleRate, framesPerBurst, numSeconds,
                                 mLittleCpu, mVoiceMarkLittle);
            if (result.err) return result.err;
            latencyMarkFixedLittleFrames = result.lightLatencyFrames;
            latencyMarkDynamicLittleFrames = result.mixedLatencyFrames;

        }
        result = measureLatency(sampleRate, framesPerBurst, numSeconds,
                             mBigCpu, mVoiceMarkBig);
        err = result.err;
        // If we don't have a LITTLE CPU then just use the BIG CPU.
        if (!mHaveBigLittle) {
            latencyMarkFixedLittleFrames = result.lightLatencyFrames;
            latencyMarkDynamicLittleFrames = result.mixedLatencyFrames;
        }

        printSummaryCDD(latencyMarkFixedLittleFrames, latencyMarkDynamicLittleFrames);

        return err;
    }


private:

    struct LatencyResult {
        int err = 0;
        double lightLatencyFrames = 999999;
        double heavyLatencyFrames = 999999;
        double mixedLatencyFrames = 999999;
    };

// Key text for CDD report.
#define kKeyVoiceMark90          "voicemark.90"
#define kKeyLatencyFixedLittle   "latencymark.fixed.little"
#define kKeyLatencyDynamicLittle "latencymark.dynamic.little"

    static constexpr double kHighLowThreshold = 0.1; // Difference between CPUs to consider them BIG-little.
    static constexpr double kMaxUtilization = 0.9; // Maximum load that we will push the CPU to.
    static constexpr int    kMaxUtilizationPercent = (int)(kMaxUtilization * 100); // as a percentage

    // These names must match the names in the Android CDD.
    void printSummaryCDD(double latencyMarkFixedLittleFrames,
                         double latencyMarkDynamicLittleFrames) {
        std::stringstream resultMessage;
        resultMessage << TEXT_CDD_SUMMARY_BEGIN << " -------" << std::endl;

        resultMessage << kKeyVoiceMark90 << " = " << mVoiceMarkBig << std::endl;

        int latencyMillis = (int)framesToMillis(latencyMarkFixedLittleFrames);
        if (latencyMillis <= 0) {
            resultMessage << "# " << kKeyLatencyFixedLittle << " could not be measured!" << std::endl;
        } else {
            resultMessage << kKeyLatencyFixedLittle << " = " << latencyMillis << std::endl;
        }

        latencyMillis = (int)framesToMillis(latencyMarkDynamicLittleFrames);
        if (latencyMillis <= 0) {
            resultMessage << "# " << kKeyLatencyDynamicLittle << " could not be measured!" << std::endl;
        } else {
            resultMessage << kKeyLatencyDynamicLittle << " = " << latencyMillis << std::endl;
        }

        resultMessage << TEXT_CDD_SUMMARY_END << " ---------" << std::endl;
        resultMessage << std::endl;
        mResult->appendMessage(resultMessage.str());
    }

    virtual int32_t measureLowHighCpuPerformance(int32_t sampleRate,
                                                 int32_t framesPerBurst,
                                                 int32_t numSeconds) {
        std::stringstream resultMessage;
        int numCPUs = HostTools::getCpuCount();
        SynthMarkResult result1;
        VoiceMarkHarness *harness = new VoiceMarkHarness(mAudioSink, &result1, mLogTool);
        harness->setTargetCpuLoad(kMaxUtilization);
        harness->setInitialVoiceCount(mNumVoices);
        harness->setDelayNoteOnSeconds(mDelayNotesOn);
        harness->setThreadType(mThreadType);

        // TODO This is hack way to choose CPUs for BIG.little architectures.
        // TODO Test each CPU or come up with something better.
        mLogTool.log("Try to find BIG/LITTLE CPUs\n");
        int lowCpu = (1 * numCPUs) / 4;
        mAudioSink->setRequestedCpu(lowCpu);
        mLogTool.log("Run low VoiceMark with CPU #%d\n", lowCpu);
        int32_t err = harness->runTest(sampleRate, framesPerBurst, numSeconds);
        if (err) {
            delete harness;
            return err;
        }

        double voiceMarkLowIndex = result1.getMeasurement();
        mLogTool.log("low VoiceMark_%d = %5.1f\n", kMaxUtilizationPercent, voiceMarkLowIndex);
        double voiceMarkHighIndex = voiceMarkLowIndex;

        int highCpu = (3 * numCPUs) / 4;
        // If there are more than 2 CPUs than measure one each from top and bottom half in
        // case they are heterogeneous.
        if (lowCpu != highCpu) {
            mAudioSink->setRequestedCpu(highCpu);
            mLogTool.log("Run high VoiceMark with CPU #%d\n", highCpu);
            harness->runTest(sampleRate, framesPerBurst, numSeconds);
            voiceMarkHighIndex = result1.getMeasurement();
            mLogTool.log("high VoiceMark_%d = %5.1f\n", kMaxUtilizationPercent, voiceMarkHighIndex);

            double averageVoiceMark = 0.5 * (voiceMarkHighIndex + voiceMarkLowIndex);
            double threshold = kHighLowThreshold * averageVoiceMark;

            mHaveBigLittle = (abs(voiceMarkHighIndex - voiceMarkLowIndex) > threshold);
        }

        if (mHaveBigLittle) {
            if (voiceMarkHighIndex > voiceMarkLowIndex) {
                mLittleCpu = lowCpu;
                mVoiceMarkLittle = voiceMarkLowIndex;
                mBigCpu = highCpu;
                mVoiceMarkBig = voiceMarkHighIndex;
            } else {
                mLittleCpu = highCpu;
                mVoiceMarkLittle = voiceMarkHighIndex;
                mBigCpu = lowCpu;
                mVoiceMarkBig = voiceMarkLowIndex;
            }
            resultMessage << "# The CPU seems to be heterogeneous. Assume BIG-little.\n";
            resultMessage << "cpu.little = " << mLittleCpu << std::endl;
            resultMessage << "cpu.big = " << mBigCpu << std::endl;
        } else {
            mLittleCpu = highCpu;
            mVoiceMarkLittle = voiceMarkHighIndex;
            mBigCpu = highCpu;
            mVoiceMarkBig = voiceMarkHighIndex;
            resultMessage << "# The CPU seems to be homogeneous.\n";
            resultMessage << "cpu = " << mLittleCpu << std::endl;
        }

        resultMessage << "voice.mark." << cpuToBigLittle(lowCpu) << " = " << voiceMarkLowIndex << std::endl;
        if (lowCpu != highCpu) {
            resultMessage << "voice.mark." << cpuToBigLittle(highCpu) << " = " << voiceMarkHighIndex << std::endl;
        }

        // std::cout << result1.getResultMessage();

        mResult->appendMessage(resultMessage.str());
        delete harness;
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
        LatencyMarkHarness *harness = new LatencyMarkHarness(mAudioSink, &result1, mLogTool);
        harness->setDelayNoteOnSeconds(mDelayNotesOn);
        harness->setNumVoices(numVoices);
        harness->setNumVoicesHigh(numVoicesHigh);
        harness->setThreadType(mThreadType);

        mAudioSink->setRequestedCpu(cpu);
        mLogTool.log("Run LatencyMark with CPU #%d, voices = %d / %d\n",
                     cpu, numVoices, numVoicesHigh);
        int32_t err = harness->runTest(sampleRate, framesPerBurst, numSeconds);
        if (err) {
            *latencyPtr = 0.0;
            delete harness;
            return err;
        }

        // std::cout << result1.getResultMessage();

        double latencyFrames = result1.getMeasurement();
        std::stringstream suffix;
        suffix << "." << cpu << "." << numVoices << "." << numVoicesHigh;
        resultMessage << "audio.latency.frames" << suffix.str() << " = " << latencyFrames << std::endl;
        double latencyMillis = framesToMillis(latencyFrames);
        resultMessage << "audio.latency.msec" << suffix.str() << " = " << latencyMillis << std::endl;
        mResult->appendMessage(resultMessage.str());
        *latencyPtr = latencyFrames;
        mLogTool.log("Got LatencyMark = %5.1f msec\n", latencyMillis);
        delete harness;
        return SYNTHMARK_RESULT_SUCCESS;
    }

    const char *cpuToBigLittle(int cpu) {
        if (cpu == mBigCpu) return "big";
        else if (cpu == mLittleCpu) return "little";
        else return "cpux";
    }

    LatencyResult measureLatency(int32_t sampleRate,
                           int32_t framesPerBurst,
                           int32_t numSeconds,
                           int cpu,
                           int32_t numVoicesMax) {
        std::stringstream message;
        LatencyResult result;

        mLogTool.log("\n---- Measure latency for CPU #%d ----\n", cpu);
        int32_t voiceMarkLow = 1 * numVoicesMax / 4;
        int32_t voiceMarkHigh = 3 * numVoicesMax / 4;

        // Test latency with a low number of voices.
        result.err = measureLatencyOnce(sampleRate, framesPerBurst, numSeconds,
                                         cpu, voiceMarkLow, voiceMarkLow, &result.lightLatencyFrames);
        if (result.err) return result;
        message << "# Latency in frames with a steady light CPU load.\n";
        message << "latency.light." << cpuToBigLittle(cpu) << " = " << result.lightLatencyFrames << std::endl;

        // Test latency with a high number of voices.
        result.err = measureLatencyOnce(sampleRate, framesPerBurst, numSeconds,
                                 cpu, voiceMarkHigh, voiceMarkHigh, &result.heavyLatencyFrames);
        if (result.err) return result;
        message << "# Latency in frames with a steady heavy CPU load.\n";
        message << "latency.heavy." << cpuToBigLittle(cpu) << " = " << result.heavyLatencyFrames << std::endl;

        // Alternate low to high to stress the CPU governor.
        result.err = measureLatencyOnce(sampleRate, framesPerBurst, numSeconds,
                                 cpu, voiceMarkLow, voiceMarkHigh, &result.mixedLatencyFrames);
        if (result.err) return result;
        message << "# Latency in frames when alternating between light and heavy CPU load.\n";
        message << "latency.mixed." << cpuToBigLittle(cpu) << " = " << result.mixedLatencyFrames << std::endl;

        // Analysis
        double mixedOverHigh = result.mixedLatencyFrames / result.heavyLatencyFrames;
        message << "latency.mixed.over.heavy." << cpuToBigLittle(cpu)
                << " = " << mixedOverHigh << std::endl;
        if (mixedOverHigh > 1.1) {
            message << "# Dynamic load on CPU " << cpu << " has higher latency than"
                    << " a steady heavy load.\n";
            message << "# This suggests that the CPU governor is not responding\n";
            message << "# very quickly to sudden changes in load.\n";
        }
        message << std::endl;
        mResult->appendMessage(message.str());
        return result;
    }

private:

    int              mBigCpu = 0;  // A CPU index in fast half of CPUs available.
    double           mVoiceMarkBig = 0.0;  // for CDD voicemark.90

    int              mLittleCpu = 0; // A CPU index in slow half of CPUs available.
    double           mVoiceMarkLittle = 0.0;

    bool             mHaveBigLittle = false;

};

#endif //SYNTHMARK_AUTOMATED_TEST_SUITE_H
