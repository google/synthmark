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

#ifndef SYNTHMARK_REAL_AUDIO_SINK_H
#define SYNTHMARK_REAL_AUDIO_SINK_H

#include <cstdint>
#include <ctime>
#include <memory>
#include <unistd.h>

#include <aaudio/AAudio.h>

#include "AudioSinkBase.h"
#include "AdpfWrapper.h"
#include "HostTools.h"
#include "HostThreadFactory.h"
#include "LogTool.h"
#include "SynthMark.h"
#include "SynthMarkResult.h"
#include "UtilClampAudioBehavior.h"
#include "UtilClampController.h"

class RealAudioSink : public AudioSinkBase
{
public:
    RealAudioSink(LogTool &logTool)
    : mLogTool(logTool)
    {}

    virtual ~RealAudioSink() = default;

    virtual int32_t close() override {
        if (mStream) {
            AAudioStream_close(mStream);
            mStream = nullptr;
        }
        return 0;
    }

    static aaudio_data_callback_result_t aaudio_data_callback(
            AAudioStream *stream __unused,
            void *userData,
            void *audioData,
            int32_t numFrames) {
        RealAudioSink *audioSink = (RealAudioSink *) userData;
        return audioSink->onCallback(audioData, numFrames);
    }

    int32_t openAAudioStream(int32_t sampleRate, int32_t samplesPerFrame) {
        AAudioStreamBuilder *aaudioBuilder = nullptr;

        // Use an AAudioStreamBuilder to contain requested parameters.
        aaudio_result_t result = AAudio_createStreamBuilder(&aaudioBuilder);
        if (result != AAUDIO_OK) {
            return result;
        }

        // Request stream properties.
        AAudioStreamBuilder_setDataCallback(aaudioBuilder, aaudio_data_callback, this);
        AAudioStreamBuilder_setPerformanceMode(aaudioBuilder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
        //AAudioStreamBuilder_setSharingMode(aaudioBuilder, AAUDIO_SHARING_MODE_EXCLUSIVE);
        AAudioStreamBuilder_setSharingMode(aaudioBuilder, AAUDIO_SHARING_MODE_SHARED);
        AAudioStreamBuilder_setFormat(aaudioBuilder, AAUDIO_FORMAT_PCM_FLOAT);
        AAudioStreamBuilder_setBufferCapacityInFrames(aaudioBuilder, 16*1024);

        // Create an AAudioStream using the Builder.
        result = AAudioStreamBuilder_openStream(aaudioBuilder, &mStream);
        AAudioStreamBuilder_delete(aaudioBuilder);
        if (result != AAUDIO_OK) {
            return result;
        }

        mBufferCapacityInFrames = AAudioStream_getBufferCapacityInFrames(mStream);
//        mLogTool.log("AAudioStream_getBufferCapacityInFrames = %d\n",
//                     mBufferCapacityInFrames);

        return (int32_t) result;
    }

    int32_t open(int32_t sampleRate, int32_t samplesPerFrame,
            int32_t framesPerBurst) override {
        int32_t result = openAAudioStream(sampleRate, samplesPerFrame);
        if (result < 0) {
            return result;
        }

        int32_t actualFramesPerBurst = AAudioStream_getFramesPerBurst(mStream);
        if (framesPerBurst != actualFramesPerBurst) {
            mLogTool.log("ERROR - burst mismatch, requested %d, actual = %d",
                         framesPerBurst, actualFramesPerBurst);
            return -1;
        }

        result = AudioSinkBase::open(sampleRate, samplesPerFrame, framesPerBurst);
        if (result < 0) {
            return result;
        }

        // If UNSPECIFIED then set to a reasonable default.
        if (getBufferSizeInFrames() == 0) {
            int32_t bufferSize = mDefaultBufferSizeInBursts * framesPerBurst;
            setRealBufferSizeInFrames(bufferSize);
        } else {
            // Set based on previous setting.
            setRealBufferSizeInFrames(getBufferSizeInFrames());
        }

        return result;
    }

    virtual int32_t start() override {
        mThreadDone = false;
        mCallbackCount = 0;
        return AAudioStream_requestStart(mStream);
    }

    virtual int32_t stop() override {
        int32_t result =  AAudioStream_requestStop(mStream);
        // Close after the stream is stopped so we do not collide with the callback thread.
        if (mUseADPF) {
            mAdpfWrapper.close();
        }
        return result;
    }

    HostThreadFactory::ThreadType getThreadType() const override {
        return HostThreadFactory::ThreadType::Audio;
    }

    void setThreadType(HostThreadFactory::ThreadType threadType) override {
    }

    aaudio_data_callback_result_t onCallback(
            void *audioData,
            int32_t numFrames) {

        if (mCallbackCount == 0 && isAdpfEnabled()) {
            int64_t targetDurationNanos = (mFramesPerBurst * 1e9) / getSampleRate();
            // This has to be called from the callback thread so we get the right TID.
            int adpfResult = mAdpfWrapper.open(gettid(), targetDurationNanos);
            if (adpfResult < 0) {
                mLogTool.log("WARNING ADPF not supported, %d\n", adpfResult);
                mUseADPF = false;
            } else {
                mLogTool.log("ADPF is active\n");
                mUseADPF = true;
            }
        }
        int64_t beginCallback = HostTools::getNanoTime();

        aaudio_data_callback_result_t aaudioCallbackResult = AAUDIO_CALLBACK_RESULT_CONTINUE;
        IAudioSinkCallback *callback = getCallback();
        if (callback != NULL) {
            if (getRequestedCpu() != SYNTHMARK_CPU_UNSPECIFIED) {
                int err = HostThread::setCpuAffinity(getRequestedCpu());
                if (err) {
                    mLogTool.log("WARNING setCpuAffinity() returned %d\n", err);
                    setActualCpu(SYNTHMARK_CPU_UNSPECIFIED);
                } else {
                    setActualCpu(getRequestedCpu());
                }
            } else {
                setActualCpu(SYNTHMARK_CPU_UNSPECIFIED);
            }

            mSchedulerUsed = sched_getscheduler(0);

            // Call the synthesizer to render the audio data.
            IAudioSinkCallback::Result callbackResult = fireCallback(
                    (float *)audioData,
                    mFramesPerBurst);
            if (callbackResult != IAudioSinkCallback::Result::Continue) {
                aaudioCallbackResult = AAUDIO_CALLBACK_RESULT_STOP;
                mThreadDone = true;
            }

            // Update from actual AAudio underruns.
            setUnderrunCount(AAudioStream_getXRunCount(mStream));
        } else {
            aaudioCallbackResult = AAUDIO_CALLBACK_RESULT_STOP;
        }
        if (mUseADPF) {
            int64_t endCallback = HostTools::getNanoTime();
            int64_t actualDurationNanos = endCallback - beginCallback;
            mAdpfWrapper.reportActualDuration(actualDurationNanos);
        }
        mCallbackCount++;
        return aaudioCallbackResult;
    }


    /**
     * Wait for the AAudio stream to stop.
     */
    int32_t runCallbackLoop() override {
        int32_t result = SYNTHMARK_RESULT_SUCCESS;
        if (mStream) {
            while (!mThreadDone) {
                usleep(20 * 1000);
            }
        }
        return result;
    }

    /**
     * Set the amount of the buffer that will be used. Determines latency.
     * @return the actual size, which may not match the requested size, or a negative error
     */
    int32_t setRealBufferSizeInFrames(int32_t numFrames)  {
        int32_t numFrames2 = AudioSinkBase::setBufferSizeInFrames(numFrames);
        if (mStream) {
            int32_t actualSize = AAudioStream_setBufferSizeInFrames(mStream, numFrames2);
//            mLogTool.log("AAudioStream_setBufferSizeInFrames(%d) => %d\n",
//                         numFrames2, actualSize);
            if (actualSize < 0) return actualSize;
            AudioSinkBase::setBufferSizeInFrames(actualSize);
        }
        return AudioSinkBase::getBufferSizeInFrames();
    }

private:
    LogTool           &mLogTool;
    AAudioStream      *mStream = nullptr;
    std::atomic<bool>  mThreadDone{false};
    bool               mUseADPF = false;
    int32_t            mCallbackCount = 0;
    AdpfWrapper        mAdpfWrapper;
};

#endif // SYNTHMARK_REAL_AUDIO_SINK_H
