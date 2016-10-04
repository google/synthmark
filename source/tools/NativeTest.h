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

#include <string>
#include <sstream>
#include "LogTool.h"
#include "../SynthMark.h"
#include "TimingAnalyzer.h"
#include "../synth/Synthesizer.h"
#include "VirtualAudioSink.h"
#include "VoiceMarkHarness.h"
#include "LatencyMarkHarness.h"
#include "JitterMarkHarness.h"
#include "Params.h"


#define NATIVETEST_SUCCESS 0
#define NATIVETEST_ERROR -1

#define NATIVETEST_STATUS_ERROR -1
#define NATIVETEST_STATUS_UNDEFINED 0
#define NATIVETEST_STATUS_READY 1
#define NATIVETEST_STATUS_RUNNING 2
#define NATIVETEST_STATUS_COMPLETED 3


typedef enum {
    NATIVETEST_ID_MIN           = 1,
    NATIVETEST_ID_VOICEMARK     = 1,
    NATIVETEST_ID_LATENCYMARK   = 2,
    NATIVETEST_ID_JITTERMARK    = 3,
    NATIVETEST_ID_MAX           = 3,
} native_test_t;


//SHARED
#define PARAMS_SAMPLE_RATE "sample_rate"
#define PARAMS_SAMPLES_PER_FRAME "samples_per_frame"
#define PARAMS_FRAMES_PER_RENDER "frames_per_render"
#define PARAMS_FRAMES_PER_BURST "frames_per_burst"
#define PARAMS_NUM_SECONDS "num_seconds"

#define PARAMS_TARGET_CPU_LOAD  "target_cpu_load"
#define PARAMS_NUM_VOICES "num_voices"

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

protected:
    int mCurrentStatus;
    LogTool *mLogTool;
    std::string mTitle;
    ParamGroup mParams;
};


//============================
// Custom Tests
//============================

class TestVoiceMark : public NativeTestUnit {
public:
    TestVoiceMark(LogTool *logTool = NULL) : NativeTestUnit("VoiceMark", logTool) {

    }

    int init()  {
        //Register parameters

        // GENERAL Parameters:
        ParamInteger paramSamplingRate(PARAMS_SAMPLE_RATE, "Sample Rate", SYNTHMARK_SAMPLE_RATE,
        8000, 96000);
        ParamInteger paramSamplesPerFrame(PARAMS_SAMPLES_PER_FRAME, "Samples per Frame",
        SAMPLES_PER_FRAME, 1, 8);
        ParamInteger paramFramesPerRender(PARAMS_FRAMES_PER_RENDER, "Frames per Render",
        SYNTHMARK_FRAMES_PER_RENDER, 1, 64);
        ParamInteger paramFramesPerBurst(PARAMS_FRAMES_PER_BURST, "Frames per Burst",
        SYNTHMARK_FRAMES_PER_BURST, 1, 512);
        ParamFloat paramTargetCpuLoad(PARAMS_TARGET_CPU_LOAD, "Target CPU Load",
        SYNTHMARK_TARGET_CPU_LOAD, 0, 1.0);
        ParamFloat paramNumSeconds(PARAMS_NUM_SECONDS,"Number of Seconds",
        SYNTHMARK_NUM_SECONDS, 1, 3600);

        mParams.addParam(&paramSamplingRate);
        mParams.addParam(&paramSamplesPerFrame);
        mParams.addParam(&paramFramesPerRender);
        mParams.addParam(&paramFramesPerBurst);
        mParams.addParam(&paramTargetCpuLoad);
        mParams.addParam(&paramNumSeconds);

        return SYNTHMARK_RESULT_SUCCESS;
    }

