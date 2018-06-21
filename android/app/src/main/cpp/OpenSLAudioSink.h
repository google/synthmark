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
#ifndef ANDROID_OPENSLAUDIOSINK_H
#define ANDROID_OPENSLAUDIOSINK_H

#include <SLES/OpenSLES_Android.h>
#include <tools/AudioSinkBase.h>
#include <mutex>
#include <condition_variable>

/**
 * Android specific audio sink which will output audio data to the device's
 * audio hardware using the OpenSL ES API
 */
class OpenSLAudioSink : public AudioSinkBase
{
public:
    OpenSLAudioSink() {};
    virtual ~OpenSLAudioSink() {};
    void playerCallback(SLAndroidSimpleBufferQueueItf bq);
    int32_t open(int32_t sampleRate,
                 int32_t samplesPerFrame,
                 int32_t framesPerBurst) override;
    int32_t start() override;
//    int32_t write(const float *buffer, int32_t numFrames) override;
    int32_t stop() override;
    int32_t close() override;
    int32_t getBufferSizeInFrames() override;
    int32_t getSampleRate() override;
    int32_t getUnderrunCount() override;
    int32_t runCallbackLoop() override;

    /**
     * Set the amount of the buffer that will be used. Determines latency.
     * @return the actual size, which may not match the requested size, or a negative error
     */
    virtual int32_t setBufferSizeInFrames(int32_t numFrames) override {
        return -1;
    }

    /**
     * Get the maximum size of the buffer.
     */
    virtual int32_t getBufferCapacityInFrames() override {
        return -1;
    }


private:
    int32_t mSampleRate;
    int32_t mBufferSizeInFrames = 0;
    int32_t mFramesPerBurst = 0;
    int32_t mSamplesPerBurst = 0;
    int32_t mBytesPerBurst = 0;
    int32_t mSamplesPerFrame = 0;
    float *mBurstBuffer;
    int32_t mUnderrunCount = 0;
    std::mutex mCallbackMutex;
    std::condition_variable mCallbackConditionVariable;
    IAudioSinkCallback::Result mCallbackResult;

    // engine interfaces
    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine = NULL;

    // output mix interfaces
    SLObjectItf outputMixObject = NULL;
    SLDataLocator_OutputMix locator_output_mix;
    SLAndroidDataFormat_PCM_EX format_pcm;

    // buffer queue player interfaces
    SLObjectItf bqPlayerObject = NULL;
    SLPlayItf bqPlayerPlay = NULL;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue = NULL;

    SLDataLocator_AndroidSimpleBufferQueue locator_bufferqueue_source;

    // Single result object used for every SLES method call to determine whether it succeeded
    SLresult slResult;
};


#endif //ANDROID_OPENSLAUDIOSINK_H
