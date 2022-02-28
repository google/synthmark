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

#include <cmath>
#include <cstdint>
#include <sstream>

#include "IAudioSinkCallback.h"
#include "SynthMark.h"
#include "HostThreadFactory.h"

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

    enum : int32_t {
        UTIL_CLAMP_OFF = 0,
        UTIL_CLAMP_ON = 1,
        UTIL_CLAMP_ON_LOGGED = 2
    };

    enum : int32_t {
        AUDIO_LEVEL_NORMAL = 0,
        AUDIO_LEVEL_CALLBACK = 1,
        AUDIO_LEVEL_OUTPUT = 2,
    };

    virtual int32_t initialize() {
        return 0;
    }

    virtual int32_t open(int32_t sampleRate, int32_t samplesPerFrame, int32_t framesPerBurst) {
        mSampleRate = sampleRate;
        mSamplesPerFrame = samplesPerFrame;
        mFramesPerBurst = framesPerBurst;
        return 0;
    }

    virtual int32_t start() = 0;

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

    virtual IAudioSinkCallback::Result
            fireCallback(float *buffer, int32_t numFrames) {
        return mCallback->onRenderAudio(buffer, numFrames);
    }

    void setDefaultBufferSizeInBursts(int32_t numBursts) {
        mDefaultBufferSizeInBursts = numBursts;
    }

    /**
     * Set the amount of the buffer that will be used. Determines latency.
     * @return the actual size, which may not match the requested size, or a negative error
     */
    virtual int32_t setBufferSizeInFrames(int32_t numFrames) {
        // mLogTool.log("VirtualAudioSink::%s(%d) max = %d\n", __func__, numFrames, mBufferCapacityInFrames);
        if (numFrames < 1) {
            numFrames = 1;
        } else if (numFrames > mBufferCapacityInFrames) {
            numFrames = mBufferCapacityInFrames;
        }
        mBufferSizeInFrames = numFrames;
        return mBufferSizeInFrames;
    }

    int32_t getBufferSizeInFrames() {
        return mBufferSizeInFrames;
    }

    /**
     * Get the maximum allocated size of the buffer.
     */
    int32_t getBufferCapacityInFrames() {
        return mBufferCapacityInFrames;
    }

    int32_t getUnderrunCount() {
        return mUnderrunCount;
    }

    void setUnderrunCount(int i) {
        mUnderrunCount = i;
    }

    int32_t getSampleRate() {
        return mSampleRate;
    }
    int32_t getSamplesPerFrame() {
        return mSamplesPerFrame;
    }
    int32_t getFramesPerBurst() {
        return mFramesPerBurst;
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

    int32_t getUtilClampLevel() {
        return mUtilClampLevel;
    }

    void setUtilClampLevel(int32_t level) {
        mUtilClampLevel = level;
    }

    bool isUtilClampEnabled() {
        return mUtilClampLevel > 0;
    }

    bool isUtilClampDynamic() {
        return mUtilClampLevel == UTIL_CLAMP_ON
               || mUtilClampLevel == UTIL_CLAMP_ON_LOGGED;
    }

    bool isUtilClampLoggingEnabled() {
        return mUtilClampLevel == UTIL_CLAMP_ON_LOGGED;
    }

    virtual HostThreadFactory::ThreadType getThreadType() const {
        return HostThreadFactory::ThreadType::Default;
    }

    virtual void setThreadType(HostThreadFactory::ThreadType mThreadType) {
    }

    bool isSchedFifoEnabled() const {
        return mSchedFifoEnabled;
    }

    void setSchedFifoEnabled(bool enabled) {
        mSchedFifoEnabled = enabled;
    }

    bool isAdpfEnabled() const {
        return mAdpfEnabled;
    }

    void setAdpfEnabled(bool enabled) {
        mAdpfEnabled = enabled;
    }

    static const char *schedulerToString(int scheduler) {
        scheduler = scheduler & 0xFFFF; // clear high flags like SCHED_RESET_ON_FORK
        switch(scheduler) {
            case SCHED_FIFO:
                return "SCHED_FIFO";
            case SCHED_OTHER:
                return "SCHED_OTHER";
            case SCHED_DEADLINE:
                return "SCHED_DEADLINE";
            case SCHED_RR:
                return "SCHED_RR";
            default:
                return "unknown";
        }
    }

    std::string  dump() {
        std::stringstream resultMessage;
        resultMessage << std::endl;
        resultMessage << "AudioSink:" << std::endl;
        resultMessage << "  frames.per.burst       = " << getFramesPerBurst() << std::endl;
        resultMessage << "  scheduler              = "
                         << schedulerToString(mSchedulerUsed)
                         << " # 0x" << std::hex << mSchedulerUsed << std::dec << std::endl;
        resultMessage << "  buffer.size.frames     = "  << getBufferSizeInFrames() << std::endl;
        resultMessage << "  buffer.size.bursts     = "
                         << (getBufferSizeInFrames() / getFramesPerBurst()) << std::endl;
        resultMessage << "  buffer.capacity.frames = "   << getBufferCapacityInFrames() << std::endl;
        resultMessage << "  sample.rate            = "   << getSampleRate() << std::endl;
        resultMessage << "  cpu.affinity           = "   << getActualCpu() << std::endl;
        return resultMessage.str();
    }

protected:
    void setActualCpu(int cpuAffinity) {
        mActualCpu = cpuAffinity;
    }

    // set in open
    int32_t       mSampleRate = kSynthmarkSampleRate;
    int32_t       mSamplesPerFrame = 1;
    int32_t       mFramesPerBurst = 0;
    int32_t       mUnderrunCount = 0;
    // set in callback loop
    int           mSchedulerUsed = -1;

    int32_t       mBufferSizeInFrames = 0;
// Use 2 for double buffered
    static constexpr int kBufferSizeInBursts = 8;
    int32_t       mDefaultBufferSizeInBursts = kBufferSizeInBursts;
    int32_t       mBufferCapacityInFrames = 4 * 1024;

private:
    IAudioSinkCallback *mCallback = NULL;
    int64_t        mFramesWritten = 0;
    int            mRequestedCpu = SYNTHMARK_CPU_UNSPECIFIED;
    int            mActualCpu = SYNTHMARK_CPU_UNSPECIFIED;
    bool           mSchedFifoEnabled = true;
    bool           mAdpfEnabled = false;
    int32_t        mUtilClampLevel = UTIL_CLAMP_OFF;
};

#endif // SYNTHMARK_AUDIO_SINK_BASE_H
