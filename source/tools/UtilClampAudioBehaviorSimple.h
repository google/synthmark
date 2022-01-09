/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef ANDROID_UTILCLAMPAUDIOBEHAVIOR_SIMPLE_H
#define ANDROID_UTILCLAMPAUDIOBEHAVIOR_SIMPLE_H

#include "AudioSinkBase.h"

constexpr int64_t kSecondsToNanos = 1000000000;

constexpr int16_t kMaxUclamp = 1024;                 // max uclamp value that can be reach
constexpr int16_t kMinUclamp = 0;                    // min uclamp value that can be reach
constexpr int16_t kIncreaseStepUclamp = kMaxUclamp;  // uclamp step value to increase
constexpr int16_t kDecreaseStepUclamp = 30;          // uclamp step value to decrease
constexpr int64_t kStartIntervalNanos = 300000000;   // nsecs to wait before starting (time at max uclamp)

constexpr double kLowUtilization = 0.50;
constexpr double kHighUtilization = 0.95;

class UtilClampAudioBehaviorSimple {

public:

    UtilClampAudioBehaviorSimple(int64_t sampleRate, int64_t startUclampNanos, int32_t startUclampValue) {
        mSampleRate = sampleRate;
        mStartTimeNanos = startUclampNanos;
        mCurrUtilClamp = startUclampValue;
    }

    int32_t processTiming(
            int64_t callbackBeginNanos,     // when the app callback began
            int64_t callbackFinishNanos,    // when the app callback finished
            int64_t numFrames               // the number of frames processed by the callback
    ) {

        if ((callbackFinishNanos - mStartTimeNanos) < kStartIntervalNanos) {
            return mCurrUtilClamp;
        }

        int64_t callbackDurationNanos  = callbackFinishNanos - callbackBeginNanos;
        int64_t audioDurationNanos = (numFrames * kSecondsToNanos) / mSampleRate;

        if (callbackDurationNanos < (int64_t)(audioDurationNanos * kLowUtilization)) {
            decreaseUtilClamp();
        } else if (callbackDurationNanos > (int64_t)(audioDurationNanos * kHighUtilization)) {
            increaseUtilClamp();
        }

        return mCurrUtilClamp;
    }

private:

    int64_t mSampleRate = 48000;
    int16_t mCurrUtilClamp = 0;
    int64_t mStartTimeNanos = 0;

    void decreaseUtilClamp() {
        int16_t suggestedUtilClamp = mCurrUtilClamp - kDecreaseStepUclamp;
        mCurrUtilClamp = std::max(suggestedUtilClamp, kMinUclamp);
    }

    void increaseUtilClamp(){
        int16_t suggestedUtilClamp = mCurrUtilClamp + kIncreaseStepUclamp;
        mCurrUtilClamp = std::min(suggestedUtilClamp, kMaxUclamp);
    }

};

#endif //ANDROID_UTILCLAMPAUDIOBEHAVIOR_SIMPLE_H
