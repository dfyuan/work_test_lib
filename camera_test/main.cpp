#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <semaphore.h>
#include <cutils/properties.h>
#include <utils/Log.h>

#include "device_mode.h"
#include "typedef.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#include "sensors_interface.h"
#define LOG_TAG  "slam_demo"

static long get_tick_count()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}
static void test_video_fps(void)
{
	static int frames_num = 0;
	static long start_time = 1;

	if (start_time == 1) {
		start_time = get_tick_count();
	}

	frames_num++;

	if (frames_num % 60 == 0) {
		double cur_framerate = 0;
		cur_framerate = (double)frames_num * 1000 / (get_tick_count() - start_time);
		start_time = get_tick_count();
		frames_num = 0;
		printf("VIDEO Frame Rate = %f\n", cur_framerate);
	}
}

static void test_imu_fps(void)
{
	static int frames_num = 0;
	static long start_time = 1;

	if (start_time == 1) {
		start_time = get_tick_count();
	}

	frames_num++;

	if (frames_num % 200 == 0) {
		double cur_framerate = 0;
		cur_framerate = (double)frames_num * 1000 / (get_tick_count() - start_time);
		start_time = get_tick_count();
		frames_num = 0;
		ALOGD("IMU Frame Rate = %f\n", cur_framerate);
	}
}


static unsigned int slam_get_videobuffer(unsigned char *buf, unsigned int len, unsigned char *r_buf, unsigned int r_len)
{
	static uint32_t old_lframe_count = 0, new_lframe_count = 0;
	static uint32_t total_frame_count = 0;

	if (buf == NULL || len == 0)
		return -1;

	S_MetaData* frame_header;
	frame_header = (S_MetaData*)buf;
	if (frame_header->cameraNum == 0) {
		total_frame_count++;
		new_lframe_count = frame_header->frameCount;
		if ((new_lframe_count - old_lframe_count) != 1) {
			ALOGD(" Framecount: %5d;new_lframe_count: %d;old_lframe_count:%d\n",
			       total_frame_count,
			       new_lframe_count,
			       old_lframe_count);
		}
		old_lframe_count = new_lframe_count;
	}
	test_video_fps();
	return 0;
}

extern unsigned int write_video_buffer(unsigned char* pBuffer, unsigned int nBufferLen);

enum {
	CAMERA_A,
	CAMERA_B,
	CAMERA_C,
	CAMERA_D,
};

enum {
	YUV_8 = 8,
	RAW_10 = 10,
	RAW_12 = 12,
	RAW_16 = 16,
};

enum {
	BAYER_DEF = 0,
	BAYER_GBRG = 0,
	BAYER_RGGB = 1,
	BAYER_BGGR = 2,
	BAYER_GRBG = 3,
	YUV422 = 4,
	YUV420_I420 = 5,
	MONO_8BIT = 6,
	YV12 = 7,
	NV12 = 8,
};

S_MetaData	 MetaData_CamB = {
	0, 0, 0, 0,
	CAMERA_C,
	1280,
	720,
	YUV_8,
	YUV420_I420,
	0,
	0,
	0,
	HEAD_LEN,
};

#define IMAGE_BUF_LEN 1280*720*3/2
unsigned char image_buf[IMAGE_BUF_LEN + HEAD_LEN]={0};
unsigned int sensor_mode1_handle(unsigned char *l_buf, unsigned int l_len, unsigned char *r_buf, unsigned int r_len)
{
	static long int count = 0;
	unsigned char buf[(640*480*3/2+64) * 3];
	//test_video_fps();
	//memcpy(buf, l_buf, l_len);
	//memcpy(buf + l_len, r_buf + 64, r_len - 64);
	//memcpy(buf + l_len + r_len - 64, r_buf + 64, r_len - 64);

	if (count%2 == 0) {
		write_video_buffer(image_buf, IMAGE_BUF_LEN + HEAD_LEN);
	}
	count++;

	return 0;
}

static int slam_get_imubuffer(struct imu_data *data)
{
	if (data == NULL)
		return -1;
	test_imu_fps();
	//TBD
	//memcpy();
	return 0;
}

extern void *move_test_thread(void *arg);

int main(void)
{
	int ret = -1;
	video_hdl videohdl;
	imu_hdl imuhdl;

	videohdl.callback = sensor_mode1_handle;
	imuhdl.callback = slam_get_imubuffer;

	#define IMAGE_TEST_PIC "/system/bin/test"
	memcpy(image_buf, &MetaData_CamB, sizeof(MetaData_CamB));
	int fd = open(IMAGE_TEST_PIC, O_RDONLY);
	if (fd < 0) {
		printf("open file error:%s\n", IMAGE_TEST_PIC);
		return -1;
	}
	read(fd, image_buf + HEAD_LEN, IMAGE_BUF_LEN);
	close(fd);

	static pthread_t move_test_threadid;
	ret = pthread_create(&move_test_threadid, NULL, move_test_thread, NULL);

	ret = Sensors_Init(&videohdl, &imuhdl);
	if (ret < 0) {
		ALOGD("Sensors Init Failed \n");
		return -1;
	}

	while (1) {
		if	(get_sensors_mode() == DISK_MODE)
			break;
		sleep(1);
	}

	Sensors_Uninit();
	return 0;
}
