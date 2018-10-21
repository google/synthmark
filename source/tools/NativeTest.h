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

#ifndef ANDROID_NATIVETEST_H
#define ANDROID_NATIVETEST_H

#define HOST_IS_APPLE  defined(__APPLE__)

#include <string>
#include <sstream>

#include "../SynthMark.h"
#include "../synth/Synthesizer.h"
#include "JitterMarkHarness.h"
#include "LatencyMarkHarness.h"
#include "LogTool.h"
#include "Params.h"
#include "TimingAnalyzer.h"
#include "VirtualAudioSink.h"
#include "VoiceMarkHarness.h"
#include "TestHarnessParameters.h"
#include "UtilizationMarkHarness.h"
#include "ClockRampHarness.h"

#define NATIVETEST_SUCCESS 0
#define NATIVETEST_ERROR -1

#define NATIVETEST_STATUS_ERROR -1
#define NATIVETEST_STATUS_UNDEFINED 0
#define NATIVETEST_STATUS_READY 1
#define NATIVETEST_STATUS_RUNNING 2
#define NATIVETEST_STATUS_COMPLETED 3

#define DEFAULT_TEST_SAMPLING_RATES {8000, 11025, 16000, 22050, 44100, 48000, 96000}
#define DEFAULT_TEST_FRAMES_PER_BURST {8, 16, 32, 48, 64, 96, 128, 192, 256, 384, 512, 1024}
#define DEFAULT_TEST_DURATIONS { 1, 2, 5, 10, 15, 20, 25, 30, 45, 60, 90, 120, 180, 240, 300, \
    600, 1200, 1800, 2400, 3600}
#define DEFAULT_TEST_TARGET_CPU_LOADS {0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5, \
    0.55, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0}

#define DEFAULT_CORE_AFFINITIES {-1, 0, 1, 2, 3, 4, 5, 6, 7}
#define DEFAULT_CORE_AFFINITIES_LABELS {"UNSPECIFIED", "0", "1", "2", "3", "4", "5", "6", "7"}

typedef enum {
    NATIVETEST_ID_MIN             = 0,
    NATIVETEST_ID_VOICEMARK       = 0,
    NATIVETEST_ID_LATENCYMARK     = 1,
    NATIVETEST_ID_JITTERMARK      = 2,
    NATIVETEST_ID_UTILIZATIONMARK = 3,
    NATIVETEST_ID_CLOCKRAMP       = 4,
//    NATIVETEST_ID_AUTOMARK        = 5,
    NATIVETEST_ID_MAX             = 4,
} native_test_t;


//SHARED
#define PARAMS_SAMPLE_RATE "sample_rate"
#define PARAMS_SAMPLES_PER_FRAME "samples_per_frame"
#define PARAMS_FRAMES_PER_RENDER "frames_per_render"
#define PARAMS_FRAMES_PER_BURST "frames_per_burst"
#define PARAMS_NUM_SECONDS "num_seconds"
#define PARAMS_NOTE_ON_DELAY "note_on_delay"

#define PARAMS_TARGET_CPU_LOAD  "target_cpu_load"
#define PARAMS_NUM_VOICES "num_voices"
#define PARAMS_NUMB_VOICES_HIGH "num_voices_high"
#define PARAMS_CORE_AFFINITY "core_affinity"

static constexpr int kParamsDefaultIndexFramesPerBurst = 5; // 4->64, 5->96

//============================
// NativeTestUnit
//============================

class NativeTestUnit {
public:
    NativeTestUnit(std::string title, LogTool *logTool = NULL) :
    mTitle(title), mParams(title) {
        if (!logTool) {
            mLogTool = new LogTool(this);
        } else {
            mLogTool = logTool;
        }
    }
    virtual int init() = 0;
    virtual int run() = 0;
    virtual int finish() = 0;

    ~NativeTestUnit() {
        if (mLogTool && mLogTool->getOwner() == this) {
            delete(mLogTool);
            mLogTool = NULL;
        }
    }

    std::string getTestName() {
        return mTitle;
    }

    ParamGroup * getParamGroup() {
        return &mParams;
    }

    HostThreadFactory *getHostThreadFactory() {
        return mHostThreadFactory;
    }
    void setHostThreadFactory(HostThreadFactory *factory) {
        mHostThreadFactory = factory;
    }

protected:
    int mCurrentStatus;
    LogTool *mLogTool;
    std::string mTitle;
    ParamGroup mParams;
    HostThreadFactory *mHostThreadFactory = NULL;
};

