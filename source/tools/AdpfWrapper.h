/*
 * Copyright 2021 The Android Open Source Project
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

#ifndef SYNTHMARK_ADPF_WRAPPER_H
#define SYNTHMARK_ADPF_WRAPPER_H

#include <algorithm>
#include <functional>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <mutex>

struct APerformanceHintManager;
struct APerformanceHintSession;

typedef struct APerformanceHintManager APerformanceHintManager;
typedef struct APerformanceHintSession APerformanceHintSession;

class AdpfWrapper {
public:
     /**
      * Create an ADPF session that can be used to boost performance.
      * @param threadId
      * @param targetDurationNanos - period of isochronous task
      * @return zero or negative error
      */
    int open(pid_t threadId, int64_t targetDurationNanos);

    /**
     * Report the measured duration of a callback.
     * @param actualDurationNanos
     */
    void reportActualDuration(int64_t actualDurationNanos);

    void close();

private:
    std::mutex mLock;
    APerformanceHintSession* mHintSession = nullptr;
};

#endif //SYNTHMARK_ADPF_WRAPPER_H
