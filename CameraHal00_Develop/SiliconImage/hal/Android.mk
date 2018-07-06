#
# RockChip Camera HAL 
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
	source/hal_mockup.c\
	source/cameraIonMgr.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/include_priv\
	$(LOCAL_PATH)/../include/ \

LOCAL_CFLAGS := -Wall -Wextra -std=c99   -Wformat-nonliteral -g -O0 -DDEBUG -pedantic 
LOCAL_CFLAGS += -DLINUX  -DMIPI_USE_CAMERIC -DHAL_MOCKUP -DCAM_ENGINE_DRAW_DOM_ONLY -D_FILE_OFFSET_BITS=64 -DHAS_STDINT_H

ifeq (1,$(strip $(shell expr $(PLATFORM_VERSION) \>= 5.0)))
LOCAL_C_INCLUDES += \
   system/core/libion/include/ion \
   system/core/libion/kernel-headers/linux \
   $(LOCAL_PATH)/../../CameraHal5.x\

LOCAL_CFLAGS += -DANDROID_5_X
else
LOCAL_C_INCLUDES += system/core/include/ion
endif

#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE:= libisp_hal
#LOCAL_STATIC_LIBRARIES := libisp_oslayer
LOCAL_SHARED_LIBRARIES := libion
LOCAL_WHOLE_STATIC_LIBRARIES := libisp_oslayer
full_path := $(shell pwd)
#LOCAL_LDFLAGS := $(full_path)/out/target/product/rk30sdk/obj/STATIC_LIBRARIES/libisp_oslayer_intermediates/libisp_oslayer.a
LOCAL_MODULE_TAGS:= optional
#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_STATIC_LIBRARY)
