#
# RockChip Camera HAL 
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES +=\
	source/dct_assert.c\
	source/hashtable.c\
	source/list.c\
	source/queue.c\
	source/slist.c\
	source/trace.c
  

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include\


LOCAL_CFLAGS += -Wno-error=unused-function -Wno-array-bounds
LOCAL_CFLAGS += -DLINUX  -DMIPI_USE_CAMERIC -DHAL_MOCKUP -DCAM_ENGINE_DRAW_DOM_ONLY -D_FILE_OFFSET_BITS=64 -DHAS_STDINT_H -DENABLE_ASSERT
ifeq (1,$(strip $(shell expr $(PLATFORM_VERSION) \>= 5.0)))
LOCAL_CFLAGS += -DANDROID_5_X
endif
#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE:= libisp_ebase

LOCAL_MODULE_TAGS:= optional
include $(BUILD_STATIC_LIBRARY)
