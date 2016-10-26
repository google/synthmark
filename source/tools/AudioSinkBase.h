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

/**
 * Base class for an audio output device.
 * This may be implemented using an actual audio device,
 * or a realtime hardware simulator.
 *
 * You can im
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

private:
    IAudioSinkCallback *mCallback = NULL;
    int64_t        mFramesWritten = 0;
    volatile bool  mSchedFifoUsed = false;
};

#endif // SYNTHMARK_AUDIO_SINK_BASE_H

