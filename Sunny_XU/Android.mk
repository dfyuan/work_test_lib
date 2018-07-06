LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    sd03c.cpp \
	AitXU.cpp \
	UvcXU.cpp

LOCAL_MODULE_PATH := $(LOCAL_PATH)/../lib
	
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/
	
LOCAL_CFLAGS += -Wno-multichar

#LOCAL_LDFLAGS := -Wl,-v 

LOCAL_MODULE:= libsd03c

LOCAL_MODULE_TAGS := debug

include $(BUILD_STATIC_LIBRARY)

