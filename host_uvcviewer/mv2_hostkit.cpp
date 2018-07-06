#include"mv2_hostkit.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>

#include<getopt.h>             /* getopt_long() */

#include<fcntl.h>              /* low-level i/o */
#include<unistd.h>
#include<errno.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/time.h>
#include<sys/mman.h>
#include<sys/ioctl.h>
#include<QKeyEvent>
#include<QKeySequence>
#include<opencv2/highgui/highgui.hpp>
#include<opencv/cv.h>
#include<linux/uvcvideo.h>
#include"video.h"
#include<linux/videodev2.h>
#include<QDateTime>
#include"mipi_tx_header.h"
#include<QFileDialog>
#include<QMessageBox>
#include<QString>
#include<QtConcurrent/QtConcurrent>


#define IMU_DATA_SIZE 56
#define CLEAR(x) memset(&(x), 0, sizeof(x)) // set x 0

// Colors
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define NOSHOW 0
#include <time.h>

typedef unsigned char BYTE;
typedef unsigned long       DWORD;
typedef struct tagPOINT
{
    int x;
    int y;
}POINT;

#define min(x,y)  ( x>y?y:x )
#define WIDTHBYTES(bits)					((DWORD)(((bits) + 31) & (~31)) / 8)
#define CLIP(x)				static_cast<BYTE>((x) > 255 ? 255 : ((x) < 0 ? 0 : (x)))
#define GRAY(r, g, b)	static_cast<BYTE>(0.299 * (r) + 0.587 * (g) + 0.114 * (b));

typedef enum
{
    BayerB,
    BayerG,
    BayerR
}BayerColor;

typedef enum
{
    BAYER_DEF = 0,
    BAYER_GBRG = BAYER_DEF,
    BAYER_RGGB = 1,
    BAYER_BGGR = 2,
    BAYER_GRBG = 3,
    YUV422 =4,
    YUV420_I420=5,
    Mono = 6,
    YV12  = 7,
    NV21 = 8,
} eBayer;

typedef struct imu_data {

float accelX;

float accelY;

float accelZ;

float gyroX;

float gyroY;

float gyroZ;

long int timestampus;

} imu_data_t;

int cvt_conversion_type;
int frame_lenght_q;
CvVideoWriter* video= NULL;

typedef enum
{

    B					= 0,
    G					= 1,
    R					= 2,
    Y					= 3,
    ALL				= 4
} eColorChannel;

typedef struct
{
    eBayer eBayerPattern;
    int nW, nH;

} PLContext;

enum io_method {
        IO_METHOD_READ,
        IO_METHOD_MMAP,
        IO_METHOD_USERPTR,
};

typedef struct tagVideoRes
{
    int  width;
    int  height;
    unsigned int format;
}VideoRes;

typedef struct tagVideoResList
{
	VideoRes *video_res;
	int num_res;
}VideoResList;


static char       *dev_name="/dev/v4l/by-id/usb-Ningbo_Sunny_opto_Co._Ltd_Sunny_HD_WebCam-V3.1.0_Sunny_HD_WebCam-V3.1.0-video-index0";

static enum io_method   io = IO_METHOD_MMAP;
static int              fd = -1;
struct buffer {
        void   *start;
        size_t  length;
};
struct buffer          *buffers;
static unsigned int     n_buffers;
static int              out_buf;
static int              frame_count = 9999999;
static unsigned int     frames_captured = 0;
static int init_flag = 0;
static int save_yuv_l = 0;
static int save_yuv_r = 0;
static int imu_cnt = 0;
static int current_mode = 0;
static int quit_flag = 0;
static int imu_enable_flag = 0;
static int video_enable_flag = 0;

static char file_log[60];
QString  illu= QString("D50") ;
struct v4l2_buffer buf;
int scross=0;
client_tx_frame_header_t* frame_header;
QPointF mouseclick=QPointF(0,0);
int igain ,iexptime ;
int rightcount=0;
int filenameindex=1;
BYTE camconbination=0;

QPixmap *pixmapa,*pixmapc;

void* buffercama=malloc(6666*5555*2);
void* buffercamc=malloc(6666*5555*2);
void* bufferimg=malloc(6666*5555*2);
double bufferindex = 2;

client_tx_frame_header_t* frameheadera,*frameheaderc;

IplImage* incoming_img =cvCreateImage( cvSize(6666,5555),IPL_DEPTH_8U,2);
IplImage* fullsize_outputa =cvCreateImage( cvSize(6666,5555),IPL_DEPTH_8U,4);
IplImage* fullsize_outputc =cvCreateImage( cvSize(6666,5555),IPL_DEPTH_8U,4);

CvVideoWriter* videoa =NULL;
CvVideoWriter* videoc=NULL;

QPointF positiona=QPointF(0,0);
QPointF positionc=QPointF(0,0);

static int frames_num = 0;
static long start_time = 1;
static __int64_t total_frame_size = 0;

#define MAX_SUPPORT_RES 64
static VideoRes 	s_video_res[MAX_SUPPORT_RES];
static VideoResList svideo_res_list;
static int emun_resolution(void);
static int deivce_mode=SENSOR_MODE0;

