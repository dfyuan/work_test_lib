#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <utils/Log.h>

#include "typedef.h"
#include "imu_mpu6500.h"
#include "uvc_stream.h"
#include "uvc_interface.h"
#include "log_tag.h"

extern int camera_init(void);
extern void camera_uninit(void);

int Sensors_Init(video_hdl* videohdl, imu_hdl* imuhdl)
{
	int ret = -1;
	ret = camera_init();
	if (ret < 0) {
		ALOGD("camera init failed!\n");
		return -1;
	}

	ret = imu_init();
	if (ret < 0) {
		ALOGD("imu init failed!\n");
		return -1;
	}

	ret = uvc_init(videohdl, imuhdl);
	if (ret < 0) {
		ALOGD("uvc init failed!\n");
		return -1;
	}

	return ret;
}
bool is_sensors_ums_mode(void)
{
	return is_umsmode() ;
}
void Sensors_Uninit(void)
{
	camera_uninit();
	imu_uninit();
	uvc_uninit();
	ALOGD("Now begin to rmmod the drives \n");
}

