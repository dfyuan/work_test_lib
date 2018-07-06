#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <asm/types.h>
#include <pthread.h>
#include <time.h>
#include <utils/Log.h>

#include "typedef.h"
#include "uvc_ctrl.h"
#include "uvc_stream.h"
#include "libsfifo/sfifo.h"
#include "log_tag.h"
#define UVC_AVC_DEVICE_NAME			"/dev/g_uvc_mjpeg"
#define UVC_AVC_NUMBUFS				6
//#define UVC_AVC_BUFSIZE				(2*640*480)
#define UVC_AVC_BUFSIZE				(2*1280*720)

#define CLEAR(x)					memset (&(x), 0, sizeof (x))
#define MAX_BUFFERS 				128


BOOL bStreamOn = FALSE;
BOOL b_streamoff_flag = FALSE;
BOOL b_diskmode_flag  = FALSE;
static EZY_BufHndl *videobufferhndl = NULL;
static pthread_t videoth_tid;
static int g_devicemode = SENSOR_MODE0;
static pthread_rwlock_t  hvideomodemutex;
static BOOL ums_mode = FALSE;


extern struct sfifo_des_s *rvideo_sfifo_handle;
extern struct sfifo_des_s *lvideo_sfifo_handle;
extern video_cb g_fpvideo_cb;

static void buffer_uninit(EZY_BufHndl *hndl);
static int buffer_streamon(EZY_BufHndl *hndl);
static void buffer_streamoff(EZY_BufHndl *hndl);
static int uvcstream_buffer_init(void);
static void uvcstream_buffer_uninit(void);
static int buffer_init(EZY_BufHndl *hndl, const char *name, int numBufs, int bufSize, enum EzyMemory memory, int qflag, int nonBlock);



static long get_tick_count(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}
static void test_video0_fps(void)
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
		ALOGD("Right VIDEO Frame Rate = %f\n", cur_framerate);
	}
}

static void test_video1_fps(void)
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
		ALOGD("Left VIDEO Frame Rate = %f\n", cur_framerate);
	}
}


static int buffer_init(EZY_BufHndl *hndl, const char *name, int numBufs, int bufSize, enum EzyMemory memory, int qflag, int nonBlock)
{
	struct ezy_buffer bufd;
	struct ezy_reqbufs reqOutBufs;
	int i;

	if (numBufs > MAX_BUFFERS)
		return -1;

	if (!name || !hndl)
		return -1;

	memset(hndl, 0, sizeof(*hndl));

	if (hndl->fd > 0)
		close(hndl->fd);

	hndl->fd = open(name, (nonBlock == EZY_BUF_NBLOCK) ? (O_RDWR | O_NONBLOCK) : O_RDWR);
	if (hndl->fd < 0) {
		ALOGD("Gedget device open  failed\n");
		goto done;
	}

	memset(&reqOutBufs, 0, sizeof(reqOutBufs));

	reqOutBufs.memory = (enum ezy_memory)memory;
	reqOutBufs.size = bufSize;
	reqOutBufs.count = numBufs;
	if (ioctl(hndl->fd, EZY_REQBUF, &reqOutBufs) == -1) {
		ALOGD("buffer allocation error\n");
		goto done;
	}

	if (hndl->ezyBufs && hndl->numBufs != 0)
		buffer_uninit(hndl);

	hndl->ezyBufs = (EzyBuf **)calloc(sizeof(*hndl->ezyBufs), numBufs);
	hndl->numBufs = numBufs;
	for (i = 0; i < numBufs; i++) {
		CLEAR(bufd);
		bufd.index = i;
		if (ioctl(hndl->fd, EZY_QUERYBUF, &bufd) == -1) {
			ALOGD(" query buffer failed \n");
			goto done;
		}

		hndl->ezyBufs[i] = (EzyBuf *)calloc(sizeof(*hndl->ezyBufs[i]), 1);
		if (memory == MEMORY_MMAP) {
			hndl->ezyBufs[i]->start = (char *) mmap(0, bufSize, PROT_READ | PROT_WRITE, MAP_SHARED, hndl->fd, bufd.phyaddr);
			if (hndl->ezyBufs[i]->start == MAP_FAILED) {
				ALOGD("error in mmaping output buffer\n");
				goto done;
			}
		}

		hndl->ezyBufs[i]->index = i;
		hndl->ezyBufs[i]->phyaddr = bufd.phyaddr;
		hndl->ezyBufs[i]->size = bufd.size;
		hndl->ezyBufs[i]->headlen = bufd.headlen;
	}
	return 0;
done:
	ALOGD("buffer_init: ERR done\n");
	buffer_uninit(hndl);

	return -1;
}