static const BayerColor g_BayerRGB[4][2][2] =
{
    BayerG, // 0 0 0
    BayerB, // 0 0 1
    BayerR, // 0 1 0
    BayerG, // 0 1 1
    BayerR, // 1 0 0
    BayerG, // 1 0 1
    BayerG, // 1 1 0
    BayerB, // 1 1 1
    BayerB, // 2 0 0
    BayerG, // 2 0 1
    BayerG, // 2 1 0
    BayerR, // 2 1 1
    BayerG, // 3 0 0
    BayerB, // 3 1 0
    BayerR, // 3 0 1
    BayerG  // 3 1 1
};

unsigned long get_tick_count()
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

QImage Yuvtoqimg(BYTE* frame, int w, int h,int channel)
{
    switch (channel)
	{
		case 0:
		{
		    incoming_img->imageData = (char*)frame;
			
		    cvCvtColor(incoming_img,fullsize_outputa, cvt_conversion_type);
		    uchar* imgData = (uchar*)(fullsize_outputa->imageData);
		    QImage *qImg=new QImage( (uchar*)imgData,w,h,QImage::Format_RGB32);
		    return *qImg;
		    break;
		}
		
		case 2:
		{
		    incoming_img->imageData = (char*)frame;
		    cvCvtColor(incoming_img,fullsize_outputc, cvt_conversion_type);
		    uchar* imgData = (uchar*)(fullsize_outputc->imageData);
		    QImage *qImg=new QImage( (uchar*)imgData,w,h,QImage::Format_RGB32);
		    return *qImg;
		    break;
		}
		
		default:
		    break;
    }
}
void BorderInterpolate(PLContext* pCtx, BYTE* pRaw, BYTE* pBmp24, const int nW, const int nH, const int nBorder)
{
    if ((nBorder < 1) || (nBorder > min(nW, nH) / 2))
        return;

    BYTE* raw = NULL;
    BYTE* dib = NULL;
    for (long y = 0; y < nH; ++y)
    {
        raw = pRaw + y * nW;
        dib = pBmp24 + y * WIDTHBYTES(nW * 24);
        for (long x = 0; x < nW; ++x)
        {
            if ((x == nBorder) && (y >= nBorder) && (y < nH - nBorder))
            {
                x = nW - nBorder;
                raw += (nW - 2 * nBorder);
                dib += (nW - 2 * nBorder) * 3;
            }
            long sum[3];
            memset(sum,0 ,sizeof(sum));
            long cnt[3];
            memset(cnt, 0,sizeof(cnt));
            BayerColor color = g_BayerRGB[pCtx->eBayerPattern][x & 0x01][y & 0x01];
            for (long yoffset = -1; yoffset <= 1; ++yoffset)
            {
                for (long xoffset = -1; xoffset <= 1; ++xoffset)
                {
                    if ((yoffset == 0) && (xoffset == 0))
                    {
                        continue;
                    }
                    long y1 = y + yoffset;
                    long x1 = x + xoffset;
                    if ((y1 >= 0) && (y1 < nH) && (x1 >= 0) && (x1 < nW))
                    {
                        BayerColor c = g_BayerRGB[pCtx->eBayerPattern][x1 & 0x01][y1 & 0x01];
                        if (c == color)
                        {
                            continue;
                        }
                        sum[c] += raw[yoffset * nW + xoffset];
                        ++cnt[c];
                    }
                }
            }
            for (int c = R; c >= B; --c)
            {
                if (c == color)
                {
                    dib[c] = *raw;
                }
                else
                {
                    dib[c] = CLIP(1.0 * sum[c] / cnt[c]);
                }
            }
            ++raw;
            dib += 3;
        }
    }
}

