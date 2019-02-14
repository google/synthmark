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

#ifndef ANDROID_NATIVE_LIB_H
#define ANDROID_NATIVE_LIB_H

#include <jni.h>
#include <android/log.h>

#define  LOG_TAG    "native-lib"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jlong JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1create(
    JNIEnv* env,
    jobject obj);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_testInit(
    JNIEnv* env,
    jobject obj,
    jlong nativeTest,
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

JNIEXPORT jboolean JNICALL
Java_com_sonodroid_synthmark_AppObject_testHasLogs(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest);

JNIEXPORT jstring JNICALL
Java_com_sonodroid_synthmark_AppObject_testReadLog(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getTestCount(
    JNIEnv* env,
    jobject obj __unused,
    jlong nativeTest);

JNIEXPORT jstring JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getTestName(
    JNIEnv* env,
    jobject obj __unused,
    jlong nativeTest,
    jint testId);


JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamCount(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId);


JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamType(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamHoldType(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamListSize(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamListCurrentIndex(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamListDefaultIndex(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1setParamListCurrentIndex(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex,
        jint index);

JNIEXPORT jstring JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamListNameFromIndex(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex,
        jint index);

JNIEXPORT jstring JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamName(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex);

JNIEXPORT jstring JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getparamDesc(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1resetParamValue(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamIntMin(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamIntMax(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamIntValue(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamIntDefault(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1setParamIntValue(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex,
        jint value);

JNIEXPORT jfloat JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamFloatMin(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex);

JNIEXPORT jfloat JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamFloatMax(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex);

JNIEXPORT jfloat JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamFloatValue(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex);

JNIEXPORT jfloat JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamFloatDefault(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex);

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1setParamFloatValue(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex,
        jfloat value);

#ifdef __cplusplus
}
#endif
#endif //ANDROID_NATIVE_LIB_H
