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

class NativeTest {
public:
    NativeTest() : mCurrentStatus(NATIVETEST_STATUS_UNDEFINED) {
        mLog.setStream(&mStream);
    }

    ~NativeTest() {
        closeTest();
    };

    int init(int testId) {
        mCurrentStatus = NATIVETEST_STATUS_READY;
        mCurrentTest = testId;
        return NATIVETEST_SUCCESS;
    }

    int run() {
        mCurrentStatus = NATIVETEST_STATUS_RUNNING;
        switch(mCurrentTest) {
            case NATIVETEST_ID_VOICEMARK: {
                SynthMarkResult result;
                VirtualAudioSink audioSink;
                VoiceMarkHarness harness(&audioSink, &result, &mLog);

                harness.open(SYNTHMARK_SAMPLE_RATE, SAMPLES_PER_FRAME,
                             SYNTHMARK_FRAMES_PER_RENDER,
                             SYNTHMARK_FRAMES_PER_BURST);
                harness.setTargetCpuLoad(SYNTHMARK_TARGET_CPU_LOAD);
                harness.measure(SYNTHMARK_NUM_SECONDS);
                harness.close();

                mStream << result.getResultMessage() << "\n";
            }
                break;
            case NATIVETEST_ID_LATENCYMARK: {
                int32_t framesPerBurst = 32;
                SynthMarkResult result;
                VirtualAudioSink audioSink;
                LatencyMarkHarness harness(&audioSink, &result, &mLog);

                harness.open(SYNTHMARK_SAMPLE_RATE, SAMPLES_PER_FRAME,
                             SYNTHMARK_FRAMES_PER_RENDER,
                             framesPerBurst);
                harness.measure(SYNTHMARK_NUM_SECONDS);
                harness.close();

                mStream << result.getResultMessage() << "\n";

            }
                break;
            case NATIVETEST_ID_JITTERMARK: {
                int32_t framesPerBurst = 32;
                SynthMarkResult result;
                VirtualAudioSink audioSink;
                JitterMarkHarness harness(&audioSink, &result, &mLog);

                harness.open(SYNTHMARK_SAMPLE_RATE, SAMPLES_PER_FRAME,
                             SYNTHMARK_FRAMES_PER_RENDER,
                             framesPerBurst);
                harness.setNumVoices(SYNTHMARK_NUM_VOICES_JITTER);
                harness.measure(SYNTHMARK_NUM_SECONDS);
                harness.close();

                mStream << result.getResultMessage() << "\n";
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
            case NATIVETEST_ID_VOICEMARK: name = "VoiceMark"; break;
            case NATIVETEST_ID_LATENCYMARK: name = "LatencyMark"; break;
            case NATIVETEST_ID_JITTERMARK: name = "JitterMark"; break;
        }
        return name;
    }

private:
    int mCurrentTest;
    int mCurrentStatus;

    LogTool mLog;
    std::stringstream mStream;
};



#endif //ANDROID_NATIVETEST_H
