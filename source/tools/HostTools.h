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

#ifndef ANDROID_HOSTTOOLS_H
#define ANDROID_HOSTTOOLS_H

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <cerrno>
#include <memory.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <mach/mach_time.h>
#else
#include <sys/sysinfo.h>
#endif

constexpr int64_t kNanosPerMicrosecond  = 1000;
constexpr int64_t kNanosPerSecond       = 1000000 * kNanosPerMicrosecond;

constexpr int kMaxCpuCount = 64;

/**
 * Put all low level host platform dependencies in this file.
 */
class HostTools
{
public:

    /**
     * @return system time in nanoseconds, CLOCK_MONOTONIC on Linux
     */
#if defined(__APPLE__)
    static int64_t getNanoTime() {
        mach_timebase_info_data_t info;
        mach_timebase_info(&info);
        return (int64_t)(mach_absolute_time() * info.numer / info.denom);
    }
#else
    static int64_t getNanoTime() {
        struct timespec res;
        int result = clock_gettime(CLOCK_MONOTONIC, &res);
        if (result < 0) {
            return result;
        }
        return (res.tv_sec * kNanosPerSecond) + res.tv_nsec;
    }
#endif

    /**
     * Sleep for the specified nanoseconds.
     * @return the time we actually woke up
     */
    static int64_t sleepForNanoseconds(int64_t sleepTime) {
        int64_t wakeupTime = getNanoTime() + sleepTime;
        return sleepUntilNanoTime(wakeupTime);
    }

    /**
     * Sleep until the specified time.
     * @return the time we actually woke up
     */
    static int64_t sleepUntilNanoTime(int64_t wakeupTime) {
        const int32_t kMaxMicros = 999999; // from usleep documentation
        int64_t currentTime = getNanoTime();
        int64_t nanosToSleep = wakeupTime - currentTime;
        while (nanosToSleep > 0) {
            int32_t microsToSleep = (int32_t)
                    ((nanosToSleep + kNanosPerMicrosecond - 1)
                     / kNanosPerMicrosecond);
            if (microsToSleep < 1) {
                microsToSleep = 1;
            } else if (microsToSleep > kMaxMicros) {
                microsToSleep = kMaxMicros;
            }
            usleep(microsToSleep);
            currentTime = getNanoTime();
            nanosToSleep = wakeupTime - currentTime;
        }
        return currentTime;
    }

    static int getCpuCount() {
#if defined(__APPLE__)
        return -1;
#else
        return get_nprocs();
#endif
    }

};

typedef void * host_thread_proc_t(void *arg);

/**
 * Run a specified function on a background thread.
 * This object may only be started once.
 */
class HostThread {
public:
    // Note that destructors should not call virtual methods.
    virtual ~HostThread() {};

    virtual int start(host_thread_proc_t proc, void *arg) {
        mProcedure = proc;
        mArgument = arg;
        int err = 1;
        if (!mRunning && !mDead) {
            err = pthread_create(&mPthread, NULL, mProcedure, mArgument);
            mRunning = (err == 0);
        }
        return err;
    }

    static void yield() {
        sched_yield();
    }

    virtual int join() {
        int err = 0;
        if (mRunning && !mDead) {
            err = pthread_join(mPthread, NULL);
            mDead = true;
        }
        return err;
    }

    /**
     * Boost performance of the calling thread and set the thread priority.
     * Set the thread scheduler to SCHED_FIFO or host equivalent.
     *
     * WARNING: This call may not be permitted unless you are running as root!
     *
     * @return 0 on success or a negative errno
     */
    virtual int promote(int priority) {
#if defined(__APPLE__)
        return -1;
#else
        struct sched_param sp;
        memset(&sp, 0, sizeof(sp));
        sp.sched_priority = priority;
        int err = sched_setscheduler((pid_t) 0, SCHED_FIFO, &sp);
        return err == 0 ? 0 : -errno;
#endif
    }

    /**
     * @param cpuIndex
     * @return 0 on success or a negative errno
     */
    static int setCpuAffinity(int cpuIndex) {
#if defined(__APPLE__)
        return -1;
#else
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        CPU_SET(cpuIndex, &cpu_set);
        int err = sched_setaffinity((pid_t) 0, sizeof(cpu_set_t), &cpu_set);
        return err == 0 ? 0 : -errno;
#endif
    }

