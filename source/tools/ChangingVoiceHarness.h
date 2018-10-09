//
// Created by philburk on 10/9/18.
//

#ifndef ANDROID_CHANGINGVOICEHARNESS_H
#define ANDROID_CHANGINGVOICEHARNESS_H


#include "tools/TestHarnessBase.h"
#include "TestHarnessParameters.h"
#include "LatencyMarkHarness.h"

enum VoicesMode {
    VOICES_UNDEFINED,
    VOICES_SWITCH,
    VOICES_RANDOM,
    VOICES_LINEAR_LOOP,
};

#define NOTES_PER_STEP   10


class ChangingVoiceHarness : public TestHarnessBase {

public:
    ChangingVoiceHarness(AudioSinkBase *audioSink,
            SynthMarkResult *result,
            LogTool *logTool = nullptr)
    : TestHarnessBase(audioSink, result, logTool)
    {
        // Constant seed to obtain a fixed pattern of pseudo-random voices
        // for experiment repeatability and reproducibility
        //srand(time(NULL)); // "Random" seed
        srand(0); // Fixed seed
    }

    virtual ~ChangingVoiceHarness() {
    }

    void setVoicesMode(VoicesMode vm) {
        mVoicesMode = vm;
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
                printf("%s() returns %d\n", __func__, lastVoices);
                fflush(stdout);
            }
            return lastVoices;
        } else {
            return getNumVoices();
        }
    }
private:

    VoicesMode        mVoicesMode = VOICES_SWITCH;
};


#endif //ANDROID_CHANGINGVOICEHARNESS_H
