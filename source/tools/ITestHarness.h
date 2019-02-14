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

#ifndef SYNTHMARK_ITEST_HARNESS_H
#define SYNTHMARK_ITEST_HARNESS_H

#include <cmath>
#include <cstdint>

class ITestHarness {

public:
    virtual void setNumVoices(int32_t numVoices) = 0;

    virtual void setDelayNoteOnSeconds(int32_t seconds) = 0;

    virtual const char *getName() const = 0;

    virtual int32_t runCompleteTest(int32_t sampleRate,
                                    int32_t framesPerBurst,
                                    int32_t numSeconds) = 0;

    virtual void setThreadType(HostThreadFactory::ThreadType mThreadType) = 0;

    virtual void launch(int32_t sampleRate,
                   int32_t framesPerBurst,
                   int32_t numSeconds) = 0;

    virtual bool isRunning() = 0;

};

#endif //SYNTHMARK_ITEST_HARNESS_H
