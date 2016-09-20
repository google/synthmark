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

#include <cstdint>
#include <math.h>
#include "SynthMark.h"

#ifndef SYNTHMARK_AUDIO_SINK_BASE_H
#define SYNTHMARK_AUDIO_SINK_BASE_H

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

    virtual int32_t open(int32_t sampleRate, int32_t samplesPerFrame, int32_t framesPerBuffer) = 0;
    virtual int32_t start() = 0;
    virtual int32_t write(const float *buffer, int32_t numFrames) = 0;
    virtual int32_t stop() = 0;
    virtual int32_t close() = 0;

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

    /**
     * @return valid time or zero
     */
    virtual int64_t convertFrameToTime(int64_t framePosition) {
        (void) framePosition;
        return 0;
    }


    virtual int32_t terminate() {
        return 0;
    }

};

#endif // SYNTHMARK_AUDIO_SINK_BASE_H