class CommonNativeTestUnit : public NativeTestUnit {
public:
    CommonNativeTestUnit(std::string title, LogTool *logTool = NULL)
            : NativeTestUnit(title, logTool) {

    }

    int init() override {
        //Register parameters

        std::vector<int> vSamplingRates = DEFAULT_TEST_SAMPLING_RATES;
        ParamInteger paramSamplingRate(PARAMS_SAMPLE_RATE, "Sample Rate", &vSamplingRates, 5);

        ParamInteger paramSamplesPerFrame(PARAMS_SAMPLES_PER_FRAME, "Samples per Frame",
                                          SAMPLES_PER_FRAME, 1, 8);
        ParamInteger paramFramesPerRender(PARAMS_FRAMES_PER_RENDER, "Frames per Render",
                                          kSynthmarkFramesPerRender, 1, 8);

        std::vector<int> vFramesPerBurst = DEFAULT_TEST_FRAMES_PER_BURST;
        ParamInteger paramFramesPerBurst(PARAMS_FRAMES_PER_BURST,
                                         "Frames per Burst",
                                         &vFramesPerBurst,
                                         kParamsDefaultIndexFramesPerBurst);

        ParamInteger paramNoteOnDelay(PARAMS_NOTE_ON_DELAY, "Note On Delay Seconds", 0, 0, 300);

        std::vector<float> vDurations = DEFAULT_TEST_DURATIONS;
        ParamFloat paramNumSeconds(PARAMS_NUM_SECONDS, "Number of Seconds", &vDurations, 3);

        mParams.addParam(&paramSamplingRate);
        mParams.addParam(&paramSamplesPerFrame);
        mParams.addParam(&paramFramesPerRender);
        mParams.addParam(&paramFramesPerBurst);
        mParams.addParam(&paramNoteOnDelay);
        mParams.addParam(&paramNumSeconds);

#if !HOST_IS_APPLE
        std::vector<int> vCoreAffinity = DEFAULT_CORE_AFFINITIES;
        std::vector<std::string> vCoreAffinityLabels = DEFAULT_CORE_AFFINITIES_LABELS;
        ParamInteger paramCoreAffinity(PARAMS_CORE_AFFINITY, "Core Affinity", &vCoreAffinity, 0,
                                       &vCoreAffinityLabels);
        mParams.addParam(&paramCoreAffinity);
#endif

        return SYNTHMARK_RESULT_SUCCESS;
    }

    int run(TestHarnessBase &harness, VirtualAudioSink &audioSink) {
        audioSink.setHostThread(mHostThreadFactory->createThread(
                HostThreadFactory::ThreadType::Audio));

        int32_t sampleRate = mParams.getValueFromInt(PARAMS_SAMPLE_RATE);
        int32_t samplesPerFrame = mParams.getValueFromInt(PARAMS_SAMPLES_PER_FRAME);
        int32_t framesPerRender = mParams.getValueFromInt(PARAMS_FRAMES_PER_RENDER);
        int32_t framesPerBurst = mParams.getValueFromInt(PARAMS_FRAMES_PER_BURST);

        int32_t noteOnDelay = mParams.getValueFromInt(PARAMS_NOTE_ON_DELAY);
        float numSeconds = mParams.getValueFromFloat(PARAMS_NUM_SECONDS);

#if !HOST_IS_APPLE
        int CoreAffinity = mParams.getValueFromInt(PARAMS_CORE_AFFINITY);
        audioSink.setRequestedCpu(CoreAffinity);
#endif
        mLogTool->log(mParams.toString(ParamBase::PRINT_COMPACT).c_str());

        harness.open(sampleRate,
                     samplesPerFrame,
                     framesPerRender,
                     framesPerBurst);

        harness.setDelayNoteOnSeconds(noteOnDelay);
        harness.measure(numSeconds);
        harness.close();

        mLogTool->log(harness.getResult()->getResultMessage().c_str());
        mLogTool->log("\n");

        return SYNTHMARK_RESULT_SUCCESS;
    }

    int finish() {
        return SYNTHMARK_RESULT_SUCCESS;
    }

protected:
};

//============================
// Custom Tests
//============================

class TestVoiceMark : public CommonNativeTestUnit {
public:
    TestVoiceMark(LogTool *logTool = NULL) : CommonNativeTestUnit("VoiceMark", logTool) {

    }

    int init() override {
        int err = CommonNativeTestUnit::init();
        if (err != SYNTHMARK_RESULT_SUCCESS) {
            return err;
        }

        std::vector<float> vCpuLoads = DEFAULT_TEST_TARGET_CPU_LOADS;
        ParamFloat paramTargetCpuLoad(PARAMS_TARGET_CPU_LOAD, "Target CPU Load", &vCpuLoads, 9);
        mParams.addParam(&paramTargetCpuLoad);

        return err;
    }

