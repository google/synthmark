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

#include "OpenSLAudioSink.h"

#include <android/log.h>
#include <assert.h>
#include <tools/SynthTools.h>

#define CLASSNAME "OpenSLAudioSink"
#define OPENSL_BUFFER_COUNT 2 // double buffering
#define OPENSL_CHANNELS 2 // stereo

/**
 * Wrapper for OpenSL ES buffer queue callback
 */
static void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    ((OpenSLAudioSink*)context)->playerCallback(bq);
}

/**
 * Android specific audio sink which will output audio data to the device's
 * audio hardware using the OpenSL ES API
 */
int32_t OpenSLAudioSink::open(int32_t sampleRate, int32_t samplesPerFrame,
                         int32_t framesPerBurst){

    __android_log_print(ANDROID_LOG_VERBOSE,
                        CLASSNAME,
                        "Creating audio engine with %d sample rate, %d samples per frame, "
                        "%d frames per burst",
                        sampleRate,
                        samplesPerFrame,
                        framesPerBurst
    );

    mSampleRate = sampleRate;
    mSamplesPerFrame = samplesPerFrame;
    mFramesPerBurst = framesPerBurst;
    mSamplesPerBurst = mFramesPerBurst * mSamplesPerFrame;
    mBytesPerBurst = mSamplesPerBurst * sizeof(float);

    // create the audio buffer
    mBufferSizeInFrames = OPENSL_BUFFER_COUNT * mFramesPerBurst;
    size_t bufferSizeInSamples = (size_t) (mBufferSizeInFrames * mSamplesPerFrame);
    //mAudioBuffer = new CircularBuffer(bufferSizeInSamples);
    mBurstBuffer = new float[mSamplesPerBurst];

    __android_log_print(ANDROID_LOG_DEBUG,
                        CLASSNAME,
                        "Buffer size is %d frames, %d samples",
                        mBufferSizeInFrames,
                        (int) bufferSizeInSamples
    );

    // create engine
    slResult = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == slResult);
    (void) slResult;

    // realize the engine
    slResult = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == slResult);
    (void) slResult;

    // get the engine interface, which is needed in order to create other objects
    slResult = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    assert(SL_RESULT_SUCCESS == slResult);
    (void) slResult;

    // create output mix,
    slResult = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == slResult);
    (void) slResult;

    // realize the output mix
    slResult = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == slResult);
    (void) slResult;

    // configure the audio source (supply data through a buffer queue in PCM format)
    SLDataSource audio_source;

    // source location
    locator_bufferqueue_source.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    locator_bufferqueue_source.numBuffers = (SLuint32) OPENSL_BUFFER_COUNT;

    // source format
    format_pcm.formatType = SL_ANDROID_DATAFORMAT_PCM_EX;
    format_pcm.numChannels = OPENSL_CHANNELS;

    format_pcm.sampleRate = (SLuint32) mSampleRate * 1000;
    format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_32;
    format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_32;
    format_pcm.channelMask = (OPENSL_CHANNELS == 1) ? SL_SPEAKER_FRONT_CENTER :
                             SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
    format_pcm.representation = SL_ANDROID_PCM_REPRESENTATION_FLOAT;

    audio_source.pLocator = &locator_bufferqueue_source;
    audio_source.pFormat = &format_pcm;

    // configure the output: An output mix sink
    SLDataSink audio_sink;

    locator_output_mix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    locator_output_mix.outputMix = outputMixObject;

    audio_sink.pLocator = &locator_output_mix;
    audio_sink.pFormat = NULL;

    // create audio player
    const SLInterfaceID interfaceIds[] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean interfacesRequired[] = {SL_BOOLEAN_TRUE};

    slResult = (*engineEngine)->CreateAudioPlayer(
            engineEngine,
            &bqPlayerObject,
            &audio_source,
            &audio_sink,
            1, // Number of interfaces
            interfaceIds,
            interfacesRequired
    );

    assert(SL_RESULT_SUCCESS == slResult);
    (void) slResult;

    // realize the player
    slResult = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == slResult);
    (void) slResult;

    // get the play interface
    slResult = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    assert(SL_RESULT_SUCCESS == slResult);
    (void) slResult;

    // get the buffer queue interface
    slResult = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                               &bqPlayerBufferQueue);
    assert(SL_RESULT_SUCCESS == slResult);
    (void) slResult;

    // register callback on the buffer queue
    slResult = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback,
            this);
    assert(SL_RESULT_SUCCESS == slResult);
    (void) slResult;

    return 0;
}

/**
 * This function does nothing in OpenSL. Audio data is written to the audio sink via a callback.
 *
 * @see setCallback(IAudioSinkCallback *callback)
 */
int32_t OpenSLAudioSink::write(const float *buffer __unused, int32_t numFrames __unused){
    return 0;
}

int32_t OpenSLAudioSink::start() {
    return 0;
}

int32_t OpenSLAudioSink::stop(){

    // set the player's state to stopped
    slResult = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
    assert(SL_RESULT_SUCCESS == slResult);
    (void) slResult;

    (*bqPlayerObject)->Destroy(bqPlayerObject);
    return 0;
}

int32_t OpenSLAudioSink::close(){

    (*outputMixObject)->Destroy(outputMixObject);
    (*engineObject)->Destroy(engineObject);

    delete mBurstBuffer;
    return 0;
}


void OpenSLAudioSink::playerCallback(SLAndroidSimpleBufferQueueItf bq){

    mCallbackResult = fireCallback(mBurstBuffer, mFramesPerBurst);

    if (mCallbackResult != IAudioSinkCallback::CALLBACK_CONTINUE){
        // notify the waiting thread that the callbacks have finished
        mCallbackConditionVariable.notify_all();
    } else {
        // enqueue the audio data
        slResult = (*bq)->Enqueue(bqPlayerBufferQueue,
                                  mBurstBuffer,
                                  (SLuint32) mBytesPerBurst);
        assert(SL_RESULT_SUCCESS == slResult);
    }
}

int32_t OpenSLAudioSink::getBufferSizeInFrames() {
    return mBufferSizeInFrames;
}

int32_t OpenSLAudioSink::getSampleRate(){
    return mSampleRate;
}

int32_t OpenSLAudioSink::getUnderrunCount(){
    return mUnderrunCount;
}

int32_t OpenSLAudioSink::runCallbackLoop(){

    std::unique_lock<std::mutex> lck(mCallbackMutex);
    mCallbackResult = IAudioSinkCallback::CALLBACK_CONTINUE;

    // set the player's state to playing
    slResult = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    assert(SL_RESULT_SUCCESS == slResult);
    (void) slResult;

    // Fill the OpenSL buffers with silence to kick off the callbacks
    SynthTools::fillBuffer(mBurstBuffer, mSamplesPerBurst, 0);

    for (int i = 0; i < OPENSL_BUFFER_COUNT; i++){
        slResult = (*bqPlayerBufferQueue)->Enqueue(
                bqPlayerBufferQueue,
                mBurstBuffer,
                (SLuint32) mBytesPerBurst);
        assert(SL_RESULT_SUCCESS == slResult);
        (void) slResult;
    }

    // Wait until the callbacks have finished
    while (mCallbackResult == IAudioSinkCallback::CALLBACK_CONTINUE){
        mCallbackConditionVariable.wait(lck);
    }

    return mCallbackResult;
}
