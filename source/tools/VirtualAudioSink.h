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
#include "HostTools.h"
#include "SynthMarkResult.h"
#include "AudioSinkBase.h"

#ifndef SYNTHMARK_VIRTUAL_AUDIO_SINK_H
#define SYNTHMARK_VIRTUAL_AUDIO_SINK_H

#ifndef MAX_BUFFER_CAPACITY_IN_BURSTS
#define MAX_BUFFER_CAPACITY_IN_BURSTS 128
#endif

// Use 2 for double buffered
#define BUFFER_SIZE_IN_BURSTS 8

class VirtualAudioSink : public AudioSinkBase
{
public:
    VirtualAudioSink() {}
    virtual ~VirtualAudioSink() {}

    virtual int32_t open(int32_t sampleRate, int32_t samplesPerFrame __unused,
            int32_t framesPerBurst) override {
        mSampleRate = sampleRate;
        mFramesPerBurst = framesPerBurst;
        mBufferSizeInFrames = BUFFER_SIZE_IN_BURSTS * framesPerBurst;
        mMaxBufferCapacityInFrames = MAX_BUFFER_CAPACITY_IN_BURSTS * framesPerBurst;
        mNanosPerBurst = (int32_t) (SYNTHMARK_NANOS_PER_SECOND * mFramesPerBurst / mSampleRate);

        mBurstBuffer = new float[samplesPerFrame * framesPerBurst];
        return 0;
    }

    virtual int32_t start() override {
        setFramesWritten(mBufferSizeInFrames);  // start full and primed
        mStartTimeNanos = HostTools::getNanoTime();
        mNextHardwareReadTimeNanos = mStartTimeNanos;
        return 0;
    }

    virtual int64_t convertFrameToTime(int64_t framePosition) override {
        return (int64_t) (mStartTimeNanos + (framePosition * mNanosPerBurst / mFramesPerBurst));
    }

    int32_t getEmptyFramesAvailable() {
        return mBufferSizeInFrames - getFullFramesAvailable();
    }

    int32_t getFullFramesAvailable() {
        return (int32_t) (getFramesWritten() - mFramesConsumed);
    }

    virtual int32_t getUnderrunCount() override {
        return mUnderrunCount;
    }

    int32_t getSampleRate() override {
        return mSampleRate;
    }

    /**
     * Set the amount of the buffer that will be used. Determines latency.
     */
    virtual int32_t setBufferSizeInFrames(int32_t numFrames) override {
        if (numFrames < 2) {
            mBufferSizeInFrames = 2;
        } else if (numFrames > mMaxBufferCapacityInFrames) {
            mBufferSizeInFrames = mMaxBufferCapacityInFrames;
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
        return mMaxBufferCapacityInFrames;
    }

    /**
     * Sleep until the specified time.
     * @return the time we actually woke up
     */
    static int64_t sleepUntilNanoTime(int64_t wakeupTime) {
        const int32_t kMaxMicros = 999999; // from usleep documentation
        int64_t currentTime = HostTools::getNanoTime();
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
            currentTime = HostTools::getNanoTime();
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
                setFramesWritten(getFramesWritten() + framesToWrite);
                framesLeft -= framesToWrite;
            }
        }
        return 0;
    }

    virtual int32_t stop() override {
        return 0;
    }

    virtual int32_t close() override {
        delete[] mBurstBuffer;
        return 0;
    }

    /**
     * Call the callback in a loop until it is finished or an error occurs.
     * Data from the callback will be passed to the write() method.
     */
    int32_t innerCallbackLoop() {
        int32_t result = SYNTHMARK_RESULT_SUCCESS;
        IAudioSinkCallback *callback = getCallback();
        setSchedFifoUsed(false);
        if (callback != NULL) {
            if (mUseRealThread && isSchedFifoEnabled()) {
                int err = HostThread::promote(mThreadPriority);
                if (err) {
                    result = err;
                } else {
                    setSchedFifoUsed(true);
                }
            }

            IAudioSinkCallback::audio_sink_callback_result_t callbackResult
                    = IAudioSinkCallback::CALLBACK_CONTINUE;
            while (callbackResult == IAudioSinkCallback::CALLBACK_CONTINUE
                    && result == SYNTHMARK_RESULT_SUCCESS) {
                // Gather audio data from the synthesizer.
                callbackResult = fireCallback(mBurstBuffer, mFramesPerBurst);
                if (callbackResult == IAudioSinkCallback::CALLBACK_CONTINUE) {
                    // Output the audio using a blocking write.
                    if (write(mBurstBuffer, mFramesPerBurst) < 0) {
                        result = SYNTHMARK_RESULT_AUDIO_SINK_WRITE_FAILURE;
                    }
                }
            }
        }
        mCallbackLoopResult = result;
        return result;
    }

    static void * threadProcWrapper(void *arg) {
        VirtualAudioSink *sink = (VirtualAudioSink *) arg;
        sink->innerCallbackLoop();
        return NULL;
    }

    /**
     * Call the callback directly or from a thread.
     */
    virtual int32_t runCallbackLoop() override {
        if (mUseRealThread) {
            mCallbackLoopResult = SYNTHMARK_RESULT_THREAD_FAILURE;
            HostThread thread(threadProcWrapper, this);
            int err = thread.start();
            if (err != 0) {
                return SYNTHMARK_RESULT_THREAD_FAILURE;
            } else {
                err = thread.join();
                if (err != 0) {
                    return SYNTHMARK_RESULT_THREAD_FAILURE;
                }
            }
            // TODO use memory barrier or pass back through join()
            return mCallbackLoopResult;
        } else {
            return innerCallbackLoop();
        }
    }


private:
    int32_t mSampleRate = SYNTHMARK_SAMPLE_RATE;
    int32_t mFramesPerBurst = 64;
    int64_t mStartTimeNanos = 0;
    int32_t mNanosPerBurst = 1;
    int64_t mNextHardwareReadTimeNanos = 0;
    int32_t mBufferSizeInFrames = 0;
    int32_t mMaxBufferCapacityInFrames = 0;
    int64_t mFramesConsumed = 0;
    int32_t mUnderrunCount = 0;
    float*  mBurstBuffer = NULL;   // contains output of the synthesizer
    bool    mUseRealThread = true;   // TODO control using new settings object
    int     mThreadPriority = 2;   // TODO control using new settings object
    int32_t mCallbackLoopResult = 0;

    void updateHardwareSimulator() {
        int64_t currentTime = HostTools::getNanoTime();
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

