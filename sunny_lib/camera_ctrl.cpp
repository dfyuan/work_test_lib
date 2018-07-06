#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>

#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>

#include <camera/Camera.h>
#include <camera/ICamera.h>
#include <media/mediarecorder.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <camera/CameraParameters.h>
#include <system/audio.h>
#include <system/camera.h>

#include <cutils/memory.h>
#include <utils/Log.h>

#include <sys/wait.h>

#include "typedef.h"
#include "camera_ctrl.h"
#include "imu_mpu6500.h"
#include "uvc_stream.h"
#include "libsfifo/sfifo.h"
#include "log_tag.h"

using namespace android;

sp<Camera> camera;
sp<Camera> camera0;
sp<Camera> camera1;

CameraParameters params0;
CameraParameters params1;
static int ret0 = -1, ret1 = -1;
static const String16 processName("sunny_test");
extern int VS_GetVideomode(void);

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

static void stopPreview0();
static void stopPreview1();

struct sfifo_des_s *rvideo_sfifo_handle;
struct sfifo_des_s *lvideo_sfifo_handle;

void CameraHandler::notify(int32_t msgType, int32_t ext1, int32_t ext2)
{
	int32_t tmpext1, tmpext2, tmpmsgType;
	tmpext1 = ext1;
	tmpext2 = ext2;
	tmpmsgType = msgType;
}

void CameraHandler::postData(int32_t msgType, const sp<IMemory>& dataPtr, camera_frame_metadata_t *metadata)
{
	camera_frame_metadata_t* tmp = metadata;
	int32_t tmpmsgType = msgType;
	static int framecount = 0;
	struct timeval *tm0 = NULL;

	struct sfifo_s *sfifo;

	S_MetaData   MetaData_CamA = {
		0, 0, 0, 0,
		CAMERA_A,
		IMAGE_WIDTH,
		IMAGE_HEIGHT,
		YUV_8,
		NV12,
		0,
		0,
		0,
		HEAD_LEN,
	};

	tm0 = (struct timeval*)((unsigned char *)dataPtr->pointer() + dataPtr->size() - 16);
	MetaData_CamA.tv_sec = tm0->tv_sec;
	MetaData_CamA.tv_usec = tm0->tv_usec;
	MetaData_CamA.frameCount = framecount;
	sfifo = sfifo_get_free_buf(lvideo_sfifo_handle);
	if (sfifo != NULL) {
		memcpy(sfifo->buffer, &MetaData_CamA, HEAD_LEN);
		memcpy(sfifo->buffer + HEAD_LEN, (unsigned char *)dataPtr->pointer(), 640 * 480);
		memcpy(sfifo->buffer + HEAD_LEN + 640 * 480, (unsigned char *)dataPtr->pointer() + 640 * 481, 640 * 480 / 2);
		sfifo_put_active_buf(sfifo, lvideo_sfifo_handle);
		framecount++;
	}
}

void CameraHandler::postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr)
{
	uint8_t *ptr = (uint8_t*) dataPtr->pointer();
	nsecs_t tmptimestamp = timestamp;
	int32_t tmpmsgType = msgType;
}

void CameraHandler1::notify(int32_t msgType, int32_t ext1, int32_t ext2)
{
	int32_t tmpext1, tmpext2, tmpmsgType;
	tmpext1 = ext1;
	tmpext2 = ext2;
	tmpmsgType = msgType;
}

void CameraHandler1::postData(int32_t msgType, const sp<IMemory>& dataPtr, camera_frame_metadata_t *metadata)
{
	camera_frame_metadata_t* tmp = metadata;
	int32_t tmpmsgType = msgType;
	struct sfifo_s *sfifo;
	static int framecount = 0;
	struct timeval *tm1 = NULL;
	S_MetaData   MetaData_CamB = {
		0, 0, 0, 0,
		CAMERA_C,
		IMAGE_WIDTH,
		IMAGE_HEIGHT,
		YUV_8,
		NV12,
		0,
		0,
		0,
		HEAD_LEN,
	};

	tm1 = (struct timeval*)((unsigned char *)dataPtr->pointer() + dataPtr->size() - 16);
	sfifo = sfifo_get_free_buf(rvideo_sfifo_handle);
	if (sfifo != NULL) {
		MetaData_CamB.tv_sec = tm1->tv_sec;
		MetaData_CamB.tv_usec = tm1->tv_usec;
		MetaData_CamB.frameCount = framecount;
		memcpy(sfifo->buffer, &MetaData_CamB, HEAD_LEN);
		memcpy(sfifo->buffer + HEAD_LEN, (unsigned char *)dataPtr->pointer(), 640 * 480);
		memcpy(sfifo->buffer + HEAD_LEN + 640 * 480, (unsigned char *)dataPtr->pointer() + 640 * 481, 640 * 480 / 2);
		sfifo_put_active_buf(sfifo, rvideo_sfifo_handle);
		framecount++;
	}
}

