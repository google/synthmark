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

#define HOST_IS_APPLE       defined(__APPLE__)
#define HOST_IS_NOT_APPLE  !defined(__APPLE__)

#include <string>
#include <sstream>

#include "../SynthMark.h"
#include "../synth/Synthesizer.h"
#include "AutomatedTestSuite.h"
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
#define DEFAULT_TEST_NUM_VOICES  1, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, \
    80, 90, 100, 125, 150, 175, 200, 250, 300, 350, 400, 450, 500
#define DEFAULT_TEST_NUM_VOICES_HIGH  0, DEFAULT_TEST_NUM_VOICES

typedef enum {
    NATIVETEST_ID_AUTOMATED       = 0,
    NATIVETEST_ID_CLOCKRAMP       = 1,
    NATIVETEST_ID_JITTERMARK      = 2,
    NATIVETEST_ID_LATENCYMARK     = 3,
    NATIVETEST_ID_UTILIZATIONMARK = 4,
    NATIVETEST_ID_VOICEMARK       = 5,

    NATIVETEST_ID_COUNT           = 6,
} native_test_t;


//SHARED
#define PARAMS_SAMPLE_RATE "sample_rate"
#define PARAMS_SAMPLES_PER_FRAME "samples_per_frame"
#define PARAMS_FRAMES_PER_RENDER "frames_per_render"
#define PARAMS_FRAMES_PER_BURST "frames_per_burst"
#define PARAMS_NUM_SECONDS "num_seconds"
#define PARAMS_NOTE_ON_DELAY "note_on_delay"
#define PARAMS_TEST_START_DELAY "test_start_delay"
#define PARAMS_AUDIO_LEVEL  "audio_level"
#define PARAMS_TARGET_CPU_LOAD  "target_cpu_load"
#define PARAMS_NUM_VOICES "num_voices"
#define PARAMS_NUM_VOICES_HIGH "num_voices_high"
#define PARAMS_CORE_AFFINITY "core_affinity"
#define PARAMS_ADPF_ENABLED  "adpf_enabled"

static constexpr int kParamsDefaultIndexFramesPerBurst = 5; // 4->64, 5->96

//============================
// NativeTestUnit
//============================

class NativeTestUnit {
public:
    NativeTestUnit(std::string title, LogTool &logTool)
            : mTitle(title)
            , mParams(title)
            , mLogTool(logTool)
    {}

    virtual int init() = 0;
    virtual int run() = 0;
    virtual int finish() = 0;

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

    virtual bool hasLogs() {
        return mLogTool.hasLogs();
    }

    virtual std::string readLog() {
        return mLogTool.readLog();
    }

protected:
    int mCurrentStatus;
    std::string mTitle;
    ParamGroup  mParams;
    LogTool    &mLogTool;
    HostThreadFactory *mHostThreadFactory = NULL;
};

class CommonNativeTestUnit : public NativeTestUnit {
public:
    CommonNativeTestUnit(std::string title, LogTool &logTool)
            : NativeTestUnit(title, logTool) {

    }

    void createAudioSink() {
        int audioLevel = mParams.getValueFromInt(PARAMS_AUDIO_LEVEL);
        if (audioLevel == AudioSinkBase::AUDIO_LEVEL_OUTPUT) {
            mAudioSink = std::make_unique<RealAudioSink>(mLogTool);
        } else {
            mAudioSink = std::make_unique<VirtualAudioSink>(mLogTool);
        }
        mAudioSink->setThreadType(HostThreadFactory::ThreadType::Audio);

        int adpfLevel = mParams.getValueFromInt(PARAMS_ADPF_ENABLED);
        mAudioSink->setAdpfEnabled(adpfLevel > 0);
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

        ParamInteger paramAudioLevel(PARAMS_AUDIO_LEVEL, "Audio Level: 1=callback, 2=audible",
                                     AudioSinkBase::AUDIO_LEVEL_CALLBACK,
                                     AudioSinkBase::AUDIO_LEVEL_CALLBACK,
                                     AudioSinkBase::AUDIO_LEVEL_OUTPUT);

        ParamInteger paramAdpfEnabled(PARAMS_ADPF_ENABLED, "ADPF: 0=off, 1=on",0, 0, 1);

        ParamInteger paramNoteOnDelay(PARAMS_NOTE_ON_DELAY, "Note On Delay Seconds", 0, 0, 300);
        // Touch boost usually dies down in a few seconds.
        ParamInteger paramTestStartDelay(PARAMS_TEST_START_DELAY, "Test Start Delay Seconds", 4, 0, 20);

        std::vector<float> vDurations = DEFAULT_TEST_DURATIONS;
        ParamFloat paramNumSeconds(PARAMS_NUM_SECONDS, "Number of Seconds", &vDurations, 3);

        mParams.addParam(&paramSamplingRate);
        mParams.addParam(&paramSamplesPerFrame);
        mParams.addParam(&paramFramesPerRender);
        mParams.addParam(&paramFramesPerBurst);
        mParams.addParam(&paramAudioLevel);
        mParams.addParam(&paramAdpfEnabled);
        mParams.addParam(&paramNoteOnDelay);
        mParams.addParam(&paramTestStartDelay);
        mParams.addParam(&paramNumSeconds);

#ifndef __APPLE__
        // Build Core Affinity slider dynamically based on number of CPUs.
        int cpuCount = HostTools::getCpuCount();
        std::vector<int> vCoreAffinity;
        std::vector<std::string> vCoreAffinityLabels;
        vCoreAffinity.push_back(-1);
        vCoreAffinityLabels.push_back("UNSPECIFIED");
        for (int cpu = 0; cpu < cpuCount; cpu++) {
            vCoreAffinity.push_back(cpu);
            vCoreAffinityLabels.push_back(std::to_string(cpu));
        }
        ParamInteger paramCoreAffinity(PARAMS_CORE_AFFINITY, "Core Affinity", &vCoreAffinity, 0,
                                       &vCoreAffinityLabels);
        mParams.addParam(&paramCoreAffinity);
#endif

        return SYNTHMARK_RESULT_SUCCESS;
    }

