LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

LOCAL_SRC_FILES := main.cpp \
				   libmove/move_utils.cpp \
				   libmove/tty_utils.cpp \

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include 

LOCAL_SHARED_LIBRARIES := libsunny_sensor liblog 

LOCAL_CFLAGS += -Wno-multichar

LOCAL_MODULE := slam_demo

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
