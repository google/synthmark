LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := tests
LOCAL_C_INCLUDES := \
    vendor/box/user/philburk/apps/SynthMark/source
LOCAL_SRC_FILES:= ../apps/sm_hs_measure50_app.cpp
LOCAL_SHARED_LIBRARIES := libcutils libutils
LOCAL_MODULE := sm_hs_measure50_app
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := tests
LOCAL_C_INCLUDES := \
    vendor/box/user/philburk/apps/SynthMark/source
LOCAL_SRC_FILES:= ../apps/sm_hs_latency.cpp
LOCAL_SHARED_LIBRARIES := libcutils libutils
LOCAL_MODULE := sm_hs_latency
include $(BUILD_EXECUTABLE)
