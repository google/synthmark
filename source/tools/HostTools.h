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

#include <cstdint>
#include <unistd.h>
#include <time.h>
#include <sched.h>
#include <pthread.h>

#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif

#define NANOS_PER_SECOND ((int64_t)1000000000)

#define HOST_IS_APPLE  defined(__APPLE__)

/**
 * Put all low level host platform dependencies in this file.
 */
class HostTools
{
public:

    /**
     * @return system time in nanoseconds, CLOCK_MONOTONIC on Linux
     */
#if HOST_IS_APPLE
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
        return (res.tv_sec * NANOS_PER_SECOND) + res.tv_nsec;
    }
#endif
};

typedef void * host_thread_proc_t(void *arg);

/**
 * Run a specified function on a background thread.
 * This object may only be started once.
 */
class HostThread {
public:
    HostThread(host_thread_proc_t proc, void *arg)
            : mProcedure(proc), mArgument(arg) { }

    ~HostThread() {
        join();
    }

#if HOST_IS_APPLE
    int start() {
        return EPERM;
    }
#else
    int start() {
        int err = 1;
        if (!mRunning && !mDead) {
            err = pthread_create(&mPthread, NULL, mProcedure, mArgument);
            mRunning = (err == 0);
        }
        return err;
    }
#endif

#if HOST_IS_APPLE
    int join() {
        return EPERM;
    }

#else
    int join() {
        int err = 0;
        if (mRunning && !mDead) {
            err = pthread_join(mPthread, NULL);
            mDead = true;
        }
        return err;
    }
#endif

    /**
     * Boost performance of the calling thread and set the thread priority.
     * Set the thread scheduler to SCHED_FIFO or host equivalent.
     *
     * WARNING: This call may not be permitted unless you are running as root!
     */
#if HOST_IS_APPLE
    static int promote(int priority) {
        return -1;
    }
#else
    static int promote(int priority) {
        struct sched_param sp;
        memset(&sp, 0, sizeof(sp));
        sp.sched_priority = priority;
        return sched_setscheduler((pid_t) 0, SCHED_FIFO, &sp);
    }
#endif

private:
    host_thread_proc_t *mProcedure = NULL;
    void *mArgument = NULL;
    bool mRunning = false;
    bool mDead = false;

#if HOST_IS_APPLE
#else
    pthread_t mPthread;
#endif
};
#endif //ANDROID_HOSTTOOLS_H