    int run() {
        SynthMarkResult result;
        VirtualAudioSink audioSink;
        VoiceMarkHarness harness(&audioSink, &result, mLogTool);

        int32_t sampleRate = mParams.getValueFromInt(PARAMS_SAMPLE_RATE);
        int32_t samplesPerFrame = mParams.getValueFromInt(PARAMS_SAMPLES_PER_FRAME);
        int32_t framesPerRender = mParams.getValueFromInt(PARAMS_FRAMES_PER_RENDER);
        int32_t framesPerBurst = mParams.getValueFromInt(PARAMS_FRAMES_PER_BURST);

        float targetCpuLoad = mParams.getValueFromFloat(PARAMS_TARGET_CPU_LOAD);
        float numSeconds = mParams.getValueFromFloat(PARAMS_NUM_SECONDS);

        mLogTool->log(mParams.toString(ParamBase::PRINT_COMPACT).c_str());

        harness.open(sampleRate,
                     samplesPerFrame,
                     framesPerRender,
                     framesPerBurst);
        harness.setTargetCpuLoad(targetCpuLoad);
        harness.measure(numSeconds);
        harness.close();

        mLogTool->log(result.getResultMessage().c_str());
        mLogTool->log("\n");

        return SYNTHMARK_RESULT_SUCCESS;
    }

    int finish() {
        return SYNTHMARK_RESULT_SUCCESS;
    }

protected:
};

class TestLatencyMark : public NativeTestUnit {
public:
    TestLatencyMark(LogTool *logTool = NULL) : NativeTestUnit("LatencyMark", logTool) {

    }

    int init()  {
        //Register parameters

        // GENERAL Parameters:
        ParamInteger paramSamplingRate(PARAMS_SAMPLE_RATE, "Sample Rate", SYNTHMARK_SAMPLE_RATE,
        8000, 96000);
        ParamInteger paramSamplesPerFrame(PARAMS_SAMPLES_PER_FRAME, "Samples per Frame",
        SAMPLES_PER_FRAME, 1, 8);
        ParamInteger paramFramesPerRender(PARAMS_FRAMES_PER_RENDER, "Frames per Render",
        SYNTHMARK_FRAMES_PER_RENDER, 1, 64);
        ParamInteger paramFramesPerBurst(PARAMS_FRAMES_PER_BURST, "Frames per Burst", 32, 1, 512);

        ParamFloat paramNumSeconds(PARAMS_NUM_SECONDS,"Number of Seconds", SYNTHMARK_NUM_SECONDS,
        1,1000);

        mParams.addParam(&paramSamplingRate);
        mParams.addParam(&paramSamplesPerFrame);
        mParams.addParam(&paramFramesPerRender);
        mParams.addParam(&paramFramesPerBurst);
        mParams.addParam(&paramNumSeconds);

        return SYNTHMARK_RESULT_SUCCESS;
    }

    int run() {
        SynthMarkResult result;
        VirtualAudioSink audioSink;
        LatencyMarkHarness harness(&audioSink, &result, mLogTool);

        int32_t sampleRate = mParams.getValueFromInt(PARAMS_SAMPLE_RATE);
        int32_t samplesPerFrame = mParams.getValueFromInt(PARAMS_SAMPLES_PER_FRAME);
        int32_t framesPerRender = mParams.getValueFromInt(PARAMS_FRAMES_PER_RENDER);
        int32_t framesPerBurst = mParams.getValueFromInt(PARAMS_FRAMES_PER_BURST);

        float numSeconds = mParams.getValueFromFloat(PARAMS_NUM_SECONDS);
        mLogTool->log(mParams.toString(ParamBase::PRINT_COMPACT).c_str());

        harness.open(sampleRate,
                     samplesPerFrame,
                     framesPerRender,
                     framesPerBurst);

        harness.measure(numSeconds);
        harness.close();

        mLogTool->log(result.getResultMessage().c_str());
        mLogTool->log("\n");

        return SYNTHMARK_RESULT_SUCCESS;
    }

    int finish() {
        return SYNTHMARK_RESULT_SUCCESS;
    }

protected:
};


class TestJitterMark : public NativeTestUnit {
public:
    TestJitterMark(LogTool *logTool = NULL) : NativeTestUnit("JitterMark", logTool) {
    }

