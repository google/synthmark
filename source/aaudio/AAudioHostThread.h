/*
 * Copyright (C) 2018 The Android Open Source Project
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
#ifndef ANDROID_AAUDIOHOSTTHREAD_H
#define ANDROID_AAUDIOHOSTTHREAD_H

#if defined(__ANDROID__)
#include <mutex>

#include <aaudio/AAudio.h>
#include "tools/HostTools.h"

/**
 * Thread provider that uses an AAudio callback to run with SCHED_FIFO.
 * When AAudio makes the callback, we just hijack the callback and run our benchmark.
 */
class AAudioHostThread : public HostThread {
public:
    virtual ~AAudioHostThread() {};

    /** Start the thread and call the specified proc. */
    int start(host_thread_proc_t proc, void *arg) override;

    /** Wait for the thread proc to return. */
    int join() override;

    /** Try to use SCHED_FIFO. */
    int promote(int priority) override {
        return 0; // Nothing to do because AAudio thread starts at SCHED_FIFO.
    }

    void callHostThread();

private:
    AAudioStream       *mAaudioStream = nullptr;
    std::mutex          mDoneLock; // Use a mutex so we can sleep on it while join()ing.
    std::atomic<bool>   mDone{false};
};

#endif // __ANDROID__
#endif // ANDROID_AAUDIOHOSTTHREAD_H
