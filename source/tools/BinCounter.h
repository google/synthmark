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

#ifndef ANDROID_BINCOUNTER_H
#define ANDROID_BINCOUNTER_H

#include <cstdint>

/**
 * Gather histogram data.
 */
class BinCounter
{
public:
    BinCounter(int32_t numBins)
            : mBins(NULL)
            , mLastMarkers(NULL)
            , mNumBins(0)
            , mIndex(0)
    {
        mBins = new int32_t[numBins]{};
        mLastMarkers = new int32_t[numBins]{};
        mNumBins = numBins;
    }

    virtual ~BinCounter() {
        delete[] mBins;
        delete[] mLastMarkers;
    }

    void increment(int32_t binIndex) {
        if (binIndex < 0) {
            binIndex = 0;
        } else if (binIndex >= mNumBins) {
            binIndex = mNumBins - 1;
        }
        mBins[binIndex] += 1;
        mLastMarkers[binIndex] = mIndex++;
    }

    int32_t getNumBins() const {
        return mNumBins;
    }

    const int32_t *getBins() const {
        return mBins;
    }

    const int32_t *getLastMarkers()  const {
        return mLastMarkers;
    }

private:
    int32_t *mBins;
    int32_t *mLastMarkers;
    int32_t  mNumBins;
    int32_t  mIndex;
};

#endif //ANDROID_BINCOUNTER_H
