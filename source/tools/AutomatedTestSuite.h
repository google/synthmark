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
#include <vector>

#include "tools/LatencyMarkHarness.h"
#include "tools/TimingAnalyzer.h"
#include "tools/UtilizationMarkHarness.h"
#include "tools/VirtualAudioSink.h"
#include "tools/VoiceMarkHarness.h"
#include "tools/TestHarnessParameters.h"
#include "BenchMarkHarness.h"

/**
 * Run an automated test, analyze the results and print a report.
 * 1) Determine what CPUs are available.
 * 2) Run a quick BenchMark on each CPU to identify, little, medium, big and huge CPUs.
 * 3) Use LatencyMark to detect various problems.
 * 4) With a light load on a small core to detect preemption from bad drivers.
 * 6) With a dynamic load on a small core to detect slow response frequency scaling CPU governor.
 * 5) With moderate loads with no CPU affinity to detect problems with CPU migration.
 * 6) With a dynamic load to detect slow response from CPU governor.
 *
 * TODO handle more architectures
 * TODO Measure latency without CPU affinity
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
        int32_t err = measureCpuSpeeds(sampleRate, framesPerBurst, 10);
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

    std::vector<std::vector<std::pair<int, int>>> findClustersWithIndices(const std::vector<int>& data, int threshold) {
        std::vector<std::pair<int, int>> indexedData;
        for (size_t i = 0; i < data.size(); ++i) {
            indexedData.emplace_back(data[i], static_cast<int>(i)); // Store value and index
        }

        std::sort(indexedData.begin(), indexedData.end()); // Sort based on values

        std::vector<std::vector<std::pair<int, int>>> clusters;
        std::vector<std::pair<int, int>> currentCluster;

        if (!indexedData.empty()) {
            currentCluster.push_back(indexedData[0]);
            clusters.push_back(currentCluster);
            currentCluster.clear();
        }

        for (size_t i = 1; i < indexedData.size(); ++i) {
            if (std::abs(indexedData[i].first - indexedData[i - 1].first) > threshold) {
                clusters.push_back(currentCluster);
                currentCluster.clear();
            }
            if(currentCluster.empty()){
                clusters.back().push_back(indexedData[i]);
            }
            else{
                currentCluster.push_back(indexedData[i]);
            }
        }
        if(!currentCluster.empty()){
            clusters.push_back(currentCluster);
        }
        if(clusters[0].empty()){
            clusters.erase(clusters.begin());
        }

        return clusters;
    }

    // Measure all of the CPU speeds.
    virtual int32_t measureCpuSpeeds(int32_t sampleRate,
                                     int32_t framesPerBurst,
                                     int32_t numSeconds) {
        std::stringstream resultMessage;
        int numCPUs = HostTools::getCpuCount();
        std::vector<int32_t> cpuMaxVoices;
        int32_t maxVoicesPossible = 0;
        SynthMarkResult result1;
        for (int cpuIndex = 0; cpuIndex < numCPUs; cpuIndex++) {
            BenchMarkHarness harness(mAudioSink, &result1, mLogTool);
            harness.setNumVoices(getNumVoices());
            harness.setDelayNoteOnSeconds(mDelayNotesOn);
            harness.setThreadType(mThreadType);

            mAudioSink->setRequestedCpu(cpuIndex);
            mLogTool.log("Run BenchMark with CPU #%d\n", cpuIndex);
            int32_t err = harness.runTest(sampleRate, framesPerBurst, numSeconds);
            if (err) {
                return err;
            }

            int32_t maxVoicesPerCpu = harness.getMaxVoices();
            mLogTool.log("max_voices_%d = %5.1f\n", cpuIndex, maxVoicesPerCpu);
            cpuMaxVoices.push_back(maxVoicesPerCpu);
            maxVoicesPossible = std::max(maxVoicesPossible, maxVoicesPerCpu);
            // Give CPU time to settle down before the next benchmark.
            HostTools::sleepForNanoseconds(500 * SYNTHMARK_NANOS_PER_MILLISECOND);
        }

        // Figure out CPU clusters from the benchmarks.
        const int threshold = 30;
        std::vector<std::vector<std::pair<int, int>>> clusters = findClustersWithIndices(cpuMaxVoices, threshold);

        for (size_t i = 0; i < clusters.size(); ++i) {
            mLogTool.log("Cluster[#%d]: ", i);
            for (const auto& pair : clusters[i]) {
                mLogTool.log("( %d, %d) " ,  pair.first, pair.second);
            }
            mLogTool.log("\n");
            std::cout << std::endl;
        }

        mHaveBigLittle = clusters.size() > 1;

        const auto littleCluster = clusters[0];
        const auto& pair = littleCluster[littleCluster.size() - 1];
        mLittleCpu = pair.second;
        mVoiceMarkLittle = pair.first;
        if (mHaveBigLittle) {
            resultMessage << "# The CPU seems to be heterogeneous.\n";
            resultMessage << "cpu.little = " << mLittleCpu << std::endl;
        } else {
            resultMessage << "# The CPU seems to be homogeneous.\n";
            resultMessage << "cpu = " << mLittleCpu << std::endl;
        }

        resultMessage << "max.voices.little  = " << mVoiceMarkLittle << std::endl;

        //std::cout << result1.getResultMessage();

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
        suffix << "." << ((cpu < 0) ? "N" : std::to_string(cpu)) << "." << numVoices << "." << numVoicesHigh;
        resultMessage << "audio.latency.frames" << suffix.str() << " = " << latencyFrames << std::endl;
        double latencyMillis = framesToMillis(latencyFrames);
        resultMessage << "audio.latency.msec" << suffix.str() << " = " << latencyMillis << std::endl;
        mResult->appendMessage(resultMessage.str());
        *latencyPtr = latencyFrames;
        mLogTool.log("Got latencyMillis = %5.1f msec\n", latencyMillis);
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
        int32_t numVoicesLow = 1 * numVoicesMax / 4;
        int32_t numVoicesHigh = 3 * numVoicesMax / 4;

        // Test latency with a low number of voices.
        result.err = measureLatencyOnce(sampleRate, framesPerBurst, numSeconds,
                                        cpu, numVoicesLow, numVoicesLow, &result.lightLatencyFrames);
        if (result.err) return result;
        message << "# Latency in frames with a steady light CPU load.\n";
        message << "latency.light." << cpuToBigLittle(cpu) << " = " << result.lightLatencyFrames << std::endl;

        // Test latency with a high number of voices.
        result.err = measureLatencyOnce(sampleRate, framesPerBurst, numSeconds,
                                        cpu, numVoicesHigh, numVoicesHigh, &result.heavyLatencyFrames);
        if (result.err) return result;
        message << "# Latency in frames with a steady heavy CPU load.\n";
        message << "latency.heavy." << cpuToBigLittle(cpu) << " = " << result.heavyLatencyFrames << std::endl;

        // Alternate low to high to stress the CPU governor.
        result.err = measureLatencyOnce(sampleRate, framesPerBurst, numSeconds,
                                        cpu, numVoicesLow, numVoicesHigh, &result.mixedLatencyFrames);
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
