#if !defined(SYNTHMARK_SCHEDDL_H) && !defined(__APPLE__)
#define SYNTHMARK_SCHEDDL_H

#include <linux/sched.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>


#ifndef SCHED_DEADLINE
#define SCHED_DEADLINE 6
#endif

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
    return syscall(SYS_sched_getattr, pid, attr, size, flags);
}

#endif /* SYNTHMARK_SCHEDDL_H */

