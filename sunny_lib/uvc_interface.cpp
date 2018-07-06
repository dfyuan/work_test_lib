#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <utils/Log.h>

#include "typedef.h"
#include "uvc_ctrl.h"
#include "uvc_stream.h"
#include "log_tag.h"

int uvc_init(video_hdl* videohdl, imu_hdl* imuhdl)
{
	int ret = 0;

	ret = uvcctrl_init(videohdl, imuhdl);
	if (ret < 0) {
		ALOGD("uvcctrl init failed! \n");
		return -1;
	}

	ret = uvcstream_init();
	if (ret < 0) {
		ALOGD("uvc stream init failed! \n");
		return -1;
	}
	return 0;
}
void uvc_uninit(void)
{
	uvcstream_uninit();
	uvcctrl_uninit();
}
