LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
				cam_engine_interface.cpp\
				camdevice.cpp\
				dom_ctrl_interface.cpp\
				exa_ctrl_interface.cpp\
				halholder.cpp\
				impexinfo.cpp\
				mapcaps.cpp\
				readpgmraw.cpp\
				som_ctrl_interface.cpp\
				vom_ctrl_interface.cpp\
				dom_ctrl_display_api_mockup.c\

ifeq (1,$(strip $(shell expr $(PLATFORM_VERSION) \>= 5.0)))
LOCAL_C_INCLUDES := \
				bionic\
				$(LOCAL_PATH)/../../include\
				$(LOCAL_PATH)/../include\
				external/tinyxml2\
				$(LOCAL_PATH)/../../../CameraHal5.x\
				$(LOCAL_PATH)/../../cam_calibdb/include\
				$(LOCAL_PATH)/../../cam_calibdb/include_priv\

#has no "external/stlport" from Android 6.0 on				
ifeq (1,$(strip $(shell expr $(PLATFORM_VERSION) \< 6.0)))
LOCAL_C_INCLUDES += external/stlport/stlport
endif

else
LOCAL_C_INCLUDES := \
				bionic\
				external/stlport/stlport\
				$(LOCAL_PATH)/../../include\
				$(LOCAL_PATH)/../include\
				external/tinyxml2\
				$(LOCAL_PATH)/../../../CameraHal4.4\
				
endif

LOCAL_CPPFLAGS :=  -Wall -Wextra -std=c++0x   -Wformat-nonliteral -g -O0 -DDEBUG -pedantic -Wno-error=unused-function
#LOCAL_CFLAGS := -Wall -Wextra     -Wno-variadic-macros  -Wformat-nonliteral -g -O0 -DDEBUG 
LOCAL_CFLAGS += -DLINUX  -DMIPI_USE_CAMERIC -DHAL_MOCKUP -DCAM_ENGINE_DRAW_DOM_ONLY -DHAS_STDINT_H

#define this in Android 6.0 will cause file "fstream” compile error, why ? 
ifeq (1,$(strip $(shell expr $(PLATFORM_VERSION) \< 6.0)))
LOCAL_CFLAGS += -D_FILE_OFFSET_BITS=64
endif

LOCAL_STATIC_LIBRARIES := libisp_calibdb libisp_ebase libisp_oslayer libisp_common libisp_hal libisp_isi\
							libisp_vom_ctrl libisp_som_ctrl libisp_dom_ctrl libisp_exa_ctrl\
							libisp_cam_engine  libisp_version libisp_cameric_reg_drv libisp_cam_calibdb \
							libtinyxml2
LOCAL_SHARED_LIBRARIES := libbinder libutils libcutils libdl libion

ifeq (1,$(strip $(shell expr $(PLATFORM_VERSION) \< 6.0))) 
	LOCAL_SHARED_LIBRARIES += libstlport
endif

#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

ifneq (1,$(strip $(shell expr $(PLATFORM_VERSION) \>= 5.0)))
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
else
ifneq ($(strip $(TARGET_2ND_ARCH)), )
LOCAL_MULTILIB := both
endif
LOCAL_MODULE_RELATIVE_PATH := hw
endif

LOCAL_MODULE:=libisp_silicomimageisp_api

LOCAL_MODULE_TAGS:= optional
include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)

