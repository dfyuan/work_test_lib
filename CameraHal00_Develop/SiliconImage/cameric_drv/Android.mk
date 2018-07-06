#
# RockChip Camera HAL 
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
	source/cameric.c\
	source/cameric_cproc.c\
	source/cameric_dual_cropping.c\
	source/cameric_ie.c\
	source/cameric_isp.c\
	source/cameric_isp_afm.c\
	source/cameric_isp_awb.c\
	source/cameric_isp_bls.c\
	source/cameric_isp_cac.c\
	source/cameric_isp_cnr.c\
	source/cameric_isp_degamma.c\
	source/cameric_isp_dpcc.c\
	source/cameric_isp_dpf.c\
	source/cameric_isp_elawb.c\
	source/cameric_isp_exp.c\
	source/cameric_isp_flt.c\
	source/cameric_isp_hist.c\
	source/cameric_isp_is.c\
	source/cameric_isp_lsc.c\
	source/cameric_isp_vsm.c\
	source/cameric_isp_wdr.c\
	source/cameric_jpe.c\
	source/cameric_mipi.c\
	source/cameric_mi.c\
	source/cameric_scale.c\
	source/cameric_simp.c\
	source/cameric_isp_flash.c


LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include\
	$(LOCAL_PATH)/include_priv\
	$(LOCAL_PATH)/../include/

LOCAL_CFLAGS :=  -Wall -Wextra -std=c99   -Wformat-nonliteral -g -O0 -DDEBUG -pedantic
LOCAL_CFLAGS += -DLINUX  -DMIPI_USE_CAMERIC -DHAL_MOCKUP -DCAM_ENGINE_DRAW_DOM_ONLY -D_FILE_OFFSET_BITS=64 -DHAS_STDINT_H
LOCAL_STATIC_LIBRARIES := libisp_ebase libisp_oslayer libisp_common libisp_hal libisp_bufferpool libisp_cameric_reg_drv

#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE:= libisp_cameric_drv

LOCAL_MODULE_TAGS:= optional
include $(BUILD_STATIC_LIBRARY)
