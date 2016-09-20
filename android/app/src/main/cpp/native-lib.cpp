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

#include "native-lib.h"
#include <string>
#include <sstream>
#include <android/log.h>

#include "tools/NativeTest.h"

JNIEXPORT jlong JNICALL
Java_com_sonodroid_synthmark_AppObject_testInit(
    JNIEnv* env __unused,
    jobject obj __unused,
    jint testId) {

    NativeTest * pNativeTest = new NativeTest();
    if (pNativeTest != NULL) {
        pNativeTest->init(testId);
    }
    return (long) pNativeTest;
}

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_testClose(
    JNIEnv* env __unused,
    jobject obj __unused,
    jlong nativeTest) {
    NativeTest * pNativeTest = (NativeTest*) (size_t) nativeTest;

    if (pNativeTest != NULL) {
        int status = pNativeTest->closeTest();
        delete(pNativeTest);
        return status;
    }
    return NATIVETEST_ERROR;
}

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_testRun(
    JNIEnv* env __unused,
    jobject obj __unused,
    jlong nativeTest) {

    NativeTest * pNativeTest = (NativeTest*) (size_t) nativeTest;

    if (pNativeTest != NULL) {
        return pNativeTest->run();
    }
    return NATIVETEST_ERROR;
}

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_testProgress(
    JNIEnv* env __unused,
    jobject obj __unused,
    jlong nativeTest) {
    NativeTest * pNativeTest = (NativeTest*) (size_t) nativeTest;

    if (pNativeTest != NULL) {
        return pNativeTest->getProgress();
    }
    return -1;
}

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_testStatus(
    JNIEnv* env __unused,
    jobject obj __unused,
    jlong nativeTest) {
    NativeTest * pNativeTest = (NativeTest*) (size_t) nativeTest;

    if (pNativeTest != NULL) {
        return pNativeTest->getStatus();
    }
    return NATIVETEST_STATUS_ERROR;
}

JNIEXPORT jstring JNICALL
Java_com_sonodroid_synthmark_AppObject_testResult(
    JNIEnv* env,
    jobject obj __unused,
    jlong nativeTest) {
    NativeTest * pNativeTest = (NativeTest*) (size_t) nativeTest;

    if (pNativeTest != NULL) {
        std::string result = pNativeTest->getResult();
        return env->NewStringUTF(result.c_str());
    }
    return NULL;
}

