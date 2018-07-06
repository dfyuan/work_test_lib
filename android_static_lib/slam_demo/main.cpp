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

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


typedef unsigned int (*video_cb)(unsigned char *buf, unsigned int len);
typedef int (*imu_cb)(struct imu_data* data);
typedef struct tagvideo_hdl {
	video_cb callback;
} video_hdl;
typedef struct tagimu_hdl {
	imu_cb callback;
} imu_hdl;

#include "sensors_interface.h"
#define LOG_TAG  "slam_demo"

typedef struct tagS_MetaData {
	long int verison[4];
	long int cameraNum;
	long int width;
	long int height;
	long int bpp;
	long int dataFormat;
	long int frameCount;
	long int tv_sec;
	long int tv_usec;
	long int headerLength;
} S_MetaData;


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

	if (frames_num % 120 == 0) {
		double cur_framerate = 0;
		cur_framerate = (double)frames_num * 1000 / (get_tick_count() - start_time);
		start_time = get_tick_count();
		frames_num = 0;
		ALOGD("VIDEO Frame Rate = %f\n", cur_framerate);
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


static unsigned int slam_get_videobuffer(unsigned char *buf, unsigned int len)
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

	videohdl.callback = slam_get_videobuffer;
	imuhdl.callback = slam_get_imubuffer;

//	static pthread_t move_test_threadid;
//	ret = pthread_create(&move_test_threadid, NULL, move_test_thread, NULL);

	ret = Sensors_Init(&videohdl, &imuhdl);
	if (ret < 0) {
		ALOGD("Sensors Init Failed \n");
		return -1;
	}

	while (1) {
		if	(is_sensors_ums_mode() == TRUE)
			break;
		sleep(1);
	}

	Sensors_Uninit();
	return 0;
}
