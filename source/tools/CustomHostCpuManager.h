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

#ifndef CUSTOM_HOST_CPU_MANAGER_H
#define CUSTOM_HOST_CPU_MANAGER_H

#include <cstdint>
#include <iostream>
#include <map>

#include "HostTools.h"
#include "scheddl.h"


constexpr double ALPHA_BIG	= 0.95;
constexpr double ALPHA_SMALL	= 0.1;
constexpr uint32_t STATS_PERIOD	= 30;
constexpr double BW_MAX		= 0.9;
constexpr double BW_BOOST	= BW_MAX;
constexpr double BW_OFFSET_REL	= 1.1;
constexpr double BW_OFFSET_ABS	= 0.05;

class CustomHostCpuManager : public HostCpuManagerBase
{
public:
    /**
     * Update runtime statistics and sleeps for wakeupTime_ns.
     *
     * @param wakeupTime_ns sleep duration, in nanoseconds
     */
    void sleepAndTuneCPU(int64_t wakeupTime_ns) override {
        /* Read finishing time of the callback */
        mEndingTime_ns = HostTools::getNanoTime();
        getDeadlineAttribute(mDLEnd);

        /* Update callback duration statistics */
        calculateRuntime();

        if (wakeupTime_ns == 0) {
            /*
             * If wakeupTime_ns is 0, it means that two consecutive bursts must
             * be written, so the system is facing an overload condition. In
             * this case, the bandwidth of the task is forced to a high value
             * to solve the transitory.
             */
            boostBandwidth();
        } else {
            if (!mBoosted) {
                /*
                 * The bandwidth is also updated every mBWUpdatePeriod bursts
                 * written during normal (without boosting) behaviors. This
                 * allows the system to to adjust the bandwidth without having
                 * to wait for the number of workUnits to change.
                 *
                 * TODO: this is a very simple approach. Other solutions could
                 * be, for example, to adjust the mBWUpdatePeriod value
                 * according to the standard deviation of the measured runtimes
                 * or to force the bandwidth update when the last measured
                 * workload is above/below a given threshold.
                 */
                mBWUpdateCounter++;
                if (mBWUpdateCounter >= STATS_PERIOD) {
                    mBWUpdateCounter = 0;
                    setApplicationLoad(getCurrentWorkUnits(), getMaxWorkUnits());
                }
            } else {
                unboostBandwidth();
            }

            HostTools::sleepUntilNanoTime(wakeupTime_ns);
        }

        /* Read starting time of the callback */
        getDeadlineAttribute(mDLStart);
        mStartingTime_ns = HostTools::getNanoTime();
    }

    /**
     * This is called when the application determines its workload for the
     * callback.
     *
     * It calculates the runtime parameter to be assigned to the SCHED_DEADLINE
     * Linux scheduling class, which adapts the CPU frequency accordingly.
     *
     * @param currentWorkUnits the current work units requested by the
     * application, representing the computational effort that is going to be
     * requested in the future computations. The meaning of this value may be
     * different for different applications, e.g. it can be the number of notes
     * played or the number of filters applied to a given signal.
     * @param maxWorkUnits (not used) the maximum number of work units the
     * application is planning to request.
     */
    void setApplicationLoad(int32_t currentWorkUnits,
                            int32_t maxWorkUnits) override {
        /*
         * Perform an update only when the number of work units is changed, or
         * if the mBWUpdateCounter is 0 (periodic update).
         */
        if (currentWorkUnits != getCurrentWorkUnits() ||
                maxWorkUnits != getMaxWorkUnits() ||
                mBWUpdateCounter == 0) {
            int64_t expectedRuntime_ns;
            double bandwidth;

            /*
             * mBWUpdateCounter is reset to avoid consecutive, automatic
             * bandwith updates.
             */
            if (mBWUpdateCounter != 0)
                mBWUpdateCounter = 0;

            expectedRuntime_ns = mCpuTiming.getWorkUnitRuntime(currentWorkUnits);

            /*
             * When we read a 0 runtime for != 0 workUnits, it means that
             * something bad is going on, so, set the maximum runtime.
             * This may happen when the linear regression fails.
             */
            if (expectedRuntime_ns == 0 && currentWorkUnits != 0) {
                bandwidth = BW_MAX;
            } else {
                bandwidth = static_cast<double>(expectedRuntime_ns) / mPeriod_ns;

                /*
                 * Add some margins to the computed bandwidth, since the
                 * application execution times are noisy.
                 * A first margin is a multiplication factor, meaning that the margin
                 * proportionally increases with the duration.
                 */ 
                bandwidth *= BW_OFFSET_REL;
                /* A second margin is an absolute offset. */
                bandwidth += BW_OFFSET_ABS;

                /* Bound the bandwidth to the limit set by the Kernel */
                if (bandwidth > BW_MAX)
                    bandwidth = BW_MAX;
            }

            if (currentWorkUnits != getCurrentWorkUnits() ||
                    maxWorkUnits != getMaxWorkUnits()) {
                /* Print the current status only when the workUnits changed */
                printf("workUnits=%d\trequestDLBandwidth=%f",
                       currentWorkUnits, bandwidth);
                printf("\t[");
                int width = 30;
                int i;
                for (i=0; i<bandwidth * width; ++i)
                    printf("#");
                for (; i<width; ++i)
                    printf("_");
                printf("]");
                printf("\n");
                fflush(stdout);
            }

            expectedRuntime_ns = mPeriod_ns * bandwidth;
            updateDeadlineParams(expectedRuntime_ns,
                                 mDeadline_ns,
                                 mPeriod_ns);
        }
        HostCpuManagerBase::setApplicationLoad(currentWorkUnits, maxWorkUnits);
    }

