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
#include "time.h"
#include "tools/IAudioSinkCallback.h"
#include "OpenSLHostThread.h"

// OpenSLAudioSink calls this. It then calls the thread proc.
IAudioSinkCallback::Result OpenSLHostThread::renderAudio(
        float *buffer, int32_t numFrames) {
    // Just call the thread proc and let it run for awhile.
    (*mProcedure)(mArgument);
    mRunning = false;
    return IAudioSinkCallback::Result::Finished;
}

int OpenSLHostThread::start(host_thread_proc_t proc, void *arg) {
    mProcedure = proc;
    mArgument = arg;
    int32_t result = mAudioSink.open(48000, 2, 192);
    if (result >= 0) {
        mAudioSink.setCallback(this);
        mRunning = true;
        result = mAudioSink.start();
    }
    return result;
}

int OpenSLHostThread::join() {
    mAudioSink.runCallbackLoop();
    mAudioSink.stop();
    mAudioSink.close();
    mDead = true;
    return 0;
}