void DemosaicBilinear(PLContext* pCtx, BYTE* pRaw, BYTE* pBmp24)
{
    const int nW = pCtx->nW;
    const int nH = pCtx->nH;
    memset(pBmp24,0 ,WIDTHBYTES(nW * 24) * nH);
    BorderInterpolate(pCtx, pRaw, pBmp24, nW, nH, 2);
    POINT ptROrg;
    POINT ptG1Org;
    POINT ptG2Org;
    POINT ptBOrg;
    int nG1B = 0; // B @ G1
    int nG1R = 0; // R @ G1
    int nG2B = 0; // B @ G2
    int nG2R = 0; // R @ G2
    switch (pCtx->eBayerPattern)
    {
    case BAYER_GBRG:
        {
            ptROrg.x = 1;
            ptROrg.y = 0;
            ptG1Org.x = 0;
            ptG1Org.y = 0;
            ptG2Org.x = 1;
            ptG2Org.y = 1;
            ptBOrg.x = 0;
            ptBOrg.y = 1;
            nG1B = nW;
            nG1R = 1;
            nG2B = 1;
            nG2R = nW;
            break;
        }
    case BAYER_RGGB:
        {
            ptROrg.x = 0;
            ptROrg.y = 0;
            ptG1Org.x = 0;
            ptG1Org.y = 1;
            ptG2Org.x = 1;
            ptG2Org.y = 0;
            ptBOrg.x = 1;
            ptBOrg.y = 1;
            nG1B = 1;
            nG1R = nW;
            nG2B = nW;
            nG2R = 1;
            break;
        }
    case BAYER_BGGR:
        {
            ptROrg.x = 1;
            ptROrg.y = 1;
            ptG1Org.x = 0;
            ptG1Org.y = 1;
            ptG2Org.x = 1;
            ptG2Org.y = 0;
            ptBOrg.x = 0;
            ptBOrg.y = 0;
            nG1B = nW;
            nG1R = 1;
            nG2B = 1;
            nG2R = nW;
            break;
        }
    case BAYER_GRBG:
        {
            ptROrg.x = 0;
            ptROrg.y = 1;
            ptG1Org.x = 0;
            ptG1Org.y = 0;
            ptG2Org.x = 1;
            ptG2Org.y = 1;
            ptBOrg.x = 1;
            ptBOrg.y = 0;
            nG1B = 1;
            nG1R = nW;
            nG2B = nW;
            nG2R = 1;
            break;
        }
    default:
        {
            assert(false);
            break;
        }
    }
    const int nLine = WIDTHBYTES(nW * 24);
    // G1
    for (int y = 2 + ptG1Org.y; y < nH - 2; y += 2)
    {
        int nIdxBmp = nLine * y + 3 * (2 + ptG1Org.x);
        int nIdxRaw = nW * y + 2 + ptG1Org.x;
        for (int x = 2 + ptG1Org.x; x < nW - 2; x += 2)
        {
            pBmp24[nIdxBmp + BayerG] = (pRaw[nIdxRaw] + pRaw[nIdxRaw - nW - 1]) >> 1;
            pBmp24[nIdxBmp + BayerB] = (pRaw[nIdxRaw - nG1B] + pRaw[nIdxRaw + nG1B]) >> 1;
            pBmp24[nIdxBmp + BayerR] = (pRaw[nIdxRaw - nG1R] + pRaw[nIdxRaw + nG1R]) >> 1;
            nIdxBmp += 6;
            nIdxRaw += 2;
        }
        nIdxBmp += 3 * (2 + ptG1Org.x);
        nIdxRaw += 2 + ptG1Org.x;
    }
    // G2
    for (int y = 2 + ptG2Org.y; y < nH - 2; y += 2)
    {
        int nIdxBmp = nLine * y + 3 * (2 + ptG2Org.x);
        int nIdxRaw = nW * y + 2 + ptG2Org.x;
        for (int x = 2 + ptG2Org.x; x < nW - 2; x += 2)
        {
            pBmp24[nIdxBmp + BayerG] = (pRaw[nIdxRaw] + pRaw[nIdxRaw - nW - 1]) >> 1;
            pBmp24[nIdxBmp + BayerB] = (pRaw[nIdxRaw - nG2B] + pRaw[nIdxRaw + nG2B]) >> 1;
            pBmp24[nIdxBmp + BayerR] = (pRaw[nIdxRaw - nG2R] + pRaw[nIdxRaw + nG2R]) >> 1;
            nIdxBmp += 6;
            nIdxRaw  += 2;
        }
        nIdxBmp += 3 * (2 + ptG2Org.x);
        nIdxRaw += 2 + ptG2Org.x;
    }
    // B
    for (int y = 2 + ptBOrg.y; y < nH - 2; y += 2)
    {
        int nIdxBmp = nLine * y + 3 * (2 + ptBOrg.x);
        int nIdxRaw = nW * y + 2 + ptBOrg.x;
        for (int x = 2 + ptBOrg.x; x < nW - 2; x += 2)
        {
            pBmp24[nIdxBmp + BayerB] = pRaw[nIdxRaw];
            pBmp24[nIdxBmp + BayerG] = (pRaw[nIdxRaw - nW] + pRaw[nIdxRaw + 1] + pRaw[nIdxRaw + nW] + pRaw[nIdxRaw - 1]) >> 2;
            pBmp24[nIdxBmp + BayerR] = (pRaw[nIdxRaw - nW - 1] + pRaw[nIdxRaw - nW + 1] + pRaw[nIdxRaw + nW - 1] + pRaw[nIdxRaw + nW + 1]) >> 2;
            nIdxBmp += 6;
            nIdxRaw += 2;
        }
        nIdxBmp += 3 * (2 + ptBOrg.x);
        nIdxRaw += 2 + ptBOrg.x;
    }
    // R
    for (int y = 2 + ptROrg.y; y < nH - 2; y += 2)
    {
        int nIdxBmp = nLine * y + 3 * (2 + ptROrg.x);
        int nIdxRaw = nW * y + 2 + ptROrg.x;
        for (int x = 2 + ptROrg.x; x < nW - 2; x += 2)
        {
            int nIdxBmp = nLine * y + 3 * x;
            int nIdxRaw = nW * y + x;
            pBmp24[nIdxBmp + BayerB] = (pRaw[nIdxRaw - nW - 1] + pRaw[nIdxRaw - nW + 1] + pRaw[nIdxRaw + nW - 1] + pRaw[nIdxRaw + nW + 1]) >> 2;
            pBmp24[nIdxBmp + BayerG] = (pRaw[nIdxRaw - nW] + pRaw[nIdxRaw + 1] + pRaw[nIdxRaw + nW] + pRaw[nIdxRaw - 1]) >> 2;
            pBmp24[nIdxBmp + BayerR] = pRaw[nIdxRaw];
            nIdxBmp += 6;
            nIdxRaw += 2;
        }
        nIdxBmp += 3 * (2 + ptROrg.x);
        nIdxRaw += 2 + ptROrg.x;
    }
}

void rawtoraw8(BYTE* frame, BYTE* raw8, int bit, int w, int h)
{
    for(int i = 0; i < h; i++)
    {
        for(int j = 0; j < w; j++)
        {
            BYTE low=*(frame+2*(i*w+ j));
            BYTE high=*(frame+2*(i*w+ j)+1);
            *(raw8+i*w+ j)=((low>>(bit-8)) | (high<<(16-bit)) ) ;
         }
    }
}

