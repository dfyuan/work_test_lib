#ifndef _UVCCTRL__H_
#define _UVCCTRL__H_

/* unsigned quantities */
typedef unsigned long long Uint64;      ///< Unsigned 64-bit integer
typedef unsigned int Uint32;            ///< Unsigned 32-bit integer
typedef unsigned short Uint16;          ///< Unsigned 16-bit integer
typedef unsigned char Uint8;            ///< Unsigned  8-bit integer

/* signed quantities */
typedef long long Int64;               ///< Signed 64-bit integer
typedef int Int32;                     ///< Signed 32-bit integer
typedef short Int16;                   ///< Signed 16-bit integer
typedef char Int8;                     ///< Signed  8-bit integer


struct imu_data {
	float acl_x;
	float acl_y;
	float acl_z;
	float gyro_x;
	float gyro_y;
	float gyro_z;
	long timestamp;
};

#define SENSOR_MODE0 	1
#define SENSOR_MODE1	2
#define SENSOR_MODE2	3
#define DISK_MODE		4
#define BURNING_MODE	5



#define IMU_SIZE sizeof(struct imu_data)

struct uvc_frame_info {
	Uint16 FormatID;		// uvc driver Format descriptor id
	Uint16 FrameID;		//uvc driver frame descriptor id
	Uint16 CompQuality;	//shoude map to bit rate image compress
	Uint32 FrameInterval;	// shoude map to frame rate
	Uint16 Width;			// actually codec width
	Uint16 Height;		// actually codec Height
};

struct uvc_cmd {
	unsigned int cmd;
	unsigned int data;
	struct uvc_frame_info frame_info;
};

int uvcctrl_init(video_hdl* videohdl, imu_hdl* imuhdl);
int uvcctrl_uninit();
void imu_function_lock(void);
void imu_function_unlock(void);
void video_function_lock(void);
void video_function_unlock(void);

#endif

