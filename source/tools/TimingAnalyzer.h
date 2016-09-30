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

#include <cstdint>
#include <math.h>
#include <time.h>
#include <string.h>
#include "SynthMark.h"
#include "HostTools.h"

#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif

#ifndef SYNTHMARK_TIMING_ANALYZER_H
#define SYNTHMARK_TIMING_ANALYZER_H

class TimingAnalyzer
{
public:
    TimingAnalyzer()
    : mBins(NULL)
    , mLastMarkers(NULL)
    , mNumBins(0)
    {
        reset();
    }

    virtual ~TimingAnalyzer() {
        delete[] mBins;
        delete[] mLastMarkers;
    }

    void setupJitterRecording(int32_t nanosPerBin, int32_t numBins) {
        mBins = new int32_t[numBins];
        mLastMarkers = new int32_t[numBins];
        memset(mBins, 0, numBins * sizeof(int32_t));
        mNumBins = numBins;
        mNanosPerBin = nanosPerBin;
    }

    void markEntry() {
        int64_t now = HostTools::getNanoTime();
        if (mBaseTime == 0) {
            mBaseTime = now;
        }
        mEntryTime = now;
    }

    void markExit(int64_t idealTime) {
        int64_t now = HostTools::getNanoTime();
        mActiveTime += now - mEntryTime;
        // Calculate jitter delay value for histogram.
        int64_t previousTime = mExitTime;
        mExitTime = now;
        if (mBins != NULL) {
            if (idealTime == 0) {
                if (previousTime > 0) {
                    recordJitter(now - previousTime); // Relative
                }
            } else {
                recordJitter(now - idealTime); // Absolute
            }
        }
        mLoopIndex++;
    }

    void recordJitter(int64_t jitterDuration) {
        // throw away first bin
        if (mLoopIndex > 0) {
            if (jitterDuration < 0) {
                jitterDuration = 0;
            }
            int32_t binIndex = jitterDuration / mNanosPerBin;
            if (binIndex >= mNumBins) {
                binIndex = mNumBins - 1;
            }
            mBins[binIndex] += 1;
            mLastMarkers[binIndex] = mLoopIndex;
        }
    }

    void reset() {
        mBaseTime = 0;
        mEntryTime = 0;
        mExitTime = 0;
        mActiveTime = 0;
        mLoopIndex = 0;
        delete[] mBins;
        mBins = NULL;
        delete[] mLastMarkers;
        mLastMarkers = NULL;
    }

    int64_t getActiveTime() {
        return mActiveTime;
    }

    int32_t getNumBins() {
        return mNumBins;
    }
    int32_t *getBins() {
        return mBins;
    }
    int32_t *getLastMarkers() {
        return mLastMarkers;
    }

    int64_t getTotalTime() {
        return mExitTime - mBaseTime;
    }

    double getDutyCycle() {
        int64_t lastActivePeriod = mExitTime - mEntryTime;
        return (double) (mActiveTime - lastActivePeriod)/ (mEntryTime - mBaseTime);
    }

private:
    int64_t  mBaseTime;
    int64_t  mEntryTime;
    int64_t  mExitTime;
    int64_t  mActiveTime;
    int32_t *mBins;
    int32_t *mLastMarkers;
    int32_t  mNumBins;
    int32_t  mNanosPerBin;
    int32_t  mLoopIndex;
};

#endif // SYNTHMARK_TIMING_ANALYZER_H

