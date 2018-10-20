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

#ifndef SYNTHMARK_VIRTUAL_AUDIO_SINK_H
#define SYNTHMARK_VIRTUAL_AUDIO_SINK_H

#include <cstdint>
#include <ctime>
#include <unistd.h>

#include "AudioSinkBase.h"
#include "HostTools.h"
#include "HostThreadFactory.h"
#include "LogTool.h"
#include "SynthMark.h"
#include "SynthMarkResult.h"


constexpr int kMaxBufferCapacityInBursts = 128;

// Use 2 for double buffered
constexpr int kBufferSizeInBursts = 8;

class VirtualAudioSink : public AudioSinkBase
{
public:
    VirtualAudioSink(LogTool *logTool = NULL)
    : mLogTool(logTool)
    {}

    virtual ~VirtualAudioSink() {
        delete mThread;
    }

    int32_t open(int32_t sampleRate, int32_t samplesPerFrame,
            int32_t framesPerBurst) override {
        int32_t result = AudioSinkBase::open(sampleRate, samplesPerFrame, framesPerBurst);
        if (result < 0) {
            return result;
        }

        mBufferSizeInFrames = kBufferSizeInBursts * framesPerBurst;
        mMaxBufferCapacityInFrames = kMaxBufferCapacityInBursts * framesPerBurst;

        int64_t nanosPerBurst = mFramesPerBurst * SYNTHMARK_NANOS_PER_SECOND / mSampleRate;
        mNanosPerBurst = (int32_t) nanosPerBurst;
        HostCpuManager::getInstance()->setNanosPerBurst(nanosPerBurst);

        mBurstBuffer = new float[samplesPerFrame * framesPerBurst];

        mNextHardwareReadTimeNanos = 0;
        mFramesConsumed = 0;
        mStartTimeNanos = 0;
        return result;
    }

    int64_t convertFrameToTime(int64_t framePosition) override {
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

    virtual void writeBurst(const float *buffer) {
        (void) buffer;
        if (mStartTimeNanos == 0) {
            mStartTimeNanos = HostTools::getNanoTime();
            mNextHardwareReadTimeNanos = mStartTimeNanos;
        }

        updateHardwareSimulator();
        int32_t available = getEmptyFramesAvailable();

        // If there is not enough room then sleep until the hardware reads another burst.
        if (available < mFramesPerBurst) {
            HostCpuManager::getInstance()->sleepAndTuneCPU(mNextHardwareReadTimeNanos);
            updateHardwareSimulator();
            available = getEmptyFramesAvailable();
            assert(available >= mFramesPerBurst);
        } else {
            // Just let CPU Manager know that a burst has occurred.
            HostCpuManager::getInstance()->sleepAndTuneCPU(0);
        }

        // Simulate writing to a buffer.
        setFramesWritten(getFramesWritten() + mFramesPerBurst);
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
                mThread = HostThreadFactory::createThread(mThreadType);
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

    HostThreadFactory::ThreadType getThreadType() const override {
        return mThreadType;
    }

    void setThreadType(HostThreadFactory::ThreadType threadType) override {
        mThreadType = threadType;
    }

private:

    /**
     * Call the callback in a loop until it is finished or an error occurs.
     * Data from the callback will be passed to the write() method.
     */
    int32_t innerCallbackLoop() {
        assert(mFramesPerBurst > 0); // set in open

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

            // Write in a loop until the callback says we are done.
            IAudioSinkCallback::Result callbackResult
                    = IAudioSinkCallback::Result::Continue;
            while (callbackResult == IAudioSinkCallback::Result::Continue
                   && result == SYNTHMARK_RESULT_SUCCESS) {

                // Call the synthesizer to render the audio data.
                callbackResult = fireCallback(mBurstBuffer, mFramesPerBurst);

                if (callbackResult == IAudioSinkCallback::Result::Continue) {
                    // Output the audio using a blocking write.
                    writeBurst(mBurstBuffer);
                } else if (callbackResult != IAudioSinkCallback::Result::Finished) {
                    result = callbackResult;
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
            if (err != 0) {
                printf("Could not join() callback thread! err = %d\n", err);
            }
            delete mThread;
            mThread = NULL;
            return mCallbackLoopResult;
        } else {
            return innerCallbackLoop();
        }
    }

private:

    int64_t mStartTimeNanos = 0;
    int32_t mNanosPerBurst = 1; // set in open
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
    HostThreadFactory::ThreadType mThreadType = HostThreadFactory::ThreadType::Audio;

private:
    LogTool    * mLogTool = NULL;


    // Advance mFramesConsumed and mNextHardwareReadTimeNanos based on real-time clock.
    void updateHardwareSimulator() {
        int64_t currentTime = HostTools::getNanoTime();
        int countdown = 32; // Avoid spinning like crazy.
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
