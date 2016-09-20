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
#include <unistd.h>
#include <time.h>
#include "SynthMark.h"
#include "AudioSinkBase.h"

#ifndef SYNTHMARK_VIRTUAL_AUDIO_SINK_H
#define SYNTHMARK_VIRTUAL_AUDIO_SINK_H

#ifndef MAX_BUFFER_CAPACITY_IN_BURSTS
#define MAX_BUFFER_CAPACITY_IN_BURSTS 128
#endif

class VirtualAudioSink : public AudioSinkBase
{
public:
    VirtualAudioSink() {}
    virtual ~VirtualAudioSink() {}

    virtual int32_t open(int32_t sampleRate, int32_t samplesPerFrame,
            int32_t framesPerBurst) override {
        mSampleRate = sampleRate;
        (void) samplesPerFrame;
        mFramesPerBurst = framesPerBurst;
        mBufferSizeInFrames = 2 * framesPerBurst;
        mBufferCapacityInFrames = MAX_BUFFER_CAPACITY_IN_BURSTS * framesPerBurst;
        mFramesWritten = mBufferSizeInFrames;  // start full and primed
        mNanosPerBurst = (int32_t) (SYNTHMARK_NANOS_PER_SECOND * mFramesPerBurst / mSampleRate);
        return 0;
    }

    virtual int32_t start() override {
        mStartTimeNanos = TimingAnalyzer::getNanoTime();
        mNextHardwareReadTimeNanos = mStartTimeNanos;
        return 0;
    }

    virtual int64_t convertFrameToTime(int64_t framePosition) {
        return mStartTimeNanos + (framePosition * mNanosPerBurst / mFramesPerBurst);
    }

    int32_t getEmptyFramesAvailable() {
        return mBufferSizeInFrames - getFullFramesAvailable();
    }

    int32_t getFullFramesAvailable() {
        return (int32_t) (mFramesWritten - mFramesConsumed);
    }

    virtual int32_t getUnderrunCount() override {
        return mUnderrunCount;
    }

    /**
     * Set the amount of the buffer that will be used. Determines latency.
     */
    virtual int32_t setBufferSizeInFrames(int32_t numFrames) override {
        if (numFrames < 2) {
            mBufferSizeInFrames = 2;
        } else if (numFrames > mBufferCapacityInFrames) {
            mBufferSizeInFrames = mBufferCapacityInFrames;
        } else {
            mBufferSizeInFrames = numFrames;
        }
        return mBufferSizeInFrames;
    }

    /**
     * Get the size size of the buffer.
     */
    virtual int32_t getBufferSizeInFrames() override {
        return mBufferSizeInFrames;
    }

    /**
     * Get the maximum size of the buffer.
     */
    virtual int32_t getBufferCapacityInFrames() override {
        return mBufferCapacityInFrames;
    }

    /**
     * Sleep until the specified time.
     * @return the time we actually woke up
     */
    static int64_t sleepUntilNanoTime(int64_t wakeupTime) {
        const int32_t kMaxMicros = 999999; // from usleep documentation
        int64_t currentTime = TimingAnalyzer::getNanoTime();
        int64_t nanosToSleep = wakeupTime - currentTime;
        while (nanosToSleep > 0) {
            int32_t microsToSleep = (int32_t)
                    ((nanosToSleep + SYNTHMARK_NANOS_PER_MICROSECOND - 1)
                    / SYNTHMARK_NANOS_PER_MICROSECOND);
            if (microsToSleep < 1) {
                microsToSleep = 1;
            } else if (microsToSleep > kMaxMicros) {
                microsToSleep = kMaxMicros;
            }
            //printf("Sleep for %d micros\n", microsToSleep);
            usleep(microsToSleep);
            currentTime = TimingAnalyzer::getNanoTime();
            nanosToSleep = wakeupTime - currentTime;
        }
        return currentTime;
    }

    int32_t sleepUntilRoomAvailable() {
        updateHardwareSimulator();
        int32_t roomAvailable = getEmptyFramesAvailable();
        while (roomAvailable <= 0) {
            // Calculate when the next buffer will be consumed.
            sleepUntilNanoTime(mNextHardwareReadTimeNanos);
            updateHardwareSimulator();
            roomAvailable = getEmptyFramesAvailable();
        }
        return roomAvailable;
    }

    virtual int32_t write(const float *buffer, int32_t numFrames) override {
        (void) buffer;
        int32_t framesLeft = numFrames;
        while (framesLeft > 0) {
            // Calculate how much room is available in the buffer.
            int32_t available = sleepUntilRoomAvailable();
            if (available > 0) {
                // Simulate writing to a buffer.
                int32_t framesToWrite = (available > framesLeft) ? framesLeft : available;
                mFramesWritten += framesToWrite;
                framesLeft -= framesToWrite;
            }
        }
        return 0;
    }

    virtual int32_t stop() override {
        return 0;
    }

    virtual int32_t close() override {
        return 0;
    }

private:
    int32_t mSampleRate = SYNTHMARK_SAMPLE_RATE;
    int32_t mFramesPerBurst = 128;
    int64_t mStartTimeNanos = 0;
    int32_t mNanosPerBurst = 1;
    int64_t mNextHardwareReadTimeNanos = 0;
    int32_t mBufferSizeInFrames = 0;
    int32_t mBufferCapacityInFrames = 0;
    int64_t mFramesWritten = 0;
    int64_t mFramesConsumed = 0;
    int32_t mUnderrunCount = 0;

    void updateHardwareSimulator() {
        int64_t currentTime = TimingAnalyzer::getNanoTime();
        int countdown = 16; // Avoid spinning like crazy.
        // Is it time to consume a block?
        while ((currentTime >= mNextHardwareReadTimeNanos) && (countdown-- > 0)) {
            int32_t available = getFullFramesAvailable();
            if (available < mFramesPerBurst) {
                // Not enough data!
                mUnderrunCount++;
            }
            mFramesConsumed += mFramesPerBurst; // fake read
            mNextHardwareReadTimeNanos += mNanosPerBurst; // avoid drift
        }
    }
};

#endif // SYNTHMARK_VIRTUAL_AUDIO_SINK_H

