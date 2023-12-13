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

#ifndef SYNTHMARK_LATENCYMARK_HARNESS_H
#define SYNTHMARK_LATENCYMARK_HARNESS_H

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <sstream>

#include "AudioSinkBase.h"
#include "ChangingVoiceHarness.h"
#include "SynthMark.h"
#include "synth/Synthesizer.h"
#include "TestHarnessParameters.h"
#include "tools/LogTool.h"
#include "tools/TestHarnessBase.h"
#include "tools/TimingAnalyzer.h"

#define BURSTS_OVER_RANGE 99999

/**
 * Determine buffer latency required to avoid glitches.
 * The "LatencyMark" is the minimum buffer size that is a multiple
 * of a burst size that can be used for N minutes without glitching.
 */
class LatencyMarkHarness : public ChangingVoiceHarness {
public:
    LatencyMarkHarness(AudioSinkBase *audioSink, SynthMarkResult *result, LogTool &logTool)
    : ChangingVoiceHarness(audioSink, result, logTool) {
    }

    const char *getName() const override {
        return "LatencyMark";
    }

    void setInitialBursts(int32_t bursts) {
        mInitialBursts = bursts;
    }

    // Run the benchmark.
    int32_t runTest(int32_t sampleRate, int32_t framesPerBurst, int32_t numSeconds) override {
        std::stringstream resultMessage;
        int32_t result = measureLatencyInBursts(sampleRate, framesPerBurst, numSeconds);
        if (result < 0) {
            resultMessage << "ERROR in latency search = " << result << std::endl;
            mResult->appendMessage(resultMessage.str());
            mResult->setResultCode(SYNTHMARK_RESULT_UNRECOVERABLE_ERROR);
            mResult->setMeasurement(0);
            return result;
        } else if (result >= BURSTS_OVER_RANGE) {
            resultMessage << "ERROR - latency was too high to measure" << std::endl;
            mResult->appendMessage(resultMessage.str());
            mResult->setResultCode(SYNTHMARK_RESULT_OUT_OF_RANGE);
            mResult->setMeasurement(0);
            return SYNTHMARK_RESULT_OUT_OF_RANGE;
        }

        int32_t latencyBursts = result;
        int32_t sizeFrames = latencyBursts * framesPerBurst;
        double latencyMsec = 1000.0 * sizeFrames / getSampleRate();

        resultMessage << "frames.per.burst     = " << getFramesPerBurst() << std::endl;
        resultMessage << "# Latency values apply only to the top level buffer." << std::endl;
        resultMessage << "audio.latency.bursts = " << latencyBursts << std::endl;
        resultMessage << "audio.latency.frames = " << sizeFrames << std::endl;
        resultMessage << "audio.latency.msec   = " << latencyMsec << std::endl;
        mResult->appendMessage(resultMessage.str());
        mResult->setResultCode(SYNTHMARK_RESULT_SUCCESS);
        mResult->setMeasurement((double) sizeFrames);
        return 0;
    }

private:
    /**
     *
     * @param sampleRate
     * @param framesPerBurst
     * @param maxSeconds
     * @return latency in bursts or a negative error code or BURSTS_OVER_RANGE
     */
    int32_t measureLatencyInBursts(int32_t sampleRate, int32_t framesPerBurst, int32_t maxSeconds) {
        int32_t testCount = 1;
        bool glitched = true;
        int32_t bursts = mInitialBursts;
        int32_t numSeconds = std::min(maxSeconds, 10);
        mLogTool.log("LatencyMark: #%d, %d seconds with bursts = %d ----\n",
                      testCount++, numSeconds, bursts);
        int32_t desiredSizeInFrames = bursts * getFramesPerBurst();
        int32_t actualSize = mAudioSink->setBufferSizeInFrames(desiredSizeInFrames);
        if (actualSize < desiredSizeInFrames) {
            mLogTool.log("WARNING - requested buffer size %d, got %d\n",
                         desiredSizeInFrames, actualSize);
        }
        fflush(stdout);

        mAudioSink->setUnderrunCount(0);
        int32_t err = ChangingVoiceHarness::runTest(sampleRate, framesPerBurst, numSeconds);
        if (err < 0) {
            mLogTool.log("LatencyMark: %s returning err = %d ----\n",  __func__, err);
            return err;
        }
        glitched = (mAudioSink->getUnderrunCount() > 0);
        if (!glitched) {
            mLogTool.log("LatencyMark: no glitches\n");
        }

        // Determine number of bursts needed to prevent the biggest under-run.
        int32_t measuredBursts = calculateRequiredLatencyBursts(framesPerBurst);

        return measuredBursts;
    }
/*
    int32_t measureOnce(int32_t sampleRate,
                        int32_t framesPerBurst,
                        int32_t numSeconds) {
    }
*/
private:
    int32_t           mInitialBursts = kDefaultBufferSizeBursts;
};

#endif // SYNTHMARK_LATENCYMARK_HARNESS_H
