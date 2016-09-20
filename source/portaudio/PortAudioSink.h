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
#include "tools/AudioSinkBase.h"

#ifndef SYNTHMARK_PORTAUDIO_SINK_H
#define SYNTHMARK_PORTAUDIO_SINK_H

class PortAudioSink : public AudioSinkBase
{
public:
    PortAudioSink() {}
    virtual ~PortAudioSink() {}

    int32_t initialize() override {
        return Pa_Initialize();
    }

    int32_t open(int32_t sampleRate, int32_t samplesPerFrame, int32_t framesPerBuffer) override {
        int32_t err = Pa_Initialize();
        if( err != paNoError ) return err;

        PaStreamParameters outputParameters;
        outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
        if (outputParameters.device == paNoDevice) {
            fprintf(stderr,"Error: No default output device.\n");
            goto error;
        }
        outputParameters.channelCount = samplesPerFrame;
        outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
        outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultHighOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = NULL;

        err = Pa_OpenStream(
                            &mStream,
                            NULL, /* no input */
                            &outputParameters,
                            sampleRate,
                            framesPerBuffer,
                            0,
                            NULL,
                            NULL );
        if( err != paNoError ) goto error;
    error:
        return err;
    }

    int32_t start() override {
        return Pa_StartStream( mStream );
    }

    int32_t write(const float *buffer, int32_t numFrames) override {
        return Pa_WriteStream(mStream, buffer, numFrames);
    }

    int32_t stop() override {
        return Pa_StopStream( mStream );
    }

    int32_t close() override {
        return Pa_CloseStream( mStream );
    }

    int32_t terminate() override {
        return Pa_Terminate();
    }

    /**
     * Set the amount of the buffer that will be used. Determines latency.
     */
    virtual int32_t setBufferSizeInFrames(int32_t numFrames) override {
        return -1; // unimplemented
    }
    /**
     * Get the size size of the buffer.
     */
    virtual int32_t getBufferSizeInFrames() override {
        return -1;  // unimplemented
    }
    /**
     * Get the maximum size of the buffer.
     */
    virtual int32_t getBufferCapacityInFrames() override {
        return -1;  // unimplemented
    }

private:
    PaStream *mStream;

};


#endif // SYNTHMARK_PORTAUDIO_SINK_H

