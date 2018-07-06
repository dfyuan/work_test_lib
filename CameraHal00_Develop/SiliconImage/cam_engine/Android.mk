#
# RockChip Camera HAL 
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
	source/cam_engine.c\
	source/cam_engine_aaa_api.c\
	source/cam_engine_api.c\
	source/cam_engine_cb.c\
	source/cam_engine_cproc_api.c\
	source/cam_engine_drv.c\
	source/cam_engine_drv_api.c\
	source/cam_engine_imgeffects_api.c\
	source/cam_engine_isp_api.c\
	source/cam_engine_jpe_api.c\
	source/cam_engine_mi_api.c\
	source/cam_engine_modules.c\
	source/cam_engine_simp_api.c\
	source/cam_engine_subctrls.c


LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/include_priv\
	$(LOCAL_PATH)/../include/

LOCAL_CFLAGS := -Wall -Wextra -std=c99   -Wformat-nonliteral -g -O0 -DDEBUG -pedantic
LOCAL_CFLAGS += -DLINUX  -DMIPI_USE_CAMERIC -DHAL_MOCKUP -DCAM_ENGINE_DRAW_DOM_ONLY -D_FILE_OFFSET_BITS=64 -DHAS_STDINT_H
LOCAL_STATIC_LIBRARIES := libisp_ebase \
	libisp_oslayer \
	libisp_common \
	libisp_hal \
	libisp_cam_calibdb \
	libisp_cameric_reg_drv \
	libisp_cameric_drv\
	libisp_aaa_awb \
	libisp_aaa_aec \
	libisp_aaa_avs \
	libisp_aaa_adpcc \
	libisp_aaa_adpf \
	libisp_aaa_af \
	libisp_mipi_drv \
	libisp_mom_ctrl \
	libisp_mim_ctrl \
	libisp_bufferpool \
	libisp_bufsync_ctrl \
	libisp_isi 

LOCAL_WHOLE_STATIC_LIBRARIES := libisp_aaa_awb \
    libisp_aaa_aec \
    libisp_aaa_avs \
    libisp_aaa_adpcc \
    libisp_aaa_adpf \
    libisp_aaa_af \
    libisp_mipi_drv \
    libisp_cameric_drv \
	libisp_common \
	libisp_mim_ctrl \
	libisp_mom_ctrl \
	libisp_bufsync_ctrl

#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE:= libisp_cam_engine

LOCAL_MODULE_TAGS:= optional
#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_STATIC_LIBRARY)

