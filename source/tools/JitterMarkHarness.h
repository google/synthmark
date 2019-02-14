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

#ifndef SYNTHMARK_JITTERMARK_HARNESS_H
#define SYNTHMARK_JITTERMARK_HARNESS_H

#include <cmath>
#include <cstdint>
#include <iomanip>

#include "AudioSinkBase.h"
#include "ChangingVoiceHarness.h"
#include "SynthMark.h"
#include "synth/Synthesizer.h"
#include "TestHarnessParameters.h"
#include "tools/CpuAnalyzer.h"
#include "tools/LogTool.h"
#include "tools/TestHarnessBase.h"
#include "tools/TimingAnalyzer.h"

/**
 * Measure the wakeup time and render time for each wakeup period.
 */
class JitterMarkHarness : public ChangingVoiceHarness {
public:
    JitterMarkHarness(AudioSinkBase *audioSink, SynthMarkResult *result, LogTool &logTool)
            : ChangingVoiceHarness(audioSink, result, logTool) {
        mTestName = "JitterMark";
    }

    virtual ~JitterMarkHarness() {
    }

    void onBeginMeasurement() override {
        mResult->setTestName(mTestName);
        mLogTool.log("---- Measure scheduling jitter ---- #voices = %d\n", getNumVoices());
        setupHistograms();
    }

    virtual int32_t onBeforeNoteOn() override {
        return 0;
    }

    virtual void onEndMeasurement() override {

        double measurement = 0.0;
        std::stringstream resultMessage;
        resultMessage << mTestName << " = " << measurement << std::endl;

        resultMessage << dumpJitter();
        resultMessage << "underrun.count = " << mAudioSink->getUnderrunCount() << "\n";
        resultMessage << mCpuAnalyzer.dump();

        mResult->setMeasurement(measurement);
        mResult->appendMessage(resultMessage.str());
    }


};

#endif // SYNTHMARK_JITTERMARK_HARNESS_H
