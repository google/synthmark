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
#include "portaudio/PortAudioSink.h"
#include "tools/SynthMarkHarness.h"

#define NUM_SECONDS     10
#define SAMPLE_RATE     SYNTHMARK_SAMPLE_RATE

int main(void)
{
    PortAudioSink audioSink;
    SynthMarkHarness harness(&audioSink);
    double fraction = 0.5;

    printf("---- Measure number of voices for 50%c CPU load. ----\n", '%');

    harness.open(SAMPLE_RATE, SAMPLES_PER_FRAME, SYNTHMARK_FRAMES_PER_BUFFER);
    harness.measureSynthMark(NUM_SECONDS, fraction);
    harness.close();
    printf("SynthMark %d = %7.3f\n", (int)(fraction * 100), harness.getSynthMark());

    printf("Test complete.\n");
    return 0;
}

