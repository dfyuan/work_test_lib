LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	camera_ctrl.cpp	\
	imu_mpu6500.cpp	\
	sensors_interface.cpp \
	uvc_ctrl.cpp \
	uvc_stream.cpp \
	uvc_interface.cpp \
	libsfifo/sfifo.cpp \
	libmsg/msg_util.cpp \

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../hardware/rockchip/sensor/mpu_vr/libsensors/ \
	$(LOCAL_PATH)/../../hardware/rockchip/sensor/mpu_vr/libsensors/software/core/mllite/ \
	$(LOCAL_PATH)/../../hardware/rockchip/sensor/mpu_vr/libsensors/software/core/mllite/linux/ \
	$(LOCAL_PATH)/../../hardware/rockchip/sensor/mpu_vr/libsensors/software/core/mpl/ \
	$(LOCAL_PATH)/../../hardware/rockchip/sensor/mpu_vr/libsensors/software/core/driver/include/ \
	$(LOCAL_PATH)/../../hardware/rockchip/sensor/mpu_vr/libsensors/software/core/driver/include/linux/ \
	$(LOCAL_PATH)/../../hardware/rockchip/sensor/mpu_vr/libsensors/ \
	$(LOCAL_PATH)/../../bionic/libc \
	$(LOCAL_PATH)/../../frameworks/base/include/ui \
	$(LOCAL_PATH)/../../frameworks/base/include/surfaceflinger \
	$(LOCAL_PATH)/../../frameworks/base/include/camera \
	$(LOCAL_PATH)/../../frameworks/base/include/media \
	$(PV_INCLUDES)	\

LOCAL_SHARED_LIBRARIES:= \
	libinvensense_hal \
	libdl \
	libui \
	libutils \
	libcutils \
	liblog \
	libmllite \
	libbinder \
	libmedia \
	libgui \
	libcamera_client \

LOCAL_CFLAGS := -fno-short-enums -DCOPY_IMAGE_BUFFER
LOCAL_CFLAGS += -DLINUX  -DMIPI_USE_CAMERIC -DHAL_MOCKUP -DCAM_ENGINE_DRAW_DOM_ONLY -D_FILE_OFFSET_BITS=64 -DHAS_STDINT_H
LOCAL_CFLAGS += -DANDROID_JELLYBEAN
LOCAL_MODULE := libsunny_sensor

LOCAL_MODULE_TAGS:= optional
include $(BUILD_SHARED_LIBRARY)
