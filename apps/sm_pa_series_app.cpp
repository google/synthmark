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

#include <stdio.h>
#include <math.h>
#include "portaudio.h"
#include "SynthMark.h"
#include "tools/TimingAnalyzer.h"
#include "synth/Synthesizer.h"

#define NUM_LOOPS          8
#define MIN_VOICES         2
#define MAX_VOICES         120
#define NUM_SECONDS        3
#define SAMPLE_RATE        SYNTHMARK_SAMPLE_RATE
#define FRAMES_PER_BUFFER  (SYNTHMARK_FRAMES_PER_BUFFER * 2)

typedef struct
{
    int32_t counter;
    Synthesizer synth;
    TimingAnalyzer timer;
} paTestData;

static int patestCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;
    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) inputBuffer;

    data->timer.markEntry();
    uint32_t gear = data->counter & 63;
    if (gear == 0) {
        data->synth.allNotesOn();
    } else if (gear == 20) {
        data->synth.allNotesOff();
    }
    data->counter++;
    data->synth.renderStereo(out, (int32_t) framesPerBuffer);
    data->timer.markExit();

    return paContinue;
}

PaError measureCpuLoad(int numVoices, double *smLoadPtr)
{
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;
    paTestData data = {0};

    data.synth.setup(SAMPLE_RATE, numVoices);

    outputParameters.device = Pa_GetDefaultOutputDevice();
    if (outputParameters.device == paNoDevice) {
        fprintf(stderr,"Error: No default output device.\n");
        goto error;
    }
    outputParameters.channelCount = SAMPLES_PER_FRAME;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &stream,
              NULL, /* no input */
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              patestCallback,
              &data );
    if (err != paNoError) goto error;

    err = Pa_StartStream( stream );
    if (err != paNoError) goto error;

    printf("Play for %d seconds.\n", NUM_SECONDS );
    Pa_Sleep(NUM_SECONDS * 1000);

    err = Pa_StopStream(stream);
    if (err != paNoError) goto error;
    *smLoadPtr = data.timer.getDutyCycle();

    err = Pa_CloseStream(stream);
    if (err != paNoError) goto error;

error:
    return err;
}

int main(void)
{
    PaError err;
    int32_t counts[NUM_LOOPS];
    double smLoads[NUM_LOOPS];
    int loopIndex = 0;
    bool go = true;

    printf("SynthMark %d %d\n", SAMPLE_RATE, FRAMES_PER_BUFFER);

    err = Pa_Initialize();
    if (err != paNoError) goto error;

    for (;loopIndex < NUM_LOOPS && go; loopIndex++) {
        int numVoices = MIN_VOICES + ((NUM_LOOPS < 2) ? 0 :
                                      (loopIndex * (MAX_VOICES - MIN_VOICES) / (NUM_LOOPS - 1)));
        counts[loopIndex] = numVoices;
        err = measureCpuLoad(numVoices, &smLoads[loopIndex]);
        if (err != paNoError) goto error;
        printf("SM Load = %f\n", smLoads[loopIndex]);
        if (smLoads[loopIndex] > 0.7) {
            printf("exiting loop at %d so we do not burden the CPU\n", loopIndex);
        }
    }

    printf("index, voices, smload\n");
    for (int i = 0; i < loopIndex; i++) {
        printf("%3d, %4d, %7.4f\n", i, counts[i], smLoads[i] );
    }


    Pa_Terminate();
    printf("Test finished. err = %d\n", err);
    return 0;

error:
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return 1;
}