    static int getCpu() {
#if defined(__APPLE__)
        return -1;
#else
        return sched_getcpu();
#endif
    }

protected:
    host_thread_proc_t *mProcedure = NULL;
    void               *mArgument = NULL;
    volatile bool       mRunning = false;
    bool                mDead = false;

    pthread_t           mPthread;
};


class HostCpuManagerBase
{
public:

    /**
     * Sleep until the specified time and tune the CPU for optimal performance.
     *
     * @param wakeupTime MONOTONIC time to wake up or zero for no sleep
     */
    virtual void sleepAndTuneCPU(int64_t wakeupTime) = 0;

    /**
     * This is called when the application determines its work load for the callback.
     *
     * It calculates what clock speed would be required for that load and
     * requests a higher clock speed if necessary.
     *
     * @param currentWorkUnits
     * @param maxWorkUnits
     */
    virtual void setApplicationLoad(int32_t currentWorkUnits, int32_t maxWorkUnits) {
        mCurrentWorkUnits = currentWorkUnits;
        mMaxWorkUnits = maxWorkUnits;
    }

    int32_t getCurrentWorkUnits() const {
        return mCurrentWorkUnits;
    }

    int32_t getMaxWorkUnits() const {
        return mMaxWorkUnits;
    }

    void setNanosPerBurst(int64_t nanosPerBurst) {
        mNanosPerBurst = nanosPerBurst;
    }

    int64_t getNanosPerBurst() const {
        return mNanosPerBurst;
    }

private:

    int32_t mCurrentWorkUnits = 0;
    int32_t mMaxWorkUnits = 1;
    int64_t mNanosPerBurst = 0;
};


/**
 * Measure CPU utilization and then give hints to the CPU governor stub to control clock speed.
 * This is a stub that you can replace in HostCpuManager::getInstance() below.
 * This stub is only meant to illustrate the idea and may be implemented very differently
 * in a real system.
 */
class HostCpuManagerStub : public HostCpuManagerBase
{
public:

    void sleepAndTuneCPU(int64_t wakeupTime) override {

        mEndingCpu = HostThread::getCpu();
        mEndingTime = HostTools::getNanoTime();
        if (mStartingCpu == mEndingCpu) { // probably did not migrate CPUs
            calculateNormalizedRuntime(mEndingCpu, mEndingTime - mStartingTime);
        }

        if (wakeupTime > 0) {
            HostTools::sleepUntilNanoTime(wakeupTime); // SLEEP
        }

        mStartingCpu = HostThread::getCpu();
        mStartingTime = HostTools::getNanoTime();
    };

    /**
     * This is called when the application determines its work load for the callback.
     *
     * It calculates what clock speed would be required for that load and
     * requests a higher clock speed if necessary.
     *
     * @param currentWorkUnits
     * @param maxWorkUnits
     */
    void setApplicationLoad(int32_t currentWorkUnits, int32_t maxWorkUnits) override {
        // Did anything change?
        if (currentWorkUnits != getCurrentWorkUnits()
            || maxWorkUnits != getMaxWorkUnits()) {

            // Calculate CPU speed needed to carry this load.
            int cpuIndex = HostThread::getCpu();
            CpuTiming &cpuTiming = mCpuTimings[cpuIndex];
            // Use previously measured statistics.
            double normalizedWorkUnitUtilization = cpuTiming.getNormalizedWorkUnitUtilization();

            int32_t maxClockSpeedMHz     = getMaxCpuSpeedMHz(cpuIndex); // FIXME get from device
            const double targetUtilization = 0.8; // arbitrary
            double normalizedCurrentUtilization = currentWorkUnits * normalizedWorkUnitUtilization;
            double requiredClockSpeed = normalizedCurrentUtilization * maxClockSpeedMHz
                                        / targetUtilization;
//            printf("workUnits = %d, requestCpuSpeedMHz(%d, %d)\n",
//                   currentWorkUnits, cpuIndex, (int32_t) requiredClockSpeed);
            requestCpuSpeedMHz(cpuIndex, (int32_t) requiredClockSpeed);
        }
        HostCpuManagerBase::setApplicationLoad(currentWorkUnits, maxWorkUnits);
    }

private:

