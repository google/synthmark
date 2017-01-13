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
#include <sstream>
#include <math.h>
#include "SynthMark.h"
#include "synth/Synthesizer.h"
#include "tools/TimingAnalyzer.h"
#include "AudioSinkBase.h"
#include "tools/LogTool.h"
#include "tools/TestHarnessBase.h"

#ifndef SYNTHMARK_LATENCYMARK_HARNESS_H
#define SYNTHMARK_LATENCYMARK_HARNESS_H

#define NOTES_PER_STEP   10

/**
 * Determine buffer latency required to avoid glitches.
 * The "LatencyMark" is the minimum buffer size that is a multiple
 * of a burst size that can be used for N minutes without glitching.
 */
class LatencyMarkHarness : public TestHarnessBase {
public:
    LatencyMarkHarness(AudioSinkBase *audioSink,
                       SynthMarkResult *result,
                       LogTool *logTool = NULL)
    : TestHarnessBase(audioSink, result, logTool)
    {
        mTestName = "LatencyMark";
        mNumVoices = SYNTHMARK_NUM_VOICES_LATENCY;
    }

    virtual ~LatencyMarkHarness() {
    }

    virtual void onBeginMeasurement() override {
        mPreviousUnderrunCount = 0;
        mAudioSink->setBufferSizeInFrames(mFramesPerBurst);
        mLogTool->log("---- Measure latency ---- #voices = %d\n", mNumVoices);
    }

    virtual int32_t onBeforeNoteOn() override {
        if (mTimer.getActiveTime() > 0) {
            int32_t underruns = mAudioSink->getUnderrunCount();
            if (underruns > mPreviousUnderrunCount) {
                mPreviousUnderrunCount = underruns;
                // Increase latency to void glitches.
                int32_t sizeInFrames = mAudioSink->getBufferSizeInFrames();
                int32_t desiredSizeInFrames = sizeInFrames + mFramesPerBurst;
                int32_t actualSize = mAudioSink->setBufferSizeInFrames(desiredSizeInFrames);
                if (actualSize < desiredSizeInFrames) {
                    mLogTool->log("ERROR - at maximum buffer size and still glitching\n");
                    return -1;
                }
                // Reset clock so we get a full run without glitches.
                float glitchTime = ((float)mFrameCounter / mSampleRate);
                mLogTool->log("Audio glitch at %.2fs, "
                              "restarting test with buffer size %d = %d * %d\n",
                              glitchTime, actualSize,
                              actualSize / mFramesPerBurst, mFramesPerBurst);
                mFrameCounter = 0;
            }
        }
        return 0;
    }

    /**
     * calculate the final size in frames of the output buffer
     */
    virtual void onEndMeasurement() override {
        int32_t sizeFrames = mAudioSink->getBufferSizeInFrames();
        double latencyMsec = 1000.0 * sizeFrames / mSampleRate;

        std::stringstream resultMessage;
        resultMessage << sizeFrames << " = " << latencyMsec << " msec at burst size " <<
                mFramesPerBurst << " frames"
                << std::endl;
        mResult->setResultMessage(resultMessage.str());
        mResult->setResultCode(SYNTHMARK_RESULT_SUCCESS);
        mResult->setMeasurement((double) sizeFrames);
    }


    void setNumVoicesHigh(int32_t numVoicesHigh) {
        mNumVoicesHigh = numVoicesHigh;
    }

    int32_t getNumVoicesHigh() {
        return mNumVoicesHigh;
    }

    int32_t getCurrentNumVoices() override {
        // Toggle back and forth between high and low
        if (mNumVoicesHigh > 0) {
            return ((getNoteCounter() % NOTES_PER_STEP) < (NOTES_PER_STEP / 2)) ?
                    mNumVoices : mNumVoicesHigh;
        } else {
            return mNumVoices;
        }
    }

    int32_t mPreviousUnderrunCount;

private:

    int32_t        mNumVoicesHigh = 0;
};

#endif // SYNTHMARK_LATENCYMARK_HARNESS_H
