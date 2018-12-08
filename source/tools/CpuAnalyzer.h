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

#ifndef SYNTHMARK_CPU_ANALYZER_H
#define SYNTHMARK_CPU_ANALYZER_H

#include <cmath>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "BinCounter.h"
#include "HostTools.h"
#include "SynthMark.h"

constexpr int MAX_CPUS  = 32;

/**
 * Measure CPU migration.
 */
class CpuAnalyzer
{
public:

    void recordCpu() {
        int cpuIndex = HostThread::getCpu();
        // Bump histogram for the current CPU.
        if (cpuIndex >= 0 && cpuIndex < MAX_CPUS) {
            mCpuBins.increment(cpuIndex);
        }
        // Did we change CPUs?
        if (cpuIndex != mPreviousCpu && mPreviousCpu != kCpuIndexInvalid) {
            mMigrationCount++;
        }
        mPreviousCpu = cpuIndex;
        mTotalCount++;
    }

    std::string dump() {
        std::stringstream result;
        result << std::endl << "CPU Core Migration" << std::endl;
        result << "migration.count = " << mMigrationCount << std::endl;
        result << "migration.measurements = " << mTotalCount << std::endl;

        // Report bins with non-zero counts.
        int32_t numBins = mCpuBins.getNumBins();
        const int32_t *cpuCounts = mCpuBins.getBins();
        const int32_t *cpuLast = mCpuBins.getLastMarkers();
        result << TEXT_CSV_BEGIN << std::endl;
        result << " cpu#,    count,     last" << std::endl;
        for (int i = 0; i < numBins; i++) {
            if (cpuCounts[i] > 0) {
                result << "  " << std::setw(3) << i
                << ", " << std::setw(8) << cpuCounts[i]
                << ", " << std::setw(8) << cpuLast[i]
                << std::endl;
            }
        }
        result << TEXT_CSV_END << std::endl;
        return result.str();
    }

private:
    static constexpr int kCpuIndexInvalid = -1;

    int         mPreviousCpu = kCpuIndexInvalid;
    int32_t     mMigrationCount = 0;
    int32_t     mTotalCount = 0;
    BinCounter  mCpuBins{MAX_CPUS};
};

#endif // SYNTHMARK_CPU_ANALYZER_H
