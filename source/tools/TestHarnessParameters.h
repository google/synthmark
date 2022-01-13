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
#include <thread>
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
#include "UtilClampController.h"

enum VoicesMode {
    VOICES_UNDEFINED,
    VOICES_SWITCH,
    VOICES_RANDOM,
    VOICES_LINEAR_LOOP,
};

class TestHarnessParameters : public ITestHarness {

public:
    TestHarnessParameters(AudioSinkBase *audioSink,
                          SynthMarkResult *result,
                          LogTool &logTool)
    : mAudioSink(audioSink)
    , mResult(result)
    , mLogTool(logTool) {
        setNumVoices(kSynthmarkNumVoicesLatency);
    }

    virtual ~TestHarnessParameters() = default;

    virtual int32_t runTest(int32_t sampleRate,
                                    int32_t framesPerBurst,
                                    int32_t numSeconds) { return SYNTHMARK_RESULT_SUCCESS; };

    int32_t runCompleteTest(int32_t sampleRate,
                            int32_t framesPerBurst,
                            int32_t numSeconds) override {
        mResult->appendMessage("\n" TEXT_RESULTS_BEGIN "\n");
        int32_t result = runTest(sampleRate, framesPerBurst, numSeconds);
        mResult->appendMessage(mAudioSink->dump());
        mResult->appendMessage(TEXT_RESULTS_END "\n");
        mRunning = false;
        return result;
    };


    void launch(int32_t sampleRate,
                           int32_t framesPerBurst,
                           int32_t numSeconds) override {
        mRunning = true;
        mTestThread = std::thread(&TestHarnessParameters::runCompleteTest, this,
                sampleRate, framesPerBurst, numSeconds);
    }

    bool isRunning() override {
        return mRunning;
    }

    void setNumVoices(int32_t numVoices) override {
        mNumVoices = numVoices;
    }

    int32_t getNumVoices() {
        return mNumVoices;
    }

    void setVoicesMode(VoicesMode voicesMode) {
        mVoicesMode = voicesMode;
    }

    VoicesMode getVoicesMode() {
        return mVoicesMode;
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

    int32_t getFramesPerBurst() {
        return mAudioSink->getFramesPerBurst();
    }

    int32_t getSampleRate() {
        return mAudioSink->getSampleRate();
    }

protected:
    int32_t          mNumVoices = 8;
    int32_t          mDelayNotesOn = 0;
    int32_t          mNumVoicesHigh = 0;

    VoicesMode       mVoicesMode = VOICES_SWITCH;

    AudioSinkBase   *mAudioSink = nullptr;
    SynthMarkResult *mResult = nullptr;
    LogTool         &mLogTool;
    std::thread      mTestThread;
    bool             mRunning;

    HostThreadFactory::ThreadType mThreadType = HostThreadFactory::ThreadType::Audio;
};

#endif //ANDROID_TEST_HARNESS_PARAMETERS_H
