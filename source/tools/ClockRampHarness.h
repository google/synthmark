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

#ifndef SYNTHMARK_CLOCKRAMP_HARNESS_H
#define SYNTHMARK_CLOCKRAMP_HARNESS_H

#include <cmath>
#include <cstdint>
#include <iomanip>

#include "AudioSinkBase.h"
#include "ChangingVoiceHarness.h"
#include "SynthMark.h"
#include "synth/Synthesizer.h"
#include "TestHarnessParameters.h"
#include "tools/CpuAnalyzer.h"
#include "tools/LogTool.h"
#include "tools/TestHarnessBase.h"
#include "tools/TimingAnalyzer.h"

/**
 * Measure the time it takes for the CPU clock frequency to ramp up after a change in workload.
 *
 * This is measured indirectly.
 * Detect when the render time is higher than real-time, that is > 100% CPU utilization.
 * Then measure how long it takes to return to real-time.
 */
class ClockRampHarness : public ChangingVoiceHarness {
public:
    ClockRampHarness(AudioSinkBase *audioSink, SynthMarkResult *result, LogTool &logTool)
            : ChangingVoiceHarness(audioSink, result, logTool) {
        mTestName = "ClockRamp";
    }

    virtual ~ClockRampHarness() {
    }

    void onBeginMeasurement() override {
        mResult->setTestName(mTestName);
        mLogTool.log("---- Measure clock ramp ---- #voices = %d => %d\n",
            getNumVoices(), getNumVoicesHigh());
        setupHistograms();

        mNanosPerBurst = mAudioSink->getFramesPerBurst() * SYNTHMARK_NANOS_PER_SECOND
                /  mAudioSink->getSampleRate() ;
        mBurstsPerNanosecond = 1.0 / mNanosPerBurst; // avoid dividing for every burst
        mRampDurationSum = 0;
        mRampDurationCount = 0;
        mPreviousVoiceCount = getNumVoices();
    }

    int32_t onBeforeNoteOn() override {
        return 0;
    }

    double getBurstUtilization() {
        return (double) mTimer.getLastRenderDurationNanos() * mBurstsPerNanosecond;

    }

    IAudioSinkCallback::Result onRenderAudio(float *buffer,
                                                     int32_t numFrames) override {
        IAudioSinkCallback::Result result = ChangingVoiceHarness::onRenderAudio(buffer, numFrames);

        double utilization = getBurstUtilization();

        // State machine for analysing clock ramp.
        switch(mState) {
            case STATE_LOW:
                // Did we increase the workload?
                if (mSynth.getActiveVoiceCount() > mPreviousVoiceCount) {
                    double highUtilization = mSynth.getActiveVoiceCount() * mPreviousUtilization
                            / mPreviousVoiceCount;

                    if (mPreviousUtilization > kUtilizationThresholdHigh) {
                        // low voices, -n, too high
                        mState = STATE_TOO_HIGH;
                        mLogTool.log("LOW => STATE_TOO_HIGH: voices = %d, prevUtilization = %f\n",
                                      getNumVoices(), mPreviousUtilization);
                    } else if (highUtilization < kUtilizationThresholdHigh) {
                        // high voices, -N, too low
                        mState = STATE_TOO_LOW;
                        mLogTool.log("LOW => TOO_LOW: voices = %d, highUtilization = %f\n",
                                      mSynth.getActiveVoiceCount(), highUtilization);
                    } else if (utilization > kUtilizationThresholdHigh) {
                        // start a valid measurement
                        mState = STATE_SATURATED;
                        mJumpTimeNanos = mTimer.getLastEntryTime();
                        mLogTool.log("LOW => SATURATED: voices = %d, utilization = %f\n",
                                      mSynth.getActiveVoiceCount(), utilization);
                    } else {
                        // Clock must have ramped up immediately! That's good.
                        mState = STATE_HIGH;
                        mRampDurationSum += 0.0;
                        mRampDurationCount++;
                        mLogTool.log("LOW => HIGH, utilization = %f, immediate ramp up\n",
                                      utilization);
                    }
                }
                break;

            case STATE_SATURATED:
                // If we are at the end of the high work load then it took too long to ramp up.
                if (mSynth.getActiveVoiceCount() < mPreviousVoiceCount) {
                    mState = STATE_SLOW;
                } else {
                    // Is the clock running fast enough for real-time?
                    // Note that we may still be catching up on late data delivery.
                    if (utilization < kUtilizationThresholdLow) {
                        mState = STATE_HIGH;
                        int32_t rampNanos = (int32_t)(mTimer.getLastEntryTime() - mJumpTimeNanos);
                        mRampDurationSum += rampNanos;
                        mRampDurationCount++;
                        int32_t rampMicros = rampNanos / SYNTHMARK_NANOS_PER_MICROSECOND;
                        mLogTool.log("SATURATED => HIGH, utilization = %f, ramp(us) = %d\n",
                                      utilization, (int)rampMicros);
                    }
                }
                break;

            case STATE_HIGH:
                if (mSynth.getActiveVoiceCount() < mPreviousVoiceCount) {
                    mState = STATE_LOW;
                    mLogTool.log("HIGH => LOW\n");
                }
                break;

            case STATE_SLOW:
            case STATE_TOO_LOW:
            case STATE_TOO_HIGH:
                result = IAudioSinkCallback::Finished;
            default:
                break;

        }

        mPreviousVoiceCount = mSynth.getActiveVoiceCount();
        mPreviousUtilization = utilization;

        return result;
    }

