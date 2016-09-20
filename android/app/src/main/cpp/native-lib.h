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

#include <jni.h>
#include <android/log.h>
#ifndef ANDROID_NATIVE_LIB_H
#define ANDROID_NATIVE_LIB_H

#define  LOG_TAG    "native-lib"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jlong JNICALL
Java_com_sonodroid_synthmark_AppObject_testInit(
    JNIEnv* env,
    jobject obj,
    jint testId);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_testClose(
    JNIEnv* env __unused,
    jobject obj __unused,
    jlong nativeTest);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_testRun(
    JNIEnv* env,
    jobject obj,
    jlong nativeTest);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_testProgress(
    JNIEnv* env,
    jobject obj __unused,
    jlong nativeTest);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_testStatus(
    JNIEnv* env __unused,
    jobject obj __unused,
    jlong nativeTest);

JNIEXPORT jstring JNICALL
Java_com_sonodroid_synthmark_AppObject_testResult(
    JNIEnv* env,
    jobject obj __unused,
    jlong nativeTest);


#ifdef __cplusplus
}
#endif
#endif //ANDROID_NATIVE_LIB_H
