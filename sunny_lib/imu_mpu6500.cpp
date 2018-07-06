/*
* Copyright (C) 2012 Invensense, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#define FUNC_LOG LOGV("%s", __PRETTY_FUNCTION__)

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>

#include <linux/input.h>

#include <utils/Atomic.h>
#include <utils/Log.h>

#include "sensors.h"
#include "MPLSensor.h"
#include "hal_outputs.h"

/*****************************************************************************/
/* The SENSORS Module */

#define VERSION     "version: 1.18"
#define ENABLE_LIGHT_SENSOR       0
#define ENABLE_PROXIMITY_SENSOR   0
#include "LightSensor.h"
#include "ProximitySensor.h"
#include "imu_mpu6500.h"
#include "typedef.h"
#include "uvc_ctrl.h"
#include "uvc_stream.h"


#ifdef ENABLE_DMP_SCREEN_AUTO_ROTATION
#define LOCAL_SENSORS (MPLSensor::NumSensors + ENABLE_LIGHT_SENSOR + ENABLE_PROXIMITY_SENSOR + 1)
#else
#define LOCAL_SENSORS (MPLSensor::NumSensors + ENABLE_LIGHT_SENSOR + ENABLE_PROXIMITY_SENSOR)
#endif

#define SENSORS_LIGHT_HANDLE            (ID_L)
#define SENSORS_PROXIMITY_HANDLE        (ID_P)



static struct sensor_t sSensorList[LOCAL_SENSORS];
static int sensors = (sizeof(sSensorList) / sizeof(sensor_t));
extern imu_cb g_fpimu_cb;

typedef int (*CallBackWriteIMUDATA)(struct imu_data * imudata);

static int open_sensors(const struct hw_module_t* module, const char* id, struct hw_device_t** device)
{
	return 0;
}
static int sensors__get_sensors_list(struct sensors_module_t* module,
                                     struct sensor_t const** list)
{
	*list = sSensorList;
	return sensors;
}

static struct hw_module_methods_t sensors_module_methods = {
open:
	open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
common:
	{
tag:
		HARDWARE_MODULE_TAG,
		version_major: 1,
		version_minor: 0,
id:
		SENSORS_HARDWARE_MODULE_ID,
name: "Invensense module"
		,
author: "Invensense Inc."
		,
methods:
		&sensors_module_methods,
dso:
		NULL,
reserved:
		{0}
	},
get_sensors_list:
	sensors__get_sensors_list,
};

struct sensors_poll_context_t {
	sensors_poll_device_1_t device; // must be first

	sensors_poll_context_t();
	~sensors_poll_context_t();
	int activate(int handle, int enabled);
	int setDelay(int handle, int64_t ns);
	int pollEvents(sensors_event_t* data, int count);
	int batch(int handle, int flags, int64_t period_ns, int64_t timeout);



private:
	enum {
		mpl = 0,
		// compass,
		// dmpOrient,
		// dmpSign,
		// light,
		// proximity,
		numSensorDrivers,   // wake pipe goes here
		numFds,
	};

	struct pollfd mPollFds[numSensorDrivers];
	SensorBase *mSensor;
	CompassSensor *mCompassSensor;
};

