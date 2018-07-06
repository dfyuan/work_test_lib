#
# RockChip Camera HAL 
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES +=\
	source/oslayer_generic.c\
	source/oslayer_linux.c\


LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/include_priv\
	$(LOCAL_PATH)/../include/

LOCAL_CFLAGS += -Wall -Wextra -std=c99   -Wformat-nonliteral -DLINUX -g -O0 -DDEBUG -pedantic -Wno-long-long
LOCAL_CFLAGS += -DLINUX  -DMIPI_USE_CAMERIC -DHAL_MOCKUP -DCAM_ENGINE_DRAW_DOM_ONLY -D_FILE_OFFSET_BITS=64 -DHAS_STDINT_H
LOCAL_SHARED_LIBRARIES := \
		libutils \
    libcutils \
    
LOCAL_LDLIBS +=-lc -lpthread
#LOCAL_LDFLAGS:=-lc -lpthread

#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE:= libisp_oslayer

LOCAL_MODULE_TAGS:= optional
include $(BUILD_STATIC_LIBRARY)
