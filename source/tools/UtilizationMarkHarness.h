/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef SYNTHMARK_UTILIZATION_MARK_HARNESS_H
#define SYNTHMARK_UTILIZATION_MARK_HARNESS_H

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


/**
 * Play a specified number of voices on a Synthesizer and
 * measure the percentage of the CPU that they consume.
 */
class UtilizationMarkHarness : public TestHarnessBase {
public:
    UtilizationMarkHarness(AudioSinkBase *audioSink, SynthMarkResult *result, LogTool &logTool)
            : TestHarnessBase(audioSink, result, logTool)
    {
        std::stringstream testName;
        testName << "UtilizationMark";
        mTestName = testName.str();
    }

    virtual ~UtilizationMarkHarness() {
    }

    virtual void onBeginMeasurement() override {

        mResult->setTestName(mTestName);
        mLogTool.log("---- Starting %s ----\n", mTestName.c_str());

        float bufferSizeInMs = (float)
                               (mAudioSink->getBufferSizeInFrames() * SYNTHMARK_MILLIS_PER_SECOND)
                               / mAudioSink->getSampleRate();
        mLogTool.log("Buffer size: %.2fms\n", bufferSizeInMs);
        mBeatCount = 0;
    }

    void reportUtilization() {
        mFractionOfCpu = mTimer.getDutyCycle();
        mLogTool.log("%2d: %3d voices used %5.3f of CPU\n", mBeatCount, getNumVoices(), mFractionOfCpu);
    }

    virtual int32_t onBeforeNoteOn() override {
        if (mBeatCount > 0) {
            reportUtilization();
        }
        mTimer.reset();
        mBeatCount++;
        return 0;
    }

    virtual void onEndMeasurement() override {

        int8_t resultCode = SYNTHMARK_RESULT_SUCCESS;
        std::stringstream resultMessage;

        reportUtilization();
        double measurement = mFractionOfCpu;
        resultCode = SYNTHMARK_RESULT_SUCCESS;
        resultMessage << "underrun.count = " << mAudioSink->getUnderrunCount() << std::endl;
        resultMessage << mTestName << " = " << measurement << std::endl;

        resultMessage << "normalized.voices.100 = " << (getNumVoices() / mFractionOfCpu) << std::endl;
        mResult->setResultCode(resultCode);

        resultMessage << mCpuAnalyzer.dump();

        mResult->setMeasurement(measurement);
        mResult->appendMessage(resultMessage.str());
    }

private:
    double  mFractionOfCpu = 0.0;
    int32_t mBeatCount = 0;
};

#endif //SYNTHMARK_UTILIZATION_MARK_HARNESS_H
