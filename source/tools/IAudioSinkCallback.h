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

#ifndef ANDROID_AUDIOSINKCALLBACK_H
#define ANDROID_AUDIOSINKCALLBACK_H

#include <cstdint>

/**
 * Interface for a callback object that renders audio into a buffer.
 * This can be passed to an AudioSinkBase or its subclasses.
 */
class IAudioSinkCallback {
public:
    IAudioSinkCallback() { }

    virtual ~IAudioSinkCallback() = default;

    typedef enum {
        CALLBACK_ERROR = -1,
        CALLBACK_CONTINUE = 0,
        CALLBACK_FINISHED = 1,   // stop calling the callback
    } audio_sink_callback_result_t;

    virtual audio_sink_callback_result_t renderAudio(float *buffer, int32_t numFrames) = 0;
};
#endif //ANDROID_AUDIOSINKCALLBACK_H