static void buffer_uninit(EZY_BufHndl *hndl)
{
	int i = 0;

	if (hndl->ezyBufs) {
		for (i = 0; i < hndl->numBufs; i++) {
			if (hndl->ezyBufs[i]->start) {
				munmap(hndl->ezyBufs[i]->start, hndl->ezyBufs[i]->size);
			}
			free(hndl->ezyBufs[i]);
			hndl->ezyBufs[i] = NULL;
		}
		free(hndl->ezyBufs);
		hndl->ezyBufs = NULL;
		hndl->numBufs = 0;
	}
}
static int buffer_streamon(EZY_BufHndl *hndl)
{
	struct ezy_buffer bufd;
	int j;

	for (j = 0; j < hndl->numBufs; j++)
	{
		CLEAR(bufd);
		bufd.index = j;
		bufd.bytesused = 0;
		if (ioctl(hndl->fd, EZY_QBUF, &bufd) == -1) {
			ALOGD("Fisrt Q buffer failed \n");
		}
	}
	
	CLEAR(bufd);
	if (ioctl(hndl->fd, EZY_STREAMON, &bufd) == -1) {
		ALOGD(" EZY_STREAMON failed \n");
		return -1;
	}

	return 0;
}

static void buffer_streamoff(EZY_BufHndl *hndl)
{
	struct ezy_buffer bufd;

	CLEAR(bufd);

	if (ioctl(hndl->fd, EZY_STREAMOFF, &bufd) == -1) {
		ALOGD(" EZY_STREAMOFF failed \n");
	}

	return;
}

int buffer_get_free(EZY_BufHndl *hndl, struct EzyBuf *eb)
{
	int ret;
	struct ezy_buffer bufd;

	CLEAR(bufd);
	if ((ret = ioctl(hndl->fd, EZY_DQBUF, &bufd)) < 0) {
		ALOGD("get an empty buffer failed! \n");
		return ret;
	}

	*eb = *hndl->ezyBufs[bufd.index];
	return 0;
}

int buffer_put_full(EZY_BufHndl *hndl, struct EzyBuf *eb)
{
	struct ezy_buffer bufd;

	CLEAR(bufd);

	bufd.size	= eb->size;
	bufd.bytesused = eb->bytesused;
	bufd.phyaddr = eb->phyaddr;
	bufd.index = eb->index;
	if (ioctl(hndl->fd, EZY_QBUF, &bufd) < 0) {
		ALOGD(" Q buffer failed \n");
		return -1;
	}

	return 0;
}

static int uvcstream_buffer_init(void)
{
	int ret;

	videobufferhndl = (EZY_BufHndl *)malloc(sizeof(EZY_BufHndl));
	if (videobufferhndl == NULL)
		return -1;
	//change block to nblock for change device mode
	if (buffer_init(videobufferhndl, UVC_AVC_DEVICE_NAME, UVC_AVC_NUMBUFS, UVC_AVC_BUFSIZE, MEMORY_MMAP, EZY_BUF_Q, EZY_BUF_NBLOCK) < 0) {
		ALOGD("InitGedgetDevice err\n");
		return -1;
	}

	ret = pthread_rwlock_init(&hvideomodemutex, NULL);
	if (ret) {
		ALOGD("Cannot init the thread mutex\n");
	}

	return 0;
}

static void uvcstream_buffer_uninit(void)
{
	buffer_uninit(videobufferhndl);
	pthread_rwlock_destroy(&hvideomodemutex);
}

unsigned int write_video_buffer(unsigned char* pBuffer, unsigned int nBufferLen)
{
	struct EzyBuf eBuf;
	unsigned int read_len;

	if (videobufferhndl == NULL || pBuffer == NULL || nBufferLen == 0)
		return -1;

	if (buffer_get_free(videobufferhndl, &eBuf) < 0) {
		return -1;
	}

	memcpy((unsigned char*)eBuf.start + eBuf.headlen, pBuffer, nBufferLen);
	eBuf.bytesused = nBufferLen;

	if (buffer_put_full(videobufferhndl, &eBuf) < 0) {
		ALOGD("Put GadgetBuf fail\n");
		return -1;
	}

	return 0;
}

int uvcstream_on(void)
{
	if(bStreamOn == TRUE)
		return 0;

	buffer_streamon(videobufferhndl);
	sfifo_reset(lvideo_sfifo_handle);
	sfifo_reset(rvideo_sfifo_handle);
	bStreamOn = TRUE;
	
	return 0;
}

int uvcstream_off(void)
{
	if(bStreamOn == FALSE)
		return 0;

	bStreamOn = FALSE;
	do{
		usleep(100);
	}
	while(!b_streamoff_flag);
	buffer_streamoff(videobufferhndl);
	return 0;
}

bool is_uvcstreamon(void)
{
	return bStreamOn;
}