static void errno_exit(const char *s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg)
{
    int r;

    do {
            r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

int MV2_HostKit::read_frame(void)
{
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
	
    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
     	errno_exit("VIDIOC_DQBUF");
	

    cv_display(buffers[buf.index].start, buf.bytesused);


    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            errno_exit("VIDIOC_QBUF");
    return 1;
}
static void stop_capturing(void)
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
            errno_exit("VIDIOC_STREAMOFF");
}

static void start_capturing(void)
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
		errno_exit("VIDIOC_STREAMON");
}
static void open_device(void)
{
	struct stat st;

	if (-1 == stat(dev_name, &st)) 
	{
		fprintf(stderr, "Cannot identify '%s': %d, %s\n",dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!S_ISCHR(st.st_mode)) 
	{
		fprintf(stderr, "%s is no device\n", dev_name);
		exit(EXIT_FAILURE);
	}

	fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
	if (-1 == fd)
	{
		fprintf(stderr, "Cannot open '%s': %d, %s\n",dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}
static void close_device(void)
{
    if (-1 == close(fd))
            errno_exit("close");
    fd = -1;
}
static int emun_resolution(void)
{
	struct v4l2_fmtdesc fmt;
 	struct v4l2_frmsizeenum size;
	int ret;
	
	fmt.index=1; //NV12 format
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(fd, VIDIOC_ENUM_FMT, &fmt))
		errno_exit("VIDIOC_ENUM_FMT");

	memset (&size, 0, sizeof(struct v4l2_frmsizeenum));
	size.index = 0;
	size.pixel_format = fmt.pixelformat;
	if((xioctl (fd, VIDIOC_ENUM_FRAMESIZES, &size)<0)){
		errno_exit("can not list the frame size!\n");	
	}

	if (size.type == V4L2_FRMSIZE_TYPE_DISCRETE)
    {
        do
        {
            s_video_res[size.index].width = size.discrete.width;
            s_video_res[size.index].height = size.discrete.height;
	     	s_video_res[size.index].format = size.pixel_format;
            printf("got discrete frame size %dx%d, format: 0x%x\n",
                   size.discrete.width,
                   size.discrete.height,
                   size.pixel_format);
            size.index++;
            svideo_res_list.num_res++;
            if(size.index>=MAX_SUPPORT_RES)
            {
                break;
            }
        } while (xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &size) >= 0);
    }
    else
    {
        printf("unknown type!\n");
    }
exit:
	if(svideo_res_list.num_res == 0)
	{
	 	printf("have no right resoltion!\n");
		return -1;
	}
	return 0;
}
static void init_mmap(void)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) 
	{
        if (EINVAL == errno)
		{
            fprintf(stderr, "%s does not support ""memory mapping\n", dev_name);
            exit(EXIT_FAILURE);
        } 
		else 
        {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) 
	{
        fprintf(stderr, "Insufficient buffer memory on %s\n",dev_name);
        exit(EXIT_FAILURE);
    }

    buffers = (buffer*)calloc(req.count, sizeof(*buffers));

    if (!buffers) 
	{
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
	{
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = n_buffers;

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
        {
            errno_exit("VIDIOC_QUERYBUF");
        }

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start =mmap(NULL,buf.length,PROT_READ | PROT_WRITE,MAP_SHARED,fd, buf.m.offset);
        if (MAP_FAILED == buffers[n_buffers].start)
        {
            errno_exit("mmap");
        }
    }
}

static void prepare_buffers(void)
{
	int i ;

	for (i = 0; i < n_buffers; ++i) 
	{
		struct v4l2_buffer buf;
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");
	}
}
static int set_device_mode(int mode,struct uvc_xu_control_query xu)
{
	unsigned char cur_data[2]={0x00,0x00};
	printf("device mode=%d\n",mode);
	switch (mode){
	case SENSOR_MODE0:
	case SENSOR_MODE1:	
	case SENSOR_MODE2:
		printf("SENSOR_MODE \n");
		cur_data[0] = mode;
		xu.selector = 0x01;
		break;
		
	case DISK_MODE:
		xu.selector = 0x02;
		break;
		
	case BURNING_MODE:
		xu.selector = 0x03;
		break;
		
	default:
		printf("unkown mode ,use default mode\n");
		break;
	}
	
	memcpy(xu.data,&cur_data,2);
	int ret = ioctl(fd, UVCIOC_CTRL_QUERY, &xu);
	if (ret == -1) 
	{
		printf("V4lVideo::extention ioctl failed: %d\n", errno);
		return ret;
	}

}

static int set_format_framesize(int fmt_index,int width,int height)
{
	struct v4l2_format fmt;
	struct v4l2_fmtdesc fmtdes;
	int ret;
	
	fmtdes.index=fmt_index; //NV12 format
	fmtdes.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(fd, VIDIOC_ENUM_FMT, &fmtdes))
		errno_exit("VIDIOC_ENUM_FMT");

	memset(&fmt, 0, sizeof(struct v4l2_format));	
	fmt.type				= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width		= width; 
	fmt.fmt.pix.height		= height;
	fmt.fmt.pix.pixelformat = fmtdes.pixelformat;
	fmt.fmt.pix.field		= V4L2_FIELD_INTERLACED;
	if (-1 == xioctl (fd ,VIDIOC_S_FMT, &fmt))
	{
		printf("failed set video resolution\n");
		return	-1;
	}
}

