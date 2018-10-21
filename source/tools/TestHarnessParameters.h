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

#ifndef ANDROID_TEST_HARNESS_PARAMETERS_H
#define ANDROID_TEST_HARNESS_PARAMETERS_H

#include <cstdint>
#include "AudioSinkBase.h"
#include "BinCounter.h"
#include "HostTools.h"
#include "IAudioSinkCallback.h"
#include "SynthMarkResult.h"
#include "synth/Synthesizer.h"
#include "tools/CpuAnalyzer.h"
#include "tools/LogTool.h"
#include "tools/ITestHarness.h"
#include "tools/TimingAnalyzer.h"
#include "tools/TestHarnessBase.h"
#include "HostThreadFactory.h"

class TestHarnessParameters : public ITestHarness {

public:
    TestHarnessParameters(AudioSinkBase *audioSink,
                          SynthMarkResult *result,
                          LogTool *logTool = nullptr)
    : mAudioSink(audioSink)
    , mResult(result)
    , mLogTool(logTool) {
        setNumVoices(kSynthmarkNumVoicesLatency);
        if (!logTool) {
            mLogTool = new LogTool(this);
        } else {
            mLogTool = logTool;
        }
    }

    virtual ~TestHarnessParameters() {
        if (mLogTool && mLogTool->getOwner() == this) {
            delete(mLogTool);
            mLogTool = nullptr;
        }
    }

    void setNumVoices(int32_t numVoices) override {
        mNumVoices = numVoices;
    }

    int32_t getNumVoices() {
        return mNumVoices;
    }

    HostThreadFactory::ThreadType getThreadType() const {
        return mThreadType;
    }

    void setThreadType(HostThreadFactory::ThreadType threadType) override {
        mThreadType = threadType;
    }

    void setDelayNoteOnSeconds(int32_t delayNotesOn) override {
        mDelayNotesOn = delayNotesOn;
    }

    void setNumVoicesHigh(int32_t numVoicesHigh) {
        mNumVoicesHigh = numVoicesHigh;
    }

    int32_t getNumVoicesHigh() {
        return mNumVoicesHigh;
    }

    SynthMarkResult *getResult() {
        return mResult;
    }

protected:
    int32_t          mNumVoices = 8;
    int32_t          mDelayNotesOn = 0;
    int32_t          mNumVoicesHigh = 0;

    AudioSinkBase   *mAudioSink = nullptr;
    SynthMarkResult *mResult = nullptr;
    LogTool         *mLogTool = nullptr;

    HostThreadFactory::ThreadType mThreadType = HostThreadFactory::ThreadType::Audio;
};

#endif //ANDROID_TEST_HARNESS_PARAMETERS_H