int set_device_mode(int mode)
{
	int ret;
	ret = pthread_rwlock_wrlock(&hvideomodemutex);
	if (ret) {
		ALOGD("Cannot lock the thread mutex\n");
	}

	g_devicemode = mode;
	ret = pthread_rwlock_unlock(&hvideomodemutex);
	if (ret) {
		ALOGD("Cannot unlock the thread mutex\n");
	}
	return 0;
}
int get_device_mode(void)
{
	int ret;
	int mode;
	ret = pthread_rwlock_tryrdlock(&hvideomodemutex);
	if (ret) {
		ALOGD("Cannot lock the thread mutex\n");
	}

	mode = g_devicemode;

	ret = pthread_rwlock_unlock(&hvideomodemutex);
	if (ret) {
		ALOGD("Cannot unlock the thread mutex\n");
	}
	return mode;
}

static void frame_sync()
{
	struct sfifo_s *sfifo;

	long int l_frame_t;
	long int r_frame_t;

	sfifo = sfifo_get_active_buf(lvideo_sfifo_handle);
	l_frame_t = ((S_MetaData*)(sfifo->buffer))->tv_sec * 1000 * 1000 + ((S_MetaData*)(sfifo->buffer))->tv_usec;
	sfifo_put_free_buf(sfifo, lvideo_sfifo_handle);

	sfifo = sfifo_get_active_buf(rvideo_sfifo_handle);
	r_frame_t = ((S_MetaData*)(sfifo->buffer))->tv_sec * 1000 * 1000 + ((S_MetaData*)(sfifo->buffer))->tv_usec;
	sfifo_put_free_buf(sfifo, rvideo_sfifo_handle);

	printf("l_time = %ld\n", l_frame_t);
	printf("r_time = %ld\n", r_frame_t);

	while (r_frame_t - l_frame_t > 500) {
		sfifo = sfifo_get_active_buf(lvideo_sfifo_handle);
		l_frame_t = ((S_MetaData*)(sfifo->buffer))->tv_sec * 1000 * 1000 + ((S_MetaData*)(sfifo->buffer))->tv_usec;
		sfifo_put_free_buf(sfifo, lvideo_sfifo_handle);
	}

	while (l_frame_t - r_frame_t > 500) {
		sfifo = sfifo_get_active_buf(rvideo_sfifo_handle);
		r_frame_t = ((S_MetaData*)(sfifo->buffer))->tv_sec * 1000 * 1000 + ((S_MetaData*)(sfifo->buffer))->tv_usec;
		sfifo_put_free_buf(sfifo, rvideo_sfifo_handle);
	}

	return;
}

static void* video_stream_thread(void* param)
{
	static long long loop = 0;
	struct sfifo_s *l_sfifo, *r_sfifo;
	void* arg = param;
	int retval = -1;
	int need_sync = 1;
	long int time_s;

	while (1) {

		if (get_device_mode() == DISK_MODE) {
			ALOGD("device switch to disk mode \n");
			b_diskmode_flag= TRUE;
			break;
		}

		if (!bStreamOn) {
			b_streamoff_flag=TRUE;
			usleep(10000);
			need_sync = 1;
			continue;
		} else if (bStreamOn && need_sync == 1) {
			frame_sync();
			need_sync = 0;
		}

		l_sfifo = sfifo_get_active_buf(lvideo_sfifo_handle);
		r_sfifo = sfifo_get_active_buf(rvideo_sfifo_handle);

		if (l_sfifo != NULL  && r_sfifo != NULL) {
#if 0
			time_s = ((S_MetaData*)(l_sfifo->buffer))->tv_sec * 1000 * 1000 + ((S_MetaData*)(l_sfifo->buffer))->tv_usec;
			printf("l = %ld\n", time_s);
			time_s = ((S_MetaData*)(r_sfifo->buffer))->tv_sec * 1000 * 1000 + ((S_MetaData*)(r_sfifo->buffer))->tv_usec;
			printf("r = %ld\n\n\n", time_s);
#endif
			video_function_lock();
			if (g_fpvideo_cb != NULL){
				g_fpvideo_cb(l_sfifo->buffer, IMAGE_SIZE, r_sfifo->buffer, IMAGE_SIZE);
			}
			video_function_unlock();
			sfifo_put_free_buf(l_sfifo, lvideo_sfifo_handle);
			sfifo_put_free_buf(r_sfifo, rvideo_sfifo_handle);
		} else {
			sfifo_put_free_buf(l_sfifo, lvideo_sfifo_handle);
			sfifo_put_free_buf(r_sfifo, rvideo_sfifo_handle);
		}

	}

	return NULL;
}

int uvcstream_init(void)
{
	int ret = -1;

	ret = uvcstream_buffer_init();
	if (ret != 0) {
		ALOGD("uvcstream buffer init failed!\n");
		return -1;
	}
	ret = pthread_create(&videoth_tid, NULL, video_stream_thread, NULL);
	if (ret != 0) {
		ALOGD("Create video stream thread failed!\n");
	}

	return ret;
}

void uvcstream_uninit(void)
{
	pthread_join(videoth_tid, NULL);
	uvcstream_buffer_uninit();
}