    void onEndMeasurement() override {
        std::stringstream resultMessage;

        resultMessage << dumpJitter();

        resultMessage << mTestName;
        if (mState == STATE_SLOW) {
            resultMessage << " FAIL - Clock ramp duration too long or high voice num "
                    <<  getNumVoicesHigh() << " too high." << std::endl;
        } else if (mState == STATE_TOO_HIGH) {
            resultMessage << " FAIL - Low voice num " << getNumVoices()
                          << " was too much for CPU." << std::endl;
        } else if (mState == STATE_TOO_LOW || mRampDurationCount == 0) {
            resultMessage << " FAIL - Never saturated the CPU. Difference in voices was too low."
                          << std::endl;
        } else {
            resultMessage << " valid" << std::endl;
            double averageRampMillis = ((double) mRampDurationSum)
                                        / (mRampDurationCount * SYNTHMARK_NANOS_PER_MILLISECOND);
            mResult->setMeasurement(averageRampMillis);
            resultMessage << "clock.ramp.msec = " << averageRampMillis << std::endl;
        }

        resultMessage << "underrun.count = " << mAudioSink->getUnderrunCount() << "\n";
        resultMessage << mCpuAnalyzer.dump();

        mResult->appendMessage(resultMessage.str());
    }

private:
    enum states_t {
        STATE_LOW,        // low num voices
        STATE_SATURATED,  // high num voices, running at saturated CPU
        STATE_HIGH,       // high num voices but keeping up with workload
        STATE_SLOW,        // did not ramp up in time
        STATE_TOO_LOW,     // high voices not high enough
        STATE_TOO_HIGH     // low voices too high
    };

    double      mBurstsPerNanosecond = 0.0;
    double      mPreviousUtilization = 0.0;
    int64_t     mNanosPerBurst = 0;
    int64_t     mJumpTimeNanos;
    int64_t     mRampDurationSum = 0;
    int32_t     mRampDurationCount = 0;
    int32_t     mPreviousVoiceCount = 0;
    states_t    mState = STATE_LOW;

    // Trigger points with hysteresis.
    static constexpr double kUtilizationThresholdLow = 0.95;
    static constexpr double kUtilizationThresholdHigh = 1.0;
};

#endif // SYNTHMARK_CLOCKRAMP_HARNESS_H
