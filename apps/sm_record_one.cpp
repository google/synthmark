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
#include "SynthMark.h"
#include "synth/Synthesizer.h"
#include "portaudio/write_wav.h"

#define MAX_SECONDS            60
#define DEFAULT_SECONDS    2
#define DEFAULT_FILENAME       "synthmark_voice.wav"

void usage(const char *name) {
    printf("%s -n{numVoices} -r{sampleRate} -s{seconds} {filename}\n", name);
    printf("    numVoices between 1 and %d\n", SYNTHMARK_MAX_VOICES);
    printf("    rate should be typical, 44100, 48000, etc. default is %d\n", SYNTHMARK_SAMPLE_RATE);
    printf("    seconds between 1 and %d, default is %d\n", MAX_SECONDS, DEFAULT_SECONDS);
}

/**
 * Write PCM to a WAV file.
 */
int main(int argc, char **argv)
{
    const char *filename = DEFAULT_FILENAME;
    int32_t numSeconds = DEFAULT_SECONDS;
    int32_t sampleRate = SYNTHMARK_SAMPLE_RATE;

    WAV_Writer writer;
    float outputBuffer[SYNTHMARK_FRAMES_PER_RENDER * SAMPLES_PER_FRAME];
    int16_t shortBuffer[SYNTHMARK_FRAMES_PER_RENDER * SAMPLES_PER_FRAME];
    Synthesizer synth;

    int32_t frameCounter = 0;
    int32_t numVoices = 1;
    int32_t samplesPerBlock = SYNTHMARK_FRAMES_PER_RENDER * SAMPLES_PER_FRAME;
    long wavResult;

    // Parse command line arguments.
    for (int iarg = 1; iarg < argc; iarg++) {
        char * arg = argv[iarg];
        if (arg[0] == '-') {
            switch(arg[1]) {
                case 'n':
                    numVoices = atoi(&arg[2]);
                    break;
                case 'r':
                    sampleRate = atoi(&arg[2]);
                    break;
                case 's':
                    numSeconds = atoi(&arg[2]);
                    break;
                case 'h': // help
                case '?': // help
                    usage(argv[0]);
                    return 0;
                default:
                    printf("Unrecognized switch -%c\n", arg[1]);
                    usage(argv[0]);
                    return 1;
            }
        } else {
            filename = arg;
        }
    }
    if (numVoices < 1 || numVoices > SYNTHMARK_MAX_VOICES) {
        printf("Invalid num voices = %d\n", numVoices);
        usage(argv[0]);
        return 1;
    }
    if (numSeconds < 1 || numSeconds > MAX_SECONDS) {
        printf("Invalid duration in seconds = %d\n", numSeconds);
        usage(argv[0]);
        return 1;
    }

    printf("Write SynthMark to WAV file %s, num voice = %d\n", filename, numVoices);

    int32_t framesNeeded = sampleRate * numSeconds;
    synth.setup(sampleRate, numVoices);
    synth.allNotesOn();
    bool on = true;

    wavResult =  Audio_WAV_OpenWriter(&writer, filename, sampleRate, SAMPLES_PER_FRAME);
    if(wavResult < 0) goto error;

    while (frameCounter < framesNeeded) {
        synth.renderStereo(outputBuffer, (int32_t) SYNTHMARK_FRAMES_PER_RENDER);
        const int32_t minSample = -32768;
        const int32_t maxSample = 32767;
        // Convert floats to short.
        for (int is = 0; is < samplesPerBlock; is++) {
            int32_t sample = outputBuffer[is] * maxSample;
            // Sample does not need clipping because it is in range.
            // But let's do it anyway.
            if (sample > maxSample) {
                sample = maxSample;
            } else if (sample < minSample) {
                sample = minSample;
            }
            shortBuffer[is] = (int16_t) sample;
        }
        // Write to WAV file.
        wavResult =  Audio_WAV_WriteShorts(&writer, shortBuffer, samplesPerBlock);
        if( wavResult < 0 ) goto error;

        if (on && frameCounter > 30000) {
            synth.allNotesOff();
            on = false;
        }
        frameCounter += SYNTHMARK_FRAMES_PER_RENDER;
    }

    wavResult =  Audio_WAV_CloseWriter( &writer );
    if( wavResult < 0 ) goto error;
    printf("Test complete. Sound written to %s\n", filename);
    return 0;

error:
    printf("Test failed. wavResult = %ld\n", wavResult);
    return 1;
}