/******************************************************************************/
sensors_poll_context_t::sensors_poll_context_t()
{
	VFUNC_LOG;

	mCompassSensor = new CompassSensor();
	MPLSensor *mplSensor = new MPLSensor(mCompassSensor);

	/* For Vendor-defined Accel Calibration File Load
	 * Use the Following Constructor and Pass Your Load Cal File Function
	 *
	* MPLSensor *mplSensor = new MPLSensor(mCompassSensor, AccelLoadConfig);
	*/

	// setup the callback object for handing mpl callbacks
	setCallbackObject(mplSensor);

	sensors = mplSensor->populateSensorList(sSensorList, sizeof(sSensorList));
#if 0
	memset(&sSensorList[LOCAL_SENSORS - 2], 0, sizeof(sSensorList[0]));
	sSensorList[LOCAL_SENSORS - 2].name       = "Light sensor";
	sSensorList[LOCAL_SENSORS - 2].vendor     = "Invensense";
	sSensorList[LOCAL_SENSORS - 2].version    = 1;
	sSensorList[LOCAL_SENSORS - 2].handle     = SENSORS_LIGHT_HANDLE;
	sSensorList[LOCAL_SENSORS - 2].type       = SENSOR_TYPE_LIGHT;
	sSensorList[LOCAL_SENSORS - 2].maxRange   = 10240.0f;
	sSensorList[LOCAL_SENSORS - 2].resolution = 1.0f;
	sSensorList[LOCAL_SENSORS - 2].power      = 0.5f;
	sSensorList[LOCAL_SENSORS - 2].minDelay   = 20000;
	sensors += 1;

	memset(&sSensorList[LOCAL_SENSORS - 1], 0, sizeof(sSensorList[0]));
	sSensorList[LOCAL_SENSORS - 1].name		= "Proximity sensor";
	sSensorList[LOCAL_SENSORS - 1].vendor 	= "Invensense";
	sSensorList[LOCAL_SENSORS - 1].version	= 1;
	sSensorList[LOCAL_SENSORS - 1].handle 	= SENSORS_PROXIMITY_HANDLE;
	sSensorList[LOCAL_SENSORS - 1].type		= SENSOR_TYPE_PROXIMITY;
	sSensorList[LOCAL_SENSORS - 1].maxRange	= 9.0f;
	sSensorList[LOCAL_SENSORS - 1].power		= 0.5f;
	sSensorList[LOCAL_SENSORS - 1].minDelay	= 10000;
	sensors += 1;
#endif
	mSensor = mplSensor;
	mPollFds[mpl].fd = mSensor->getFd();
	//mPollFds[mpl].fd = -1;//mtcai
	mPollFds[mpl].events = POLLIN;
	mPollFds[mpl].revents = 0;
#if 0
#if 1
	//int compass_fd = open("/dev/compass", O_RDONLY);
	mPollFds[compass].fd = mCompassSensor->getFd();
	LOGD("compass poll fd=%d", mPollFds[compass].fd);
	mPollFds[compass].events = POLLIN;
	mPollFds[compass].revents = 0;
#else
	mPollFds[compass].fd = -1;
	mPollFds[compass].events = POLLIN;
	mPollFds[compass].revents = 0;
#endif

	mPollFds[dmpOrient].fd = ((MPLSensor*) mSensor)->getDmpOrientFd();
	mPollFds[dmpOrient].events = POLLPRI;
	mPollFds[dmpOrient].revents = 0;

	mPollFds[dmpSign].fd = ((MPLSensor*) mSensor)->getDmpSignificantMotionFd();
	mPollFds[dmpSign].events = POLLPRI;
	mPollFds[dmpSign].revents = 0;
#endif
}

sensors_poll_context_t::~sensors_poll_context_t()
{
	FUNC_LOG;
	delete mSensor;
	delete mCompassSensor;
}

int sensors_poll_context_t::activate(int handle, int enabled)
{
	FUNC_LOG;

	return mSensor->enable(handle, enabled);
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns)
{
	FUNC_LOG;

	return mSensor->setDelay(handle, ns);
}

#define NSEC_PER_SEC            1000000000

static inline int64_t timespec_to_ns(const struct timespec *ts)
{
	return ((int64_t) ts->tv_sec * NSEC_PER_SEC) + ts->tv_nsec;
}

static int64_t get_time_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return timespec_to_ns(&ts);
}

#include "sensor_params.h"

static int64_t tm_min = 0;
static int64_t tm_max = 0;
static int64_t tm_sum = 0;
static int64_t tm_last_print = 0;
static int64_t tm_count = 0;

#define SENSOR_KEEP_ALIVE       1

#if SENSOR_KEEP_ALIVE
static int sensor_activate[32];
static int sensor_delay[32];
static int64_t sensor_prev_time[32];
#endif

/*
    0 - 0000 - no debug
    1 - 0001 - gyro data
    2 - 0010 - accl data
    4 - 0100 - mag data
    8 - 1000 - raw gyro data with uncalib and bias
 */
static int debug_lvl = 0;

/* print sensor data latency */
static int debug_time = 0;
#include <cutils/properties.h>


static pthread_t Sensor_thread_id;
static sensors_poll_context_t *g_ctx = NULL;
static int g_sensor_exit = 1;

