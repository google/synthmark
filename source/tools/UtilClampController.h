//
// Created by biagio on 26/07/21.
//

#ifndef ANDROID_UTILCLAMPCONTROLLER_H
#define ANDROID_UTILCLAMPCONTROLLER_H


#include <linux/sched.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

#define USE_SCHED_FIFO 1

class UtilClampController {

public:

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

    inline bool isUtilClampSupported(){
        if(syscall(SYS_sched_setattr, 0 /* self */, &util_set, 0) != 0){
            return 0;
        }
        if(syscall(SYS_sched_setattr, 0 /* self */, &util_default, 0) != 0){
            return 0;
        }
        return 1;
    }

    inline int setUtilClampMin(int clamp_value) {
        util_set.sched_util_min = clamp_value;
        return syscall(SYS_sched_setattr, 0 /* self */, &util_set, 0);
    }

    inline int setUtilClampMax(int clamp_value) {
        util_set.sched_util_max = clamp_value;
        return syscall(SYS_sched_setattr, 0 /* self */, &util_set, 0);
    }

    inline int setUtilClampDefault() {
        return syscall(SYS_sched_setattr, 0 /* self */, &util_default, 0);
    }

    inline int getUtilClampMin() {
        if(syscall(SYS_sched_getattr, 0 /* self */, &util_get, sizeof(struct sched_attr), 0) != 0){
            return -1;
        }
        else{
            return util_get.sched_util_min;
        }
    }

    inline int getUtilClampMax() {
        if(syscall(SYS_sched_getattr, 0 /* self */, &util_get, sizeof(struct sched_attr), 0) != 0){
            return -1;
        }
        else{
            return util_get.sched_util_max;
        }
    }

};

#endif //ANDROID_UTILCLAMPCONTROLLER_H

