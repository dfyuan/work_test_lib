LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
				calibdb.cpp\
				xmltags.cpp\

LOCAL_C_INCLUDES := \
				bionic\
				$(LOCAL_PATH)/../../include\
				$(LOCAL_PATH)/../include\
				$(LOCAL_PATH)/./calib_xml\
				external/tinyxml2

#has no "external/stlport" from Android 6.0 on                         
ifeq (1,$(strip $(shell expr $(PLATFORM_VERSION) \< 6.0)))
LOCAL_C_INCLUDES += external/stlport/stlport
endif


LOCAL_CPPFLAGS := -fuse-cxa-atexit -Wall -Wextra -std=c++0x   -Wformat-nonliteral -g -O0 -DDEBUG -pedantic -Wno-error=unused-function

LOCAL_CFLAGS += -DLINUX  -DMIPI_USE_CAMERIC -DHAL_MOCKUP -DCAM_ENGINE_DRAW_DOM_ONLY -D_FILE_OFFSET_BITS=64 -DHAS_STDINT_H

LOCAL_STATIC_LIBRARIES := libtinyxml2 libisp_cam_calibdb libisp_ebase libisp_common
LOCAL_SHARED_LIBRARIES := libutils libcutils

LOCAL_MODULE:= libisp_calibdb

LOCAL_MODULE_TAGS:= optional
include $(BUILD_STATIC_LIBRARY)

