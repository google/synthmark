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

#ifndef ANDROID_UTILCLAMPAUDIOBEHAVIOR_H
#define ANDROID_UTILCLAMPAUDIOBEHAVIOR_H


#include "LogTool.h"
#include "UtilClampController.h"
#include "AudioSinkBase.h"
#include <ctime>

constexpr int kDecreaseMargin = 450;     // usecs to hysteresis down
constexpr int kIncreaseMargin = 210;     // usecs to hysteresis up
constexpr int kIncreaseStepUclamp = 300; // uclamp step value to increase
constexpr int kDecreaseStepUclamp = 30;  // uclamp step value to decrease
constexpr int kMaxUclamp = 1024;         // max uclamp value that can be reach
constexpr int kMinUclamp = 0;            // min uclamp value that can be reach
constexpr int kDecreaseInterval = 50000; // usecs to wait before decreasing again
constexpr int kIncreaseInterval = 0;     // usecs to wait before increasing again
constexpr int kStartInterval = 300000;   // usecs to wait before starting (time at max uclamp)

class UtilClampAudioBehavior {

public:

    void UtilClampBehavior(int32_t sampleRate, int16_t start_uclamp) {

        if (Controller.isUtilClampSupported()) {
            mSampleRate = sampleRate;
            Controller.setUtilClampMin(start_uclamp);
            currUtilClamp = Controller.getUtilClampMin();
            startTime = getMicroTime();
        }
    }

    int64_t processTiming(
            int64_t beginNanos,     // when the app callback began
            int64_t finishNanos,    // when the app callback finished
            int32_t numFrames       // the number of frames processed by the callback
    ) {

        if (!Controller.isUtilClampSupported()) {
            return -1;
        }

        if ((getMicroTime() - startTime) < kStartInterval) {
            return currUtilClamp;
        }

        int64_t callbackDuration  = (finishNanos - beginNanos) / 1000; // nsecs to usecs
        int64_t maxBearableDuration = (numFrames * 1000) / (mSampleRate / 1000);  // usecs

        if (callbackDuration < (maxBearableDuration - kDecreaseMargin) &&
                (getMicroTime() - lastUclampChange) > kDecreaseInterval) {
            lastUclampChange = getMicroTime();
            decreaseUtilClamp();
        } else if (callbackDuration > (maxBearableDuration + kIncreaseMargin) &&
                    (getMicroTime() - lastUclampChange) > kIncreaseInterval) {
            lastUclampChange = getMicroTime();
            increaseUtilClamp();
        }

        return currUtilClamp;
    }

private:

    UtilClampController Controller;
    uint32_t mSampleRate = 48000;
    int16_t currUtilClamp = -1;
    uint64_t lastUclampChange = 0;
    uint64_t startTime = 0;

    uint64_t getMicroTime(){
        struct timespec res;
        int result = clock_gettime(CLOCK_MONOTONIC, &res);
        if (result < 0) {
            return result;
        }
        return (res.tv_sec * 1000000) + (res.tv_nsec / 1000);
    }

    void decreaseUtilClamp() {
        if (currUtilClamp > (kDecreaseStepUclamp + kMinUclamp)){
            currUtilClamp -= kDecreaseStepUclamp;
        } else if (currUtilClamp != kMinUclamp) {
            currUtilClamp = kMinUclamp;
        }
        Controller.setUtilClampMin(currUtilClamp);
        currUtilClamp = Controller.getUtilClampMin();
    }

    void increaseUtilClamp(){
        if (currUtilClamp < (kMaxUclamp - kIncreaseStepUclamp)){
            currUtilClamp += kIncreaseStepUclamp;
        } else if (currUtilClamp != kMaxUclamp) {
            currUtilClamp = kMaxUclamp;
        }
        Controller.setUtilClampMin(currUtilClamp);
        currUtilClamp = Controller.getUtilClampMin();
    }

};

#endif //ANDROID_UTILCLAMPAUDIOBEHAVIOR_H
