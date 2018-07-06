#
# RockChip Camera HAL 
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
	source/bufsync_ctrl.c\
	source/bufsync_ctrl_api.c\
	source/bufsync_ctrl_cb.c\


LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/include_priv\
	$(LOCAL_PATH)/../include/

LOCAL_CFLAGS := -Wall -Wextra -std=c99   -Wformat-nonliteral -g -O0 -DDEBUG -pedantic
LOCAL_CFLAGS += -DLINUX  -DMIPI_USE_CAMERIC -DHAL_MOCKUP -DCAM_ENGINE_DRAW_DOM_ONLY -D_FILE_OFFSET_BITS=64 -DHAS_STDINT_H
LOCAL_STATIC_LIBRARIES := libisp_ebase libisp_oslayer libisp_common libisp_hal libisp_bufferpool 

#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE:= libisp_bufsync_ctrl

LOCAL_MODULE_TAGS:= optional
include $(BUILD_STATIC_LIBRARY)
