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

#include "synth/IncludeMeOnce.h"

#include "tools/RealAudioSink.h"
#include "tools/NativeTest.h"

JNIEXPORT jlong JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1create(
    JNIEnv* env,
    jobject obj) {

    NativeTest * pNativeTest = new NativeTest();
    return (jlong) pNativeTest;
}

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_testInit(
        JNIEnv* env,
        jobject obj,
        jlong nativeTest,
        jint testId) {
    NativeTest * pNativeTest = (NativeTest*) (size_t) nativeTest;
    if (pNativeTest != NULL) {
        return pNativeTest->init(testId);
    }
    return NATIVETEST_ERROR;
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

JNIEXPORT jboolean JNICALL
Java_com_sonodroid_synthmark_AppObject_testHasLogs(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest) {
    NativeTest * pNativeTest = (NativeTest*) (size_t) nativeTest;
    if (pNativeTest != NULL) {
        return pNativeTest->hasLogs();
    }
    return false;
}

JNIEXPORT jstring JNICALL
Java_com_sonodroid_synthmark_AppObject_testReadLog(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest) {
    NativeTest * pNativeTest = (NativeTest*) (size_t) nativeTest;
    if (pNativeTest != NULL) {
        std::string log = pNativeTest->readLog();
        return env->NewStringUTF(log.c_str());
    }
    return env->NewStringUTF("[native closed]");
}

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getTestCount(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest) {
    NativeTest * pNativeTest = (NativeTest*) (size_t) nativeTest;
    int count = 0;
    if (pNativeTest != NULL) {
        count = pNativeTest->getTestCount();
    }
    return count;
}

JNIEXPORT jstring JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getTestName(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId) {
    NativeTest * pNativeTest = (NativeTest*) (size_t) nativeTest;
    if (pNativeTest != NULL) {
        std::string name = pNativeTest->getTestName(testId);;
        return env->NewStringUTF(name.c_str());
    }
    return NULL;
}

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamCount(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId) {
    int count = 0;
    NativeTest * pNativeTest = (NativeTest*) (size_t) nativeTest;
    if (pNativeTest != NULL) {
        count = pNativeTest->getParamCount(testId);
    }
    return count;
}


//helper function
ParamGroup * helperGetParamGroup(NativeTest * pNativeTest, int testId) {
    ParamGroup *pParamGroup = NULL;
     if (pNativeTest != NULL) {
        pParamGroup = pNativeTest->getParamGroup(testId);
     }
    return pParamGroup;
}

ParamBase * helperGetParamBase(NativeTest *pNativeTest, int testId, int paramIndex) {
    ParamGroup * pParamGroup = helperGetParamGroup(pNativeTest, testId);
    if (pParamGroup != NULL) {
        return pParamGroup->getParamByIndex(paramIndex);
    }
    return NULL;
}

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamType(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex) {
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    if (pBase != NULL) {
        return pBase->getType();
    }
    return -1;
}

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamHoldType(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex) {
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    if (pBase != NULL) {
        return pBase->getHoldType();
    }
    return -1;
}

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamListSize(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex) {
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    if (pBase != NULL) {
        return pBase->getListSize();
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamListCurrentIndex(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex) {
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    if (pBase != NULL) {
        return pBase->getListCurrentIndex();
    }
    return -1;
}

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamListDefaultIndex(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex){
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    if (pBase != NULL) {
        return pBase->getListDefaultIndex();
    }
    return -1;
}

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1setParamListCurrentIndex(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex,
        jint index) {
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    if (pBase != NULL) {
        pBase->setListCurrentIndex(index);
        return NATIVETEST_SUCCESS;
    }
    return NATIVETEST_ERROR;
}

JNIEXPORT jstring JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamListNameFromIndex(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex,
        jint index){
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    if (pBase != NULL) {
        int type = pBase->getType();
        std::string name;
        switch (type) {
            case ParamBase::PARAM_INTEGER: {
                ParamInteger * pInt = static_cast<ParamInteger*>(pBase);
                if (pInt != NULL) {
                    name = pInt->getParamNameFromList(index);
                }
            }
            break;
            case ParamBase::PARAM_FLOAT: {
                ParamFloat * pFloat = static_cast<ParamFloat*>(pBase);
                if (pFloat != NULL) {
                    name = pFloat->getParamNameFromList(index);
                }
            }
            break;
        }
        return env->NewStringUTF(name.c_str());
    }
    return NULL;
}

JNIEXPORT jstring JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamName(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex) {
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    if (pBase != NULL) {
        std::string name = pBase->getName();
        return env->NewStringUTF(name.c_str());
    }
    return NULL;
}

JNIEXPORT jstring JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getparamDesc(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex) {
     ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
     if (pBase != NULL) {
         std::string desc = pBase->getDescription();
         return env->NewStringUTF(desc.c_str());
     }
     return NULL;
 }

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1resetParamValue(
         JNIEnv* env,
         jobject obj __unused,
         jlong nativeTest,
         jint testId,
         jint paramIndex) {
     ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
     if (pBase != NULL) {
        pBase->setDefaultValue();
        return NATIVETEST_SUCCESS;
     }
     return NATIVETEST_ERROR;
 }

 JNIEXPORT jint JNICALL
 Java_com_sonodroid_synthmark_AppObject_native_1getParamIntMin(
         JNIEnv* env,
         jobject obj __unused,
         jlong nativeTest,
         jint testId,
         jint paramIndex) {
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    ParamInteger * pInt = static_cast<ParamInteger*>(pBase);
    if (pInt != NULL) {
        return pInt->getMin();
    }
    return 0;
}

 JNIEXPORT jint JNICALL
 Java_com_sonodroid_synthmark_AppObject_native_1getParamIntMax(
         JNIEnv* env,
         jobject obj __unused,
         jlong nativeTest,
         jint testId,
         jint paramIndex) {
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    ParamInteger * pInt = static_cast<ParamInteger*>(pBase);
    if (pInt != NULL) {
       return pInt->getMax();
    }
    return 0;
}

 JNIEXPORT jint JNICALL
 Java_com_sonodroid_synthmark_AppObject_native_1getParamIntValue(
         JNIEnv* env,
         jobject obj __unused,
         jlong nativeTest,
         jint testId,
         jint paramIndex) {
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    ParamInteger * pInt = static_cast<ParamInteger*>(pBase);
    if (pInt != NULL) {
      return pInt->getValue();
    }
    return 0;
}

 JNIEXPORT jint JNICALL
 Java_com_sonodroid_synthmark_AppObject_native_1getParamIntDefault(
         JNIEnv* env,
         jobject obj __unused,
         jlong nativeTest,
         jint testId,
         jint paramIndex) {
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    ParamInteger * pInt = static_cast<ParamInteger*>(pBase);
    if (pInt != NULL) {
      return pInt->getDefaultValue();
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1setParamIntValue(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex,
        jint value) {
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    ParamInteger * pInt = static_cast<ParamInteger*>(pBase);
    if (pInt != NULL) {
        pInt->setValue(value);
        return NATIVETEST_SUCCESS;
    }
    return NATIVETEST_ERROR;
}


JNIEXPORT jfloat JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamFloatMin(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex) {
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    ParamFloat * pFloat = static_cast<ParamFloat*>(pBase);
    if (pFloat != NULL) {
        return pFloat->getMin();
    }
    return 0;
}

JNIEXPORT jfloat JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamFloatMax(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex) {
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    ParamFloat * pFloat = static_cast<ParamFloat*>(pBase);
    if (pFloat != NULL) {
        return pFloat->getMax();
    }
    return 0;
}

JNIEXPORT jfloat JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamFloatValue(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex) {
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    ParamFloat * pFloat = static_cast<ParamFloat*>(pBase);
    if (pFloat != NULL) {
        return pFloat->getValue();
    }
    return 0;
}

JNIEXPORT jfloat JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1getParamFloatDefault(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex) {
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    ParamFloat * pFloat = static_cast<ParamFloat*>(pBase);
    if (pFloat != NULL) {
        return pFloat->getDefaultValue();
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_sonodroid_synthmark_AppObject_native_1setParamFloatValue(
        JNIEnv* env,
        jobject obj __unused,
        jlong nativeTest,
        jint testId,
        jint paramIndex,
        jfloat value) {
    ParamBase * pBase = helperGetParamBase((NativeTest*) (size_t) nativeTest, testId, paramIndex);
    ParamFloat * pFloat = static_cast<ParamFloat*>(pBase);
    if (pFloat != NULL) {
        pFloat->setValue(value);
        return NATIVETEST_SUCCESS;
    }
    return NATIVETEST_ERROR;
}
