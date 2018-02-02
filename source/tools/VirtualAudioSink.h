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
#include "LogTool.h"
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
    VirtualAudioSink(LogTool *logTool = NULL)
    : mLogTool(logTool)
    {}

    virtual ~VirtualAudioSink() {
        delete mThread;
    }

    virtual int32_t open(int32_t sampleRate, int32_t samplesPerFrame,
            int32_t framesPerBurst) override {
        mSampleRate = sampleRate;
        mFramesPerBurst = framesPerBurst;
        mBufferSizeInFrames = BUFFER_SIZE_IN_BURSTS * framesPerBurst;
        mMaxBufferCapacityInFrames = MAX_BUFFER_CAPACITY_IN_BURSTS * framesPerBurst;
        mNanosPerBurst = (int32_t) (SYNTHMARK_NANOS_PER_SECOND * mFramesPerBurst / mSampleRate);

        mBurstBuffer = new float[samplesPerFrame * framesPerBurst];
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
        if (mStartTimeNanos == 0) {
            mStartTimeNanos = HostTools::getNanoTime();
            mNextHardwareReadTimeNanos = mStartTimeNanos;
        }
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

    HostThread *getHostThread() {
        return mThread;
    }
    void setHostThread(HostThread *thread) {
        mThread = thread;
    }

    virtual int32_t start() override {
        setFramesWritten(mBufferSizeInFrames);  // start full and primed

        if (mUseRealThread) {
            mCallbackLoopResult = SYNTHMARK_RESULT_THREAD_FAILURE;
            if (mThread == NULL) {
                mThread = new HostThread();
            }
            int err = mThread->start(threadProcWrapper, this);
            if (err != 0) {
                if (mLogTool) {
                    mLogTool->log("ERROR in VirtualAudioSink, thread start() failed, %d\n", err);
                }
                return SYNTHMARK_RESULT_THREAD_FAILURE;
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
            if (mUseRealThread) {
                int err = mThread->promote(mThreadPriority);
                if (err == 0) {
                    setSchedFifoUsed(true);
                }
                if (getRequestedCpu() != SYNTHMARK_CPU_UNSPECIFIED) {
                    err = mThread->setCpuAffinity(getRequestedCpu());
                    if (err) {
                        result = err;
                    } else {
                        setActualCpu(getRequestedCpu());
                    }
                }
            }

            // Render and write audio in a loop.
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
        if (mThread != NULL) {
            int err = mThread->join();
            if (err == 0) {
                mCallbackLoopResult = 0;
            }
            delete mThread;
            mThread = NULL;
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
    bool    mUseRealThread = true; // TODO control using new settings object
    int     mThreadPriority = SYNTHMARK_THREAD_PRIORITY_DEFAULT;   // TODO control using new settings object
    int32_t mCallbackLoopResult = 0;
    HostThread * mThread = NULL;
    LogTool    * mLogTool = NULL;


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
