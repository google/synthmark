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

#ifndef ANDROID_UTIL_CLAMP_AUDIO_BEHAVIOR_H
#define ANDROID_UTIL_CLAMP_AUDIO_BEHAVIOR_H

#include <stdint.h>

class UtilClampAudioBehavior {
public:
    void setup(int32_t minValue,
               int32_t maxValue,
               int64_t targetDurationNanos) {
        mMinValue = minValue;
        mMaxValue = maxValue;
        mTargetDurationNanos = targetDurationNanos;
    }

    int32_t processTiming(int64_t actualDurationNanos) {
        const int64_t nowNanos = HostTools::getNanoTime();
        if (actualDurationNanos >= (int64_t)(mTargetDurationNanos * kBumpUtilization)) {
            bumpUtilClamp(nowNanos);
        } else if (actualDurationNanos > (int64_t)(mTargetDurationNanos * kHighUtilization)) {
            increaseUtilClamp(nowNanos);
        } else if (actualDurationNanos < (int64_t)(mTargetDurationNanos * kLowUtilization)) {
            decreaseUtilClamp(nowNanos);
        } // else do not change current value
        mLastProcessTime = nowNanos;
        return convertToClamp(mCurrent);
    }

    double calculateFractionRealTime(int64_t actualDurationNanos) const {
        return ((double)actualDurationNanos) / mTargetDurationNanos;
    }

    int32_t getMin() const {
        return mMinValue;
    }

    int32_t getMax() const {
        return mMaxValue;
    }

    /**
     * @return fractional utilization that will trigger a bump in frequency.
     */
    double getBumpUtilization() {
        return kBumpUtilization;
    }

private:
    static constexpr int64_t kMillisToNanos = 1e6;

    // Time it takes for the CPU clock frequency to rise in response to
    // a bump in sched_util_min. Based on measurement of Pixel 6.
    static constexpr int64_t kBumpResponseTimeNanos = 4 * kMillisToNanos;
    static constexpr double  kBumpUtilization = 0.99;  // bump if over this
    static constexpr double  kHighUtilization = 0.85;  // high hysteresis threshold
    static constexpr double  kLowUtilization  = 0.60;  // low hysteresis threshold
    static constexpr double  kLowerFraction   = 0.2;   // lower = bump_target * this
    static constexpr double  kBumpIncrement   = 0.1;
    static constexpr double  kLowerDefault    = 0.0;
    static constexpr double  kRisePerSecond   = 3.0;
    static constexpr double  kFallPerSecond   = 0.2;

    int64_t mTargetDurationNanos = 0;
    int64_t mLastProcessTime = 0;
    int64_t mLastBumpTime = 0;
    int32_t mMinValue = 0;    // original value determined by scheduler
    int32_t mMaxValue = 1024; // original value determined by scheduler

    double  mLower = kLowerDefault; // bottom of active fractional range
    double  mCurrent = 0.0;   // current fractional value

    int32_t convertToClamp(float value) const {
        return ((int32_t) (value * (mMaxValue - mMinValue))) + mMinValue;
    }

    bool shouldHaveRespondedToBumpByNow(int64_t nowNanos) const {
        return nowNanos > (mLastBumpTime + kBumpResponseTimeNanos);
    }

    void bumpUtilClamp(int64_t nowNanos) {
        if (shouldHaveRespondedToBumpByNow(nowNanos)) {
            mCurrent = std::min(1.0, mCurrent + kBumpIncrement);
            mLower = mCurrent * kLowerFraction;
            mLastBumpTime = nowNanos;
        } // else wait for CPU to respond to the last bump
    }

    // Slowly fall down.
    void decreaseUtilClamp(int64_t nowNanos) {
        const double elapsedSeconds = (nowNanos - mLastProcessTime) * 1.0e-9;
        if (elapsedSeconds > 0.0) {
            mCurrent -= kFallPerSecond * elapsedSeconds;
            mCurrent = std::max(mLower, mCurrent);
        }
    }

    // Slowly rise up.
    void increaseUtilClamp(int64_t nowNanos) {
        const double elapsedSeconds = (nowNanos - mLastProcessTime) * 1.0e-9;
        if (elapsedSeconds > 0.0) {
            mCurrent += kRisePerSecond * elapsedSeconds;
            mCurrent = std::min(1.0, mCurrent);
        }
    }
};

#endif //ANDROID_UTIL_CLAMP_AUDIO_BEHAVIOR_H