    /**
     * Update the current deadline parameters both of the scheduler and the
     * class members.
     */
    int updateDeadlineParams(uint64_t runtime_ns,
                             uint64_t deadline_ns,
                             uint64_t period_ns,
                             bool boostChanged = false) {
        int ret = 0;

        if (boostChanged ||
                mRuntime_ns != runtime_ns ||
                mDeadline_ns != deadline_ns ||
                mPeriod_ns != period_ns) {
            /*
             * Fixed parameters to define a DEADLINE task. All the
             * non-initialized set elements of the structure are automatically
             * set to zero.
             */
            struct sched_attr sa = {
                .size = sizeof(sa),
                .sched_policy = SCHED_DEADLINE
            };

            /*
             * Parameters that define the behavior of a DEADLINE task.
             * If the task is boosted, then apply the maximum bandwidth.
             */
            if (mBoosted)
                sa.sched_runtime = period_ns * BW_BOOST;
            else
                sa.sched_runtime = runtime_ns;
            sa.sched_deadline = deadline_ns;
            sa.sched_period = period_ns;

            ret = sched_setattr(0, &sa, 0);
            if (ret == -1) {
                perror("Error setting SCHED_DEADLINE");
                fflush(stderr);
                std::cout << "- runtime:\t" << sa.sched_runtime << std::endl;
                std::cout << "- deadline:\t" << sa.sched_deadline << std::endl;
                std::cout << "- period:\t" << sa.sched_period << std::endl;
                exit(-1);
            } else {
                /*
                 * If the syscall to change the scheduler succeeds, then update
                 * the class members.
                 */
                mRuntime_ns = runtime_ns;
                mDeadline_ns = deadline_ns;
                mPeriod_ns = period_ns;
            }
        }

        return ret;
    }

private:
    /*
     * Forces the maximum bandwidth to the task.
     */
    void boostBandwidth() {
        if (!mBoosted) {
            mBoosted = true;
            updateDeadlineParams(mRuntime_ns, mDeadline_ns, mPeriod_ns, true);
        }
    }

    /*
     * Returns from a boosted bandwidth to the normal behavior.
     */
    void unboostBandwidth() {
        if (mBoosted) {
            mBoosted = false;
            updateDeadlineParams(mRuntime_ns, mDeadline_ns, mPeriod_ns, true);
        }
    }

    /**
     * Updates the data structure with the current absolute parameters returned
     * by a sched_getattr().
     * This function is currently used to obtain the remaining SCHED_DEADLINE
     * runtime (that is normalized according to the cpu capacity and the cpu
     * frequency) and the current absolute deadline, that can be used to
     * measure how many overruns happened during the task execution.
     *
     * @param sa the sched_attr structure that is filled with the
     * sched_getattr results.
     */
    int getDeadlineAttribute(struct sched_attr &sa) {
        int ret = sched_getattr(0,
                                &sa,
                                sizeof(sa),
                                SCHED_GETATTR_FLAGS_DL_ABSOLUTE);
        if (ret == -1) {
            perror("ERROR performing absolute getattr for SCHED_DEADLINE");
            perror("ERROR This kernel probably does not have the necessary patches");
            exit(-1);
        }

        return ret;
    }
    // ------------------------------------------------------------------------------

    /**
     * Updates the callback runtime statistics.
     */
    void calculateRuntime() {
        /* Initialization state, still inconsistent. Skip! */
        if (mEndingTime_ns < 0 || mStartingTime_ns < 0)
            return;

        int64_t duration; /* The actual duration of the callback */

        /*
         * The remaining runtime acts like a timer: it is set to the maximum
         * value when the task starts, and then is decremented until it reaches
         * 0. The duration of a slice of code can be thus measured by
         * subtracting the sampled remaining runtime at the beginning with the
         * sampled remaining runtime at the end.
         */
        int64_t durationDL = mDLStart.sched_runtime - mDLEnd.sched_runtime;

        /*
         * When the runtime reaches 0, the task is throttled and has to wait
         * until the replenishment time to start back executing. When the
         * replenishment occurs, the task is put back in execution and may
         * finish its budget again and again (this shouldn't happen, it means
         * that a DEADLINE task is misbehaving).
         *
         * "delta" represents the range between the absolute deadlines sampled
         * at the beginning and the end of the measured code slice.
         *
         * "overruns" represents how many replenishments happened between the
         * to samples.
         */
        int64_t delta = mDLEnd.sched_deadline - mDLStart.sched_deadline;
        if (delta > 0) {
            int64_t overruns = delta / mDLEnd.sched_period;
            if (overruns > 0)
                durationDL += overruns * mDLEnd.sched_runtime;
        }

        /*
         * If for some precision reason the remaining runtime is not 0, then
         * use the CLOCK_MONOTONIC time */
        int64_t durationTimer;
        if (durationDL != 0) {
            duration = durationDL;
        } else {
            durationTimer = mEndingTime_ns - mStartingTime_ns;
            duration = durationTimer;
        }

        mCpuTiming.reportApplicationRuntime(duration, getCurrentWorkUnits());
    }