void CameraHandler1::postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr)
{
	uint8_t *ptr = (uint8_t*) dataPtr->pointer();
	nsecs_t tmptimestamp = timestamp;
	int32_t tmpmsgType = msgType;
}

static int openCamera0()
{
	ALOGD("%s ,%d \n", __func__, __LINE__);
	camera0 = Camera::connect(0, processName, Camera::USE_CALLING_UID);

	if (NULL == camera0.get()) {
		ALOGD("Unable to connect to Camera0Service\n");
		ALOGD("Retrying... \n");
		sleep(1);
		camera0 = Camera::connect(0, processName, Camera::USE_CALLING_UID);

		if (NULL == camera0.get()) {
			ALOGD("connect to Camera0 Giving up!! \n");
			return -1;
		}
	}

	params0 = camera0->getParameters();
	camera0->setParameters(params0.flatten());
	camera0->setListener(new CameraHandler());
	camera0->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_ENABLE_MASK);

	return 0;
}

static int openCamera1()
{
	ALOGD("%s ,%d \n", __func__, __LINE__);
	camera1 = Camera::connect(1, processName, Camera::USE_CALLING_UID);
	if (NULL == camera1.get()) {
		ALOGD("Unable to connect to Camera1Service\n");
		ALOGD("Retrying... \n");

		sleep(1);
		camera1 = Camera::connect(1, processName, Camera::USE_CALLING_UID);

		if (NULL == camera1.get()) {
			ALOGD("connect to Camera1 Giving up!! \n");
			return -1;
		}
	}

	params1 = camera1->getParameters();
	camera1->setParameters(params1.flatten());
	camera1->setListener(new CameraHandler1());
	camera1->setPreviewCallbackFlags(CAMERA_FRAME_CALLBACK_FLAG_ENABLE_MASK);

	return 0;
}

static int closeCamera0()
{
	if (NULL == camera0.get()) {
		ALOGD("invalid camera0 reference\n");
		return -1;
	}

	camera0->disconnect();
	camera0.clear();
	return 0;
}

static int closeCamera1()
{
	if (NULL == camera1.get()) {
		ALOGD("invalid camera1 reference\n");
		return -1;
	}

	camera1->disconnect();
	camera1.clear();
	return 0;
}

static int startPreview0()
{
	params0.setPreviewSize(640, 481);
	camera0->setParameters(params0.flatten());

	camera0->startPreview();
	return 0;
}

static int startPreview1()
{
	params1.setPreviewSize(640, 481);
	camera1->setParameters(params1.flatten());
	camera1->startPreview();
	return 0;
}

static void stopPreview0()
{
	camera0->stopPreview();
	closeCamera0();
}

static void stopPreview1()
{
	camera1->stopPreview();
	closeCamera1();
}

int camera_init(void)
{
	sp<ProcessState> proc(ProcessState::self());
	ProcessState::self()->startThreadPool();

	rvideo_sfifo_handle = sfifo_init(3, (640 * 480 * 3 / 2 + HEAD_LEN), 5);
	lvideo_sfifo_handle = sfifo_init(3, (640 * 480 * 3 / 2 + HEAD_LEN), 5);
	ret0 = openCamera0();
	if (ret0 < 0) {
		ALOGD("Camera0 initialization failed\n");
		return -1;
	}
	
	ret1 = openCamera1();
	if (ret1 < 0) {
		ALOGD("Camera1 initialization failed\n");
		if (ret0 == 0) {
			closeCamera0();
		}
		return -1;
	}


	if (ret0 == 0) {
		if (startPreview0() < 0) {
			ALOGD("Error while starting preview\n");
			if (ret1 == 0) {
				closeCamera1();
			}
			return -1;
		}
	}

	if (ret1 == 0) {
		if (startPreview1() < 0) {
			ALOGD("Error while starting2 preview\n");
			if (ret0 == 0) {
				closeCamera0();
			}

			if (ret1 == 0) {
				closeCamera1();
			}
			return -1;
		}
	}
	return 0;
}

void camera_uninit(void)
{
	ALOGD("%s,%d \n",__func__,__LINE__);

	if (ret0 == 0) {
		stopPreview0();
	}
	if (ret1 == 0) {
		stopPreview1();
	}
}
