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
#include <memory>
#include <unistd.h>

#include "AudioSinkBase.h"
#include "HostTools.h"
#include "HostThreadFactory.h"
#include "LogTool.h"
#include "SynthMark.h"
#include "SynthMarkResult.h"
#include "UtilClampAudioBehavior.h"
#include "UtilClampController.h"
#include "AdpfWrapper.h"

constexpr int kMaxBufferCapacityInBursts = 512;

class VirtualAudioSink : public AudioSinkBase
{
public:
    VirtualAudioSink(LogTool &logTool)
    : mLogTool(logTool)
    {}

    virtual ~VirtualAudioSink() {
        delete mThread;
    }

    virtual int32_t close() override {
        mBurstBuffer.reset();
        return 0;
    }

    int32_t open(int32_t sampleRate, int32_t samplesPerFrame,
            int32_t framesPerBurst) override {
        int32_t result = AudioSinkBase::open(sampleRate, samplesPerFrame, framesPerBurst);
        if (result < 0) {
            return result;
        }

        // This must be set before setting the size.
        mBufferCapacityInFrames = kMaxBufferCapacityInBursts * framesPerBurst;

        // If UNSPECIFIED then set to a reasonable default.
        if (getBufferSizeInFrames() == 0) {
            int32_t bufferSize = mDefaultBufferSizeInBursts * framesPerBurst;
            setBufferSizeInFrames(bufferSize);
        }

        int64_t nanosPerBurst = mFramesPerBurst * SYNTHMARK_NANOS_PER_SECOND / mSampleRate;
        mNanosPerBurst = (int32_t) nanosPerBurst;
        HostCpuManager::getInstance()->setNanosPerBurst(nanosPerBurst);

        mBurstBuffer = std::make_unique<float[]>(samplesPerFrame * framesPerBurst);

        mNextHardwareReadTimeNanos = 0;
        mFramesConsumed = 0;
        mStartTimeNanos = 0;

        return result;
    }

    int64_t convertFrameToTime(int64_t framePosition) override {
        return (int64_t) (mStartTimeNanos + (framePosition * mNanosPerBurst / mFramesPerBurst));
    }

    int32_t getEmptyFramesAvailable() {
        return getBufferSizeInFrames() - getFullFramesAvailable();
    }

    int32_t getFullFramesAvailable() {
        return (int32_t) (getFramesWritten() - mFramesConsumed);
    }