    int init()  {
        //Register parameters

        // GENERAL Parameters:
        ParamInteger paramSamplingRate(PARAMS_SAMPLE_RATE, "Sample Rate", SYNTHMARK_SAMPLE_RATE,
        8000, 96000);
        ParamInteger paramSamplesPerFrame(PARAMS_SAMPLES_PER_FRAME, "Samples per Frame",
        SAMPLES_PER_FRAME, 1, 8);
        ParamInteger paramFramesPerRender(PARAMS_FRAMES_PER_RENDER, "Frames per Render",
        SYNTHMARK_FRAMES_PER_RENDER, 1, 64);
        ParamInteger paramFramesPerBurst(PARAMS_FRAMES_PER_BURST, "Frames per Burst", 32, 1, 512);

        ParamInteger paramNumVoices(PARAMS_NUM_VOICES,"Number of Voices",
        SYNTHMARK_NUM_VOICES_JITTER, 1,300);
        ParamFloat paramNumSeconds(PARAMS_NUM_SECONDS,"Number of Seconds", SYNTHMARK_NUM_SECONDS,
        1, 3600);

        mParams.addParam(&paramSamplingRate);
        mParams.addParam(&paramSamplesPerFrame);
        mParams.addParam(&paramFramesPerRender);
        mParams.addParam(&paramFramesPerBurst);

        mParams.addParam(&paramNumVoices);
        mParams.addParam(&paramNumSeconds);

        return SYNTHMARK_RESULT_SUCCESS;
    }

    int run() {
        SynthMarkResult result;
        VirtualAudioSink audioSink;
        JitterMarkHarness harness(&audioSink, &result, mLogTool);

        int32_t sampleRate = mParams.getValueFromInt(PARAMS_SAMPLE_RATE);
        int32_t samplesPerFrame = mParams.getValueFromInt(PARAMS_SAMPLES_PER_FRAME);
        int32_t framesPerRender = mParams.getValueFromInt(PARAMS_FRAMES_PER_RENDER);
        int32_t framesPerBurst = mParams.getValueFromInt(PARAMS_FRAMES_PER_BURST);

        int32_t numVoices = mParams.getValueFromInt(PARAMS_NUM_VOICES);
        float numSeconds = mParams.getValueFromFloat(PARAMS_NUM_SECONDS);

        mLogTool->log(mParams.toString(ParamBase::PRINT_COMPACT).c_str());

        harness.open(sampleRate,
                     samplesPerFrame,
                     framesPerRender,
                     framesPerBurst);
        harness.setNumVoices(numVoices);
        harness.measure(numSeconds);
        harness.close();

        mLogTool->log(result.getResultMessage().c_str());
        mLogTool->log("\n");

        return SYNTHMARK_RESULT_SUCCESS;
    }

    int finish() {
        return SYNTHMARK_RESULT_SUCCESS;
    }

protected:
};

//Native Test manager.
class NativeTest {
public:
    NativeTest() :  mCurrentStatus(NATIVETEST_STATUS_UNDEFINED),
    mTestVoiceMark(&mLog), mTestLatencyMark(&mLog), mTestJitterMark(&mLog){
        mLog.setStream(&mStream);
        initTests();
    }

    ~NativeTest() {
        closeTest();
    };

    void initTests() {

        mTestVoiceMark.init();
        mTestLatencyMark.init();
        mTestJitterMark.init();
    }

    int init(int testId) {
        mCurrentStatus = NATIVETEST_STATUS_READY;
        mCurrentTest = testId;

        return NATIVETEST_SUCCESS;
    }

    int run() {
        mCurrentStatus = NATIVETEST_STATUS_RUNNING;
        switch(mCurrentTest) {
            case NATIVETEST_ID_VOICEMARK: {
                mTestVoiceMark.run();
            }
                break;
            case NATIVETEST_ID_LATENCYMARK: {
                mTestLatencyMark.run();
            }
                break;
            case NATIVETEST_ID_JITTERMARK: {
                mTestJitterMark.run();
            }
                break;
        }
        mCurrentStatus = NATIVETEST_STATUS_COMPLETED;
        return NATIVETEST_SUCCESS;
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

    std::string getTestName(int testId) {
        std::string name = "--";
        switch (testId) {
            case NATIVETEST_ID_VOICEMARK: name = mTestVoiceMark.getTestName(); break;
            case NATIVETEST_ID_LATENCYMARK: name = mTestLatencyMark.getTestName(); break;
            case NATIVETEST_ID_JITTERMARK: name = mTestJitterMark.getTestName(); break;
        }
        return name;
    }

private:
    int mCurrentTest;
    int mCurrentStatus;

    TestVoiceMark mTestVoiceMark;
    TestLatencyMark mTestLatencyMark;
    TestJitterMark mTestJitterMark;

    LogTool mLog;
    std::stringstream mStream;
};



#endif //ANDROID_NATIVETEST_H