    int runTestHarness(ITestHarness &harness) {
        // Sleep to allow time for touch boost to die down.
        int32_t testDelaySeconds = mParams.getValueFromInt(PARAMS_TEST_START_DELAY);
        if (testDelaySeconds > 0) {
            mLogTool.log("Wait %d seconds for the test to start...\n", testDelaySeconds);
            sleep(testDelaySeconds);
        }

        mResult.reset();

        int32_t sampleRate = mParams.getValueFromInt(PARAMS_SAMPLE_RATE);
        // int32_t samplesPerFrame = mParams.getValueFromInt(PARAMS_SAMPLES_PER_FRAME); // IGNORED!
        // int32_t framesPerRender = mParams.getValueFromInt(PARAMS_FRAMES_PER_RENDER); // IGNORED!
        int32_t framesPerBurst = mParams.getValueFromInt(PARAMS_FRAMES_PER_BURST);

        int32_t noteOnDelay = mParams.getValueFromInt(PARAMS_NOTE_ON_DELAY);
        float numSeconds = mParams.getValueFromFloat(PARAMS_NUM_SECONDS);

#ifndef __APPLE__
        int CoreAffinity = mParams.getValueFromInt(PARAMS_CORE_AFFINITY);
        mAudioSink->setRequestedCpu(CoreAffinity);
#endif
        mLogTool.log(mParams.toString(ParamBase::PRINT_COMPACT).c_str());

        harness.setDelayNoteOnSeconds(noteOnDelay);
        harness.setThreadType(HostThreadFactory::ThreadType::Audio);

        harness.runCompleteTest(sampleRate, framesPerBurst, numSeconds);

        // Send one line at a time so we do not overflow the LOG buffer.
        std::istringstream f(mResult.getResultMessage());
        std::string line;
        while (std::getline(f, line)) {
            mLogTool.log((line + "\n").c_str());
        }

        return SYNTHMARK_RESULT_SUCCESS;
    }

    int finish() override {
        return SYNTHMARK_RESULT_SUCCESS;
    }


protected:

    std::unique_ptr<AudioSinkBase> mAudioSink;
    SynthMarkResult mResult;
};

//============================
// Custom Tests
//============================

class TestVoiceMark : public CommonNativeTestUnit {
public:
    TestVoiceMark(LogTool &logTool) : CommonNativeTestUnit("VoiceMark", logTool) {
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
        createAudioSink();
        VoiceMarkHarness harness(mAudioSink.get(), &mResult, mLogTool);
        
        float targetCpuLoad = mParams.getValueFromFloat(PARAMS_TARGET_CPU_LOAD);
        harness.setTargetCpuLoad(targetCpuLoad);

        return CommonNativeTestUnit::runTestHarness(harness);
    }

protected:
};

class TestUtilizationMark : public CommonNativeTestUnit {
public:
    TestUtilizationMark(LogTool &logTool) : CommonNativeTestUnit("UtilizationMark", logTool) {
    }

    int init() override {
        int err = CommonNativeTestUnit::init();
        if (err != SYNTHMARK_RESULT_SUCCESS) {
            return err;
        }

        std::vector<int> vNumVoices = { DEFAULT_TEST_NUM_VOICES };
        ParamInteger paramNumVoices(PARAMS_NUM_VOICES,
                                    "Number of Voices",
                                    &vNumVoices,
                                    2);
        mParams.addParam(&paramNumVoices);

        return err;
    }

    int run() override {
        createAudioSink();
        UtilizationMarkHarness harness(mAudioSink.get(), &mResult, mLogTool);

        int32_t numVoices = mParams.getValueFromInt(PARAMS_NUM_VOICES);
        harness.setNumVoices(numVoices);

        return CommonNativeTestUnit::runTestHarness(harness);
    }

protected:
};