int sensors_poll_context_t::pollEvents(sensors_event_t *data, int count)
{
	//  VHANDLER_LOG;

	int nbEvents = 0;
	int nb, polltimeout = 5;
	int i = 0;

	// look for new events
	nb = poll(mPollFds, numSensorDrivers, polltimeout);

	if (nb > 0) {
		for (int i = 0; count && i < numSensorDrivers; i++) {
			if (mPollFds[i].revents & (POLLIN | POLLPRI)) {
				nb = 0;
				if (i == mpl) {
					((MPLSensor*) mSensor)->buildMpuEvent();
					mPollFds[i].revents = 0;

				}
#if 0
				else if (i == compass) {
					((MPLSensor*) mSensor)->buildCompassEvent();
					mPollFds[i].revents = 0;
				} else if (i == dmpOrient) {
					nb = ((MPLSensor*) mSensor)->readDmpOrientEvents(data, count);
					mPollFds[dmpOrient].revents = 0;
					if (isDmpScreenAutoRotationEnabled() && nb > 0) {
						count -= nb;
						nbEvents += nb;
						data += nb;
					}
				} else if (i == dmpSign) {
					LOGI_IF(0, "HAL: dmpSign interrupt");
					nb = ((MPLSensor*) mSensor)->readDmpSignificantMotionEvents(data, count);
					mPollFds[i].revents = 0;
					count -= nb;
					nbEvents += nb;
					data += nb;
				}
#endif
			}
		}
		nb = ((MPLSensor*) mSensor)->readEvents(data, count);
		LOGI_IF(0, "sensors_mpl:readEvents() - nb=%d, count=%d, nbEvents=%d, data->timestamp=%lld, data->data[0]=%f,",
		        nb, count, nbEvents, data->timestamp, data->data[0]);//mtcai,default 0
#if SENSOR_KEEP_ALIVE
		for (i = 0; i < nb; i++) {
			if (data[i].sensor >= 0 && sensor_activate[data[i].sensor] > 0) {
//              LOGD("Skip sensor data, %d, %d", data[i].sensor, sensor_activate[data[i].sensor]);
				--sensor_activate[data[i].sensor];
				memset(data + i, 0, sizeof(sensors_event_t));
				data[i].sensor = -1;
			}
		}
#endif
		//LOGI_IF(1,"data[i].sensor :%d, data[i].sensor :%d\n", data[0].sensor, data[1].sensor);
		int gyro_i = -1;
		int accel_i = -1;
		for (i = 0; i < nb; i++) {
			if (data[i].sensor == SENSORS_GYROSCOPE_HANDLE)
				gyro_i = i;
			if (data[i].sensor == SENSORS_ACCELERATION_HANDLE)
				accel_i = i;
		}
		if (gyro_i >= 0 && accel_i >= 0) {
			//rkvr_add_sensor_data2(data[accel_i].acceleration.v, data[gyro_i].gyro.v, data[gyro_i].timestamp);
		}
		//LOGI_IF(1,"debug_lvl :%d\n", debug_lvl);
		debug_lvl = 0;
		if (debug_lvl > 0) {
			for (i = 0; i < nb; i++) {
				if ((debug_lvl & 1) && data[i].sensor == SENSORS_RAW_GYROSCOPE_HANDLE) {
					float gyro_data[3] = {0, 0, 0};
					gyro_data[0] = data[i].uncalibrated_gyro.uncalib[0] - data[i].uncalibrated_gyro.bias[0];
					gyro_data[1] = data[i].uncalibrated_gyro.uncalib[1] - data[i].uncalibrated_gyro.bias[1];
					gyro_data[2] = data[i].uncalibrated_gyro.uncalib[2] - data[i].uncalibrated_gyro.bias[2];
					if (debug_lvl & 8) {
						LOGD("RAW GYRO: %+f %+f %+f, %+f %+f %+f, %+f %+f %+f - %lld",
						     gyro_data[0], gyro_data[1], gyro_data[2],
						     data[i].uncalibrated_gyro.uncalib[0], data[i].uncalibrated_gyro.uncalib[1], data[i].uncalibrated_gyro.uncalib[2],
						     data[i].uncalibrated_gyro.bias[0], data[i].uncalibrated_gyro.bias[1], data[i].uncalibrated_gyro.bias[2], data[i].timestamp);
					} else {
						LOGD("RAW GYRO: %+f %+f %+f - %lld", gyro_data[0], gyro_data[1], gyro_data[2], data[i].timestamp);
					}
				}
				if ((debug_lvl & 1) && data[i].sensor == SENSORS_GYROSCOPE_HANDLE) {
					// LOGI_IF(1, "GYRO: %+f %+f %+f - %lld", data[i].gyro.v[0], data[i].gyro.v[1], data[i].gyro.v[2], data[i].timestamp);
				}
				if ((debug_lvl & 2) && data[i].sensor == SENSORS_ACCELERATION_HANDLE) {
					//  LOGI_IF(1, "ACCL: %+f %+f %+f - %lld", data[i].acceleration.v[0], data[i].acceleration.v[1], data[i].acceleration.v[2], data[i].timestamp);
					//Get_Sensors_data(float accel_x, float accel_y, float accel_z);
				}
				if ((debug_lvl & 4) && (data[i].sensor == SENSORS_MAGNETIC_FIELD_HANDLE)) {
					LOGD("MAG: %+f %+f %+f - %lld", data[i].magnetic.v[0], data[i].magnetic.v[1], data[i].magnetic.v[2], data[i].timestamp);
				}
			}
		}

		if (nb > 0) {
			if (debug_time) {
				int64_t tm_cur = get_time_ns();
				int64_t tm_delta = tm_cur - data->timestamp;
				if (tm_min == 0 && tm_max == 0)
					tm_min = tm_max = tm_delta;
				else if (tm_delta < tm_min)
					tm_min = tm_delta;
				else if (tm_delta > tm_max)
					tm_max = tm_delta;
				tm_sum += tm_delta;
				tm_count++;

				if ((tm_cur - tm_last_print) > 1000000000) {
					LOGD("MPU HAL report rate[%4lld]: %8lld, %8lld, %8lld\n", tm_count, tm_min, (tm_sum / tm_count), tm_max);
					tm_last_print = tm_cur;
					tm_min = tm_max = tm_count = tm_sum = 0;
				}
			}
			count -= nb;
			nbEvents += nb;
			data += nb;
		}
	}


	return nbEvents;
}

