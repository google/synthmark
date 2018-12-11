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

    void setupHistograms(int32_t nanosPerBin, int32_t numBins) {
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
        if (mCallCount > 0) {
            if (mWakeupBins != NULL) {
                // Be fair. We can't wake up before we go to sleep.
                int64_t realisticWakeTime = (mExitTime > idealTime) ? mExitTime : idealTime;
                int64_t wakeupDelay = now - realisticWakeTime;
                mTotalWakeupDelay += wakeupDelay;
                int32_t binIndex = wakeupDelay / mNanosPerBin;
                mWakeupBins->increment(binIndex);
            }
        }
    }

    // This is called right after the audio task finishes computation.
    void markExit() {
        int64_t now = HostTools::getNanoTime();
        mLastRenderDuration = now - mEntryTime;
        mActiveTime += mLastRenderDuration; // for CPU load calculation
        // Calculate jitter delay values for histogram.
        mExitTime = now;
        if (mCallCount > 0) {
            if (mRenderBins != NULL) {
                int32_t binIndex = mLastRenderDuration / mNanosPerBin;
                mRenderBins->increment(binIndex);
            }
            if (mDeliveryBins != NULL) {
                int64_t deliveryTime = now - mIdealTime;
                int32_t binIndex = deliveryTime / mNanosPerBin;
                mDeliveryBins->increment(binIndex);
            }
        }
        mCallCount++;
    }

    void reset() {
        mBaseTime = 0;
        mIdealTime = 0;
        mEntryTime = 0;
        mExitTime = 0;
        mActiveTime = 0;
        mCallCount = 0;
        mTotalWakeupDelay = 0;
        delete mWakeupBins;
        delete mRenderBins;
        delete mDeliveryBins;
    }

    int64_t getActiveTime() {
        return mActiveTime;
    }

    int32_t getCallCount() {
        return mCallCount;
    }

    int64_t getTotalWakeupDelayNanos() {
        return mTotalWakeupDelay;
    }

    int64_t getLastRenderDurationNanos() {
        return mLastRenderDuration;
    }

    int64_t getLastEntryTime() {
        return mEntryTime;
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

    std::string dumpJitter() {
        const bool showDeliveryTime = false;
        std::stringstream resultMessage;
        // Print jitter histogram
        if (mWakeupBins != NULL && mRenderBins != NULL && mDeliveryBins != NULL) {
            resultMessage << TEXT_CSV_BEGIN << std::endl;
            int32_t numBins = mDeliveryBins->getNumBins();
            const int32_t *wakeupCounts = mWakeupBins->getBins();
            const int32_t *wakeupLast = mWakeupBins->getLastMarkers();
            const int32_t *renderCounts = mRenderBins->getBins();
            const int32_t *renderLast = mRenderBins->getLastMarkers();
            const int32_t *deliveryCounts = mDeliveryBins->getBins();
            const int32_t *deliveryLast = mDeliveryBins->getLastMarkers();
            resultMessage << " bin#,  msec,"
                          << "   wakeup#,  wlast,"
                          << "   render#,  rlast";
            if (showDeliveryTime) {
                resultMessage << " delivery#,  dlast";
            }
            resultMessage << std::endl;
            for (int i = 0; i < numBins; i++) {
                if (wakeupCounts[i] > 0 || renderCounts[i] > 0
                    || (deliveryCounts[i] > 0 && showDeliveryTime)) {
                    double msec = (double) i * mNanosPerBin * SYNTHMARK_MILLIS_PER_SECOND
                                  / SYNTHMARK_NANOS_PER_SECOND;
                    resultMessage << "  " << std::setw(3) << i
                                  << ", " << std::fixed << std::setw(5) << std::setprecision(2)
                                  << msec
                                  << ", " << std::setw(9) << wakeupCounts[i]
                                  << ", " << std::setw(6) << wakeupLast[i]
                                  << ", " << std::setw(9) << renderCounts[i]
                                  << ", " << std::setw(6) << renderLast[i];
                    if (showDeliveryTime) {
                        resultMessage << ", " << std::setw(9) << deliveryCounts[i]
                                      << ", " << std::setw(6) << deliveryLast[i];
                    }
                    resultMessage << std::endl;
                }
            }
            resultMessage << TEXT_CSV_END << std::endl;

            double averageWakeupDelayMicros = getTotalWakeupDelayNanos()
                    / (double) (mCallCount * SYNTHMARK_NANOS_PER_MICROSECOND);
            resultMessage << "average.wakeup.delay.micros = " << averageWakeupDelayMicros
                          << std::endl;
        } else {
            resultMessage << "ERROR NULL BinCounter!\n";
        }
        return resultMessage.str();
    }

private:
    int64_t  mBaseTime;
    int64_t  mIdealTime;
    int64_t  mEntryTime;
    int64_t  mExitTime;
    int64_t  mActiveTime;
    int64_t  mTotalWakeupDelay;
    int64_t  mLastRenderDuration = 0;
    BinCounter *mWakeupBins;
    BinCounter *mRenderBins;
    BinCounter *mDeliveryBins;
    int32_t  mNanosPerBin;
    int32_t  mCallCount;
};

#endif // SYNTHMARK_TIMING_ANALYZER_H
