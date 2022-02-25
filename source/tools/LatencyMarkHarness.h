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
 * Measure the wakeup time and render time for each wakeup period.
 */
class StopOnGlitchHarness : public ChangingVoiceHarness {
public:
    StopOnGlitchHarness(AudioSinkBase *audioSink, SynthMarkResult *result, LogTool &logTool)
            : ChangingVoiceHarness(audioSink, result, logTool) {
        mTestName = "StopOnGlitch";
    }

    IAudioSinkCallback::Result onRenderAudio(float *buffer,
                                             int32_t numFrames) override {
        IAudioSinkCallback::Result result = ChangingVoiceHarness::onRenderAudio(buffer, numFrames);
        if (mAudioSink->getUnderrunCount() > 0) {
             result = IAudioSinkCallback::Finished;
        }
        return result;
    }
};

/**
 * Determine buffer latency required to avoid glitches.
 * The "LatencyMark" is the minimum buffer size that is a multiple
 * of a burst size that can be used for N minutes without glitching.
 */
class LatencyMarkHarness : public TestHarnessParameters {
public:
    LatencyMarkHarness(AudioSinkBase *audioSink, SynthMarkResult *result, LogTool &logTool)
    : TestHarnessParameters(audioSink, result, logTool) {
        resetBinarySearch();
    }

    const char *getName() const override {
        return "LatencyMark";
    }

