/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_CHANGING_VOICE_HARNESS_H
#define ANDROID_CHANGING_VOICE_HARNESS_H

#include "tools/TestHarnessBase.h"
#include "TestHarnessParameters.h"


#define NOTES_PER_STEP   10


class ChangingVoiceHarness : public TestHarnessBase {

public:
    ChangingVoiceHarness(AudioSinkBase *audioSink, SynthMarkResult *result, LogTool &logTool)
    : TestHarnessBase(audioSink, result, logTool)
    {
        // Constant seed to obtain a fixed pattern of pseudo-random voices
        // for experiment repeatability and reproducibility
        //srand(time(NULL)); // "Random" seed
        srand(0); // Fixed seed
    }

    virtual ~ChangingVoiceHarness() {
    }

    int32_t getCurrentNumVoices() override {
        if (mNumVoicesHigh > 0) {
            // The same number of voices is kept every (NOTES_PER_STEP / 2).
            bool needUpdate = ((getNoteCounter() % (NOTES_PER_STEP / 2)) == 0);
            static int32_t lastVoices;

            if (needUpdate) {
                switch (mVoicesMode) {
                    case VOICES_LINEAR_LOOP:
                        // The number of voices is linearly incremented in the
                        // range [-n, -N]. When it reaches -N, it restarts back
                        // from -n.
                        lastVoices += NOTES_PER_STEP / 2;
                        if (lastVoices > mNumVoicesHigh ||
                                lastVoices < getNumVoices())
                            lastVoices = getNumVoices();
                        break;
                    case VOICES_RANDOM:
                        // Return a number of voices in the range [-n, -N].
                        lastVoices = (rand() % (mNumVoicesHigh - getNumVoices() + 1))
                                + getNumVoices();
                        break;
                    case VOICES_SWITCH:
                    default:
                        // Start low then high then low and so on.
                        // Pattern should restart on each test.
                        lastVoices = ((getNoteCounter() % NOTES_PER_STEP) < (NOTES_PER_STEP / 2))
                                     ? getNumVoices()
                                     : mNumVoicesHigh;
                        break;
                }
            }
            if (isVerbose()) {
                mLogTool.log("%s() returns %d\n", __func__, lastVoices);
            }
            return lastVoices;
        } else {
            return getNumVoices();
        }
    }

private:

};


#endif //ANDROID_CHANGING_VOICE_HARNESS_H
