LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := tests
LOCAL_C_INCLUDES := \
    ../source/
LOCAL_SRC_FILES:= \
    ../../apps/synthmark.cpp \
    ../../source/tools/AdpfWrapper.cpp \
    ../../source/tools/HostTools.cpp \
    ../../source/tools/SynthMarkCommand.cpp \
    ../../source/aaudio/AAudioHostThread.cpp
LOCAL_CFLAGS += -g -std=c++14 -Ofast -Wall -Werror
LOCAL_LDLIBS := -laaudio
LOCAL_MODULE := synthmark
include $(BUILD_EXECUTABLE)
