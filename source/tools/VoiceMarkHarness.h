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

#include <cstdint>
#include <sstream>
#include <math.h>
#include "SynthMark.h"
#include "synth/Synthesizer.h"
#include "tools/TimingAnalyzer.h"
#include "AudioSinkBase.h"
#include "tools/LogTool.h"
#include "tools/TestHarnessBase.h"


#define SYNTHMARK_MINIMUM_VOICE_COUNT 4

/**
 * Play notes on a Synthesizer and measure the number
 * of voices that consume 50% of the CPU.
 */
class VoiceMarkHarness : public TestHarnessBase {
public:
    VoiceMarkHarness(AudioSinkBase *audioSink,
                     SynthMarkResult *result,
                     LogTool *logTool = NULL)
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
        mLogTool->log("---- Starting %s ----\n", mTestName.c_str());

        float bufferSizeInMs = (float) (mAudioSink->getBufferSizeInFrames() * SYNTHMARK_MILLIS_PER_SECOND)
                        / mAudioSink->getSampleRate();
        mLogTool->log("Buffer size: %.2fms\n", bufferSizeInMs);
        mNumVoices = mInitialVoiceCount;
        sumVoicesOn = 0;
        sumVoicesCount = 0;
        mStable = false;
    }

    virtual int32_t onBeforeNoteOn() override {
        if (mTimer.getActiveTime() > 0) {
            // Estimate how many voices it would take to use a fraction of the CPU.
            double cpuLoad = mTimer.getDutyCycle();
            int32_t oldNumVoices = mNumVoices;
            double voicesFraction = mFractionOfCpu * oldNumVoices / cpuLoad;
            int32_t newNumVoices = (int32_t)(voicesFraction + 0.5); // round
            if (newNumVoices > SYNTHMARK_MAX_VOICES) {
                mLogTool->log("measureSynthMark() - numVoices clipped to SYNTHMARK_MAX_VOICES\n");
                newNumVoices = SYNTHMARK_MAX_VOICES;
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
                    sumVoicesOn += voicesFraction;
                    sumVoicesCount++;
                    accepted = true;
                }
            }
            mLogTool->log("%3d voices used %5.3f of CPU, %s\n", oldNumVoices, cpuLoad,
                          accepted ? "" : " - not used");
            mTimer.reset();
            mNumVoices = newNumVoices;
        }
        return 0;
    }

    virtual void onEndMeasurement() override {

        int8_t resultCode;
        double measurement = 0.0;
        std::stringstream resultMessage;

        if (sumVoicesCount < SYNTHMARK_MINIMUM_VOICE_COUNT) {

            resultCode = SYNTHMARK_RESULT_TOO_FEW_MEASUREMENTS;
            resultMessage << "Only " << sumVoicesCount << " measurements. Minimum is " <<
                    SYNTHMARK_MINIMUM_VOICE_COUNT << ". Not a valid result!"
                    << std::endl;

        } else {

            double measurement = sumVoicesOn / sumVoicesCount;
            resultCode = SYNTHMARK_RESULT_SUCCESS;
            resultMessage << "Underruns " << mAudioSink->getUnderrunCount() << "\n";
            resultMessage << mTestName << "_"
                << ((int)(mFractionOfCpu * 100)) << " = " << measurement;
            resultMessage << ", normalized to 100% = "
                    << (measurement / mFractionOfCpu) << std::endl;
        }

        mResult->setResultCode(resultCode);
        mResult->setMeasurement(measurement);
        mResult->setResultMessage(resultMessage.str());
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

    double sumVoicesOn = 0;
    int32_t sumVoicesCount = 0;
    double mFractionOfCpu = SYNTHMARK_TARGET_CPU_LOAD;

private:
    bool mStable = false;
    int32_t mInitialVoiceCount = 10;
};


#endif // SYNTHMARK_VOICEMARK_HARNESS_H

