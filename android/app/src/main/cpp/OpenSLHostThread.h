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
#ifndef ANDROID_OPENSLHOSTTHREAD_H
#define ANDROID_OPENSLHOSTTHREAD_H

#include "tools/HostTools.h"
#include "OpenSLAudioSink.h"

/**
 * Thread provider that uses an OpenSL callback to run with SCHED_FIFO.
 * When OpenSL makes the callback, we just hijack the callback and run our benchmark.
 */
class OpenSLHostThread : public HostThread, IAudioSinkCallback {
public:
    OpenSLHostThread() {};
    virtual ~OpenSLHostThread() {};

    /** Start the thread and call the specified proc. */
    virtual int start(host_thread_proc_t proc, void *arg) override;

    /** Wait for the thread proc to return. */
    virtual int join() override;

    /** Try to use SCHED_FIFO. */
    virtual int promote(int priority) override {
        return 0; // Nothing to do because OpenSL thread starts at SCHED_FIFO.
    }

    // Needed for IAudioSinkCallback
    virtual audio_sink_callback_result_t renderAudio(float *buffer, int32_t numFrames) override;

private:
    OpenSLAudioSink  mAudioSink;
};


class OpenSLHostThreadFactory : public HostThreadFactory
{
public:
    OpenSLHostThreadFactory() {}
    virtual ~OpenSLHostThreadFactory() = default;

    virtual HostThread * createThread() {
        return (HostThread *) new OpenSLHostThread();
    }
};
#endif //ANDROID_OPENSLHOSTTHREAD_H