    // ------------------------------------------------------------------------------
    // Note that these are just fake values for this stub implementation.
    // A real custom implementation would use real values, or may use
    // some other way of controlling CPU speed that is more indirect.
    int32_t mFakeCpuSpeed = 600;
    const int32_t mFakeMaxCpuSpeed = 2000;
    int32_t getCpuSpeedMHz(int cpuIndex) { return mFakeCpuSpeed; } // FIXME
    int32_t getMaxCpuSpeedMHz(int cpuIndex) { return mFakeMaxCpuSpeed; } // FIXME

    void requestCpuSpeedMHz(int cpuIndex, int32_t cpuSpeedMHz) {  // FIXME
        if (cpuSpeedMHz < 300) cpuSpeedMHz = 300;
        else if (cpuSpeedMHz > mFakeMaxCpuSpeed) cpuSpeedMHz = mFakeMaxCpuSpeed;
        mFakeCpuSpeed =  cpuSpeedMHz;
    }
    // ------------------------------------------------------------------------------

    /**
     * Calculate the normalized CPU runtime per work unit.
     *
     * @param cpuIndex
     * @param callbackComputingTime
     */
    void calculateNormalizedRuntime(int cpuIndex, int64_t callbackComputingTime) {
        if (getCurrentWorkUnits() <= 0) {
            return;
        }
        assert(cpuIndex < kMaxCpuCount);
        assert(getNanosPerBurst() > 0);
        CpuTiming &cpuTiming = mCpuTimings[cpuIndex];
        int32_t currentClockSpeedMHz =  getCpuSpeedMHz(cpuIndex);
        int32_t maxClockSpeedMHz     = getMaxCpuSpeedMHz(cpuIndex);
        double normalizedCpuSpeed = currentClockSpeedMHz / (double) maxClockSpeedMHz;
        cpuTiming.reportApplicationTiming(callbackComputingTime,
                                          getNanosPerBurst(),
                                          getCurrentWorkUnits(),
                                          normalizedCpuSpeed);
    }

    /**
     * Measure utilization for one CPU.
     */
    class CpuTiming {
    public:

        double getNormalizedWorkUnitUtilization() const { return mNormalizedWorkUnitUtilization; }

        void reportApplicationTiming(int64_t callbackComputingTime,
                                     int64_t audioTime,
                                     int32_t currentWorkUnits,
                                     double normalizedCpuSpeed) {
            double callbackUtilization = callbackComputingTime / (double) audioTime;
            double normalizedWorkUnitUtilization = normalizedCpuSpeed * callbackUtilization
                                             / currentWorkUnits;
            // Simple averaging IIR to smooth out this measured value.
            mNormalizedWorkUnitUtilization =
                    (mNormalizedWorkUnitUtilization + normalizedWorkUnitUtilization) * 0.5;
        }
    private:
        double mNormalizedWorkUnitUtilization = 0.0;
    };

    CpuTiming mCpuTimings[kMaxCpuCount];

    int       mStartingCpu = -1;
    int64_t   mStartingTime = 0;
    int       mEndingCpu = -1;
    int64_t   mEndingTime = 0;
};

#include "CustomHostCpuManager.h"

/**
 * Return a singleton instance of a HostCpuManagerBase.
 */
class HostCpuManager
{
public:

    enum : int32_t {
        WORKLOAD_HINTS_OFF = 0,
        WORKLOAD_HINTS_ON = 1,
        WORKLOAD_HINTS_ON_LOGGED = 2
    };

    static HostCpuManagerBase *getInstance() {
        if (mInstance == nullptr) {
            if (areWorkloadHintsEnabled()) {
                mInstance = new CustomHostCpuManager();
            } else {
                mInstance = new HostCpuManagerStub();
            }
        }
        return mInstance;
    }

    static int32_t getWorkloadHintsLevel() {
        return mWorkloadHintsLevel;
    }

    static void setWorkloadHintsLevel(int32_t level) {
        mWorkloadHintsLevel = level;
    }

    static bool areWorkloadHintsEnabled() {
        return mWorkloadHintsLevel == WORKLOAD_HINTS_ON
               || mWorkloadHintsLevel == WORKLOAD_HINTS_ON_LOGGED;
    }

private:
    HostCpuManager() {}
    static HostCpuManagerBase *mInstance;
    static int32_t             mWorkloadHintsLevel;

};

#endif //ANDROID_HOSTTOOLS_H
