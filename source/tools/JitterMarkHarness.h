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

#include <cstdint>
#include <math.h>
#include "SynthMark.h"
#include "synth/Synthesizer.h"
#include "tools/TimingAnalyzer.h"
#include "AudioSinkBase.h"
#include "tools/LogTool.h"
#include "tools/TestHarnessBase.h"

#ifndef SYNTHMARK_JITTERMARK_HARNESS_H
#define SYNTHMARK_JITTERMARK_HARNESS_H

// TODO increase resolution when we have a nice chart to show the histogram
#define JITTER_BINS_PER_MSEC  4
#define JITTER_MAX_MSEC       100
/**
 * Determine buffer latency required to avoid glitches.
 * The "LatencyMark" is the minimum buffer size that is a multiple
 * of a block size that can be used for N minutes without glitching.
 */
class JitterMarkHarness : public TestHarnessBase {
public:
    JitterMarkHarness(AudioSinkBase *audioSink, SynthMarkResult *result, LogTool *logTool = NULL)
    : TestHarnessBase(audioSink, result, logTool)
    , mNanosPerBin(1)
    {
        mTestName = "JitterMark";
    }

    virtual ~JitterMarkHarness() {
    }

    virtual void onBeginMeasurement() override {
        mResult->setTestName(mTestName);
        mLogTool->log("---- Measure scheduling jitter ---- #voices = %d\n", mNumVoices);
        int32_t blocksPerBurst = 8;
        int32_t desiredSizeInFrames = blocksPerBurst * mFramesPerBurst;
        int32_t actualSizeInFrames = mAudioSink->setBufferSizeInFrames(desiredSizeInFrames);
        if (actualSizeInFrames < desiredSizeInFrames) {
            mLogTool->log("WARNING - could not set desired buffer size\n");
        }
        // set resolution and size of histogram
        int32_t nanosPerMilli = (int32_t)(SYNTHMARK_NANOS_PER_SECOND / SYNTHMARK_MILLIS_PER_SECOND);
        mNanosPerBin = nanosPerMilli / JITTER_BINS_PER_MSEC;
        int32_t numBins = JITTER_MAX_MSEC * JITTER_BINS_PER_MSEC;
        mTimer.setupJitterRecording(mNanosPerBin, numBins);
    }

    virtual int32_t onBeforeNoteOn() override {
        return 0;
    }

    virtual void onEndMeasurement() override {

        double measurement = 0.0;
        std::stringstream resultMessage;
        resultMessage << mTestName << " = " << measurement;

        printJitterBins();
        mResult->setMeasurement(measurement);
        mResult->setResultMessage(resultMessage.str());
    }

    void printJitterBins() {
        int32_t numBins = mTimer.getNumBins();
        int32_t *bins = mTimer.getBins();
        int32_t *lastMarkers = mTimer.getLastMarkers();
        mLogTool->log("bin#, msec, count, last\n");
        for (int i = 0; i < numBins; i++) {
            if (bins[i] > 0) {
                double msec = (double) i * mNanosPerBin * SYNTHMARK_MILLIS_PER_SECOND
                              / SYNTHMARK_NANOS_PER_SECOND;
                mLogTool->log("%3d, %5.2f, %6d, %6d\n", i, msec, bins[i], lastMarkers[i]);
            }
        }
    }

    int32_t mNanosPerBin;

};


#endif // SYNTHMARK_JITTERMARK_HARNESS_H

