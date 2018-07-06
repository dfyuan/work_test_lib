#include <sys/ioctl.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utils/Log.h>
#include "typedef.h"
#include "uvc_ctrl.h"
#include "uvc_stream.h"
#include "libmsg/msg_util.h"
#include "log_tag.h"

#define VIDEO_CMD_DEVICE		"/dev/g_uvc_ctrl"
#define	CTRL_IOC_BASE			'C'
#define	CTRL_2A_SET_CMD			_IOW(CTRL_IOC_BASE,  2, struct imu_data *)
#define	CTRL_2A_GET_CMD			_IOR(CTRL_IOC_BASE,  3, struct uvc_cmd *)

#define DEVICE_PW_SV			0x00
#define DEVICE_PW_ON			0x01
#define VENDER_MODE_SET			0x02
#define FRAME_MJPEG_CHG_SET		0x07
#define VENDER_EXTERN_SET		0x27

static	int videoFd = -1;
static  pthread_t uvcgetctrl_tid;

static pthread_mutex_t imufp_lock_mutex;
static pthread_mutex_t videofp_lock_mutex;

video_cb g_fpvideo_cb;
imu_cb   g_fpimu_cb;
static video_hdl g_videohdl;
static imu_hdl   g_imuhdl;

int  imu_uvc_process(struct imu_data *data);

static inline int uvcctrl_cmd_get(struct uvc_cmd *cmd)
{
	return ioctl(videoFd, CTRL_2A_GET_CMD, cmd);
}

static int uvcctrl_dev_open(void)
{
	while (1) {
		if (access(VIDEO_CMD_DEVICE, R_OK | W_OK) == 0) {
			break;
		} else {
			usleep(500);
		}
	}

	videoFd = open(VIDEO_CMD_DEVICE, O_RDWR | O_NONBLOCK, S_IRUSR | S_IWUSR);
	if (videoFd < 0) {
		ALOGD("failed to open %s\n", VIDEO_CMD_DEVICE);
		return -1;
	}

	ALOGD("uvc ctrl device open ok!\n");
	return 0;
}

static void uvcctrl_dev_close(void)
{
	if (videoFd > 0) {
		close(videoFd);
		videoFd = -1;
	}
}

int uvcctrl_put_data(struct imu_data *data)
{
	return ioctl(videoFd, CTRL_2A_SET_CMD, data);
}

void imu_function_lock(void)
{
	pthread_mutex_lock(&imufp_lock_mutex);
}
void imu_function_unlock(void)
{
	pthread_mutex_unlock(&imufp_lock_mutex);
}
void video_function_lock(void)
{
	pthread_mutex_lock(&videofp_lock_mutex);
}
void video_function_unlock(void)
{
	pthread_mutex_unlock(&videofp_lock_mutex);
}


static void* uvcctrl_cmd_process(void* arg)
{
	struct uvc_cmd cmd;
	int ret = -1;
	void *tmp;
	tmp = arg;
	int quit_flag = 0;

	while (1) {

		ret = uvcctrl_cmd_get(&cmd);
		if (ret < 0) {
			ALOGD("uvcctrl_cmdget() error\n");
		}

		switch (cmd.cmd) {
		case DEVICE_PW_SV:
			ALOGD("DEVICE_PW_SV \n");
			uvcstream_off();
			break;

		case DEVICE_PW_ON:
			printf("DEVICE_PW_ON \n");
			uvcstream_on();
			break;

		case VENDER_MODE_SET:
			ALOGD("VENDER_MODE_SET \n");
			uvcstream_setmode(cmd.data);
			while (1) {
				if (uvcstream_getmode() == 0x06) {
					msg_send();
					msg_uninit();
					ALOGD("uvcstream_getmode quit_flag=1\n");
					quit_flag=1;
					break;
				}
			}
			break;

		case VENDER_EXTERN_SET:
			if (cmd.data == 0x04) {
				uvcstream_setmode(cmd.data);
				ALOGD("Video OffLine Mode \n");
				video_function_lock();
				g_fpvideo_cb = write_video_buffer;
				video_function_unlock();
				imu_function_lock();
				g_fpimu_cb = imu_uvc_process;
				imu_function_unlock();
			} else if (cmd.data == 0x05) {
				uvcstream_setmode(cmd.data);
				ALOGD("Video OnLine Mode \n");
				if (g_videohdl.callback != NULL){
					video_function_lock();
				if (g_videohdl.callback != NULL)
					g_fpvideo_cb = g_videohdl.callback;
					video_function_unlock();
				}
				if (g_imuhdl.callback != NULL){
					imu_function_lock();
					g_fpimu_cb = g_imuhdl.callback;
					imu_function_unlock();
					}
			}

			break;

		default :
			break;
		}

		if (quit_flag == 1) {
			ALOGD("uvc ctrl break\n");
			break;
		}

		usleep(10 * 1000);
	}

	return NULL;
}

int  imu_uvc_process(struct imu_data *data)
{
	int retval = -1;

	if (is_uvcstreamon() == TRUE || data != NULL) {
		retval = uvcctrl_put_data(data);
		if (retval < 0) {
			ALOGD("put imu to uvc_drv failed!\n");
		}
	}
	return retval;
}
int uvcctrl_init(video_hdl* videohdl, imu_hdl* imuhdl)
{
	int ret = -1;

	if (videohdl != NULL  &&  videohdl->callback)
		g_videohdl.callback = videohdl->callback;

	if (imuhdl != NULL  &&  imuhdl->callback)
		g_imuhdl.callback = imuhdl->callback;

	pthread_mutex_init(&imufp_lock_mutex, NULL);
	pthread_mutex_init(&videofp_lock_mutex, NULL);

	ret = uvcctrl_dev_open();
	if (ret < 0) {
		ALOGD("uvcctrl dev open failed!\n");
		return -1;
	}

	ret = msg_init();
	if (ret < 0) {
		ALOGD("msg socket creat failed \n");
		return -1;
	}

	ret = pthread_create(&uvcgetctrl_tid, NULL, uvcctrl_cmd_process, NULL);
	if (ret < 0) {
		ALOGD("Create uvc ctrl get cmd thread failed!\n");
	}
	return 	ret;
}

int uvcctrl_uninit(void)
{
	ALOGD("%s,%d \n", __func__, __LINE__);
	pthread_join(uvcgetctrl_tid, NULL);
	pthread_mutex_destroy(&imufp_lock_mutex);
	pthread_mutex_destroy(&videofp_lock_mutex);
	uvcctrl_dev_close();

	return 0;
}