int sensors_poll_context_t::batch(int handle, int flags, int64_t period_ns, int64_t timeout)
{
	FUNC_LOG;
	return mSensor->batch(handle, flags, period_ns, timeout);
}

void* Sensor_poll_thread(void* parg)
{
	g_sensor_exit = 1;
	sensors_event_t data[64];
	int count = sizeof(data);
	int i = 0;
	struct imu_data imudata;
	int gyro_ready = 0;
	int acl_ready = 0;
	int64_t tm_cur = 0;
	int64_t tm_lasttimes = 0;
	int videomode;
	int (*imu_proc)(struct imu_data * data);

	while (g_sensor_exit) {
		int nb = g_ctx->pollEvents(data, count);
		for (i = 0; i < nb; i++) {
			if (data[i].sensor == SENSORS_GYROSCOPE_HANDLE) {
				imudata.gyro_x    = data[i].gyro.v[0];
				imudata.gyro_y    = data[i].gyro.v[1];
				imudata.gyro_z 	 = data[i].gyro.v[2];
				imudata.timestamp = (long)(data[i].timestamp);
				gyro_ready = 1;
			}

			if (data[i].sensor == SENSORS_ACCELERATION_HANDLE) {
				imudata.acl_x     = data[i].acceleration.v[0];
				imudata.acl_y     = data[i].acceleration.v[1];
				imudata.acl_z 	  = data[i].acceleration.v[2];
				imudata.timestamp = (long)(data[i].timestamp);
				acl_ready = 1;
			}
		}
		if (gyro_ready == 1 && acl_ready == 1) {
			gyro_ready = 0;
			acl_ready = 0;

			if (is_uvcstreamon() == TRUE){
				imu_function_lock();
				if	(g_fpimu_cb != NULL){
					g_fpimu_cb(&imudata);
					}
				imu_function_unlock();
			}
		}
	}

	return NULL;
}

int imu_init(void)
{
	g_ctx = new sensors_poll_context_t();
	g_ctx->activate(SENSORS_GYROSCOPE_HANDLE, 1);
	g_ctx->setDelay(SENSORS_GYROSCOPE_HANDLE, 5000000);//200HZ
	g_ctx->activate(SENSORS_ACCELERATION_HANDLE, 1);
	g_ctx->setDelay(SENSORS_ACCELERATION_HANDLE, 5000000);
	pthread_create(&Sensor_thread_id, NULL, Sensor_poll_thread, NULL);
	return 0;
}
void imu_uninit(void)
{
	g_sensor_exit = 0;
	pthread_join(Sensor_thread_id, NULL);
	g_ctx->activate(SENSORS_GYROSCOPE_HANDLE, 0);
	g_ctx->activate(SENSORS_ACCELERATION_HANDLE, 0);
	delete g_ctx;
}