static void set_framerate(int rate)
{
	struct v4l2_streamparm setfps;
	CLEAR(setfps);
	setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	setfps.parm.capture.timeperframe.numerator    = 1;
	setfps.parm.capture.timeperframe.denominator  = rate;
	if (-1 == xioctl(fd, VIDIOC_S_PARM, &setfps))
	       errno_exit("VIDIOC_S_PRRM");
	else
	    printf("set video frame rate ok\n");
}

static void init_device(void)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	struct v4l2_streamparm setfps;
	unsigned int min;

	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) 
	{
        if (EINVAL == errno) 
		{
            fprintf(stderr, "%s is no V4L2 device\n",dev_name);
            exit(EXIT_FAILURE);
        }
		else
		{
             errno_exit("VIDIOC_QUERYCAP");
        }
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) && !(cap.capabilities & V4L2_CAP_STREAMING)) 
	{
        fprintf(stderr, "%s is no video capture device\n",dev_name);
        exit(EXIT_FAILURE);
	}


#if 0
	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt)){
            errno_exit("VIDIOC_G_FMT");
    }
#endif
#if 0
	//add by dfyuan
	CLEAR(setfps);
	setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	setfps.parm.capture.timeperframe.numerator     = 1;
	setfps.parm.capture.timeperframe.denominator  = 120;
	if (-1 == xioctl(fd, VIDIOC_S_PARM, &setfps))
	       errno_exit("VIDIOC_S_PRRM");
	else
	    printf("dfyuan set video rate ok\n");
	//endif
#endif
#if 0
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
	        fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
	        fmt.fmt.pix.sizeimage = min;
#endif
	switch (io) 
	{
		case IO_METHOD_MMAP:
			init_mmap();
			prepare_buffers();
			break;
	    default:
		        break;
	}

}
static void uninit_device(void)
{
    unsigned int i;

    switch (io) 
	{
		case IO_METHOD_MMAP:
		    for (i = 0; i < n_buffers; ++i)
			{
		        if (-1 == munmap(buffers[i].start, buffers[i].length))
				{
					errno_exit("munmap");
		        }
		    }
		    break;
        default:
	        break;
    }

    free(buffers);
}

int MV2_HostKit::set_mode(int mode)
{
	set_device_mode(mode,xu_mode);
	if( mode == BURNING_MODE || mode == DISK_MODE){
		quit_flag=1;
		streamoff();
		close_device();
		exit(EXIT_SUCCESS);
		return 0;
	}
	
	if(mode == SENSOR_MODE0){
		set_format_framesize(1,640,481);
		set_framerate(120);
	}else if(mode == SENSOR_MODE1){
		set_format_framesize(1,1280,721);
		set_framerate(30);
	}else if(mode == SENSOR_MODE2){
	}
}

void MV2_HostKit::teardown()
{
    stop_capturing();
	imu_enable_flag   = 0;
	video_enable_flag = 0;
    uninit_device();;
    close_device();

	quit_flag=1;
    exit(EXIT_SUCCESS);
}
void MV2_HostKit::videomain()
{
    open_device();
}
void MV2_HostKit::streamoff()
{
	imu_enable_flag   = 0;
	video_enable_flag = 0;
    stop_capturing();
	uninit_device();
}
void MV2_HostKit::streamon()
{
	init_device();
    start_capturing();
	imu_enable_flag   = 1;
	video_enable_flag = 1;
	mainloop();
}

void MV2_HostKit::getpositiona(int x,int y)
{
   positiona=QPointF(x,y);
}

void MV2_HostKit::getpositionc(int x,int y)
{
   positionc=QPointF(x,y);
}
void MV2_HostKit::keypressevent(QKeyEvent *event)
{
	QString  sbayord[9]=
	{
		QString("RGGB"),
		QString("GBRG"),
		QString("BGGR"),
		QString("GRBG"),
		QString("YUV422"),
		QString("YUV420_I420"),
		QString("Mono"),
		QString("YV12"),
		QString("NV21"),
	} ;
	QString filename;
	
    if(event->key( )== Qt::Key_S)
    {
	    if((camconbination&0x01)!=0)
	    {
	        QString  sbayor=sbayord[frameheadera->bayeroder];
	        filename = QString( "stereo_img_l_%1.png" ).arg(frameheadera->timestamp1*1000+(frameheadera->timestamp2/1000));
	        filename.replace(QString("sbayord"),sbayor);
	        pixmapa->save ( filename, "PNG") ;
	        cvWaitKey(0);
    	}
	
	    if((camconbination&0x04)!=0)
	    {
	        QString  sbayor=sbayord[frameheaderc->bayeroder];
	        filename = QString( "stereo_img_r_%1.png" ).arg(frameheaderc->timestamp1*1000+(frameheaderc->timestamp2/1000));
	        filename.replace(QString("sbayord"),sbayor);
	        pixmapc->save ( filename, "PNG") ;
	        cvWaitKey(2);
	    }
		
	 	filenameindex++;
  }
}
int MV2_HostKit::testtimer()
{   
	static struct imu_data imudataold;
	static long total_lost=0;
	static long total_count=0;
	long timegap;
    while(1)
    {
        if(imu_enable_flag==1){
            usleep(2000);
            int ret = ioctl(fd, UVCIOC_CTRL_QUERY, &xu_data);
            if (ret == -1) 
			{
              // printf("MV2_HostKit::GetImuData() ioctl error: %d\n", errno);
            }
			else
            {

               struct imu_data *imudata;
               imudata = (imu_data*)xu_data.data;
			   total_count++;
			   timegap=(imudata->timestampus)/1000000 -(imudataold.timestampus)/1000000 ;
			   if( timegap > 6)
			   {
			   		total_lost++;
			   		printf("GetImuData:timegap=%d total num=%ld,lost imu number =%ld \n",timegap,total_count,total_lost);
			   }
			   memcpy(&imudataold,imudata,sizeof(struct imu_data));
               //printf("accelX=%f accelY=%f,accelZ =%f,accelX=%f accelY=%f,accelZ =%f\n",imudata->accelX,imudata->accelY,imudata->accelZ,imudata->gyroX,imudata->gyroY,imudata->gyroY);
             }
       }else{
			usleep(1000);
	   }

	  if(quit_flag ==1)
	  {
		break;
	  }
    }
}