    // Run the benchmark.
    int32_t runTest(int32_t sampleRate, int32_t framesPerBurst, int32_t numSeconds) override {
        std::stringstream resultMessage;
        if (mInitialBursts > 0) {
            resetLinearSearch();
        } else {
            resetBinarySearch();
        }
        int32_t result = searchForLowestLatencyInBursts(sampleRate, framesPerBurst, numSeconds);
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

    int32_t testSearchSeries() {
        for (int i=1; i<200; i++) {
            int latency = testSearch(i);
            if (latency != i) {
                mLogTool.log("ERROR expected %d, got %d\n", i, latency);
                return 1;
            }
        }
        return 0;
    }

    /**
     *
     * @param sampleRate
     * @param framesPerBurst
     * @param maxSeconds
     * @return latency in bursts or a negative error code or BURSTS_OVER_RANGE
     */
    int32_t searchForLowestLatencyInBursts(int32_t sampleRate, int32_t framesPerBurst, int32_t maxSeconds) {
        int32_t testCount = 1;
        resetBinarySearch();
        int32_t bursts = getNextBurstsToTry(true); // assume glitches at zero latency
        while (bursts > 0) {
            int32_t numSeconds = (mState == STATE_VERIFY) ? maxSeconds : std::min(maxSeconds, 10);
            mLogTool.log("LatencyMark: #%d, %d seconds with bursts = %d ----\n",
                          testCount++, numSeconds, bursts);
            int32_t desiredSizeInFrames = bursts * framesPerBurst;
            int32_t actualSize = mAudioSink->setBufferSizeInFrames(desiredSizeInFrames);
            if (actualSize < desiredSizeInFrames) {
                mLogTool.log("ERROR - requested buffer size %d, got %d, still glitching\n",
                             desiredSizeInFrames, actualSize);
                break;
            }
            fflush(stdout);

            int32_t err = measureOnce(sampleRate, framesPerBurst, numSeconds);
            if (err < 0) {
                mLogTool.log("LatencyMark: %s returning err = %d ----\n",  __func__, err);
                return err;
            }
            bool glitched = (mAudioSink->getUnderrunCount() > 0);
            if (!glitched) {
                mLogTool.log("LatencyMark: no glitches\n");
            }
            bursts = getNextBurstsToTry(glitched);
        }

        return mLowestGoodBursts;
    }

    int32_t measureOnce(int32_t sampleRate,
                        int32_t framesPerBurst,
                        int32_t numSeconds) {
        std::stringstream resultMessage;
        SynthMarkResult result1;
        mAudioSink->setUnderrunCount(0);
        StopOnGlitchHarness harness(mAudioSink, &result1, mLogTool);
        harness.setNumVoices(getNumVoices());
        harness.setNumVoicesHigh(getNumVoicesHigh());
        harness.setVoicesMode(getVoicesMode());
        harness.setDelayNoteOnSeconds(mDelayNotesOn);
        harness.setThreadType(mThreadType);

        int32_t err = harness.runTest(sampleRate, framesPerBurst, numSeconds);

        if (mAudioSink->getUnderrunCount() > 0) {
            // Record when the glitch occurred.
            float glitchTime = ((float) harness.getFrameCount() / mAudioSink->getSampleRate());
            mLogTool.log("LatencyMark: detected glitch at %5.2f seconds\n", glitchTime);
            fflush(stdout);
        }

        return err;
    }

    void setInitialBursts(int32_t bursts) {
        mInitialBursts = bursts;
    }

    void resetLinearSearch() {
        mCurrentBursts = mInitialBursts;
        mState = STATE_VERIFY;
    }

    void resetBinarySearch() {
        mCurrentBursts = 0;
        mHighestBadBursts = 0;
        mLowestGoodBursts = BURSTS_OVER_RANGE;
        mPowerOf2 = 1;
        mState = STATE_RAMP_UP;
    }

    int32_t testSearch(int32_t target) {
        resetBinarySearch();
        int32_t bursts = getNextBurstsToTry(true); // assume zero latency glitches
        while (bursts > 0) {
            mLogTool.log("%2d, ", bursts);
            bool glitched = (bursts < target);
            bursts = getNextBurstsToTry(glitched);
        }
        mLogTool.log("\n");
        fflush(stdout);
        return mLowestGoodBursts;
    }

    // Determine number of bursts for next test.
    // It is faster to glitch and then try again then to not glitch and run the whole test.
    // So prefer burst sizes that will glitch.
    int32_t getNextBurstsToTry(bool glitched) {
        // Determine next state.
        switch(mState) {
            case STATE_RAMP_UP:
            case STATE_BOUNDED:
                if (glitched) {
                    mHighestBadBursts = mCurrentBursts;
                } else {
                    mLowestGoodBursts = mCurrentBursts;
                    if (mLowestGoodBursts == mHighestBadBursts + 1) {
                        mState = STATE_VERIFY;
                    } else {
                        mDelta = std::max(1, (mLowestGoodBursts - mHighestBadBursts) / 8);
                        mState = STATE_BOUNDED;
                    }
                }
                break;

            case STATE_VERIFY:
                if (glitched) {
                    mHighestBadBursts = mCurrentBursts;
                    mLowestGoodBursts = mHighestBadBursts + 1;
                } else {
                    mState = STATE_DONE;
                }

            case STATE_DONE:
                break;
        }

        // Determine number of bursts for next test.
        int32_t candidate = 0;
        switch(mState) {
            case STATE_RAMP_UP:
                candidate = mPowerOf2;
                mPowerOf2 *= 2;
                break;

            case STATE_BOUNDED:
                candidate = mHighestBadBursts + mDelta;
                break;

            case STATE_VERIFY:
                candidate = mLowestGoodBursts;
                break;

            case STATE_DONE:
                candidate = 0;
                break;
        }
        mCurrentBursts = candidate;
        return mCurrentBursts;
    }

    enum state_search_t {
        STATE_RAMP_UP,
        STATE_BOUNDED,
        STATE_VERIFY,
        STATE_DONE,
    };

    int32_t           mCurrentBursts;
    int32_t           mInitialBursts = 0;
    int32_t           mHighestBadBursts;
    int32_t           mLowestGoodBursts;
    int32_t           mDelta;
    int32_t           mPowerOf2;
    state_search_t    mState = STATE_RAMP_UP;
};

#endif // SYNTHMARK_LATENCYMARK_HARNESS_H
