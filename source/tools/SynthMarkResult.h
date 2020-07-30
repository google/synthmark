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

#ifndef ANDROID_SYNTHMARKRESULT_H
#define ANDROID_SYNTHMARKRESULT_H

#include <cstdint>
#include <string>


// TODO Use synthmark namespace and more C++ style naming conventions.
enum : int32_t {
    SYNTHMARK_RESULT_SUCCESS = 0,

    SYNTHMARK_RESULT_TOO_FEW_MEASUREMENTS = -1,
    SYNTHMARK_RESULT_UNINITIALIZED = -2,
    SYNTHMARK_RESULT_AUDIO_SINK_START_FAILURE = -3,
    SYNTHMARK_RESULT_AUDIO_SINK_WRITE_FAILURE = -4,
    SYNTHMARK_RESULT_THREAD_FAILURE = -5,
    SYNTHMARK_RESULT_UNRECOVERABLE_ERROR = -6,
    SYNTHMARK_RESULT_GLITCH_DETECTED = -7,
    SYNTHMARK_RESULT_OUT_OF_RANGE = -8
};

/**
 * Class for holding the results of a SynthMark test
 *
 * Information stored includes:
 *
 * SynthMark version
 * Numeric result of test
 * Text report that may include CSV values.
 *
 */
class SynthMarkResult {
public:
    SynthMarkResult()
            : mResultCode(SYNTHMARK_RESULT_UNINITIALIZED)
            , mMeasurement(0)
    {
    }

    void reset() {
        mMeasurement = 0.0;
        mResultMessage.clear();
    }

    std::string getTestName() {
        return mTestName;
    }

    void setTestName(std::string testName) {
        mTestName = testName;
    }

    std::string getResultMessage() {
        return mResultMessage;
    }

    void appendMessage(std::string message) {
        mResultMessage.append(message);
    }

    int32_t getResultCode() {
        return mResultCode;
    }

    void setResultCode(int32_t resultCode) {
        mResultCode = resultCode;
    }

    bool isSuccessful() {
        return (mResultCode == SYNTHMARK_RESULT_SUCCESS);
    }

    double getMeasurement() {
        return mMeasurement;
    };

    void setMeasurement(double measurement) {
        mMeasurement = measurement;
    }

private:

    std::string mTestName;
    std::string mResultMessage;
    int32_t mResultCode;
    double mMeasurement;

};

#endif //ANDROID_SYNTHMARKRESULT_H
