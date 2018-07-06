#ifndef __SENSORS_INTERFACE_H__
#define __SENSORS_INTERFACE_H__

int Sensors_Init(video_hdl*, imu_hdl*);
int get_sensors_mode(void);
int Sensors_Uninit(void);
#endif