void MV2_HostKit::closeevent(QCloseEvent *event)
{
	exit(EXIT_SUCCESS);
}

int MV2_HostKit::getimudata()
{
    QtConcurrent::run(this,&MV2_HostKit::testtimer);
    cvWaitKey(0);
    return 0;
}

void MV2_HostKit::cv_display(void* frame, int frame_lenght)
{
    int frame_size = 640*480*3/2;
    int header_size_bytes;
	static long total_framecount=0;
    static uint32_t old_rframe_count =0,new_rframe_count =0;
	
    frame_header =(client_tx_frame_header_t*) (frame);
	
	if(start_time == 1)
	{
		start_time = get_tick_count();
	}
	 
     frames_num++;
	 total_framecount++;
     if(frames_num%120 == 0)
     {
         double cur_framerate = 0;
         cur_framerate = (double)frames_num*1000/(get_tick_count()-start_time);
         start_time = get_tick_count();
         frames_num = 0;
		 printf(" Frame Rate = %f\n", cur_framerate);
     }

    //printf("channel =%ld ,timestamp1=%d  timestamp2=%d\n",frame_header->channel,frame_header->timestamp1,frame_header->timestamp2);

     if(frame_header->channel == 2)
     {
     	 old_rframe_count=new_rframe_count;
         new_rframe_count=frame_header->framecounter;
         if((new_rframe_count-old_rframe_count) !=1 )
         {
			printf("total count =%ld ,lost the frame new_rframe_count=%d  old_rframe_count=%d\n",total_framecount,new_rframe_count,old_rframe_count);
     	 }
     }
	 
    if(incoming_img->width != frame_header->width)
    {
		 if ((5==frame_header->bayeroder)||(7==frame_header->bayeroder)||(8==frame_header->bayeroder)||(9==frame_header->bayeroder))
		 {
		     incoming_img->height=frame_header->height*3/2;

		     incoming_img->width=frame_header->width;
		     incoming_img->widthStep=frame_header->width;
		     incoming_img->imageSize=frame_header->height*frame_header->width*3/2;
		     bufferindex = 1.5;
		 }
		 else
		 {
		     incoming_img->height=frame_header->height;
		     incoming_img->width=frame_header->width;
		     incoming_img->widthStep=frame_header->width*2;
		     incoming_img->imageSize=frame_header->height*frame_header->width*2;
		     bufferindex = 2;
		 }

		fullsize_outputa->height=frame_header->height;
		fullsize_outputa->width=frame_header->width;
		fullsize_outputa->widthStep=frame_header->width*4;
		fullsize_outputa->imageSize=frame_header->height*frame_header->width*4;

		fullsize_outputc->height=frame_header->height;
		fullsize_outputc->width=frame_header->width;
		fullsize_outputc->widthStep=frame_header->width*4;
		fullsize_outputc->imageSize=frame_header->height*frame_header->width*4;
        return;
    }


    if ( frame_header->headerlength<10000&&frame_header->headerlength>10)
    {
    	header_size_bytes = frame_header->headerlength;
    }
    else
    {
    	header_size_bytes = (int)(2*frame_header->width);
    }

    frame += header_size_bytes;
    frame_lenght -= header_size_bytes;
    frame_lenght_q = frame_lenght;

    QImage imga(frame_header->width,frame_header->height,QImage::Format_RGB32);
    QImage imgc(frame_header->width,frame_header->height,QImage::Format_RGB32);


    if((3==frame_header->bayeroder)|(2==frame_header->bayeroder)|(1==frame_header->bayeroder)|(0==frame_header->bayeroder))
    {
	    PLContext pCtx={eBayer(frame_header->bayeroder),frame_header->width,frame_header->height };
	    BYTE* pBmp24=(BYTE*)malloc(WIDTHBYTES(frame_header->width * 24) * frame_header->height);
	    BYTE* raw8=(BYTE*)malloc(WIDTHBYTES(frame_header->width * 8) * frame_header->height);
	    rawtoraw8((BYTE*)frame,raw8,10,frame_header->width,frame_header->height);

	    DemosaicBilinear(&pCtx, raw8, pBmp24);

        switch (frame_header->channel) 
		{
		    case 0:
		        {
		            for(int i = 0; i < frame_header->height; i++)
		            {
		                for(int j = 0; j < frame_header->width; j++)
		                {
		                   BYTE  bmp24b =* (pBmp24+3*(i*frame_header->width+ j));
		                   BYTE  bmp24g =* (pBmp24+3*(i*frame_header->width+ j)+1);
		                   BYTE  bmp24r =* (pBmp24+3*(i*frame_header->width+ j)+2);
		                   QRgb qRgb = (0xff << 24) | ((bmp24r & 0xff) << 16) | ((bmp24g & 0xff) << 8) | (bmp24b & 0xff);
		                   imga.setPixel(j,i,qRgb);
		                 }
		            }
		            break;
		        }
		    case 2:
		        {
		            for(int i = 0; i < frame_header->height; i++)
		            {
		                for(int j = 0; j < frame_header->width; j++)
		                {
		                   BYTE  bmp24b =* (pBmp24+3*(i*frame_header->width+ j));
		                   BYTE  bmp24g =* (pBmp24+3*(i*frame_header->width+ j)+1);
		                   BYTE  bmp24r =* (pBmp24+3*(i*frame_header->width+ j)+2);
		                   QRgb qRgb = (0xff << 24) | ((bmp24r & 0xff) << 16) | ((bmp24g & 0xff) << 8) | (bmp24b & 0xff);
		                   imgc.setPixel(j,i,qRgb);
		                 }
		            }
		            break;
		        }
		    default:
		        break;
        }

		free(pBmp24);
		free(raw8);
    }
	else if((4==frame_header->bayeroder)||(5==frame_header->bayeroder)||(7==frame_header->bayeroder)||(8==frame_header->bayeroder)||(9==frame_header->bayeroder))
    {
            if(4==frame_header->bayeroder)
            {
                cvt_conversion_type=CV_YUV2RGBA_YVYU;
            }
             if(5==frame_header->bayeroder)
             {
                 cvt_conversion_type= CV_YUV2BGRA_I420  ;
             }
             if(7==frame_header->bayeroder)
             {
                 cvt_conversion_type= CV_YUV2BGRA_YV12  ;
             }
             if(8==frame_header->bayeroder)
             {
                 cvt_conversion_type= CV_YUV2BGRA_NV21  ;
             }
             if(9==frame_header->bayeroder)
             {
                 cvt_conversion_type= CV_YUV2BGRA_NV12  ;
             }

			 
            switch (frame_header->channel)
			{
	            case 0:
	            {
	          		imga = Yuvtoqimg((BYTE*)frame,frame_header->width,frame_header->height,frame_header->channel);
	                break;
	            }
	            case 2:
	            {
	          		imgc = Yuvtoqimg((BYTE*)frame,frame_header->width,frame_header->height,frame_header->channel);
	                break;
	            }
	            default:
	                break;
            }

        } 
        else if(6==frame_header->bayeroder)
        {
            BYTE* raw8=(BYTE*)malloc(WIDTHBYTES(frame_header->width * 8) * frame_header->height);
            rawtoraw8((BYTE*)frame,raw8,10,frame_header->width,frame_header->height);

            BYTE* pBmp24=(BYTE*)malloc(WIDTHBYTES(frame_header->width * 24) * frame_header->height);

            switch (frame_header->channel)
			{
	            case 0:
	            {
	                for(int i = 0; i < frame_header->height; i++)
	                {
	                    for(int j = 0; j < frame_header->width; j++)
	                    {

	                        BYTE  bmp24b =* ((BYTE*)raw8+(i*frame_header->width+ j));
	                        BYTE  bmp24g =* ((BYTE*)raw8+(i*frame_header->width+ j));
	                        BYTE  bmp24r =* ((BYTE*)raw8+(i*frame_header->width+ j));
	                       QRgb qRgb = (0xff << 24) | ((bmp24r & 0xff) << 16) | ((bmp24g & 0xff) << 8) | (bmp24b & 0xff);
	                        imga.setPixel(j,i,qRgb);
	                     }
	                }
	                break;
	            }
	            case 2:
	            {
	                for(int i = 0; i < frame_header->height; i++)
	                {
	                    for(int j = 0; j < frame_header->width; j++)
	                    {
	                        BYTE  bmp24b =* ((BYTE*)raw8+(i*frame_header->width+ j));
	                        BYTE  bmp24g =* ((BYTE*)raw8+(i*frame_header->width+ j));
	                        BYTE  bmp24r =* ((BYTE*)raw8+(i*frame_header->width+ j));
	                       QRgb qRgb = (0xff << 24) | ((bmp24r & 0xff) << 16) | ((bmp24g & 0xff) << 8) | (bmp24b & 0xff);
	                        imgc.setPixel(j,i,qRgb);
	                     }
	                }
	                break;
	            }
	            default:
	                break;
            }
            free(pBmp24);
        }
        else
        {
            return;
        }

        switch (frame_header->channel) 
		{
	        case 0:
	        {
	            camconbination=camconbination|0x01;

				frameheadera=frame_header;
				
	            memcpy(buffercama, frame, frameheadera->width*frameheadera->height*bufferindex);
				
	            int rawa=*(unsigned short*)( buffercama+2*((int)positiona.y()*frameheadera->width+(int) positiona.x()));
				
	            QRgb qRgba=imga.pixel(positiona.x(),  positiona.y());
				
	            int brightnessa=((qRgba& 0x00ff0000)>>16)*0.299+((qRgba& 0x0000ff00)>>8)*0.587+(qRgba& 0x000000ff)*0.114;
				
				sendinfocama(frameheadera->width,frameheadera->height, frameheadera->bit, frameheadera->bayeroder, positiona.x(),   positiona.y(), brightnessa, rawa);
				
	            *pixmapa=QPixmap::fromImage(imga);
				
	            emit displaycama(&imga);
	            break;
	        }
			
	        case 2:
	        {
				
	            camconbination=camconbination|0x04;
				
	            frameheaderc=frame_header;
				
	            memcpy(buffercamc, frame, frameheaderc->width*frameheaderc->height*bufferindex);
				
	            int rawc=*(unsigned short*)( buffercamc+2*((int)positionc.y()*frameheaderc->width+(int) positionc.x()));
				
	            QRgb qRgbc=imgc.pixel(positionc.x(),  positionc.y());
				
	            int brightnessc=((qRgbc& 0x00ff0000)>>16)*0.299+((qRgbc& 0x0000ff00)>>8)*0.587+(qRgbc& 0x000000ff)*0.114;
				
	            sendinfocamc(frameheaderc->width,frameheaderc->height, frameheaderc->bit, frameheaderc->bayeroder, positionc.x(),   positionc.y(), brightnessc, rawc);
				
	            *pixmapc=QPixmap::fromImage(imgc);
				 
	            emit displaycamc(&imgc);
	            break;
	        }
			
	        default:
	            break;
        }

}

