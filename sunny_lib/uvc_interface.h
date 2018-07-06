#ifndef __UVC_INTERFACE_H__
#define __UVC_INTERFACE_H__
#include "typedef.h"

int uvc_init(video_hdl* videohdl, imu_hdl* imuhdl);
void uvc_uninit(void);
#endif