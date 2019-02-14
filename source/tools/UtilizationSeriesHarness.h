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


#ifndef ANDROID_UTILIZATION_SERIES_HARNESS_H
#define ANDROID_UTILIZATION_SERIES_HARNESS_H

#include "TestHarnessParameters.h"
#include "VoiceMarkHarness.h"
#include "UtilizationMarkHarness.h"

class UtilizationSeriesHarness : public TestHarnessParameters {

public:
    UtilizationSeriesHarness(AudioSinkBase *audioSink, SynthMarkResult *result, LogTool &logTool)
            : TestHarnessParameters(audioSink, result, logTool) {}

    virtual ~UtilizationSeriesHarness() {}

    const char *getName() const override {
        return "Utilization Series";
    }

    int32_t runTest(int32_t sampleRate, int32_t framesPerBurst, int32_t numSeconds) override {
        int32_t err = 0;

        // If user does not specify a max value then measure a reasonable one.
        if (getNumVoicesHigh()  == 0) {
            int32_t maxVoices = 0;
            err = measureVoiceMark(sampleRate, framesPerBurst, numSeconds, 0.95, &maxVoices);
            if (err != SYNTHMARK_RESULT_SUCCESS) {
                return err;
            }
            setNumVoicesHigh((int32_t) maxVoices);
        }

        mResult->appendMessage("voices, utilization\n");

        // Iterate over a range of voice counts.
        int32_t numVoicesBegin = getNumVoices();
        int32_t numVoicesEnd = getNumVoicesHigh();
        mLogTool.log("max voices for series = %d\n", numVoicesEnd);
        int32_t range = numVoicesEnd - numVoicesBegin;
        const int kNumSteps = 20;
        for (int i = 0; i < kNumSteps; i++) {
            double utilization = 0.0;
            int32_t numVoices = numVoicesBegin + ((i * range) / (kNumSteps - 1));
            err = measureUtilizationOnce(sampleRate, framesPerBurst,
                                         numSeconds, numVoices,
                                         &utilization);
            if (err != SYNTHMARK_RESULT_SUCCESS) {
                return err;
            }
            // We are maxing out the CPU so stop the series.
            if (utilization > 0.96) {
                break;
            }
        }
        return err;
    }

    int32_t measureVoiceMark(int32_t sampleRate,
                             int32_t framesPerBurst,
                             int32_t numSeconds,
                             double fractionUtilization,
                             int32_t *maxVoicesPtr) {
        std::stringstream resultMessage;
        SynthMarkResult result1;
        VoiceMarkHarness *harness = new VoiceMarkHarness(mAudioSink, &result1, mLogTool);
        harness->setTargetCpuLoad(fractionUtilization);
        harness->setInitialVoiceCount(getNumVoices());
        harness->setDelayNoteOnSeconds(mDelayNotesOn);
        harness->setThreadType(mThreadType);

        int32_t err = harness->runTest(sampleRate, framesPerBurst, 15);
        delete harness;
        if (err != SYNTHMARK_RESULT_SUCCESS) {
            return err;
        }

        *maxVoicesPtr = (int32_t) result1.getMeasurement();

        resultMessage << "VoiceMarkMax = " << *maxVoicesPtr << std::endl;

        mResult->appendMessage(resultMessage.str());
        return SYNTHMARK_RESULT_SUCCESS;
    }

    int32_t measureUtilizationOnce(int32_t sampleRate,
                               int32_t framesPerBurst,
                               int32_t numSeconds,
                               int32_t numVoices,
                               double *utilizationPtr) {
        std::stringstream resultMessage;
        SynthMarkResult result1;
        UtilizationMarkHarness *harness = new UtilizationMarkHarness(mAudioSink,
                                                                     &result1,
                                                                     mLogTool);
        harness->setNumVoices(numVoices);
        harness->setDelayNoteOnSeconds(mDelayNotesOn);
        harness->setThreadType(mThreadType);

        int32_t err = harness->runTest(sampleRate, framesPerBurst, numSeconds);
        delete harness;
        if (err != SYNTHMARK_RESULT_SUCCESS) {
            return err;
        }

        double utilization = result1.getMeasurement();

        resultMessage << "   " << numVoices << ", " << utilization << std::endl;
        mResult->appendMessage(resultMessage.str());

        *utilizationPtr = utilization;
        return SYNTHMARK_RESULT_SUCCESS;
    }

};

#endif //ANDROID_UTILIZATION_SERIES_HARNESS_H