void MV2_HostKit::mainloop(void)
{
    while (1) 
	{
        if(video_enable_flag)
        {
	        for (;;) 
			{
                fd_set fds;
                struct timeval tv;
                int r;

			    FD_ZERO(&fds);
			    FD_SET(fd, &fds);

			    /* Timeout. */
                tv.tv_sec = 100;
			    tv.tv_usec = 0;

			    r = select(fd + 1, &fds, NULL, NULL, &tv);
			    if (-1 == r)
				{

			        if (EINTR == errno)
			                continue;
			        errno_exit("select");
			    }

                if (0 == r) 
				{
                    fprintf(stderr, "select timeout\n");
                    exit(EXIT_FAILURE);
                }

                if (read_frame())
                        break;
            }
        }
		else
        {
           break;
        }
    }
}

MV2_HostKit::MV2_HostKit(QWidget *parent):QMainWindow(parent)
{
    this->resize( QSize( 2000, 1000 ));
	
    mainguiwidget = new maingui;

    camviea = new CameraView(this);
   	camviec = new CameraView(this);
	
    QSplitter *m_splitter = new QSplitter(Qt::Vertical, this);
    QSplitter *splitter1 = new QSplitter(Qt::Horizontal);
    QSplitter *splitter4 = new QSplitter(Qt::Horizontal);

    QWidget *blank= new QWidget;

    m_splitter->setHandleWidth(1);
    splitter1->setHandleWidth(1);
    splitter4->setHandleWidth(1);

    splitter1->addWidget(camviea);
    splitter1->addWidget(camviec);


    splitter4->addWidget(mainguiwidget);
    splitter4->addWidget(blank);

	m_splitter->addWidget(splitter4);
	m_splitter->addWidget(splitter1);

    setCentralWidget(m_splitter);
    mainstatusbar = new QStatusBar;

    connect( mainguiwidget, SIGNAL(opencamsig()), this, SLOT(videomain()));
    connect( mainguiwidget, SIGNAL(closecamsig()), this, SLOT(teardown()));

    //when change video mode ,trigger change mode cmd to device
    connect(mainguiwidget, SIGNAL(streamonsig()),this,SLOT(streamon()));
	connect(mainguiwidget, SIGNAL(streamoffsig()),this,SLOT(streamoff()));
	//when start to get videostream ,trigger imu thread to get imu data
    connect(this, SIGNAL(getimusig()), this, SLOT(getimudata()));
	//set device mode
	connect(mainguiwidget, SIGNAL(sentmodeinfosig(int)), this, SLOT(set_mode(int)));
    connect(this,SIGNAL(displaycama(QImage*)),camviea,SLOT(display(QImage*)));
    connect(this,SIGNAL(displaycamc(QImage*)),camviec,SLOT(display(QImage*)));

    connect(camviea,SIGNAL(sendpostion(int,int)),this,SLOT(getpositiona(int,int)));
    connect(camviec,SIGNAL(sendpostion(int,int)),this,SLOT(getpositionc(int,int)));

	connect(this,SIGNAL(sendinfocama(int,int,int,int,int,int,int,int)),mainguiwidget,SLOT(setinfoa(int,int,int,int,int,int,int,int)));
	connect(this,SIGNAL(sendinfocamc(int,int,int,int,int,int,int,int)),mainguiwidget,SLOT(setinfoc(int,int,int,int,int,int,int,int)));

    pixmapa = new QPixmap(5000,5000);
    pixmapc = new QPixmap(5000,5000);

	//init the imu cmd 
    xu_data.unit = 0x03;
    xu_data.selector = 0x0C;
    xu_data.size = IMU_DATA_SIZE;
    xu_data.data= (__u8*)malloc(IMU_DATA_SIZE);
    xu_data.query = UVC_GET_CUR;

	xu_mode.unit = 0x03;
    xu_mode.selector = 0x01;
    xu_mode.size = 2;
    xu_mode.data= (__u8*)malloc(2);
    xu_mode.query = UVC_SET_CUR;
	 
	 //start to triger the get imu data signal
     emit getimusig();
}

MV2_HostKit::~MV2_HostKit()
{
	free(xu_data.data);
	free(xu_mode.data);
}
