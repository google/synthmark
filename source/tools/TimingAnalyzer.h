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

#ifndef SYNTHMARK_TIMING_ANALYZER_H
#define SYNTHMARK_TIMING_ANALYZER_H

#include <cmath>
#include <cstdint>
#include <cstring>
#include <ctime>

#include "BinCounter.h"
#include "HostTools.h"
#include "SynthMark.h"

#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif


class TimingAnalyzer
{
public:
    TimingAnalyzer()
            : mWakeupBins(NULL)
            , mRenderBins(NULL)
            , mDeliveryBins(NULL)
    {
        reset();
    }

    virtual ~TimingAnalyzer() {
        delete mWakeupBins;
        delete mRenderBins;
        delete mDeliveryBins;
    }

    void setupJitterRecording(int32_t nanosPerBin, int32_t numBins) {
        mWakeupBins = new BinCounter(numBins);
        mRenderBins = new BinCounter(numBins);
        mDeliveryBins = new BinCounter(numBins);
        mNanosPerBin = nanosPerBin;
    }

    /**
     * This is called soon after the audio task wakes up and right before it
     * begins doing the audio computation.
     *
     * @param idealTime when the audio task should have woken up
     */
    void markEntry(int64_t idealTime) {
        int64_t now = HostTools::getNanoTime(); // when we actually woke up
        if (mBaseTime == 0) {
            mBaseTime = now; // when we started
        }
        mIdealTime = idealTime;
        mEntryTime = now;
        if (mLoopIndex > 0) {
            if (mWakeupBins != NULL) {
                // Be fair. We can't wake up before we go to sleep.
                int64_t realisticWakeTime = (mExitTime > idealTime) ? mExitTime : idealTime;
                int64_t wakeupDelay = now - realisticWakeTime;
                int32_t binIndex = wakeupDelay / mNanosPerBin;
                mWakeupBins->increment(binIndex);
            }
        }
    }

    // This is called right after the audio task finishes computation.
    void markExit() {
        int64_t now = HostTools::getNanoTime();
        int64_t renderTime = now - mEntryTime;
        mActiveTime += renderTime; // for CPU load calculation
        // Calculate jitter delay values for histogram.
        mExitTime = now;
        if (mLoopIndex > 0) {
            if (mRenderBins != NULL) {
                int32_t binIndex = renderTime / mNanosPerBin;
                mRenderBins->increment(binIndex);
            }
            if (mDeliveryBins != NULL) {
                int64_t deliveryTime = now - mIdealTime;
                int32_t binIndex = deliveryTime / mNanosPerBin;
                mDeliveryBins->increment(binIndex);
            }
        }
        mLoopIndex++;
    }

//    void recordJitter(int64_t jitterDuration) {
//        // throw away first bin
//        if (mLoopIndex > 0) {
//            int32_t binIndex = jitterDuration / mNanosPerBin;
//            mDeliveryBins->increment(binIndex);
//        }
//    }

    void reset() {
        mBaseTime = 0;
        mIdealTime = 0;
        mEntryTime = 0;
        mExitTime = 0;
        mActiveTime = 0;
        mLoopIndex = 0;
        delete mWakeupBins;
        delete mRenderBins;
        delete mDeliveryBins;
    }

    int64_t getActiveTime() {
        return mActiveTime;
    }


    int64_t getTotalTime() {
        return mExitTime - mBaseTime;
    }

    /**
     * @return average duty cycle since the test began
     */
    double getDutyCycle() {
        int64_t lastActivePeriod = mExitTime - mEntryTime;
        int64_t totalTime = mEntryTime - mBaseTime;
        if (totalTime <= 0) {
            return 0.0;
        } else {
            return (double) (mActiveTime - lastActivePeriod) / totalTime;
        }
    }

    BinCounter *getWakeupBins() {
        return mWakeupBins;
    }
    BinCounter *getRenderBins() {
        return mRenderBins;
    }
    BinCounter *getDeliveryBins() {
        return mDeliveryBins;
    }

private:
    int64_t  mBaseTime;
    int64_t  mIdealTime;
    int64_t  mEntryTime;
    int64_t  mExitTime;
    int64_t  mActiveTime;
    BinCounter *mWakeupBins;
    BinCounter *mRenderBins;
    BinCounter *mDeliveryBins;
    int32_t  mNanosPerBin;
    int32_t  mLoopIndex;
};

#endif // SYNTHMARK_TIMING_ANALYZER_H
