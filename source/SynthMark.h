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
 *
 */

#ifndef SYNTHMARK_SYNTHMARK_H
#define SYNTHMARK_SYNTHMARK_H

// We use #define here so we can build strings easier.
#define SYNTHMARK_MAJOR_VERSION        1
// #define SYNTHMARK_MINOR_VERSION        6   /* Run under OpenSL thread for SCHED_FIFO. */
// #define SYNTHMARK_MINOR_VERSION        7   /* wakeup/render/delivery jitter */
// #define SYNTHMARK_MINOR_VERSION        8   /* changed SYNTHMARK_FRAMES_PER_BURST from 128 => 64 */
// #define SYNTHMARK_MINOR_VERSION        9   /* Fixed iOS version */
// #define SYNTHMARK_MINOR_VERSION        10  /* Added CPU migration histogram */
// #define SYNTHMARK_MINOR_VERSION        11  /* Added UtilizationMark, print Jitter after Latency */
// #define SYNTHMARK_MINOR_VERSION        12  /* Added CPU Governor hints */
// #define SYNTHMARK_MINOR_VERSION        13  /* Default burst size changed from 64 to 96 frames */
// #define SYNTHMARK_MINOR_VERSION        14  /* Use more consistent report format */
// #define SYNTHMARK_MINOR_VERSION        15  /* Fix LatencyMark low-high pattern. */
// #define SYNTHMARK_MINOR_VERSION        16  /* Use AAudio callback thread on Android. */
// #define SYNTHMARK_MINOR_VERSION        17  /* Add -N to JitterMark. */
// #define SYNTHMARK_MINOR_VERSION        18  /* Add "-tc", ClockRamp test. Add harness to Android app. */
// #define SYNTHMARK_MINOR_VERSION        19  /* Add -w1 for SCHED_DEADLINE. */
// #define SYNTHMARK_MINOR_VERSION        20  /* Optimize search for LatencyMark. */
// #define SYNTHMARK_MINOR_VERSION        21  /* Fix intermittent hang on STOP */
// #define SYNTHMARK_MINOR_VERSION        22  /* Add CDD Summary to Auto Test */
// #define SYNTHMARK_MINOR_VERSION        23  /* Add UtilClamp -u, and SCHED_FIFO option -f */
// #define SYNTHMARK_MINOR_VERSION        24  /* Add real-time audio output using AAudio, -a2 */
#define SYNTHMARK_MINOR_VERSION        25  /* Add ADPF support, -z1 */

#define SYNTHMARK_STRINGIFY(x) #x
#define SYNTHMARK_TOSTRING(x) SYNTHMARK_STRINGIFY(x)

// Type: String literal. See below for description.
#define SYNTHMARK_VERSION_TEXT \
        SYNTHMARK_TOSTRING(SYNTHMARK_MAJOR_VERSION) "." \
        SYNTHMARK_TOSTRING(SYNTHMARK_MINOR_VERSION)

// This may be increased without invalidating the benchmark.
constexpr int kSynthmarkMaxVoices   = 1024;

constexpr int kSynthmarkNumVoicesLatency  = 10;

// The number of frames that are synthesized at one time.
constexpr int kSynthmarkFramesPerRender  =  8;

constexpr int kSynthmarkSampleRate = 48000;

// These should not be changed.
constexpr int64_t SYNTHMARK_MILLIS_PER_SECOND      = 1000;
constexpr int64_t SYNTHMARK_MICROS_PER_SECOND      = 1000 * 1000;
constexpr int64_t SYNTHMARK_NANOS_PER_MICROSECOND  = 1000;
constexpr int64_t SYNTHMARK_NANOS_PER_MILLISECOND  = 1000 * 1000;
constexpr int64_t SYNTHMARK_NANOS_PER_SECOND       = 1000 * 1000 * 1000;

typedef float synth_float_t;

#define TEXT_CSV_BEGIN         "CSV_BEGIN"
#define TEXT_CSV_END           "CSV_END"
#define TEXT_RESULTS_BEGIN     "RESULTS_BEGIN"
#define TEXT_RESULTS_END       "RESULTS_END"
#define TEXT_CDD_SUMMARY_BEGIN "CDD_SUMMARY_BEGIN"
#define TEXT_CDD_SUMMARY_END   "CDD_SUMMARY_END"

#endif // SYNTHMARK_SYNTHMARK_H
