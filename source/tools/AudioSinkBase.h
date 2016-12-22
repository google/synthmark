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

#ifndef SYNTHMARK_AUDIO_SINK_BASE_H
#define SYNTHMARK_AUDIO_SINK_BASE_H

#include <cstdint>
#include <math.h>
#include "SynthMark.h"
#include "IAudioSinkCallback.h"

#define SYNTHMARK_THREAD_PRIORITY_DEFAULT   2 // 2nd lowest priority, recommended by timmurray@
#define SYNTHMARK_CPU_UNSPECIFIED           -1

/**
 * Base class for an audio output device.
 * This may be implemented using an actual audio device,
 * or a realtime hardware simulator.
 */
class AudioSinkBase
{
public:
    AudioSinkBase() {}
    virtual ~AudioSinkBase() {}

    virtual int32_t initialize() {
        return 0;
    }

    virtual int32_t open(int32_t sampleRate, int32_t samplesPerFrame, int32_t framesPerBurst) = 0;
    virtual int32_t start() = 0;

    /**
     * You don't need to implement a real blocking write if you are using
     * a real audio callback, eg. OpenSL ES. You can just implement a stub.
     */
    virtual int32_t write(const float *buffer, int32_t numFrames) = 0;

    virtual int32_t stop() = 0;
    virtual int32_t close() = 0;

    void setCallback(IAudioSinkCallback *callback) {
        mCallback = callback;
    }

    IAudioSinkCallback * getCallback() {
        return mCallback;
    }

    /**
     * Wait for callback loop to complete.
     * This may wait for a real hardware device to finish,
     * or it may call the callback itself in a loop
     */
    virtual int32_t runCallbackLoop() = 0;

    virtual IAudioSinkCallback::audio_sink_callback_result_t
            fireCallback(float *buffer, int32_t numFrames) {
        return mCallback->renderAudio(buffer, numFrames);
    }

    /**
     * Set the amount of the buffer that will be used. Determines latency.
     * @return the actual size, which may not match the requested size, or a negative error
     */
    virtual int32_t setBufferSizeInFrames(int32_t numFrames) = 0;

    /**
     * Get the size size of the buffer.
     */
    virtual int32_t getBufferSizeInFrames() = 0;

    /**
     * Get the maximum size of the buffer.
     */
    virtual int32_t getBufferCapacityInFrames() = 0;

    virtual int32_t getUnderrunCount() {
        return -1; // UNIMPLEMENTED
    }

    virtual int32_t getSampleRate() = 0;

    void setSchedFifoUsed(bool schedFifoUsed) {
        mSchedFifoUsed = schedFifoUsed;
    }
    bool wasSchedFifoUsed() {
        return mSchedFifoUsed;
    }

    void setFramesWritten(int32_t framesWritten) {
        mFramesWritten = framesWritten;
    }

    int32_t getFramesWritten() {
        return mFramesWritten;
    }

    /**
     * @return valid time or zero
     */
    virtual int64_t convertFrameToTime(int64_t framePosition) {
        (void) framePosition;
        return (int64_t) 0;
    }

    virtual int32_t terminate() {
        return 0;
    }

    int getActualCpu() {
        return mActualCpu;
    }
    int getRequestedCpu() {
        return mRequestedCpu;
    }

    /**
     * Set CPU affinity to the requested index.
     * Index range is zero to a device specific maximum, typically 3 or 7.
     */
    void setRequestedCpu(int cpuAffinity) {
        mRequestedCpu = cpuAffinity;
    }
protected:
    void setActualCpu(int cpuAffinity) {
        mActualCpu = cpuAffinity;
    }

private:
    IAudioSinkCallback *mCallback = NULL;
    int64_t        mFramesWritten = 0;
    volatile bool  mSchedFifoUsed = false;
    int            mRequestedCpu = SYNTHMARK_CPU_UNSPECIFIED;
    int            mActualCpu = SYNTHMARK_CPU_UNSPECIFIED;
};

#endif // SYNTHMARK_AUDIO_SINK_BASE_H