    /**
     * Manage the runtime, by updating and returning statistics.
     */
    class CpuTiming {
    public:

        /**
         * Updates the runtime statistics for a given number of workUnits.
         *
         * @param callbackComputingTime callback runtime in nanoseconds
         * @param currentWorkUnits the current number of workUnits
         */
        void reportApplicationRuntime(int64_t callbackComputingTime,
                                      int32_t currentWorkUnits) {
            if (callbackComputingTime == 0)
                return;

            if (smallestSampledWorkUnits > currentWorkUnits ||
                    smallestSampledWorkUnits == -1)
                smallestSampledWorkUnits = currentWorkUnits;

            if (mUtils.find(currentWorkUnits) == mUtils.end()) {
                /*
                 * If this is the first time we encounter that workUnits, the
                 * result is directly stored.
                 */
                mUtils[currentWorkUnits] = callbackComputingTime;
            } else {
                /*
                 * If there are already statistics for that given number of
                 * workUnits, apply simple exponential smoothing filter.
                 * In order to give more weight to the increasing values, two
                 * different time constants are applied when the value is
                 * increasing or decreasing.
                 */
                int64_t stored = mUtils[currentWorkUnits];
                if (stored < callbackComputingTime)
                    mUtils[currentWorkUnits] =
                            ALPHA_BIG * callbackComputingTime +
                            (1.0 - ALPHA_BIG) * stored;
                else
                    mUtils[currentWorkUnits] =
                            ALPHA_SMALL * callbackComputingTime +
                            (1.0 - ALPHA_SMALL) * stored;
            }
        }

        /**
         * Return the a runtime estimation, given the number of workUnits.
         *
         * @param workUnits
         */
        int64_t getWorkUnitRuntime(int32_t workUnits) {
            int64_t runtime;

            if (mUtils.find(workUnits) != mUtils.end()) {
                /* The number of workUnits has been found in the hash table! */
                runtime = mUtils[workUnits];
            } else if (workUnits < smallestSampledWorkUnits) {
                /*
                 * If we are requesting a number of workUnits that is smaller
                 * than any other workUnits in our database, then we assign it
                 * the bandwidth of the runtime associated with the smallest
                 * workUnits entry of our database. This can be much bigger
                 * than what the callback requires, but is safe and is fast.
                 */
                runtime = mUtils[smallestSampledWorkUnits];
            } else {
                /*
                 * When is required a number of workUnits that is bigger than
                 * what we ever measured, a linea regression problem is solved.
                 */
                std::pair<double, double> linear_params = getLinearParams();

                runtime = linear_params.first
                        + linear_params.second * workUnits;
            }
            return runtime;
        }

    private:

        /*
         * Returns the (a, b) parameters of the equation "y = a + b*x",
         * obtained by performing a min-square linear regression on the
         * statistics.
         */
        std::pair<double, double> getLinearParams() const {
            uint32_t samples = mUtils.size();

            if (samples == 0)
                return std::make_pair(0, 0);
            if (samples == 1)
                return std::make_pair(0, (*mUtils.begin()).second / (*mUtils.begin()).first);

            // linear regression using least squares
            double avg_x = 0, avg_y = 0;

            { // Compute the average values
                for (auto it=mUtils.begin(); it!=mUtils.end(); ++it) {
                    avg_x += (*it).first;
                    avg_y += (*it).second;
                }
                avg_x /= samples;
                avg_y /= samples;
            }

            double beta;
            { // Compute beta
                double numerator = 0, denumerator = 0;
                double x_value;

                for (auto it=mUtils.begin(); it!=mUtils.end(); ++it) {
                    x_value = (*it).first - avg_x;
                    numerator += x_value * ((*it).second - avg_y);
                    denumerator += x_value * x_value;
                }

                beta = numerator / denumerator;
            }

            double alpha = avg_y - beta * avg_x;

            if (alpha < 0)
                alpha = 0;
            if (beta < 0)
                beta = 0;

            return std::make_pair(alpha, beta);
        }

        int32_t smallestSampledWorkUnits = -1;
        std::map<int32_t, int64_t> mUtils;
    };

    CpuTiming mCpuTiming;

    struct sched_attr mDLStart;
    struct sched_attr mDLEnd;

    int64_t   mStartingTime_ns = -1;
    int64_t   mEndingTime_ns   = -1;

    bool      mBoosted         = false;

    uint64_t  mRuntime_ns      = 0;
    uint64_t  mDeadline_ns     = 0;
    uint64_t  mPeriod_ns       = 0;

    uint32_t  mBWUpdateCounter = 0;
};

#endif /* CUSTOM_HOST_CPU_MANAGER_H */
