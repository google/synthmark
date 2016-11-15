LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := tests
LOCAL_C_INCLUDES := \
    ../source/
LOCAL_SRC_FILES:= ../../apps/synthmark.cpp
LOCAL_CFLAGS += -g -std=c++11 -Ofast
#LOCAL_SHARED_LIBRARIES := libcutils libutils
LOCAL_MODULE := synthmark
include $(BUILD_EXECUTABLE)
