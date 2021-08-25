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

#ifndef ANDROID_UTILCLAMPCONTROLLER_H
#define ANDROID_UTILCLAMPCONTROLLER_H


#include <linux/sched.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

#define USE_SCHED_FIFO 1

class UtilClampController {

public:

    int16_t util_supported = -1;

    struct sched_attr util_default = {
            .size = sizeof(struct sched_attr),
            .sched_policy = SCHED_NORMAL,
    };

    /* struct used to set Utilclamp values*/
#if USE_SCHED_FIFO
    struct sched_attr util_set = {
            .size = sizeof(struct sched_attr),
            .sched_policy = SCHED_FIFO,
            .sched_flags = SCHED_FLAG_UTIL_CLAMP,
            .sched_priority = 99,
            .sched_util_min = 0,
            .sched_util_max = 1024,
    };
#else
    struct sched_attr util_set = {
                .size = sizeof(struct sched_attr),
                .sched_policy = SCHED_NORMAL,
                .sched_util_min = 0,
                .sched_util_max = 1024,
        };
#endif

    /* struct used to get value from sched_attr*/
    struct sched_attr util_get;

    inline bool isUtilClampSupported() {
        if (util_supported == -1) {
            if (syscall(SYS_sched_setattr, 0, &util_set, 0) != 0) {
                util_supported = 0;
            } else if (syscall(SYS_sched_setattr, 0, &util_default, 0) != 0) {
                util_supported = 0;
            } else {
                util_supported = 1;
            }
        }

        return util_supported;
    }

    inline int setUtilClampMin(int clamp_value) {
        util_set.sched_util_min = clamp_value;
        return syscall(SYS_sched_setattr, 0, &util_set, 0);
    }

    inline int setUtilClampMax(int clamp_value) {
        util_set.sched_util_max = clamp_value;
        return syscall(SYS_sched_setattr, 0, &util_set, 0);
    }

    inline int setUtilClampDefault() {
        return syscall(SYS_sched_setattr, 0, &util_default, 0);
    }

    inline int getUtilClampMin() {
        if (syscall(SYS_sched_getattr, 0, &util_get, sizeof(struct sched_attr), 0) != 0) {
            return -1;
        } else {
            return util_get.sched_util_min;
        }
    }

    inline int getUtilClampMax() {
        if (syscall(SYS_sched_getattr, 0, &util_get, sizeof(struct sched_attr), 0) != 0) {
            return -1;
        } else {
            return util_get.sched_util_max;
        }
    }

};

#endif //ANDROID_UTILCLAMPCONTROLLER_H

