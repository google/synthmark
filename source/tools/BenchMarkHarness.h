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

#ifndef SYNTHMARK_BENCHMARK_HARNESS_H
#define SYNTHMARK_BENCHMARK_HARNESS_H

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
 * Play a specified number of voices on a Synthesizer
 * as fast as possible and measure the time it takes.
 */
class BenchMarkHarness : public TestHarnessBase {
public:
    BenchMarkHarness(AudioSinkBase *audioSink, SynthMarkResult *result, LogTool &logTool)
            : TestHarnessBase(audioSink, result, logTool)
    {
        std::stringstream testName;
        testName << "BenchMark";
        mTestName = testName.str();
        audioSink->setRealTime(false); // run as fast as possible
    }

    virtual ~BenchMarkHarness() {
    }

    virtual void onBeginMeasurement() override {
        mResult->setTestName(mTestName);
        mLogTool.log("---- Starting %s ----\n", mTestName.c_str());
        mBeatCount = 0;
        mTestStartTimeNanos = HostTools::getNanoTime();
    }

    double reportSpeed() {
        double elapsedTimeSeconds = mTimer.getTotalTime() * SYNTHMARK_SECONDS_PER_NANO;
        int32_t framesWritten = getFramesWritten();
        double virtualTime = (framesWritten - mLastFramesWritten) / (double) getSampleRate();
        double fractionRealTime = elapsedTimeSeconds / virtualTime;
        mLogTool.log("%2d: %5.3f%% of real-time\n",
                     mBeatCount, fractionRealTime * 100);
        mLastFramesWritten = framesWritten;
        return fractionRealTime;
    }

    virtual int32_t onBeforeNoteOn() override {
        if (mBeatCount > 0) {
            double fractionRealTime = reportSpeed();
            mStableCount += (fractionRealTime > (mMinFractionRealTime * 0.99)) ? 1 : 0;
            mMinFractionRealTime = std::min(mMinFractionRealTime, fractionRealTime);
        }
        mTimer.reset();
        mBeatCount++;
        int64_t now = HostTools::getNanoTime();
        bool timeOut = (now - mTestStartTimeNanos) > (5 * SYNTHMARK_NANOS_PER_SECOND);
        return ((mStableCount < 2) && !timeOut)  ? 0 : 1;
    }

    int32_t getMaxVoices() {
        return (int32_t) (getNumVoices() / mMinFractionRealTime);
    }

    virtual void onEndMeasurement() override {
        int8_t resultCode = SYNTHMARK_RESULT_SUCCESS;
        std::stringstream resultMessage;
        std::stringstream cpuPart;
        if (mAudioSink->getActualCpu() >= 0) {
            cpuPart << ".cpu." << mAudioSink->getActualCpu();
        }
        resultMessage << "percent.realtime" << cpuPart.str()
                << ".voices." << getNumVoices()
                << " = " <<  (mMinFractionRealTime * 100) << "%%" << std::endl;
        resultMessage << "max.voices" << cpuPart.str()
                << " = " << getMaxVoices() << std::endl;
        mResult->setResultCode(resultCode);
        resultMessage << mCpuAnalyzer.dump();

        mResult->setMeasurement(mMinFractionRealTime);
        mResult->appendMessage(resultMessage.str());
    }

private:
    double  mMinFractionRealTime = 1.0e10; // start very high
    int32_t mBeatCount = 0;
    int32_t mLastFramesWritten = 0;
    int32_t mStableCount = 0;
    int64_t mTestStartTimeNanos = 0;
};

#endif //SYNTHMARK_BENCHMARK_HARNESS_H
