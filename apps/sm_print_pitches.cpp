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
#include "SynthMark.h"
#include "synth/PitchToFrequency.h"

int main(void)
{
    PitchToFrequency tuner;
    printf("---- Frequencies for semitones. ----\n");
    printf("middle C  = %1f\n", PitchToFrequency::convertPitchToFrequency(60));
    printf("concert A = %f\n", PitchToFrequency::convertPitchToFrequency(69));

    for (int i = 0; i < 127; i++) {
        printf("%d => %8.4f\n", i, tuner.lookupPitchToFrequency(i));
    }
    printf("Test complete.\n");
    return 0;
}