    int run() override {
        SynthMarkResult result;
        VirtualAudioSink audioSink(mLogTool);
        VoiceMarkHarness harness(&audioSink, &result, mLogTool);
        
        float targetCpuLoad = mParams.getValueFromFloat(PARAMS_TARGET_CPU_LOAD);
        harness.setTargetCpuLoad(targetCpuLoad);

        return CommonNativeTestUnit::run(harness, audioSink);
    }

protected:
};

class TestUtilizationMark : public CommonNativeTestUnit {
public:
    TestUtilizationMark(LogTool *logTool = NULL) : CommonNativeTestUnit("UtilizationMark", logTool) {
    }

    int init() override {
        int err = CommonNativeTestUnit::init();
        if (err != SYNTHMARK_RESULT_SUCCESS) {
            return err;
        }

        ParamInteger paramNumVoices(PARAMS_NUM_VOICES,
                                    "Number of Voices",
                                    kSynthmarkNumVoicesLatency,
                                    1,
                                    300);
        mParams.addParam(&paramNumVoices);

        return err;
    }

    int run() override {
        SynthMarkResult result;
        VirtualAudioSink audioSink(mLogTool);
        UtilizationMarkHarness harness(&audioSink, &result, mLogTool);

        int32_t numVoices = mParams.getValueFromInt(PARAMS_NUM_VOICES);
        harness.setNumVoices(numVoices);

        return CommonNativeTestUnit::run(harness, audioSink);
    }

protected:
};

class ChangingVoiceTestUnit : public CommonNativeTestUnit {
public:
    ChangingVoiceTestUnit(std::string title, LogTool *logTool = NULL)
            : CommonNativeTestUnit(title, logTool) {
    }

    int init() override {
        int err = CommonNativeTestUnit::init();
        if (err != SYNTHMARK_RESULT_SUCCESS) {
            return err;
        }

        ParamInteger paramNumVoices(PARAMS_NUM_VOICES,
                                    "Number of Voices",
                                    kSynthmarkNumVoicesLatency,
                                    1,
                                    300);
        mParams.addParam(&paramNumVoices);

        ParamInteger paramNumVoicesHigh(PARAMS_NUMB_VOICES_HIGH,
                                        "Number of Voices High",
                                        0,
                                        0,
                                        300);
        mParams.addParam(&paramNumVoicesHigh);

        return err;
    }

    int run(TestHarnessBase &harness, VirtualAudioSink &audioSink) {

        int32_t numVoices = mParams.getValueFromInt(PARAMS_NUM_VOICES);
        harness.setNumVoices(numVoices);

        int32_t numVoicesHigh = mParams.getValueFromInt(PARAMS_NUMB_VOICES_HIGH);
        harness.setNumVoicesHigh(numVoicesHigh);

        return CommonNativeTestUnit::run(harness, audioSink);
    }

protected:
};

class TestLatencyMark : public ChangingVoiceTestUnit {
public:
    TestLatencyMark(LogTool *logTool = NULL)
            : ChangingVoiceTestUnit("LatencyMark", logTool) {

    }

    int run() override {
        SynthMarkResult result;
        VirtualAudioSink audioSink(mLogTool);
        LatencyMarkHarness harness(&audioSink, &result, mLogTool);

        return ChangingVoiceTestUnit::run(harness, audioSink);
    }
protected:
};

class TestJitterMark : public ChangingVoiceTestUnit {
public:
    TestJitterMark(LogTool *logTool = NULL)
            : ChangingVoiceTestUnit("JitterMark", logTool) {

    }

    int run() override {
        SynthMarkResult result;
        VirtualAudioSink audioSink(mLogTool);
        JitterMarkHarness harness(&audioSink, &result, mLogTool);

        return ChangingVoiceTestUnit::run(harness, audioSink);
    }
protected:
};

class TestClockRamp : public ChangingVoiceTestUnit {
public:
    TestClockRamp(LogTool *logTool = NULL)
            : ChangingVoiceTestUnit("ClockRamp", logTool) {

    }

    int run() override {
        SynthMarkResult result;
        VirtualAudioSink audioSink(mLogTool);
        ClockRampHarness harness(&audioSink, &result, mLogTool);

        return ChangingVoiceTestUnit::run(harness, audioSink);
    }
protected:
};