    virtual void writeBurst(const float *buffer) {
        (void) buffer; // discard the audio data

        if (mStartTimeNanos == 0) {
            mStartTimeNanos = HostTools::getNanoTime();
            mNextHardwareReadTimeNanos = mStartTimeNanos;
        }

        int32_t availableData = getFullFramesAvailable();
        if (availableData < 0) {
            // Not enough data! Hardware underflowed while we were gone.
            mUnderrunCount++;
        }

        updateHardwareSimulator();
        int32_t availableRoom = getEmptyFramesAvailable();

        // If there is not enough room then sleep until the hardware reads another burst.
        if (availableRoom < mFramesPerBurst) {
            HostCpuManager::getInstance()->sleepAndTuneCPU(mNextHardwareReadTimeNanos);
            updateHardwareSimulator();
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
        setFramesWritten(getBufferSizeInFrames());  // start full and primed

        if (mUseRealThread) {
            mCallbackLoopResult = SYNTHMARK_RESULT_THREAD_FAILURE;
            if (mThread == NULL) {
                mThread = HostThreadFactory::createThread(mThreadType);
            }
            int err = mThread->start(threadProcWrapper, this);
            if (err != 0) {
                mLogTool.log("ERROR in VirtualAudioSink, thread start() failed, %d\n", err);
                return SYNTHMARK_RESULT_THREAD_FAILURE;
            }
        }
        return 0;
    }

    virtual int32_t stop() override {
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
        int lastCpu = -1;
        int32_t result = SYNTHMARK_RESULT_SUCCESS;
        IAudioSinkCallback *callback = getCallback();
        if (callback != NULL) {
            if (mUseRealThread) {
                if (HostCpuManager::areWorkloadHintsEnabled()) {
                    double initial_bw = BW_MAX;

                    int err = static_cast<CustomHostCpuManager *>(HostCpuManager::getInstance())->updateDeadlineParams(
                            mNanosPerBurst * initial_bw,
                            mNanosPerBurst,
                            mNanosPerBurst);
                    if (err == 0) {
                        mLogTool.log("requestDLBandwidth(%lf)\n", initial_bw);
                        fflush(stdout);
                    } else {
                        mLogTool.log("ERROR setting SCHED_DEADLINE\n");
                        return -1;
                    }
                    if (getRequestedCpu() != SYNTHMARK_CPU_UNSPECIFIED) {
                        mLogTool.log("ERROR setting affinity for SCHED_DEADLINE task\n");
                        setActualCpu(SYNTHMARK_CPU_UNSPECIFIED);
                        return -1;
                    }
                } else {
                    int err;
                    if (isSchedFifoEnabled()) {
                        err = mThread->promote(mThreadPriority);
                        if (err != 0) {
                            mLogTool.log("ERROR promoting thread\n");
                        }
                    }
                    if (getRequestedCpu() != SYNTHMARK_CPU_UNSPECIFIED) {
                        err = mThread->setCpuAffinity(getRequestedCpu());
                        if (err) {
                            mLogTool.log("WARNING setCpuAffinity() returned %d\n", err);
                            setActualCpu(SYNTHMARK_CPU_UNSPECIFIED);
                        } else {
                            setActualCpu(getRequestedCpu());
                        }
                    } else {
                        setActualCpu(SYNTHMARK_CPU_UNSPECIFIED);
                    }
                }
            }

            mSchedulerUsed = sched_getscheduler(0);

            // Write in a loop until the callback says we are done.
            IAudioSinkCallback::Result callbackResult
                    = IAudioSinkCallback::Result::Continue;

            UtilClampController utilClampController;
            UtilClampAudioBehavior behavior;
            AdpfWrapper adpfWrapper;
            int originalUtilClamp = 0;
            int currentUtilClamp = 0;
            constexpr int32_t kUtilClampQuanta = 10;
            // Restrict the range to save power.
            constexpr int32_t kUtilClampLow = 40;
            constexpr int32_t kUtilClampHigh = 300;
            int64_t targetDurationNanos = (mFramesPerBurst * 1e9) / getSampleRate();

            // init UtilClampBehavior
            if (isUtilClampEnabled()) {
                if (utilClampController.isSupported()) {
                    originalUtilClamp = utilClampController.getMin();
                    currentUtilClamp = originalUtilClamp;
                    mLogTool.log("utilClamp active\n");
                    if (isUtilClampLoggingEnabled()) {
                        mLogTool.log("burst, uclamp_min, load, cpu#\n");
                    }
                    if (isUtilClampDynamic()) {
                        behavior.setup(kUtilClampLow,
                                       kUtilClampHigh,
                                       targetDurationNanos);
                    } else {
                        // Set to fixed level for entire time.
                        utilClampController.setMin(getUtilClampLevel());
                        currentUtilClamp = utilClampController.getMin();
                        mLogTool.log("sched_util_min fixed at %d\n", currentUtilClamp);
                    }
                } else {
                    mLogTool.log("WARNING utilClamp not supported\n");
                    setUtilClampLevel(UTIL_CLAMP_OFF);
                }
            } else if (isAdpfEnabled()) {
                int adpfResult = adpfWrapper.open(gettid(), targetDurationNanos);
                if (adpfResult < 0) {
                    mLogTool.log("WARNING ADPF not supported, %d\n", adpfResult);
                    setAdpfEnabled(false);
                } else {
                    mLogTool.log("ADPF is active\n");
                }
            }

            while (callbackResult == IAudioSinkCallback::Result::Continue
                   && result == SYNTHMARK_RESULT_SUCCESS) {

                int64_t beginCallback = HostTools::getNanoTime();
                // Call the synthesizer to render the audio data.
                callbackResult = fireCallback(mBurstBuffer.get(), mFramesPerBurst);
                int64_t endCallback = HostTools::getNanoTime();
                int64_t actualDurationNanos = endCallback - beginCallback;
                if (isUtilClampDynamic()) {
                    bool utilClampChanged = false;
                    int cpu = HostThread::getCpu(); // query before changing util_clamp
                    int suggestedUtilClamp = behavior.processTiming(actualDurationNanos);
                    if (abs(suggestedUtilClamp - currentUtilClamp) >= kUtilClampQuanta) {
                        utilClampController.setMin(suggestedUtilClamp);
                        utilClampChanged = true;
                        currentUtilClamp = utilClampController.getMin();
                    }
                    if (isUtilClampLoggingEnabled() && (utilClampChanged || cpu != lastCpu)) {
                        double realTime = behavior.calculateFractionRealTime(actualDurationNanos);
                        mLogTool.log("%4d, %5.1f, %d\n",
                                     currentUtilClamp,
                                     realTime * 100,
                                     cpu);
                    }
                    lastCpu = cpu;
                } else if (isAdpfEnabled()) {
                    adpfWrapper.reportActualDuration(actualDurationNanos);
                }

                if (callbackResult == IAudioSinkCallback::Result::Continue) {
                    // Output the audio using a blocking write.
                    writeBurst(mBurstBuffer.get());
                } else if (callbackResult != IAudioSinkCallback::Result::Finished) {
                    result = callbackResult;
                }
            }
            // Restore original value.
            if (isUtilClampEnabled()) {
                utilClampController.setMin(originalUtilClamp);
            }
            if (isAdpfEnabled()) {
                adpfWrapper.close();
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
    int32_t runCallbackLoop() override {
        if (mThread != NULL) {
            int err = mThread->join();
            if (err != 0) {
                mLogTool.log("Could not join() callback thread! err = %d\n", err);
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
    int64_t mFramesConsumed = 0;
    std::unique_ptr<float[]>  mBurstBuffer;   // contains output of the synthesizer
    bool    mUseRealThread = true; // TODO control using new settings object
    int     mThreadPriority = SYNTHMARK_THREAD_PRIORITY_DEFAULT;   // TODO control using new settings object
    int32_t mCallbackLoopResult = 0;
    HostThread *mThread = NULL;
    HostThreadFactory::ThreadType mThreadType = HostThreadFactory::ThreadType::Audio;

    LogTool    &mLogTool;

    // Advance mFramesConsumed and mNextHardwareReadTimeNanos based on real-time clock.
    void updateHardwareSimulator() {
        int64_t currentTime = HostTools::getNanoTime();
        int countdown = 32; // Avoid spinning like crazy.
        // Is it time to consume a block?
        while ((currentTime >= mNextHardwareReadTimeNanos) && (countdown-- > 0)) {
            mFramesConsumed += mFramesPerBurst; // fake read
            mNextHardwareReadTimeNanos += mNanosPerBurst; // avoid drift
        }
    }

};

#endif // SYNTHMARK_VIRTUAL_AUDIO_SINK_H