class ChangingVoiceTestUnit : public CommonNativeTestUnit {
public:
    ChangingVoiceTestUnit(std::string title, LogTool &logTool)
            : CommonNativeTestUnit(title, logTool) {
    }

    int init() override {
        int err = CommonNativeTestUnit::init();
        if (err != SYNTHMARK_RESULT_SUCCESS) {
            return err;
        }

        std::vector<int> vNumVoices = { DEFAULT_TEST_NUM_VOICES };
        ParamInteger paramNumVoices(PARAMS_NUM_VOICES,
                                    "Number of Voices",
                                    &vNumVoices,
                                    2);
        mParams.addParam(&paramNumVoices);

        std::vector<int> vNumVoicesHigh = { DEFAULT_TEST_NUM_VOICES_HIGH };
        ParamInteger paramNumVoicesHigh(PARAMS_NUM_VOICES_HIGH,
                                    "Number of Voices High",
                                    &vNumVoicesHigh,
                                    0);
        mParams.addParam(&paramNumVoicesHigh);

        return err;
    }

    int runTestHarness(TestHarnessBase &harness) {

        int32_t numVoices = mParams.getValueFromInt(PARAMS_NUM_VOICES);
        harness.setNumVoices(numVoices);

        int32_t numVoicesHigh = mParams.getValueFromInt(PARAMS_NUM_VOICES_HIGH);
        harness.setNumVoicesHigh(numVoicesHigh);

        return CommonNativeTestUnit::runTestHarness(harness);
    }

protected:
};

class TestLatencyMark : public ChangingVoiceTestUnit {
public:
    TestLatencyMark(LogTool &logTool)
            : ChangingVoiceTestUnit("LatencyMark", logTool) {

    }

    int run() override {
        createAudioSink();
        LatencyMarkHarness harness(mAudioSink.get(), &mResult, mLogTool);

        return ChangingVoiceTestUnit::runTestHarness((TestHarnessBase &) harness);
    }
protected:
};

class TestJitterMark : public ChangingVoiceTestUnit {
public:
    TestJitterMark(LogTool &logTool)
            : ChangingVoiceTestUnit("JitterMark", logTool) {

    }

    int run() override {
        createAudioSink();
        JitterMarkHarness harness(mAudioSink.get(), &mResult, mLogTool);

        return ChangingVoiceTestUnit::runTestHarness((TestHarnessBase &) harness);
    }
protected:
};

class TestClockRamp : public ChangingVoiceTestUnit {
public:
    TestClockRamp(LogTool &logTool)
            : ChangingVoiceTestUnit("ClockRamp", logTool) {

    }

    int run() override {
        createAudioSink();
        ClockRampHarness harness(mAudioSink.get(), &mResult, mLogTool);

        return ChangingVoiceTestUnit::runTestHarness((TestHarnessBase &) harness);
    }
protected:
};

class TestAutomatedSuite : public CommonNativeTestUnit {
public:
    TestAutomatedSuite(LogTool &logTool) : CommonNativeTestUnit("AutomatedSuite", logTool) {
    }

    int init() override {
        int err = CommonNativeTestUnit::init();
        if (err != SYNTHMARK_RESULT_SUCCESS) {
            return err;
        }

        return err;
    }

    int run() override {
        createAudioSink();
        AutomatedTestSuite harness(mAudioSink.get(), &mResult, mLogTool);

        return CommonNativeTestUnit::runTestHarness(harness);
    }

protected:
};

//Native Test manager.
class NativeTest {
public:
    NativeTest()
            : mCurrentStatus(NATIVETEST_STATUS_UNDEFINED)
            , mTestVoiceMark(mLog)
            , mTestLatencyMark(mLog)
            , mTestJitterMark(mLog)
            , mTestUtilizationMark(mLog)
            , mTestClockRamp(mLog)
            , mTestAutomatedSuite(mLog)
    {
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
        return NATIVETEST_ID_COUNT;
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

    bool hasLogs() {
        NativeTestUnit *pTestUnit = getNativeTestUnit(mCurrentTest);
        if (pTestUnit != NULL) {
            return pTestUnit->hasLogs();
        } else {
            return false;
        }
    }

    std::string readLog() {
        NativeTestUnit *pTestUnit = getNativeTestUnit(mCurrentTest);
        if (pTestUnit != NULL) {
            return pTestUnit->readLog();
        } else {
            return std::string("[nolog]");
        }
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
            case NATIVETEST_ID_AUTOMATED: pTestUnit = &mTestAutomatedSuite; break;
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
    TestAutomatedSuite   mTestAutomatedSuite;

    LogTool              mLog;
    HostThreadFactory    mHostThreadFactoryOwned;
    HostThreadFactory   *mHostThreadFactory = NULL;
};

#endif //ANDROID_NATIVETEST_H