//Native Test manager.
class NativeTest {
public:
    NativeTest()
            : mCurrentStatus(NATIVETEST_STATUS_UNDEFINED)
            , mTestVoiceMark(&mLog)
            , mTestLatencyMark(&mLog)
            , mTestJitterMark(&mLog)
            , mTestUtilizationMark(&mLog)
            , mTestClockRamp(&mLog)
    {
        mLog.setStream(&mStream);
        initTests();
        setHostThreadFactory(&mHostThreadFactoryOwned);
    }

    ~NativeTest() {
        closeTest();
    };

    void initTests() {

        int testCount = getTestCount();
        for (int i = 0; i < testCount; i ++) {
            NativeTestUnit * pTestUnit = getNativeTestUnit(i);
            if (pTestUnit != NULL) {
                pTestUnit->init();
            }
        }
    }

    int init(int testId) {
        mCurrentStatus = NATIVETEST_STATUS_READY;
        mCurrentTest = testId;

        mStream.str("");
        mStream.clear();
        return NATIVETEST_SUCCESS;
    }

    int run() {
        mCurrentStatus = NATIVETEST_STATUS_RUNNING;
        NativeTestUnit *pTestUnit = getNativeTestUnit(mCurrentTest);
        if (pTestUnit != NULL) {
            pTestUnit->run();
        }
        mCurrentStatus = NATIVETEST_STATUS_COMPLETED;
        return NATIVETEST_SUCCESS;
    }

    int finish() {
        NativeTestUnit *pTestUnit = getNativeTestUnit(mCurrentTest);
        if (pTestUnit != NULL) {
            return pTestUnit->finish();
        }
        return NATIVETEST_ERROR;
    }

    int getProgress() {
        return mLog.getVar1();
    }

    int getStatus() {
        return mCurrentStatus;
    }

    std::string getResult() {
        return mStream.str();
    }

    int closeTest() {
        return NATIVETEST_SUCCESS;
    }

    HostThreadFactory *getHostThreadFactory() {
        return mHostThreadFactory;
    }

    void setHostThreadFactory(HostThreadFactory *factory) {
        mHostThreadFactory = factory;
        int testCount = getTestCount();
        for (int i = 0; i < testCount; i ++) {
            NativeTestUnit * pTestUnit = getNativeTestUnit(i);
            if (pTestUnit != NULL) {
                pTestUnit->setHostThreadFactory(factory);
            }
        }
    }

    std::string getTestName(int testId) {
        std::string name = "--";
        NativeTestUnit *pTestUnit = getNativeTestUnit(testId);
        if (pTestUnit != NULL) {
            name = pTestUnit->getTestName();
        }
        return name;
    }

    int getTestCount() {
        return NATIVETEST_ID_MAX + 1;
    }

    ParamGroup * getParamGroup(int testId) {
        ParamGroup *pParamGroup = NULL;

        NativeTestUnit *pTestUnit = getNativeTestUnit(testId);

        if (pTestUnit != NULL) {
            pParamGroup = pTestUnit->getParamGroup();
        }

        return pParamGroup;
    }

    int getParamCount(int testId) {
        int count = 0;
        ParamGroup * pParamGroup = getParamGroup(testId);
        if (pParamGroup != NULL) {
            count = pParamGroup->getParamCount();
        }
        return count;
    }

private:
    NativeTestUnit * getNativeTestUnit(int testId) {
        NativeTestUnit *pTestUnit = NULL;
        switch (testId) {
            case NATIVETEST_ID_VOICEMARK: pTestUnit = &mTestVoiceMark; break;
            case NATIVETEST_ID_LATENCYMARK: pTestUnit = &mTestLatencyMark; break;
            case NATIVETEST_ID_JITTERMARK: pTestUnit = &mTestJitterMark; break;
            case NATIVETEST_ID_UTILIZATIONMARK: pTestUnit = &mTestUtilizationMark; break;
            case NATIVETEST_ID_CLOCKRAMP: pTestUnit = &mTestClockRamp; break;
        }
        return pTestUnit;
    }

    int mCurrentTest;
    int mCurrentStatus;

    TestVoiceMark        mTestVoiceMark;
    TestLatencyMark      mTestLatencyMark;
    TestJitterMark       mTestJitterMark;
    TestUtilizationMark  mTestUtilizationMark;
    TestClockRamp        mTestClockRamp;

    LogTool mLog;
    std::stringstream mStream;
    HostThreadFactory mHostThreadFactoryOwned;
    HostThreadFactory *mHostThreadFactory = NULL;
};

#endif //ANDROID_NATIVETEST_H
