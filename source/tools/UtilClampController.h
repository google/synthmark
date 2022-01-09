/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef ANDROID_UTIL_CLAMP_CONTROLLER_H
#define ANDROID_UTIL_CLAMP_CONTROLLER_H

#include <linux/sched.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

#if 0
const uint64_t SCHED_GETATTR_FLAGS_DL_ABSOLUTE = 0x01;

struct sched_attr {
    uint32_t size;
    uint32_t sched_policy;
    // For future implementation. By default should be set to 0.
    uint64_t sched_flags;
    // SCHED_OTHER niceness
    int32_t sched_nice;
    // SCHED_RT priority
    uint32_t sched_priority;
    // SCHED_DEADLINE parameters, in nsec
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
    //UtilClamp value
    uint32_t sched_util_min;
    uint32_t sched_util_max;
};

inline int sched_setattr(pid_t pid,
                                const struct sched_attr *attr,
                                unsigned int flags) {
    return syscall(SYS_sched_setattr, pid, attr, flags);
}

inline int sched_getattr(pid_t pid,
                                struct sched_attr *attr,
                                unsigned int size,
                                unsigned int flags) {
    attr->size = (uint32_t) sizeof(struct sched_attr);
    return syscall(SYS_sched_getattr, pid, attr, size, flags);
}
#endif

class UtilClampController {
public:
    static bool isSupported(pid_t pid = 0) {
        // Try modifying then restoring sched_util_min.
        int savedMin = getMin(pid);
        if (savedMin < 0) {
            return false;
        }
        if (setMin(123, pid) != 0) {
            return false;
        }
        if (setMin(savedMin, pid) != 0) {
            return false;
        }
        return true;
    }

    static int setMin(uint32_t clamp_value, pid_t pid = 0) {
        struct sched_attr attr;
        if (syscall(SYS_sched_getattr, pid, &attr, sizeof(attr), 0) != 0) {
            return -1;
        }
        attr.sched_util_min = clamp_value;
        attr.sched_flags = SCHED_FLAG_KEEP_ALL | SCHED_FLAG_UTIL_CLAMP_MIN;
        return syscall(SYS_sched_setattr, pid, &attr, 0);
    }

    static int setMax(int clamp_value, pid_t pid = 0) {
        struct sched_attr attr;
        if (syscall(SYS_sched_getattr, pid, &attr, sizeof(attr), 0) != 0) {
            return -1;
        }
        attr.sched_util_max = clamp_value;
        attr.sched_flags = SCHED_FLAG_KEEP_ALL | SCHED_FLAG_UTIL_CLAMP_MAX;
        return syscall(SYS_sched_setattr, pid, &attr, 0);
    }

    static int getMin(pid_t pid = 0) {
        struct sched_attr attr;
        if (syscall(SYS_sched_getattr, pid, &attr, sizeof(attr), 0) != 0) {
            return -1;
        } else {
            return attr.sched_util_min;
        }
    }

    static int getMax(pid_t pid = 0) {
        struct sched_attr attr;
        if (syscall(SYS_sched_getattr, pid, &attr, sizeof(attr), 0) != 0) {
            return -1;
        } else {
            return attr.sched_util_max;
        }
    }
};

#endif //ANDROID_UTILCLAMPCONTROLLER_H
