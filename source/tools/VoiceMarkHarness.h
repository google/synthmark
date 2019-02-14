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

#ifndef SYNTHMARK_VOICEMARK_HARNESS_H
#define SYNTHMARK_VOICEMARK_HARNESS_H

#include <cmath>
#include <cstdint>
#include <sstream>

#include "AudioSinkBase.h"
#include "SynthMark.h"
#include "synth/Synthesizer.h"
#include "tools/LogTool.h"
#include "tools/TestHarnessBase.h"
#include "tools/TimingAnalyzer.h"
#include "TestHarnessParameters.h"


constexpr int kMinimumVoiceCount  = 4;
constexpr int kMinimumNoteOnCount = 1;

/**
 * Play notes on a Synthesizer and measure the number
 * of voices that consume a specified percentage of the CPU.
 */
class VoiceMarkHarness : public TestHarnessBase {
public:
    VoiceMarkHarness(AudioSinkBase *audioSink, SynthMarkResult *result, LogTool &logTool)
    : TestHarnessBase(audioSink, result, logTool)
    {
        std::stringstream testName;
        testName << "VoiceMark";
        mTestName = testName.str();
    }

    virtual ~VoiceMarkHarness() {
    }

    virtual void onBeginMeasurement() override {
        mResult->setTestName(mTestName);
        mLogTool.log("---- Starting %s ----\n", mTestName.c_str());

        float bufferSizeInMs = (float)
                        (mAudioSink->getBufferSizeInFrames() * SYNTHMARK_MILLIS_PER_SECOND)
                        / mAudioSink->getSampleRate();
        mLogTool.log("Buffer size: %.2fms\n", bufferSizeInMs);
        TestHarnessBase::setNumVoices(mInitialVoiceCount);
        mSumVoicesOn = 0;
        mSumVoicesCount = 0;
        mBeatCount = 0;
        mStable = false;
    }

    virtual int32_t onBeforeNoteOn() override {
        if (mBeatCount >= kMinimumNoteOnCount) {
            // Estimate how many voices it would take to use a fraction of the CPU.
            double cpuLoad = mTimer.getDutyCycle();
            int32_t oldNumVoices = getNumVoices();
            double voicesFraction = mFractionOfCpu * oldNumVoices / cpuLoad;
            int32_t newNumVoices = (int32_t)(voicesFraction + 0.5); // round
            if (newNumVoices > kSynthmarkMaxVoices) {
                mLogTool.log("measureSynthMark() - numVoices clipped to kSynthmarkMaxVoices\n");
                newNumVoices = kSynthmarkMaxVoices;
            }

            // Is this data point reliable?
            bool accepted = false;
            double lowerLimit = mFractionOfCpu * 0.5;
            double upperLimit = 1.0 - ((1.0 - mFractionOfCpu) * 0.5);
            if (cpuLoad >= lowerLimit && cpuLoad <= upperLimit) {
                // Only start accepting voices when it stops going up each time.
                if (!mStable && newNumVoices <= oldNumVoices) {
                    mStable = true;
                }
                if (mStable) {
                    mSumVoicesOn += voicesFraction;
                    mSumVoicesCount++;
                    accepted = true;
                }
            }
            mLogTool.log("%2d: %3d voices used %5.3f of CPU, %s\n",
                          mBeatCount, oldNumVoices, cpuLoad,
                          accepted ? "" : " - not used");
            TestHarnessBase::setNumVoices(newNumVoices);
        }
        mTimer.reset();
        mBeatCount++;
        return 0;
    }

    virtual void onEndMeasurement() override {

        int8_t resultCode = SYNTHMARK_RESULT_SUCCESS;
        double measurement = 0.0;
        std::stringstream resultMessage;

        if (mSumVoicesCount < kMinimumVoiceCount) {

            resultCode = SYNTHMARK_RESULT_TOO_FEW_MEASUREMENTS;
            resultMessage << "Only " << mSumVoicesCount << " measurements. Minimum is " <<
                    kMinimumVoiceCount << ". Not a valid result!"
                    << std::endl;

        } else {

            measurement = mSumVoicesOn / mSumVoicesCount;
            resultMessage << "Underruns = " << mAudioSink->getUnderrunCount() << std::endl;
            resultMessage << mTestName << "_"
                << ((int)(mFractionOfCpu * 100)) << " = " << measurement << std::endl;
            resultMessage << "normalized.voices.100 = "
                    << (measurement / mFractionOfCpu) << std::endl;
        }

        mResult->setResultCode(resultCode);

        resultMessage << mCpuAnalyzer.dump();

        mResult->setMeasurement(measurement);
        mResult->appendMessage(resultMessage.str());
    }

    /**
     * Fractional load, 0.5 would be 50% of one CPU.
     */
    void setTargetCpuLoad(double load) {
        mFractionOfCpu = load;
    }
    /**
     * Initial number of voices.
     */
    void setInitialVoiceCount(int numVoices) {
        mInitialVoiceCount = numVoices;
    }


private:
    double  mFractionOfCpu = 0.0;
    int32_t mInitialVoiceCount = 10;

    // These need to be reset before each measurement.
    double  mSumVoicesOn = 0;     // sum of fractional number of voices on
    int32_t mSumVoicesCount = 0;  // number of measurements for taking an average
    int32_t mBeatCount = 0;
    bool    mStable = false;
};

#endif // SYNTHMARK_VOICEMARK_HARNESS_H
