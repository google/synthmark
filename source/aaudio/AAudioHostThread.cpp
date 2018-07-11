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

//#include <mutex>

#include <aaudio/AAudio.h>
#include "AAudioHostThread.h"

void AAudioHostThread::callHostThread() {
    std::lock_guard<std::mutex> lock(mDoneLock);
    (*mProcedure)(mArgument); // This should take a long time.
    mDone = true; // Let join() know we are done.
}

static aaudio_data_callback_result_t sm_aaudio_data_callback(
        AAudioStream *stream __unused,
        void *userData,
        void *audioData __unused,
        int32_t numFrames __unused) {
    AAudioHostThread *hostThread = (AAudioHostThread *) userData;
    hostThread->callHostThread();
    return AAUDIO_CALLBACK_RESULT_STOP;
}

/** Start the thread and call the specified proc. */
int AAudioHostThread::start(host_thread_proc_t proc, void *arg) {

    mProcedure = proc;
    mArgument = arg;

    AAudioStreamBuilder *aaudioBuilder = nullptr;

    // Use an AAudioStreamBuilder to contain requested parameters.
    aaudio_result_t result = AAudio_createStreamBuilder(&aaudioBuilder);
    if (result != AAUDIO_OK) {
        return result;
    }

    // Request stream properties.
    AAudioStreamBuilder_setDataCallback(aaudioBuilder, sm_aaudio_data_callback, this);
    AAudioStreamBuilder_setPerformanceMode(aaudioBuilder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);

    // Create an AAudioStream using the Builder.
    result = AAudioStreamBuilder_openStream(aaudioBuilder, &mAaudioStream);
    AAudioStreamBuilder_delete(aaudioBuilder);
    if (result != AAUDIO_OK) {
        return result;
    }
    // Start it running.
    result = AAudioStream_requestStart(mAaudioStream);
    if (result != AAUDIO_OK) {
        join();
        return result;
    }
    return result;
}

/** Wait for the thread proc to return. */
int AAudioHostThread::join() {
    AAudioStream *stream = mAaudioStream;
    if (stream != nullptr) {
        while (true) {
            {
                std::lock_guard<std::mutex> lock(mDoneLock); // will block if the thread is running
                if (mDone) break;
            }
            usleep(50 * 1000); // sleep then check again
        }
        mAaudioStream = nullptr;
        return AAudioStream_close(stream);
    }
    return AAUDIO_ERROR_INVALID_STATE;
}
